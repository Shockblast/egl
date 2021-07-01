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

	VIDEO MENU

=======================================================================
*/

typedef struct m_videomenu_s {
	uiFrameWork_t	framework;

	uiImage_t		banner;

	uiAction_t		vidsettings_menu;
	uiAction_t		glexts_menu;

	uiAction_t		back_action;
} m_videomenu_t;

m_videomenu_t	m_video_menu;

static void GLExts_Menu (void *unused) {
	UI_GLExtsMenu_f ();
}

static void VIDSettings_Menu (void *unused) {
	UI_VIDSettingsMenu_f ();
}


/*
=============
VideoMenu_Init
=============
*/
void VideoMenu_Init (void) {
	float	y;

	m_video_menu.framework.x		= uis.vidWidth*0.5;
	m_video_menu.framework.y		= 0;
	m_video_menu.framework.numItems	= 0;

	UI_Banner (&m_video_menu.banner, uiMedia.videoBanner, &y);

	m_video_menu.vidsettings_menu.generic.type		= UITYPE_ACTION;
	m_video_menu.vidsettings_menu.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_video_menu.vidsettings_menu.generic.x			= 0;
	m_video_menu.vidsettings_menu.generic.y			= y += UIFT_SIZEINC;
	m_video_menu.vidsettings_menu.generic.name		= "Video Settings";
	m_video_menu.vidsettings_menu.generic.callBack	= VIDSettings_Menu;
	m_video_menu.vidsettings_menu.generic.statusBar	= "Opens the Video Settings menu";

	m_video_menu.glexts_menu.generic.type		= UITYPE_ACTION;
	m_video_menu.glexts_menu.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_video_menu.glexts_menu.generic.x			= 0;
	m_video_menu.glexts_menu.generic.y			= y += UIFT_SIZEINCLG;
	m_video_menu.glexts_menu.generic.name		= "OpenGL Extensions";
	m_video_menu.glexts_menu.generic.callBack	= GLExts_Menu;
	m_video_menu.glexts_menu.generic.statusBar	= "Opens the GL Extensions menu";

	m_video_menu.back_action.generic.type		= UITYPE_ACTION;
	m_video_menu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_video_menu.back_action.generic.x			= 0;
	m_video_menu.back_action.generic.y			= y += (UIFT_SIZEINCLG*2);
	m_video_menu.back_action.generic.name		= "< Back";
	m_video_menu.back_action.generic.callBack	= Back_Menu;
	m_video_menu.back_action.generic.statusBar	= "Back a menu";

	UI_AddItem (&m_video_menu.framework,		&m_video_menu.banner);

	UI_AddItem (&m_video_menu.framework,		&m_video_menu.vidsettings_menu);
	UI_AddItem (&m_video_menu.framework,		&m_video_menu.glexts_menu);

	UI_AddItem (&m_video_menu.framework,		&m_video_menu.back_action);

	UI_Center (&m_video_menu.framework);
	UI_Setup (&m_video_menu.framework);
}


/*
=============
VideoMenu_Draw
=============
*/
void VideoMenu_Draw (void) {
	UI_Center (&m_video_menu.framework);
	UI_Setup (&m_video_menu.framework);

	UI_AdjustTextCursor (&m_video_menu.framework, 1);
	UI_Draw (&m_video_menu.framework);
}


/*
=============
VideoMenuKey
=============
*/
struct sfx_s *VideoMenu_Key (int key) {
	return DefaultMenu_Key (&m_video_menu.framework, key);
}


/*
=============
UI_VideoMenu_f
=============
*/
void UI_VideoMenu_f (void) {
	VideoMenu_Init ();
	UI_PushMenu (&m_video_menu.framework, VideoMenu_Draw, VideoMenu_Key);
}
