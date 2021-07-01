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
=============================================================================

	GAME MENU

=============================================================================
*/

typedef struct {
	uiFrameWork_t	framework;

	uiImage_t		banner;

	uiAction_t		easy_action;
	uiAction_t		medium_action;
	uiAction_t		hard_action;
	uiAction_t		nightmare_action;

	uiAction_t		load_action;
	uiAction_t		save_action;
	uiAction_t		credits_action;

	uiAction_t		back_action;
} m_gamemenu_t;

m_gamemenu_t	m_game_menu;

static void StartGame (void) {
	// disable updates and start the cinematic going
	uii.CL_ResetServerCount ();
	UI_ForceMenuOff ();
	uii.Cvar_SetValue ("deathmatch", 0);
	uii.Cvar_SetValue ("coop", 0);

	uii.Cvar_SetValue ("gamerules", 0);

	uii.Cbuf_AddText ("loading ; killserver ; wait ; newgame\n");
	uii.Key_SetKeyDest (KD_GAME);
}

static void EasyGameFunc (void *unused) {
	uii.Cvar_ForceSet ("skill", "0");
	StartGame ();
}

static void MediumGameFunc (void *unused) {
	uii.Cvar_ForceSet ("skill", "1");
	StartGame ();
}

static void HardGameFunc (void *unused) {
	uii.Cvar_ForceSet ("skill", "2");
	StartGame ();
}

static void HardPlusGameFunc (void *unused) {
	uii.Cvar_ForceSet ("skill", "3");
	StartGame ();
}

static void LoadGameFunc (void *unused) {
	UI_LoadGameMenu_f ();
}

static void SaveGameFunc (void *unused) {
	UI_SaveGameMenu_f ();
}

static void CreditsFunc (void *unused) {
	UI_CreditsMenu_f ();
}


/*
=============
Game_MenuInit
=============
*/
void Game_MenuInit (void) {
	float	y;

	m_game_menu.framework.x			= uis.vidWidth * 0.50;
	m_game_menu.framework.y			= 0;
	m_game_menu.framework.nitems	= 0;

	UI_Banner (&m_game_menu.banner, uiMedia.gameBanner, &y);

	m_game_menu.easy_action.generic.type		= UITYPE_ACTION;
	m_game_menu.easy_action.generic.flags		= UIF_LEFT_JUSTIFY|UIF_LARGE|UIF_SHADOW;
	m_game_menu.easy_action.generic.x			= 0;
	m_game_menu.easy_action.generic.y			= y += UIFT_SIZEINC;
	m_game_menu.easy_action.generic.name		= "Easy";
	m_game_menu.easy_action.generic.callback	= EasyGameFunc;

	m_game_menu.medium_action.generic.type		= UITYPE_ACTION;
	m_game_menu.medium_action.generic.flags		= UIF_LEFT_JUSTIFY|UIF_LARGE|UIF_SHADOW;
	m_game_menu.medium_action.generic.x			= 0;
	m_game_menu.medium_action.generic.y			= y += UIFT_SIZEINCLG;
	m_game_menu.medium_action.generic.name		= "Medium";
	m_game_menu.medium_action.generic.callback	= MediumGameFunc;

	m_game_menu.hard_action.generic.type		= UITYPE_ACTION;
	m_game_menu.hard_action.generic.flags		= UIF_LEFT_JUSTIFY|UIF_LARGE|UIF_SHADOW;
	m_game_menu.hard_action.generic.x			= 0;
	m_game_menu.hard_action.generic.y			= y += UIFT_SIZEINCLG;
	m_game_menu.hard_action.generic.name		= "Hard";
	m_game_menu.hard_action.generic.callback	= HardGameFunc;

	m_game_menu.nightmare_action.generic.type		= UITYPE_ACTION;
	m_game_menu.nightmare_action.generic.flags		= UIF_LEFT_JUSTIFY|UIF_LARGE|UIF_SHADOW;
	m_game_menu.nightmare_action.generic.x			= 0;
	m_game_menu.nightmare_action.generic.y			= y += UIFT_SIZEINCLG;
	m_game_menu.nightmare_action.generic.name		= S_COLOR_RED "NIGHTMARE!";
	m_game_menu.nightmare_action.generic.callback	= HardPlusGameFunc;

	m_game_menu.load_action.generic.type		= UITYPE_ACTION;
	m_game_menu.load_action.generic.flags		= UIF_LEFT_JUSTIFY|UIF_LARGE|UIF_SHADOW;
	m_game_menu.load_action.generic.x			= 0;
	m_game_menu.load_action.generic.y			= y += (UIFT_SIZEINCLG*2);
	m_game_menu.load_action.generic.name		= "Load game";
	m_game_menu.load_action.generic.callback	= LoadGameFunc;

	m_game_menu.save_action.generic.type		= UITYPE_ACTION;
	m_game_menu.save_action.generic.flags		= UIF_LEFT_JUSTIFY|UIF_LARGE|UIF_SHADOW;
	m_game_menu.save_action.generic.x			= 0;
	m_game_menu.save_action.generic.y			= y += UIFT_SIZEINCLG;
	m_game_menu.save_action.generic.name		= "Save game";
	m_game_menu.save_action.generic.callback	= SaveGameFunc;

	m_game_menu.credits_action.generic.type		= UITYPE_ACTION;
	m_game_menu.credits_action.generic.flags	= UIF_LEFT_JUSTIFY|UIF_LARGE|UIF_SHADOW;
	m_game_menu.credits_action.generic.x		= 0;
	m_game_menu.credits_action.generic.y		= y += UIFT_SIZEINCLG;
	m_game_menu.credits_action.generic.name		= "View credits";
	m_game_menu.credits_action.generic.callback	= CreditsFunc;

	m_game_menu.back_action.generic.type		= UITYPE_ACTION;
	m_game_menu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_game_menu.back_action.generic.x			= 0;
	m_game_menu.back_action.generic.y			= y += (UIFT_SIZEINCLG*2);
	m_game_menu.back_action.generic.name		= "< Back";
	m_game_menu.back_action.generic.callback	= Back_Menu;
	m_game_menu.back_action.generic.statusbar	= "Back a menu";

	UI_AddItem (&m_game_menu.framework,		&m_game_menu.banner);

	UI_AddItem (&m_game_menu.framework,		&m_game_menu.easy_action);
	UI_AddItem (&m_game_menu.framework,		&m_game_menu.medium_action);
	UI_AddItem (&m_game_menu.framework,		&m_game_menu.hard_action);
	UI_AddItem (&m_game_menu.framework,		&m_game_menu.nightmare_action);

	UI_AddItem (&m_game_menu.framework,		&m_game_menu.load_action);
	UI_AddItem (&m_game_menu.framework,		&m_game_menu.save_action);
	UI_AddItem (&m_game_menu.framework,		&m_game_menu.credits_action);

	UI_AddItem (&m_game_menu.framework,		&m_game_menu.back_action);

	UI_Center (&m_game_menu.framework);
	UI_Setup (&m_game_menu.framework);
}


/*
=============
Game_MenuDraw
=============
*/
void Game_MenuDraw (void) {
	UI_Center (&m_game_menu.framework);
	UI_Setup (&m_game_menu.framework);

	UI_AdjustTextCursor (&m_game_menu.framework, 1);
	UI_Draw (&m_game_menu.framework);
}


/*
=============
Game_MenuKey
=============
*/
const char *Game_MenuKey (int key) {
	return DefaultMenu_Key (&m_game_menu.framework, key);
}


/*
=============
UI_GameMenu_f
=============
*/
void UI_GameMenu_f (void) {
	Game_MenuInit();
	UI_PushMenu (&m_game_menu.framework, Game_MenuDraw, Game_MenuKey);
}
