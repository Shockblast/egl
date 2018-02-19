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

// all sounds
typedef struct cgMediaSounds_s {
	cgStepMedia_t		steps;
	cgMzMedia_t			mz;
	cgMz2Media_t		mz2;

	struct sfx_s		*ricochet[3];
	struct sfx_s		*spark[7];

	struct sfx_s		*disruptExplo;
	struct sfx_s		*grenadeExplo;
	struct sfx_s		*rocketExplo;
	struct sfx_s		*waterExplo;

	struct sfx_s		*gib;
	struct sfx_s		*gibSplat[3];

	struct sfx_s		*itemRespawn;
	struct sfx_s		*laserHit;
	struct sfx_s		*lightning;

	struct sfx_s		*playerFall;
	struct sfx_s		*playerFallShort;
	struct sfx_s		*playerFallFar;

	struct sfx_s		*playerTeleport;
	struct sfx_s		*bigTeleport;

	struct sfx_s		*mgShell[2];
	struct sfx_s		*sgShell[2];
} cgMediaSounds_t;

// ==========================================================================

typedef struct cgMedia_s {
	qBool				initialized;
	qBool				loadScreenPrepped;

	// fonts
	struct font_s		*defaultFont;

	// engine generated textures
	struct material_s		*noTexture;
	struct material_s		*whiteTexture;
	struct material_s		*blackTexture;

	// console
	struct material_s		*consoleMaterial;

	// load screen images
	struct material_s		*loadSplash;
	struct material_s		*loadBarPos;
	struct material_s		*loadBarNeg;
	struct material_s		*loadNoMapShot;
	struct material_s		*loadMapShot;

	// screen materials
	struct material_s		*alienInfraredVision;
	struct material_s		*infraredGoggles;

	// sounds
	cgMediaSounds_t		sfx;

	// models
	struct refModel_s	*parasiteSegmentModel;
	struct refModel_s	*grappleCableModel;
	struct refModel_s	*powerScreenModel;

	struct refModel_s	*brassMGModel;
	struct refModel_s	*brassSGModel;

	struct refModel_s	*lightningModel;
	struct refModel_s	*heatBeamModel;
	struct refModel_s	*monsterHeatBeamModel;

	struct refModel_s	*maleDisguiseModel;
	struct refModel_s	*femaleDisguiseModel;
	struct refModel_s	*cyborgDisguiseModel;

	// skins
	struct material_s		*maleDisguiseSkin;
	struct material_s		*femaleDisguiseSkin;
	struct material_s		*cyborgDisguiseSkin;

	struct material_s		*modelShellGod;
	struct material_s		*modelShellHalfDam;
	struct material_s		*modelShellDouble;
	struct material_s		*modelShellRed;
	struct material_s		*modelShellGreen;
	struct material_s		*modelShellBlue;

	// images
	struct material_s		*crosshairMat;

	struct material_s		*tileBackMat;

	struct material_s		*hudFieldMat;
	struct material_s		*hudInventoryMat;
	struct material_s		*hudNetMat;
	struct material_s		*hudNumMats[2][11];
	struct material_s		*hudPausedMat;

	// particle/decal media
	struct material_s		*decalTable[DT_PICTOTAL];
	struct material_s		*particleTable[PT_PICTOTAL];
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
	struct material_s	*addressBook;
	struct material_s	*multiplayer;
	struct material_s	*startServer;
	struct material_s	*joinServer;
	struct material_s	*options;
	struct material_s	*game;
	struct material_s	*loadGame;
	struct material_s	*saveGame;
	struct material_s	*video;
	struct material_s	*quit;
} uiBannerMedia_t;

// menu media
#define MAINMENU_CURSOR_NUMFRAMES	15
typedef struct uiMenuMedia_s {
	struct material_s	*mainCursors[MAINMENU_CURSOR_NUMFRAMES];
	struct material_s	*mainPlaque;
	struct material_s	*mainLogo;

	struct material_s	*mainGame;
	struct material_s	*mainMultiplayer;
	struct material_s	*mainOptions;
	struct material_s	*mainVideo;
	struct material_s	*mainQuit;

	struct material_s	*mainGameSel;
	struct material_s	*mainMultiplayerSel;
	struct material_s	*mainOptionsSel;
	struct material_s	*mainVideoSel;
	struct material_s	*mainQuitSel;
} uiMenuMedia_t;

// ==========================================================================

typedef struct uiMedia_s {
	// sounds
	uiSoundMedia_t		sounds;

	// background images
	struct material_s		*bgBig;

	// cursor images
	struct material_s		*cursorMat;
	struct material_s		*cursorHoverMat;

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
void	CG_CrosshairMaterialInit (void);
