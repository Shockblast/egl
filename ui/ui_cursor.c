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

#include "ui_local.h"

/*
=======================================================================

	MENU MOUSE CURSOR

=======================================================================
*/

static float	ui_CursorScale;

qBool	uiCursorLock;
void	(*uiCursorItem);
void	(*uiMouseItem);
void	(*uiSelItem);

#define CURSOR_ONITEMSCALE		0.66f

/*
=============
UI_CursorInit
=============
*/
void UI_CursorInit (void) {
	ui_CursorScale = 1;
}


/*
=============
UI_DrawMouseCursor
=============
*/
void UI_DrawMouseCursor (void) {
	int		width, height;
	uii.Img_GetSize (uiMedia.cursorImage, &width, &height);
	UI_DrawPic (uiMedia.cursorImage, uis.cursorX, uis.cursorY,
				width * clamp (UIFT_SCALE, 0.5, 1) * ui_CursorScale,
				height * clamp (UIFT_SCALE, 0.5, 1) * ui_CursorScale,
				0, 0, 1, 1, colorWhite);
}


/*
=============
UI_UpdateMousePos
=============
*/
void UI_UpdateMousePos (void) {
	int		i;

	if (!uiActiveUI || !uiActiveUI->numItems || uiCursorLock)
		return;

	if (uiCursorLock)
		return;

	// search for the current item that the mouse is over
	uiMouseItem = NULL;
	for (i=0 ; i<uiActiveUI->numItems ; i++) {
		if (((uiCommon_t *) uiActiveUI->items[i])->flags & UIF_NOSELECT)
			continue;

		if (uis.cursorX > ((uiCommon_t *) uiActiveUI->items[i])->br[0] || 
			uis.cursorY > ((uiCommon_t *) uiActiveUI->items[i])->br[1] ||
			uis.cursorX < ((uiCommon_t *) uiActiveUI->items[i])->tl[0] || 
			uis.cursorY < ((uiCommon_t *) uiActiveUI->items[i])->tl[1])
			continue;

		uiCursorItem	= uiActiveUI->items[i];
		uiMouseItem	= uiActiveUI->items[i];

		if (uiActiveUI->cursor == i)
			break;

		UI_AdjustTextCursor (uiActiveUI, i - uiActiveUI->cursor);
		uiActiveUI->cursor = i;

		if (!uiEnterSound)
			uii.Snd_StartLocalSound (uiMedia.menuMoveSfx);

		break;
	}

	// shrink cursor when over an item with the mouse
	if (uiMouseItem) {
		ui_CursorScale = CURSOR_ONITEMSCALE;
	} else {
		ui_CursorScale = 1;
	}
}


/*
=============
UI_MoveMouse
=============
*/
void UI_MoveMouse (int mx, int my) {
	if (uiCursorLock)
		return;

	uis.cursorX = clamp (uis.cursorX += (mx * ui_sensitivity->value), 2, uis.vidWidth - 2);
	uis.cursorY = clamp (uis.cursorY += (my * ui_sensitivity->value), 2, uis.vidHeight - 2);

	if (mx || my)
		UI_UpdateMousePos ();
}


/*
=============
UI_Setup
=============
*/
void Action_Setup (uiAction_t *s) {
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

void Field_Setup (uiField_t *s) {
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

void MenuImage_Setup (uiImage_t *s) {
	int		width, height;

	if (!s)
		return;

	uii.Img_GetSize (s->image, &width, &height);

	s->generic.tl[0] = s->generic.x + s->generic.parent->x;

	if (s->generic.flags & UIF_CENTERED)
		s->generic.tl[0] -= (width * 0.5 * UISCALE_TYPE (s->generic.flags));
	else if (s->generic.flags & UIF_LEFT_JUSTIFY)
		s->generic.tl[0] += LCOLUMN_OFFSET;

	s->generic.tl[1] = s->generic.y + s->generic.parent->y;
	s->generic.br[0] = s->generic.tl[0] + width * UISCALE_TYPE (s->generic.flags);
	s->generic.br[1] = s->generic.tl[1] + height * UISCALE_TYPE (s->generic.flags);
}

// kthx need calculations to detect when on the left/right side of the slider
// so left == less and right == more
// if the mouse cursor position is less than the cursor position and is within the bounding box, left
// if the mouse cursor position is greater than the cursor position and is within the bounding box, right
void Slider_Setup (uiSlider_t *s) {
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

void SpinControl_Setup (uiList_t *s) {
	float		xsize = 0, ysize = 0;

	if (!s)
		return;

	ysize = UISIZE_TYPE (s->generic.flags);
	if (s->generic.name) {
		s->generic.tl[0] = s->generic.x + s->generic.parent->x - (Q_ColorCharCount (s->generic.name, strlen (s->generic.name)) * UISIZE_TYPE (s->generic.flags)) - UISIZE_TYPE (s->generic.flags);
		s->generic.tl[1] = s->generic.y + s->generic.parent->y;

		xsize = (Q_ColorCharCount (s->generic.name, strlen (s->generic.name)) * UISIZE_TYPE (s->generic.flags)) + UISIZE_TYPE (s->generic.flags)*3;
	}

	if (s->itemNames[s->curValue] && (!strchr(s->itemNames[s->curValue], '\n'))) {
		if (!s->generic.name)
			s->generic.tl[0] += UISIZE_TYPE (s->generic.flags)*20;
	} else
		ysize += UISIZE_TYPE (s->generic.flags);

	if (s->itemNames[s->curValue])
		xsize += (Q_ColorCharCount (s->itemNames[s->curValue], strlen (s->itemNames[s->curValue])) * UISIZE_TYPE (s->generic.flags));

	s->generic.br[0] = s->generic.tl[0] + xsize;
	s->generic.br[1] = s->generic.tl[1] + ysize;
}

void UI_Setup (uiFrameWork_t *menu) {
	int i;

	// draw contents
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
			// compensate for font style
			if (((uiCommon_t *) menu->items[i])->type != UITYPE_IMAGE) {
				if (((uiCommon_t *) menu->items[i])->flags & UIF_SHADOW) {
					((uiCommon_t *) menu->items[i])->br[0] += FT_SHAOFFSET;
					((uiCommon_t *) menu->items[i])->br[1] += FT_SHAOFFSET;
				}
			}
		}
	}
}
