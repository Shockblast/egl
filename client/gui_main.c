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
// gui_main.c
//

#include "gui_local.h"

static gui_t	*cl_guiLayers[MAX_GUI_DEPTH];
static uint32	cl_guiDepth;

/*
=============================================================================

	GUI STATE

=============================================================================
*/

/*
==================
GUI_ResetGUIState
==================
*/
static void GUI_r_ResetGUIState (gui_t *window)
{
	gui_t	*child;
	uint32	i;

	memcpy (&window->d, &window->s, sizeof (guiData_t));

	// Recurse down the children
	for (i=0, child=window->childList ; i<window->numChildren ; child++, i++)
		GUI_r_ResetGUIState (child);
}
void GUI_ResetGUIState (gui_t *gui)
{
	// Cursor
	if (gui->type == WTP_GUI)
		memcpy (&gui->cursor->d, &gui->cursor->s, sizeof (guiCursorData_t));

	// Recurse
	GUI_r_ResetGUIState (gui);
}

/*
=============================================================================

	OPENED GUIS

=============================================================================
*/

/*
==================
GUI_OpenGUI
==================
*/
void GUI_OpenGUI (gui_t *gui)
{
	uint32	i;

	if (!gui)
		return;

	// Check if it's open
	for (i=0 ; i<cl_guiDepth ; i++) {
		if (gui == cl_guiLayers[i])
			break;
	}
	if (i != cl_guiDepth) {
		Com_Error (ERR_DROP, "GUI_OpenGUI: GUI '%s' already opened!", gui->name);
		return;
	}

	// Check for room
	if (cl_guiDepth+1 >= MAX_GUI_DEPTH) {
		Com_Error (ERR_DROP, "GUI_OpenGUI: Too many GUIs open to open '%s'!", gui->name);
		return;
	}

	// Add it to the list
	cl_guiLayers[cl_guiDepth++] = gui;
}


/*
==================
GUI_CloseGUI
==================
*/
void GUI_CloseGUI (gui_t *gui)
{
	qBool	found;
	uint32	i;

	if (!gui)
		return;

	// Reset the state
	GUI_ResetGUIState (gui);

	// Remove it from the list
	found = qFalse;
	for (i=0 ; i<cl_guiDepth ; i++) {
		if (found)
			cl_guiLayers[i-1] = cl_guiLayers[i];
		else if (gui == cl_guiLayers[i])
			found = qTrue;
	}

	// Check if it's open
	if (!found) {
		Com_Error (ERR_DROP, "GUI_CloseGUI: GUI '%s' not opened!", gui->name);
		return;
	}

	// Trim off the extra at the end
	cl_guiLayers[--cl_guiDepth] = NULL;
}


/*
==================
GUI_CloseAllGUIs
==================
*/
void GUI_CloseAllGUIs (void)
{
	gui_t	*gui;
	uint32	i;

	for (i=0, gui=cl_guiLayers[0] ; i<cl_guiDepth ; gui++, i++)
		GUI_CloseGUI (gui);
}

/*
=============================================================================

	GUI RENDERING

=============================================================================
*/

/*
==================
GUI_ScaleGUI
==================
*/
static void GUI_r_ScaleGUI (gui_t *window, vec4_t rect, float xScale, float yScale)
{
	gui_t	*child;
	uint32	i;

	// Scale this window
	window->d.rect[0] = (window->s.rect[0] + rect[0]) * xScale;
	window->d.rect[1] = (window->s.rect[1] + rect[1]) * yScale;
	window->d.rect[2] = window->s.rect[2] * xScale;
	window->d.rect[3] = window->s.rect[3] * yScale;

	// Recurse down the children
	for (i=0, child=window->childList ; i<window->numChildren ; child++, i++) {
		if (!window->d.visible || window->d.noEvents)
			continue;

		GUI_r_ScaleGUI (child, window->d.rect, xScale, yScale);
	}
}
static void GUI_ScaleGUI (gui_t *gui)
{
	float	xScale, yScale;
	vec4_t	rect;

	if (!gui->d.visible || gui->d.noEvents)
		return;

	// Calculate aspect
	// FIXME: ...
	xScale = 1;
	yScale = 1;
	rect[0] = 0;
	rect[1] = 0;
	rect[2] = 0;
	rect[3] = 0;

	// Scale the children
	GUI_r_ScaleGUI (gui, rect, xScale, yScale);
}


/*
==================
GUI_DrawWindows
==================
*/
static void GUI_DrawWindows (gui_t *gui)
{
	gui_t	*child;
	uint32	i;

	if (!gui->d.visible)
		return;

	// Fill
	if (gui->flags & WFL_FILL_COLOR)
		R_DrawFill (gui->d.rect[0], gui->d.rect[1], gui->d.rect[2], gui->d.rect[3], gui->d.fillColor);

	// Material
	if (gui->flags & WFL_MATERIAL && gui->d.matShader)
		R_DrawPic (gui->d.matShader, 0,
			gui->d.rect[0], gui->d.rect[1], gui->d.rect[2], gui->d.rect[3],
			0, 0, gui->d.matScaleX, gui->d.matScaleY,
			gui->d.matColor);

	// Bounds debug
	if (gui_debugBounds->integer == 1 && gui->cursor->activeWindow == gui
	|| gui_debugBounds->integer == 2)
		R_DrawFill (gui->d.rect[0], gui->d.rect[1], gui->d.rect[2], gui->d.rect[3], colorYellow);

	// Recurse down the children
	for (i=0, child=gui->childList ; i<gui->numChildren ; child++, i++)
		GUI_DrawWindows (child);
}


/*
==================
GUI_DrawCursor
==================
*/
static void GUI_DrawCursor (gui_t *gui)
{
	if (!gui->cursor->d.mat || gui->cursor->d.disabled)
		return;

	R_DrawPic (gui->cursor->d.mat, 0,
		gui->cursor->d.pos[0], gui->cursor->d.pos[1], gui->cursor->d.size[0], gui->cursor->d.size[1],
		0, 0, 1, 1,
		gui->cursor->d.color);
}


/*
==================
GUI_Refresh
==================
*/
void GUI_Refresh (void)
{
	gui_t	*gui;
	uint32	i;

	for (i=0, gui=cl_guiLayers[0] ; i<cl_guiDepth ; gui++, i++) {
		// Fire event triggers queued last frame
		GUI_FireTriggers (gui);

		// Calculate scale
		GUI_ScaleGUI (gui);

		// Generate bounds for collision
		GUI_GenerateBounds (gui);

		// Calculate bounds and check collision
		GUI_CursorUpdate (gui);

		// Render all windows
		GUI_DrawWindows (gui);

		// Render the cursor
		GUI_DrawCursor (gui);
	}
}
