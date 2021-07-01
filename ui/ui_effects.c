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

typedef struct {
	uiFrameWork_t	framework;

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

	uiList_t		shader_advir_toggle;

	uiList_t		shader_shaders_toggle;
	uiList_t		shader_caustics_toggle;

	//
	// misc
	//

	uiAction_t		back_action;
} m_effectsmenu_t;

m_effectsmenu_t	m_effects_menu;

//
// particles
//

static void GoreTypeFunc (void *unused) {
	uii.Cvar_SetValue ("cl_gore", m_effects_menu.part_gore_list.curvalue);
}

static void SmokeLingerFunc (void *unused) {
	uii.Cvar_SetValue ("cl_smokelinger", m_effects_menu.part_smokelinger_slider.curvalue);
	m_effects_menu.part_smokelinger_amount.generic.name = uii.Cvar_VariableString ("cl_smokelinger");
}

static void RailTrailFunc (void *unused) {
	uii.Cvar_SetValue ("cl_railtrail", m_effects_menu.part_railtrail_list.curvalue);
}

static void RailRedColorFunc (void *unused) {
	uii.Cvar_SetValue ("cl_railred", m_effects_menu.part_railred_slider.curvalue * 0.1);
	m_effects_menu.part_railred_amount.generic.name = uii.Cvar_VariableString ("cl_railred");
}

static void RailGreenColorFunc (void *unused) {
	uii.Cvar_SetValue ("cl_railgreen", m_effects_menu.part_railgreen_slider.curvalue * 0.1);
	m_effects_menu.part_railgreen_amount.generic.name = uii.Cvar_VariableString ("cl_railgreen");
}

static void RailBlueColorFunc (void *unused) {
	uii.Cvar_SetValue ("cl_railblue", m_effects_menu.part_railblue_slider.curvalue * 0.1);
	m_effects_menu.part_railblue_amount.generic.name = uii.Cvar_VariableString ("cl_railblue");
}

static void SpiralRedColorFunc (void *unused) {
	uii.Cvar_SetValue ("cl_spiralred", m_effects_menu.part_spiralred_slider.curvalue * 0.1);
	m_effects_menu.part_spiralred_amount.generic.name = uii.Cvar_VariableString ("cl_spiralred");
}

static void SpiralGreenColorFunc (void *unused) {
	uii.Cvar_SetValue ("cl_spiralgreen", m_effects_menu.part_spiralgreen_slider.curvalue * 0.1);
	m_effects_menu.part_spiralgreen_amount.generic.name = uii.Cvar_VariableString ("cl_spiralgreen");
}

static void SpiralBlueColorFunc (void *unused) {
	uii.Cvar_SetValue ("cl_spiralblue", m_effects_menu.part_spiralblue_slider.curvalue * 0.1);
	m_effects_menu.part_spiralblue_amount.generic.name = uii.Cvar_VariableString ("cl_spiralblue");
}

static void PartCullFunc (void *unused) {
	uii.Cvar_SetValue ("part_cull", m_effects_menu.part_cull_toggle.curvalue);
}

static void PartDegreeFunc (void *unused) {
	uii.Cvar_SetValue ("part_degree", m_effects_menu.part_degree_slider.curvalue * 0.5);
	m_effects_menu.part_degree_amount.generic.name = uii.Cvar_VariableString ("part_degree");
}

static void PartLODFunc (void *unused) {
	uii.Cvar_SetValue ("part_lod", m_effects_menu.part_lod_toggle.curvalue);
}

static void PartShadeFunc (void *unused) {
	uii.Cvar_SetValue ("part_shade", m_effects_menu.part_shade_toggle.curvalue);
}

static void ExploRattleFunc (void *unused) {
	uii.Cvar_SetValue ("cl_explorattle", m_effects_menu.explorattle_toggle.curvalue);
}

static void ExploRattleScaleFunc (void *unused) {
	uii.Cvar_SetValue ("cl_explorattle_scale", m_effects_menu.explorattle_scale_slider.curvalue * 0.1);
	m_effects_menu.explorattle_scale_amount.generic.name = uii.Cvar_VariableString ("cl_explorattle_scale");
}

//
// decals
//

static void DecalToggleFunc (void *unused) {
	uii.Cvar_SetValue ("cl_decals", m_effects_menu.dec_toggle.curvalue);
}

static void DecalLODFunc (void *unused) {
	uii.Cvar_SetValue ("cl_decal_lod", m_effects_menu.dec_lod_toggle.curvalue);
}

static void DecalLifeFunc (void *unused) {
	uii.Cvar_SetValue ("cl_decal_life", m_effects_menu.dec_life_slider.curvalue * 100);
	m_effects_menu.dec_life_amount.generic.name = uii.Cvar_VariableString ("cl_decal_life");
}
static void DecalBurnLifeFunc (void *unused) {
	uii.Cvar_SetValue ("cl_decal_burnlife", m_effects_menu.dec_burnlife_slider.curvalue * 10);
	m_effects_menu.dec_burnlife_amount.generic.name = uii.Cvar_VariableString ("cl_decal_burnlife");
}

static void DecalMaxFunc (void *unused) {
	uii.Cvar_SetValue ("cl_decal_max", m_effects_menu.dec_max_slider.curvalue * 1000);
	m_effects_menu.dec_max_amount.generic.name = uii.Cvar_VariableString ("cl_decal_max");
}

//
// shaders
//

static void AdvIRFunc (void *unused) {
	uii.Cvar_SetValue ("r_advir", m_effects_menu.shader_advir_toggle.curvalue);
}

static void ShadersFunc (void *unused) {
	uii.Cvar_SetValue ("r_shaders", m_effects_menu.shader_shaders_toggle.curvalue);
}

static void CausticsFunc (void *unused) {
	uii.Cvar_SetValue ("r_caustics", m_effects_menu.shader_caustics_toggle.curvalue);
}

/*
=============
EffectsMenu_SetValues
=============
*/
static void EffectsMenu_SetValues (void) {
	//
	// particles
	//

	uii.Cvar_SetValue ("cl_gore",			clamp (uii.Cvar_VariableInteger ("cl_gore"), 0, 10));
	m_effects_menu.part_gore_list.curvalue	= uii.Cvar_VariableInteger ("cl_gore");

	m_effects_menu.part_smokelinger_slider.curvalue	= uii.Cvar_VariableValue ("cl_smokelinger");
	m_effects_menu.part_smokelinger_amount.generic.name	= uii.Cvar_VariableString ("cl_smokelinger");

	uii.Cvar_SetValue ("cl_railtrail",				clamp (uii.Cvar_VariableInteger ("cl_railtrail"), 0, 2));
	m_effects_menu.part_railtrail_list.curvalue		= uii.Cvar_VariableInteger ("cl_railtrail");

	m_effects_menu.part_railred_slider.curvalue			= uii.Cvar_VariableValue ("cl_railred") * 10;
	m_effects_menu.part_railred_amount.generic.name		= uii.Cvar_VariableString ("cl_railred");
	m_effects_menu.part_railgreen_slider.curvalue		= uii.Cvar_VariableValue ("cl_railgreen") * 10;
	m_effects_menu.part_railgreen_amount.generic.name	= uii.Cvar_VariableString ("cl_railgreen");
	m_effects_menu.part_railblue_slider.curvalue		= uii.Cvar_VariableValue ("cl_railblue") * 10;
	m_effects_menu.part_railblue_amount.generic.name	= uii.Cvar_VariableString ("cl_railblue");

	m_effects_menu.part_spiralred_slider.curvalue		= uii.Cvar_VariableValue ("cl_spiralred") * 10;
	m_effects_menu.part_spiralred_amount.generic.name	= uii.Cvar_VariableString ("cl_spiralred");
	m_effects_menu.part_spiralgreen_slider.curvalue		= uii.Cvar_VariableValue ("cl_spiralgreen") * 10;
	m_effects_menu.part_spiralgreen_amount.generic.name	= uii.Cvar_VariableString ("cl_spiralgreen");
	m_effects_menu.part_spiralblue_slider.curvalue		= uii.Cvar_VariableValue ("cl_spiralblue") * 10;
	m_effects_menu.part_spiralblue_amount.generic.name	= uii.Cvar_VariableString ("cl_spiralblue");

	uii.Cvar_SetValue ("part_cull",				clamp (uii.Cvar_VariableInteger ("part_cull"), 0, 1));
	m_effects_menu.part_cull_toggle.curvalue	= uii.Cvar_VariableInteger ("part_cull");

	m_effects_menu.part_degree_slider.curvalue		= uii.Cvar_VariableValue ("part_degree") * 2;
	m_effects_menu.part_degree_amount.generic.name	= uii.Cvar_VariableString ("part_degree");

	uii.Cvar_SetValue ("part_lod",				clamp (uii.Cvar_VariableInteger ("part_lod"), 0, 1));
	m_effects_menu.part_lod_toggle.curvalue		= uii.Cvar_VariableInteger ("part_lod");

	uii.Cvar_SetValue ("part_shade",			clamp (uii.Cvar_VariableInteger ("part_shade"), 0, 1));
	m_effects_menu.part_shade_toggle.curvalue	= uii.Cvar_VariableInteger ("part_shade");

	uii.Cvar_SetValue ("cl_explorattle",					clamp (uii.Cvar_VariableInteger ("cl_explorattle"), 0, 1));
	m_effects_menu.explorattle_toggle.curvalue				= uii.Cvar_VariableInteger ("cl_explorattle");
	m_effects_menu.explorattle_scale_slider.curvalue		= uii.Cvar_VariableInteger ("cl_explorattle_scale");
	m_effects_menu.explorattle_scale_amount.generic.name	= uii.Cvar_VariableString ("cl_explorattle_scale");

	//
	// decals
	//

	uii.Cvar_SetValue ("cl_decal_lod",		clamp (uii.Cvar_VariableInteger ("cl_decal_lod"), 0, 1));
	m_effects_menu.dec_lod_toggle.curvalue	= uii.Cvar_VariableInteger ("cl_decal_lod");

	uii.Cvar_SetValue ("cl_decals",			clamp (uii.Cvar_VariableInteger ("cl_decals"), 0, 1));
	m_effects_menu.dec_toggle.curvalue		= uii.Cvar_VariableInteger ("cl_decals");

	m_effects_menu.dec_life_slider.curvalue		= uii.Cvar_VariableValue ("cl_decal_life") * 0.01;
	m_effects_menu.dec_life_amount.generic.name	= uii.Cvar_VariableString ("cl_decal_life");

	m_effects_menu.dec_burnlife_slider.curvalue		= uii.Cvar_VariableValue ("cl_decal_burnlife") * 0.1;
	m_effects_menu.dec_burnlife_amount.generic.name	= uii.Cvar_VariableString ("cl_decal_burnlife");

	m_effects_menu.dec_max_slider.curvalue		= uii.Cvar_VariableValue ("cl_decal_max") * 0.001;
	m_effects_menu.dec_max_amount.generic.name	= uii.Cvar_VariableString ("cl_decal_max");

	//
	// shaders
	//

	uii.Cvar_SetValue ("r_advir",				clamp (uii.Cvar_VariableInteger ("r_advir"), 0, 1));
	m_effects_menu.shader_advir_toggle.curvalue	= uii.Cvar_VariableInteger ("r_advir");

	uii.Cvar_SetValue ("r_shaders",					clamp (uii.Cvar_VariableInteger ("r_shaders"), 0, 1));
	m_effects_menu.shader_shaders_toggle.curvalue	= uii.Cvar_VariableInteger ("r_shaders");

	uii.Cvar_SetValue ("r_caustics",				clamp (uii.Cvar_VariableInteger ("r_caustics"), 0, 1));
	m_effects_menu.shader_caustics_toggle.curvalue	= uii.Cvar_VariableInteger ("r_caustics");
}


/*
=============
EffectsMenu_Init
=============
*/
void EffectsMenu_Init (void) {
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
		"beam + spiral",
		0
	};

	float	y;

	m_effects_menu.framework.x		= (uis.vidWidth*0.5);
	m_effects_menu.framework.y		= 0;
	m_effects_menu.framework.nitems	= 0;

	UI_Banner (&m_effects_menu.banner, uiMedia.optionsBanner, &y);

	//
	// particles
	//

	m_effects_menu.part_header.generic.type		= UITYPE_ACTION;
	m_effects_menu.part_header.generic.flags	= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_effects_menu.part_header.generic.x		= 0;
	m_effects_menu.part_header.generic.y		= y += UIFT_SIZEINC;
	m_effects_menu.part_header.generic.name		= "Particle Settings";

	m_effects_menu.part_gore_list.generic.type		= UITYPE_SPINCONTROL;
	m_effects_menu.part_gore_list.generic.x			= 0;
	m_effects_menu.part_gore_list.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_effects_menu.part_gore_list.generic.name		= "Gore severity";
	m_effects_menu.part_gore_list.generic.callback	= GoreTypeFunc;
	m_effects_menu.part_gore_list.itemnames			= goreamount_names;
	m_effects_menu.part_gore_list.generic.statusbar	= "Select your gore";

	m_effects_menu.part_smokelinger_slider.generic.type			= UITYPE_SLIDER;
	m_effects_menu.part_smokelinger_slider.generic.x			= 0;
	m_effects_menu.part_smokelinger_slider.generic.y			= y += UIFT_SIZEINC;
	m_effects_menu.part_smokelinger_slider.generic.name			= "Smoke lingering";
	m_effects_menu.part_smokelinger_slider.generic.callback		= SmokeLingerFunc;
	m_effects_menu.part_smokelinger_slider.minvalue				= 0;
	m_effects_menu.part_smokelinger_slider.maxvalue				= 10;
	m_effects_menu.part_smokelinger_slider.generic.statusbar	= "Customize smoke linger duration";
	m_effects_menu.part_smokelinger_amount.generic.type			= UITYPE_ACTION;
	m_effects_menu.part_smokelinger_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_effects_menu.part_smokelinger_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effects_menu.part_smokelinger_amount.generic.y			= y;

	m_effects_menu.part_railtrail_list.generic.type			= UITYPE_SPINCONTROL;
	m_effects_menu.part_railtrail_list.generic.x			= 0;
	m_effects_menu.part_railtrail_list.generic.y			= y += (UIFT_SIZEINC*2);
	m_effects_menu.part_railtrail_list.generic.name			= "Rail-Trail style";
	m_effects_menu.part_railtrail_list.generic.callback		= RailTrailFunc;
	m_effects_menu.part_railtrail_list.itemnames			= railtrail_names;
	m_effects_menu.part_railtrail_list.generic.statusbar	= "Select your style";

	m_effects_menu.part_railred_slider.generic.type			= UITYPE_SLIDER;
	m_effects_menu.part_railred_slider.generic.x			= 0;
	m_effects_menu.part_railred_slider.generic.y			= y += (UIFT_SIZEINC*2);
	m_effects_menu.part_railred_slider.generic.name			= "Rail center red";
	m_effects_menu.part_railred_slider.generic.callback		= RailRedColorFunc;
	m_effects_menu.part_railred_slider.minvalue				= 0;
	m_effects_menu.part_railred_slider.maxvalue				= 10;
	m_effects_menu.part_railred_slider.generic.statusbar	= "Rail core red color";
	m_effects_menu.part_railred_amount.generic.type			= UITYPE_ACTION;
	m_effects_menu.part_railred_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_effects_menu.part_railred_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effects_menu.part_railred_amount.generic.y			= y;

	m_effects_menu.part_railgreen_slider.generic.type		= UITYPE_SLIDER;
	m_effects_menu.part_railgreen_slider.generic.x			= 0;
	m_effects_menu.part_railgreen_slider.generic.y			= y += UIFT_SIZEINC;
	m_effects_menu.part_railgreen_slider.generic.name		= "Rail center green";
	m_effects_menu.part_railgreen_slider.generic.callback	= RailGreenColorFunc;
	m_effects_menu.part_railgreen_slider.minvalue			= 0;
	m_effects_menu.part_railgreen_slider.maxvalue			= 10;
	m_effects_menu.part_railgreen_slider.generic.statusbar	= "Rail core green color";
	m_effects_menu.part_railgreen_amount.generic.type		= UITYPE_ACTION;
	m_effects_menu.part_railgreen_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_effects_menu.part_railgreen_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effects_menu.part_railgreen_amount.generic.y			= y;

	m_effects_menu.part_railblue_slider.generic.type		= UITYPE_SLIDER;
	m_effects_menu.part_railblue_slider.generic.x			= 0;
	m_effects_menu.part_railblue_slider.generic.y			= y += UIFT_SIZEINC;
	m_effects_menu.part_railblue_slider.generic.name		= "Rail center blue";
	m_effects_menu.part_railblue_slider.generic.callback	= RailBlueColorFunc;
	m_effects_menu.part_railblue_slider.minvalue			= 0;
	m_effects_menu.part_railblue_slider.maxvalue			= 10;
	m_effects_menu.part_railblue_slider.generic.statusbar	= "Rail core blue color";
	m_effects_menu.part_railblue_amount.generic.type		= UITYPE_ACTION;
	m_effects_menu.part_railblue_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_effects_menu.part_railblue_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effects_menu.part_railblue_amount.generic.y			= y;

	m_effects_menu.part_spiralred_slider.generic.type		= UITYPE_SLIDER;
	m_effects_menu.part_spiralred_slider.generic.x			= 0;
	m_effects_menu.part_spiralred_slider.generic.y			= y += (UIFT_SIZEINC*2);
	m_effects_menu.part_spiralred_slider.generic.name		= "Rail spiral red";
	m_effects_menu.part_spiralred_slider.generic.callback	= SpiralRedColorFunc;
	m_effects_menu.part_spiralred_slider.minvalue			= 0;
	m_effects_menu.part_spiralred_slider.maxvalue			= 10;
	m_effects_menu.part_spiralred_slider.generic.statusbar	= "Rail spiral red color";
	m_effects_menu.part_spiralred_amount.generic.type		= UITYPE_ACTION;
	m_effects_menu.part_spiralred_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_effects_menu.part_spiralred_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effects_menu.part_spiralred_amount.generic.y			= y;

	m_effects_menu.part_spiralgreen_slider.generic.type			= UITYPE_SLIDER;
	m_effects_menu.part_spiralgreen_slider.generic.x			= 0;
	m_effects_menu.part_spiralgreen_slider.generic.y			= y += UIFT_SIZEINC;
	m_effects_menu.part_spiralgreen_slider.generic.name			= "Rail spiral green";
	m_effects_menu.part_spiralgreen_slider.generic.callback		= SpiralGreenColorFunc;
	m_effects_menu.part_spiralgreen_slider.minvalue				= 0;
	m_effects_menu.part_spiralgreen_slider.maxvalue				= 10;
	m_effects_menu.part_spiralgreen_slider.generic.statusbar	= "Rail spiral green color";
	m_effects_menu.part_spiralgreen_amount.generic.type			= UITYPE_ACTION;
	m_effects_menu.part_spiralgreen_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_effects_menu.part_spiralgreen_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effects_menu.part_spiralgreen_amount.generic.y			= y;

	m_effects_menu.part_spiralblue_slider.generic.type		= UITYPE_SLIDER;
	m_effects_menu.part_spiralblue_slider.generic.x			= 0;
	m_effects_menu.part_spiralblue_slider.generic.y			= y += UIFT_SIZEINC;
	m_effects_menu.part_spiralblue_slider.generic.name		= "Rail spiral blue";
	m_effects_menu.part_spiralblue_slider.generic.callback	= SpiralBlueColorFunc;
	m_effects_menu.part_spiralblue_slider.minvalue			= 0;
	m_effects_menu.part_spiralblue_slider.maxvalue			= 10;
	m_effects_menu.part_spiralblue_slider.generic.statusbar	= "Rail spiral blue color";
	m_effects_menu.part_spiralblue_amount.generic.type		= UITYPE_ACTION;
	m_effects_menu.part_spiralblue_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_effects_menu.part_spiralblue_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effects_menu.part_spiralblue_amount.generic.y			= y;

	m_effects_menu.part_cull_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_effects_menu.part_cull_toggle.generic.x			= 0;
	m_effects_menu.part_cull_toggle.generic.y			= y += (UIFT_SIZEINC*2);
	m_effects_menu.part_cull_toggle.generic.name		= "Particle culling";
	m_effects_menu.part_cull_toggle.generic.callback	= PartCullFunc;
	m_effects_menu.part_cull_toggle.itemnames			= onoff_names;
	m_effects_menu.part_cull_toggle.generic.statusbar	= "Particle culling (on = faster)";

	m_effects_menu.part_degree_slider.generic.type		= UITYPE_SLIDER;
	m_effects_menu.part_degree_slider.generic.x			= 0;
	m_effects_menu.part_degree_slider.generic.y			= y += UIFT_SIZEINC;
	m_effects_menu.part_degree_slider.generic.name		= "Particle quality";
	m_effects_menu.part_degree_slider.generic.callback	= PartDegreeFunc;
	m_effects_menu.part_degree_slider.minvalue			= 0;
	m_effects_menu.part_degree_slider.maxvalue			= 4;
	m_effects_menu.part_degree_slider.generic.statusbar	= "Particle quality multiplier (lower = faster)";
	m_effects_menu.part_degree_amount.generic.type		= UITYPE_ACTION;
	m_effects_menu.part_degree_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_effects_menu.part_degree_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effects_menu.part_degree_amount.generic.y			= y;

	m_effects_menu.part_lod_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_effects_menu.part_lod_toggle.generic.x			= 0;
	m_effects_menu.part_lod_toggle.generic.y			= y += UIFT_SIZEINC;
	m_effects_menu.part_lod_toggle.generic.name			= "Particle LOD";
	m_effects_menu.part_lod_toggle.generic.callback		= PartLODFunc;
	m_effects_menu.part_lod_toggle.itemnames			= onoff_names;
	m_effects_menu.part_lod_toggle.generic.statusbar	= "Particle 'Level of Detail' (on = faster)";

	m_effects_menu.part_shade_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_effects_menu.part_shade_toggle.generic.x			= 0;
	m_effects_menu.part_shade_toggle.generic.y			= y += UIFT_SIZEINC;
	m_effects_menu.part_shade_toggle.generic.name		= "Particle lighting";
	m_effects_menu.part_shade_toggle.generic.callback	= PartShadeFunc;
	m_effects_menu.part_shade_toggle.itemnames			= onoff_names;
	m_effects_menu.part_shade_toggle.generic.statusbar	= "Enables particle lighting";

	m_effects_menu.explorattle_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_effects_menu.explorattle_toggle.generic.x			= 0;
	m_effects_menu.explorattle_toggle.generic.y			= y += UIFT_SIZEINC;
	m_effects_menu.explorattle_toggle.generic.name		= "Explosion rattling";
	m_effects_menu.explorattle_toggle.generic.callback	= ExploRattleFunc;
	m_effects_menu.explorattle_toggle.itemnames			= onoff_names;
	m_effects_menu.explorattle_toggle.generic.statusbar	= "Toggles screen rattling when near explosions";

	m_effects_menu.explorattle_scale_slider.generic.type		= UITYPE_SLIDER;
	m_effects_menu.explorattle_scale_slider.generic.x			= 0;
	m_effects_menu.explorattle_scale_slider.generic.y			= y += UIFT_SIZEINC;
	m_effects_menu.explorattle_scale_slider.generic.name		= "Rattle severity";
	m_effects_menu.explorattle_scale_slider.generic.callback	= ExploRattleScaleFunc;
	m_effects_menu.explorattle_scale_slider.minvalue			= 0;
	m_effects_menu.explorattle_scale_slider.maxvalue			= 9;
	m_effects_menu.explorattle_scale_slider.generic.statusbar	= "Severity of explosion rattles";
	m_effects_menu.explorattle_scale_amount.generic.type		= UITYPE_ACTION;
	m_effects_menu.explorattle_scale_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_effects_menu.explorattle_scale_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effects_menu.explorattle_scale_amount.generic.y			= y;

	//
	// decals
	//

	m_effects_menu.dec_header.generic.type		= UITYPE_ACTION;
	m_effects_menu.dec_header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_effects_menu.dec_header.generic.x			= 0;
	m_effects_menu.dec_header.generic.y			= y += UIFT_SIZEINC*2;
	m_effects_menu.dec_header.generic.name		= "Decal Settings";

	m_effects_menu.dec_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_effects_menu.dec_toggle.generic.x			= 0;
	m_effects_menu.dec_toggle.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_effects_menu.dec_toggle.generic.name		= "Decals";
	m_effects_menu.dec_toggle.generic.callback	= DecalToggleFunc;
	m_effects_menu.dec_toggle.itemnames			= onoff_names;
	m_effects_menu.dec_toggle.generic.statusbar	= "Toggle decals on or off";

	m_effects_menu.dec_lod_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_effects_menu.dec_lod_toggle.generic.x			= 0;
	m_effects_menu.dec_lod_toggle.generic.y			= y += UIFT_SIZEINC;
	m_effects_menu.dec_lod_toggle.generic.name		= "Decal LOD";
	m_effects_menu.dec_lod_toggle.generic.callback	= DecalLODFunc;
	m_effects_menu.dec_lod_toggle.itemnames			= onoff_names;
	m_effects_menu.dec_lod_toggle.generic.statusbar	= "Toggle decal lod (skips small far away decals)";

	m_effects_menu.dec_life_slider.generic.type			= UITYPE_SLIDER;
	m_effects_menu.dec_life_slider.generic.x			= 0;
	m_effects_menu.dec_life_slider.generic.y			= y += UIFT_SIZEINC;
	m_effects_menu.dec_life_slider.generic.name			= "Decal life";
	m_effects_menu.dec_life_slider.generic.callback		= DecalLifeFunc;
	m_effects_menu.dec_life_slider.minvalue				= 0;
	m_effects_menu.dec_life_slider.maxvalue				= 10;
	m_effects_menu.dec_life_slider.generic.statusbar	= "Decal life time";
	m_effects_menu.dec_life_amount.generic.type			= UITYPE_ACTION;
	m_effects_menu.dec_life_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_effects_menu.dec_life_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effects_menu.dec_life_amount.generic.y			= y;

	m_effects_menu.dec_burnlife_slider.generic.type			= UITYPE_SLIDER;
	m_effects_menu.dec_burnlife_slider.generic.x			= 0;
	m_effects_menu.dec_burnlife_slider.generic.y			= y += UIFT_SIZEINC;
	m_effects_menu.dec_burnlife_slider.generic.name			= "Burn life";
	m_effects_menu.dec_burnlife_slider.generic.callback		= DecalBurnLifeFunc;
	m_effects_menu.dec_burnlife_slider.minvalue				= 0;
	m_effects_menu.dec_burnlife_slider.maxvalue				= 20;
	m_effects_menu.dec_burnlife_slider.generic.statusbar	= "Decal burn life time";
	m_effects_menu.dec_burnlife_amount.generic.type			= UITYPE_ACTION;
	m_effects_menu.dec_burnlife_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_effects_menu.dec_burnlife_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effects_menu.dec_burnlife_amount.generic.y			= y;

	m_effects_menu.dec_max_slider.generic.type		= UITYPE_SLIDER;
	m_effects_menu.dec_max_slider.generic.x			= 0;
	m_effects_menu.dec_max_slider.generic.y			= y += UIFT_SIZEINC;
	m_effects_menu.dec_max_slider.generic.name		= "Max decals";
	m_effects_menu.dec_max_slider.generic.callback	= DecalMaxFunc;
	m_effects_menu.dec_max_slider.minvalue			= 1;
	m_effects_menu.dec_max_slider.maxvalue			= MAX_DECALS / 1000;
	m_effects_menu.dec_max_slider.generic.statusbar	= "Maximum decals allowed";
	m_effects_menu.dec_max_amount.generic.type		= UITYPE_ACTION;
	m_effects_menu.dec_max_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;
	m_effects_menu.dec_max_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_effects_menu.dec_max_amount.generic.y			= y;

	//
	// shaders
	//

	m_effects_menu.shader_header.generic.type		= UITYPE_ACTION;
	m_effects_menu.shader_header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_effects_menu.shader_header.generic.x			= 0;
	m_effects_menu.shader_header.generic.y			= y += UIFT_SIZEINC*2;
	m_effects_menu.shader_header.generic.name		= "Shader Settings";

	m_effects_menu.shader_advir_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_effects_menu.shader_advir_toggle.generic.x			= 0;
	m_effects_menu.shader_advir_toggle.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_effects_menu.shader_advir_toggle.generic.name			= "Nicer infrared (not done)";
	m_effects_menu.shader_advir_toggle.generic.callback		= AdvIRFunc;
	m_effects_menu.shader_advir_toggle.itemnames			= nicenorm_names;
	m_effects_menu.shader_advir_toggle.generic.statusbar	= "Advanced Infra-red (not done)";

	m_effects_menu.shader_shaders_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_effects_menu.shader_shaders_toggle.generic.x			= 0;
	m_effects_menu.shader_shaders_toggle.generic.y			= y += (UIFT_SIZEINC * 2);
	m_effects_menu.shader_shaders_toggle.generic.name		= "Shaders";
	m_effects_menu.shader_shaders_toggle.generic.callback	= ShadersFunc;
	m_effects_menu.shader_shaders_toggle.itemnames			= onoff_names;
	m_effects_menu.shader_shaders_toggle.generic.statusbar	= "Use shaders";
	
	m_effects_menu.shader_caustics_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_effects_menu.shader_caustics_toggle.generic.x			= 0;
	m_effects_menu.shader_caustics_toggle.generic.y			= y += UIFT_SIZEINC;
	m_effects_menu.shader_caustics_toggle.generic.name		= "Caustics";
	m_effects_menu.shader_caustics_toggle.generic.callback	= CausticsFunc;
	m_effects_menu.shader_caustics_toggle.itemnames			= onoff_names;
	m_effects_menu.shader_caustics_toggle.generic.statusbar	= "Shader-based underwater/lava/slime surface caustics";

	//
	// misc
	//

	m_effects_menu.back_action.generic.type			= UITYPE_ACTION;
	m_effects_menu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_effects_menu.back_action.generic.x			= 0;
	m_effects_menu.back_action.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCLG;
	m_effects_menu.back_action.generic.name			= "< Back";
	m_effects_menu.back_action.generic.callback		= Back_Menu;
	m_effects_menu.back_action.generic.statusbar	= "Back a menu";

	EffectsMenu_SetValues ();


	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.banner);

	//
	// particles
	//

	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.part_header);

	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.part_gore_list);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.part_smokelinger_slider);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.part_smokelinger_amount);

	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.part_railtrail_list);

	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.part_railred_slider);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.part_railred_amount);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.part_railgreen_slider);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.part_railgreen_amount);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.part_railblue_slider);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.part_railblue_amount);

	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.part_spiralred_slider);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.part_spiralred_amount);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.part_spiralgreen_slider);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.part_spiralgreen_amount);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.part_spiralblue_slider);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.part_spiralblue_amount);

	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.part_cull_toggle);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.part_degree_slider);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.part_degree_amount);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.part_lod_toggle);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.part_shade_toggle);

	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.explorattle_toggle);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.explorattle_scale_slider);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.explorattle_scale_amount);

	//
	// decals
	//

	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.dec_header);

	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.dec_toggle);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.dec_lod_toggle);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.dec_life_slider);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.dec_life_amount);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.dec_burnlife_slider);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.dec_burnlife_amount);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.dec_max_slider);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.dec_max_amount);

	//
	// shaders
	//

	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.shader_header);

	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.shader_advir_toggle);

	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.shader_shaders_toggle);
	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.shader_caustics_toggle);

	//
	// misc
	//

	UI_AddItem (&m_effects_menu.framework,		&m_effects_menu.back_action);

	UI_Center (&m_effects_menu.framework);
	UI_Setup (&m_effects_menu.framework);
}


/*
=============
EffectsMenu_Draw
=============
*/
void EffectsMenu_Draw (void) {
	UI_Center (&m_effects_menu.framework);
	UI_Setup (&m_effects_menu.framework);

	UI_AdjustTextCursor (&m_effects_menu.framework, 1);
	UI_Draw (&m_effects_menu.framework);
}


/*
=============
EffectsMenu_Key
=============
*/
const char *EffectsMenu_Key (int key) {
	return DefaultMenu_Key (&m_effects_menu.framework, key);
}

/*
=============
UI_EffectsMenu_f
=============
*/

void UI_EffectsMenu_f (void) {
	EffectsMenu_Init ();
	UI_PushMenu (&m_effects_menu.framework, EffectsMenu_Draw, EffectsMenu_Key);
}
