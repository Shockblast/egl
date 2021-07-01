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

// cmd.c
// - script command processing module

#include "common.h"

#define	MAX_ALIAS_NAME	32

typedef struct cmdAlias_s {
	struct	cmdAlias_s	*next;
	char	name[MAX_ALIAS_NAME];
	char	*value;
} cmdAlias_t;

cmdAlias_t	*cmdAlias;

qBool	cmdWait;

#define	ALIAS_LOOP_COUNT	16
int		aliasCount;		// for detecting runaway loops

// ==========================================================================

/*
================
Cmd_CopyString
================
*/
static char *Cmd_CopyString (char *in) {
	char	*out;
	
	out = Z_TagMalloc (strlen(in)+1, ZTAG_CMDSYS);
	strcpy (out, in);
	return out;
}

/*
=============================================================================

	COMMAND BUFFER

=============================================================================
*/

netMsg_t	cmdText;
byte		cmdTextBuf[8192];

byte		deferTextBuf[8192];

/*
============
Cbuf_Init
============
*/
static void Cbuf_Init (void) {
	MSG_Init (&cmdText, cmdTextBuf, sizeof (cmdTextBuf));
}


/*
============
Cbuf_AddText

Adds command text at the end of the buffer
============
*/
void Cbuf_AddText (char *text) {
	int		l;
	
	l = strlen (text);

	if (cmdText.curSize + l >= cmdText.maxSize) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Cbuf_AddText: overflow\n");
		return;
	}

	MSG_Write (&cmdText, text, strlen (text));
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
	templen = cmdText.curSize;
	if (templen) {
		temp = Z_TagMalloc (templen, ZTAG_CMDSYS);
		memcpy (temp, cmdText.data, templen);
		MSG_Clear (&cmdText);
	} else
		temp = NULL;	// shut up compiler
		
	// add the entire text of the file
	Cbuf_AddText (text);
	
	// add the copied off data
	if (templen) {
		MSG_Write (&cmdText, temp, templen);
		Z_Free (temp);
	}
}


/*
============
Cbuf_CopyToDefer
============
*/
void Cbuf_CopyToDefer (void) {
	memcpy (deferTextBuf, cmdTextBuf, cmdText.curSize);
	deferTextBuf[cmdText.curSize] = 0;
	cmdText.curSize = 0;
}


/*
============
Cbuf_InsertFromDefer
============
*/
void Cbuf_InsertFromDefer (void) {
	Cbuf_InsertText (deferTextBuf);
	deferTextBuf[0] = 0;
}


/*
============
Cbuf_ExecuteText
============
*/
void Cbuf_ExecuteText (int execWhen, char *text) {
	switch (execWhen) {
	case EXEC_NOW:		Cmd_ExecuteString (text);		break;
	case EXEC_INSERT:	Cbuf_InsertText (text);			break;
	case EXEC_APPEND:	Cbuf_AddText (text);			break;
	default:			Com_Error (ERR_FATAL, "Cbuf_ExecuteText: bad execWhen");
	}
}


/*
============
Cbuf_Execute
============
*/
void Cbuf_Execute (void) {
	int		i;
	char	*text;
	char	line[1024];
	int		quotes;

	aliasCount = 0;		// don't allow infinite alias loops

	while (cmdText.curSize) {
		// find a \n or ; line break
		text = (char *)cmdText.data;

		quotes = 0;
		for (i=0 ; i< cmdText.curSize ; i++) {
			if (text[i] == '"')
				quotes++;
			if (!(quotes&1) &&  (text[i] == ';'))
				break;	// don't break if inside a quoted string
			if (text[i] == '\n')
				break;
		}

		if (i > sizeof( line ) - 1)
			i =  sizeof( line ) - 1;

		memcpy (line, text, i);
		line[i] = 0;

		/*
		** delete the text from the command buffer and move remaining commands down
		** this is necessary because commands (exec, alias) can insert data at the
		** beginning of the text buffer
		*/
		if (i == cmdText.curSize)
			cmdText.curSize = 0;
		else {
			i++;
			cmdText.curSize -= i;
			memmove (text, text+i, cmdText.curSize);
		}

		// execute the command line
		Cmd_ExecuteString (line);
		
		if (cmdWait) {
			// skip out while text still remains in buffer, leaving it for next frame
			cmdWait = qFalse;
			break;
		}
	}
}

/*
=============================================================================

	COMMAND EXECUTION

=============================================================================
*/

typedef struct cmdFunction_s {
	struct		cmdFunction_s	*next;
	char		*name;
	void		(*function) (void);
} cmdFunction_t;

static	int			cmd_argc;
static	char		*cmd_argv[MAX_STRING_TOKENS];
static	char		*cmd_null_string = "";
static	char		cmd_args[MAX_STRING_CHARS];

static	cmdFunction_t	*cmd_functions;		// possible commands to execute

/*
============
Cmd_Argc
============
*/
int Cmd_Argc (void) {
	return cmd_argc;
}


/*
============
Cmd_Argv
============
*/
char *Cmd_Argv (int arg) {
	if ((unsigned)arg >= cmd_argc)
		return cmd_null_string;
	return cmd_argv[arg];	
}


/*
============
Cmd_Args

Returns a single string containing argv(1) to argv(argc()-1)
============
*/
char *Cmd_Args (void) {
	return cmd_args;
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
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Line exceeded %i chars, discarded.\n", MAX_STRING_CHARS);
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
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Expanded line exceeded %i chars, discarded.\n", MAX_STRING_CHARS);
			return NULL;
		}

		strncpy (temporary, scan, i);
		strcpy (temporary+i, token);
		strcpy (temporary+i+j, start);

		strcpy (expanded, temporary);
		scan = expanded;
		i--;

		if (++count == 100) {
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Macro expansion loop, discarded.\n");
			return NULL;
		}
	}

	if (inquote) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Line has unmatched quote, discarded.\n");
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
	for (i=0 ; i<cmd_argc ; i++)
		Z_Free (cmd_argv[i]);
		
	cmd_argc = 0;
	cmd_args[0] = 0;
	
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

		// set cmd_args to everything after the first arg
		if (cmd_argc == 1) {
			int		l;

			Q_strcat (cmd_args, text, sizeof (cmd_args));

			// strip off any trailing whitespace
			l = strlen (cmd_args) - 1;
			for ( ; l>=0 ; l--)
				if (cmd_args[l] <= ' ')
					cmd_args[l] = 0;
				else
					break;
		}
			
		com_token = Com_Parse (&text);
		if (!text)
			return;

		if (cmd_argc < MAX_STRING_TOKENS) {
			cmd_argv[cmd_argc] = Z_TagMalloc (strlen(com_token)+1, ZTAG_CMDSYS);
			strcpy (cmd_argv[cmd_argc], com_token);
			cmd_argc++;
		}
	}
}


/*
============
Cmd_AddCommand
============
*/
void Cmd_AddCommand (char *cmd_name, void (*function) (void)) {
	cmdFunction_t	*cmd;
	
	// fail if the command is a variable name
	if (Cvar_VariableString(cmd_name)[0]) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Cmd_AddCommand: %s already defined as a var\n", cmd_name);
		return;
	}
	
	// fail if the command already exists
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next) {
		if (cmd && (!strcmp (cmd_name, cmd->name))) {
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Cmd_AddCommand: %s already defined\n", cmd_name);
			return;
		}
	}

	cmd = Z_TagMalloc (sizeof (cmdFunction_t), ZTAG_CMDSYS);
	cmd->name = cmd_name;
	cmd->function = function;
	cmd->next = cmd_functions;
	cmd_functions = cmd;
}


/*
============
Cmd_RemoveCommand
============
*/
void Cmd_RemoveCommand (char *cmd_name) {
	cmdFunction_t	*cmd, **back;

	back = &cmd_functions;
	for ( ; ; ) {
		cmd = *back;
		if (!cmd) {
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Cmd_RemoveCommand: %s not added\n", cmd_name);
			return;
		}

		if (!strcmp (cmd_name, cmd->name)) {
			*back = cmd->next;
			Z_Free (cmd);
			return;
		}
		back = &cmd->next;
	}
}


/*
============
Cmd_Exists
============
*/
qBool Cmd_Exists (char *cmd_name) {
	cmdFunction_t	*cmd;

	for (cmd=cmd_functions ; cmd ; cmd=cmd->next) {
		if (!strcmp (cmd_name,cmd->name))
			return qTrue;
	}

	return qFalse;
}


/*
============
Cmd_CompleteCommand
============
*/
char *Cmd_CompleteCommand (char *partial) {
	cmdFunction_t	*cmd;
	int				len;
	cmdAlias_t		*a;
	
	len = strlen(partial);
	
	if (!len)
		return NULL;
		
	// check for exact match
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
		if (!strcmp (partial,cmd->name))
			return cmd->name;
	for (a=cmdAlias ; a ; a=a->next)
		if (!strcmp (partial, a->name))
			return a->name;

	// check for partial match
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
		if (!strncmp (partial,cmd->name, len))
			return cmd->name;
	for (a=cmdAlias ; a ; a=a->next)
		if (!strncmp (partial, a->name, len))
			return a->name;

	return NULL;
}


/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
FIXME: lookupnoadd the token to speed search?
============
*/
void Cmd_ExecuteString (char *text) {	
	cmdFunction_t	*cmd;
	cmdAlias_t		*a;

	Cmd_TokenizeString (text, qTrue);
			
	// execute the command line
	if (!Cmd_Argc ())
		return;		// no tokens

	// check functions
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next) {
		if (!Q_stricmp (cmd_argv[0], cmd->name)) {
			if (!cmd->function) {
				// forward to server command
				Cmd_ExecuteString (va("cmd %s", text));
			} else
				cmd->function ();
			return;
		}
	}

	// check alias
	for (a=cmdAlias ; a ; a=a->next) {
		if (!Q_stricmp (cmd_argv[0], a->name)) {
			if (++aliasCount == ALIAS_LOOP_COUNT) {
				Com_Printf (PRINT_ALL, S_COLOR_YELLOW "ALIAS_LOOP_COUNT\n");
				return;
			}
			Cbuf_InsertText (a->value);
			return;
		}
	}
	
	// check cvars
	if (Cvar_Command ())
		return;

	// send it as a server command if we are connected
	Cmd_ForwardToServer ();
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
void Cmd_Exec_f (void) {
	char	*f, *f2;
	int		len;

	if (Cmd_Argc () != 2) {
		Com_Printf (PRINT_ALL, "exec <filename> : execute a script file\n");
		return;
	}

	len = FS_LoadFile (Cmd_Argv (1), (void **)&f);
	if (!f) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "couldn't exec %s\n", Cmd_Argv (1));
		return;
	}
	Com_Printf (PRINT_ALL, "execing %s\n", Cmd_Argv (1));
	
	// the file doesn't have a trailing 0, so we need to copy it off
	f2 = Z_TagMalloc (len + 1, ZTAG_CMDSYS);
	memcpy (f2, f, len);
	f2[len] = 0;

	Cbuf_InsertText (f2);

	Z_Free (f2);
	FS_FreeFile (f);
}


/*
===============
Cmd_Echo_f

Just prints the rest of the line to the console
===============
*/
void Cmd_Echo_f (void) {
	int		i;
	
	for (i=1 ; i<Cmd_Argc () ; i++)
		Com_Printf (PRINT_ALL, "%s ", Cmd_Argv (i));

	Com_Printf (PRINT_ALL, "\n");
}


/*
===============
Cmd_Alias_f

Creates a new command that executes a command string (possibly ; seperated)
===============
*/
void Cmd_Alias_f (void) {
	cmdAlias_t	*a;
	char		cmd[1024];
	int			i, c;
	char		*s;

	if (Cmd_Argc () == 1) {
		Com_Printf (PRINT_ALL, "Current alias commands:\n");
		for (a=cmdAlias ; a ; a=a->next)
			Com_Printf (PRINT_ALL, "%s : %s\n", a->name, a->value);
		return;
	}

	s = Cmd_Argv (1);
	if (strlen(s) >= MAX_ALIAS_NAME) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Alias name is too long\n");
		return;
	}

	// if the alias already exists, reuse it
	for (a=cmdAlias ; a ; a=a->next) {
		if (!strcmp (s, a->name)) {
			Z_Free (a->value);
			break;
		}
	}

	if (!a) {
		a = Z_TagMalloc (sizeof (cmdAlias_t), ZTAG_CMDSYS);
		a->next = cmdAlias;
		cmdAlias = a;
	}
	strcpy (a->name, s);	

	// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	c = Cmd_Argc ();
	for (i=2 ; i< c ; i++) {
		strcat (cmd, Cmd_Argv (i));
		if (i != (c - 1))
			strcat (cmd, " ");
	}
	strcat (cmd, "\n");
	
	a->value = Cmd_CopyString (cmd);
}


/*
============
Cmd_List_f
============
*/
void Cmd_List_f (void) {
	cmdFunction_t	*cmd;
	int				i, j, c;
	char			*wc;

	c = Cmd_Argc ();

	if ((c != 1) && (c != 2)) {
		Com_Printf (PRINT_ALL, "usage: cmdlist [wildcard]\n");
		return;
	}

	if (c == 2)
		wc = va ("*%s*", Cmd_Argv (1));
	else
		wc = "*";

	for (i=0, j=0, cmd=cmd_functions ; cmd ; cmd=cmd->next, i++) {
		if (Com_WildCmp (wc, cmd->name, 1)) {
			j++;
			Com_Printf (PRINT_ALL, "%s\n", cmd->name);
		}
	}
	Com_Printf (PRINT_ALL, "%i commands total, %i matching\n", i, j);
}


/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
============
*/
void Cmd_Wait_f (void) {
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
	Cbuf_Init ();

	Cmd_AddCommand ("cmdlist",	Cmd_List_f);
	Cmd_AddCommand ("exec",		Cmd_Exec_f);
	Cmd_AddCommand ("echo",		Cmd_Echo_f);
	Cmd_AddCommand ("alias",	Cmd_Alias_f);
	Cmd_AddCommand ("wait",		Cmd_Wait_f);
}


/*
============
Cmd_Shutdown
============
*/
void Cmd_Shutdown (void) {
	Cmd_RemoveCommand ("cmdlist");
	Cmd_RemoveCommand ("exec");
	Cmd_RemoveCommand ("echo");
	Cmd_RemoveCommand ("alias");
	Cmd_RemoveCommand ("wait");
}
