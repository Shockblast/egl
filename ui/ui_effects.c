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

	EFFECTS MENU

=======================================================================
*/

typedef struct m_effectsMenu_s {
	// Menu items
	uiFrameWork_t	frameWork;

	uiImage_t		banner;

	//
	// particles
	//

	uiAction_t		part_header;

	uiList_t		part_gore_list;
	uiSlider_t		part_smokelinger_slider;
	uiAction_t		part_smokelinger_amount;

	uiList_t		part_railtrail_list;

	uiSlider_t		part_railred_slider;
	uiAction_t		part_railred_amount;
	uiSlider_t		part_railgreen_slider;
	uiAction_t		part_railgreen_amount;
	uiSlider_t		part_railblue_slider;
	uiAction_t		part_railblue_amount;

	uiSlider_t		part_spiralred_slider;
	uiAction_t		part_spiralred_amount;
	uiSlider_t		part_spiralgreen_slider;
	uiAction_t		part_spiralgreen_amount;
	uiSlider_t		part_spiralblue_slider;
	uiAction_t		part_spiralblue_amount;

	uiList_t		part_cull_toggle;
	uiSlider_t		part_degree_slider;
	uiAction_t		part_degree_amount;
	uiList_t		part_lod_toggle;
	uiList_t		part_shade_toggle;

	uiList_t		explorattle_toggle;
	uiSlider_t		explorattle_scale_slider;
	uiAction_t		explorattle_scale_amount;

	uiList_t		mapeffects_toggle;

	//
	// decals
	//

	uiAction_t		dec_header;

	uiList_t		dec_toggle;
	uiList_t		dec_lod_toggle;
	uiSlider_t		dec_life_slider;
	uiAction_t		dec_life_amount;
	uiSlider_t		dec_burnlife_slider;
	uiAction_t		dec_burnlife_amount;
	uiSlider_t		dec_max_slider;
	uiAction_t		dec_max_amount;

	//
	// shaders
	//

	uiAction_t		shader_header;

	uiList_t		shader_caustics_toggle;
	uiList_t		shader_detail_toggle;

	//
	// misc
	//

	uiAction_t		back_action;
} m_effectsMenu_t;

static m_effectsMenu_t	m_effectsMenu;

//
// particles
//

static void GoreTypeFunc (void *unused)
{
	uii.Cvar_SetValue ("cg_particleGore", m_effectsMenu.part_gore_list.curValue);
}

static void SmokeLingerFunc (void *unused)
{
	uii.Cvar_SetValue ("cg_particleSmokeLinger", m_effectsMenu.part_smokelinger_slider.curValue);
	m_effectsMenu.part_smokelinger_amount.generic.name = uii.Cvar_VariableString ("cg_particleSmokeLinger");
}

static void RailTrailFunc (void *unused)
{
	uii.Cvar_SetValue ("cl_railtrail", m_effectsMenu.part_railtrail_list.curValue);
}

static void RailRedColorFunc (void *unused)
{
	uii.Cvar_SetValue ("cl_railred", m_effectsMenu.part_railred_slider.curValue * 0.1);
	m_effectsMenu.part_railred_amount.generic.name = uii.Cvar_VariableString ("cl_railred");
}

static void RailGreenColorFunc (void *unused)
{
	uii.Cvar_SetValue ("cl_railgreen", m_effectsMenu.part_railgreen_slider.curValue * 0.1);
	m_effectsMenu.part_railgreen_amount.generic.name = uii.Cvar_VariableString ("cl_railgreen");
}

static void RailBlueColorFunc (void *unused)
{
	uii.Cvar_SetValue ("cl_railblue", m_effectsMenu.part_railblue_slider.curValue * 0.1);
	m_effectsMenu.part_railblue_amount.generic.name = uii.Cvar_VariableString ("cl_railblue");
}

static void SpiralRedColorFunc (void *unused)
{
	uii.Cvar_SetValue ("cl_spiralred", m_effectsMenu.part_spiralred_slider.curValue * 0.1);
	m_effectsMenu.part_spiralred_amount.generic.name = uii.Cvar_VariableString ("cl_spiralred");
}

static void SpiralGreenColorFunc (void *unused)
{
	uii.Cvar_SetValue ("cl_spiralgreen", m_effectsMenu.part_spiralgreen_slider.curValue * 0.1);
	m_effectsMenu.part_spiralgreen_amount.generic.name = uii.Cvar_VariableString ("cl_spiralgreen");
}

static void SpiralBlueColorFunc (void *unused)
{
	uii.Cvar_SetValue ("cl_spiralblue", m_effectsMenu.part_spiralblue_slider.curValue * 0.1);
	m_effectsMenu.part_spiralblue_amount.generic.name = uii.Cvar_VariableString ("cl_spiralblue");
}

static void PartCullFunc (void *unused)
{
	uii.Cvar_SetValue ("cg_particleCulling", m_effectsMenu.part_cull_toggle.curValue);
}

static void PartDegreeFunc (void *unused)
{
	uii.Cvar_SetValue ("cg_particleQuality", m_effectsMenu.part_degree_slider.curValue * 0.5);
	m_effectsMenu.part_degree_amount.generic.name = uii.Cvar_VariableString ("cg_particleQuality");
}

static void PartLODFunc (void *unused)
{
	uii.Cvar_SetValue ("cg_particleLOD", m_effectsMenu.part_lod_toggle.curValue);
}

static void PartShadeFunc (void *unused)
{
	uii.Cvar_SetValue ("cg_particleShading", m_effectsMenu.part_shade_toggle.curValue);
}

static void ExploRattleFunc (void *unused)
{
	uii.Cvar_SetValue ("cl_explorattle", m_effectsMenu.explorattle_toggle.curValue);
}

static void ExploRattleScaleFunc (void *unused)
{
	uii.Cvar_SetValue ("cl_explorattle_scale", m_effectsMenu.explorattle_scale_slider.curValue * 0.1);
	m_effectsMenu.explorattle_scale_amount.generic.name = uii.Cvar_VariableString ("cl_explorattle_scale");
}

static void MapEffectsToggleFunc (void *unused)
{
	uii.Cvar_SetValue ("cg_mapeffects", m_effectsMenu.mapeffects_toggle.curValue);
}

//
// decals
//

static void DecalToggleFunc (void *unused)
{
	uii.Cvar_SetValue ("cl_decals", m_effectsMenu.dec_toggle.curValue);
}

static void DecalLODFunc (void *unused)
{
	uii.Cvar_SetValue ("cl_decal_lod", m_effectsMenu.dec_lod_toggle.curValue);
}

static void DecalLifeFunc (void *unused)
{
	uii.Cvar_SetValue ("cl_decal_life", m_effectsMenu.dec_life_slider.curValue * 100);
	m_effectsMenu.dec_life_amount.generic.name = uii.Cvar_VariableString ("cl_decal_life");
}
static void DecalBurnLifeFunc (void *unused)
{
	uii.Cvar_SetValue ("cl_decal_burnlife", m_effectsMenu.dec_burnlife_slider.curValue * 10);
	m_effectsMenu.dec_burnlife_amount.generic.name = uii.Cvar_VariableString ("cl_decal_burnlife");
}

static void DecalMaxFunc (void *unused)
{
	uii.Cvar_SetValue ("cl_decal_max", m_effectsMenu.dec_max_slider.curValue * 1000);
	m_effectsMenu.dec_max_amount.generic.name = uii.Cvar_VariableString ("cl_decal_max");
}

//
// shaders
//

static void CausticsFunc (void *unused)
{
	uii.Cvar_SetValue ("r_caustics", m_effectsMenu.shader_caustics_toggle.curValue);
}
static void ShaderDetailFunc (void *unused)
{
	uii.Cvar_SetValue ("r_detailTextures", m_effectsMenu.shader_detail_toggle.curValue);
}

/*
=============
EffectsMenu_SetValues
=============
*/
static void EffectsMenu_SetValues (void)
{
	//
	// particles
	//

	uii.Cvar_SetValue ("cg_particleGore",	clamp (uii.Cvar_VariableInteger ("cg_particleGore"), 0, 10));
	m_effectsMenu.part_gore_list.curValue	= uii.Cvar_VariableInteger ("cg_particleGore");

	m_effectsMenu.part_smokelinger_slider.curValue	= uii.Cvar_VariableValue ("cg_particleSmokeLinger");
	m_effectsMenu.part_smokelinger_amount.generic.name	= uii.Cvar_VariableString ("cg_particleSmokeLinger");

	uii.Cvar_SetValue ("cl_railtrail",				clamp (uii.Cvar_VariableInteger ("cl_railtrail"), 0, 1));
	m_effectsMenu.part_railtrail_list.curValue		= uii.Cvar_VariableInteger ("cl_railtrail");

	m_effectsMenu.part_railred_slider.curValue			= uii.Cvar_VariableValue ("cl_railred") * 10;
	m_effectsMenu.part_railred_amount.generic.name		= uii.Cvar_VariableString ("cl_railred");
	m_effectsMenu.part_railgreen_slider.curValue		= uii.Cvar_VariableValue ("cl_railgreen") * 10;
	m_effectsMenu.part_railgreen_amount.generic.name	= uii.Cvar_VariableString ("cl_railgreen");
	m_effectsMenu.part_railblue_slider.curValue		= uii.Cvar_VariableValue ("cl_railblue") * 10;
	m_effectsMenu.part_railblue_amount.generic.name	= uii.Cvar_VariableString ("cl_railblue");

	m_effectsMenu.part_spiralred_slider.curValue		= uii.Cvar_VariableValue ("cl_spiralred") * 10;
	m_effectsMenu.part_spiralred_amount.generic.name	= uii.Cvar_VariableString ("cl_spiralred");
	m_effectsMenu.part_spiralgreen_slider.curValue		= uii.Cvar_VariableValue ("cl_spiralgreen") * 10;
	m_effectsMenu.part_spiralgreen_amount.generic.name	= uii.Cvar_VariableString ("cl_spiralgreen");
	m_effectsMenu.part_spiralblue_slider.curValue		= uii.Cvar_VariableValue ("cl_spiralblue") * 10;
	m_effectsMenu.part_spiralblue_amount.generic.name	= uii.Cvar_VariableString ("cl_spiralblue");

	uii.Cvar_SetValue ("cg_particleCulling",	clamp (uii.Cvar_VariableInteger ("cg_particleCulling"), 0, 1));
	m_effectsMenu.part_cull_toggle.curValue		= uii.Cvar_VariableInteger ("cg_particleCulling");

	m_effectsMenu.part_degree_slider.curValue		= uii.Cvar_VariableValue ("cg_particleQuality") * 2;
	m_effectsMenu.part_degree_amount.generic.name	= uii.Cvar_VariableString ("cg_particleQuality");

	uii.Cvar_SetValue ("cg_particleLOD",		clamp (uii.Cvar_VariableInteger ("cg_particleLOD"), 0, 1));
	m_effectsMenu.part_lod_toggle.curValue		= uii.Cvar_VariableInteger ("cg_particleLOD");

	uii.Cvar_SetValue ("cg_particleShading",	clamp (uii.Cvar_VariableInteger ("cg_particleShading"), 0, 1));
	m_effectsMenu.part_shade_toggle.curValue	= uii.Cvar_VariableInteger ("cg_particleShading");

	uii.Cvar_SetValue ("cl_explorattle",					clamp (uii.Cvar_VariableInteger ("cl_explorattle"), 0, 1));
	m_effectsMenu.explorattle_toggle.curValue				= uii.Cvar_VariableInteger ("cl_explorattle");
	m_effectsMenu.explorattle_scale_slider.curValue		= uii.Cvar_VariableInteger ("cl_explorattle_scale") * 10;
	m_effectsMenu.explorattle_scale_amount.generic.name	= uii.Cvar_VariableString ("cl_explorattle_scale");

	uii.Cvar_SetValue ("cg_mapeffects",			clamp (uii.Cvar_VariableInteger ("cg_mapeffects"), 0, 1));
	m_effectsMenu.mapeffects_toggle.curValue	= uii.Cvar_VariableInteger ("cg_mapeffects");
	

	//
	// decals
	//

	uii.Cvar_SetValue ("cl_decal_lod",		clamp (uii.Cvar_VariableInteger ("cl_decal_lod"), 0, 1));
	m_effectsMenu.dec_lod_toggle.curValue	= uii.Cvar_VariableInteger ("cl_decal_lod");

	uii.Cvar_SetValue ("cl_decals",			clamp (uii.Cvar_VariableInteger ("cl_decals"), 0, 1));
	m_effectsMenu.dec_toggle.curValue		= uii.Cvar_VariableInteger ("cl_decals");

	m_effectsMenu.dec_life_slider.curValue		= uii.Cvar_VariableValue ("cl_decal_life") * 0.01;
	m_effectsMenu.dec_life_amount.generic.name	= uii.Cvar_VariableString ("cl_decal_life");

	m_effectsMenu.dec_burnlife_slider.curValue		= uii.Cvar_VariableValue ("cl_decal_burnlife") * 0.1;
	m_effectsMenu.dec_burnlife_amount.generic.name	= uii.Cvar_VariableString ("cl_decal_burnlife");

	m_effectsMenu.dec_max_slider.curValue		= uii.Cvar_VariableValue ("cl_decal_max") * 0.001;
	m_effectsMenu.dec_max_amount.generic.name	= uii.Cvar_VariableString ("cl_decal_max");

	//
	// shaders
	//

	uii.Cvar_SetValue ("r_caustics",				clamp (uii.Cvar_VariableInteger ("r_caustics"), 0, 1));
	m_effectsMenu.shader_caustics_toggle.curValue	= uii.Cvar_VariableInteger ("r_caustics");

	uii.Cvar_SetValue ("r_detailTextures",			clamp (uii.Cvar_VariableInteger ("r_detailTextures"), 0, 1));
	m_effectsMenu.shader_detail_toggle.curValue	= uii.Cvar_VariableInteger ("r_detailTextures");
}


/*
=============
EffectsMenu_Init
=============
*/
static void EffectsMenu_Init (void)
{
	static char *goreamount_names[] = {
		"low",
		"normal",
		"x 2",
		"x 3",
		"x 4",
		"x 5",
		"x 6",
		"x 7",
		"x 8",
		"x 9",
		"x 10 !",
		0
	};

	static char *nicenorm_names[] = {
		"normal",
		"nicer",
		0
	};

	static char *onoff_names[] = {
		"off",
		"on",
		0
	};

	static char *railtrail_names[] = {
		"normal",
		"beam",
		0
	};

	UI_StartFramework (&m_effectsMenu.frameWork);

	m_effectsMenu.banner.generic.type	= UITYPE_IMAGE;
	m_effectsMenu.banner.generic.flags	= UIF_NOSELECT|UIF_CENTERED;
	m_effectsMenu.banner.generic.name	= NULL;
	m_effectsMenu.banner.shader			= uiMedia.banners.options;

	//
	// particles
	//

	m_effectsMenu.part_header.generic.type		= UITYPE_ACTION;
	m_effectsMenu.part_header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_effectsMenu.part_header.generic.name		= "Particle Settings";

	m_effectsMenu.part_gore_list.generic.type		= UITYPE_SPINCONTROL;
	m_effectsMenu.part_gore_list.generic.name		= "Gore severity";
	m_effectsMenu.part_gore_list.generic.callBack	= GoreTypeFunc;
	m_effectsMenu.part_gore_list.itemNames			= goreamount_names;
	m_effectsMenu.part_gore_list.generic.statusBar	= "Select your gore";

	m_effectsMenu.part_smokelinger_slider.generic.type			= UITYPE_SLIDER;
	m_effectsMenu.part_smokelinger_slider.generic.name			= "Smoke lingering";
	m_effectsMenu.part_smokelinger_slider.generic.callBack		= SmokeLingerFunc;
	m_effectsMenu.part_smokelinger_slider.minValue				= 0;
	m_effectsMenu.part_smokelinger_slider.maxValue				= 10;
	m_effectsMenu.part_smokelinger_slider.generic.statusBar		= "Customize smoke linger duration";
	m_effectsMenu.part_smokelinger_amount.generic.type			= UITYPE_ACTION;
	m_effectsMenu.part_smokelinger_amount.generic.flags			= UIF_LEFT_JUSTIFY|UIF_NOSELECT;

	m_effectsMenu.part_railtrail_list.generic.type			= UITYPE_SPINCONTROL;
	m_effectsMenu.part_railtrail_list.generic.name			= "Rail-Trail style";
	m_effectsMenu.part_railtrail_list.generic.callBack		= RailTrailFunc;
	m_effectsMenu.part_railtrail_list.itemNames				= railtrail_names;
	m_effectsMenu.part_railtrail_list.generic.statusBar		= "Select your style";

	m_effectsMenu.part_railred_slider.generic.type			= UITYPE_SLIDER;
	m_effectsMenu.part_railred_slider.generic.name			= "Rail center red";
	m_effectsMenu.part_railred_slider.generic.callBack		= RailRedColorFunc;
	m_effectsMenu.part_railred_slider.minValue				= 0;
	m_effectsMenu.part_railred_slider.maxValue				= 10;
	m_effectsMenu.part_railred_slider.generic.statusBar		= "Rail core red color";
	m_effectsMenu.part_railred_amount.generic.type			= UITYPE_ACTION;
	m_effectsMenu.part_railred_amount.generic.flags			= UIF_LEFT_JUSTIFY|UIF_NOSELECT;

	m_effectsMenu.part_railgreen_slider.generic.type		= UITYPE_SLIDER;
	m_effectsMenu.part_railgreen_slider.generic.name		= "Rail center green";
	m_effectsMenu.part_railgreen_slider.generic.callBack	= RailGreenColorFunc;
	m_effectsMenu.part_railgreen_slider.minValue			= 0;
	m_effectsMenu.part_railgreen_slider.maxValue			= 10;
	m_effectsMenu.part_railgreen_slider.generic.statusBar	= "Rail core green color";
	m_effectsMenu.part_railgreen_amount.generic.type		= UITYPE_ACTION;
	m_effectsMenu.part_railgreen_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;

	m_effectsMenu.part_railblue_slider.generic.type			= UITYPE_SLIDER;
	m_effectsMenu.part_railblue_slider.generic.name			= "Rail center blue";
	m_effectsMenu.part_railblue_slider.generic.callBack		= RailBlueColorFunc;
	m_effectsMenu.part_railblue_slider.minValue				= 0;
	m_effectsMenu.part_railblue_slider.maxValue				= 10;
	m_effectsMenu.part_railblue_slider.generic.statusBar	= "Rail core blue color";
	m_effectsMenu.part_railblue_amount.generic.type			= UITYPE_ACTION;
	m_effectsMenu.part_railblue_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;

	m_effectsMenu.part_spiralred_slider.generic.type		= UITYPE_SLIDER;
	m_effectsMenu.part_spiralred_slider.generic.name		= "Rail spiral red";
	m_effectsMenu.part_spiralred_slider.generic.callBack	= SpiralRedColorFunc;
	m_effectsMenu.part_spiralred_slider.minValue			= 0;
	m_effectsMenu.part_spiralred_slider.maxValue			= 10;
	m_effectsMenu.part_spiralred_slider.generic.statusBar	= "Rail spiral red color";
	m_effectsMenu.part_spiralred_amount.generic.type		= UITYPE_ACTION;
	m_effectsMenu.part_spiralred_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;

	m_effectsMenu.part_spiralgreen_slider.generic.type			= UITYPE_SLIDER;
	m_effectsMenu.part_spiralgreen_slider.generic.name			= "Rail spiral green";
	m_effectsMenu.part_spiralgreen_slider.generic.callBack		= SpiralGreenColorFunc;
	m_effectsMenu.part_spiralgreen_slider.minValue				= 0;
	m_effectsMenu.part_spiralgreen_slider.maxValue				= 10;
	m_effectsMenu.part_spiralgreen_slider.generic.statusBar		= "Rail spiral green color";
	m_effectsMenu.part_spiralgreen_amount.generic.type			= UITYPE_ACTION;
	m_effectsMenu.part_spiralgreen_amount.generic.flags			= UIF_LEFT_JUSTIFY|UIF_NOSELECT;

	m_effectsMenu.part_spiralblue_slider.generic.type		= UITYPE_SLIDER;
	m_effectsMenu.part_spiralblue_slider.generic.name		= "Rail spiral blue";
	m_effectsMenu.part_spiralblue_slider.generic.callBack	= SpiralBlueColorFunc;
	m_effectsMenu.part_spiralblue_slider.minValue			= 0;
	m_effectsMenu.part_spiralblue_slider.maxValue			= 10;
	m_effectsMenu.part_spiralblue_slider.generic.statusBar	= "Rail spiral blue color";
	m_effectsMenu.part_spiralblue_amount.generic.type		= UITYPE_ACTION;
	m_effectsMenu.part_spiralblue_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;

	m_effectsMenu.part_cull_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_effectsMenu.part_cull_toggle.generic.name			= "Particle culling";
	m_effectsMenu.part_cull_toggle.generic.callBack		= PartCullFunc;
	m_effectsMenu.part_cull_toggle.itemNames			= onoff_names;
	m_effectsMenu.part_cull_toggle.generic.statusBar	= "Particle culling (on = faster)";

	m_effectsMenu.part_degree_slider.generic.type		= UITYPE_SLIDER;
	m_effectsMenu.part_degree_slider.generic.name		= "Particle quality";
	m_effectsMenu.part_degree_slider.generic.callBack	= PartDegreeFunc;
	m_effectsMenu.part_degree_slider.minValue			= 0;
	m_effectsMenu.part_degree_slider.maxValue			= 4;
	m_effectsMenu.part_degree_slider.generic.statusBar	= "Particle quality multiplier (lower = faster)";
	m_effectsMenu.part_degree_amount.generic.type		= UITYPE_ACTION;
	m_effectsMenu.part_degree_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;

	m_effectsMenu.part_lod_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_effectsMenu.part_lod_toggle.generic.name			= "Particle LOD";
	m_effectsMenu.part_lod_toggle.generic.callBack		= PartLODFunc;
	m_effectsMenu.part_lod_toggle.itemNames				= onoff_names;
	m_effectsMenu.part_lod_toggle.generic.statusBar		= "Particle 'Level of Detail' (on = faster)";

	m_effectsMenu.part_shade_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_effectsMenu.part_shade_toggle.generic.name		= "Particle lighting";
	m_effectsMenu.part_shade_toggle.generic.callBack	= PartShadeFunc;
	m_effectsMenu.part_shade_toggle.itemNames			= onoff_names;
	m_effectsMenu.part_shade_toggle.generic.statusBar	= "Enables particle lighting";

	m_effectsMenu.explorattle_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_effectsMenu.explorattle_toggle.generic.name		= "Explosion rattling";
	m_effectsMenu.explorattle_toggle.generic.callBack	= ExploRattleFunc;
	m_effectsMenu.explorattle_toggle.itemNames			= onoff_names;
	m_effectsMenu.explorattle_toggle.generic.statusBar	= "Toggles screen rattling when near explosions";

	m_effectsMenu.explorattle_scale_slider.generic.type			= UITYPE_SLIDER;
	m_effectsMenu.explorattle_scale_slider.generic.name			= "Rattle severity";
	m_effectsMenu.explorattle_scale_slider.generic.callBack		= ExploRattleScaleFunc;
	m_effectsMenu.explorattle_scale_slider.minValue				= 1;
	m_effectsMenu.explorattle_scale_slider.maxValue				= 9;
	m_effectsMenu.explorattle_scale_slider.generic.statusBar	= "Severity of explosion rattles";
	m_effectsMenu.explorattle_scale_amount.generic.type			= UITYPE_ACTION;
	m_effectsMenu.explorattle_scale_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;

	m_effectsMenu.mapeffects_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_effectsMenu.mapeffects_toggle.generic.name		= "Map effects";
	m_effectsMenu.mapeffects_toggle.generic.callBack	= MapEffectsToggleFunc;
	m_effectsMenu.mapeffects_toggle.itemNames			= onoff_names;
	m_effectsMenu.mapeffects_toggle.generic.statusBar	= "Toggles scripted map particle effects";
	
	//
	// decals
	//

	m_effectsMenu.dec_header.generic.type		= UITYPE_ACTION;
	m_effectsMenu.dec_header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_effectsMenu.dec_header.generic.name		= "Decal Settings";

	m_effectsMenu.dec_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_effectsMenu.dec_toggle.generic.name		= "Decals";
	m_effectsMenu.dec_toggle.generic.callBack	= DecalToggleFunc;
	m_effectsMenu.dec_toggle.itemNames			= onoff_names;
	m_effectsMenu.dec_toggle.generic.statusBar	= "Toggle decals on or off";

	m_effectsMenu.dec_lod_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_effectsMenu.dec_lod_toggle.generic.name		= "Decal LOD";
	m_effectsMenu.dec_lod_toggle.generic.callBack	= DecalLODFunc;
	m_effectsMenu.dec_lod_toggle.itemNames			= onoff_names;
	m_effectsMenu.dec_lod_toggle.generic.statusBar	= "Toggle decal lod (skips small far away decals)";

	m_effectsMenu.dec_life_slider.generic.type			= UITYPE_SLIDER;
	m_effectsMenu.dec_life_slider.generic.name			= "Decal life";
	m_effectsMenu.dec_life_slider.generic.callBack		= DecalLifeFunc;
	m_effectsMenu.dec_life_slider.minValue				= 0;
	m_effectsMenu.dec_life_slider.maxValue				= 10;
	m_effectsMenu.dec_life_slider.generic.statusBar		= "Decal life time";
	m_effectsMenu.dec_life_amount.generic.type			= UITYPE_ACTION;
	m_effectsMenu.dec_life_amount.generic.flags			= UIF_LEFT_JUSTIFY|UIF_NOSELECT;

	m_effectsMenu.dec_burnlife_slider.generic.type			= UITYPE_SLIDER;
	m_effectsMenu.dec_burnlife_slider.generic.name			= "Burn life";
	m_effectsMenu.dec_burnlife_slider.generic.callBack		= DecalBurnLifeFunc;
	m_effectsMenu.dec_burnlife_slider.minValue				= 0;
	m_effectsMenu.dec_burnlife_slider.maxValue				= 20;
	m_effectsMenu.dec_burnlife_slider.generic.statusBar	= "Decal burn life time";
	m_effectsMenu.dec_burnlife_amount.generic.type			= UITYPE_ACTION;
	m_effectsMenu.dec_burnlife_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;

	m_effectsMenu.dec_max_slider.generic.type		= UITYPE_SLIDER;
	m_effectsMenu.dec_max_slider.generic.name		= "Max decals";
	m_effectsMenu.dec_max_slider.generic.callBack	= DecalMaxFunc;
	m_effectsMenu.dec_max_slider.minValue			= 1;
	m_effectsMenu.dec_max_slider.maxValue			= MAX_DECALS / 1000;
	m_effectsMenu.dec_max_slider.generic.statusBar	= "Maximum decals allowed";
	m_effectsMenu.dec_max_amount.generic.type		= UITYPE_ACTION;
	m_effectsMenu.dec_max_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;

	//
	// shaders
	//

	m_effectsMenu.shader_header.generic.type		= UITYPE_ACTION;
	m_effectsMenu.shader_header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_effectsMenu.shader_header.generic.name		= "Shader Settings";

	m_effectsMenu.shader_caustics_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_effectsMenu.shader_caustics_toggle.generic.name		= "Caustics";
	m_effectsMenu.shader_caustics_toggle.generic.callBack	= CausticsFunc;
	m_effectsMenu.shader_caustics_toggle.itemNames			= onoff_names;
	m_effectsMenu.shader_caustics_toggle.generic.statusBar	= "Shader-based underwater/lava/slime surface caustics";

	m_effectsMenu.shader_detail_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_effectsMenu.shader_detail_toggle.generic.name			= "Detail Textures";
	m_effectsMenu.shader_detail_toggle.generic.callBack		= ShaderDetailFunc;
	m_effectsMenu.shader_detail_toggle.itemNames			= onoff_names;
	m_effectsMenu.shader_detail_toggle.generic.statusBar	= "Toggle shader passes marked as 'detail'";

	//
	// misc
	//

	m_effectsMenu.back_action.generic.type		= UITYPE_ACTION;
	m_effectsMenu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_effectsMenu.back_action.generic.name		= "< Back";
	m_effectsMenu.back_action.generic.callBack	= Menu_Pop;
	m_effectsMenu.back_action.generic.statusBar	= "Back a menu";

	EffectsMenu_SetValues ();


	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.banner);

	//
	// particles
	//

	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.part_header);

	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.part_gore_list);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.part_smokelinger_slider);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.part_smokelinger_amount);

	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.part_railtrail_list);

	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.part_railred_slider);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.part_railred_amount);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.part_railgreen_slider);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.part_railgreen_amount);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.part_railblue_slider);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.part_railblue_amount);

	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.part_spiralred_slider);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.part_spiralred_amount);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.part_spiralgreen_slider);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.part_spiralgreen_amount);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.part_spiralblue_slider);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.part_spiralblue_amount);

	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.part_cull_toggle);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.part_degree_slider);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.part_degree_amount);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.part_lod_toggle);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.part_shade_toggle);

	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.explorattle_toggle);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.explorattle_scale_slider);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.explorattle_scale_amount);

	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.mapeffects_toggle);
	//
	// decals
	//

	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.dec_header);

	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.dec_toggle);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.dec_lod_toggle);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.dec_life_slider);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.dec_life_amount);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.dec_burnlife_slider);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.dec_burnlife_amount);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.dec_max_slider);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.dec_max_amount);

	//
	// shaders
	//

	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.shader_header);

	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.shader_caustics_toggle);
	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.shader_detail_toggle);

	//
	// misc
	//

	UI_AddItem (&m_effectsMenu.frameWork,		&m_effectsMenu.back_action);

	UI_FinishFramework (&m_effectsMenu.frameWork, qTrue);
}


/*
=============
EffectsMenu_Close
=============
*/
static struct sfx_s *EffectsMenu_Close (void)
{
	return uiMedia.sounds.menuOut;
}


/*
=============
EffectsMenu_Draw
=============
*/
static void EffectsMenu_Draw (void)
{
	float	y;

	// Initialize if necessary
	if (!m_effectsMenu.frameWork.initialized)
		EffectsMenu_Init ();

	// Dynamically position
	m_effectsMenu.frameWork.x			= uiState.vidWidth * 0.5f;
	m_effectsMenu.frameWork.y			= 0;

	m_effectsMenu.banner.generic.x		= 0;
	m_effectsMenu.banner.generic.y		= 0;

	y = m_effectsMenu.banner.height * UI_SCALE;

	//
	// particles
	//
	m_effectsMenu.part_header.generic.x					= 0;
	m_effectsMenu.part_header.generic.y					= y += UIFT_SIZEINC;
	m_effectsMenu.part_gore_list.generic.x				= 0;
	m_effectsMenu.part_gore_list.generic.y				= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_effectsMenu.part_smokelinger_slider.generic.x		= 0;
	m_effectsMenu.part_smokelinger_slider.generic.y		= y += UIFT_SIZEINC;
	m_effectsMenu.part_smokelinger_amount.generic.x		= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effectsMenu.part_smokelinger_amount.generic.y		= y;
	m_effectsMenu.part_railtrail_list.generic.x			= 0;
	m_effectsMenu.part_railtrail_list.generic.y			= y += (UIFT_SIZEINC*2);
	m_effectsMenu.part_railred_slider.generic.x			= 0;
	m_effectsMenu.part_railred_slider.generic.y			= y += (UIFT_SIZEINC*2);
	m_effectsMenu.part_railred_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effectsMenu.part_railred_amount.generic.y			= y;
	m_effectsMenu.part_railgreen_slider.generic.x		= 0;
	m_effectsMenu.part_railgreen_slider.generic.y		= y += UIFT_SIZEINC;
	m_effectsMenu.part_railgreen_amount.generic.x		= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effectsMenu.part_railgreen_amount.generic.y		= y;
	m_effectsMenu.part_railblue_slider.generic.x		= 0;
	m_effectsMenu.part_railblue_slider.generic.y		= y += UIFT_SIZEINC;
	m_effectsMenu.part_railblue_amount.generic.x		= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effectsMenu.part_railblue_amount.generic.y		= y;
	m_effectsMenu.part_spiralred_slider.generic.x		= 0;
	m_effectsMenu.part_spiralred_slider.generic.y		= y += (UIFT_SIZEINC*2);
	m_effectsMenu.part_spiralred_amount.generic.x		= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effectsMenu.part_spiralred_amount.generic.y		= y;
	m_effectsMenu.part_spiralgreen_slider.generic.x		= 0;
	m_effectsMenu.part_spiralgreen_slider.generic.y		= y += UIFT_SIZEINC;
	m_effectsMenu.part_spiralgreen_amount.generic.x		= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effectsMenu.part_spiralgreen_amount.generic.y		= y;
	m_effectsMenu.part_spiralblue_slider.generic.x		= 0;
	m_effectsMenu.part_spiralblue_slider.generic.y		= y += UIFT_SIZEINC;
	m_effectsMenu.part_spiralblue_amount.generic.x		= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effectsMenu.part_spiralblue_amount.generic.y		= y;
	m_effectsMenu.part_cull_toggle.generic.x			= 0;
	m_effectsMenu.part_cull_toggle.generic.y			= y += (UIFT_SIZEINC*2);
	m_effectsMenu.part_degree_slider.generic.x			= 0;
	m_effectsMenu.part_degree_slider.generic.y			= y += UIFT_SIZEINC;
	m_effectsMenu.part_degree_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effectsMenu.part_degree_amount.generic.y			= y;
	m_effectsMenu.part_lod_toggle.generic.x				= 0;
	m_effectsMenu.part_lod_toggle.generic.y				= y += UIFT_SIZEINC;
	m_effectsMenu.part_shade_toggle.generic.x			= 0;
	m_effectsMenu.part_shade_toggle.generic.y			= y += UIFT_SIZEINC;
	m_effectsMenu.explorattle_toggle.generic.x			= 0;
	m_effectsMenu.explorattle_toggle.generic.y			= y += UIFT_SIZEINC;
	m_effectsMenu.explorattle_scale_slider.generic.x	= 0;
	m_effectsMenu.explorattle_scale_slider.generic.y	= y += UIFT_SIZEINC;
	m_effectsMenu.explorattle_scale_amount.generic.x	= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effectsMenu.explorattle_scale_amount.generic.y	= y;
	m_effectsMenu.mapeffects_toggle.generic.x			= 0;
	m_effectsMenu.mapeffects_toggle.generic.y			= y += UIFT_SIZEINC*2;

	//
	// decals
	//
	m_effectsMenu.dec_header.generic.x				= 0;
	m_effectsMenu.dec_header.generic.y				= y += UIFT_SIZEINC*2;
	m_effectsMenu.dec_toggle.generic.x				= 0;
	m_effectsMenu.dec_toggle.generic.y				= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_effectsMenu.dec_lod_toggle.generic.x			= 0;
	m_effectsMenu.dec_lod_toggle.generic.y			= y += UIFT_SIZEINC;
	m_effectsMenu.dec_life_slider.generic.x			= 0;
	m_effectsMenu.dec_life_slider.generic.y			= y += UIFT_SIZEINC;
	m_effectsMenu.dec_life_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effectsMenu.dec_life_amount.generic.y			= y;
	m_effectsMenu.dec_burnlife_slider.generic.x		= 0;
	m_effectsMenu.dec_burnlife_slider.generic.y		= y += UIFT_SIZEINC;
	m_effectsMenu.dec_burnlife_amount.generic.x		= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effectsMenu.dec_burnlife_amount.generic.y		= y;
	m_effectsMenu.dec_max_slider.generic.x			= 0;
	m_effectsMenu.dec_max_slider.generic.y			= y += UIFT_SIZEINC;
	m_effectsMenu.dec_max_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effectsMenu.dec_max_amount.generic.y			= y;

	//
	// shaders
	//
	m_effectsMenu.shader_header.generic.x			= 0;
	m_effectsMenu.shader_header.generic.y			= y += UIFT_SIZEINC*2;
	m_effectsMenu.shader_caustics_toggle.generic.x	= 0;
	m_effectsMenu.shader_caustics_toggle.generic.y	= y += UIFT_SIZEINC;
	m_effectsMenu.shader_detail_toggle.generic.x	= 0;
	m_effectsMenu.shader_detail_toggle.generic.y	= y += UIFT_SIZEINC;

	//
	// misc
	//
	m_effectsMenu.back_action.generic.x			= 0;
	m_effectsMenu.back_action.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCLG;

	// Render
	UI_CenterMenuHeight (&m_effectsMenu.frameWork);
	UI_SetupMenuBounds (&m_effectsMenu.frameWork);

	UI_AdjustCursor (&m_effectsMenu.frameWork, 1);
	UI_Draw (&m_effectsMenu.frameWork);
}


/*
=============
EffectsMenu_Key
=============
*/
static struct sfx_s *EffectsMenu_Key (int key)
{
	return DefaultMenu_Key (&m_effectsMenu.frameWork, key);
}

/*
=============
UI_EffectsMenu_f
=============
*/

void UI_EffectsMenu_f (void)
{
	EffectsMenu_Init ();
	UI_PushMenu (&m_effectsMenu.frameWork, EffectsMenu_Draw, EffectsMenu_Close, EffectsMenu_Key);
}
