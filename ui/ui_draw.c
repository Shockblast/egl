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

	MENU ITEM DRAWING

=======================================================================
*/

/*
=============
UI_Banner
=============
*/
void UI_Banner (uiImage_t *item, struct image_s *image, float *y) {
	int		width, height;

	if (!image) {
		*y = 0;
		return;
	}

	uii.Img_GetSize (image, &width, &height);
	*y = height * UI_SCALE;

	item->generic.type		= UITYPE_IMAGE;
	item->generic.flags		= UIF_NOSELECT;
	item->generic.name		= NULL;
	item->generic.x			= (width * UI_SCALE * 0.5) * -1; // center
	item->generic.y			= 0;
	item->image				= image;
}


/*
=============
UI_DrawTextBox
=============
*/
void FASTCALL UI_DrawTextBox (float x, float y, float scale, int width, int lines) {
	int		n;
	float	cx, cy;

	// fill in behind it
	UI_DrawFill (x, y, (width + 2)*scale*8, (lines + 2)*scale*8, colorBlack);

	// draw left side
	cx = x;
	cy = y;
	UI_DrawChar (cx, cy, FS_SHADOW, scale, 1, colorWhite);
	for (n=0 ; n<lines ; n++) {
		cy += (8*scale);
		UI_DrawChar (cx, cy, FS_SHADOW, scale, 4, colorWhite);
	}
	UI_DrawChar (cx, cy+(8*scale), FS_SHADOW, scale, 7, colorWhite);

	// draw middle
	cx += (8*scale);
	while (width > 0) {
		cy = y;
		UI_DrawChar (cx, cy, FS_SHADOW, scale, 2, colorWhite);
		for (n = 0; n < lines; n++)
		{
			cy += (8*scale);
			UI_DrawChar (cx, cy, FS_SHADOW, scale, 5, colorWhite);
		}
		UI_DrawChar (cx, cy+(8*scale), FS_SHADOW, scale, 8, colorWhite);
		width -= 1;
		cx += (8*scale);
	}

	// draw right side
	cy = y;
	UI_DrawChar (cx, cy, FS_SHADOW, scale, 3, colorWhite);
	for (n=0 ; n<lines ; n++) {
		cy += (8*scale);
		UI_DrawChar (cx, cy, FS_SHADOW, scale, 6, colorWhite);
	}
	UI_DrawChar (cx, cy+(8*scale), FS_SHADOW, scale, 9, colorWhite);
}


/*
=============
UI_Draw
=============
*/
#define ItemTxtFlags(i, outflags) { \
	if (i->generic.flags & UIF_SHADOW) outflags |= FS_SHADOW; \
}
void Action_Draw (uiAction_t *a) {
	int		txtflags = 0;
	float	xoffset = 0;

	if (!a) return;
	if (!a->generic.name) return;

	ItemTxtFlags (a, txtflags);

	if (a->generic.flags & UIF_LEFT_JUSTIFY)
		xoffset = 0;
	else if (a->generic.flags & UIF_CENTERED)
		xoffset = (((Q_ColorCharCount (a->generic.name, strlen (a->generic.name))) * UISIZE_TYPE (a->generic.flags))*0.5);
	else
		xoffset = ((Q_ColorCharCount (a->generic.name, strlen (a->generic.name))) * UISIZE_TYPE (a->generic.flags)) + RCOLUMN_OFFSET;

	UI_DrawString (a->generic.x + a->generic.parent->x - xoffset, a->generic.y + a->generic.parent->y,
		txtflags, UISCALE_TYPE (a->generic.flags), a->generic.name, colorWhite);

	if (a->generic.ownerDraw)
		a->generic.ownerDraw (a);
}

void Field_Draw (uiField_t *f) {
	int		i, txtflags = 0, curOffset;
	char	tempbuffer[128]="";
	float	x, y;

	if (!f) return;

	ItemTxtFlags (f, txtflags);

	x = f->generic.x + f->generic.parent->x;
	y = f->generic.y + f->generic.parent->y;

	if (f->generic.flags & UIF_CENTERED) {
		x -= ((f->visibleLength + 2) * UISIZE_TYPE (f->generic.flags)) * 0.5;

		if (f->generic.name)
			x -= (Q_ColorCharCount (f->generic.name, strlen (f->generic.name)) * UISIZE_TYPE (f->generic.flags)) * 0.5;
	}

	// title
	if (f->generic.name)
		UI_DrawString (x - ((Q_ColorCharCount (f->generic.name, strlen (f->generic.name)) + 1) * UISIZE_TYPE (f->generic.flags)),
						y, txtflags, UISCALE_TYPE (f->generic.flags), f->generic.name, colorGreen);

	strncpy (tempbuffer, f->buffer + f->visibleOffset, f->visibleLength);

	// left
	UI_DrawChar (x, y - (UISIZE_TYPE (f->generic.flags)*0.5), txtflags, UISCALE_TYPE (f->generic.flags), 18, colorWhite);
	UI_DrawChar (x, y + (UISIZE_TYPE (f->generic.flags)*0.5), txtflags, UISCALE_TYPE (f->generic.flags), 24, colorWhite);

	// right
	UI_DrawChar (x + UISIZE_TYPE (f->generic.flags) + (f->visibleLength * UISIZE_TYPE (f->generic.flags)), y - (UISIZE_TYPE (f->generic.flags)*0.5), txtflags, UISCALE_TYPE (f->generic.flags), 20, colorWhite);
	UI_DrawChar (x + UISIZE_TYPE (f->generic.flags) + (f->visibleLength * UISIZE_TYPE (f->generic.flags)), y + (UISIZE_TYPE (f->generic.flags)*0.5), txtflags, UISCALE_TYPE (f->generic.flags), 26, colorWhite);

	// middle
	for (i=0 ; i<f->visibleLength ; i++) {
		UI_DrawChar (x + UISIZE_TYPE (f->generic.flags) + (i * UISIZE_TYPE (f->generic.flags)), y - (UISIZE_TYPE (f->generic.flags)*0.5), txtflags, UISCALE_TYPE (f->generic.flags), 19, colorWhite);
		UI_DrawChar (x + UISIZE_TYPE (f->generic.flags) + (i * UISIZE_TYPE (f->generic.flags)), y + (UISIZE_TYPE (f->generic.flags)*0.5), txtflags, UISCALE_TYPE (f->generic.flags), 25, colorWhite);
	}

	// text
	curOffset = UI_DrawString (x + UISIZE_TYPE (f->generic.flags), y, txtflags, UISCALE_TYPE (f->generic.flags), tempbuffer, colorWhite);

	// blinking cursor
	if (UI_ItemAtCursor (f->generic.parent) == f) {
		int offset;

		if (f->visibleOffset)
			offset = f->visibleLength;
		else
			offset = curOffset;

		offset++;

		if ((((int)(uii.Sys_Milliseconds() / 250))&1))
			UI_DrawChar (x + (offset * UISIZE_TYPE (f->generic.flags)),
				y, txtflags, UISCALE_TYPE (f->generic.flags), 11, colorWhite);
	}
}

void MenuImage_Draw (uiImage_t *i) {
	struct	image_s *image;
	float	x, y;
	int		width, height;

	if (!i)
		return;

	if ((uiCursorItem && (uiCursorItem == i)) && i->selImage)
		image = i->selImage;
	else
		image = i->image;

	uii.Img_GetSize (image, &width, &height);

	x = i->generic.x + i->generic.parent->x;
	if (i->generic.flags & UIF_CENTERED)
		x -= (width * 0.5 * UISCALE_TYPE (i->generic.flags));
	else if (i->generic.flags & UIF_LEFT_JUSTIFY)
		x += LCOLUMN_OFFSET;

	y = i->generic.y + i->generic.parent->y;

	if (image)
		UI_DrawPic (image, x, y,
					width * UISCALE_TYPE (i->generic.flags), height * UISCALE_TYPE (i->generic.flags),
					0, 0, 1, 1, colorWhite);
}

void Slider_Draw (uiSlider_t *s) {
	int		i, txtflags = 0;

	if (!s) return;

	ItemTxtFlags (s, txtflags);

	// name
	if (s->generic.name) {
		UI_DrawString (s->generic.x + s->generic.parent->x + LCOLUMN_OFFSET - ((Q_ColorCharCount (s->generic.name, strlen (s->generic.name)) - 1)*UISIZE_TYPE (s->generic.flags)),
			s->generic.y + s->generic.parent->y, txtflags, UISCALE_TYPE (s->generic.flags), s->generic.name, colorGreen);
	}

	s->range = clamp (((s->curValue - s->minValue) / (float) (s->maxValue - s->minValue)), 0, 1);

	// left
	UI_DrawChar (s->generic.x + s->generic.parent->x + RCOLUMN_OFFSET, s->generic.y + s->generic.parent->y, txtflags, UISCALE_TYPE (s->generic.flags), 128, colorWhite);

	// center
	for (i=0 ; i<SLIDER_RANGE ; i++)
		UI_DrawChar (RCOLUMN_OFFSET + s->generic.x + i*UISIZE_TYPE (s->generic.flags) + s->generic.parent->x + UISIZE_TYPE (s->generic.flags), s->generic.y + s->generic.parent->y, txtflags, UISCALE_TYPE (s->generic.flags), 129, colorWhite);

	// right
	UI_DrawChar (RCOLUMN_OFFSET + s->generic.x + i*UISIZE_TYPE (s->generic.flags) + s->generic.parent->x + UISIZE_TYPE (s->generic.flags), s->generic.y + s->generic.parent->y, txtflags, UISCALE_TYPE (s->generic.flags), 130, colorWhite);

	// slider bar
	UI_DrawChar ((UISIZE_TYPE (s->generic.flags) + RCOLUMN_OFFSET + s->generic.parent->x + s->generic.x + (SLIDER_RANGE-1)*UISIZE_TYPE (s->generic.flags) * s->range), s->generic.y + s->generic.parent->y, txtflags, UISCALE_TYPE (s->generic.flags), 131, colorWhite);
}

void SpinControl_Draw (uiList_t *s) {
	int		txtflags = 0;
	char	buffer[100];

	if (!s) return;

	ItemTxtFlags (s, txtflags);

	if (s->generic.name) {
		UI_DrawString (s->generic.x + s->generic.parent->x + LCOLUMN_OFFSET - ((Q_ColorCharCount (s->generic.name, strlen (s->generic.name)) - 1)*UISIZE_TYPE (s->generic.flags)),
					s->generic.y + s->generic.parent->y, txtflags, UISCALE_TYPE (s->generic.flags), s->generic.name, colorGreen);
	}

	if (!strchr(s->itemNames[s->curValue], '\n')) {
		UI_DrawString (RCOLUMN_OFFSET + s->generic.x + s->generic.parent->x,
					s->generic.y + s->generic.parent->y, txtflags, UISCALE_TYPE (s->generic.flags), s->itemNames[s->curValue], colorWhite);
	} else {
		strcpy (buffer, s->itemNames[s->curValue]);
		*strchr (buffer, '\n') = 0;
		UI_DrawString (RCOLUMN_OFFSET + s->generic.x + s->generic.parent->x,
					s->generic.y + s->generic.parent->y, txtflags, UISCALE_TYPE (s->generic.flags), buffer, colorWhite);

		strcpy (buffer, strchr (s->itemNames[s->curValue], '\n') + 1);
		UI_DrawString (RCOLUMN_OFFSET + s->generic.x + s->generic.parent->x,
					s->generic.y + s->generic.parent->y + UISIZEINC_TYPE(s->generic.flags), txtflags, UISCALE_TYPE (s->generic.flags), buffer, colorWhite);
	}
}

void Selection_Draw (uiCommon_t *i) {
	if (i->flags & UIF_NOSELBAR)
		return;
	else {
		vec4_t bgColor;
		Vector4Set (bgColor, colorDkGrey[0], colorDkGrey[1], colorDkGrey[2], 0.33);

		if ((uiCursorItem && (uiCursorItem == i)) || (uiSelItem && (uiSelItem == i)))
			UI_DrawFill (i->tl[0], i->tl[1], i->br[0] - i->tl[0], i->br[1] - i->tl[1], bgColor);
	}
}

void UI_Draw (uiFrameWork_t *menu) {
	int			i;
	uiCommon_t	*item;
	uiCommon_t	*selitem;

	/*
	** draw contents
	*/
	for (i=0 ; i<menu->numItems ; i++) {
		Selection_Draw ((uiCommon_t *) menu->items[i]);

		switch (((uiCommon_t *) menu->items[i])->type) {
		case UITYPE_ACTION:			Action_Draw ((uiAction_t *) menu->items[i]);			break;
		case UITYPE_FIELD:			Field_Draw ((uiField_t *) menu->items[i]);				break;
		case UITYPE_IMAGE:			MenuImage_Draw ((uiImage_t *) menu->items[i]);			break;
		case UITYPE_SLIDER:			Slider_Draw ((uiSlider_t *) menu->items[i]);			break;
		case UITYPE_SPINCONTROL:	SpinControl_Draw ((uiList_t *) menu->items[i]);			break;
		}
	}

	//
	// blinking cursor
	//

	item = UI_ItemAtCursor (menu);
	if (item && item->cursorDraw)
		item->cursorDraw (item);
	else if (menu->cursorDraw)
		menu->cursorDraw (menu);
	else if (item && (item->type != UITYPE_FIELD) && (item->type != UITYPE_IMAGE) && (!(item->flags & UIF_NOSELECT))) {
		if (item->flags & UIF_LEFT_JUSTIFY)
			UI_DrawChar (menu->x + item->x + LCOLUMN_OFFSET + item->cursorOffset, menu->y + item->y, 0, UISCALE_TYPE (item->flags), 12 + ((int)(uii.Sys_Milliseconds()/250) & 1), colorWhite);
		else if (item->flags & UIF_CENTERED)
			UI_DrawChar (menu->x + item->x - (((Q_ColorCharCount (item->name, strlen (item->name)) + 4) * UISIZE_TYPE (item->flags))*0.5) + item->cursorOffset, menu->y + item->y, 0, UISCALE_TYPE (item->flags), 12 + ((int)(uii.Sys_Milliseconds()/250) & 1), colorWhite);
		else
			UI_DrawChar (menu->x + item->x + item->cursorOffset, menu->y + item->y, 0, UISCALE_TYPE (item->flags), 12 + ((int)(uii.Sys_Milliseconds()/250) & 1), colorWhite);
	}

	selitem = (uiCommon_t *) uiSelItem;
	if (selitem && (selitem->type != UITYPE_FIELD) && (selitem->type != UITYPE_IMAGE) && (!(selitem->flags & UIF_NOSELECT))) {
		if (selitem->flags & UIF_LEFT_JUSTIFY)
			UI_DrawChar (menu->x + selitem->x + LCOLUMN_OFFSET + selitem->cursorOffset, menu->y + selitem->y, 0, UISCALE_TYPE (selitem->flags), 12 + ((int)(uii.Sys_Milliseconds()/250) & 1), colorWhite);
		else if (selitem->flags & UIF_CENTERED)
			UI_DrawChar (menu->x + selitem->x - (((Q_ColorCharCount (selitem->name, strlen (selitem->name)) + 4) * UISIZE_TYPE (selitem->flags))*0.5) + selitem->cursorOffset, menu->y + selitem->y, 0, UISCALE_TYPE (selitem->flags), 12 + ((int)(uii.Sys_Milliseconds()/250) & 1), colorWhite);
		else
			UI_DrawChar (menu->x + selitem->x + selitem->cursorOffset, menu->y + selitem->y, 0, UISCALE_TYPE (selitem->flags), 12 + ((int)(uii.Sys_Milliseconds()/250) & 1), colorWhite);
	}

	//
	// statusbar
	//
	if (item) {
		if (item->statusBarFunc)
			item->statusBarFunc ((void *) item);
		else if (item->statusBar)
			UI_DrawStatusBar (item->statusBar);
		else
			UI_DrawStatusBar (menu->statusBar);
	} else
		UI_DrawStatusBar (menu->statusBar);
}


/*
=============
UI_DrawStatusBar
=============
*/
void UI_DrawStatusBar (char *string) {
	if (string) {
		int col = (uis.vidWidth*0.5) - (((Q_ColorCharCount (string, strlen (string))) * 0.5) * UIFT_SIZE);

		UI_DrawFill (0, uis.vidHeight - UIFT_SIZE - 2, uis.vidWidth, UIFT_SIZE + 2, colorDkGrey);
		UI_DrawString (col, uis.vidHeight - UIFT_SIZE - 1, FS_SHADOW, UIFT_SCALE, string, colorWhite);
	}
}


/*
=================
UI_Refresh
=================
*/
void UI_Refresh (int time, int connectionState, int serverState, int vidWidth, int vidHeight, qBool backGround) {
	if (!uii.Key_KeyDest (KD_MENU))
		return;

	uis.time = time;
	uis.connectionState = connectionState;
	uis.serverState = serverState;
	uis.vidWidth = vidWidth;
	uis.vidHeight = vidHeight;

	if (backGround) {
		vec4_t	fillColor;
		Vector4Set (fillColor, colorBlack[0], colorBlack[1], colorBlack[2], 1);
		UI_DrawFill (0, 0, uis.vidWidth, uis.vidHeight, fillColor);

		UI_DrawPic (uiMedia.bgBig, 0, 0, vidWidth, vidHeight, 0, 0, 1, 1, colorWhite);
	} else {
		vec4_t	fillColor;
		Vector4Set (fillColor, colorBlack[0], colorBlack[1], colorBlack[2], 0.9);
		UI_DrawFill (0, 0, uis.vidWidth, uis.vidHeight, fillColor);
	}

	UI_DrawFunc ();

	UI_DrawMouseCursor ();

	/*
	** delay playing the enter sound until after the menu has
	** been drawn, to avoid delay while caching images
	*/
	if (uiEnterSound) {
		uii.Snd_StartLocalSound (uiMedia.menuInSfx);
		uiEnterSound = qFalse;
	}
}
