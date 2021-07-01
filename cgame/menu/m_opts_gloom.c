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

//
// m_opts_gloom.c
//

#include "m_local.h"

/*
=======================================================================

	GLOOM OPTIONS MENU

=======================================================================
*/

typedef struct m_gloomOptionsMenu_s {
	// Menu items
	uiFrameWork_t	frameWork;

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
} m_gloomOptionsMenu_t;

static m_gloomOptionsMenu_t	m_gloomOptionsMenu;

static void GlmAdvGasFunc (void *unused)
{
	cgi.Cvar_SetValue ("glm_advgas", m_gloomOptionsMenu.advgas_toggle.curValue, qFalse);
}

static void GlmAdvStingerFireFunc (void *unused)
{
	cgi.Cvar_SetValue ("glm_advstingfire", m_gloomOptionsMenu.advstingfire_toggle.curValue, qFalse);
}

static void GlmBlueStingerFireFunc (void *unused)
{
	cgi.Cvar_SetValue ("glm_bluestingfire", m_gloomOptionsMenu.bluestingfire_toggle.curValue, qFalse);
}

static void GlmBlobTypeFunc (void *unused)
{
	cgi.Cvar_SetValue ("glm_blobtype", m_gloomOptionsMenu.blobtype_list.curValue, qFalse);
}

static void GlmFlashPredFunc (void *unused)
{
	cgi.Cvar_SetValue ("glm_flashpred", m_gloomOptionsMenu.flashpred_toggle.curValue, qFalse);
}

static void GlmFlWhiteFunc (void *unused)
{
	cgi.Cvar_SetValue ("glm_flwhite", m_gloomOptionsMenu.flashwhite_toggle.curValue, qFalse);
}

static void GlmForceCacheFunc (void *unused)
{
	cgi.Cvar_SetValue ("glm_forcecache", m_gloomOptionsMenu.forcecache_toggle.curValue, qFalse);
}

static void GlmJumpPredFunc (void *unused)
{
	cgi.Cvar_SetValue ("glm_jumppred", m_gloomOptionsMenu.jumppred_toggle.curValue, qFalse);
}

static void GlmClassDispFunc (void *unused)
{
	cgi.Cvar_SetValue ("glm_showclass", m_gloomOptionsMenu.classdisplay_toggle.curValue, qFalse);
}


/*
=============
GloomMenu_SetValues
=============
*/
static void GloomMenu_SetValues (void)
{
	cgi.Cvar_SetValue ("glm_advgas",		clamp (cgi.Cvar_GetIntegerValue ("glm_advgas"), 0, 1), qFalse);
	m_gloomOptionsMenu.advgas_toggle.curValue	= cgi.Cvar_GetIntegerValue ("glm_advgas");

	cgi.Cvar_SetValue ("glm_advstingfire",		clamp (cgi.Cvar_GetIntegerValue ("glm_advstingfire"), 0, 1), qFalse);
	m_gloomOptionsMenu.advstingfire_toggle.curValue	= cgi.Cvar_GetIntegerValue ("glm_advstingfire");

	cgi.Cvar_SetValue ("glm_bluestingfire",		clamp (cgi.Cvar_GetIntegerValue ("glm_bluestingfire"), 0, 1), qFalse);
	m_gloomOptionsMenu.bluestingfire_toggle.curValue	= cgi.Cvar_GetIntegerValue ("glm_bluestingfire");

	cgi.Cvar_SetValue ("glm_blobtype",			clamp (cgi.Cvar_GetIntegerValue ("glm_blobtype"), 0, 1), qFalse);
	m_gloomOptionsMenu.blobtype_list.curValue		= cgi.Cvar_GetIntegerValue ("glm_blobtype");

	cgi.Cvar_SetValue ("glm_flashpred",			clamp (cgi.Cvar_GetIntegerValue ("glm_flashpred"), 0, 1), qFalse);
	m_gloomOptionsMenu.flashpred_toggle.curValue	= cgi.Cvar_GetIntegerValue ("glm_flashpred");

	cgi.Cvar_SetValue ("glm_flwhite",			clamp (cgi.Cvar_GetIntegerValue ("glm_flwhite"), 0, 1), qFalse);
	m_gloomOptionsMenu.flashwhite_toggle.curValue	= cgi.Cvar_GetIntegerValue ("glm_flwhite");

	cgi.Cvar_SetValue ("glm_forcecache",		clamp (cgi.Cvar_GetIntegerValue ("glm_forcecache"), 0, 1), qFalse);
	m_gloomOptionsMenu.forcecache_toggle.curValue	= cgi.Cvar_GetIntegerValue ("glm_forcecache");

	cgi.Cvar_SetValue ("glm_jumppred",			clamp (cgi.Cvar_GetIntegerValue ("glm_jumppred"), 0, 1), qFalse);
	m_gloomOptionsMenu.jumppred_toggle.curValue		= cgi.Cvar_GetIntegerValue ("glm_jumppred");

	cgi.Cvar_SetValue ("glm_showclass",			clamp (cgi.Cvar_GetIntegerValue ("glm_showclass"), 0, 1), qFalse);
	m_gloomOptionsMenu.classdisplay_toggle.curValue	= cgi.Cvar_GetIntegerValue ("glm_showclass");
}


/*
=============
GloomMenu_Init
=============
*/
static void GloomMenu_Init (void)
{
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

	UI_StartFramework (&m_gloomOptionsMenu.frameWork, FWF_CENTERHEIGHT);

	m_gloomOptionsMenu.banner.generic.type		= UITYPE_IMAGE;
	m_gloomOptionsMenu.banner.generic.flags		= UIF_NOSELECT|UIF_CENTERED;
	m_gloomOptionsMenu.banner.generic.name		= NULL;
	m_gloomOptionsMenu.banner.shader			= uiMedia.banners.options;

	m_gloomOptionsMenu.header.generic.type		= UITYPE_ACTION;
	m_gloomOptionsMenu.header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_gloomOptionsMenu.header.generic.name		= "Gloom Settings";

	m_gloomOptionsMenu.advgas_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_gloomOptionsMenu.advgas_toggle.generic.name		= "Advanced gas";
	m_gloomOptionsMenu.advgas_toggle.generic.callBack	= GlmAdvGasFunc;
	m_gloomOptionsMenu.advgas_toggle.itemNames			= onoff_names;
	m_gloomOptionsMenu.advgas_toggle.generic.statusBar	= "Better Stinger/Shock Trooper particles in Gloom";

	m_gloomOptionsMenu.advstingfire_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_gloomOptionsMenu.advstingfire_toggle.generic.name			= "Advanced stinger fire";
	m_gloomOptionsMenu.advstingfire_toggle.generic.callBack		= GlmAdvStingerFireFunc;
	m_gloomOptionsMenu.advstingfire_toggle.itemNames			= nicenorm_names;
	m_gloomOptionsMenu.advstingfire_toggle.generic.statusBar	= "Better Stinger fire particles in Gloom";

	m_gloomOptionsMenu.bluestingfire_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_gloomOptionsMenu.bluestingfire_toggle.generic.name		= "Blue stinger fire";
	m_gloomOptionsMenu.bluestingfire_toggle.generic.callBack	= GlmBlueStingerFireFunc;
	m_gloomOptionsMenu.bluestingfire_toggle.itemNames			= onoff_names;
	m_gloomOptionsMenu.bluestingfire_toggle.generic.statusBar	= "Blue Stinger fire particles in Gloom (requires nicer stinger fire above)";

	m_gloomOptionsMenu.blobtype_list.generic.type		= UITYPE_SPINCONTROL;
	m_gloomOptionsMenu.blobtype_list.generic.name		= "Blob trail type";
	m_gloomOptionsMenu.blobtype_list.generic.callBack	= GlmBlobTypeFunc;
	m_gloomOptionsMenu.blobtype_list.itemNames			= blobtype_names;
	m_gloomOptionsMenu.blobtype_list.generic.statusBar	= "Gloom homing blob trail type selection";

	m_gloomOptionsMenu.flashpred_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_gloomOptionsMenu.flashpred_toggle.generic.name		= "Flashlight prediction";
	m_gloomOptionsMenu.flashpred_toggle.generic.callBack	= GlmFlashPredFunc;
	m_gloomOptionsMenu.flashpred_toggle.itemNames			= yesno_names;
	m_gloomOptionsMenu.flashpred_toggle.generic.statusBar	= "Gloom Flashlight Prediction";

	m_gloomOptionsMenu.flashwhite_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_gloomOptionsMenu.flashwhite_toggle.generic.name		= "Whiter flashlight";
	m_gloomOptionsMenu.flashwhite_toggle.generic.callBack	= GlmFlWhiteFunc;
	m_gloomOptionsMenu.flashwhite_toggle.itemNames			= yesno_names;
	m_gloomOptionsMenu.flashwhite_toggle.generic.statusBar	= "Makes the Gloom flashlight whiter";

	m_gloomOptionsMenu.forcecache_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_gloomOptionsMenu.forcecache_toggle.generic.name		= "Forced Caching";
	m_gloomOptionsMenu.forcecache_toggle.generic.callBack	= GlmForceCacheFunc;
	m_gloomOptionsMenu.forcecache_toggle.itemNames			= onoff_names;
	m_gloomOptionsMenu.forcecache_toggle.generic.statusBar	= "Forces caching of common Gloom models";

	m_gloomOptionsMenu.jumppred_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_gloomOptionsMenu.jumppred_toggle.generic.name			= "Jump prediction";
	m_gloomOptionsMenu.jumppred_toggle.generic.callBack		= GlmJumpPredFunc;
	m_gloomOptionsMenu.jumppred_toggle.itemNames			= onoff_names;
	m_gloomOptionsMenu.jumppred_toggle.generic.statusBar	= "Gloom per-class jump prediction";

	m_gloomOptionsMenu.classdisplay_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_gloomOptionsMenu.classdisplay_toggle.generic.name			= "Class display";
	m_gloomOptionsMenu.classdisplay_toggle.generic.callBack		= GlmClassDispFunc;
	m_gloomOptionsMenu.classdisplay_toggle.itemNames			= onoff_names;
	m_gloomOptionsMenu.classdisplay_toggle.generic.statusBar	= "Gloom on-screen class display";

	m_gloomOptionsMenu.back_action.generic.type			= UITYPE_ACTION;
	m_gloomOptionsMenu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_gloomOptionsMenu.back_action.generic.name			= "< Back";
	m_gloomOptionsMenu.back_action.generic.callBack		= Menu_Pop;
	m_gloomOptionsMenu.back_action.generic.statusBar	= "Back a menu";

	GloomMenu_SetValues ();

	UI_AddItem (&m_gloomOptionsMenu.frameWork,		&m_gloomOptionsMenu.banner);
	UI_AddItem (&m_gloomOptionsMenu.frameWork,		&m_gloomOptionsMenu.header);

	UI_AddItem (&m_gloomOptionsMenu.frameWork,		&m_gloomOptionsMenu.advgas_toggle);
	UI_AddItem (&m_gloomOptionsMenu.frameWork,		&m_gloomOptionsMenu.advstingfire_toggle);
	UI_AddItem (&m_gloomOptionsMenu.frameWork,		&m_gloomOptionsMenu.bluestingfire_toggle);
	UI_AddItem (&m_gloomOptionsMenu.frameWork,		&m_gloomOptionsMenu.blobtype_list);

	UI_AddItem (&m_gloomOptionsMenu.frameWork,		&m_gloomOptionsMenu.flashpred_toggle);
	UI_AddItem (&m_gloomOptionsMenu.frameWork,		&m_gloomOptionsMenu.flashwhite_toggle);

	UI_AddItem (&m_gloomOptionsMenu.frameWork,		&m_gloomOptionsMenu.forcecache_toggle);

	UI_AddItem (&m_gloomOptionsMenu.frameWork,		&m_gloomOptionsMenu.jumppred_toggle);

	UI_AddItem (&m_gloomOptionsMenu.frameWork,		&m_gloomOptionsMenu.classdisplay_toggle);

	UI_AddItem (&m_gloomOptionsMenu.frameWork,		&m_gloomOptionsMenu.back_action);

	UI_FinishFramework (&m_gloomOptionsMenu.frameWork, qTrue);
}


/*
=============
GloomMenu_Close
=============
*/
static struct sfx_s *GloomMenu_Close (void)
{
	return uiMedia.sounds.menuOut;
}


/*
=============
GloomMenu_Draw
=============
*/
static void GloomMenu_Draw (void)
{
	float	y;

	// Initialize if necessary
	if (!m_gloomOptionsMenu.frameWork.initialized)
		GloomMenu_Init ();

	// Dynamically position
	m_gloomOptionsMenu.frameWork.x			= cg.glConfig.vidWidth * 0.5;
	m_gloomOptionsMenu.frameWork.y			= 0;

	m_gloomOptionsMenu.banner.generic.x		= 0;
	m_gloomOptionsMenu.banner.generic.y		= 0;

	y = m_gloomOptionsMenu.banner.height * UI_SCALE;

	m_gloomOptionsMenu.header.generic.x					= 0;
	m_gloomOptionsMenu.header.generic.y					= y += UIFT_SIZEINC;
	m_gloomOptionsMenu.advgas_toggle.generic.x			= 0;
	m_gloomOptionsMenu.advgas_toggle.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_gloomOptionsMenu.advstingfire_toggle.generic.x	= 0;
	m_gloomOptionsMenu.advstingfire_toggle.generic.y	= y += UIFT_SIZEINC;
	m_gloomOptionsMenu.bluestingfire_toggle.generic.x	= 0;
	m_gloomOptionsMenu.bluestingfire_toggle.generic.y	= y += UIFT_SIZEINC;
	m_gloomOptionsMenu.blobtype_list.generic.x			= 0;
	m_gloomOptionsMenu.blobtype_list.generic.y			= y += UIFT_SIZEINC;
	m_gloomOptionsMenu.flashpred_toggle.generic.x		= 0;
	m_gloomOptionsMenu.flashpred_toggle.generic.y		= y += UIFT_SIZEINC*2;
	m_gloomOptionsMenu.flashwhite_toggle.generic.x		= 0;
	m_gloomOptionsMenu.flashwhite_toggle.generic.y		= y += UIFT_SIZEINC;
	m_gloomOptionsMenu.forcecache_toggle.generic.x		= 0;
	m_gloomOptionsMenu.forcecache_toggle.generic.y		= y += UIFT_SIZEINC*2;
	m_gloomOptionsMenu.jumppred_toggle.generic.x		= 0;
	m_gloomOptionsMenu.jumppred_toggle.generic.y		= y += UIFT_SIZEINC*2;
	m_gloomOptionsMenu.classdisplay_toggle.generic.x	= 0;
	m_gloomOptionsMenu.classdisplay_toggle.generic.y	= y += (UIFT_SIZEINC*2);
	m_gloomOptionsMenu.back_action.generic.x			= 0;
	m_gloomOptionsMenu.back_action.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCLG;

	// Render
	UI_DrawInterface (&m_gloomOptionsMenu.frameWork);
}


/*
=============
UI_GloomMenu_f
=============
*/
void UI_GloomMenu_f (void)
{
	GloomMenu_Init ();
	M_PushMenu (&m_gloomOptionsMenu.frameWork, GloomMenu_Draw, GloomMenu_Close, M_KeyHandler);
}
