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
==============================================================

	PALETTE COLORING

	Fix by removing the need for these, by using actual RGB
	colors and ditch the stupid shit palette
==============================================================
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
==============================================================

	MEDIA INITIALIZATION

==============================================================
*/

static qBool	mediaLoading;

/*
=================
CG_PrintLoading

Prints message then forces a screen update
=================
*/
static void CG_PrintLoading (char *fmt, ...) {
	va_list		argptr;
	char		text[1024];

	if (!mediaLoading)
		return;

	va_start (argptr, fmt);
	vsnprintf (text, sizeof (text), fmt, argptr);
	va_end (argptr);

	Com_Printf (0, "%s\r", text);
	cgi.R_UpdateScreen ();
}


/*
=================
CG_ClearLoading

Clears the current print message, with optional frame updating
=================
*/
static void CG_ClearLoading (qBool updateFrame) {
	Com_Printf (0, " \r");

	if (updateFrame)
		cgi.R_UpdateScreen ();
}


/*
================
CG_InitMapMedia
================
*/
void CG_InitMapMedia (void) {
	char		mapName[64];
	char		name[MAX_QPATH];
	int			i;
	float		rotate;
	vec3_t		axis;

	if (!cg.configStrings[CS_MODELS+1][0])
		Com_Error (ERR_FATAL, "CG_InitMapMedia: null map name!");	// no map loaded

	// let the render dll load the map
	strcpy (mapName, cg.configStrings[CS_MODELS+1] + 5);	// skip "maps/"
	mapName[strlen (mapName) - 4] = 0;						// cut off ".bsp"

	// register models, pics, and skins
	CG_PrintLoading ("Map: %s", mapName);
	cgi.Mod_RegisterMap (mapName);

	// register map effects
	CG_PrintLoading ("map fx");
	CG_InitMapFX (mapName);

	// load models
	cgNumWeaponModels = 1;
	strcpy (cgWeaponModels[0], "weapon.md2");
	for (i=1 ; i<MAX_CS_MODELS && cg.configStrings[CS_MODELS+i][0] ; i++) {
		strcpy (name, cg.configStrings[CS_MODELS+i]);
		name[37] = 0;	// never go beyond one line
		if (name[0] != '*')
			CG_PrintLoading ("%s", name);

		cgi.R_UpdateScreen ();
		cgi.Sys_SendKeyEvents ();	// pump message loop
		if (name[0] == '#') {
			// special player weapon model
			if (cgNumWeaponModels < MAX_CLIENTWEAPONMODELS) {
				strncpy (cgWeaponModels[cgNumWeaponModels], cg.configStrings[CS_MODELS+i]+1,
					sizeof (cgWeaponModels[cgNumWeaponModels]) - 1);
				cgNumWeaponModels++;
			}
		} else {
			cg.modelDraw[i] = cgi.Mod_RegisterModel (cg.configStrings[CS_MODELS+i]);
			if (name[0] == '*')
				cg.modelClip[i] = cgi.CM_InlineModel (cg.configStrings[CS_MODELS+i]);
			else
				cg.modelClip[i] = NULL;
		}

		if (name[0] != '*')
			CG_ClearLoading (qFalse);
	}

	CG_PrintLoading ("images");
	for (i=1 ; i<MAX_CS_IMAGES && cg.configStrings[CS_IMAGES+i][0] ; i++) {
		cgi.Img_RegisterPic (cg.configStrings[CS_IMAGES+i]);
		cgi.Sys_SendKeyEvents ();	// pump message loop
	}

	CG_PrintLoading ("clients");
	for (i=0 ; i<MAX_CS_CLIENTS ; i++) {
		if (!cg.configStrings[CS_PLAYERSKINS+i][0])
			continue;

		CG_PrintLoading ("client %i", i);
		cgi.R_UpdateScreen ();
		cgi.Sys_SendKeyEvents ();	// pump message loop
		CG_ParseClientinfo (i);
	}

	CG_PrintLoading ("base client info");
	CG_LoadClientinfo (&cg.baseClientInfo, "unnamed\\male/grunt");

	// set sky textures and speed
	CG_PrintLoading ("sky");
	rotate = atof (cg.configStrings[CS_SKYROTATE]);
	sscanf (cg.configStrings[CS_SKYAXIS], "%f %f %f", &axis[0], &axis[1], &axis[2]);
	cgi.R_SetSky (cg.configStrings[CS_SKY], rotate, axis);
}


/*
================
CG_InitModelMedia
================
*/
void CG_InitModelMedia (void) {
	CG_PrintLoading ("model media");

	cgMedia.parasiteSegmentMOD	= cgi.Mod_RegisterModel ("models/monsters/parasite/segment/tris.md2");
	cgMedia.grappleCableMOD		= cgi.Mod_RegisterModel ("models/ctf/segment/tris.md2");

	cgMedia.lightningMOD		= cgi.Mod_RegisterModel ("models/proj/lightning/tris.md2");
	cgMedia.heatBeamMOD			= cgi.Mod_RegisterModel ("models/proj/beam/tris.md2");
	cgMedia.monsterHeatBeamMOD	= cgi.Mod_RegisterModel ("models/proj/widowbeam/tris.md2");
}


/*
================
CL_InitCrosshairImage
================
*/
void CL_InitCrosshairImage (void) {
	char	chPic[MAX_QPATH];

	CG_PrintLoading ("crosshair");

	crosshair->modified = qFalse;
	if (crosshair->integer) {
		crosshair->integer = (crosshair->integer < 0) ? 0 : crosshair->integer;

		Q_snprintfz (chPic, sizeof (chPic), "ch%d", crosshair->integer);
		cgMedia.crosshairImage = cgi.Img_RegisterPic (chPic);
	}
}


/*
================
CG_InitPicMedia
================
*/
void CG_InitPicMedia (void) {
	int		i, j;
	static char	*sb_nums[2][11] = {
		{"num_0",  "num_1",  "num_2",  "num_3",  "num_4",  "num_5",  "num_6",  "num_7",  "num_8",  "num_9",  "num_minus"},
		{"anum_0", "anum_1", "anum_2", "anum_3", "anum_4", "anum_5", "anum_6", "anum_7", "anum_8", "anum_9", "anum_minus"}
	};

	CL_InitCrosshairImage ();

	CG_PrintLoading ("pic media");

	cgMedia.tileBackImage				= cgi.Img_RegisterPic ("backtile");
	cgMedia.hudFieldImage				= cgi.Img_RegisterPic ("field_3");
	cgMedia.hudInventoryImage			= cgi.Img_RegisterPic ("inventory");
	cgMedia.hudNetImage					= cgi.Img_RegisterPic ("net");
	for (i=0 ; i<2 ; i++)
		for (j=0 ; j<11 ; j++)
			cgMedia.hudNumImages[i][j]	= cgi.Img_RegisterPic (Q_VarArgs ("%s", sb_nums[i][j]));
	cgMedia.hudPausedImage				= cgi.Img_RegisterPic ("pause");

	cgi.Img_RegisterPic ("w_machinegun");
	cgi.Img_RegisterPic ("a_bullets");
	cgi.Img_RegisterPic ("i_health");
	cgi.Img_RegisterPic ("a_grenades");
}


/*
================
CG_InitFXMedia
================
*/
#define PD_DEFFLAGS (IF_NOGAMMA|IF_NOINTENS|IF_CLAMP)
void CG_InitFXMedia (void) {
	int		i;

	CG_PrintLoading ("fx media");

	for (i=0 ; i<(NUMVERTEXNORMALS * 3) ; i++)
		avelocities[0][i] = (frand () * 255) * 0.01;

	// Particles / Decals
	cgMedia.mediaTable[PT_BLUE]			= cgi.Img_FindImage ("egl/parts/blue.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_GREEN]		= cgi.Img_FindImage ("egl/parts/green.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_RED]			= cgi.Img_FindImage ("egl/parts/red.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_WHITE]		= cgi.Img_FindImage ("egl/parts/white.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_SMOKE]		= cgi.Img_FindImage ("egl/parts/smoke.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_SMOKE2]		= cgi.Img_FindImage ("egl/parts/smoke2.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_BLUEFIRE]		= cgi.Img_FindImage ("egl/parts/bluefire.tga",		PD_DEFFLAGS);

	cgMedia.mediaTable[PT_FIRE1]		= cgi.Img_FindImage ("egl/parts/fire1.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_FIRE2]		= cgi.Img_FindImage ("egl/parts/fire2.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_FIRE3]		= cgi.Img_FindImage ("egl/parts/fire3.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_FIRE4]		= cgi.Img_FindImage ("egl/parts/fire4.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EMBERS1]		= cgi.Img_FindImage ("egl/parts/embers1.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EMBERS2]		= cgi.Img_FindImage ("egl/parts/embers2.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EMBERS3]		= cgi.Img_FindImage ("egl/parts/embers3.tga",		PD_DEFFLAGS);

	cgMedia.mediaTable[PT_BEAM]			= cgi.Img_FindImage ("egl/parts/beam.tga",			PD_DEFFLAGS|IF_NOMIPMAP);
	cgMedia.mediaTable[PT_BLDDRIP]		= cgi.Img_FindImage ("egl/parts/blooddrip.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_BUBBLE]		= cgi.Img_FindImage ("egl/parts/bubble.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_DRIP]			= cgi.Img_FindImage ("egl/parts/drip.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EXPLOFLASH]	= cgi.Img_FindImage ("egl/parts/exploflash.tga",	PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EXPLOWAVE]	= cgi.Img_FindImage ("egl/parts/explowave.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_FLARE]		= cgi.Img_FindImage ("egl/parts/flare.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_FLY]			= cgi.Img_FindImage ("egl/parts/fly.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_MIST]			= cgi.Img_FindImage ("egl/parts/mist.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_SPARK]		= cgi.Img_FindImage ("egl/parts/spark.tga",			PD_DEFFLAGS);
	cgMedia.mediaTable[PT_SPLASH]		= cgi.Img_FindImage ("egl/parts/splash.tga",		PD_DEFFLAGS);

	// Animated explosions
	cgMedia.mediaTable[PT_EXPLO1]		= cgi.Img_FindImage ("egl/parts/explo1.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EXPLO2]		= cgi.Img_FindImage ("egl/parts/explo2.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EXPLO3]		= cgi.Img_FindImage ("egl/parts/explo3.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EXPLO4]		= cgi.Img_FindImage ("egl/parts/explo4.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EXPLO5]		= cgi.Img_FindImage ("egl/parts/explo5.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EXPLO6]		= cgi.Img_FindImage ("egl/parts/explo6.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EXPLO7]		= cgi.Img_FindImage ("egl/parts/explo7.tga",		PD_DEFFLAGS);

	cgMedia.mediaTable[PT_EXPLOEMBERS1]	= cgi.Img_FindImage ("egl/parts/exploembers.tga",	PD_DEFFLAGS);
	cgMedia.mediaTable[PT_EXPLOEMBERS2]	= cgi.Img_FindImage ("egl/parts/exploembers2.tga",	PD_DEFFLAGS);

	// Decal specific
	cgMedia.mediaTable[DT_BLOOD]		= cgi.Img_FindImage ("egl/decals/blood.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[DT_BLOOD2]		= cgi.Img_FindImage ("egl/decals/blood2.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[DT_BLOOD3]		= cgi.Img_FindImage ("egl/decals/blood3.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[DT_BLOOD4]		= cgi.Img_FindImage ("egl/decals/blood4.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[DT_BLOOD5]		= cgi.Img_FindImage ("egl/decals/blood5.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[DT_BLOOD6]		= cgi.Img_FindImage ("egl/decals/blood6.tga",		PD_DEFFLAGS);

	cgMedia.mediaTable[DT_BULLET]		= cgi.Img_FindImage ("egl/decals/bullet.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[DT_BURNMARK]		= cgi.Img_FindImage ("egl/decals/burnmark.tga",		PD_DEFFLAGS);

	cgMedia.mediaTable[DT_EXPLOMARK]	= cgi.Img_FindImage ("egl/decals/explomark.tga",	PD_DEFFLAGS);
	cgMedia.mediaTable[DT_EXPLOMARK2]	= cgi.Img_FindImage ("egl/decals/explomark2.tga",	PD_DEFFLAGS);
	cgMedia.mediaTable[DT_EXPLOMARK3]	= cgi.Img_FindImage ("egl/decals/explomark3.tga",	PD_DEFFLAGS);

	cgMedia.mediaTable[DT_SLASH]		= cgi.Img_FindImage ("egl/decals/slash.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[DT_SLASH2]		= cgi.Img_FindImage ("egl/decals/slash2.tga",		PD_DEFFLAGS);
	cgMedia.mediaTable[DT_SLASH3]		= cgi.Img_FindImage ("egl/decals/slash3.tga",		PD_DEFFLAGS);

	// mapfx media
	cgMedia.mediaTable[MFX_CORONA]		= cgi.Img_FindImage ("egl/mfx/corona.tga",			PD_DEFFLAGS);
}


/*
================
CG_InitSoundMedia

Called on CGame init and on snd_restart
================
*/
void CG_InitSoundMedia (void) {
	int		i;
	char	name[MAX_QPATH];

	CG_PrintLoading ("sound media");

	// generic sounds
	cgMedia.disruptExploSfx			= cgi.Snd_RegisterSound ("weapons/disrupthit.wav");
	cgMedia.grenadeExploSfx			= cgi.Snd_RegisterSound ("weapons/grenlx1a.wav");
	cgMedia.rocketExploSfx			= cgi.Snd_RegisterSound ("weapons/rocklx1a.wav");
	cgMedia.waterExploSfx			= cgi.Snd_RegisterSound ("weapons/xpld_wat.wav");

	cgMedia.gibSfx					= cgi.Snd_RegisterSound ("misc/udeath.wav");
	cgMedia.itemRespawnSfx			= cgi.Snd_RegisterSound ("items/respawn1.wav");
	cgMedia.laserHitSfx				= cgi.Snd_RegisterSound ("weapons/lashit.wav");
	cgMedia.lightningSfx			= cgi.Snd_RegisterSound ("weapons/tesla.wav");

	cgMedia.playerFallSfx			= cgi.Snd_RegisterSound ("*fall2.wav");
	cgMedia.playerFallShortSfx		= cgi.Snd_RegisterSound ("player/land1.wav");
	cgMedia.playerFallFarSfx		= cgi.Snd_RegisterSound ("*fall1.wav");

	cgMedia.playerTeleport			= cgi.Snd_RegisterSound ("misc/tele1.wav");
	cgMedia.bigTeleport				= cgi.Snd_RegisterSound ("misc/bigtele.wav");

	for (i=0 ; i<7 ; i++) {
		Q_snprintfz (name, sizeof (name), "world/spark%i.wav", i+1);
		cgMedia.sparkSfx[i]			= cgi.Snd_RegisterSound (name);

		if (i > 3)
			continue;

		Q_snprintfz (name, sizeof (name), "player/step%i.wav", i+1);
		cgMedia.footstepSfx[i]		= cgi.Snd_RegisterSound (name);

		if (i > 2)
			continue;

		Q_snprintfz (name, sizeof (name), "world/ric%i.wav", i+1);
		cgMedia.ricochetSfx[i]			= cgi.Snd_RegisterSound (name);
	}

	// muzzleflash sounds
	CG_PrintLoading ("mz sound media");

	cgMedia.mzBFGFireSfx			= cgi.Snd_RegisterSound ("weapons/bfg__f1y.wav");
	cgMedia.mzBlasterFireSfx		= cgi.Snd_RegisterSound ("weapons/blastf1a.wav");
	cgMedia.mzETFRifleFireSfx		= cgi.Snd_RegisterSound ("weapons/nail1.wav");
	cgMedia.mzGrenadeFireSfx		= cgi.Snd_RegisterSound ("weapons/grenlf1a.wav");
	cgMedia.mzGrenadeReloadSfx		= cgi.Snd_RegisterSound ("weapons/grenlr1b.wav");
	cgMedia.mzHyperBlasterFireSfx	= cgi.Snd_RegisterSound ("weapons/hyprbf1a.wav");
	cgMedia.mzIonRipperFireSfx		= cgi.Snd_RegisterSound ("weapons/rippfire.wav");

	for (i=0 ; i<5 ; i++) {
		Q_snprintfz (name, sizeof (name), "weapons/machgf%ib.wav", i+1);
		cgMedia.mzMachineGunSfx[i]	= cgi.Snd_RegisterSound (name);
	}

	cgMedia.mzPhalanxFireSfx		= cgi.Snd_RegisterSound ("weapons/plasshot.wav");
	cgMedia.mzRailgunFireSfx		= cgi.Snd_RegisterSound ("weapons/railgf1a.wav");
	cgMedia.mzRailgunReloadSfx		= cgi.Snd_RegisterSound ("weapons/railgr1a.wav");
	cgMedia.mzRocketFireSfx			= cgi.Snd_RegisterSound ("weapons/rocklf1a.wav");
	cgMedia.mzRocketReloadSfx		= cgi.Snd_RegisterSound ("weapons/rocklr1b.wav");
	cgMedia.mzShotgunFireSfx		= cgi.Snd_RegisterSound ("weapons/shotgf1b.wav");
	cgMedia.mzShotgun2FireSfx		= cgi.Snd_RegisterSound ("weapons/shotg2.wav");
	cgMedia.mzShotgunReloadSfx		= cgi.Snd_RegisterSound ("weapons/shotgr1b.wav");
	cgMedia.mzSuperShotgunFireSfx	= cgi.Snd_RegisterSound ("weapons/sshotf1b.wav");
	cgMedia.mzTrackerFireSfx		= cgi.Snd_RegisterSound ("weapons/disint2.wav");

	// monster muzzleflash sounds
	CG_PrintLoading ("mz2 sound media");

	cgMedia.mz2ChicRocketSfx		= cgi.Snd_RegisterSound ("chick/chkatck2.wav");
	cgMedia.mz2FloatBlasterSfx		= cgi.Snd_RegisterSound ("floater/fltatck1.wav");
	cgMedia.mz2FlyerBlasterSfx		= cgi.Snd_RegisterSound ("flyer/flyatck3.wav");
	cgMedia.mz2GunnerGrenadeSfx		= cgi.Snd_RegisterSound ("gunner/gunatck3.wav");
	cgMedia.mz2GunnerMachGunSfx		= cgi.Snd_RegisterSound ("gunner/gunatck2.wav");
	cgMedia.mz2HoverBlasterSfx		= cgi.Snd_RegisterSound ("hover/hovatck1.wav");
	cgMedia.mz2JorgMachGunSfx		= cgi.Snd_RegisterSound ("boss3/xfire.wav");
	cgMedia.mz2MachGunSfx			= cgi.Snd_RegisterSound ("infantry/infatck1.wav");
	cgMedia.mz2MakronBlasterSfx		= cgi.Snd_RegisterSound ("makron/blaster.wav");
	cgMedia.mz2MedicBlasterSfx		= cgi.Snd_RegisterSound ("medic/medatck1.wav");
	cgMedia.mz2SoldierBlasterSfx	= cgi.Snd_RegisterSound ("soldier/solatck2.wav");
	cgMedia.mz2SoldierMachGunSfx	= cgi.Snd_RegisterSound ("soldier/solatck3.wav");
	cgMedia.mz2SoldierShotgunSfx	= cgi.Snd_RegisterSound ("soldier/solatck1.wav");
	cgMedia.mz2SuperTankRocketSfx	= cgi.Snd_RegisterSound ("tank/rocket.wav");
	cgMedia.mz2TankBlasterSfx		= cgi.Snd_RegisterSound ("tank/tnkatck3.wav");

	for (i=0 ; i<5 ; i++) {
		Q_snprintfz (name, sizeof (name), "tank/tnkatk2%c.wav", 'a' + i);
		cgMedia.mz2TankMachGunSfx[i] = cgi.Snd_RegisterSound (name);
	}

	cgMedia.mz2TankRocketSfx		= cgi.Snd_RegisterSound ("tank/tnkatck1.wav");
}


/*
=================
CG_InitGloomMedia
=================
*/
void CG_InitGloomMedia (void) {
	if (!cgi.FS_CurrGame ("gloom"))
		return;

	if (!glm_forcecache->integer)
		return;

	//
	// human classes
	//
	CG_PrintLoading ("gloom media [human classes]");
	cgi.Mod_RegisterModel ("players/engineer/tris.md2");
	cgi.Mod_RegisterModel ("players/male/tris.md2");
	cgi.Mod_RegisterModel ("players/female/tris.md2");
	cgi.Mod_RegisterModel ("players/hsold/tris.md2");
	cgi.Mod_RegisterModel ("players/exterm/tris.md2");
	cgi.Mod_RegisterModel ("players/mech/tris.md2");

	// human structures
	CG_PrintLoading ("gloom media [human structures]");
	cgi.Mod_RegisterModel ("models/objects/dmspot/tris.md2");
	cgi.Mod_RegisterModel ("models/turret/base.md2");
	cgi.Mod_RegisterModel ("models/turret/gun.md2");
	cgi.Mod_RegisterModel ("models/turret/mgun.md2");
	cgi.Mod_RegisterModel ("models/objects/detector/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/tripwire/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/depot/tris.md2");

	// human weapons
	CG_PrintLoading ("gloom media [human weapons]");
	cgi.Mod_RegisterModel ("players/engineer/weapon.md2");
	cgi.Mod_RegisterModel ("players/male/autogun.md2");
	cgi.Mod_RegisterModel ("players/male/shotgun.md2");
	cgi.Mod_RegisterModel ("players/male/smg.md2");
	cgi.Mod_RegisterModel ("players/male/weapon.md2");
	cgi.Mod_RegisterModel ("players/female/weapon.md2");
	cgi.Mod_RegisterModel ("players/hsold/weapon.md2");
	cgi.Mod_RegisterModel ("players/exterm/weapon.md2");
 	cgi.Mod_RegisterModel ("players/mech/weapon.md2");

	// human view weapons
	CG_PrintLoading ("gloom media [human view weapons]");
	cgi.Mod_RegisterModel ("models/weapons/v_auto/tris.md2");
	cgi.Mod_RegisterModel ("models/weapons/v_shot/tris.md2");
	cgi.Mod_RegisterModel ("models/weapons/v_spas/tris.md2");
	cgi.Mod_RegisterModel ("models/weapons/v_launch/tris.md2");
	cgi.Mod_RegisterModel ("models/weapons/v_pist/tris.md2");
 	cgi.Mod_RegisterModel ("models/weapons/v_sub/tris.md2");
	cgi.Mod_RegisterModel ("models/weapons/v_mag/tris.md2");
	cgi.Mod_RegisterModel ("models/weapons/v_plas/tris.md2");
	cgi.Mod_RegisterModel ("models/weapons/v_mech/tris.md2");

	// human objects
	CG_PrintLoading ("gloom media [human objects]");
	cgi.Mod_RegisterModel ("models/objects/c4/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/grenade/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/debris1/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/debris2/tris.md2");

	//
	// alien classes
	//
	CG_PrintLoading ("gloom media [alien classes]");
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

	// alien structures
	CG_PrintLoading ("gloom media [alien structures]");
	cgi.Mod_RegisterModel ("models/objects/cocoon/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/organ/spiker/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/organ/healer/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/organ/obstacle/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/organ/gas/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/spike/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/spore/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/smokexp/tris.md2");
	cgi.Mod_RegisterModel ("models/objects/web/ball.md2");

	// alien objects
	CG_PrintLoading ("gloom media [alien objects]");
	cgi.Mod_RegisterModel ("models/gibs/hatchling/leg/tris.md2");
	cgi.Mod_RegisterModel ("models/gibs/guardian/gib2.md2");
	cgi.Mod_RegisterModel ("models/gibs/guardian/gib1.md2");
	cgi.Mod_RegisterModel ("models/gibs/stalker/gib1.md2");
	cgi.Mod_RegisterModel ("models/gibs/stalker/gib2.md2");
	cgi.Mod_RegisterModel ("models/gibs/stalker/gib3.md2");
	cgi.Mod_RegisterModel ("models/objects/sspore/tris.md2");
}

/*
==============================================================

	MEDIA INITIALIZATION

==============================================================
*/

/*
================
CG_StartMedia

Called before all the cgame is initialized
================
*/
void CG_StartMedia (void) {
	if (cgMedia.initialized)
		return;

	mediaLoading = qTrue;
	CG_ClearLoading (qTrue);
}


/*
================
CG_FinishMedia

Called after all the cgame media was initialized
================
*/
void CG_FinishMedia (void) {
	CG_ClearLoading (qTrue);

	mediaLoading = qFalse;
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
