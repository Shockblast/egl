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

// SHARED.C

#include "shared.h"

/*
============
Com_DefaultExtension
============
*/
void Com_DefaultExtension (char *path, char *extension) {
	char	*src;

	/*
	** if path doesn't have a .EXT, append extension
	** (extension should include the .)
	*/
	src = path + strlen(path) - 1;

	while ((*src != '/') && (src != path))
	{
		if (*src == '.')
			return;				 // it has an extension
		src--;
	}

	strcat (path, extension);
}


/*
============
Com_FileBase
============
*/
void Com_FileBase (char *in, char *out) {
	char *s, *s2;
	
	s = in + strlen(in) - 1;
	
	while (s != in && *s != '.')
		s--;
	
	for (s2=s ; (s2 != in) && (*s2 != '/') ; s2--)
	;
	
	if (s-s2 < 2)
		out[0] = 0;
	else
	{
		s--;
		strncpy (out,s2+1, s-s2);
		out[s-s2] = 0;
	}
}


/*
============
COM_FileExtension
============
*/
char *COM_FileExtension (char *in) {
	static char exten[8];
	int		i;

	while (*in && *in != '.')
		in++;
	if (!*in)
		return "";
	in++;
	for (i=0 ; i<7 && *in ; i++,in++)
		exten[i] = *in;
	exten[i] = 0;
	return exten;
}


/*
============
Com_FilePath

Returns the path up to, but not including the last /
============
*/
void Com_FilePath (char *in, char *out) {
	char *s;
	
	s = in + strlen(in) - 1;
	
	while (s != in && *s != '/')
		s--;

	strncpy (out,in, s-in);
	out[s-in] = 0;
}


/*
============
Com_PageInMemory
============
*/
void Com_PageInMemory (byte *buffer, int size) {
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
char *Com_Parse (char **data_p) {
	int		c;
	int		len;
	char	*data;

	data = *data_p;
	len = 0;
	com_token[0] = 0;
	
	if (!data) {
		*data_p = NULL;
		return "";
	}
		
	// skip whitespace
skipwhite:
	while ((c = *data) <= ' ') {
		if (c == 0) {
			*data_p = NULL;
			return "";
		}

		data++;
	}
	
	// skip // comments
	if ((c=='/') && (data[1] == '/')) {
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}

	// handle quoted strings specially
	if (c == '\"') {
		data++;
		for ( ; ; ) {
			c = *data++;
			if ((c=='\"') || !c) {
				com_token[len] = 0;
				*data_p = data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS) {
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
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

	*data_p = data;
	return com_token;
}


/*
============
Com_ParseExt
============
*/
char *Com_ParseExt (char **data_p, qBool allowNewLines) { 
	int			c, len = 0;
	char		*data;
	qBool		hasNewLines = qFalse;

	data = *data_p;
	com_token[0] = 0;

	// make sure incoming data is valid
	if (!data) {
		*data_p = NULL;
		return com_token;
	}

	for ( ; ;) {
		// skip whitespace
		data = Com_SkipWhiteSpace(data, &hasNewLines);
		if (!data) {
			*data_p = NULL;
			return com_token;
		}

		if (hasNewLines && !allowNewLines) {
			*data_p = data;
			return com_token;
		}

		c = *data;

		// skip // comments
		if ((c == '/') && (data[1] == '/')) {
			while (*data && *data != '\n')
			data++;
		}
		// skip /* comments
		else if ((c == '/') && (data[1] == '*')) {
			data += 2;

			while (*data && ((*data != '*') || (data[1] != '/')))
			{
				data++;
			}

			if (*data)
				data += 2;
		// an actual token
		} else
			break;
	} 

	// handle quoted strings specially
	if (c == '\"') {
		data++;
		for ( ; ; ) {
			c = *data++;

			if ((c == '\"') || !c) {
				*data_p = data;
				com_token[len] = 0;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS)
				com_token[len++] = c;
		}
	}

	// parse a regular word
	do {
		if (len < MAX_TOKEN_CHARS)
			com_token[len++] = c;

		data++;
		c = *data;
	} while (c > 32);

	if (len == MAX_TOKEN_CHARS)
		len = 0;

	com_token[len] = 0;

	*data_p = data;
	return com_token;
}


/*
============
Com_SkipPath
============
*/
char *Com_SkipPath (char *pathname) {
	char	*last;
	
	last = pathname;
	while (*pathname) {
		if (*pathname=='/')
			last = pathname+1;
		pathname++;
	}
	return last;
}


/*
============
Com_SkipRestOfLine
============
*/
void Com_SkipRestOfLine (char **data_p) { 
	char	*data;
	int		c;

	data = *data_p;
	while ((c = *data++) != 0) {
		if (c == '\n')
			break;
	}

	*data_p = data;
}


/*
============
Com_SkipWhiteSpace
============
*/
char *Com_SkipWhiteSpace (char *data_p, qBool *hasNewLines) { 
	int		c;

	while ((c = *data_p) <= ' ') {
		if (!c)
			return NULL;

		if (c == '\n')
			*hasNewLines = qTrue;

		data_p++;
	}

	return data_p;
}


/*
============
Com_StripExtension
============
*/
void Com_StripExtension (char *in, char *out) {
	while (*in && (*in != '.'))
		*out++ = *in++;
	*out = 0;
}
