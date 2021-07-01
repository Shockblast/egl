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

static netMsg_t	cbufText;
static byte		cbufTextBuf[32768];
static byte		cbufDeferTextBuf[32768];

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
void Cbuf_AddText (char *text) {
	int		l;
	
	l = Q_strlen (text);

	if (cbufText.curSize + l >= cbufText.maxSize) {
		Com_Printf (0, S_COLOR_YELLOW "Cbuf_AddText: overflow\n");
		return;
	}

	MSG_Write (&cbufText, text, Q_strlen (text));
}


/*
============
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
FIXME: actually change the command buffer to do less copying
============
*/
void Cbuf_InsertText (char *text) {
	char	*temp;
	int		templen;

	// copy off any commands still remaining in the exec buffer
	templen = cbufText.curSize;
	if (templen) {
		temp = Mem_Alloc (templen);
		memcpy (temp, cbufText.data, templen);
		MSG_Clear (&cbufText);
	}
	else
		temp = NULL;	// shut up compiler
		
	// add the entire text of the file
	Cbuf_AddText (text);
	
	// add the copied off data
	if (templen) {
		MSG_Write (&cbufText, temp, templen);
		Mem_Free (temp);
	}
}


/*
============
Cbuf_CopyToDefer
============
*/
void Cbuf_CopyToDefer (void) {
	memcpy (cbufDeferTextBuf, cbufTextBuf, cbufText.curSize);
	cbufDeferTextBuf[cbufText.curSize] = 0;
	cbufText.curSize = 0;
}


/*
============
Cbuf_InsertFromDefer
============
*/
void Cbuf_InsertFromDefer (void) {
	Cbuf_InsertText (cbufDeferTextBuf);
	cbufDeferTextBuf[0] = 0;
}


/*
============
Cbuf_Execute
============
*/
void Cbuf_Execute (void) {
	char		*text;
	char		line[1024];
	int			i, quotes;

	aliasCount = 0;		// don't allow infinite alias loops

	while (cbufText.curSize) {
		// find a \n or ; line break
		text = (char *)cbufText.data;

		quotes = 0;
		for (i=0 ; i< cbufText.curSize ; i++) {
			if (text[i] == '"')
				quotes++;
			if (!(quotes&1) &&  (text[i] == ';'))
				break;	// don't break if inside a quoted string
			if (text[i] == '\n')
				break;
		}

		if (i > sizeof (line) - 1)
			i =  sizeof (line) - 1;

		memcpy (line, text, i);
		line[i] = 0;

		/*
		** delete the text from the command buffer and move remaining commands down
		** this is necessary because commands (exec, alias) can insert data at the
		** beginning of the text buffer
		*/
		if (i == cbufText.curSize)
			cbufText.curSize = 0;
		else {
			i++;
			cbufText.curSize -= i;
			memmove (text, text+i, cbufText.curSize);
		}

		// execute the command line
		Cmd_ExecuteString (line);
		
		if (cmdWait) {
			// skip out while text still remains in buffer, leaving it for next frame
			CL_ForcePacket ();
			cmdWait = qFalse;
			break;
		}
	}
}

/*
=============================================================================

	INIT / SHUTDOWN

=============================================================================
*/

/*
============
Cbuf_Init
============
*/
void Cbuf_Init (void) {
	MSG_Init (&cbufText, cbufTextBuf, sizeof (cbufTextBuf));
}


/*
============
Cbuf_Shutdown
============
*/
void Cbuf_Shutdown (void) {
}
