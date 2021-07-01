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

	MAIN MENU

=======================================================================
*/

typedef struct {
	uiFrameWork_t		framework;

	uiImage_t			game_menu;
	uiImage_t			multiplayer_menu;
	uiImage_t			options_menu;
	uiImage_t			video_menu;
	uiImage_t			quit_menu;

	uiImage_t			plaque_pic;
	uiImage_t			idlogo_pic;
} m_mainmenu_t;

m_mainmenu_t	m_main_menu;

static void SP_Menu (void *unused) {
	UI_GameMenu_f ();
}

static void MP_Menu (void *unused) {
	UI_MultiplayerMenu_f ();
}

static void OPTS_Menu (void *unused) {
	UI_OptionsMenu_f ();
}

static void VID_Menu (void *unused) {
	UI_VideoMenu_f ();
}

static void QUIT_Menu (void *unused) {
	UI_QuitMenu_f ();
}

#define MAIN_ITEMS			5
static void MainCursorDrawFunc (uiFrameWork_t *menu) {
	float	y;
	float	w, h;
	int		cursorNum;

	cursorNum = ((int)(uis.time / 100)) % MAINMENU_CURSOR_NUMFRAMES;

	y = (((m_main_menu.framework.cursor%MAIN_ITEMS) * (uiMedia.mainGame->height + 5)));

	w = uiMedia.mainCursors[cursorNum]->width * UIFT_SCALE;
	h = uiMedia.mainCursors[cursorNum]->height * UIFT_SCALE;
	UI_DrawPic (uiMedia.mainCursors[cursorNum],
				menu->x + LCOLUMN_OFFSET - w, menu->y + (y * UIFT_SCALE),
				w, h, 0, 0, 1, 1, colorWhite);
}


/*
=============
MainMenu_Init
=============
*/
void MainMenu_Init (void) {
	int		y = 0;
	int		w = 0, h = 0;
	int		cursor_width = 0;

	m_main_menu.framework.x				= uis.vidWidth * 0.5 - (40 * UIFT_SCALE);
	m_main_menu.framework.y				= 0;
	m_main_menu.framework.nitems		= 0;
	m_main_menu.framework.cursordraw	= MainCursorDrawFunc;

	m_main_menu.game_menu.generic.type		= UITYPE_IMAGE;
	m_main_menu.game_menu.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELBAR;
	m_main_menu.game_menu.generic.x			= 0;
	m_main_menu.game_menu.generic.y			= y;
	m_main_menu.game_menu.image				= uiMedia.mainGame;
	m_main_menu.game_menu.selImage			= uiMedia.mainGameSel;
	m_main_menu.game_menu.generic.callback	= SP_Menu;
	y += ((uiMedia.mainGame->height + 5) * UIFT_SCALE);

	m_main_menu.multiplayer_menu.generic.type		= UITYPE_IMAGE;
	m_main_menu.multiplayer_menu.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELBAR;
	m_main_menu.multiplayer_menu.generic.x			= 0;
	m_main_menu.multiplayer_menu.generic.y			= y;
	m_main_menu.multiplayer_menu.image				= uiMedia.mainMultiplayer;
	m_main_menu.multiplayer_menu.selImage			= uiMedia.mainMultiplayerSel;
	m_main_menu.multiplayer_menu.generic.callback	= MP_Menu;
	y += ((uiMedia.mainMultiplayer->height + 5) * UIFT_SCALE);

	m_main_menu.options_menu.generic.type		= UITYPE_IMAGE;
	m_main_menu.options_menu.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELBAR;
	m_main_menu.options_menu.generic.x			= 0;
	m_main_menu.options_menu.generic.y			= y;
	m_main_menu.options_menu.image				= uiMedia.mainOptions;
	m_main_menu.options_menu.selImage			= uiMedia.mainOptionsSel;
	m_main_menu.options_menu.generic.callback	= OPTS_Menu;
	y += ((uiMedia.mainOptions->height + 5) * UIFT_SCALE);

	m_main_menu.video_menu.generic.type		= UITYPE_IMAGE;
	m_main_menu.video_menu.generic.flags	= UIF_LEFT_JUSTIFY|UIF_NOSELBAR;
	m_main_menu.video_menu.generic.x		= 0;
	m_main_menu.video_menu.generic.y		= y;
	m_main_menu.video_menu.image			= uiMedia.mainVideo;
	m_main_menu.video_menu.selImage			= uiMedia.mainVideoSel;
	m_main_menu.video_menu.generic.callback	= VID_Menu;
	y += ((uiMedia.mainVideo->height + 5) * UIFT_SCALE);

	m_main_menu.quit_menu.generic.type		= UITYPE_IMAGE;
	m_main_menu.quit_menu.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELBAR;
	m_main_menu.quit_menu.generic.x			= 0;
	m_main_menu.quit_menu.generic.y			= y;
	m_main_menu.quit_menu.image				= uiMedia.mainQuit;
	m_main_menu.quit_menu.selImage			= uiMedia.mainQuitSel;
	m_main_menu.quit_menu.generic.callback	= QUIT_Menu;

	UI_ImgGetPicSize (&cursor_width, &h, "m_cursor0");
	m_main_menu.plaque_pic.generic.type		= UITYPE_IMAGE;
	m_main_menu.plaque_pic.generic.flags	= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_main_menu.plaque_pic.generic.x		= - ((cursor_width + uiMedia.mainPlaque->width) * UIFT_SCALE);
	m_main_menu.plaque_pic.generic.y		= y = - (15 * UIFT_SCALE);
	m_main_menu.plaque_pic.image			= uiMedia.mainPlaque;
	m_main_menu.plaque_pic.selImage			= NULL;
	y += uiMedia.mainPlaque->height * UIFT_SCALE;

	UI_ImgGetPicSize (&w, &h, "m_main_logo");
	m_main_menu.idlogo_pic.generic.type		= UITYPE_IMAGE;
	m_main_menu.idlogo_pic.generic.flags	= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_main_menu.idlogo_pic.generic.x		= - ((cursor_width + w) * UIFT_SCALE);
	m_main_menu.idlogo_pic.generic.y		= y + (5 * UIFT_SCALE);
	m_main_menu.idlogo_pic.image			= uiMedia.mainLogo;
	m_main_menu.idlogo_pic.selImage			= NULL;

	UI_AddItem (&m_main_menu.framework,		&m_main_menu.game_menu);
	UI_AddItem (&m_main_menu.framework,		&m_main_menu.multiplayer_menu);
	UI_AddItem (&m_main_menu.framework,		&m_main_menu.options_menu);
	UI_AddItem (&m_main_menu.framework,		&m_main_menu.video_menu);
	UI_AddItem (&m_main_menu.framework,		&m_main_menu.quit_menu);

	UI_AddItem (&m_main_menu.framework,		&m_main_menu.plaque_pic);
	UI_AddItem (&m_main_menu.framework,		&m_main_menu.idlogo_pic);

	UI_Center (&m_main_menu.framework);
	UI_Setup (&m_main_menu.framework);
}


/*
=============
MainMenu_Draw
=============
*/
void MainMenu_Draw (void) {
	UI_Center (&m_main_menu.framework);
	UI_Setup (&m_main_menu.framework);

	UI_AdjustTextCursor (&m_main_menu.framework, 1);
	UI_Draw (&m_main_menu.framework);
}


/*
=============
MainMenu_Key
=============
*/
const char *MainMenu_Key (int key) {
	return DefaultMenu_Key (&m_main_menu.framework, key);
}


/*
=============
UI_MainMenu_f
=============
*/
void UI_MainMenu_f (void) {
	MainMenu_Init ();
	UI_PushMenu (&m_main_menu.framework, MainMenu_Draw, MainMenu_Key);
}
