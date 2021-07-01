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
// gui_cursor.c
//

#include "gui_local.h"

static gui_t	*cl_inputGUI;

/*
=============================================================================

	BOUNDS CALCULATION

=============================================================================
*/

/*
==================
GUI_GenerateBounds
==================
*/
void GUI_GenerateBounds (gui_t *window)
{
	gui_t	*child;
	uint32	i;

	if (!window->d.visible || window->d.noEvents)
		return;

	// Generate bounds
	window->mins[0] = window->d.rect[0];
	window->mins[1] = window->d.rect[1];
	window->maxs[0] = window->mins[0] + window->d.rect[2];
	window->maxs[1] = window->mins[1] + window->d.rect[3];

	// Generate bounds for children
	for (i=0, child=window->childList ; i<window->numChildren ; child++, i++) {
		GUI_GenerateBounds (child);

		// Add to the parent
		AddBoundsTo2DBounds (child->mins, child->maxs, window->mins, window->maxs);
	}
}

/*
=============================================================================

	CURSOR COLLISION

=============================================================================
*/

/*
==================
GUI_CursorUpdate
==================
*/
void GUI_r_CursorUpdate (gui_t *window)
{
	gui_t	*child;
	uint32	i;

	// Check for collision
	if (!window->d.visible
	|| window->d.noEvents
	|| window->cursor->d.pos[0] < window->mins[0]
	|| window->cursor->d.pos[1] < window->mins[1]
	|| window->cursor->d.pos[0] > window->maxs[0]
	|| window->cursor->d.pos[1] > window->maxs[1])
		return;

	// Collided with this window
	if (window->d.modal && !cl_inputGUI->d.modal)
		cl_inputGUI = window;

	// Recurse down the children
	for (i=0, child=window->childList ; i<window->numChildren ; child++, i++)
		GUI_r_CursorUpdate (child);
}
void GUI_CursorUpdate (gui_t *window)
{
	gui_t	*child;
	uint32	i;

	cl_inputGUI = NULL;

	// Check for collision
	if (!window->d.visible
	|| window->d.noEvents
	|| window->cursor->d.pos[0] < window->mins[0]
	|| window->cursor->d.pos[1] < window->mins[1]
	|| window->cursor->d.pos[0] > window->maxs[0]
	|| window->cursor->d.pos[1] > window->maxs[1])
		return;

	// Collided with this window
	cl_inputGUI = window;

	// Recurse down the children
	for (i=0, child=window->childList ; i<window->numChildren ; child++, i++)
		GUI_r_CursorUpdate (child);

	window->cursor->activeWindow = cl_inputGUI;
}

/*
=============================================================================

	CURSOR MOVEMENT

=============================================================================
*/

/*
==================
GUI_MoveMouse
==================
*/
void GUI_MoveMouse (int xMove, int yMove)
{
	gui_t	*gui;

	gui = cl_inputGUI;
	if (!gui || gui->cursor->d.locked)
		return;

	// Move
	if (gui_mouseFilter->integer) {
		gui->cursor->d.pos[0] = (gui->cursor->d.pos[0] * 2 + (xMove * gui_mouseSensitivity->value)) * 0.5f;
		gui->cursor->d.pos[1] = (gui->cursor->d.pos[1] * 2 + (yMove * gui_mouseSensitivity->value)) * 0.5f;
	}
	else {
		gui->cursor->d.pos[0] += xMove * gui_mouseSensitivity->value;
		gui->cursor->d.pos[1] += yMove * gui_mouseSensitivity->value;
	}

	// Clamp
	gui->cursor->d.pos[0] = clamp (gui->cursor->d.pos[0], 2, cls.glConfig.vidWidth - 2);
	gui->cursor->d.pos[1] = clamp (gui->cursor->d.pos[1], 2, cls.glConfig.vidHeight - 2);

	// Update collision
	if (xMove || yMove)
		GUI_CursorUpdate (gui);
}
