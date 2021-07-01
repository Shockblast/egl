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
// ui_api.c
//

#include "ui_local.h"

uiImport_t	uii;

/*
=============================================================================

	UI IMPORT FUNCTIONS

=============================================================================
*/

/*
=============
UI_DrawChar
=============
*/
void UI_DrawChar (float x, float y, int flags, float scale, int num, vec4_t color)
{
	float		frow, fcol;
	float		ftWidth, ftHeight;
	vec4_t		shdColor;
	struct shader_s		*shader;

	if (flags & FS_SHADOW) {
		VectorCopy (colorBlack, shdColor);
		shdColor[3] = color[3];
	}

	if (uiMedia.uiCharsShader)
		shader = uiMedia.uiCharsShader;
	else
		shader = uiMedia.conCharsShader;

	ftWidth = 8.0 * scale;
	ftHeight = 8.0 * scale;

	if ((flags & FS_SECONDARY) && (num < 128))
		num |= 128;
	num &= 255;

	if ((num&127) == 32)
		return;		// space

	frow = (num>>4) * (1.0f/16.0f);
	fcol = (num&15) * (1.0f/16.0f);

	if (flags & FS_SHADOW)
		UI_DrawPic (shader, x+FT_SHAOFFSET, y+FT_SHAOFFSET, ftWidth, ftHeight, fcol, frow, fcol+(1.0f/16.0f), frow+(1.0f/16.0f), shdColor);

	UI_DrawPic (shader, x, y, ftWidth, ftHeight, fcol, frow, fcol+(1.0f/16.0f), frow+(1.0f/16.0f), color);
}


/*
=============
UI_DrawString
=============
*/
int UI_DrawString (float x, float y, int flags, float scale, char *string, vec4_t color)
{
	int			num, i;
	float		frow, fcol;
	float		ftWidth, ftHeight;
	vec4_t		shdColor, strColor;
	qBool		isShadowed;
	qBool		skipNext = qFalse;
	qBool		inColorCode = qFalse;
	struct shader_s		*shader;

	Vector4Copy (color, strColor);

	if (flags & FS_SHADOW)
		isShadowed = qTrue;
	else
		isShadowed = qFalse;

	VectorCopy (colorBlack, shdColor);
	shdColor[3] = strColor[3];

	if (uiMedia.uiCharsShader)
		shader = uiMedia.uiCharsShader;
	else
		shader = uiMedia.conCharsShader;

	ftWidth = 8.0 * scale;
	ftHeight = 8.0 * scale;

	for (i=0 ; *string ; ) {
		if ((flags & FS_SECONDARY) && (*string < 128))
			*string |= 128;

		if (skipNext)
			skipNext = qFalse;
		else if (Q_IsColorString (string)) {
			switch (string[1] & 127) {
			case Q_COLOR_ESCAPE:
				string++;
				skipNext = qTrue;
				continue;

			case 'r':
			case 'R':
				isShadowed = qFalse;
				inColorCode = qFalse;
				VectorCopy (colorWhite, strColor);
				string += 2;
				continue;

			case 's':
			case 'S':
				isShadowed = !isShadowed;
				string += 2;
				continue;

			case COLOR_BLACK:
			case COLOR_RED:
			case COLOR_GREEN:
			case COLOR_YELLOW:
			case COLOR_BLUE:
			case COLOR_CYAN:
			case COLOR_MAGENTA:
			case COLOR_WHITE:
			case COLOR_GREY:
				VectorCopy (strColorTable[Q_StrColorIndex (string[1])], strColor);
				inColorCode = qTrue;

			default:
				string += 2;
				continue;
			}
		}

		num = *string++;
		if (inColorCode)
			num &= 127;
		else
			num &= 255;

		// skip spaces
		if ((num&127) != 32) {
			frow = (num>>4) * (1.0f/16.0f);
			fcol = (num&15) * (1.0f/16.0f);

			if (isShadowed)
				UI_DrawPic (shader, x+FT_SHAOFFSET, y+FT_SHAOFFSET, ftWidth, ftHeight, fcol, frow, fcol+(1.0f/16.0f), frow+(1.0f/16.0f), shdColor);

			UI_DrawPic (shader, x, y, ftWidth, ftHeight, fcol, frow, fcol+(1.0f/16.0f), frow+(1.0f/16.0f), strColor);
		}

		x += ftWidth;
		i++;
	}

	return i;
}


/*
=============
UI_DrawPic
=============
*/
void UI_DrawPic (struct shader_s *shader, float x, float y, int w, int h, float s1, float t1, float s2, float t2, vec4_t color)
{
	uii.R_DrawPic (shader, x, y, w, h, s1, t1, s2, t2, color);
}


/*
=============
UI_DrawFill
=============
*/
void UI_DrawFill (float x, float y, int w, int h, vec4_t color)
{
	uii.R_DrawFill (x, y, w, h, color);
}

/*
=============================================================================

	UI EXPORT API

=============================================================================
*/

/*
===============
GetUIAPI
===============
*/
uiExport_t *GetUIAPI (uiImport_t *uiimp)
{
	static uiExport_t	uie;

	uii = *uiimp;

	uie.apiVersion			= UI_APIVERSION;

	uie.Init				= UI_Init;
	uie.Shutdown			= UI_Shutdown;
	uie.DrawConnectScreen	= UI_DrawConnectScreen;
	uie.UpdateDownloadInfo	= UI_UpdateDownloadInfo;
	uie.Refresh				= UI_Refresh;

	uie.MainMenu			= UI_MainMenu_f;
	uie.ForceMenuOff		= UI_ForceMenuOff;
	uie.MoveMouse			= UI_MoveMouse;
	uie.Keydown				= UI_Keydown;
	uie.ParseServerInfo		= UI_ParseServerInfo;
	uie.ParseServerStatus	= UI_ParseServerStatus;

	uie.RegisterMedia		= UI_InitMedia;
	uie.RegisterSounds		= UI_InitSoundMedia;

	return &uie;
}
