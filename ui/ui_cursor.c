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

/*
=======================================================================

	MENU MOUSE CURSOR

=======================================================================
*/

static qBool	ui_cursorOverItem;

qBool	ui_cursorLock;
void	(*ui_cursorItem);
void	(*ui_mouseItem);
void	(*ui_selectedItem);

/*
=============
UI_CursorInit
=============
*/
void UI_CursorInit (void)
{
	ui_cursorOverItem = qFalse;
}


/*
=============
UI_DrawMouseCursor
=============
*/
void UI_DrawMouseCursor (void)
{
	int		width, height;
	struct shader_s *cursor;

	if (ui_cursorOverItem)
		cursor = uiMedia.cursorHoverShader;
	else
		cursor = uiMedia.cursorShader;

	uii.R_GetImageSize (cursor, &width, &height);
	UI_DrawPic (cursor, uiState.cursorX + 1, uiState.cursorY + 1,
				width * clamp (UIFT_SCALE, 0.5, 1),
				height * clamp (UIFT_SCALE, 0.5, 1),
				0, 0, 1, 1, colorWhite);
}


/*
=============
UI_UpdateMousePos
=============
*/
void UI_UpdateMousePos (void)
{
	int		i;

	if (!UI_ActiveUI || !UI_ActiveUI->numItems || ui_cursorLock)
		return;

	// Search for the current item that the mouse is over
	ui_mouseItem = NULL;
	for (i=0 ; i<UI_ActiveUI->numItems ; i++) {
		if (((uiCommon_t *) UI_ActiveUI->items[i])->flags & UIF_NOSELECT)
			continue;

		if (uiState.cursorX > ((uiCommon_t *) UI_ActiveUI->items[i])->br[0] || 
			uiState.cursorY > ((uiCommon_t *) UI_ActiveUI->items[i])->br[1] ||
			uiState.cursorX < ((uiCommon_t *) UI_ActiveUI->items[i])->tl[0] || 
			uiState.cursorY < ((uiCommon_t *) UI_ActiveUI->items[i])->tl[1])
			continue;

		ui_cursorItem = UI_ActiveUI->items[i];
		ui_mouseItem = UI_ActiveUI->items[i];

		if (UI_ActiveUI->cursor == i)
			break;

		UI_AdjustCursor (UI_ActiveUI, i - UI_ActiveUI->cursor);
		UI_ActiveUI->cursor = i;

		if (!ui_playEnterSound)
			uii.Snd_StartLocalSound (uiMedia.sounds.menuMove);

		break;
	}

	// Rollover cursor image
	if (ui_mouseItem)
		ui_cursorOverItem = qTrue;
	else
		ui_cursorOverItem = qFalse;
}


/*
=============
UI_MoveMouse
=============
*/
void UI_MoveMouse (float x, float y)
{
	if (ui_cursorLock)
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
	uiState.cursorX = clamp (uiState.cursorX, 2, uiState.vidWidth - 2);
	uiState.cursorY = clamp (uiState.cursorY, 2, uiState.vidHeight - 2);

	if (x || y)
		UI_UpdateMousePos ();
}


/*
=============
UI_SetCursorPos

If the position is out of screen bounds, it's forced to the center of the screen
=============
*/
void UI_SetCursorPos (float x, float y)
{
	if (x < 2 || y < 2 || x > uiState.vidWidth - 2 || y > uiState.vidHeight - 2) {
		uiState.cursorX = uiState.vidWidth * 0.5f;
		uiState.cursorY = uiState.vidHeight * 0.5f;
	}
	else {
		uiState.cursorX = x;
		uiState.cursorY = y;
	}
}


/*
=============
UI_SetupMenuBounds
=============
*/
static void Action_Setup (uiAction_t *s)
{
	if (!s)
		return;
	if (!s->generic.name)
		return;

	s->generic.tl[0] = s->generic.x + s->generic.parent->x;
	s->generic.tl[1] = s->generic.y + s->generic.parent->y;

	s->generic.br[1] = s->generic.tl[1] + UISIZE_TYPE (s->generic.flags);

	if (s->generic.flags & UIF_CENTERED)
		s->generic.tl[0] -= ((Q_ColorCharCount (s->generic.name, strlen (s->generic.name)) * UISIZE_TYPE (s->generic.flags))*0.5);
	else if (!(s->generic.flags & UIF_LEFT_JUSTIFY))
		s->generic.tl[0] += LCOLUMN_OFFSET - ((Q_ColorCharCount (s->generic.name, strlen (s->generic.name))) * UISIZE_TYPE (s->generic.flags));

	s->generic.br[0] = s->generic.tl[0] + (Q_ColorCharCount (s->generic.name, strlen (s->generic.name)) * UISIZE_TYPE (s->generic.flags));
}

static void Field_Setup (uiField_t *s)
{
	if (!s)
		return;

	s->generic.tl[0] = s->generic.x + s->generic.parent->x;

	if (s->generic.name)
		s->generic.tl[0] -= (Q_ColorCharCount (s->generic.name, strlen (s->generic.name)) + 1) * UISIZE_TYPE (s->generic.flags);

	s->generic.tl[1] = s->generic.y + s->generic.parent->y - (UISIZE_TYPE (s->generic.flags) * 0.5);

	s->generic.br[0] = s->generic.x + s->generic.parent->x + ((s->visibleLength + 2) * UISIZE_TYPE (s->generic.flags));
	s->generic.br[1] = s->generic.tl[1] + UISIZE_TYPE (s->generic.flags)*2;

	if (s->generic.flags & UIF_CENTERED) {
		s->generic.tl[0] -= ((s->visibleLength + 2) * UISIZE_TYPE (s->generic.flags))*0.5;
		s->generic.br[0] -= ((s->visibleLength + 2) * UISIZE_TYPE (s->generic.flags))*0.5;
	}
}

static void MenuImage_Setup (uiImage_t *s)
{
	int		width, height;

	if (!s)
		return;

	if (s->width || s->height) {
		width = s->width;
		height = s->height;
	}
	else
		uii.R_GetImageSize (s->shader, &width, &height);

	width *= UISCALE_TYPE (s->generic.flags);
	height *= UISCALE_TYPE (s->generic.flags);

	s->generic.tl[0] = s->generic.x + s->generic.parent->x;

	if (s->generic.flags & UIF_CENTERED)
		s->generic.tl[0] -= width * 0.5;
	else if (s->generic.flags & UIF_LEFT_JUSTIFY)
		s->generic.tl[0] += LCOLUMN_OFFSET;

	s->generic.tl[1] = s->generic.y + s->generic.parent->y;
	s->generic.br[0] = s->generic.tl[0] + width;
	s->generic.br[1] = s->generic.tl[1] + height;
}

// kthx need calculations to detect when on the left/right side of the slider
// so left == less and right == more
// if the mouse cursor position is less than the cursor position and is within the bounding box, left
// if the mouse cursor position is greater than the cursor position and is within the bounding box, right
static void Slider_Setup (uiSlider_t *s)
{
	float		xsize, ysize;
	float		offset;

	if (!s)
		return;

	if (s->generic.name)
		offset = Q_ColorCharCount (s->generic.name, strlen (s->generic.name)) * UISIZE_TYPE (s->generic.flags);
	else
		offset = 0;

	s->generic.tl[0] = s->generic.x + s->generic.parent->x - offset - UISIZE_TYPE (s->generic.flags);
	s->generic.tl[1] = s->generic.y + s->generic.parent->y;

	xsize = (UISIZE_TYPE (s->generic.flags) * 6) + offset + ((SLIDER_RANGE-1) * UISIZE_TYPE (s->generic.flags));
	ysize = UISIZE_TYPE (s->generic.flags);

	s->generic.br[0] = s->generic.tl[0] + xsize;
	s->generic.br[1] = s->generic.tl[1] + ysize;
}

static void SpinControl_Setup (uiList_t *s)
{
	float		xsize = 0, ysize = 0;

	if (!s)
		return;

	ysize = UISIZE_TYPE (s->generic.flags);
	if (s->generic.name) {
		s->generic.tl[0] = s->generic.x + s->generic.parent->x - (Q_ColorCharCount (s->generic.name, strlen (s->generic.name)) * UISIZE_TYPE (s->generic.flags)) - UISIZE_TYPE (s->generic.flags);
		s->generic.tl[1] = s->generic.y + s->generic.parent->y;

		xsize = (Q_ColorCharCount (s->generic.name, strlen (s->generic.name)) * UISIZE_TYPE (s->generic.flags)) + UISIZE_TYPE (s->generic.flags)*3;
	}

	if (s->itemNames) {
		if (s->itemNames[s->curValue] && (!strchr(s->itemNames[s->curValue], '\n'))) {
			if (!s->generic.name)
				s->generic.tl[0] += UISIZE_TYPE (s->generic.flags)*20;
		}
		else
			ysize += UISIZE_TYPE (s->generic.flags);

		if (s->itemNames[s->curValue])
			xsize += (Q_ColorCharCount (s->itemNames[s->curValue], strlen (s->itemNames[s->curValue])) * UISIZE_TYPE (s->generic.flags));
	}

	s->generic.br[0] = s->generic.tl[0] + xsize;
	s->generic.br[1] = s->generic.tl[1] + ysize;
}

void UI_SetupMenuBounds (uiFrameWork_t *menu)
{
	int i;

	// Draw contents
	for (i=0 ; i<menu->numItems ; i++) {
		if (((uiCommon_t *) menu->items[i])->flags & (UIF_NOSELECT))
			continue;

		switch (((uiCommon_t *) menu->items[i])->type) {
		case UITYPE_ACTION:			Action_Setup ((uiAction_t *) menu->items[i]);			break;
		case UITYPE_FIELD:			Field_Setup ((uiField_t *) menu->items[i]);				break;
		case UITYPE_IMAGE:			MenuImage_Setup ((uiImage_t *) menu->items[i]);			break;
		case UITYPE_SLIDER:			Slider_Setup ((uiSlider_t *) menu->items[i]);			break;
		case UITYPE_SPINCONTROL:	SpinControl_Setup ((uiList_t *) menu->items[i]);		break;
		}

		if ((((uiCommon_t *) menu->items[i])->tl[0] != ((uiCommon_t *) menu->items[i])->br[0]) &&
			(((uiCommon_t *) menu->items[i])->tl[1] != ((uiCommon_t *) menu->items[i])->br[1])) {
			// Compensate for font style
			if (((uiCommon_t *) menu->items[i])->type != UITYPE_IMAGE) {
				if (((uiCommon_t *) menu->items[i])->flags & UIF_SHADOW) {
					((uiCommon_t *) menu->items[i])->br[0] += FT_SHAOFFSET;
					((uiCommon_t *) menu->items[i])->br[1] += FT_SHAOFFSET;
				}
			}
		}
	}
}
