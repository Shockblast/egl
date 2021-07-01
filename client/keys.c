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

#include "cl_local.h"

qBool		key_ShiftDown = qFalse;
qBool		key_CapslockOn = qFalse;
qBool		key_InsertOn = qTrue;

int			key_AnyDown;
int			key_Waiting;

char		*key_Bindings[K_MAXKEYS];
qBool		key_Down[K_MAXKEYS];		// key up events are sent even if in console mode
int			key_Repeats[K_MAXKEYS];		// if > 1, it is autorepeating
qBool		key_ConsoleKeys[K_MAXKEYS];	// if qTrue, can't be typed while in the console
int			key_ShiftKeys[K_MAXKEYS];	// key to map to if shift held down when this key is pressed
qBool		key_MenuBound[K_MAXKEYS];	// if qTrue, can't be rebound while in menu

char		key_Buffer[32][MAXCMDLINE];
int			key_LinePos = 1;
int			key_EditLine = 0;
int			key_HistoryLine = 0;

qBool		chat_Team;
char		chat_Buffer[32][MAXCMDLINE];
int			chat_LinePos = 0;
int			chat_EditLine = 0;
int			chat_HistoryLine = 0;

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
void Key_CompleteCommand (void) {
	char	*cmd, *s;

	s = key_Buffer[key_EditLine]+1;
	if ((*s == '\\') || (*s == '/'))
		s++;

	cmd = Cmd_CompleteCommand (s);
	if (!cmd)
		cmd = Cvar_CompleteVariable (s);
	if (cmd) {
		key_Buffer[key_EditLine][1] = '/';
		strcpy (key_Buffer[key_EditLine]+2, cmd);
		key_LinePos = strlen (cmd) + 2;
		key_Buffer[key_EditLine][key_LinePos] = ' ';
		key_LinePos++;
		key_Buffer[key_EditLine][key_LinePos] = 0;
		return;
	}
}


/*
====================
Key_Message
====================
*/
void Key_Message (int key) {
	if ((toupper (key) == 'V' && key_Down[K_CTRL]) || (((key == K_INS) || (key == K_KP_INS)) && KEY_SHIFTDOWN (key_Down))) {
		char *cbd;
		
		if ((cbd = Sys_GetClipboardData ()) != 0) {
			int i;

			strtok (cbd, "\n\r\b");

			i = strlen (cbd);
			if (i + chat_LinePos >= MAXCMDLINE)
				i= MAXCMDLINE - chat_LinePos;

			if (i > 0) {
				cbd[i] = 0;
				strcat (chat_Buffer[chat_EditLine], cbd);
				chat_LinePos += i;
			}
			free (cbd);
		}

		return;
	}

	if ((key == 'l') && (key_Down[K_CTRL])) {
		chat_Buffer[chat_EditLine][0] = 0;
		chat_LinePos = 0;
		return;
	}

	if ((key == K_ENTER) || (key == K_KP_ENTER)) {
		if (chat_Team)
			Cbuf_AddText ("say_team \"");
		else
			Cbuf_AddText ("say \"");

		Cbuf_AddText (chat_Buffer[chat_EditLine]);
		Cbuf_AddText ("\"\n");

		chat_EditLine = (chat_EditLine + 1) & 31;
		chat_HistoryLine = chat_EditLine;
		chat_Buffer[chat_EditLine][0] = 0;
		chat_LinePos = 0;

		Key_SetKeyDest (KD_GAME);
		return;
	}

	if (key == K_ESCAPE) {
		chat_HistoryLine = chat_EditLine;
		chat_Buffer[chat_EditLine][0] = 0;
		chat_LinePos = 0;

		Key_SetKeyDest (KD_GAME);
		return;
	}

	if ((key == K_LEFTARROW) || (key == K_KP_LEFTARROW) || ((key == 'h') && key_Down[K_CTRL])) {
		int		charcount;
		// jump over invisible color sequences
		charcount = Q_ColorCharCount (chat_Buffer[chat_EditLine], chat_LinePos);
		if (charcount > 0)
			chat_LinePos = Q_ColorCharOffset (chat_Buffer[chat_EditLine], charcount - 1);
		return;
	}

	if (key == K_BACKSPACE) {
		if (chat_LinePos > 0) {
			// skip to the end of color sequence
			while (Q_IsColorString (chat_Buffer[chat_EditLine] + chat_LinePos))
				chat_LinePos += 2;

			strcpy (chat_Buffer[chat_EditLine] + chat_LinePos - 1, chat_Buffer[chat_EditLine] + chat_LinePos);
			chat_LinePos--;
		}

		return;
	}

	if (key == K_DEL) {
		if (chat_LinePos < strlen (chat_Buffer[chat_EditLine]))
			strcpy (chat_Buffer[chat_EditLine] + chat_LinePos, chat_Buffer[chat_EditLine] + chat_LinePos + 1);
		return;
	}

	if (key == K_INS) {
		key_InsertOn = !key_InsertOn;
		return;
	}

	if (key == K_RIGHTARROW) {
		if (strlen (chat_Buffer[chat_EditLine]) == chat_LinePos) {
			if (strlen (chat_Buffer[(chat_EditLine + 31) & 31]) <= chat_LinePos)
				return;

			chat_Buffer[chat_EditLine][chat_LinePos] = chat_Buffer[(chat_EditLine + 31) & 31][chat_LinePos];
			chat_LinePos++;
			chat_Buffer[chat_EditLine][chat_LinePos] = 0;
		} else {
			int		charcount;
			// jump over invisible color sequences
			charcount = Q_ColorCharCount (chat_Buffer[chat_EditLine], chat_LinePos);
			chat_LinePos = Q_ColorCharOffset (chat_Buffer[chat_EditLine], charcount + 1);
		}
		return;
	}

	if ((key == K_UPARROW) || (key == K_KP_UPARROW) || ((key == 'p') && key_Down[K_CTRL])) {
		do {
			chat_HistoryLine = (chat_HistoryLine - 1) & 31;
		} while ((chat_HistoryLine != chat_EditLine) && !chat_Buffer[chat_HistoryLine][0]);

		if (chat_HistoryLine == chat_EditLine)
			chat_HistoryLine = (chat_EditLine + 1) & 31;

		strcpy (chat_Buffer[chat_EditLine], chat_Buffer[chat_HistoryLine]);
		chat_LinePos = strlen (chat_Buffer[chat_EditLine]);
		return;
	}

	if ((key == K_DOWNARROW) || (key == K_KP_DOWNARROW) || ((key == 'n') && key_Down[K_CTRL])) {
		if (chat_HistoryLine == chat_EditLine)
			return;

		do {
			chat_HistoryLine = (chat_HistoryLine + 1) & 31;
		}
		while ((chat_HistoryLine != chat_EditLine) && !chat_Buffer[chat_HistoryLine][0]);

		if (chat_HistoryLine == chat_EditLine) {
			chat_Buffer[chat_EditLine][0] = 0;
			chat_LinePos = 0;
		} else {
			strcpy (chat_Buffer[chat_EditLine], chat_Buffer[chat_HistoryLine]);
			chat_LinePos = strlen (chat_Buffer[chat_EditLine]);
		}
		return;
	}

	if ((key == K_HOME) || (key == K_KP_HOME)) {
		chat_LinePos = 0;
		return;
	}

	if ((key == K_END) || (key == K_KP_END)) {
		chat_LinePos = strlen (chat_Buffer[chat_EditLine]);
		return;
	}

	if ((key < 32) || (key > 127))
		return;	// non printable

	if (chat_LinePos < MAXCMDLINE-1) {
		int i;

		// check insert mode
		if (key_InsertOn) {
			// can't do strcpy to move string to right
			i = strlen (chat_Buffer[chat_EditLine]) - 1;

			if (i == 254) 
				i--;

			for ( ; i >= chat_LinePos; i--)
				chat_Buffer[chat_EditLine][i + 1] = chat_Buffer[chat_EditLine][i];
		}

		// only null terminate if at the end
		i = chat_Buffer[chat_EditLine][chat_LinePos];
		chat_Buffer[chat_EditLine][chat_LinePos] = key;
		chat_LinePos++;
		if (!i)
			chat_Buffer[chat_EditLine][chat_LinePos] = 0;	
	}
}


/*
====================
Key_Console

Interactive line editing and console scrollback
====================
*/
void Key_Console (int key) {
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

	if ((toupper (key) == 'V' && key_Down[K_CTRL]) || (((key == K_INS) || (key == K_KP_INS)) && KEY_SHIFTDOWN (key_Down))) {
		char *cbd;
		
		if ((cbd = Sys_GetClipboardData ()) != 0) {
			int i;

			strtok (cbd, "\n\r\b");

			i = strlen (cbd);
			if (i + key_LinePos >= MAXCMDLINE)
				i= MAXCMDLINE - key_LinePos;

			if (i > 0) {
				cbd[i] = 0;
				strcat (key_Buffer[key_EditLine], cbd);
				key_LinePos += i;
			}
			free (cbd);
		}

		return;
	}

	if ((key == 'l') && (key_Down[K_CTRL])) {
		Cbuf_AddText ("clear\n");
		return;
	}

	if ((key == K_ENTER) || (key == K_KP_ENTER)) {
		// backslash text are commands, else chat
		if ((key_Buffer[key_EditLine][1] == '\\') || (key_Buffer[key_EditLine][1] == '/'))
			Cbuf_AddText (key_Buffer[key_EditLine]+2);	// skip the >
		else
			Cbuf_AddText (key_Buffer[key_EditLine]+1);	// valid command

		Cbuf_AddText ("\n");
		Com_Printf (PRINT_ALL, "%s\n", key_Buffer[key_EditLine]);
		key_EditLine = (key_EditLine + 1) & 31;
		key_HistoryLine = key_EditLine;
		key_Buffer[key_EditLine][0] = ']';
		key_Buffer[key_EditLine][1] = 0;
		key_LinePos = 1;

		if (CL_ConnectionState (CA_DISCONNECTED))
			SCR_UpdateScreen ();	// force an update, because the command may take some time
		return;
	}

	if (key == K_TAB) {
		// command completion
		Key_CompleteCommand ();
		return;
	}
	
	if ((key == K_LEFTARROW) || (key == K_KP_LEFTARROW) || ((key == 'h') && key_Down[K_CTRL])) {
		int		charcount;
		// jump over invisible color sequences
		charcount = Q_ColorCharCount (key_Buffer[key_EditLine], key_LinePos);
		if (charcount > 1)
			key_LinePos = Q_ColorCharOffset (key_Buffer[key_EditLine], charcount - 1);

		return;
	}

	if (key == K_BACKSPACE) {
		if (key_LinePos > 1) {
			// skip to the end of color sequence
			while (Q_IsColorString (key_Buffer[key_EditLine] + key_LinePos))
				key_LinePos += 2;

			strcpy (key_Buffer[key_EditLine] + key_LinePos - 1, key_Buffer[key_EditLine] + key_LinePos);
			key_LinePos--;
		}

		return;
	}

	if (key == K_DEL) {
		if (key_LinePos < strlen (key_Buffer[key_EditLine]))
			strcpy (key_Buffer[key_EditLine] + key_LinePos, key_Buffer[key_EditLine] + key_LinePos + 1);
		return;
	}

	if (key == K_INS) {
		key_InsertOn = !key_InsertOn;
		return;
	}

	if (key == K_RIGHTARROW) {
		if (strlen (key_Buffer[key_EditLine]) == key_LinePos) {
			if (strlen (key_Buffer[(key_EditLine + 31) & 31]) <= key_LinePos)
				return;

			key_Buffer[key_EditLine][key_LinePos] = key_Buffer[(key_EditLine + 31) & 31][key_LinePos];
			key_LinePos++;
			key_Buffer[key_EditLine][key_LinePos] = 0;
		} else {
			int		charcount;
			// jump over invisible color sequences
			charcount = Q_ColorCharCount (key_Buffer[key_EditLine], key_LinePos);
			key_LinePos = Q_ColorCharOffset (key_Buffer[key_EditLine], charcount + 1);
		}
		return;
	}

	if ((key == K_UPARROW) || (key == K_KP_UPARROW) || ((key == 'p') && key_Down[K_CTRL])) {
		do {
			key_HistoryLine = (key_HistoryLine - 1) & 31;
		} while ((key_HistoryLine != key_EditLine) && !key_Buffer[key_HistoryLine][1]);

		if (key_HistoryLine == key_EditLine)
			key_HistoryLine = (key_EditLine + 1) & 31;

		strcpy (key_Buffer[key_EditLine], key_Buffer[key_HistoryLine]);
		key_LinePos = strlen (key_Buffer[key_EditLine]);
		return;
	}

	if ((key == K_DOWNARROW) || (key == K_KP_DOWNARROW) || ((key == 'n') && key_Down[K_CTRL])) {
		if (key_HistoryLine == key_EditLine)
			return;

		do {
			key_HistoryLine = (key_HistoryLine + 1) & 31;
		}
		while ((key_HistoryLine != key_EditLine) && !key_Buffer[key_HistoryLine][1]);

		if (key_HistoryLine == key_EditLine) {
			key_Buffer[key_EditLine][0] = ']';
			key_Buffer[key_EditLine][1] = 0;
			key_LinePos = 1;
		} else {
			strcpy (key_Buffer[key_EditLine], key_Buffer[key_HistoryLine]);
			key_LinePos = strlen (key_Buffer[key_EditLine]);
		}
		return;
	}

	if ((key == K_PGUP) || (key == K_KP_PGUP) || (key == K_MWHEELUP)) {
		con.display -= con_scroll->integer;
		return;
	}

	if ((key == K_PGDN) || (key == K_KP_PGDN) || (key == K_MWHEELDOWN)) {
		con.display += con_scroll->integer;
		if (con.display > con.current)
			con.display = con.current;
		return;
	}

	if ((key == K_HOME) || (key == K_KP_HOME)) {
		con.display = con.current - con.totalLines + 10;
		return;
	}

	if ((key == K_END) || (key == K_KP_END)) {
		con.display = con.current;
		return;
	}

	if ((key < 32) || (key > 127))
		return;	// non printable

	if (key_LinePos < MAXCMDLINE-1) {
		int i;

		// check insert mode
		if (key_InsertOn) {
			// can't do strcpy to move string to right
			i = strlen (key_Buffer[key_EditLine]) - 1;

			if (i == 254) 
				i--;

			for ( ; i >= key_LinePos; i--)
				key_Buffer[key_EditLine][i + 1] = key_Buffer[key_EditLine][i];
		}

		// only null terminate if at the end
		i = key_Buffer[key_EditLine][key_LinePos];
		key_Buffer[key_EditLine][key_LinePos] = key;
		key_LinePos++;
		if (!i)
			key_Buffer[key_EditLine][key_LinePos] = 0;	
	}
}

//============================================================================

/*
===================
Key_StringToKeynum

Returns a key number to be used to index key_Bindings[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.
===================
*/
int Key_StringToKeynum (char *str) {
	keyName_t	*kn;
	
	if (!str || !str[0])
		return -1;
	if (!str[1])
		return str[0];

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
		if (!key_Bindings[keyNum])
			return key_Bindings[K_SHIFT];
	}

	return key_Bindings[keyNum];
}

void Key_Event (int keyNum, qBool isDown, unsigned int time) {
	char	*kb;
	char	cmd[1024];

	// hack for modal presses
	if (key_Waiting == -1) {
		if (isDown)
			key_Waiting = keyNum;
		return;
	}

	// update auto-repeat status
	if (isDown) {
		key_Repeats[keyNum]++;

		if (key_Repeats[keyNum] > 1) {
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

		if ((keyNum >= 200) && !key_Bindings[keyNum])
			Com_Printf (PRINT_ALL, "%s is unbound.\n", Key_KeynumToString (keyNum));
	} else
		key_Repeats[keyNum] = 0;

	if ((keyNum == K_SHIFT) || (keyNum == K_LSHIFT) || (keyNum == K_RSHIFT))
		key_ShiftDown = isDown;
	key_CapslockOn = Key_CapslockState ();

	// console key is hardcoded, so the user can never unbind it
	if (!key_ShiftDown && ((keyNum == '`') || (keyNum == '~'))) {
		if (!isDown)
			return;

		Con_ToggleConsole_f ();

		return;
	}

	// any key during the attract mode will bring up the menu
	if ((cl.attractLoop && CL_ConnectionState (CA_ACTIVE)) && !Key_KeyDest (KD_MENU) && !((keyNum >= K_F1) && (keyNum <= K_F12))) {
		if (!isDown)
			return;

		keyNum = K_ESCAPE;
	}

	// menu key is hardcoded, so the user can never unbind it
	if (keyNum == K_ESCAPE) {
		if (!isDown)
			return;

		if (cl.frame.playerState.stats[STAT_LAYOUTS] && Key_KeyDest (KD_GAME)) {
			// put away help computer / inventory
			Cbuf_AddText ("cmd putaway\n");
			return;
		}

		switch (cls.keyDest) {
		case KD_MESSAGE:
			Key_Message (keyNum);
			break;
		case KD_MENU:
			CL_UIModule_Keydown (keyNum);
			break;
		case KD_GAME:
		case KD_CONSOLE:
			CL_UIModule_MainMenu ();
			break;
		default:
			Com_Error (ERR_FATAL, "Bad cls.keyDest");
		}
		return;
	}

	// track if any key is down for BUTTON_ANY
	key_Down[keyNum] = isDown;
	if (isDown) {
		if (key_Repeats[keyNum] == 1)
			key_AnyDown++;
	} else {
		key_AnyDown--;
		if (key_AnyDown < 0)
			key_AnyDown = 0;
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
			Com_sprintf (cmd, sizeof (cmd), "-%s %i %i\n", kb+1, keyNum, time);
			Cbuf_AddText (cmd);
		}

		if (key_ShiftKeys[keyNum] != keyNum) {
			kb = key_Bindings[key_ShiftKeys[keyNum]];
			if (kb && (kb[0] == '+')) {
				Com_sprintf (cmd, sizeof (cmd), "-%s %i %i\n", kb+1, keyNum, time);
				Cbuf_AddText (cmd);
			}
		}
		return;
	}

	// if not a consolekey, send to the interpreter no matter what mode is
	if (
		(Key_KeyDest (KD_MENU) && key_MenuBound[keyNum])						||
		(Key_KeyDest (KD_CONSOLE) && !key_ShiftDown && !key_ConsoleKeys[keyNum])	||
		(Key_KeyDest (KD_GAME) && (CL_ConnectionState (CA_ACTIVE) || (!key_ShiftDown && !key_ConsoleKeys[keyNum])))
		) {
		kb = Key_FallBackKey (keyNum);
		if (kb) {
			if (kb[0] == '+') {
				// button commands add keynum and time as a parm
				Com_sprintf (cmd, sizeof (cmd), "%s %i %i\n", kb, keyNum, time);
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

	if (key_ShiftDown ^ (key_CapslockOn && (keyNum >= 'a') && (keyNum <= 'z')))
		keyNum = key_ShiftKeys[keyNum];

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

	key_AnyDown = 0;

	for (i=0 ; i<K_MAXKEYS ; i++) {
		if (key_Down[i] || key_Repeats[i])
			Key_Event (i, qFalse, 0);

		key_Down[i] = 0;
		key_Repeats[i] = 0;
	}
}


/*
================
Key_ClearTyping
================
*/
void Key_ClearTyping (void) {
	key_Buffer[key_EditLine][1] = 0;	// clear any typing
	key_LinePos = 1;
}


/*
===================
Key_GetKey
===================
*/
int Key_GetKey (void) {
	key_Waiting = -1;

	while (key_Waiting == -1)
		Sys_SendKeyEvents ();

	return key_Waiting;
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
	int		l;
			
	if (keyNum == -1)
		return;

	// free old bindings
	if (key_Bindings[keyNum]) {
		Z_Free (key_Bindings[keyNum]);
		key_Bindings[keyNum] = NULL;
	}
			
	// allocate memory for new binding
	l = strlen (binding);	
	newbind = Z_TagMalloc (l+1, ZTAG_KEYSYS);
	strcpy (newbind, binding);
	newbind[l] = 0;
	key_Bindings[keyNum] = newbind;	
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
		if (key_Bindings[i] && key_Bindings[i][0]) {
			fprintf (f, "bind %s \"%s\"\n", Key_KeynumToString (i), key_Bindings[i]);
		}
	}
}


/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f (void) {
	int		b;

	if (Cmd_Argc () != 2) {
		Com_Printf (PRINT_ALL, "unbind <key> : remove commands from a key\n");
		return;
	}
	
	b = Key_StringToKeynum (Cmd_Argv (1));
	if (b == -1) {
		Com_Printf (PRINT_ALL, "\"%s\" isn't a valid key\n", Cmd_Argv (1));
		return;
	}

	Key_SetBinding (b, "");
}


/*
====================
Key_Unbindall_f
====================
*/
void Key_Unbindall_f (void) {
	int		i;
	
	for (i=0 ; i<K_MAXKEYS ; i++) {
		if (key_Bindings[i]) {
			Key_SetBinding (i, "");
		}
	}
}


/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f (void) {
	int			i, c, b;
	char		cmd[1024];
	
	c = Cmd_Argc ();

	if (c < 2) {
		Com_Printf (PRINT_ALL, "bind <key> [command] : attach a command to a key\n");
		return;
	}

	b = Key_StringToKeynum (Cmd_Argv (1));
	if (b==-1) {
		Com_Printf (PRINT_ALL, "\"%s\" isn't a valid key\n", Cmd_Argv (1));
		return;
	}

	if (c == 2) {
		if (key_Bindings[b])
			Com_Printf (PRINT_ALL, "\"%s\" = \"%s\"\n", Cmd_Argv (1), key_Bindings[b]);
		else
			Com_Printf (PRINT_ALL, "\"%s\" is not bound\n", Cmd_Argv (1));
		return;
	}
	
	// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	for (i=2 ; i< c ; i++) {
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
void Key_Bindlist_f (void) {
	int		i;

	for (i=0 ; i<K_MAXKEYS ; i++) {
		if (key_Bindings[i] && key_Bindings[i][0]) {
			Com_Printf (PRINT_ALL, "%s \"%s\"\n", Key_KeynumToString(i), key_Bindings[i]);
		}
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

	Cmd_AddCommand ("bind",			Key_Bind_f);
	Cmd_AddCommand ("unbind",		Key_Unbind_f);
	Cmd_AddCommand ("unbindall",	Key_Unbindall_f);
	Cmd_AddCommand ("bindlist",		Key_Bindlist_f);

	for (i=0 ; i<32 ; i++) {
		key_Buffer[i][0]	= ']';
		key_Buffer[i][1]	= 0;

		chat_Buffer[i][0]	= 0;
	}
	key_LinePos = 1;
	chat_LinePos = 0;

	// ascii chars are allowed in the console
	for (i=32 ; i<128 ; i++)
		key_ConsoleKeys[i] = qTrue;

	// other keys allowed to be typed in the console
	key_ConsoleKeys[K_ENTER			] = qTrue;
	key_ConsoleKeys[K_KP_ENTER		] = qTrue;
	key_ConsoleKeys[K_TAB			] = qTrue;
	key_ConsoleKeys[K_LEFTARROW		] = qTrue;
	key_ConsoleKeys[K_KP_LEFTARROW	] = qTrue;
	key_ConsoleKeys[K_RIGHTARROW	] = qTrue;
	key_ConsoleKeys[K_KP_RIGHTARROW	] = qTrue;
	key_ConsoleKeys[K_UPARROW		] = qTrue;
	key_ConsoleKeys[K_KP_UPARROW	] = qTrue;
	key_ConsoleKeys[K_DOWNARROW		] = qTrue;
	key_ConsoleKeys[K_KP_DOWNARROW	] = qTrue;
	key_ConsoleKeys[K_BACKSPACE		] = qTrue;
	key_ConsoleKeys[K_HOME			] = qTrue;
	key_ConsoleKeys[K_KP_HOME		] = qTrue;
	key_ConsoleKeys[K_END			] = qTrue;
	key_ConsoleKeys[K_KP_END		] = qTrue;
	key_ConsoleKeys[K_PGUP			] = qTrue;
	key_ConsoleKeys[K_KP_PGUP		] = qTrue;
	key_ConsoleKeys[K_PGDN			] = qTrue;
	key_ConsoleKeys[K_KP_PGDN		] = qTrue;

	key_ConsoleKeys[K_SHIFT			] = qTrue;
	key_ConsoleKeys[K_LSHIFT		] = qTrue;
	key_ConsoleKeys[K_RSHIFT		] = qTrue;

	key_ConsoleKeys[K_INS			] = qTrue;
	key_ConsoleKeys[K_DEL			] = qTrue;
	key_ConsoleKeys[K_KP_INS		] = qTrue;
	key_ConsoleKeys[K_KP_DEL		] = qTrue;
	key_ConsoleKeys[K_KP_SLASH		] = qTrue;
	key_ConsoleKeys[K_KP_PLUS		] = qTrue;
	key_ConsoleKeys[K_KP_MINUS		] = qTrue;
	key_ConsoleKeys[K_KP_FIVE		] = qTrue;

	key_ConsoleKeys[K_MWHEELUP		] = qTrue;
	key_ConsoleKeys[K_MWHEELDOWN	] = qTrue;
	key_ConsoleKeys[K_MWHEELLEFT	] = qTrue;
	key_ConsoleKeys[K_MWHEELRIGHT	] = qTrue;

	key_ConsoleKeys['`'] = qFalse;
	key_ConsoleKeys['~'] = qFalse;

	// set what a key looks like when shift is held
	for (i=0 ; i<K_MAXKEYS ; i++)
		key_ShiftKeys[i] = i;

	for (i='a' ; i<='z' ; i++)
		key_ShiftKeys[i] = i - 'a' + 'A';

	key_ShiftKeys['1' ]	= '!';
	key_ShiftKeys['2' ]	= '@';
	key_ShiftKeys['3' ]	= '#';
	key_ShiftKeys['4' ]	= '$';
	key_ShiftKeys['5' ]	= '%';
	key_ShiftKeys['6' ]	= '^';
	key_ShiftKeys['7' ]	= '&';
	key_ShiftKeys['8' ]	= '*';
	key_ShiftKeys['9' ]	= '(';
	key_ShiftKeys['0' ]	= ')';
	key_ShiftKeys['-' ]	= '_';
	key_ShiftKeys['=' ]	= '+';
	key_ShiftKeys[',' ]	= '<';
	key_ShiftKeys['.' ]	= '>';
	key_ShiftKeys['/' ]	= '?';
	key_ShiftKeys[';' ]	= ':';
	key_ShiftKeys['\'']	= '"';
	key_ShiftKeys['[' ]	= '{';
	key_ShiftKeys[']' ]	= '}';
	key_ShiftKeys['`' ]	= '~';
	key_ShiftKeys['\\']	= '|';

	// keys with binds not allowed to be used while in the menu
	key_MenuBound[K_F1 ] = qTrue;
	key_MenuBound[K_F2 ] = qTrue;
	key_MenuBound[K_F3 ] = qTrue;
	key_MenuBound[K_F4 ] = qTrue;
	key_MenuBound[K_F5 ] = qTrue;
	key_MenuBound[K_F6 ] = qTrue;
	key_MenuBound[K_F7 ] = qTrue;
	key_MenuBound[K_F8 ] = qTrue;
	key_MenuBound[K_F9 ] = qTrue;
	key_MenuBound[K_F10] = qTrue;
	key_MenuBound[K_F11] = qTrue;
	key_MenuBound[K_F12] = qTrue;
}


/*
===================
Key_Shutdown
===================
*/
void Key_Shutdown (void) {
	Cmd_RemoveCommand ("bind");
	Cmd_RemoveCommand ("unbind");
	Cmd_RemoveCommand ("unbindall");
	Cmd_RemoveCommand ("bindlist");
}
