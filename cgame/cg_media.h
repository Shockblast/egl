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
// cg_media.h
//

/*
=============================================================================

	CGAME MEDIA

=============================================================================
*/

// surface-specific step sounds
typedef struct cgStepMedia_s {
	struct sfx_s	*standard[4];

	struct sfx_s	*concrete[4];
	struct sfx_s	*dirt[4];
	struct sfx_s	*duct[4];
	struct sfx_s	*grass[4];
	struct sfx_s	*gravel[4];
	struct sfx_s	*metal[4];
	struct sfx_s	*metalGrate[4];
	struct sfx_s	*metalLadder[4];
	struct sfx_s	*mud[4];
	struct sfx_s	*sand[4];
	struct sfx_s	*slosh[4];
	struct sfx_s	*snow[6];
	struct sfx_s	*tile[4];
	struct sfx_s	*wade[4];
	struct sfx_s	*wood[4];
	struct sfx_s	*woodPanel[4];
} cgStepMedia_t;

// muzzle flash sounds
typedef struct cgMzMedia_s {
	struct sfx_s	*bfgFireSfx;
	struct sfx_s	*blasterFireSfx;
	struct sfx_s	*etfRifleFireSfx;
	struct sfx_s	*grenadeFireSfx;
	struct sfx_s	*grenadeReloadSfx;
	struct sfx_s	*hyperBlasterFireSfx;
	struct sfx_s	*ionRipperFireSfx;
	struct sfx_s	*machineGunSfx[5];
	struct sfx_s	*phalanxFireSfx;
	struct sfx_s	*railgunFireSfx;
	struct sfx_s	*railgunReloadSfx;
	struct sfx_s	*rocketFireSfx;
	struct sfx_s	*rocketReloadSfx;
	struct sfx_s	*shotgunFireSfx;
	struct sfx_s	*shotgun2FireSfx;
	struct sfx_s	*shotgunReloadSfx;
	struct sfx_s	*superShotgunFireSfx;
	struct sfx_s	*trackerFireSfx;
} cgMzMedia_t;

// monster muzzle flash sounds
typedef struct cgMz2Media_s {
	struct sfx_s	*chicRocketSfx;
	struct sfx_s	*floatBlasterSfx;
	struct sfx_s	*flyerBlasterSfx;
	struct sfx_s	*gunnerGrenadeSfx;
	struct sfx_s	*gunnerMachGunSfx;
	struct sfx_s	*hoverBlasterSfx;
	struct sfx_s	*jorgMachGunSfx;
	struct sfx_s	*machGunSfx;
	struct sfx_s	*makronBlasterSfx;
	struct sfx_s	*medicBlasterSfx;
	struct sfx_s	*soldierBlasterSfx;
	struct sfx_s	*soldierMachGunSfx;
	struct sfx_s	*soldierShotgunSfx;
	struct sfx_s	*superTankRocketSfx;
	struct sfx_s	*tankBlasterSfx;
	struct sfx_s	*tankMachGunSfx[5];
	struct sfx_s	*tankRocketSfx;
} cgMz2Media_t;

// ==========================================================================

typedef struct cgMedia_s {
	qBool				initialized;
	qBool				loadScreenPrepped;

	// engine generated textures
	struct shader_s		*noTexture;
	struct shader_s		*whiteTexture;
	struct shader_s		*blackTexture;

	// console
	struct shader_s		*consoleShader;
	struct shader_s		*conCharsShader;

	// load screen images
	struct shader_s		*loadSplash;
	struct shader_s		*loadBarPos;
	struct shader_s		*loadBarNeg;
	struct shader_s		*loadNoMapShot;
	struct shader_s		*loadMapShot;

	// screen shaders
	struct shader_s		*alienInfraredVision;
	struct shader_s		*infraredGoggles;

	// sounds
	cgStepMedia_t		steps;

	struct sfx_s		*ricochetSfx[3];
	struct sfx_s		*sparkSfx[7];

	struct sfx_s		*disruptExploSfx;
	struct sfx_s		*grenadeExploSfx;
	struct sfx_s		*rocketExploSfx;
	struct sfx_s		*waterExploSfx;

	struct sfx_s		*gibSfx;
	struct sfx_s		*gibSplatSfx[3];

	struct sfx_s		*itemRespawnSfx;
	struct sfx_s		*laserHitSfx;
	struct sfx_s		*lightningSfx;

	struct sfx_s		*playerFallSfx;
	struct sfx_s		*playerFallShortSfx;
	struct sfx_s		*playerFallFarSfx;

	struct sfx_s		*playerTeleport;
	struct sfx_s		*bigTeleport;

	// muzzleflash sounds
	cgMzMedia_t			mz;
	cgMz2Media_t		mz2;

	// models
	struct model_s		*parasiteSegmentMOD;
	struct model_s		*grappleCableMOD;

	struct model_s		*lightningMOD;
	struct model_s		*heatBeamMOD;
	struct model_s		*monsterHeatBeamMOD;

	struct model_s		*maleDisguiseModel;
	struct model_s		*femaleDisguiseModel;
	struct model_s		*cyborgDisguiseModel;

	// skins
	struct shader_s		*maleDisguiseSkin;
	struct shader_s		*femaleDisguiseSkin;
	struct shader_s		*cyborgDisguiseSkin;

	struct shader_s		*modelShellGod;
	struct shader_s		*modelShellHalfDam;
	struct shader_s		*modelShellDouble;
	struct shader_s		*modelShellRed;
	struct shader_s		*modelShellGreen;
	struct shader_s		*modelShellBlue;

	// images
	struct shader_s		*crosshairShader;

	struct shader_s		*tileBackShader;

	struct shader_s		*hudFieldShader;
	struct shader_s		*hudInventoryShader;
	struct shader_s		*hudNetShader;
	struct shader_s		*hudNumShaders[2][11];
	struct shader_s		*hudPausedShader;

	// particle/decal media
	struct shader_s		*decalTable[DT_PICTOTAL];
	struct shader_s		*particleTable[PT_PICTOTAL];
} cgMedia_t;

extern cgMedia_t	cgMedia;

/*
=============================================================================

	UI MEDIA

=============================================================================
*/

// sounds
typedef struct uiSoundMedia_s {
	struct sfx_s	*menuIn;
	struct sfx_s	*menuMove;
	struct sfx_s	*menuOut;
} uiSoundMedia_t;

// menu banners
typedef struct uiBannerMedia_s {
	struct shader_s	*addressBook;
	struct shader_s	*multiplayer;
	struct shader_s	*startServer;
	struct shader_s	*joinServer;
	struct shader_s	*options;
	struct shader_s	*game;
	struct shader_s	*loadGame;
	struct shader_s	*saveGame;
	struct shader_s	*video;
	struct shader_s	*quit;
} uiBannerMedia_t;

// menu media
#define MAINMENU_CURSOR_NUMFRAMES	15
typedef struct uiMenuMedia_s {
	struct shader_s	*mainCursors[MAINMENU_CURSOR_NUMFRAMES];
	struct shader_s	*mainPlaque;
	struct shader_s	*mainLogo;

	struct shader_s	*mainGame;
	struct shader_s	*mainMultiplayer;
	struct shader_s	*mainOptions;
	struct shader_s	*mainVideo;
	struct shader_s	*mainQuit;

	struct shader_s	*mainGameSel;
	struct shader_s	*mainMultiplayerSel;
	struct shader_s	*mainOptionsSel;
	struct shader_s	*mainVideoSel;
	struct shader_s	*mainQuitSel;
} uiMenuMedia_t;

// ==========================================================================

typedef struct uiMedia_s {
	// sounds
	uiSoundMedia_t		sounds;

	// generic textures
	struct shader_s		*blackTexture;
	struct shader_s		*noTexture;
	struct shader_s		*whiteTexture;

	// chars image
	struct shader_s		*conCharsShader;
	struct shader_s		*uiCharsShader;

	// background images
	struct shader_s		*bgBig;

	// cursor images
	struct shader_s		*cursorShader;
	struct shader_s		*cursorHoverShader;

	// menu items
	uiBannerMedia_t		banners;
	uiMenuMedia_t		menus;
} uiMedia_t;

extern uiMedia_t	uiMedia;

// ==========================================================================

//
// cg_media.c
//

void	CG_CacheGloomMedia (void);
void	CG_InitBaseMedia (void);
void	CG_MapInit (void);
void	CG_ShutdownMap (void);

void	CG_SoundMediaInit (void);
void	CG_CrosshairShaderInit (void);
