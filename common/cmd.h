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
// cmd.h
//

/*
=============================================================================

	CONSOLE COMMANDS

=============================================================================
*/

extern qBool		com_cmdWait;

#define		Cmd_AddCommand(name,func,desc)	_Cmd_AddCommand(qFalse,(name),(func),(desc))

void		*Cmd_Exists (char *name);
void		Cmd_CallBack (void (*callBack) (const char *name));

void		*_Cmd_AddCommand (qBool cGame, char *name, void (*function) (void), const char *description);
void		Cmd_RemoveCommand (char *name, void *command);
void		Cmd_RemoveCGameCmds (void);

int			Cmd_Argc (void);
char		*Cmd_Argv (int arg);
char		*Cmd_Args (void);
char		*Cmd_ArgsOffset (int start);

char		*Cmd_MacroExpandString (char *text);
void		Cmd_TokenizeString (char *text, qBool macroExpand);
void		Cmd_ExecuteString (char *text);

void		Cmd_Init (void);

/*
=============================================================================

	COMMAND BUFFER

=============================================================================
*/

#define	COMMAND_BUFFER_SIZE	0x10000

void	Cbuf_AddText (char *text);

void	Cbuf_InsertText (char *text);
void	Cbuf_Execute (void);

void	Cbuf_CopyToDefer (void);
void	Cbuf_InsertFromDefer (void);

void	Cbuf_Init (void);
