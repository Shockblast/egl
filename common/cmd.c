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

#include "common.h"

cmdFunc_t		*cmdFunctions;
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

	if (!cmdName)
		return NULL;

	for (cmd=cmdFunctions ; cmd ; cmd=cmd->next) {
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

	len = strlen (scan);
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

		j = strlen(token);
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
			l = strlen (cmdArgs) - 1;
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
			cmdArgv[cmdArgc] = Mem_Alloc (strlen(com_token)+1);
			strcpy (cmdArgv[cmdArgc], com_token);
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
		} else
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
	aliasCmd_t	*alias;
	
	// fail if the command is a variable name
	if (Cvar_VariableString (cmdName)[0]) {
		Com_Printf (0, S_COLOR_YELLOW "Cmd_AddCommand: \"%s\" already defined as a cvar\n", cmdName);
		return;
	}

	// fail if the command already exists
	if (Cmd_Exists (cmdName)) {
		Com_Printf (0, S_COLOR_YELLOW "Cmd_AddCommand: \"%s\" already defined as a command\n", cmdName);
		return;
	}

	// don't fail if the alias already exists
	alias = Alias_Exists (cmdName);
	if (alias) {
		Com_Printf (0, S_COLOR_YELLOW "Cmd_AddCommand: overwriting alias \"%s\" with a command\n", cmdName);
		Alias_RemoveAlias (cmdName);
	}

	// fill it in
	cmd = Mem_Alloc (sizeof (cmdFunc_t));
	cmd->name = cmdName;
	cmd->function = function;
	cmd->description = description;

	// link it in
	cmd->next = cmdFunctions;
	cmdFunctions = cmd;
}


/*
============
Cmd_RemoveCommand
============
*/
void Cmd_RemoveCommand (char *cmdName) {
	cmdFunc_t	*cmd, **back;

	back = &cmdFunctions;
	for ( ; ; ) {
		cmd = *back;
		if (!cmd) {
			Com_Printf (0, S_COLOR_YELLOW "Cmd_RemoveCommand: %s not added\n", cmdName);
			return;
		}

		if (!Q_stricmp (cmdName, cmd->name)) {
			*back = cmd->next;
			Mem_Free (cmd);
			return;
		}
		back = &cmd->next;
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
	
	for (i=1 ; i<Cmd_Argc () ; i++)
		Com_Printf (0, "%s ", Cmd_Argv (i));

	Com_Printf (0, "\n");
}


/*
============
Cmd_List_f
============
*/
static void Cmd_List_f (void) {
	cmdFunc_t	*cmd;
	int			i, j;
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

	// get longest cmd length
	for (i=0, longest=0, cmd=cmdFunctions ; cmd ; cmd=cmd->next, i++) {
		if (Q_WildCmp (wildCard, cmd->name, 1)) {
			if (strlen (cmd->name) > longest)
				longest = strlen (cmd->name);
		}
	}

	// print
	for (i=0, matching=0, cmd=cmdFunctions ; cmd ; cmd=cmd->next, i++) {
		if (Q_WildCmp (wildCard, cmd->name, 1)) {
			matching++;

			Com_Printf (0, "%s ", cmd->name);
			for (j=0 ; j<longest-strlen(cmd->name) ; j++)
				Com_Printf (0, " ");
			Com_Printf (0, "%s\n", cmd->description);
		}
	}
	Com_Printf (0, "%i commands total, %i matching\n", i, matching);
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
