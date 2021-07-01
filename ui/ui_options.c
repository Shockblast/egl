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

	OPTIONS MENU

=======================================================================
*/

typedef struct {
	uiFrameWork_t	framework;

	uiImage_t		banner;

	uiAction_t		console_menu;
	uiAction_t		controls_menu;
	uiAction_t		crosshair_menu;
	uiAction_t		effects_menu;
	uiAction_t		fog_menu;
	uiAction_t		gloom_menu;
	uiAction_t		hud_menu;
	uiAction_t		input_menu;
	uiAction_t		misc_menu;
	uiAction_t		screen_menu;
	uiAction_t		sound_menu;
	uiAction_t		view_menu;

	uiAction_t		back_action;
} m_optionsmenu_t;

m_optionsmenu_t	m_options_menu;

static void Controls_Menu (void *unused) {
	UI_ControlsMenu_f ();
}

static void Effects_Menu (void *unused) {
	UI_EffectsMenu_f ();
}

static void Fog_Menu (void *unused) {
	UI_FogMenu_f ();
}

static void Gloom_Menu (void *unused) {
	UI_GloomMenu_f ();
}

static void HUD_Menu (void *unused) {
	UI_HUDMenu_f ();
}

static void Input_Menu (void *unused) {
	UI_InputMenu_f ();
}

static void Misc_Menu (void *unused) {
	UI_MiscMenu_f ();
}

static void Screen_Menu (void *unused) {
	UI_ScreenMenu_f ();
}

static void Sound_Menu (void *unused) {
	UI_SoundMenu_f ();
}


/*
=============
OptionsMenu_Init
=============
*/
void OptionsMenu_Init (void) {
	float	y;

	m_options_menu.framework.x		= uis.vidWidth * 0.5;
	m_options_menu.framework.y		= 0;
	m_options_menu.framework.nitems	= 0;

	UI_Banner (&m_options_menu.banner, uiMedia.optionsBanner, &y);

	m_options_menu.controls_menu.generic.type		= UITYPE_ACTION;
	m_options_menu.controls_menu.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_options_menu.controls_menu.generic.x			= 0;
	m_options_menu.controls_menu.generic.y			= y += UIFT_SIZEINC;
	m_options_menu.controls_menu.generic.name		= "Controls";
	m_options_menu.controls_menu.generic.callback	= Controls_Menu;
	m_options_menu.controls_menu.generic.statusbar	= "Opens the Controls menu";

	m_options_menu.effects_menu.generic.type		= UITYPE_ACTION;
	m_options_menu.effects_menu.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_options_menu.effects_menu.generic.x			= 0;
	m_options_menu.effects_menu.generic.y			= y += UIFT_SIZEINCLG;
	m_options_menu.effects_menu.generic.name		= "Effects";
	m_options_menu.effects_menu.generic.callback	= Effects_Menu;
	m_options_menu.effects_menu.generic.statusbar	= "Opens the Effects Settings menu";

	m_options_menu.fog_menu.generic.type		= UITYPE_ACTION;
	m_options_menu.fog_menu.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_options_menu.fog_menu.generic.x			= 0;
	m_options_menu.fog_menu.generic.y			= y += UIFT_SIZEINCLG;
	m_options_menu.fog_menu.generic.name		= "Fogging";
	m_options_menu.fog_menu.generic.callback	= Fog_Menu;
	m_options_menu.fog_menu.generic.statusbar	= "Opens the Fog Settings menu";

	m_options_menu.gloom_menu.generic.type		= UITYPE_ACTION;
	m_options_menu.gloom_menu.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_options_menu.gloom_menu.generic.x			= 0;
	m_options_menu.gloom_menu.generic.y			= y += UIFT_SIZEINCLG;
	m_options_menu.gloom_menu.generic.name		= "Gloom";
	m_options_menu.gloom_menu.generic.callback	= Gloom_Menu;
	m_options_menu.gloom_menu.generic.statusbar	= "Opens the Gloom Settings menu";

	m_options_menu.hud_menu.generic.type		= UITYPE_ACTION;
	m_options_menu.hud_menu.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_options_menu.hud_menu.generic.x			= 0;
	m_options_menu.hud_menu.generic.y			= y += UIFT_SIZEINCLG;
	m_options_menu.hud_menu.generic.name		= "HUD";
	m_options_menu.hud_menu.generic.callback	= HUD_Menu;
	m_options_menu.hud_menu.generic.statusbar	= "Opens the HUD Settings menu";

	m_options_menu.input_menu.generic.type		= UITYPE_ACTION;
	m_options_menu.input_menu.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_options_menu.input_menu.generic.x			= 0;
	m_options_menu.input_menu.generic.y			= y += UIFT_SIZEINCLG;
	m_options_menu.input_menu.generic.name		= "Input";
	m_options_menu.input_menu.generic.callback	= Input_Menu;
	m_options_menu.input_menu.generic.statusbar	= "Opens the Input Settings menu";

	m_options_menu.misc_menu.generic.type		= UITYPE_ACTION;
	m_options_menu.misc_menu.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_options_menu.misc_menu.generic.x			= 0;
	m_options_menu.misc_menu.generic.y			= y += UIFT_SIZEINCLG;
	m_options_menu.misc_menu.generic.name		= "Misc";
	m_options_menu.misc_menu.generic.callback	= Misc_Menu;
	m_options_menu.misc_menu.generic.statusbar	= "Opens the Misc Settings menu";

	m_options_menu.screen_menu.generic.type			= UITYPE_ACTION;
	m_options_menu.screen_menu.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_options_menu.screen_menu.generic.x			= 0;
	m_options_menu.screen_menu.generic.y			= y += UIFT_SIZEINCLG;
	m_options_menu.screen_menu.generic.name			= "Screen";
	m_options_menu.screen_menu.generic.callback		= Screen_Menu;
	m_options_menu.screen_menu.generic.statusbar	= "Opens the Screen Settings menu";

	m_options_menu.sound_menu.generic.type		= UITYPE_ACTION;
	m_options_menu.sound_menu.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_options_menu.sound_menu.generic.x			= 0;
	m_options_menu.sound_menu.generic.y			= y += UIFT_SIZEINCLG;
	m_options_menu.sound_menu.generic.name		= "Sound";
	m_options_menu.sound_menu.generic.callback	= Sound_Menu;
	m_options_menu.sound_menu.generic.statusbar	= "Opens the Sound Settings menu";

	m_options_menu.back_action.generic.type			= UITYPE_ACTION;
	m_options_menu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_options_menu.back_action.generic.x			= 0;
	m_options_menu.back_action.generic.y			= y += (UIFT_SIZEINCLG*2);
	m_options_menu.back_action.generic.name			= "< Back";
	m_options_menu.back_action.generic.callback		= Back_Menu;
	m_options_menu.back_action.generic.statusbar	= "Back a menu";

	UI_AddItem (&m_options_menu.framework,		&m_options_menu.banner);

	UI_AddItem (&m_options_menu.framework,		&m_options_menu.controls_menu);
	UI_AddItem (&m_options_menu.framework,		&m_options_menu.effects_menu);
	UI_AddItem (&m_options_menu.framework,		&m_options_menu.fog_menu);
	UI_AddItem (&m_options_menu.framework,		&m_options_menu.gloom_menu);
	UI_AddItem (&m_options_menu.framework,		&m_options_menu.hud_menu);
	UI_AddItem (&m_options_menu.framework,		&m_options_menu.input_menu);
	UI_AddItem (&m_options_menu.framework,		&m_options_menu.misc_menu);
	UI_AddItem (&m_options_menu.framework,		&m_options_menu.screen_menu);
	UI_AddItem (&m_options_menu.framework,		&m_options_menu.sound_menu);

	UI_AddItem (&m_options_menu.framework,		&m_options_menu.back_action);

	UI_Center (&m_options_menu.framework);
	UI_Setup (&m_options_menu.framework);
}


/*
=============
OptionsMenu_Draw
=============
*/
void OptionsMenu_Draw (void) {
	UI_Center (&m_options_menu.framework);
	UI_Setup (&m_options_menu.framework);

	UI_AdjustTextCursor (&m_options_menu.framework, 1);
	UI_Draw (&m_options_menu.framework);
}


/*
=============
OptionsMenuKey
=============
*/
const char *OptionsMenu_Key (int key) {
	return DefaultMenu_Key (&m_options_menu.framework, key);
}


/*
=============
UI_OptionsMenu_f
=============
*/
void UI_OptionsMenu_f (void) {
	OptionsMenu_Init ();
	UI_PushMenu (&m_options_menu.framework, OptionsMenu_Draw, OptionsMenu_Key);
}
