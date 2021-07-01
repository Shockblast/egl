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
int Q_ColorCharCount (const char *s, int byteOfs)
{
	int			c;
	const char	*end;
	qBool		skipNext = qFalse;

	end = s + byteOfs;

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
int Q_ColorCharOffset (const char *s, int charcount)
{
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
void Q_snprintfz (char *dest, size_t size, const char *fmt, ...)
{
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
Q_strcatz
===============
*/
void Q_strcatz (char *dst, const char *src, int dstSize)
{
	int		len;

	len = strlen (dst);
	if (len >= dstSize) {
		Com_Printf (PRNT_ERROR, "Q_strcatz: already overflowed");
		return;
	}

	Q_strncpyz (dst + len, src, dstSize - len);
}


/*
===============
Q_strncpyz
===============
*/
void Q_strncpyz (char *dest, const char *src, size_t size)
{
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
Q_strlwr
===============
*/
char *Q_strlwr (char *s)
{
	char *p;

	if (s) {
		for (p=s ; *s ; s++)
			*s = Q_tolower (*s);
		return p;
	}

	return NULL;
}


/*
===============
Q_tolower

by R1CH
===============
*/
#ifdef id386
__declspec(naked) int __cdecl Q_tolower (int c)
{
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
Q_WildcardMatch

from Q2ICE
============
*/
int Q_WildcardMatch (const char *filter, const char *string, int ignoreCase)
{
	switch (*filter) {
	case '\0':	return !*string;
	case '*':	return Q_WildcardMatch (filter + 1, string, ignoreCase) || *string && Q_WildcardMatch (filter, string + 1, ignoreCase);
	case '?':	return *string && Q_WildcardMatch (filter + 1, string + 1, ignoreCase);
	default:	return (*filter == *string || (ignoreCase && toupper (*filter) == toupper (*string))) && Q_WildcardMatch (filter + 1, string + 1, ignoreCase);
	}
}


/*
============
Q_VarArgs

Does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
============
*/
char *Q_VarArgs (char *format, ...)
{
	va_list		argptr;
	static char	string[2][1024];
	static int	strIndex;

	strIndex ^= 1;

	va_start (argptr, format);
	vsnprintf (string[strIndex], sizeof (string[strIndex]), format, argptr);
	va_end (argptr);

	return string[strIndex];
}
