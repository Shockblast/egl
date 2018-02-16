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
// gui_vars.c
// FIXME TODO:
// - Hash optimize!
//

#include "gui_local.h"

static guiVar_t		gui_varList[MAX_GUIVARS];
static uint32		gui_numVars;

/*
=============================================================================

	HELPER FUNCTIONS

=============================================================================
*/

/*
==================
GUIVar_Find
==================
*/
static guiVar_t *GUIVar_Find (char *name, guiVarType_t type)
{
	guiVar_t	*var;
	uint32		i;

	for (i=0, var=&gui_varList[0] ; i<gui_numVars ; var++, i++) {
		if (strcmp (var->name, name))
			continue;
		if (var->type != type)
			continue;

		return var;
	}

	return NULL;
}

/*
=============================================================================

	VARIABLE REGISTRATION

=============================================================================
*/

/*
==================
GUIVar_Register
==================
*/
guiVar_t *GUIVar_Register (char *name, guiVarType_t type)
{
	guiVar_t	*var;
	char		fixedName[MAX_GV_NAMELEN];

	// Check name
	if (!name || !name[0]) {
		Com_Printf (PRNT_ERROR, "GUIVar_Register: NULL name!\n");
		assert (0);
		return NULL;
	}
	if (strlen(name)+1 >= MAX_GV_NAMELEN) {
		Com_Printf (PRNT_ERROR, "GUIVar_Register: name length too long!\n");
		return NULL;
	}

	// Check type
	switch (type) {
	case GVT_FLOAT:
	case GVT_STR:
	case GVT_STR_PTR:
	case GVT_VEC:
		break;

	default:
		assert (0);
		Com_Error (ERR_DROP, "GUIVar_Register: invalid type '%i'", type);
		break;
	}

	// See if it's already loaded
	Q_strncpyz (fixedName, name, sizeof (fixedName));
	var = GUIVar_Find (fixedName, type);
	if (var)
		return var;

	// Create it
	if (gui_numVars+1 >= MAX_GUIVARS)
		Com_Error (ERR_DROP, "GUIVar_Register: MAX_GUIVARS");

	var = &gui_varList[gui_numVars++];
	var->name = GUI_StrDup (fixedName, GUITAG_VARS);
	var->type = type;
	var->modified = qTrue;
	return var;
}

/*
=============================================================================

	VALUE GETTING

=============================================================================
*/

/*
==================
GUIVar_GetFloatValue
==================
*/
qBool GUIVar_GetFloatValue (guiVar_t *var, float *dest)
{
	// Stupidity check
	if (!var) {
		assert (0);
		Com_Printf (PRNT_ERROR, "GUIVar_GetFloatValue: NULL var!\n");
		return qFalse;
	}
	if (!dest) {
		assert (0);
		Com_Printf (PRNT_ERROR, "GUIVar_GetFloatValue: NULL dest!\n");
		return qFalse;
	}
	if (var->type != GVT_FLOAT) {
		Com_Printf (PRNT_ERROR, "GUIVar_GetFloatValue: variable '%s' is not type float!\n", var->name);
		return qFalse;
	}

	// Get the value
	*dest = var->floatVal;
	return qTrue;
}


/*
==================
GUIVar_GetStrValue
==================
*/
qBool GUIVar_GetStrValue (guiVar_t *var, char *dest, size_t size)
{
	// Stupidity check
	if (!var) {
		assert (0);
		Com_Printf (PRNT_ERROR, "GUIVar_GetStrValue: NULL var!\n");
		return qFalse;
	}
	if (!dest) {
		assert (0);
		Com_Printf (PRNT_ERROR, "GUIVar_GetStrValue: NULL target!\n");
		return qFalse;
	}
	if (!size) {
		assert (0);
		Com_Printf (PRNT_ERROR, "GUIVar_GetStrValue: invalid size!\n");
		return qFalse;
	}
	if (var->type != GVT_STR && var->type != GVT_STR_PTR) {
		Com_Printf (PRNT_ERROR, "GUIVar_GetStrValue: variable '%s' is not type string!\n", var->name);
		return qFalse;
	}

	// Get the value
	if (!var->strVal) {
		dest[0] = '\0';
	}
	else {
		if (strlen(var->strVal)+1 > size)
			Com_Printf (PRNT_WARNING, "GUIVar_GetStrValue: dest size is shorter than value length, dest will be truncated!\n");

		Q_strncpyz (dest, var->strVal, size);
	}
	return qTrue;
}


/*
==================
GUIVar_GetVecValue
==================
*/
qBool GUIVar_GetVecValue (guiVar_t *var, vec4_t dest)
{
	// Stupidity check
	if (!var) {
		assert (0);
		Com_Printf (PRNT_ERROR, "GUIVar_GetVecValue: NULL var!\n");
		return qFalse;
	}
	if (!dest) {
		assert (0);
		Com_Printf (PRNT_ERROR, "GUIVar_GetVecValue: NULL target!\n");
		return qFalse;
	}
	if (var->type != GVT_VEC) {
		Com_Printf (PRNT_ERROR, "GUIVar_GetVecValue: variable '%s' is not type vec!\n", var->name);
		return qFalse;
	}

	// Get the value
	dest[0] = var->vecVal[0];
	dest[1] = var->vecVal[1];
	dest[2] = var->vecVal[2];
	dest[3] = var->vecVal[3];
	return qTrue;
}

/*
=============================================================================

	VALUE SETTING

=============================================================================
*/

/*
==================
GUIVar_SetFloatValue
==================
*/
void GUIVar_SetFloatValue (guiVar_t *var, float value)
{
	// Stupidity check
	if (!var) {
		assert (0);
		Com_Printf (PRNT_ERROR, "GUIVar_SetFloatValue: NULL var!\n");
		return;
	}
	if (var->type != GVT_FLOAT) {
		Com_Printf (PRNT_ERROR, "GUIVar_SetFloatValue: variable '%s' is not type float!\n", var->name);
		return;
	}

	// Set the value
	var->floatVal = value;
	var->modified = qTrue;
}


/*
==================
GUIVar_SetStrValue
==================
*/
void GUIVar_SetStrValue (guiVar_t *var, char *value)
{
	// Stupidity check
	if (!var) {
		assert (0);
		Com_Printf (PRNT_ERROR, "GUIVar_SetStrValue: NULL var!\n");
		return;
	}
	if (var->type != GVT_STR && var->type != GVT_STR_PTR) {
		Com_Printf (PRNT_ERROR, "GUIVar_SetStrValue: variable '%s' is not type string!\n", var->name);
		return;
	}
	if (strlen(value)+1 >= MAX_GV_STRLEN) {
		Com_Printf (PRNT_ERROR, "GUIVar_SetStrValue: value exceeds maximum value!\n", var->name);
		return;
	}

	// Set the value
	if (var->type == GVT_STR_PTR) {
		var->strVal = value;
	}
	else {
		if (var->strVal)
			GUI_MemFree (var->strVal);
		var->strVal = GUI_StrDup (value, GUITAG_VARS);
	}
	var->modified = qTrue;
}


/*
==================
GUIVar_SetVecValue
==================
*/
void GUIVar_SetVecValue (guiVar_t *var, vec4_t value)
{
	// Stupidity check
	if (!var) {
		assert (0);
		Com_Printf (PRNT_ERROR, "GUIVar_SetVecValue: NULL var!\n");
		return;
	}
	if (var->type != GVT_VEC) {
		Com_Printf (PRNT_ERROR, "GUIVar_SetVecValue: variable '%s' is not type vec!\n", var->name);
		return;
	}

	// Set the value
	Vec4Copy (value, var->vecVal);
	var->modified = qTrue;
}

/*
=============================================================================

	INIT / SHUTDOWN

=============================================================================
*/

static void	*cmd_gui_varList;

/*
==================
GUIVar_Init
==================
*/
static void GUI_VarList_f (void)
{
	guiVar_t	*var;
	uint32		i;

	Com_Printf (0, "type  name\n");
	Com_Printf (0, "----- --------------------------------\n");
	for (i=0, var=&gui_varList[0] ; i<gui_numVars ; var++, i++) {
		switch (var->type) {
		case GVT_FLOAT:	Com_Printf (0, "float ");	break;
		case GVT_STR:	Com_Printf (0, "str   ");	break;
		case GVT_VEC:	Com_Printf (0, "vec   ");	break;
		}

		Com_Printf (0, "%s\n", var->name);
	}
	Com_Printf (0, "----- --------------------------------\n");
	Com_Printf (0, "%i vars total.\n", i);
}


/*
==================
GUIVar_Init
==================
*/
void GUIVar_Init (void)
{
	// Register commands
	cmd_gui_varList = Cmd_AddCommand ("gui_varList",	GUI_VarList_f,		"lists GUI variables");
}


/*
==================
GUIVar_Shutdown
==================
*/
void GUIVar_Shutdown (void)
{
	guiVar_t	*var;
	uint32		i;

	// Remove commands
	Cmd_RemoveCommand ("gui_varlist", cmd_gui_varList);

	// Free memory
	for (i=0, var=&gui_varList[0] ; i<gui_numVars ; var++, i++) {
		GUI_MemFree (var->name);
		switch (var->type) {
		case GVT_FLOAT:
			break;
		case GVT_STR:
			if (var->strVal)
				GUI_MemFree (var->strVal);
			break;
		case GVT_STR_PTR:
			break;
		case GVT_VEC:
			break;
		default:
			assert (0);
			break;
		}
	}

	// Anything missed will be caught with this.
	// (though nothing should be missed)
	GUI_FreeTag (GUITAG_VARS);

	// Clear slots
	gui_numVars = 0;
	memset (&gui_varList[0], 0, sizeof (guiVar_t) * MAX_GUIVARS);
}
