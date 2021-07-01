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
static void __fastcall HUD_DrawString (char *string, float x, float y, float centerwidth, qBool bold)
{
	float	margin;
	char	line[1024];
	int		width;
	vec4_t	color;

	Vector4Set (color, colorWhite[0], colorWhite[1], colorWhite[2], scr_hudalpha->value);
	margin = x;

	while (*string) {
		// Scan out one line of text from the string
		width = 0;
		while (*string && (*string != '\n'))
			line[width++] = *string++;
		line[width] = 0;

		if (centerwidth)
			x = margin + (centerwidth - (width * HUD_FTSIZE)) * 0.5;
		else
			x = margin;

		CG_DrawStringLen (x, y, bold ? FS_SECONDARY : 0, HUD_SCALE, line, width, color);

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
static void __fastcall HUD_DrawField (float x, float y, int altclr, int width, int value)
{
	char	num[16], *ptr;
	int		l, frame;
	vec4_t	color;

	VectorCopy (colorWhite, color);
	color[3] = scr_hudalpha->value;
	if (width < 1)
		return;

	// Draw number string
	if (width > 5)
		width = 5;

	Q_snprintfz (num, sizeof (num), "%i", value);
	l = (int)strlen (num);
	if (l > width)
		l = width;
	x += 2 * HUD_SCALE + (ICON_WIDTH * HUD_SCALE) * (width - l);

	ptr = num;
	while (*ptr && l) {
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		cgi.R_DrawPic (
			cgMedia.hudNumShaders[altclr % 2][frame % 11], 0, x, y,
			16 * HUD_SCALE,
			24 * HUD_SCALE,
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
static void CG_DrawHUDModel (int x, int y, int w, int h, struct model_s *model, struct shader_s *shader, float yawSpeed)
{
	vec3_t	mins, maxs;
	vec3_t	origin, angles;

	// Get model bounds
	cgi.R_ModelBounds (model, mins, maxs);

	// Try to fill the the window with the model
	origin[0] = 0.5 * (maxs[2] - mins[2]) * (1.0 / 0.179);
	origin[1] = 0.5 * (mins[1] + maxs[1]);
	origin[2] = -0.5 * (mins[2] + maxs[2]);
	VectorSet (angles, 0, AngleModf (yawSpeed * (cg.refreshTime & 2047) * (360.0 / 2048.0)), 0);

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
	parse_t			*ps;

	VectorCopy (colorWhite, color);
	color[3] = scr_hudalpha->value;

	if (!layout[0])
		return;

	x = 0;
	y = 0;
	width = 3;

	ps = Com_BeginParseSession (&layout);
	for ( ; ; ) {
		token = Com_Parse (ps);
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
					int		w, h;
					cgi.R_GetImageSize (cgMedia.hudFieldShader, &w, &h);
					cgi.R_DrawPic (
						cgMedia.hudFieldShader, 0, x, y,
						w*HUD_SCALE,
						h*HUD_SCALE,
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

				token = Com_Parse (ps);
				x = (cg.glConfig.vidWidth * 0.5) - (160.0 * HUD_SCALE) + atoi (token) * HUD_SCALE;
				token = Com_Parse (ps);
				y = (cg.glConfig.vidHeight * 0.5) - (120.0 * HUD_SCALE) + atoi (token) * HUD_SCALE;

				token = Com_Parse (ps);
				value = atoi (token);
				if (value >= cg.maxClients || value < 0)
					Com_Error (ERR_DROP, "client >= maxClients");
				ci = &cg.clientInfo[value];

				token = Com_Parse (ps);
				score = atoi (token);

				token = Com_Parse (ps);
				ping = atoi (token);

				token = Com_Parse (ps);
				time = atoi (token);

				CG_DrawString (x+(HUD_FTSIZE*4), y, FS_SECONDARY, HUD_SCALE, ci->name, color);
				CG_DrawString (x+(HUD_FTSIZE*4), y+(HUD_FTSIZE), 0, HUD_SCALE,  "Score: ", color);
				CG_DrawString (x+(HUD_FTSIZE*4) + (HUD_FTSIZE*7), y+(HUD_FTSIZE), FS_SECONDARY, HUD_SCALE, Q_VarArgs ("%i", score), color);
				CG_DrawString (x+(HUD_FTSIZE*4), y+(HUD_FTSIZE*2), 0, HUD_SCALE, Q_VarArgs ("Ping:  %i", ping), color);
				CG_DrawString (x+(HUD_FTSIZE*4), y+(HUD_FTSIZE*3), 0, HUD_SCALE, Q_VarArgs ("Time:  %i", time), color);

				if (!ci->icon)
					ci = &cg.baseClientInfo;

				if (ci->icon)
					shader = ci->icon;
				else
					shader = ci->icon = cgi.R_RegisterPic (ci->iconName);
				if (shader) {
					int		w, h;
					cgi.R_GetImageSize (shader, &w, &h);
					cgi.R_DrawPic (shader, 0, x, y, w*HUD_SCALE, h*HUD_SCALE, 0, 0, 1, 1, color);
				}
				continue;
			}

			// Draw a string
			if (!strcmp (token, "cstring")) {
				token = Com_Parse (ps);
				HUD_DrawString (token, x, y, 320 * HUD_SCALE, qFalse);
				continue;
			}

			// Draw a high-bit string
			if (!strcmp (token, "cstring2")) {
				token = Com_Parse (ps);
				HUD_DrawString (token, x, y, 320 * HUD_SCALE, qTrue);
				continue;
			}

			// Draw a ctf client block
			if (!strcmp (token, "ctf")) {
				int		score, ping;
				char	block[80];

				token = Com_Parse (ps);
				x = (cg.glConfig.vidWidth * 0.5);
				x -= (160.0 * HUD_SCALE);
				x += atof (token) * HUD_SCALE;

				token = Com_Parse (ps);
				y = (cg.glConfig.vidHeight * 0.5);
				y -= (120.0 * HUD_SCALE);
				y += atof (token) * HUD_SCALE;

				token = Com_Parse (ps);
				value = atoi (token);

				if (value >= cg.maxClients || value < 0)
					Com_Error (ERR_DROP, "client >= maxClients");
				ci = &cg.clientInfo[value];

				token = Com_Parse (ps);
				score = atoi (token);

				token = Com_Parse (ps);
				ping = clamp (atoi (token), 0, 999);

				Q_snprintfz (block, sizeof (block), "%3d %3d %-12.12s", score, ping, ci->name);
				CG_DrawString (x, y, (value == cg.playerNum) ? FS_SECONDARY : 0, HUD_SCALE, block, color);
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
					int		w, h;
					cgi.R_GetImageSize (cgMedia.hudFieldShader, &w, &h);
					cgi.R_DrawPic (
						cgMedia.hudFieldShader, 0, x, y,
						w*HUD_SCALE,
						h*HUD_SCALE,
						0, 0, 1, 1, color);
				}

				HUD_DrawField (x, y, altclr, width, value);
				continue;
			}
			break;

		case 'i':
			// Evaluate the expression
			if (!strcmp (token, "if")) {
				token = Com_Parse (ps);
				value = cg.frame.playerState.stats[atoi (token)];
				if (!value) {
					// Skip to endif
					while (token && token[0] && strcmp (token, "endif"))
						token = Com_Parse (ps);
				}

				continue;
			}
			break;

		case 'n':
			// Draw a HUD number
			if (!strcmp (token, "num")) {
				token = Com_Parse (ps);
				width = atoi (token);
				token = Com_Parse (ps);
				value = cg.frame.playerState.stats[atoi (token)];
				HUD_DrawField (x, y, 0, width, value);
				continue;
			}
			break;

		case 'p':
			// Draw a pic from a stat number
			if (!strcmp (token, "pic")) {
				int		w, h;

				token = Com_Parse (ps);
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
				cgi.R_DrawPic (shader, 0, x, y, w*HUD_SCALE, h*HUD_SCALE, 0, 0, 1, 1, color);
				continue;
			}

			// Draw a pic from a name
			if (!strcmp (token, "picn")) {
				token = Com_Parse (ps);

				shader = CG_RegisterPic (token);
				if (shader) {
					int		w, h;
					cgi.R_GetImageSize (shader, &w, &h);
					cgi.R_DrawPic (shader, 0, x, y, w*HUD_SCALE, h*HUD_SCALE, 0, 0, 1, 1, color);
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
					int		w, h;
					cgi.R_GetImageSize (cgMedia.hudFieldShader, &w, &h);
					cgi.R_DrawPic (
						cgMedia.hudFieldShader, 0, x, y,
						w*HUD_SCALE,
						h*HUD_SCALE,
						0, 0, 1, 1, color);
				}

				HUD_DrawField (x, y, 0, width, value);
				continue;
			}
			break;

		case 's':
			// Draw a string in the config strings
			if (!strcmp (token, "stat_string")) {
				token = Com_Parse (ps);
				index = atoi (token);
				if (index < 0 || index >= MAX_STATS)
					Com_Error (ERR_DROP, "Bad stat_string stat index");

				index = cg.frame.playerState.stats[index];
				if (index < 0 || index >= MAX_CFGSTRINGS)
					Com_Error (ERR_DROP, "Bad stat_string config string index");

				CG_DrawString (x, y, 0, HUD_SCALE, cg.configStrings[index], color);
				continue;
			}

			// Draw a string
			if (!strcmp (token, "string")) {
				token = Com_Parse (ps);
				CG_DrawString (x, y, 0, HUD_SCALE, token, color);
				continue;
			}

			// Draw a string in high-bit form
			if (!strcmp (token, "string2")) {
				token = Com_Parse (ps);
				CG_DrawString (x, y, FS_SECONDARY, HUD_SCALE, token, color);
				continue;
			}
			break;

		case 'x':
			// X position
			if (!strcmp (token, "xl")) {
				token = Com_Parse (ps);
				x = atoi (token) * HUD_SCALE;
				continue;
			}

			if (!strcmp (token, "xr")) {
				token = Com_Parse (ps);
				x = cg.glConfig.vidWidth + atoi (token) * HUD_SCALE;
				continue;
			}

			if (!strcmp (token, "xv")) {
				token = Com_Parse (ps);
				x = (cg.glConfig.vidWidth * 0.5) - (160.0 * HUD_SCALE) + (atoi (token) * HUD_SCALE);
				continue;
			}
			break;

		case 'y':
			// Y position
			if (!strcmp (token, "yt")) {
				token = Com_Parse (ps);
				y = atoi (token) * HUD_SCALE;
				continue;
			}

			if (!strcmp (token, "yb")) {
				token = Com_Parse (ps);
				y = cg.glConfig.vidHeight + atoi (token) * HUD_SCALE;
				continue;
			}

			if (!strcmp (token, "yv")) {
				token = Com_Parse (ps);
				y = (cg.glConfig.vidHeight * 0.5) - (120.0 * HUD_SCALE) + (atoi (token) * HUD_SCALE);
				continue;
			}
			break;
		}
	}
	Com_EndParseSession (ps);
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
