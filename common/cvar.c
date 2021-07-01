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

#define USE_HASH
#include "common.h"

#define MAX_CVAR_HASH		128

cVar_t	*cvarVariables;
cVar_t	*cvarHashTree[MAX_CVAR_HASH];

qBool	cvarUserInfoModified;

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
char *Cvar_BitInfo (int bit) {
	static char	info[MAX_INFO_STRING];
	cVar_t		*var;

	memset (&info, 0, sizeof (info));
	for (var=cvarVariables ; var ; var=var->next) {
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
static qBool Cvar_InfoValidate (char *s) {
	if (strstr (s, "\\") || strstr (s, "\"") || strstr (s, ";"))
		return qFalse;

	return qTrue;
}


/*
============
Cvar_WriteVariables

Appends "set variable value" for all archive flagged variables
============
*/
void Cvar_WriteVariables (FILE *f) {
	cVar_t	*var;
	char	buffer[1024];
	char	*flag, *value;

	for (var=cvarVariables ; var ; var=var->next) {
		if (var->flags & CVAR_ARCHIVE) {
			if (var->flags & CVAR_USERINFO)
				flag = " u";
			else if (var->flags & CVAR_SERVERINFO)
				flag = " s";
			else
				flag = "";

			if ((var->flags & (CVAR_LATCH_SERVER|CVAR_LATCH_VIDEO|CVAR_LATCH_AUDIO)) && var->latchedString)
				value = var->latchedString;
			else
				value = var->string;

			Q_snprintfz (buffer, sizeof (buffer), "seta %s \"%s\"%s\n", var->name, var->string, flag);

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
cVar_t *Cvar_Exists (char *varName) {
	cVar_t	*var;
	uLong	hash;

	if (!varName || !varName[0])
		return NULL;

	hash = CalcHash (varName, MAX_CVAR_HASH);
	for (var=cvarHashTree[hash] ; var ; var=var->hashNext) {
		if (!Q_stricmp (varName, var->name))
			return var;
	}

	return NULL;
}


/*
============
Cvar_Get

If the variable already exists, the value will not be set
The flags will be or'ed in if the variable exists.
============
*/
cVar_t *Cvar_Get (char *varName, char *varValue, int flags) {
	cVar_t	*var;
	uLong	hashValue;

	if (flags & (CVAR_USERINFO|CVAR_SERVERINFO)) {
		if (!Cvar_InfoValidate (varName)) {
			Com_Printf (0, S_COLOR_YELLOW "invalid info cvar name\n");
			return NULL;
		}
	}

	var = Cvar_Exists (varName);
	if (var) {
		Mem_Free (var->defaultString);
		var->defaultString = Mem_StrDup (varValue);
		var->flags |= flags;

		return var;
	}

	if (!varValue)
		return NULL;

	if (flags & (CVAR_USERINFO|CVAR_SERVERINFO)) {
		if (!Cvar_InfoValidate (varValue)) {
			Com_Printf (0, S_COLOR_YELLOW "invalid info cvar value\n");
			return NULL;
		}
	}

	// allocate
	var = Mem_Alloc (sizeof (*var));

	// set values
	var->name = Mem_StrDup (varName);
	var->string = Mem_StrDup (varValue);
	var->defaultString = Mem_StrDup (varValue);
	var->flags = flags;
	var->modified = qTrue;
	var->value = atof (var->string);
	var->integer = atoi (var->string);

	// link it into the variable list
	var->next = cvarVariables;
	cvarVariables = var;

	// link it into the hash tree
	hashValue = CalcHash (varName, MAX_CVAR_HASH);

	var->hashNext = cvarHashTree[hashValue];
	cvarHashTree[hashValue] = var;

	return var;
}


/*
============
Cvar_GetLatchedVars

Any variables with latched values will now be updated
============
*/
void Cvar_GetLatchedVars (int flags) {
	cVar_t	*var;

	for (var=cvarVariables ; var ; var=var->next) {
		if (!(var->flags & flags))
			continue;
		if (!var->latchedString)
			continue;

		Mem_Free (var->string);
		var->string = var->latchedString;
		var->latchedString = NULL;
		var->value = atof (var->string);
		var->integer = atoi (var->string);

		if (var->flags & CVAR_RESET_GAMEDIR)
			FS_SetGamedir (var->string, qFalse);
	}
}


/*
============
Cvar_VariableInteger
============
*/
int Cvar_VariableInteger (char *varName) {
	cVar_t	*var;
	
	var = Cvar_Exists (varName);
	if (!var)
		return 0;

	return atoi (var->string);
}


/*
============
Cvar_VariableString
============
*/
char *Cvar_VariableString (char *varName) {
	cVar_t *var;
	
	var = Cvar_Exists (varName);
	if (!var)
		return "";

	return var->string;
}


/*
============
Cvar_VariableValue
============
*/
float Cvar_VariableValue (char *varName) {
	cVar_t	*var;
	
	var = Cvar_Exists (varName);
	if (!var)
		return 0;

	return atof (var->string);
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
static cVar_t *Cvar_SetVariableValue (cVar_t *var, char *value, qBool force) {
	if (!var)
		return NULL;

	if (var->flags & (CVAR_USERINFO | CVAR_SERVERINFO)) {
		if (!Cvar_InfoValidate (value)) {
			Com_Printf (0, S_COLOR_YELLOW "invalid info cvar value\n");
			return var;
		}
	}

	if (!force) {
		if (var->flags & CVAR_NOSET) {
			Com_Printf (0, "%s is write protected.\n", var->name);
			return var;
		}

		if (var->flags & CVAR_LATCH_SERVER) {
			if (var->latchedString) {
				if (!strcmp (value, var->latchedString))
					return var;
				Mem_Free (var->latchedString);
			}
			else {
				if (!strcmp (value, var->string))
					return var;
			}

			if (Com_ServerState ()) {
				Com_Printf (0, "%s will be changed for next game.\n", var->name);
				var->latchedString = Mem_StrDup (value);
			}
			else {
				var->string = Mem_StrDup (value);
				var->value = atof (var->string);
				var->integer = atoi (var->string);
				if (var->flags & CVAR_RESET_GAMEDIR)
					FS_SetGamedir (var->string, qFalse);
			}
			return var;
		}

		if (var->flags & CVAR_LATCH_VIDEO) {
			if (var->latchedString) {
				if (strcmp (value, var->latchedString) == 0)
					return var;
				Mem_Free (var->latchedString);
			}
			else {
				if (strcmp (value, var->string) == 0)
					return var;
			}

			Com_Printf (0, "%s will be changed after a vid_restart.\n", var->name);
			var->latchedString = Mem_StrDup (value);
			return var;
		}

		if (var->flags & CVAR_LATCH_AUDIO) {
			if (var->latchedString) {
				if (strcmp (value, var->latchedString) == 0)
					return var;
				Mem_Free (var->latchedString);
			}
			else {
				if (strcmp (value, var->string) == 0)
					return var;
			}

			Com_Printf (0, "%s will be changed after a snd_restart.\n", var->name);
			var->latchedString = Mem_StrDup (value);
			return var;
		}
	}
	else {
		if (var->latchedString) {
			Mem_Free (var->latchedString);
			var->latchedString = NULL;
		}
	}

	if (!strcmp (value, var->string))
		return var;	// not changed

	var->modified = qTrue;

	if (var->flags & CVAR_USERINFO)
		cvarUserInfoModified = qTrue;	// transmit at next opportunity
	
	Mem_Free (var->string);	// free the old value string
	
	var->string = Mem_StrDup (value);
	var->value = atof (var->string);
	var->integer = atoi (var->string);

	return var;
}


/*
============
Cvar_FindAndSet
============
*/
static cVar_t *Cvar_FindAndSet (char *varName, char *value, qBool force) {
	cVar_t	*var;

	var = Cvar_Exists (varName);
	if (!var) {
		// create it
		return Cvar_Get (varName, value, 0);
	}

	return Cvar_SetVariableValue (var, value, force);
}

// ============================================================================

/*
============
Cvar_VariableForceSet
============
*/
cVar_t *Cvar_VariableForceSet (cVar_t *var, char *value) {
	if (!var)
		return NULL;

	return Cvar_SetVariableValue (var, value, qTrue);
}


/*
============
Cvar_VariableForceSetValue
============
*/
cVar_t *Cvar_VariableForceSetValue (cVar_t *var, float value) {
	char	val[64];

	if (!var)
		return NULL;

	if (value == (int)value)
		Q_snprintfz (val, sizeof (val), "%i", (int)value);
	else
		Q_snprintfz (val, sizeof (val), "%f", value);

	return Cvar_SetVariableValue (var, val, qTrue);
}

// ============================================================================

/*
============
Cvar_ForceSet
============
*/
cVar_t *Cvar_ForceSet (char *varName, char *value) {
	return Cvar_FindAndSet (varName, value, qTrue);
}


/*
============
Cvar_ForceSetValue
============
*/
cVar_t *Cvar_ForceSetValue (char *varName, float value) {
	char	val[64];

	if (value == (int)value)
		Q_snprintfz (val, sizeof (val), "%i", (int)value);
	else
		Q_snprintfz (val, sizeof (val), "%f", value);

	return Cvar_FindAndSet (varName, val, qTrue);
}


/*
============
Cvar_FullSet
============
*/
cVar_t *Cvar_FullSet (char *varName, char *value, int flags) {
	cVar_t	*var;
	
	var = Cvar_Exists (varName);
	if (!var) {
		// create it
		return Cvar_Get (varName, value, flags);
	}

	var->modified = qTrue;

	if (var->flags & CVAR_USERINFO)
		cvarUserInfoModified = qTrue;	// transmit at next opportunity
	
	Mem_Free (var->string);	// free the old value string
	
	var->string = Mem_StrDup (value);
	var->value = atof (var->string);
	var->integer = atoi (var->string);
	var->flags = flags;

	return var;
}


/*
============
Cvar_Set
============
*/
cVar_t *Cvar_Set (char *varName, char *value) {
	return Cvar_FindAndSet (varName, value, qFalse);
}


/*
============
Cvar_SetValue
============
*/
cVar_t *Cvar_SetValue (char *varName, float value) {
	char	val[64];

	if (value == (int)value)
		Q_snprintfz (val, sizeof (val), "%i", (int)value);
	else
		Q_snprintfz (val, sizeof (val), "%f", value);

	return Cvar_Set (varName, val);
}

// ============================================================================

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qBool Cvar_Command (void) {
	cVar_t			*var;

	// check variables
	var = Cvar_Exists (Cmd_Argv (0));
	if (!var)
		return qFalse;
		
	// perform a variable print or set
	if (Cmd_Argc () == 1) {
		Com_Printf (0, "\"%s\" is: \"%s" S_STYLE_RETURN "\"", var->name, var->string);
		if (!(var->flags & CVAR_NOSET))
			Com_Printf (0, " default: \"%s" S_STYLE_RETURN "\"", var->defaultString);
		Com_Printf (0, "\n");
		return qTrue;
	}

	Cvar_SetVariableValue (var, Cmd_Argv (1), qFalse);
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
static void Cvar_Set_f (void) {
	int		c;
	int		flags;

	c = Cmd_Argc ();
	if ((c != 3) && (c != 4)) {
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

		Cvar_FullSet (Cmd_Argv (1), Cmd_Argv (2), flags);
	}
	else
		Cvar_Set (Cmd_Argv (1), Cmd_Argv (2));
}

static void Cvar_SetA_f (void) {
	int		c;
	int		flags;

	c = Cmd_Argc ();
	if ((c != 3) && (c != 4)) {
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

	Cvar_FullSet (Cmd_Argv (1), Cmd_Argv (2), CVAR_ARCHIVE|flags);
}

static void Cvar_SetS_f (void) {
	if (Cmd_Argc () != 3) {
		Com_Printf (0, "usage: sets <variable> <value>\n");
		return;
	}

	Cvar_FullSet (Cmd_Argv (1), Cmd_Argv (2), CVAR_SERVERINFO);
}

static void Cvar_SetU_f (void) {
	if (Cmd_Argc () != 3) {
		Com_Printf (0, "usage: setu <variable> <value>\n");
		return;
	}

	Cvar_FullSet (Cmd_Argv (1), Cmd_Argv (2), CVAR_USERINFO);
}


/*
================
Cvar_Reset_f
================
*/
static void Cvar_Reset_f (void) {
	cVar_t	*var;

	if (Cmd_Argc () != 2) {
		Com_Printf (0, "usage: reset <variable>\n");
		return;
	}

	var = Cvar_Exists (Cmd_Argv (1));
	if (!var) {
		Com_Printf (0, "'%s' is not a registered cvar\n", Cmd_Argv (1));
		return;
	}

	Cvar_FullSet (var->name, var->defaultString, var->flags);
}


/*
================
Cvar_Toggle_f
================
*/
static void Cvar_Toggle_f (void) {	
	char	*name, *string;
	char	*opt1 = "1", *opt2 = "0";

	if (Cmd_Argc () < 2) {
		Com_Printf (0, "usage: toggle <cvar> [option1] [option2]\n");
		return;
	}

	name = Cmd_Argv (1);
	if (Cmd_Argc () > 2)
		opt1 = Cmd_Argv (2);

	if (Cmd_Argc () > 3)
		opt2 = Cmd_Argv (3);
	
	string = Cvar_VariableString (name);

	if (Q_stricmp (string, opt1))
		Cvar_Set (name, opt1);
	else
		Cvar_Set (name, opt2);  
}


/*
================
Cvar_IncVar_f
================
*/
static void Cvar_IncVar_f (void) {	
	char	*cvarName;
	float	inc;
	cVar_t	*cvar;
	
	if (Cmd_Argc () < 2) {
		Com_Printf (0, "usage: inc <cvar> [amount]\n");
		return;
	}

	cvarName = Cmd_Argv (1);
	if (Cmd_Argc () > 2)
		inc = atof (Cmd_Argv (2));
	else
		inc = 1;
	
	cvar = Cvar_Get (cvarName, "0", 0);
	Cvar_SetValue (cvarName, cvar->value + inc);  
}


/*
================
Cvar_DecVar_f
================
*/
static void Cvar_DecVar_f (void) {	
	char	*cvarName;
	float	dec;
	cVar_t	*cvar;
	
	if (Cmd_Argc () < 2) {
		Com_Printf (0, "usage: dec <cvar> [amount]\n");
		return;
	}

	cvarName = Cmd_Argv (1);
	if (Cmd_Argc () > 2)
		dec = atof (Cmd_Argv (2));
	else
		dec = 1;
	
	cvar = Cvar_Get (cvarName, "0", 0);
	Cvar_SetValue (cvarName, cvar->value - dec);  
}


/*
============
Cvar_List_f
============
*/
static int alphaSortCmp (const cVar_t *_a, const cVar_t *_b) {
	const cVar_t *a = (const cVar_t *) _a;
	const cVar_t *b = (const cVar_t *) _b;

	if (a && a->name && b && b->name)
		return Q_stricmp (a->name, b->name);

	return 0;
}
#define MAX_CVARLIST 512
static void Cvar_List_f (void) {
	cVar_t	*var;
	cVar_t	baseList[MAX_CVARLIST];
	int		total, matching, i;
	char	*wildCard;

	if ((Cmd_Argc () != 1) && (Cmd_Argc () != 2)) {
		Com_Printf (0, "usage: cvarlist [wildcard]\n");
		return;
	}

	if (Cmd_Argc () == 2)
		wildCard = Q_VarArgs ("*%s*", Cmd_Argv (1));
	else
		wildCard = "*";

	// create a list
	for (matching=0, total=0, var=cvarVariables ; var && matching<MAX_CVARLIST ; var=var->next, total++) {
		if (!Q_WildCmp (wildCard, var->name, 1))
			continue;

		baseList[matching++] = *var;
	}

	// sort it
	qsort (&baseList, matching, sizeof (cVar_t), alphaSortCmp);

	// print it
	for (i=0 ; i<matching ;  i++) {
		var = &baseList[i];

		Com_Printf (0, "%s", (var->flags & CVAR_ARCHIVE) ?		"*" : " ");
		Com_Printf (0, "%s", (var->flags & CVAR_USERINFO) ?		"U" : " ");
		Com_Printf (0, "%s", (var->flags & CVAR_SERVERINFO) ?	"S" : " ");
		Com_Printf (0, "%s", (var->flags & CVAR_NOSET) ?		"-" : " ");
		Com_Printf (0, "%s", (var->flags & CVAR_LATCH_SERVER) ?	"LS" : "  ");
		Com_Printf (0, "%s", (var->flags & CVAR_LATCH_VIDEO) ?	"LV" : "  ");
		Com_Printf (0, "%s", (var->flags & CVAR_LATCH_AUDIO) ?	"LA" : "  ");

		Com_Printf (0, " %s is: \"%s" S_STYLE_RETURN "\"", var->name, var->string);
		if (!(var->flags & CVAR_NOSET))
			Com_Printf (0, " default: \"%s" S_STYLE_RETURN "\"", var->defaultString);
		Com_Printf (0, "\n");
	}

	Com_Printf (0, "%i cvars total, %i matching\n", total, matching);
}

/*
===============================================================================

	INIT / SHTUDOWN

===============================================================================
*/

/*
============
Cvar_Init
============
*/
void Cvar_Init (void) {
	memset (cvarHashTree, 0, sizeof (cvarHashTree));

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


/*
============
Cvar_Shutdown
============
*/
void Cvar_Shutdown (void) {
}
