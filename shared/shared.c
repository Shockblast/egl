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
============
Com_DefaultExtension
============
*/
void Com_DefaultExtension (char *path, char *extension)
{
	char	*src;

	/*
	** If path doesn't have a .EXT, append extension
	** (extension should include the .)
	*/
	src = path + strlen (path) - 1;
	while ((*src != '/') && (src != path)) {
		if (*src == '.')
			return;		 // It has an extension
		src--;
	}

	strcat (path, extension);
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
	
	if (s-s2 < 2)
		out[0] = 0;
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
	uInt		i;

	while (*path && (*path != '.'))
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
void Com_FilePath (char *path, char *out, int size)
{
	char	*s;

	s = path + strlen(path) - 1;
	while ((s != path) && (*s != '/'))
		s--;

	Q_strncpyz (out, path, size);
	if (s-path < size)
		out[s-path] = '\0';
}


/*
============
Com_PageInMemory

Causes memory access, which pulls the contents of buffer out of the pagefile
============
*/
void Com_PageInMemory (byte *buffer, int size)
{
	int			i;
	static int	pagedTotal;

	for (i=size-1 ; i>0 ; i-=4096)
		pagedTotal += buffer[i];
}


/*
============
Com_Parse

Parse a token out of a string
============
*/
char	com_token[MAX_TOKEN_CHARS];
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
	if ((c=='/') && (data[1] == '/')) {
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}

	// Handle quoted strings specially
	if (c == '\"') {
		data++;
		for ( ; ; ) {
			c = *data++;
			if ((c=='\"') || !c) {
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
	} while (c>32);

	if (len == MAX_TOKEN_CHARS)
		len = 0;
	com_token[len] = 0;

	*dataPtr = data;
	return com_token;
}


/*
============
Com_ParseExt
============
*/
char *Com_ParseExt (char **dataPtr, qBool allowNewLines)
{ 
	int			c, len = 0;
	char		*data;
	qBool		hasNewLines = qFalse;

	data = *dataPtr;
	com_token[0] = 0;

	// Make sure incoming data is valid
	if (!data) {
		*dataPtr = NULL;
		return com_token;
	}

	for ( ; ; ) {
		// Skip whitespace
		data = Com_SkipWhiteSpace (data, &hasNewLines);
		if (!data) {
			*dataPtr = NULL;
			return com_token;
		}

		if (hasNewLines && !allowNewLines) {
			*dataPtr = data;
			return com_token;
		}

		c = *data;

		// Skip // comments
		if ((c == '/') && (data[1] == '/')) {
			while (*data && *data != '\n')
			data++;
		}
		// Skip /* comments
		else if ((c == '/') && (data[1] == '*')) {
			data += 2;

			while (*data && ((*data != '*') || (data[1] != '/')))
				data++;

			if (*data)
				data += 2;
		// An actual token
		}
		else
			break;
	} 

	// Handle quoted strings specially
	if (c == '\"') {
		data++;
		for ( ; ; ) {
			c = *data++;

			if ((c == '\"') || !c) {
				*dataPtr = data;
				com_token[len] = 0;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS)
				com_token[len++] = c;
		}
	}

	// Parse a regular word
	do {
		if (len < MAX_TOKEN_CHARS)
			com_token[len++] = c;

		data++;
		c = *data;
	} while (c > 32);

	if (len == MAX_TOKEN_CHARS)
		len = 0;

	com_token[len] = 0;

	*dataPtr = data;
	return com_token;
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
		if (*pathName == '/')
			last = pathName+1;
		else if (*pathName == '\\')
			last = pathName+1;
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
		if (!c)
			return NULL;

		if (c == '\n')
			*hasNewLines = qTrue;

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
		while (--size && (*src != '.') && (*dest++ = *src++)) ;
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
			*in++;
	}
	*dest = 0;

	dest = dest + strlen (dest)-1;
	while (*dest && (*dest == ' ')) {
		*dest = 0;
		dest--;
	}
}
