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
// cbuf.c
//

#include "common.h"

static netMsg_t	com_cbufText;
static byte		com_cbufTextBuf[32768];
static byte		com_cbufDeferTextBuf[32768];

/*
=============================================================================

	COMMAND BUFFER

=============================================================================
*/

/*
============
Cbuf_AddText

Adds command text at the end of the buffer
============
*/
void Cbuf_AddText (char *text)
{
	int		l;
	
	l = strlen (text);

	if (com_cbufText.curSize + l >= com_cbufText.maxSize) {
		Com_Printf (PRNT_WARNING, "Cbuf_AddText: overflow\n");
		return;
	}

	MSG_Write (&com_cbufText, text, strlen (text));
}


/*
============
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
FIXME: actually change the command buffer to do less copying
============
*/
void Cbuf_InsertText (char *text)
{
	char	*temp;
	int		templen;

	// Copy off any commands still remaining in the exec buffer
	templen = com_cbufText.curSize;
	if (templen) {
		temp = Mem_Alloc (templen);
		memcpy (temp, com_cbufText.data, templen);
		MSG_Clear (&com_cbufText);
	}
	else
		temp = NULL;	// Shut up compiler
		
	// Add the entire text of the file
	Cbuf_AddText (text);
	
	// Add the copied off data
	if (templen) {
		MSG_Write (&com_cbufText, temp, templen);
		Mem_Free (temp);
	}
}


/*
============
Cbuf_CopyToDefer
============
*/
void Cbuf_CopyToDefer (void)
{
	memcpy (com_cbufDeferTextBuf, com_cbufTextBuf, com_cbufText.curSize);
	com_cbufDeferTextBuf[com_cbufText.curSize] = 0;
	com_cbufText.curSize = 0;
}


/*
============
Cbuf_InsertFromDefer
============
*/
void Cbuf_InsertFromDefer (void)
{
	Cbuf_InsertText ((char *)com_cbufDeferTextBuf);
	com_cbufDeferTextBuf[0] = 0;
}


/*
============
Cbuf_Execute
============
*/
void Cbuf_Execute (void)
{
	char		*text;
	char		line[1024];
	int			i, quotes;

	com_aliasCount = 0;		// Don't allow infinite alias loops

	while (com_cbufText.curSize) {
		// Find a \n or ; line break
		text = (char *)com_cbufText.data;

		quotes = 0;
		for (i=0 ; i< com_cbufText.curSize ; i++) {
			if (text[i] == '"')
				quotes++;
			if (!(quotes&1) && text[i] == ';')
				break;	// Don't break if inside a quoted string
			if (text[i] == '\n')
				break;
		}

		if (i > sizeof (line) - 1)
			i =  sizeof (line) - 1;

		memcpy (line, text, i);
		line[i] = 0;

		/*
		** Delete the text from the command buffer and move remaining commands down
		** this is necessary because commands (exec, alias) can insert data at the
		** beginning of the text buffer
		*/
		if (i == com_cbufText.curSize)
			com_cbufText.curSize = 0;
		else {
			i++;
			com_cbufText.curSize -= i;
			memmove (text, text+i, com_cbufText.curSize);
		}

		// Execute the command line
		Cmd_ExecuteString (line);

		if (com_cmdWait) {
			// Skip out while text still remains in buffer, leaving it for next frame
#ifndef DEDICATED_ONLY
			if (!dedicated->integer)
				CL_ForcePacket ();
#endif
			com_cmdWait = qFalse;
			break;
		}
	}
}

/*
=============================================================================

	INITIALIZATION

=============================================================================
*/

/*
============
Cbuf_Init
============
*/
void Cbuf_Init (void)
{
	MSG_Init (&com_cbufText, com_cbufTextBuf, sizeof (com_cbufTextBuf));
}
