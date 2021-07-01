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

	INPUT MENU

=======================================================================
*/

typedef struct m_inputmenu_s {
	uiFrameWork_t	framework;

	uiImage_t		banner;
	uiAction_t		header;

	uiList_t		always_run_toggle;
	uiList_t		joystick_toggle;

	uiSlider_t		ui_sensitivity_slider;
	uiSlider_t		ui_sensitivity_amount;
	uiSlider_t		sensitivity_slider;
	uiSlider_t		sensitivity_amount;
	uiList_t		maccel_list;
	uiList_t		autosensitivity_toggle;
	uiList_t		invert_mouse_toggle;
	uiList_t		lookspring_toggle;
	uiList_t		lookstrafe_toggle;
	uiList_t		freelook_toggle;

	uiAction_t		back_action;
} m_inputmenu_t;

m_inputmenu_t	m_input_menu;

static void AlwaysRunFunc (void *unused) {
	uii.Cvar_SetValue ("cl_run", m_input_menu.always_run_toggle.curValue);
}

static void JoystickFunc (void *unused) {
	uii.Cvar_SetValue ("in_joystick", m_input_menu.joystick_toggle.curValue);
}

static void UISensFunc (void *unused) {
	uii.Cvar_SetValue ("ui_sensitivity", m_input_menu.ui_sensitivity_slider.curValue / 2.0F);
	m_input_menu.ui_sensitivity_amount.generic.name = uii.Cvar_VariableString ("ui_sensitivity");
}

static void SensitivityFunc (void *unused) {
	uii.Cvar_SetValue ("sensitivity", m_input_menu.sensitivity_slider.curValue / 2.0F);
	m_input_menu.sensitivity_amount.generic.name = uii.Cvar_VariableString ("sensitivity");
}

static void MouseAccelFunc (void *unused) {
	uii.Cvar_SetValue ("m_accel", m_input_menu.maccel_list.curValue);
}

static void InvertMouseFunc (void *unused) {
	uii.Cvar_SetValue ("m_pitch", (uii.Cvar_VariableValue ("m_pitch")) * -1);
	m_input_menu.invert_mouse_toggle.curValue		= (!!(uii.Cvar_VariableValue ("m_pitch") < 0));
}

static void AutoSensFunc (void *unused) {
	uii.Cvar_SetValue ("autosensitivity", m_input_menu.autosensitivity_toggle.curValue);
}

static void LookspringFunc (void *unused) {
	uii.Cvar_SetValue ("lookspring", m_input_menu.lookspring_toggle.curValue);
}

static void LookstrafeFunc (void *unused) {
	uii.Cvar_SetValue ("lookstrafe", m_input_menu.lookstrafe_toggle.curValue);
}

static void FreeLookFunc (void *unused)
{
	uii.Cvar_SetValue ("freelook", m_input_menu.freelook_toggle.curValue);
}


/*
=============
InputMenu_SetValues
=============
*/
static void InputMenu_SetValues (void) {
	uii.Cvar_SetValue ("cl_run",				clamp (uii.Cvar_VariableInteger ("cl_run"), 0, 1));
	m_input_menu.always_run_toggle.curValue		= uii.Cvar_VariableInteger ("cl_run");

	uii.Cvar_SetValue ("in_joystick",			clamp (uii.Cvar_VariableInteger ("in_joystick"), 0, 1));
	m_input_menu.joystick_toggle.curValue		= uii.Cvar_VariableInteger ("in_joystick");

	m_input_menu.ui_sensitivity_slider.curValue		= (uii.Cvar_VariableValue ("ui_sensitivity")) * 2;
	m_input_menu.ui_sensitivity_amount.generic.name = uii.Cvar_VariableString ("ui_sensitivity");
	m_input_menu.sensitivity_slider.curValue		= (uii.Cvar_VariableValue ("sensitivity")) * 2;
	m_input_menu.sensitivity_amount.generic.name	= uii.Cvar_VariableString ("sensitivity");

	uii.Cvar_SetValue ("m_accel",					clamp (uii.Cvar_VariableInteger ("m_accel"), 0, 2));
	m_input_menu.maccel_list.curValue				= uii.Cvar_VariableInteger ("m_accel");

	m_input_menu.invert_mouse_toggle.curValue		= (!!(uii.Cvar_VariableValue ("m_pitch") < 0));

	uii.Cvar_SetValue ("autosensitivity",			clamp (uii.Cvar_VariableInteger ("autosensitivity"), 0, 1));
	m_input_menu.autosensitivity_toggle.curValue	= uii.Cvar_VariableInteger ("autosensitivity");

	uii.Cvar_SetValue ("lookspring",			clamp (uii.Cvar_VariableInteger ("lookspring"), 0, 1));
	m_input_menu.lookspring_toggle.curValue		= uii.Cvar_VariableInteger ("lookspring");

	uii.Cvar_SetValue ("lookstrafe",			clamp (uii.Cvar_VariableInteger ("lookstrafe"), 0, 1));
	m_input_menu.lookstrafe_toggle.curValue		= uii.Cvar_VariableInteger ("lookstrafe");

	uii.Cvar_SetValue ("freelook",				clamp (uii.Cvar_VariableInteger ("freelook"), 0, 1));
	m_input_menu.freelook_toggle.curValue		= uii.Cvar_VariableInteger ("freelook");
}


/*
=============
InputMenu_Init
=============
*/
void InputMenu_Init (void) {
	static char *yesno_names[] = {
		"no",
		"yes",
		0
	};

	static char *maccel_items[] = {
		"no accel",
		"normal",
		"os values",
		0
	};

	static char *onoff_names[] = {
		"off",
		"on",
		0
	};

	float	y;

	m_input_menu.framework.x		= uis.vidWidth * 0.5;
	m_input_menu.framework.y		= 0;
	m_input_menu.framework.numItems	= 0;

	UI_Banner (&m_input_menu.banner, uiMedia.banners.options, &y);

	m_input_menu.header.generic.type		= UITYPE_ACTION;
	m_input_menu.header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_input_menu.header.generic.x			= 0;
	m_input_menu.header.generic.y			= y += UIFT_SIZEINC;
	m_input_menu.header.generic.name		= "Input Settings";

	m_input_menu.always_run_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_input_menu.always_run_toggle.generic.x			= 0;
	m_input_menu.always_run_toggle.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_input_menu.always_run_toggle.generic.name			= "Always run";
	m_input_menu.always_run_toggle.generic.callBack		= AlwaysRunFunc;
	m_input_menu.always_run_toggle.itemNames			= yesno_names;
	m_input_menu.always_run_toggle.generic.statusBar	= "Always Run";

	m_input_menu.joystick_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_input_menu.joystick_toggle.generic.x			= 0;
	m_input_menu.joystick_toggle.generic.y			= y += UIFT_SIZEINC;
	m_input_menu.joystick_toggle.generic.name		= "Use joystick";
	m_input_menu.joystick_toggle.generic.callBack	= JoystickFunc;
	m_input_menu.joystick_toggle.itemNames			= yesno_names;
	m_input_menu.joystick_toggle.generic.statusBar	= "Use Joystick";

	m_input_menu.ui_sensitivity_slider.generic.type			= UITYPE_SLIDER;
	m_input_menu.ui_sensitivity_slider.generic.x			= 0;
	m_input_menu.ui_sensitivity_slider.generic.y			= y += (UIFT_SIZEINC*2);
	m_input_menu.ui_sensitivity_slider.generic.name			= "UI speed";
	m_input_menu.ui_sensitivity_slider.generic.callBack		= UISensFunc;
	m_input_menu.ui_sensitivity_slider.minValue				= 1;
	m_input_menu.ui_sensitivity_slider.maxValue				= 10;
	m_input_menu.ui_sensitivity_slider.generic.statusBar	= "Menu mouse cursor sensitivity";
	m_input_menu.ui_sensitivity_amount.generic.type			= UITYPE_ACTION;
	m_input_menu.ui_sensitivity_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_input_menu.ui_sensitivity_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_input_menu.ui_sensitivity_amount.generic.y			= y;

	m_input_menu.sensitivity_slider.generic.type		= UITYPE_SLIDER;
	m_input_menu.sensitivity_slider.generic.x			= 0;
	m_input_menu.sensitivity_slider.generic.y			= y += UIFT_SIZEINC;
	m_input_menu.sensitivity_slider.generic.name		= "Mouse speed";
	m_input_menu.sensitivity_slider.generic.callBack	= SensitivityFunc;
	m_input_menu.sensitivity_slider.minValue			= 2;
	m_input_menu.sensitivity_slider.maxValue			= 52;
	m_input_menu.sensitivity_slider.generic.statusBar	= "Mouse Sensitivity";
	m_input_menu.sensitivity_amount.generic.type		= UITYPE_ACTION;
	m_input_menu.sensitivity_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_input_menu.sensitivity_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_input_menu.sensitivity_amount.generic.y			= y;

	m_input_menu.maccel_list.generic.type		= UITYPE_SPINCONTROL;
	m_input_menu.maccel_list.generic.x			= 0;
	m_input_menu.maccel_list.generic.y			= y += UIFT_SIZEINC;
	m_input_menu.maccel_list.generic.name		= "Mouse accel";
	m_input_menu.maccel_list.generic.callBack	= MouseAccelFunc;
	m_input_menu.maccel_list.itemNames			= maccel_items;
	m_input_menu.maccel_list.generic.statusBar	= "Mouse Acceleration options";

	m_input_menu.autosensitivity_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_input_menu.autosensitivity_toggle.generic.x			= 0;
	m_input_menu.autosensitivity_toggle.generic.y			= y += UIFT_SIZEINC;
	m_input_menu.autosensitivity_toggle.generic.name		= "Auto sensitivity";
	m_input_menu.autosensitivity_toggle.generic.callBack	= AutoSensFunc;
	m_input_menu.autosensitivity_toggle.itemNames			= yesno_names;
	m_input_menu.autosensitivity_toggle.generic.statusBar	= "FOV auto-affects mouse sensitivity";

	m_input_menu.invert_mouse_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_input_menu.invert_mouse_toggle.generic.x			= 0;
	m_input_menu.invert_mouse_toggle.generic.y			= y += UIFT_SIZEINC;
	m_input_menu.invert_mouse_toggle.generic.name		= "Invert mouse";
	m_input_menu.invert_mouse_toggle.generic.callBack	= InvertMouseFunc;
	m_input_menu.invert_mouse_toggle.itemNames			= yesno_names;
	m_input_menu.invert_mouse_toggle.generic.statusBar	= "Invert Mouse";

	m_input_menu.lookspring_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_input_menu.lookspring_toggle.generic.x			= 0;
	m_input_menu.lookspring_toggle.generic.y			= y += (UIFT_SIZEINC*2);
	m_input_menu.lookspring_toggle.generic.name			= "Lookspring";
	m_input_menu.lookspring_toggle.generic.callBack		= LookspringFunc;
	m_input_menu.lookspring_toggle.itemNames			= onoff_names;
	m_input_menu.lookspring_toggle.generic.statusBar	= "Lookspring";

	m_input_menu.lookstrafe_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_input_menu.lookstrafe_toggle.generic.x			= 0;
	m_input_menu.lookstrafe_toggle.generic.y			= y += UIFT_SIZEINC;
	m_input_menu.lookstrafe_toggle.generic.name			= "Lookstrafe";
	m_input_menu.lookstrafe_toggle.generic.callBack		= LookstrafeFunc;
	m_input_menu.lookstrafe_toggle.itemNames			= onoff_names;
	m_input_menu.lookstrafe_toggle.generic.statusBar	= "Lookstrafe";

	m_input_menu.freelook_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_input_menu.freelook_toggle.generic.x			= 0;
	m_input_menu.freelook_toggle.generic.y			= y += UIFT_SIZEINC;
	m_input_menu.freelook_toggle.generic.name		= "Free look";
	m_input_menu.freelook_toggle.generic.callBack	= FreeLookFunc;
	m_input_menu.freelook_toggle.itemNames			= onoff_names;
	m_input_menu.freelook_toggle.generic.statusBar	= "Free Look";

	m_input_menu.back_action.generic.type		= UITYPE_ACTION;
	m_input_menu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_input_menu.back_action.generic.x			= 0;
	m_input_menu.back_action.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCLG;
	m_input_menu.back_action.generic.name		= "< Back";
	m_input_menu.back_action.generic.callBack	= Menu_Pop;
	m_input_menu.back_action.generic.statusBar	= "Back a menu";

	InputMenu_SetValues ();

	UI_AddItem (&m_input_menu.framework,		&m_input_menu.banner);
	UI_AddItem (&m_input_menu.framework,		&m_input_menu.header);

	UI_AddItem (&m_input_menu.framework,		&m_input_menu.always_run_toggle);
	UI_AddItem (&m_input_menu.framework,		&m_input_menu.joystick_toggle);

	UI_AddItem (&m_input_menu.framework,		&m_input_menu.ui_sensitivity_slider);
	UI_AddItem (&m_input_menu.framework,		&m_input_menu.ui_sensitivity_amount);
	UI_AddItem (&m_input_menu.framework,		&m_input_menu.sensitivity_slider);
	UI_AddItem (&m_input_menu.framework,		&m_input_menu.sensitivity_amount);
	UI_AddItem (&m_input_menu.framework,		&m_input_menu.maccel_list);
	UI_AddItem (&m_input_menu.framework,		&m_input_menu.autosensitivity_toggle);
	UI_AddItem (&m_input_menu.framework,		&m_input_menu.invert_mouse_toggle);

	UI_AddItem (&m_input_menu.framework,		&m_input_menu.lookspring_toggle);
	UI_AddItem (&m_input_menu.framework,		&m_input_menu.lookstrafe_toggle);
	UI_AddItem (&m_input_menu.framework,		&m_input_menu.freelook_toggle);

	UI_AddItem (&m_input_menu.framework,		&m_input_menu.back_action);

	UI_Center (&m_input_menu.framework);
	UI_Setup (&m_input_menu.framework);
}


/*
=============
InputMenu_Draw
=============
*/
void InputMenu_Draw (void) {
	UI_Center (&m_input_menu.framework);
	UI_Setup (&m_input_menu.framework);

	UI_AdjustTextCursor (&m_input_menu.framework, 1);
	UI_Draw (&m_input_menu.framework);
}


/*
=============
InputMenu_Key
=============
*/
struct sfx_s *InputMenu_Key (int key) {
	return DefaultMenu_Key (&m_input_menu.framework, key);
}


/*
=============
UI_InputMenu_f
=============
*/
void UI_InputMenu_f (void) {
	InputMenu_Init ();
	UI_PushMenu (&m_input_menu.framework, InputMenu_Draw, InputMenu_Key);
}
