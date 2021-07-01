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
// r_local.h
// Refresh only header file
//

#ifdef _WIN32
# include <windows.h>
#endif

#include <GL/gl.h>
#include <math.h>
#include <stdio.h>

#include "../common/common.h"
#include "refresh.h"
#include "glext.h"
#include "qgl.h"

#define SHADOW_VOLUMES	2
#define SHADOW_ALPHA	0.5f

/*
=============================================================================

	IMAGING

=============================================================================
*/

enum {
	TEXUNIT0,
	TEXUNIT1,
	TEXUNIT2,
	TEXUNIT3,
	TEXUNIT4,
	TEXUNIT5,
	TEXUNIT6,
	TEXUNIT7,

	MAX_TEXUNITS
};

enum {
	IT_CUBEMAP			= 1 << 0,		// it's a cubemap env base image
	IT_HEIGHTMAP		= 1 << 1,		// heightmap

	IF_CLAMP			= 1 << 2,		// texcoords edge clamped
	IF_NOCOMPRESS		= 1 << 3,		// no texture compression
	IF_NOGAMMA			= 1 << 4,		// not affected by vid_gama
	IF_NOINTENS			= 1 << 5,		// not affected by intensity
	IF_NOMIPMAP_LINEAR	= 1 << 6,		// not mipmapped, linear filtering
	IF_NOMIPMAP_NEAREST	= 1 << 7,		// not mipmapped, nearest filtering
	IF_NOPICMIP			= 1 << 8,		// not affected by gl_picmip
	IF_NOFLUSH			= 1 << 9,		// do not flush at the end of registration (internal only)
	IF_NOALPHA			= 1 << 10,		// force alpha to 255
	IF_NORGB			= 1 << 11		// force rgb to 255 255 255
};

typedef struct image_s {
	char					name[MAX_QPATH];				// game path, including extension
	char					bareName[MAX_QPATH];			// filename only, as called when searching

	int						width,		upWidth;			// source image
	int						height,		upHeight;			// after power of two and picmip
	int						flags;
	float					bumpScale;

	int						tcWidth,	tcHeight;			// special case for high-res texture scaling

	int						target;							// destination for binding
	int						format;							// uploaded texture color components

	qBool					finishedLoading;				// if false, free on the beginning of the next sequence
	uLong					touchFrame;						// free if this doesn't match the current frame
	uInt					texNum;							// gl texture binding, r_numImages + 1, can't be 0

	uLong					hashValue;						// calculated hash value
	struct image_s			*hashNext;						// hash image tree
} image_t;

#define MAX_IMAGES			1024			// maximum local images
#define TEX_LIGHTMAPS		MAX_IMAGES		// start point for lightmaps
#define	MAX_LIGHTMAPS		128				// maximum local lightmap textures

#define	LIGHTMAP_SIZE		128
#define LIGHTMAP_BYTES		4

#define GL_LIGHTMAP_FORMAT	GL_RGBA

extern image_t		r_lmTextures[MAX_LIGHTMAPS];
extern uInt			r_numImages;

extern const char	*sky_NameSuffix[6];
extern const char	*cubeMap_NameSuffix[6];

//
// r_image.c
//

extern image_t		*r_noTexture;
extern image_t		*r_whiteTexture;
extern image_t		*r_blackTexture;
extern image_t		*r_cinTexture;

void	GL_BindTexture (image_t *image);
void	GL_SelectTexture (int target);
void	GL_TextureEnv (GLfloat mode);

void	GL_LoadTexMatrix (mat4x4_t m);
void	GL_LoadIdentityTexMatrix (void);

void	GL_TextureBits (qBool verbose, qBool verboseOnly);
void	GL_TextureMode (qBool verbose, qBool verboseOnly);

void	GL_ResetAnisotropy (void);

void	R_BeginImageRegistration (void);
void	R_EndImageRegistration (void);

image_t	*R_RegisterImage (char *name, int flags, float bumpScale);

void	R_UpdateGammaRamp (void);

void	R_ImageInit (void);
void	R_ImageShutdown (void);

/*
=============================================================================

	FRAGMENT / VERTEX PROGRAMS

=============================================================================
*/

typedef struct program_s {
	char				name[MAX_QPATH];

	uInt				progNum;

	uLong				touchFrame;

	uLong				hashValue;
	struct program_s	*hashNext;
} program_t;

program_t	*R_RegisterProgram (char *name, qBool fragProg);
void		R_EndProgramRegistration (void);

void		R_ProgramInit (void);
void		R_ProgramShutdown (void);

/*
=============================================================================

	SHADERS

=============================================================================
*/

#define MAX_SHADERS					2048
#define MAX_SHADER_PASSES			8
#define MAX_SHADER_DEFORMVS			8
#define MAX_SHADER_ANIM_FRAMES		16
#define MAX_SHADER_TCMODS			8

// Shader type
enum {
	SHADER_PATHTYPE_INTERNAL,
	SHADER_PATHTYPE_BASEDIR,
	SHADER_PATHTYPE_MODDIR
};

// Shader flags
enum {
	SHADER_DEPTHRANGE			= 1 << 0,
	SHADER_DEPTHWRITE			= 1 << 1,
	SHADER_NOMARK				= 1 << 2,
	SHADER_NOSHADOW				= 1 << 3,
	SHADER_POLYGONOFFSET		= 1 << 4,
	SHADER_SUBDIVIDE			= 1 << 5,
	SHADER_ENTITY_MERGABLE		= 1 << 6,
	SHADER_AUTOSPRITE			= 1 << 7
};

// Shader cull functions
enum {
	SHADER_CULL_FRONT,
	SHADER_CULL_BACK,
	SHADER_CULL_NONE
};

// Shader pass flags
enum {
    SHADER_PASS_LIGHTMAP		= 1 << 0,
    SHADER_PASS_BLEND			= 1 << 1,
    SHADER_PASS_ANIMMAP			= 1 << 2,
	SHADER_PASS_DETAIL			= 1 << 3,
	SHADER_PASS_NOCOLORARRAY	= 1 << 4,
	SHADER_PASS_CUBEMAP			= 1 << 5,
	SHADER_PASS_VERTEXPROGRAM	= 1 << 6,
	SHADER_PASS_FRAGMENTPROGRAM	= 1 << 7
};	

// Transform functions
enum {
    SHADER_FUNC_SIN             = 1,
    SHADER_FUNC_TRIANGLE        = 2,
    SHADER_FUNC_SQUARE          = 3,
    SHADER_FUNC_SAWTOOTH        = 4,
    SHADER_FUNC_INVERSESAWTOOTH = 5,
	SHADER_FUNC_NOISE			= 6,
	SHADER_FUNC_CONSTANT		= 7
};

// Shader sortKeys
enum {
	SHADER_SORT_SKY			= 0,
	SHADER_SORT_OPAQUE		= 1,
	SHADER_SORT_DECAL		= 2,
	SHADER_SORT_SEETHROUGH	= 3,
	SHADER_SORT_BANNER		= 4,
	SHADER_SORT_UNDERWATER	= 5,
	SHADER_SORT_ENTITY		= 6,
	SHADER_SORT_PARTICLE	= 7,
	SHADER_SORT_WATER		= 8,
	SHADER_SORT_ADDITIVE	= 9,
	SHADER_SORT_NEAREST		= 13,
	SHADER_SORT_POSTPROCESS	= 14,

	SHADER_SORT_MAX
};

// Surfflag flags
enum {
	SHADER_SURF_TRANS33			= 1 << 0,
	SHADER_SURF_TRANS66			= 1 << 1,
	SHADER_SURF_WARP			= 1 << 2,
	SHADER_SURF_FLOWING			= 1 << 3,
	SHADER_SURF_LIGHTMAP		= 1 << 4
};

// RedGreenBlue pal generation
enum {
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
	RGB_GEN_LIGHTING_DIFFUSE
};

// alpha channel generation
enum {
	ALPHA_GEN_UNKNOWN,
	ALPHA_GEN_IDENTITY,
	ALPHA_GEN_CONST,
	ALPHA_GEN_PORTAL,
	ALPHA_GEN_VERTEX,
	ALPHA_GEN_ENTITY,
	ALPHA_GEN_SPECULAR,
	ALPHA_GEN_WAVE,
	ALPHA_GEN_DOT,
	ALPHA_GEN_ONE_MINUS_DOT
};

// AlphaFunc
enum {
	ALPHA_FUNC_NONE,
	ALPHA_FUNC_GT0,
	ALPHA_FUNC_LT128,
	ALPHA_FUNC_GE128
};

// texture coordinates generation
enum {
	TC_GEN_BASE,
	TC_GEN_LIGHTMAP,
	TC_GEN_ENVIRONMENT,
	TC_GEN_VECTOR,
	TC_GEN_REFLECTION,
	TC_GEN_WARP
};

// tcmod functions
enum {
	TC_MOD_NONE,
	TC_MOD_SCALE,
	TC_MOD_SCROLL,
	TC_MOD_ROTATE,
	TC_MOD_TRANSFORM,
	TC_MOD_TURB,
	TC_MOD_STRETCH
};

// vertices deformation
enum {
	DEFORMV_NONE,
	DEFORMV_WAVE,
	DEFORMV_NORMAL,
	DEFORMV_MOVE,
	DEFORMV_AUTOSPRITE,
	DEFORMV_AUTOSPRITE2,
	DEFORMV_PROJECTION_SHADOW,
	DEFORMV_AUTOPARTICLE
};

// registration types
enum {
	SHADER_ALIAS,
	SHADER_BSP,
	SHADER_PIC,
	SHADER_POLY,
	SHADER_SKYBOX
};

// Periodic functions
typedef struct shaderFunc_s {
    int				type;			// SHADER_FUNC enum
    float			args[4];		// offset, amplitude, phase_offset, rate
} shaderFunc_t;

typedef struct tcMod_s {
	int				type;
	float			args[6];
} tcMod_t;

typedef struct rgbGen_s {
	int				type;
	float			args[3];
    shaderFunc_t	func;
} rgbGen_t;

typedef struct alphaGen_s {
	int				type;
	float			args[2];
    shaderFunc_t	func;
} alphaGen_t;

typedef struct vertDeform_s {
	int				type;
	float			args[4];
	shaderFunc_t	func;
} vertDeform_t;

//
// pass
//
typedef struct shaderPass_s {
	int							animFPS;
	int							animNumFrames;
	image_t						*animFrames[MAX_SHADER_ANIM_FRAMES];
	char						names[MAX_SHADER_ANIM_FRAMES][MAX_QPATH];
	int							numNames;
	int							texFlags;

	program_t					*vertProg;
	char						vertProgName[MAX_QPATH];
	program_t					*fragProg;
	char						fragProgName[MAX_QPATH];

	int							flags;
	float						bumpScale;

	alphaGen_t					alphaGen;
	rgbGen_t					rgbGen;

	int							tcGen;
	vec4_t						tcGenVec[2];

	int							numTCMods;
	tcMod_t						*tcMods;

	int							totalMask;
	qBool						maskRed;
	qBool						maskGreen;
	qBool						maskBlue;
	qBool						maskAlpha;

	uInt						alphaFunc;
	uInt						blendSource;
	uInt						blendDest;
	uInt						blendMode;
	uInt						depthFunc;
} shaderPass_t;

//
// base shader
//
typedef struct shader_s {
	char						name[MAX_QPATH];		// shader name
	int							flags;
	byte						pathType;				// gameDir > baseDir > internal

	int							sizeBase;				// used for texcoord generation and image size lookup function
	uLong						touchFrame;				// free if this doesn't match the current frame
	int							surfParams;
	int							features;
	int							sortKey;

	int							numPasses;
	shaderPass_t				*passes;

	int							numDeforms;
	vertDeform_t				*deforms;

	byte						cullType;
	short						subdivide;

	float						offsetFactor;			// these are used for polygonOffset
	float						offsetUnits;

	float						depthNear;
	float						depthFar;

	uLong						hashValue;
	struct shader_s				*hashNext;
} shader_t;

//
// r_shader.c
//

extern shader_t	*r_noShader;
extern shader_t *r_noShaderLightmap;
extern shader_t	*r_whiteShader;
extern shader_t	*r_blackShader;

void	R_EndShaderRegistration (void);

shader_t *R_RegisterSky (char *name);

void	R_ShaderInit (void);
void	R_ShaderShutdown (void);

/*
=============================================================================

	MESH BUFFERING

=============================================================================
*/

#define MAX_MESH_BUFFER				16384

enum {
	MF_NONE				= 0,
	MF_NONBATCHED		= 1 << 0,
	MF_NORMALS			= 1 << 1,
	MF_STCOORDS			= 1 << 2,
	MF_LMCOORDS			= 1 << 3,
	MF_COLORS			= 1 << 4,
	MF_TRNORMALS		= 1 << 5,
	MF_NOCULL			= 1 << 6,
	MF_DEFORMVS			= 1 << 7,
	MF_STVECTORS		= 1 << 8,
	MF_TRIFAN			= 1 << 9
};

enum {
	MBT_MODEL_ALIAS,
	MBT_MODEL_BSP,
	MBT_MODEL_SP2,
	MBT_POLY,
	MBT_SKY,

	MBT_MAX
};

typedef struct mesh_s {
	int						numIndexes;
	int						numVerts;

	bvec4_t					*colorArray;
	vec2_t					*coordArray;
	vec2_t					*lmCoordArray;
	index_t					*indexArray;
	vec3_t					*normalsArray;
	vec3_t					*sVectorsArray;
	vec3_t					*tVectorsArray;
	index_t					*trNeighborsArray;
	vec3_t					*trNormalsArray;
	vec3_t					*vertexArray;
} mesh_t;

typedef struct meshBuffer_s {
	uInt					sortKey;

	entity_t				*entity;
	shader_t				*shader;
	void					*mesh;
} meshBuffer_t;

typedef struct meshList_s {
	qBool					skyDrawn;
	float					skyMins[6][2];
	float					skyMaxs[6][2];

	int						numMeshes;
	meshBuffer_t			meshBuffer[MAX_MESH_BUFFER];

	int						numAdditiveMeshes;
	meshBuffer_t			meshBufferAdditive[MAX_MESH_BUFFER];

	int						numPostProcessMeshes;
	meshBuffer_t			meshBufferPostProcess[MAX_MESH_BUFFER];
} meshList_t;

extern meshList_t	r_worldList;
extern meshList_t	*r_currentList;

meshBuffer_t	*R_AddMeshToList (shader_t *shader, entity_t *ent, int meshType, void *mesh);
void	R_SortMeshList (void);
void	R_DrawMeshList (qBool triangleOutlines);
void	R_DrawMeshOutlines (void);

/*
=============================================================================

	BACKEND

=============================================================================
*/

#define VERTARRAY_MAX_VERTS			4096
#define VERTARRAY_MAX_INDEXES		VERTARRAY_MAX_VERTS*6
#define VERTARRAY_MAX_TRIANGLES		VERTARRAY_MAX_INDEXES/3
#define VERTARRAY_MAX_NEIGHBORS		VERTARRAY_MAX_TRIANGLES*3

#define QUAD_INDEXES				6
#define TRIFAN_INDEXES				((VERTARRAY_MAX_VERTS-2)*3)

extern bvec4_t		rb_colorArray[VERTARRAY_MAX_VERTS];
extern vec2_t		rb_coordArray[VERTARRAY_MAX_VERTS];
extern index_t		rb_indexArray[VERTARRAY_MAX_INDEXES];
extern vec2_t		rb_lMCoordArray[VERTARRAY_MAX_VERTS];
extern vec3_t		rb_normalsArray[VERTARRAY_MAX_VERTS];
extern vec3_t		rb_sVectorsArray[VERTARRAY_MAX_VERTS];
extern vec3_t		rb_tVectorsArray[VERTARRAY_MAX_VERTS];
extern vec3_t		rb_vertexArray[VERTARRAY_MAX_VERTS];

#ifdef SHADOW_VOLUMES
extern int			rb_neighborsArray[VERTARRAY_MAX_NEIGHBORS];
extern vec3_t		rb_trNormalsArray[VERTARRAY_MAX_TRIANGLES];
#endif

extern bvec4_t		*rb_inColorArray;
extern vec2_t		*rb_inCoordArray;
extern index_t		*rb_inIndexArray;
extern vec2_t		*rb_inLMCoordArray;
extern vec3_t		*rb_inNormalsArray;
extern vec3_t		*rb_inSVectorsArray;
extern vec3_t		*rb_inTVectorsArray;
extern vec3_t		*rb_inVertexArray;
extern int			rb_numIndexes;
extern int			rb_numVerts;

#ifdef SHADOW_VOLUMES
extern int			*rb_inNeighborsArray;
extern vec3_t		*rb_inTrNormalsArray;
extern int			*rb_currentTrNeighbor;
extern float		*rb_currentTrNormal;
#endif

extern index_t		rb_quadIndices[QUAD_INDEXES];
extern index_t		rb_triFanIndices[TRIFAN_INDEXES];

void	RB_LockArrays (int numVerts);
void	RB_UnlockArrays (void);
void	RB_ResetPointers (void);

qBool	RB_BackendOverflow (const mesh_t *mesh);
qBool	RB_InvalidMesh (const mesh_t *mesh);
void	RB_PushMesh (mesh_t *mesh, int meshFeatures);
void	RB_RenderMeshBuffer (meshBuffer_t *mb, qBool shadowPass);

void	RB_BeginTriangleOutlines (void);
void	RB_EndTriangleOutlines (void);

void	RB_BeginFrame (void);
void	RB_EndFrame (void);

void	RB_Init (void);
void	RB_Shutdown (void);

/*
=============================================================================

	MODELS

=============================================================================
*/

#include "r_model.h"

extern model_t		*r_worldModel;
extern entity_t		r_worldEntity;

//
// r_alias.c
//

void	R_AddAliasModelToList (entity_t *ent);
void	R_DrawAliasModel (meshBuffer_t *mb, qBool shadowPass);

//
// r_model.c
//

mBspLeaf_t *R_PointInBSPLeaf (float *point, model_t *model);
byte	*R_BSPClusterPVS (int cluster, model_t *model);

void	R_RegisterMap (char *mapName);
struct	model_s *R_RegisterModel (char *name);

void	R_ModelBounds (model_t *model, vec3_t mins, vec3_t maxs);

void	R_BeginModelRegistration (void);
void	R_EndModelRegistration (void);

void	R_ModelInit (void);
void	R_ModelShutdown (void);

//
// r_sprite.c
//

void	R_AddSpriteModelToList (entity_t *ent);
void	R_DrawSpriteModel (meshBuffer_t *mb);

/*
=============================================================================

	REFRESH

=============================================================================
*/

enum {
	REND_CLASS_DEFAULT,
	REND_CLASS_MCD,

	REND_CLASS_3DLABS_GLINT_MX,
	REND_CLASS_3DLABS_PERMEDIA,
	REND_CLASS_3DLABS_REALIZM,
	REND_CLASS_ATI,
	REND_CLASS_ATI_RADEON,
	REND_CLASS_INTEL,
	REND_CLASS_NVIDIA,
	REND_CLASS_NVIDIA_GEFORCE,
	REND_CLASS_PMX,
	REND_CLASS_POWERVR_PCX1,
	REND_CLASS_POWERVR_PCX2,
	REND_CLASS_RENDITION,
	REND_CLASS_S3,
	REND_CLASS_SGI,
	REND_CLASS_SIS,
	REND_CLASS_VOODOO
};

// ==========================================================

typedef struct glConfig_s {
	short		renderClass;

	const char	*rendererString;
	const char	*vendorString;
	const char	*versionString;
	const char	*extensionString;

#ifdef _WIN32
	qBool		allowCDS;
#endif // _WIN32

	// Gamma ramp
	qBool		hwGamma;
	qBool		rampDownloaded;
	uShort		originalRamp[768];
	uShort		gammaRamp[768];

	// Extensions
	qBool		extArbMultitexture;
	qBool		extArbTexCompression;
	qBool		extArbTexCubeMap;
	qBool		extCompiledVertArray;
	qBool		extDrawRangeElements;
	qBool		extFragmentProgram;
	qBool		extNVTexEnvCombine4;
	qBool		extSGISGenMipmap;
	qBool		extSGISMultiTexture;
	qBool		extTexEdgeClamp;
	qBool		extTexEnvAdd;
	qBool		extTexEnvCombine;
	qBool		extTexEnvDot3;
	qBool		extTexFilterAniso;
	qBool		extVertexBufferObject;
	qBool		extVertexProgram;
	qBool		extWinSwapInterval;

	// GL Queries
	GLint		maxAniso;
	GLint		maxCMTexSize;
	GLint		maxElementVerts;
	GLint		maxElementIndices;
	GLint		maxTexCoords;
	GLint		maxTexImageUnits;
	GLint		maxTexSize;
	GLint		maxTexUnits;

	// Resolution
	qBool		vidFullScreen;
	int			vidFrequency;
	int			vidBitDepth;
	int			safeMode;

	// PFD Stuff
	qBool		useStencil;		// stencil buffer toggle

	byte		cAlphaBits;
	byte		cColorBits;
	byte		cDepthBits;
	byte		cStencilBits;
} glConfig_t;

typedef struct glMedia_s {
	shader_t	*charShader;

	shader_t	*worldLavaCaustics;
	shader_t	*worldSlimeCaustics;
	shader_t	*worldWaterCaustics;
} glMedia_t;

typedef struct glState_s {
	float		realTime;

	// Texture
	float		invIntens;		// inverse intensity

	GLenum		texUnit;
	GLuint		texNums[MAX_TEXUNITS];
	GLfloat		texEnvModes[MAX_TEXUNITS];
	qBool		texMatIdentity[MAX_TEXUNITS];

	GLint		texRGBFormat;
	GLint		texRGBAFormat;

	GLint		texMinFilter;
	GLint		texMagFilter;

	// Scene
	qBool		in2D;
	qBool		stereoEnabled;
	float		cameraSeparation;

	// State management
	GLenum		blendSFactor;
	GLenum		blendDFactor;

	GLenum		shadeModel;

	qBool		multiTextureOn;
} glState_t;

typedef struct glSpeeds_s {
	// Totals
	uInt			worldElements;
	uInt			worldLeafs;
	uInt			worldPolys;

	uInt			numTris;
	uInt			numVerts;
	uInt			numElements;

	// Misc
	uInt			aliasPolys;

	uInt			shaderCount;
	uInt			shaderPasses;

	// Image
	uInt			textureBinds;
	uInt			texEnvChanges;
	uInt			texUnitChanges;
} glSpeeds_t;

extern glConfig_t	glConfig;
extern glMedia_t	glMedia;
extern glSpeeds_t	glSpeeds;
extern glState_t	glState;

/*
=============================================================================

	CVARS

=============================================================================
*/

extern cVar_t	*e_test_0;
extern cVar_t	*e_test_1;

extern cVar_t	*con_font;

extern cVar_t	*intensity;

extern cVar_t	*gl_3dlabs_broken;
extern cVar_t	*gl_bitdepth;
extern cVar_t	*gl_clear;
extern cVar_t	*gl_cull;
extern cVar_t	*gl_drawbuffer;
extern cVar_t	*gl_driver;
extern cVar_t	*gl_dynamic;
extern cVar_t	*gl_errorcheck;

extern cVar_t	*gl_extensions;
extern cVar_t	*gl_arb_texture_compression;
extern cVar_t	*gl_arb_texture_cube_map;
extern cVar_t	*gl_ext_compiled_vertex_array;
extern cVar_t	*gl_ext_draw_range_elements;
extern cVar_t	*gl_ext_fragment_program;
extern cVar_t	*gl_ext_max_anisotropy;
extern cVar_t	*gl_ext_multitexture;
extern cVar_t	*gl_ext_swapinterval;
extern cVar_t	*gl_ext_texture_edge_clamp;
extern cVar_t	*gl_ext_texture_env_add;
extern cVar_t	*gl_ext_texture_env_combine;
extern cVar_t	*gl_nv_texture_env_combine4;
extern cVar_t	*gl_ext_texture_env_dot3;
extern cVar_t	*gl_ext_texture_filter_anisotropic;
extern cVar_t	*gl_ext_vertex_buffer_object;
extern cVar_t	*gl_ext_vertex_program;
extern cVar_t	*gl_sgis_generate_mipmap;

extern cVar_t	*gl_finish;
extern cVar_t	*gl_flashblend;
extern cVar_t	*gl_jpgquality;
extern cVar_t	*gl_lightmap;
extern cVar_t	*gl_lockpvs;
extern cVar_t	*gl_log;
extern cVar_t	*gl_maxTexSize;
extern cVar_t	*gl_mode;
extern cVar_t	*gl_modulate;

extern cVar_t	*gl_polyblend;
extern cVar_t	*gl_screenshot;
extern cVar_t	*gl_shadows;
extern cVar_t	*gl_shownormals;
extern cVar_t	*gl_showtris;
extern cVar_t	*gl_stencilbuffer;
extern cVar_t	*gl_swapinterval;
extern cVar_t	*gl_texturebits;
extern cVar_t	*gl_texturemode;

extern cVar_t	*qgl_debug;

extern cVar_t	*r_bumpScale;
extern cVar_t	*r_caustics;
extern cVar_t	*r_detailTextures;
extern cVar_t	*r_displayFreq;
extern cVar_t	*r_drawEntities;
extern cVar_t	*r_drawworld;
extern cVar_t	*r_fontscale;
extern cVar_t	*r_fullbright;
extern cVar_t	*r_hwGamma;
extern cVar_t	*r_lerpmodels;
extern cVar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level
extern cVar_t	*r_noCull;
extern cVar_t	*r_noRefresh;
extern cVar_t	*r_noVis;
extern cVar_t	*r_speeds;
extern cVar_t	*r_sphereCull;

extern cVar_t	*vid_gamma;

/*
=============================================================================

	FUNCTIONS

=============================================================================
*/

//
// console.c
//

void	Con_CheckResize (void);

//
// r_cin.c
//

void	R_CinematicPalette (const byte *palette);
void	R_DrawCinematic (int x, int y, int w, int h, int cols, int rows, byte *data);

//
// r_cull.c
//

// frustum
extern	cBspPlane_t	r_viewFrustum[4];

void	R_SetupFrustum (void);

qBool	R_CullBox (vec3_t mins, vec3_t maxs, int clipFlags);
qBool	R_CullSphere (const vec3_t origin, const float radius, int clipFlags);

// misc
qBool	R_CullVisNode (struct mBspNode_s *node);

//
// r_draw.c
//

void	R_CheckFont (void);

void	R_DrawPic (shader_t *shader, float x, float y, int w, int h, float s1, float t1, float s2, float t2, vec4_t color);
void	R_DrawChar (shader_t *charsShader, float x, float y, int flags, float scale, int num, vec4_t color);
int		R_DrawString (shader_t *charsShader, float x, float y, int flags, float scale, char *string, vec4_t color);
int		R_DrawStringLen (shader_t *charsShader, float x, float y, int flags, float scale, char *string, int len, vec4_t color);
void	R_DrawFill (float x, float y, int w, int h, vec4_t color);

//
// r_entity.c
//

void	R_LoadModelIdentity (void);
void	R_RotateForEntity (entity_t *ent);
void	R_RotateForAliasShadow (entity_t *ent);
void	R_TranslateForEntity (entity_t *ent);

void	R_AddEntitiesToList (void);
void	R_DrawNullModelList (void);

//
// r_glstate.c
//

void	GL_BlendFunc (GLenum sfactor, GLenum dfactor);
void	GL_SetDefaultState (void);
void	GL_Setup2D (void);
void	GL_Setup3D (void);
void	GL_ShadeModel (GLenum mode);

//
// r_light.c
//

void	R_MarkLights (qBool visCull, dLight_t *light, int bit, mBspNode_t *node);

void	R_AddDLightsToList (void);
void	R_PushDLights (void);

void	R_LightBounds (const vec3_t origin, float intensity, vec3_t mins, vec3_t maxs);

qBool	R_ShadowForEntity (entity_t *ent, vec3_t shadowSpot);
void	R_LightForEntity (entity_t *ent, int numVerts, byte *bArray);
void	R_LightPoint (vec3_t point, vec3_t light);
void	R_SetLightLevel (void);

void	LM_UpdateLightmap (mBspSurface_t *surf);
void	LM_BeginBuildingLightmaps (void);
void	LM_CreateSurfaceLightmap (mBspSurface_t *surf);
void	LM_EndBuildingLightmaps (void);

//
// r_main.c
//

extern uLong	r_frameCount;
extern uLong	r_registrationFrame;

// view angles
extern vec3_t	r_forwardVec;
extern vec3_t	r_rightVec;
extern vec3_t	r_upVec;

extern mat4x4_t	r_modelViewMatrix;
extern mat4x4_t	r_projectionMatrix;
extern mat4x4_t	r_worldViewMatrix;

extern refDef_t	r_refDef;

// scene
extern int			r_numEntities;
extern entity_t		r_entityList[MAX_ENTITIES];

extern int			r_numPolys;
extern poly_t		r_polyList[MAX_POLYS];

extern int			r_numDLights;
extern dLight_t		r_dLightList[MAX_DLIGHTS];

extern lightStyle_t	r_lightStyles[MAX_CS_LIGHTSTYLES];

void	GL_CheckForError (char *where);

void	R_BeginRegistration (void);
void	R_EndRegistration (void);
void	R_MediaInit (void);

void	R_UpdateCvars (void);

qBool	R_GetInfoForMode (int mode, int *width, int *height);

//
// r_poly.c
//

void	R_AddPolysToList (void);
void	R_PushPoly (meshBuffer_t *mb);
qBool	R_PolyOverflow (meshBuffer_t *mb);
void	R_DrawPoly (meshBuffer_t *mb);

//
// r_shadow.c
//

#ifdef SHADOW_VOLUMES
void	R_DrawShadowVolumes (mesh_t *mesh, entity_t *ent, vec3_t mins, vec3_t maxs, float radius);
void	R_ShadowBlend (void);
#endif

void	R_SimpleShadow (entity_t *ent, vec3_t shadowSpot);

//
// r_world.c
//

extern	uLong	r_visFrameCount;

extern	int		r_oldViewCluster;
extern	int		r_viewCluster;

void	R_DrawSky (meshBuffer_t *mb);

void	R_SetSky (char *name, float rotate, vec3_t axis);

void	R_AddWorldToList (void);
void	R_AddBrushModelToList (entity_t *ent);

void	R_WorldInit (void);
void	R_WorldShutdown (void);

/*
=============================================================================

	IMPLEMENTATION SPECIFIC

=============================================================================
*/

extern cVar_t	*vid_fullscreen;
extern cVar_t	*vid_xpos;
extern cVar_t	*vid_ypos;

//
// glimp_imp.c
//

void	GLimp_BeginFrame (void);
void	GLimp_EndFrame (void);

void	GLimp_AppActivate (qBool active);
qBool	GLimp_GetGammaRamp (uShort *ramp);
void	GLimp_SetGammaRamp (uShort *ramp);

void	GLimp_Shutdown (void);
qBool	GLimp_Init (void *hInstance, void *hWnd);
qBool	GLimp_AttemptMode (int mode);
