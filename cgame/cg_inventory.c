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
// cg_inventory.c
//

#include "cg_local.h"

#define INV_DISPLAY_ITEMS	17

/*
=======================================================================

	INVENTORY

=======================================================================
*/

/*
================
Inv_ParseInventory
================
*/
void Inv_ParseInventory (void)
{
	int		i;

	for (i=0 ; i<MAX_CS_ITEMS ; i++)
		cg.inventory[i] = cgi.MSG_ReadShort ();
}


/*
================
Inv_DrawInventory

FIXME: INV_DISPLAY_ITEMS should be determined based on max height and character height.
================
*/
void Inv_DrawInventory (void)
{
	int		i, j;
	int		num, selected_num, item;
	int		index[MAX_CS_ITEMS];
	char	string[1024];
	char	binding[1024];
	char	*bind;
	float	x, y;
	int		w, h;
	int		selected;
	int		top;
	vec4_t	color;
	vec4_t	selBarColor;
	vec2_t	charSize;

	if (!(cg.frame.playerState.stats[STAT_LAYOUTS] & 2))
		return;

	Vec4Set (color, Q_colorWhite[0], Q_colorWhite[1], Q_colorWhite[2], scr_hudalpha->floatVal);
	Vec4Set (selBarColor, Q_colorDkGrey[0], Q_colorDkGrey[1], Q_colorDkGrey[2], scr_hudalpha->floatVal * 0.66);
	cgi.R_GetFontDimensions (NULL, cg.hudScale[0], cg.hudScale[1], FS_SHADOW|FS_SQUARE, charSize);

	selected = cg.frame.playerState.stats[STAT_SELECTED_ITEM];

	num = 0;
	selected_num = 0;
	for (i=0 ; i<MAX_CS_ITEMS ; i++) {
		if (i==selected)
			selected_num = num;
		if (cg.inventory[i]) {
			index[num] = i;
			num++;
		}
	}

	// Determine scroll point
	top = selected_num - (INV_DISPLAY_ITEMS * 0.5f);
	if (num - top < INV_DISPLAY_ITEMS)
		top = num - INV_DISPLAY_ITEMS;
	if (top < 0)
		top = 0;

	x = (cg.refConfig.vidWidth - (256 * cg.hudScale[0])) * 0.5f;
	y = (cg.refConfig.vidHeight - (240 * cg.hudScale[1])) * 0.5f;

	// Draw backsplash image
	cgi.R_GetImageSize (cgMedia.hudInventoryShader, &w, &h);
	cgi.R_DrawPic (
		cgMedia.hudInventoryShader, 0, x, y + charSize[1],
		w * cg.hudScale[0],
		h * cg.hudScale[1],
		0, 0, 1, 1, color);

	y += 24 * cg.hudScale[0];
	x += 24 * cg.hudScale[1];

	// Head of list
	cgi.R_DrawString (NULL, x, y, cg.hudScale[0], cg.hudScale[1], FS_SHADOW|FS_SQUARE, "hotkey ### item", color);
	cgi.R_DrawString (NULL, x, y+charSize[1], cg.hudScale[0], cg.hudScale[1], FS_SHADOW|FS_SQUARE, "------ --- ----", color);

	// List
	y += charSize[1] * 2;
	for (i=top ; i<num && i<top+INV_DISPLAY_ITEMS ; i++) {
		item = index[i];
		// Search for a binding
		Q_snprintfz (binding, sizeof (binding), "use %s", cg.configStrings[CS_ITEMS+item]);
		bind = "";
		for (j=0 ; j<256 ; j++) {
			if (cgi.Key_GetBindingBuf (j) && !Q_stricmp (cgi.Key_GetBindingBuf (j), binding)) {
				bind = cgi.Key_KeynumToString (j);
				break;
			}
		}

		Q_snprintfz (string, sizeof (string), "%6s %3i %s", bind, cg.inventory[item], cg.configStrings[CS_ITEMS+item]);
		if (item == selected) {
			CG_DrawFill (x, y, 26*charSize[0], charSize[1], selBarColor);

			// Draw blinky cursors by the selected item
			if ((cg.frame.serverFrame>>2) & 1) {
				cgi.R_DrawChar (NULL, x-charSize[0], y, cg.hudScale[0], cg.hudScale[1], FS_SHADOW|FS_SQUARE, 15, Q_colorRed);
				cgi.R_DrawChar (NULL, x+(26*charSize[0]), y, cg.hudScale[0], cg.hudScale[1], FS_SHADOW|FS_SQUARE, 15, Q_colorRed);
			}
		}
		cgi.R_DrawStringLen (NULL, x, y, cg.hudScale[0], cg.hudScale[1], (item == selected) ? FS_SHADOW|FS_SQUARE : FS_SHADOW|FS_SECONDARY|FS_SQUARE, string, 26, color);
		y += charSize[1];
	}
}
