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
// cvar.c
//

#include "common.h"

#define MAX_CVAR_HASH		(MAX_CVARS/4)

cVar_t	com_cvarList[MAX_CVARS];
int		com_numCvars;
cVar_t	*com_cvarHashTree[MAX_CVAR_HASH];

qBool	com_userInfoModified;

/*
===============================================================================

	MISCELLANEOUS

===============================================================================
*/

/*
============
Cvar_BitInfo

Returns an info string containing all the 'bit' cvars
============
*/
char *Cvar_BitInfo (int bit)
{
	static char	info[MAX_INFO_STRING];
	cVar_t		*var;
	int			i;

	memset (&info, 0, sizeof (info));
	for (i=0, var=com_cvarList ; i<com_numCvars ; var++, i++) {
		if (var->flags & bit) {
			Info_SetValueForKey (info, var->name, var->string);
		}
	}

	return info;
}


/*
============
Cvar_InfoValidate
============
*/
static qBool Cvar_InfoValidate (char *s)
{
	if (strchr (s, '\\') || strchr (s, '\"') || strchr (s, ';'))
		return qFalse;

	return qTrue;
}


/*
============
Cvar_WriteVariables

Appends "set variable value" for all archive flagged variables
============
*/
void Cvar_WriteVariables (FILE *f)
{
	cVar_t	*var;
	char	buffer[1024];
	char	*value;
	int		i;

	for (i=0, var=com_cvarList ; i<com_numCvars ; var++, i++) {
		if (var->flags & CVAR_ARCHIVE) {
			if ((var->flags & (CVAR_LATCH_SERVER|CVAR_LATCH_VIDEO|CVAR_LATCH_AUDIO)) && var->latchedString)
				value = var->latchedString;
			else
				value = var->string;

			Q_snprintfz (buffer, sizeof (buffer), "seta %s \"%s\"\n", var->name, var->string);

			fprintf (f, "%s", buffer);
		}
	}
}

/*
===============================================================================

	CVAR RETRIEVAL

===============================================================================
*/

/*
============
Cvar_Exists
============
*/
cVar_t *Cvar_Exists (char *varName)
{
	cVar_t	*var;
	uint32	hash;

	if (!varName || !varName[0])
		return NULL;

	hash = Com_HashGeneric (varName, MAX_CVAR_HASH);
	for (var=com_cvarHashTree[hash] ; var ; var=var->hashNext) {
		if (!Q_stricmp (varName, var->name))
			return var;
	}

	return NULL;
}


/*
============
Cvar_CallBack
============
*/
void Cvar_CallBack (void (*callBack) (const char *name))
{
	cVar_t	*var;
	int		i;

	for (i=0, var=com_cvarList ; i<com_numCvars ; var++, i++) {
		callBack (var->name);
	}
}


/*
============
Cvar_Register

If the variable already exists, the value will not be set
The flags will be or'ed in if the variable exists.
============
*/
cVar_t *Cvar_Register (char *varName, char *varValue, int flags)
{
	cVar_t	*var;
	uint32	hashValue;

	if (!varName)
		return NULL;

	// Overwrite commands
	if (Cmd_Exists (varName)) {
		Com_Printf (PRNT_WARNING, "Cvar_Register: command with same name '%s' already exists\n", varName);
		return NULL;
	}

	// Overwrite aliases
	if (Alias_Exists (varName)) {
		Com_Printf (PRNT_WARNING, "Cvar_Register: overwriting alias '%s' with a cvar\n", varName);
		Alias_RemoveAlias (varName);
	}

	// Verify user/serverinfo cvar name
	if (flags & (CVAR_USERINFO|CVAR_SERVERINFO)) {
		if (!Cvar_InfoValidate (varName)) {
			Com_Printf (PRNT_WARNING, "Cvar_Register: '%s' invalid info cvar name\n", varName);
			return NULL;
		}
	}

	// If it already exists, replace the value and return it
	var = Cvar_Exists (varName);
	if (var) {
		Mem_Free (var->defaultString);
		var->defaultString = Mem_PoolStrDup (varValue, com_cvarSysPool, 0);
		var->flags |= flags;

		return var;
	}

	if (!varValue)
		return NULL;

	// Verify user/serverinfo cvar value
	if (flags & (CVAR_USERINFO|CVAR_SERVERINFO)) {
		if (!Cvar_InfoValidate (varValue)) {
			Com_Printf (PRNT_WARNING, "Cvar_Register: '%s' invalid info cvar value\n", varName);
			return NULL;
		}
	}

	// Allocate
	if (com_numCvars >= MAX_CVARS)
		Com_Error (ERR_FATAL, "Cvar_Register: MAX_CVARS");
	var = &com_cvarList[com_numCvars++];
	hashValue = Com_HashGeneric (varName, MAX_CVAR_HASH);

	// Fill it in
	var->name = Mem_PoolStrDup (varName, com_cvarSysPool, 0);
	var->string = Mem_PoolStrDup (varValue, com_cvarSysPool, 0);
	var->defaultString = Mem_PoolStrDup (varValue, com_cvarSysPool, 0);
	var->floatVal = atof (var->string);
	var->intVal = atoi (var->string);

	var->flags = flags;
	var->modified = qTrue;

	// Link it into the hash list
	var->hashNext = com_cvarHashTree[hashValue];
	com_cvarHashTree[hashValue] = var;

	return var;
}


/*
============
Cvar_FixCheatVars
============
*/
void Cvar_FixCheatVars (void)
{
	cVar_t	*var;
	int		i;

	if (!(Com_ServerState () && !Cvar_GetIntegerValue ("cheats"))
	&& !(Com_ClientState () >= CA_CONNECTED && !Com_ServerState ()))
		return;

	for (i=0, var=com_cvarList ; i<com_numCvars ; var++, i++) {
		if (!(var->flags & CVAR_CHEAT))
			continue;

		Mem_Free (var->string);
		var->string = Mem_PoolStrDup (var->defaultString, com_cvarSysPool, 0);
		var->floatVal = atof (var->string);
		var->intVal = atoi (var->string);
	}
}


/*
============
Cvar_GetLatchedVars

Any variables with latched values will now be updated
============
*/
void Cvar_GetLatchedVars (int flags)
{
	cVar_t	*var;
	int		i;

	for (i=0, var=com_cvarList ; i<com_numCvars ; var++, i++) {
		if (!(var->flags & flags))
			continue;
		if (!var->latchedString)
			continue;

		Mem_Free (var->string);
		var->string = var->latchedString;
		var->latchedString = NULL;
		var->floatVal = atof (var->string);
		var->intVal = atoi (var->string);

		if (var->flags & CVAR_RESET_GAMEDIR)
			FS_SetGamedir (var->string, qFalse);
	}
}


/*
============
Cvar_GetIntegerValue
============
*/
int Cvar_GetIntegerValue (char *varName)
{
	cVar_t	*var;
	
	var = Cvar_Exists (varName);
	if (!var)
		return 0;

	return var->intVal;
}


/*
============
Cvar_GetStringValue
============
*/
char *Cvar_GetStringValue (char *varName)
{
	cVar_t *var;
	
	var = Cvar_Exists (varName);
	if (!var)
		return "";

	return var->string;
}


/*
============
Cvar_GetFloatValue
============
*/
float Cvar_GetFloatValue (char *varName)
{
	cVar_t	*var;
	
	var = Cvar_Exists (varName);
	if (!var)
		return 0;

	return var->floatVal;
}

/*
===============================================================================

	CVAR SETTING

===============================================================================
*/

/*
============
Cvar_SetVariableValue
============
*/
static cVar_t *Cvar_SetVariableValue (cVar_t *var, char *value, int flags, qBool force)
{
	if (!var)
		return NULL;

	var->flags |= flags;

	// Validate the userinfo string
	if (var->flags & (CVAR_USERINFO|CVAR_SERVERINFO)) {
		if (!Cvar_InfoValidate (value)) {
			Com_Printf (PRNT_WARNING, "Cvar_SetVariableValue: '%s' invalid info cvar value\n", var->name);
			return var;
		}
	}

	if (force) {
		if (var->latchedString) {
			Mem_Free (var->latchedString);
			var->latchedString = NULL;
		}
	}
	else {
		// Don't touch if read only
		if (var->flags & CVAR_READONLY) {
			Com_Printf (0, "Cvar_SetVariableValue: '%s' is write protected.\n", var->name);
			return var;
		}

		// Check cheat vars
		if (var->flags & CVAR_CHEAT) {
			if ((Com_ServerState () && !Cvar_GetIntegerValue ("cheats"))
			|| (Com_ClientState () >= CA_CONNECTING && !Com_ServerState ())) {
				// Some modifications use this to check a variable value
				// We still want them to get the info, just don't change the variable
				if (var->flags & CVAR_USERINFO)
					com_userInfoModified = qTrue;	// Transmit at next opportunity

				if (!strcmp (value, var->string))
					return var;
				Com_Printf (0, "Cvar_SetVariableValue: '%s' is cheat protected.\n", var->name);
				return var;
			}
		}

		if (var->flags & (CVAR_LATCH_SERVER|CVAR_LATCH_VIDEO|CVAR_LATCH_AUDIO)) {
			// Not changed
			if (var->latchedString) {
				if (!strcmp (value, var->latchedString))
					return var;
				Mem_Free (var->latchedString);
			}
			else if (!strcmp (value, var->string))
				return var;

			// Change the latched value
			if (var->flags & CVAR_LATCH_SERVER) {
				if (Com_ServerState ()) {
					Com_Printf (0, "Cvar_SetVariableValue: '%s' will be changed to '%s' for next game.\n", var->name, value);
					var->latchedString = Mem_PoolStrDup (value, com_cvarSysPool, 0);
					return var;
				}
			}
			else if (var->flags & CVAR_LATCH_VIDEO) {
				Com_Printf (0, "Cvar_SetVariableValue: '%s' will be changed to '%s' after a vid_restart.\n", var->name, value);
				var->latchedString = Mem_PoolStrDup (value, com_cvarSysPool, 0);
				return var;
			}
			else if (var->flags & CVAR_LATCH_AUDIO) {
				Com_Printf (0, "Cvar_SetVariableValue: '%s' will be changed to '%s' after a snd_restart.\n", var->name, value);
				var->latchedString = Mem_PoolStrDup (value, com_cvarSysPool, 0);
				return var;
			}
		}
	}

	// Not changed
	if (!strcmp (value, var->string))
		return var;

	// Free the old value string
	Mem_Free (var->string);

	// Fill in the new value
	var->string = Mem_PoolStrDup (value, com_cvarSysPool, 0);
	var->floatVal = atof (var->string);
	var->intVal = atoi (var->string);

	// It has changed
	var->modified = qTrue;
	if (var->flags & CVAR_USERINFO)
		com_userInfoModified = qTrue;	// Transmit at next opportunity

	// Update the game directory
	if (var->flags & CVAR_RESET_GAMEDIR)
		FS_SetGamedir (var->string, qFalse);

	return var;
}


/*
============
Cvar_FindAndSet
============
*/
static cVar_t *Cvar_FindAndSet (char *varName, char *value, int flags, qBool force)
{
	cVar_t	*var;

	var = Cvar_Exists (varName);
	if (!var) {
		// Create it
		return Cvar_Register (varName, value, flags);
	}

	return Cvar_SetVariableValue (var, value, flags, force);
}

// ============================================================================

/*
============
Cvar_VariableSet
============
*/
cVar_t *Cvar_VariableSet (cVar_t *var, char *value, qBool force)
{
	if (!var)
		return NULL;

	return Cvar_SetVariableValue (var, value, 0, force);
}


/*
============
Cvar_VariableSetValue
============
*/
cVar_t *Cvar_VariableSetValue (cVar_t *var, float value, qBool force)
{
	char	val[64];

	if (!var)
		return NULL;

	if (value == (int)value)
		Q_snprintfz (val, sizeof (val), "%i", (int)value);
	else
		Q_snprintfz (val, sizeof (val), "%f", value);

	return Cvar_SetVariableValue (var, val, 0, force);
}


/*
============
Cvar_VariableReset
============
*/
cVar_t *Cvar_VariableReset (cVar_t *var, qBool force)
{
	if (!var)
		return NULL;

	return Cvar_SetVariableValue (var, var->defaultString, 0, force);
}

// ============================================================================

/*
============
Cvar_Set
============
*/
cVar_t *Cvar_Set (char *varName, char *value, qBool force)
{
	return Cvar_FindAndSet (varName, value, 0, force);
}


/*
============
Cvar_SetValue
============
*/
cVar_t *Cvar_SetValue (char *varName, float value, qBool force)
{
	char	val[64];

	if (value == (int)value)
		Q_snprintfz (val, sizeof (val), "%i", (int)value);
	else
		Q_snprintfz (val, sizeof (val), "%f", value);

	return Cvar_FindAndSet (varName, val, 0, force);
}


/*
============
Cvar_Reset
============
*/
cVar_t *Cvar_Reset (char *varName, qBool force)
{
	cVar_t	*var;

	// Make sure it exists
	var = Cvar_Exists (varName);
	if (!var) {
		Com_Printf (0, "Cvar_Reset: '%s' is not a registered cvar\n", varName);
		return NULL;
	}

	// Reset
	return Cvar_SetVariableValue (var, var->defaultString, 0, force);
}

// ============================================================================

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qBool Cvar_Command (void)
{
	cVar_t			*var;

	// Check variables
	var = Cvar_Exists (Cmd_Argv (0));
	if (!var)
		return qFalse;

	// Perform a variable print or set
	if (Cmd_Argc () == 1) {
		Com_Printf (0, "\"%s\" is: \"%s" S_STYLE_RETURN "\"", var->name, var->string);
		if (!(var->flags & CVAR_READONLY))
			Com_Printf (0, " default: \"%s" S_STYLE_RETURN "\"", var->defaultString);
		Com_Printf (0, "\n");
		return qTrue;
	}

	Cvar_SetVariableValue (var, Cmd_Argv (1), 0, qFalse);
	return qTrue;
}

/*
===============================================================================

	CONSOLE FUNCTIONS

===============================================================================
*/

/*
============
Cvar_Set_f

Allows setting and defining of arbitrary cvars from console
============
*/
static void Cvar_Set_f (void)
{
	int		c;
	int		flags;

	c = Cmd_Argc ();
	if (c != 3 && c != 4) {
		Com_Printf (0, "usage: set <variable> <value> [u / s]\n");
		return;
	}

	if (c == 4) {
		if (!Q_stricmp (Cmd_Argv (3), "u"))
			flags = CVAR_USERINFO;
		else if (!Q_stricmp (Cmd_Argv (3), "s"))
			flags = CVAR_SERVERINFO;
		else {
			Com_Printf (0, "flags can only be 'u' or 's'\n");
			return;
		}
	}
	else {
		flags = 0;
	}

	Cvar_FindAndSet (Cmd_Argv (1), Cmd_Argv (2), flags, qFalse);
}

static void Cvar_SetA_f (void)
{
	int		c;
	int		flags;

	c = Cmd_Argc ();
	if (c != 3 && c != 4) {
		Com_Printf (0, "usage: set <variable> <value> [u / s]\n");
		return;
	}

	if (c == 4) {
		if (!Q_stricmp (Cmd_Argv (3), "u"))
			flags = CVAR_USERINFO;
		else if (!Q_stricmp (Cmd_Argv (3), "s"))
			flags = CVAR_SERVERINFO;
		else {
			Com_Printf (0, "flags can only be 'u' or 's'\n");
			return;
		}
	}
	else
		flags = 0;

	Cvar_FindAndSet (Cmd_Argv (1), Cmd_Argv (2), CVAR_ARCHIVE|flags, qFalse);
}

static void Cvar_SetS_f (void)
{
	if (Cmd_Argc () != 3) {
		Com_Printf (0, "usage: sets <variable> <value>\n");
		return;
	}

	Cvar_FindAndSet (Cmd_Argv (1), Cmd_Argv (2), CVAR_SERVERINFO, qFalse);
}

static void Cvar_SetU_f (void)
{
	if (Cmd_Argc () != 3) {
		Com_Printf (0, "usage: setu <variable> <value>\n");
		return;
	}

	Cvar_FindAndSet (Cmd_Argv (1), Cmd_Argv (2), CVAR_USERINFO, qFalse);
}


/*
================
Cvar_Reset_f
================
*/
static void Cvar_Reset_f (void)
{
	cVar_t	*var;

	// Check args
	if (Cmd_Argc () != 2) {
		Com_Printf (0, "usage: reset <variable>\n");
		return;
	}

	// See if it exists
	var = Cvar_Exists (Cmd_Argv (1));
	if (!var) {
		Com_Printf (0, "'%s' is not a registered cvar\n", Cmd_Argv (1));
		return;
	}

	Cvar_VariableReset (var, qFalse);
}


/*
================
Cvar_Toggle_f
================
*/
static void Cvar_Toggle_f (void)
{
	cVar_t	*var;
	char	*opt1 = "1", *opt2 = "0";
	int		c;

	// Check args
    c = Cmd_Argc ();
	if (c < 2) {
		Com_Printf (0, "usage: toggle <cvar> [option1] [option2]\n");
		return;
	}
	if (c > 2)
		opt1 = Cmd_Argv (2);
	if (c > 3)
		opt2 = Cmd_Argv (3);

	// See if it exists
	var = Cvar_Exists (Cmd_Argv (1));
	if (!var) {
		Com_Printf (0, "'%s' is not a registered cvar\n", Cmd_Argv (1));
		return;
	}

	if (Q_stricmp (var->string, opt1))
		Cvar_VariableSet (var, opt1, qFalse);
	else
		Cvar_VariableSet (var, opt2, qFalse);  
}


/*
================
Cvar_IncVar_f
================
*/
static void Cvar_IncVar_f (void)
{
	cVar_t	*var;
	float	inc;
	int		c;

	// Check args
	c = Cmd_Argc ();
	if (c < 2) {
		Com_Printf (0, "usage: inc <cvar> [amount]\n");
		return;
	}
	if (c > 2)
		inc = atof (Cmd_Argv (2));
	else
		inc = 1;

	// See if it exists
	var = Cvar_Exists (Cmd_Argv (1));
	if (!var) {
		Com_Printf (0, "'%s' is not a registered cvar\n", Cmd_Argv (1));
		return;
	}

	Cvar_VariableSetValue (var, var->floatVal + inc, qFalse);  
}


/*
================
Cvar_DecVar_f
================
*/
static void Cvar_DecVar_f (void)
{
	cVar_t	*var;
	float	dec;
	int		c;

	// Check args
	c = Cmd_Argc ();
	if (c < 2) {
		Com_Printf (0, "usage: dec <cvar> [amount]\n");
		return;
	}
	if (c > 2)
		dec = atof (Cmd_Argv (2));
	else
		dec = 1;

	// See if it exists
	var = Cvar_Exists (Cmd_Argv (1));
	if (!var) {
		Com_Printf (0, "'%s' is not a registered cvar\n", Cmd_Argv (1));
		return;
	}

	Cvar_VariableSetValue (var, var->floatVal - dec, qFalse);  
}


/*
============
Cvar_List_f
============
*/
static int alphaSortCmp (const void *_a, const void *_b)
{
	const cVar_t *a = (const cVar_t *) _a;
	const cVar_t *b = (const cVar_t *) _b;

	return Q_stricmp (a->name, b->name);
}
static void Cvar_List_f (void)
{
	cVar_t	*var, *sortedList;
	int		total, matching, i;
	char	*wildCard;
	int		c;

	c = Cmd_Argc ();
	if (c != 1 && c != 2) {
		Com_Printf (0, "usage: cvarlist [wildcard]\n");
		return;
	}

	if (c == 2)
		wildCard = Cmd_Argv (1);
	else
		wildCard = "*";

	// create a list
	for (matching=0, total=0, i=0, var=com_cvarList ; i<com_numCvars ; var++, total++, i++) {
		if (!Q_WildcardMatch (wildCard, var->name, 1))
			continue;

		matching++;
	}

	if (!matching) {
		Com_Printf (0, "%i cvars total, %i matching\n", total, matching);
		return;
	}

	sortedList = Mem_PoolAlloc (matching * sizeof (cVar_t), com_cvarSysPool, 0);
	for (matching=0, i=0, var=com_cvarList ; i<com_numCvars ; var++, i++) {
		if (!Q_WildcardMatch (wildCard, var->name, 1))
			continue;

		sortedList[matching++] = *var;
	}

	// sort it
	qsort (sortedList, matching, sizeof (sortedList[0]), alphaSortCmp);

	// print it
	for (i=0 ; i<matching ;  i++) {
		var = &sortedList[i];

		Com_Printf (0, "%s", (var->flags & CVAR_ARCHIVE) ?		"*" : " ");
		Com_Printf (0, "%s", (var->flags & CVAR_USERINFO) ?		"U" : " ");
		Com_Printf (0, "%s", (var->flags & CVAR_SERVERINFO) ?	"S" : " ");
		Com_Printf (0, "%s", (var->flags & CVAR_READONLY) ?		"-" : " ");
		Com_Printf (0, "%s", (var->flags & CVAR_LATCH_SERVER) ?	"LS" : "  ");
		Com_Printf (0, "%s", (var->flags & CVAR_LATCH_VIDEO) ?	"LV" : "  ");
		Com_Printf (0, "%s", (var->flags & CVAR_LATCH_AUDIO) ?	"LA" : "  ");

		Com_Printf (0, " %s is: \"%s" S_STYLE_RETURN "\"", var->name, var->string);
		if (!(var->flags & CVAR_READONLY))
			Com_Printf (0, " default: \"%s" S_STYLE_RETURN "\"", var->defaultString);
		Com_Printf (0, "\n");
	}

	if (matching)
		Mem_Free (sortedList);
	Com_Printf (0, "%i cvars total, %i matching\n", total, matching);
}

/*
===============================================================================

	INITIALIZATION

===============================================================================
*/

/*
============
Cvar_Init
============
*/
void Cvar_Init (void)
{
	com_numCvars = 0;
	memset (com_cvarHashTree, 0, sizeof (com_cvarHashTree));

	Cmd_AddCommand ("set",		Cvar_Set_f,			"Sets a cvar with a value");
	Cmd_AddCommand ("seta",		Cvar_SetA_f,		"Sets a cvar with a value and adds to be archived");
	Cmd_AddCommand ("sets",		Cvar_SetS_f,		"Sets a cvar with a value and makes it serverinfo");
	Cmd_AddCommand ("setu",		Cvar_SetU_f,		"Sets a cvar with a value and makes it userinfo");
	Cmd_AddCommand ("reset",	Cvar_Reset_f,		"Resets a cvar to it's default value");

	Cmd_AddCommand ("toggle",	Cvar_Toggle_f,		"Toggles a cvar between two values");
	Cmd_AddCommand ("inc",		Cvar_IncVar_f,		"Increments a cvar's value by one");
	Cmd_AddCommand ("dec",		Cvar_DecVar_f,		"Decrements a cvar's value by one");

	Cmd_AddCommand ("cvarlist",	Cvar_List_f,		"Prints out a list of the current cvars");
}
