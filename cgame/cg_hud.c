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
// cg_hud.c
//

#include "cg_local.h"

/*
=======================================================================

	SUPPORTING FUNCTIONS

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
static void FASTCALL HUD_DrawString (char *string, float x, float y, float centerwidth, qBool bold) {
	float	margin;
	char	line[1024];
	int		width;
	vec4_t	color;

	Vector4Set (color, colorWhite[0], colorWhite[1], colorWhite[2], scr_hudalpha->value);
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

		cgi.Draw_StringLen (NULL, x, y, bold ? FS_SECONDARY : 0, HUD_SCALE, line, width, color);

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
static void FASTCALL HUD_DrawField (float x, float y, int altclr, int width, int value) {
	char	num[16], *ptr;
	int		l, frame;
	int		imgWidth, imgHeight;
	vec4_t	color;

	VectorCopy (colorWhite, color);
	color[3] = scr_hudalpha->value;
	if (width < 1)
		return;

	// draw number string
	if (width > 5)
		width = 5;

	Q_snprintfz (num, sizeof (num), "%i", value);
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

		cgi.Img_GetSize (cgMedia.hudNumImages[altclr % 2][frame % 11], &imgWidth, &imgHeight);
		cgi.Draw_Pic (
			cgMedia.hudNumImages[altclr % 2][frame % 11], x, y,
			imgWidth * HUD_SCALE,
			imgHeight * HUD_SCALE,
			0, 0, 1, 1, color);

		x += ICON_WIDTH*HUD_SCALE;
		ptr++;
		l--;
	}
}


/*
================
CG_DrawHUDModel
================
*/
static void CG_DrawHUDModel (int x, int y, int w, int h, struct model_s *model, struct image_s *skin, float yawSpeed) {
	vec3_t	mins, maxs;
	vec3_t	origin, angles;

	// get model bounds
	cgi.Mod_ModelBounds (model, mins, maxs);

	// try to fill the the window with the model
	origin[0] = 0.5 * (maxs[2] - mins[2]) * (1.0 / 0.179);
	origin[1] = 0.5 * (mins[1] + maxs[1]);
	origin[2] = -0.5 * (mins[2] + maxs[2]);
	VectorSet (angles, 0, AngleMod (yawSpeed * (cg.time & 2047) * (360.0 / 2048.0)), 0);

	CG_DrawModel (x, y, w, h, model, skin, origin, angles);
}

/*
=======================================================================

	RENDERING

=======================================================================
*/

/*
================
HUD_ExecuteLayoutString
================
*/
static void HUD_ExecuteLayoutString (char *str) {
	float			x, y;
	int				value, width, index;
	char			*token;
	clientInfo_t	*ci;
	struct image_s	*image;
	vec4_t			color;

	VectorCopy (colorWhite, color);
	color[3] = scr_hudalpha->value;

	if (!str[0])
		return;

	x = 0;
	y = 0;
	width = 3;

	while (str) {
		token = Com_Parse (&str);

		if (!strcmp (token, "xl")) {
			token = Com_Parse (&str);
			x = atoi (token) * HUD_SCALE;
			continue;
		}

		if (!strcmp (token, "xr")) {
			token = Com_Parse (&str);
			x = cg.vidWidth + atoi (token) * HUD_SCALE;
			continue;
		}

		if (!strcmp (token, "xv")) {
			token = Com_Parse (&str);
			x = (cg.vidWidth * 0.5) - (160.0 * HUD_SCALE) + (atoi (token) * HUD_SCALE);
			continue;
		}

		if (!strcmp (token, "yt")) {
			token = Com_Parse (&str);
			y = atoi (token) * HUD_SCALE;
			continue;
		}

		if (!strcmp (token, "yb")) {
			token = Com_Parse (&str);
			y = cg.vidHeight + atoi (token) * HUD_SCALE;
			continue;
		}

		if (!strcmp (token, "yv")) {
			token = Com_Parse (&str);
			y = (cg.vidHeight * 0.5) - (120.0 * HUD_SCALE) + (atoi (token) * HUD_SCALE);
			continue;
		}

		if (!strcmp (token, "pic")) {
			// draw a pic from a stat number
			token = Com_Parse (&str);
			value = cg.frame.playerState.stats[atoi (token)];
			if (value >= MAX_CS_IMAGES)
				Com_Error (ERR_DROP, "Pic >= MAX_CS_IMAGES");

			if (cg.configStrings[CS_IMAGES+value] && cg.configStrings[CS_IMAGES+value][0]) {
				image = cgi.Img_RegisterPic (cg.configStrings[CS_IMAGES+value]);
				if (image) {
					int		w, h;
					cgi.Img_GetSize (image, &w, &h);
					cgi.Draw_Pic (image, x, y, w*HUD_SCALE, h*HUD_SCALE, 0, 0, 1, 1, color);
				}
			}
			continue;
		}

		if (!strcmp (token, "client")) {
			// draw a deathmatch client block
			int		score, ping, time;

			token = Com_Parse (&str);
			x = (cg.vidWidth * 0.5) - (160.0 * HUD_SCALE) + atoi (token) * HUD_SCALE;
			token = Com_Parse (&str);
			y = (cg.vidHeight * 0.5) - (120.0 * HUD_SCALE) + atoi (token) * HUD_SCALE;

			token = Com_Parse (&str);
			value = atoi (token);
			if ((value >= MAX_CS_CLIENTS) || (value < 0))
				Com_Error (ERR_DROP, "client >= MAX_CS_CLIENTS");
			ci = &cg.clientInfo[value];

			token = Com_Parse (&str);
			score = atoi (token);

			token = Com_Parse (&str);
			ping = atoi (token);

			token = Com_Parse (&str);
			time = atoi (token);

			cgi.Draw_String (NULL, x+(HUD_FTSIZE*4), y, FS_SECONDARY, HUD_SCALE, ci->name, color);
			cgi.Draw_String (NULL, x+(HUD_FTSIZE*4), y+(HUD_FTSIZE), 0, HUD_SCALE,  "Score: ", color);
			cgi.Draw_String (NULL, x+(HUD_FTSIZE*4) + (HUD_FTSIZE*7), y+(HUD_FTSIZE), FS_SECONDARY, HUD_SCALE, Q_VarArgs ("%i", score), color);
			cgi.Draw_String (NULL, x+(HUD_FTSIZE*4), y+(HUD_FTSIZE*2), 0, HUD_SCALE, Q_VarArgs ("Ping:  %i", ping), color);
			cgi.Draw_String (NULL, x+(HUD_FTSIZE*4), y+(HUD_FTSIZE*3), 0, HUD_SCALE, Q_VarArgs ("Time:  %i", time), color);

			if (!ci->icon)
				ci = &cg.baseClientInfo;

			image = cgi.Img_RegisterPic (ci->iconName);
			if (image) {
				int		w, h;
				cgi.Img_GetSize (image, &w, &h);
				cgi.Draw_Pic (image, x, y, w*HUD_SCALE, h*HUD_SCALE, 0, 0, 1, 1, color);
			}
			continue;
		}

		if (!strcmp (token, "ctf")) {
			// draw a ctf client block
			int		score, ping;
			char	block[80];

			token = Com_Parse (&str);
			x = (cg.vidWidth * 0.5);
			x -= (160.0 * HUD_SCALE);
			x += atof (token) * HUD_SCALE;

			token = Com_Parse (&str);
			y = (cg.vidHeight * 0.5);
			y -= (120.0 * HUD_SCALE);
			y += atof (token) * HUD_SCALE;

			token = Com_Parse (&str);
			value = atoi (token);

			if ((value >= MAX_CS_CLIENTS) || (value < 0))
				Com_Error (ERR_DROP, "client >= MAX_CS_CLIENTS");
			ci = &cg.clientInfo[value];

			token = Com_Parse (&str);
			score = atoi (token);

			token = Com_Parse (&str);
			ping = clamp (atoi (token), 0, 999);

			Q_snprintfz (block, sizeof (block), "%3d %3d %-12.12s", score, ping, ci->name);
			cgi.Draw_String (NULL, x, y, (value == cg.playerNum) ? FS_SECONDARY : 0, HUD_SCALE, block, color);
			continue;
		}

		if (!strcmp (token, "picn")) {
			// draw a pic from a name
			token = Com_Parse (&str);

			image = cgi.Img_RegisterPic (token);
			if (image) {
				int		w, h;
				cgi.Img_GetSize (image, &w, &h);
				cgi.Draw_Pic (image, x, y, w*HUD_SCALE, h*HUD_SCALE, 0, 0, 1, 1, color);
			}
			continue;
		}

		if (!strcmp (token, "num")) {
			// draw a number
			token = Com_Parse (&str);
			width = atoi (token);
			token = Com_Parse (&str);
			value = cg.frame.playerState.stats[atoi (token)];
			HUD_DrawField (x, y, 0, width, value);
			continue;
		}

		if (!strcmp (token, "hnum")) {
			// health number
			int		altclr;

			width = 3;
			value = cg.frame.playerState.stats[STAT_HEALTH];
			if (value > 25)
				altclr = 0;	// green
			else if (value > 0)
				altclr = (cg.frame.serverFrame>>2) & 1;	// flash
			else
				altclr = 1;

			if (cg.frame.playerState.stats[STAT_FLASHES] & 1) {
				int		w, h;
				cgi.Img_GetSize (cgMedia.hudFieldImage, &w, &h);
				cgi.Draw_Pic (
					cgMedia.hudFieldImage, x, y,
					w*HUD_SCALE,
					h*HUD_SCALE,
					0, 0, 1, 1, color);
			}

			HUD_DrawField (x, y, altclr, width, value);
			continue;
		}

		if (!strcmp (token, "anum")) {
			// ammo number
			int		altclr;

			width = 3;
			value = cg.frame.playerState.stats[STAT_AMMO];
			if (value > 5)
				altclr = 0;	// green
			else if (value >= 0)
				altclr = (cg.frame.serverFrame>>2) & 1;		// flash
			else
				continue;	// negative number = don't show

			if (cg.frame.playerState.stats[STAT_FLASHES] & 4) {
				int		w, h;
				cgi.Img_GetSize (cgMedia.hudFieldImage, &w, &h);
				cgi.Draw_Pic (
					cgMedia.hudFieldImage, x, y,
					w*HUD_SCALE,
					h*HUD_SCALE,
					0, 0, 1, 1, color);
			}

			HUD_DrawField (x, y, altclr, width, value);
			continue;
		}

		if (!strcmp (token, "rnum")) {
			// armor number
			width = 3;
			value = cg.frame.playerState.stats[STAT_ARMOR];
			if (value < 1)
				continue;

			if (cg.frame.playerState.stats[STAT_FLASHES] & 2) {
				int		w, h;
				cgi.Img_GetSize (cgMedia.hudFieldImage, &w, &h);
				cgi.Draw_Pic (
					cgMedia.hudFieldImage, x, y,
					w*HUD_SCALE,
					h*HUD_SCALE,
					0, 0, 1, 1, color);
			}

			HUD_DrawField (x, y, 0, width, value);
			continue;
		}

		if (!strcmp (token, "stat_string")) {
			token = Com_Parse (&str);
			index = atoi (token);
			if ((index < 0) || (index >= MAX_CONFIGSTRINGS))
				Com_Error (ERR_DROP, "Bad stat_string index");

			index = cg.frame.playerState.stats[index];
			if ((index < 0) || (index >= MAX_CONFIGSTRINGS))
				Com_Error (ERR_DROP, "Bad stat_string index");

			cgi.Draw_String (NULL, x, y, 0, HUD_SCALE, cg.configStrings[index], color);
			continue;
		}

		if (!strcmp (token, "cstring")) {
			token = Com_Parse (&str);
			HUD_DrawString (token, x, y, 320 * HUD_SCALE, qFalse);
			continue;
		}

		if (!strcmp (token, "string")) {
			token = Com_Parse (&str);
			cgi.Draw_String (NULL, x, y, 0, HUD_SCALE, token, color);
			continue;
		}

		if (!strcmp (token, "cstring2")) {
			token = Com_Parse (&str);
			HUD_DrawString (token, x, y, 320 * HUD_SCALE, qTrue);
			continue;
		}

		if (!strcmp (token, "string2")) {
			token = Com_Parse (&str);
			cgi.Draw_String (NULL, x, y, FS_SECONDARY, HUD_SCALE, token, color);
			continue;
		}

		if (!strcmp (token, "if")) {
			// draw a number
			token = Com_Parse (&str);
			value = cg.frame.playerState.stats[atoi (token)];
			if (!value) {
				// skip to endif
				while (str && strcmp (token, "endif"))
					token = Com_Parse (&str);
			}

			continue;
		}
	}
}

/*
=======================================================================

	HANDLING

=======================================================================
*/

/*
================
HUD_CopyLayout
================
*/
void HUD_CopyLayout (void) {
	Q_strncpyz (cg.layout, cgi.MSG_ReadString (), sizeof (cg.layout));
}


/*
================
HUD_DrawStatusBar

The status bar is a small layout program that is based on the stats array
================
*/
void HUD_DrawStatusBar (void) {
	HUD_ExecuteLayoutString (cg.configStrings[CS_STATUSBAR]);
}


/*
================
HUD_DrawLayout
================
*/
void HUD_DrawLayout (void) {
	if (cg.frame.playerState.stats[STAT_LAYOUTS] & 1)
		HUD_ExecuteLayoutString (cg.layout);
}
