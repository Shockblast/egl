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

//
// cg_media.c
//

#include "cg_local.h"

vec3_t	avelocities[NUMVERTEXNORMALS];

/*
=============================================================================

	PALETTE COLORING

	Fix by removing the need for these, by using actual RGB
	colors and ditch the stupid shit palette
=============================================================================
*/

static byte cg_colorPal[] = {
	#include "colorpal.h"
};
float	palRed		(int index) { return (cg_colorPal[index*3+0]); }
float	palGreen	(int index) { return (cg_colorPal[index*3+1]); }
float	palBlue		(int index) { return (cg_colorPal[index*3+2]); }

float	palRedf		(int index) { return ((cg_colorPal[index*3+0] > 0) ? cg_colorPal[index*3+0] * (1.0f/255.0f) : 0); }
float	palGreenf	(int index) { return ((cg_colorPal[index*3+1] > 0) ? cg_colorPal[index*3+1] * (1.0f/255.0f) : 0); }
float	palBluef	(int index) { return ((cg_colorPal[index*3+2] > 0) ? cg_colorPal[index*3+2] * (1.0f/255.0f) : 0); }

/*
=============================================================================

	MEDIA INITIALIZATION

=============================================================================
*/

#define CG_MAPMEDIA_RANGE		30
#define CG_MAPMEDIA_PERCENT		30
#define CG_MODELMEDIA_RANGE		10
#define CG_MODELMEDIA_PERCENT	40
#define CG_PICMEDIA_RANGE		10
#define CG_PICMEDIA_PERCENT		50
#define CG_FXMEDIA_RANGE		15
#define CG_FXMEDIA_PERCENT		65
#define CG_SOUNDMEDIA_RANGE		15
#define CG_SOUNDMEDIA_PERCENT	80
#define CG_GLOOMMEDIA_RANGE		20
#define CG_GLOOMMEDIA_PERCENT	100

/*
================
CG_InitBaseMedia
================
*/
void CG_InitBaseMedia (void)
{
	if (cgMedia.baseInitialized)
		return;
	cgMedia.baseInitialized = qTrue;

	cgMedia.conCharsShader = cgi.R_RegisterPic (Q_VarArgs ("fonts/%s.tga", cgi.Cvar_VariableString ("con_font")));
	if (!cgMedia.conCharsShader) {
		cgMedia.conCharsShader = cgi.R_RegisterPic ("pics/conchars.tga");
	}
}


/*
================
CG_MapMediaInit
================
*/
void CG_MapMediaInit (void)
{
	char		mapName[64];
	char		name[MAX_QPATH];
	int			i;
	float		rotate, pctInc;
	vec3_t		axis;

	if (!cg.configStrings[CS_MODELS+1][0])
		return;

	// Let the render dll load the map
	Q_strncpyz (mapName, cg.configStrings[CS_MODELS+1] + 5, sizeof (mapName));	// skip "maps/"
	mapName[strlen (mapName) - 4] = 0;											// cut off ".bsp"

	// Register models, pics, and skins
	CG_LoadingString ("Loading map...");
	cgi.R_RegisterMap (mapName);

	CG_IncLoadPercent (CG_MAPMEDIA_RANGE*0.1);

	// Register map effects
	CG_LoadingString ("Loading map fx...");
	CG_MapFXInit (mapName);

	CG_IncLoadPercent (CG_MAPMEDIA_RANGE*0.05);

	// Load models
	CG_LoadingString ("Loading models...");
	cg_numWeaponModels = 1;
	Q_strncpyz (cg_weaponModels[0], "weapon.md2", sizeof (cg_weaponModels[0]));
	for (i=1 ; i<MAX_CS_MODELS && cg.configStrings[CS_MODELS+i][0] ; i++) ;
	pctInc = 1.0f/(float)i;

	for (i=1 ; i<MAX_CS_MODELS && cg.configStrings[CS_MODELS+i][0] ; i++) {
		Q_strncpyz (name, cg.configStrings[CS_MODELS+i], sizeof (name));
		name[37] = 0;	// never go beyond one line
		if (name[0] != '*')
			CG_LoadingFilename (name);

		cgi.R_UpdateScreen ();
		cgi.Sys_SendKeyEvents ();	// pump message loop
		if (name[0] == '#') {
			// Special player weapon model
			if (cg_numWeaponModels < MAX_CLIENTWEAPONMODELS) {
				Q_strncpyz (cg_weaponModels[cg_numWeaponModels], cg.configStrings[CS_MODELS+i]+1,
					sizeof (cg_weaponModels[cg_numWeaponModels]));
				cg_numWeaponModels++;
			}
		}
		else {
			cg.modelDraw[i] = cgi.R_RegisterModel (cg.configStrings[CS_MODELS+i]);
			if (name[0] == '*')
				cg.modelClip[i] = CGI_CM_InlineModel (cg.configStrings[CS_MODELS+i]);
			else
				cg.modelClip[i] = NULL;
		}


		CG_IncLoadPercent (pctInc*(CG_MAPMEDIA_RANGE*0.35f));

		if (name[0] != '*')
			CG_LoadingFilename (0);
	}

	// Images
	CG_LoadingString ("Loading images...");
	for (i=1 ; i<MAX_CS_IMAGES && cg.configStrings[CS_IMAGES+i][0] ; i++) {
		CG_LoadingFilename (cg.configStrings[CS_IMAGES+i]);
		CG_RegisterPic (cg.configStrings[CS_IMAGES+i]);
		cgi.Sys_SendKeyEvents ();	// Pump message loop
	}

	CG_LoadingFilename (0);
	CG_IncLoadPercent (CG_MAPMEDIA_RANGE*0.2);

	// Clients
	CG_LoadingString ("Loading clients...");
	for (i=0 ; i<MAX_CS_CLIENTS ; i++) {
		if (!cg.configStrings[CS_PLAYERSKINS+i][0])
			continue;

		CG_LoadingFilename (Q_VarArgs ("Client #%i", i));
		cgi.Sys_SendKeyEvents ();	// Pump message loop
		CG_ParseClientinfo (i);
	}

	CG_IncLoadPercent (CG_MAPMEDIA_RANGE*0.15);

	CG_LoadingFilename ("Base client info");
	CG_LoadClientinfo (&cg.baseClientInfo, "unnamed\\male/grunt");
	CG_LoadingFilename (0);

	CG_IncLoadPercent (CG_MAPMEDIA_RANGE*0.05);

	// Set sky textures and speed
	CG_LoadingString ("Loading sky env...");
	rotate = (float)atof (cg.configStrings[CS_SKYROTATE]);
	sscanf (cg.configStrings[CS_SKYAXIS], "%f %f %f", &axis[0], &axis[1], &axis[2]);
	cgi.R_SetSky (cg.configStrings[CS_SKY], rotate, axis);
}


/*
================
CG_ModelMediaInit
================
*/
void CG_ModelMediaInit (void)
{
	CG_LoadingString ("Loading model media...");
	CG_LoadingFilename ("Segment models");

	cgMedia.parasiteSegmentMOD	= cgi.R_RegisterModel ("models/monsters/parasite/segment/tris.md2");
	cgMedia.grappleCableMOD		= cgi.R_RegisterModel ("models/ctf/segment/tris.md2");

	CG_IncLoadPercent (CG_MODELMEDIA_RANGE*0.30);
	CG_LoadingFilename ("Beam models");

	cgMedia.lightningMOD		= cgi.R_RegisterModel ("models/proj/lightning/tris.md2");
	cgMedia.heatBeamMOD			= cgi.R_RegisterModel ("models/proj/beam/tris.md2");
	cgMedia.monsterHeatBeamMOD	= cgi.R_RegisterModel ("models/proj/widowbeam/tris.md2");

	CG_IncLoadPercent (CG_MODELMEDIA_RANGE*0.40);
	CG_LoadingFilename ("Disguise models");

	cgMedia.maleDisguiseModel	= cgi.R_RegisterModel ("players/male/tris.md2");
	cgMedia.femaleDisguiseModel	= cgi.R_RegisterModel ("players/female/tris.md2");
	cgMedia.cyborgDisguiseModel	= cgi.R_RegisterModel ("players/cyborg/tris.md2");

	CG_LoadingFilename (0);
}


/*
================
CG_CrosshairShaderInit
================
*/
void CG_CrosshairShaderInit (void)
{
	crosshair->modified = qFalse;
	if (crosshair->integer) {
		crosshair->integer = (crosshair->integer < 0) ? 0 : crosshair->integer;

		cgMedia.crosshairShader = cgi.R_RegisterPic (Q_VarArgs ("pics/ch%d.tga", crosshair->integer));
	}
}


/*
================
CG_PicMediaInit
================
*/
void CG_PicMediaInit (void)
{
	int		i, j;
	static char	*sb_nums[2][11] = {
		{"num_0",  "num_1",  "num_2",  "num_3",  "num_4",  "num_5",  "num_6",  "num_7",  "num_8",  "num_9",  "num_minus"},
		{"anum_0", "anum_1", "anum_2", "anum_3", "anum_4", "anum_5", "anum_6", "anum_7", "anum_8", "anum_9", "anum_minus"}
	};

	CG_LoadingString ("Loading image media...");
	CG_LoadingFilename ("Crosshair");

	CG_CrosshairShaderInit ();

	CG_IncLoadPercent (CG_PICMEDIA_RANGE*0.25);
	CG_LoadingFilename ("Pics");

	cgMedia.noTexture			= cgi.R_RegisterTexture ("***r_noTexture***", -1);
	cgMedia.whiteTexture		= cgi.R_RegisterTexture ("***r_whiteTexture***", -1);
	cgMedia.blackTexture		= cgi.R_RegisterTexture ("***r_blackTexture***", -1);

	cgMedia.tileBackShader		= cgi.R_RegisterPic ("pics/backtile.tga");

	cgMedia.alienInfraredVision	= cgi.R_RegisterPic ("alienInfraredVision");
	cgMedia.infraredGoggles		= cgi.R_RegisterPic ("infraredGoggles");

	cgi.R_RegisterPic ("pics/w_machinegun.tga");
	cgi.R_RegisterPic ("pics/a_bullets.tga");
	cgi.R_RegisterPic ("pics/i_health.tga");
	cgi.R_RegisterPic ("pics/a_grenades.tga");

	CG_IncLoadPercent (CG_PICMEDIA_RANGE*0.25);
	CG_LoadingFilename ("HUD");

	cgMedia.hudFieldShader		= cgi.R_RegisterPic ("pics/field_3.tga");
	cgMedia.hudInventoryShader	= cgi.R_RegisterPic ("pics/inventory.tga");
	cgMedia.hudNetShader		= cgi.R_RegisterPic ("pics/net.tga");
	for (i=0 ; i<2 ; i++) {
		for (j=0 ; j<11 ; j++)
			cgMedia.hudNumShaders[i][j] = cgi.R_RegisterPic (Q_VarArgs ("pics/%s.tga", sb_nums[i][j]));
	}
	cgMedia.hudPausedShader		= cgi.R_RegisterPic ("pics/pause.tga");

	CG_IncLoadPercent (CG_PICMEDIA_RANGE*0.25);
	CG_LoadingFilename ("Disguise skins");

	cgMedia.maleDisguiseSkin	= cgi.R_RegisterSkin ("players/male/disguise.tga");
	cgMedia.femaleDisguiseSkin	= cgi.R_RegisterSkin ("players/female/disguise.tga");
	cgMedia.cyborgDisguiseSkin	= cgi.R_RegisterSkin ("players/cyborg/disguise.tga");

	CG_LoadingFilename ("Shell skins");
	cgMedia.modelShellGod		= cgi.R_RegisterSkin ("shell_god");
	cgMedia.modelShellHalfDam	= cgi.R_RegisterSkin ("shell_halfdam");
	cgMedia.modelShellDouble	= cgi.R_RegisterSkin ("shell_double");
	cgMedia.modelShellRed		= cgi.R_RegisterSkin ("shell_red");
	cgMedia.modelShellGreen		= cgi.R_RegisterSkin ("shell_green");
	cgMedia.modelShellBlue		= cgi.R_RegisterSkin ("shell_blue");

	CG_LoadingFilename (0);
}


/*
================
CG_FXMediaInit
================
*/
void CG_FXMediaInit (void)
{
	int		i;

	// Particles / Decals
	CG_LoadingString ("Loading effect media...");
	CG_LoadingFilename ("Particles");

	for (i=0 ; i<(NUMVERTEXNORMALS * 3) ; i++)
		avelocities[0][i] = (frand () * 255) * 0.01f;

	cgMedia.particleTable[PT_BFG_DOT]		= cgi.R_RegisterPoly ("egl/parts/bfg_dot.tga");

	cgMedia.particleTable[PT_BLASTER_BLUE]	= cgi.R_RegisterPoly ("egl/parts/blaster_blue.tga");
	cgMedia.particleTable[PT_BLASTER_GREEN]	= cgi.R_RegisterPoly ("egl/parts/blaster_green.tga");
	cgMedia.particleTable[PT_BLASTER_RED]	= cgi.R_RegisterPoly ("egl/parts/blaster_red.tga");

	cgMedia.particleTable[PT_IONTAIL]		= cgi.R_RegisterPoly ("egl/parts/iontail.tga");
	cgMedia.particleTable[PT_IONTIP]		= cgi.R_RegisterPoly ("egl/parts/iontip.tga");
	cgMedia.particleTable[PT_ITEMRESPAWN]	= cgi.R_RegisterPoly ("egl/parts/respawn_dots.tga");
	cgMedia.particleTable[PT_ENGYREPAIR_DOT]= cgi.R_RegisterPoly ("egl/parts/engy_repair_dot.tga");
	cgMedia.particleTable[PT_PHALANXTIP]	= cgi.R_RegisterPoly ("egl/parts/phalanxtip.tga");

	cgMedia.particleTable[PT_GENERIC]		= cgi.R_RegisterPoly ("egl/parts/generic.tga");
	cgMedia.particleTable[PT_GENERIC_GLOW]	= cgi.R_RegisterPoly ("egl/parts/generic_glow.tga");

	cgMedia.particleTable[PT_SMOKE]			= cgi.R_RegisterPoly ("egl/parts/smoke1.tga");
	cgMedia.particleTable[PT_SMOKE2]		= cgi.R_RegisterPoly ("egl/parts/smoke2.tga");

	cgMedia.particleTable[PT_SMOKEGLOW]		= cgi.R_RegisterPoly ("egl/parts/smoke_glow.tga");
	cgMedia.particleTable[PT_SMOKEGLOW2]	= cgi.R_RegisterPoly ("egl/parts/smoke_glow2.tga");

	cgMedia.particleTable[PT_BLUEFIRE]		= cgi.R_RegisterPoly ("egl/parts/bluefire.tga");
	cgMedia.particleTable[PT_FIRE1]			= cgi.R_RegisterPoly ("egl/parts/fire1.tga");
	cgMedia.particleTable[PT_FIRE2]			= cgi.R_RegisterPoly ("egl/parts/fire2.tga");
	cgMedia.particleTable[PT_FIRE3]			= cgi.R_RegisterPoly ("egl/parts/fire3.tga");
	cgMedia.particleTable[PT_FIRE4]			= cgi.R_RegisterPoly ("egl/parts/fire4.tga");
	cgMedia.particleTable[PT_EMBERS1]		= cgi.R_RegisterPoly ("egl/parts/embers1.tga");
	cgMedia.particleTable[PT_EMBERS2]		= cgi.R_RegisterPoly ("egl/parts/embers2.tga");
	cgMedia.particleTable[PT_EMBERS3]		= cgi.R_RegisterPoly ("egl/parts/embers3.tga");

	cgMedia.particleTable[PT_BLOOD]			= cgi.R_RegisterPoly ("egl/parts/blood.tga");
	cgMedia.particleTable[PT_BLOOD2]		= cgi.R_RegisterPoly ("egl/parts/blood2.tga");
	cgMedia.particleTable[PT_BLOOD3]		= cgi.R_RegisterPoly ("egl/parts/blood3.tga");
	cgMedia.particleTable[PT_BLOOD4]		= cgi.R_RegisterPoly ("egl/parts/blood4.tga");
	cgMedia.particleTable[PT_BLOOD5]		= cgi.R_RegisterPoly ("egl/parts/blood5.tga");
	cgMedia.particleTable[PT_BLOOD6]		= cgi.R_RegisterPoly ("egl/parts/blood6.tga");

	cgMedia.particleTable[PT_GRNBLOOD]		= cgi.R_RegisterPoly ("egl/parts/blood_grn.tga");
	cgMedia.particleTable[PT_GRNBLOOD2]		= cgi.R_RegisterPoly ("egl/parts/blood_grn2.tga");
	cgMedia.particleTable[PT_GRNBLOOD3]		= cgi.R_RegisterPoly ("egl/parts/blood_grn3.tga");
	cgMedia.particleTable[PT_GRNBLOOD4]		= cgi.R_RegisterPoly ("egl/parts/blood_grn4.tga");
	cgMedia.particleTable[PT_GRNBLOOD5]		= cgi.R_RegisterPoly ("egl/parts/blood_grn5.tga");
	cgMedia.particleTable[PT_GRNBLOOD6]		= cgi.R_RegisterPoly ("egl/parts/blood_grn6.tga");

	CG_IncLoadPercent (CG_FXMEDIA_RANGE*0.25);

	cgMedia.particleTable[PT_BEAM]			= cgi.R_RegisterPoly ("egl/parts/beam.tga");

	cgMedia.particleTable[PT_BLDDRIP]		= cgi.R_RegisterPoly ("egl/parts/blooddrip.tga");
	cgMedia.particleTable[PT_BLDDRIP_GRN]	= cgi.R_RegisterPoly ("egl/parts/blooddrip_green.tga");
	cgMedia.particleTable[PT_BLDSPURT]		= cgi.R_RegisterPoly ("egl/parts/bloodspurt.tga");
	cgMedia.particleTable[PT_BLDSPURT2]		= cgi.R_RegisterPoly ("egl/parts/bloodspurt2.tga");

	cgMedia.particleTable[PT_EXPLOFLASH]	= cgi.R_RegisterPoly ("egl/parts/exploflash.tga");
	cgMedia.particleTable[PT_EXPLOWAVE]		= cgi.R_RegisterPoly ("egl/parts/explowave.tga");

	cgMedia.particleTable[PT_FLARE]			= cgi.R_RegisterPoly ("egl/parts/flare.tga");
	cgMedia.particleTable[PT_FLAREGLOW]		= cgi.R_RegisterPoly ("egl/parts/flare_glow.tga");

	cgMedia.particleTable[PT_FLY]			= cgi.R_RegisterPoly ("egl/parts/fly.tga");

	cgMedia.particleTable[PT_RAIL_CORE]		= cgi.R_RegisterPoly ("egl/parts/rail_core.tga");
	cgMedia.particleTable[PT_RAIL_WAVE]		= cgi.R_RegisterPoly ("egl/parts/rail_wave.tga");
	cgMedia.particleTable[PT_SPIRAL]		= cgi.R_RegisterPoly ("egl/parts/rail_spiral.tga");

	cgMedia.particleTable[PT_SPARK]			= cgi.R_RegisterPoly ("egl/parts/spark.tga");

	cgMedia.particleTable[PT_WATERBUBBLE]		= cgi.R_RegisterPoly ("egl/parts/water_bubble.tga");
	cgMedia.particleTable[PT_WATERDROPLET]		= cgi.R_RegisterPoly ("egl/parts/water_droplet.tga");
	cgMedia.particleTable[PT_WATERIMPACT]		= cgi.R_RegisterPoly ("egl/parts/water_impact.tga");
	cgMedia.particleTable[PT_WATERMIST]			= cgi.R_RegisterPoly ("egl/parts/water_mist.tga");
	cgMedia.particleTable[PT_WATERMIST_GLOW]	= cgi.R_RegisterPoly ("egl/parts/water_mist_glow.tga");
	cgMedia.particleTable[PT_WATERPLUME]		= cgi.R_RegisterPoly ("egl/parts/water_plume.tga");
	cgMedia.particleTable[PT_WATERPLUME_GLOW]	= cgi.R_RegisterPoly ("egl/parts/water_plume_glow.tga");
	cgMedia.particleTable[PT_WATERRING]			= cgi.R_RegisterPoly ("egl/parts/water_ring.tga");
	cgMedia.particleTable[PT_WATERRIPPLE]		= cgi.R_RegisterPoly ("egl/parts/water_ripple.tga");

	// Animated explosions
	CG_IncLoadPercent (CG_FXMEDIA_RANGE*0.25);
	CG_LoadingFilename ("Explosions");

	cgMedia.particleTable[PT_EXPLO1]		= cgi.R_RegisterPoly ("egl/parts/explo1.tga");
	cgMedia.particleTable[PT_EXPLO2]		= cgi.R_RegisterPoly ("egl/parts/explo2.tga");
	cgMedia.particleTable[PT_EXPLO3]		= cgi.R_RegisterPoly ("egl/parts/explo3.tga");
	cgMedia.particleTable[PT_EXPLO4]		= cgi.R_RegisterPoly ("egl/parts/explo4.tga");
	cgMedia.particleTable[PT_EXPLO5]		= cgi.R_RegisterPoly ("egl/parts/explo5.tga");
	cgMedia.particleTable[PT_EXPLO6]		= cgi.R_RegisterPoly ("egl/parts/explo6.tga");
	cgMedia.particleTable[PT_EXPLO7]		= cgi.R_RegisterPoly ("egl/parts/explo7.tga");

	cgMedia.particleTable[PT_EXPLOEMBERS1]	= cgi.R_RegisterPoly ("egl/parts/exploembers.tga");
	cgMedia.particleTable[PT_EXPLOEMBERS2]	= cgi.R_RegisterPoly ("egl/parts/exploembers2.tga");

	// mapfx media
	CG_LoadingFilename ("MapFX Media");

	cgMedia.particleTable[MFX_CORONA]		= cgi.R_RegisterPoly ("egl/mfx/corona.tga");
	cgMedia.particleTable[MFX_WHITE]		= cgi.R_RegisterPoly ("egl/mfx/white.tga");

	// Decal specific
	CG_IncLoadPercent (CG_FXMEDIA_RANGE*0.25);
	CG_LoadingFilename ("Decals");

	cgMedia.decalTable[DT_BFG_GLOWMARK]			= cgi.R_RegisterPoly ("egl/decals/bfg_glowmark.tga");

	cgMedia.decalTable[DT_BLASTER_BLUEMARK]		= cgi.R_RegisterPoly ("egl/decals/blaster_bluemark.tga");
	cgMedia.decalTable[DT_BLASTER_BURNMARK]		= cgi.R_RegisterPoly ("egl/decals/blaster_burnmark.tga");
	cgMedia.decalTable[DT_BLASTER_GREENMARK]	= cgi.R_RegisterPoly ("egl/decals/blaster_greenmark.tga");
	cgMedia.decalTable[DT_BLASTER_REDMARK]		= cgi.R_RegisterPoly ("egl/decals/blaster_redmark.tga");

	cgMedia.decalTable[DT_DRONE_SPIT_GLOW]		= cgi.R_RegisterPoly ("egl/decals/drone_spit_glow.tga");

	cgMedia.decalTable[DT_ENGYREPAIR_BURNMARK]	= cgi.R_RegisterPoly ("egl/decals/engy_repair_burnmark.tga");
	cgMedia.decalTable[DT_ENGYREPAIR_GLOWMARK]	= cgi.R_RegisterPoly ("egl/decals/engy_repair_glowmark.tga");

	cgMedia.decalTable[DT_BLOOD]				= cgi.R_RegisterPoly ("egl/decals/blood.tga");
	cgMedia.decalTable[DT_BLOOD2]				= cgi.R_RegisterPoly ("egl/decals/blood2.tga");
	cgMedia.decalTable[DT_BLOOD3]				= cgi.R_RegisterPoly ("egl/decals/blood3.tga");
	cgMedia.decalTable[DT_BLOOD4]				= cgi.R_RegisterPoly ("egl/decals/blood4.tga");
	cgMedia.decalTable[DT_BLOOD5]				= cgi.R_RegisterPoly ("egl/decals/blood5.tga");
	cgMedia.decalTable[DT_BLOOD6]				= cgi.R_RegisterPoly ("egl/decals/blood6.tga");

	cgMedia.decalTable[DT_GRNBLOOD]				= cgi.R_RegisterPoly ("egl/decals/blood_grn.tga");
	cgMedia.decalTable[DT_GRNBLOOD2]			= cgi.R_RegisterPoly ("egl/decals/blood_grn2.tga");
	cgMedia.decalTable[DT_GRNBLOOD3]			= cgi.R_RegisterPoly ("egl/decals/blood_grn3.tga");
	cgMedia.decalTable[DT_GRNBLOOD4]			= cgi.R_RegisterPoly ("egl/decals/blood_grn4.tga");
	cgMedia.decalTable[DT_GRNBLOOD5]			= cgi.R_RegisterPoly ("egl/decals/blood_grn5.tga");
	cgMedia.decalTable[DT_GRNBLOOD6]			= cgi.R_RegisterPoly ("egl/decals/blood_grn6.tga");

	cgMedia.decalTable[DT_BULLET]				= cgi.R_RegisterPoly ("egl/decals/bullet.tga");

	cgMedia.decalTable[DT_EXPLOMARK]			= cgi.R_RegisterPoly ("egl/decals/explomark.tga");
	cgMedia.decalTable[DT_EXPLOMARK2]			= cgi.R_RegisterPoly ("egl/decals/explomark2.tga");
	cgMedia.decalTable[DT_EXPLOMARK3]			= cgi.R_RegisterPoly ("egl/decals/explomark3.tga");

	cgMedia.decalTable[DT_RAIL_BURNMARK]		= cgi.R_RegisterPoly ("egl/decals/rail_burnmark.tga");
	cgMedia.decalTable[DT_RAIL_GLOWMARK]		= cgi.R_RegisterPoly ("egl/decals/rail_glowmark.tga");
	cgMedia.decalTable[DT_RAIL_WHITE]			= cgi.R_RegisterPoly ("egl/decals/rail_white.tga");

	cgMedia.decalTable[DT_SLASH]				= cgi.R_RegisterPoly ("egl/decals/slash.tga");
	cgMedia.decalTable[DT_SLASH2]				= cgi.R_RegisterPoly ("egl/decals/slash2.tga");
	cgMedia.decalTable[DT_SLASH3]				= cgi.R_RegisterPoly ("egl/decals/slash3.tga");

	// clear filename
	CG_LoadingFilename (0);
}


/*
================
CG_SoundMediaInit

Called on CGame init and on snd_restart
================
*/
void CG_SoundMediaInit (void)
{
	int		i;
	char	name[MAX_QPATH];

	CG_LoadingString ("Loading sound media...");

	// Generic sounds
	cgMedia.disruptExploSfx			= cgi.Snd_RegisterSound ("weapons/disrupthit.wav");
	cgMedia.grenadeExploSfx			= cgi.Snd_RegisterSound ("weapons/grenlx1a.wav");
	cgMedia.rocketExploSfx			= cgi.Snd_RegisterSound ("weapons/rocklx1a.wav");
	cgMedia.waterExploSfx			= cgi.Snd_RegisterSound ("weapons/xpld_wat.wav");

	cgMedia.gibSfx					= cgi.Snd_RegisterSound ("misc/udeath.wav");
	cgMedia.gibSplatSfx[0]			= cgi.Snd_RegisterSound ("egl/gibimp1.wav");
	cgMedia.gibSplatSfx[1]			= cgi.Snd_RegisterSound ("egl/gibimp2.wav");
	cgMedia.gibSplatSfx[2]			= cgi.Snd_RegisterSound ("egl/gibimp3.wav");

	CG_IncLoadPercent (CG_SOUNDMEDIA_RANGE*0.2);

	cgMedia.itemRespawnSfx			= cgi.Snd_RegisterSound ("items/respawn1.wav");
	cgMedia.laserHitSfx				= cgi.Snd_RegisterSound ("weapons/lashit.wav");
	cgMedia.lightningSfx			= cgi.Snd_RegisterSound ("weapons/tesla.wav");

	cgMedia.playerFallSfx			= cgi.Snd_RegisterSound ("*fall2.wav");
	cgMedia.playerFallShortSfx		= cgi.Snd_RegisterSound ("player/land1.wav");
	cgMedia.playerFallFarSfx		= cgi.Snd_RegisterSound ("*fall1.wav");

	cgMedia.playerTeleport			= cgi.Snd_RegisterSound ("misc/tele1.wav");
	cgMedia.bigTeleport				= cgi.Snd_RegisterSound ("misc/bigtele.wav");

	for (i=0 ; i<7 ; i++) {
		CG_IncLoadPercent (CG_SOUNDMEDIA_RANGE*0.02f);

		Q_snprintfz (name, sizeof (name), "world/spark%i.wav", i+1);
		cgMedia.sparkSfx[i]				= cgi.Snd_RegisterSound (name);

		if (i > 5)
			continue;

		Q_snprintfz (name, sizeof (name), "egl/steps/snow%i.wav", i+1);
		cgMedia.steps.snow[i]			= cgi.Snd_RegisterSound (name);

		if (i > 3)
			continue;

		Q_snprintfz (name, sizeof (name), "player/step%i.wav", i+1);
		cgMedia.steps.standard[i]		= cgi.Snd_RegisterSound (name);


		Q_snprintfz (name, sizeof (name), "egl/steps/concrete%i.wav", i+1);
		cgMedia.steps.concrete[i]		= cgi.Snd_RegisterSound (name);

		Q_snprintfz (name, sizeof (name), "egl/steps/dirt%i.wav", i+1);
		cgMedia.steps.dirt[i]			= cgi.Snd_RegisterSound (name);

		Q_snprintfz (name, sizeof (name), "egl/steps/duct%i.wav", i+1);
		cgMedia.steps.duct[i]			= cgi.Snd_RegisterSound (name);

		Q_snprintfz (name, sizeof (name), "egl/steps/grass%i.wav", i+1);
		cgMedia.steps.grass[i]			= cgi.Snd_RegisterSound (name);

		Q_snprintfz (name, sizeof (name), "egl/steps/gravel%i.wav", i+1);
		cgMedia.steps.gravel[i]			= cgi.Snd_RegisterSound (name);

		Q_snprintfz (name, sizeof (name), "egl/steps/metal%i.wav", i+1);
		cgMedia.steps.metal[i]			= cgi.Snd_RegisterSound (name);

		Q_snprintfz (name, sizeof (name), "egl/steps/metalgrate%i.wav", i+1);
		cgMedia.steps.metalGrate[i]		= cgi.Snd_RegisterSound (name);

		Q_snprintfz (name, sizeof (name), "egl/steps/metalladder%i.wav", i+1);
		cgMedia.steps.metalLadder[i]	= cgi.Snd_RegisterSound (name);

		Q_snprintfz (name, sizeof (name), "egl/steps/mud%i.wav", i+1);
		cgMedia.steps.mud[i]			= cgi.Snd_RegisterSound (name);

		Q_snprintfz (name, sizeof (name), "egl/steps/sand%i.wav", i+1);
		cgMedia.steps.sand[i]			= cgi.Snd_RegisterSound (name);

		Q_snprintfz (name, sizeof (name), "egl/steps/slosh%i.wav", i+1);
		cgMedia.steps.slosh[i]			= cgi.Snd_RegisterSound (name);

		Q_snprintfz (name, sizeof (name), "egl/steps/tile%i.wav", i+1);
		cgMedia.steps.tile[i]			= cgi.Snd_RegisterSound (name);

		Q_snprintfz (name, sizeof (name), "egl/steps/wade%i.wav", i+1);
		cgMedia.steps.wade[i]			= cgi.Snd_RegisterSound (name);

		Q_snprintfz (name, sizeof (name), "egl/steps/wood%i.wav", i+1);
		cgMedia.steps.wood[i]			= cgi.Snd_RegisterSound (name);

		Q_snprintfz (name, sizeof (name), "egl/steps/woodpanel%i.wav", i+1);
		cgMedia.steps.woodPanel[i]		= cgi.Snd_RegisterSound (name);

		if (i > 2)
			continue;

		Q_snprintfz (name, sizeof (name), "world/ric%i.wav", i+1);
		cgMedia.ricochetSfx[i]			= cgi.Snd_RegisterSound (name);
	}

	CG_IncLoadPercent (CG_SOUNDMEDIA_RANGE*0.2);
	CG_LoadingFilename ("Muzzle flashes");

	// Muzzleflash sounds
	cgMedia.mz.bfgFireSfx			= cgi.Snd_RegisterSound ("weapons/bfg__f1y.wav");
	cgMedia.mz.blasterFireSfx		= cgi.Snd_RegisterSound ("weapons/blastf1a.wav");
	cgMedia.mz.etfRifleFireSfx		= cgi.Snd_RegisterSound ("weapons/nail1.wav");
	cgMedia.mz.grenadeFireSfx		= cgi.Snd_RegisterSound ("weapons/grenlf1a.wav");
	cgMedia.mz.grenadeReloadSfx		= cgi.Snd_RegisterSound ("weapons/grenlr1b.wav");
	cgMedia.mz.hyperBlasterFireSfx	= cgi.Snd_RegisterSound ("weapons/hyprbf1a.wav");
	cgMedia.mz.ionRipperFireSfx		= cgi.Snd_RegisterSound ("weapons/rippfire.wav");

	for (i=0 ; i<5 ; i++) {
		Q_snprintfz (name, sizeof (name), "weapons/machgf%ib.wav", i+1);
		cgMedia.mz.machineGunSfx[i]	= cgi.Snd_RegisterSound (name);
	}

	cgMedia.mz.phalanxFireSfx		= cgi.Snd_RegisterSound ("weapons/plasshot.wav");
	cgMedia.mz.railgunFireSfx		= cgi.Snd_RegisterSound ("weapons/railgf1a.wav");
	cgMedia.mz.railgunReloadSfx		= cgi.Snd_RegisterSound ("weapons/railgr1a.wav");
	cgMedia.mz.rocketFireSfx			= cgi.Snd_RegisterSound ("weapons/rocklf1a.wav");
	cgMedia.mz.rocketReloadSfx		= cgi.Snd_RegisterSound ("weapons/rocklr1b.wav");
	cgMedia.mz.shotgunFireSfx		= cgi.Snd_RegisterSound ("weapons/shotgf1b.wav");
	cgMedia.mz.shotgun2FireSfx		= cgi.Snd_RegisterSound ("weapons/shotg2.wav");
	cgMedia.mz.shotgunReloadSfx		= cgi.Snd_RegisterSound ("weapons/shotgr1b.wav");
	cgMedia.mz.superShotgunFireSfx	= cgi.Snd_RegisterSound ("weapons/sshotf1b.wav");
	cgMedia.mz.trackerFireSfx		= cgi.Snd_RegisterSound ("weapons/disint2.wav");

	CG_IncLoadPercent (CG_SOUNDMEDIA_RANGE*0.2);

	// Monster muzzleflash sounds
	cgMedia.mz2.chicRocketSfx		= cgi.Snd_RegisterSound ("chick/chkatck2.wav");
	cgMedia.mz2.floatBlasterSfx		= cgi.Snd_RegisterSound ("floater/fltatck1.wav");
	cgMedia.mz2.flyerBlasterSfx		= cgi.Snd_RegisterSound ("flyer/flyatck3.wav");
	cgMedia.mz2.gunnerGrenadeSfx	= cgi.Snd_RegisterSound ("gunner/gunatck3.wav");
	cgMedia.mz2.gunnerMachGunSfx	= cgi.Snd_RegisterSound ("gunner/gunatck2.wav");
	cgMedia.mz2.hoverBlasterSfx		= cgi.Snd_RegisterSound ("hover/hovatck1.wav");
	cgMedia.mz2.jorgMachGunSfx		= cgi.Snd_RegisterSound ("boss3/xfire.wav");
	cgMedia.mz2.machGunSfx			= cgi.Snd_RegisterSound ("infantry/infatck1.wav");
	cgMedia.mz2.makronBlasterSfx	= cgi.Snd_RegisterSound ("makron/blaster.wav");
	cgMedia.mz2.medicBlasterSfx		= cgi.Snd_RegisterSound ("medic/medatck1.wav");
	cgMedia.mz2.soldierBlasterSfx	= cgi.Snd_RegisterSound ("soldier/solatck2.wav");
	cgMedia.mz2.soldierMachGunSfx	= cgi.Snd_RegisterSound ("soldier/solatck3.wav");
	cgMedia.mz2.soldierShotgunSfx	= cgi.Snd_RegisterSound ("soldier/solatck1.wav");
	cgMedia.mz2.superTankRocketSfx	= cgi.Snd_RegisterSound ("tank/rocket.wav");
	cgMedia.mz2.tankBlasterSfx		= cgi.Snd_RegisterSound ("tank/tnkatck3.wav");

	for (i=0 ; i<5 ; i++) {
		Q_snprintfz (name, sizeof (name), "tank/tnkatk2%c.wav", 'a' + i);
		cgMedia.mz2.tankMachGunSfx[i] = cgi.Snd_RegisterSound (name);
	}

	cgMedia.mz2.tankRocketSfx		= cgi.Snd_RegisterSound ("tank/tnkatck1.wav");

	// Clear filename
	CG_LoadingFilename (0);
}


/*
=================
CG_CacheGloomMedia
=================
*/
void CG_CacheGloomMedia (void)
{
	CG_LoadingString ("Loading Gloom media...");

	//
	// Human classes
	//
	CG_LoadingFilename ("Human classes");
	cgi.R_RegisterModel ("players/engineer/tris.md2");
	cgi.R_RegisterModel ("players/male/tris.md2");
	cgi.R_RegisterModel ("players/female/tris.md2");

	CG_IncLoadPercent (CG_GLOOMMEDIA_RANGE*0.0625);

	cgi.R_RegisterModel ("players/hsold/tris.md2");
	cgi.R_RegisterModel ("players/exterm/tris.md2");
	cgi.R_RegisterModel ("players/mech/tris.md2");

	CG_IncLoadPercent (CG_GLOOMMEDIA_RANGE*0.0625);

	// Human structures
	CG_LoadingFilename ("Human structures");
	cgi.R_RegisterModel ("models/objects/dmspot/tris.md2");
	cgi.R_RegisterModel ("models/turret/base.md2");
	cgi.R_RegisterModel ("models/turret/gun.md2");
	cgi.R_RegisterModel ("models/turret/mgun.md2");

	CG_IncLoadPercent (CG_GLOOMMEDIA_RANGE*0.0625);

	cgi.R_RegisterModel ("models/objects/detector/tris.md2");
	cgi.R_RegisterModel ("models/objects/tripwire/tris.md2");
	cgi.R_RegisterModel ("models/objects/depot/tris.md2");

	CG_IncLoadPercent (CG_GLOOMMEDIA_RANGE*0.0625);

	// Human weapons
	CG_LoadingFilename ("Human weapons");
	cgi.R_RegisterModel ("players/engineer/weapon.md2");
	cgi.R_RegisterModel ("players/male/autogun.md2");
	cgi.R_RegisterModel ("players/male/shotgun.md2");
	cgi.R_RegisterModel ("players/male/smg.md2");
	cgi.R_RegisterModel ("players/male/weapon.md2");

	CG_IncLoadPercent (CG_GLOOMMEDIA_RANGE*0.0625);

	cgi.R_RegisterModel ("players/female/weapon.md2");
	cgi.R_RegisterModel ("players/hsold/weapon.md2");
	cgi.R_RegisterModel ("players/exterm/weapon.md2");
 	cgi.R_RegisterModel ("players/mech/weapon.md2");

	CG_IncLoadPercent (CG_GLOOMMEDIA_RANGE*0.0625);

	// Human view weapons
	CG_LoadingFilename ("Human view weapons");
	cgi.R_RegisterModel ("models/weapons/v_auto/tris.md2");
	cgi.R_RegisterModel ("models/weapons/v_shot/tris.md2");
	cgi.R_RegisterModel ("models/weapons/v_spas/tris.md2");
	cgi.R_RegisterModel ("models/weapons/v_launch/tris.md2");

	CG_IncLoadPercent (CG_GLOOMMEDIA_RANGE*0.0625);

	cgi.R_RegisterModel ("models/weapons/v_pist/tris.md2");
 	cgi.R_RegisterModel ("models/weapons/v_sub/tris.md2");
	cgi.R_RegisterModel ("models/weapons/v_mag/tris.md2");
	cgi.R_RegisterModel ("models/weapons/v_plas/tris.md2");
	cgi.R_RegisterModel ("models/weapons/v_mech/tris.md2");

	CG_IncLoadPercent (CG_GLOOMMEDIA_RANGE*0.0625);

	// Human items
	CG_LoadingFilename ("Human Items");
	cgi.R_RegisterModel ("models/objects/c4/tris.md2");
	cgi.R_RegisterModel ("models/objects/r_explode/tris.md2");
	cgi.R_RegisterModel ("models/objects/explode/tris.md2");
	cgi.R_RegisterModel ("models/objects/ggrenade/tris.md2");
	cgi.R_RegisterModel ("models/objects/laser/tris.md2");
	cgi.R_RegisterModel ("models/objects/tlaser/tris.md2");

	CG_IncLoadPercent (CG_GLOOMMEDIA_RANGE*0.0625);

	cgi.R_RegisterModel ("models/objects/c4/tris.md2");
	cgi.R_RegisterModel ("models/objects/grenade/tris.md2");
	cgi.R_RegisterModel ("models/objects/debris1/tris.md2");
	cgi.R_RegisterModel ("models/objects/debris2/tris.md2");

	CG_IncLoadPercent (CG_GLOOMMEDIA_RANGE*0.0625);

	//
	// Alien classes
	//
	CG_LoadingFilename ("Alien classes");
	cgi.R_RegisterModel ("players/breeder/tris.md2");
	cgi.R_RegisterModel ("players/breeder/weapon.md2");
	cgi.R_RegisterModel ("players/hatch/tris.md2");
	cgi.R_RegisterModel ("players/hatch/weapon.md2");
	cgi.R_RegisterModel ("players/drone/tris.md2");
	cgi.R_RegisterModel ("players/drone/weapon.md2");
	cgi.R_RegisterModel ("players/wraith/tris.md2");

	CG_IncLoadPercent (CG_GLOOMMEDIA_RANGE*0.0625);

	cgi.R_RegisterModel ("players/wraith/weapon.md2");
	cgi.R_RegisterModel ("players/stinger/tris.md2");
	cgi.R_RegisterModel ("players/stinger/weapon.md2");
	cgi.R_RegisterModel ("players/guardian/tris.md2");
	cgi.R_RegisterModel ("players/guardian/weapon.md2");
	cgi.R_RegisterModel ("players/stalker/tris.md2");
	cgi.R_RegisterModel ("players/stalker/weapon.md2");

	CG_IncLoadPercent (CG_GLOOMMEDIA_RANGE*0.0625);

	// Alien structures
	CG_LoadingFilename ("Alien structures");
	cgi.R_RegisterModel ("models/objects/cocoon/tris.md2");
	cgi.R_RegisterModel ("models/objects/organ/spiker/tris.md2");
	cgi.R_RegisterModel ("models/objects/organ/healer/tris.md2");
	cgi.R_RegisterModel ("models/objects/organ/obstacle/tris.md2");
	cgi.R_RegisterModel ("models/objects/organ/gas/tris.md2");

	CG_IncLoadPercent (CG_GLOOMMEDIA_RANGE*0.0625);

	cgi.R_RegisterModel ("models/objects/spike/tris.md2");
	cgi.R_RegisterModel ("models/objects/spore/tris.md2");
	cgi.R_RegisterModel ("models/objects/smokexp/tris.md2");
	cgi.R_RegisterModel ("models/objects/web/ball.md2");

	CG_IncLoadPercent (CG_GLOOMMEDIA_RANGE*0.0625);

	// Alien objects
	CG_LoadingFilename ("Alien objects");
	cgi.R_RegisterModel ("models/gibs/hatchling/leg/tris.md2");
	cgi.R_RegisterModel ("models/gibs/guardian/gib2.md2");
	cgi.R_RegisterModel ("models/gibs/guardian/gib1.md2");
	cgi.R_RegisterModel ("models/gibs/stalker/gib1.md2");

	CG_IncLoadPercent (CG_GLOOMMEDIA_RANGE*0.0625);

	cgi.R_RegisterModel ("models/gibs/stalker/gib2.md2");
	cgi.R_RegisterModel ("models/gibs/stalker/gib3.md2");
	cgi.R_RegisterModel ("models/objects/sspore/tris.md2");

	CG_LoadingFilename (0);
}


/*
=================
CG_GloomMediaInit
=================
*/
void CG_GloomMediaInit (void)
{
	if (cgs.currGameMod != GAME_MOD_GLOOM)
		return;
	if (!glm_forcecache->integer)
		return;

	CG_CacheGloomMedia ();
}

/*
=============================================================================

	MEDIA INITIALIZATION

=============================================================================
*/

/*
================
CG_MediaInit

Called before all the cgame is initialized
================
*/
void CG_MediaInit (void)
{
	if (cgMedia.initialized)
		return;

	CG_InitBaseMedia ();

	CG_LoadingPercent (0);
	CG_LoadingString (0);
	CG_LoadingFilename (0);

	cgi.R_UpdateScreen ();

	CG_MapMediaInit ();
		CG_LoadingPercent (CG_MAPMEDIA_PERCENT);
	CG_ModelMediaInit ();
		CG_LoadingPercent (CG_MODELMEDIA_PERCENT);
	CG_PicMediaInit ();
		CG_LoadingPercent (CG_PICMEDIA_PERCENT);
	CG_FXMediaInit ();
		CG_LoadingPercent (CG_FXMEDIA_PERCENT);
	CG_SoundMediaInit ();
		CG_LoadingPercent (CG_SOUNDMEDIA_PERCENT);
	CG_GloomMediaInit ();
		CG_LoadingPercent (CG_GLOOMMEDIA_PERCENT);

	CG_LoadingPercent (100);
	CG_LoadingString (0);
	CG_LoadingFilename (0);

	cgMedia.initialized = qTrue;
}


/*
================
CG_ShutdownMedia
================
*/
void CG_ShutdownMedia (void)
{
	if (!cgMedia.initialized)
		return;

	cgMedia.initialized = qFalse;
	cgMedia.baseInitialized = qFalse;
}
