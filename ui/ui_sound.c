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

void Sound_DrawUpdateBox (void) {
	float	midrow = (uis.vidWidth*0.5) - (18*UIFT_SIZEMED);
	float	midcol = (uis.vidHeight*0.5) - (3*UIFT_SIZEMED);

	UI_DrawTextBox (midrow, midcol, UIFT_SCALEMED, 36, 4);
	UI_DrawString (midrow + (UIFT_SIZEMED*2), midcol + UIFT_SIZEMED, 0, UIFT_SCALEMED,		"       --- PLEASE WAIT! ---       ",	colorGreen);
	UI_DrawString (midrow + (UIFT_SIZEMED*2), midcol + (UIFT_SIZEMED*2), 0, UIFT_SCALEMED,	"The sound system is currently res-",	colorGreen);
	UI_DrawString (midrow + (UIFT_SIZEMED*2), midcol + (UIFT_SIZEMED*3), 0, UIFT_SCALEMED,	"tarting, this could take up to a",		colorGreen);
	UI_DrawString (midrow + (UIFT_SIZEMED*2), midcol + (UIFT_SIZEMED*4), 0, UIFT_SCALEMED,	"minute so, please be patient.",		colorGreen);

	uii.R_EndFrame ();		// the text box won't show up unless we do a buffer swap
	uii.Snd_Restart_f ();	// restart the sound system
}

/*
=======================================================================

	SOUND MENU

=======================================================================
*/

typedef struct {
	uiFrameWork_t	framework;

	uiImage_t		banner;
	uiAction_t		header;

	uiList_t		sound_toggle;

	uiSlider_t		sfxvolume_slider;
	uiAction_t		sfxvolume_amount;
	uiList_t		cdtoggle_toggle;
	uiList_t		quality_list;
	uiList_t		combatibility_list;

	uiAction_t		back_action;
} m_soundmenu_t;

m_soundmenu_t	m_sound_menu;

static void SoundToggleFunc (void *unused) {
	uii.Cvar_SetValue ("s_initsound", m_sound_menu.sound_toggle.curvalue);
	Sound_DrawUpdateBox ();
}

static void UpdateVolumeFunc (void *unused) {
	uii.Cvar_SetValue ("s_volume", m_sound_menu.sfxvolume_slider.curvalue * 0.05);
	m_sound_menu.sfxvolume_amount.generic.name = uii.Cvar_VariableString ("s_volume");
}

static void UpdateCDToggleFunc (void *unused) {
	uii.Cvar_SetValue ("cd_nocd", !m_sound_menu.cdtoggle_toggle.curvalue);
}

static void UpdateSoundQualityFunc (void *unused) {
	static char quality_ints[] = {
		8,
		11,
		16,
		22,
		32,
		44,
		48,
		0
	};

	uii.Cvar_SetValue ("s_khz", quality_ints[m_sound_menu.quality_list.curvalue]);

	if (uii.Cvar_VariableInteger ("s_khz") < 16) { uii.Cvar_SetValue ("s_loadas8bit", 1); }
	else { uii.Cvar_SetValue ("s_loadas8bit", 0); }

	uii.Cvar_SetValue ("s_primary", m_sound_menu.combatibility_list.curvalue);

	Sound_DrawUpdateBox ();
}


/*
=============
Sound_SetValues
=============
*/
static void SoundMenu_SetValues (void) {
	uii.Cvar_SetValue ("s_initsound",	clamp (uii.Cvar_VariableInteger ("s_initsound"), 0, 2));
	m_sound_menu.sound_toggle.curvalue	= uii.Cvar_VariableInteger ("s_initsound");

	m_sound_menu.sfxvolume_slider.curvalue	= uii.Cvar_VariableValue ("s_volume") * 20;
	m_sound_menu.sfxvolume_amount.generic.name = uii.Cvar_VariableString ("s_volume");

	uii.Cvar_SetValue ("cd_nocd",			clamp (uii.Cvar_VariableInteger ("cd_nocd"), 0, 1));
	m_sound_menu.cdtoggle_toggle.curvalue	= !uii.Cvar_VariableInteger ("cd_nocd");

	if (uii.Cvar_VariableInteger ("s_khz") == 11) { m_sound_menu.quality_list.curvalue = 1; }
	else if (uii.Cvar_VariableInteger ("s_khz") == 16) { m_sound_menu.quality_list.curvalue = 2; }
	else if (uii.Cvar_VariableInteger ("s_khz") == 22) { m_sound_menu.quality_list.curvalue = 3; }
	else if (uii.Cvar_VariableInteger ("s_khz") == 32) { m_sound_menu.quality_list.curvalue = 4; }
	else if (uii.Cvar_VariableInteger ("s_khz") == 44) { m_sound_menu.quality_list.curvalue = 5; }
	else if (uii.Cvar_VariableInteger ("s_khz") == 48) { m_sound_menu.quality_list.curvalue = 6; }
	else { m_sound_menu.quality_list.curvalue = 0; }

	m_sound_menu.combatibility_list.curvalue	= uii.Cvar_VariableInteger  ("s_primary");
}


/*
=============
SoundMenu_Init
=============
*/
void SoundMenu_Init (void) {
	static char *cd_music_items[] = {
		"disabled",
		"enabled",
		0
	};

	static char *compatibility_items[] = {
		"max compatibility",
		"max performance",
		0
	};

	static char *soundinit_items[] = {
		"off",
		"on",
		"OpenAL [ EXPERIMENTAL ]",
		0
	};

	static char *quality_items[] = {
		"8KHz",
		"11KHz",
		"16KHz",
		"22KHz",
		"32KHz",
		"44KHz",
		"48KHz",
		0
	};

	float	y;

	m_sound_menu.framework.x		= uis.vidWidth * 0.5;
	m_sound_menu.framework.y		= 0;
	m_sound_menu.framework.nitems	= 0;

	UI_Banner (&m_sound_menu.banner, uiMedia.optionsBanner, &y);

	m_sound_menu.header.generic.type		= UITYPE_ACTION;
	m_sound_menu.header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_sound_menu.header.generic.x			= 0;
	m_sound_menu.header.generic.y			= y += UIFT_SIZEINC;
	m_sound_menu.header.generic.name		= "Sound Settings";

	m_sound_menu.sound_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_sound_menu.sound_toggle.generic.x			= 0;
	m_sound_menu.sound_toggle.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_sound_menu.sound_toggle.generic.name		= "Sound";
	m_sound_menu.sound_toggle.generic.callback	= SoundToggleFunc;
	m_sound_menu.sound_toggle.itemnames			= soundinit_items;
	m_sound_menu.sound_toggle.generic.statusbar	= "Toggle Sound";

	m_sound_menu.sfxvolume_slider.generic.type		= UITYPE_SLIDER;
	m_sound_menu.sfxvolume_slider.generic.x			= 0;
	m_sound_menu.sfxvolume_slider.generic.y			= y += UIFT_SIZEINC;
	m_sound_menu.sfxvolume_slider.generic.name		= "Volume";
	m_sound_menu.sfxvolume_slider.generic.callback	= UpdateVolumeFunc;
	m_sound_menu.sfxvolume_slider.minvalue			= 0;
	m_sound_menu.sfxvolume_slider.maxvalue			= 20;
	m_sound_menu.sfxvolume_slider.generic.statusbar	= "Volume Control";
	m_sound_menu.sfxvolume_amount.generic.type		= UITYPE_ACTION;
	m_sound_menu.sfxvolume_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_sound_menu.sfxvolume_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_sound_menu.sfxvolume_amount.generic.y			= y;

	m_sound_menu.cdtoggle_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_sound_menu.cdtoggle_toggle.generic.x			= 0;
	m_sound_menu.cdtoggle_toggle.generic.y			= y += UIFT_SIZEINC;
	m_sound_menu.cdtoggle_toggle.generic.name		= "CD music";
	m_sound_menu.cdtoggle_toggle.generic.callback	= UpdateCDToggleFunc;
	m_sound_menu.cdtoggle_toggle.itemnames			= cd_music_items;
	m_sound_menu.cdtoggle_toggle.generic.statusbar	= "Toggle CD Play";

	m_sound_menu.quality_list.generic.type		= UITYPE_SPINCONTROL;
	m_sound_menu.quality_list.generic.x			= 0;
	m_sound_menu.quality_list.generic.y			= y += UIFT_SIZEINC;
	m_sound_menu.quality_list.generic.name		= "Sound quality";
	m_sound_menu.quality_list.generic.callback	= UpdateSoundQualityFunc;
	m_sound_menu.quality_list.itemnames			= quality_items;
	m_sound_menu.quality_list.generic.statusbar	= "Sound Quality";

	m_sound_menu.combatibility_list.generic.type		= UITYPE_SPINCONTROL;
	m_sound_menu.combatibility_list.generic.x			= 0;
	m_sound_menu.combatibility_list.generic.y			= y += UIFT_SIZEINC;
	m_sound_menu.combatibility_list.generic.name		= "Compatibility";
	m_sound_menu.combatibility_list.generic.callback	= UpdateSoundQualityFunc;
	m_sound_menu.combatibility_list.itemnames			= compatibility_items;
	m_sound_menu.combatibility_list.generic.statusbar	= "Primary/Secondary sound buffer";

	m_sound_menu.back_action.generic.type		= UITYPE_ACTION;
	m_sound_menu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_sound_menu.back_action.generic.x			= 0;
	m_sound_menu.back_action.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCLG;
	m_sound_menu.back_action.generic.name		= "< Back";
	m_sound_menu.back_action.generic.callback	= Back_Menu;
	m_sound_menu.back_action.generic.statusbar	= "Back a menu";

	SoundMenu_SetValues ();

	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.banner);
	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.header);

	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.sound_toggle);
	
	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.sfxvolume_slider);
	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.sfxvolume_amount);
	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.cdtoggle_toggle);
	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.quality_list);
	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.combatibility_list);

	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.back_action);

	UI_Center (&m_sound_menu.framework);
	UI_Setup (&m_sound_menu.framework);
}


/*
=============
SoundMenu_Draw
=============
*/
void SoundMenu_Draw (void) {
	UI_Center (&m_sound_menu.framework);
	UI_Setup (&m_sound_menu.framework);

	UI_AdjustTextCursor (&m_sound_menu.framework, 1);
	UI_Draw (&m_sound_menu.framework);
}


/*
=============
SoundMenu_Key
=============
*/
const char *SoundMenu_Key (int key) {
	return DefaultMenu_Key (&m_sound_menu.framework, key);
}


/*
=============
UI_SoundMenu_f
=============
*/
void UI_SoundMenu_f (void) {
	SoundMenu_Init ();
	UI_PushMenu (&m_sound_menu.framework, SoundMenu_Draw, SoundMenu_Key);
}
