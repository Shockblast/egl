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

typedef struct m_downloadOptionsMenu_s {
	// Menu items
	uiFrameWork_t		frameWork;

	uiImage_t			banner;
	uiAction_t			header;

	uiList_t			download_toggle;
	uiList_t			download_maps_toggle;
	uiList_t			download_models_toggle;
	uiList_t			download_players_toggle;
	uiList_t			download_sounds_box;

	uiAction_t			back_action;
} m_downloadOptionsMenu_t;

static m_downloadOptionsMenu_t	m_downloadOptionsMenu;

static void DownloadCallback (void *self)
{
	uiList_t *f = (uiList_t *) self;

	if (f == &m_downloadOptionsMenu.download_toggle)
		uii.Cvar_SetValue ("allow_download", f->curValue);
	else if (f == &m_downloadOptionsMenu.download_maps_toggle)
		uii.Cvar_SetValue ("allow_download_maps", f->curValue);
	else if (f == &m_downloadOptionsMenu.download_models_toggle)
		uii.Cvar_SetValue ("allow_download_models", f->curValue);
	else if (f == &m_downloadOptionsMenu.download_players_toggle)
		uii.Cvar_SetValue ("allow_download_players", f->curValue);
	else if (f == &m_downloadOptionsMenu.download_sounds_box)
		uii.Cvar_SetValue ("allow_download_sounds", f->curValue);
}


/*
=============
DLOptionsMenu_SetValues
=============
*/
static void DLOptionsMenu_SetValues (void)
{
	uii.Cvar_SetValue ("allow_download", clamp (uii.Cvar_VariableInteger ("allow_download"), 0, 1));
	m_downloadOptionsMenu.download_toggle.curValue	= (uii.Cvar_VariableValue("allow_download") != 0);

	uii.Cvar_SetValue ("allow_download_maps", clamp (uii.Cvar_VariableInteger ("allow_download_maps"), 0, 1));
	m_downloadOptionsMenu.download_maps_toggle.curValue	= (uii.Cvar_VariableValue("allow_download_maps") != 0);

	uii.Cvar_SetValue ("allow_download_players", clamp (uii.Cvar_VariableInteger ("allow_download_players"), 0, 1));
	m_downloadOptionsMenu.download_players_toggle.curValue	= (uii.Cvar_VariableValue("allow_download_players") != 0);

	uii.Cvar_SetValue ("allow_download_models", clamp (uii.Cvar_VariableInteger ("allow_download_models"), 0, 1));
	m_downloadOptionsMenu.download_models_toggle.curValue	= (uii.Cvar_VariableValue("allow_download_models") != 0);

	uii.Cvar_SetValue ("allow_download_sounds", clamp (uii.Cvar_VariableInteger ("allow_download_sounds"), 0, 1));
	m_downloadOptionsMenu.download_sounds_box.curValue	= (uii.Cvar_VariableValue("allow_download_sounds") != 0);
}


/*
=============
DLOptionsMenu_Init
=============
*/
static void DLOptionsMenu_Init (void)
{
	static char *yes_no_names[] = {
		"no",
		"yes",
		0
	};

	UI_StartFramework (&m_downloadOptionsMenu.frameWork);

	m_downloadOptionsMenu.banner.generic.type		= UITYPE_IMAGE;
	m_downloadOptionsMenu.banner.generic.flags		= UIF_NOSELECT|UIF_CENTERED;
	m_downloadOptionsMenu.banner.generic.name		= NULL;
	m_downloadOptionsMenu.banner.shader				= uiMedia.banners.multiplayer;

	m_downloadOptionsMenu.header.generic.type		= UITYPE_ACTION;
	m_downloadOptionsMenu.header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_downloadOptionsMenu.header.generic.name		= "Download Options";

	m_downloadOptionsMenu.download_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_downloadOptionsMenu.download_toggle.generic.name		= "Allow downloading";
	m_downloadOptionsMenu.download_toggle.generic.callBack	= DownloadCallback;
	m_downloadOptionsMenu.download_toggle.generic.statusBar	= "Completely toggling downloading";
	m_downloadOptionsMenu.download_toggle.itemNames			= yes_no_names;

	m_downloadOptionsMenu.download_maps_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_downloadOptionsMenu.download_maps_toggle.generic.name			= "Download maps";
	m_downloadOptionsMenu.download_maps_toggle.generic.callBack		= DownloadCallback;
	m_downloadOptionsMenu.download_maps_toggle.generic.statusBar	= "Toggling downloading maps";
	m_downloadOptionsMenu.download_maps_toggle.itemNames			= yes_no_names;

	m_downloadOptionsMenu.download_players_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_downloadOptionsMenu.download_players_toggle.generic.name		= "Download models/skins";
	m_downloadOptionsMenu.download_players_toggle.generic.callBack	= DownloadCallback;
	m_downloadOptionsMenu.download_players_toggle.generic.statusBar	= "Toggling downloading player models/skins";
	m_downloadOptionsMenu.download_players_toggle.itemNames			= yes_no_names;

	m_downloadOptionsMenu.download_models_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_downloadOptionsMenu.download_models_toggle.generic.name		= "Download models";
	m_downloadOptionsMenu.download_models_toggle.generic.callBack	= DownloadCallback;
	m_downloadOptionsMenu.download_models_toggle.generic.statusBar	= "Toggling downloading models";
	m_downloadOptionsMenu.download_models_toggle.itemNames			= yes_no_names;

	m_downloadOptionsMenu.download_sounds_box.generic.type		= UITYPE_SPINCONTROL;
	m_downloadOptionsMenu.download_sounds_box.generic.name		= "Download sounds";
	m_downloadOptionsMenu.download_sounds_box.generic.callBack	= DownloadCallback;
	m_downloadOptionsMenu.download_sounds_box.generic.statusBar	= "Toggling downloading sounds";
	m_downloadOptionsMenu.download_sounds_box.itemNames			= yes_no_names;

	m_downloadOptionsMenu.back_action.generic.type			= UITYPE_ACTION;
	m_downloadOptionsMenu.back_action.generic.flags			= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_downloadOptionsMenu.back_action.generic.name			= "< Back";
	m_downloadOptionsMenu.back_action.generic.callBack		= Menu_Pop;
	m_downloadOptionsMenu.back_action.generic.statusBar		= "Back a menu";

	DLOptionsMenu_SetValues ();

	UI_AddItem (&m_downloadOptionsMenu.frameWork,				&m_downloadOptionsMenu.banner);
	UI_AddItem (&m_downloadOptionsMenu.frameWork,				&m_downloadOptionsMenu.header);

	UI_AddItem (&m_downloadOptionsMenu.frameWork,				&m_downloadOptionsMenu.download_toggle);
	UI_AddItem (&m_downloadOptionsMenu.frameWork,				&m_downloadOptionsMenu.download_maps_toggle);
	UI_AddItem (&m_downloadOptionsMenu.frameWork,				&m_downloadOptionsMenu.download_players_toggle);
	UI_AddItem (&m_downloadOptionsMenu.frameWork,				&m_downloadOptionsMenu.download_models_toggle);
	UI_AddItem (&m_downloadOptionsMenu.frameWork,				&m_downloadOptionsMenu.download_sounds_box);

	UI_AddItem (&m_downloadOptionsMenu.frameWork,				&m_downloadOptionsMenu.back_action);

	UI_FinishFramework (&m_downloadOptionsMenu.frameWork, qTrue);
}


/*
=============
DLOptionsMenu_Close
=============
*/
static struct sfx_s *DLOptionsMenu_Close (void)
{
	return uiMedia.sounds.menuOut;
}


/*
=============
DLOptionsMenu_Draw
=============
*/
static void DLOptionsMenu_Draw (void)
{
	float	y;

	// Initialize if necessary
	if (!m_downloadOptionsMenu.frameWork.initialized)
		DLOptionsMenu_Init ();

	// Dynamically position
	m_downloadOptionsMenu.frameWork.x						= uiState.vidWidth * 0.50f;
	m_downloadOptionsMenu.frameWork.y						= 0;

	m_downloadOptionsMenu.banner.generic.x					= 0;
	m_downloadOptionsMenu.banner.generic.y					= 0;

	y = m_downloadOptionsMenu.banner.height * UI_SCALE;

	m_downloadOptionsMenu.header.generic.x					= 0;
	m_downloadOptionsMenu.header.generic.y					= y += UIFT_SIZEINC;
	m_downloadOptionsMenu.download_toggle.generic.x			= 0;
	m_downloadOptionsMenu.download_toggle.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_downloadOptionsMenu.download_maps_toggle.generic.x	= 0;
	m_downloadOptionsMenu.download_maps_toggle.generic.y	= y += (UIFT_SIZEINC * 2);
	m_downloadOptionsMenu.download_players_toggle.generic.x	= 0;
	m_downloadOptionsMenu.download_players_toggle.generic.y	= y += UIFT_SIZEINC;
	m_downloadOptionsMenu.download_models_toggle.generic.x	= 0;
	m_downloadOptionsMenu.download_models_toggle.generic.y	= y += UIFT_SIZEINC;
	m_downloadOptionsMenu.download_sounds_box.generic.x		= 0;
	m_downloadOptionsMenu.download_sounds_box.generic.y		= y += UIFT_SIZEINC;

	m_downloadOptionsMenu.back_action.generic.x				= 0;
	m_downloadOptionsMenu.back_action.generic.y				= y += UIFT_SIZEINC + UIFT_SIZEINCLG;

	// Render
	UI_CenterMenuHeight (&m_downloadOptionsMenu.frameWork);
	UI_SetupMenuBounds (&m_downloadOptionsMenu.frameWork);

	UI_AdjustCursor (&m_downloadOptionsMenu.frameWork, 1);
	UI_Draw (&m_downloadOptionsMenu.frameWork);
}


/*
=============
DLOptionsMenu_Key
=============
*/
static struct sfx_s *DLOptionsMenu_Key (int key)
{
	return DefaultMenu_Key (&m_downloadOptionsMenu.frameWork, key);
}


/*
=============
UI_DLOptionsMenu_f
=============
*/
void UI_DLOptionsMenu_f (void)
{
	DLOptionsMenu_Init ();
	UI_PushMenu (&m_downloadOptionsMenu.frameWork, DLOptionsMenu_Draw, DLOptionsMenu_Close, DLOptionsMenu_Key);
}
