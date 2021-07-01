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

typedef struct cmdFunc_s {
	struct cmdFunc_s	*next;
	char				*name;
	void				(*function) (void);
	char				*description;

	uint32				hashValue;
	struct cmdFunc_s	*hashNext;
} cmdFunc_t;

extern qBool		com_cmdWait;

void		*Cmd_Exists (char *cmdName);
void		Cmd_CallBack (void (*callBack) (const char *name));

void		*Cmd_AddCommand (char *cmdName, void (*function) (void), char *description);
void		Cmd_RemoveCommand (char *cmdName, void *cmdPtr);

int			Cmd_Argc (void);
char		*Cmd_Argv (int arg);
char		*Cmd_Args (void);

char		*Cmd_MacroExpandString (char *text);
void		Cmd_TokenizeString (char *text, qBool macroExpand);
void		Cmd_ExecuteString (char *text);

void		Cmd_Init (void);

/*
=============================================================================

	COMMAND BUFFER

=============================================================================
*/

void	Cbuf_AddText (char *text);

void	Cbuf_InsertText (char *text);
void	Cbuf_Execute (void);

void	Cbuf_CopyToDefer (void);
void	Cbuf_InsertFromDefer (void);

void	Cbuf_Init (void);
