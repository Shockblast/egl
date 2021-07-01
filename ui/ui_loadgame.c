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

	LOADGAME MENU

=============================================================================
*/

typedef struct {
	uiFrameWork_t	framework;

	uiImage_t		banner;

	uiAction_t		actions[MAX_SAVEGAMES];
	uiAction_t		back_action;
} m_loadgamemenu_t;

m_loadgamemenu_t	m_loadgame_menu;

char		ui_SaveStrings[MAX_SAVEGAMES][32];
qBool		ui_SaveValid[MAX_SAVEGAMES];

void Create_Savestrings (void) {
	int		i;
	FILE	*f;
	char	name[MAX_OSPATH];

	for (i=0 ; i<MAX_SAVEGAMES ; i++) {
		Com_sprintf (name, sizeof (name), "%s/save/save%i/server.ssv", uii.FS_Gamedir(), i);
		f = fopen (name, "rb");
		if (!f) {
			strcpy (ui_SaveStrings[i], "<EMPTY>");
			ui_SaveValid[i] = qFalse;
		} else {
			fread (ui_SaveStrings[i], 1, sizeof (ui_SaveStrings[i]), f);
			fclose (f);
			ui_SaveValid[i] = qTrue;
		}
	}
}

void LoadGameCallback (void *self) {
	uiAction_t *a = (uiAction_t *) self;

	if (ui_SaveValid[a->generic.localdata[0]])
		uii.Cbuf_AddText (va("load save%i\n", a->generic.localdata[0]));
	UI_ForceMenuOff ();
}


/*
=============
LoadGame_MenuInit
=============
*/
void LoadGame_MenuInit (void) {
	int		i;
	float	y;

	m_loadgame_menu.framework.x			= uis.vidWidth * 0.5;
	m_loadgame_menu.framework.y			= 0;
	m_loadgame_menu.framework.nitems	= 0;

	Create_Savestrings ();

	UI_Banner (&m_loadgame_menu.banner, uiMedia.loadGameBanner, &y);
	UI_AddItem (&m_loadgame_menu.framework,		&m_loadgame_menu.banner);

	for (i=0 ; i<MAX_SAVEGAMES ; i++) {
		m_loadgame_menu.actions[i].generic.type			= UITYPE_ACTION;
		m_loadgame_menu.actions[i].generic.name			= ui_SaveStrings[i];
		m_loadgame_menu.actions[i].generic.flags		= UIF_LEFT_JUSTIFY;
		m_loadgame_menu.actions[i].generic.localdata[0]	= i;
		m_loadgame_menu.actions[i].generic.callback		= LoadGameCallback;
		m_loadgame_menu.actions[i].generic.x			= -UIFT_SIZE*15;
		m_loadgame_menu.actions[i].generic.y			= y + ((i + 1) * UIFT_SIZEINC);
		if (i > 0)	// separate from autosave
			m_loadgame_menu.actions[i].generic.y		+= UIFT_SIZEINC;

		UI_AddItem (&m_loadgame_menu.framework,	&m_loadgame_menu.actions[i]);
	}

	m_loadgame_menu.back_action.generic.type		= UITYPE_ACTION;
	m_loadgame_menu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_loadgame_menu.back_action.generic.x			= 0;
	m_loadgame_menu.back_action.generic.y			= y + ((i + 1) * UIFT_SIZEINC) + (UIFT_SIZEINCLG);
	m_loadgame_menu.back_action.generic.name		= "< Back";
	m_loadgame_menu.back_action.generic.callback	= Back_Menu;
	m_loadgame_menu.back_action.generic.statusbar	= "Back a menu";

	UI_AddItem (&m_loadgame_menu.framework,	&m_loadgame_menu.back_action);

	UI_Center (&m_loadgame_menu.framework);
	UI_Setup (&m_loadgame_menu.framework);
}


/*
=============
LoadGame_MenuDraw
=============
*/
void LoadGame_MenuDraw (void) {
	UI_Center (&m_loadgame_menu.framework);
	UI_Setup (&m_loadgame_menu.framework);

	UI_AdjustTextCursor (&m_loadgame_menu.framework, 1);
	UI_Draw (&m_loadgame_menu.framework);
}


/*
=============
LoadGame_MenuKey
=============
*/
const char *LoadGame_MenuKey (int key) {
	return DefaultMenu_Key (&m_loadgame_menu.framework, key);
}


/*
=============
UI_LoadGameMenu_f
=============
*/
void UI_LoadGameMenu_f (void) {
	LoadGame_MenuInit ();
	UI_PushMenu (&m_loadgame_menu.framework, LoadGame_MenuDraw, LoadGame_MenuKey);
}
