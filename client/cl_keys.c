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
// cl_keys.c
//

#include "cl_local.h"

static keyDest_t	cl_keyDest;

static qBool		key_shiftDown = qFalse;
static qBool		key_capsLockOn = qFalse;
qBool				key_insertOn = qTrue;

int					key_anyKeyDown;

keyInfo_t			key_keyInfo[K_MAXKEYS];

char				key_consoleBuffer[32][MAXCMDLINE];
int					key_consoleLinePos = 1;
int					key_consoleEditLine = 0;
int					key_consoleHistoryLine = 0;

qBool				key_chatTeam;
char				key_chatBuffer[32][MAXCMDLINE];
int					key_chatLinePos = 0;
int					key_chatEditLine = 0;
int					key_chatHistoryLine = 0;

typedef struct keyName_s {
	char	*name;
	int		keyNum;
} keyName_t;

static keyName_t key_Names[] = {
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
Key_GetDest
=====================
*/
keyDest_t Key_GetDest (void)
{
	return cl_keyDest;
}


/*
=====================
Key_SetDest
=====================
*/
void Key_SetDest (keyDest_t keyDest)
{
	if (keyDest < KD_MINDEST || keyDest > KD_MAXDEST)
		Com_Error (ERR_FATAL, "Key_SetDest: invalid keyDest");

	cl_keyDest = keyDest;
}


/*
====================
Key_CompleteCommand
====================
*/
enum {
	GRP_ALIAS,
	GRP_CMD,
	GRP_CVAR,
	GRP_TOTAL = 3
};

static char	key_shortMatch[MAX_TOKEN_CHARS];
static int	key_totalMatches;
static int	key_totalMatchGroup[GRP_TOTAL];
static int	key_matchGroup;

static char	key_completePartial[MAX_TOKEN_CHARS];
static int	key_completeLen;

static void Key_FindMatch (const char *name)
{
	int		i;

	// Make certain this partially matches
	if (Q_strnicmp (name, key_completePartial, key_completeLen))
		return;
	key_totalMatches++;
	key_totalMatchGroup[key_matchGroup]++;

	// If it's the first match, just copy
	if (key_totalMatches == 1) {
		Q_strncpyz (key_shortMatch, name, sizeof (key_shortMatch));
		return;
	}

	// Cut off where the matching stops
	for (i=0 ; name[i] ; i++) {
		if (Q_tolower (key_shortMatch[i]) != Q_tolower (name[i])) {
			key_shortMatch[i] = '\0';
			break;
		}
	}
}

static void Key_CompleteCommand (void)
{
	char	*str;
	int		len, i, j;
	qBool	inWord;

	// Clear
	memset (key_completePartial, 0, sizeof (key_completePartial));
	inWord = qFalse;

	str = key_consoleBuffer[key_consoleEditLine];
	len = strlen (str);
	if (len <= 1)
		return;	// If it's only ']' then why bother?

	// Copy the valid word (ignore the initial ']')
	for (i=1, j=0 ; i<len ; i++) {
		if (str[i] == '\\' || str[i] == '/' || str[i] == ' ' || str[i] == ';') {
			if (inWord) {
				key_completePartial[i] = '\0';
				break;
			}
			else {
				continue;
			}
		}

		inWord = qTrue;
		key_completePartial[j++] = str[i];
	}

	// Get the length
	key_completeLen = strlen (key_completePartial);
	if (!key_completeLen)
		return;

	// Collect shortest match and total match count
	key_totalMatches = 0;
	memset (key_totalMatchGroup, 0, sizeof (key_totalMatchGroup));

	key_matchGroup = GRP_ALIAS;
	Alias_CallBack (Key_FindMatch);

	key_matchGroup = GRP_CMD;
	Cmd_CallBack (Key_FindMatch);

	key_matchGroup = GRP_CVAR;
	Cvar_CallBack (Key_FindMatch);

	switch (key_totalMatches) {
	case 0:
		// No matches
		Com_Printf (0, "No matches...\n");
		return;

	case 1:
		// Found an exact match
		Q_strncpyz (key_consoleBuffer[key_consoleEditLine]+1, key_shortMatch, sizeof (key_consoleBuffer[key_consoleEditLine])-1);
		key_consoleLinePos = strlen (key_shortMatch)+1;
		key_consoleBuffer[key_consoleEditLine][key_consoleLinePos] = ' ';
		key_consoleLinePos++;
		key_consoleBuffer[key_consoleEditLine][key_consoleLinePos] = 0;
		return;
	}

	// Cut off where partial matching stops
	for (i=0 ; i<key_completeLen ; i++) {
		if (Q_tolower (key_shortMatch[i]) != Q_tolower (key_completePartial[i])) {
			key_shortMatch[i] = '\0';
			break;
		}
	}

	// Print matching aliases
	if (key_totalMatchGroup[GRP_ALIAS] > 0) {
		Cbuf_AddText ("echo ========== Matching aliases ===========\n");
		Cbuf_AddText (Q_VarArgs ("aliaslist %s*\n", key_shortMatch));
	}

	// Print matching commands
	if (key_totalMatchGroup[GRP_CMD] > 0) {
		Cbuf_AddText ("echo ========== Matching commands ==========\n");
		Cbuf_AddText (Q_VarArgs ("cmdlist %s*\n", key_shortMatch));
	}

	// Print matching cvars
	if (key_totalMatchGroup[GRP_CVAR] > 0) {
		Cbuf_AddText ("echo =========== Matching cvars ============\n");
		Cbuf_AddText (Q_VarArgs ("cvarlist %s*\n", key_shortMatch));
	}

	// Place completion
	Q_strncpyz (key_consoleBuffer[key_consoleEditLine]+1, key_shortMatch, sizeof (key_consoleBuffer[key_consoleEditLine])-1);
	key_consoleLinePos = strlen (key_shortMatch)+1;
	key_consoleBuffer[key_consoleEditLine][key_consoleLinePos] = 0;
}


/*
====================
Key_Message
====================
*/
static void Key_Message (int key)
{
	int		charCount, i;
	char	*cbd;

	switch (key) {
	case 'v':
	case 'V':
		if (!key_keyInfo[K_CTRL].down)
			break;
		goto pasteIntoMessage;

	case K_INS:
	case K_KP_INS:
		if (!key_keyInfo[K_SHIFT].down && !key_keyInfo[K_LSHIFT].down && !key_keyInfo[K_RSHIFT].down) {
			if (key == K_INS)
				key_insertOn = !key_insertOn;
			break;
		}
pasteIntoMessage:
		if ((cbd = Sys_GetClipboardData ()) != 0) {
			strtok (cbd, "\n\r\b");

			i = strlen (cbd);
			if (i + key_chatLinePos >= MAXCMDLINE)
				i = MAXCMDLINE - key_chatLinePos;

			if (i > 0) {
				cbd[i] = 0;
				Q_strcatz (key_chatBuffer[key_chatEditLine], cbd, sizeof (key_chatBuffer[key_chatEditLine]));
				key_chatLinePos += i;
			}

			Mem_Free (cbd);
		}
		return;

	case 'l':
		if (!key_keyInfo[K_CTRL].down)
			break;
		key_chatBuffer[key_chatEditLine][0] = 0;
		key_chatLinePos = 0;
		return;

	case K_ENTER:
	case K_KP_ENTER:
		if (key_chatTeam)
			Cbuf_AddText ("say_team \"");
		else
			Cbuf_AddText ("say \"");

		Cbuf_AddText (key_chatBuffer[key_chatEditLine]);
		Cbuf_AddText ("\"\n");

		key_chatEditLine = (key_chatEditLine + 1) & 31;
		key_chatHistoryLine = key_chatEditLine;
		key_chatBuffer[key_chatEditLine][0] = 0;
		key_chatLinePos = 0;

		Key_SetDest (KD_GAME);
		return;

	case K_ESCAPE:
		key_chatHistoryLine = key_chatEditLine;
		key_chatBuffer[key_chatEditLine][0] = 0;
		key_chatLinePos = 0;

		Key_SetDest (KD_GAME);
		return;

	case 'h':
		if (!key_keyInfo[K_CTRL].down)
			break;
	case K_LEFTARROW:
	case K_KP_LEFTARROW:
		// Jump over invisible color sequences
		charCount = Q_ColorCharCount (key_chatBuffer[key_chatEditLine], key_chatLinePos);
		if (charCount > 0)
			key_chatLinePos = Q_ColorCharOffset (key_chatBuffer[key_chatEditLine], charCount - 1);
		return;

	case K_BACKSPACE:
		if (key_chatLinePos > 0) {
			// Skip to the end of color sequence
			while (Q_IsColorString (key_chatBuffer[key_chatEditLine] + key_chatLinePos))
				key_chatLinePos += 2;

			strcpy (key_chatBuffer[key_chatEditLine] + key_chatLinePos - 1, key_chatBuffer[key_chatEditLine] + key_chatLinePos);
			key_chatLinePos--;
		}

		return;

	case K_DEL:
		if (key_chatLinePos < (int)strlen (key_chatBuffer[key_chatEditLine]))
			strcpy (key_chatBuffer[key_chatEditLine] + key_chatLinePos, key_chatBuffer[key_chatEditLine] + key_chatLinePos + 1);
		return;

	case K_RIGHTARROW:
		if (strlen (key_chatBuffer[key_chatEditLine]) == key_chatLinePos) {
			if ((int)strlen (key_chatBuffer[(key_chatEditLine + 31) & 31]) <= key_chatLinePos)
				return;

			key_chatBuffer[key_chatEditLine][key_chatLinePos] = key_chatBuffer[(key_chatEditLine + 31) & 31][key_chatLinePos];
			key_chatLinePos++;
			key_chatBuffer[key_chatEditLine][key_chatLinePos] = 0;
		}
		else {
			// Jump over invisible color sequences
			charCount = Q_ColorCharCount (key_chatBuffer[key_chatEditLine], key_chatLinePos);
			key_chatLinePos = Q_ColorCharOffset (key_chatBuffer[key_chatEditLine], charCount + 1);
		}
		return;

	case 'p':
		if (!key_keyInfo[K_CTRL].down)
			break;
	case K_UPARROW:
	case K_KP_UPARROW:
		do {
			key_chatHistoryLine = (key_chatHistoryLine - 1) & 31;
		} while ((key_chatHistoryLine != key_chatEditLine) && !key_chatBuffer[key_chatHistoryLine][0]);

		if (key_chatHistoryLine == key_chatEditLine)
			key_chatHistoryLine = (key_chatEditLine + 1) & 31;

		strcpy (key_chatBuffer[key_chatEditLine], key_chatBuffer[key_chatHistoryLine]);
		key_chatLinePos = strlen (key_chatBuffer[key_chatEditLine]);
		return;

	case 'n':
		if (!key_keyInfo[K_CTRL].down)
			break;
	case K_DOWNARROW:
	case K_KP_DOWNARROW:
		if (key_chatHistoryLine == key_chatEditLine)
			return;

		do {
			key_chatHistoryLine = (key_chatHistoryLine + 1) & 31;
		} while ((key_chatHistoryLine != key_chatEditLine) && !key_chatBuffer[key_chatHistoryLine][0]);

		if (key_chatHistoryLine == key_chatEditLine) {
			key_chatBuffer[key_chatEditLine][0] = 0;
			key_chatLinePos = 0;
		}
		else {
			strcpy (key_chatBuffer[key_chatEditLine], key_chatBuffer[key_chatHistoryLine]);
			key_chatLinePos = strlen (key_chatBuffer[key_chatEditLine]);
		}
		return;

	case K_HOME:
	case K_KP_HOME:
		key_chatLinePos = 0;
		return;

	case K_END:
	case K_KP_END:
		key_chatLinePos = strlen (key_chatBuffer[key_chatEditLine]);
		return;
	}

	// Printable?
	if (key < 32 || key > 127)
		return;

	if (key_chatLinePos < MAXCMDLINE-1) {
		// Check insert mode
		if (key_insertOn) {
			// Can't do strcpy to move string to right
			i = strlen (key_chatBuffer[key_chatEditLine]) - 1;

			if (i == MAXCMDLINE-2) 
				i--;

			for ( ; i>=key_chatLinePos ; i--)
				key_chatBuffer[key_chatEditLine][i + 1] = key_chatBuffer[key_chatEditLine][i];
		}

		// Only null terminate if at the end
		i = key_chatBuffer[key_chatEditLine][key_chatLinePos];
		key_chatBuffer[key_chatEditLine][key_chatLinePos] = key;
		key_chatLinePos++;
		if (!i) {
			for (i=key_chatLinePos ; i<MAXCMDLINE ; i++)
				key_chatBuffer[key_chatEditLine][i] = '\0';
		}
	}
}


/*
====================
Key_Console

Interactive line editing and console scrollback
====================
*/
static void Key_Console (int key)
{
	int		charCount, i;
	char	*cbd;

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

	switch (key) {
	case 'c':
	case 'C':
		if (!key_keyInfo[K_CTRL].down)
			break;

		Com_Printf (0, "%s\n", key_consoleBuffer[key_consoleEditLine]);
		key_consoleEditLine = (key_consoleEditLine + 1) & 31;
		key_consoleHistoryLine = key_consoleEditLine;
		key_consoleBuffer[key_consoleEditLine][0] = ']';
		key_consoleBuffer[key_consoleEditLine][1] = 0;
		key_consoleLinePos = 1;

		if (Com_ClientState () == CA_DISCONNECTED)
			SCR_UpdateScreen ();	// Force an update, because the command may take some time
		return;

	case 'v':
	case 'V':
		if (!key_keyInfo[K_CTRL].down)
			break;
		goto pasteIntoMessage;

	case K_INS:
	case K_KP_INS:
		if (!key_keyInfo[K_SHIFT].down && !key_keyInfo[K_LSHIFT].down && !key_keyInfo[K_RSHIFT].down) {
			if (key == K_INS)
				key_insertOn = !key_insertOn;
			break;
		}
pasteIntoMessage:
		if ((cbd = Sys_GetClipboardData ()) != 0) {
			strtok (cbd, "\n\r\b");

			i = strlen (cbd);
			if (i + key_consoleLinePos >= MAXCMDLINE)
				i = MAXCMDLINE - key_consoleLinePos;

			if (i > 0) {
				cbd[i] = 0;
				Q_strcatz (key_consoleBuffer[key_consoleEditLine], cbd, sizeof (key_consoleBuffer[key_consoleEditLine]));
				key_consoleLinePos += i;
			}
			Mem_Free (cbd);
		}
		return;

	case 'l':
		if (!key_keyInfo[K_CTRL].down)
			break;
		Cbuf_AddText ("clear\n");
		return;

	case K_ESCAPE:
		CL_ToggleConsole_f ();
		break;

	case K_ENTER:
	case K_KP_ENTER:
		// Backslash text are commands, else chat
		if (key_consoleBuffer[key_consoleEditLine][1] == '\\' || key_consoleBuffer[key_consoleEditLine][1] == '/')
			Cbuf_AddText (key_consoleBuffer[key_consoleEditLine]+2);	// Skip the >
		else
			Cbuf_AddText (key_consoleBuffer[key_consoleEditLine]+1);	// Valid command

		Cbuf_AddText ("\n");
		Com_Printf (0, "%s\n", key_consoleBuffer[key_consoleEditLine]);
		key_consoleEditLine = (key_consoleEditLine + 1) & 31;
		key_consoleHistoryLine = key_consoleEditLine;
		key_consoleBuffer[key_consoleEditLine][0] = ']';
		key_consoleBuffer[key_consoleEditLine][1] = 0;
		key_consoleLinePos = 1;

		if (Com_ClientState () == CA_DISCONNECTED)
			SCR_UpdateScreen ();	// Force an update, because the command may take some time
		return;

	case K_TAB:
		// Command completion
		Key_CompleteCommand ();
		return;

	case 'h':
		if (!key_keyInfo[K_CTRL].down)
			break;
	case K_LEFTARROW:
	case K_KP_LEFTARROW:
		// Jump over invisible color sequences
		charCount = Q_ColorCharCount (key_consoleBuffer[key_consoleEditLine], key_consoleLinePos);
		if (charCount > 1)
			key_consoleLinePos = Q_ColorCharOffset (key_consoleBuffer[key_consoleEditLine], charCount - 1);

		return;

	case K_BACKSPACE:
		if (key_consoleLinePos > 1) {
			// Skip to the end of color sequence
			while (Q_IsColorString (key_consoleBuffer[key_consoleEditLine] + key_consoleLinePos))
				key_consoleLinePos += 2;

			strcpy (key_consoleBuffer[key_consoleEditLine] + key_consoleLinePos - 1, key_consoleBuffer[key_consoleEditLine] + key_consoleLinePos);
			key_consoleLinePos--;
		}

		return;

	case K_DEL:
		if (key_consoleLinePos < (int)strlen (key_consoleBuffer[key_consoleEditLine]))
			strcpy (key_consoleBuffer[key_consoleEditLine] + key_consoleLinePos, key_consoleBuffer[key_consoleEditLine] + key_consoleLinePos + 1);
		return;

	case K_RIGHTARROW:
		if (strlen (key_consoleBuffer[key_consoleEditLine]) == key_consoleLinePos) {
			if ((int)strlen (key_consoleBuffer[(key_consoleEditLine + 31) & 31]) <= key_consoleLinePos)
				return;

			key_consoleBuffer[key_consoleEditLine][key_consoleLinePos] = key_consoleBuffer[(key_consoleEditLine + 31) & 31][key_consoleLinePos];
			key_consoleLinePos++;
			key_consoleBuffer[key_consoleEditLine][key_consoleLinePos] = 0;
		}
		else {
			// Jump over invisible color sequences
			charCount = Q_ColorCharCount (key_consoleBuffer[key_consoleEditLine], key_consoleLinePos);
			key_consoleLinePos = Q_ColorCharOffset (key_consoleBuffer[key_consoleEditLine], charCount + 1);
		}
		return;

	case 'p':
		if (!key_keyInfo[K_CTRL].down)
			break;
	case K_UPARROW:
	case K_KP_UPARROW:
		do {
			key_consoleHistoryLine = (key_consoleHistoryLine - 1) & 31;
		} while ((key_consoleHistoryLine != key_consoleEditLine) && !key_consoleBuffer[key_consoleHistoryLine][1]);

		if (key_consoleHistoryLine == key_consoleEditLine)
			key_consoleHistoryLine = (key_consoleEditLine + 1) & 31;

		memset (&key_consoleBuffer[key_consoleEditLine], '\0', sizeof (key_consoleBuffer[key_consoleEditLine]));
		strcpy (key_consoleBuffer[key_consoleEditLine], key_consoleBuffer[key_consoleHistoryLine]);
		key_consoleLinePos = strlen (key_consoleBuffer[key_consoleEditLine]);
		return;

	case 'n':
		if (!key_keyInfo[K_CTRL].down)
			break;
	case K_DOWNARROW:
	case K_KP_DOWNARROW:
		if (key_consoleHistoryLine == key_consoleEditLine)
			return;

		do {
			key_consoleHistoryLine = (key_consoleHistoryLine + 1) & 31;
		} while ((key_consoleHistoryLine != key_consoleEditLine) && !key_consoleBuffer[key_consoleHistoryLine][1]);

		if (key_consoleHistoryLine == key_consoleEditLine) {
			key_consoleBuffer[key_consoleEditLine][0] = ']';
			key_consoleBuffer[key_consoleEditLine][1] = 0;
			key_consoleLinePos = 1;
		}
		else {
			memset (&key_consoleBuffer[key_consoleEditLine], '\0', sizeof (key_consoleBuffer[key_consoleEditLine]));
			strcpy (key_consoleBuffer[key_consoleEditLine], key_consoleBuffer[key_consoleHistoryLine]);
			key_consoleLinePos = strlen (key_consoleBuffer[key_consoleEditLine]);
		}
		return;

	case K_PGUP:
	case K_KP_PGUP:
	case K_MWHEELUP:
		CL_MoveConsoleDisplay (-con_scroll->integer);
		return;

	case K_PGDN:
	case K_KP_PGDN:
	case K_MWHEELDOWN:
		CL_MoveConsoleDisplay (con_scroll->integer);
		return;

	case K_HOME:
	case K_KP_HOME:
		CL_SetConsoleDisplay (qTrue);
		return;

	case K_END:
	case K_KP_END:
		CL_SetConsoleDisplay (qFalse);
		return;
	}

	// Printable?
	if (key < 32 || key > 127)
		return;

	if (key_consoleLinePos < MAXCMDLINE-1) {
		int i;

		// Check insert mode
		if (key_insertOn) {
			// Can't do strcpy to move string to right
			i = strlen (key_consoleBuffer[key_consoleEditLine]) - 1;

			if (i == MAXCMDLINE-2) 
				i--;

			for ( ; i>=key_consoleLinePos ; i--)
				key_consoleBuffer[key_consoleEditLine][i + 1] = key_consoleBuffer[key_consoleEditLine][i];
		}

		// Only null terminate if at the end
		i = key_consoleBuffer[key_consoleEditLine][key_consoleLinePos];
		key_consoleBuffer[key_consoleEditLine][key_consoleLinePos] = key;
		key_consoleLinePos++;
		if (!i) {
			for (i=key_consoleLinePos ; i<MAXCMDLINE ; i++)
				key_consoleBuffer[key_consoleEditLine][i] = '\0';
		}
	}
}

//============================================================================

/*
===================
Key_StringToKeynum

Returns a key number to be used to index key_keyInfo[].bind by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.
===================
*/
int Key_StringToKeynum (char *str)
{
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
char *Key_KeynumToString (int keyNum)
{
	keyName_t		*kn;	
	static	char	tinystr[2];
	
	if (keyNum == -1)
		return "<KEY NOT FOUND>";

	if (keyNum > 32 && keyNum < 127) {
		// Printable ascii
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
================
Key_InsertOn
================
*/
qBool Key_InsertOn (void)
{
	return key_insertOn;
}


/*
================
Key_CapslockOn
================
*/
qBool Key_CapslockOn (void)
{
	return key_capsLockOn;
}


/*
================
Key_ShiftDown
================
*/
qBool Key_ShiftDown (void)
{
	return key_shiftDown;
}


/*
===================
Key_Event

Called by the system between frames for both key up and key down events
Should NOT be called during an interrupt!
===================
*/
char *Key_FallBackKey (int keyNum)
{
	if (keyNum == K_LSHIFT || keyNum == K_RSHIFT) {
		if (!key_keyInfo[keyNum].bind)
			return key_keyInfo[K_SHIFT].bind;
	}

	return key_keyInfo[keyNum].bind;
}

void Key_Event (int keyNum, qBool isDown, uint32 time)
{
	char	*kb;
	char	cmd[1024];

	// Update auto-repeat status
	if (isDown) {
		key_keyInfo[keyNum].repeated++;

		if (key_keyInfo[keyNum].repeated > 1) {
			// Ignore most autorepeats
			switch (keyNum) {
			case K_BACKSPACE:
			case K_DEL:
			case K_LEFTARROW:
			case K_RIGHTARROW:
			case K_UPARROW:
			case K_DOWNARROW:
			case K_PGUP:
			case K_PGDN:
			case K_KP_PGUP:
			case K_KP_PGDN:
				// Allow these to repeat
				break;

			default:
				// This allows ascii to repeat
				if (keyNum < 32 || keyNum > 126) {
					return;
				}
				break;
			}
		}

		if (keyNum >= 200 && keyNum < K_MWHEELDOWN && !key_keyInfo[keyNum].bind)
			Com_Printf (0, "%s is unbound.\n", Key_KeynumToString (keyNum));
	}
	else {
		key_keyInfo[keyNum].repeated = 0;
	}

	if (keyNum == K_SHIFT || keyNum == K_LSHIFT || keyNum == K_RSHIFT)
		key_shiftDown = isDown;
#ifdef WIN32 // FIXME
	key_capsLockOn = In_GetKeyState (K_CAPSLOCK);
#else
	key_capsLockOn = qFalse;
#endif

	// Console key is hardcoded, so the user can never unbind it
	if (!key_shiftDown && (keyNum == '`' || keyNum == '~')) {
		if (!isDown)
			return;

		CL_ToggleConsole_f ();
		return;
	}

	// Menu key is hardcoded, so the user can never unbind it
	if (keyNum == K_ESCAPE) {
		switch (cl_keyDest) {
		case KD_MESSAGE:
			if (isDown)
				Key_Message (keyNum);
			break;

		case KD_GAME:
			if (isDown) {
				if (cl.attractLoop || cl.cin.frameNum < 0) {
					// Kill the cinematic
					Cbuf_AddText ("killserver\n");
					break;
				}
				else if (cl.cin.time > 0) {
					// Finish this cinematic
					CIN_FinishCinematic ();
					break;
				}
			}
			CL_CGModule_KeyEvent (keyNum, isDown);
			break;

		case KD_MENU:
			if (isDown)
				GUI_KeyDown (keyNum);
			else
				GUI_KeyUp (keyNum);
			CL_CGModule_KeyEvent (keyNum, isDown);
			break;

		case KD_CONSOLE:
			if (isDown)
				Key_Console (keyNum);
			break;

		default:
			Com_Error (ERR_FATAL, "Bad keyDest");
		}
		return;
	}

	// Track if any key is down for BUTTON_ANY
	key_keyInfo[keyNum].down = isDown;
	if (isDown) {
		if (key_keyInfo[keyNum].repeated == 1)
			key_anyKeyDown++;
	}
	else {
		key_anyKeyDown--;
		if (key_anyKeyDown < 0)
			key_anyKeyDown = 0;
	}

	/*
	** Key up events only generate commands if the game key binding is a button command (leading + sign).
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

		if (key_keyInfo[keyNum].shiftValue != keyNum) {
			kb = key_keyInfo[key_keyInfo[keyNum].shiftValue].bind;
			if (kb && (kb[0] == '+')) {
				Q_snprintfz (cmd, sizeof (cmd), "-%s %i %i\n", kb+1, keyNum, time);
				Cbuf_AddText (cmd);
			}
		}
		return;
	}

	// If not a consolekey, send to the interpreter no matter what mode is
	if ((Key_GetDest () == KD_CONSOLE && !key_shiftDown && !key_keyInfo[keyNum].console)
	|| (Key_GetDest () == KD_GAME && (Com_ClientState () == CA_ACTIVE
	|| (!key_shiftDown && !key_keyInfo[keyNum].console)))) {
		kb = Key_FallBackKey (keyNum);
		if (kb) {
			if (kb[0] == '+') {
				// Button commands add keynum and time as a parm
				Q_snprintfz (cmd, sizeof (cmd), "%s %i %i\n", kb, keyNum, time);
				Cbuf_AddText (cmd);
			}
			else {
				Cbuf_AddText (kb);
				Cbuf_AddText ("\n");
			}
		}
		return;
	}

	// Other systems only care about key down events
	if (key_shiftDown ^ (key_capsLockOn && keyNum >= 'a' && keyNum <= 'z'))
		keyNum = key_keyInfo[keyNum].shiftValue;

	switch (cl_keyDest) {
	case KD_MESSAGE:
		if (isDown)
			Key_Message (keyNum);
		break;

	case KD_MENU:
		if (isDown)
			GUI_KeyDown (keyNum);
		else
			GUI_KeyUp (keyNum);
		CL_CGModule_KeyEvent (keyNum, isDown);
		break;

	case KD_GAME:
	case KD_CONSOLE:
		if (isDown)
			Key_Console (keyNum);
		break;

	default:
		Com_Error (ERR_FATAL, "Bad keyDest");
	}
}


/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates (void)
{
	int		i;

	key_anyKeyDown = 0;
	for (i=0 ; i<K_MAXKEYS ; i++) {
		if (key_keyInfo[i].down || key_keyInfo[i].repeated)
			Key_Event (i, qFalse, 0);

		key_keyInfo[i].down = 0;
		key_keyInfo[i].repeated = 0;
	}
}


/*
================
Key_ClearTyping
================
*/
void Key_ClearTyping (void)
{
	// Clear any typing
	key_consoleBuffer[key_consoleEditLine][1] = 0;
	key_consoleLinePos = 1;
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
void Key_SetBinding (int keyNum, char *binding)
{
	if (keyNum == -1)
		return;

	// Free old bindings
	if (key_keyInfo[keyNum].bind) {
		Mem_Free (key_keyInfo[keyNum].bind);
		key_keyInfo[keyNum].bind = NULL;
	}

	// Allocate memory for new binding
	key_keyInfo[keyNum].bind = Mem_StrDup (binding);
}


/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings (FILE *f)
{
	int		i;

	for (i=0 ; i<K_MAXKEYS ; i++) {
		if (key_keyInfo[i].bind && key_keyInfo[i].bind[0])
			fprintf (f, "bind %s \"%s\"\n", Key_KeynumToString (i), key_keyInfo[i].bind);
	}
}


/*
===================
Key_GetBindingBuf
===================
*/
char *Key_GetBindingBuf (int binding)
{
	return key_keyInfo[binding].bind;
}

/*
===================
Key_IsDown
===================
*/
qBool Key_IsDown (int keyNum)
{
	if (keyNum < 0 || keyNum >= K_MAXKEYS)
		return qFalse;
	if (keyNum == K_SHIFT)
		return (key_keyInfo[K_SHIFT].down || key_keyInfo[K_LSHIFT].down || key_keyInfo[K_RSHIFT].down);

	return key_keyInfo[keyNum].down;
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
static void Key_Unbind_f (void)
{
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
static void Key_Unbindall_f (void)
{
	int		i;
	
	for (i=0 ; i<K_MAXKEYS ; i++) {
		if (key_keyInfo[i].bind)
			Key_SetBinding (i, "");
	}
}


/*
===================
Key_Bind_f
===================
*/
static void Key_Bind_f (void)
{
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
		if (key_keyInfo[b].bind)
			Com_Printf (0, "\"%s\" = \"%s\"\n", Cmd_Argv (1), key_keyInfo[b].bind);
		else
			Com_Printf (0, "\"%s\" is not bound\n", Cmd_Argv (1));
		return;
	}
	
	// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	for (i=2 ; i<c ; i++) {
		Q_strcatz (cmd, Cmd_Argv (i), sizeof (cmd));
		if (i != (c-1))
			Q_strcatz (cmd, " ", sizeof (cmd));
	}

	Key_SetBinding (b, cmd);
}


/*
============
Key_Bindlist_f
============
*/
static void Key_Bindlist_f (void)
{
	int		i;
	char	*wildCard;

	if (Cmd_Argc () == 2)
		wildCard = Cmd_Argv (1);
	else
		wildCard = "*";

	for (i=0 ; i<K_MAXKEYS ; i++) {
		if (key_keyInfo[i].bind && key_keyInfo[i].bind[0] && Q_WildcardMatch (wildCard, key_keyInfo[i].bind, 1))
			Com_Printf (0, "%s \"%s\"\n", Key_KeynumToString(i), key_keyInfo[i].bind);
	}
}

/*
===============================================================================

	INITIALIZATION

===============================================================================
*/

/*
===================
Key_Init
===================
*/
void Key_Init (void)
{
	int		i;

	//
	// Add commands
	//
	Cmd_AddCommand ("bind",			Key_Bind_f,			"Bind a key to do something");
	Cmd_AddCommand ("unbind",		Key_Unbind_f,		"Remove a key binding");
	Cmd_AddCommand ("unbindall",	Key_Unbindall_f,	"Remove all key bindings");
	Cmd_AddCommand ("bindlist",		Key_Bindlist_f,		"Prints a list of key bindings");

	//
	// Set defaults
	//
	for (i=0 ; i<32 ; i++) {
		key_consoleBuffer[i][0] = ']';
		key_consoleBuffer[i][1] = 0;

		key_chatBuffer[i][0] = 0;
	}
	key_consoleLinePos = 1;
	key_chatLinePos = 0;

	//
	// ASCII chars are allowed in the console
	//
	for (i=32 ; i<128 ; i++)
		key_keyInfo[i].console = qTrue;

	//
	// Other keys allowed to be typed in the console
	//
	key_keyInfo[K_ENTER			].console = qTrue;
	key_keyInfo[K_KP_ENTER		].console = qTrue;
	key_keyInfo[K_TAB			].console = qTrue;
	key_keyInfo[K_LEFTARROW		].console = qTrue;
	key_keyInfo[K_KP_LEFTARROW	].console = qTrue;
	key_keyInfo[K_RIGHTARROW	].console = qTrue;
	key_keyInfo[K_KP_RIGHTARROW	].console = qTrue;
	key_keyInfo[K_UPARROW		].console = qTrue;
	key_keyInfo[K_KP_UPARROW	].console = qTrue;
	key_keyInfo[K_DOWNARROW		].console = qTrue;
	key_keyInfo[K_KP_DOWNARROW	].console = qTrue;
	key_keyInfo[K_BACKSPACE		].console = qTrue;
	key_keyInfo[K_HOME			].console = qTrue;
	key_keyInfo[K_KP_HOME		].console = qTrue;
	key_keyInfo[K_END			].console = qTrue;
	key_keyInfo[K_KP_END		].console = qTrue;
	key_keyInfo[K_PGUP			].console = qTrue;
	key_keyInfo[K_KP_PGUP		].console = qTrue;
	key_keyInfo[K_PGDN			].console = qTrue;
	key_keyInfo[K_KP_PGDN		].console = qTrue;

	key_keyInfo[K_SHIFT			].console = qTrue;
	key_keyInfo[K_LSHIFT		].console = qTrue;
	key_keyInfo[K_RSHIFT		].console = qTrue;

	key_keyInfo[K_INS			].console = qTrue;
	key_keyInfo[K_DEL			].console = qTrue;
	key_keyInfo[K_KP_INS		].console = qTrue;
	key_keyInfo[K_KP_DEL		].console = qTrue;
	key_keyInfo[K_KP_SLASH		].console = qTrue;
	key_keyInfo[K_KP_PLUS		].console = qTrue;
	key_keyInfo[K_KP_MINUS		].console = qTrue;
	key_keyInfo[K_KP_FIVE		].console = qTrue;

	key_keyInfo[K_MWHEELUP		].console = qTrue;
	key_keyInfo[K_MWHEELDOWN	].console = qTrue;
	key_keyInfo[K_MWHEELLEFT	].console = qTrue;
	key_keyInfo[K_MWHEELRIGHT	].console = qTrue;

	key_keyInfo['`'				].console = qFalse;
	key_keyInfo['~'				].console = qFalse;

	//
	// Set what a key looks like when shift is held
	//
	for (i=0 ; i<K_MAXKEYS ; i++)
		key_keyInfo[i].shiftValue = i;

	for (i='a' ; i<='z' ; i++)
		key_keyInfo[i].shiftValue = i - 'a' + 'A';

	key_keyInfo['1' ].shiftValue = '!';
	key_keyInfo['2' ].shiftValue = '@';
	key_keyInfo['3' ].shiftValue = '#';
	key_keyInfo['4' ].shiftValue = '$';
	key_keyInfo['5' ].shiftValue = '%';
	key_keyInfo['6' ].shiftValue = '^';
	key_keyInfo['7' ].shiftValue = '&';
	key_keyInfo['8' ].shiftValue = '*';
	key_keyInfo['9' ].shiftValue = '(';
	key_keyInfo['0' ].shiftValue = ')';
	key_keyInfo['-' ].shiftValue = '_';
	key_keyInfo['=' ].shiftValue = '+';
	key_keyInfo[',' ].shiftValue = '<';
	key_keyInfo['.' ].shiftValue = '>';
	key_keyInfo['/' ].shiftValue = '?';
	key_keyInfo[';' ].shiftValue = ':';
	key_keyInfo['\''].shiftValue = '"';
	key_keyInfo['[' ].shiftValue = '{';
	key_keyInfo[']' ].shiftValue = '}';
	key_keyInfo['`' ].shiftValue = '~';
	key_keyInfo['\\'].shiftValue = '|';
}
