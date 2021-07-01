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
// ui_cursor.c
//

#include "ui_local.h"

static cVar_t		*ui_cursorX;
static cVar_t		*ui_cursorY;

/*
=======================================================================

	MENU MOUSE CURSOR

=======================================================================
*/

/*
=============
UI_DrawMouseCursor
=============
*/
void UI_DrawMouseCursor (void)
{
	struct shader_s *cursor;

	if (uiState.cursorOverItem)
		cursor = uiMedia.cursorHoverShader;
	else
		cursor = uiMedia.cursorShader;

	cgi.R_DrawPic (cursor, 0, uiState.cursorX + 1, uiState.cursorY + 1,
		uiState.cursorW * clamp (UIFT_SCALE, 0.5f, 1),
		uiState.cursorH * clamp (UIFT_SCALE, 0.5f, 1),
		0, 0, 1, 1, colorWhite);
}


/*
=============
UI_FindMouseItem

Finds the item that the mouse is over for the specified framework
=============
*/
static void UI_FindMouseItem (uiFrameWork_t *fw)
{
	int		i;

	// Search for the current item that the mouse is over
	uiState.mouseItem = NULL;
	for (i=0 ; i<fw->numItems ; i++) {
		// Skip NOSELECT items
		if (((uiCommon_t *) fw->items[i])->flags & UIF_NOSELECT)
			continue;

		// Check if the mouse is colliding
		if (uiState.cursorX > ((uiCommon_t *) fw->items[i])->botRight[0]
		|| uiState.cursorY > ((uiCommon_t *) fw->items[i])->botRight[1]
		|| uiState.cursorX < ((uiCommon_t *) fw->items[i])->topLeft[0]
		|| uiState.cursorY < ((uiCommon_t *) fw->items[i])->topLeft[1])
			continue;

		uiState.cursorItem = fw->items[i];
		uiState.mouseItem = fw->items[i];

		if (fw->cursor == i)
			break;
		uiState.newCursorItem = qTrue;
		fw->cursor = i;
		break;
	}

	// Rollover cursor image
	uiState.cursorOverItem = (uiState.mouseItem != NULL);
}


/*
=============
UI_UpdateMousePos
=============
*/
void UI_UpdateMousePos (void)
{
	if (!uiState.activeUI || !uiState.activeUI->numItems || uiState.cursorLock)
		return;

	uiState.newCursorItem = qFalse;
	UI_FindMouseItem (uiState.activeUI);
}


/*
=============
UI_MoveMouse
=============
*/
void UI_MoveMouse (float x, float y)
{
	if (uiState.cursorLock)
		return;

	// Filter
	if (ui_filtermouse->integer) {
		uiState.cursorX = (uiState.cursorX * 2 + (x * ui_sensitivity->value)) * 0.5f;
		uiState.cursorY = (uiState.cursorY * 2 + (y * ui_sensitivity->value)) * 0.5f;
	}
	else {
		uiState.cursorX += x * ui_sensitivity->value;
		uiState.cursorY += y * ui_sensitivity->value;
	}

	// Clamp
	uiState.cursorX = clamp (uiState.cursorX, 2, cg.glConfig.vidWidth - 2);
	uiState.cursorY = clamp (uiState.cursorY, 2, cg.glConfig.vidHeight - 2);

	if (x || y)
		UI_UpdateMousePos ();
}


/*
=============
UI_SetupBounds
=============
*/
static void Action_Setup (uiAction_t *s)
{
	if (!s->generic.name)
		return;

	s->generic.topLeft[0] = s->generic.x + s->generic.parent->x;
	s->generic.topLeft[1] = s->generic.y + s->generic.parent->y;

	s->generic.botRight[1] = s->generic.topLeft[1] + UISIZE_TYPE (s->generic.flags);

	if (s->generic.flags & UIF_CENTERED)
		s->generic.topLeft[0] -= ((Q_ColorCharCount (s->generic.name, (int)strlen (s->generic.name)) * UISIZE_TYPE (s->generic.flags))*0.5);
	else if (!(s->generic.flags & UIF_LEFT_JUSTIFY))
		s->generic.topLeft[0] += LCOLUMN_OFFSET - ((Q_ColorCharCount (s->generic.name, (int)strlen (s->generic.name))) * UISIZE_TYPE (s->generic.flags));

	s->generic.botRight[0] = s->generic.topLeft[0] + (Q_ColorCharCount (s->generic.name, (int)strlen (s->generic.name)) * UISIZE_TYPE (s->generic.flags));
}

static void Field_Setup (uiField_t *s)
{
	s->generic.topLeft[0] = s->generic.x + s->generic.parent->x;

	if (s->generic.name)
		s->generic.topLeft[0] -= (Q_ColorCharCount (s->generic.name, (int)strlen (s->generic.name)) + 1) * UISIZE_TYPE (s->generic.flags);

	s->generic.topLeft[1] = s->generic.y + s->generic.parent->y - (UISIZE_TYPE (s->generic.flags) * 0.5);

	s->generic.botRight[0] = s->generic.x + s->generic.parent->x + ((s->visibleLength + 2) * UISIZE_TYPE (s->generic.flags));
	s->generic.botRight[1] = s->generic.topLeft[1] + UISIZE_TYPE (s->generic.flags)*2;

	if (s->generic.flags & UIF_CENTERED) {
		s->generic.topLeft[0] -= ((s->visibleLength + 2) * UISIZE_TYPE (s->generic.flags))*0.5;
		s->generic.botRight[0] -= ((s->visibleLength + 2) * UISIZE_TYPE (s->generic.flags))*0.5;
	}
}

static void MenuImage_Setup (uiImage_t *s)
{
	int		width, height;

	if (s->width || s->height) {
		width = s->width;
		height = s->height;
	}
	else {
		cgi.R_GetImageSize (s->shader, &width, &height);
		s->width = width;
		s->height = height;
	}

	width *= UISCALE_TYPE (s->generic.flags);
	height *= UISCALE_TYPE (s->generic.flags);

	s->generic.topLeft[0] = s->generic.x + s->generic.parent->x;

	if (s->generic.flags & UIF_CENTERED)
		s->generic.topLeft[0] -= width * 0.5;
	else if (s->generic.flags & UIF_LEFT_JUSTIFY)
		s->generic.topLeft[0] += LCOLUMN_OFFSET;

	s->generic.topLeft[1] = s->generic.y + s->generic.parent->y;
	s->generic.botRight[0] = s->generic.topLeft[0] + width;
	s->generic.botRight[1] = s->generic.topLeft[1] + height;
}

// FIXME need calculations to detect when on the left/right side of the slider
// so left == less and right == more
// if the mouse cursor position is less than the cursor position and is within the bounding box, left
// if the mouse cursor position is greater than the cursor position and is within the bounding box, right
static void Slider_Setup (uiSlider_t *s)
{
	float		xsize, ysize;
	float		offset;

	if (s->generic.name)
		offset = Q_ColorCharCount (s->generic.name, (int)strlen (s->generic.name)) * UISIZE_TYPE (s->generic.flags);
	else
		offset = 0;

	s->generic.topLeft[0] = s->generic.x + s->generic.parent->x - offset - UISIZE_TYPE (s->generic.flags);
	s->generic.topLeft[1] = s->generic.y + s->generic.parent->y;

	xsize = (UISIZE_TYPE (s->generic.flags) * 6) + offset + ((SLIDER_RANGE-1) * UISIZE_TYPE (s->generic.flags));
	ysize = UISIZE_TYPE (s->generic.flags);

	s->generic.botRight[0] = s->generic.topLeft[0] + xsize;
	s->generic.botRight[1] = s->generic.topLeft[1] + ysize;
}

static void SpinControl_Setup (uiList_t *s)
{
	float		xsize = 0, ysize = 0;

	ysize = UISIZE_TYPE (s->generic.flags);
	if (s->generic.name) {
		s->generic.topLeft[0] = s->generic.x + s->generic.parent->x - (Q_ColorCharCount (s->generic.name, (int)strlen (s->generic.name)) * UISIZE_TYPE (s->generic.flags)) - UISIZE_TYPE (s->generic.flags);
		s->generic.topLeft[1] = s->generic.y + s->generic.parent->y;

		xsize = (Q_ColorCharCount (s->generic.name, (int)strlen (s->generic.name)) * UISIZE_TYPE (s->generic.flags)) + UISIZE_TYPE (s->generic.flags)*3;
	}

	if (s->itemNames && s->itemNames[s->curValue]) {
		if (s->itemNames[s->curValue] && (!strchr(s->itemNames[s->curValue], '\n'))) {
			if (!s->generic.name)
				s->generic.topLeft[0] += UISIZE_TYPE (s->generic.flags)*20;
		}
		else
			ysize += UISIZE_TYPE (s->generic.flags);

		if (s->itemNames[s->curValue])
			xsize += (Q_ColorCharCount (s->itemNames[s->curValue], (int)strlen (s->itemNames[s->curValue])) * UISIZE_TYPE (s->generic.flags));
	}

	s->generic.botRight[0] = s->generic.topLeft[0] + xsize;
	s->generic.botRight[1] = s->generic.topLeft[1] + ysize;
}

void UI_SetupBounds (uiFrameWork_t *menu)
{
	uiCommon_t	*item;
	int			i;

	// Generate collision boxes for items
	for (i=0 ; i<menu->numItems ; i++) {
		item = (uiCommon_t *)menu->items[i];

		if (item->flags & UIF_NOSELECT) {
			item->topLeft[0] = item->topLeft[1] = 0;
			item->botRight[0] = item->botRight[1] = 0;
			continue;
		}

		switch (item->type) {
		case UITYPE_ACTION:
			Action_Setup ((uiAction_t *) menu->items[i]);
			break;

		case UITYPE_FIELD:
			Field_Setup ((uiField_t *) menu->items[i]);
			break;

		case UITYPE_IMAGE:
			MenuImage_Setup ((uiImage_t *) menu->items[i]);
			break;

		case UITYPE_SLIDER:
			Slider_Setup ((uiSlider_t *) menu->items[i]);
			break;

		case UITYPE_SPINCONTROL:
			SpinControl_Setup ((uiList_t *) menu->items[i]);
			break;
		}

		if (item->topLeft[0] != item->botRight[0] && item->topLeft[1] != item->botRight[1]) {
			// Compensate for font style
			if (item->type != UITYPE_IMAGE && item->flags & UIF_SHADOW) {
				item->botRight[0] += FT_SHAOFFSET;
				item->botRight[1] += FT_SHAOFFSET;
			}
		}
	}
}

/*
=======================================================================

	INITIALIZATION/SHUTDOWN

=======================================================================
*/

/*
=============
UI_CursorInit
=============
*/
void UI_CursorInit (void)
{
	uiState.cursorOverItem = qFalse;

	// Cursor position
	ui_cursorX	= cgi.Cvar_Register ("ui_cursorX",	"-1",	CVAR_READONLY);
	ui_cursorY	= cgi.Cvar_Register ("ui_cursorY",	"-1",	CVAR_READONLY);
	if (ui_cursorX->value == -1 && ui_cursorY->value == -1) {
		uiState.cursorX = cg.glConfig.vidWidth * 0.5f;
		uiState.cursorY = cg.glConfig.vidHeight * 0.5f;
	}
	else {
		uiState.cursorX = ui_cursorX->value;
		uiState.cursorY = ui_cursorY->value;
	}
}


/*
=============
UI_CursorShutdown
=============
*/
void UI_CursorShutdown (void)
{
	// Store the cursor position
	cgi.Cvar_VariableSetValue (ui_cursorX, uiState.cursorX, qTrue);
	cgi.Cvar_VariableSetValue (ui_cursorY, uiState.cursorY, qTrue);
}
