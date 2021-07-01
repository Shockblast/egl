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

#define MAX_ALIAS_HASH	64

aliasCmd_t	*com_aliasList;
aliasCmd_t	*com_aliasHashTree[MAX_ALIAS_HASH];

int			com_aliasCount;

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
aliasCmd_t *Alias_Exists (char *aliasName)
{
	aliasCmd_t	*alias;
	uLong		hash;

	if (!aliasName)
		return NULL;

	hash = CalcHash (aliasName, MAX_ALIAS_HASH);
	for (alias=com_aliasHashTree[hash] ; alias ; alias=alias->hashNext) {
		if (!Q_stricmp (aliasName, alias->name))
			return alias;
	}

	return NULL;
}


/*
============
Alias_AddAlias
============
*/
static void Alias_AddAlias (char *aliasName)
{
	aliasCmd_t	*alias;
	char		cmd[1024];
	int			i, c;

	// check if it's already a command
	if (Cmd_Exists (aliasName)) {
		Com_Printf (0, S_COLOR_YELLOW "Alias_AddAlias: \"%s\" already defined as a command\n", aliasName);
		return;
	}

	// check if it's already a cvar
	if (Cvar_Exists (aliasName)) {
		Com_Printf (0, S_COLOR_YELLOW "Alias_AddAlias: \"%s\" already defined as a cvar\n", aliasName);
		return;
	}

	// check name length
	if (strlen (aliasName) >= MAX_ALIAS_NAME) {
		Com_Printf (0, S_COLOR_YELLOW "Alias_AddAlias: name \"%s\" too long\n", aliasName);
		return;
	}

	// see if it's actually getting a value
	if (Cmd_Argc () < 3) {
		Com_Printf (0, S_COLOR_YELLOW "Alias_AddAlias: no commands for \"%s\" alias\n", aliasName);
		return;
	}

	// if the alias already exists, reuse it
	alias = Alias_Exists (aliasName);
	if (alias) {
		Mem_Free (alias->value);
	}
	else {
		alias = Mem_Alloc (sizeof (aliasCmd_t));
		alias->hashValue = CalcHash (aliasName, MAX_ALIAS_HASH);

		// link it in
		alias->next = com_aliasList;
		com_aliasList = alias;

		// link it into the hash tree
		alias->hashNext = com_aliasHashTree[alias->hashValue];
		com_aliasHashTree[alias->hashValue] = alias;
	}
	Q_strncpyz (alias->name, aliasName, sizeof (alias->name));

	// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	c = Cmd_Argc ();
	for (i=2 ; i<c ; i++) {
		Q_strcatz (cmd, Cmd_Argv (i), sizeof (cmd));
		if (i != (c - 1))
			Q_strcatz (cmd, " ", sizeof (cmd));
	}

	alias->value = Mem_StrDup (cmd);
}


/*
============
Alias_RemoveAlias
============
*/
void Alias_RemoveAlias (char *aliasName)
{
	aliasCmd_t	*alias, **prev;

	if (!aliasName || !aliasName[0]) {
		Com_Printf (0, S_COLOR_YELLOW "Alias_RemoveAlias: NULL aliasName\n");
		return;
	}

	// de-link it from alias list
	prev = &com_aliasList;
	for ( ; ; ) {
		alias = *prev;
		if (!alias) {
			Com_Printf (0, S_COLOR_YELLOW "Alias_RemoveAlias: %s not added\n", aliasName);
			return;
		}

		if (!Q_stricmp (aliasName, alias->name)) {
			*prev = alias->next;
			break;
		}
		prev = &alias->next;
	}

	// de-link it from hash list
	prev = &com_aliasHashTree[alias->hashValue];
	for ( ; ; ) {
		alias = *prev;
		if (!alias) {
			Com_Printf (0, S_COLOR_YELLOW "Alias_RemoveAlias: %s not added to hash list\n", aliasName);
			return;
		}

		if (!Q_stricmp (aliasName, alias->name)) {
			*prev = alias->hashNext;
			Mem_Free (alias);
			return;
		}
		prev = &alias->hashNext;
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
static void Cmd_Alias_f (void)
{
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
static void Cmd_Unalias_f (void)
{
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
static int alphaSortCmp (const aliasCmd_t *_a, const aliasCmd_t *_b) {
	const aliasCmd_t *a = (const aliasCmd_t *) _a;
	const aliasCmd_t *b = (const aliasCmd_t *) _b;

	return Q_stricmp (a->name, b->name);
}
static void Cmd_AliasList_f (void)
{
	aliasCmd_t	*alias, *sortedList;
	int			i, j;
	int			matching;
	int			longest;
	int			total;
	char		*wildCard;

	if ((Cmd_Argc () != 1) && (Cmd_Argc () != 2)) {
		Com_Printf (0, "usage: aliaslist [wildcard]\n");
		return;
	}

	if (Cmd_Argc () == 2)
		wildCard = Cmd_Argv (1);
	else
		wildCard = "*";

	// create a list
	for (matching=0, total=0, alias=com_aliasList ; alias ; alias=alias->next, total++) {
		if (!Q_WildcardMatch (wildCard, alias->name, 1))
			continue;
		matching++;
	}

	if (!matching) {
		Com_Printf (0, "%i aliases total, %i matching\n", total, matching);
		return;
	}

	sortedList = Mem_Alloc (matching * sizeof (aliasCmd_t));
	for (matching=0, longest=0, alias=com_aliasList ; alias ; alias=alias->next) {
		if (!Q_WildcardMatch (wildCard, alias->name, 1))
			continue;

		if ((int)strlen (alias->name) > longest)
			longest = strlen (alias->name);

		sortedList[matching++] = *alias;
	}

	// sort it
	qsort (sortedList, matching, sizeof (sortedList[0]), alphaSortCmp);

	// print it
	longest++;
	for (i=0 ; i<matching ;  i++) {
		alias = &sortedList[i];

		Com_Printf (0, alias->name);
		for (j=0 ; j<longest-(int)strlen(alias->name) ; j++)
			Com_Printf (0, " ");
		Com_Printf (0, "\"%s\"\n", alias->value);
	}

	if (matching)
		Mem_Free (sortedList);
	Com_Printf (0, "%i aliases total, %i matching\n", total, matching);
}

/*
=============================================================================

	INITIALIZATION

=============================================================================
*/

/*
============
Alias_Init
============
*/
void Alias_Init (void)
{
	memset (com_aliasHashTree, 0, sizeof (com_aliasHashTree));

	Cmd_AddCommand ("alias",		Cmd_Alias_f,		"Adds an alias command");
	Cmd_AddCommand ("unalias",		Cmd_Unalias_f,		"Removes an alias command");
	Cmd_AddCommand ("aliaslist",	Cmd_AliasList_f,	"Prints out a list of alias commands");
}
