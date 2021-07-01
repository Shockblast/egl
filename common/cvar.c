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

// cvar.c
// - dynamic variable tracking

#include "common.h"

cvar_t	*cvar_Variables;

qBool	cvar_UserInfoModified;

/*
===============================================================================

	MISCELLANEOUS

===============================================================================
*/

/*
============
Cvar_WriteVariables

Appends lines containing "set variable value" for all variables
with the archive flag set to qTrue.
============
*/
void Cvar_WriteVariables (FILE *f) {
	cvar_t	*var;
	char	buffer[1024];

	for (var=cvar_Variables ; var ; var=var->next) {
		if (var->flags & CVAR_ARCHIVE) {
			Com_sprintf (buffer, sizeof (buffer), "set %s \"%s\"\n", var->name, var->string);
			fprintf (f, "%s", buffer);
		}
	}
}


/*
============
Cvar_CompleteVariable
============
*/
char *Cvar_CompleteVariable (char *partial) {
	cvar_t		*cvar;
	int			len;
	
	len = strlen(partial);
	
	if (!len)
		return NULL;

	// kthx cycle through possibilities

	// check exact match
	for (cvar=cvar_Variables ; cvar ; cvar=cvar->next)
		if (!strcmp (partial, cvar->name))
			return cvar->name;

	// check partial match
	for (cvar=cvar_Variables ; cvar ; cvar=cvar->next)
		if (!strncmp (partial, cvar->name, len))
			return cvar->name;

	return NULL;
}


/*
============
Cvar_BitInfo

Returns an info string containing all the 'bit' cvars
============
*/
static char *Cvar_BitInfo (int bit) {
	static char	info[MAX_INFO_STRING];
	cvar_t		*var;

	info[0] = 0;
	for (var=cvar_Variables ; var ; var=var->next) {
		if (var->flags & bit)
			Info_SetValueForKey (info, var->name, var->string);
	}

	return info;
}


/*
============
Cvar_Userinfo

Returns an info string containing all the CVAR_USERINFO cvars
============
*/
char *Cvar_Userinfo (void) {
	return Cvar_BitInfo (CVAR_USERINFO);
}


/*
============
Cvar_Serverinfo

Returns an info string containing all the CVAR_SERVERINFO cvars
============
*/
char *Cvar_Serverinfo (void) {
	return Cvar_BitInfo (CVAR_SERVERINFO);
}


/*
================
Cvar_CopyString
================
*/
static char *Cvar_CopyString (char *in) {
	char	*out;
	
	out = Z_TagMalloc (strlen(in)+1, ZTAG_CVARSYS);
	strcpy (out, in);
	return out;
}


/*
============
Cvar_InfoValidate
============
*/
static qBool Cvar_InfoValidate (char *s) {
	if (strstr (s, "\\"))	return qFalse;
	if (strstr (s, "\""))	return qFalse;
	if (strstr (s, ";"))	return qFalse;

	return qTrue;
}

/*
===============================================================================

	CVAR RETRIEVAL

===============================================================================
*/

/*
============
Cvar_FindVar
============
*/
static cvar_t *Cvar_FindVar (char *varName) {
	cvar_t	*var;
	
	for (var=cvar_Variables ; var ; var=var->next)
		if (!strcmp (varName, var->name))
			return var;

	return NULL;
}


/*
============
Cvar_Get

If the variable already exists, the value will not be set
The flags will be or'ed in if the variable exists.
============
*/
cvar_t *Cvar_Get (char *varName, char *varValue, int flags) {
	cvar_t	*var;

	if (flags & (CVAR_USERINFO|CVAR_SERVERINFO)) {
		if (!Cvar_InfoValidate (varName)) {
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "invalid info cvar name\n");
			return NULL;
		}
	}

	var = Cvar_FindVar (varName);
	if (var) {
		Z_Free (var->defString);
		var->defString = Cvar_CopyString (varValue);
		var->flags |= flags;

		return var;
	}

	if (!varValue)
		return NULL;

	if (flags & (CVAR_USERINFO|CVAR_SERVERINFO)) {
		if (!Cvar_InfoValidate (varValue)) {
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "invalid info cvar value\n");
			return NULL;
		}
	}

	var = Z_TagMalloc (sizeof (*var), ZTAG_CVARSYS);
	var->name = Cvar_CopyString (varName);
	var->string = Cvar_CopyString (varValue);
	var->defString = Cvar_CopyString (varValue);
	var->flags = flags;
	var->modified = qTrue;
	var->value = atof (var->string);
	var->integer = atoi (var->string);

	// link the variable in
	var->next = cvar_Variables;
	cvar_Variables = var;

	return var;
}


/*
============
Cvar_GetLatchedVars

Any variables with latched values will now be updated
============
*/
void Cvar_GetLatchedVars (int flags) {
	cvar_t	*var;

	for (var=cvar_Variables ; var ; var=var->next) {
		if (!(var->flags & flags))
			continue;

		if (!var->latchedString)
			continue;

		Z_Free (var->string);
		var->string = var->latchedString;
		var->latchedString = NULL;
		var->value = atof (var->string);
		var->integer = atoi (var->string);

		if (!strcmp (var->name, "game")) {
			FS_SetGamedir (var->string);
			FS_ExecAutoexec ();
		}
	}
}


/*
============
Cvar_VariableInteger
============
*/
int Cvar_VariableInteger (char *varName) {
	cvar_t	*var;
	
	var = Cvar_FindVar (varName);
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
	cvar_t *var;
	
	var = Cvar_FindVar (varName);
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
	cvar_t	*var;
	
	var = Cvar_FindVar (varName);
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
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qBool Cvar_Command (void) {
	cvar_t			*var;

	// check variables
	var = Cvar_FindVar (Cmd_Argv (0));
	if (!var)
		return qFalse;
		
	// perform a variable print or set
	if (Cmd_Argc () == 1) {
		Com_Printf (PRINT_ALL, "\"%s\" is: \"%s" S_STYLE_RETURN "\"", var->name, var->string);
		if (!(var->flags & CVAR_NOSET))
			Com_Printf (PRINT_ALL, " default: \"%s" S_STYLE_RETURN "\"", var->defString);
		Com_Printf (PRINT_ALL, "\n");
		return qTrue;
	}

	Cvar_Set (var->name, Cmd_Argv (1));
	return qTrue;
}


/*
============
Cvar_Set2
============
*/
cvar_t *Cvar_Set2 (char *varName, char *value, qBool force) {
	cvar_t	*var;

	var = Cvar_FindVar (varName);
	if (!var) {
		// create it
		return Cvar_Get (varName, value, 0);
	}

	if (var->flags & (CVAR_USERINFO | CVAR_SERVERINFO)) {
		if (!Cvar_InfoValidate (value)) {
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "invalid info cvar value\n");
			return var;
		}
	}

	if (!force) {
		if (var->flags & CVAR_NOSET) {
			Com_Printf (PRINT_ALL, "%s is write protected.\n", varName);
			return var;
		}

		if (var->flags & CVAR_LATCH) {
			if (var->latchedString) {
				if (strcmp (value, var->latchedString) == 0)
					return var;
				Z_Free (var->latchedString);
			} else {
				if (strcmp (value, var->string) == 0)
					return var;
			}

			if (Com_ServerState ()) {
				Com_Printf (PRINT_ALL, "%s will be changed for next game.\n", varName);
				var->latchedString = Cvar_CopyString (value);
			} else {
				var->string = Cvar_CopyString (value);
				var->value = atof (var->string);
				var->integer = atoi (var->string);
				if (!strcmp (var->name, "game")) {
					FS_SetGamedir (var->string);
					FS_ExecAutoexec ();
				}
			}
			return var;
		}
		
		if (var->flags & CVAR_LATCHVIDEO) {
			if (var->latchedString) {
				if (strcmp (value, var->latchedString) == 0)
					return var;
				Z_Free (var->latchedString);
			} else {
				if (strcmp (value, var->string) == 0)
					return var;
			}

			
			if (var->flags & CVAR_LATCH) {
				if (Com_ServerState ()) {
					Com_Printf (PRINT_ALL, "%s will be changed for next game.\n", varName);
					var->latchedString = Cvar_CopyString (value);
				} else {
					var->string = Cvar_CopyString (value);
					var->value = atof (var->string);
					var->integer = atoi (var->string);
					if (!strcmp (var->name, "game")) {
						FS_SetGamedir (var->string);
						FS_ExecAutoexec ();
					}
				}
			} else {
				Com_Printf (PRINT_ALL, "%s will be changed after a vid_restart.\n", varName);
				var->latchedString = Cvar_CopyString (value);
			}
			return var;
		}
	} else {
		if (var->latchedString) {
			Z_Free (var->latchedString);
			var->latchedString = NULL;
		}
	}

	if (!strcmp (value, var->string))
		return var;	// not changed

	var->modified = qTrue;

	if (var->flags & CVAR_USERINFO)
		cvar_UserInfoModified = qTrue;	// transmit at next oportunity
	
	Z_Free (var->string);	// free the old value string
	
	var->string = Cvar_CopyString (value);
	var->value = atof (var->string);
	var->integer = atoi (var->string);

	return var;
}


/*
============
Cvar_ForceSet
============
*/
cvar_t *Cvar_ForceSet (char *varName, char *value) {
	return Cvar_Set2 (varName, value, qTrue);
}


/*
============
Cvar_ForceSetValue
============
*/
cvar_t *Cvar_ForceSetValue (char *varName, float value) {
	char	val[64];

	if (value == (int)value)
		Com_sprintf (val, sizeof (val), "%i", (int)value);
	else
		Com_sprintf (val, sizeof (val), "%f", value);

	return Cvar_Set2 (varName, val, qTrue);
}


/*
============
Cvar_FullSet
============
*/
cvar_t *Cvar_FullSet (char *varName, char *value, int flags) {
	cvar_t	*var;
	
	var = Cvar_FindVar (varName);
	if (!var) {
		// create it
		return Cvar_Get (varName, value, flags);
	}

	var->modified = qTrue;

	if (var->flags & CVAR_USERINFO)
		cvar_UserInfoModified = qTrue;	// transmit at next oportunity
	
	Z_Free (var->string);	// free the old value string
	
	var->string = Cvar_CopyString(value);
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
cvar_t *Cvar_Set (char *varName, char *value) {
	return Cvar_Set2 (varName, value, qFalse);
}


/*
============
Cvar_SetValue
============
*/
void Cvar_SetValue (char *varName, float value) {
	char	val[64];

	if (value == (int)value)
		Com_sprintf (val, sizeof (val), "%i", (int)value);
	else
		Com_sprintf (val, sizeof (val), "%f", value);

	Cvar_Set (varName, val);
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
		Com_Printf (PRINT_ALL, "usage: set <variable> <value> [u / s]\n");
		return;
	}

	if (c == 4) {
		if (!strcmp (Cmd_Argv (3), "u"))
			flags = CVAR_USERINFO;
		else if (!strcmp (Cmd_Argv (3), "s"))
			flags = CVAR_SERVERINFO;
		else {
			Com_Printf (PRINT_ALL, "flags can only be 'u' or 's'\n");
			return;
		}

		Cvar_FullSet (Cmd_Argv (1), Cmd_Argv (2), flags);
	} else
		Cvar_Set (Cmd_Argv (1), Cmd_Argv (2));
}


/*
================
Cvar_CMDToggle_f
================
*/
static void Cvar_CMDToggle_f (void)
{	
	char	*name, *string;
	char	*opt1 = "1", *opt2 = "0";

	if (Cmd_Argc () < 2) {
		Com_Printf (PRINT_ALL, "usage: toggle <cvar> [option1] [option2]\n");
		return;
	}

	name = Cmd_Argv (1);
	if (Cmd_Argc () > 2)
		opt1 = Cmd_Argv (2);

	if (Cmd_Argc () > 3)
		opt2 = Cmd_Argv (3);
	
	string = Cvar_VariableString (name);

	if (strcmp (string, opt1))
		Cvar_Set (name, opt1);
	else
		Cvar_Set (name, opt2);  
}


/*
================
Cvar_CMDInc_f
================
*/
static void Cvar_CMDInc_f (void) {	
	char	*cvarName;
	float	inc;
	cvar_t	*cvar;
	
	if (Cmd_Argc () < 2) {
		Com_Printf (PRINT_ALL, "usage: inc <cvar> [amount]\n");
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
Cvar_CMDDec_f
================
*/
static void Cvar_CMDDec_f (void) {	
	char	*cvarName;
	float	dec;
	cvar_t	*cvar;
	
	if (Cmd_Argc () < 2) {
		Com_Printf (PRINT_ALL, "usage: dec <cvar> [amount]\n");
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
static void Cvar_List_f (void) {
	cvar_t	*var;
	int		i, j, c;
	char	*wc;

	c = Cmd_Argc ();

	if ((c != 1) && (c != 2)) {
		Com_Printf (PRINT_ALL, "usage: cvarlist [wildcard]\n");
		return;
	}

	if (c == 2)
		wc = va ("*%s*", Cmd_Argv (1));
	else
		wc = "*";

	for (i=0, j=0, var=cvar_Variables ; var ; var=var->next, i++) {
		if (Com_WildCmp (wc, var->name, 1)) {
			j++;

			Com_Printf (PRINT_ALL, "%s", (var->flags & CVAR_ARCHIVE) ? "*" : " ");
			Com_Printf (PRINT_ALL, "%s", (var->flags & CVAR_USERINFO) ? "U" : " ");
			Com_Printf (PRINT_ALL, "%s", (var->flags & CVAR_SERVERINFO) ? "S" : " ");
			Com_Printf (PRINT_ALL, "%s", (var->flags & CVAR_NOSET) ? "-" : " ");
			Com_Printf (PRINT_ALL, "%s", (var->flags & CVAR_LATCH) ? "L" : " ");
			Com_Printf (PRINT_ALL, "%s", (var->flags & CVAR_LATCHVIDEO) ? "V" : " ");

			Com_Printf (PRINT_ALL, " %s is: \"%s" S_STYLE_RETURN "\"", var->name, var->string);
			if (!(var->flags & CVAR_NOSET))
				Com_Printf (PRINT_ALL, " default: \"%s" S_STYLE_RETURN "\"", var->defString);
			Com_Printf (PRINT_ALL, "\n");
		}
	}

	Com_Printf (PRINT_ALL, "%i cvars total, %i matching\n", i, j);
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
void Cvar_Init (void)
{
	Cmd_AddCommand ("set",		Cvar_Set_f);
	Cmd_AddCommand ("cvarlist",	Cvar_List_f);
	
	Cmd_AddCommand ("toggle",	Cvar_CMDToggle_f);
	Cmd_AddCommand ("inc",		Cvar_CMDInc_f);
	Cmd_AddCommand ("dec",		Cvar_CMDDec_f);
}


/*
============
Cvar_Shutdown
============
*/
void Cvar_Shutdown (void)
{
	Cmd_RemoveCommand ("set");
	Cmd_RemoveCommand ("cvarlist");
	
	Cmd_RemoveCommand ("toggle");
	Cmd_RemoveCommand ("inc");
	Cmd_RemoveCommand ("dec");
}
