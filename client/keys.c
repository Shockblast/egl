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

static qBool	key_shiftDown = qFalse;
static qBool	key_capsLockOn = qFalse;
qBool			key_insertOn = qTrue;

int				key_anyKeyDown;
static int		key_waitingKey;

static char		*key_boundKeys[K_MAXKEYS];
static qBool	key_downKeys[K_MAXKEYS];		// key up events are sent even if in console mode
static int		key_repeatedKeys[K_MAXKEYS];	// if > 1, it is autorepeating
static qBool	key_consoleKeys[K_MAXKEYS];		// if qTrue, can't be typed while in the console
static int		key_shiftKeys[K_MAXKEYS];		// key to map to if shift held down when this key is pressed

char			key_consoleBuffer[32][MAXCMDLINE];
int				key_consoleLinePos = 1;
int				key_consoleEditLine = 0;
int				key_consoleHistoryLine = 0;

qBool			key_chatTeam;
char			key_chatBuffer[32][MAXCMDLINE];
int				key_chatLinePos = 0;
int				key_chatEditLine = 0;
int				key_chatHistoryLine = 0;

typedef struct keyName_s {
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
qBool Key_KeyDest (int keyDest)
{
	if (keyDest < KD_MINDEST || keyDest > KD_MAXDEST)
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
void Key_SetKeyDest (int keyDest)
{
	if (keyDest < KD_MINDEST || keyDest > KD_MAXDEST)
		Com_Error (ERR_FATAL, "Key_KeyDest: invalid keyDest");

	cls.keyDest = keyDest;
}


/*
====================
Key_CompleteCommand
====================
*/
static void Key_CompleteCommand (void)
{
	static uLong	compFrame = 0;
	cmdFunc_t		*cmd, *cmdFound;
	aliasCmd_t		*alias, *aliasFound;
	cVar_t			*cvar, *cvarFound;
	int				aliasCnt, cmdCnt, cvarCnt, len;
	uInt			maxCmdLen, maxAliasLen, maxCvarLen;
	int				totalMatches;
	char			*partial;

	partial = key_consoleBuffer[key_consoleEditLine]+1;
	// skip '/' and '\'
	if ((*partial == '\\') || (*partial == '/'))
		partial++;

	len = strlen (partial);
	if (!len)
		return;

	compFrame++;

	cmdFound = NULL;
	aliasFound = NULL;
	cvarFound = NULL;

	aliasCnt = 0;
	cmdCnt = 0;
	cvarCnt = 0;

	maxCmdLen = 0;
	maxAliasLen = 0;
	maxCvarLen = 0;

	//
	// check for exact match
	//
	for (alias=com_aliasList ; alias ; alias=alias->next) {
		if (!Q_stricmp (partial, alias->name)) {
			aliasFound = alias;
			if (alias->compFrame != compFrame) {
				aliasCnt++;
				alias->compFrame = compFrame;

				if (strlen (alias->name) > maxAliasLen)
					maxAliasLen = strlen (alias->name);
			}
		}
	}
	for (cmd=com_commandList ; cmd ; cmd=cmd->next) {
		if (!Q_stricmp (partial, cmd->name)) {
			cmdFound = cmd;
			if (cmd->compFrame != compFrame) {
				cmdCnt++;
				cmd->compFrame = compFrame;

				if (strlen (cmd->name) > maxCmdLen)
					maxCmdLen = strlen (cmd->name);
			}
		}
	}
	for (cvar=com_cvarList ; cvar ; cvar=cvar->next) {
		if (!Q_stricmp (partial, cvar->name)) {
			cvarFound = cvar;
			if (cvar->compFrame != compFrame) {
				cvarCnt++;
				cvar->compFrame = compFrame;

				if (strlen (cvar->name) > maxCvarLen)
					maxCvarLen = strlen (cvar->name);
			}
		}
	}

	//
	// check for partial match
	//
	for (alias=com_aliasList ; alias ; alias=alias->next) {
		if (!Q_strnicmp (partial, alias->name, len)) {
			aliasFound = alias;
			if (alias->compFrame != compFrame) {
				aliasCnt++;
				alias->compFrame = compFrame;

				if (strlen (alias->name) > maxAliasLen)
					maxAliasLen = strlen (alias->name);
			}
		}
	}
	for (cmd=com_commandList ; cmd ; cmd=cmd->next) {
		if (!Q_strnicmp (partial, cmd->name, len)) {
			cmdFound = cmd;
			if (cmd->compFrame != compFrame) {
				cmdCnt++;
				cmd->compFrame = compFrame;

				if (strlen (cmd->name) > maxCmdLen)
					maxCmdLen = strlen (cmd->name);
			}
		}
	}
	for (cvar=com_cvarList ; cvar ; cvar=cvar->next) {
		if (!Q_strnicmp (partial, cvar->name, len)) {
			cvarFound = cvar;
			if (cvar->compFrame != compFrame) {
				cvarCnt++;
				cvar->compFrame = compFrame;

				if (strlen (cvar->name) > maxCvarLen)
					maxCvarLen = strlen (cvar->name);
			}
		}
	}

	// return the exact match if there was only one
	totalMatches = aliasCnt + cmdCnt + cvarCnt;
	if (totalMatches == 1) {
		char	*completed;

		if (cmdFound)
			completed = cmdFound->name;
		else if (aliasFound)
			completed = aliasFound->name;
		else if (cvarFound)
			completed = cvarFound->name;
		else // shouldn't happen!
			return;

		Q_strncpyz (key_consoleBuffer[key_consoleEditLine]+1, completed, sizeof (key_consoleBuffer[key_consoleEditLine])-1);
		key_consoleLinePos = strlen (completed)+1;
		key_consoleBuffer[key_consoleEditLine][key_consoleLinePos] = ' ';
		key_consoleLinePos++;
		key_consoleBuffer[key_consoleEditLine][key_consoleLinePos] = 0;
	}
	else if (totalMatches > 1) {
		char	*aliasCompleted;
		char	*cmdCompleted;
		char	*cvarCompleted;
		uInt	outAliasLen;
		uInt	outCmdLen;
		uInt	outCvarLen;
		char	*completed, *source;
		uInt	i, shortest, longest;
		byte	*buffer, *maxesBuffer;
		char	c;

		maxesBuffer = buffer = Mem_Alloc (maxAliasLen+maxCmdLen+maxCvarLen+3);

		// construct longest alias completion
		aliasCompleted = (char *)buffer;
		outAliasLen = 0;
		for (i=0 ; i<maxAliasLen ; i++) {
			c = 0;

			for (alias=com_aliasList ; alias ; alias=alias->next) {
				// wasn't touched
				if (alias->compFrame != compFrame)
					continue;

				// matches
				if (!Q_strnicmp (partial, alias->name, len)) {
					if (!c)
						c = tolower (alias->name[i]);
					else if (c != tolower (alias->name[i])) {
						i = maxAliasLen;
						break;
					}
				}
			}

			aliasCompleted[i] = c;
			outAliasLen++;
		}
		buffer += maxAliasLen+1;

		// construct longest command completion
		cmdCompleted = (char *)buffer;
		outCmdLen = 0;
		for (i=0 ; i<maxCmdLen ; i++) {
			c = 0;

			for (cmd=com_commandList ; cmd ; cmd=cmd->next) {
				// wasn't touched
				if (cmd->compFrame != compFrame)
					continue;

				// matches
				if (!Q_strnicmp (partial, cmd->name, len)) {
					if (!c)
						c = tolower (cmd->name[i]);
					else if (c != tolower (cmd->name[i])) {
						i = maxCmdLen;
						break;
					}
				}
			}

			cmdCompleted[i] = c;
			outCmdLen++;
		}
		buffer += maxCmdLen+1;

		// construct longest cvar completion
		cvarCompleted = (char *)buffer;
		outCvarLen = 0;
		for (i=0 ; i<maxCvarLen ; i++) {
			c = 0;

			for (cvar=com_cvarList ; cvar ; cvar=cvar->next) {
				// wasn't touched
				if (cvar->compFrame != compFrame)
					continue;

				// matches
				if (!Q_strnicmp (partial, cvar->name, len)) {
					if (!c)
						c = tolower (cvar->name[i]);
					else if (c != tolower (cvar->name[i])) {
						i = maxCvarLen;
						break;
					}
				}
			}

			cvarCompleted[i] = c;
			outCvarLen++;
		}

		// find the longest value
		if (outAliasLen > outCmdLen) {
			longest = outAliasLen;
			source = aliasCompleted;
		}
		else {
			longest = outCmdLen;
			source = cmdCompleted;
		}
		if (outCvarLen > longest) {
			longest = outCvarLen;
			source = cvarCompleted;
		}

		// find where the matching stops
		for (i=0 ; i<longest ; i++) {
			if (outAliasLen && i > outAliasLen)
				break;
			if (outCmdLen && i > outCmdLen)
				break;
			if (outCvarLen && i > outCvarLen)
				break;

			if (outAliasLen) {
				if (outCmdLen && aliasCompleted[i] != cmdCompleted[i])
					break;
				if (outCvarLen && aliasCompleted[i] != cvarCompleted[i])
					break;
			}

			if (outCmdLen) {
				if (outAliasLen && cmdCompleted[i] != aliasCompleted[i])
					break;
				if (outCvarLen && cmdCompleted[i] != cvarCompleted[i])
					break;
			}

			if (outCvarLen) {
				if (outAliasLen && cvarCompleted[i] != aliasCompleted[i])
					break;
				if (outCmdLen && cvarCompleted[i] != cmdCompleted[i])
					break;
			}
		}
		shortest = i;

		// construct the shortest completion
		completed = Mem_Alloc (shortest+1);
		for (i=0 ; i<shortest ; i++) {
			completed[i] = source[i];
		}

		Q_strncpyz (key_consoleBuffer[key_consoleEditLine]+1, completed, sizeof (key_consoleBuffer[key_consoleEditLine])-1);
		key_consoleLinePos = strlen (completed)+1;
		key_consoleBuffer[key_consoleEditLine][key_consoleLinePos] = 0;

		// print matching aliases
		if (outAliasLen > 0) {
			Cbuf_AddText ("echo ^2^s========== Matching aliases ===========\n");
			Cbuf_AddText (Q_VarArgs ("aliaslist %s*\n", completed));
		}

		// print matching commands
		if (outCmdLen > 0) {
			Cbuf_AddText ("echo ^2^s========== Matching commands ==========\n");
			Cbuf_AddText (Q_VarArgs ("cmdlist %s*\n", completed));
		}

		// print matching cvars
		if (outCvarLen > 0) {
			Cbuf_AddText ("echo ^2^s=========== Matching cvars ============\n");
			Cbuf_AddText (Q_VarArgs ("cvarlist %s*\n", completed));
		}

		// free values
		Mem_Free (maxesBuffer);
		Mem_Free (completed);
	}
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
		if (!key_downKeys[K_CTRL])
			break;
		goto pasteIntoMessage;

	case K_INS:
	case K_KP_INS:
		if (!key_downKeys[K_SHIFT] && !key_downKeys[K_LSHIFT] && !key_downKeys[K_RSHIFT]) {
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
		if (!key_downKeys[K_CTRL])
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

		Key_SetKeyDest (KD_GAME);
		return;

	case K_ESCAPE:
		key_chatHistoryLine = key_chatEditLine;
		key_chatBuffer[key_chatEditLine][0] = 0;
		key_chatLinePos = 0;

		Key_SetKeyDest (KD_GAME);
		return;

	case 'h':
		if (!key_downKeys[K_CTRL])
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
		if (!key_downKeys[K_CTRL])
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
		if (!key_downKeys[K_CTRL])
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
	case 'v':
	case 'V':
		if (!key_downKeys[K_CTRL])
			break;
		goto pasteIntoMessage;

	case K_INS:
	case K_KP_INS:
		if (!key_downKeys[K_SHIFT] && !key_downKeys[K_LSHIFT] && !key_downKeys[K_RSHIFT]) {
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
		if (!key_downKeys[K_CTRL])
			break;
		Cbuf_AddText ("clear\n");
		return;

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

		if (cls.connectState == CA_DISCONNECTED)
			SCR_UpdateScreen ();	// Force an update, because the command may take some time
		return;

	case K_TAB:
		// Command completion
		Key_CompleteCommand ();
		return;

	case 'h':
		if (!key_downKeys[K_CTRL])
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
		if (!key_downKeys[K_CTRL])
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
		if (!key_downKeys[K_CTRL])
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
		con.display -= con_scroll->integer;
		return;

	case K_PGDN:
	case K_KP_PGDN:
	case K_MWHEELDOWN:
		con.display += con_scroll->integer;
		if (con.display > con.currentLine)
			con.display = con.currentLine;
		return;

	case K_HOME:
	case K_KP_HOME:
		con.display = con.currentLine - con.totalLines + 10;
		return;

	case K_END:
	case K_KP_END:
		con.display = con.currentLine;
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

Returns a key number to be used to index key_boundKeys[] by looking at
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
===================
Key_Event

Called by the system between frames for both key up and key down events
Should NOT be called during an interrupt!
===================
*/
char *Key_FallBackKey (int keyNum)
{
	if (keyNum == K_LSHIFT || keyNum == K_RSHIFT){
		if (!key_boundKeys[keyNum])
			return key_boundKeys[K_SHIFT];
	}

	return key_boundKeys[keyNum];
}

void Key_Event (int keyNum, qBool isDown, uInt time)
{
	char	*kb;
	char	cmd[1024];

	// hack for modal presses
	if (key_waitingKey == -1) {
		if (isDown)
			key_waitingKey = keyNum;
		return;
	}

	// update auto-repeat status
	if (isDown) {
		key_repeatedKeys[keyNum]++;

		if (key_repeatedKeys[keyNum] > 1) {
			// ignore most autorepeats
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
				// allow these to repeat
				break;

			default:
				// this allows ascii to repeat
				if (keyNum < 32 || keyNum > 126) {
					return;
				}
				break;
			}
		}

		if (keyNum >= 200 && keyNum < K_MWHEELDOWN && !key_boundKeys[keyNum])
			Com_Printf (0, "%s is unbound.\n", Key_KeynumToString (keyNum));
	}
	else {
		key_repeatedKeys[keyNum] = 0;
	}

	if (keyNum == K_SHIFT || keyNum == K_LSHIFT || keyNum == K_RSHIFT)
		key_shiftDown = isDown;
	key_capsLockOn = In_GetKeyState (K_CAPSLOCK);

	// console key is hardcoded, so the user can never unbind it
	if (!key_shiftDown && ((keyNum == '`') || (keyNum == '~'))) {
		if (!isDown)
			return;

		Con_ToggleConsole_f ();

		return;
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
			if (cl.attractLoop || cl.cin.frame < 0) {
				// kill the cinematic
				Cbuf_AddText ("killserver\n");
			}
			else if (cl.cin.time > 0) {
				// finish this cinematic
				CIN_FinishCinematic ();
				break;
			}
			else if (cl.frame.playerState.stats[STAT_LAYOUTS]) {
				// put away help computer / inventory
				Cbuf_AddText ("cmd putaway\n");
				break;
			}

			// bring up the menu
			CL_UIModule_MainMenu ();
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
	key_downKeys[keyNum] = isDown;
	if (isDown) {
		if (key_repeatedKeys[keyNum] == 1)
			key_anyKeyDown++;
	}
	else {
		key_anyKeyDown--;
		if (key_anyKeyDown < 0)
			key_anyKeyDown = 0;
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

		if (key_shiftKeys[keyNum] != keyNum) {
			kb = key_boundKeys[key_shiftKeys[keyNum]];
			if (kb && (kb[0] == '+')) {
				Q_snprintfz (cmd, sizeof (cmd), "-%s %i %i\n", kb+1, keyNum, time);
				Cbuf_AddText (cmd);
			}
		}
		return;
	}

	// if not a consolekey, send to the interpreter no matter what mode is
	if ((Key_KeyDest (KD_CONSOLE) && !key_shiftDown && !key_consoleKeys[keyNum])
	|| (Key_KeyDest (KD_GAME) && ((cls.connectState == CA_ACTIVE)
	|| (!key_shiftDown && !key_consoleKeys[keyNum])))) {
		kb = Key_FallBackKey (keyNum);
		if (kb) {
			if (kb[0] == '+') {
				// button commands add keynum and time as a parm
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

	// other systems only care about key down events
	if (!isDown)
		return;

	if (key_shiftDown ^ (key_capsLockOn && keyNum >= 'a' && keyNum <= 'z'))
		keyNum = key_shiftKeys[keyNum];

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
void Key_ClearStates (void)
{
	int		i;

	key_anyKeyDown = 0;
	for (i=0 ; i<K_MAXKEYS ; i++) {
		if (key_downKeys[i] || key_repeatedKeys[i])
			Key_Event (i, qFalse, 0);

		key_downKeys[i] = 0;
		key_repeatedKeys[i] = 0;
	}
}


/*
================
Key_ClearTyping
================
*/
void Key_ClearTyping (void)
{
	key_consoleBuffer[key_consoleEditLine][1] = 0;	// clear any typing
	key_consoleLinePos = 1;
}


/*
===================
Key_GetKey
===================
*/
int Key_GetKey (void)
{
	key_waitingKey = -1;

	while (key_waitingKey == -1)
		Sys_SendKeyEvents ();

	return key_waitingKey;
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
	char	*newbind;
	int		len;

	if (keyNum == -1)
		return;

	// Free old bindings
	if (key_boundKeys[keyNum]) {
		Mem_Free (key_boundKeys[keyNum]);
		key_boundKeys[keyNum] = NULL;
	}

	// Allocate memory for new binding
	len = strlen (binding);	
	newbind = Mem_Alloc (len+1);
	strcpy (newbind, binding);
	newbind[len] = 0;
	key_boundKeys[keyNum] = newbind;	
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
		if (key_boundKeys[i] && key_boundKeys[i][0])
			fprintf (f, "bind %s \"%s\"\n", Key_KeynumToString (i), key_boundKeys[i]);
	}
}


/*
===================
Key_GetBindingBuf
===================
*/
char *Key_GetBindingBuf (int binding)
{
	return key_boundKeys[binding];
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

	return key_downKeys[keyNum];
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
		if (key_boundKeys[i])
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
		if (key_boundKeys[b])
			Com_Printf (0, "\"%s\" = \"%s\"\n", Cmd_Argv (1), key_boundKeys[b]);
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
		if (key_boundKeys[i] && key_boundKeys[i][0] && Q_WildcardMatch (wildCard, key_boundKeys[i], 1))
			Com_Printf (0, "%s \"%s\"\n", Key_KeynumToString(i), key_boundKeys[i]);
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
	// add commands
	//
	Cmd_AddCommand ("bind",			Key_Bind_f,			"Bind a key to do something");
	Cmd_AddCommand ("unbind",		Key_Unbind_f,		"Remove a key binding");
	Cmd_AddCommand ("unbindall",	Key_Unbindall_f,	"Remove all key bindings");
	Cmd_AddCommand ("bindlist",		Key_Bindlist_f,		"Prints a list of key bindings");

	//
	// clear them all
	//
	memset (&key_boundKeys, 0, sizeof (key_boundKeys));
	memset (&key_consoleKeys, qFalse, sizeof (key_consoleKeys));
	memset (&key_shiftKeys, 0, sizeof (key_shiftKeys));

	//
	// set defaults
	//
	for (i=0 ; i<32 ; i++) {
		key_consoleBuffer[i][0] = ']';
		key_consoleBuffer[i][1] = 0;

		key_chatBuffer[i][0] = 0;
	}
	key_consoleLinePos = 1;
	key_chatLinePos = 0;

	//
	// ascii chars are allowed in the console
	//
	for (i=32 ; i<128 ; i++)
		key_consoleKeys[i] = qTrue;

	//
	// other keys allowed to be typed in the console
	//
	key_consoleKeys[K_ENTER			] = qTrue;
	key_consoleKeys[K_KP_ENTER		] = qTrue;
	key_consoleKeys[K_TAB			] = qTrue;
	key_consoleKeys[K_LEFTARROW		] = qTrue;
	key_consoleKeys[K_KP_LEFTARROW	] = qTrue;
	key_consoleKeys[K_RIGHTARROW	] = qTrue;
	key_consoleKeys[K_KP_RIGHTARROW	] = qTrue;
	key_consoleKeys[K_UPARROW		] = qTrue;
	key_consoleKeys[K_KP_UPARROW	] = qTrue;
	key_consoleKeys[K_DOWNARROW		] = qTrue;
	key_consoleKeys[K_KP_DOWNARROW	] = qTrue;
	key_consoleKeys[K_BACKSPACE		] = qTrue;
	key_consoleKeys[K_HOME			] = qTrue;
	key_consoleKeys[K_KP_HOME		] = qTrue;
	key_consoleKeys[K_END			] = qTrue;
	key_consoleKeys[K_KP_END		] = qTrue;
	key_consoleKeys[K_PGUP			] = qTrue;
	key_consoleKeys[K_KP_PGUP		] = qTrue;
	key_consoleKeys[K_PGDN			] = qTrue;
	key_consoleKeys[K_KP_PGDN		] = qTrue;

	key_consoleKeys[K_SHIFT			] = qTrue;
	key_consoleKeys[K_LSHIFT		] = qTrue;
	key_consoleKeys[K_RSHIFT		] = qTrue;

	key_consoleKeys[K_INS			] = qTrue;
	key_consoleKeys[K_DEL			] = qTrue;
	key_consoleKeys[K_KP_INS		] = qTrue;
	key_consoleKeys[K_KP_DEL		] = qTrue;
	key_consoleKeys[K_KP_SLASH		] = qTrue;
	key_consoleKeys[K_KP_PLUS		] = qTrue;
	key_consoleKeys[K_KP_MINUS		] = qTrue;
	key_consoleKeys[K_KP_FIVE		] = qTrue;

	key_consoleKeys[K_MWHEELUP		] = qTrue;
	key_consoleKeys[K_MWHEELDOWN	] = qTrue;
	key_consoleKeys[K_MWHEELLEFT	] = qTrue;
	key_consoleKeys[K_MWHEELRIGHT	] = qTrue;

	key_consoleKeys['`'] = qFalse;
	key_consoleKeys['~'] = qFalse;

	//
	// set what a key looks like when shift is held
	//
	for (i=0 ; i<K_MAXKEYS ; i++)
		key_shiftKeys[i] = i;

	for (i='a' ; i<='z' ; i++)
		key_shiftKeys[i] = i - 'a' + 'A';

	key_shiftKeys['1' ]	= '!';
	key_shiftKeys['2' ]	= '@';
	key_shiftKeys['3' ]	= '#';
	key_shiftKeys['4' ]	= '$';
	key_shiftKeys['5' ]	= '%';
	key_shiftKeys['6' ]	= '^';
	key_shiftKeys['7' ]	= '&';
	key_shiftKeys['8' ]	= '*';
	key_shiftKeys['9' ]	= '(';
	key_shiftKeys['0' ]	= ')';
	key_shiftKeys['-' ]	= '_';
	key_shiftKeys['=' ]	= '+';
	key_shiftKeys[',' ]	= '<';
	key_shiftKeys['.' ]	= '>';
	key_shiftKeys['/' ]	= '?';
	key_shiftKeys[';' ]	= ':';
	key_shiftKeys['\'']	= '"';
	key_shiftKeys['[' ]	= '{';
	key_shiftKeys[']' ]	= '}';
	key_shiftKeys['`' ]	= '~';
	key_shiftKeys['\\']	= '|';
}
