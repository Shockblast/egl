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
// cmd.c
//

#define USE_HASH
#include "common.h"

#define MAX_CMD_HASH	128

cmdFunc_t		*cmdFunctions;
cmdFunc_t		*cmdHashTree[MAX_CMD_HASH];

qBool			cmdWait;

static int		cmdArgc;
static char		*cmdArgv[MAX_STRING_TOKENS];
static char		cmdArgs[MAX_STRING_CHARS];

// ==========================================================================

/*
============
Cmd_Exists
============
*/
cmdFunc_t *Cmd_Exists (char *cmdName) {
	cmdFunc_t	*cmd;
	uLong		hash;

	if (!cmdName)
		return NULL;

	hash = CalcHash (cmdName, MAX_CMD_HASH);
	for (cmd=cmdHashTree[hash] ; cmd ; cmd=cmd->hashNext) {
		if (!Q_stricmp (cmdName, cmd->name))
			return cmd;
	}

	return NULL;
}

/*
=============================================================================

	COMMAND EXECUTION

=============================================================================
*/

/*
============
Cmd_Argc
============
*/
int Cmd_Argc (void) {
	return cmdArgc;
}


/*
============
Cmd_Argv
============
*/
char *Cmd_Argv (int arg) {
	if ((uInt)arg >= cmdArgc)
		return "";
	return cmdArgv[arg];	
}


/*
============
Cmd_Args

Returns a single string containing argv(1) to argv(argc()-1)
============
*/
char *Cmd_Args (void) {
	return cmdArgs;
}


/*
======================
Cmd_MacroExpandString
======================
*/
char *Cmd_MacroExpandString (char *text) {
	int		i, j, count, len;
	qBool	inquote;
	char	*scan;
	static	char	expanded[MAX_STRING_CHARS];
	char	temporary[MAX_STRING_CHARS];
	char	*token, *start;

	inquote = qFalse;
	scan = text;

	len = Q_strlen (scan);
	if (len >= MAX_STRING_CHARS) {
		Com_Printf (0, S_COLOR_YELLOW "Line exceeded %i chars, discarded.\n", MAX_STRING_CHARS);
		return NULL;
	}

	count = 0;

	for (i=0 ; i<len ; i++) {
		if (scan[i] == '"')
			inquote ^= 1;
		if (inquote)
			continue;	// don't expand inside quotes
		if (scan[i] != '$')
			continue;
		// scan out the complete macro
		start = scan+i+1;
		token = Com_Parse (&start);
		if (!start)
			continue;
	
		token = Cvar_VariableString (token);

		j = Q_strlen (token);
		len += j;
		if (len >= MAX_STRING_CHARS) {
			Com_Printf (0, S_COLOR_YELLOW "Expanded line exceeded %i chars, discarded.\n", MAX_STRING_CHARS);
			return NULL;
		}

		strncpy (temporary, scan, i);
		strcpy (temporary+i, token);
		strcpy (temporary+i+j, start);

		strcpy (expanded, temporary);
		scan = expanded;
		i--;

		if (++count == 100) {
			Com_Printf (0, S_COLOR_YELLOW "Macro expansion loop, discarded.\n");
			return NULL;
		}
	}

	if (inquote) {
		Com_Printf (0, S_COLOR_YELLOW "Line has unmatched quote, discarded.\n");
		return NULL;
	}

	return scan;
}


/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
$Cvars will be expanded unless they are in a quoted token
============
*/
void Cmd_TokenizeString (char *text, qBool macroExpand) {
	int		i;
	char	*com_token;

	// clear the args from the last string
	for (i=0 ; i<cmdArgc ; i++)
		Mem_Free (cmdArgv[i]);

	cmdArgc = 0;
	cmdArgs[0] = 0;
	
	// macro expand the text
	if (macroExpand)
		text = Cmd_MacroExpandString (text);

	if (!text)
		return;

	for ( ; ; ) {
		// skip whitespace up to a /n
		while (*text && (*text <= ' ') && (*text != '\n'))
			text++;
		
		if (*text == '\n') {
			// a newline seperates commands in the buffer
			text++;
			break;
		}

		if (!*text)
			return;

		// set cmdArgs to everything after the first arg
		if (cmdArgc == 1) {
			int		l;

			Q_strcat (cmdArgs, text, sizeof (cmdArgs));

			// strip off any trailing whitespace
			l = Q_strlen (cmdArgs) - 1;
			for ( ; l>=0 ; l--)
				if (cmdArgs[l] <= ' ')
					cmdArgs[l] = 0;
				else
					break;
		}
			
		com_token = Com_Parse (&text);
		if (!text)
			return;

		if (cmdArgc < MAX_STRING_TOKENS) {
			cmdArgv[cmdArgc] = Mem_StrDup (com_token);
			cmdArgc++;
		}
	}
}


/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
FIXME: lookupnoadd the token to speed search?
============
*/
void Cmd_ExecuteString (char *text) {	
	cmdFunc_t	*cmd;
	aliasCmd_t	*alias;

	Cmd_TokenizeString (text, qTrue);
			
	// execute the command line
	if (!Cmd_Argc ())
		return;		// no tokens

	// check functions
	cmd = Cmd_Exists (cmdArgv[0]);
	if (cmd) {
		if (!cmd->function) {
			// forward to server command
			Cmd_ExecuteString (Q_VarArgs ("cmd %s", text));
		}
		else
			cmd->function ();
		return;
	}

	// check alias
	alias = Alias_Exists (cmdArgv[0]);
	if (alias) {
		if (++aliasCount == MAX_ALIAS_LOOPS) {
			Com_Printf (0, S_COLOR_YELLOW "Cmd_ExecuteString: MAX_ALIAS_LOOPS hit\n");
			return;
		}

		Cbuf_InsertText (Q_VarArgs ("%s\n", alias->value));
		return;
	}
	
	// check cvars
	if (Cvar_Command ())
		return;

	// send it as a server command if we are connected
	Cmd_ForwardToServer ();
}


/*
============
Cmd_AddCommand
============
*/
void Cmd_AddCommand (char *cmdName, void (*function) (void), char *description) {
	cmdFunc_t	*cmd;
	
	// fail if the command is a variable name
	if (Cvar_Exists (cmdName)) {
		Com_Printf (0, S_COLOR_YELLOW "Cmd_AddCommand: \"%s\" already defined as a cvar\n", cmdName);
		return;
	}

	// fail if the command already exists
	if (Cmd_Exists (cmdName)) {
		Com_Printf (0, S_COLOR_YELLOW "Cmd_AddCommand: \"%s\" already defined as a command\n", cmdName);
		return;
	}

	// don't fail if the alias already exists
	if (Alias_Exists (cmdName)) {
		Com_Printf (0, S_COLOR_YELLOW "Cmd_AddCommand: overwriting alias \"%s\" with a command\n", cmdName);
		Alias_RemoveAlias (cmdName);
	}

	// fill it in
	cmd = Mem_Alloc (sizeof (cmdFunc_t));
	cmd->name = Mem_StrDup (cmdName);
	cmd->function = function;
	cmd->description = description;

	// link it into the command list
	cmd->next = cmdFunctions;
	cmdFunctions = cmd;

	// link it into the hash tree
	cmd->hashValue = CalcHash (cmdName, MAX_CMD_HASH);

	cmd->hashNext = cmdHashTree[cmd->hashValue];
	cmdHashTree[cmd->hashValue] = cmd;
}


/*
============
Cmd_RemoveCommand
============
*/
void Cmd_RemoveCommand (char *cmdName) {
	cmdFunc_t	*cmd, **prev;

	// de-link it from command list
	prev = &cmdFunctions;
	for ( ; ; ) {
		cmd = *prev;
		if (!cmd) {
			Com_Printf (0, S_COLOR_YELLOW "Cmd_RemoveCommand: %s not added\n", cmdName);
			return;
		}

		if (!Q_stricmp (cmdName, cmd->name)) {
			*prev = cmd->next;
			break;
		}
		prev = &cmd->next;
	}

	// de-link it from hash list
	prev = &cmdHashTree[cmd->hashValue];
	for ( ; ; ) {
		cmd = *prev;
		if (!cmd) {
			Com_Printf (0, S_COLOR_YELLOW "Cmd_RemoveCommand: %s not added\n", cmdName);
			return;
		}

		if (!Q_stricmp (cmdName, cmd->name)) {
			*prev = cmd->hashNext;
			Mem_Free (cmd->name);
			Mem_Free (cmd);
			return;
		}
		prev = &cmd->hashNext;
	}
}

/*
==============================================================================

	CONSOLE COMMANDS

==============================================================================
*/

/*
===============
Cmd_Exec_f
===============
*/
static void Cmd_Exec_f (void) {
	char	*f, *f2;
	int		len;

	if (Cmd_Argc () != 2) {
		Com_Printf (0, "syntax: exec <filename> : execute a script file\n");
		return;
	}

	len = FS_LoadFile (Cmd_Argv (1), (void **)&f);
	if (!f) {
		Com_Printf (0, S_COLOR_YELLOW "couldn't exec %s\n", Cmd_Argv (1));
		return;
	}
	Com_Printf (0, "executing %s\n", Cmd_Argv (1));

	// the file doesn't have a trailing \n\0, so we need to put it there
	f2 = Mem_Alloc (len + 2);
	memcpy (f2, f, len);
	f2[len] = '\n';
	f2[len+1] = '\0';

	Cbuf_InsertText (f2);

	Mem_Free (f2);
	FS_FreeFile (f);
}


/*
===============
Cmd_Echo_f

Just prints the rest of the line to the console
===============
*/
static void Cmd_Echo_f (void) {
	int		i;
	
	for (i=1 ; i<Cmd_Argc () ; i++) {
		Com_Printf (0, "%s ", Cmd_Argv (i));
	}

	Com_Printf (0, "\n");
}


/*
============
Cmd_List_f
============
*/
static int alphaSortCmp (const cmdFunc_t *_a, const cmdFunc_t *_b) {
	const cmdFunc_t *a = (const cmdFunc_t *) _a;
	const cmdFunc_t *b = (const cmdFunc_t *) _b;

	if (a && a->name && b && b->name)
		return Q_stricmp (a->name, b->name);

	return 0;
}
#define MAX_CMDLIST 512
static void Cmd_List_f (void) {
	cmdFunc_t	*cmd;
	cmdFunc_t	baseList[MAX_CMDLIST];
	int			i, j, total;
	int			matching, longest;
	char		*wildCard;

	if ((Cmd_Argc () != 1) && (Cmd_Argc () != 2)) {
		Com_Printf (0, "usage: cmdlist [wildcard]\n");
		return;
	}

	if (Cmd_Argc () == 2)
		wildCard = Q_VarArgs ("*%s*", Cmd_Argv (1));
	else
		wildCard = "*";

	// create a list and get longest cmd length
	for (matching=0, total=0, longest=0, cmd=cmdFunctions ; cmd && matching<MAX_CMDLIST ; cmd=cmd->next, total++) {
		if (Q_WildCmp (wildCard, cmd->name, 1)) {
			if (Q_strlen (cmd->name) > longest)
				longest = Q_strlen (cmd->name);
			baseList[matching++] = *cmd;
		}
	}

	// sort it
	qsort (&baseList, matching, sizeof (cmdFunc_t), alphaSortCmp);

	// print it
	longest++;
	for (j=0 ; j<matching ;  j++) {
		cmd = &baseList[j];

		Com_Printf (0, "%s ", cmd->name);
		for (i=0 ; i<longest-Q_strlen(cmd->name) ; i++)
			Com_Printf (0, " ");
		Com_Printf (0, "%s\n", cmd->description);
	}

	Com_Printf (0, "%i commands total, %i matching\n", total, matching);
}


/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until next frame.
This allows commands like: bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
============
*/
static void Cmd_Wait_f (void) {
	cmdWait = qTrue;
}

/*
=============================================================================

	INIT / SHUTDOWN

=============================================================================
*/

/*
============
Cmd_Init
============
*/
void Cmd_Init (void) {
	memset (cmdHashTree, 0, sizeof (cmdHashTree));

	Cmd_AddCommand ("cmdlist",		Cmd_List_f,		"Prints out a list of commands");
	Cmd_AddCommand ("echo",			Cmd_Echo_f,		"Echos text to the console");
	Cmd_AddCommand ("exec",			Cmd_Exec_f,		"Execute a cfg file");
	Cmd_AddCommand ("wait",			Cmd_Wait_f,		"Forces remainder of command buffer to be delayed until next frame");
}


/*
============
Cmd_Shutdown
============
*/
void Cmd_Shutdown (void) {
}
