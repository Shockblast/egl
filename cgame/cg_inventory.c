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

	if (!(cg.frame.playerState.stats[STAT_LAYOUTS] & 2))
		return;

	Vector4Set (color, colorWhite[0], colorWhite[1], colorWhite[2], scr_hudalpha->value);
	Vector4Set (selBarColor, colorDkGrey[0], colorDkGrey[1], colorDkGrey[2], scr_hudalpha->value * 0.66);

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
	top = selected_num - (INV_DISPLAY_ITEMS * 0.5);
	if (num - top < INV_DISPLAY_ITEMS)
		top = num - INV_DISPLAY_ITEMS;
	if (top < 0)
		top = 0;

	x = (cg.glConfig.vidWidth - (256 * HUD_SCALE)) * 0.5;
	y = (cg.glConfig.vidHeight - (240 * HUD_SCALE)) * 0.5;

	// Draw backsplash image
	cgi.R_GetImageSize (cgMedia.hudInventoryShader, &w, &h);
	cgi.R_DrawPic (
		cgMedia.hudInventoryShader, 0, x, y + HUD_FTSIZE,
		w * HUD_SCALE, h * HUD_SCALE,
		0, 0, 1, 1, color);

	y += 24*HUD_SCALE;
	x += 24*HUD_SCALE;

	// Head of list
	CG_DrawString (x, y, 0, HUD_SCALE, S_STYLE_SHADOW "hotkey ### item", color);
	CG_DrawString (x, y+HUD_FTSIZE, 0, HUD_SCALE, S_STYLE_SHADOW "------ --- ----", color);

	// List
	y += HUD_FTSIZE*2;
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

		Q_snprintfz (string, sizeof (string), S_STYLE_SHADOW "%6s %3i %s", bind, cg.inventory[item], cg.configStrings[CS_ITEMS+item]);
		if (item == selected) {
			cgi.R_DrawFill (x, y, 26*HUD_FTSIZE, HUD_FTSIZE, selBarColor);

			// Draw blinky cursors by the selected item
			if ((cg.frame.serverFrame>>2) & 1) {
				CG_DrawChar (x-HUD_FTSIZE, y, FS_SHADOW, HUD_SCALE, 15, colorRed);
				CG_DrawChar (x+(26*HUD_FTSIZE), y, FS_SHADOW, HUD_SCALE, 15, colorRed);
			}
		}
		CG_DrawStringLen (x, y, (item == selected) ? 0 : FS_SECONDARY, HUD_SCALE, string, 26, color);
		y += HUD_FTSIZE;
	}
}
