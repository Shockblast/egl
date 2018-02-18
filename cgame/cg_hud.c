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
=============================================================================

	SUPPORTING FUNCTIONS

=============================================================================
*/

#define STAT_MINUS	10	// num frame for '-' stats digit

#define ICON_WIDTH	16
#define ICON_HEIGHT	24

/*
==============
HUD_DrawString
==============
*/
static void HUD_DrawString (char *string, float x, float y, float centerwidth, qBool bold)
{
	float	margin;
	char	line[1024];
	int		width;
	vec4_t	color;
	vec2_t	charSize = { 0, 0 };

	Vec4Set (color, Q_colorWhite[0], Q_colorWhite[1], Q_colorWhite[2], scr_hudalpha->floatVal);
	margin = x;

	while (*string) {
		// Scan out one line of text from the string
		width = 0;
		while (*string && (*string != '\n'))
			line[width++] = *string++;
		line[width] = 0;

		if (centerwidth) {
			cgi.R_GetFontDimensions (NULL, cg.hudScale[0], cg.hudScale[1], bold ? FS_SECONDARY|FS_SQUARE : FS_SQUARE, charSize);
			x = margin + (centerwidth - (width * charSize[0])) * 0.5;
		}
		else
			x = margin;

		cgi.R_DrawStringLen (NULL, x, y, cg.hudScale[0], cg.hudScale[1], bold ? FS_SECONDARY|FS_SQUARE : FS_SQUARE, line, width, color);

		if (*string) {
			string++;	// skip the \n
			x = margin;
			y += charSize[1];
		}
	}
}


/*
==============
HUD_DrawField
==============
*/
static void HUD_DrawField (float x, float y, int altclr, int width, int value)
{
	char	num[16], *ptr;
	int		l, frame;
	vec4_t	color;

	Vec3Copy (Q_colorWhite, color);
	color[3] = scr_hudalpha->floatVal;
	if (width < 1)
		return;

	// Draw number string
	if (width > 5)
		width = 5;

	Q_snprintfz (num, sizeof (num), "%i", value);
	l = (int)strlen (num);
	if (l > width)
		l = width;
	x += 2 * cg.hudScale[0] + (ICON_WIDTH * cg.hudScale[1]) * (width - l);

	ptr = num;
	while (*ptr && l) {
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		cgi.R_DrawPic (
			cgMedia.hudNumShaders[altclr % 2][frame % 11], 0, x, y,
			ICON_WIDTH * cg.hudScale[0],
			ICON_HEIGHT * cg.hudScale[1],
			0, 0, 1, 1, color);

		x += ICON_WIDTH * cg.hudScale[0];
		ptr++;
		l--;
	}
}


/*
================
CG_DrawHUDModel
================
*/
static void CG_DrawHUDModel (int x, int y, int w, int h, struct refModel_s *model, struct shader_s *shader, float yawSpeed)
{
	vec3_t	mins, maxs;
	vec3_t	origin, angles;

	// Get model bounds
	cgi.R_ModelBounds (model, mins, maxs);

	// Try to fill the the window with the model
	origin[0] = 0.5 * (maxs[2] - mins[2]) * (1.0 / 0.179);
	origin[1] = 0.5 * (mins[1] + maxs[1]);
	origin[2] = -0.5 * (mins[2] + maxs[2]);
	Vec3Set (angles, 0, AngleModf (yawSpeed * (cg.refreshTime & 2047) * (360.0 / 2048.0)), 0);

	CG_DrawModel (x, y, w, h, model, shader, origin, angles);
}

/*
=============================================================================

	RENDERING

=============================================================================
*/

/*
================
HUD_ExecuteLayoutString
================
*/
static void HUD_ExecuteLayoutString (char *layout)
{
	float			x, y;
	int				value, width, index;
	char			*token;
	clientInfo_t	*ci;
	struct shader_s	*shader;
	vec4_t			color;
	vec2_t			charSize;
	int				w, h;

	Vec3Copy (Q_colorWhite, color);
	color[3] = scr_hudalpha->floatVal;

	if (!layout[0])
		return;

	cgi.R_GetFontDimensions (NULL, cg.hudScale[0], cg.hudScale[1], FS_SQUARE, charSize);

	x = 0;
	y = 0;
	width = 3;

	for ( ; layout ; ) {
		token = Com_Parse (&layout);
		if (!token[0])
			break;

		switch (token[0]) {
		case 'a':
			// Ammo number
			if (!strcmp (token, "anum")) {
				int		altclr;

				width = 3;
				value = cg.frame.playerState.stats[STAT_AMMO];
				if (value > 5)
					altclr = 0;	// Green
				else if (value >= 0)
					altclr = (cg.frame.serverFrame>>2) & 1;		// Flash
				else
					continue;	// Negative number = don't show

				if (cg.frame.playerState.stats[STAT_FLASHES] & 4) {
					cgi.R_GetImageSize (cgMedia.hudFieldShader, &w, &h);
					cgi.R_DrawPic (
						cgMedia.hudFieldShader, 0, x, y,
						w * cg.hudScale[0],
						h * cg.hudScale[1],
						0, 0, 1, 1, color);
				}

				HUD_DrawField (x, y, altclr, width, value);
				continue;
			}
			break;

		case 'c':
			// Draw a deathmatch client block
			if (!strcmp (token, "client")) {
				int		score, ping, time;

				token = Com_Parse (&layout);
				x = (cg.refConfig.vidWidth * 0.5f) - (160.0f * cg.hudScale[0]) + atoi (token) * cg.hudScale[0];
				token = Com_Parse (&layout);
				y = (cg.refConfig.vidHeight * 0.5f) - (120.0f * cg.hudScale[1]) + atoi (token) * cg.hudScale[1];

				token = Com_Parse (&layout);
				value = atoi (token);
				if (value >= cg.maxClients || value < 0)
					Com_Error (ERR_DROP, "client >= maxClients");
				ci = &cg.clientInfo[value];

				token = Com_Parse (&layout);
				score = atoi (token);

				token = Com_Parse (&layout);
				ping = atoi (token);

				token = Com_Parse (&layout);
				time = atoi (token);

				// Icon
				if (!ci->icon)
					ci->icon = cgi.R_RegisterPic (ci->iconName);
				if (!ci->icon)
					ci->icon = cg.baseClientInfo.icon;
				if (ci->icon) {
					cgi.R_GetImageSize (ci->icon, &w, &h);
					cgi.R_DrawPic (ci->icon, 0, x, y, w * cg.hudScale[0], h * cg.hudScale[1], 0, 0, 1, 1, color);
				}
				else {
					w = 32;
					h = 32;
				}

				// Block
				cgi.R_DrawString (NULL, x+(charSize[0]*4), y, cg.hudScale[0], cg.hudScale[1], FS_SECONDARY|FS_SQUARE, ci->name, color);
				cgi.R_DrawString (NULL, x+(charSize[0]*4), y+charSize[1], cg.hudScale[0], cg.hudScale[1], FS_SQUARE, "Score: ", color);
				cgi.R_DrawString (NULL, x+(charSize[0]*4) + (charSize[0]*7), y+charSize[1], cg.hudScale[0], cg.hudScale[1], FS_SECONDARY|FS_SQUARE, Q_VarArgs ("%i", score), color);
				cgi.R_DrawString (NULL, x+(charSize[0]*4), y+(charSize[1]*2), cg.hudScale[0], cg.hudScale[1], FS_SQUARE, Q_VarArgs ("Ping:  %i", ping), color);
				cgi.R_DrawString (NULL, x+(charSize[0]*4), y+(charSize[1]*3), cg.hudScale[0], cg.hudScale[1], FS_SQUARE, Q_VarArgs ("Time:  %i", time), color);
				continue;
			}

			// Draw a string
			if (!strcmp (token, "cstring")) {
				token = Com_Parse (&layout);
				HUD_DrawString (token, x, y, 320.0f * cg.hudScale[0], qFalse);
				continue;
			}

			// Draw a high-bit string
			if (!strcmp (token, "cstring2")) {
				token = Com_Parse (&layout);
				HUD_DrawString (token, x, y, 320.0f * cg.hudScale[0], qTrue);
				continue;
			}

			// Draw a ctf client block
			if (!strcmp (token, "ctf")) {
				int		score, ping;
				char	block[80];

				token = Com_Parse (&layout);
				x = (cg.refConfig.vidWidth * 0.5f);
				x -= (160.0f * cg.hudScale[0]);
				x += atof (token) * cg.hudScale[0];

				token = Com_Parse (&layout);
				y = (cg.refConfig.vidHeight * 0.5f);
				y -= (120.0f * cg.hudScale[1]);
				y += atof (token) * cg.hudScale[1];

				token = Com_Parse (&layout);
				value = atoi (token);

				if (value >= cg.maxClients || value < 0)
					Com_Error (ERR_DROP, "client >= maxClients");
				ci = &cg.clientInfo[value];

				token = Com_Parse (&layout);
				score = atoi (token);

				token = Com_Parse (&layout);
				ping = clamp (atoi (token), 0, 999);

				Q_snprintfz (block, sizeof (block), "%3d %3d %-12.12s", score, ping, ci->name);
				cgi.R_DrawString (NULL, x, y, cg.hudScale[0], cg.hudScale[1], (value == cg.playerNum) ? FS_SECONDARY|FS_SQUARE : FS_SQUARE, block, color);
				continue;
			}
			break;

		case 'h':
			// Health number
			if (!strcmp (token, "hnum")) {
				int		altclr;

				width = 3;
				value = cg.frame.playerState.stats[STAT_HEALTH];
				if (value > 25)
					altclr = 0;	// Green
				else if (value > 0)
					altclr = (cg.frame.serverFrame>>2) & 1;	// Flash
				else
					altclr = 1;

				if (cg.frame.playerState.stats[STAT_FLASHES] & 1) {
					cgi.R_GetImageSize (cgMedia.hudFieldShader, &w, &h);
					cgi.R_DrawPic (
						cgMedia.hudFieldShader, 0, x, y,
						w * cg.hudScale[0],
						h * cg.hudScale[1],
						0, 0, 1, 1, color);
				}

				HUD_DrawField (x, y, altclr, width, value);
				continue;
			}
			break;

		case 'i':
			// Evaluate the expression
			if (!strcmp (token, "if")) {
				token = Com_Parse (&layout);
				value = cg.frame.playerState.stats[atoi (token)];
				if (!value) {
					// Skip to endif
					while (token && token[0] && strcmp (token, "endif"))
						token = Com_Parse (&layout);
				}

				continue;
			}
			break;

		case 'n':
			// Draw a HUD number
			if (!strcmp (token, "num")) {
				token = Com_Parse (&layout);
				width = atoi (token);

				token = Com_Parse (&layout);
				value = cg.frame.playerState.stats[atoi(token)];

				HUD_DrawField (x, y, 0, width, value);
				continue;
			}
			break;

		case 'p':
			// Draw a pic from a stat number
			if (!strcmp (token, "pic")) {
				token = Com_Parse (&layout);
				value = cg.frame.playerState.stats[atoi (token)];
				if (value >= MAX_CS_IMAGES)
					Com_Error (ERR_DROP, "Pic >= MAX_CS_IMAGES");

				if (cg.imageCfgStrings[value])
					shader = cg.imageCfgStrings[value];
				else if (cg.configStrings[CS_IMAGES+value] && cg.configStrings[CS_IMAGES+value][0])
					shader = CG_RegisterPic (cg.configStrings[CS_IMAGES+value]);
				else
					shader = cgMedia.noTexture;

				cgi.R_GetImageSize (shader, &w, &h);
				cgi.R_DrawPic (shader, 0, x, y, w * cg.hudScale[0], h * cg.hudScale[1], 0, 0, 1, 1, color);
				continue;
			}

			// Draw a pic from a name
			if (!strcmp (token, "picn")) {
				token = Com_Parse (&layout);

				shader = CG_RegisterPic (token);
				if (shader) {
					cgi.R_GetImageSize (shader, &w, &h);
					cgi.R_DrawPic (shader, 0, x, y, w * cg.hudScale[0], h * cg.hudScale[1], 0, 0, 1, 1, color);
				}
				continue;
			}
			break;

		case 'r':
			// Armor number
			if (!strcmp (token, "rnum")) {
				width = 3;
				value = cg.frame.playerState.stats[STAT_ARMOR];
				if (value < 1)
					continue;

				if (cg.frame.playerState.stats[STAT_FLASHES] & 2) {
					cgi.R_GetImageSize (cgMedia.hudFieldShader, &w, &h);
					cgi.R_DrawPic (
						cgMedia.hudFieldShader, 0, x, y,
						w * cg.hudScale[0],
						h * cg.hudScale[1],
						0, 0, 1, 1, color);
				}

				HUD_DrawField (x, y, 0, width, value);
				continue;
			}
			break;

		case 's':
			// Draw a string in the config strings
			if (!strcmp (token, "stat_string")) {
				token = Com_Parse (&layout);
				index = atoi (token);
				if (index < 0 || index >= MAX_STATS)
					Com_Error (ERR_DROP, "Bad stat_string stat index");

				index = cg.frame.playerState.stats[index];
				if (index < 0 || index >= MAX_CFGSTRINGS)
					Com_Error (ERR_DROP, "Bad stat_string config string index");

				cgi.R_DrawString (NULL, x, y, cg.hudScale[0], cg.hudScale[1], FS_SQUARE, cg.configStrings[index], color);
				continue;
			}

			// Draw a string
			if (!strcmp (token, "string")) {
				token = Com_Parse (&layout);
				cgi.R_DrawString (NULL, x, y, cg.hudScale[0], cg.hudScale[1], FS_SQUARE, token, color);
				continue;
			}

			// Draw a string in high-bit form
			if (!strcmp (token, "string2")) {
				token = Com_Parse (&layout);
				cgi.R_DrawString (NULL, x, y, cg.hudScale[0], cg.hudScale[1], FS_SECONDARY|FS_SQUARE, token, color);
				continue;
			}
			break;

		case 'x':
			// X position
			if (!strcmp (token, "xl")) {
				token = Com_Parse (&layout);
				x = atoi (token) * cg.hudScale[0];
				continue;
			}

			if (!strcmp (token, "xr")) {
				token = Com_Parse (&layout);
				x = cg.refConfig.vidWidth + atoi (token) * cg.hudScale[0];
				continue;
			}

			if (!strcmp (token, "xv")) {
				token = Com_Parse (&layout);
				x = (cg.refConfig.vidWidth * 0.5f) - (160.0f * cg.hudScale[0]) + (atoi (token) * cg.hudScale[0]);
				continue;
			}
			break;

		case 'y':
			// Y position
			if (!strcmp (token, "yt")) {
				token = Com_Parse (&layout);
				y = atoi (token) * cg.hudScale[1];
				continue;
			}

			if (!strcmp (token, "yb")) {
				token = Com_Parse (&layout);
				y = cg.refConfig.vidHeight + atoi (token) * cg.hudScale[1];
				continue;
			}

			if (!strcmp (token, "yv")) {
				token = Com_Parse (&layout);
				y = (cg.refConfig.vidHeight * 0.5f) - (120.0f * cg.hudScale[1]) + (atoi (token) * cg.hudScale[1]);
				continue;
			}
			break;
		}
	}
}

/*
=============================================================================

	HANDLING

=============================================================================
*/

/*
================
HUD_CopyLayout
================
*/
void HUD_CopyLayout (void)
{
	Q_strncpyz (cg.layout, cgi.MSG_ReadString (), sizeof (cg.layout));
}


/*
================
HUD_DrawStatusBar

The status bar is a small layout program that is based on the stats array
================
*/
void HUD_DrawStatusBar (void)
{
	HUD_ExecuteLayoutString (cg.configStrings[CS_STATUSBAR]);
}


/*
================
HUD_DrawLayout
================
*/
void HUD_DrawLayout (void)
{
	if (cg.frame.playerState.stats[STAT_LAYOUTS] & 1)
		HUD_ExecuteLayoutString (cg.layout);
}
