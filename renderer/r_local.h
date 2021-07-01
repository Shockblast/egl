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

/*
=============================================================

	IMAGING

=============================================================
*/

#define MAX_GLTEXTURES		4096			// maximum local images
#define TEXNUM_LIGHTMAPS	MAX_GLTEXTURES	// start texnum offset for lightmap textures

extern image_t		img_Textures[MAX_GLTEXTURES];
extern int			img_NumTextures;

extern int			img_RegistrationFrame;

extern const char	*sky_NameSuffix[6];
extern const char	*cubeMap_NameSuffix[6];

enum {
	TU0,
	TU1,
	TU2,
	TU3,
	GL_MAXTU
};

//
// r_image.c
//

extern unsigned		img_PaletteTable[256];

extern	image_t		*r_NoTexture;
extern	image_t		*r_WhiteTexture;
extern	image_t		*r_BlackTexture;
extern	image_t		*r_CinTexture;
extern	image_t		*r_DSTTexture;

void	GL_Bind (GLenum target, image_t *image);
void	GL_SelectTexture (int target);
void	GL_ResetAnisotropy (void);
void	GL_TexEnv (GLfloat mode);
void	GL_TextureBits (void);
void	GL_TextureMode (void);
void	GL_MTexCoord2f (int target, GLfloat s, GLfloat t);
void	GL_ToggleMultitexture (qBool enable);
void	GL_ToggleOverbrights (qBool enable);

void	Img_BeginRegistration (void);
void	Img_EndRegistration (void);

void	Img_ScreenShot_f (void);

void	Img_Init (void);
void	Img_Shutdown (void);

/*
=============================================================

	MODELS

=============================================================
*/

#include "r_model.h"

extern	model_t		*r_WorldModel;
extern	entity_t	r_WorldEntity;

extern int			mod_RegistrationFrame;


//
// r_alias.c
//

void	R_DrawAliasModel (entity_t *ent);

//
// r_model.c
//

void	Mod_Init (void);
void	Mod_Shutdown (void);
mBspLeaf_t *Mod_PointInLeaf (float *point, model_t *model);
byte	*Mod_ClusterPVS (int cluster, model_t *model);

void	Mod_FreeAll (void);
void	Mod_Free (model_t *mod);

void	Mod_BeginRegistration (char *map);
void	Mod_EndRegistration (void);

struct	model_s *Mod_RegisterModel (char *name);

//
// r_sprite.c
//

void	R_DrawSpriteModel (entity_t *ent);

/*
=============================================================

	BACKEND

=============================================================
*/

#define VERTARRAY_MAX_VERTS			4096
#define VERTARRAY_MAX_INDEXES		VERTARRAY_MAX_VERTS*6

extern index_t		indexesArray[VERTARRAY_MAX_INDEXES];

extern vec3_t		colorArray[VERTARRAY_MAX_VERTS];
extern vec2_t		coordArray[VERTARRAY_MAX_VERTS];
extern vec3_t		normalsArray[VERTARRAY_MAX_VERTS];
extern vec3_t		vertexArray[VERTARRAY_MAX_VERTS];

extern index_t		*rbIndexArray;

extern vec3_t		*rbColorArray;
extern vec2_t		*rbCoordArray;
extern vec3_t		*rbNormalsArray;
extern vec3_t		*rbVertexArray;

void	RB_Init (void);
void	RB_Shutdown (void);

/*
=============================================================

	REFRESH

=============================================================
*/

enum {
	GLREND_DEFAULT		= 1 << 0,
	GLREND_MCD			= 1 << 1,

	GLREND_3DLABS		= 1 << 2,
	GLREND_GLINT_MX		= 1 << 3,
	GLREND_PCX1			= 1 << 4,
	GLREND_PCX2			= 1 << 5,
	GLREND_PERMEDIA2	= 1 << 6,
	GLREND_PMX			= 1 << 7,
	GLREND_POWERVR		= 1 << 8,
	GLREND_REALIZM		= 1 << 9,
	GLREND_RENDITION	= 1 << 10,
	GLREND_SGI			= 1 << 11,
	GLREND_SIS			= 1 << 12,
	GLREND_VOODOO		= 1 << 13,

	GLREND_NVIDIA		= 1 << 14,
	GLREND_GEFORCE		= 1 << 15,

	GLREND_ATI			= 1 << 16,
	GLREND_RADEON		= 1 << 17
};

// ==========================================================

typedef struct {
	int			rendType;

	const char	*rendString;
	const char	*vendString;
	const char	*versString;
	const char	*extsString;

	qBool		allowCDS;

	// extensions
	qBool		extArbMultitexture;
	qBool		extArbTexCompression;
	qBool		extArbTexCubeMap;
	qBool		extArbVertBufferObject;
	qBool		extBGRA;
	qBool		extCompiledVertArray;
	qBool		extDrawRangeElements;
	qBool		extMTexCombine;
	qBool		extMultiSample;
	qBool		extNVMulSampleHint;
	qBool		extNVTexShader;
	qBool		extSGISGenMipmap;
	qBool		extSGISMultiTexture;
	qBool		extTexEdgeClamp;
	qBool		extTexEnvAdd;
	qBool		extTexFilterAniso;
	qBool		extWinSwapInterval;

	// gl queries
	GLint		maxAniso;
	GLint		maxCMTexSize;
	GLint		maxElementVerts;
	GLint		maxElementIndices;
	GLint		maxTexSize;
	GLint		maxTexUnits;

	// resolution
	int			vidFrequency;
	int			vidBitDepth;
	qBool		vidFullscreen;
	int			safeMode;

	// pfd stuff
	qBool		wglPFD;			// if true, we're using a WGL PFD
	qBool		useStencil;		// stencil buffer toggle

	byte		cAlphaBits;
	byte		cColorBits;
	byte		cDepthBits;
	byte		cStencilBits;
} glConfig_t;

typedef struct {
	image_t		*charsImage;
	image_t		*uiCharsImage;				// hack kthx

	image_t		*model_ShellTexture;		// hack kthx

	shader_t	*model_ShellGod;			// hack kthx
	shader_t	*model_ShellHalfDam;		// hack kthx
	shader_t	*model_ShellDouble;			// hack kthx
	shader_t	*model_ShellRed;			// hack kthx
	shader_t	*model_ShellGreen;			// hack kthx
	shader_t	*model_ShellBlue;			// hack kthx
	shader_t	*model_ShellIR;				// hack kthx

	shader_t	*world_LavaCaustics;
	shader_t	*world_SlimeCaustics;
	shader_t	*world_WaterCaustics;
} glMedia_t;

typedef struct {
	float		realTime;

	// texture
	float		invIntens;		// inverse intensity

	GLenum		texUnit;
	GLuint		texNums[3];
	GLfloat		texEnvModes[3];

	GLint		texRGBFormat;
	GLint		texRGBAFormat;

	GLint		texMinFilter;
	GLint		texMagFilter;

	// scene
	qBool		in2D;
	qBool		stereoEnabled;
	float		cameraSeparation;

	// state management
	GLenum		blendSFactor;
	GLenum		blendDFactor;

	GLenum		shadeModel;

	qBool		multiSampleOn;
	qBool		overBrightsOn;
} glState_t;

typedef struct {
	int			numEntities;
	int			aliasPolys;

	int			worldPolys;
	int			worldLeafs;
	int			skyPolys;

	int			visibleLightmaps;
	int			visibleTextures;

	int			shaderCount;
	int			shaderPasses;
	int			shaderCMPasses;
} glSpeeds_t;

extern glConfig_t	glConfig;
extern glMedia_t	glMedia;
extern glSpeeds_t	glSpeeds;
extern glState_t	glState;

/*
=============================================================

	ERROR REPORTING

=============================================================
*/

enum {
	RSErr_ok,

	RSErr_InvalidFullscreen,
	RSErr_InvalidMode,

	RSErr_unknown
};

/*
=============================================================

	CVARS

=============================================================
*/

extern	cvar_t	*e_test_0;
extern	cvar_t	*e_test_1;

extern	cvar_t	*con_font;

extern	cvar_t	*gl_extensions;
extern	cvar_t	*gl_arb_texture_compression;
extern	cvar_t	*gl_arb_texture_cube_map;
extern	cvar_t	*gl_arb_vertex_buffer_object;
extern	cvar_t	*gl_ext_bgra;
extern	cvar_t	*gl_ext_compiled_vertex_array;
extern	cvar_t	*gl_ext_draw_range_elements;
extern	cvar_t	*gl_ext_max_anisotropy;
extern	cvar_t	*gl_ext_mtexcombine;
extern	cvar_t	*gl_ext_multitexture;
extern	cvar_t	*gl_ext_swapinterval;
extern	cvar_t	*gl_ext_texture_edge_clamp;
extern	cvar_t	*gl_ext_texture_env_add;
extern	cvar_t	*gl_ext_texture_filter_anisotropic;
extern	cvar_t	*gl_nv_multisample_filter_hint;
extern	cvar_t	*gl_nv_texture_shader;
extern	cvar_t	*gl_sgis_generate_mipmap;

extern	cvar_t	*gl_3dlabs_broken;
extern	cvar_t	*gl_bitdepth;
extern	cvar_t	*gl_clear;
extern	cvar_t	*gl_cull;
extern	cvar_t	*gl_drawbuffer;
extern  cvar_t  *gl_driver;
extern	cvar_t	*gl_dynamic;

extern	cvar_t	*gl_finish;
extern	cvar_t	*gl_flashblend;
extern	cvar_t	*gl_jpgquality;
extern	cvar_t	*gl_lightmap;
extern  cvar_t  *gl_lockpvs;
extern	cvar_t	*gl_log;
extern	cvar_t	*gl_mode;
extern	cvar_t	*gl_modulate;

extern	cvar_t	*gl_polyblend;
extern  cvar_t  *gl_saturatelighting;
extern	cvar_t	*gl_screenshot;
extern	cvar_t	*gl_shadows;
extern	cvar_t	*gl_showtris;
extern	cvar_t	*gl_stencilbuffer;
extern	cvar_t	*gl_swapinterval;
extern	cvar_t	*gl_texturebits;
extern	cvar_t	*gl_texturemode;

extern	cvar_t	*part_shade;

extern	cvar_t	*r_advir;
extern	cvar_t	*r_caustics;
extern	cvar_t	*r_displayfreq;
extern	cvar_t	*r_drawentities;
extern	cvar_t	*r_drawworld;

extern	cvar_t	*r_fog;
extern	cvar_t	*r_fog_start;
extern	cvar_t	*r_fog_density;
extern	cvar_t	*r_fog_end;
extern	cvar_t	*r_fog_red;
extern	cvar_t	*r_fog_green;
extern	cvar_t	*r_fog_blue;
extern	cvar_t	*r_fog_water;

extern	cvar_t	*r_fontscale;
extern	cvar_t	*r_fullbright;
extern	cvar_t	*r_hudscale;
extern	cvar_t	*r_lerpmodels;
extern	cvar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level
extern	cvar_t	*r_nocull;
extern	cvar_t	*r_norefresh;
extern	cvar_t	*r_novis;
extern	cvar_t	*r_overbrightbits;
extern	cvar_t	*r_shaders;
extern	cvar_t	*r_speeds;

extern	cvar_t	*vid_fullscreen;

/*
=============================================================

	FUNCTIONS

=============================================================
*/

#define TURBSCALE (256.0 / M_PI_TIMES2)

//
// cl_main.c
//

void	CL_InitMedia (void);

//
// console.c
//

void	Con_CheckResize (void);

//
// r_cin.c
//

void	R_SetPalette (const byte *palette);
void	R_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data);

//
// r_draw.c
//

void	Draw_CheckFont (void);

void	Draw_Pic (image_t *image, float x, float y, int w, int h, float s1, float t1, float s2, float t2, vec4_t color);
void	Draw_StretchPic (char *pic, float x, float y, int w, int h, vec4_t color);
int		Draw_String (float x, float y, int flags, float scale, char *string, vec4_t color);
int		Draw_StringLen (float x, float y, int flags, float scale, char *string, int len, vec4_t color);
void	Draw_Char (float x, float y, int flags, float scale, int num, vec4_t color);
void	Draw_Fill (float x, float y, int w, int h, vec4_t color);

//
// r_entity.c
//

void	R_LoadModelIdentity (void);
void	R_RotateForEntity (entity_t *ent);
void	R_RotateForAliasShadow (entity_t *ent);
void	R_TranslateForEntity (entity_t *ent);

void	R_DrawEntities (qBool vWeapOnly);

//
// r_fx.c
//

void	R_DrawDecals (void);
void	R_DrawParticles (void);

//
// r_glstate.c
//

void	GL_BlendFunc (GLenum sfactor, GLenum dfactor);
void	GL_SetDefaultState (void);
void	GL_SetupGL2D (void);
void	GL_SetupGL3D (void);
void	GL_ShadeModel (GLenum mode);
void	GL_ToggleMultisample (qBool enable);

//
// r_image.c
//

image_t	*Img_FindImage (char *name, int flags);

image_t	*Img_RegisterPic (char *name);
image_t	*Img_RegisterSkin (char *name);

//
// r_light.c
//

void	R_SetCacheState (mBspSurface_t *surf);
void	R_BuildLightMap (mBspSurface_t *surf, byte *dest, int stride);

void	R_MarkLights (dLight_t *light, int bit, mBspNode_t *node);

void	R_LightForEntity (entity_t *ent, int numVerts, qBool hasShell, qBool hasShadow, vec3_t shadowspot);
void	R_LightPoint (vec3_t point, vec3_t light);
void	R_SetLightLevel (void);

void	R_PushDLights (void);
void	R_RenderDLights (void);

//
// r_main.c
//

extern float	r_WarpSinTable[];

// view angles
extern vec3_t	v_Forward;
extern vec3_t	v_Right;
extern vec3_t	v_Up;

extern mat4_t	r_ModelViewMatrix;
extern mat4_t	r_ProjectionMatrix;
extern mat4_t	r_WorldViewMatrix;

extern refDef_t	r_RefDef;

extern entity_t	*r_CurrEntity;
extern model_t	*r_CurrModel;

// scene
extern int			r_NumEntities;
extern entity_t		r_Entities[MAX_ENTITIES];

extern int			r_NumParticles;
extern particle_t	r_Particles[MAX_PARTICLES];

extern int			r_NumDecals;
extern decal_t		r_Decals[MAX_DECALS];

extern int			r_NumDLights;
extern dLight_t		r_DLights[MAX_DLIGHTS];

extern lightStyle_t	r_LightStyles[MAX_LIGHTSTYLES];

// frustum
extern	cBspPlane_t	r_Frustum[4];

qBool R_CullBox (vec3_t mins, vec3_t maxs);
qBool R_CullSphere (const vec3_t origin, const float radius, const int clipFlags);

void	GL_CheckForError (void);

qBool	R_ExtensionAvailable (const char *extensions, const char *extension);

void	R_EndRegistration (void);
void	R_RegisterMedia (void);

void	R_UpdateCvars (void);

qBool	R_GetInfoForMode (int mode, int *width, int *height);

//
// r_shader.c
//

void	RS_Init (void);
void	RS_Shutdown (void);

void	RS_LoadShader (char *fileName, qBool recursed, qBool silent);
void	RS_FreeShader (shader_t *shader);
shader_t *RS_FindShader (char *name);
void	RS_ReadyShader (shader_t *shader);
void	RS_ScanPathForShaders (qBool silent);
image_t *RS_Animate (shaderStage_t *stage);

void	RS_FreeUnmarked (void);
shader_t *RS_RegisterShader (char *name);
void	RS_EndRegistration (void);
void	RS_SetTexcoords (shaderStage_t *stage, mBspSurface_t *surf, vec2_t scrollCoords);
void	RS_SetTexcoords2D (shaderStage_t *stage, vec2_t coords);

void	RS_AlphaShift (shaderStage_t *stage, float *alpha);
void	RS_BlendFunc (shaderStage_t *stage, qBool force, qBool blend);
void	RS_Scroll (shaderStage_t *stage, vec2_t scrollCoords);
void	RS_SetEnvmap (vec3_t origin, vec3_t axis[3], vec3_t vertOrg, vec3_t normal, vec2_t coords);

void	RS_DrawSurface (mBspSurface_t *surf, shader_t *shader, qBool lightmap, int lmTexNum);
#define RS_DrawLightMapPoly(surf,lmTexNum) RS_DrawSurface ((surf), (surf->texInfo->shader), qTrue, (lmTexNum))
#define RS_DrawPoly(surf) RS_DrawSurface ((surf), (surf->texInfo->shader), qFalse, 0)

void	RS_SpecialSurface (mBspSurface_t *surf, qBool lightmap, int lmTexNum);

//
// r_sky.c
//

typedef struct
{
	qBool	loaded;

	char	baseName[MAX_QPATH];
	float	rotation;
	vec3_t	axis;

	image_t	*textures[6];

	float	mins[2][6];
	float	maxs[2][6];

} r_Sky_t;

extern r_Sky_t	r_Sky;

void	R_AddSkySurface (mBspSurface_t *surf);
void	R_ClearSky (void);
void	R_DrawSky (void);
void	R_SetSky (char *name, float rotate, vec3_t axis);

void	Sky_Init (void);
void	Sky_Shutdown (void);

//
// r_surface.c
//

extern	int		r_FrameCount;
extern	int		r_VisFrameCount;

extern	int		r_OldViewCluster;
extern	int		r_OldViewCluster2;
extern	int		r_ViewCluster;
extern	int		r_ViewCluster2;

void	R_DrawBrushModel (entity_t *ent);
void	R_DrawWorld (void);
void	R_DrawAlphaSurfaces (void);
void	R_MarkLeaves (void);

void	R_BuildPolygonFromSurface (model_t *model, mBspSurface_t *surf);
void	LM_CreateSurfaceLightmap (mBspSurface_t *surf);
void	LM_EndBuildingLightmaps (void);
void	LM_BeginBuildingLightmaps (void);

/*
=============================================================

	IMPLIMENTATION SPECIFIC

=============================================================
*/

//
// glimp_imp.c
//

void	GLimp_BeginFrame (void);
void	GLimp_EndFrame (void);

void	GLimp_AppActivate (qBool active);

void	GLimp_Shutdown (void);
int		GLimp_Init (void *hInstance, void *hWnd);
int		GLimp_AttemptMode (int mode);

//
// qgl_imp.c
//

void	QGL_ToggleLogging (qBool enable);
void	QGL_LogBeginFrame (void);
void	QGL_LogEndFrame (void);

void	*QGL_GetProcAddress (const GLubyte *procName);

//
// vid_imp.c
//

void	VID_NewWindow (void);
