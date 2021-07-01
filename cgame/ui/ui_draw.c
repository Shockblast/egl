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
// ui_draw.c
//

#include "ui_local.h"

/*
=======================================================================

	MENU ITEM DRAWING

=======================================================================
*/

/*
=============
UI_DrawTextBox
=============
*/
void UI_DrawTextBox (float x, float y, float scale, int width, int lines)
{
	int		n;
	float	cx, cy;

	// fill in behind it
	cgi.R_DrawFill (x, y, (width + 2)*scale*8, (lines + 2)*scale*8, colorBlack);

	// draw left side
	cx = x;
	cy = y;
	CG_DrawChar (cx, cy, FS_SHADOW, scale, 1, colorWhite);
	for (n=0 ; n<lines ; n++) {
		cy += (8*scale);
		CG_DrawChar (cx, cy, FS_SHADOW, scale, 4, colorWhite);
	}
	CG_DrawChar (cx, cy+(8*scale), FS_SHADOW, scale, 7, colorWhite);

	// draw middle
	cx += (8*scale);
	while (width > 0) {
		cy = y;
		CG_DrawChar (cx, cy, FS_SHADOW, scale, 2, colorWhite);
		for (n = 0; n < lines; n++)
		{
			cy += (8*scale);
			CG_DrawChar (cx, cy, FS_SHADOW, scale, 5, colorWhite);
		}
		CG_DrawChar (cx, cy+(8*scale), FS_SHADOW, scale, 8, colorWhite);
		width -= 1;
		cx += (8*scale);
	}

	// draw right side
	cy = y;
	CG_DrawChar (cx, cy, FS_SHADOW, scale, 3, colorWhite);
	for (n=0 ; n<lines ; n++) {
		cy += (8*scale);
		CG_DrawChar (cx, cy, FS_SHADOW, scale, 6, colorWhite);
	}
	CG_DrawChar (cx, cy+(8*scale), FS_SHADOW, scale, 9, colorWhite);
}


/*
=============
UI_CenterHeight
=============
*/
static void UI_CenterHeight (uiFrameWork_t *fw)
{
	float		lowest, highest, height;
	uiCommon_t	*item;
	int			i;

	if (!fw)
		return;

	highest = lowest = height = 0;
	for (i=0 ; i<fw->numItems ; i++) {
		item = (uiCommon_t *) fw->items[i];

		if (item->y < highest)
			highest = item->y;

		if (item->type == UITYPE_IMAGE)
			height = ((uiImage_t *) fw->items[i])->height * UISCALE_TYPE (item->flags);
		else if (item->flags & UIF_MEDIUM)
			height = UIFT_SIZEINCMED;
		else if (item->flags & UIF_LARGE)
			height = UIFT_SIZEINCLG;
		else
			height = UIFT_SIZEINC;

		if (item->y + height > lowest)
			lowest = item->y + height;
	}

	fw->y += (cg.glConfig.vidHeight - (lowest - highest)) * 0.5f;
}


/*
=============
UI_DrawStatusBar
=============
*/
static void UI_DrawStatusBar (char *string)
{
	if (string) {
		int col = (cg.glConfig.vidWidth*0.5) - (((Q_ColorCharCount (string, (int)strlen (string))) * 0.5) * UIFT_SIZE);

		cgi.R_DrawFill (0, cg.glConfig.vidHeight - UIFT_SIZE - 2, cg.glConfig.vidWidth, UIFT_SIZE + 2, colorDkGrey);
		CG_DrawString (col, cg.glConfig.vidHeight - UIFT_SIZE - 1, FS_SHADOW, UIFT_SCALE, string, colorWhite);
	}
}


/*
=============
UI_DrawInterface
=============
*/
#define ItemTxtFlags(i, outflags) { \
	if (i->generic.flags & UIF_SHADOW) outflags |= FS_SHADOW; \
}
static void Action_Draw (uiAction_t *a)
{
	int		txtflags = 0;
	float	xoffset = 0;

	if (!a) return;
	if (!a->generic.name) return;

	ItemTxtFlags (a, txtflags);

	if (a->generic.flags & UIF_LEFT_JUSTIFY)
		xoffset = 0;
	else if (a->generic.flags & UIF_CENTERED)
		xoffset = (((Q_ColorCharCount (a->generic.name, (int)strlen (a->generic.name))) * UISIZE_TYPE (a->generic.flags))*0.5);
	else
		xoffset = ((Q_ColorCharCount (a->generic.name, (int)strlen (a->generic.name))) * UISIZE_TYPE (a->generic.flags)) + RCOLUMN_OFFSET;

	CG_DrawString (a->generic.x + a->generic.parent->x - xoffset, a->generic.y + a->generic.parent->y,
		txtflags, UISCALE_TYPE (a->generic.flags), a->generic.name, colorWhite);

	if (a->generic.ownerDraw)
		a->generic.ownerDraw (a);
}

static void Field_Draw (uiField_t *f)
{
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
			x -= (Q_ColorCharCount (f->generic.name, (int)strlen (f->generic.name)) * UISIZE_TYPE (f->generic.flags)) * 0.5;
	}

	// title
	if (f->generic.name)
		CG_DrawString (x - ((Q_ColorCharCount (f->generic.name, (int)strlen (f->generic.name)) + 1) * UISIZE_TYPE (f->generic.flags)),
						y, txtflags, UISCALE_TYPE (f->generic.flags), f->generic.name, colorGreen);

	Q_strncpyz (tempbuffer, f->buffer + f->visibleOffset, f->visibleLength);

	// left
	CG_DrawChar (x, y - (UISIZE_TYPE (f->generic.flags)*0.5), txtflags, UISCALE_TYPE (f->generic.flags), 18, colorWhite);
	CG_DrawChar (x, y + (UISIZE_TYPE (f->generic.flags)*0.5), txtflags, UISCALE_TYPE (f->generic.flags), 24, colorWhite);

	// right
	CG_DrawChar (x + UISIZE_TYPE (f->generic.flags) + (f->visibleLength * UISIZE_TYPE (f->generic.flags)), y - (UISIZE_TYPE (f->generic.flags)*0.5), txtflags, UISCALE_TYPE (f->generic.flags), 20, colorWhite);
	CG_DrawChar (x + UISIZE_TYPE (f->generic.flags) + (f->visibleLength * UISIZE_TYPE (f->generic.flags)), y + (UISIZE_TYPE (f->generic.flags)*0.5), txtflags, UISCALE_TYPE (f->generic.flags), 26, colorWhite);

	// middle
	for (i=0 ; i<f->visibleLength ; i++) {
		CG_DrawChar (x + UISIZE_TYPE (f->generic.flags) + (i * UISIZE_TYPE (f->generic.flags)), y - (UISIZE_TYPE (f->generic.flags)*0.5), txtflags, UISCALE_TYPE (f->generic.flags), 19, colorWhite);
		CG_DrawChar (x + UISIZE_TYPE (f->generic.flags) + (i * UISIZE_TYPE (f->generic.flags)), y + (UISIZE_TYPE (f->generic.flags)*0.5), txtflags, UISCALE_TYPE (f->generic.flags), 25, colorWhite);
	}

	// text
	curOffset = CG_DrawString (x + UISIZE_TYPE (f->generic.flags), y, txtflags, UISCALE_TYPE (f->generic.flags), tempbuffer, colorWhite);

	// blinking cursor
	if (UI_ItemAtCursor (f->generic.parent) == f) {
		int offset;

		if (f->visibleOffset)
			offset = f->visibleLength;
		else
			offset = curOffset;

		offset++;

		if ((((int)(cg.realTime / 250))&1))
			CG_DrawChar (x + (offset * UISIZE_TYPE (f->generic.flags)),
				y, txtflags, UISCALE_TYPE (f->generic.flags), 11, colorWhite);
	}
}

static void Image_Draw (uiImage_t *i)
{
	struct	shader_s *shader;
	float	x, y;
	int		width, height;

	if (!i)
		return;

	if ((uiState.cursorItem && uiState.cursorItem == i) && i->hoverShader)
		shader = i->hoverShader;
	else
		shader = i->shader;
	if (!shader)
		shader = uiMedia.whiteTexture;

	if (i->width || i->height) {
		width = i->width;
		height = i->height;
	}
	else {
		cgi.R_GetImageSize (shader, &width, &height);
		i->width = width;
		i->height = height;
	}

	width *= UISCALE_TYPE (i->generic.flags);
	height *= UISCALE_TYPE (i->generic.flags);

	x = i->generic.x + i->generic.parent->x;
	if (i->generic.flags & UIF_CENTERED)
		x -= width * 0.5f;
	else if (i->generic.flags & UIF_LEFT_JUSTIFY)
		x += LCOLUMN_OFFSET;

	y = i->generic.y + i->generic.parent->y;

	cgi.R_DrawPic (shader, 0, x, y,
				width, height,
				0, 0, 1, 1, colorWhite);
}

static void Slider_Draw (uiSlider_t *s)
{
	int		i, txtflags = 0;

	if (!s) return;

	ItemTxtFlags (s, txtflags);

	// name
	if (s->generic.name) {
		CG_DrawString (s->generic.x + s->generic.parent->x + LCOLUMN_OFFSET - ((Q_ColorCharCount (s->generic.name, (int)strlen (s->generic.name)) - 1)*UISIZE_TYPE (s->generic.flags)),
			s->generic.y + s->generic.parent->y, txtflags, UISCALE_TYPE (s->generic.flags), s->generic.name, colorGreen);
	}

	s->range = clamp (((s->curValue - s->minValue) / (float) (s->maxValue - s->minValue)), 0, 1);

	// left
	CG_DrawChar (s->generic.x + s->generic.parent->x + RCOLUMN_OFFSET, s->generic.y + s->generic.parent->y, txtflags, UISCALE_TYPE (s->generic.flags), 128, colorWhite);

	// center
	for (i=0 ; i<SLIDER_RANGE ; i++)
		CG_DrawChar (RCOLUMN_OFFSET + s->generic.x + i*UISIZE_TYPE (s->generic.flags) + s->generic.parent->x + UISIZE_TYPE (s->generic.flags), s->generic.y + s->generic.parent->y, txtflags, UISCALE_TYPE (s->generic.flags), 129, colorWhite);

	// right
	CG_DrawChar (RCOLUMN_OFFSET + s->generic.x + i*UISIZE_TYPE (s->generic.flags) + s->generic.parent->x + UISIZE_TYPE (s->generic.flags), s->generic.y + s->generic.parent->y, txtflags, UISCALE_TYPE (s->generic.flags), 130, colorWhite);

	// slider bar
	CG_DrawChar ((UISIZE_TYPE (s->generic.flags) + RCOLUMN_OFFSET + s->generic.parent->x + s->generic.x + (SLIDER_RANGE-1)*UISIZE_TYPE (s->generic.flags) * s->range), s->generic.y + s->generic.parent->y, txtflags, UISCALE_TYPE (s->generic.flags), 131, colorWhite);
}

static void SpinControl_Draw (uiList_t *s)
{
	int		txtflags = 0;
	char	buffer[100];

	if (!s)
		return;

	ItemTxtFlags (s, txtflags);

	if (s->generic.name) {
		CG_DrawString (s->generic.x + s->generic.parent->x + LCOLUMN_OFFSET - ((Q_ColorCharCount (s->generic.name, (int)strlen (s->generic.name)) - 1)*UISIZE_TYPE (s->generic.flags)),
					s->generic.y + s->generic.parent->y, txtflags, UISCALE_TYPE (s->generic.flags), s->generic.name, colorGreen);
	}

	if (!s->itemNames || !s->itemNames[s->curValue])
		return;

	if (!strchr (s->itemNames[s->curValue], '\n')) {
		CG_DrawString (RCOLUMN_OFFSET + s->generic.x + s->generic.parent->x,
					s->generic.y + s->generic.parent->y, txtflags, UISCALE_TYPE (s->generic.flags), s->itemNames[s->curValue], colorWhite);
	}
	else {
		Q_strncpyz (buffer, s->itemNames[s->curValue], sizeof (buffer));
		*strchr (buffer, '\n') = 0;
		CG_DrawString (RCOLUMN_OFFSET + s->generic.x + s->generic.parent->x,
					s->generic.y + s->generic.parent->y, txtflags, UISCALE_TYPE (s->generic.flags), buffer, colorWhite);

		Q_strncpyz (buffer, strchr (s->itemNames[s->curValue], '\n') + 1, sizeof (buffer));
		CG_DrawString (RCOLUMN_OFFSET + s->generic.x + s->generic.parent->x,
					s->generic.y + s->generic.parent->y + UISIZEINC_TYPE(s->generic.flags), txtflags, UISCALE_TYPE (s->generic.flags), buffer, colorWhite);
	}
}

static void Selection_Draw (uiCommon_t *i)
{
	if (i->flags & UIF_NOSELBAR)
		return;
	else {
		vec4_t bgColor;
		Vector4Set (bgColor, colorDkGrey[0], colorDkGrey[1], colorDkGrey[2], 0.33f);

		if ((i->flags & UIF_FORCESELBAR) || ((uiState.cursorItem && uiState.cursorItem == i) || (uiState.selectedItem && uiState.selectedItem == i)))
			cgi.R_DrawFill (i->topLeft[0], i->topLeft[1], i->botRight[0] - i->topLeft[0], i->botRight[1] - i->topLeft[1], bgColor);
	}
}

void UI_DrawInterface (uiFrameWork_t *fw)
{
	int			i;
	uiCommon_t	*item;
	uiCommon_t	*selitem;

	// Center the height
	if (fw->flags & FWF_CENTERHEIGHT)
		UI_CenterHeight (fw);

	// Setup collision bounds
	UI_SetupBounds (fw);

	// Make sure we're on a valid item
	UI_AdjustCursor (fw, 1);

	// Draw the items
	for (i=0 ; i<fw->numItems ; i++) {
		Selection_Draw ((uiCommon_t *) fw->items[i]);

		switch (((uiCommon_t *) fw->items[i])->type) {
		case UITYPE_ACTION:
			Action_Draw ((uiAction_t *) fw->items[i]);
			break;

		case UITYPE_FIELD:
			Field_Draw ((uiField_t *) fw->items[i]);
			break;

		case UITYPE_IMAGE:
			Image_Draw ((uiImage_t *) fw->items[i]);
			break;

		case UITYPE_SLIDER:
			Slider_Draw ((uiSlider_t *) fw->items[i]);
			break;

		case UITYPE_SPINCONTROL:
			SpinControl_Draw ((uiList_t *) fw->items[i]);
			break;
		}
	}

	//
	// Blinking cursor
	//

	item = UI_ItemAtCursor (fw);
	if (item && item->cursorDraw)
		item->cursorDraw (item);
	else if (fw->cursorDraw)
		fw->cursorDraw (fw);
	else if (item && (item->type != UITYPE_FIELD) && (item->type != UITYPE_IMAGE) && (!(item->flags & UIF_NOSELECT))) {
		if (item->flags & UIF_LEFT_JUSTIFY)
			CG_DrawChar (fw->x + item->x + LCOLUMN_OFFSET + item->cursorOffset, fw->y + item->y, 0, UISCALE_TYPE (item->flags), 12 + ((int)(cg.realTime/250) & 1), colorWhite);
		else if (item->flags & UIF_CENTERED)
			CG_DrawChar (fw->x + item->x - (((Q_ColorCharCount (item->name, (int)strlen (item->name)) + 4) * UISIZE_TYPE (item->flags))*0.5) + item->cursorOffset, fw->y + item->y, 0, UISCALE_TYPE (item->flags), 12 + ((int)(cg.realTime/250) & 1), colorWhite);
		else
			CG_DrawChar (fw->x + item->x + item->cursorOffset, fw->y + item->y, 0, UISCALE_TYPE (item->flags), 12 + ((int)(cg.realTime/250) & 1), colorWhite);
	}

	selitem = (uiCommon_t *) uiState.selectedItem;
	if (selitem && (selitem->type != UITYPE_FIELD) && (selitem->type != UITYPE_IMAGE) && (!(selitem->flags & UIF_NOSELECT))) {
		if (selitem->flags & UIF_LEFT_JUSTIFY)
			CG_DrawChar (fw->x + selitem->x + LCOLUMN_OFFSET + selitem->cursorOffset, fw->y + selitem->y, 0, UISCALE_TYPE (selitem->flags), 12 + ((int)(cg.realTime/250) & 1), colorWhite);
		else if (selitem->flags & UIF_CENTERED)
			CG_DrawChar (fw->x + selitem->x - (((Q_ColorCharCount (selitem->name, (int)strlen (selitem->name)) + 4) * UISIZE_TYPE (selitem->flags))*0.5) + selitem->cursorOffset, fw->y + selitem->y, 0, UISCALE_TYPE (selitem->flags), 12 + ((int)(cg.realTime/250) & 1), colorWhite);
		else
			CG_DrawChar (fw->x + selitem->x + selitem->cursorOffset, fw->y + selitem->y, 0, UISCALE_TYPE (selitem->flags), 12 + ((int)(cg.realTime/250) & 1), colorWhite);
	}

	//
	// Statusbar
	//
	if (item) {
		if (item->statusBarFunc)
			item->statusBarFunc ((void *) item);
		else if (item->statusBar)
			UI_DrawStatusBar (item->statusBar);
		else
			UI_DrawStatusBar (fw->statusBar);
	}
	else
		UI_DrawStatusBar (fw->statusBar);
}


/*
=================
UI_Refresh
=================
*/
void UI_Refresh (qBool fullScreen)
{
	vec4_t	fillColor;

	if (!cg.menuOpen || !uiState.drawFunc)
		return;

	// Draw the backdrop
	if (fullScreen) {
		cgi.R_DrawPic (uiMedia.bgBig, 0, 0, 0, cg.glConfig.vidWidth, cg.glConfig.vidHeight, 0, 0, 1, 1, colorWhite);
	}
	else {
		Vector4Set (fillColor, colorBlack[0], colorBlack[1], colorBlack[2], 0.8f);
		cgi.R_DrawFill (0, 0, cg.glConfig.vidWidth, cg.glConfig.vidHeight, fillColor);
	}

	// Draw the menu
	uiState.drawFunc ();

	// Draw the cursor
	UI_DrawMouseCursor ();

	// Let the menu do it's updates
	M_Refresh ();
}
