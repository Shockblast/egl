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

static byte cgColorPal[] = {
	#include "colorpal.h"
};
float	palRed		(int index) { return (cgColorPal[index*3+0]); }
float	palGreen	(int index) { return (cgColorPal[index*3+1]); }
float	palBlue		(int index) { return (cgColorPal[index*3+2]); }

float	palRedf		(int index) { return ((cgColorPal[index*3+0] > 0) ? cgColorPal[index*3+0] * ONEDIV255 : 0); }
float	palGreenf	(int index) { return ((cgColorPal[index*3+1] > 0) ? cgColorPal[index*3+1] * ONEDIV255 : 0); }
float	palBluef	(int index) { return ((cgColorPal[index*3+2] > 0) ? cgColorPal[index*3+2] * ONEDIV255 : 0); }

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
CG_MapMediaInit
================
*/
void CG_MapMediaInit (void) {
	char		mapName[64];
	char		name[MAX_QPATH];
	int			i;
	float		rotate, pctInc;
	vec3_t		axis;

	if (!cg.configStrings[CS_MODELS+1][0])
		return;

	// let the render dll load the map
	Q_strncpyz (mapName, cg.configStrings[CS_MODELS+1] + 5, sizeof (mapName));	// skip "maps/"
	mapName[Q_strlen (mapName) - 4] = 0;										// cut off ".bsp"

	// register models, pics, and skins
	CG_LoadingString ("Loading map...");
	cgi.Mod_RegisterMap (mapName);

	CG_IncLoadPercent (CG_MAPMEDIA_RANGE*0.1);

	// register map effects
	CG_LoadingString ("Loading map fx...");
	CG_MapFXInit (mapName);

	CG_IncLoadPercent (CG_MAPMEDIA_RANGE*0.05);

	// load models
	CG_LoadingString ("Loading models...");
	cgNumWeaponModels = 1;
	Q_strncpyz (cgWeaponModels[0], "weapon.md2", sizeof (cgWeaponModels[0]));
	for (i=1 ; i<MAX_CS_MODELS && cg.configStrings[CS_MODELS+i][0] ; i++) ;
	pctInc = 1.0/(float)i;

	for (i=1 ; i<MAX_CS_MODELS && cg.configStrings[CS_MODELS+i][0] ; i++) {
		Q_strncpyz (name, cg.configStrings[CS_MODELS+i], sizeof (name));
		name[37] = 0;	// never go beyond one line
		if (name[0] != '*')
			CG_LoadingFilename (name);

		cgi.R_UpdateScreen ();
		cgi.Sys_SendKeyEvents ();	// pump message loop
		if (name[0] == '#') {
			// special player weapon model
			if (cgNumWeaponModels < MAX_CLIENTWEAPONMODELS) {
				Q_strncpyz (cgWeaponModels[cgNumWeaponModels], cg.configStrings[CS_MODELS+i]+1,
					sizeof (cgWeaponModels[cgNumWeaponModels]));
				cgNumWeaponModels++;
			}
		}
		else {
			cg.modelDraw[i] = cgi.Mod_RegisterModel (cg.configStrings[CS_MODELS+i]);
			if (name[0] == '*')
				cg.modelClip[i] = CGI_CM_InlineModel (cg.configStrings[CS_MODELS+i]);
			else
				cg.modelClip[i] = NULL;
		}


		CG_IncLoadPercent (pctInc*(CG_MAPMEDIA_RANGE*0.35));

		if (name[0] != '*')
			CG_LoadingFilename (0);
	}

	// images
	CG_LoadingString ("Loading images...");
	for (i=1 ; i<MAX_CS_IMAGES && cg.configStrings[CS_IMAGES+i][0] ; i++) {
		CG_LoadingFilename (cg.configStrings[CS_IMAGES+i]);
		CG_RegisterPic (cg.configStrings[CS_IMAGES+i]);
		cgi.Sys_SendKeyEvents ();	// pump message loop
	}

	CG_LoadingFilename (0);
	CG_IncLoadPercent (CG_MAPMEDIA_RANGE*0.2);

	// clients
	CG_LoadingString ("Loading clients...");
	for (i=0 ; i<MAX_CS_CLIENTS ; i++) {
		if (!cg.configStrings[CS_PLAYERSKINS+i][0])
			continue;

		CG_LoadingFilename (Q_VarArgs ("Client #%i", i));
		cgi.Sys_SendKeyEvents ();	// pump message loop
		CG_ParseClientinfo (i);
	}

	CG_IncLoadPercent (CG_MAPMEDIA_RANGE*0.15);

	CG_LoadingFilename ("Base client info");
	CG_LoadClientinfo (&cg.baseClientInfo, "unnamed\\male/grunt");
	CG_LoadingFilename (0);

	CG_IncLoadPercent (CG_MAPMEDIA_RANGE*0.05);

	// set sky textures and speed
	CG_LoadingString ("Loading sky env...");
	rotate = atof (cg.configStrings[CS_SKYROTATE]);
	sscanf (cg.configStrings[CS_SKYAXIS], "%f %f %f", &axis[0], &axis[1], &axis[2]);
	cgi.R_SetSky (cg.configStrings[CS_SKY], rotate, axis);
}


/*
================
CG_ModelMediaInit
================
*/
void CG_ModelMediaInit (void) {
	CG_LoadingString ("Loading model media...");

	cgMedia.parasiteSegmentMOD	= cgi.Mod_RegisterModel ("models/monsters/parasite/segment/tris.md2");
	cgMedia.grappleCableMOD		= cgi.Mod_RegisterModel ("models/ctf/segment/tris.md2");

	CG_IncLoadPercent (CG_MODELMEDIA_RANGE*0.5);

	cgMedia.lightningMOD		= cgi.Mod_RegisterModel ("models/proj/lightning/tris.md2");
	cgMedia.heatBeamMOD			= cgi.Mod_RegisterModel ("models/proj/beam/tris.md2");
	cgMedia.monsterHeatBeamMOD	= cgi.Mod_RegisterModel ("models/proj/widowbeam/tris.md2");
}


/*
================
CG_CrosshairImageInit
================
*/
void CG_CrosshairImageInit (void) {
	crosshair->modified = qFalse;
	if (crosshair->integer) {
		crosshair->integer = (crosshair->integer < 0) ? 0 : crosshair->integer;

		cgMedia.crosshairImage = cgi.Img_RegisterImage (Q_VarArgs ("pics/ch%d.tga", crosshair->integer), IF_NOMIPMAP);
	}
}


/*
================
CG_PicMediaInit
================
*/
void CG_PicMediaInit (void) {
	int		i, j;
	static char	*sb_nums[2][11] = {
		{"num_0",  "num_1",  "num_2",  "num_3",  "num_4",  "num_5",  "num_6",  "num_7",  "num_8",  "num_9",  "num_minus"},
		{"anum_0", "anum_1", "anum_2", "anum_3", "anum_4", "anum_5", "anum_6", "anum_7", "anum_8", "anum_9", "anum_minus"}
	};

	CG_LoadingString ("Loading image media...");

	CG_LoadingFilename ("Crosshair");
	CG_CrosshairImageInit ();

	CG_IncLoadPercent (CG_PICMEDIA_RANGE*0.25);

	CG_LoadingFilename ("Pics");
	cgMedia.tileBackImage				= cgi.Img_RegisterImage ("pics/backtile.tga",		IF_NOMIPMAP);
	cgMedia.hudFieldImage				= cgi.Img_RegisterImage ("pics/field_3.tga",		IF_NOMIPMAP);
	cgMedia.hudInventoryImage			= cgi.Img_RegisterImage ("pics/inventory.tga",		IF_NOMIPMAP);
	cgMedia.hudNetImage					= cgi.Img_RegisterImage ("pics/net.tga",			IF_NOMIPMAP);
	for (i=0 ; i<2 ; i++)
		for (j=0 ; j<11 ; j++)
			cgMedia.hudNumImages[i][j]	= cgi.Img_RegisterImage (Q_VarArgs ("pics/%s.tga", sb_nums[i][j]), IF_NOMIPMAP);
	cgMedia.hudPausedImage				= cgi.Img_RegisterImage ("pics/pause.tga",		IF_NOMIPMAP);

	CG_IncLoadPercent (CG_PICMEDIA_RANGE*0.5);

	cgi.Img_RegisterImage ("pics/w_machinegun.tga",		IF_NOMIPMAP);
	cgi.Img_RegisterImage ("pics/a_bullets.tga",		IF_NOMIPMAP);
	cgi.Img_RegisterImage ("pics/i_health.tga",			IF_NOMIPMAP);
	cgi.Img_RegisterImage ("pics/a_grenades.tga",		IF_NOMIPMAP);

	CG_LoadingFilename (0);
}


/*
================
CG_FXMediaInit
================
*/
#define PD_DEFFLAGS (IF_NOGAMMA|IF_NOINTENS|IF_CLAMP)
void CG_FXMediaInit (void) {
	int		i;

	// Particles / Decals
	CG_LoadingString ("Loading effect media...");
	CG_LoadingFilename ("Particles");

	for (i=0 ; i<(NUMVERTEXNORMALS * 3) ; i++)
		avelocities[0][i] = (frand () * 255) * 0.01;

	cgMedia.mediaTable[PT_BLUE]			= cgi.Img_RegisterImage ("egl/parts/blue.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_GREEN]		= cgi.Img_RegisterImage ("egl/parts/green.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_RED]			= cgi.Img_RegisterImage ("egl/parts/red.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_WHITE]		= cgi.Img_RegisterImage ("egl/parts/white.tga",			PD_DEFFLAGS);

	cgMedia.mediaTable[PT_SMOKE]		= cgi.Img_RegisterImage ("egl/parts/smoke.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_SMOKE2]		= cgi.Img_RegisterImage ("egl/parts/smoke2.tga",		PD_DEFFLAGS);

	cgMedia.mediaTable[PT_BLUEFIRE]		= cgi.Img_RegisterImage ("egl/parts/bluefire.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_FIRE1]		= cgi.Img_RegisterImage ("egl/parts/fire1.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_FIRE2]		= cgi.Img_RegisterImage ("egl/parts/fire2.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_FIRE3]		= cgi.Img_RegisterImage ("egl/parts/fire3.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_FIRE4]		= cgi.Img_RegisterImage ("egl/parts/fire4.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EMBERS1]		= cgi.Img_RegisterImage ("egl/parts/embers1.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EMBERS2]		= cgi.Img_RegisterImage ("egl/parts/embers2.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EMBERS3]		= cgi.Img_RegisterImage ("egl/parts/embers3.tga",		PD_DEFFLAGS);

	CG_IncLoadPercent (CG_FXMEDIA_RANGE*0.25);

	cgMedia.mediaTable[PT_BEAM]			= cgi.Img_RegisterImage ("egl/parts/beam.tga",			PD_DEFFLAGS|IF_NOMIPMAP);
	cgMedia.mediaTable[PT_BLDDRIP]		= cgi.Img_RegisterImage ("egl/parts/blooddrip.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_BLDSPURT]		= cgi.Img_RegisterImage ("egl/parts/bloodspurt.tga",	PD_DEFFLAGS);
	cgMedia.mediaTable[PT_BUBBLE]		= cgi.Img_RegisterImage ("egl/parts/bubble.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_DRIP]			= cgi.Img_RegisterImage ("egl/parts/drip.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EXPLOFLASH]	= cgi.Img_RegisterImage ("egl/parts/exploflash.tga",	PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EXPLOWAVE]	= cgi.Img_RegisterImage ("egl/parts/explowave.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_FLARE]		= cgi.Img_RegisterImage ("egl/parts/flare.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_FLY]			= cgi.Img_RegisterImage ("egl/parts/fly.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_MIST]			= cgi.Img_RegisterImage ("egl/parts/mist.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_SPARK]		= cgi.Img_RegisterImage ("egl/parts/spark.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_SPLASH]		= cgi.Img_RegisterImage ("egl/parts/splash.tga",		PD_DEFFLAGS);

	// Animated explosions
	CG_IncLoadPercent (CG_FXMEDIA_RANGE*0.25);
	CG_LoadingFilename ("Explosions");

	cgMedia.mediaTable[PT_EXPLO1]		= cgi.Img_RegisterImage ("egl/parts/explo1.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EXPLO2]		= cgi.Img_RegisterImage ("egl/parts/explo2.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EXPLO3]		= cgi.Img_RegisterImage ("egl/parts/explo3.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EXPLO4]		= cgi.Img_RegisterImage ("egl/parts/explo4.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EXPLO5]		= cgi.Img_RegisterImage ("egl/parts/explo5.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EXPLO6]		= cgi.Img_RegisterImage ("egl/parts/explo6.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EXPLO7]		= cgi.Img_RegisterImage ("egl/parts/explo7.tga",		PD_DEFFLAGS);

	cgMedia.mediaTable[PT_EXPLOEMBERS1]	= cgi.Img_RegisterImage ("egl/parts/exploembers.tga",	PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EXPLOEMBERS2]	= cgi.Img_RegisterImage ("egl/parts/exploembers2.tga",	PD_DEFFLAGS);

	// Decal specific
	CG_IncLoadPercent (CG_FXMEDIA_RANGE*0.25);
	CG_LoadingFilename ("Decals");

	cgMedia.mediaTable[DT_BLOOD]		= cgi.Img_RegisterImage ("egl/decals/blood.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[DT_BLOOD2]		= cgi.Img_RegisterImage ("egl/decals/blood2.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[DT_BLOOD3]		= cgi.Img_RegisterImage ("egl/decals/blood3.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[DT_BLOOD4]		= cgi.Img_RegisterImage ("egl/decals/blood4.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[DT_BLOOD5]		= cgi.Img_RegisterImage ("egl/decals/blood5.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[DT_BLOOD6]		= cgi.Img_RegisterImage ("egl/decals/blood6.tga",		PD_DEFFLAGS);

	cgMedia.mediaTable[DT_BULLET]		= cgi.Img_RegisterImage ("egl/decals/bullet.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[DT_BURNMARK]		= cgi.Img_RegisterImage ("egl/decals/burnmark.tga",		PD_DEFFLAGS);

	cgMedia.mediaTable[DT_EXPLOMARK]	= cgi.Img_RegisterImage ("egl/decals/explomark.tga",	PD_DEFFLAGS);
	cgMedia.mediaTable[DT_EXPLOMARK2]	= cgi.Img_RegisterImage ("egl/decals/explomark2.tga",	PD_DEFFLAGS);
	cgMedia.mediaTable[DT_EXPLOMARK3]	= cgi.Img_RegisterImage ("egl/decals/explomark3.tga",	PD_DEFFLAGS);

	cgMedia.mediaTable[DT_SLASH]		= cgi.Img_RegisterImage ("egl/decals/slash.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[DT_SLASH2]		= cgi.Img_RegisterImage ("egl/decals/slash2.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[DT_SLASH3]		= cgi.Img_RegisterImage ("egl/decals/slash3.tga",		PD_DEFFLAGS);

	// mapfx media
	CG_LoadingFilename ("MapFX Media");

	cgMedia.mediaTable[MFX_CORONA]		= cgi.Img_RegisterImage ("egl/mfx/corona.tga",			PD_DEFFLAGS);

	// clear filename
	CG_LoadingFilename (0);
}


/*
================
CG_SoundMediaInit

Called on CGame init and on snd_restart
================
*/
void CG_SoundMediaInit (void) {
	int		i;
	char	name[MAX_QPATH];

	CG_LoadingString ("Loading sound media...");

	// generic sounds
	cgMedia.disruptExploSfx			= cgi.Snd_RegisterSound ("weapons/disrupthit.wav");
	cgMedia.grenadeExploSfx			= cgi.Snd_RegisterSound ("weapons/grenlx1a.wav");
	cgMedia.rocketExploSfx			= cgi.Snd_RegisterSound ("weapons/rocklx1a.wav");
	cgMedia.waterExploSfx			= cgi.Snd_RegisterSound ("weapons/xpld_wat.wav");

	cgMedia.gibSfx					= cgi.Snd_RegisterSound ("misc/udeath.wav");
	cgMedia.gibSplatSfx[0]			= cgi.Snd_RegisterSound ("egl/gibimp1.wav");
	cgMedia.gibSplatSfx[1]			= cgi.Snd_RegisterSound ("egl/gibimp2.wav");
	cgMedia.gibSplatSfx[2]			= cgi.Snd_RegisterSound ("egl/gibimp3.wav");

	cgMedia.itemRespawnSfx			= cgi.Snd_RegisterSound ("items/respawn1.wav");
	cgMedia.laserHitSfx				= cgi.Snd_RegisterSound ("weapons/lashit.wav");
	cgMedia.lightningSfx			= cgi.Snd_RegisterSound ("weapons/tesla.wav");

	cgMedia.playerFallSfx			= cgi.Snd_RegisterSound ("*fall2.wav");
	cgMedia.playerFallShortSfx		= cgi.Snd_RegisterSound ("player/land1.wav");
	cgMedia.playerFallFarSfx		= cgi.Snd_RegisterSound ("*fall1.wav");

	cgMedia.playerTeleport			= cgi.Snd_RegisterSound ("misc/tele1.wav");
	cgMedia.bigTeleport				= cgi.Snd_RegisterSound ("misc/bigtele.wav");

	CG_IncLoadPercent (CG_SOUNDMEDIA_RANGE*0.25);

	for (i=0 ; i<7 ; i++) {
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

	CG_IncLoadPercent (CG_SOUNDMEDIA_RANGE*0.25);
	CG_LoadingFilename ("Muzzle flashes");

	// muzzleflash sounds
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

	CG_IncLoadPercent (CG_SOUNDMEDIA_RANGE*0.25);

	// monster muzzleflash sounds
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

	// clear filename
	CG_LoadingFilename (0);
}


/*
=================
CG_CacheGloomMedia
=================
*/
void CG_CacheGloomMedia (void) {
	CG_LoadingString ("Loading Gloom media...");

	//
	// human classes
	//
	CG_LoadingFilename ("Human classes");
	cgi.Mod_RegisterModel ("players/engineer/tris.md2");
	cgi.Mod_RegisterModel ("players/male/tris.md2");
	cgi.Mod_RegisterModel ("players/female/tris.md2");
	cgi.Mod_RegisterModel ("players/hsold/tris.md2");
	cgi.Mod_RegisterModel ("players/exterm/tris.md2");
	cgi.Mod_RegisterModel ("players/mech/tris.md2");

	CG_IncLoadPercent (CG_GLOOMMEDIA_RANGE*0.125);

	// human structures
	CG_LoadingFilename ("Human structures");
	cgi.Mod_RegisterModel ("models/objects/dmspot/tris.md2");
	cgi.Mod_RegisterModel ("models/turret/base.md2");
	cgi.Mod_RegisterModel ("models/turret/gun.md2");
	cgi.Mod_RegisterModel ("models/turret/mgun.md2");
	cgi.Mod_RegisterModel ("models/objects/detector/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/tripwire/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/depot/tris.md2");

	CG_IncLoadPercent (CG_GLOOMMEDIA_RANGE*0.125);

	// human weapons
	CG_LoadingFilename ("Human weapons");
	cgi.Mod_RegisterModel ("players/engineer/weapon.md2");
	cgi.Mod_RegisterModel ("players/male/autogun.md2");
	cgi.Mod_RegisterModel ("players/male/shotgun.md2");
	cgi.Mod_RegisterModel ("players/male/smg.md2");
	cgi.Mod_RegisterModel ("players/male/weapon.md2");
	cgi.Mod_RegisterModel ("players/female/weapon.md2");
	cgi.Mod_RegisterModel ("players/hsold/weapon.md2");
	cgi.Mod_RegisterModel ("players/exterm/weapon.md2");
 	cgi.Mod_RegisterModel ("players/mech/weapon.md2");

	CG_IncLoadPercent (CG_GLOOMMEDIA_RANGE*0.125);

	// human view weapons
	CG_LoadingFilename ("Human view weapons");
	cgi.Mod_RegisterModel ("models/weapons/v_auto/tris.md2");
	cgi.Mod_RegisterModel ("models/weapons/v_shot/tris.md2");
	cgi.Mod_RegisterModel ("models/weapons/v_spas/tris.md2");
	cgi.Mod_RegisterModel ("models/weapons/v_launch/tris.md2");
	cgi.Mod_RegisterModel ("models/weapons/v_pist/tris.md2");
 	cgi.Mod_RegisterModel ("models/weapons/v_sub/tris.md2");
	cgi.Mod_RegisterModel ("models/weapons/v_mag/tris.md2");
	cgi.Mod_RegisterModel ("models/weapons/v_plas/tris.md2");
	cgi.Mod_RegisterModel ("models/weapons/v_mech/tris.md2");

	CG_IncLoadPercent (CG_GLOOMMEDIA_RANGE*0.125);

	// human items
	CG_LoadingFilename ("Human Items");
	cgi.Mod_RegisterModel ("models/objects/c4/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/r_explode/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/explode/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/ggrenade/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/laser/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/tlaser/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/c4/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/grenade/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/debris1/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/debris2/tris.md2");

	CG_IncLoadPercent (CG_GLOOMMEDIA_RANGE*0.125);

	//
	// alien classes
	//
	CG_LoadingFilename ("Alien classes");
	cgi.Mod_RegisterModel ("players/breeder/tris.md2");
	cgi.Mod_RegisterModel ("players/breeder/weapon.md2");
	cgi.Mod_RegisterModel ("players/hatch/tris.md2");
	cgi.Mod_RegisterModel ("players/hatch/weapon.md2");
	cgi.Mod_RegisterModel ("players/drone/tris.md2");
	cgi.Mod_RegisterModel ("players/drone/weapon.md2");
	cgi.Mod_RegisterModel ("players/wraith/tris.md2");
	cgi.Mod_RegisterModel ("players/wraith/weapon.md2");
	cgi.Mod_RegisterModel ("players/stinger/tris.md2");
	cgi.Mod_RegisterModel ("players/stinger/weapon.md2");
	cgi.Mod_RegisterModel ("players/guardian/tris.md2");
	cgi.Mod_RegisterModel ("players/guardian/weapon.md2");
	cgi.Mod_RegisterModel ("players/stalker/tris.md2");
	cgi.Mod_RegisterModel ("players/stalker/weapon.md2");

	CG_IncLoadPercent (CG_GLOOMMEDIA_RANGE*0.125);

	// alien structures
	CG_LoadingFilename ("Alien structures");
	cgi.Mod_RegisterModel ("models/objects/cocoon/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/organ/spiker/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/organ/healer/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/organ/obstacle/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/organ/gas/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/spike/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/spore/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/smokexp/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/web/ball.md2");

	CG_IncLoadPercent (CG_GLOOMMEDIA_RANGE*0.125);

	// alien objects
	CG_LoadingFilename ("Alien objects");
	cgi.Mod_RegisterModel ("models/gibs/hatchling/leg/tris.md2");
	cgi.Mod_RegisterModel ("models/gibs/guardian/gib2.md2");
	cgi.Mod_RegisterModel ("models/gibs/guardian/gib1.md2");
	cgi.Mod_RegisterModel ("models/gibs/stalker/gib1.md2");
	cgi.Mod_RegisterModel ("models/gibs/stalker/gib2.md2");
	cgi.Mod_RegisterModel ("models/gibs/stalker/gib3.md2");
	cgi.Mod_RegisterModel ("models/objects/sspore/tris.md2");

	CG_LoadingFilename (0);
}


/*
=================
CG_GloomMediaInit
=================
*/
void CG_GloomMediaInit (void) {
	if (!cgi.FS_CurrGame ("gloom"))
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
void CG_MediaInit (void) {
	if (cgMedia.initialized)
		return;

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
void CG_ShutdownMedia (void) {
	if (!cgMedia.initialized)
		return;

	cgMedia.initialized = qFalse;
}
