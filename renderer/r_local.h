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

#define SHADOW_VOLUMES	2
#define SHADOW_ALPHA	0.5f

#ifdef WIN32
# include <windows.h>
#endif

#include <GL/gl.h>
#include <math.h>

#include "refresh.h"
#include "qgl.h"

#include "r_typedefs.h"
#include "r_image.h"
#include "r_program.h"
#include "r_shader.h"
#include "r_entity.h"
#include "r_backend.h"
#include "r_model.h"
#include "r_glext.h"
#include "r_glstate.h"

/*
=============================================================================

	REFRESH

=============================================================================
*/

enum { // rendererClass_t
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

typedef struct glMedia_s {
	shader_t		*charShader;

	shader_t		*worldLavaCaustics;
	shader_t		*worldSlimeCaustics;
	shader_t		*worldWaterCaustics;
} glMedia_t;

typedef struct glStatic_s {
	rendererClass_t	renderClass;

	const byte		*rendererString;
	const byte		*vendorString;
	const byte		*versionString;
	const byte		*extensionString;

#ifdef WIN32
	qBool			allowCDS;
#endif // WIN32

	// Hardware gamma
	qBool			rampDownloaded;
	uint16			originalRamp[768];
	uint16			gammaRamp[768];

	// Texture
	int				rgbFormat;
	int				rgbaFormat;

	int				rgbFormatCompressed;
	int				rgbaFormatCompressed;

	// PFD Stuff
	qBool			useStencil;

	byte			cAlphaBits;
	byte			cColorBits;
	byte			cDepthBits;
	byte			cStencilBits;
} glStatic_t;

extern glConfig_t	glConfig;
extern glMedia_t	glMedia;
extern glStatic_t	glStatic;

/*
=============================================================================

	PERFORMANCE COUNTERS

=============================================================================
*/

typedef struct refStats_s {
	// Totals
	uint32			numTris;
	uint32			numVerts;
	uint32			numElements;

	uint32			meshCount;
	uint32			meshPasses;

	// Alias Models
	uint32			aliasElements;
	uint32			aliasPolys;

	// Culling
	uint32			cullBounds[2];	// [fail|pass]
	uint32			cullPlanar[2];	// [fail|pass]
	uint32			cullRadius[2];	// [fail|pass]
	uint32			cullVis[2];		// [fail|pass]

	// Image
	uint32			textureBinds;
	uint32			textureEnvChanges;
	uint32			textureUnitChanges;

	uint32			texelsInUse;
	uint32			texturesInUse;

	// World model
	uint32			worldElements;
	uint32			worldPolys;
} refStats_t;

extern refStats_t	r_refStats;

/*
=============================================================================

	REGISTRATION INFORMATION

=============================================================================
*/

typedef struct refRegist_s {
	qBool			inSequence;			// True when in a registration sequence
	uint32			registerFrame;		// Used to determine what's kept and what's not

	// Models
	uint32			modelsReleased;
	uint32			modelsSearched;
	uint32			modelsTouched;

	// Shaders
	uint32			shadersReleased;
	uint32			shadersSearched;
	uint32			shadersTouched;

	// Programs
	uint32			fragProgsReleased;
	uint32			vertProgsReleased;

	// Images
	uint32			imagesReleased;
	uint32			imagesResampled;
	uint32			imagesSearched;
	uint32			imagesTouched;
} refRegist_t;

extern refRegist_t r_refRegInfo;

/*
=============================================================================

	SCENE

=============================================================================
*/

typedef struct refScene_s {
	uint32			frameCount;

	// View
	cBspPlane_t		viewFrustum[4];

	mat4x4_t		modelViewMatrix;
	mat4x4_t		projectionMatrix;
	mat4x4_t		worldViewMatrix;

	qBool			mirrorView;
	qBool			portalView;
	vec3_t			portalOrigin;
	cBspPlane_t		clipPlane;

	float			zFar;

	entity_t		defaultEntity;

	// World model
	model_t			*worldModel;
	entity_t		worldEntity;

	uint32			visFrameCount;	// Bumped when going to a new PVS
	int				viewCluster;
	int				oldViewCluster;

	// Items
	int				numEntities;
	entity_t		entityList[MAX_ENTITIES];

	int				numPolys;
	poly_t			polyList[MAX_POLYS];

	int				numDLights;
	dLight_t		dLightList[MAX_DLIGHTS];

	lightStyle_t	lightStyles[MAX_CS_LIGHTSTYLES];
} refScene_t;

extern refDef_t		r_refDef;
extern refScene_t	r_refScene;

void	R_RenderToList (refDef_t *rd, meshList_t *list);

void	GL_CheckForError (char *where);

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

extern cVar_t	*r_ext_maxAnisotropy;

extern cVar_t	*gl_finish;
extern cVar_t	*gl_flashblend;
extern cVar_t	*gl_jpgquality;
extern cVar_t	*gl_lightmap;
extern cVar_t	*gl_lockpvs;
extern cVar_t	*gl_log;
extern cVar_t	*gl_maxTexSize;
extern cVar_t	*gl_mode;
extern cVar_t	*gl_modulate;

extern cVar_t	*gl_screenshot;
extern cVar_t	*gl_shadows;
extern cVar_t	*gl_shownormals;
extern cVar_t	*gl_showtris;
extern cVar_t	*gl_stencilbuffer;
extern cVar_t	*gl_texturemode;

extern cVar_t	*qgl_debug;

extern cVar_t	*r_caustics;
extern cVar_t	*r_colorMipLevels;
extern cVar_t	*r_debugCulling;
extern cVar_t	*r_debugLighting;
extern cVar_t	*r_debugSorting;
extern cVar_t	*r_detailTextures;
extern cVar_t	*r_displayFreq;
extern cVar_t	*r_drawEntities;
extern cVar_t	*r_drawPolys;
extern cVar_t	*r_drawworld;
extern cVar_t	*r_facePlaneCull;
extern cVar_t	*r_flares;
extern cVar_t	*r_flareFade;
extern cVar_t	*r_flareSize;
extern cVar_t	*r_fontscale;
extern cVar_t	*r_fullbright;
extern cVar_t	*r_hwGamma;
extern cVar_t	*r_lerpmodels;
extern cVar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level
extern cVar_t	*r_lmMaxBlockSize;
extern cVar_t	*r_lmModulate;
extern cVar_t	*r_lmPacking;
extern cVar_t	*r_noCull;
extern cVar_t	*r_noRefresh;
extern cVar_t	*r_noVis;
extern cVar_t	*r_patchDivLevel;
extern cVar_t	*r_roundImagesDown;
extern cVar_t	*r_skipBackend;
extern cVar_t	*r_speeds;
extern cVar_t	*r_sphereCull;
extern cVar_t	*r_swapInterval;
extern cVar_t	*r_textureBits;
extern cVar_t	*r_vertexLighting;

extern cVar_t	*r_alphabits;
extern cVar_t	*r_colorbits;
extern cVar_t	*r_depthbits;
extern cVar_t	*r_stencilbits;
extern cVar_t	*cl_stereo;
extern cVar_t	*gl_allow_software;
extern cVar_t	*gl_stencilbuffer;

extern cVar_t	*vid_gammapics;
extern cVar_t	*vid_gamma;
extern cVar_t	*vid_width;
extern cVar_t	*vid_height;

extern cVar_t	*intensity;

extern cVar_t	*gl_jpgquality;
extern cVar_t	*gl_nobind;
extern cVar_t	*gl_picmip;
extern cVar_t	*gl_screenshot;
extern cVar_t	*gl_texturemode;

/*
=============================================================================

	SUPPORTING FUNCTIONS

=============================================================================
*/

//
// cl_console.c
//

void		CL_ConsoleCheckResize (void);

//
// r_cin.c
//

void		R_CinematicPalette (const byte *palette);
void		R_DrawCinematic (int x, int y, int w, int h, int cols, int rows, byte *data);

//
// r_cull.c
//

void		R_SetupFrustum (void);

qBool		R_CullBox (vec3_t mins, vec3_t maxs, int clipFlags);
qBool		R_CullSphere (const vec3_t origin, const float radius, int clipFlags);

qBool		R_CullVisNode (struct mBspNode_s *node);

//
// r_draw.c
//

void		R_CheckFont (void);

//
// r_init.c
//
qBool		R_GetInfoForMode (int mode, int *width, int *height);

//
// r_light.c
//
void		R_Q2BSP_MarkLights (model_t *model, mBspNode_t *node, dLight_t *light, uint32 bit, qBool visCull);

void		R_AddDLightsToList (void);

void		R_Q2BSP_PushDLights (void);

void		R_Q2BSP_UpdateLightmap (mBspSurface_t *surf);
void		R_Q2BSP_BeginBuildingLightmaps (void);
void		R_Q2BSP_CreateSurfaceLightmap (mBspSurface_t *surf);
void		R_Q2BSP_EndBuildingLightmaps (void);

void		R_LightBounds (const vec3_t origin, float intensity, vec3_t mins, vec3_t maxs);
void		R_LightPoint (vec3_t point, vec3_t light);
void		R_SetLightLevel (void);

void		R_TouchLightmaps (void);

qBool		R_ShadowForEntity (entity_t *ent, vec3_t shadowSpot);
void		R_LightForEntity (entity_t *ent, int numVerts, byte *bArray);

//
// r_lightq3.c
//

void		R_Q3BSP_MarkLights (void);
void		R_Q3BSP_MarkLightsBModel (entity_t *ent, vec3_t mins, vec3_t maxs);
void		R_Q3BSP_AddDynamicLights (uint32 dLightBits, entity_t *ent, GLenum depthFunc, GLenum sFactor, GLenum dFactor);

void		R_Q3BSP_LightForEntity (entity_t *ent, int numVerts, byte *bArray);

void		R_Q3BSP_BuildLightmaps (int numLightmaps, int w, int h, const byte *data, mQ3BspLightmapRect_t *rects);

void		R_Q3BSP_TouchLightmaps (void);

//
// r_math.c
//

byte		R_FloatToByte (float x);

float		R_ColorNormalize (const float *in, float *out);

void		Matrix4_Copy2D (const mat4x4_t m1, mat4x4_t m2);
void		Matrix4_Multiply2D (const mat4x4_t m1, const mat4x4_t m2, mat4x4_t out);
void		Matrix4_Scale2D (mat4x4_t m, float x, float y);
void		Matrix4_Stretch2D (mat4x4_t m, float s, float t);
void		Matrix4_Translate2D (mat4x4_t m, float x, float y);

void		Matrix4_Multiply_Vector (const mat4x4_t m, const vec4_t v, vec4_t out);

//
// r_poly.c
//

void		R_AddPolysToList (void);
void		R_PushPoly (meshBuffer_t *mb);
qBool		R_PolyOverflow (meshBuffer_t *mb);

//
// r_shadow.c
//

#ifdef SHADOW_VOLUMES
void		R_DrawShadowVolumes (mesh_t *mesh, entity_t *ent, vec3_t mins, vec3_t maxs, float radius);
void		R_ShadowBlend (void);
#endif

void		R_SimpleShadow (entity_t *ent, vec3_t shadowSpot);

//
// r_sky.c
//

#define SKY_MAXCLIPVERTS	64
#define SKY_BOXSIZE			8192

void		R_ClipSkySurface (mBspSurface_t *surf);
void		R_AddSkyToList (void);

void		R_ClearSky (void);
void		R_DrawSky (meshBuffer_t *mb);

void		R_SetSky (char *name, float rotate, vec3_t axis);

void		R_SkyInit (void);
void		R_SkyShutdown (void);

//
// r_world.c
//

void		R_AddWorldToList (void);
void		R_AddQ2BrushModel (entity_t *ent);

void		R_WorldInit (void);
void		R_WorldShutdown (void);

//
// r_worldq3.c
//

void		R_AddQ3WorldToList (void);
void		R_AddQ3BrushModel (entity_t *ent);

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
qBool	GLimp_GetGammaRamp (uint16 *ramp);
void	GLimp_SetGammaRamp (uint16 *ramp);

void	GLimp_Shutdown (void);
qBool	GLimp_Init (void *hInstance, void *hWnd);
qBool	GLimp_AttemptMode (qBool fullScreen, int width, int height);
