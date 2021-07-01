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

// cl_hud.c

#include "cl_local.h"

/*
=======================================================================

	INVENTORY

=======================================================================
*/

/*
================
HUD_ParseInventory
================
*/
void HUD_ParseInventory (void) {
	int		i;

	for (i=0 ; i<MAX_ITEMS ; i++)
		cl.inventory[i] = MSG_ReadShort (&net_Message);
}


/*
================
HUD_DrawInventory
================
*/
#define	DISPLAY_ITEMS	17
void HUD_DrawInventory (void) {
	int		i, j;
	int		num, selected_num, item;
	int		index[MAX_ITEMS];
	char	string[1024];
	char	binding[1024];
	char	*bind;
	float	x, y;
	int		selected;
	int		top;
	vec4_t	color = { colorWhite[0], colorWhite[1], colorWhite[2], scr_hudalpha->value };
	vec4_t	selBarColor = { colorDkGrey[0], colorDkGrey[1], colorDkGrey[2], scr_hudalpha->value * 0.66 };

	selected = cl.frame.playerState.stats[STAT_SELECTED_ITEM];

	num = 0;
	selected_num = 0;
	for (i=0 ; i<MAX_ITEMS ; i++) {
		if (i==selected)
			selected_num = num;
		if (cl.inventory[i]) {
			index[num] = i;
			num++;
		}
	}

	// determine scroll point
	top = selected_num - (DISPLAY_ITEMS * 0.5);
	if (num - top < DISPLAY_ITEMS)
		top = num - DISPLAY_ITEMS;
	if (top < 0)
		top = 0;

	x = (vidDef.width - (256 * HUD_SCALE)) * 0.5;
	y = (vidDef.height - (240 * HUD_SCALE)) * 0.5;

	// draw backsplash image
	Draw_Pic (clMedia.hudInventoryImage, x, y + HUD_FTSIZE,
			clMedia.hudInventoryImage->width * HUD_SCALE,
			clMedia.hudInventoryImage->height * HUD_SCALE,
			0, 0, 1, 1, color);

	y += 24*HUD_SCALE;
	x += 24*HUD_SCALE;

	// head of list
	Draw_String (x, y, 0, HUD_SCALE, S_STYLE_SHADOW "hotkey ### item", color);
	Draw_String (x, y+HUD_FTSIZE, 0, HUD_SCALE, S_STYLE_SHADOW "------ --- ----", color);

	// list
	y += HUD_FTSIZE*2;
	for (i=top ; i<num && i<top+DISPLAY_ITEMS ; i++) {
		item = index[i];
		// search for a binding
		Com_sprintf (binding, sizeof (binding), "use %s", cl.configStrings[CS_ITEMS+item]);
		bind = "";
		for (j=0 ; j<256 ; j++) {
			if (key_Bindings[j] && !Q_stricmp (key_Bindings[j], binding)) {
				bind = Key_KeynumToString (j);
				break;
			}
		}

		Com_sprintf (string, sizeof (string), S_STYLE_SHADOW "%6s %3i %s", bind, cl.inventory[item], cl.configStrings[CS_ITEMS+item]);
		if (item == selected) {
			Draw_Fill (x, y, 26*HUD_FTSIZE, HUD_FTSIZE, selBarColor);

			// draw blinky cursors by the selected item
			if ((cl.frame.serverFrame>>2) & 1) {
				Draw_Char (x-HUD_FTSIZE, y, FS_SHADOW, HUD_SCALE, 15, colorRed);
				Draw_Char (x+(26*HUD_FTSIZE), y, FS_SHADOW, HUD_SCALE, 15, colorRed);
			}
		}
		Draw_StringLen (x, y, (item == selected) ? 0 : FS_SECONDARY, HUD_SCALE, string, 26, color);
		y += HUD_FTSIZE;
	}
}

/*
=======================================================================

	HUD

=======================================================================
*/

#define STAT_MINUS	10	// num frame for '-' stats digit

#define	ICON_WIDTH	16
#define	ICON_HEIGHT	24

/*
==============
HUD_DrawString
==============
*/
void FASTCALL HUD_DrawString (char *string, float x, float y, float centerwidth, qBool bold) {
	float	margin;
	char	line[1024];
	int		width;
	vec4_t	color = { colorWhite[0], colorWhite[1], colorWhite[2], scr_hudalpha->value };

	margin = x;

	while (*string) {
		// scan out one line of text from the string
		width = 0;
		while (*string && (*string != '\n'))
			line[width++] = *string++;
		line[width] = 0;

		if (centerwidth)
			x = margin + (centerwidth - (width * HUD_FTSIZE)) * 0.5;
		else
			x = margin;

		Draw_StringLen (x, y, bold ? FS_SECONDARY : 0, HUD_SCALE, line, width, color);

		if (*string) {
			string++;	// skip the \n
			x = margin;
			y += HUD_FTSIZE;
		}
	}
}


/*
==============
HUD_DrawField
==============
*/
void FASTCALL HUD_DrawField (float x, float y, int altclr, int width, int value) {
	char	num[16], *ptr;
	int		l, frame;
	vec4_t	color = { colorWhite[0], colorWhite[1], colorWhite[2], scr_hudalpha->value };

	if (width < 1)
		return;

	// draw number string
	if (width > 5)
		width = 5;

	Com_sprintf (num, sizeof (num), "%i", value);
	l = strlen (num);
	if (l > width)
		l = width;
	x += 2 * HUD_SCALE + (ICON_WIDTH * HUD_SCALE) * (width - l);

	ptr = num;
	while (*ptr && l) {
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		Draw_Pic (clMedia.hudNumImages[altclr % 2][frame % 11], x, y,
					clMedia.hudNumImages[altclr % 2][frame % 11]->width * HUD_SCALE,
					clMedia.hudNumImages[altclr % 2][frame % 11]->height * HUD_SCALE,
					0, 0, 1, 1, color);

		x += ICON_WIDTH*HUD_SCALE;
		ptr++;
		l--;
	}
}


/*
================
HUD_ExecuteLayoutString 
================
*/
void HUD_ExecuteLayoutString (char *s) {
	float			x = 0, y = 0;
	int				value, width = 3, index;
	int				picwidth, picheight;
	char			*token;
	clientInfo_t	*ci;
	vec4_t			color = { colorWhite[0], colorWhite[1], colorWhite[2], scr_hudalpha->value };

	if (!CL_ConnectionState (CA_ACTIVE) || !cl.refreshPrepped)
		return;

	if (!s[0])
		return;

	while (s) {
		token = Com_Parse (&s);
		if (!strcmp (token, "xl")) {
			token = Com_Parse (&s);
			x = atoi (token) * HUD_SCALE;
			continue;
		}

		if (!strcmp (token, "xr")) {
			token = Com_Parse (&s);
			x = vidDef.width + atoi (token) * HUD_SCALE;
			continue;
		}

		if (!strcmp (token, "xv")) {
			token = Com_Parse (&s);
			x = (vidDef.width * 0.5) - (160.0 * HUD_SCALE) + (atoi (token) * HUD_SCALE);
			continue;
		}

		if (!strcmp (token, "yt")) {
			token = Com_Parse (&s);
			y = atoi (token) * HUD_SCALE;
			continue;
		}

		if (!strcmp (token, "yb")) {
			token = Com_Parse (&s);
			y = vidDef.height + atoi (token) * HUD_SCALE;
			continue;
		}

		if (!strcmp (token, "yv")) {
			token = Com_Parse (&s);
			y = (vidDef.height * 0.5) - (120.0 * HUD_SCALE) + (atoi (token) * HUD_SCALE);
			continue;
		}

		if (!strcmp (token, "pic")) {
			// draw a pic from a stat number
			token = Com_Parse (&s);
			value = cl.frame.playerState.stats[atoi (token)];
			if (value >= MAX_IMAGES)
				Com_Error (ERR_DROP, "Pic >= MAX_IMAGES");

			if (cl.configStrings[CS_IMAGES+value]) {
				Img_GetPicSize (&picwidth, &picheight, cl.configStrings[CS_IMAGES+value]);
				Draw_StretchPic (cl.configStrings[CS_IMAGES+value], x, y, picwidth*HUD_SCALE, picheight*HUD_SCALE, color);
			}
			continue;
		}

		if (!strcmp (token, "client")) {
			// draw a deathmatch client block
			int		score, ping, time;

			token = Com_Parse (&s);
			x = (vidDef.width * 0.5) - (160.0 * HUD_SCALE) + atoi (token) * HUD_SCALE;
			token = Com_Parse (&s);
			y = (vidDef.height * 0.5) - (120.0 * HUD_SCALE) + atoi (token) * HUD_SCALE;

			token = Com_Parse (&s);
			value = atoi (token);
			if ((value >= MAX_CLIENTS) || (value < 0))
				Com_Error (ERR_DROP, "client >= MAX_CLIENTS");
			ci = &cl.clientInfo[value];

			token = Com_Parse (&s);
			score = atoi (token);

			token = Com_Parse (&s);
			ping = atoi (token);

			token = Com_Parse (&s);
			time = atoi (token);

			Draw_String (x+(HUD_FTSIZE*4), y, FS_SECONDARY, HUD_SCALE, ci->name, color);
			Draw_String (x+(HUD_FTSIZE*4), y+(HUD_FTSIZE), 0, HUD_SCALE,  "Score: ", color);
			Draw_String (x+(HUD_FTSIZE*4) + (HUD_FTSIZE*7), y+(HUD_FTSIZE), FS_SECONDARY, HUD_SCALE,  va("%i", score), color);
			Draw_String (x+(HUD_FTSIZE*4), y+(HUD_FTSIZE*2), 0, HUD_SCALE, va("Ping:  %i", ping), color);
			Draw_String (x+(HUD_FTSIZE*4), y+(HUD_FTSIZE*3), 0, HUD_SCALE, va("Time:  %i", time), color);

			if (!ci->icon)
				ci = &cl.baseClientInfo;

			Img_GetPicSize (&picwidth, &picheight, ci->iconname);
			Draw_StretchPic (ci->iconname, x, y, picwidth*HUD_SCALE, picheight*HUD_SCALE, color);
			continue;
		}

		if (!strcmp (token, "ctf")) {
			// draw a ctf client block
			int		score, ping;
			char	block[80];

			token = Com_Parse (&s);
			x = (vidDef.width * 0.5);
			x -= (160.0 * HUD_SCALE);
			x += atof (token) * HUD_SCALE;

			token = Com_Parse (&s);
			y = (vidDef.height * 0.5);
			y -= (120.0 * HUD_SCALE);
			y += atof (token) * HUD_SCALE;

			token = Com_Parse (&s);
			value = atoi (token);

			if ((value >= MAX_CLIENTS) || (value < 0))
				Com_Error (ERR_DROP, "client >= MAX_CLIENTS");

			ci = &cl.clientInfo[value];

			token = Com_Parse (&s);
			score = atoi (token);

			token = Com_Parse (&s);
			ping = clamp (atoi (token), 0, 999);

			sprintf (block, "%3d %3d %-12.12s", score, ping, ci->name);

			Draw_String (x, y, (value == cl.playerNum) ? FS_SECONDARY : 0, HUD_SCALE, block, color);

			continue;
		}

		if (!strcmp (token, "picn")) {
			// draw a pic from a name
			token = Com_Parse (&s);
			Img_GetPicSize (&picwidth, &picheight, token);
			Draw_StretchPic (token, x, y, picwidth*HUD_SCALE, picheight*HUD_SCALE, color);
			continue;
		}

		if (!strcmp (token, "num")) {
			// draw a number
			token = Com_Parse (&s);
			width = atoi (token);
			token = Com_Parse (&s);
			value = cl.frame.playerState.stats[atoi (token)];
			HUD_DrawField (x, y, 0, width, value);
			continue;
		}

		if (!strcmp (token, "hnum")) {
			// health number
			int		altclr;

			width = 3;
			value = cl.frame.playerState.stats[STAT_HEALTH];
			if (value > 25)
				altclr = 0;	// green
			else if (value > 0)
				altclr = (cl.frame.serverFrame>>2) & 1;	// flash
			else
				altclr = 1;

			if (cl.frame.playerState.stats[STAT_FLASHES] & 1) {
				Draw_Pic (clMedia.hudFieldImage, x, y,
					clMedia.hudFieldImage->width*HUD_SCALE,
					clMedia.hudFieldImage->height*HUD_SCALE,
					0, 0, 1, 1, color);
			}

			HUD_DrawField (x, y, altclr, width, value);
			continue;
		}

		if (!strcmp (token, "anum")) {
			// ammo number
			int		altclr;

			width = 3;
			value = cl.frame.playerState.stats[STAT_AMMO];
			if (value > 5)
				altclr = 0;	// green
			else if (value >= 0)
				altclr = (cl.frame.serverFrame>>2) & 1;		// flash
			else
				continue;	// negative number = don't show

			if (cl.frame.playerState.stats[STAT_FLASHES] & 4) {
				Draw_Pic (clMedia.hudFieldImage, x, y,
					clMedia.hudFieldImage->width*HUD_SCALE,
					clMedia.hudFieldImage->height*HUD_SCALE,
					0, 0, 1, 1, color);
			}

			HUD_DrawField (x, y, altclr, width, value);
			continue;
		}

		if (!strcmp (token, "rnum")) {
			// armor number
			width = 3;
			value = cl.frame.playerState.stats[STAT_ARMOR];
			if (value < 1)
				continue;

			if (cl.frame.playerState.stats[STAT_FLASHES] & 2) {
				Draw_Pic (clMedia.hudFieldImage, x, y,
					clMedia.hudFieldImage->width*HUD_SCALE,
					clMedia.hudFieldImage->height*HUD_SCALE,
					0, 0, 1, 1, color);
			}

			HUD_DrawField (x, y, 0, width, value);
			continue;
		}


		if (!strcmp (token, "stat_string")) {
			token = Com_Parse (&s);
			index = atoi (token);
			if ((index < 0) || (index >= MAX_CONFIGSTRINGS))
				Com_Error (ERR_DROP, "Bad stat_string index");

			index = cl.frame.playerState.stats[index];
			if ((index < 0) || (index >= MAX_CONFIGSTRINGS))
				Com_Error (ERR_DROP, "Bad stat_string index");

			Draw_String (x, y, 0, HUD_SCALE, cl.configStrings[index], color);
			continue;
		}

		if (!strcmp (token, "cstring")) {
			token = Com_Parse (&s);
			HUD_DrawString (token, x, y, 320 * HUD_SCALE, qFalse);
			continue;
		}

		if (!strcmp (token, "string")) {
			token = Com_Parse (&s);
			Draw_String (x, y, 0, HUD_SCALE, token, color);
			continue;
		}

		if (!strcmp (token, "cstring2")) {
			token = Com_Parse (&s);
			HUD_DrawString (token, x, y, 320 * HUD_SCALE, qTrue);
			continue;
		}

		if (!strcmp (token, "string2")) {
			token = Com_Parse (&s);
			Draw_String (x, y, FS_SECONDARY, HUD_SCALE, token, color);
			continue;
		}

		if (!strcmp (token, "if")) {
			// draw a number
			token = Com_Parse (&s);
			value = cl.frame.playerState.stats[atoi (token)];
			if (!value) {
				// skip to endif
				while (s && strcmp (token, "endif"))
					token = Com_Parse (&s);
			}

			continue;
		}
	}
}


/*
================
HUD_DrawLayout
================
*/
void HUD_DrawLayout (void) {
	// The status bar is a small layout program that is based on the stats array
	HUD_ExecuteLayoutString (cl.configStrings[CS_STATUSBAR]);

	if (cl.frame.playerState.stats[STAT_LAYOUTS] & 1)
		HUD_ExecuteLayoutString (cl.layout);

	if (cl.frame.playerState.stats[STAT_LAYOUTS] & 2)
		HUD_DrawInventory ();
}
