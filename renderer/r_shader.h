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
// r_shader.h
//

/*
=============================================================================

	SHADERS

=============================================================================
*/

#define MAX_SHADERS					4096
#define MAX_SHADER_DEFORMVS			8
#define MAX_SHADER_PASSES			8
#define MAX_SHADER_ANIM_FRAMES		16
#define MAX_SHADER_TCMODS			8

// Shader pass flags
enum { // shPassFlags_t
    SHADER_PASS_ANIMMAP			= 1 << 0,
    SHADER_PASS_BLEND			= 1 << 1,
	SHADER_PASS_CUBEMAP			= 1 << 2,
	SHADER_PASS_DEPTHWRITE		= 1 << 3,
	SHADER_PASS_DETAIL			= 1 << 4,
	SHADER_PASS_NOTDETAIL		= 1 << 5,
	SHADER_PASS_DLIGHT			= 1 << 6,
    SHADER_PASS_LIGHTMAP		= 1 << 7,
	SHADER_PASS_NOCOLORARRAY	= 1 << 8,
	SHADER_PASS_FRAGMENTPROGRAM	= 1 << 9,
	SHADER_PASS_VERTEXPROGRAM	= 1 << 10,
};

// Shader pass alphaFunc functions
enum { // shPassAlphaFunc_t
	ALPHA_FUNC_NONE,
	ALPHA_FUNC_GT0,
	ALPHA_FUNC_LT128,
	ALPHA_FUNC_GE128
};

// Shader pass tcGen functions
enum { // shPassTcGen_t
	TC_GEN_BASE,
	TC_GEN_LIGHTMAP,
	TC_GEN_ENVIRONMENT,
	TC_GEN_VECTOR,
	TC_GEN_REFLECTION,
	TC_GEN_WARP,
	TC_GEN_FOG
};

// Periodic functions
enum { // shTableFunc_t
    SHADER_FUNC_SIN             = 1,
    SHADER_FUNC_TRIANGLE        = 2,
    SHADER_FUNC_SQUARE          = 3,
    SHADER_FUNC_SAWTOOTH        = 4,
    SHADER_FUNC_INVERSESAWTOOTH = 5,
	SHADER_FUNC_NOISE			= 6,
	SHADER_FUNC_CONSTANT		= 7
};

typedef struct shaderFunc_s {
    shTableFunc_t	type;			// SHADER_FUNC enum
    float			args[4];		// offset, amplitude, phase_offset, rate
} shaderFunc_t;

// Shader pass tcMod functions
enum { // shPassTcMod_t
	TC_MOD_NONE,
	TC_MOD_SCALE,
	TC_MOD_SCROLL,
	TC_MOD_ROTATE,
	TC_MOD_TRANSFORM,
	TC_MOD_TURB,
	TC_MOD_STRETCH
};

typedef struct tcMod_s {
	shPassTcMod_t	type;
	float			args[6];
} tcMod_t;

// Shader pass rgbGen functions
enum { // shPassRGBGen_t
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
	shPassRGBGen_t		type;
	float				args[3];
    shaderFunc_t		func;
} rgbGen_t;

// Shader pass alphaGen functions
enum { // shPassAlphaGen_t
	ALPHA_GEN_UNKNOWN,
	ALPHA_GEN_IDENTITY,
	ALPHA_GEN_CONST,
	ALPHA_GEN_PORTAL,
	ALPHA_GEN_VERTEX,
	ALPHA_GEN_ENTITY,
	ALPHA_GEN_SPECULAR,
	ALPHA_GEN_WAVE,
	ALPHA_GEN_DOT,
	ALPHA_GEN_ONE_MINUS_DOT,
	ALPHA_GEN_FOG
};

typedef struct alphaGen_s {
	shPassAlphaGen_t	type;
	float				args[2];
    shaderFunc_t		func;
} alphaGen_t;

//
// Shader passes
//
typedef struct shaderPass_s {
	int							animFPS;
	byte						animNumFrames;
	image_t						*animFrames[MAX_SHADER_ANIM_FRAMES];
	char						names[MAX_SHADER_ANIM_FRAMES][MAX_QPATH];
	byte						numNames;
	int							texFlags;

	program_t					*vertProg;
	char						vertProgName[MAX_QPATH];
	program_t					*fragProg;
	char						fragProgName[MAX_QPATH];

	shPassFlags_t				flags;

	shPassAlphaFunc_t			alphaFunc;
	alphaGen_t					alphaGen;
	rgbGen_t					rgbGen;

	shPassTcGen_t				tcGen;
	vec4_t						tcGenVec[2];

	byte						numTCMods;
	tcMod_t						*tcMods;

	int							totalMask;
	qBool						maskRed;
	qBool						maskGreen;
	qBool						maskBlue;
	qBool						maskAlpha;

	uint32						blendSource;
	uint32						blendDest;
	uint32						blendMode;
	uint32						depthFunc;
} shaderPass_t;

// Shader path types
enum { // shPathType_t
	SHADER_PATHTYPE_INTERNAL,
	SHADER_PATHTYPE_BASEDIR,
	SHADER_PATHTYPE_MODDIR
};

// Shader registration types
enum { // shRegType_t
	SHADER_ALIAS,
	SHADER_BSP,
	SHADER_BSP_FLARE,
	SHADER_BSP_LM,
	SHADER_BSP_VERTEX,
	SHADER_PIC,
	SHADER_POLY,
	SHADER_SKYBOX
};

// Shader flags
enum { // shBaseFlags_t
	SHADER_DEFORMV_BULGE		= 1 << 0,
	SHADER_DEPTHRANGE			= 1 << 1,
	SHADER_DEPTHWRITE			= 1 << 2,
	SHADER_FLARE				= 1 << 3,
	SHADER_NOFLUSH				= 1 << 4,
	SHADER_NOLERP				= 1 << 5,
	SHADER_NOMARK				= 1 << 6,
	SHADER_NOSHADOW				= 1 << 7,
	SHADER_POLYGONOFFSET		= 1 << 8,
	SHADER_SUBDIVIDE			= 1 << 9,
	SHADER_ENTITY_MERGABLE		= 1 << 10,
	SHADER_AUTOSPRITE			= 1 << 11,
	SHADER_SKY					= 1 << 12
};

// Shader cull functions
enum { // shCullType_t
	SHADER_CULL_FRONT,
	SHADER_CULL_BACK,
	SHADER_CULL_NONE
};

// Shader sortKeys
enum { // shSortKey_t
	SHADER_SORT_NONE			= 0,
	SHADER_SORT_PORTAL			= 1,
	SHADER_SORT_SKY				= 2,
	SHADER_SORT_OPAQUE			= 3,
	SHADER_SORT_DECAL			= 4,
	SHADER_SORT_SEETHROUGH		= 5,
	SHADER_SORT_BANNER			= 6,
	SHADER_SORT_UNDERWATER		= 7,
	SHADER_SORT_ENTITY			= 8,
	SHADER_SORT_PARTICLE		= 9,
	SHADER_SORT_WATER			= 10,
	SHADER_SORT_ADDITIVE		= 11,
	SHADER_SORT_NEAREST			= 13,
	SHADER_SORT_POSTPROCESS		= 14,

	SHADER_SORT_MAX
};

// Shader surfParam flags
enum { // shSurfParams_t
	SHADER_SURF_TRANS33			= 1 << 0,
	SHADER_SURF_TRANS66			= 1 << 1,
	SHADER_SURF_WARP			= 1 << 2,
	SHADER_SURF_FLOWING			= 1 << 3,
	SHADER_SURF_LIGHTMAP		= 1 << 4
};

// Shader vertice deformation functions
enum { // shDeformvType_t
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
	shDeformvType_t	type;
	float			args[4];
	shaderFunc_t	func;
} vertDeform_t;

//
// Base shader structure
//
typedef struct shader_s {
	char						name[MAX_QPATH];		// shader name
	shBaseFlags_t				flags;
	shPathType_t				pathType;				// gameDir > baseDir > internal

	shPassFlags_t				addPassFlags;			// add these to all passes before completion
	int							addTexFlags;			// add these to all passes before registration

	int							sizeBase;				// used for texcoord generation and image size lookup function
	uint32						touchFrame;				// touch if this matches the current r_refRegInfo.registerFrame
	shSurfParams_t				surfParams;
	meshFeatures_t				features;
	shSortKey_t					sortKey;

	int							numPasses;
	shaderPass_t				*passes;

	int							numDeforms;
	vertDeform_t				*deforms;

	bvec4_t						fogColor;
	double						fogDist;

	shCullType_t				cullType;
	int16						subdivide;

	float						offsetFactor;			// these are used for polygonOffset
	float						offsetUnits;

	float						depthNear;
	float						depthFar;

	uint32						hashValue;
	struct shader_s				*hashNext;
} shader_t;

//
// r_shader.c
//

extern shader_t	*r_noShader;
extern shader_t *r_noShaderLightmap;
extern shader_t	*r_noShaderSky;
extern shader_t	*r_whiteShader;
extern shader_t	*r_blackShader;

void		R_EndShaderRegistration (void);

shader_t	*R_RegisterFlare (char *name);
shader_t	*R_RegisterSky (char *name);
shader_t	*R_RegisterTextureLM (char *name);
shader_t	*R_RegisterTextureVertex (char *name);

void		R_ShaderInit (void);
void		R_ShaderShutdown (void);
