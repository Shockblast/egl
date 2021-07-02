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
// cvar.h
//

/*
=============================================================================

	CVARS

	Console Variables
=============================================================================
*/

#define MAX_CVARS 1024

extern cVar_t	com_cvarList[MAX_CVARS];
extern int		com_numCvars;
extern qBool	com_userInfoModified;

void		Cvar_WriteVariables (FILE *f);

char		*Cvar_BitInfo (int bit);

cVar_t		*Cvar_Exists (char *varName);
void		Cvar_CallBack (void (*callBack) (const char *name));
cVar_t		*Cvar_Register (char *varName, char *value, int flags);

void		Cvar_FixCheatVars (void);
void		Cvar_GetLatchedVars (int flags);

int			Cvar_GetIntegerValue (char *varName);
char		*Cvar_GetStringValue (char *varName);
float		Cvar_GetFloatValue (char *varName);

cVar_t		*Cvar_VariableSet (cVar_t *var, char *value, qBool force);
cVar_t		*Cvar_VariableSetValue (cVar_t *var, float value, qBool force);
cVar_t		*Cvar_VariableReset (cVar_t *var, qBool force);

cVar_t		*Cvar_Set (char *varName, char *value, qBool force);
cVar_t		*Cvar_SetValue (char *varName, float value, qBool force);
cVar_t		*Cvar_Reset (char *varName, qBool force);

qBool		Cvar_Command (void);

void		Cvar_Init (void);
