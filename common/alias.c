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
// alias.c
//

#include "common.h"

aliasCmd_t	*aliasCmds;
int			aliasCount;

/*
=============================================================================

	ALIASES

=============================================================================
*/

/*
============
Alias_Exists
============
*/
aliasCmd_t *Alias_Exists (char *aliasName) {
	aliasCmd_t	*alias;

	if (!aliasName)
		return NULL;

	for (alias=aliasCmds ; alias ; alias=alias->next) {
		if (!Q_stricmp (aliasName, alias->name))
			return alias;
	}

	return NULL;
}


/*
============
Alias_WriteAliases

Appends "alias name value" for all aliases
============
*/
void Alias_WriteAliases (FILE *f) {
	aliasCmd_t	*alias;

	for (alias=aliasCmds ; alias ; alias=alias->next)
		fprintf (f, "alias %s \"%s\"\n", alias->name, alias->value);
}


/*
============
Alias_AddAlias
============
*/
void Alias_AddAlias (char *aliasName) {
	aliasCmd_t	*alias;
	char		cmd[1024];
	int			i, c;

	if (strlen (aliasName) >= MAX_ALIAS_NAME) {
		Com_Printf (0, S_COLOR_YELLOW "Alias name is too long\n");
		return;
	}

	// if the alias already exists as a command, fail
	if (Cmd_Exists (aliasName)) {
		Com_Printf (0, S_COLOR_YELLOW "Cmd_Alias_f: \"%s\" already defined as a command\n", aliasName);
		return;
	}

	// if the alias already exists, reuse it
	alias = Alias_Exists (aliasName);
	if (alias)
		Mem_Free (alias->value);
	else {
		alias = Mem_Alloc (sizeof (aliasCmd_t));
		alias->next = aliasCmds;
		aliasCmds = alias;
	}
	strcpy (alias->name, aliasName);	

	// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	c = Cmd_Argc ();
	for (i=2 ; i< c ; i++) {
		strcat (cmd, Cmd_Argv (i));
		if (i != (c - 1))
			strcat (cmd, " ");
	}
	
	alias->value = Mem_StrDup (cmd);
}


/*
============
Alias_RemoveAlias
============
*/
void Alias_RemoveAlias (char *aliasName) {
	aliasCmd_t	*alias, **back;

	back = &aliasCmds;
	for ( ; ; ) {
		alias = *back;
		if (!alias) {
			Com_Printf (0, S_COLOR_YELLOW "Alias_RemoveAlias: %s not added\n", aliasName);
			return;
		}

		if (!Q_stricmp (aliasName, alias->name)) {
			*back = alias->next;
			Mem_Free (alias);
			return;
		}
		back = &alias->next;
	}
}

/*
==============================================================================

	CONSOLE COMMANDS

==============================================================================
*/

/*
===============
Cmd_Alias_f

Creates a new command that executes a command string (possibly ; seperated)
===============
*/
static void Cmd_Alias_f (void) {
	if (Cmd_Argc () == 1) {
		Com_Printf (0, "syntax: alias <name> <value>; 'aliaslist' to see a list\n");
		return;
	}

	Alias_AddAlias (Cmd_Argv (1));
}

/*
===============
Cmd_Unalias_f

Kills an alias
===============
*/
static void Cmd_Unalias_f (void) {
	if (Cmd_Argc () != 2) {
		Com_Printf (0, "syntax: unalias <name>; 'aliaslist' to see a list\n");
		return;
	}

	Alias_RemoveAlias (Cmd_Argv (1));
}


/*
============
Cmd_AliasList_f
============
*/
static void Cmd_AliasList_f (void) {
	aliasCmd_t	*alias;
	int			i, j;
	char		*wildCard;

	if ((Cmd_Argc () != 1) && (Cmd_Argc () != 2)) {
		Com_Printf (0, "usage: aliaslist [wildcard]\n");
		return;
	}

	if (Cmd_Argc () == 2)
		wildCard = Q_VarArgs ("*%s*", Cmd_Argv (1));
	else
		wildCard = "*";

	for (i=0, j=0, alias=aliasCmds ; alias ; alias=alias->next, i++) {
		if (Q_WildCmp (wildCard, alias->name, 1)) {
			j++;
			Com_Printf (0, "%s \"%s\"\n", alias->name, alias->value);
		}
	}
	Com_Printf (0, "%i aliases total, %i matching\n", i, j);
}

/*
=============================================================================

	INIT / SHUTDOWN

=============================================================================
*/

/*
============
Alias_Init
============
*/
void Alias_Init (void) {
	Cmd_AddCommand ("alias",		Cmd_Alias_f,		"Adds an alias command");
	Cmd_AddCommand ("unalias",		Cmd_Unalias_f,		"Removes an alias command");
	Cmd_AddCommand ("aliaslist",	Cmd_AliasList_f,	"Prints out a list of alias commands");
}


/*
============
Alias_Shutdown
============
*/
void Alias_Shutdown (void) {
}
