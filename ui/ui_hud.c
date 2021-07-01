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

	HUD MENU

=======================================================================
*/

typedef struct m_hudmenu_s {
	uiFrameWork_t	framework;

	uiImage_t		banner;
	uiAction_t		header;

	uiList_t		showfps_toggle;
	uiList_t		showping_toggle;
	uiList_t		showtime_toggle;
	uiList_t		demomsg_toggle;

	uiSlider_t		hudscale_slider;
	uiAction_t		hudscale_amount;
	uiSlider_t		hudalpha_slider;
	uiAction_t		hudalpha_amount;

	uiList_t		netgraph_toggle;
	uiSlider_t		netgraph_alpha_slider;
	uiAction_t		netgraph_alpha_amount;

	uiAction_t		back_action;
} m_hudmenu_t;

m_hudmenu_t	m_hud_menu;

static void ShowFPSFunc (void *unused) {
	uii.Cvar_SetValue ("cl_showfps", m_hud_menu.showfps_toggle.curValue);
}

static void ShowPINGFunc (void *unused) {
	uii.Cvar_SetValue ("cl_showping", m_hud_menu.showping_toggle.curValue);
}

static void ShowTIMEFunc (void *unused){
	uii.Cvar_SetValue ("cl_showtime", m_hud_menu.showtime_toggle.curValue);
}

static void DemoMSGFunc (void *unused) {
	uii.Cvar_SetValue ("scr_demomsg", m_hud_menu.demomsg_toggle.curValue);
}

static void HudScaleFunc (void *unused) {
	uii.Cvar_SetValue ("r_hudscale", m_hud_menu.hudscale_slider.curValue * 0.25);
	m_hud_menu.hudscale_amount.generic.name = uii.Cvar_VariableString ("r_hudscale");
}

static void HudAlphaFunc (void *unused) {
	uii.Cvar_SetValue ("scr_hudalpha", m_hud_menu.hudalpha_slider.curValue * 0.1);
	m_hud_menu.hudalpha_amount.generic.name = uii.Cvar_VariableString ("scr_hudalpha");
}

static void NetgraphToggleFunc (void *unused) {
	uii.Cvar_SetValue ("netgraph", m_hud_menu.netgraph_toggle.curValue);
}

static void NetgraphAlphaFunc (void *unused) {
	uii.Cvar_SetValue ("scr_graphalpha", m_hud_menu.netgraph_alpha_slider.curValue * 0.1);
	m_hud_menu.netgraph_alpha_amount.generic.name = uii.Cvar_VariableString ("scr_graphalpha");
}


/*
=============
HUDMenu_SetValues
=============
*/
static void HUDMenu_SetValues (void) {
	uii.Cvar_SetValue ("cl_showfps",			clamp (uii.Cvar_VariableInteger ("cl_showfps"), 0, 1));
	m_hud_menu.showfps_toggle.curValue			= uii.Cvar_VariableInteger ("cl_showfps");

	uii.Cvar_SetValue ("cl_showping",			clamp (uii.Cvar_VariableInteger ("cl_showping"), 0, 1));
	m_hud_menu.showping_toggle.curValue			= uii.Cvar_VariableInteger ("cl_showping");

	uii.Cvar_SetValue ("cl_showtime",			clamp (uii.Cvar_VariableInteger ("cl_showtime"), 0, 1));
	m_hud_menu.showtime_toggle.curValue			= uii.Cvar_VariableInteger ("cl_showtime");

	uii.Cvar_SetValue ("scr_demomsg",			clamp (uii.Cvar_VariableInteger ("scr_demomsg"), 0, 1));
	m_hud_menu.demomsg_toggle.curValue			= uii.Cvar_VariableInteger ("scr_demomsg");

	m_hud_menu.hudscale_slider.curValue			= uii.Cvar_VariableValue ("r_hudscale") * 4;
	m_hud_menu.hudscale_amount.generic.name		= uii.Cvar_VariableString ("r_hudscale");

	m_hud_menu.hudalpha_slider.curValue			= uii.Cvar_VariableValue ("scr_hudalpha") * 10;
	m_hud_menu.hudalpha_amount.generic.name		= uii.Cvar_VariableString ("scr_hudalpha");

	uii.Cvar_SetValue ("netgraph",				clamp (uii.Cvar_VariableInteger ("netgraph"), 0, 1));
	m_hud_menu.netgraph_toggle.curValue			= uii.Cvar_VariableInteger ("netgraph");

	m_hud_menu.netgraph_alpha_slider.curValue		= uii.Cvar_VariableValue ("scr_graphalpha") * 10;
	m_hud_menu.netgraph_alpha_amount.generic.name	= uii.Cvar_VariableString ("scr_graphalpha");
}


/*
=============
HUDMenu_Init
=============
*/
void HUDMenu_Init (void) {
	static char *onoff_names[] = {
		"off",
		"on",
		0
	};

	float	y;

	m_hud_menu.framework.x			= uis.vidWidth * 0.5;
	m_hud_menu.framework.y			= 0;
	m_hud_menu.framework.numItems	= 0;

	UI_Banner (&m_hud_menu.banner, uiMedia.optionsBanner, &y);

	m_hud_menu.header.generic.type			= UITYPE_ACTION;
	m_hud_menu.header.generic.flags			= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_hud_menu.header.generic.x				= 0;
	m_hud_menu.header.generic.y				= y += UIFT_SIZEINC;
	m_hud_menu.header.generic.name			= "HUD Settings";

	m_hud_menu.showfps_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_hud_menu.showfps_toggle.generic.x			= 0;
	m_hud_menu.showfps_toggle.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_hud_menu.showfps_toggle.generic.name		= "Framerate display";
	m_hud_menu.showfps_toggle.generic.callBack	= ShowFPSFunc;
	m_hud_menu.showfps_toggle.itemNames			= onoff_names;
	m_hud_menu.showfps_toggle.generic.statusBar	= "Framerate Display";

	m_hud_menu.showping_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_hud_menu.showping_toggle.generic.x			= 0;
	m_hud_menu.showping_toggle.generic.y			= y += UIFT_SIZEINC;
	m_hud_menu.showping_toggle.generic.name			= "Ping display";
	m_hud_menu.showping_toggle.generic.callBack		= ShowPINGFunc;
	m_hud_menu.showping_toggle.itemNames			= onoff_names;
	m_hud_menu.showping_toggle.generic.statusBar	= "Ping Display";

	m_hud_menu.showtime_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_hud_menu.showtime_toggle.generic.x			= 0;
	m_hud_menu.showtime_toggle.generic.y			= y += UIFT_SIZEINC;
	m_hud_menu.showtime_toggle.generic.name			= "Map timer";
	m_hud_menu.showtime_toggle.generic.callBack		= ShowTIMEFunc;
	m_hud_menu.showtime_toggle.itemNames			= onoff_names;
	m_hud_menu.showtime_toggle.generic.statusBar	= "Map Timer";

	m_hud_menu.demomsg_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_hud_menu.demomsg_toggle.generic.x			= 0;
	m_hud_menu.demomsg_toggle.generic.y			= y += UIFT_SIZEINC;
	m_hud_menu.demomsg_toggle.generic.name		= "Demo message";
	m_hud_menu.demomsg_toggle.generic.callBack	= DemoMSGFunc;
	m_hud_menu.demomsg_toggle.itemNames			= onoff_names;
	m_hud_menu.demomsg_toggle.generic.statusBar	= "Demo playing bottom status-bar notification";

	m_hud_menu.hudscale_slider.generic.type			= UITYPE_SLIDER;
	m_hud_menu.hudscale_slider.generic.x			= 0;
	m_hud_menu.hudscale_slider.generic.y			= y += (UIFT_SIZEINC*2);
	m_hud_menu.hudscale_slider.generic.name			= "HUD scale";
	m_hud_menu.hudscale_slider.generic.callBack		= HudScaleFunc;
	m_hud_menu.hudscale_slider.minValue				= 1;
	m_hud_menu.hudscale_slider.maxValue				= 12;
	m_hud_menu.hudscale_slider.generic.statusBar	= "HUD font size scaling";
	m_hud_menu.hudscale_amount.generic.type			= UITYPE_ACTION;
	m_hud_menu.hudscale_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_hud_menu.hudscale_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_hud_menu.hudscale_amount.generic.y			= y;

	m_hud_menu.hudalpha_slider.generic.type			= UITYPE_SLIDER;
	m_hud_menu.hudalpha_slider.generic.x			= 0;
	m_hud_menu.hudalpha_slider.generic.y			= y += UIFT_SIZEINC;
	m_hud_menu.hudalpha_slider.generic.name			= "HUD alpha";
	m_hud_menu.hudalpha_slider.generic.callBack		= HudAlphaFunc;
	m_hud_menu.hudalpha_slider.minValue				= 0;
	m_hud_menu.hudalpha_slider.maxValue				= 10;
	m_hud_menu.hudalpha_slider.generic.statusBar	= "HUD alpha transparency";
	m_hud_menu.hudalpha_amount.generic.type			= UITYPE_ACTION;
	m_hud_menu.hudalpha_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_hud_menu.hudalpha_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_hud_menu.hudalpha_amount.generic.y			= y;

	m_hud_menu.netgraph_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_hud_menu.netgraph_toggle.generic.x			= 0;
	m_hud_menu.netgraph_toggle.generic.y			= y += (UIFT_SIZEINC*2);
	m_hud_menu.netgraph_toggle.generic.name			= "Netgraph";
	m_hud_menu.netgraph_toggle.generic.callBack		= NetgraphToggleFunc;
	m_hud_menu.netgraph_toggle.itemNames			= onoff_names;
	m_hud_menu.netgraph_toggle.generic.statusBar	= "Toggles showing the lag netgraph";

	m_hud_menu.netgraph_alpha_slider.generic.type		= UITYPE_SLIDER;
	m_hud_menu.netgraph_alpha_slider.generic.x			= 0;
	m_hud_menu.netgraph_alpha_slider.generic.y			= y += UIFT_SIZEINC;
	m_hud_menu.netgraph_alpha_slider.generic.name		= "Netgraph alpha";
	m_hud_menu.netgraph_alpha_slider.generic.callBack	= NetgraphAlphaFunc;
	m_hud_menu.netgraph_alpha_slider.minValue			= 0;
	m_hud_menu.netgraph_alpha_slider.maxValue			= 10;
	m_hud_menu.netgraph_alpha_slider.generic.statusBar	= "Netgraph alpha transparency";
	m_hud_menu.netgraph_alpha_amount.generic.type		= UITYPE_ACTION;
	m_hud_menu.netgraph_alpha_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_hud_menu.netgraph_alpha_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_hud_menu.netgraph_alpha_amount.generic.y			= y;

	m_hud_menu.back_action.generic.type			= UITYPE_ACTION;
	m_hud_menu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_hud_menu.back_action.generic.x			= 0;
	m_hud_menu.back_action.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCLG;
	m_hud_menu.back_action.generic.name			= "< Back";
	m_hud_menu.back_action.generic.callBack		= Back_Menu;
	m_hud_menu.back_action.generic.statusBar	= "Back a menu";

	HUDMenu_SetValues ();

	UI_AddItem (&m_hud_menu.framework,			&m_hud_menu.banner);
	UI_AddItem (&m_hud_menu.framework,			&m_hud_menu.header);

	UI_AddItem (&m_hud_menu.framework,			&m_hud_menu.showfps_toggle);
	UI_AddItem (&m_hud_menu.framework,			&m_hud_menu.showping_toggle);
	UI_AddItem (&m_hud_menu.framework,			&m_hud_menu.showtime_toggle);
	UI_AddItem (&m_hud_menu.framework,			&m_hud_menu.demomsg_toggle);

	UI_AddItem (&m_hud_menu.framework,			&m_hud_menu.hudscale_slider);
	UI_AddItem (&m_hud_menu.framework,			&m_hud_menu.hudscale_amount);
	UI_AddItem (&m_hud_menu.framework,			&m_hud_menu.hudalpha_slider);
	UI_AddItem (&m_hud_menu.framework,			&m_hud_menu.hudalpha_amount);

	UI_AddItem (&m_hud_menu.framework,			&m_hud_menu.netgraph_toggle);
	UI_AddItem (&m_hud_menu.framework,			&m_hud_menu.netgraph_alpha_slider);
	UI_AddItem (&m_hud_menu.framework,			&m_hud_menu.netgraph_alpha_amount);

	UI_AddItem (&m_hud_menu.framework,			&m_hud_menu.back_action);

	UI_Center (&m_hud_menu.framework);
	UI_Setup (&m_hud_menu.framework);
}


/*
=============
HUDMenu_Draw
=============
*/
void HUDMenu_Draw (void) {
	UI_Center (&m_hud_menu.framework);
	UI_Setup (&m_hud_menu.framework);

	UI_AdjustTextCursor (&m_hud_menu.framework, 1);
	UI_Draw (&m_hud_menu.framework);
}


/*
=============
HUDMenu_Key
=============
*/
struct sfx_s *HUDMenu_Key (int key) {
	return DefaultMenu_Key (&m_hud_menu.framework, key);
}


/*
=============
UI_HUDMenu_f
=============
*/
void UI_HUDMenu_f (void) {
	HUDMenu_Init ();
	UI_PushMenu (&m_hud_menu.framework, HUDMenu_Draw, HUDMenu_Key);
}
