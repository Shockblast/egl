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

typedef struct m_extensionsMenu_s {
	// Menu items
	uiFrameWork_t		frameWork;

	uiImage_t			banner;
	uiAction_t			header;

	uiList_t			extensions_toggle;

	uiList_t			multitexture_toggle;

	uiList_t			mtexcombine_toggle;

	uiList_t			cubemap_toggle;
	uiList_t			edgeclamp_toggle;
	uiList_t			genmipmap_toggle;
	uiList_t			texcompress_toggle;

	uiList_t			aniso_toggle;
	uiSlider_t			anisoamount_slider;

	uiAction_t			apply_action;
	uiAction_t			reset_action;
	uiAction_t			cancel_action;
} m_extensionsMenu_t;

static m_extensionsMenu_t	m_extensionsMenu;

static void GLExtsMenu_SetValues (void);
static void ResetDefaults (void *unused)
{
	GLExtsMenu_SetValues ();
}

static void CancelChanges (void *unused)
{
	UI_PopMenu();
}


/*
=============
GLExtsMenu_ApplyValues
=============
*/
static void GLExtsMenu_ApplyValues (void *unused)
{
	uii.Cvar_ForceSetValue ("gl_extensions",		m_extensionsMenu.extensions_toggle.curValue);

	uii.Cvar_ForceSetValue ("gl_ext_multitexture",	m_extensionsMenu.multitexture_toggle.curValue);

	uii.Cvar_ForceSetValue ("gl_ext_texture_env_combine",	m_extensionsMenu.mtexcombine_toggle.curValue);

	uii.Cvar_ForceSetValue ("gl_arb_texture_cube_map",		m_extensionsMenu.cubemap_toggle.curValue);
	uii.Cvar_ForceSetValue ("gl_ext_texture_edge_clamp",	m_extensionsMenu.edgeclamp_toggle.curValue);
	uii.Cvar_ForceSetValue ("gl_sgis_generate_mipmap",		m_extensionsMenu.genmipmap_toggle.curValue);
	uii.Cvar_ForceSetValue ("gl_arb_texture_compression",	m_extensionsMenu.texcompress_toggle.curValue);

	uii.Cvar_ForceSetValue ("gl_ext_texture_filter_anisotropic",	m_extensionsMenu.aniso_toggle.curValue);
	uii.Cvar_ForceSetValue ("gl_ext_max_anisotropy",				m_extensionsMenu.anisoamount_slider.curValue);

	uii.Cbuf_AddText ("vid_restart\n");
	UI_ForceMenuOff ();
}


/*
=============
GLExtsMenu_SetValues
=============
*/
static void GLExtsMenu_SetValues (void)
{
	uii.Cvar_ForceSetValue ("gl_extensions",	clamp (uii.Cvar_VariableInteger ("gl_extensions"), 0, 1));
	m_extensionsMenu.extensions_toggle.curValue	= uii.Cvar_VariableInteger ("gl_extensions");

	uii.Cvar_ForceSetValue ("gl_ext_multitexture",	clamp (uii.Cvar_VariableInteger ("gl_ext_multitexture"), 0, 1));
	m_extensionsMenu.multitexture_toggle.curValue		= uii.Cvar_VariableInteger ("gl_ext_multitexture");

	uii.Cvar_ForceSetValue ("gl_ext_texture_env_combine",	clamp (uii.Cvar_VariableInteger ("gl_ext_texture_env_combine"), 0, 1));
	m_extensionsMenu.mtexcombine_toggle.curValue				= uii.Cvar_VariableInteger ("gl_ext_texture_env_combine");

	uii.Cvar_ForceSetValue ("gl_arb_texture_cube_map",	clamp (uii.Cvar_VariableInteger ("gl_arb_texture_cube_map"), 0, 1));
	m_extensionsMenu.cubemap_toggle.curValue				= uii.Cvar_VariableInteger ("gl_arb_texture_cube_map");

	uii.Cvar_ForceSetValue ("gl_ext_texture_edge_clamp",	clamp (uii.Cvar_VariableInteger ("gl_ext_texture_edge_clamp"), 0, 1));
	m_extensionsMenu.edgeclamp_toggle.curValue					= uii.Cvar_VariableInteger ("gl_ext_texture_edge_clamp");

	uii.Cvar_ForceSetValue ("gl_sgis_generate_mipmap",	clamp (uii.Cvar_VariableInteger ("gl_sgis_generate_mipmap"), 0, 1));
	m_extensionsMenu.genmipmap_toggle.curValue				= uii.Cvar_VariableInteger ("gl_sgis_generate_mipmap");

	uii.Cvar_ForceSetValue ("gl_arb_texture_compression",	clamp (uii.Cvar_VariableInteger ("gl_arb_texture_compression"), 0, 1));
	m_extensionsMenu.texcompress_toggle.curValue				= uii.Cvar_VariableInteger ("gl_arb_texture_compression");

	uii.Cvar_ForceSetValue ("gl_ext_texture_filter_anisotropic",	clamp (uii.Cvar_VariableInteger ("gl_ext_texture_filter_anisotropic"), 0, 1));
	m_extensionsMenu.aniso_toggle.curValue								= uii.Cvar_VariableInteger ("gl_ext_texture_filter_anisotropic");

	uii.Cvar_ForceSetValue ("gl_ext_max_anisotropy",	clamp (uii.Cvar_VariableInteger ("gl_ext_max_anisotropy"), 0, 16));
	m_extensionsMenu.anisoamount_slider.curValue			= uii.Cvar_VariableInteger ("gl_ext_max_anisotropy");
}


/*
=============
GLExtsMenu_Init
=============
*/
static void GLExtsMenu_Init (void)
{
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

	UI_StartFramework (&m_extensionsMenu.frameWork);

	m_extensionsMenu.banner.generic.type		= UITYPE_IMAGE;
	m_extensionsMenu.banner.generic.flags		= UIF_NOSELECT|UIF_CENTERED;
	m_extensionsMenu.banner.generic.name		= NULL;
	m_extensionsMenu.banner.shader				= uiMedia.banners.video;

	m_extensionsMenu.header.generic.type		= UITYPE_ACTION;
	m_extensionsMenu.header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_extensionsMenu.header.generic.name		= "OpenGL Extensions";

	m_extensionsMenu.extensions_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_extensionsMenu.extensions_toggle.generic.name			= "Extensions";
	m_extensionsMenu.extensions_toggle.itemNames			= yesno_names;
	m_extensionsMenu.extensions_toggle.generic.statusBar	= "Toggles the use of any of the below extensions entirely";

	m_extensionsMenu.multitexture_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_extensionsMenu.multitexture_toggle.generic.name		= "Multitexture";
	m_extensionsMenu.multitexture_toggle.itemNames			= yesno_names;
	m_extensionsMenu.multitexture_toggle.generic.statusBar	= "Multitexturing (increases performance)";

	m_extensionsMenu.mtexcombine_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_extensionsMenu.mtexcombine_toggle.generic.name		= "Multitexture Combine";
	m_extensionsMenu.mtexcombine_toggle.itemNames			= yesno_names;
	m_extensionsMenu.mtexcombine_toggle.generic.statusBar	= "Multitexture Combine Extension";

	m_extensionsMenu.cubemap_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_extensionsMenu.cubemap_toggle.generic.name		= "Cube Mapping";
	m_extensionsMenu.cubemap_toggle.itemNames			= yesno_names;
	m_extensionsMenu.cubemap_toggle.generic.statusBar	= "Enables the use of cubemapping";

	m_extensionsMenu.edgeclamp_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_extensionsMenu.edgeclamp_toggle.generic.name		= "Edge Clamping";
	m_extensionsMenu.edgeclamp_toggle.itemNames			= yesno_names;
	m_extensionsMenu.edgeclamp_toggle.generic.statusBar	= "Use an extension for clamping specified textures";

	m_extensionsMenu.genmipmap_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_extensionsMenu.genmipmap_toggle.generic.name		= "Mipmap Generation";
	m_extensionsMenu.genmipmap_toggle.itemNames			= yesno_names;
	m_extensionsMenu.genmipmap_toggle.generic.statusBar	= "Hardware mipmap generation";

	m_extensionsMenu.texcompress_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_extensionsMenu.texcompress_toggle.generic.name		= "Texture Compression";
	m_extensionsMenu.texcompress_toggle.itemNames			= yesno_names;
	m_extensionsMenu.texcompress_toggle.generic.statusBar	= "Texture compression (quality sapping, performance increasing)";

	m_extensionsMenu.aniso_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_extensionsMenu.aniso_toggle.generic.name		= "Anisotropic Filtering";
	m_extensionsMenu.aniso_toggle.itemNames			= yesno_names;
	m_extensionsMenu.aniso_toggle.generic.statusBar	= "Anisotropic mipmap filtering";

	m_extensionsMenu.anisoamount_slider.generic.type		= UITYPE_SLIDER;
	m_extensionsMenu.anisoamount_slider.generic.name		= "Maximum Anisotropy";
	m_extensionsMenu.anisoamount_slider.minValue			= 1;
	m_extensionsMenu.anisoamount_slider.maxValue			= 16;
	m_extensionsMenu.anisoamount_slider.generic.statusBar	= "Maximum anisotropic filtering";

	m_extensionsMenu.apply_action.generic.type			= UITYPE_ACTION;
	m_extensionsMenu.apply_action.generic.flags			= UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_extensionsMenu.apply_action.generic.name			= "Apply";
	m_extensionsMenu.apply_action.generic.callBack		= GLExtsMenu_ApplyValues;
	m_extensionsMenu.apply_action.generic.statusBar		= "Apply Changes";

	m_extensionsMenu.reset_action.generic.type			= UITYPE_ACTION;
	m_extensionsMenu.reset_action.generic.flags			= UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_extensionsMenu.reset_action.generic.name			= "Reset";
	m_extensionsMenu.reset_action.generic.callBack		= ResetDefaults;
	m_extensionsMenu.reset_action.generic.statusBar		= "Reset Changes";

	m_extensionsMenu.cancel_action.generic.type			= UITYPE_ACTION;
	m_extensionsMenu.cancel_action.generic.flags		= UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_extensionsMenu.cancel_action.generic.name			= "Cancel";
	m_extensionsMenu.cancel_action.generic.callBack		= CancelChanges;
	m_extensionsMenu.cancel_action.generic.statusBar	= "Closes the Menu";

	GLExtsMenu_SetValues ();

	UI_AddItem (&m_extensionsMenu.frameWork,			&m_extensionsMenu.banner);
	UI_AddItem (&m_extensionsMenu.frameWork,			&m_extensionsMenu.header);

	UI_AddItem (&m_extensionsMenu.frameWork,			&m_extensionsMenu.extensions_toggle);

	UI_AddItem (&m_extensionsMenu.frameWork,			&m_extensionsMenu.multitexture_toggle);

	UI_AddItem (&m_extensionsMenu.frameWork,			&m_extensionsMenu.mtexcombine_toggle);

	UI_AddItem (&m_extensionsMenu.frameWork,			&m_extensionsMenu.cubemap_toggle);
	UI_AddItem (&m_extensionsMenu.frameWork,			&m_extensionsMenu.edgeclamp_toggle);
	UI_AddItem (&m_extensionsMenu.frameWork,			&m_extensionsMenu.genmipmap_toggle);
	UI_AddItem (&m_extensionsMenu.frameWork,			&m_extensionsMenu.texcompress_toggle);

	UI_AddItem (&m_extensionsMenu.frameWork,			&m_extensionsMenu.aniso_toggle);
	UI_AddItem (&m_extensionsMenu.frameWork,			&m_extensionsMenu.anisoamount_slider);

	UI_AddItem (&m_extensionsMenu.frameWork,			&m_extensionsMenu.apply_action);
	UI_AddItem (&m_extensionsMenu.frameWork,			&m_extensionsMenu.reset_action);
	UI_AddItem (&m_extensionsMenu.frameWork,			&m_extensionsMenu.cancel_action);

	UI_FinishFramework (&m_extensionsMenu.frameWork, qTrue);
}


/*
=============
GLExtsMenu_Close
=============
*/
static struct sfx_s *GLExtsMenu_Close (void)
{
	return uiMedia.sounds.menuOut;
}


/*
=============
GLExtsMenu_Draw
=============
*/
static void GLExtsMenu_Draw (void)
{
	float	y;

	// Initialize if necessary
	if (!m_extensionsMenu.frameWork.initialized)
		GLExtsMenu_Init ();

	// Dynamically position
	m_extensionsMenu.frameWork.x			= uiState.vidWidth * 0.5f;
	m_extensionsMenu.frameWork.y			= 0;

	m_extensionsMenu.banner.generic.x		= 0;
	m_extensionsMenu.banner.generic.y		= 0;

	y = m_extensionsMenu.banner.height * UI_SCALE;

	m_extensionsMenu.header.generic.x					= 0;
	m_extensionsMenu.header.generic.y					= y += UIFT_SIZEINC;
	m_extensionsMenu.extensions_toggle.generic.x		= 0;
	m_extensionsMenu.extensions_toggle.generic.y		= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_extensionsMenu.multitexture_toggle.generic.x		= 0;
	m_extensionsMenu.multitexture_toggle.generic.y		= y += UIFT_SIZEINC*2;
	m_extensionsMenu.mtexcombine_toggle.generic.x		= 0;
	m_extensionsMenu.mtexcombine_toggle.generic.y		= y += UIFT_SIZEINC*2;
	m_extensionsMenu.cubemap_toggle.generic.x			= 0;
	m_extensionsMenu.cubemap_toggle.generic.y			= y += UIFT_SIZEINC;
	m_extensionsMenu.edgeclamp_toggle.generic.x			= 0;
	m_extensionsMenu.edgeclamp_toggle.generic.y			= y += UIFT_SIZEINC;
	m_extensionsMenu.genmipmap_toggle.generic.x			= 0;
	m_extensionsMenu.genmipmap_toggle.generic.y			= y += UIFT_SIZEINC;
	m_extensionsMenu.texcompress_toggle.generic.x		= 0;
	m_extensionsMenu.texcompress_toggle.generic.y		= y += UIFT_SIZEINC;
	m_extensionsMenu.aniso_toggle.generic.x				= 0;
	m_extensionsMenu.aniso_toggle.generic.y				= y += UIFT_SIZEINC*2;
	m_extensionsMenu.anisoamount_slider.generic.x		= 0;
	m_extensionsMenu.anisoamount_slider.generic.y		= y += UIFT_SIZEINC;
	m_extensionsMenu.apply_action.generic.x				= 0;
	m_extensionsMenu.apply_action.generic.y				= y += UIFT_SIZEINCMED*2;
	m_extensionsMenu.reset_action.generic.x				= 0;
	m_extensionsMenu.reset_action.generic.y				= y += UIFT_SIZEINCMED;
	m_extensionsMenu.cancel_action.generic.x			= 0;
	m_extensionsMenu.cancel_action.generic.y			= y += UIFT_SIZEINCMED;

	// Render
	UI_CenterMenuHeight (&m_extensionsMenu.frameWork);
	UI_SetupMenuBounds (&m_extensionsMenu.frameWork);

	UI_AdjustCursor (&m_extensionsMenu.frameWork, 1);
	UI_Draw (&m_extensionsMenu.frameWork);
}


/*
=============
GLExtsMenu_Key
=============
*/
static struct sfx_s *GLExtsMenu_Key (int key)
{
	return DefaultMenu_Key (&m_extensionsMenu.frameWork, key);
}


/*
=============
UI_GLExtsMenu_f
=============
*/
void UI_GLExtsMenu_f (void)
{
	GLExtsMenu_Init ();
	UI_PushMenu (&m_extensionsMenu.frameWork, GLExtsMenu_Draw, GLExtsMenu_Close, GLExtsMenu_Key);
}
