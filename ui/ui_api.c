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

uiImport_t	uii;

/*
=============================================================================

	UI IMPORT FUNCTIONS

=============================================================================
*/

void UI_DrawChar (float x, float y, int flags, float scale, int num, vec4_t color) {
	uii.Draw_Char (uiMedia.uiCharsImage, x, y, flags, scale, num, color);
}

int UI_DrawString (float x, float y, int flags, float scale, char *string, vec4_t color) {
	return uii.Draw_String (uiMedia.uiCharsImage, x, y, flags, scale, string, color);
}

void UI_DrawStretchPic (char *pic, float x, float y, int w, int h, vec4_t color) {
	uii.Draw_StretchPic (pic, x, y, w, h, color);
}

void UI_DrawPic (struct image_s *image, float x, float y, int w, int h, float s1, float t1, float s2, float t2, vec4_t color) {
	uii.Draw_Pic (image, x, y, w, h, s1, t1, s2, t2, color);
}

void UI_DrawFill (float x, float y, int w, int h, vec4_t color) {
	uii.Draw_Fill (x, y, w, h, color);
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
uiExport_t *GetUIAPI (uiImport_t *uiimp) {
	static uiExport_t	uie;

	uii = *uiimp;

	uie.apiVersion		= UI_APIVERSION;

	uie.Init			= UI_Init;
	uie.Shutdown		= UI_Shutdown;
	uie.Refresh			= UI_Refresh;

	uie.MainMenu		= UI_MainMenu_f;
	uie.ForceMenuOff	= UI_ForceMenuOff;
	uie.MoveMouse		= UI_MoveMouse;
	uie.Keydown			= UI_Keydown;
	uie.AddToServerList	= UI_AddToServerList;
	uie.DrawStatusBar	= UI_DrawStatusBar;

	uie.RegisterSounds	= UI_InitSoundMedia;

	return &uie;
}
