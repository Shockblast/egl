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

typedef struct {
	uiFrameWork_t	framework;

	uiImage_t		banner;
	uiAction_t		header;

	uiList_t		defaultpaks_toggle;

	uiList_t		shadows_list;			// effects menu kthx
	uiList_t		screenshot_list;
	uiSlider_t		jpgquality_slider;
	uiAction_t		jpgquality_amount;

	uiList_t		flashblend_list;		// effects menu kthx

	uiAction_t		back_action;
} m_miscmenu_t;

m_miscmenu_t	m_misc_menu;

static void PakLoadFunc (void *unused) {
	uii.Cvar_SetValue ("sv_defaultpaks", m_misc_menu.defaultpaks_toggle.curvalue);
}

static void ShadowFunc (void *unused) {
	uii.Cvar_SetValue ("gl_shadows", m_misc_menu.shadows_list.curvalue);
}

static void ScreenshotFunc (void *unused){
	if (m_misc_menu.screenshot_list.curvalue == 1)
		uii.Cvar_Set ("gl_screenshot", "png");
	else if (m_misc_menu.screenshot_list.curvalue == 2)
		uii.Cvar_Set ("gl_screenshot", "jpg");
	else
		uii.Cvar_Set ("gl_screenshot", "tga");
}

static void JPGQualityFunc (void *unused) {
	uii.Cvar_SetValue ("gl_jpgquality", m_misc_menu.jpgquality_slider.curvalue * 5);
	m_misc_menu.jpgquality_amount.generic.name = uii.Cvar_VariableString ("gl_jpgquality");
}

static void FlashBlendFunc (void *unused) {
	uii.Cvar_SetValue ("gl_flashblend", m_misc_menu.flashblend_list.curvalue);
}


/*
=============
MiscMenu_SetValues
=============
*/
static void MiscMenu_SetValues (void) {
	uii.Cvar_SetValue ("sv_defaultpaks",		clamp (uii.Cvar_VariableInteger ("sv_defaultpaks"), 0, 1));
	m_misc_menu.defaultpaks_toggle.curvalue		= uii.Cvar_VariableInteger ("sv_defaultpaks");

	uii.Cvar_SetValue ("gl_shadows",			clamp (uii.Cvar_VariableInteger ("gl_shadows"), 0, 2));
	m_misc_menu.shadows_list.curvalue			= uii.Cvar_VariableInteger ("gl_shadows");

	if (!strcmp (uii.Cvar_VariableString ("gl_screenshot"), "png"))
		m_misc_menu.screenshot_list.curvalue	= 1;
	else if (!strcmp (uii.Cvar_VariableString ("gl_screenshot"), "jpg"))
		m_misc_menu.screenshot_list.curvalue	= 2;
	else
		m_misc_menu.screenshot_list.curvalue	= 0;

	uii.Cvar_SetValue ("gl_jpgquality",			clamp (uii.Cvar_VariableValue ("gl_jpgquality"), 0, 100));
	m_misc_menu.jpgquality_slider.curvalue		= uii.Cvar_VariableValue ("gl_jpgquality") / 5;
	m_misc_menu.jpgquality_amount.generic.name	= uii.Cvar_VariableString ("gl_jpgquality");

	uii.Cvar_SetValue ("gl_flashblend",		clamp (uii.Cvar_VariableInteger ("gl_flashblend"), 0, 2));
	m_misc_menu.flashblend_list.curvalue	= uii.Cvar_VariableInteger ("gl_flashblend");
}


/*
=============
MiscMenu_Init
=============
*/
void MiscMenu_Init (void) {
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

	float	y;

	m_misc_menu.framework.x			= uis.vidWidth * 0.5;
	m_misc_menu.framework.y			= 0;
	m_misc_menu.framework.nitems	= 0;

	UI_Banner (&m_misc_menu.banner, uiMedia.optionsBanner, &y);

	m_misc_menu.header.generic.type			= UITYPE_ACTION;
	m_misc_menu.header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_misc_menu.header.generic.x			= 0;
	m_misc_menu.header.generic.y			= y += UIFT_SIZEINC;
	m_misc_menu.header.generic.name			= "Misc Settings";

	m_misc_menu.defaultpaks_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_misc_menu.defaultpaks_toggle.generic.x			= 0;
	m_misc_menu.defaultpaks_toggle.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_misc_menu.defaultpaks_toggle.generic.name			= "*.pak loading";
	m_misc_menu.defaultpaks_toggle.generic.callback		= PakLoadFunc;
	m_misc_menu.defaultpaks_toggle.itemnames			= noyes_names;
	m_misc_menu.defaultpaks_toggle.generic.statusbar	= "pak#.pak; *.pak; loading order";

	m_misc_menu.shadows_list.generic.type		= UITYPE_SPINCONTROL;
	m_misc_menu.shadows_list.generic.x			= 0;
	m_misc_menu.shadows_list.generic.y			= y += (UIFT_SIZEINC*2);
	m_misc_menu.shadows_list.generic.name		= "Alias shadows";
	m_misc_menu.shadows_list.generic.callback	= ShadowFunc;
	m_misc_menu.shadows_list.itemnames			= shadow_names;
	m_misc_menu.shadows_list.generic.statusbar	= "Entity Shadows options";

	m_misc_menu.screenshot_list.generic.type		= UITYPE_SPINCONTROL;
	m_misc_menu.screenshot_list.generic.x			= 0;
	m_misc_menu.screenshot_list.generic.y			= y += UIFT_SIZEINC;
	m_misc_menu.screenshot_list.generic.name		= "Screenshot type";
	m_misc_menu.screenshot_list.generic.callback	= ScreenshotFunc;
	m_misc_menu.screenshot_list.itemnames			= screenshot_names;
	m_misc_menu.screenshot_list.generic.statusbar	= "Selects screenshot output format";

	m_misc_menu.jpgquality_slider.generic.type		= UITYPE_SLIDER;
	m_misc_menu.jpgquality_slider.generic.x			= 0;
	m_misc_menu.jpgquality_slider.generic.y			= y += UIFT_SIZEINC;
	m_misc_menu.jpgquality_slider.generic.name		= "JPG screenshot quality";
	m_misc_menu.jpgquality_slider.generic.callback	= JPGQualityFunc;
	m_misc_menu.jpgquality_slider.minvalue			= 0;
	m_misc_menu.jpgquality_slider.maxvalue			= 20;
	m_misc_menu.jpgquality_slider.generic.statusbar	= "JPG Screenshot Quality";
	m_misc_menu.jpgquality_amount.generic.type		= UITYPE_ACTION;
	m_misc_menu.jpgquality_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_misc_menu.jpgquality_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_misc_menu.jpgquality_amount.generic.y			= y;

	m_misc_menu.flashblend_list.generic.type		= UITYPE_SPINCONTROL;
	m_misc_menu.flashblend_list.generic.x			= 0;
	m_misc_menu.flashblend_list.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_misc_menu.flashblend_list.generic.name		= "Flashblend";
	m_misc_menu.flashblend_list.generic.callback	= FlashBlendFunc;
	m_misc_menu.flashblend_list.itemnames			= flashblend_names;
	m_misc_menu.flashblend_list.generic.statusbar	= "Dynamic light style";

	m_misc_menu.back_action.generic.type		= UITYPE_ACTION;
	m_misc_menu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_misc_menu.back_action.generic.x			= 0;
	m_misc_menu.back_action.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCLG;
	m_misc_menu.back_action.generic.name		= "< Back";
	m_misc_menu.back_action.generic.callback	= Back_Menu;
	m_misc_menu.back_action.generic.statusbar	= "Back a menu";

	MiscMenu_SetValues ();

	UI_AddItem (&m_misc_menu.framework,			&m_misc_menu.banner);
	UI_AddItem (&m_misc_menu.framework,			&m_misc_menu.header);

	UI_AddItem (&m_misc_menu.framework,			&m_misc_menu.defaultpaks_toggle);

	UI_AddItem (&m_misc_menu.framework,			&m_misc_menu.shadows_list);
	UI_AddItem (&m_misc_menu.framework,			&m_misc_menu.screenshot_list);
	UI_AddItem (&m_misc_menu.framework,			&m_misc_menu.jpgquality_slider);
	UI_AddItem (&m_misc_menu.framework,			&m_misc_menu.jpgquality_amount);

	UI_AddItem (&m_misc_menu.framework,			&m_misc_menu.flashblend_list);

	UI_AddItem (&m_misc_menu.framework,			&m_misc_menu.back_action);

	UI_Center (&m_misc_menu.framework);
	UI_Setup (&m_misc_menu.framework);
}


/*
=============
MiscMenu_Draw
=============
*/
void MiscMenu_Draw (void) {
	UI_Center (&m_misc_menu.framework);
	UI_Setup (&m_misc_menu.framework);

	UI_AdjustTextCursor (&m_misc_menu.framework, 1);
	UI_Draw (&m_misc_menu.framework);
}


/*
=============
MiscMenu_Key
=============
*/
const char *MiscMenu_Key (int key) {
	return DefaultMenu_Key (&m_misc_menu.framework, key);
}


/*
=============
UI_MiscMenu_f
=============
*/
void UI_MiscMenu_f (void) {
	MiscMenu_Init ();
	UI_PushMenu (&m_misc_menu.framework, MiscMenu_Draw, MiscMenu_Key);
}
