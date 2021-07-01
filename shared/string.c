/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

//
// string.c
//

#include "shared.h"

/*
=============================================================================

	COLOR STRING HANDLING

=============================================================================
*/

vec4_t	colorBlack	= { 0, 0, 0, 1 };
vec4_t	colorRed	= { 1, 0, 0, 1 };
vec4_t	colorGreen	= { 0, 1, 0, 1 };
vec4_t	colorYellow	= { 1, 1, 0, 1 };
vec4_t	colorBlue	= { 0, 0, 1, 1 };
vec4_t	colorCyan	= { 0, 1, 1, 1 };
vec4_t	colorMagenta= { 1, 0, 1, 1 };
vec4_t	colorWhite	= { 1, 1, 1, 1 };

vec4_t	colorLtGrey	= { 0.75, 0.75, 0.75, 1 };
vec4_t	colorMdGrey	= { 0.5, 0.5, 0.5, 1 };
vec4_t	colorDkGrey	= { 0.25, 0.25, 0.25, 1 };

vec4_t	strColorTable[10] = {
	{ 0.0, 0.0, 0.0, 1.0 },
	{ 1.0, 0.0, 0.0, 1.0 },
	{ 0.0, 1.0, 0.0, 1.0 },
	{ 1.0, 1.0, 0.0, 1.0 },
	{ 0.0, 0.0, 1.0, 1.0 },
	{ 0.0, 1.0, 1.0, 1.0 },
	{ 1.0, 0.0, 1.0, 1.0 },
	{ 1.0, 1.0, 1.0, 1.0 },
	
	{ 0.75, 0.75, 0.75, 1.0 },
	{ 0.25, 0.25, 0.25, 1.0 }
};


/*
===============
Q_ColorCharCount
===============
*/
int Q_ColorCharCount (const char *s, int byteofs) {
	int			c;
	const char	*end;
	qBool		skipNext = qFalse;

	end = s + byteofs;

	for (c=0 ; *s && s<end ; s++, c++) {
		if (skipNext)
			skipNext = qFalse;
		else if (Q_IsColorString (s)) {
			if ((s[1] & 127) != Q_COLOR_ESCAPE)
				s++;
			else
				skipNext = qTrue;

			c--;
		}
	}

	return c;
}


/*
===============
Q_ColorCharOffset
===============
*/
int Q_ColorCharOffset (const char *s, int charcount) {
	const char	*start = s;
	qBool		skipNext = qFalse;

	for ( ; *s && charcount ; s++) {
		if (skipNext)
			skipNext = qFalse;
		else if (Q_IsColorString (s)) {
			if ((s[1] & 127) != Q_COLOR_ESCAPE)
				s++;
			else
				skipNext = qTrue;
		}
		else
			charcount--;
	}

	return s - start;
}

/*
============================================================================

	LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

/*
===============
Q_snprintfz
===============
*/
void Q_snprintfz (char *dest, size_t size, const char *fmt, ...) {
	if (size) {
		va_list		argptr;

		va_start (argptr, fmt);
		vsnprintf (dest, size, fmt, argptr);
		va_end (argptr);

		dest[size-1] = 0;
	}
}


/*
===============
Q_strcat
===============
*/
void Q_strcat (char *dst, const char *src, int dstSize) {
	int		len;

	len = strlen (dst);
	if (len >= dstSize)
		Com_Printf (0, S_COLOR_RED "Q_strcat: already overflowed");

	Q_strncpyz (dst + len, src, dstSize - len);
}


/*
===============
Q_strncpyz
===============
*/
void Q_strncpyz (char *dest, const char *src, size_t size) {
#ifdef HAVE_STRLCPY
	strlcpy (dest, src, size);
#else
	if (size) {
		while (--size && (*dest++ = *src++)) ;
		*dest = '\0';
	}
#endif
}


/*
===============
Q_strlen
===============
*/
#ifdef id386
__declspec(naked) size_t __cdecl Q_strlen (const char *s) {
    __asm {
		; ------------ version 8 ------------
		; My fast version
		; 92 bytes
		; c=17+1*n

			mov	eax, [esp+4]
			xor	ecx, ecx
			test	al, 3
			jz	loop1
			cmp	byte ptr [eax], cl
			jz	short ret0
			cmp	byte ptr [eax+1], cl
			jz	short ret1
			cmp	byte ptr [eax+2], cl
			jnz	short adjust
			inc	eax

		ret1:
			inc	eax

		ret0:
			sub	eax, [esp+4]
			ret

		adjust:
			add	eax, 3
			and	eax, 0FFFFFFFCh

		loop1:
			mov	edx, [eax]
			mov	ecx, 81010100h
			sub	ecx, edx
			add	eax, 4
			xor	ecx, edx
			and	ecx, 81010100h
			jz	loop1
			sub	eax, [esp+4]
			shr	ecx, 9
			jc	minus4
			shr	ecx, 8
			jc	minus3
			shr	ecx, 8
			jc	minus2
			dec	eax
			ret	

		minus4:
			sub	eax, 4
			ret	

		minus3:
			sub	eax, 3
			ret	

		minus2:
			sub	eax, 2
			ret	
	}
}
#endif // id386

/*
===============
Q_strlwr

Adapted from http://www.assembly-journal.com/viewarticle.php?id=131&layout=html by R1CH
===============
*/
#ifdef id386
__declspec(naked) void __cdecl Q_strlwr (char *s) {
	__asm {
			push esi						;save esi
			push edi						;save edi

			mov esi, ds:dword ptr[esp+12]	;get string
			mov edi, esi					;destination
		loop1:
			lodsb							;load byte
			cmp    al, 5Ah					;greater than 'Z'
			ja     short store2
			cmp    al, 41h					;less than 'A'
			jb     short store1
			or     al, 00100000b			;to lower (-32)
			stosb							;store
			jmp    short loop1				;loop
		store1:
			cmp    al, 0h					;end of string
			je     short finish1
		store2:
			inc		edi						;no change, increment pointer
			jmp    loop1					;loop
		finish1:
			pop edi							;restore reg
			pop esi
			ret	
	}
}
#else // id386
void Q_strlwr (char *s) {
	if (s) {
		for ( ; *s ; s++)
			*s = tolower (*s);
	}

	return NULL;
}
#endif // id386


/*
===============
Q_tolower

by R1CH
===============
*/
#ifdef id386
__declspec(naked) int __cdecl Q_tolower (int c) {
	__asm {
			mov eax, [esp+4]		;get character
			cmp	eax, 5Ah
			ja  short finish1

			cmp	eax, 41h
			jb  short finish1

			or  eax, 00100000b		;to lower (-32)
		finish1:
			ret	
	}
}
#endif // id386

// =========================================================================

/*
============
Q_WildCmp

from Q2ICE
============
*/
int Q_WildCmp (const char *filter, const char *string, int ignoreCase) {
	switch (*filter) {
	case '\0':	return !*string;
	case '*':	return Q_WildCmp (filter + 1, string, ignoreCase) || *string && Q_WildCmp (filter, string + 1, ignoreCase);
	case '?':	return *string && Q_WildCmp (filter + 1, string + 1, ignoreCase);
	default:	return ((*filter == *string) || (ignoreCase && (toupper (*filter) == toupper (*string)))) && Q_WildCmp (filter + 1, string + 1, ignoreCase);
	}
}


/*
============
Q_VarArgs

Does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
============
*/
char *Q_VarArgs (char *format, ...) {
	va_list		argptr;
	static char	string[1024];
	
	va_start (argptr, format);
	vsnprintf (string, sizeof (string), format, argptr);
	va_end (argptr);

	return string;
}
