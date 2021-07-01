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

// cl_local.h
// Primary header for client

//#define	PARANOID			// speed sapping error checking

#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "../common/common.h"
#include "../renderer/refresh.h"

#include "../sound/snd_pub.h"
#include "../sound/cdaudio.h"

#include "../ui/ui_api.h"

#include "keys.h"

/*
==================================================================

	INPUT

==================================================================
*/

void IN_Init (void);
void IN_Shutdown (void);

void IN_Commands (void);	// oportunity for devices to stick commands on the script buffer
void IN_Frame (void);
void IN_Move (userCmd_t *cmd);	// add additional movement on top of the keyboard move cmd

void IN_Activate (qBool active);

/*
==================================================================

	CONSOLE

==================================================================
*/

#define		CON_TEXTSIZE	32768
#define		CON_MAXTIMES	128

typedef struct {
	qBool	initialized;

	char	text[CON_TEXTSIZE];
	float	times[CON_MAXTIMES];	// cls.realTime time the line was generated
									// for transparent notify lines

	int		orMask;

	int		current;		// line where next message will be printed
	int		display;		// bottom of console displays this line

	float	visLines;
	int		totalLines;		// total lines in console scrollback
	int		lineWidth;		// characters across screen

	float	currentDrop;	// aproaches con_DropHeight at scr_conspeed->value
	float	dropHeight;		// 0.0 to 1.0 lines of console to display
} console_t;

extern	console_t	con;

void Con_ClearText (void);
void Con_ClearNotify (void);

void Con_ToggleConsole_f (void);

void Con_CheckResize (void);

void Con_Print (int flags, const char *txt);

void Con_Init (void);
void Con_Shutdown (void);

void Con_RunConsole (void);
void Con_DrawConsole (void);

/*
==================================================================

	ENTITY

==================================================================
*/

typedef struct {
	qBool			valid;			// cleared if delta parsing was invalid
	int				serverFrame;
	int				serverTime;		// server time the message is valid for (in msec)
	int				deltaFrame;
	byte			areaBits[BSP_MAX_AREAS/8];		// portalarea visibility bits
	playerState_t	playerState;
	int				numEntities;
	int				parseEntities;	// non-masked index into cl_ParseEntities array
} frame_t;

typedef struct {
	entityState_t	baseLine;		// delta from this if not from a previous frame
	entityState_t	current;
	entityState_t	prev;			// will always be valid, but might just be a copy of current

	int				serverFrame;		// if not current, this ent isn't in the frame

	vec3_t			lerpOrigin;		// for trails (variable hz)

	int				flyStopTime;

	qBool			muzzleOn;
	int				muzzType;
	qBool			muzzSilenced;
	qBool			muzzVWeap;
} centity_t;

extern	centity_t	cl_Entities[MAX_EDICTS];

#define MAX_CLIENTWEAPONMODELS		20		// PGM -- upped from 16 to fit the chainfist vwep
#define	MAX_PARSE_ENTITIES			1024

extern char	cl_WeaponModels[MAX_CLIENTWEAPONMODELS][MAX_QPATH];
extern int	cl_NumWeaponModels;

// the cl_ParseEntities must be large enough to hold UPDATE_BACKUP frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original
extern	entityState_t	cl_ParseEntities[MAX_PARSE_ENTITIES];

/*
==================================================================

	CLIENT STATE

==================================================================
*/

#define	CMD_BACKUP		64	// allow a lot of command backups for very fast systems

typedef struct {
	char				name[MAX_QPATH];
	char				cinfo[MAX_QPATH];
	struct image_s		*skin;
	struct image_s		*icon;
	char				iconname[MAX_QPATH];
	struct model_s		*model;
	struct model_s		*weaponmodel[MAX_CLIENTWEAPONMODELS];
} clientInfo_t;

// the clientState_t structure is wiped completely at every
// server map change

typedef struct {
	int					timeOutCount;

	int					timeDemoFrames;
	int					timeDemoStart;

	qBool				refreshPrepped;		// false if on new level or new ref dll
	qBool				soundPrepped;		// ambient sounds can start
	qBool				forceRefresh;		// vid has changed, so we can't use a paused refdef

	int					parseEntities;		// index (not anded off) into cl_ParseEntities[]

	userCmd_t			cmd;
	userCmd_t			cmds[CMD_BACKUP];	// each mesage will send several old cmds
	int					cmdTime[CMD_BACKUP];	// time sent, for calculating pings
	short				predictedOrigins[CMD_BACKUP][3];	// for debug comparing against server

	float				predictedStep;		// for stair up smoothing
	unsigned			predictedStepTime;

	vec3_t				predictedOrigin;	// generated by CL_PredictMovement
	vec3_t				predictedAngles;
	vec3_t				predictionError;

	frame_t				frame;				// received from server
	int					surpressCount;		// number of messages rate supressed
	frame_t				frames[UPDATE_BACKUP];

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  It is cleared to 0 upon entering each level.
	// the server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes
	vec3_t				viewAngles;

	int					time;			// this is the time value that the client
								// is rendering at. always <= cls.realTime
	float				lerpFrac;		// between oldframe and frame

	refDef_t			refDef;

	vec3_t				v_Forward, v_Right, v_Up;	// set when refdef.angles is set

	//
	// transient data from server
	//
	char				layout[1024];		// general 2D overlay
	int					inventory[MAX_ITEMS];

	//
	// non-gameserver infornamtion
	FILE				*cinematicFile;
	int					cinematicTime;		// cls.realTime for first cinematic frame
	int					cinematicFrame;
	char				cinematicPalette[768];
	qBool				cinematicPaletteActive;

	//
	// server state information
	//
	qBool				attractLoop;		// running the attract loop, any key will menu
	int					serverCount;		// server identification for prespawns
	char				gameDir[MAX_QPATH];
	int					playerNum;

	char				configStrings[MAX_CONFIGSTRINGS][MAX_QPATH];

	//
	// locally derived information from server state
	//
	struct model_s		*modelDraw[MAX_MODELS];
	struct cBspModel_s	*modelClip[MAX_MODELS];

	struct sfx_s		*soundPrecache[MAX_SOUNDS];
	struct image_s		*imagePrecache[MAX_IMAGES];

	clientInfo_t		clientInfo[MAX_CLIENTS];
	clientInfo_t		baseClientInfo;
} clientState_t;

extern	clientState_t	cl;

/*
==================================================================

	EFFECTS

==================================================================
*/

#ifndef __gl_h_
#ifndef __GL_H__
#define  GL_ZERO						0
#define  GL_ONE							1
#define  GL_SRC_COLOR					0x0300
#define  GL_ONE_MINUS_SRC_COLOR			0x0301
#define  GL_SRC_ALPHA					0x0302
#define  GL_ONE_MINUS_SRC_ALPHA			0x0303
#define  GL_DST_ALPHA					0x0304
#define  GL_ONE_MINUS_DST_ALPHA			0x0305
#define  GL_DST_COLOR					0x0306
#define  GL_ONE_MINUS_DST_COLOR			0x0307
#define  GL_SRC_ALPHA_SATURATE			0x0308
#define  GL_CONSTANT_COLOR				0x8001
#define  GL_ONE_MINUS_CONSTANT_COLOR	0x8002
#define  GL_CONSTANT_ALPHA				0x8003
#define  GL_ONE_MINUS_CONSTANT_ALPHA	0x8004
#endif
#endif

enum {
	// ----------- PARTICLES ----------
	PT_BLUE,
	PT_GREEN,
	PT_RED,
	PT_WHITE,

	PT_SMOKE,
	PT_SMOKE2,

	PT_BLUEFIRE,
	PT_FIRE,

	PT_BEAM,
	PT_BLDDRIP,
	PT_BUBBLE,
	PT_DRIP,
	PT_EMBERS,
	PT_EMBERS2,
	PT_EXPLOFLASH,
	PT_EXPLOWAVE,
	PT_FLARE,
	PT_FLY,
	PT_MIST,
	PT_SPARK,
	PT_SPLASH,
	// ----------- ANIMATED -----------
	PT_EXPLO1,
	PT_EXPLO2,
	PT_EXPLO3,
	PT_EXPLO4,
	PT_EXPLO5,
	PT_EXPLO6,
	PT_EXPLO7,
	// ------------ DECALS ------------
	DT_BLOOD,
	DT_BLOOD2,
	DT_BLOOD3,
	DT_BLOOD4,
	DT_BLOOD5,
	DT_BLOOD6,

	DT_BULLET,

	DT_BURNMARK,
	DT_BURNMARK2,
	DT_BURNMARK3,

	DT_SLASH,
	DT_SLASH2,
	DT_SLASH3,
	// ------------- TOTAL ------------
	PDT_PICTOTAL
};

/*
==================================================================

	DECALS

==================================================================
*/

typedef struct cdecal_s {
	struct cdecal_s	*next;

	double			time;

	int				numVerts;
	vec2_t			coords[MAX_DECAL_VERTS];
	vec3_t			verts[MAX_DECAL_VERTS];

	struct mNode_s	*node;

	vec3_t			org;
	vec3_t			dir;

	dvec4_t			color;
	dvec4_t			colorVel;

	double			size;
	double			sizeVel;

	double			lifeTime;

	image_t			*image;
	int				flags;

	int				sFactor;
	int				dFactor;

	void			(*think)(struct cdecal_s *d, dvec4_t color, int *type, int *flags);
	qBool			thinkNext;

	double			angle;
} cdecal_t;

// ===============================================================

#define DECAL_GLOWTIME	( cl_decal_burnlife->value )
#define DECAL_INSTANT	-10000.0

//
//		MANAGEMENT
//

cdecal_t	*makeDecal (double org0,					double org1,					double org2,
						double dir0,					double dir1,					double dir2,
						double red,						double green,					double blue,
						double redVel,					double greenVel,				double blueVel,
						double alpha,					double alphaVel,
						double size,					double sizeVel,
						int type,						int flags,
						int sFactor,					int dFactor,
						void (*think)(struct cdecal_s *d, dvec4_t color, int *type, int *flags),
						qBool thinkNext,
						double lifeTime,				 double angle);

void CL_FreeDecal (cdecal_t *d);
void CL_ClearDecals (void);
void CL_AddDecals (void);

//
//		IMAGING
//

int dRandBloodMark (void);
int dRandBurnMark (void);
int dRandSlashMark (void);

/*
==================================================================

	PARTICLES

==================================================================
*/

typedef struct cparticle_s {
	struct cparticle_s	*next;

	double				time;

	vec3_t				org;
	vec3_t				oldOrigin;

	vec3_t				angle;
	vec3_t				vel;
	vec3_t				accel;

	dvec4_t				color;
	dvec4_t				colorVel;

	double				size;
	double				sizeVel;

	image_t				*image;
	int					flags;

	int					sFactor;
	int					dFactor;

	double				orient;

	void				(*think)(struct cparticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time);
	qBool				thinkNext;
} cparticle_t;

// ================================================================

#define	PART_GRAVITY	110
#define PART_INSTANT	-10000.0
#define	BEAMLENGTH		16

vec3_t avelocities[NUMVERTEXNORMALS];

//
//		MANAGEMENT
//

void FASTCALL makePart (double org0,					double org1,					double org2,
						double angle0,					double angle1,					double angle2,
						double vel0,					double vel1,					double vel2,
						double accel0,					double accel1,					double accel2,
						double red,						double green,					double blue,
						double redVel,					double greenVel,				double blueVel,
						double alpha,					double alphaVel,
						double size,					double sizeVel,
						int type,						int flags,
						int sFactor,					int dFactor,
						void (*think)(struct cparticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time),
						qBool thinkNext,
						double orient);

float FASTCALL pDecDegree (vec3_t org, float dec, float scale, qBool lod);
float FASTCALL pIncDegree (vec3_t org, float inc, float scale, qBool lod);
qBool pDegree (vec3_t org, qBool lod);

void CL_FreeParticle (cparticle_t *p);
void CL_ClearParticles (void);
void CL_AddParticles (void);

//
//		PARTICLE IMAGING
//

int pRandSmoke (void);

//
//		INTELLIGENCE
//

#define PMAXBLDRIPLEN	2.75
#define PMAXSPLASHLEN	2.0

void pBloodDripThink (struct cparticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time);
void pBloodThink (struct cparticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time);
void pBounceThink (struct cparticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time);
void pExploAnimThink (struct cparticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time);
void pExploWaveThink (struct cparticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time);
void pFastSmokeThink (struct cparticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time);
void pFireThink (struct cparticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time);
void pFireTrailThink (struct cparticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time);
void pFlareThink (struct cparticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time);
void pRailSpiralThink (struct cparticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time);
void pRicochetSparkThink (struct cparticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time);
void pSlowFireThink (struct cparticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time);
void pSmokeThink (struct cparticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time);
void pSparkGrowThink (struct cparticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time);
void pSplashThink (struct cparticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time);
void pSplashPlumeThink (struct cparticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time);
void pSmokeTrailThink (struct cparticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time);

//
//		CREATING FUNCTIONS
//

//
// pfx__effects.c
//

void CL_BlasterBlueParticles (vec3_t org, vec3_t dir);
void CL_BlasterGoldParticles (vec3_t org, vec3_t dir);
void CL_BlasterGreenParticles (vec3_t org, vec3_t dir);
void CL_BlasterGreyParticles (vec3_t org, vec3_t dir);
void CL_BleedEffect (vec3_t org, vec3_t dir, int count);
void CL_BleedGreenEffect (vec3_t org, vec3_t dir, int count);
void CL_BubbleEffect (vec3_t origin);
void CL_ExplosionBFGEffect (vec3_t org);
void FASTCALL CL_FlareEffect (vec3_t origin, int type, double orient, double size, double sizevel, int color, int colorvel, double alpha, double alphavel);
void FASTCALL CL_GenericParticleEffect (vec3_t org, vec3_t dir, int color, int count, int numcolors, int dirspread, double alphavel);
void CL_ItemRespawnEffect (vec3_t org);
void CL_LogoutEffect (vec3_t org, int type);
void FASTCALL CL_ParticleEffect (vec3_t org, vec3_t dir, int color, int count);
void FASTCALL CL_ParticleEffect2 (vec3_t org, vec3_t dir, int color, int count);
void FASTCALL CL_ParticleEffect3 (vec3_t org, vec3_t dir, int color, int count);
void FASTCALL CL_ParticleSteamEffect (vec3_t org, vec3_t dir, int color, int count, int magnitude);
void FASTCALL CL_ParticleSmokeEffect (vec3_t org, vec3_t dir, int color, int count, int magnitude);
void FASTCALL CL_RicochetEffect (vec3_t org, vec3_t dir, int count);
void CL_RocketFireParticles (vec3_t org, vec3_t dir);
void FASTCALL CL_SparkEffect (vec3_t org, vec3_t dir, int color, int colorvel, int count, float lifescale);
void FASTCALL CL_SplashEffect (vec3_t org, vec3_t dir, int color, int count);

//
// pfx_gloom.c
//

void CL_GloomBlobTip (vec3_t start, vec3_t end);
void CL_GloomDroneEffect (vec3_t org, vec3_t dir);
void CL_GloomEmberTrail (vec3_t start, vec3_t end);
void CL_GloomFlareTrail (vec3_t start, vec3_t end);
void CL_GloomGasEffect (vec3_t origin);
void CL_GloomRepairEffect (vec3_t org, vec3_t dir, int count);
void CL_GloomStingerFire (vec3_t start, vec3_t end, double size, qBool light);

//
// pfx_misc.c
//

void CL_BigTeleportParticles (vec3_t org);
void CL_BlasterTip (vec3_t start, vec3_t end);
void CL_ExplosionParticles (vec3_t org, double scale, qBool exploonly, qBool inwater);
void CL_ExplosionBFGParticles (vec3_t org);
void CL_ExplosionColorParticles (vec3_t org);
void CL_FlyParticles (vec3_t origin, int count);
void CL_FlyEffect (centity_t *ent, vec3_t origin);
void CL_ForceWall (vec3_t start, vec3_t end, int color);
void CL_MonsterPlasma_Shell (vec3_t origin);
void CL_PhalanxTip (vec3_t start, vec3_t end);
void CL_TeleportParticles (vec3_t org);
void CL_TeleporterParticles (entityState_t *ent);
void CL_TrackerExplode (vec3_t origin);
void CL_TrackerShell (vec3_t origin);
void CL_TrapParticles (entity_t *ent);
void CL_WidowSplash (vec3_t org);

//
// pfx_sustain.c
//

void CL_ClearSustains (void);
void CL_ProcessSustains (void);
void CL_ParseNuke (void);
void CL_ParseSteam (void);
void CL_ParseWidow (void);

//
// pfx_trail.c
//

void FASTCALL CL_BeamTrail (vec3_t start, vec3_t end, int color, double size, double alpha, double alphaVel);
void CL_BfgTrail (entity_t *ent);
void CL_BlasterGoldTrail (vec3_t start, vec3_t end);
void CL_BlasterGreenTrail (vec3_t start, vec3_t end);
void CL_BubbleTrail (vec3_t start, vec3_t end);
void CL_BubbleTrail2 (vec3_t start, vec3_t end, int dist);
void CL_DebugTrail (vec3_t start, vec3_t end);
void CL_FlagTrail (vec3_t start, vec3_t end, int flags);
void CL_GibTrail (vec3_t start, vec3_t end, int flags);
void CL_GrenadeTrail (vec3_t start, vec3_t end);
void CL_Heatbeam (vec3_t start, vec3_t forward);
void CL_IonripperTrail (vec3_t start, vec3_t end);
void CL_QuadTrail (vec3_t start, vec3_t end);
void CL_RailTrail (vec3_t start, vec3_t end);
void CL_RocketTrail (vec3_t start, vec3_t end);
void CL_TagTrail (vec3_t start, vec3_t end);
void CL_TrackerTrail (vec3_t start, vec3_t end);

/*
==============================================================

	CLIENT-SIDE ENTITIES

==============================================================
*/

//
//		MANAGEMENT
//

void CL_AddCLEnts (void);
void CL_ClearCLEnts (void);

//
//		INTELLIGENCE
//

//
//		CREATING FUNCTIONS
//

void CL_BulletShell (vec3_t origin, int count);
void CL_ExplosionShards (vec3_t origin, int count);
void CL_RailShell (vec3_t origin, int count);
void CL_ShotgunShell (vec3_t origin, int count);

/*
==================================================================

	MEDIA

==================================================================
*/

typedef struct {
	// sounds
	struct sfx_s		*ricochet1SFX;
	struct sfx_s		*ricochet2SFX;
	struct sfx_s		*ricochet3SFX;

	struct sfx_s		*spark5SFX;
	struct sfx_s		*spark6SFX;
	struct sfx_s		*spark7SFX;

	struct sfx_s		*disruptExploSFX;
	struct sfx_s		*grenadeExploSFX;
	struct sfx_s		*rocketExploSFX;
	struct sfx_s		*waterExploSFX;

	struct sfx_s		*laserHitSFX;
	struct sfx_s		*railgunFireSFX;
	struct sfx_s		*lightningSFX;

	struct sfx_s		*footstepSFX[4];

	// models
	struct model_s		*parasiteSegmentMOD;
	struct model_s		*grappleCableMOD;
	struct model_s		*bfgExploMOD;

	struct model_s		*lightningMOD;
	struct model_s		*heatBeamMOD;
	struct model_s		*monsterHeatBeamMOD;

	// images
	image_t				*consoleImage;
	image_t				*crosshairImage;
	image_t				*tileBackImage;

	image_t				*hudFieldImage;
	image_t				*hudInventoryImage;
	image_t				*hudLoadingImage;
	image_t				*hudNetImage;
	image_t				*hudNumImages[2][11];
	image_t				*hudPausedImage;

	// particle/decal media
	image_t				*mediaTable[PDT_PICTOTAL];
} clMedia_t;

extern clMedia_t	clMedia;

/*
==================================================================

	CONNECTION

==================================================================
*/

enum {
	CA_UNINITIALIZED,
	CA_DISCONNECTED,	// not talking to a server
	CA_CONNECTING,		// sending request packets to the server
	CA_CONNECTED,		// netChan_t established, waiting for svc_serverdata
	CA_ACTIVE			// game views should be displayed
};

extern	netAdr_t	net_From;
extern	netMsg_t	net_Message;

// the clientStatic_t structure is persistant through an arbitrary number of server connections
typedef struct {
	int			connState;
	int			keyDest;

	int			frameCount;
	float		frameTime;			// seconds since last frame

	int			realTime;			// always increasing, no clamping, etc

	// screen rendering information
	float		disableScreen;		// showing loading plaque between levels
									// or changing rendering dlls
									// if time gets > 30 seconds ahead, break it
	int			disableServerCount;	// when we receive a frame and cl.servercount
									// > cls.disableServerCount, clear disableScreen

	// connection information
	char		serverName[MAX_OSPATH];	// name of server from original connect
	float		connectTime;			// for connection retransmits

	int			quakePort;			// a 16 bit value that allows quake servers
									// to work around address translating routers
	netChan_t	netChan;
	int			serverProtocol;		// in case we are doing some kind of version hack

	int			challenge;			// from the server to use for connecting

	FILE		*downloadFile;		// file transfer from server
	char		downloadTempName[MAX_OSPATH];
	char		downloadName[MAX_OSPATH];
	int			downloadNumber;
	int			downloadPercent;

	// demo recording info must be here, so it isn't cleared on level change
	FILE		*demoFile;
	qBool		demoRecording;
	qBool		demoWaiting;		// don't record until a non-delta message is received
} clientStatic_t;

extern clientStatic_t	cls;

/*
==================================================================

	DLIGHT

==================================================================
*/

typedef struct {
	vec3_t	origin;
	vec3_t	color;

	int		key;				// so entities can reuse same entry

	float	radius;
	float	die;				// stop lighting after this time
	float	decay;				// drop this each second
	float	minlight;			// don't add when contributing less
} cDLight_t;

void CL_ClearLightStyles (void);
void CL_RunLightStyles (void);
void CL_SetLightstyle (int i);
void CL_AddLightStyles (void);

void CL_ClearDLights (void);
cDLight_t *CL_AllocDLight (int key);
void CL_RunDLights (void);
void CL_AddDLights (void);

void CL_Flashlight (int ent, vec3_t pos);
void FASTCALL CL_ColorFlash (vec3_t pos, int ent, int intensity, float r, float g, float b);

/*
==================================================================

	SUSTAINED EFFECTS

==================================================================
*/

typedef struct csustain_s {
	vec3_t		org;
	vec3_t		dir;

	int			id;
	int			type;

	int			endtime;
	int			nextthink;
	int			thinkinterval;

	int			color;
	int			count;
	int			magnitude;

	void		(*think)(struct csustain_s *self);
} csustain_t;

#define MAX_SUSTAINS	32


/*
==================================================================

	CVARS

==================================================================
*/

extern	cvar_t	*cl_add_particles;
extern	cvar_t	*cl_gun;
extern	cvar_t	*cl_predict;
extern	cvar_t	*cl_noskins;
extern	cvar_t	*cl_autoskins;
extern	cvar_t	*cl_upspeed;
extern	cvar_t	*cl_forwardspeed;
extern	cvar_t	*cl_sidespeed;
extern	cvar_t	*cl_yawspeed;
extern	cvar_t	*cl_pitchspeed;
extern	cvar_t	*cl_run;
extern	cvar_t	*cl_anglespeedkey;
extern	cvar_t	*cl_shownet;
extern	cvar_t	*cl_showmiss;
extern	cvar_t	*cl_showclamp;
extern	cvar_t	*cl_stereo_separation;
extern	cvar_t	*cl_stereo;
extern	cvar_t	*cl_lightlevel;	// FIXME HACK
extern	cvar_t	*cl_paused;
extern	cvar_t	*cl_timedemo;
extern	cvar_t	*cl_vwep;

extern	cvar_t	*freelook;
extern	cvar_t	*lookspring;
extern	cvar_t	*lookstrafe;
extern	cvar_t	*sensitivity;
extern	cvar_t	*s_khz;

extern	cvar_t	*m_pitch;
extern	cvar_t	*m_yaw;
extern	cvar_t	*m_forward;
extern	cvar_t	*m_side;

extern	cvar_t	*autosensitivity;
extern	cvar_t	*ch_alpha;
extern	cvar_t	*ch_pulse;
extern	cvar_t	*ch_scale;
extern	cvar_t	*ch_red;
extern	cvar_t	*ch_green;
extern	cvar_t	*ch_blue;
extern	cvar_t	*cl_add_decals;
extern	cvar_t	*cl_decal_burnlife;
extern	cvar_t	*cl_decal_life;
extern	cvar_t	*cl_decal_lod;
extern	cvar_t	*cl_decal_max;
extern	cvar_t	*cl_explorattle;
extern	cvar_t	*cl_explorattle_scale;
extern	cvar_t	*cl_footsteps;
extern	cvar_t	*cl_gore;
extern	cvar_t	*cl_railred;
extern	cvar_t	*cl_railgreen;
extern	cvar_t	*cl_railblue;
extern	cvar_t	*cl_railtrail;
extern	cvar_t	*cl_showfps;
extern	cvar_t	*cl_showping;
extern	cvar_t	*cl_showtime;
extern	cvar_t	*cl_smokelinger;
extern	cvar_t	*cl_spiralred;
extern	cvar_t	*cl_spiralgreen;
extern	cvar_t	*cl_spiralblue;
extern	cvar_t	*cl_timestamp;
extern	cvar_t	*con_alpha;
extern	cvar_t	*con_clock;
extern	cvar_t	*con_drop;
extern	cvar_t	*con_scroll;
extern	cvar_t	*glm_advgas;
extern	cvar_t	*glm_advstingfire;
extern	cvar_t	*glm_blobtype;
extern	cvar_t	*glm_bluestingfire;
extern	cvar_t	*glm_flashpred;
extern	cvar_t	*glm_flwhite;
extern	cvar_t	*glm_jumppred;
extern	cvar_t	*glm_showclass;
extern	cvar_t	*hand;
extern	cvar_t	*m_accel;
extern	cvar_t	*part_cull;
extern	cvar_t	*part_degree;
extern	cvar_t	*part_lod;
extern	cvar_t	*r_advir;
extern	cvar_t	*r_fontscale;
extern	cvar_t	*r_hudscale;
extern	cvar_t	*scr_demomsg;
extern	cvar_t	*scr_graphalpha;
extern	cvar_t	*scr_hudalpha;
extern	cvar_t	*sv_defaultpaks;
extern	cvar_t	*ui_sensitivity;

/*
==================================================================

	MISCELLANEOUS

==================================================================
*/

//
// cl_cin.c
//

void CIN_PlayCinematic (char *name);
qBool CIN_DrawCinematic (void);
void CIN_RunCinematic (void);
void CIN_StopCinematic (void);
void CIN_FinishCinematic (void);

//
// cl_demo.c
//

void CL_WriteDemoMessage (void);
void CL_Stop_f (void);
void CL_Record_f (void);

//
// cl_ents.c
//

int CL_ParseEntityBits (unsigned *bits);
void CL_ParseDelta (entityState_t *from, entityState_t *to, int number, int bits);
void CL_EntityEvent (entityState_t *ent);
void CL_ParseFrame (void);
void CL_AddEntities (void);

//
// cl_dlight.c
//

void CL_ParseMuzzleFlash (void);
void CL_ParseMuzzleFlash2 (void);

//
// cl_input.c
//

typedef struct
{
	int			down[2];		// key nums holding it down
	unsigned	downTime;		// msec timestamp
	unsigned	msec;			// msec down this frame
	int			state;
} kButton_t;

extern	kButton_t	in_Strafe;
extern	kButton_t	in_Speed;

void CL_InitInput (void);
void CL_SendCmd (void);
void CL_SendMove (userCmd_t *cmd);

int CL_ReadFromServer (void);

void CL_WriteToServer (userCmd_t *cmd);
void CL_BaseMove (userCmd_t *cmd);

void IN_CenterView (void);

float CL_KeyState (kButton_t *key);

//
// cl_hud.c
//

void	HUD_ParseInventory (void);
void	HUD_DrawLayout	(void);

//
// cl_main.c
//

void	CL_ResetServerCount (void);

void	CL_SetConnectState (int state);
qBool	CL_ConnectionState (int state);
void	CL_ClearState (void);

void	CL_Disconnect (void);

void	CL_Quit_f (void);
void	CL_PingServers_f (void);

void	CL_RegisterClientSounds (void);
void	CL_InitMedia (void);

void	CL_RequestNextDownload (void);

void	FASTCALL CL_Frame (int msec);

void	CL_Init (void);
void	CL_Shutdown (void);

//
// cl_parse.c
//

extern int	classtype;

extern char	*svc_strings[256];

void	CL_ParseConfigString (void);

void	CL_ParseServerMessage (void);
void	CL_LoadClientinfo (clientInfo_t *ci, char *s);
void	SHOWNET(char *s);
void	CL_ParseClientinfo (int player);
void	CL_Download_f (void);

qBool	CL_CheckOrDownloadFile (char *filename);

//
// cl_prediction.c
//

void	CL_CheckPredictionError (void);

trace_t	CL_PMTrace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);
int		CL_PMPointContents (vec3_t point);

void	CL_PredictMovement (void);

//
// cl_screen.c
//

float	palRed		(int color);
float	palGreen	(int color);
float	palBlue		(int color);
float	palRedf		(int color);
float	palGreenf	(int color);
float	palBluef	(int color);

extern	cvar_t		*crosshair;

extern	cvar_t		*scr_conspeed;
extern	cvar_t		*scr_viewsize;

void	SCR_Init (void);
void	SCR_Shutdown (void);

void	SCR_UpdateScreen (void);

void	SCR_SizeUp (void);
void	SCR_SizeDown (void);

void	SCR_CenterPrint (char *str);
void	SCR_BeginLoadingPlaque (void);
void	SCR_EndLoadingPlaque (void);
void	SCR_DebugGraph (float value, int color);

void	SCR_Benchmark_f (void);
void	SCR_TimeRefresh_f (void);

void	CL_AddNetgraph (void);

//
// cl_tent.c
//

void CL_ExploRattle (vec3_t org, float scale);

void CL_ClearTEnts (void);
void CL_ParseTEnt (void);
void CL_AddTEnts (void);

//
// cl_uiapi.c
//

void CL_UIModule_Init (void);
void CL_UIModule_Shutdown (void);
void CL_UIModule_Refresh (qBool backGround);
void CL_UIModule_MainMenu (void);
void CL_UIModule_ForceMenuOff (void);
void CL_UIModule_MoveMouse (int mx, int my);
void CL_UIModule_Keydown (int key);
void CL_UIModule_AddToServerList (netAdr_t adr, char *info);
void CL_UIModule_DrawStatusBar (char *string);

void CL_UI_CheckChanges (void);
void CL_UI_Restart_f (void);
void CL_UI_Init (void);
void CL_UI_Shutdown (void);

//
// cl_view.c
//

extern	int				gun_frame;
extern	struct model_s	*gun_model;

void CL_ClearEffects (void);

void View_RenderView (vRect_t vRect, float stereoSeparation);

void View_Init (void);
void View_Shutdown (void);

//
// r_cin.c
//

void	R_SetPalette (const byte *palette);
void	R_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data);

//
// r_draw.c
//

void	Draw_Pic (image_t *image, float x, float y, int w, int h, float s1, float t1, float s2, float t2, vec4_t color);
void	Draw_StretchPic (char *pic, float x, float y, int w, int h, vec4_t color);
int		Draw_String (float x, float y, int flags, float scale, char *string, vec4_t color);
int		Draw_StringLen (float x, float y, int flags, float scale, char *string, int len, vec4_t color);
void	Draw_Char (float x, float y, int flags, float scale, int num, vec4_t color);
void	Draw_Fill (float x, float y, int w, int h, vec4_t color);

//
// r_fx.c
//

extern int R_GetClippedFragments (vec3_t origin, float radius, vec3_t axis[3], int maxfverts, vec3_t *fverts, int maxfragments, fragment_t *fragments);

//
// r_image.c
//

image_t	*Img_FindImage (char *name, int flags);

image_t	*Img_RegisterPic (char *name);
image_t	*Img_RegisterSkin (char *name);

void	Img_GetPicSize (int *w, int *h, char *pic);

//
// r_main.c
//

void	R_RenderFrame (refDef_t *fd);
void	R_BeginFrame (float cameraSeparation);
void	R_EndFrame (void);

void	GL_RendererMsg_f (void);
void	GL_VersionMsg_f (void);

void	R_BeginRegistration (char *mapName);
void	R_EndRegistration (void);
void	R_RegisterMedia (void);

qBool	R_Init (void *hInstance, void *hWnd);
void	R_Shutdown (void);

//
// r_model.c
//

struct model_s	*Mod_RegisterModel (char *name);

//
// r_sky.c
//

qBool	R_CheckLoadSky (void);
void	R_SetSky (char *name, float rotate, vec3_t axis);

/*
=============================================================

	IMPLIMENTATION SPECIFIC

=============================================================
*/

//
// vid_win.c
//

extern	vidDef_t	vidDef;				// global video state

void	VID_Restart_f (void);
void	VID_Init (void);
void	VID_Shutdown (void);
void	VID_CheckChanges (void);

//
// key_win.c
//

int		Key_MapKey (int key);
qBool	Key_CapslockState (void);
