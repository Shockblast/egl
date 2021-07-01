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

	MISC MENU

=======================================================================
*/

typedef struct m_miscMenu_s {
	// Menu items
	uiFrameWork_t	frameWork;

	uiImage_t		banner;
	uiAction_t		header;

	uiList_t		defaultpaks_toggle;

	uiList_t		shadows_list;			// effects menu kthx
	uiList_t		screenshot_list;
	uiSlider_t		jpgquality_slider;
	uiAction_t		jpgquality_amount;

	uiList_t		flashblend_list;		// effects menu kthx

	uiAction_t		back_action;
} m_miscMenu_t;

static m_miscMenu_t	m_miscMenu;

static void PakLoadFunc (void *unused)
{
	uii.Cvar_SetValue ("fs_defaultPaks", m_miscMenu.defaultpaks_toggle.curValue);
}

static void ShadowFunc (void *unused)
{
	uii.Cvar_SetValue ("gl_shadows", m_miscMenu.shadows_list.curValue);
}

static void ScreenshotFunc (void *unused)
{
	if (m_miscMenu.screenshot_list.curValue == 1)
		uii.Cvar_Set ("gl_screenshot", "png");
	else if (m_miscMenu.screenshot_list.curValue == 2)
		uii.Cvar_Set ("gl_screenshot", "jpg");
	else
		uii.Cvar_Set ("gl_screenshot", "tga");
}

static void JPGQualityFunc (void *unused)
{
	uii.Cvar_SetValue ("gl_jpgquality", m_miscMenu.jpgquality_slider.curValue * 5);
	m_miscMenu.jpgquality_amount.generic.name = uii.Cvar_VariableString ("gl_jpgquality");
}

static void FlashBlendFunc (void *unused)
{
	uii.Cvar_SetValue ("gl_flashblend", m_miscMenu.flashblend_list.curValue);
}


/*
=============
MiscMenu_SetValues
=============
*/
static void MiscMenu_SetValues (void)
{
	uii.Cvar_SetValue ("fs_defaultPaks",		clamp (uii.Cvar_VariableInteger ("fs_defaultPaks"), 0, 1));
	m_miscMenu.defaultpaks_toggle.curValue		= uii.Cvar_VariableInteger ("fs_defaultPaks");

	uii.Cvar_SetValue ("gl_shadows",			clamp (uii.Cvar_VariableInteger ("gl_shadows"), 0, 2));
	m_miscMenu.shadows_list.curValue			= uii.Cvar_VariableInteger ("gl_shadows");

	if (!Q_stricmp (uii.Cvar_VariableString ("gl_screenshot"), "png"))
		m_miscMenu.screenshot_list.curValue	= 1;
	else if (!Q_stricmp (uii.Cvar_VariableString ("gl_screenshot"), "jpg"))
		m_miscMenu.screenshot_list.curValue	= 2;
	else
		m_miscMenu.screenshot_list.curValue	= 0;

	uii.Cvar_SetValue ("gl_jpgquality",			clamp (uii.Cvar_VariableValue ("gl_jpgquality"), 0, 100));
	m_miscMenu.jpgquality_slider.curValue		= uii.Cvar_VariableValue ("gl_jpgquality") / 5;
	m_miscMenu.jpgquality_amount.generic.name	= uii.Cvar_VariableString ("gl_jpgquality");

	uii.Cvar_SetValue ("gl_flashblend",		clamp (uii.Cvar_VariableInteger ("gl_flashblend"), 0, 2));
	m_miscMenu.flashblend_list.curValue	= uii.Cvar_VariableInteger ("gl_flashblend");
}


/*
=============
MiscMenu_Init
=============
*/
static void MiscMenu_Init (void)
{
	static char *flashblend_names[] = {
		"lightmap",
		"polygon",
		"combo",
		0
	};

	static char *noyes_names[] = {
		"yes",
		"no",
		0
	};

	static char *onoff_names[] = {
		"off",
		"on",
		0
	};

	static char *screenshot_names[] = {
		"tga",
		"png",
		"jpg",
		0
	};

	static char *shadow_names[] = {
		"off",
		"normal",
		"stencil",
		0
	};

	UI_StartFramework (&m_miscMenu.frameWork);

	m_miscMenu.banner.generic.type		= UITYPE_IMAGE;
	m_miscMenu.banner.generic.flags		= UIF_NOSELECT|UIF_CENTERED;
	m_miscMenu.banner.generic.name		= NULL;
	m_miscMenu.banner.shader			= uiMedia.banners.options;

	m_miscMenu.header.generic.type		= UITYPE_ACTION;
	m_miscMenu.header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_miscMenu.header.generic.name		= "Misc Settings";

	m_miscMenu.defaultpaks_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_miscMenu.defaultpaks_toggle.generic.name		= "*.pak loading";
	m_miscMenu.defaultpaks_toggle.generic.callBack	= PakLoadFunc;
	m_miscMenu.defaultpaks_toggle.itemNames			= noyes_names;
	m_miscMenu.defaultpaks_toggle.generic.statusBar	= "pak#.pak; *.pak; loading order";

	m_miscMenu.shadows_list.generic.type		= UITYPE_SPINCONTROL;
	m_miscMenu.shadows_list.generic.name		= "Alias shadows";
	m_miscMenu.shadows_list.generic.callBack	= ShadowFunc;
	m_miscMenu.shadows_list.itemNames			= shadow_names;
	m_miscMenu.shadows_list.generic.statusBar	= "Entity Shadows options";

	m_miscMenu.screenshot_list.generic.type			= UITYPE_SPINCONTROL;
	m_miscMenu.screenshot_list.generic.name			= "Screenshot type";
	m_miscMenu.screenshot_list.generic.callBack		= ScreenshotFunc;
	m_miscMenu.screenshot_list.itemNames			= screenshot_names;
	m_miscMenu.screenshot_list.generic.statusBar	= "Selects screenshot output format";

	m_miscMenu.jpgquality_slider.generic.type		= UITYPE_SLIDER;
	m_miscMenu.jpgquality_slider.generic.name		= "JPG screenshot quality";
	m_miscMenu.jpgquality_slider.generic.callBack	= JPGQualityFunc;
	m_miscMenu.jpgquality_slider.minValue			= 0;
	m_miscMenu.jpgquality_slider.maxValue			= 20;
	m_miscMenu.jpgquality_slider.generic.statusBar	= "JPG Screenshot Quality";
	m_miscMenu.jpgquality_amount.generic.type		= UITYPE_ACTION;
	m_miscMenu.jpgquality_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;

	m_miscMenu.flashblend_list.generic.type			= UITYPE_SPINCONTROL;
	m_miscMenu.flashblend_list.generic.name			= "Flashblend";
	m_miscMenu.flashblend_list.generic.callBack		= FlashBlendFunc;
	m_miscMenu.flashblend_list.itemNames			= flashblend_names;
	m_miscMenu.flashblend_list.generic.statusBar	= "Dynamic light style";

	m_miscMenu.back_action.generic.type			= UITYPE_ACTION;
	m_miscMenu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_miscMenu.back_action.generic.name			= "< Back";
	m_miscMenu.back_action.generic.callBack		= Menu_Pop;
	m_miscMenu.back_action.generic.statusBar	= "Back a menu";

	MiscMenu_SetValues ();

	UI_AddItem (&m_miscMenu.frameWork,			&m_miscMenu.banner);
	UI_AddItem (&m_miscMenu.frameWork,			&m_miscMenu.header);

	UI_AddItem (&m_miscMenu.frameWork,			&m_miscMenu.defaultpaks_toggle);

	UI_AddItem (&m_miscMenu.frameWork,			&m_miscMenu.shadows_list);
	UI_AddItem (&m_miscMenu.frameWork,			&m_miscMenu.screenshot_list);
	UI_AddItem (&m_miscMenu.frameWork,			&m_miscMenu.jpgquality_slider);
	UI_AddItem (&m_miscMenu.frameWork,			&m_miscMenu.jpgquality_amount);

	UI_AddItem (&m_miscMenu.frameWork,			&m_miscMenu.flashblend_list);

	UI_AddItem (&m_miscMenu.frameWork,			&m_miscMenu.back_action);

	UI_FinishFramework (&m_miscMenu.frameWork, qTrue);
}


/*
=============
MiscMenu_Close
=============
*/
static struct sfx_s *MiscMenu_Close (void)
{
	return uiMedia.sounds.menuOut;
}


/*
=============
MiscMenu_Draw
=============
*/
static void MiscMenu_Draw (void)
{
	float	y;

	// Initialize if necessary
	if (!m_miscMenu.frameWork.initialized)
		MiscMenu_Init ();

	// Dynamically position
	m_miscMenu.frameWork.x			= uiState.vidWidth * 0.5f;
	m_miscMenu.frameWork.y			= 0;

	m_miscMenu.banner.generic.x			= 0;
	m_miscMenu.banner.generic.y			= 0;

	y = m_miscMenu.banner.height * UI_SCALE;

	m_miscMenu.header.generic.x					= 0;
	m_miscMenu.header.generic.y					= y += UIFT_SIZEINC;
	m_miscMenu.defaultpaks_toggle.generic.x		= 0;
	m_miscMenu.defaultpaks_toggle.generic.y		= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_miscMenu.shadows_list.generic.x			= 0;
	m_miscMenu.shadows_list.generic.y			= y += (UIFT_SIZEINC*2);
	m_miscMenu.screenshot_list.generic.x		= 0;
	m_miscMenu.screenshot_list.generic.y		= y += UIFT_SIZEINC;
	m_miscMenu.jpgquality_slider.generic.x		= 0;
	m_miscMenu.jpgquality_slider.generic.y		= y += UIFT_SIZEINC;
	m_miscMenu.jpgquality_amount.generic.x		= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_miscMenu.jpgquality_amount.generic.y		= y;
	m_miscMenu.flashblend_list.generic.x		= 0;
	m_miscMenu.flashblend_list.generic.y		= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_miscMenu.back_action.generic.x			= 0;
	m_miscMenu.back_action.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCLG;

	// Render
	UI_CenterMenuHeight (&m_miscMenu.frameWork);
	UI_SetupMenuBounds (&m_miscMenu.frameWork);

	UI_AdjustCursor (&m_miscMenu.frameWork, 1);
	UI_Draw (&m_miscMenu.frameWork);
}


/*
=============
MiscMenu_Key
=============
*/
static struct sfx_s *MiscMenu_Key (int key)
{
	return DefaultMenu_Key (&m_miscMenu.frameWork, key);
}


/*
=============
UI_MiscMenu_f
=============
*/
void UI_MiscMenu_f (void)
{
	MiscMenu_Init ();
	UI_PushMenu (&m_miscMenu.frameWork, MiscMenu_Draw, MiscMenu_Close, MiscMenu_Key);
}
