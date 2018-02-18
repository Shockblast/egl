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
// gui_public.h
// Public GUI header, shown to the rest of the client.
//

/*
=============================================================================

	FUNCTION PROTOTYPES

=============================================================================
*/

typedef struct gui_s gui_t;
typedef struct guiVar_s guiVar_t;

//
// gui_cursor.c
//
void		GUI_MoveMouse (int xMove, int yMove);

//
// gui_events.c
//
void		GUI_NamedGlobalEvent (char *name);
void		GUI_NamedGUIEvent (gui_t *gui, char *name);

//
// gui_init.c
//
gui_t *GUI_RegisterGUI (char *name);

void		GUI_BeginRegistration (void);
void		GUI_RegisterSounds (void);
void		GUI_EndRegistration (void);

void		GUI_Init (void);

//
// gui_keys.c
//
void		GUI_KeyUp (keyNum_t keyNum);
void		GUI_KeyDown (keyNum_t keyNum);

//
// gui_main.c
//
void		GUI_OpenGUI (gui_t *gui);
void		GUI_CloseGUI (gui_t *gui);
void		GUI_CloseAllGUIs (void);

void		GUI_Refresh (void);

//
// gui_vars.c
//

guiVar_t	*GUIVar_Register (char *name, guiVarType_t type);
qBool		GUIVar_GetFloatValue (guiVar_t *var, float *dest);
qBool		GUIVar_GetStrValue (guiVar_t *var, char *dest, size_t size);
qBool		GUIVar_GetVecValue (guiVar_t *var, vec4_t dest);
void		GUIVar_SetFloatValue (guiVar_t *var, float value);
void		GUIVar_SetStrValue (guiVar_t *var, char *value);
void		GUIVar_SetVecValue (guiVar_t *var, vec4_t value);
