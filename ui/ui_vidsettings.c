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

#include "../ui/ui_local.h"

#define REF_OPENGL	0
#define REF_3DFX	1
#define REF_POWERVR	2
#define REF_VERITE	3

/*
====================================================================

	VIDEO SETTINGS MENU

====================================================================
*/

typedef struct m_vidsettingsmenu_s {
	uiFrameWork_t	framework;

	uiImage_t		banner;
	uiAction_t		header;

	uiList_t		ref_list;
	uiList_t		mode_list;

	uiList_t		tm_list;
	uiList_t		tb_list;
	uiList_t		bitd_list;
	uiList_t		stencilbuf_list;
	uiList_t		dispfreq_list;

	uiList_t		fullscreen_toggle;
	uiSlider_t		screensize_slider;
	uiAction_t		screensize_amount;
	uiSlider_t		brightness_slider;
	uiAction_t		brightness_amount;
	uiList_t		gammapics_toggle;

	uiSlider_t		texqual_slider;
	uiAction_t		texqual_amount;

	uiList_t		finish_toggle;

	uiAction_t		apply_action;
	uiAction_t		reset_action;
	uiAction_t		cancel_action;
} m_vidsettingsmenu_t;

m_vidsettingsmenu_t	m_vidsettings_menu;

void VIDSettingsMenu_Init (void);
static void ResetDefaults (void *unused) {
	VIDSettingsMenu_Init ();
}

static void CancelChanges (void *unused) {
	UI_PopMenu();
}


/*
================
VIDSettingsMenu_ApplyValues
================
*/
static void VIDSettingsMenu_ApplyValues (void *unused) {
	float	gamma;

	switch (m_vidsettings_menu.ref_list.curValue) {
	case REF_OPENGL:		uii.Cvar_ForceSet ("gl_driver", "opengl32");		break;
	case REF_3DFX:			uii.Cvar_ForceSet ("gl_driver", "3dfxgl");			break;
	case REF_POWERVR:		uii.Cvar_ForceSet ("gl_driver", "pvrgl");			break;
	case REF_VERITE:		uii.Cvar_ForceSet ("gl_driver", "veritegl");		break;
	}

	uii.Cvar_ForceSetValue ("gl_mode",			m_vidsettings_menu.mode_list.curValue);

	if (m_vidsettings_menu.tm_list.curValue == 1)		uii.Cvar_ForceSet ("gl_texturemode", "GL_LINEAR_MIPMAP_LINEAR");
	else if (m_vidsettings_menu.tm_list.curValue == 0)	uii.Cvar_ForceSet ("gl_texturemode", "GL_LINEAR_MIPMAP_NEAREST");

	if (m_vidsettings_menu.tb_list.curValue == 2)		uii.Cvar_ForceSet ("gl_texturebits", "32");
	else if (m_vidsettings_menu.tb_list.curValue == 1)	uii.Cvar_ForceSet ("gl_texturebits", "16");
	else												uii.Cvar_ForceSet ("gl_texturebits", "default");

	if (m_vidsettings_menu.bitd_list.curValue == 0)			uii.Cvar_ForceSet ("gl_bitdepth", "0");
	else if (m_vidsettings_menu.bitd_list.curValue == 1)	uii.Cvar_ForceSet ("gl_bitdepth", "16");
	else if (m_vidsettings_menu.bitd_list.curValue == 2)	uii.Cvar_ForceSet ("gl_bitdepth", "32");

	uii.Cvar_ForceSetValue ("gl_stencilbuffer",		m_vidsettings_menu.stencilbuf_list.curValue);

	if (m_vidsettings_menu.dispfreq_list.curValue == 1)			uii.Cvar_ForceSet ("r_displayfreq", "60");
	else if (m_vidsettings_menu.dispfreq_list.curValue == 2)	uii.Cvar_ForceSet ("r_displayfreq", "65");
	else if (m_vidsettings_menu.dispfreq_list.curValue == 3)	uii.Cvar_ForceSet ("r_displayfreq", "70");
	else if (m_vidsettings_menu.dispfreq_list.curValue == 4)	uii.Cvar_ForceSet ("r_displayfreq", "75");
	else if (m_vidsettings_menu.dispfreq_list.curValue == 5)	uii.Cvar_ForceSet ("r_displayfreq", "80");
	else if (m_vidsettings_menu.dispfreq_list.curValue == 6)	uii.Cvar_ForceSet ("r_displayfreq", "85");
	else if (m_vidsettings_menu.dispfreq_list.curValue == 7)	uii.Cvar_ForceSet ("r_displayfreq", "90");
	else if (m_vidsettings_menu.dispfreq_list.curValue == 8)	uii.Cvar_ForceSet ("r_displayfreq", "95");
	else if (m_vidsettings_menu.dispfreq_list.curValue == 9)	uii.Cvar_ForceSet ("r_displayfreq", "100");
	else if (m_vidsettings_menu.dispfreq_list.curValue == 10)	uii.Cvar_ForceSet ("r_displayfreq", "105");
	else if (m_vidsettings_menu.dispfreq_list.curValue == 11)	uii.Cvar_ForceSet ("r_displayfreq", "110");
	else if (m_vidsettings_menu.dispfreq_list.curValue == 12)	uii.Cvar_ForceSet ("r_displayfreq", "115");
	else if (m_vidsettings_menu.dispfreq_list.curValue == 13)	uii.Cvar_ForceSet ("r_displayfreq", "120");
	else														uii.Cvar_ForceSet ("r_displayfreq", "0");

	uii.Cvar_ForceSetValue ("vid_fullscreen",	m_vidsettings_menu.fullscreen_toggle.curValue);
	uii.Cvar_ForceSetValue ("viewsize",			m_vidsettings_menu.screensize_slider.curValue * 10);

	/*
	** invert sense so greater = brighter, and scale to a range of 0.5 to 1.3
	*/
	gamma = (0.8 - ((m_vidsettings_menu.brightness_slider.curValue * 0.1) - 0.5)) + 0.5;
	uii.Cvar_ForceSetValue ("vid_gamma",		gamma);

	uii.Cvar_ForceSetValue ("vid_gammapics",	m_vidsettings_menu.gammapics_toggle.curValue);

	uii.Cvar_ForceSetValue ("gl_picmip",		3 - m_vidsettings_menu.texqual_slider.curValue);
	uii.Cvar_ForceSetValue ("gl_finish",		m_vidsettings_menu.finish_toggle.curValue);

	uii.Cbuf_AddText ("vid_restart\n");
	UI_ForceMenuOff ();
}


/*
================
VIDSettingsMenu_SetValues
================
*/
static void VIDSettingsMenu_SetValues (void) {
	if (Q_stricmp (uii.Cvar_VariableString ("gl_driver"), "3dfxgl") == 0)		m_vidsettings_menu.ref_list.curValue = REF_3DFX;
	else if (Q_stricmp (uii.Cvar_VariableString ("gl_driver"), "pvrgl") == 0)	m_vidsettings_menu.ref_list.curValue = REF_POWERVR;
	else																		m_vidsettings_menu.ref_list.curValue = REF_OPENGL;

	uii.Cvar_ForceSetValue ("gl_mode",		clamp (uii.Cvar_VariableInteger ("gl_mode"), 0, 12));
	m_vidsettings_menu.mode_list.curValue	= uii.Cvar_VariableInteger ("gl_mode");

	if (!Q_stricmp (uii.Cvar_VariableString ("gl_texturemode"), "GL_LINEAR_MIPMAP_LINEAR"))			m_vidsettings_menu.tm_list.curValue = 1;
	else if (!Q_stricmp (uii.Cvar_VariableString ("gl_texturemode"), "GL_LINEAR_MIPMAP_NEAREST"))	m_vidsettings_menu.tm_list.curValue = 0;
	else																							m_vidsettings_menu.tm_list.curValue = 2;

	if (uii.Cvar_VariableValue ("gl_texturebits") == 32)		m_vidsettings_menu.tb_list.curValue = 2;
	else if (uii.Cvar_VariableValue ("gl_texturebits") == 16)	m_vidsettings_menu.tb_list.curValue = 1;
	else														m_vidsettings_menu.tb_list.curValue = 0;

	if (uii.Cvar_VariableValue ("gl_bitdepth") == 0)		m_vidsettings_menu.bitd_list.curValue = 0;
	else if (uii.Cvar_VariableValue ("gl_bitdepth") == 16)	m_vidsettings_menu.bitd_list.curValue = 1;
	else if (uii.Cvar_VariableValue ("gl_bitdepth") == 32)	m_vidsettings_menu.bitd_list.curValue = 2;

	uii.Cvar_ForceSetValue ("gl_stencilbuffer",		clamp (uii.Cvar_VariableInteger ("gl_stencilbuffer"), 0, 1));
	m_vidsettings_menu.stencilbuf_list.curValue	= uii.Cvar_VariableInteger ("gl_stencilbuffer");

	if (uii.Cvar_VariableValue ("r_displayfreq") == 60)			m_vidsettings_menu.dispfreq_list.curValue = 1;
	else if (uii.Cvar_VariableValue ("r_displayfreq") == 65)	m_vidsettings_menu.dispfreq_list.curValue = 2;
	else if (uii.Cvar_VariableValue ("r_displayfreq") == 70)	m_vidsettings_menu.dispfreq_list.curValue = 3;
	else if (uii.Cvar_VariableValue ("r_displayfreq") == 75)	m_vidsettings_menu.dispfreq_list.curValue = 4;
	else if (uii.Cvar_VariableValue ("r_displayfreq") == 80)	m_vidsettings_menu.dispfreq_list.curValue = 5;
	else if (uii.Cvar_VariableValue ("r_displayfreq") == 85)	m_vidsettings_menu.dispfreq_list.curValue = 6;
	else if (uii.Cvar_VariableValue ("r_displayfreq") == 90)	m_vidsettings_menu.dispfreq_list.curValue = 7;
	else if (uii.Cvar_VariableValue ("r_displayfreq") == 95)	m_vidsettings_menu.dispfreq_list.curValue = 8;
	else if (uii.Cvar_VariableValue ("r_displayfreq") == 100)	m_vidsettings_menu.dispfreq_list.curValue = 9;
	else if (uii.Cvar_VariableValue ("r_displayfreq") == 105)	m_vidsettings_menu.dispfreq_list.curValue = 10;
	else if (uii.Cvar_VariableValue ("r_displayfreq") == 110)	m_vidsettings_menu.dispfreq_list.curValue = 11;
	else if (uii.Cvar_VariableValue ("r_displayfreq") == 115)	m_vidsettings_menu.dispfreq_list.curValue = 12;
	else if (uii.Cvar_VariableValue ("r_displayfreq") == 120)	m_vidsettings_menu.dispfreq_list.curValue = 13;
	else														m_vidsettings_menu.dispfreq_list.curValue = 0;

	uii.Cvar_ForceSetValue ("vid_fullscreen",		clamp (uii.Cvar_VariableInteger ("vid_fullscreen"), 0, 1));
	m_vidsettings_menu.fullscreen_toggle.curValue	= uii.Cvar_VariableInteger ("vid_fullscreen");

	uii.Cvar_ForceSetValue ("viewsize",				clamp (uii.Cvar_VariableInteger ("viewsize"), 40, 100));
	m_vidsettings_menu.screensize_slider.curValue	= uii.Cvar_VariableInteger ("viewsize") * 0.1; // kthx view menu?

	uii.Cvar_ForceSetValue ("vid_gamma",			clamp (uii.Cvar_VariableValue ("vid_gamma"), 0.5, 1.3));
	m_vidsettings_menu.brightness_slider.curValue	= (1.3 - uii.Cvar_VariableValue ("vid_gamma") + 0.5) * 10;

	uii.Cvar_ForceSetValue ("vid_gammapics",		clamp (uii.Cvar_VariableInteger ("vid_gammapics"), 0, 1));
	m_vidsettings_menu.gammapics_toggle.curValue	= uii.Cvar_VariableInteger ("vid_gammapics");

	m_vidsettings_menu.texqual_slider.curValue		= 3 - uii.Cvar_VariableInteger ("gl_picmip");

	uii.Cvar_ForceSetValue ("gl_finish",		clamp (uii.Cvar_VariableInteger ("gl_finish"), 0, 1));
	m_vidsettings_menu.finish_toggle.curValue	= uii.Cvar_VariableInteger ("gl_finish");
}


/*
================
VIDSettingsMenu_Init
================
*/
void VIDSettingsMenu_Init (void) {
	static char *resolutions[] = {
		// 4:3
		"[320 240   ]",
		"[400 300   ]",
		"[512 384   ]",
		"[640 480   ]",
		"[800 600   ]",
		"[960 720   ]",
		"[1024 768  ]",
		"[1152 864  ]",
		"[1280 960  ]",
		"[1600 1200 ]",
		"[1920 1440 ]",
		"[2048 1536 ]",

		// widescreen
		"[1280 800 (ws)]",
		"[1440 900 (ws)]",
		0
	};

	static char *refs[] = {
		"[default OpenGL]",
		"[3Dfx OpenGL   ]",
		"[PowerVR OpenGL]",
		0
	};

	static char *yesno_names[] = {
		"no",
		"yes",
		0
	};

	static char *tm_names[] = {
		"bilinear (fast)",
		"trilinear (best)",
		"other",
		0
	};

	static char *tb_names[] = {
		"default",
		"16 bit",
		"32 bit",
		0
	};

	static char *bitd_names[] = {
		"desktop",
		"16 bit",
		"32 bit",
		"other",
		0
	};

	static char *dispfreq_names[] = {
		"desktop",
		"60",
		"65",
		"70",
		"75",
		"80",
		"85",
		"90",
		"95",
		"100",
		"105",
		"110",
		"115",
		"120",
		"other",
		0
	};

	float	y;

	m_vidsettings_menu.framework.x			= uis.vidWidth * 0.5;
	m_vidsettings_menu.framework.y			= 0;
	m_vidsettings_menu.framework.numItems	= 0;

	UI_Banner (&m_vidsettings_menu.banner, uiMedia.videoBanner, &y);

	m_vidsettings_menu.header.generic.type		= UITYPE_ACTION;
	m_vidsettings_menu.header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_vidsettings_menu.header.generic.x			= 0;
	m_vidsettings_menu.header.generic.y			= y += UIFT_SIZEINC;
	m_vidsettings_menu.header.generic.name		= "Video Settings";

	m_vidsettings_menu.ref_list.generic.type		= UITYPE_SPINCONTROL;
	m_vidsettings_menu.ref_list.generic.name		= "Mini-Driver";
	m_vidsettings_menu.ref_list.generic.x			= 0;
	m_vidsettings_menu.ref_list.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_vidsettings_menu.ref_list.itemNames			= refs;
	m_vidsettings_menu.ref_list.generic.statusBar	= "Renderer-Specific MiniDriver";

	m_vidsettings_menu.mode_list.generic.type		= UITYPE_SPINCONTROL;
	m_vidsettings_menu.mode_list.generic.name		= "Resolution";
	m_vidsettings_menu.mode_list.generic.x			= 0;
	m_vidsettings_menu.mode_list.generic.y			= y += UIFT_SIZEINC;
	m_vidsettings_menu.mode_list.itemNames			= resolutions;
	m_vidsettings_menu.mode_list.generic.statusBar	= "Resolution Selection";

	m_vidsettings_menu.tm_list.generic.type			= UITYPE_SPINCONTROL;
	m_vidsettings_menu.tm_list.generic.name			= "Texture Mode";
	m_vidsettings_menu.tm_list.generic.x			= 0;
	m_vidsettings_menu.tm_list.generic.y			= y += UIFT_SIZEINC*2;
	m_vidsettings_menu.tm_list.itemNames			= tm_names;
	m_vidsettings_menu.tm_list.generic.statusBar	= "Texture mode";

	m_vidsettings_menu.tb_list.generic.type			= UITYPE_SPINCONTROL;
	m_vidsettings_menu.tb_list.generic.name			= "Texture Bits";
	m_vidsettings_menu.tb_list.generic.x			= 0;
	m_vidsettings_menu.tb_list.generic.y			= y += UIFT_SIZEINC;
	m_vidsettings_menu.tb_list.itemNames			= tb_names;
	m_vidsettings_menu.tb_list.generic.statusBar	= "Texture bits";

	m_vidsettings_menu.bitd_list.generic.type		= UITYPE_SPINCONTROL;
	m_vidsettings_menu.bitd_list.generic.name		= "Bit Depth";
	m_vidsettings_menu.bitd_list.generic.x			= 0;
	m_vidsettings_menu.bitd_list.generic.y			= y += UIFT_SIZEINC;
	m_vidsettings_menu.bitd_list.itemNames			= bitd_names;
	m_vidsettings_menu.bitd_list.generic.statusBar	= "Display bit depth";

	m_vidsettings_menu.stencilbuf_list.generic.type			= UITYPE_SPINCONTROL;
	m_vidsettings_menu.stencilbuf_list.generic.name			= "Stencil Buffer";
	m_vidsettings_menu.stencilbuf_list.generic.x			= 0;
	m_vidsettings_menu.stencilbuf_list.generic.y			= y += UIFT_SIZEINC;
	m_vidsettings_menu.stencilbuf_list.itemNames			= yesno_names;
	m_vidsettings_menu.stencilbuf_list.generic.statusBar	= "Stencil buffer";

	m_vidsettings_menu.dispfreq_list.generic.type		= UITYPE_SPINCONTROL;
	m_vidsettings_menu.dispfreq_list.generic.name		= "Display Freq";
	m_vidsettings_menu.dispfreq_list.generic.x			= 0;
	m_vidsettings_menu.dispfreq_list.generic.y			= y += UIFT_SIZEINC;
	m_vidsettings_menu.dispfreq_list.itemNames			= dispfreq_names;
	m_vidsettings_menu.dispfreq_list.generic.statusBar	= "Display frequency";

	m_vidsettings_menu.fullscreen_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_vidsettings_menu.fullscreen_toggle.generic.x			= 0;
	m_vidsettings_menu.fullscreen_toggle.generic.y			= y += (UIFT_SIZEINC * 2);
	m_vidsettings_menu.fullscreen_toggle.generic.name		= "Fullscreen";
	m_vidsettings_menu.fullscreen_toggle.itemNames			= yesno_names;
	m_vidsettings_menu.fullscreen_toggle.generic.statusBar	= "Fullscreen or Window";

	m_vidsettings_menu.screensize_slider.generic.type		= UITYPE_SLIDER;
	m_vidsettings_menu.screensize_slider.generic.x			= 0;
	m_vidsettings_menu.screensize_slider.generic.y			= y += UIFT_SIZEINC;
	m_vidsettings_menu.screensize_slider.generic.name		= "Screen Size";
	m_vidsettings_menu.screensize_slider.minValue			= 4;
	m_vidsettings_menu.screensize_slider.maxValue			= 10;
	m_vidsettings_menu.screensize_slider.generic.statusBar	= "Screen Size";

	m_vidsettings_menu.brightness_slider.generic.type		= UITYPE_SLIDER;
	m_vidsettings_menu.brightness_slider.generic.x			= 0;
	m_vidsettings_menu.brightness_slider.generic.y			= y += UIFT_SIZEINC;
	m_vidsettings_menu.brightness_slider.generic.name		= "Brightness";
	m_vidsettings_menu.brightness_slider.minValue			= 5;
	m_vidsettings_menu.brightness_slider.maxValue			= 13;
	m_vidsettings_menu.brightness_slider.generic.statusBar	= "Brightness";

	m_vidsettings_menu.gammapics_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_vidsettings_menu.gammapics_toggle.generic.x			= 0;
	m_vidsettings_menu.gammapics_toggle.generic.y			= y += UIFT_SIZEINC;
	m_vidsettings_menu.gammapics_toggle.generic.name		= "Pic Gamma";
	m_vidsettings_menu.gammapics_toggle.itemNames			= yesno_names;
	m_vidsettings_menu.gammapics_toggle.generic.statusBar	= "Apply Gamma to Pics";

	m_vidsettings_menu.texqual_slider.generic.type		= UITYPE_SLIDER;
	m_vidsettings_menu.texqual_slider.generic.x			= 0;
	m_vidsettings_menu.texqual_slider.generic.y			= y += (UIFT_SIZEINC*2);
	m_vidsettings_menu.texqual_slider.generic.name		= "Texture Quality";
	m_vidsettings_menu.texqual_slider.minValue			= 0;
	m_vidsettings_menu.texqual_slider.maxValue			= 3;
	m_vidsettings_menu.texqual_slider.generic.statusBar	= "Texture upload quality";

	m_vidsettings_menu.finish_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_vidsettings_menu.finish_toggle.generic.x			= 0;
	m_vidsettings_menu.finish_toggle.generic.y			= y += (UIFT_SIZEINC*2);
	m_vidsettings_menu.finish_toggle.generic.name		= "Frame Sync";
	m_vidsettings_menu.finish_toggle.itemNames			= yesno_names;
	m_vidsettings_menu.finish_toggle.generic.statusBar	= "Framerate sync (waits on gl/buf/net commands to complete after each frame)";

	m_vidsettings_menu.apply_action.generic.type		= UITYPE_ACTION;
	m_vidsettings_menu.apply_action.generic.flags		= UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_vidsettings_menu.apply_action.generic.name		= "Apply";
	m_vidsettings_menu.apply_action.generic.x			= 0;
	m_vidsettings_menu.apply_action.generic.y			= y += UIFT_SIZEINCMED*2;
	m_vidsettings_menu.apply_action.generic.callBack	= VIDSettingsMenu_ApplyValues;
	m_vidsettings_menu.apply_action.generic.statusBar	= "Apply Changes";

	m_vidsettings_menu.reset_action.generic.type		= UITYPE_ACTION;
	m_vidsettings_menu.reset_action.generic.flags		= UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_vidsettings_menu.reset_action.generic.name		= "Reset";
	m_vidsettings_menu.reset_action.generic.x			= 0;
	m_vidsettings_menu.reset_action.generic.y			= y += UIFT_SIZEINCMED;
	m_vidsettings_menu.reset_action.generic.callBack	= ResetDefaults;
	m_vidsettings_menu.reset_action.generic.statusBar	= "Reset Changes";

	m_vidsettings_menu.cancel_action.generic.type		= UITYPE_ACTION;
	m_vidsettings_menu.cancel_action.generic.flags		= UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_vidsettings_menu.cancel_action.generic.name		= "Cancel";
	m_vidsettings_menu.cancel_action.generic.x			= 0;
	m_vidsettings_menu.cancel_action.generic.y			= y += UIFT_SIZEINCMED;
	m_vidsettings_menu.cancel_action.generic.callBack	= CancelChanges;
	m_vidsettings_menu.cancel_action.generic.statusBar	= "Closes the Menu";

	VIDSettingsMenu_SetValues ();

	UI_AddItem (&m_vidsettings_menu.framework,		&m_vidsettings_menu.banner);
	UI_AddItem (&m_vidsettings_menu.framework,		&m_vidsettings_menu.header);

	UI_AddItem (&m_vidsettings_menu.framework,		&m_vidsettings_menu.ref_list);
	UI_AddItem (&m_vidsettings_menu.framework,		&m_vidsettings_menu.mode_list);

	UI_AddItem (&m_vidsettings_menu.framework,		&m_vidsettings_menu.tm_list);
	UI_AddItem (&m_vidsettings_menu.framework,		&m_vidsettings_menu.tb_list);
	UI_AddItem (&m_vidsettings_menu.framework,		&m_vidsettings_menu.bitd_list);
	UI_AddItem (&m_vidsettings_menu.framework,		&m_vidsettings_menu.stencilbuf_list);
	UI_AddItem (&m_vidsettings_menu.framework,		&m_vidsettings_menu.dispfreq_list);

	UI_AddItem (&m_vidsettings_menu.framework,		&m_vidsettings_menu.fullscreen_toggle);
	UI_AddItem (&m_vidsettings_menu.framework,		&m_vidsettings_menu.screensize_slider);
	UI_AddItem (&m_vidsettings_menu.framework,		&m_vidsettings_menu.brightness_slider);
	UI_AddItem (&m_vidsettings_menu.framework,		&m_vidsettings_menu.gammapics_toggle);
	UI_AddItem (&m_vidsettings_menu.framework,		&m_vidsettings_menu.texqual_slider);
	UI_AddItem (&m_vidsettings_menu.framework,		&m_vidsettings_menu.finish_toggle);

	UI_AddItem (&m_vidsettings_menu.framework,		&m_vidsettings_menu.apply_action);
	UI_AddItem (&m_vidsettings_menu.framework,		&m_vidsettings_menu.reset_action);
	UI_AddItem (&m_vidsettings_menu.framework,		&m_vidsettings_menu.cancel_action);

	UI_Center (&m_vidsettings_menu.framework);
	UI_Setup (&m_vidsettings_menu.framework);
}


/*
================
VIDSettingsMenu_Draw
================
*/
void VIDSettingsMenu_Draw (void) {
	UI_Center (&m_vidsettings_menu.framework);
	UI_Setup (&m_vidsettings_menu.framework);

	UI_AdjustTextCursor (&m_vidsettings_menu.framework, 1);
	UI_Draw (&m_vidsettings_menu.framework);
}


/*
================
VIDSettingsMenu_Key
================
*/
struct sfx_s *VIDSettingsMenu_Key (int key) {
	return DefaultMenu_Key (&m_vidsettings_menu.framework, key);
}


/*
=============
UI_VIDSettingsMenu_f
=============
*/
void UI_VIDSettingsMenu_f (void) {
	VIDSettingsMenu_Init ();
	UI_PushMenu (&m_vidsettings_menu.framework, VIDSettingsMenu_Draw, VIDSettingsMenu_Key);
}
