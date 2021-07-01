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

	FOG MENU

=======================================================================
*/

typedef struct m_fogmenu_s {
	uiFrameWork_t	framework;

	uiImage_t		banner;
	uiAction_t		header;

	uiList_t		mode_list;
	uiSlider_t		start_slider;
	uiAction_t		start_amount;
	uiSlider_t		end_slider;
	uiAction_t		end_amount;
	uiSlider_t		density_slider;
	uiAction_t		density_amount;
	uiList_t		water_toggle;

	uiSlider_t		red_slider;
	uiAction_t		red_amount;
	uiSlider_t		green_slider;
	uiAction_t		green_amount;
	uiSlider_t		blue_slider;
	uiAction_t		blue_amount;

	uiAction_t		back_action;
} m_fogmenu_t;

m_fogmenu_t	m_fog_menu;

static void FogModeFunc (void *unused) {
	uii.Cvar_SetValue ("r_fog", m_fog_menu.mode_list.curValue);
}

static void FogStartFunc (void *unused) {
	uii.Cvar_SetValue ("r_fog_start", m_fog_menu.start_slider.curValue * 128.0);
	m_fog_menu.start_amount.generic.name = uii.Cvar_VariableString ("r_fog_start");
}

static void FogEndFunc (void *unused) {
	uii.Cvar_SetValue ("r_fog_end", m_fog_menu.end_slider.curValue * 256.0);
	m_fog_menu.end_amount.generic.name = uii.Cvar_VariableString ("r_fog_end");
}

static void FogDensityFunc (void *unused) {
	uii.Cvar_SetValue ("r_fog_density", m_fog_menu.density_slider.curValue * 0.0001);
	m_fog_menu.density_amount.generic.name = uii.Cvar_VariableString ("r_fog_density");
}

static void FogRedFunc (void *unused) {
	uii.Cvar_SetValue ("r_fog_red", m_fog_menu.red_slider.curValue * 0.1);
	m_fog_menu.red_amount.generic.name = uii.Cvar_VariableString ("r_fog_red");
}

static void FogGreenFunc (void *unused) {
	uii.Cvar_SetValue ("r_fog_green", m_fog_menu.green_slider.curValue * 0.1);
	m_fog_menu.green_amount.generic.name = uii.Cvar_VariableString ("r_fog_green");
}

static void FogBlueFunc (void *unused) {
	uii.Cvar_SetValue ("r_fog_blue", m_fog_menu.blue_slider.curValue * 0.1);
	m_fog_menu.blue_amount.generic.name = uii.Cvar_VariableString ("r_fog_blue");
}

static void FogWaterFunc (void *unused) {
	uii.Cvar_SetValue ("r_fog_water", m_fog_menu.water_toggle.curValue);
}


/*
=============
FogMenu_SetValues
=============
*/
static void FogMenu_SetValues (void) {
	uii.Cvar_SetValue ("r_fog",			clamp (uii.Cvar_VariableInteger ("r_fog"), 0, 3));
	m_fog_menu.mode_list.curValue		= uii.Cvar_VariableInteger ("r_fog");

	m_fog_menu.start_slider.curValue		= uii.Cvar_VariableValue ("r_fog_start") / 128.0;
	m_fog_menu.start_amount.generic.name	= uii.Cvar_VariableString ("r_fog_start");

	m_fog_menu.end_slider.curValue		= uii.Cvar_VariableValue ("r_fog_end") / 256.0;
	m_fog_menu.end_amount.generic.name	= uii.Cvar_VariableString ("r_fog_end");

	m_fog_menu.density_slider.curValue		= uii.Cvar_VariableValue ("r_fog_density") * 10000;
	m_fog_menu.density_amount.generic.name	= uii.Cvar_VariableString ("r_fog_density");

	m_fog_menu.red_slider.curValue		= uii.Cvar_VariableValue ("r_fog_red") * 10;
	m_fog_menu.red_amount.generic.name	= uii.Cvar_VariableString ("r_fog_red");

	m_fog_menu.green_slider.curValue		= uii.Cvar_VariableValue ("r_fog_green") * 10;
	m_fog_menu.green_amount.generic.name	= uii.Cvar_VariableString ("r_fog_green");

	m_fog_menu.blue_slider.curValue		= uii.Cvar_VariableValue ("r_fog_blue") * 10;
	m_fog_menu.blue_amount.generic.name = uii.Cvar_VariableString ("r_fog_blue");

	uii.Cvar_SetValue ("r_fog_water",	clamp (uii.Cvar_VariableInteger ("r_fog_water"), 0, 1));
	m_fog_menu.water_toggle.curValue	= uii.Cvar_VariableInteger ("r_fog_water");
}


/*
=============
FogMenu_Init
=============
*/
void FogMenu_Init (void) {
	static char *fogmode_names[] = {
		"off",
		"linear",
		"exponential",
		"exp^2",
		0
	};

	static char *onoff_names[] = {
		"off",
		"on",
		0
	};

	float	y;

	m_fog_menu.framework.x			= (uis.vidWidth*0.5);
	m_fog_menu.framework.y			= 0;
	m_fog_menu.framework.numItems	= 0;

	UI_Banner (&m_fog_menu.banner, uiMedia.optionsBanner, &y);

	m_fog_menu.header.generic.type			= UITYPE_ACTION;
	m_fog_menu.header.generic.flags			= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_fog_menu.header.generic.x				= 0;
	m_fog_menu.header.generic.y				= y += UIFT_SIZEINC;
	m_fog_menu.header.generic.name			= "Fog Settings";

	m_fog_menu.mode_list.generic.type		= UITYPE_SPINCONTROL;
	m_fog_menu.mode_list.generic.x			= 0;
	m_fog_menu.mode_list.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_fog_menu.mode_list.generic.name		= "Fog mode";
	m_fog_menu.mode_list.generic.callBack	= FogModeFunc;
	m_fog_menu.mode_list.itemNames			= fogmode_names;
	m_fog_menu.mode_list.generic.statusBar	= "Fog Mode";

	m_fog_menu.start_slider.generic.type		= UITYPE_SLIDER;
	m_fog_menu.start_slider.generic.x			= 0;
	m_fog_menu.start_slider.generic.y			= y += UIFT_SIZEINC;
	m_fog_menu.start_slider.generic.name		= "Linear start";
	m_fog_menu.start_slider.generic.callBack	= FogStartFunc;
	m_fog_menu.start_slider.minValue			= 0;
	m_fog_menu.start_slider.maxValue			= 32;
	m_fog_menu.start_slider.generic.statusBar	= "Fog Start (only used in linear)";
	m_fog_menu.start_amount.generic.type		= UITYPE_ACTION;
	m_fog_menu.start_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_fog_menu.start_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_fog_menu.start_amount.generic.y			= y;

	m_fog_menu.end_slider.generic.type		= UITYPE_SLIDER;
	m_fog_menu.end_slider.generic.x			= 0;
	m_fog_menu.end_slider.generic.y			= y += UIFT_SIZEINC;
	m_fog_menu.end_slider.generic.name		= "Linear end";
	m_fog_menu.end_slider.generic.callBack	= FogEndFunc;
	m_fog_menu.end_slider.minValue			= 0;
	m_fog_menu.end_slider.maxValue			= 32;
	m_fog_menu.end_slider.generic.statusBar	= "Fog End (only used in linear)";
	m_fog_menu.end_amount.generic.type		= UITYPE_ACTION;
	m_fog_menu.end_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_fog_menu.end_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_fog_menu.end_amount.generic.y			= y;

	m_fog_menu.density_slider.generic.type		= UITYPE_SLIDER;
	m_fog_menu.density_slider.generic.x			= 0;
	m_fog_menu.density_slider.generic.y			= y += UIFT_SIZEINC;
	m_fog_menu.density_slider.generic.name		= "Exp density";
	m_fog_menu.density_slider.generic.callBack	= FogDensityFunc;
	m_fog_menu.density_slider.minValue			= 1;
	m_fog_menu.density_slider.maxValue			= 10;
	m_fog_menu.density_slider.generic.statusBar	= "Fog Density (used in exp and exp^2)";
	m_fog_menu.density_amount.generic.type		= UITYPE_ACTION;
	m_fog_menu.density_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_fog_menu.density_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_fog_menu.density_amount.generic.y			= y;

	m_fog_menu.red_slider.generic.type			= UITYPE_SLIDER;
	m_fog_menu.red_slider.generic.x				= 0;
	m_fog_menu.red_slider.generic.y				= y += (UIFT_SIZEINC*2);
	m_fog_menu.red_slider.generic.name			= "Red color";
	m_fog_menu.red_slider.generic.callBack		= FogRedFunc;
	m_fog_menu.red_slider.minValue				= 0;
	m_fog_menu.red_slider.maxValue				= 10;
	m_fog_menu.red_slider.generic.statusBar		= "Fog Red Amount";
	m_fog_menu.red_amount.generic.type			= UITYPE_ACTION;
	m_fog_menu.red_amount.generic.flags			= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_fog_menu.red_amount.generic.x				= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_fog_menu.red_amount.generic.y				= y;

	m_fog_menu.green_slider.generic.type		= UITYPE_SLIDER;
	m_fog_menu.green_slider.generic.x			= 0;
	m_fog_menu.green_slider.generic.y			= y += UIFT_SIZEINC;
	m_fog_menu.green_slider.generic.name		= "Green color";
	m_fog_menu.green_slider.generic.callBack	= FogGreenFunc;
	m_fog_menu.green_slider.minValue			= 0;
	m_fog_menu.green_slider.maxValue			= 10;
	m_fog_menu.green_slider.generic.statusBar	= "Fog Green Amount";
	m_fog_menu.green_amount.generic.type		= UITYPE_ACTION;
	m_fog_menu.green_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_fog_menu.green_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_fog_menu.green_amount.generic.y			= y;

	m_fog_menu.blue_slider.generic.type			= UITYPE_SLIDER;
	m_fog_menu.blue_slider.generic.x			= 0;
	m_fog_menu.blue_slider.generic.y			= y += UIFT_SIZEINC;
	m_fog_menu.blue_slider.generic.name			= "Blue color";
	m_fog_menu.blue_slider.generic.callBack		= FogBlueFunc;
	m_fog_menu.blue_slider.minValue				= 0;
	m_fog_menu.blue_slider.maxValue				= 10;
	m_fog_menu.blue_slider.generic.statusBar	= "Fog Blue Amount";
	m_fog_menu.blue_amount.generic.type			= UITYPE_ACTION;
	m_fog_menu.blue_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_fog_menu.blue_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_fog_menu.blue_amount.generic.y			= y;

	m_fog_menu.water_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_fog_menu.water_toggle.generic.x			= 0;
	m_fog_menu.water_toggle.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_fog_menu.water_toggle.generic.name		= "Water fogging";
	m_fog_menu.water_toggle.generic.callBack	= FogWaterFunc;
	m_fog_menu.water_toggle.itemNames			= onoff_names;
	m_fog_menu.water_toggle.generic.statusBar	= "water fogging";

	m_fog_menu.back_action.generic.type			= UITYPE_ACTION;
	m_fog_menu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_fog_menu.back_action.generic.x			= 0;
	m_fog_menu.back_action.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCLG;
	m_fog_menu.back_action.generic.name			= "< Back";
	m_fog_menu.back_action.generic.callBack		= Back_Menu;
	m_fog_menu.back_action.generic.statusBar	= "Back a menu";

	FogMenu_SetValues ();

	UI_AddItem (&m_fog_menu.framework,			&m_fog_menu.banner);
	UI_AddItem (&m_fog_menu.framework,			&m_fog_menu.header);

	UI_AddItem (&m_fog_menu.framework,			&m_fog_menu.mode_list);
	UI_AddItem (&m_fog_menu.framework,			&m_fog_menu.start_slider);
	UI_AddItem (&m_fog_menu.framework,			&m_fog_menu.start_amount);
	UI_AddItem (&m_fog_menu.framework,			&m_fog_menu.end_slider);
	UI_AddItem (&m_fog_menu.framework,			&m_fog_menu.end_amount);
	UI_AddItem (&m_fog_menu.framework,			&m_fog_menu.density_slider);
	UI_AddItem (&m_fog_menu.framework,			&m_fog_menu.density_amount);

	UI_AddItem (&m_fog_menu.framework,			&m_fog_menu.red_slider);
	UI_AddItem (&m_fog_menu.framework,			&m_fog_menu.red_amount);
	UI_AddItem (&m_fog_menu.framework,			&m_fog_menu.green_slider);
	UI_AddItem (&m_fog_menu.framework,			&m_fog_menu.green_amount);
	UI_AddItem (&m_fog_menu.framework,			&m_fog_menu.blue_slider);
	UI_AddItem (&m_fog_menu.framework,			&m_fog_menu.blue_amount);

	UI_AddItem (&m_fog_menu.framework,			&m_fog_menu.water_toggle);

	UI_AddItem (&m_fog_menu.framework,			&m_fog_menu.back_action);

	UI_Center (&m_fog_menu.framework);
	UI_Setup (&m_fog_menu.framework);
}


/*
=============
FogMenu_Draw
=============
*/
void FogMenu_Draw (void) {
	UI_Center (&m_fog_menu.framework);
	UI_Setup (&m_fog_menu.framework);

	UI_AdjustTextCursor (&m_fog_menu.framework, 1);
	UI_Draw (&m_fog_menu.framework);
}


/*
=============
FogMenu_Key
=============
*/
struct sfx_s *FogMenu_Key (int key) {
	return DefaultMenu_Key (&m_fog_menu.framework, key);
}


/*
=============
UI_FogMenu_f
=============
*/
void UI_FogMenu_f (void) {
	FogMenu_Init ();
	UI_PushMenu (&m_fog_menu.framework, FogMenu_Draw, FogMenu_Key);
}
