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

	SAVEGAME MENU

=============================================================================
*/

typedef struct m_savegamemenu_s {
	uiFrameWork_t	framework;

	uiImage_t		banner;

	uiAction_t		actions[MAX_SAVEGAMES];
	uiAction_t		back_action;
} m_savegamemenu_t;

m_savegamemenu_t	m_savegame_menu;

void SaveGameCallback (void *self) {
	uiAction_t *a = (uiAction_t *) self;

	uii.Cbuf_AddText (Q_VarArgs ("save save%i\n", a->generic.localData[0]));
	UI_ForceMenuOff ();
}


/*
=============
SaveGameMenu_Init
=============
*/
void SaveGameMenu_Init (void) {
	int		i;
	float	y;

	m_savegame_menu.framework.x			= uis.vidWidth * 0.5;
	m_savegame_menu.framework.y			= 0;
	m_savegame_menu.framework.numItems	= 0;

	Create_Savestrings ();

	UI_Banner (&m_savegame_menu.banner, uiMedia.banners.saveGame, &y);
	UI_AddItem (&m_savegame_menu.framework,		&m_savegame_menu.banner);

	// don't include the autosave slot
	for (i=0 ; i<MAX_SAVEGAMES-1 ; i++) {
		m_savegame_menu.actions[i].generic.type			= UITYPE_ACTION;
		m_savegame_menu.actions[i].generic.name			= uiSaveStrings[i+1];
		m_savegame_menu.actions[i].generic.localData[0]	= i+1;
		m_savegame_menu.actions[i].generic.flags		= UIF_LEFT_JUSTIFY;
		m_savegame_menu.actions[i].generic.callBack		= SaveGameCallback;
		m_savegame_menu.actions[i].generic.x			= -UIFT_SIZE*15;
		m_savegame_menu.actions[i].generic.y			= y + ((i + 1) * UIFT_SIZEINC);

		UI_AddItem (&m_savegame_menu.framework,	&m_savegame_menu.actions[i]);
	}

	m_savegame_menu.back_action.generic.type		= UITYPE_ACTION;
	m_savegame_menu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_savegame_menu.back_action.generic.x			= 0;
	m_savegame_menu.back_action.generic.y			= y + ((i + 1) * UIFT_SIZEINC) + (UIFT_SIZEINCLG);
	m_savegame_menu.back_action.generic.name		= "< Back";
	m_savegame_menu.back_action.generic.callBack	= Menu_Pop;
	m_savegame_menu.back_action.generic.statusBar	= "Back a menu";

	UI_AddItem (&m_savegame_menu.framework,	&m_savegame_menu.back_action);

	UI_Center (&m_savegame_menu.framework);
	UI_Setup (&m_savegame_menu.framework);
}


/*
=============
SaveGameMenu_Draw
=============
*/
void SaveGameMenu_Draw (void) {
	UI_Center (&m_savegame_menu.framework);
	UI_Setup (&m_savegame_menu.framework);

	UI_AdjustTextCursor (&m_savegame_menu.framework, 1);
	UI_Draw (&m_savegame_menu.framework);
}


/*
=============
SaveGameMenu_Key
=============
*/
struct sfx_s *SaveGameMenu_Key (int key) {
	return DefaultMenu_Key (&m_savegame_menu.framework, key);
}


/*
=============
UI_SaveGameMenu_f
=============
*/
void UI_SaveGameMenu_f (void) {
	if (!uis.serverState)
		return;	// not playing a game

	SaveGameMenu_Init ();
	UI_PushMenu (&m_savegame_menu.framework, SaveGameMenu_Draw, SaveGameMenu_Key);
	Create_Savestrings ();
}
