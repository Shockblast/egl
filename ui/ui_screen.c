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

	SCREEN MENU

=======================================================================
*/

typedef struct m_screenmenu_s {
	uiFrameWork_t	framework;

	uiImage_t		banner;

	//
	// console
	//

	uiAction_t		con_header;

	uiList_t		con_clock_toggle;
	uiList_t		con_notfade_toggle;
	uiList_t		con_notlarge_toggle;
	uiList_t		con_timestamp_list;

	uiSlider_t		con_notlines_slider;
	uiAction_t		con_notlines_amount;
	uiSlider_t		con_alpha_slider;
	uiAction_t		con_alpha_amount;
	uiSlider_t		con_drop_slider;
	uiAction_t		con_drop_amount;
	uiSlider_t		con_scroll_slider;
	uiAction_t		con_scroll_amount;
	uiSlider_t		con_fontscale_slider;
	uiAction_t		con_fontscale_amount;

	//
	// crosshair
	//

	uiAction_t		ch_header;

	uiList_t		ch_number_list;

	uiSlider_t		ch_alpha_slider;
	uiAction_t		ch_alpha_amount;
	uiSlider_t		ch_pulse_slider;
	uiAction_t		ch_pulse_amount;
	uiSlider_t		ch_scale_slider;
	uiAction_t		ch_scale_amount;

	uiSlider_t		ch_red_slider;
	uiAction_t		ch_red_amount;
	uiSlider_t		ch_green_slider;
	uiAction_t		ch_green_amount;
	uiSlider_t		ch_blue_slider;
	uiAction_t		ch_blue_amount;

	uiAction_t		back_action;
} m_screenmenu_t;

m_screenmenu_t	m_screen_menu;

//
// console
//

static void ConsoleClockFunc (void *unused) {
	uii.Cvar_SetValue ("con_clock", m_screen_menu.con_clock_toggle.curValue);
}

static void ConsoleNotFadeFunc (void *unused) {
	uii.Cvar_SetValue ("con_notifyfade", m_screen_menu.con_notfade_toggle.curValue);
}

static void ConsoleNotLargeFunc (void *unused) {
	uii.Cvar_SetValue ("con_notifylarge", m_screen_menu.con_notlarge_toggle.curValue);
}

static void ConsoleTimeStampFunc (void *unused) {
	uii.Cvar_SetValue ("cl_timestamp", m_screen_menu.con_timestamp_list.curValue);
}

static void ConsoleNotLinesFunc (void *unused) {
	uii.Cvar_SetValue ("con_notifylines", m_screen_menu.con_notlines_slider.curValue);
	m_screen_menu.con_notlines_amount.generic.name = uii.Cvar_VariableString ("con_notifylines");
}

static void ConsoleAlphaFunc (void *unused) {
	uii.Cvar_SetValue ("con_alpha", m_screen_menu.con_alpha_slider.curValue * 0.1);
	m_screen_menu.con_alpha_amount.generic.name = uii.Cvar_VariableString ("con_alpha");
}

static void ConsoleDropFunc (void *unused) {
	uii.Cvar_SetValue ("con_drop", m_screen_menu.con_drop_slider.curValue * 0.1);
	m_screen_menu.con_drop_amount.generic.name = uii.Cvar_VariableString ("con_drop");
}

static void ConsoleScrollFunc (void *unused) {
	uii.Cvar_SetValue ("con_scroll", m_screen_menu.con_scroll_slider.curValue);
	m_screen_menu.con_scroll_amount.generic.name = uii.Cvar_VariableString ("con_scroll");
}

static void ConsoleFontScaleFunc (void *unused) {
	uii.Cvar_SetValue ("r_fontscale", m_screen_menu.con_fontscale_slider.curValue * 0.25);
	m_screen_menu.con_fontscale_amount.generic.name = uii.Cvar_VariableString ("r_fontscale");
}

//
// crosshair
//

static void CrosshairFunc (void *unused) {
	uii.Cvar_SetValue ("crosshair", m_screen_menu.ch_number_list.curValue);
}

static void CrosshairAlphaFunc (void *unused) {
	uii.Cvar_SetValue ("ch_alpha", m_screen_menu.ch_alpha_slider.curValue * 0.1);
	m_screen_menu.ch_alpha_amount.generic.name = uii.Cvar_VariableString ("ch_alpha");
}

static void CrosshairPulseFunc (void *unused) {
	uii.Cvar_SetValue ("ch_pulse", m_screen_menu.ch_pulse_slider.curValue * 0.1);
	m_screen_menu.ch_pulse_amount.generic.name = uii.Cvar_VariableString ("ch_pulse");
}

static void CrosshairScaleFunc (void *unused) {
	uii.Cvar_SetValue ("ch_scale", m_screen_menu.ch_scale_slider.curValue * 0.1);
	m_screen_menu.ch_scale_amount.generic.name = uii.Cvar_VariableString ("ch_scale");
}

static void CrosshairRedFunc (void *unused) {
	uii.Cvar_SetValue ("ch_red", m_screen_menu.ch_red_slider.curValue * 0.1);
	m_screen_menu.ch_red_amount.generic.name = uii.Cvar_VariableString ("ch_red");
}

static void CrosshairGreenFunc (void *unused) {
	uii.Cvar_SetValue ("ch_green", m_screen_menu.ch_green_slider.curValue * 0.1);
	m_screen_menu.ch_green_amount.generic.name = uii.Cvar_VariableString ("ch_green");
}

static void CrosshairBlueFunc (void *unused) {
	uii.Cvar_SetValue ("ch_blue", m_screen_menu.ch_blue_slider.curValue * 0.1);
	m_screen_menu.ch_blue_amount.generic.name = uii.Cvar_VariableString ("ch_blue");
}


/*
=============
ScreenMenu_SetValues
=============
*/
static void ScreenMenu_SetValues (void) {
	//
	// console
	//

	uii.Cvar_SetValue ("con_clock",					clamp (uii.Cvar_VariableInteger ("con_clock"), 0, 1));
	m_screen_menu.con_clock_toggle.curValue			= uii.Cvar_VariableInteger ("con_clock");

	uii.Cvar_SetValue ("con_notifyfade",			clamp (uii.Cvar_VariableInteger ("con_notifyfade"), 0, 1));
	m_screen_menu.con_notfade_toggle.curValue		= uii.Cvar_VariableInteger ("con_notifyfade");

	uii.Cvar_SetValue ("con_notifylarge",			clamp (uii.Cvar_VariableInteger ("con_notifylarge"), 0, 1));
	m_screen_menu.con_notlarge_toggle.curValue		= uii.Cvar_VariableInteger ("con_notifylarge");

	uii.Cvar_SetValue ("cl_timestamp",				clamp (uii.Cvar_VariableInteger ("cl_timestamp"), 0, 2));
	m_screen_menu.con_timestamp_list.curValue		= uii.Cvar_VariableInteger ("cl_timestamp");

	m_screen_menu.con_notlines_slider.curValue		= uii.Cvar_VariableValue ("con_notifylines");
	m_screen_menu.con_notlines_amount.generic.name	= uii.Cvar_VariableString ("con_notifylines");
	m_screen_menu.con_alpha_slider.curValue			= uii.Cvar_VariableValue ("con_alpha") * 10;
	m_screen_menu.con_alpha_amount.generic.name		= uii.Cvar_VariableString ("con_alpha");
	m_screen_menu.con_drop_slider.curValue			= uii.Cvar_VariableValue ("con_drop") * 10;
	m_screen_menu.con_drop_amount.generic.name		= uii.Cvar_VariableString ("con_drop");
	m_screen_menu.con_scroll_slider.curValue		= uii.Cvar_VariableValue ("con_scroll");
	m_screen_menu.con_scroll_amount.generic.name	= uii.Cvar_VariableString ("con_scroll");

	m_screen_menu.con_fontscale_slider.curValue		= uii.Cvar_VariableValue ("r_fontscale") * 4;
	m_screen_menu.con_fontscale_amount.generic.name	= uii.Cvar_VariableString ("r_fontscale");

	//
	// crosshair
	//

	m_screen_menu.ch_number_list.curValue	= uii.Cvar_VariableValue ("crosshair");

	m_screen_menu.ch_alpha_slider.curValue	= uii.Cvar_VariableValue ("ch_alpha") * 10;
	m_screen_menu.ch_alpha_amount.generic.name	= uii.Cvar_VariableString ("ch_alpha");
	m_screen_menu.ch_pulse_slider.curValue	= uii.Cvar_VariableValue ("ch_pulse") * 10;
	m_screen_menu.ch_pulse_amount.generic.name	= uii.Cvar_VariableString ("ch_pulse");
	m_screen_menu.ch_scale_slider.curValue	= uii.Cvar_VariableValue ("ch_scale")* 10;
	m_screen_menu.ch_scale_amount.generic.name	= uii.Cvar_VariableString ("ch_scale");

	m_screen_menu.ch_red_slider.curValue	= uii.Cvar_VariableValue ("ch_red") * 10;
	m_screen_menu.ch_red_amount.generic.name	= uii.Cvar_VariableString ("ch_red");
	m_screen_menu.ch_green_slider.curValue	= uii.Cvar_VariableValue ("ch_green") * 10;
	m_screen_menu.ch_green_amount.generic.name	= uii.Cvar_VariableString ("ch_green");
	m_screen_menu.ch_blue_slider.curValue	= uii.Cvar_VariableValue ("ch_blue") * 10;
	m_screen_menu.ch_blue_amount.generic.name	= uii.Cvar_VariableString ("ch_blue");
}


/*
=============
ScreenMenu_Init
=============
*/
void ScreenMenu_Init (void) {
	static char *crosshair_names[] = {
		"none",
		"1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16","17","18","19","20",
		0
	}; // FIXME: find out how many ch's there are and generate this

	static char *onoff_names[] = {
		"off",
		"on",
		0
	};

	static char *timestamp_names[] = {
		"off",
		"talk text only",
		"all messages",
		0
	};

	float	y;

	m_screen_menu.framework.x			= uis.vidWidth * 0.5;
	m_screen_menu.framework.y			= 0;
	m_screen_menu.framework.numItems	= 0;

	UI_Banner (&m_screen_menu.banner, uiMedia.optionsBanner, &y);

	//
	// console
	//

	m_screen_menu.con_header.generic.type		= UITYPE_ACTION;
	m_screen_menu.con_header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_screen_menu.con_header.generic.x			= 0;
	m_screen_menu.con_header.generic.y			= y += UIFT_SIZEINC;
	m_screen_menu.con_header.generic.name		= "Console";

	m_screen_menu.con_clock_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_screen_menu.con_clock_toggle.generic.x			= 0;
	m_screen_menu.con_clock_toggle.generic.y			= y += UIFT_SIZEINCMED + UIFT_SIZEINC;
	m_screen_menu.con_clock_toggle.generic.name			= "Console clock";
	m_screen_menu.con_clock_toggle.generic.callBack		= ConsoleClockFunc;
	m_screen_menu.con_clock_toggle.itemNames			= onoff_names;
	m_screen_menu.con_clock_toggle.generic.statusBar	= "Console Clock";

	m_screen_menu.con_notfade_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_screen_menu.con_notfade_toggle.generic.x			= 0;
	m_screen_menu.con_notfade_toggle.generic.y			= y += UIFT_SIZEINC;
	m_screen_menu.con_notfade_toggle.generic.name		= "Notify fading";
	m_screen_menu.con_notfade_toggle.generic.callBack	= ConsoleNotFadeFunc;
	m_screen_menu.con_notfade_toggle.itemNames			= onoff_names;
	m_screen_menu.con_notfade_toggle.generic.statusBar	= "Notifyline Fading";

	m_screen_menu.con_notlarge_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_screen_menu.con_notlarge_toggle.generic.x			= 0;
	m_screen_menu.con_notlarge_toggle.generic.y			= y += UIFT_SIZEINC;
	m_screen_menu.con_notlarge_toggle.generic.name		= "Large notify";
	m_screen_menu.con_notlarge_toggle.generic.callBack	= ConsoleNotLargeFunc;
	m_screen_menu.con_notlarge_toggle.itemNames			= onoff_names;
	m_screen_menu.con_notlarge_toggle.generic.statusBar	= "Larger notify lines";

	m_screen_menu.con_timestamp_list.generic.type		= UITYPE_SPINCONTROL;
	m_screen_menu.con_timestamp_list.generic.x			= 0;
	m_screen_menu.con_timestamp_list.generic.y			= y += UIFT_SIZEINC;
	m_screen_menu.con_timestamp_list.generic.name		= "Timestamping";
	m_screen_menu.con_timestamp_list.generic.callBack	= ConsoleTimeStampFunc;
	m_screen_menu.con_timestamp_list.itemNames			= timestamp_names;
	m_screen_menu.con_timestamp_list.generic.statusBar	= "Message Timestamping";

	m_screen_menu.con_notlines_slider.generic.type		= UITYPE_SLIDER;
	m_screen_menu.con_notlines_slider.generic.x			= 0;
	m_screen_menu.con_notlines_slider.generic.y			= y += (UIFT_SIZEINC*2);
	m_screen_menu.con_notlines_slider.generic.name		= "Notify lines";
	m_screen_menu.con_notlines_slider.generic.callBack	= ConsoleNotLinesFunc;
	m_screen_menu.con_notlines_slider.minValue			= 0;
	m_screen_menu.con_notlines_slider.maxValue			= 10;
	m_screen_menu.con_notlines_slider.generic.statusBar	= "Maximum Notifylines";
	m_screen_menu.con_notlines_amount.generic.type		= UITYPE_ACTION;
	m_screen_menu.con_notlines_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_screen_menu.con_notlines_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_screen_menu.con_notlines_amount.generic.y			= y;

	m_screen_menu.con_alpha_slider.generic.type			= UITYPE_SLIDER;
	m_screen_menu.con_alpha_slider.generic.x			= 0;
	m_screen_menu.con_alpha_slider.generic.y			= y += UIFT_SIZEINC;
	m_screen_menu.con_alpha_slider.generic.name			= "Console alpha";
	m_screen_menu.con_alpha_slider.generic.callBack		= ConsoleAlphaFunc;
	m_screen_menu.con_alpha_slider.minValue				= 0;
	m_screen_menu.con_alpha_slider.maxValue				= 10;
	m_screen_menu.con_alpha_slider.generic.statusBar	= "Console Alpha";
	m_screen_menu.con_alpha_amount.generic.type			= UITYPE_ACTION;
	m_screen_menu.con_alpha_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_screen_menu.con_alpha_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_screen_menu.con_alpha_amount.generic.y			= y;

	m_screen_menu.con_drop_slider.generic.type		= UITYPE_SLIDER;
	m_screen_menu.con_drop_slider.generic.x			= 0;
	m_screen_menu.con_drop_slider.generic.y			= y += UIFT_SIZEINC;
	m_screen_menu.con_drop_slider.generic.name		= "Drop height";
	m_screen_menu.con_drop_slider.generic.callBack	= ConsoleDropFunc;
	m_screen_menu.con_drop_slider.minValue			= 0;
	m_screen_menu.con_drop_slider.maxValue			= 10;
	m_screen_menu.con_drop_slider.generic.statusBar	= "Console Drop Height";
	m_screen_menu.con_drop_amount.generic.type		= UITYPE_ACTION;
	m_screen_menu.con_drop_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_screen_menu.con_drop_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_screen_menu.con_drop_amount.generic.y			= y;

	m_screen_menu.con_scroll_slider.generic.type		= UITYPE_SLIDER;
	m_screen_menu.con_scroll_slider.generic.x			= 0;
	m_screen_menu.con_scroll_slider.generic.y			= y += UIFT_SIZEINC;
	m_screen_menu.con_scroll_slider.generic.name		= "Scroll lines";
	m_screen_menu.con_scroll_slider.generic.callBack	= ConsoleScrollFunc;
	m_screen_menu.con_scroll_slider.minValue			= 0;
	m_screen_menu.con_scroll_slider.maxValue			= 10;
	m_screen_menu.con_scroll_slider.generic.statusBar	= "Scroll console lines with PGUP/DN or MWHEELUP/DN";
	m_screen_menu.con_scroll_amount.generic.type		= UITYPE_ACTION;
	m_screen_menu.con_scroll_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_screen_menu.con_scroll_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_screen_menu.con_scroll_amount.generic.y			= y;

	m_screen_menu.con_fontscale_slider.generic.type			= UITYPE_SLIDER;
	m_screen_menu.con_fontscale_slider.generic.x			= 0;
	m_screen_menu.con_fontscale_slider.generic.y			= y += UIFT_SIZEINC;
	m_screen_menu.con_fontscale_slider.generic.name			= "Font scale";
	m_screen_menu.con_fontscale_slider.generic.callBack		= ConsoleFontScaleFunc;
	m_screen_menu.con_fontscale_slider.minValue				= 1;
	m_screen_menu.con_fontscale_slider.maxValue				= 12;
	m_screen_menu.con_fontscale_slider.generic.statusBar	= "Font size scaling";
	m_screen_menu.con_fontscale_amount.generic.type		= UITYPE_ACTION;
	m_screen_menu.con_fontscale_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_screen_menu.con_fontscale_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_screen_menu.con_fontscale_amount.generic.y			= y;

	//
	// crosshair
	//

	m_screen_menu.ch_header.generic.type		= UITYPE_ACTION;
	m_screen_menu.ch_header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_screen_menu.ch_header.generic.x			= 0;
	m_screen_menu.ch_header.generic.y			= y += UIFT_SIZEINC*2;
	m_screen_menu.ch_header.generic.name		= "Crosshair";

	m_screen_menu.ch_number_list.generic.type		= UITYPE_SPINCONTROL;
	m_screen_menu.ch_number_list.generic.x			= 0;
	m_screen_menu.ch_number_list.generic.y			= y += UIFT_SIZEINCMED + UIFT_SIZEINC;
	m_screen_menu.ch_number_list.generic.name		= "Crosshair";
	m_screen_menu.ch_number_list.generic.callBack	= CrosshairFunc;
	m_screen_menu.ch_number_list.itemNames			= crosshair_names;
	m_screen_menu.ch_number_list.generic.statusBar	= "Crosshair Number";

	m_screen_menu.ch_alpha_slider.generic.type		= UITYPE_SLIDER;
	m_screen_menu.ch_alpha_slider.generic.x			= 0;
	m_screen_menu.ch_alpha_slider.generic.y			= y += (UIFT_SIZEINC*2);
	m_screen_menu.ch_alpha_slider.generic.name		= "Alpha";
	m_screen_menu.ch_alpha_slider.generic.callBack	= CrosshairAlphaFunc;
	m_screen_menu.ch_alpha_slider.minValue			= 0;
	m_screen_menu.ch_alpha_slider.maxValue			= 10;
	m_screen_menu.ch_alpha_slider.generic.statusBar	= "Crosshair Alpha";
	m_screen_menu.ch_alpha_amount.generic.type		= UITYPE_ACTION;
	m_screen_menu.ch_alpha_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_screen_menu.ch_alpha_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_screen_menu.ch_alpha_amount.generic.y			= y;

	m_screen_menu.ch_pulse_slider.generic.type		= UITYPE_SLIDER;
	m_screen_menu.ch_pulse_slider.generic.x			= 0;
	m_screen_menu.ch_pulse_slider.generic.y			= y += UIFT_SIZEINC;
	m_screen_menu.ch_pulse_slider.generic.name		= "Pulse";
	m_screen_menu.ch_pulse_slider.generic.callBack	= CrosshairPulseFunc;
	m_screen_menu.ch_pulse_slider.minValue			= 0;
	m_screen_menu.ch_pulse_slider.maxValue			= 20;
	m_screen_menu.ch_pulse_slider.generic.statusBar	= "Crosshair Alpha Pulsing";
	m_screen_menu.ch_pulse_amount.generic.type		= UITYPE_ACTION;
	m_screen_menu.ch_pulse_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_screen_menu.ch_pulse_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_screen_menu.ch_pulse_amount.generic.y			= y;

	m_screen_menu.ch_scale_slider.generic.type		= UITYPE_SLIDER;
	m_screen_menu.ch_scale_slider.generic.x			= 0;
	m_screen_menu.ch_scale_slider.generic.y			= y += UIFT_SIZEINC;
	m_screen_menu.ch_scale_slider.generic.name		= "Scale";
	m_screen_menu.ch_scale_slider.generic.callBack	= CrosshairScaleFunc;
	m_screen_menu.ch_scale_slider.minValue			= -10;
	m_screen_menu.ch_scale_slider.maxValue			= 20;
	m_screen_menu.ch_scale_slider.generic.statusBar	= "Crosshair Scale";
	m_screen_menu.ch_scale_amount.generic.type		= UITYPE_ACTION;
	m_screen_menu.ch_scale_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_screen_menu.ch_scale_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_screen_menu.ch_scale_amount.generic.y			= y;

	m_screen_menu.ch_red_slider.generic.type		= UITYPE_SLIDER;
	m_screen_menu.ch_red_slider.generic.x			= 0;
	m_screen_menu.ch_red_slider.generic.y			= y += (UIFT_SIZEINC*2);
	m_screen_menu.ch_red_slider.generic.name		= "Red color";
	m_screen_menu.ch_red_slider.generic.callBack	= CrosshairRedFunc;
	m_screen_menu.ch_red_slider.minValue			= 0;
	m_screen_menu.ch_red_slider.maxValue			= 10;
	m_screen_menu.ch_red_slider.generic.statusBar	= "Crosshair Red Amount";
	m_screen_menu.ch_red_amount.generic.type		= UITYPE_ACTION;
	m_screen_menu.ch_red_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_screen_menu.ch_red_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_screen_menu.ch_red_amount.generic.y			= y;

	m_screen_menu.ch_green_slider.generic.type		= UITYPE_SLIDER;
	m_screen_menu.ch_green_slider.generic.x			= 0;
	m_screen_menu.ch_green_slider.generic.y			= y += UIFT_SIZEINC;
	m_screen_menu.ch_green_slider.generic.name		= "Green color";
	m_screen_menu.ch_green_slider.generic.callBack	= CrosshairGreenFunc;
	m_screen_menu.ch_green_slider.minValue			= 0;
	m_screen_menu.ch_green_slider.maxValue			= 10;
	m_screen_menu.ch_green_slider.generic.statusBar	= "Crosshair Green Amount";
	m_screen_menu.ch_green_amount.generic.type		= UITYPE_ACTION;
	m_screen_menu.ch_green_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_screen_menu.ch_green_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_screen_menu.ch_green_amount.generic.y			= y;

	m_screen_menu.ch_blue_slider.generic.type		= UITYPE_SLIDER;
	m_screen_menu.ch_blue_slider.generic.x			= 0;
	m_screen_menu.ch_blue_slider.generic.y			= y += UIFT_SIZEINC;
	m_screen_menu.ch_blue_slider.generic.name		= "Blue color";
	m_screen_menu.ch_blue_slider.generic.callBack	= CrosshairBlueFunc;
	m_screen_menu.ch_blue_slider.minValue			= 0;
	m_screen_menu.ch_blue_slider.maxValue			= 10;
	m_screen_menu.ch_blue_slider.generic.statusBar	= "Crosshair Blue Amount";
	m_screen_menu.ch_blue_amount.generic.type		= UITYPE_ACTION;
	m_screen_menu.ch_blue_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_screen_menu.ch_blue_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_screen_menu.ch_blue_amount.generic.y			= y;

	m_screen_menu.back_action.generic.type		= UITYPE_ACTION;
	m_screen_menu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_screen_menu.back_action.generic.x			= 0;
	m_screen_menu.back_action.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCLG;
	m_screen_menu.back_action.generic.name		= "< Back";
	m_screen_menu.back_action.generic.callBack	= Back_Menu;
	m_screen_menu.back_action.generic.statusBar	= "Back a menu";

	ScreenMenu_SetValues ();

	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.banner);

	//
	// console
	//

	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.con_header);

	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.con_clock_toggle);
	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.con_notfade_toggle);
	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.con_notlarge_toggle);
	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.con_timestamp_list);

	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.con_notlines_slider);
	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.con_notlines_amount);
	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.con_alpha_slider);
	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.con_alpha_amount);
	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.con_drop_slider);
	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.con_drop_amount);
	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.con_scroll_slider);
	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.con_scroll_amount);
	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.con_fontscale_slider);
	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.con_fontscale_amount);

	//
	// crosshair
	//

	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.ch_header);

	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.ch_number_list);

	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.ch_alpha_slider);
	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.ch_alpha_amount);
	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.ch_pulse_slider);
	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.ch_pulse_amount);
	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.ch_scale_slider);
	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.ch_scale_amount);

	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.ch_red_slider);
	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.ch_red_amount);
	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.ch_green_slider);
	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.ch_green_amount);
	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.ch_blue_slider);
	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.ch_blue_amount);

	UI_AddItem (&m_screen_menu.framework,		&m_screen_menu.back_action);

	UI_Center (&m_screen_menu.framework);
	UI_Setup (&m_screen_menu.framework);
}


/*
=============
ScreenMenu_Draw
=============
*/
void ScreenMenu_Draw (void) {
	UI_Center (&m_screen_menu.framework);
	UI_Setup (&m_screen_menu.framework);

	UI_AdjustTextCursor (&m_screen_menu.framework, 1);
	UI_Draw (&m_screen_menu.framework);
}


/*
=============
ScreenMenu_Key
=============
*/
struct sfx_s *ScreenMenu_Key (int key) {
	return DefaultMenu_Key (&m_screen_menu.framework, key);
}


/*
=============
UI_ScreenMenu_f
=============
*/
void UI_ScreenMenu_f (void) {
	ScreenMenu_Init ();
	UI_PushMenu (&m_screen_menu.framework, ScreenMenu_Draw, ScreenMenu_Key);
}
