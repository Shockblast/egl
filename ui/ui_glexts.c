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

/*
====================================================================

	OPENGL EXTENSIONS MENU

====================================================================
*/

typedef struct m_glextsmenu_s {
	uiFrameWork_t	framework;

	uiImage_t		banner;
	uiAction_t		header;

	uiList_t		extensions_toggle;

	uiList_t		multitexture_toggle;

	uiList_t		mtexcombine_toggle;
	uiSlider_t		overbrights_slider;
//	uiAction_t		overbrights_amount;

	uiList_t		bgra_toggle;
	uiList_t		cubemap_toggle;
	uiList_t		edgeclamp_toggle;
	uiList_t		genmipmap_toggle;
	uiList_t		texcompress_toggle;

	uiList_t		aniso_toggle;
	uiSlider_t		anisoamount_slider;
//	uiAction_t		anisoamount_amount;

	uiList_t		multisample_toggle;
	uiSlider_t		multisamples_slider;
//	uiAction_t		multisamples_amount;
	uiList_t		nv_msamplehint_list;

	uiList_t		nv_textureshader_list;

	uiAction_t		apply_action;
	uiAction_t		reset_action;
	uiAction_t		cancel_action;
} m_glextsmenu_t;

m_glextsmenu_t	m_glexts_menu;

#if 0
static void OverbrightBitsFunc (void *unused) {
	Q_snprintfz (m_glexts_menu.overbrights_amount.generic.name,
				sizeof (char) * 2,
				"%.0f", m_glexts_menu.overbrights_slider.curValue);
}


static void AnisoAmountFunc (void *unused) {
	Q_snprintfz (m_glexts_menu.anisoamount_amount.generic.name,
				sizeof (char) * 3,
				"%1.0f", m_glexts_menu.anisoamount_slider.curValue);
}


static void MultisamplesFunc (void *unused) {
	Q_snprintfz (m_glexts_menu.multisamples_amount.generic.name,
				sizeof (char) * 2,
				"%.0f", m_glexts_menu.multisamples_slider.curValue * 2);
}
#endif

void GLExtsMenu_SetValues (void);
static void ResetDefaults (void *unused) {
	GLExtsMenu_SetValues ();
}

static void CancelChanges (void *unused) {
	UI_PopMenu();
}


/*
=============
GLExtsMenu_ApplyValues
=============
*/
static void GLExtsMenu_ApplyValues (void *unused) {
	uii.Cvar_ForceSetValue ("gl_extensions",		m_glexts_menu.extensions_toggle.curValue);

	uii.Cvar_ForceSetValue ("gl_ext_multitexture",	m_glexts_menu.multitexture_toggle.curValue);

	uii.Cvar_ForceSetValue ("gl_ext_mtexcombine",	m_glexts_menu.mtexcombine_toggle.curValue);
	uii.Cvar_ForceSetValue ("r_overbrightbits",		m_glexts_menu.overbrights_slider.curValue * 2);

	uii.Cvar_ForceSetValue ("gl_ext_bgra",					m_glexts_menu.bgra_toggle.curValue);
	uii.Cvar_ForceSetValue ("gl_arb_texture_cube_map",		m_glexts_menu.cubemap_toggle.curValue);
	uii.Cvar_ForceSetValue ("gl_ext_texture_edge_clamp",	m_glexts_menu.edgeclamp_toggle.curValue);
	uii.Cvar_ForceSetValue ("gl_sgis_generate_mipmap",		m_glexts_menu.genmipmap_toggle.curValue);
	uii.Cvar_ForceSetValue ("gl_arb_texture_compression",	m_glexts_menu.texcompress_toggle.curValue);

	uii.Cvar_ForceSetValue ("gl_ext_texture_filter_anisotropic",	m_glexts_menu.aniso_toggle.curValue);
	uii.Cvar_ForceSetValue ("gl_ext_max_anisotropy",				m_glexts_menu.anisoamount_slider.curValue);

	uii.Cvar_ForceSetValue ("gl_ext_multisample",	m_glexts_menu.multisample_toggle.curValue);
	uii.Cvar_ForceSetValue ("gl_ext_samples",		m_glexts_menu.multisamples_slider.curValue * 2);

	if (m_glexts_menu.nv_msamplehint_list.curValue == 0)	uii.Cvar_Set ("gl_nv_multisample_filter_hint",		"fastest");
	else													uii.Cvar_Set ("gl_nv_multisample_filter_hint",		"nicest");

	uii.Cvar_ForceSetValue ("gl_nv_texture_shader",			m_glexts_menu.nv_textureshader_list.curValue);

	uii.Cbuf_AddText ("vid_restart\n");
	UI_ForceMenuOff ();
}


/*
=============
GLExtsMenu_SetValues
=============
*/
void GLExtsMenu_SetValues (void) {
	uii.Cvar_ForceSetValue ("gl_extensions",	clamp (uii.Cvar_VariableInteger ("gl_extensions"), 0, 1));
	m_glexts_menu.extensions_toggle.curValue	= uii.Cvar_VariableInteger ("gl_extensions");

	uii.Cvar_ForceSetValue ("gl_ext_multitexture",	clamp (uii.Cvar_VariableInteger ("gl_ext_multitexture"), 0, 1));
	m_glexts_menu.multitexture_toggle.curValue		= uii.Cvar_VariableInteger ("gl_ext_multitexture");

	uii.Cvar_ForceSetValue ("gl_ext_mtexcombine",	clamp (uii.Cvar_VariableInteger ("gl_ext_mtexcombine"), 0, 1));
	m_glexts_menu.mtexcombine_toggle.curValue		= uii.Cvar_VariableInteger ("gl_ext_mtexcombine");

	uii.Cvar_ForceSetValue ("r_overbrightbits",	clamp (uii.Cvar_VariableInteger ("r_overbrightbits"), 0, 4));
	m_glexts_menu.overbrights_slider.curValue	= uii.Cvar_VariableInteger ("r_overbrightbits") * 0.5;
//	m_glexts_menu.overbrights_amount.generic.name	= uii.Cvar_VariableString ("r_overbrightbits");

	uii.Cvar_ForceSetValue ("gl_ext_bgra",			clamp (uii.Cvar_VariableInteger ("gl_ext_bgra"), 0, 1));
	m_glexts_menu.bgra_toggle.curValue				= uii.Cvar_VariableInteger ("gl_ext_bgra");

	uii.Cvar_ForceSetValue ("gl_arb_texture_cube_map",	clamp (uii.Cvar_VariableInteger ("gl_arb_texture_cube_map"), 0, 1));
	m_glexts_menu.cubemap_toggle.curValue				= uii.Cvar_VariableInteger ("gl_arb_texture_cube_map");

	uii.Cvar_ForceSetValue ("gl_ext_texture_edge_clamp",	clamp (uii.Cvar_VariableInteger ("gl_ext_texture_edge_clamp"), 0, 1));
	m_glexts_menu.edgeclamp_toggle.curValue					= uii.Cvar_VariableInteger ("gl_ext_texture_edge_clamp");

	uii.Cvar_ForceSetValue ("gl_sgis_generate_mipmap",	clamp (uii.Cvar_VariableInteger ("gl_sgis_generate_mipmap"), 0, 1));
	m_glexts_menu.genmipmap_toggle.curValue				= uii.Cvar_VariableInteger ("gl_sgis_generate_mipmap");

	uii.Cvar_ForceSetValue ("gl_arb_texture_compression",	clamp (uii.Cvar_VariableInteger ("gl_arb_texture_compression"), 0, 1));
	m_glexts_menu.texcompress_toggle.curValue				= uii.Cvar_VariableInteger ("gl_arb_texture_compression");

	uii.Cvar_ForceSetValue ("gl_ext_texture_filter_anisotropic",	clamp (uii.Cvar_VariableInteger ("gl_ext_texture_filter_anisotropic"), 0, 1));
	m_glexts_menu.aniso_toggle.curValue								= uii.Cvar_VariableInteger ("gl_ext_texture_filter_anisotropic");

	uii.Cvar_ForceSetValue ("gl_ext_max_anisotropy",	clamp (uii.Cvar_VariableInteger ("gl_ext_max_anisotropy"), 0, 16));
	m_glexts_menu.anisoamount_slider.curValue			= uii.Cvar_VariableInteger ("gl_ext_max_anisotropy");
//	m_glexts_menu.anisoamount_amount.generic.name		= uii.Cvar_VariableString ("gl_ext_max_anisotropy");

	uii.Cvar_ForceSetValue ("gl_ext_multisample",	clamp (uii.Cvar_VariableInteger ("gl_ext_multisample"), 0, 1));
	m_glexts_menu.multisample_toggle.curValue		= uii.Cvar_VariableInteger ("gl_ext_multisample");

	uii.Cvar_ForceSetValue ("gl_ext_samples",		clamp (uii.Cvar_VariableInteger ("gl_ext_samples"), 0, 4));
	m_glexts_menu.multisamples_slider.curValue		= uii.Cvar_VariableInteger ("gl_ext_samples") / 2;
//	m_glexts_menu.multisamples_amount.generic.name	= uii.Cvar_VariableString ("gl_ext_samples");

	if (!Q_stricmp (uii.Cvar_VariableString ("gl_nv_multisample_filter_hint"), "fastest"))
		m_glexts_menu.nv_msamplehint_list.curValue = 0;
	else
		m_glexts_menu.nv_msamplehint_list.curValue = 1;

	uii.Cvar_ForceSetValue ("gl_nv_texture_shader",		clamp (uii.Cvar_VariableInteger ("gl_nv_texture_shader"), 0, 1));
	m_glexts_menu.nv_textureshader_list.curValue		= uii.Cvar_VariableInteger ("gl_nv_texture_shader");
}


/*
=============
GLExtsMenu_Init
=============
*/
void GLExtsMenu_Init (void) {
	static char *yesno_names[] = {
		"no",
		"yes",
		0
	};

	static char *fastnice_names[] = {
		"fastest",
		"nicest",
		0
	};

	float	y;

	m_glexts_menu.framework.x			= uis.vidWidth * 0.5;
	m_glexts_menu.framework.y			= 0;
	m_glexts_menu.framework.numItems	= 0;

	UI_Banner (&m_glexts_menu.banner, uiMedia.videoBanner, &y);

	m_glexts_menu.header.generic.type		= UITYPE_ACTION;
	m_glexts_menu.header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_glexts_menu.header.generic.x			= 0;
	m_glexts_menu.header.generic.y			= y += UIFT_SIZEINC;
	m_glexts_menu.header.generic.name		= "OpenGL Extensions";

	m_glexts_menu.extensions_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_glexts_menu.extensions_toggle.generic.x			= 0;
	m_glexts_menu.extensions_toggle.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_glexts_menu.extensions_toggle.generic.name		= "Extensions";
	m_glexts_menu.extensions_toggle.itemNames			= yesno_names;
	m_glexts_menu.extensions_toggle.generic.statusBar	= "Toggles the use of any of the below extensions entirely";

	m_glexts_menu.multitexture_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_glexts_menu.multitexture_toggle.generic.x			= 0;
	m_glexts_menu.multitexture_toggle.generic.y			= y += UIFT_SIZEINC*2;
	m_glexts_menu.multitexture_toggle.generic.name		= "Multitexture";
	m_glexts_menu.multitexture_toggle.itemNames			= yesno_names;
	m_glexts_menu.multitexture_toggle.generic.statusBar	= "Multitexturing (increases performance)";

	m_glexts_menu.mtexcombine_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_glexts_menu.mtexcombine_toggle.generic.x			= 0;
	m_glexts_menu.mtexcombine_toggle.generic.y			= y += UIFT_SIZEINC*2;
	m_glexts_menu.mtexcombine_toggle.generic.name		= "Multitexture Combine";
	m_glexts_menu.mtexcombine_toggle.itemNames			= yesno_names;
	m_glexts_menu.mtexcombine_toggle.generic.statusBar	= "Multitexture Combine Extension (used in overbrights)";

	m_glexts_menu.overbrights_slider.generic.type		= UITYPE_SLIDER;
	m_glexts_menu.overbrights_slider.generic.x			= 0;
	m_glexts_menu.overbrights_slider.generic.y			= y += UIFT_SIZEINC;
	m_glexts_menu.overbrights_slider.generic.name		= "Overbright Bits";
//	m_glexts_menu.overbrights_slider.generic.callBack	= OverbrightBitsFunc;
	m_glexts_menu.overbrights_slider.minValue			= 0.5;
	m_glexts_menu.overbrights_slider.maxValue			= 2;
	m_glexts_menu.overbrights_slider.generic.statusBar	= "Texture RGB Scaling";
	/*m_glexts_menu.overbrights_amount.generic.type		= UITYPE_ACTION;
	m_glexts_menu.overbrights_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_glexts_menu.overbrights_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_glexts_menu.overbrights_amount.generic.y			= y;*/

	m_glexts_menu.bgra_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_glexts_menu.bgra_toggle.generic.x			= 0;
	m_glexts_menu.bgra_toggle.generic.y			= y += UIFT_SIZEINC*2;
	m_glexts_menu.bgra_toggle.generic.name		= "RGBA -> BGRA";
	m_glexts_menu.bgra_toggle.itemNames			= yesno_names;
	m_glexts_menu.bgra_toggle.generic.statusBar	= "Used in TGA screenshot taking for a speed-up";

	m_glexts_menu.cubemap_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_glexts_menu.cubemap_toggle.generic.x			= 0;
	m_glexts_menu.cubemap_toggle.generic.y			= y += UIFT_SIZEINC;
	m_glexts_menu.cubemap_toggle.generic.name		= "Cube Mapping";
	m_glexts_menu.cubemap_toggle.itemNames			= yesno_names;
	m_glexts_menu.cubemap_toggle.generic.statusBar	= "Enables the use of cubemapping";

	m_glexts_menu.edgeclamp_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_glexts_menu.edgeclamp_toggle.generic.x			= 0;
	m_glexts_menu.edgeclamp_toggle.generic.y			= y += UIFT_SIZEINC;
	m_glexts_menu.edgeclamp_toggle.generic.name			= "Edge Clamping";
	m_glexts_menu.edgeclamp_toggle.itemNames			= yesno_names;
	m_glexts_menu.edgeclamp_toggle.generic.statusBar	= "Use an extension for clamping specified textures";

	m_glexts_menu.genmipmap_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_glexts_menu.genmipmap_toggle.generic.x			= 0;
	m_glexts_menu.genmipmap_toggle.generic.y			= y += UIFT_SIZEINC;
	m_glexts_menu.genmipmap_toggle.generic.name			= "Mipmap Generation";
	m_glexts_menu.genmipmap_toggle.itemNames			= yesno_names;
	m_glexts_menu.genmipmap_toggle.generic.statusBar	= "Hardware mipmap generation";

	m_glexts_menu.texcompress_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_glexts_menu.texcompress_toggle.generic.x			= 0;
	m_glexts_menu.texcompress_toggle.generic.y			= y += UIFT_SIZEINC;
	m_glexts_menu.texcompress_toggle.generic.name		= "Texture Compression";
	m_glexts_menu.texcompress_toggle.itemNames			= yesno_names;
	m_glexts_menu.texcompress_toggle.generic.statusBar	= "Texture compression (quality sapping, performance increasing)";

	m_glexts_menu.aniso_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_glexts_menu.aniso_toggle.generic.x			= 0;
	m_glexts_menu.aniso_toggle.generic.y			= y += UIFT_SIZEINC*2;
	m_glexts_menu.aniso_toggle.generic.name			= "Anisotropic Filtering";
	m_glexts_menu.aniso_toggle.itemNames			= yesno_names;
	m_glexts_menu.aniso_toggle.generic.statusBar	= "Anisotropic mipmap filtering";

	m_glexts_menu.anisoamount_slider.generic.type		= UITYPE_SLIDER;
	m_glexts_menu.anisoamount_slider.generic.x			= 0;
	m_glexts_menu.anisoamount_slider.generic.y			= y += UIFT_SIZEINC;
	m_glexts_menu.anisoamount_slider.generic.name		= "Maximum Anisotropy";
//	m_glexts_menu.anisoamount_slider.generic.callBack	= AnisoAmountFunc;
	m_glexts_menu.anisoamount_slider.minValue			= 1;
	m_glexts_menu.anisoamount_slider.maxValue			= 16;
	m_glexts_menu.anisoamount_slider.generic.statusBar	= "Maximum anisotropic filtering";
	/*m_glexts_menu.anisoamount_amount.generic.type		= UITYPE_ACTION;
	m_glexts_menu.anisoamount_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_glexts_menu.anisoamount_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_glexts_menu.anisoamount_amount.generic.y			= y;*/

	m_glexts_menu.multisample_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_glexts_menu.multisample_toggle.generic.x			= 0;
	m_glexts_menu.multisample_toggle.generic.y			= y += UIFT_SIZEINC*2;
	m_glexts_menu.multisample_toggle.generic.name		= "Multisampling";
	m_glexts_menu.multisample_toggle.itemNames			= yesno_names;
	m_glexts_menu.multisample_toggle.generic.statusBar	= "Fullscreen Antialiasing";

	m_glexts_menu.multisamples_slider.generic.type		= UITYPE_SLIDER;
	m_glexts_menu.multisamples_slider.generic.x			= 0;
	m_glexts_menu.multisamples_slider.generic.y			= y += UIFT_SIZEINC;
	m_glexts_menu.multisamples_slider.generic.name		= "Sample count";
//	m_glexts_menu.multisamples_slider.generic.callBack	= MultisamplesFunc;
	m_glexts_menu.multisamples_slider.minValue			= 0;
	m_glexts_menu.multisamples_slider.maxValue			= 2;
	m_glexts_menu.multisamples_slider.generic.statusBar	= "Antialiasing samples";
	/*m_glexts_menu.multisamples_amount.generic.type		= UITYPE_ACTION;
	m_glexts_menu.multisamples_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_glexts_menu.multisamples_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_glexts_menu.multisamples_amount.generic.y			= y;*/

	m_glexts_menu.nv_msamplehint_list.generic.type		= UITYPE_SPINCONTROL;
	m_glexts_menu.nv_msamplehint_list.generic.x			= 0;
	m_glexts_menu.nv_msamplehint_list.generic.y			= y += UIFT_SIZEINC;
	m_glexts_menu.nv_msamplehint_list.generic.name		= "Sample quality";
	m_glexts_menu.nv_msamplehint_list.itemNames			= fastnice_names;
	m_glexts_menu.nv_msamplehint_list.generic.statusBar	= "NVidia GF4+ Antialiasing quality";

	m_glexts_menu.nv_textureshader_list.generic.type		= UITYPE_SPINCONTROL;
	m_glexts_menu.nv_textureshader_list.generic.x			= 0;
	m_glexts_menu.nv_textureshader_list.generic.y			= y += UIFT_SIZEINC * 2;
	m_glexts_menu.nv_textureshader_list.generic.name		= "nVidia Texture Shader";
	m_glexts_menu.nv_textureshader_list.itemNames			= yesno_names;
	m_glexts_menu.nv_textureshader_list.generic.statusBar	= "nVidia texture shaders, used on turb surfs";

	m_glexts_menu.apply_action.generic.type			= UITYPE_ACTION;
	m_glexts_menu.apply_action.generic.flags		= UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_glexts_menu.apply_action.generic.name			= "Apply";
	m_glexts_menu.apply_action.generic.x			= 0;
	m_glexts_menu.apply_action.generic.y			= y += UIFT_SIZEINCMED*2;
	m_glexts_menu.apply_action.generic.callBack		= GLExtsMenu_ApplyValues;
	m_glexts_menu.apply_action.generic.statusBar	= "Apply Changes";

	m_glexts_menu.reset_action.generic.type			= UITYPE_ACTION;
	m_glexts_menu.reset_action.generic.flags		= UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_glexts_menu.reset_action.generic.name			= "Reset";
	m_glexts_menu.reset_action.generic.x			= 0;
	m_glexts_menu.reset_action.generic.y			= y += UIFT_SIZEINCMED;
	m_glexts_menu.reset_action.generic.callBack		= ResetDefaults;
	m_glexts_menu.reset_action.generic.statusBar	= "Reset Changes";

	m_glexts_menu.cancel_action.generic.type		= UITYPE_ACTION;
	m_glexts_menu.cancel_action.generic.flags		= UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_glexts_menu.cancel_action.generic.name		= "Cancel";
	m_glexts_menu.cancel_action.generic.x			= 0;
	m_glexts_menu.cancel_action.generic.y			= y += UIFT_SIZEINCMED;
	m_glexts_menu.cancel_action.generic.callBack	= CancelChanges;
	m_glexts_menu.cancel_action.generic.statusBar	= "Closes the Menu";

	GLExtsMenu_SetValues ();

	UI_AddItem (&m_glexts_menu.framework,			&m_glexts_menu.banner);
	UI_AddItem (&m_glexts_menu.framework,			&m_glexts_menu.header);

	UI_AddItem (&m_glexts_menu.framework,			&m_glexts_menu.extensions_toggle);

	UI_AddItem (&m_glexts_menu.framework,			&m_glexts_menu.multitexture_toggle);

	UI_AddItem (&m_glexts_menu.framework,			&m_glexts_menu.mtexcombine_toggle);
	UI_AddItem (&m_glexts_menu.framework,			&m_glexts_menu.overbrights_slider);
//	UI_AddItem (&m_glexts_menu.framework,			&m_glexts_menu.overbrights_amount);

	UI_AddItem (&m_glexts_menu.framework,			&m_glexts_menu.bgra_toggle);
	UI_AddItem (&m_glexts_menu.framework,			&m_glexts_menu.cubemap_toggle);
	UI_AddItem (&m_glexts_menu.framework,			&m_glexts_menu.edgeclamp_toggle);
	UI_AddItem (&m_glexts_menu.framework,			&m_glexts_menu.genmipmap_toggle);
	UI_AddItem (&m_glexts_menu.framework,			&m_glexts_menu.texcompress_toggle);

	UI_AddItem (&m_glexts_menu.framework,			&m_glexts_menu.aniso_toggle);
	UI_AddItem (&m_glexts_menu.framework,			&m_glexts_menu.anisoamount_slider);
//	UI_AddItem (&m_glexts_menu.framework,			&m_glexts_menu.anisoamount_amount);

	UI_AddItem (&m_glexts_menu.framework,			&m_glexts_menu.multisample_toggle);
	UI_AddItem (&m_glexts_menu.framework,			&m_glexts_menu.multisamples_slider);
//	UI_AddItem (&m_glexts_menu.framework,			&m_glexts_menu.multisamples_amount);
	UI_AddItem (&m_glexts_menu.framework,			&m_glexts_menu.nv_msamplehint_list);

	UI_AddItem (&m_glexts_menu.framework,			&m_glexts_menu.nv_textureshader_list);

	UI_AddItem (&m_glexts_menu.framework,			&m_glexts_menu.apply_action);
	UI_AddItem (&m_glexts_menu.framework,			&m_glexts_menu.reset_action);
	UI_AddItem (&m_glexts_menu.framework,			&m_glexts_menu.cancel_action);

	UI_Center (&m_glexts_menu.framework);
	UI_Setup (&m_glexts_menu.framework);
}


/*
=============
GLExtsMenu_Draw
=============
*/
void GLExtsMenu_Draw (void) {
	UI_Center (&m_glexts_menu.framework);
	UI_Setup (&m_glexts_menu.framework);

	UI_AdjustTextCursor (&m_glexts_menu.framework, 1);
	UI_Draw (&m_glexts_menu.framework);
}


/*
=============
GLExtsMenu_Key
=============
*/
struct sfx_s *GLExtsMenu_Key (int key) {
	return DefaultMenu_Key (&m_glexts_menu.framework, key);
}


/*
=============
UI_GLExtsMenu_f
=============
*/
void UI_GLExtsMenu_f (void) {
	GLExtsMenu_Init ();
	UI_PushMenu (&m_glexts_menu.framework, GLExtsMenu_Draw, GLExtsMenu_Key);
}
