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

void Sound_RestartBox (void) {
	float	midrow = (uis.vidWidth*0.5) - (18*UIFT_SIZEMED);
	float	midcol = (uis.vidHeight*0.5) - (3*UIFT_SIZEMED);

	UI_DrawTextBox (midrow, midcol, UIFT_SCALEMED, 36, 4);
	UI_DrawString (midrow + (UIFT_SIZEMED*2), midcol + UIFT_SIZEMED, 0, UIFT_SCALEMED,		"       --- PLEASE WAIT! ---       ",	colorGreen);
	UI_DrawString (midrow + (UIFT_SIZEMED*2), midcol + (UIFT_SIZEMED*2), 0, UIFT_SCALEMED,	"The sound system is currently res-",	colorGreen);
	UI_DrawString (midrow + (UIFT_SIZEMED*2), midcol + (UIFT_SIZEMED*3), 0, UIFT_SCALEMED,	"tarting, this could take up to a",		colorGreen);
	UI_DrawString (midrow + (UIFT_SIZEMED*2), midcol + (UIFT_SIZEMED*4), 0, UIFT_SCALEMED,	"minute so, please be patient.",		colorGreen);

	uii.R_EndFrame ();					// the text box won't show up unless we do a buffer swap
	uii.Cbuf_AddText ("snd_restart\n");	// restart the sound system
}

/*
=======================================================================

	SOUND MENU

=======================================================================
*/

typedef struct m_soundmenu_s {
	uiFrameWork_t	framework;

	uiImage_t		banner;
	uiAction_t		header;

	uiList_t		sound_toggle;

	uiSlider_t		sfxvolume_slider;
	uiAction_t		sfxvolume_amount;
	uiList_t		cdtoggle_toggle;

	// openal
	uiAction_t		al_header;

	uiSlider_t		dopplerfactor_slider;
	uiAction_t		dopplerfactor_amount;
	uiSlider_t		dopplervelocity_slider;
	uiAction_t		dopplervelocity_amount;

	uiList_t		al_extensions_toggle;
	uiList_t		al_ext_eax_toggle;

	// software
	uiAction_t		sw_header;
	uiList_t		sw_quality_list;
	uiList_t		sw_combatibility_list;

	uiAction_t		back_action;
} m_soundmenu_t;

m_soundmenu_t	m_sound_menu;

static void SoundToggleFunc (void *unused) {
	uii.Cvar_ForceSetValue ("s_initsound", m_sound_menu.sound_toggle.curValue);
	Sound_RestartBox ();
}

static void UpdateVolumeFunc (void *unused) {
	uii.Cvar_SetValue ("s_volume", m_sound_menu.sfxvolume_slider.curValue * 0.05);
	m_sound_menu.sfxvolume_amount.generic.name = uii.Cvar_VariableString ("s_volume");
}

static void ALDopFactorFunc (void *unused) {
	uii.Cvar_SetValue ("al_dopplerfactor", m_sound_menu.dopplerfactor_slider.curValue * 0.1);
	m_sound_menu.dopplerfactor_amount.generic.name = uii.Cvar_VariableString ("al_dopplerfactor");
}

static void ALDopVelocityFunc (void *unused) {
	uii.Cvar_SetValue ("al_dopplervelocity", m_sound_menu.dopplervelocity_slider.curValue * 100);
	m_sound_menu.dopplervelocity_amount.generic.name = uii.Cvar_VariableString ("al_dopplervelocity");
}

static void ALExtensionsFunc (void *unused) {
	uii.Cvar_ForceSetValue ("al_extensions", m_sound_menu.al_extensions_toggle.curValue);
	Sound_RestartBox ();
}

static void ALExtEAXFunc (void *unused) {
	uii.Cvar_ForceSetValue ("al_ext_eax", m_sound_menu.al_ext_eax_toggle.curValue);
	Sound_RestartBox ();
}

static void UpdateCDToggleFunc (void *unused) {
	uii.Cvar_SetValue ("cd_nocd", !m_sound_menu.cdtoggle_toggle.curValue);
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

	uii.Cvar_ForceSetValue ("s_khz", quality_ints[m_sound_menu.sw_quality_list.curValue]);

	if (uii.Cvar_VariableInteger ("s_khz") < 16) { uii.Cvar_ForceSetValue ("s_loadas8bit", 1); }
	else { uii.Cvar_ForceSetValue ("s_loadas8bit", 0); }

	uii.Cvar_ForceSetValue ("s_primary", m_sound_menu.sw_combatibility_list.curValue);

	Sound_RestartBox ();
}


/*
=============
Sound_SetValues
=============
*/
static void SoundMenu_SetValues (void) {
	uii.Cvar_ForceSetValue ("s_initsound",	clamp (uii.Cvar_VariableInteger ("s_initsound"), 0, 2));
	m_sound_menu.sound_toggle.curValue		= uii.Cvar_VariableInteger ("s_initsound");

	m_sound_menu.sfxvolume_slider.curValue		= uii.Cvar_VariableValue ("s_volume") * 20;
	m_sound_menu.sfxvolume_amount.generic.name	= uii.Cvar_VariableString ("s_volume");

	uii.Cvar_SetValue ("cd_nocd",			clamp (uii.Cvar_VariableInteger ("cd_nocd"), 0, 1));
	m_sound_menu.cdtoggle_toggle.curValue	= !uii.Cvar_VariableInteger ("cd_nocd");

	m_sound_menu.dopplerfactor_slider.curValue		= uii.Cvar_VariableValue ("al_dopplerfactor") * 10;
	m_sound_menu.dopplerfactor_amount.generic.name	= uii.Cvar_VariableString ("al_dopplerfactor");

	m_sound_menu.dopplervelocity_slider.curValue		= uii.Cvar_VariableValue ("al_dopplervelocity") * 0.01;
	m_sound_menu.dopplervelocity_amount.generic.name	= uii.Cvar_VariableString ("al_dopplervelocity");

	uii.Cvar_ForceSetValue ("al_extensions",	clamp (uii.Cvar_VariableInteger ("al_extensions"), 0, 1));
	m_sound_menu.al_extensions_toggle.curValue	= uii.Cvar_VariableInteger ("al_extensions");

	uii.Cvar_ForceSetValue ("al_ext_eax",	clamp (uii.Cvar_VariableInteger ("al_ext_eax"), 0, 1));
	m_sound_menu.al_ext_eax_toggle.curValue	= uii.Cvar_VariableInteger ("al_ext_eax");

	if (uii.Cvar_VariableInteger ("s_khz") == 11) { m_sound_menu.sw_quality_list.curValue = 1; }
	else if (uii.Cvar_VariableInteger ("s_khz") == 16) { m_sound_menu.sw_quality_list.curValue = 2; }
	else if (uii.Cvar_VariableInteger ("s_khz") == 22) { m_sound_menu.sw_quality_list.curValue = 3; }
	else if (uii.Cvar_VariableInteger ("s_khz") == 32) { m_sound_menu.sw_quality_list.curValue = 4; }
	else if (uii.Cvar_VariableInteger ("s_khz") == 44) { m_sound_menu.sw_quality_list.curValue = 5; }
	else if (uii.Cvar_VariableInteger ("s_khz") == 48) { m_sound_menu.sw_quality_list.curValue = 6; }
	else { m_sound_menu.sw_quality_list.curValue = 0; }

	m_sound_menu.sw_combatibility_list.curValue	= uii.Cvar_VariableInteger  ("s_primary");
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
	m_sound_menu.framework.numItems	= 0;

	UI_Banner (&m_sound_menu.banner, uiMedia.optionsBanner, &y);

	m_sound_menu.header.generic.type		= UITYPE_ACTION;
	m_sound_menu.header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_sound_menu.header.generic.x			= 0;
	m_sound_menu.header.generic.y			= y += UIFT_SIZEINC;
	m_sound_menu.header.generic.name		= "All Sound Settings";

	m_sound_menu.sound_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_sound_menu.sound_toggle.generic.x			= 0;
	m_sound_menu.sound_toggle.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_sound_menu.sound_toggle.generic.name		= "Sound";
	m_sound_menu.sound_toggle.generic.callBack	= SoundToggleFunc;
	m_sound_menu.sound_toggle.itemNames			= soundinit_items;
	m_sound_menu.sound_toggle.generic.statusBar	= "Toggle Sound";

	m_sound_menu.sfxvolume_slider.generic.type		= UITYPE_SLIDER;
	m_sound_menu.sfxvolume_slider.generic.x			= 0;
	m_sound_menu.sfxvolume_slider.generic.y			= y += UIFT_SIZEINC;
	m_sound_menu.sfxvolume_slider.generic.name		= "Volume";
	m_sound_menu.sfxvolume_slider.generic.callBack	= UpdateVolumeFunc;
	m_sound_menu.sfxvolume_slider.minValue			= 0;
	m_sound_menu.sfxvolume_slider.maxValue			= 20;
	m_sound_menu.sfxvolume_slider.generic.statusBar	= "Volume Control";
	m_sound_menu.sfxvolume_amount.generic.type		= UITYPE_ACTION;
	m_sound_menu.sfxvolume_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_sound_menu.sfxvolume_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_sound_menu.sfxvolume_amount.generic.y			= y;

	m_sound_menu.cdtoggle_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_sound_menu.cdtoggle_toggle.generic.x			= 0;
	m_sound_menu.cdtoggle_toggle.generic.y			= y += UIFT_SIZEINC;
	m_sound_menu.cdtoggle_toggle.generic.name		= "CD music";
	m_sound_menu.cdtoggle_toggle.generic.callBack	= UpdateCDToggleFunc;
	m_sound_menu.cdtoggle_toggle.itemNames			= cd_music_items;
	m_sound_menu.cdtoggle_toggle.generic.statusBar	= "Toggle CD Play";

	// openal
	m_sound_menu.al_header.generic.type		= UITYPE_ACTION;
	m_sound_menu.al_header.generic.flags	= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_sound_menu.al_header.generic.x		= 0;
	m_sound_menu.al_header.generic.y		= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_sound_menu.al_header.generic.name		= "OpenAL Sound Settings";

	m_sound_menu.dopplerfactor_slider.generic.type		= UITYPE_SLIDER;
	m_sound_menu.dopplerfactor_slider.generic.x			= 0;
	m_sound_menu.dopplerfactor_slider.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_sound_menu.dopplerfactor_slider.generic.name		= "Doppler Factor";
	m_sound_menu.dopplerfactor_slider.generic.callBack	= ALDopFactorFunc;
	m_sound_menu.dopplerfactor_slider.minValue			= 0;
	m_sound_menu.dopplerfactor_slider.maxValue			= 10;
	m_sound_menu.dopplerfactor_slider.generic.statusBar	= "OpenAL Doppler Factor";
	m_sound_menu.dopplerfactor_amount.generic.type		= UITYPE_ACTION;
	m_sound_menu.dopplerfactor_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_sound_menu.dopplerfactor_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_sound_menu.dopplerfactor_amount.generic.y			= y;

	m_sound_menu.dopplervelocity_slider.generic.type		= UITYPE_SLIDER;
	m_sound_menu.dopplervelocity_slider.generic.x			= 0;
	m_sound_menu.dopplervelocity_slider.generic.y			= y += UIFT_SIZEINC;
	m_sound_menu.dopplervelocity_slider.generic.name		= "Doppler Velocity";
	m_sound_menu.dopplervelocity_slider.generic.callBack	= ALDopVelocityFunc;
	m_sound_menu.dopplervelocity_slider.minValue			= 1;
	m_sound_menu.dopplervelocity_slider.maxValue			= 10;
	m_sound_menu.dopplervelocity_slider.generic.statusBar	= "OpenAL Doppler Velocity";
	m_sound_menu.dopplervelocity_amount.generic.type		= UITYPE_ACTION;
	m_sound_menu.dopplervelocity_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_sound_menu.dopplervelocity_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_sound_menu.dopplervelocity_amount.generic.y			= y;

	m_sound_menu.al_extensions_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_sound_menu.al_extensions_toggle.generic.x			= 0;
	m_sound_menu.al_extensions_toggle.generic.y			= y += UIFT_SIZEINC*2;
	m_sound_menu.al_extensions_toggle.generic.name		= "OpenAL Extensions";
	m_sound_menu.al_extensions_toggle.generic.callBack	= ALExtensionsFunc;
	m_sound_menu.al_extensions_toggle.itemNames			= soundinit_items;
	m_sound_menu.al_extensions_toggle.generic.statusBar	= "Toggle OpenAL Extensions";

	m_sound_menu.al_ext_eax_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_sound_menu.al_ext_eax_toggle.generic.x			= 0;
	m_sound_menu.al_ext_eax_toggle.generic.y			= y += UIFT_SIZEINC;
	m_sound_menu.al_ext_eax_toggle.generic.name			= "EAX 2.0 extension";
	m_sound_menu.al_ext_eax_toggle.generic.callBack		= ALExtEAXFunc;
	m_sound_menu.al_ext_eax_toggle.itemNames			= soundinit_items;
	m_sound_menu.al_ext_eax_toggle.generic.statusBar	= "Toggle the OpenAL extension EAX 2.0";		

	// software
	m_sound_menu.sw_header.generic.type		= UITYPE_ACTION;
	m_sound_menu.sw_header.generic.flags	= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_sound_menu.sw_header.generic.x		= 0;
	m_sound_menu.sw_header.generic.y		= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_sound_menu.sw_header.generic.name		= "Software Sound Settings";

	m_sound_menu.sw_quality_list.generic.type		= UITYPE_SPINCONTROL;
	m_sound_menu.sw_quality_list.generic.x			= 0;
	m_sound_menu.sw_quality_list.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_sound_menu.sw_quality_list.generic.name		= "Sound quality";
	m_sound_menu.sw_quality_list.generic.callBack	= UpdateSoundQualityFunc;
	m_sound_menu.sw_quality_list.itemNames			= quality_items;
	m_sound_menu.sw_quality_list.generic.statusBar	= "Sound Quality";

	m_sound_menu.sw_combatibility_list.generic.type		= UITYPE_SPINCONTROL;
	m_sound_menu.sw_combatibility_list.generic.x			= 0;
	m_sound_menu.sw_combatibility_list.generic.y			= y += UIFT_SIZEINC;
	m_sound_menu.sw_combatibility_list.generic.name		= "Compatibility";
	m_sound_menu.sw_combatibility_list.generic.callBack	= UpdateSoundQualityFunc;
	m_sound_menu.sw_combatibility_list.itemNames			= compatibility_items;
	m_sound_menu.sw_combatibility_list.generic.statusBar	= "Primary/Secondary sound buffer";

	m_sound_menu.back_action.generic.type		= UITYPE_ACTION;
	m_sound_menu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_sound_menu.back_action.generic.x			= 0;
	m_sound_menu.back_action.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCLG;
	m_sound_menu.back_action.generic.name		= "< Back";
	m_sound_menu.back_action.generic.callBack	= Back_Menu;
	m_sound_menu.back_action.generic.statusBar	= "Back a menu";

	SoundMenu_SetValues ();

	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.banner);
	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.header);

	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.sound_toggle);
	
	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.sfxvolume_slider);
	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.sfxvolume_amount);
	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.cdtoggle_toggle);

	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.al_header);
	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.dopplerfactor_slider);
	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.dopplerfactor_amount);
	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.dopplervelocity_slider);
	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.dopplervelocity_amount);
	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.al_extensions_toggle);
	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.al_ext_eax_toggle);

	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.sw_header);
	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.sw_quality_list);
	UI_AddItem (&m_sound_menu.framework,		&m_sound_menu.sw_combatibility_list);

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
struct sfx_s *SoundMenu_Key (int key) {
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
