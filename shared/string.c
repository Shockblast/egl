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

vec4_t	Q_colorBlack	= { 0, 0, 0, 1 };
vec4_t	Q_colorRed		= { 1, 0, 0, 1 };
vec4_t	Q_colorGreen	= { 0, 1, 0, 1 };
vec4_t	Q_colorYellow	= { 1, 1, 0, 1 };
vec4_t	Q_colorBlue		= { 0, 0, 1, 1 };
vec4_t	Q_colorCyan		= { 0, 1, 1, 1 };
vec4_t	Q_colorMagenta	= { 1, 0, 1, 1 };
vec4_t	Q_colorWhite	= { 1, 1, 1, 1 };

vec4_t	Q_colorLtGrey	= { 0.75, 0.75, 0.75, 1 };
vec4_t	Q_colorMdGrey	= { 0.5, 0.5, 0.5, 1 };
vec4_t	Q_colorDkGrey	= { 0.25, 0.25, 0.25, 1 };

vec4_t	Q_strColorTable[10] = {
	{ 0.0f, 0.0f, 0.0f, 1.0f },
	{ 1.0f, 0.0f, 0.0f, 1.0f },
	{ 0.0f, 1.0f, 0.0f, 1.0f },
	{ 1.0f, 1.0f, 0.0f, 1.0f },
	{ 0.0f, 0.0f, 1.0f, 1.0f },
	{ 0.0f, 1.0f, 1.0f, 1.0f },
	{ 1.0f, 0.0f, 1.0f, 1.0f },
	{ 1.0f, 1.0f, 1.0f, 1.0f },
	
	{ 0.75f, 0.75f, 0.75f, 1.0f },
	{ 0.25f, 0.25f, 0.25f, 1.0f }
};

/*
===============
Q_ColorCharCount
===============
*/
qBool Q_IsColorString (const char *p)
{
	if (!*p || (*p & 127) != COLOR_ESCAPE)
		return qFalse;

	switch (p[1] & 127) {
	case '0':	case '1':	case '2':	case '3':	case '4':
	case '5':	case '6':	case '7':	case '8':	case '9':
	case 'i':	case 'I':	// S_STYLE_ITALIC
	case 'r':	case 'R':	// S_STYLE_RETURN
	case 's':	case 'S':	// S_STYLE_SHADOW
	case '^':				// COLOR_ESCAPE
		return qTrue;
	}

	return qFalse;
}


/*
===============
Q_ColorCharCount
===============
*/
size_t Q_ColorCharCount (const char *s, size_t endPos)
{
	size_t		count;
	const char	*end;

	end = s + endPos;
	for (count=0 ; *s && s<end ; s++) {
		if ((s[0] & 127) != COLOR_ESCAPE)
			continue;

		switch (s[1] & 127) {
		case '0':	case '1':	case '2':	case '3':	case '4':
		case '5':	case '6':	case '7':	case '8':	case '9':
		case 'i':	case 'I':
		case 'r':	case 'R':
		case 's':	case 'S':
			count += 2;
			break;
		case '^':
			count++;
			break;
		}
	}

	return endPos - count;
}


/*
===============
Q_ColorCharOffset
===============
*/
size_t Q_ColorCharOffset (const char *s, size_t charCount)
{
	const char	*start = s;
	qBool		skipNext = qFalse;

	for ( ; *s && charCount ; s++) {
		if (skipNext)
			skipNext = qFalse;
		else if (Q_IsColorString (s)) {
			skipNext = qTrue;
		}
		else
			charCount--;
	}

	return s - start;
}


/*
===============
Q_ColorStrLastColor
===============
*/
int Q_ColorStrLastColor (char *s, size_t byteOfs)
{
	char	*end;
	int		lastClrIndex = Q_StrColorIndex (COLOR_WHITE);

	end = s + (byteOfs - 1);	// don't check last byte
	for ( ; *s && s<end ; s++) {
		if ((s[0] & 127) != COLOR_ESCAPE)
			continue;

		switch (s[1] & 127) {
		case '0':	case '1':	case '2':	case '3':	case '4':
		case '5':	case '6':	case '7':	case '8':	case '9':
			lastClrIndex = (s[1] & 127) - '0';
			break;
		case 'r':	case 'R':
			lastClrIndex = Q_StrColorIndex (COLOR_WHITE);
			break;
		default:
			continue;
		}

		s++;
	}

	return lastClrIndex;
}


/*
===============
Q_ColorStrLastStyle
===============
*/
int Q_ColorStrLastStyle (char *s, size_t byteOfs)
{
	char	*end;
	int		lastStyle;

	end = s + (byteOfs);	// don't check last byte
	lastStyle = 0;
	for ( ; *s && s<end ; s++) {
		if ((s[0] & 127) != COLOR_ESCAPE)
			continue;

		switch (s[1] & 127) {
		case 'i':	case 'I':
			lastStyle ^= FS_ITALIC;
			break;
		case 's':	case 'S':
			lastStyle ^= FS_SHADOW;
			break;
		case 'r':	case 'R':
			lastStyle = 0;
			break;
		default:
			continue;
		}

		s++;
	}

	return lastStyle;
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

		dest[size-1] = '\0';
	} else {
		dest[0] = '\0';
	}
}


/*
===============
Q_strcatz
===============
*/
void Q_strcatz (char *dst, const char *src, size_t dstSize)
{
	size_t		len;

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
			*s = tolower (*s);
		return p;
	}

	return NULL;
}

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
