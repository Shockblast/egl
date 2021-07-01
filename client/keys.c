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
// keys.c
//

#include "cl_local.h"

qBool			keyShiftDown = qFalse;
qBool			keyCapslockOn = qFalse;
qBool			keyInsertOn = qTrue;

int				keyAnyKeyDown;
int				keyWaiting;

char			*keyBindings[K_MAXKEYS];
qBool			keysDown[K_MAXKEYS];		// key up events are sent even if in console mode
static int		keysRepeated[K_MAXKEYS];	// if > 1, it is autorepeating
static qBool	keyConsoleKeys[K_MAXKEYS];	// if qTrue, can't be typed while in the console
static int		keyShiftKeys[K_MAXKEYS];	// key to map to if shift held down when this key is pressed
qBool			keyMenuBound[K_MAXKEYS];	// if qTrue, can't be rebound while in menu

char			keyConsoleBuffer[32][MAXCMDLINE];
int				keyConsoleLinePos = 1;
int				keyConsoleEditLine = 0;
int				keyConsoleHistoryLine = 0;

qBool			keyChatTeam;
char			keyChatBuffer[32][MAXCMDLINE];
int				keyChatLinePos = 0;
int				keyChatEditLine = 0;
int				keyChatHistoryLine = 0;

typedef struct {
	char	*name;
	int		keyNum;
} keyName_t;

keyName_t key_Names[] = {
	{"TAB",			K_TAB},
	{"ENTER",		K_ENTER},
	{"ESCAPE",		K_ESCAPE},
	{"SPACE",		K_SPACE},
	{"BACKSPACE",	K_BACKSPACE},

	{"UPARROW",		K_UPARROW},
	{"DOWNARROW",	K_DOWNARROW},
	{"LEFTARROW",	K_LEFTARROW},
	{"RIGHTARROW",	K_RIGHTARROW},

	{"ALT",			K_ALT},
	{"CTRL",		K_CTRL},

	{"SHIFT",		K_SHIFT},
	{"LSHIFT",		K_LSHIFT},
	{"RSHIFT",		K_RSHIFT},
	
	{"F1",			K_F1},
	{"F2",			K_F2},
	{"F3",			K_F3},
	{"F4",			K_F4},
	{"F5",			K_F5},
	{"F6",			K_F6},
	{"F7",			K_F7},
	{"F8",			K_F8},
	{"F9",			K_F9},
	{"F10",			K_F10},
	{"F11",			K_F11},
	{"F12",			K_F12},

	{"INS",			K_INS},
	{"DEL",			K_DEL},
	{"PGDN",		K_PGDN},
	{"PGUP",		K_PGUP},
	{"HOME",		K_HOME},
	{"END",			K_END},

	{"MOUSE1",		K_MOUSE1},
	{"MOUSE2",		K_MOUSE2},
	{"MOUSE3",		K_MOUSE3},
	{"MOUSE4",		K_MOUSE4},
	{"MOUSE5",		K_MOUSE5},

	{"JOY1",		K_JOY1},
	{"JOY2",		K_JOY2},
	{"JOY3",		K_JOY3},
	{"JOY4",		K_JOY4},

	{"AUX1",		K_AUX1},
	{"AUX2",		K_AUX2},
	{"AUX3",		K_AUX3},
	{"AUX4",		K_AUX4},
	{"AUX5",		K_AUX5},
	{"AUX6",		K_AUX6},
	{"AUX7",		K_AUX7},
	{"AUX8",		K_AUX8},
	{"AUX9",		K_AUX9},
	{"AUX10",		K_AUX10},
	{"AUX11",		K_AUX11},
	{"AUX12",		K_AUX12},
	{"AUX13",		K_AUX13},
	{"AUX14",		K_AUX14},
	{"AUX15",		K_AUX15},
	{"AUX16",		K_AUX16},
	{"AUX17",		K_AUX17},
	{"AUX18",		K_AUX18},
	{"AUX19",		K_AUX19},
	{"AUX20",		K_AUX20},
	{"AUX21",		K_AUX21},
	{"AUX22",		K_AUX22},
	{"AUX23",		K_AUX23},
	{"AUX24",		K_AUX24},
	{"AUX25",		K_AUX25},
	{"AUX26",		K_AUX26},
	{"AUX27",		K_AUX27},
	{"AUX28",		K_AUX28},
	{"AUX29",		K_AUX29},
	{"AUX30",		K_AUX30},
	{"AUX31",		K_AUX31},
	{"AUX32",		K_AUX32},

	{"KP_HOME",			K_KP_HOME},
	{"KP_UPARROW",		K_KP_UPARROW},
	{"KP_PGUP",			K_KP_PGUP},
	{"KP_LEFTARROW",	K_KP_LEFTARROW},
	{"KP_5",			K_KP_FIVE},
	{"KP_RIGHTARROW",	K_KP_RIGHTARROW},
	{"KP_END",			K_KP_END},
	{"KP_DOWNARROW",	K_KP_DOWNARROW},
	{"KP_PGDN",			K_KP_PGDN},
	{"KP_ENTER",		K_KP_ENTER},
	{"KP_INS",			K_KP_INS},
	{"KP_DEL",			K_KP_DEL},
	{"KP_SLASH",		K_KP_SLASH},
	{"KP_MINUS",		K_KP_MINUS},
	{"KP_PLUS",			K_KP_PLUS},

	{"MWHEELUP",	K_MWHEELUP},
	{"MWHEELDOWN",	K_MWHEELDOWN},
	{"MWHEELLEFT",	K_MWHEELLEFT},
	{"MWHEELRIGHT",	K_MWHEELRIGHT},

	{"PAUSE",		K_PAUSE},

	{"SEMICOLON",	';'},	// because a raw semicolon seperates commands

	{NULL,			0}
};

/*
==============================================================================

	KEY HANDLING	

==============================================================================
*/

/*
=====================
Key_KeyDest
=====================
*/
qBool Key_KeyDest (int keyDest) {
	if ((keyDest < KD_MINDEST) || (keyDest > KD_MAXDEST))
		Com_Error (ERR_FATAL, "Key_KeyDest: invalid keyDest");

	if (cls.keyDest == keyDest)
		return qTrue;

	return qFalse;
}


/*
=====================
Key_SetKeyDest
=====================
*/
void Key_SetKeyDest (int keyDest) {
	if ((keyDest < KD_MINDEST) || (keyDest > KD_MAXDEST))
		Com_Error (ERR_FATAL, "Key_KeyDest: invalid keyDest");

	cls.keyDest = keyDest;
}


/*
====================
Key_CompleteCommand
====================
*/
static void Key_CompleteCommand (void) {
	cmdFunc_t	*cmd, *cmdFound;
	aliasCmd_t	*alias, *aliasFound;
	cVar_t		*cvar, *cvarFound;
	char		*cmdName, *partial;
	int			aliasCnt, cmdCnt, cvarCnt, len;
	static int	compFrame = 0;

	partial = keyConsoleBuffer[keyConsoleEditLine]+1;
	// skip '/' and '\'
	if ((*partial == '\\') || (*partial == '/'))
		partial++;

	len = strlen (partial);
	if (!len)
		return;

	cmdName = NULL;
	compFrame++;

	cmd = cmdFound = NULL;
	alias = aliasFound = NULL;
	cvar = cvarFound = NULL;
	aliasCnt = cmdCnt = cvarCnt = 0;

	//
	// check for exact match
	//
	for (cmd=cmdFunctions ; cmd ; cmd=cmd->next)
		if (!Q_stricmp (partial, cmd->name)) {
			if (cmdCnt == 0)
				cmdFound = cmd;
			if (cmd->compFrame != compFrame)
				cmdCnt++;
			cmd->compFrame = compFrame;
		}
	for (alias=aliasCmds ; alias ; alias=alias->next)
		if (!Q_stricmp (partial, alias->name)) {
			if (aliasCnt == 0)
				aliasFound = alias;
			if (alias->compFrame != compFrame)
				aliasCnt++;
			alias->compFrame = compFrame;
		}
	for (cvar=cvarVariables ; cvar ; cvar=cvar->next)
		if (!Q_stricmp (partial, cvar->name)) {
			if (cvarCnt == 0)
				cvarFound = cvar;
			if (cvar->compFrame != compFrame)
				cvarCnt++;
			cvar->compFrame = compFrame;
		}

	//
	// check for partial match
	//
	for (cmd=cmdFunctions ; cmd ; cmd=cmd->next)
		if (!Q_strnicmp (partial, cmd->name, len)) {
			if (cmdCnt == 0)
				cmdFound = cmd;
			if (cmd->compFrame != compFrame)
				cmdCnt++;
			cmd->compFrame = compFrame;
		}
	for (alias=aliasCmds ; alias ; alias=alias->next)
		if (!Q_strnicmp (partial, alias->name, len)) {
			if (aliasCnt == 0)
				aliasFound = alias;
			if (alias->compFrame != compFrame)
				aliasCnt++;
			alias->compFrame = compFrame;
		}
	for (cvar=cvarVariables ; cvar ; cvar=cvar->next)
		if (!Q_strnicmp (partial, cvar->name, len)) {
			if (cvarCnt == 0)
				cvarFound = cvar;
			if (cvar->compFrame != compFrame)
				cvarCnt++;
			cvar->compFrame = compFrame;
		}

	//
	// return a match if only one was found, otherwise list matches
	//
	if ((aliasCnt + cmdCnt + cvarCnt) == 1) {
		if (cmdFound)
			cmdName = cmdFound->name;
		else if (aliasFound)
			cmdName = aliasFound->name;
		else if (cvarFound)
			cmdName = cvarFound->name;
		else
			cmdName = NULL;
	} else {
		if (aliasCnt > 0) {
			Cbuf_AddText ("echo ----\n echo Matching aliases:\n echo ----\n");
			Cbuf_AddText (Q_VarArgs ("aliaslist *%s*\n", partial));
		}

		if (cmdCnt > 0) {
			Cbuf_AddText ("echo ----\n echo Matching commands:\n echo ----\n");
			Cbuf_AddText (Q_VarArgs ("cmdlist *%s*\n", partial));
		}

		if (cvarCnt > 0) {
			Cbuf_AddText ("echo ----\n echo Matching cvars:\n echo ----\n");
			Cbuf_AddText (Q_VarArgs ("cvarlist *%s*\n", partial));
		}
	}

	if (cmdName) {
		keyConsoleBuffer[keyConsoleEditLine][1] = '/';
		strcpy (keyConsoleBuffer[keyConsoleEditLine]+2, cmdName);
		keyConsoleLinePos = strlen (cmdName) + 2;
		keyConsoleBuffer[keyConsoleEditLine][keyConsoleLinePos] = ' ';
		keyConsoleLinePos++;
		keyConsoleBuffer[keyConsoleEditLine][keyConsoleLinePos] = 0;
	}
}


/*
====================
Key_Message
====================
*/
static void Key_Message (int key) {
	if ((toupper (key) == 'V' && keysDown[K_CTRL]) ||
		((key == K_INS || key == K_KP_INS) && (Key_IsDown (K_SHIFT) || Key_IsDown (K_LSHIFT) || Key_IsDown (K_RSHIFT)))) {
		char *cbd;
		
		if ((cbd = Sys_GetClipboardData ()) != 0) {
			int i;

			strtok (cbd, "\n\r\b");

			i = strlen (cbd);
			if (i + keyChatLinePos >= MAXCMDLINE)
				i= MAXCMDLINE - keyChatLinePos;

			if (i > 0) {
				cbd[i] = 0;
				strcat (keyChatBuffer[keyChatEditLine], cbd);
				keyChatLinePos += i;
			}
			Mem_Free (cbd);
		}

		return;
	}

	if ((key == 'l') && (keysDown[K_CTRL])) {
		keyChatBuffer[keyChatEditLine][0] = 0;
		keyChatLinePos = 0;
		return;
	}

	if ((key == K_ENTER) || (key == K_KP_ENTER)) {
		if (keyChatTeam)
			Cbuf_AddText ("say_team \"");
		else
			Cbuf_AddText ("say \"");

		Cbuf_AddText (keyChatBuffer[keyChatEditLine]);
		Cbuf_AddText ("\"\n");

		keyChatEditLine = (keyChatEditLine + 1) & 31;
		keyChatHistoryLine = keyChatEditLine;
		keyChatBuffer[keyChatEditLine][0] = 0;
		keyChatLinePos = 0;

		Key_SetKeyDest (KD_GAME);
		return;
	}

	if (key == K_ESCAPE) {
		keyChatHistoryLine = keyChatEditLine;
		keyChatBuffer[keyChatEditLine][0] = 0;
		keyChatLinePos = 0;

		Key_SetKeyDest (KD_GAME);
		return;
	}

	if ((key == K_LEFTARROW) || (key == K_KP_LEFTARROW) || ((key == 'h') && keysDown[K_CTRL])) {
		int		charcount;
		// jump over invisible color sequences
		charcount = Q_ColorCharCount (keyChatBuffer[keyChatEditLine], keyChatLinePos);
		if (charcount > 0)
			keyChatLinePos = Q_ColorCharOffset (keyChatBuffer[keyChatEditLine], charcount - 1);
		return;
	}

	if (key == K_BACKSPACE) {
		if (keyChatLinePos > 0) {
			// skip to the end of color sequence
			while (Q_IsColorString (keyChatBuffer[keyChatEditLine] + keyChatLinePos))
				keyChatLinePos += 2;

			strcpy (keyChatBuffer[keyChatEditLine] + keyChatLinePos - 1, keyChatBuffer[keyChatEditLine] + keyChatLinePos);
			keyChatLinePos--;
		}

		return;
	}

	if (key == K_DEL) {
		if (keyChatLinePos < strlen (keyChatBuffer[keyChatEditLine]))
			strcpy (keyChatBuffer[keyChatEditLine] + keyChatLinePos, keyChatBuffer[keyChatEditLine] + keyChatLinePos + 1);
		return;
	}

	if (key == K_INS) {
		keyInsertOn = !keyInsertOn;
		return;
	}

	if (key == K_RIGHTARROW) {
		if (strlen (keyChatBuffer[keyChatEditLine]) == keyChatLinePos) {
			if (strlen (keyChatBuffer[(keyChatEditLine + 31) & 31]) <= keyChatLinePos)
				return;

			keyChatBuffer[keyChatEditLine][keyChatLinePos] = keyChatBuffer[(keyChatEditLine + 31) & 31][keyChatLinePos];
			keyChatLinePos++;
			keyChatBuffer[keyChatEditLine][keyChatLinePos] = 0;
		} else {
			int		charcount;
			// jump over invisible color sequences
			charcount = Q_ColorCharCount (keyChatBuffer[keyChatEditLine], keyChatLinePos);
			keyChatLinePos = Q_ColorCharOffset (keyChatBuffer[keyChatEditLine], charcount + 1);
		}
		return;
	}

	if ((key == K_UPARROW) || (key == K_KP_UPARROW) || ((key == 'p') && keysDown[K_CTRL])) {
		do {
			keyChatHistoryLine = (keyChatHistoryLine - 1) & 31;
		} while ((keyChatHistoryLine != keyChatEditLine) && !keyChatBuffer[keyChatHistoryLine][0]);

		if (keyChatHistoryLine == keyChatEditLine)
			keyChatHistoryLine = (keyChatEditLine + 1) & 31;

		strcpy (keyChatBuffer[keyChatEditLine], keyChatBuffer[keyChatHistoryLine]);
		keyChatLinePos = strlen (keyChatBuffer[keyChatEditLine]);
		return;
	}

	if ((key == K_DOWNARROW) || (key == K_KP_DOWNARROW) || ((key == 'n') && keysDown[K_CTRL])) {
		if (keyChatHistoryLine == keyChatEditLine)
			return;

		do {
			keyChatHistoryLine = (keyChatHistoryLine + 1) & 31;
		}
		while ((keyChatHistoryLine != keyChatEditLine) && !keyChatBuffer[keyChatHistoryLine][0]);

		if (keyChatHistoryLine == keyChatEditLine) {
			keyChatBuffer[keyChatEditLine][0] = 0;
			keyChatLinePos = 0;
		} else {
			strcpy (keyChatBuffer[keyChatEditLine], keyChatBuffer[keyChatHistoryLine]);
			keyChatLinePos = strlen (keyChatBuffer[keyChatEditLine]);
		}
		return;
	}

	if ((key == K_HOME) || (key == K_KP_HOME)) {
		keyChatLinePos = 0;
		return;
	}

	if ((key == K_END) || (key == K_KP_END)) {
		keyChatLinePos = strlen (keyChatBuffer[keyChatEditLine]);
		return;
	}

	if ((key < 32) || (key > 127))
		return;	// non printable

	if (keyChatLinePos < MAXCMDLINE-1) {
		int i;

		// check insert mode
		if (keyInsertOn) {
			// can't do strcpy to move string to right
			i = strlen (keyChatBuffer[keyChatEditLine]) - 1;

			if (i == 254) 
				i--;

			for ( ; i >= keyChatLinePos; i--)
				keyChatBuffer[keyChatEditLine][i + 1] = keyChatBuffer[keyChatEditLine][i];
		}
		// only null terminate if at the end
		i = keyChatBuffer[keyChatEditLine][keyChatLinePos];
		keyChatBuffer[keyChatEditLine][keyChatLinePos] = key;
		keyChatLinePos++;
		if (!i)
			keyChatBuffer[keyChatEditLine][keyChatLinePos] = 0;	
	}
}


/*
====================
Key_Console

Interactive line editing and console scrollback
====================
*/
static void Key_Console (int key) {
	switch (key) {
	case K_KP_SLASH:		key = '/';	break;
	case K_KP_MINUS:		key = '-';	break;
	case K_KP_PLUS:			key = '+';	break;
	case K_KP_HOME:			key = '7';	break;
	case K_KP_UPARROW:		key = '8';	break;
	case K_KP_PGUP:			key = '9';	break;
	case K_KP_LEFTARROW:	key = '4';	break;
	case K_KP_FIVE:			key = '5';	break;
	case K_KP_RIGHTARROW:	key = '6';	break;
	case K_KP_END:			key = '1';	break;
	case K_KP_DOWNARROW:	key = '2';	break;
	case K_KP_PGDN:			key = '3';	break;
	case K_KP_INS:			key = '0';	break;
	case K_KP_DEL:			key = '.';	break;
	}

	if ((toupper (key) == 'V' && keysDown[K_CTRL]) ||
		((key == K_INS || key == K_KP_INS) && (Key_IsDown (K_SHIFT) || Key_IsDown (K_LSHIFT) || Key_IsDown (K_RSHIFT)))) {
		char *cbd;
		
		if ((cbd = Sys_GetClipboardData ()) != 0) {
			int i;

			strtok (cbd, "\n\r\b");

			i = strlen (cbd);
			if (i + keyConsoleLinePos >= MAXCMDLINE)
				i= MAXCMDLINE - keyConsoleLinePos;

			if (i > 0) {
				cbd[i] = 0;
				strcat (keyConsoleBuffer[keyConsoleEditLine], cbd);
				keyConsoleLinePos += i;
			}
			Mem_Free (cbd);
		}

		return;
	}

	if ((key == 'l') && (keysDown[K_CTRL])) {
		Cbuf_AddText ("clear\n");
		return;
	}

	if ((key == K_ENTER) || (key == K_KP_ENTER)) {
		// backslash text are commands, else chat
		if ((keyConsoleBuffer[keyConsoleEditLine][1] == '\\') || (keyConsoleBuffer[keyConsoleEditLine][1] == '/'))
			Cbuf_AddText (keyConsoleBuffer[keyConsoleEditLine]+2);	// skip the >
		else
			Cbuf_AddText (keyConsoleBuffer[keyConsoleEditLine]+1);	// valid command

		Cbuf_AddText ("\n");
		Com_Printf (0, "%s\n", keyConsoleBuffer[keyConsoleEditLine]);
		keyConsoleEditLine = (keyConsoleEditLine + 1) & 31;
		keyConsoleHistoryLine = keyConsoleEditLine;
		keyConsoleBuffer[keyConsoleEditLine][0] = ']';
		keyConsoleBuffer[keyConsoleEditLine][1] = 0;
		keyConsoleLinePos = 1;

		if (cls.connState == CA_DISCONNECTED)
			SCR_UpdateScreen ();	// force an update, because the command may take some time
		return;
	}

	if (key == K_TAB) {
		// command completion
		Key_CompleteCommand ();
		return;
	}
	
	if ((key == K_LEFTARROW) || (key == K_KP_LEFTARROW) || ((key == 'h') && keysDown[K_CTRL])) {
		int		charcount;
		// jump over invisible color sequences
		charcount = Q_ColorCharCount (keyConsoleBuffer[keyConsoleEditLine], keyConsoleLinePos);
		if (charcount > 1)
			keyConsoleLinePos = Q_ColorCharOffset (keyConsoleBuffer[keyConsoleEditLine], charcount - 1);

		return;
	}

	if (key == K_BACKSPACE) {
		if (keyConsoleLinePos > 1) {
			// skip to the end of color sequence
			while (Q_IsColorString (keyConsoleBuffer[keyConsoleEditLine] + keyConsoleLinePos))
				keyConsoleLinePos += 2;

			strcpy (keyConsoleBuffer[keyConsoleEditLine] + keyConsoleLinePos - 1, keyConsoleBuffer[keyConsoleEditLine] + keyConsoleLinePos);
			keyConsoleLinePos--;
		}

		return;
	}

	if (key == K_DEL) {
		if (keyConsoleLinePos < strlen (keyConsoleBuffer[keyConsoleEditLine]))
			strcpy (keyConsoleBuffer[keyConsoleEditLine] + keyConsoleLinePos, keyConsoleBuffer[keyConsoleEditLine] + keyConsoleLinePos + 1);
		return;
	}

	if (key == K_INS) {
		keyInsertOn = !keyInsertOn;
		return;
	}

	if (key == K_RIGHTARROW) {
		if (strlen (keyConsoleBuffer[keyConsoleEditLine]) == keyConsoleLinePos) {
			if (strlen (keyConsoleBuffer[(keyConsoleEditLine + 31) & 31]) <= keyConsoleLinePos)
				return;

			keyConsoleBuffer[keyConsoleEditLine][keyConsoleLinePos] = keyConsoleBuffer[(keyConsoleEditLine + 31) & 31][keyConsoleLinePos];
			keyConsoleLinePos++;
			keyConsoleBuffer[keyConsoleEditLine][keyConsoleLinePos] = 0;
		} else {
			int		charcount;
			// jump over invisible color sequences
			charcount = Q_ColorCharCount (keyConsoleBuffer[keyConsoleEditLine], keyConsoleLinePos);
			keyConsoleLinePos = Q_ColorCharOffset (keyConsoleBuffer[keyConsoleEditLine], charcount + 1);
		}
		return;
	}

	if ((key == K_UPARROW) || (key == K_KP_UPARROW) || ((key == 'p') && keysDown[K_CTRL])) {
		do {
			keyConsoleHistoryLine = (keyConsoleHistoryLine - 1) & 31;
		} while ((keyConsoleHistoryLine != keyConsoleEditLine) && !keyConsoleBuffer[keyConsoleHistoryLine][1]);

		if (keyConsoleHistoryLine == keyConsoleEditLine)
			keyConsoleHistoryLine = (keyConsoleEditLine + 1) & 31;

		strcpy (keyConsoleBuffer[keyConsoleEditLine], keyConsoleBuffer[keyConsoleHistoryLine]);
		keyConsoleLinePos = strlen (keyConsoleBuffer[keyConsoleEditLine]);
		return;
	}

	if ((key == K_DOWNARROW) || (key == K_KP_DOWNARROW) || ((key == 'n') && keysDown[K_CTRL])) {
		if (keyConsoleHistoryLine == keyConsoleEditLine)
			return;

		do {
			keyConsoleHistoryLine = (keyConsoleHistoryLine + 1) & 31;
		}
		while ((keyConsoleHistoryLine != keyConsoleEditLine) && !keyConsoleBuffer[keyConsoleHistoryLine][1]);

		if (keyConsoleHistoryLine == keyConsoleEditLine) {
			keyConsoleBuffer[keyConsoleEditLine][0] = ']';
			keyConsoleBuffer[keyConsoleEditLine][1] = 0;
			keyConsoleLinePos = 1;
		} else {
			strcpy (keyConsoleBuffer[keyConsoleEditLine], keyConsoleBuffer[keyConsoleHistoryLine]);
			keyConsoleLinePos = strlen (keyConsoleBuffer[keyConsoleEditLine]);
		}
		return;
	}

	if ((key == K_PGUP) || (key == K_KP_PGUP) || (key == K_MWHEELUP)) {
		con.display -= con_scroll->integer;
		return;
	}

	if ((key == K_PGDN) || (key == K_KP_PGDN) || (key == K_MWHEELDOWN)) {
		con.display += con_scroll->integer;
		if (con.display > con.currentLine)
			con.display = con.currentLine;
		return;
	}

	if ((key == K_HOME) || (key == K_KP_HOME)) {
		con.display = con.currentLine - con.totalLines + 10;
		return;
	}

	if ((key == K_END) || (key == K_KP_END)) {
		con.display = con.currentLine;
		return;
	}

	if ((key < 32) || (key > 127))
		return;	// non printable

	if (keyConsoleLinePos < MAXCMDLINE-1) {
		int i;

		// check insert mode
		if (keyInsertOn) {
			// can't do strcpy to move string to right
			i = strlen (keyConsoleBuffer[keyConsoleEditLine]) - 1;

			if (i == 254) 
				i--;

			for ( ; i >= keyConsoleLinePos; i--)
				keyConsoleBuffer[keyConsoleEditLine][i + 1] = keyConsoleBuffer[keyConsoleEditLine][i];
		}

		// only null terminate if at the end
		i = keyConsoleBuffer[keyConsoleEditLine][keyConsoleLinePos];
		keyConsoleBuffer[keyConsoleEditLine][keyConsoleLinePos] = key;
		keyConsoleLinePos++;
		if (!i)
			keyConsoleBuffer[keyConsoleEditLine][keyConsoleLinePos] = 0;	
	}
}

//============================================================================

/*
===================
Key_StringToKeynum

Returns a key number to be used to index keyBindings[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.
===================
*/
int Key_StringToKeynum (char *str) {
	keyName_t	*kn;
	
	if (!str || !str[0])
		return -1;
	if (!str[1])
		return (int)(byte)str[0];

	for (kn=key_Names ; kn->name ; kn++) {
		if (!Q_stricmp (str, kn->name))
			return kn->keyNum;
	}

	return -1;
}


/*
===================
Key_KeynumToString

Returns a string (either a single ascii char, or a K_* name) for the
given keynum.
FIXME: handle quote special (general escape sequence?)
===================
*/
char *Key_KeynumToString (int keyNum) {
	keyName_t		*kn;	
	static	char	tinystr[2];
	
	if (keyNum == -1)
		return "<KEY NOT FOUND>";

	if ((keyNum > 32) && (keyNum < 127)) {
		// printable ascii
		tinystr[0] = keyNum;
		tinystr[1] = 0;
		return tinystr;
	}
	
	for (kn=key_Names ; kn->name ; kn++) {
		if (keyNum == kn->keyNum)
			return kn->name;
	}

	return "<UNKNOWN KEYNUM>";
}


/*
===================
Key_Event

Called by the system between frames for both key up and key down events
Should NOT be called during an interrupt!
===================
*/
char *Key_FallBackKey (int keyNum) {
	if ((keyNum == K_LSHIFT) || (keyNum == K_RSHIFT)) {
		if (!keyBindings[keyNum])
			return keyBindings[K_SHIFT];
	}

	return keyBindings[keyNum];
}

void Key_Event (int keyNum, qBool isDown, uInt time) {
	char	*kb;
	char	cmd[1024];

	// hack for modal presses
	if (keyWaiting == -1) {
		if (isDown)
			keyWaiting = keyNum;
		return;
	}

	// update auto-repeat status
	if (isDown) {
		keysRepeated[keyNum]++;

		if (keysRepeated[keyNum] > 1) {
			if (
					(keyNum != K_BACKSPACE)	&& (keyNum != K_DEL)			&&
					(keyNum != K_LEFTARROW)	&& (keyNum != K_RIGHTARROW)		&&
					(keyNum != K_UPARROW)	&& (keyNum != K_DOWNARROW)		&&
					(keyNum != K_PGUP)		&& (keyNum != K_PGDN)			&&
					(keyNum != K_KP_PGUP)	&& (keyNum != K_KP_PGDN)		&&
					(
						(keyNum < 32)		|| (keyNum > 126)
					)
				)
				return;	// ignore most autorepeats
		}

		if ((keyNum >= 200) && !keyBindings[keyNum])
			Com_Printf (0, "%s is unbound.\n", Key_KeynumToString (keyNum));
	} else
		keysRepeated[keyNum] = 0;

	if ((keyNum == K_SHIFT) || (keyNum == K_LSHIFT) || (keyNum == K_RSHIFT))
		keyShiftDown = isDown;
	keyCapslockOn = Key_CapslockState ();

	// console key is hardcoded, so the user can never unbind it
	if (!keyShiftDown && ((keyNum == '`') || (keyNum == '~'))) {
		if (!isDown)
			return;

		Con_ToggleConsole_f ();

		return;
	}

	// any key during the attract mode will bring up the menu
	if ((cl.attractLoop && (cls.connState == CA_ACTIVE)) && !Key_KeyDest (KD_MENU) && !((keyNum >= K_F1) && (keyNum <= K_F12))) {
		if (!isDown)
			return;

		keyNum = K_ESCAPE;
	}

	// menu key is hardcoded, so the user can never unbind it
	if (keyNum == K_ESCAPE) {
		if (!isDown)
			return;

		switch (cls.keyDest) {
		case KD_MESSAGE:
			Key_Message (keyNum);
			break;
		case KD_MENU:
			CL_UIModule_Keydown (keyNum);
			break;
		case KD_GAME:
			if (cl.attractLoop) {
				// kill the cinematic
				Cbuf_AddText ("killserver\n");
			} else if (cl.frame.playerState.stats[STAT_LAYOUTS]) {
				// put away help computer / inventory
				Cbuf_AddText ("cmd putaway\n");
			} else {
				// bring up the menu
				CL_UIModule_MainMenu ();
			}
			break;
		case KD_CONSOLE:
			Con_ToggleConsole_f ();
			break;
		default:
			Com_Error (ERR_FATAL, "Bad cls.keyDest");
		}
		return;
	}

	// track if any key is down for BUTTON_ANY
	keysDown[keyNum] = isDown;
	if (isDown) {
		if (keysRepeated[keyNum] == 1)
			keyAnyKeyDown++;
	} else {
		keyAnyKeyDown--;
		if (keyAnyKeyDown < 0)
			keyAnyKeyDown = 0;
	}

	/*
	** key up events only generate commands if the game key binding is a button command (leading + sign).
	** These will occur even in console mode, to keep the character from continuing an action started
	** before a console switch. Button commands include the kenum as a parameter, so multiple downs can
	** be matched with ups
	*/
	if (!isDown) {
		kb = Key_FallBackKey (keyNum);
		if (kb && (kb[0] == '+')) {
			Q_snprintfz (cmd, sizeof (cmd), "-%s %i %i\n", kb+1, keyNum, time);
			Cbuf_AddText (cmd);
		}

		if (keyShiftKeys[keyNum] != keyNum) {
			kb = keyBindings[keyShiftKeys[keyNum]];
			if (kb && (kb[0] == '+')) {
				Q_snprintfz (cmd, sizeof (cmd), "-%s %i %i\n", kb+1, keyNum, time);
				Cbuf_AddText (cmd);
			}
		}
		return;
	}

	// if not a consolekey, send to the interpreter no matter what mode is
	if (
		(Key_KeyDest (KD_MENU) && keyMenuBound[keyNum])						||
		(Key_KeyDest (KD_CONSOLE) && !keyShiftDown && !keyConsoleKeys[keyNum])	||
		(Key_KeyDest (KD_GAME) && ((cls.connState == CA_ACTIVE) || (!keyShiftDown && !keyConsoleKeys[keyNum])))
		) {
		kb = Key_FallBackKey (keyNum);
		if (kb) {
			if (kb[0] == '+') {
				// button commands add keynum and time as a parm
				Q_snprintfz (cmd, sizeof (cmd), "%s %i %i\n", kb, keyNum, time);
				Cbuf_AddText (cmd);
			} else {
				Cbuf_AddText (kb);
				Cbuf_AddText ("\n");
			}
		}
		return;
	}

	// other systems only care about key down events
	if (!isDown)
		return;

	if (keyShiftDown ^ (keyCapslockOn && (keyNum >= 'a') && (keyNum <= 'z')))
		keyNum = keyShiftKeys[keyNum];

	switch (cls.keyDest) {
	case KD_MESSAGE:
		Key_Message (keyNum);
		break;
	case KD_MENU:
		CL_UIModule_Keydown (keyNum);
		break;
	case KD_GAME:
	case KD_CONSOLE:
		Key_Console (keyNum);
		break;
	default:
		Com_Error (ERR_FATAL, "Bad cls.keyDest");
	}
}


/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates (void) {
	int		i;

	keyAnyKeyDown = 0;

	for (i=0 ; i<K_MAXKEYS ; i++) {
		if (keysDown[i] || keysRepeated[i])
			Key_Event (i, qFalse, 0);

		keysDown[i] = 0;
		keysRepeated[i] = 0;
	}
}


/*
================
Key_ClearTyping
================
*/
void Key_ClearTyping (void) {
	keyConsoleBuffer[keyConsoleEditLine][1] = 0;	// clear any typing
	keyConsoleLinePos = 1;
}


/*
===================
Key_GetKey
===================
*/
int Key_GetKey (void) {
	keyWaiting = -1;

	while (keyWaiting == -1)
		Sys_SendKeyEvents ();

	return keyWaiting;
}

/*
===============================================================================

	KEY BINDING

===============================================================================
*/

/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding (int keyNum, char *binding) {
	char	*newbind;
	int		len;

	if (keyNum == -1)
		return;

	// free old bindings
	if (keyBindings[keyNum]) {
		Mem_Free (keyBindings[keyNum]);
		keyBindings[keyNum] = NULL;
	}

	// allocate memory for new binding
	len = strlen (binding);	
	newbind = Mem_Alloc (len+1);
	strcpy (newbind, binding);
	newbind[len] = 0;
	keyBindings[keyNum] = newbind;	
}


/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings (FILE *f) {
	int		i;

	for (i=0 ; i<K_MAXKEYS ; i++) {
		if (keyBindings[i] && keyBindings[i][0])
			fprintf (f, "bind %s \"%s\"\n", Key_KeynumToString (i), keyBindings[i]);
	}
}


/*
===================
Key_GetBindingBuf
===================
*/
char *Key_GetBindingBuf (int binding) {
	return keyBindings[binding];
}

/*
===================
Key_IsDown
===================
*/
qBool Key_IsDown (int keyNum) {
	if ((keyNum < 0) || (keyNum >= K_MAXKEYS))
		return qFalse;

	return keysDown[keyNum];
}

/*
===============================================================================

	CONSOLE COMMANDS

===============================================================================
*/

/*
===================
Key_Unbind_f
===================
*/
static void Key_Unbind_f (void) {
	int		b;

	if (Cmd_Argc () != 2) {
		Com_Printf (0, "unbind <key> : remove commands from a key\n");
		return;
	}
	
	b = Key_StringToKeynum (Cmd_Argv (1));
	if (b == -1) {
		Com_Printf (0, "\"%s\" isn't a valid key\n", Cmd_Argv (1));
		return;
	}

	Key_SetBinding (b, "");
}


/*
====================
Key_Unbindall_f
====================
*/
static void Key_Unbindall_f (void) {
	int		i;
	
	for (i=0 ; i<K_MAXKEYS ; i++) {
		if (keyBindings[i])
			Key_SetBinding (i, "");
	}
}


/*
===================
Key_Bind_f
===================
*/
static void Key_Bind_f (void) {
	int			i, c, b;
	char		cmd[1024];
	
	c = Cmd_Argc ();

	if (c < 2) {
		Com_Printf (0, "bind <key> [command] : attach a command to a key\n");
		return;
	}

	b = Key_StringToKeynum (Cmd_Argv (1));
	if (b == -1) {
		Com_Printf (0, "\"%s\" isn't a valid key\n", Cmd_Argv (1));
		return;
	}

	if (c == 2) {
		if (keyBindings[b])
			Com_Printf (0, "\"%s\" = \"%s\"\n", Cmd_Argv (1), keyBindings[b]);
		else
			Com_Printf (0, "\"%s\" is not bound\n", Cmd_Argv (1));
		return;
	}
	
	// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	for (i=2 ; i<c ; i++) {
		strcat (cmd, Cmd_Argv (i));
		if (i != (c-1))
			strcat (cmd, " ");
	}

	Key_SetBinding (b, cmd);
}


/*
============
Key_Bindlist_f
============
*/
static void Key_Bindlist_f (void) {
	int		i;
	char	*wildCard;

	if (Cmd_Argc () == 2)
		wildCard = Q_VarArgs ("*%s*", Cmd_Argv (1));
	else
		wildCard = "*";

	for (i=0 ; i<K_MAXKEYS ; i++) {
		if (keyBindings[i] && keyBindings[i][0] && Q_WildCmp (wildCard, keyBindings[i], 1))
			Com_Printf (0, "%s \"%s\"\n", Key_KeynumToString(i), keyBindings[i]);
	}
}

/*
===============================================================================

	INIT / SHUTDOWN

===============================================================================
*/

/*
===================
Key_Init
===================
*/
void Key_Init (void) {
	int		i;

	//
	// add commands
	//
	Cmd_AddCommand ("bind",			Key_Bind_f,			"Bind a key to do something");
	Cmd_AddCommand ("unbind",		Key_Unbind_f,		"Remove a key binding");
	Cmd_AddCommand ("unbindall",	Key_Unbindall_f,	"Remove all key bindings");
	Cmd_AddCommand ("bindlist",		Key_Bindlist_f,		"Prints a list of key bindings");

	//
	// clear them all
	//
	memset (&keyBindings, 0, sizeof (keyBindings));
	memset (&keyConsoleKeys, qFalse, sizeof (keyConsoleKeys));
	memset (&keyShiftKeys, 0, sizeof (keyShiftKeys));
	memset (&keyMenuBound, qFalse, sizeof (keyMenuBound));

	//
	// set defaults
	//
	for (i=0 ; i<32 ; i++) {
		keyConsoleBuffer[i][0] = ']';
		keyConsoleBuffer[i][1] = 0;

		keyChatBuffer[i][0] = 0;
	}
	keyConsoleLinePos = 1;
	keyChatLinePos = 0;

	//
	// ascii chars are allowed in the console
	//
	for (i=32 ; i<128 ; i++)
		keyConsoleKeys[i] = qTrue;

	//
	// other keys allowed to be typed in the console
	//
	keyConsoleKeys[K_ENTER			] = qTrue;
	keyConsoleKeys[K_KP_ENTER		] = qTrue;
	keyConsoleKeys[K_TAB			] = qTrue;
	keyConsoleKeys[K_LEFTARROW		] = qTrue;
	keyConsoleKeys[K_KP_LEFTARROW	] = qTrue;
	keyConsoleKeys[K_RIGHTARROW	] = qTrue;
	keyConsoleKeys[K_KP_RIGHTARROW	] = qTrue;
	keyConsoleKeys[K_UPARROW		] = qTrue;
	keyConsoleKeys[K_KP_UPARROW	] = qTrue;
	keyConsoleKeys[K_DOWNARROW		] = qTrue;
	keyConsoleKeys[K_KP_DOWNARROW	] = qTrue;
	keyConsoleKeys[K_BACKSPACE		] = qTrue;
	keyConsoleKeys[K_HOME			] = qTrue;
	keyConsoleKeys[K_KP_HOME		] = qTrue;
	keyConsoleKeys[K_END			] = qTrue;
	keyConsoleKeys[K_KP_END		] = qTrue;
	keyConsoleKeys[K_PGUP			] = qTrue;
	keyConsoleKeys[K_KP_PGUP		] = qTrue;
	keyConsoleKeys[K_PGDN			] = qTrue;
	keyConsoleKeys[K_KP_PGDN		] = qTrue;

	keyConsoleKeys[K_SHIFT			] = qTrue;
	keyConsoleKeys[K_LSHIFT		] = qTrue;
	keyConsoleKeys[K_RSHIFT		] = qTrue;

	keyConsoleKeys[K_INS			] = qTrue;
	keyConsoleKeys[K_DEL			] = qTrue;
	keyConsoleKeys[K_KP_INS		] = qTrue;
	keyConsoleKeys[K_KP_DEL		] = qTrue;
	keyConsoleKeys[K_KP_SLASH		] = qTrue;
	keyConsoleKeys[K_KP_PLUS		] = qTrue;
	keyConsoleKeys[K_KP_MINUS		] = qTrue;
	keyConsoleKeys[K_KP_FIVE		] = qTrue;

	keyConsoleKeys[K_MWHEELUP		] = qTrue;
	keyConsoleKeys[K_MWHEELDOWN	] = qTrue;
	keyConsoleKeys[K_MWHEELLEFT	] = qTrue;
	keyConsoleKeys[K_MWHEELRIGHT	] = qTrue;

	keyConsoleKeys['`'] = qFalse;
	keyConsoleKeys['~'] = qFalse;

	//
	// set what a key looks like when shift is held
	//
	for (i=0 ; i<K_MAXKEYS ; i++)
		keyShiftKeys[i] = i;

	for (i='a' ; i<='z' ; i++)
		keyShiftKeys[i] = i - 'a' + 'A';

	keyShiftKeys['1' ]	= '!';
	keyShiftKeys['2' ]	= '@';
	keyShiftKeys['3' ]	= '#';
	keyShiftKeys['4' ]	= '$';
	keyShiftKeys['5' ]	= '%';
	keyShiftKeys['6' ]	= '^';
	keyShiftKeys['7' ]	= '&';
	keyShiftKeys['8' ]	= '*';
	keyShiftKeys['9' ]	= '(';
	keyShiftKeys['0' ]	= ')';
	keyShiftKeys['-' ]	= '_';
	keyShiftKeys['=' ]	= '+';
	keyShiftKeys[',' ]	= '<';
	keyShiftKeys['.' ]	= '>';
	keyShiftKeys['/' ]	= '?';
	keyShiftKeys[';' ]	= ':';
	keyShiftKeys['\'']	= '"';
	keyShiftKeys['[' ]	= '{';
	keyShiftKeys[']' ]	= '}';
	keyShiftKeys['`' ]	= '~';
	keyShiftKeys['\\']	= '|';

	//
	// keys with binds not allowed to be used while in the menu
	//
	keyMenuBound[K_F1 ] = qTrue;
	keyMenuBound[K_F2 ] = qTrue;
	keyMenuBound[K_F3 ] = qTrue;
	keyMenuBound[K_F4 ] = qTrue;
	keyMenuBound[K_F5 ] = qTrue;
	keyMenuBound[K_F6 ] = qTrue;
	keyMenuBound[K_F7 ] = qTrue;
	keyMenuBound[K_F8 ] = qTrue;
	keyMenuBound[K_F9 ] = qTrue;
	keyMenuBound[K_F10] = qTrue;
	keyMenuBound[K_F11] = qTrue;
	keyMenuBound[K_F12] = qTrue;
}


/*
===================
Key_Shutdown
===================
*/
void Key_Shutdown (void) {
}
