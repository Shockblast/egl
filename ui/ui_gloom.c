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

	GLOOM OPTIONS MENU

=======================================================================
*/

typedef struct m_gloommenu_s {
	uiFrameWork_t	framework;

	uiImage_t		banner;
	uiAction_t		header;

	uiList_t		advgas_toggle;
	uiList_t		advstingfire_toggle;
	uiList_t		bluestingfire_toggle;
	uiList_t		blobtype_list;

	uiList_t		flashpred_toggle;
	uiList_t		flashwhite_toggle;

	uiList_t		forcecache_toggle;

	uiList_t		jumppred_toggle;

	uiList_t		classdisplay_toggle;

	uiAction_t		back_action;
} m_gloommenu_t;

m_gloommenu_t	m_glmopts_menu;

static void GlmAdvGasFunc (void *unused) {
	uii.Cvar_SetValue ("glm_advgas", m_glmopts_menu.advgas_toggle.curValue);
}

static void GlmAdvStingerFireFunc (void *unused) {
	uii.Cvar_SetValue ("glm_advstingfire", m_glmopts_menu.advstingfire_toggle.curValue);
}

static void GlmBlueStingerFireFunc (void *unused) {
	uii.Cvar_SetValue ("glm_bluestingfire", m_glmopts_menu.bluestingfire_toggle.curValue);
}

static void GlmBlobTypeFunc (void *unused) {
	uii.Cvar_SetValue ("glm_blobtype", m_glmopts_menu.blobtype_list.curValue);
}

static void GlmFlashPredFunc (void *unused) {
	uii.Cvar_SetValue ("glm_flashpred", m_glmopts_menu.flashpred_toggle.curValue);
}

static void GlmFlWhiteFunc (void *unused) {
	uii.Cvar_SetValue ("glm_flwhite", m_glmopts_menu.flashwhite_toggle.curValue);
}

static void GlmForceCacheFunc (void *unused) {
	uii.Cvar_SetValue ("glm_forcecache", m_glmopts_menu.forcecache_toggle.curValue);
}

static void GlmJumpPredFunc (void *unused) {
	uii.Cvar_SetValue ("glm_jumppred", m_glmopts_menu.jumppred_toggle.curValue);
}

static void GlmClassDispFunc (void *unused) {
	uii.Cvar_SetValue ("glm_showclass", m_glmopts_menu.classdisplay_toggle.curValue);
}


/*
=============
GloomMenu_SetValues
=============
*/
static void GloomMenu_SetValues (void) {
	uii.Cvar_SetValue ("glm_advgas",		clamp (uii.Cvar_VariableInteger ("glm_advgas"), 0, 1));
	m_glmopts_menu.advgas_toggle.curValue	= uii.Cvar_VariableInteger ("glm_advgas");

	uii.Cvar_SetValue ("glm_advstingfire",		clamp (uii.Cvar_VariableInteger ("glm_advstingfire"), 0, 1));
	m_glmopts_menu.advstingfire_toggle.curValue	= uii.Cvar_VariableInteger ("glm_advstingfire");

	uii.Cvar_SetValue ("glm_bluestingfire",		clamp (uii.Cvar_VariableInteger ("glm_bluestingfire"), 0, 1));
	m_glmopts_menu.bluestingfire_toggle.curValue	= uii.Cvar_VariableInteger ("glm_bluestingfire");

	uii.Cvar_SetValue ("glm_blobtype",			clamp (uii.Cvar_VariableInteger ("glm_blobtype"), 0, 1));
	m_glmopts_menu.blobtype_list.curValue		= uii.Cvar_VariableInteger ("glm_blobtype");

	uii.Cvar_SetValue ("glm_flashpred",			clamp (uii.Cvar_VariableInteger ("glm_flashpred"), 0, 1));
	m_glmopts_menu.flashpred_toggle.curValue	= uii.Cvar_VariableInteger ("glm_flashpred");

	uii.Cvar_SetValue ("glm_flwhite",			clamp (uii.Cvar_VariableInteger ("glm_flwhite"), 0, 1));
	m_glmopts_menu.flashwhite_toggle.curValue	= uii.Cvar_VariableInteger ("glm_flwhite");

	uii.Cvar_SetValue ("glm_forcecache",		clamp (uii.Cvar_VariableInteger ("glm_forcecache"), 0, 1));
	m_glmopts_menu.forcecache_toggle.curValue	= uii.Cvar_VariableInteger ("glm_forcecache");

	uii.Cvar_SetValue ("glm_jumppred",			clamp (uii.Cvar_VariableInteger ("glm_jumppred"), 0, 1));
	m_glmopts_menu.jumppred_toggle.curValue		= uii.Cvar_VariableInteger ("glm_jumppred");

	uii.Cvar_SetValue ("glm_showclass",			clamp (uii.Cvar_VariableInteger ("glm_showclass"), 0, 1));
	m_glmopts_menu.classdisplay_toggle.curValue	= uii.Cvar_VariableInteger ("glm_showclass");
}


/*
=============
GloomMenu_Init
=============
*/
void GloomMenu_Init (void) {
	static char *blobtype_names[] = {
		"normal",
		"spiral",
		0
	};

	static char *nicenorm_names[] = {
		"normal",
		"nicer",
		0
	};
	
	static char *onoff_names[] = {
		"off",
		"on",
		0
	};

	static char *yesno_names[] = {
		"no",
		"yes",
		0
	};

	float	y;

	m_glmopts_menu.framework.x			= uis.vidWidth * 0.5;
	m_glmopts_menu.framework.y			= 0;
	m_glmopts_menu.framework.numItems	= 0;

	UI_Banner (&m_glmopts_menu.banner, uiMedia.optionsBanner, &y);

	m_glmopts_menu.header.generic.type		= UITYPE_ACTION;
	m_glmopts_menu.header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_glmopts_menu.header.generic.x			= 0;
	m_glmopts_menu.header.generic.y			= y += UIFT_SIZEINC;
	m_glmopts_menu.header.generic.name		= "Gloom Settings";

	m_glmopts_menu.advgas_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_glmopts_menu.advgas_toggle.generic.x			= 0;
	m_glmopts_menu.advgas_toggle.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_glmopts_menu.advgas_toggle.generic.name		= "Advanced gas";
	m_glmopts_menu.advgas_toggle.generic.callBack	= GlmAdvGasFunc;
	m_glmopts_menu.advgas_toggle.itemNames			= onoff_names;
	m_glmopts_menu.advgas_toggle.generic.statusBar	= "Better Stinger/Shock Trooper particles in Gloom";

	m_glmopts_menu.advstingfire_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_glmopts_menu.advstingfire_toggle.generic.x			= 0;
	m_glmopts_menu.advstingfire_toggle.generic.y			= y += UIFT_SIZEINC;
	m_glmopts_menu.advstingfire_toggle.generic.name			= "Advanced stinger fire";
	m_glmopts_menu.advstingfire_toggle.generic.callBack		= GlmAdvStingerFireFunc;
	m_glmopts_menu.advstingfire_toggle.itemNames			= nicenorm_names;
	m_glmopts_menu.advstingfire_toggle.generic.statusBar	= "Better Stinger fire particles in Gloom";

	m_glmopts_menu.bluestingfire_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_glmopts_menu.bluestingfire_toggle.generic.x			= 0;
	m_glmopts_menu.bluestingfire_toggle.generic.y			= y += UIFT_SIZEINC;
	m_glmopts_menu.bluestingfire_toggle.generic.name		= "Blue stinger fire";
	m_glmopts_menu.bluestingfire_toggle.generic.callBack	= GlmBlueStingerFireFunc;
	m_glmopts_menu.bluestingfire_toggle.itemNames			= onoff_names;
	m_glmopts_menu.bluestingfire_toggle.generic.statusBar	= "Blue Stinger fire particles in Gloom (requires nicer stinger fire above)";

	m_glmopts_menu.blobtype_list.generic.type		= UITYPE_SPINCONTROL;
	m_glmopts_menu.blobtype_list.generic.x			= 0;
	m_glmopts_menu.blobtype_list.generic.y			= y += UIFT_SIZEINC;
	m_glmopts_menu.blobtype_list.generic.name		= "Blob trail type";
	m_glmopts_menu.blobtype_list.generic.callBack	= GlmBlobTypeFunc;
	m_glmopts_menu.blobtype_list.itemNames			= blobtype_names;
	m_glmopts_menu.blobtype_list.generic.statusBar	= "Gloom homing blob trail type selection";

	m_glmopts_menu.flashpred_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_glmopts_menu.flashpred_toggle.generic.x			= 0;
	m_glmopts_menu.flashpred_toggle.generic.y			= y += UIFT_SIZEINC*2;
	m_glmopts_menu.flashpred_toggle.generic.name		= "Flashlight prediction";
	m_glmopts_menu.flashpred_toggle.generic.callBack	= GlmFlashPredFunc;
	m_glmopts_menu.flashpred_toggle.itemNames			= yesno_names;
	m_glmopts_menu.flashpred_toggle.generic.statusBar	= "Gloom Flashlight Prediction";

	m_glmopts_menu.flashwhite_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_glmopts_menu.flashwhite_toggle.generic.x			= 0;
	m_glmopts_menu.flashwhite_toggle.generic.y			= y += UIFT_SIZEINC;
	m_glmopts_menu.flashwhite_toggle.generic.name		= "Whiter flashlight";
	m_glmopts_menu.flashwhite_toggle.generic.callBack	= GlmFlWhiteFunc;
	m_glmopts_menu.flashwhite_toggle.itemNames			= yesno_names;
	m_glmopts_menu.flashwhite_toggle.generic.statusBar	= "Makes the Gloom flashlight whiter";

	m_glmopts_menu.forcecache_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_glmopts_menu.forcecache_toggle.generic.x			= 0;
	m_glmopts_menu.forcecache_toggle.generic.y			= y += UIFT_SIZEINC*2;
	m_glmopts_menu.forcecache_toggle.generic.name		= "Forced Caching";
	m_glmopts_menu.forcecache_toggle.generic.callBack	= GlmForceCacheFunc;
	m_glmopts_menu.forcecache_toggle.itemNames			= onoff_names;
	m_glmopts_menu.forcecache_toggle.generic.statusBar	= "Forces caching of common Gloom models";

	m_glmopts_menu.jumppred_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_glmopts_menu.jumppred_toggle.generic.x			= 0;
	m_glmopts_menu.jumppred_toggle.generic.y			= y += UIFT_SIZEINC*2;
	m_glmopts_menu.jumppred_toggle.generic.name			= "Jump prediction";
	m_glmopts_menu.jumppred_toggle.generic.callBack		= GlmJumpPredFunc;
	m_glmopts_menu.jumppred_toggle.itemNames			= onoff_names;
	m_glmopts_menu.jumppred_toggle.generic.statusBar	= "Gloom per-class jump prediction";

	m_glmopts_menu.classdisplay_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_glmopts_menu.classdisplay_toggle.generic.x			= 0;
	m_glmopts_menu.classdisplay_toggle.generic.y			= y += (UIFT_SIZEINC*2);
	m_glmopts_menu.classdisplay_toggle.generic.name			= "Class display";
	m_glmopts_menu.classdisplay_toggle.generic.callBack		= GlmClassDispFunc;
	m_glmopts_menu.classdisplay_toggle.itemNames			= onoff_names;
	m_glmopts_menu.classdisplay_toggle.generic.statusBar	= "Gloom on-screen class display";

	m_glmopts_menu.back_action.generic.type			= UITYPE_ACTION;
	m_glmopts_menu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_glmopts_menu.back_action.generic.x			= 0;
	m_glmopts_menu.back_action.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCLG;
	m_glmopts_menu.back_action.generic.name			= "< Back";
	m_glmopts_menu.back_action.generic.callBack		= Back_Menu;
	m_glmopts_menu.back_action.generic.statusBar	= "Back a menu";

	GloomMenu_SetValues ();

	UI_AddItem (&m_glmopts_menu.framework,		&m_glmopts_menu.banner);
	UI_AddItem (&m_glmopts_menu.framework,		&m_glmopts_menu.header);

	UI_AddItem (&m_glmopts_menu.framework,		&m_glmopts_menu.advgas_toggle);
	UI_AddItem (&m_glmopts_menu.framework,		&m_glmopts_menu.advstingfire_toggle);
	UI_AddItem (&m_glmopts_menu.framework,		&m_glmopts_menu.bluestingfire_toggle);
	UI_AddItem (&m_glmopts_menu.framework,		&m_glmopts_menu.blobtype_list);

	UI_AddItem (&m_glmopts_menu.framework,		&m_glmopts_menu.flashpred_toggle);
	UI_AddItem (&m_glmopts_menu.framework,		&m_glmopts_menu.flashwhite_toggle);

	UI_AddItem (&m_glmopts_menu.framework,		&m_glmopts_menu.forcecache_toggle);

	UI_AddItem (&m_glmopts_menu.framework,		&m_glmopts_menu.jumppred_toggle);

	UI_AddItem (&m_glmopts_menu.framework,		&m_glmopts_menu.classdisplay_toggle);

	UI_AddItem (&m_glmopts_menu.framework,		&m_glmopts_menu.back_action);

	UI_Center (&m_glmopts_menu.framework);
	UI_Setup (&m_glmopts_menu.framework);
}


/*
=============
GloomMenu_Draw
=============
*/
void GloomMenu_Draw (void) {
	UI_Center (&m_glmopts_menu.framework);
	UI_Setup (&m_glmopts_menu.framework);

	UI_AdjustTextCursor (&m_glmopts_menu.framework, 1);
	UI_Draw (&m_glmopts_menu.framework);
}


/*
=============
GloomMenu_Key
=============
*/
struct sfx_s *GloomMenu_Key (int key) {
	return DefaultMenu_Key (&m_glmopts_menu.framework, key);
}


/*
=============
UI_GloomMenu_f
=============
*/
void UI_GloomMenu_f (void) {
	GloomMenu_Init ();
	UI_PushMenu (&m_glmopts_menu.framework, GloomMenu_Draw, GloomMenu_Key);
}
