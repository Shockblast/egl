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
// cg_effects.h
//

/*
=============================================================================

	LIGHTING

=============================================================================
*/

typedef struct cgDlight_s {
	vec3_t		origin;
	vec3_t		color;

	int			key;				// so entities can reuse same entry

	float		radius;
	float		die;				// stop lighting after this time
	float		decay;				// drop this each second
	float		minlight;			// don't add when contributing less
} cgDLight_t;

//
// cg_light.c
//

void	CG_ClearLightStyles (void);
cgDLight_t *CG_AllocDLight (int key);
void	CG_RunLightStyles (void);
void	CG_SetLightstyle (int num);
void	CG_AddLightStyles (void);

void	CG_ClearDLights (void);
void	CG_RunDLights (void);
void	CG_AddDLights (void);

void	CG_Flashlight (int ent, vec3_t pos);
void	__fastcall CG_ColorFlash (vec3_t pos, int ent, float intensity, float r, float g, float b);
void	CG_WeldingSparkFlash (vec3_t pos);

/*
=============================================================================

	EFFECTS

=============================================================================
*/

enum {
	// ----------- PARTICLES ----------
	PT_BFG_DOT,

	PT_BLASTER_BLUE,
	PT_BLASTER_GREEN,
	PT_BLASTER_RED,

	PT_IONTAIL,
	PT_IONTIP,
	PT_ITEMRESPAWN,
	PT_ENGYREPAIR_DOT,
	PT_PHALANXTIP,

	PT_GENERIC,
	PT_GENERIC_GLOW,

	PT_SMOKE,
	PT_SMOKE2,

	PT_SMOKEGLOW,
	PT_SMOKEGLOW2,

	PT_BLUEFIRE,
	PT_FIRE1,
	PT_FIRE2,
	PT_FIRE3,
	PT_FIRE4,
	PT_EMBERS1,
	PT_EMBERS2,
	PT_EMBERS3,

	PT_BLOOD,
	PT_BLOOD2,
	PT_BLOOD3,
	PT_BLOOD4,
	PT_BLOOD5,
	PT_BLOOD6,

	PT_GRNBLOOD,
	PT_GRNBLOOD2,
	PT_GRNBLOOD3,
	PT_GRNBLOOD4,
	PT_GRNBLOOD5,
	PT_GRNBLOOD6,

	PT_BLDDRIP01,
	PT_BLDDRIP02,
	PT_BLDDRIP01_GRN,
	PT_BLDDRIP02_GRN,
	PT_BLDSPURT,
	PT_BLDSPURT2,

	PT_BEAM,

	PT_EXPLOFLASH,
	PT_EXPLOWAVE,

	PT_FLARE,
	PT_FLAREGLOW,

	PT_FLY,

	PT_RAIL_CORE,
	PT_RAIL_WAVE,
	PT_RAIL_SPIRAL,

	PT_SPARK,

	PT_WATERBUBBLE,
	PT_WATERDROPLET,
	PT_WATERIMPACT,
	PT_WATERMIST,
	PT_WATERMIST_GLOW,
	PT_WATERPLUME,
	PT_WATERPLUME_GLOW,
	PT_WATERRING,
	PT_WATERRIPPLE,
	// ----------- ANIMATED -----------
	PT_EXPLO1,
	PT_EXPLO2,
	PT_EXPLO3,
	PT_EXPLO4,
	PT_EXPLO5,
	PT_EXPLO6,
	PT_EXPLO7,

	PT_EXPLOEMBERS1,
	PT_EXPLOEMBERS2,

	// ------------- MAPFX ------------
	MFX_WHITE,
	MFX_CORONA,

	// ------------- TOTAL ------------
	PT_PICTOTAL
};

enum {
	// ------------ DECALS ------------
	DT_BFG_BURNMARK,
	DT_BFG_GLOWMARK,

	DT_BLASTER_BLUEMARK,
	DT_BLASTER_BURNMARK,
	DT_BLASTER_GREENMARK,
	DT_BLASTER_REDMARK,

	DT_DRONE_SPIT_GLOW,

	DT_ENGYREPAIR_BURNMARK,
	DT_ENGYREPAIR_GLOWMARK,

	DT_BLOOD01,
	DT_BLOOD02,
	DT_BLOOD03,
	DT_BLOOD04,
	DT_BLOOD05,
	DT_BLOOD06,
	DT_BLOOD07,
	DT_BLOOD08,
	DT_BLOOD09,
	DT_BLOOD10,
	DT_BLOOD11,
	DT_BLOOD12,
	DT_BLOOD13,
	DT_BLOOD14,
	DT_BLOOD15,
	DT_BLOOD16,

	DT_BLOOD01_GRN,
	DT_BLOOD02_GRN,
	DT_BLOOD03_GRN,
	DT_BLOOD04_GRN,
	DT_BLOOD05_GRN,
	DT_BLOOD06_GRN,
	DT_BLOOD07_GRN,
	DT_BLOOD08_GRN,
	DT_BLOOD09_GRN,
	DT_BLOOD10_GRN,
	DT_BLOOD11_GRN,
	DT_BLOOD12_GRN,
	DT_BLOOD13_GRN,
	DT_BLOOD14_GRN,
	DT_BLOOD15_GRN,
	DT_BLOOD16_GRN,

	DT_BULLET,

	DT_EXPLOMARK,
	DT_EXPLOMARK2,
	DT_EXPLOMARK3,

	DT_RAIL_BURNMARK,
	DT_RAIL_GLOWMARK,
	DT_RAIL_WHITE,

	DT_SLASH,
	DT_SLASH2,
	DT_SLASH3,
	// ------------- TOTAL ------------
	DT_PICTOTAL
};

extern vec3_t	cg_randVels[NUMVERTEXNORMALS];

/*
=============================================================================

	SCRIPTED MAP EFFECTS

=============================================================================
*/

void	CG_AddMapFXToList (void);

void	CG_MapFXLoad (char *mapName);
void	CG_MapFXClear (void);

void	CG_MapFXInit (void);
void	CG_MapFXShutdown (void);

/*
=============================================================================

	DECAL SYSTEM

=============================================================================
*/

typedef struct cgDecal_s {
	struct cgDecal_s	*prev;
	struct cgDecal_s	*next;

	refDecal_t			refDecal;

	float				time;

	vec4_t				color;
	vec4_t				colorVel;

	float				size;

	float				lifeTime;

	struct				shader_s *shader;
	uint32				flags;

	void				(*think)(struct cgDecal_s *d, vec4_t color, int *type, uint32 *flags);
	qBool				thinkNext;
} cgDecal_t;

enum {
	DF_USE_BURNLIFE	= 1 << 0,
	DF_FIXED_LIFE	= 1 << 1,
	DF_ALPHACOLOR	= 1 << 2,
};

cgDecal_t *CG_SpawnDecal(float org0,				float org1,					float org2,
						float dir0,					float dir1,					float dir2,
						float red,					float green,				float blue,
						float redVel,				float greenVel,				float blueVel,
						float alpha,				float alphaVel,
						float size,
						int type,					uint32 flags,
						void (*think)(struct cgDecal_s *d, vec4_t color, int *type, uint32 *flags),
						qBool thinkNext,
						float lifeTime,				float angle);

// constants
#define DECAL_INSTANT	-10000.0f

// random texturing
int dRandBloodMark (void);
int dRandGrnBloodMark (void);
int dRandExploMark (void);
int dRandSlashMark (void);

// management
void	CG_ClearDecals (void);
void	CG_AddDecals (void);

/*
=============================================================================

	PARTICLE SYSTEM

=============================================================================
*/

enum {
	PF_SCALED		= 1 << 0,

	PF_SHADE		= 1 << 1,
	PF_GRAVITY		= 1 << 2,
	PF_NOCLOSECULL	= 1 << 3,
	PF_NODECAL		= 1 << 4,
	PF_NOSFX		= 1 << 5,
	PF_ALPHACOLOR	= 1 << 6,

	PF_AIRONLY		= 1 << 7,
	PF_LAVAONLY		= 1 << 8,
	PF_SLIMEONLY	= 1 << 9,
	PF_WATERONLY	= 1 << 10,

	// Special
	PF_GREENBLOOD	= 1 << 11,
};

enum {
	PART_STYLE_QUAD,

	PART_STYLE_ANGLED,
	PART_STYLE_BEAM,
	PART_STYLE_DIRECTION
};

typedef struct cgParticle_s {
	struct cgParticle_s	*prev;
	struct cgParticle_s	*next;

	float				time;

	vec3_t				org;
	vec3_t				oldOrigin;

	vec3_t				angle;
	vec3_t				vel;
	vec3_t				accel;

	vec4_t				color;
	vec4_t				colorVel;

	float				size;
	float				sizeVel;

	struct				shader_s *shader;
	byte				style;
	uint32				flags;

	float				orient;

	void				(*think)(struct cgParticle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time);
	qBool				thinkNext;

	// Passed to refresh
	refPoly_t			outPoly;
	bvec4_t				outColor[4];
	vec2_t				outCoords[4];
	vec3_t				outVertices[4];
} cgParticle_t;

void	CG_SpawnParticle (float org0,					float org1,					float org2,
						float angle0,					float angle1,				float angle2,
						float vel0,						float vel1,					float vel2,
						float accel0,					float accel1,				float accel2,
						float red,						float green,				float blue,
						float redVel,					float greenVel,				float blueVel,
						float alpha,					float alphaVel,
						float size,						float sizeVel,
						uint32 type,					uint32 flags,
						void (*think)(struct cgParticle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time),
						qBool thinkNext,
						byte style,
						float orient);

// constants
#define PMAXBLDDRIPLEN	3.25f
#define PMAXSPLASHLEN	2.0f

#define PART_GRAVITY	110
#define PART_INSTANT	-1000.0f

#define BEAMLENGTH		16

// random texturing
int		pRandBloodDrip (void);
int		pRandGrnBloodDrip (void);
int		pRandBloodMark (void);
int		pRandGrnBloodMark (void);
int		pRandSmoke (void);
int		pRandGlowSmoke (void);
int		pRandEmbers (void);
int		pRandFire (void);

// management
void	CG_ClearParticles (void);
void	CG_AddParticles (void);

//
// GENERIC EFFECTS
//

void	CG_BlasterBlueParticles (vec3_t org, vec3_t dir);
void	CG_BlasterGoldParticles (vec3_t org, vec3_t dir);
void	CG_BlasterGreenParticles (vec3_t org, vec3_t dir);
void	CG_BlasterGreyParticles (vec3_t org, vec3_t dir);
void	CG_BleedEffect (vec3_t org, vec3_t dir, int count);
void	CG_BleedGreenEffect (vec3_t org, vec3_t dir, int count);
void	CG_BubbleEffect (vec3_t origin);
void	CG_ExplosionBFGEffect (vec3_t org);
void	__fastcall CG_FlareEffect (vec3_t origin, int type, float orient, float size, float sizevel, int color, int colorvel, float alpha, float alphavel);
void	CG_ItemRespawnEffect (vec3_t org);
void	CG_LogoutEffect (vec3_t org, int type);

void	__fastcall CG_ParticleEffect (vec3_t org, vec3_t dir, int color, int count);
void	__fastcall CG_ParticleEffect2 (vec3_t org, vec3_t dir, int color, int count);
void	__fastcall CG_ParticleEffect3 (vec3_t org, vec3_t dir, int color, int count);
void	__fastcall CG_ParticleSmokeEffect (vec3_t org, vec3_t dir, int color, int count, int magnitude);
void	__fastcall CG_RicochetEffect (vec3_t org, vec3_t dir, int count);

void	CG_RocketFireParticles (vec3_t org, vec3_t dir);

void	__fastcall CG_SparkEffect (vec3_t org, vec3_t dir, int color, int colorvel, int count, float smokeScale, float lifeScale);
void	__fastcall CG_SplashParticles (vec3_t org, vec3_t dir, int color, int count, qBool glow);
void	__fastcall CG_SplashEffect (vec3_t org, vec3_t dir, int color, int count);

void	CG_BigTeleportParticles (vec3_t org);
void	CG_BlasterTip (vec3_t start, vec3_t end);
void	CG_ExplosionParticles (vec3_t org, float scale, qBool exploonly, qBool inwater);
void	CG_ExplosionBFGParticles (vec3_t org);
void	CG_ExplosionColorParticles (vec3_t org);
void	CG_FlyEffect (cgEntity_t *ent, vec3_t origin);
void	CG_ForceWall (vec3_t start, vec3_t end, int color);
void	CG_MonsterPlasma_Shell (vec3_t origin);
void	CG_PhalanxTip (vec3_t start, vec3_t end);
void	CG_TeleportParticles (vec3_t org);
void	CG_TeleporterParticles (entityState_t *ent);
void	CG_TrackerShell (vec3_t origin);
void	CG_TrapParticles (refEntity_t *ent);
void	CG_WidowSplash (vec3_t org);

//
// GLOOM EFFECTS
//

void	CG_GloomBlobTip (vec3_t start, vec3_t end);
void	CG_GloomDroneEffect (vec3_t org, vec3_t dir);
void	CG_GloomEmberTrail (vec3_t start, vec3_t end);
void	CG_GloomFlareTrail (vec3_t start, vec3_t end);
void	CG_GloomGasEffect (vec3_t origin);
void	CG_GloomRepairEffect (vec3_t org, vec3_t dir, int count);
void	CG_GloomStingerFire (vec3_t start, vec3_t end, float size, qBool light);

//
// SUSTAINED EFFECTS
//

void	__fastcall CG_ParticleSteamEffect (vec3_t org, vec3_t dir, int color, int count, int magnitude);

void	CG_ParseNuke (void);
void	CG_ParseSteam (void);
void	CG_ParseWidow (void);

void	CG_ClearSustains (void);
void	CG_AddSustains (void);

//
// TRAIL EFFECTS
//

void	__fastcall CG_BeamTrail (vec3_t start, vec3_t end, int color, float size, float alpha, float alphaVel);
void	CG_BfgTrail (refEntity_t *ent);
void	CG_BlasterGoldTrail (vec3_t start, vec3_t end);
void	CG_BlasterGreenTrail (vec3_t start, vec3_t end);
void	CG_BubbleTrail (vec3_t start, vec3_t end);
void	CG_BubbleTrail2 (vec3_t start, vec3_t end, int dist);
void	CG_DebugTrail (vec3_t start, vec3_t end);
void	CG_FlagTrail (vec3_t start, vec3_t end, int flags);
void	CG_GibTrail (vec3_t start, vec3_t end, int flags);
void	CG_GrenadeTrail (vec3_t start, vec3_t end);
void	CG_Heatbeam (vec3_t start, vec3_t forward);
void	CG_IonripperTrail (vec3_t start, vec3_t end);
void	CG_QuadTrail (vec3_t start, vec3_t end);
void	CG_RailTrail (vec3_t start, vec3_t end);
void	CG_RocketTrail (vec3_t start, vec3_t end);
void	CG_TagTrail (vec3_t start, vec3_t end);
void	CG_TrackerTrail (vec3_t start, vec3_t end);

//
// PARTICLE THINK FUNCTIONS
//

void	pBloodDripThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time);
void	pBloodThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time);
void	pBounceThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time);
void	pDropletThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time);
void	pExploAnimThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time);
void	pFastSmokeThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time);
void	pFireThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time);
void	pFireTrailThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time);
void	pFlareThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time);
void	pRailSpiralThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time);
void	pRicochetSparkThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time);
void	pSlowFireThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time);
void	pSmokeThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time);
void	pSparkGrowThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time);
void	pSplashThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time);

/*
=============================================================================

	SUSTAINED PARTICLE EFFECTS

=============================================================================
*/

typedef struct cgSustainPfx_s {
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

	void		(*think)(struct cgSustainPfx_s *self);
} cgSustainPfx_t;
