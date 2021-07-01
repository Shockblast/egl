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

	DOWNLOAD OPTIONS MENU

=============================================================================
*/

typedef struct m_dloptsmenu_s {
	uiFrameWork_t	framework;

	uiImage_t		banner;
	uiAction_t		header;

	uiList_t		download_toggle;
	uiList_t		download_maps_toggle;
	uiList_t		download_models_toggle;
	uiList_t		download_players_toggle;
	uiList_t		download_sounds_box;

	uiAction_t		back_action;
} m_dloptsmenu_t;

m_dloptsmenu_t	m_dlopts_menu;

static void DownloadCallback (void *self) {
	uiList_t *f = (uiList_t *) self;

	if (f == &m_dlopts_menu.download_toggle)
		uii.Cvar_SetValue ("allow_download", f->curValue);
	else if (f == &m_dlopts_menu.download_maps_toggle)
		uii.Cvar_SetValue ("allow_download_maps", f->curValue);
	else if (f == &m_dlopts_menu.download_models_toggle)
		uii.Cvar_SetValue ("allow_download_models", f->curValue);
	else if (f == &m_dlopts_menu.download_players_toggle)
		uii.Cvar_SetValue ("allow_download_players", f->curValue);
	else if (f == &m_dlopts_menu.download_sounds_box)
		uii.Cvar_SetValue ("allow_download_sounds", f->curValue);
}


/*
=============
DLOptionsMenu_SetValues
=============
*/
static void DLOptionsMenu_SetValues (void) {
	uii.Cvar_SetValue ("allow_download",	clamp (uii.Cvar_VariableInteger ("allow_download"), 0, 1));
	m_dlopts_menu.download_toggle.curValue	= (uii.Cvar_VariableValue("allow_download") != 0);

	uii.Cvar_SetValue ("allow_download_maps",	clamp (uii.Cvar_VariableInteger ("allow_download_maps"), 0, 1));
	m_dlopts_menu.download_maps_toggle.curValue	= (uii.Cvar_VariableValue("allow_download_maps") != 0);

	uii.Cvar_SetValue ("allow_download_players",	clamp (uii.Cvar_VariableInteger ("allow_download_players"), 0, 1));
	m_dlopts_menu.download_players_toggle.curValue	= (uii.Cvar_VariableValue("allow_download_players") != 0);

	uii.Cvar_SetValue ("allow_download_models",		clamp (uii.Cvar_VariableInteger ("allow_download_models"), 0, 1));
	m_dlopts_menu.download_models_toggle.curValue	= (uii.Cvar_VariableValue("allow_download_models") != 0);

	uii.Cvar_SetValue ("allow_download_sounds",	clamp (uii.Cvar_VariableInteger ("allow_download_sounds"), 0, 1));
	m_dlopts_menu.download_sounds_box.curValue	= (uii.Cvar_VariableValue("allow_download_sounds") != 0);
}


/*
=============
DLOptionsMenu_Init
=============
*/
void DLOptionsMenu_Init (void) {
	static char *yes_no_names[] = {
		"no",
		"yes",
		0
	};

	float	y;

	m_dlopts_menu.framework.x			= uis.vidWidth * 0.50;
	m_dlopts_menu.framework.y			= 0;
	m_dlopts_menu.framework.numItems	= 0;

	UI_Banner (&m_dlopts_menu.banner, uiMedia.banners.multiplayer, &y);

	m_dlopts_menu.header.generic.type		= UITYPE_ACTION;
	m_dlopts_menu.header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_dlopts_menu.header.generic.x			= 0;
	m_dlopts_menu.header.generic.y			= y += UIFT_SIZEINC;
	m_dlopts_menu.header.generic.name		= "Download Options";

	m_dlopts_menu.download_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_dlopts_menu.download_toggle.generic.x			= 0;
	m_dlopts_menu.download_toggle.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_dlopts_menu.download_toggle.generic.name		= "Allow downloading";
	m_dlopts_menu.download_toggle.generic.callBack	= DownloadCallback;
	m_dlopts_menu.download_toggle.generic.statusBar	= "Completely toggling downloading";
	m_dlopts_menu.download_toggle.itemNames			= yes_no_names;

	m_dlopts_menu.download_maps_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_dlopts_menu.download_maps_toggle.generic.x			= 0;
	m_dlopts_menu.download_maps_toggle.generic.y			= y += (UIFT_SIZEINC * 2);
	m_dlopts_menu.download_maps_toggle.generic.name			= "Download maps";
	m_dlopts_menu.download_maps_toggle.generic.callBack		= DownloadCallback;
	m_dlopts_menu.download_maps_toggle.generic.statusBar	= "Toggling downloading maps";
	m_dlopts_menu.download_maps_toggle.itemNames			= yes_no_names;

	m_dlopts_menu.download_players_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_dlopts_menu.download_players_toggle.generic.x			= 0;
	m_dlopts_menu.download_players_toggle.generic.y			= y += UIFT_SIZEINC;
	m_dlopts_menu.download_players_toggle.generic.name		= "Download models/skins";
	m_dlopts_menu.download_players_toggle.generic.callBack	= DownloadCallback;
	m_dlopts_menu.download_players_toggle.generic.statusBar	= "Toggling downloading player models/skins";
	m_dlopts_menu.download_players_toggle.itemNames			= yes_no_names;

	m_dlopts_menu.download_models_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_dlopts_menu.download_models_toggle.generic.x			= 0;
	m_dlopts_menu.download_models_toggle.generic.y			= y += UIFT_SIZEINC;
	m_dlopts_menu.download_models_toggle.generic.name		= "Download models";
	m_dlopts_menu.download_models_toggle.generic.callBack	= DownloadCallback;
	m_dlopts_menu.download_models_toggle.generic.statusBar	= "Toggling downloading models";
	m_dlopts_menu.download_models_toggle.itemNames			= yes_no_names;

	m_dlopts_menu.download_sounds_box.generic.type		= UITYPE_SPINCONTROL;
	m_dlopts_menu.download_sounds_box.generic.x			= 0;
	m_dlopts_menu.download_sounds_box.generic.y			= y += UIFT_SIZEINC;
	m_dlopts_menu.download_sounds_box.generic.name		= "Download sounds";
	m_dlopts_menu.download_sounds_box.generic.callBack	= DownloadCallback;
	m_dlopts_menu.download_sounds_box.generic.statusBar	= "Toggling downloading sounds";
	m_dlopts_menu.download_sounds_box.itemNames			= yes_no_names;

	m_dlopts_menu.back_action.generic.type			= UITYPE_ACTION;
	m_dlopts_menu.back_action.generic.flags			= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_dlopts_menu.back_action.generic.x				= 0;
	m_dlopts_menu.back_action.generic.y				= y += UIFT_SIZEINC + UIFT_SIZEINCLG;
	m_dlopts_menu.back_action.generic.name			= "< Back";
	m_dlopts_menu.back_action.generic.callBack		= Menu_Pop;
	m_dlopts_menu.back_action.generic.statusBar		= "Back a menu";

	DLOptionsMenu_SetValues ();

	UI_AddItem (&m_dlopts_menu.framework,				&m_dlopts_menu.banner);
	UI_AddItem (&m_dlopts_menu.framework,				&m_dlopts_menu.header);

	UI_AddItem (&m_dlopts_menu.framework,				&m_dlopts_menu.download_toggle);
	UI_AddItem (&m_dlopts_menu.framework,				&m_dlopts_menu.download_maps_toggle);
	UI_AddItem (&m_dlopts_menu.framework,				&m_dlopts_menu.download_players_toggle);
	UI_AddItem (&m_dlopts_menu.framework,				&m_dlopts_menu.download_models_toggle);
	UI_AddItem (&m_dlopts_menu.framework,				&m_dlopts_menu.download_sounds_box);

	UI_AddItem (&m_dlopts_menu.framework,				&m_dlopts_menu.back_action);

	UI_Center (&m_dlopts_menu.framework);
	UI_Setup (&m_dlopts_menu.framework);
}


/*
=============
DLOptionsMenu_Draw
=============
*/
void DLOptionsMenu_Draw (void) {
	UI_Center (&m_dlopts_menu.framework);
	UI_Setup (&m_dlopts_menu.framework);

	UI_AdjustTextCursor (&m_dlopts_menu.framework, 1);
	UI_Draw (&m_dlopts_menu.framework);
}


/*
=============
DLOptionsMenu_Key
=============
*/
struct sfx_s *DLOptionsMenu_Key (int key) {
	return DefaultMenu_Key (&m_dlopts_menu.framework, key);
}


/*
=============
UI_DLOptionsMenu_f
=============
*/
void UI_DLOptionsMenu_f (void) {
	DLOptionsMenu_Init ();
	UI_PushMenu (&m_dlopts_menu.framework, DLOptionsMenu_Draw, DLOptionsMenu_Key);
}
