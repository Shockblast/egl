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

	MULTIPLAYER MENU

=============================================================================
*/

typedef struct m_multiplayermenu_s {
	uiFrameWork_t	framework;

	uiImage_t		banner;

	uiAction_t		joinservers_menu;
	uiAction_t		startserver_menu;
	uiAction_t		plyrcfg_menu;
	uiAction_t		dlopts_menu;

	uiAction_t		back_action;
} m_multiplayermenu_t;

m_multiplayermenu_t	m_multiplayer_menu;

static void JoinServer_Menu (void *unused) {
	UI_JoinServerMenu_f ();
}

static void StartServer_Menu (void *unused) {
	UI_StartServerMenu_f ();
}

static void PlayerSetup_Menu (void *unused) {
	UI_PlayerConfigMenu_f ();
}

static void DLOpts_Menu (void *unused) {
	UI_DLOptionsMenu_f ();
}


/*
=============
MultiplayerMenu_Init
=============
*/
void MultiplayerMenu_Init (void) {
	float	y;

	m_multiplayer_menu.framework.x			= uis.vidWidth * 0.5;
	m_multiplayer_menu.framework.y			= 0;
	m_multiplayer_menu.framework.numItems	= 0;

	UI_Banner (&m_multiplayer_menu.banner, uiMedia.banners.multiplayer, &y);

	m_multiplayer_menu.joinservers_menu.generic.type		= UITYPE_ACTION;
	m_multiplayer_menu.joinservers_menu.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_multiplayer_menu.joinservers_menu.generic.x			= 0;
	m_multiplayer_menu.joinservers_menu.generic.y			= y += UIFT_SIZEINC;
	m_multiplayer_menu.joinservers_menu.generic.name		= "Join Network Server";
	m_multiplayer_menu.joinservers_menu.generic.callBack	= JoinServer_Menu;

	m_multiplayer_menu.startserver_menu.generic.type		= UITYPE_ACTION;
	m_multiplayer_menu.startserver_menu.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_multiplayer_menu.startserver_menu.generic.x			= 0;
	m_multiplayer_menu.startserver_menu.generic.y			= y += UIFT_SIZEINCLG;
	m_multiplayer_menu.startserver_menu.generic.name		= "Start Network Server";
	m_multiplayer_menu.startserver_menu.generic.callBack	= StartServer_Menu;

	m_multiplayer_menu.plyrcfg_menu.generic.type		= UITYPE_ACTION;
	m_multiplayer_menu.plyrcfg_menu.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_multiplayer_menu.plyrcfg_menu.generic.x			= 0;
	m_multiplayer_menu.plyrcfg_menu.generic.y			= y += UIFT_SIZEINCLG;
	m_multiplayer_menu.plyrcfg_menu.generic.name		= "Player Configuration";
	m_multiplayer_menu.plyrcfg_menu.generic.callBack	= PlayerSetup_Menu;

	m_multiplayer_menu.dlopts_menu.generic.type		= UITYPE_ACTION;
	m_multiplayer_menu.dlopts_menu.generic.flags	= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_multiplayer_menu.dlopts_menu.generic.x		= 0;
	m_multiplayer_menu.dlopts_menu.generic.y		= y += UIFT_SIZEINCLG;
	m_multiplayer_menu.dlopts_menu.generic.name		= "Download Options";
	m_multiplayer_menu.dlopts_menu.generic.callBack	= DLOpts_Menu;

	m_multiplayer_menu.back_action.generic.type			= UITYPE_ACTION;
	m_multiplayer_menu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_multiplayer_menu.back_action.generic.x			= 0;
	m_multiplayer_menu.back_action.generic.y			= y += (UIFT_SIZEINCLG*2);
	m_multiplayer_menu.back_action.generic.name			= "< Back";
	m_multiplayer_menu.back_action.generic.callBack		= Menu_Pop;
	m_multiplayer_menu.back_action.generic.statusBar	= "Back a menu";

	UI_AddItem (&m_multiplayer_menu.framework,		&m_multiplayer_menu.banner);

	UI_AddItem (&m_multiplayer_menu.framework,		&m_multiplayer_menu.joinservers_menu);
	UI_AddItem (&m_multiplayer_menu.framework,		&m_multiplayer_menu.startserver_menu);
	UI_AddItem (&m_multiplayer_menu.framework,		&m_multiplayer_menu.plyrcfg_menu);
	UI_AddItem (&m_multiplayer_menu.framework,		&m_multiplayer_menu.dlopts_menu);

	UI_AddItem (&m_multiplayer_menu.framework,		&m_multiplayer_menu.back_action);

	UI_Center (&m_multiplayer_menu.framework);
	UI_Setup (&m_multiplayer_menu.framework);
}


/*
=============
MultiplayerMenu_Draw
=============
*/
static void MultiplayerMenu_Draw (void) {
	UI_Center (&m_multiplayer_menu.framework);
	UI_Setup (&m_multiplayer_menu.framework);

	UI_AdjustTextCursor (&m_multiplayer_menu.framework, 1);
	UI_Draw (&m_multiplayer_menu.framework);
}


/*
=============
MultiplayerMenu_Key
=============
*/
struct sfx_s *MultiplayerMenu_Key (int key) {
	return DefaultMenu_Key (&m_multiplayer_menu.framework, key);
}


/*
=============
UI_MultiplayerMenu_f
=============
*/
void UI_MultiplayerMenu_f (void) {
	MultiplayerMenu_Init ();
	UI_PushMenu (&m_multiplayer_menu.framework, MultiplayerMenu_Draw, MultiplayerMenu_Key);
}
