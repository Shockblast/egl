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
==================
Com_DefaultExtension
==================
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
===============
Com_PageInMemory
===============
*/
void Com_PageInMemory (byte *buffer, int size) {
	int			i;
	static int	pagedTotal;

	for (i=size-1 ; i>0 ; i-=4096)
		pagedTotal += buffer[i];
}


/*
==============
Com_Parse

Parse a token out of a string
==============
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
================
Com_ParseExt
================
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

	while (1) {
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
		while (1) {
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
================
Com_SkipRestOfLine
================
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
================
Com_SkipWhiteSpace
================
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


/*
===============
Com_sprintf
===============
*/
void Com_sprintf (char *dest, int size, char *fmt, ...) {
	int		len;
	va_list	argptr;
	char	bigbuffer[0x10000];

	va_start (argptr, fmt);
	len = vsprintf (bigbuffer, fmt, argptr);
	va_end (argptr);

	if (len >= size)
		Com_Printf (0, S_COLOR_YELLOW "Com_sprintf: overflow of %i in %i\n", len, size);

	strncpy (dest, bigbuffer, size-1);
}


/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
char *va (char *format, ...) {
	va_list			argptr;
	static char		string[1024];
	
	va_start (argptr, format);
	vsprintf (string, format, argptr);
	va_end (argptr);

	return string;
}

/*
============================================================================

	BYTE ORDER FUNCTIONS

	Updated thanks to Berserk
============================================================================
*/

/*
===============
FloatSwap
===============
*/
float FloatSwap (float f) {
	union {
		byte	b[4];
		float	f;
	} in, out;
	
	in.f = f;

	out.b[0] = in.b[3];
	out.b[1] = in.b[2];
	out.b[2] = in.b[1];
	out.b[3] = in.b[0];
	
	return out.f;
}


/*
===============
LongSwap
===============
*/
int LongSwap (int l) {
	union {
		byte	b[4];
		int		l;
	} in, out;

	in.l = l;

	out.b[0] = in.b[3];
	out.b[1] = in.b[2];
	out.b[2] = in.b[1];
	out.b[3] = in.b[0];

	return out.l;
}


/*
===============
ShortSwap
===============
*/
short ShortSwap (short s) {
	union {
		byte	b[2];
		short	s;
	} in, out;

	in.s = s;

	out.b[0] = in.b[1];
	out.b[1] = in.b[0];

	return out.s;
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
Q_strlwr
===============
*/
char *Q_strlwr (char *s) {
	char *p;

	if (s) {
		for (p=s ; *s ; s++)
			*s = tolower (*s);
		return p;
	}

	return NULL;
}

/*
=====================================================================

	INFO STRINGS

=====================================================================
*/

/*
================
Info_Print
================
*/
void Info_Print (char *s) {
	char	key[512];
	char	value[512];
	char	*o;
	int		l;

	if (*s == '\\')
		s++;

	while (*s) {
		o = key;
		while (*s && (*s != '\\'))
			*o++ = *s++;

		l = o - key;
		if (l < 20) {
			memset (o, ' ', 20-l);
			key[20] = 0;
		} else
			*o = 0;
		Com_Printf (0, "%s", key);

		if (!*s) {
			Com_Printf (0, "MISSING VALUE\n");
			return;
		}

		o = value;
		s++;
		while (*s && (*s != '\\'))
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;
		Com_Printf (0, "%s\n", value);
	}
}


/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
===============
*/
char *Info_ValueForKey (char *s, char *key) {
	char	pkey[512];
	static	char value[2][512];	// use two buffers so compares
								// work without stomping on each other
	static	int	valueindex;
	char	*o;
	
	valueindex ^= 1;
	if (*s == '\\')
		s++;

	for ( ; ; ) {
		o = pkey;
		while (*s != '\\') {
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while ((*s != '\\') && *s) {
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp (key, pkey))
			return value[valueindex];

		if (!*s)
			return "";
		s++;
	}
}


/*
==================
Info_RemoveKey
==================
*/
void Info_RemoveKey (char *s, char *key) {
	char	*start;
	char	pkey[512];
	char	value[512];
	char	*o;

	if (strstr (key, "\\"))
		return;

	for ( ; ; ) {
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\') {
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while ((*s != '\\') && *s) {
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp (key, pkey)) {
			strcpy (start, s);	// remove this part
			return;
		}

		if (!*s)
			return;
	}

}


/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing
==================
*/
qBool Info_Validate (char *s) {
	if (strstr (s, "\""))
		return qFalse;
	if (strstr (s, ";"))
		return qFalse;

	return qTrue;
}


/*
==================
Info_SetValueForKey
==================
*/
void Info_SetValueForKey (char *s, char *key, char *value) {
	char	newi[MAX_INFO_STRING], *v;
	int		c, maxsize = MAX_INFO_STRING;

	if (strstr (key, "\\") || strstr (value, "\\")) {
		Com_Printf (0, S_COLOR_YELLOW "Can't use keys or values with a \\\n");
		return;
	}

	if (strstr (key, ";")) {
		Com_Printf (0, S_COLOR_YELLOW "Can't use keys or values with a semicolon\n");
		return;
	}

	if (strstr (key, "\"") || strstr (value, "\"")) {
		Com_Printf (0, S_COLOR_YELLOW "Can't use keys or values with a \"\n");
		return;
	}

	if ((strlen(key) > MAX_INFO_KEY-1) || (strlen(value) > MAX_INFO_KEY-1)) {
		Com_Printf (0, S_COLOR_YELLOW "Keys and values must be < 64 characters.\n");
		return;
	}
	Info_RemoveKey (s, key);
	if (!value || !strlen(value))
		return;

	Com_sprintf (newi, sizeof (newi), "\\%s\\%s", key, value);

	if ((strlen(newi) + strlen (s)) > maxsize) {
		Com_Printf (0, S_COLOR_YELLOW "Info string length exceeded\n");
		return;
	}

	// only copy ascii values
	s += strlen (s);
	v = newi;
	while (*v) {
		c = *v++;
		c &= 127;	// strip high bits
		if ((c >= 32) && (c < 127))
			*s++ = c;
	}
	*s = 0;
}
