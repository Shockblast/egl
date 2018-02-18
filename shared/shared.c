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
// shared.c
//

#include "shared.h"

/*
==============================================================================

	PARSING
 
==============================================================================
*/

/*
============
Com_Parse

Parse a token out of a string
============
*/
static char com_token[MAX_TOKEN_CHARS];
char *Com_Parse (char **dataPtr)
{
	int		c;
	int		len;
	char	*data;

	data = *dataPtr;
	len = 0;
	com_token[0] = 0;
	
	if (!data) {
		*dataPtr = NULL;
		return "";
	}
		
	// Skip whitespace
skipwhite:
	while ((c = *data) <= ' ') {
		if (c == 0) {
			*dataPtr = NULL;
			return "";
		}

		data++;
	}
	
	// Skip // comments
	if (c == '/' && data[1] == '/') {
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}

	// Handle quoted strings specially
	if (c == '\"') {
		data++;
		for ( ; ; ) {
			c = *data++;
			if (c == '\"' || !c) {
				com_token[len] = 0;
				*dataPtr = data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS) {
				com_token[len] = c;
				len++;
			}
		}
	}

	// Parse a regular word
	do {
		if (len < MAX_TOKEN_CHARS) {
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c > 32);

	if (len >= MAX_TOKEN_CHARS)
		len = 0;
	com_token[len] = 0;

	*dataPtr = data;
	return com_token;
}


/*
============
Com_DefaultExtension

If there is no extnsion in 'path', suffix 'extension'.
============
*/
void Com_DefaultExtension (char *path, char *extension, size_t size)
{
	char	*src;

	if (!path[0])
		return;

	// If path doesn't have an .ext, append extension (extension should include the .)
	src = path + strlen (path) - 1;
	while (*src != '/' && src != path) {
		if (*src == '.')
			return;		// It has an extension
		src--;
	}

	Q_strcatz (path, extension, size);
}


/*
============
Com_FileBase
============
*/
void Com_FileBase (char *in, char *out)
{
	char *s, *s2;

	s = in + strlen(in) - 1;
	while (s != in && *s != '.')
		s--;

	for (s2=s ; s2 != in && *s2 != '/' ; s2--) ;

	if (s-s2 < 2) {
		out[0] = 0;
	}
	else {
		s--;
		strncpy (out, s2+1, s-s2);
		out[s-s2] = 0;
	}
}


/*
============
Com_FileExtension
============
*/
void Com_FileExtension (char *path, char *out, size_t size)
{
	uint32		i;

	while (*path && *path != '.')
		path++;
	if (!*path)
		return;

	path++;
	for (i=0 ; i<size-1 && *path ; i++, path++)
		out[i] = *path;
	out[i] = 0;
}


/*
============
Com_FilePath

Returns the path up to, but not including the last /
============
*/
void Com_FilePath (char *path, char *out, size_t size)
{
	char	*s;

	if (size) {
		s = path + strlen(path) - 1;
		while (s != path && *s != '/')
			s--;

		Q_strncpyz (out, path, size);
		if (s-path < (int)size) // FIXME
			out[s-path] = '\0';
	}
}


/*
============
Com_NormalizePath
============
*/
void Com_NormalizePath (char *dest, size_t size, const char *source)
{
	size_t	i, len;
	int		lastDot;
	qBool	twoDots;

	//
	// File name
	// Get last dot location -- flip \ with / -- and copy string
	// the len check in the loop makes certain that there's room for ".ext\0"
	//
	lastDot = -1;
	twoDots = qFalse;
	for (i=(source[0] == '\\' || source[0] == '/')?1:0, len=0 ; source[i] && (len < size-2) ; i++) {
		switch (source[i]) {
		case '\\':
		case '/':
			// Remove "./" but not "../"
			if (lastDot == (int) (len-1) && !twoDots)
				dest[len--] = '/';
			else
				dest[len++] = '/';
			continue;

		case '.':
			// Store the location of the last dot for later extension stripping
			if (lastDot == (int) (len-1))
				twoDots = qTrue;
			else
				twoDots = qFalse;
			lastDot = (int) len;

			// Intentional fall through
		default:
			dest[len++] = source[i];
			break;
		}
	}

	// Terminate (there's room, the above loop allows such)
	dest[len] = '\0';
}


/*
============
Com_SkipPath
============
*/
char *Com_SkipPath (char *pathName)
{
	char	*last;
	
	last = pathName;
	while (*pathName) {
		switch (*pathName) {
		case '/':
		case '\\':
			last = pathName+1;
			break;
		}
		pathName++;
	}
	return last;
}


/*
============
Com_SkipRestOfLine
============
*/
void Com_SkipRestOfLine (char **dataPtr)
{
	char	*data;
	int		c;

	data = *dataPtr;
	while ((c = *data++) != 0) {
		if (c == '\n')
			break;
	}

	*dataPtr = data;
}


/*
============
Com_SkipWhiteSpace
============
*/
char *Com_SkipWhiteSpace (char *dataPtr, qBool *hasNewLines)
{
	int		c;

	while ((c = *dataPtr) <= ' ') {
		switch (c) {
		case 0:
			return NULL;

		case '\n':
			*hasNewLines = qTrue;
			break;
		}

		dataPtr++;
	}

	return dataPtr;
}


/*
============
Com_StripExtension
============
*/
void Com_StripExtension (char *dest, size_t size, char *src)
{
	if (size) {
		while (--size && *src != '.' && (*dest++ = *src++)) ;
		*dest = '\0';
	}
}


/*
=============
Com_StripPadding

Removes spaces from the left/right of the string
=============
*/
void Com_StripPadding (char *in, char *dest)
{
	qBool	hitChar = qFalse;

	while (*in) {
		if (hitChar) {
			*dest++ = *in++;
		}
		else if (*in != ' ') {
			hitChar = qTrue;
			*dest++ = *in++;
		}
		else
			in++;
	}
	*dest = '\0';

	dest = dest + strlen (dest)-1;
	while (*dest && *dest == ' ') {
		*dest = '\0';
		dest--;
	}
}
