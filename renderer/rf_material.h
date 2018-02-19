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
// rf_material.h
//

/*
=============================================================================

	MATERIALS

=============================================================================
*/

#define MAX_MATERIALS				4096
#define MAX_MATERIAL_DEFORMVS		8
#define MAX_MATERIAL_PASSES			8
#define MAX_MATERIAL_ANIM_FRAMES	16
#define MAX_MATERIAL_TCMODS			8

// Material pass flags
enum { // matPassFlags_t
    MAT_PASS_ANIMMAP			= 1 << 0,
    MAT_PASS_BLEND				= 1 << 1,
	MAT_PASS_CUBEMAP			= 1 << 2,
	MAT_PASS_DEPTHWRITE			= 1 << 3,
	MAT_PASS_DETAIL				= 1 << 4,
	MAT_PASS_NOTDETAIL			= 1 << 5,
	MAT_PASS_DLIGHT				= 1 << 6,
    MAT_PASS_LIGHTMAP			= 1 << 7,
	MAT_PASS_NOCOLORARRAY		= 1 << 8,
	MAT_PASS_FRAGMENTPROGRAM	= 1 << 9,
	MAT_PASS_VERTEXPROGRAM		= 1 << 10,
};

// Material pass alphaFunc functions
enum { // matPassAlphaFunc_t
	ALPHA_FUNC_NONE,
	ALPHA_FUNC_GT0,
	ALPHA_FUNC_LT128,
	ALPHA_FUNC_GE128
};

// Material pass tcGen functions
enum { // matPassTcGen_t
	TC_GEN_BAD,

	TC_GEN_BASE,
	TC_GEN_LIGHTMAP,
	TC_GEN_ENVIRONMENT,
	TC_GEN_VECTOR,
	TC_GEN_REFLECTION,
	TC_GEN_WARP,

	// The following are used internally ONLY!
	TC_GEN_DLIGHT,
	TC_GEN_FOG
};

// Periodic functions
enum { // matTableFunc_t
    MAT_FUNC_SIN				= 1,
    MAT_FUNC_TRIANGLE			= 2,
    MAT_FUNC_SQUARE				= 3,
    MAT_FUNC_SAWTOOTH			= 4,
    MAT_FUNC_INVERSESAWTOOTH	= 5,
	MAT_FUNC_NOISE				= 6,
	MAT_FUNC_CONSTANT			= 7
};

typedef struct materialFunc_s {
    matTableFunc_t	type;			// MAT_FUNC enum
    float			args[4];		// offset, amplitude, phase_offset, rate
} materialFunc_t;

// Material pass tcMod functions
enum { // matPassTcMod_t
	TC_MOD_NONE,
	TC_MOD_SCALE,
	TC_MOD_SCROLL,
	TC_MOD_ROTATE,
	TC_MOD_TRANSFORM,
	TC_MOD_TURB,
	TC_MOD_STRETCH
};

typedef struct tcMod_s {
	matPassTcMod_t	type;
	float			args[6];
} tcMod_t;

// Material pass rgbGen functions
enum { // matPassRGBGen_t
	RGB_GEN_UNKNOWN,
	RGB_GEN_IDENTITY,
	RGB_GEN_IDENTITY_LIGHTING,
	RGB_GEN_CONST,
	RGB_GEN_COLORWAVE,
	RGB_GEN_ENTITY,
	RGB_GEN_ONE_MINUS_ENTITY,
	RGB_GEN_EXACT_VERTEX,
	RGB_GEN_VERTEX,
	RGB_GEN_ONE_MINUS_VERTEX,
	RGB_GEN_ONE_MINUS_EXACT_VERTEX,
	RGB_GEN_LIGHTING_DIFFUSE,
	RGB_GEN_FOG
};

typedef struct rgbGen_s {
	matPassRGBGen_t		type;
	float				fArgs[3];
	byte				bArgs[3];
    materialFunc_t		func;
} rgbGen_t;

// Material pass alphaGen functions
enum { // matPassAlphaGen_t
	ALPHA_GEN_UNKNOWN,
	ALPHA_GEN_IDENTITY,
	ALPHA_GEN_CONST,
	ALPHA_GEN_PORTAL,
	ALPHA_GEN_VERTEX,
	ALPHA_GEN_ONE_MINUS_VERTEX,
	ALPHA_GEN_ENTITY,
	ALPHA_GEN_SPECULAR,
	ALPHA_GEN_WAVE,
	ALPHA_GEN_DOT,
	ALPHA_GEN_ONE_MINUS_DOT,
	ALPHA_GEN_FOG
};

typedef struct alphaGen_s {
	matPassAlphaGen_t	type;
	float				args[2];
    materialFunc_t		func;
} alphaGen_t;

//
// Material passes
//
typedef struct matPass_s {
	int							animFPS;

	// For material registration
	byte						animNumImages;
	image_t						*animImages[MAX_MATERIAL_ANIM_FRAMES];

	// For material creation (static after material is finished)
	byte						animNumNames;
	char						*animNames[MAX_MATERIAL_ANIM_FRAMES];
	texFlags_t					animTexFlags[MAX_MATERIAL_ANIM_FRAMES];
	texFlags_t					passTexFlags;

	program_t					*vertProgPtr;
	char						vertProgName[MAX_QPATH];
	program_t					*fragProgPtr;
	char						fragProgName[MAX_QPATH];

	matPassFlags_t				flags;

	matPassAlphaFunc_t			alphaFunc;
	alphaGen_t					alphaGen;
	rgbGen_t					rgbGen;
	qBool						canAccumulate;

	matPassTcGen_t				tcGen;
	vec4_t						tcGenVec[2];

	byte						numTCMods;
	tcMod_t						*tcMods;

	byte						totalMask;
	qBool						maskRed;
	qBool						maskGreen;
	qBool						maskBlue;
	qBool						maskAlpha;

	uint32						blendSource;
	uint32						blendDest;
	uint32						blendMode;
	uint32						depthFunc;

	uint32						stateBits1;
} matPass_t;

// Material path types
enum { // matPathType_t
	MAT_PATHTYPE_INTERNAL,
	MAT_PATHTYPE_BASEDIR,
	MAT_PATHTYPE_MODDIR
};

// Material registration types
enum { // matRegType_t
	MAT_RT_ALIAS,
	MAT_RT_BSP,
	MAT_RT_BSP_FLARE,
	MAT_RT_BSP_LM,
	MAT_RT_BSP_VERTEX,
	MAT_RT_PIC,
	MAT_RT_POLY,
	MAT_RT_SKYBOX
};

// Material flags
enum { // matBaseFlags_t
	MAT_AUTOSPRITE			= 1 << 0,
	MAT_DEFORMV_BULGE		= 1 << 1,
	MAT_DEPTHRANGE			= 1 << 2,
	MAT_DEPTHWRITE			= 1 << 3,
	MAT_ENTITY_MERGABLE		= 1 << 4,
	MAT_FLARE				= 1 << 5,
	MAT_NOFLUSH				= 1 << 6,
	MAT_NOLERP				= 1 << 7,
	MAT_NOMARK				= 1 << 8,
	MAT_NOSHADOW			= 1 << 9,
	MAT_POLYGONOFFSET		= 1 << 10,
	MAT_SKY					= 1 << 11,
	MAT_SUBDIVIDE			= 1 << 12,
};

// Material cull functions
enum { // matCullType_t
	MAT_CULL_FRONT,
	MAT_CULL_BACK,
	MAT_CULL_NONE
};

// Material sortKeys
enum { // matSortKey_t
	MAT_SORT_NONE			= 0,
	MAT_SORT_PORTAL			= 1,
	MAT_SORT_SKY			= 2,
	MAT_SORT_OPAQUE			= 3,
	MAT_SORT_DECAL			= 4,
	MAT_SORT_SEETHROUGH		= 5,
	MAT_SORT_BANNER			= 6,
	MAT_SORT_UNDERWATER		= 7,
	MAT_SORT_ENTITY			= 8,
	MAT_SORT_ENTITY2		= 9,
	MAT_SORT_PARTICLE		= 10,
	MAT_SORT_WATER			= 11,
	MAT_SORT_ADDITIVE		= 12,
	MAT_SORT_NEAREST		= 13,
	MAT_SORT_POSTPROCESS	= 14,

	MAT_SORT_MAX
};

// Material surfParam flags
enum { // matSurfParams_t
	MAT_SURF_TRANS33		= 1 << 0,
	MAT_SURF_TRANS66		= 1 << 1,
	MAT_SURF_WARP			= 1 << 2,
	MAT_SURF_FLOWING		= 1 << 3,
	MAT_SURF_LIGHTMAP		= 1 << 4
};

// Material vertice deformation functions
enum { // matDeformvType_t
	DEFORMV_NONE,
	DEFORMV_WAVE,
	DEFORMV_NORMAL,
	DEFORMV_BULGE,
	DEFORMV_MOVE,
	DEFORMV_AUTOSPRITE,
	DEFORMV_AUTOSPRITE2,
	DEFORMV_PROJECTION_SHADOW,
	DEFORMV_AUTOPARTICLE
};

typedef struct vertDeform_s {
	matDeformvType_t	type;
	float				args[4];
	materialFunc_t		func;
} vertDeform_t;

//
// Base material structure
//
typedef struct material_s {
	char						name[MAX_QPATH];		// material name
	matBaseFlags_t				flags;
	matPathType_t				pathType;				// gameDir > baseDir > internal

	matPassFlags_t				addPassFlags;			// add these to all passes before completion
	texFlags_t					addTexFlags;			// add these to all passes before registration

	int							sizeBase;				// used for texcoord generation and image size lookup function
	uint32						touchFrame;				// touch if this matches the current ri.reg.registerFrame
	matSurfParams_t				surfParams;
	meshFeatures_t				features;
	matSortKey_t				sortKey;

	int							numPasses;
	matPass_t					*passes;

	int							numDeforms;
	vertDeform_t				*deforms;

	bvec4_t						fogColor;
	double						fogDist;

	matCullType_t				cullType;
	int16						subdivide;

	float						depthNear;
	float						depthFar;

	uint32						hashValue;
	struct material_s			*hashNext;
} material_t;

//
// rf_material.c
//

extern material_t	*r_cinMaterial;
extern material_t	*r_noMaterial;
extern material_t	*r_noMaterialLightmap;
extern material_t	*r_noMaterialSky;
extern material_t	*r_whiteMaterial;
extern material_t	*r_blackMaterial;

void		R_EndMaterialRegistration (void);

material_t	*R_RegisterFlare (char *name);
material_t	*R_RegisterSky (char *name);
material_t	*R_RegisterTexture (char *name, matSurfParams_t surfParams);
material_t	*R_RegisterTextureLM (char *name);
material_t	*R_RegisterTextureVertex (char *name);

void		R_MaterialInit (void);
void		R_MaterialShutdown (void);
