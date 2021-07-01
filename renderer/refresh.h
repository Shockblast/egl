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

// refresh.h

#ifndef __REFRESH_H__
#define __REFRESH_H__

#define MAX_DLIGHTS			128
#define MAX_ENTITIES		1024
#define MAX_CLENTITIES		(MAX_ENTITIES-256) // leave breathing room for normal entities
#define MAX_LIGHTSTYLES		256

#define MAX_PARTICLES		8192

#define MAX_DECALS			20000
#define MAX_DECAL_VERTS		384
#define MAX_DECAL_FRAGMENTS	256

/*
=============================================================

	ENTITY

=============================================================
*/

typedef struct {
	vec3_t			origin;
	vec3_t			axis[3];
} orientation_t;

typedef struct entity_s {
	struct model_s	*model;			// opaque type outside refresh

	struct image_s	*skin;			// NULL for inline skin
	int				skinNum;		// also used as RF_BEAM's palette index

	vec3_t			axis[3];

	// most recent data
	vec3_t			origin;			// also used as RF_BEAM's "from"
	vec3_t			oldOrigin;		// also used as RF_BEAM's "to"
	int				frame;			// also used as RF_BEAM's diameter
	int				oldFrame;
	float			backLerp;		// 0.0 = current, 1.0 = old

	int				lightStyle;		// for flashing entities
	vec4_t			color;
	int				flags;
	float			scale;

	qBool			muzzleOn;
	int				muzzType;
	qBool			muzzSilenced;
	qBool			muzzVWeap;
} entity_t;

/*
=============================================================

	DLIGHTS

=============================================================
*/

typedef struct {
	vec3_t		origin;
	vec3_t		color;
	float		intensity;
} dLight_t;

typedef struct {
	float		rgb[3];			// 0.0 - 2.0
	float		white;			// highest of rgb
} lightStyle_t;

/*
=============================================================

	IMAGING

=============================================================
*/

enum {
	IT_CUBEMAP	= 1 << 0,
	IT_TEXTURE	= 1 << 1,
	IT_LIGHTMAP	= 1 << 2,

	IF_CLAMP	= 1 << 3,
	IF_NOGAMMA	= 1 << 4,
	IF_NOINTENS	= 1 << 5,
	IF_NOMIPMAP	= 1 << 6,
	IF_NOPICMIP	= 1 << 7,
	IF_NOFLUSH	= 1 << 8
};

typedef struct image_s {
	char			name[MAX_QPATH];			// game path, including extension
	char			bareName[MAX_QPATH];		// filename only, as called when searching

	int				flags,		upFlags;
	int				width,		upWidth;		// source image
	int				height,		upHeight;		// after power of two and picmip

	int				format;						// uploaded texture color components

	int				iRegistrationFrame;			// 0 = free
	int				texNum;						// gl texture binding, img_NumTextures + 1, can't be 0

	struct			mSurface_s	*textureChain;	// for sort-by-texture world drawing
} image_t;

/*
=============================================================

	SHADERS

=============================================================
*/

enum {
	STAGE_STATIC,
	STAGE_SINE,
	STAGE_COSINE
};

// ANIMATION LOOP
typedef struct shaderAnimStage_s {
	image_t						*texture;			// texture
	char						name[MAX_QPATH];	// texture name
	struct shaderAnimStage_s	*next;				// next anim stage
} shaderAnimStage_t;

// BLENDING
typedef struct {
	int							source;				// source blend value
	int							dest;				// dest blend value
	qBool						blend;				// are we going to blend?
} shaderBlendFunc_t;

// ALPHA SHIFTING
typedef struct {
	float						min;				// min/max alpha values
	float						max;				// min/max alpha values
	float						speed;				// shifting speed
} shaderAlphaShift_t;

// SCALING
typedef struct {
	char						typeX;				// scale types
	char						typeY;				// scale types
	float						scaleX;				// scaling factors
	float						scaleY;				// scaling factors
} shaderScale_t;

// SCROLLING
typedef struct {
	char						typeX;				// scroll types
	char						typeY;				// scroll types
	float						speedX;				// speed of scroll
	float						speedY;				// speed of scroll
} shaderScroll_t;

// SCRIPT STAGE
typedef struct shaderStage_s {
	image_t						*texture;				// texture
	char						name[MAX_QPATH];		// texture name
	int							flags;					// texture flags
	
	shaderAnimStage_t			*animStage;				// first animation stage
	float						animDelay;				// Delay between anim frames
	char						animCount;				// number of animation frames
	float						animTimeLast;			// gametime of last frame change
	shaderAnimStage_t			*animLast;				// pointer to last anim

	shaderBlendFunc_t			blendFunc;				// image blending
	shaderAlphaShift_t			alphaShift;				// alpha shifting
	shaderScroll_t				scroll;					// tcmod
	shaderScale_t				scale;					// tcmod

	float						rotateSpeed;			// rotate speed (0 for no rotate);

	qBool						cubeMap;				// cube mapping
	qBool						envMap;					// fake envmapping
	qBool						lightMap;				// lightmap this stage?
	qBool						alphaMask;				// alpha masking?
	qBool						lightMapOnly;			// $lightmap

	struct shaderStage_s		*next;					// next stage
} shaderStage_t;

typedef struct rs_stagekey_s {
	char		*stage;
	void		(*func)(shaderStage_t *shader, char **token);
} shaderStageKey_t;

// BASE SCRIPT
typedef struct shader_s {
	char						fileName[MAX_QPATH];	// script filename
	char						name[MAX_QPATH];		// shader name
	
	unsigned char				subDivide;				// Heffo - chop the surface up this much for vertex warping
	float						warpDist;				// Heffo - vertex warping distance;
	float						warpSmooth;				// Heffo - vertex warping smoothness;
	float						warpSpeed;				// Heffo - vertex warping speed;

	qBool						noFlush;				// don't flush from memory on map change
	qBool						noMark;					// don't touch me with your filthy decals
	qBool						noShadow;				// entity-only option, no shadow
	qBool						noShell;				// entity-only option, no god/quad/etc shell

	qBool						ready;					// readied by the engine?
	shaderStage_t				*stage;					// first rendering stage
	struct shader_s				*next;					// next shader in linked list
} shader_t;

typedef struct shaderBaseKey_s {
	char		*shader;
	void		(*func)(shader_t *shader, char **token);
} shaderBaseKey_t;

/*
=============================================================

	EFFECTS

=============================================================
*/

enum {
	PF_GRAVITY		= 1 << 0,
	PF_NODECAL		= 1 << 1,
	PF_SHADE		= 1 << 2,
	PF_ALPHACOLOR	= 1 << 3,

	PF_SCALED		= 1 << 4,
	PF_DEPTHHACK	= 1 << 5,

	PF_DIRECTION	= 1 << 6,
	PF_BEAM			= 1 << 7,
	PF_SPIRAL		= 1 << 8,

	PF_AIRONLY		= 1 << 9,
	PF_LAVAONLY		= 1 << 10,
	PF_SLIMEONLY	= 1 << 11,
	PF_WATERONLY	= 1 << 12
};

enum {
	DF_NOTIMESCALE	= 1 << 0,

	DF_ALPHACOLOR	= 1 << 3,

	DF_AIRONLY		= 1 << 9,
	DF_LAVAONLY		= 1 << 10,
	DF_SLIMEONLY	= 1 << 11,
	DF_WATERONLY	= 1 << 12
};

typedef struct {
	int				firstVert;
	int				numVerts;

	struct	mNode_s	*node;
} fragment_t;

typedef struct {
	int				numVerts;

	vec2_t			*coords;
	vec3_t			*verts;

	struct mNode_s	*node;

	dvec4_t			color;

	image_t			*image;
	int				flags;

	int				sFactor;
	int				dFactor;
} decal_t;

typedef struct {
	vec3_t			origin;
	vec3_t			angle;

	dvec4_t			color;

	image_t			*image;
	int				flags;

	float			size;

	int				sFactor;
	int				dFactor;

	float			orient;
} particle_t;

/*
=============================================================

	REFRESH DEFINITION

=============================================================
*/

typedef struct {
	int			x;
	int			y;
	int			width;
	int			height;
} vRect_t;

typedef struct {
	int			x, y;
	int			width, height;

	float		fovX, fovY;
	vec3_t		viewOrigin;
	vec3_t		viewAngles;
	vec3_t		viewAxis[3];
	vec4_t		vBlend;				// rgba 0-1 full screen blend
	float		time;				// time is used to auto animate
	int			rdFlags;			// RDF_UNDERWATER, etc

	byte		*areaBits;			// if not NULL, only areas with set bits will be drawn
} refDef_t;

typedef struct {
	int			width;
	int			height;
} vidDef_t;

extern	vidDef_t	vidDef;

//
// r_main.c
//

void	R_ClearFrame (void);

void	R_AddEntity (entity_t *ent);
void	R_AddParticle (vec3_t org, vec3_t angle, dvec4_t color, double size, image_t *image, int flags, int sFactor, int dFactor, double orient);
void	R_AddDecal (vec2_t *coords, vec3_t *verts, int numVerts, struct mNode_s *node, dvec4_t color, image_t *image, int flags, int sFactor, int dFactor);
void	R_AddLight (vec3_t org, float intensity, float r, float g, float b);
void	R_AddLightStyle (int style, float r, float g, float b);

#endif // __REFRESH_H__
