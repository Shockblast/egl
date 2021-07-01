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
// r_main.c
//

#include "r_local.h"

glConfig_t	glConfig;
glMedia_t	glMedia;
glSpeeds_t	glSpeeds;
glState_t	glState;

vec3_t		r_forwardVec;
vec3_t		r_rightVec;
vec3_t		r_upVec;

mat4x4_t	r_modelViewMatrix;
mat4x4_t	r_projectionMatrix;
mat4x4_t	r_worldViewMatrix;

refDef_t	r_refDef;
vidDef_t	vidDef;

uLong		r_frameCount;	// used for dlight push checking
uLong		r_registrationFrame;

//
// Scene items
//

int				r_numEntities;
entity_t		r_entityList[MAX_ENTITIES];

int				r_numPolys;
poly_t			r_polyList[MAX_POLYS];

int				r_numDLights;
dLight_t		r_dLightList[MAX_DLIGHTS];

lightStyle_t	r_lightStyles[MAX_CS_LIGHTSTYLES];

//
// Cvars
//

cVar_t	*con_font;

cVar_t	*e_test_0;
cVar_t	*e_test_1;

cVar_t	*gl_3dlabs_broken;
cVar_t	*gl_bitdepth;
cVar_t	*gl_clear;
cVar_t	*gl_cull;
cVar_t	*gl_drawbuffer;
cVar_t	*gl_driver;
cVar_t	*gl_dynamic;
cVar_t	*gl_errorcheck;

cVar_t	*gl_extensions;
cVar_t	*gl_arb_texture_compression;
cVar_t	*gl_arb_texture_cube_map;

cVar_t	*gl_ext_compiled_vertex_array;
cVar_t	*gl_ext_draw_range_elements;
cVar_t	*gl_ext_fragment_program;
cVar_t	*gl_ext_max_anisotropy;
cVar_t	*gl_ext_multitexture;
cVar_t	*gl_ext_swapinterval;
cVar_t	*gl_ext_texture_edge_clamp;
cVar_t	*gl_ext_texture_env_add;
cVar_t	*gl_ext_texture_env_combine;
cVar_t	*gl_nv_texture_env_combine4;
cVar_t	*gl_ext_texture_env_dot3;
cVar_t	*gl_ext_texture_filter_anisotropic;
cVar_t	*gl_ext_vertex_buffer_object;
cVar_t	*gl_ext_vertex_program;
cVar_t	*gl_sgis_generate_mipmap;

cVar_t	*gl_finish;
cVar_t	*gl_flashblend;
cVar_t	*gl_lightmap;
cVar_t	*gl_lockpvs;
cVar_t	*gl_log;
cVar_t	*gl_maxTexSize;
cVar_t	*gl_mode;
cVar_t	*gl_modulate;

cVar_t	*gl_polyblend;
cVar_t	*gl_shadows;
cVar_t	*gl_shownormals;
cVar_t	*gl_showtris;
cVar_t	*gl_swapinterval;

cVar_t	*qgl_debug;

cVar_t	*r_bumpScale;
cVar_t	*r_caustics;
cVar_t	*r_detailTextures;
cVar_t	*r_displayFreq;
cVar_t	*r_drawEntities;
cVar_t	*r_drawworld;
cVar_t	*r_fontscale;
cVar_t	*r_fullbright;
cVar_t	*r_hwGamma;
cVar_t	*r_lerpmodels;
cVar_t	*r_lightlevel;
cVar_t	*r_noCull;
cVar_t	*r_noRefresh;
cVar_t	*r_noVis;
cVar_t	*r_speeds;
cVar_t	*r_sphereCull;

/*
=============================================================================

	SCENE

=============================================================================
*/

/*
============
R_PolyBlend

Fixed by Vic
============
*/
static void R_PolyBlend (void)
{
	if (!gl_polyblend->integer)
		return;
	if (r_refDef.vBlend[3] < 0.01f)
		return;

	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglEnable (GL_BLEND);
	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_TEXTURE_2D);

	qglMatrixMode (GL_PROJECTION);
	qglLoadIdentity ();
	qglOrtho (0, 1, 1, 0, -99999, 99999);

	qglMatrixMode (GL_MODELVIEW);
	qglLoadIdentity ();

	qglColor4fv (r_refDef.vBlend);
	qglBegin (GL_TRIANGLES);

	qglVertex2f (-5, -5);
	qglVertex2f (10, -5);
	qglVertex2f (-5, 10);

	qglEnd ();

	qglDisable (GL_BLEND);
	qglEnable (GL_TEXTURE_2D);
	qglEnable (GL_DEPTH_TEST);

	qglColor4f (1, 1, 1, 1);
}

// ==========================================================

/*
================
R_RenderScene

r_refDef must be set before the first call
================
*/
void R_RenderScene (refDef_t *rd)
{
	glState.in2D = qFalse;
	if (!r_noRefresh->integer) {
		r_refDef = *rd;
		r_currentList = &r_worldList;

		if (!r_worldModel && !(r_refDef.rdFlags & RDF_NOWORLDMODEL))
			Com_Error (ERR_DROP, "R_RenderScene: NULL worldmodel");

		RB_BeginFrame ();

		if (gl_finish->integer)
			qglFinish ();

		r_worldList.numMeshes = 0;
		r_worldList.numAdditiveMeshes = 0;

		r_refDef = *rd;

		GL_Setup3D ();
		R_SetupFrustum ();

		R_AddWorldToList ();
		R_AddPolysToList ();
		R_AddEntitiesToList ();
		R_AddDLightsToList ();
		R_SortMeshList ();
		R_DrawMeshList (qFalse);
		R_DrawMeshOutlines ();
		R_DrawNullModelList ();

		R_SetLightLevel ();
		R_PolyBlend ();

#ifdef SHADOW_VOLUMES
		R_ShadowBlend ();
#endif

		RB_EndFrame ();

		GL_Setup2D ();
	}
}


/*
==================
R_BeginFrame
==================
*/
void R_BeginFrame (float cameraSeparation)
{
	glState.cameraSeparation = cameraSeparation;
	glState.realTime = Sys_Milliseconds () * 0.001f;

	// Frame logging
	if (gl_log->modified) {
		gl_log->modified = qFalse;
		QGL_ToggleLogging ();
	}
	QGL_LogBeginFrame ();

	// Debugging
	if (qgl_debug->modified) {
		qgl_debug->modified = qFalse;
		QGL_ToggleDebug ();
	}

	// Apply cvar settings
	R_UpdateCvars ();

	// Setup the frame for rendering
	GLimp_BeginFrame ();

	// Go into 2D mode
	GL_Setup2D ();
}


/*
==================
R_EndFrame
==================
*/
void R_EndFrame (void)
{
	// Swap buffers
	GLimp_EndFrame ();

	// Go into 2D mode
	GL_Setup2D ();

	// Frame logging
	QGL_LogEndFrame ();

	// Rendering speeds
	switch (r_speeds->integer) {
	case 1:
		Com_Printf (0, "%4i wpoly %4i welem %3i leaf %4i vert %4i tris %4i element %3i/%3i shader/pass\n",
			glSpeeds.worldPolys, glSpeeds.worldElements, glSpeeds.worldLeafs,
			glSpeeds.numVerts, glSpeeds.numTris, glSpeeds.numElements, glSpeeds.shaderCount, glSpeeds.shaderPasses);
		break;

	case 2:
		Com_Printf (0, "%4i epoly %4i dlight %3i imgs %3i unitchg %3i envchg %4i bind\n",
			glSpeeds.aliasPolys, r_numDLights, r_numImages, glSpeeds.texUnitChanges, glSpeeds.texEnvChanges, glSpeeds.textureBinds);
		break;
	}
	memset (&glSpeeds, 0, sizeof (glSpeeds));

	// Next frame
	r_frameCount++;
}

// ==========================================================

/*
====================
R_ClearScene
====================
*/
void R_ClearScene (void)
{
	r_numEntities = 0;
	r_numPolys = 0;
	r_numDLights = 0;
}


/*
=====================
R_AddEntity
=====================
*/
void R_AddEntity (entity_t *ent)
{
	if (r_numEntities >= MAX_ENTITIES)
		return;
	if (ent->color[3] <= 0)
		return;

	r_entityList[r_numEntities] = *ent;
	if (ent->color[3] < 255)
		r_entityList[r_numEntities].flags |= RF_TRANSLUCENT;

	r_numEntities++;
}


/*
=====================
R_AddPoly
=====================
*/
void R_AddPoly (poly_t *poly)
{
	if (r_numPolys < MAX_POLYS)
		r_polyList[r_numPolys++] = *poly;
}


/*
=====================
R_AddLight
=====================
*/
void R_AddLight (vec3_t origin, float intensity, float r, float g, float b)
{
	dLight_t	*dl;

	if (r_numDLights >= MAX_DLIGHTS)
		return;

	if (!intensity)
		return;

	dl = &r_dLightList[r_numDLights++];

	VectorCopy (origin, dl->origin);
	VectorSet (dl->color, r, g, b);
	dl->intensity = intensity;

	R_LightBounds (origin, intensity, dl->mins, dl->maxs);
}


/*
=====================
R_AddLightStyle
=====================
*/
void R_AddLightStyle (int style, float r, float g, float b)
{
	lightStyle_t	*ls;

	if (style < 0 || style > MAX_CS_LIGHTSTYLES) {
		Com_Error (ERR_DROP, "Bad light style %i", style);
		return;
	}

	ls = &r_lightStyles[style];

	ls->white	= r+g+b;
	VectorSet (ls->rgb, r, g, b);
}

/*
=============================================================================

	MISC

=============================================================================
*/

/*
==================
GL_CheckForError
==================
*/
static inline const char *GetGLErrorString (GLenum error)
{
	switch (error) {
	case GL_INVALID_ENUM:		return ("INVALID ENUM");		break;
	case GL_INVALID_OPERATION:	return ("INVALID OPERATION");	break;
	case GL_INVALID_VALUE:		return ("INVALID VALUE");		break;
	case GL_NO_ERROR:			return ("NO ERROR");			break;
	case GL_OUT_OF_MEMORY:		return ("OUT OF MEMORY");		break;
	case GL_STACK_OVERFLOW:		return ("STACK OVERFLOW");		break;
	case GL_STACK_UNDERFLOW:	return ("STACK UNDERFLOW");		break;
	}

	return ("unknown");
}
void GL_CheckForError (char *where)
{
	int		err;

	err = qglGetError ();
	if (err != GL_NO_ERROR) {
		Com_Printf (0, S_COLOR_RED "GL_ERROR: glGetError (): '%s' (0x%x)", GetGLErrorString (err), err);
		if (where)
			Com_Printf (0, " %s\n", where);
		else
			Com_Printf (0, "\n");
	}
}


/*
==================
GL_GfxInfo_f
==================
*/
static void GL_GfxInfo_f (void)
{
	Com_Printf (0, "----------------------------------------\n");

	Com_Printf (0, "EGL v%s:\n" "GL_PFD: c(%d-bits) a(%d-bits) z(%d-bit) s(%d-bit)\n",
				EGL_VERSTR,
				glConfig.cColorBits, glConfig.cAlphaBits, glConfig.cDepthBits, glConfig.cStencilBits);

	Com_Printf (0, "Renderer Class: ");
	switch (glConfig.renderClass) {
	case REND_CLASS_DEFAULT:			Com_Printf (0, "Default");			break;
	case REND_CLASS_MCD:				Com_Printf (0, "MCD");				break;

	case REND_CLASS_3DLABS_GLINT_MX:	Com_Printf (0, "3DLabs GLIntMX");	break;
	case REND_CLASS_3DLABS_PERMEDIA:	Com_Printf (0, "3DLabs Permedia");	break;
	case REND_CLASS_3DLABS_REALIZM:		Com_Printf (0, "3DLabs Realizm");	break;
	case REND_CLASS_ATI:				Com_Printf (0, "ATi");				break;
	case REND_CLASS_ATI_RADEON:			Com_Printf (0, "ATi Radeon");		break;
	case REND_CLASS_INTEL:				Com_Printf (0, "Intel");			break;
	case REND_CLASS_NVIDIA:				Com_Printf (0, "nVidia");			break;
	case REND_CLASS_NVIDIA_GEFORCE:		Com_Printf (0, "nVidia GeForce");	break;
	case REND_CLASS_PMX:				Com_Printf (0, "PMX");				break;
	case REND_CLASS_POWERVR_PCX1:		Com_Printf (0, "PowerVR PCX1");		break;
	case REND_CLASS_POWERVR_PCX2:		Com_Printf (0, "PowerVR PCX2");		break;
	case REND_CLASS_RENDITION:			Com_Printf (0, "Rendition");		break;
	case REND_CLASS_S3:					Com_Printf (0, "S3");				break;
	case REND_CLASS_SGI:				Com_Printf (0, "SGI");				break;
	case REND_CLASS_SIS:				Com_Printf (0, "SiS");				break;
	case REND_CLASS_VOODOO:				Com_Printf (0, "Voodoo");			break;
	}
	Com_Printf (0, "\n");

	Com_Printf (0, "----------------------------------------\n");

	Com_Printf (0, "GL_VENDOR: %s\n",		glConfig.vendorString);
	Com_Printf (0, "GL_RENDERER: %s\n",		glConfig.rendererString);
	Com_Printf (0, "GL_VERSION: %s\n",		glConfig.versionString);
	Com_Printf (0, "GL_EXTENSIONS: %s\n",	glConfig.extensionString);

	Com_Printf (0, "----------------------------------------\n");

#ifdef _WIN32
	Com_Printf (0, "CDS: %s\n", (glConfig.allowCDS) ? "Enabled" : "Disabled");
#endif // _WIN32

	Com_Printf (0, "Extensions:\n");
	Com_Printf (0, "...ARB Multitexture: %s\n",				glConfig.extArbMultitexture ? "On" : "Off");
	Com_Printf (0, "...ARB Texture Compression: %s\n",		glConfig.extArbTexCompression ? "On" : "Off");

	Com_Printf (0, "...ARB Texture Cube Map: %s\n",			glConfig.extArbTexCubeMap ? "On" : "Off");
	if (glConfig.extArbTexCubeMap)
		Com_Printf (0, "...* Max cubemap texture size: %i\n", glConfig.maxCMTexSize);

	Com_Printf (0, "...Compiled Vertex Array: %s\n",		glConfig.extCompiledVertArray ? "On" : "Off");

	Com_Printf (0, "...Draw Range Elements: %s\n",			glConfig.extDrawRangeElements ? "On" : "Off");
	if (glConfig.extDrawRangeElements) {
		Com_Printf (0, "...* Max element vertices: %i\n", glConfig.maxElementVerts);
		Com_Printf (0, "...* Max element indices: %i\n", glConfig.maxElementIndices);
	}

	Com_Printf (0, "...Fragment programs: %s\n",			glConfig.extFragmentProgram ? "On" : "Off");
	if (glConfig.extFragmentProgram) {
		Com_Printf (0, "...* Max texture coordinates: %i\n", glConfig.maxTexCoords);
		Com_Printf (0, "...* Max texture image units: %i\n", glConfig.maxTexImageUnits);
	}

	Com_Printf (0, "...nVidia Texture Env Combine4: %s\n",	glConfig.extNVTexEnvCombine4 ? "On" : "Off");
	Com_Printf (0, "...SGIS Mipmap Generation: %s\n",		glConfig.extSGISGenMipmap ? "On" : "Off");
	Com_Printf (0, "...SGIS Multitexture: %s\n",			glConfig.extSGISMultiTexture ? "On" : glConfig.extArbMultitexture ? "Deprecated for ARB Multitexture" : "Off");
	Com_Printf (0, "...Texture Edge Clamp: %s\n",			glConfig.extTexEdgeClamp ? "On" : "Off");
	Com_Printf (0, "...Texture Env Add: %s\n",				glConfig.extTexEnvAdd ? "On" : "Off");
	Com_Printf (0, "...Texture Env Combine: %s\n",			glConfig.extTexEnvCombine ? "On" : "Off");
	Com_Printf (0, "...Texture Env DOT3: %s\n",				glConfig.extTexEnvDot3 ? "On" : "Off");

	Com_Printf (0, "...Texture Filter Anisotropic: %s\n",	glConfig.extTexFilterAniso ? "On" : "Off");
	if (glConfig.extTexFilterAniso)
		Com_Printf (0, "...* Max texture anisotropy: %i\n", glConfig.maxAniso);

	Com_Printf (0, "...Vertex Buffer Objects: %s\n",		glConfig.extVertexBufferObject ? "On" : "Off");
	Com_Printf (0, "...Vertex programs: %s\n",				glConfig.extVertexProgram ? "On" : "Off");

	Com_Printf (0, "----------------------------------------\n");

	GL_TextureMode (qTrue, qTrue);
	GL_TextureBits (qTrue, qTrue);

	Com_Printf (0, "----------------------------------------\n");

	Com_Printf (0, "Max texture size: %i\n", glConfig.maxTexSize);
	Com_Printf (0, "Max texture units: %i\n", glConfig.maxTexUnits);

	Com_Printf (0, "----------------------------------------\n");
}


/*
==================
GL_RendererMsg_f
==================
*/
void GL_RendererMsg_f (void)
{
	Cbuf_AddText (Q_VarArgs ("say [EGL v%s]: [%s: %s v%s] GL_PFD[c%d/a%d/z%d/s%d] RES[%dx%dx%d]\n",
				EGL_VERSTR,
				glConfig.vendorString, glConfig.rendererString, glConfig.versionString,
				glConfig.cColorBits, glConfig.cAlphaBits, glConfig.cDepthBits, glConfig.cStencilBits,
				vidDef.width, vidDef.height, glConfig.vidBitDepth));
}


/*
==================
GL_VersionMsg_f
==================
*/
void GL_VersionMsg_f (void)
{
	Cbuf_AddText (Q_VarArgs ("say [EGL v%s (%s-%s) by Echon] [http://egl.quakedev.com/]\n",
				EGL_VERSTR, BUILDSTRING, CPUSTRING));
}


/*
===============
ExtensionFound
===============
*/
static qBool ExtensionFound (const char *extensions, const char *extension)
{
	const byte	*start;
	byte		*where, *terminator;

	// Extension names should not have spaces
	where = (byte *) strchr (extension, ' ');
	if (where || (*extension == '\0'))
		return qFalse;

	start = extensions;
	for ( ; ; ) {
		where = (byte *) strstr ((const char *)start, extension);
		if (!where)
			break;
		terminator = where + strlen (extension);
		if ((where == start) || (*(where - 1) == ' ')) {
			if (*terminator == ' ' || *terminator == '\0') {
				return qTrue;
			}
		}
		start = terminator;
	}
	return qFalse;
}


/*
=============
R_TransformToScreen
=============
*/
void R_TransformToScreen (vec3_t in, vec2_t out)
{
	float	iw;
	vec4_t	temp, temp2;

	temp[0] = in[0];
	temp[1] = in[1];
	temp[2] = in[2];
	temp[3] = 1.0f;
	Matrix4_Multiply_Vec4 (r_worldViewMatrix, temp, temp2);
	Matrix4_Multiply_Vec4 (r_projectionMatrix, temp2, temp);

	iw = 1.0f / temp[3];
	out[0] = r_refDef.x + (temp[0] * iw + 1.0f) * r_refDef.width * 0.5f;
	out[1] = r_refDef.y + (temp[1] * iw + 1.0f) * r_refDef.height * 0.5f;
}

/*
==================
R_UpdateCvars

Updates scene based on cvar changes
==================
*/
static void R_UpdateCvars (void)
{
	// Draw buffer stuff
	if (gl_drawbuffer->modified) {
		gl_drawbuffer->modified = qFalse;
		if (!glState.cameraSeparation || !glState.stereoEnabled) {
			if (!Q_stricmp (gl_drawbuffer->string, "GL_FRONT"))
				qglDrawBuffer (GL_FRONT);
			else
				qglDrawBuffer (GL_BACK);
		}
	}

	// Update the swap interval
	if (gl_swapinterval->modified) {
		gl_swapinterval->modified = qFalse;

#ifdef _WIN32
		if (!glState.stereoEnabled && glConfig.extWinSwapInterval) 
			qwglSwapIntervalEXT (gl_swapinterval->integer);
#endif
	}

	// Texturemode stuff
	if (gl_texturemode->modified)
		GL_TextureMode (qFalse, qFalse);

	// Update anisotropy
	if (gl_ext_max_anisotropy->modified)
		GL_ResetAnisotropy ();

	// Update font
	if (con_font->modified)
		R_CheckFont ();

	// Gamma ramp
	if (glConfig.hwGamma && vid_gamma->modified)
		R_UpdateGammaRamp ();
}

/*
=============================================================================

	REGISTRATION

=============================================================================
*/

/*
==================
R_BeginRegistration

Starts refresh registration before map load
==================
*/
void R_BeginRegistration (void)
{
	r_registrationFrame++;

	R_BeginModelRegistration ();
	R_BeginImageRegistration ();
}


/*
==================
R_EndRegistration

Called at the end of all registration by the client
==================
*/
void R_EndRegistration (void)
{
	R_EndModelRegistration (); // Register first so shaders are touched
	R_EndShaderRegistration ();	// Register first so images are touched
	R_EndImageRegistration ();
	R_EndProgramRegistration ();
}


/*
==================
R_MediaInit
==================
*/
void R_MediaInit (void)
{
	// Chars image/shaders
	R_CheckFont ();

	// World Caustic shaders
	glMedia.worldLavaCaustics = R_RegisterTexture ("egl/lavacaustics", -1);
	glMedia.worldSlimeCaustics = R_RegisterTexture ("egl/slimecaustics", -1);
	glMedia.worldWaterCaustics = R_RegisterTexture ("egl/watercaustics", -1);
}

/*
=============================================================================

	INIT / SHUTDOWN

=============================================================================
*/

static cmdFunc_t	*cmd_gfxInfo;
static cmdFunc_t	*cmd_eglRenderer;
static cmdFunc_t	*cmd_eglVersion;

/*
============
R_GetInfoForMode
============
*/
typedef struct vidMode_s {
	char		*info;

	int			width;
	int			height;

	int			mode;
} vidMode_t;

static vidMode_t r_vidModes[] = {
	{"Mode 0: 320 x 240",			320,	240,	0 },
	{"Mode 1: 400 x 300",			400,	300,	1 },
	{"Mode 2: 512 x 384",			512,	384,	2 },
	{"Mode 3: 640 x 480",			640,	480,	3 },
	{"Mode 4: 800 x 600",			800,	600,	4 },
	{"Mode 5: 960 x 720",			960,	720,	5 },
	{"Mode 6: 1024 x 768",			1024,	768,	6 },
	{"Mode 7: 1152 x 864",			1152,	864,	7 },
	{"Mode 8: 1280 x 960",			1280,	960,	8 },
	{"Mode 9: 1600 x 1200",			1600,	1200,	9 },
	{"Mode 10: 1920 x 1440",		1920,	1440,	10},
	{"Mode 11: 2048 x 1536",		2048,	1536,	11},

	{"Mode 12: 1280 x 800 (ws)",	1280,	800,	12},
	{"Mode 13: 1440 x 900 (ws)",	1440,	900,	13}
};

#define NUM_VIDMODES (sizeof (r_vidModes) / sizeof (r_vidModes[0]))
qBool R_GetInfoForMode (int mode, int *width, int *height)
{
	if (mode < 0 || mode >= NUM_VIDMODES)
		return qFalse;

	*width  = r_vidModes[mode].width;
	*height = r_vidModes[mode].height;
	return qTrue;
}

/*
==================
R_SetMode
==================
*/
static qBool R_SetMode (void)
{
#ifdef _WIN32
	// Check if ChangeDisplaySettings is allowed (only fullscreen uses CDS in win32)
	if (vid_fullscreen->modified && !glConfig.allowCDS) {
		Com_Printf (0, S_COLOR_YELLOW "CDS not allowed with this driver\n");
		Cvar_VariableForceSetValue (vid_fullscreen, 0);
		vid_fullscreen->modified = qFalse;
	}
#endif // _WIN32

	// Set defaults
	vidDef.width = 0;
	vidDef.height = 0;
	glConfig.vidBitDepth = 0;
	glConfig.vidFrequency = 0;
	glConfig.vidFullScreen = (vid_fullscreen->integer) ? qTrue : qFalse;

	// Attempt the desired mode
	if (GLimp_AttemptMode (gl_mode->integer)) {
		// Store off as "safe" if it's not a custom mode
		if (!Cvar_VariableValue ("vid_width") && !Cvar_VariableValue ("vid_height"))
			glConfig.safeMode = gl_mode->integer;

		return qTrue;
	}

	// Bad mode, fall to the last known safe mode
	Com_Printf (0, "..." S_COLOR_YELLOW "invalid mode, attempting safe mode '%d'\n", glConfig.safeMode);
	Cvar_VariableForceSetValue (gl_mode, (float)glConfig.safeMode);

	// Set defaults
	vidDef.width = 0;
	vidDef.height = 0;
	glConfig.vidBitDepth = 0;
	glConfig.vidFrequency = 0;
	glConfig.vidFullScreen = (vid_fullscreen->integer) ? qTrue : qFalse;

	Cvar_ForceSetValue ("vid_width", 0);
	Cvar_ForceSetValue ("vid_height", 0);

	// Try setting it back to something safe
	if (!GLimp_AttemptMode (glConfig.safeMode)) {
		Com_Printf (0, "..." S_COLOR_RED "could not revert to safe mode\n");
		return qFalse;
	}

	return qTrue;
}


/*
===============
GL_InitExtensions
===============
*/
static void GL_InitExtensions (void)
{
	// Check for gl errors
	GL_CheckForError ("GL_InitExtensions");

	/*
	** GL_ARB_multitexture
	** GL_SGIS_multitexture
	*/
	if (gl_ext_multitexture->integer) {
		// GL_ARB_multitexture
		if (ExtensionFound (glConfig.extensionString, "GL_ARB_multitexture")) {
			qglMTexCoord2f = (void *) QGL_GetProcAddress ("glMultiTexCoord2fARB");
			if (qglMTexCoord2f)			qglActiveTextureARB = (void *) QGL_GetProcAddress ("glActiveTextureARB");
			if (qglActiveTextureARB)	qglClientActiveTextureARB = (void *) QGL_GetProcAddress ("glClientActiveTextureARB");

			if (!qglClientActiveTextureARB) {
				Com_Printf (0, "..." S_COLOR_RED "GL_ARB_multitexture not properly supported!\n");
				qglMTexCoord2f				= NULL;
				qglActiveTextureARB			= NULL;
				qglClientActiveTextureARB	= NULL;
			}
			else {
				Com_Printf (0, "...enabling GL_ARB_multitexture\n");
				glConfig.extArbMultitexture = qTrue;
			}
		}
		else
			Com_Printf (0, "...GL_ARB_multitexture not found\n");

		// GL_SGIS_multitexture
		if (!glConfig.extArbMultitexture) {
			Com_Printf (0, "...attempting GL_SGIS_multitexture\n");

			if (ExtensionFound (glConfig.extensionString, "GL_SGIS_multitexture")) {
				qglMTexCoord2f = (void *) QGL_GetProcAddress ("glMTexCoord2fSGIS");
				if (qglMTexCoord2f)		qglSelectTextureSGIS = (void *) QGL_GetProcAddress ("glSelectTextureSGIS");

				if (!qglSelectTextureSGIS) {
					Com_Printf (0, "..." S_COLOR_RED "GL_SGIS_multitexture not properly supported!\n");
					qglMTexCoord2f			= NULL;
					qglSelectTextureSGIS	= NULL;
				}
				else {
					Com_Printf (0, "...enabling GL_SGIS_multitexture\n");
					glConfig.extSGISMultiTexture = qTrue;
				}
			}
			else
				Com_Printf (0, "...GL_SGIS_multitexture not found\n");
		}
	}
	else {
		qglMTexCoord2f				= NULL;
		qglActiveTextureARB			= NULL;
		qglClientActiveTextureARB	= NULL;

		qglSelectTextureSGIS		= NULL;

		Com_Printf (0, "...ignoring GL_ARB/SGIS_multitexture\n");
		Com_Printf (0, S_COLOR_YELLOW "WARNING: Disabling multitexture is not recommended!\n");
	}

	// Keep texture unit counts in check
	if (glConfig.extSGISMultiTexture || glConfig.extArbMultitexture) {
		qglGetIntegerv (GL_MAX_TEXTURE_UNITS, &glConfig.maxTexUnits);

		if (glConfig.maxTexUnits < 1) {
			glConfig.maxTexUnits = 1;
			Com_Printf (0, "...* forcing 1 texture unit\n");
		}
		else {
			if (glConfig.extSGISMultiTexture && glConfig.maxTexUnits > 2) {
				// GL_SGIS_multitexture doesn't support more than 2 units does it?
				glConfig.maxTexUnits = 2;
				Com_Printf (0, "...* GL_SGIS_multitexture clamped to 2 texture units\n");
			}
			else if (glConfig.maxTexUnits > MAX_TEXUNITS) {
				// Clamp at the maximum amount the engine supports
				glConfig.maxTexUnits = MAX_TEXUNITS;
				Com_Printf (0, "...* clamped to engine maximum of %i texture units\n", glConfig.maxTexUnits);
			}
			else
				Com_Printf (0, "...* using video card maximum of %i texture units\n", glConfig.maxTexUnits);
		}
	}
	else {
		glConfig.maxTexUnits = 1;
		Com_Printf (0, "...* multitexture disabled/not found -- using 1 texture unit\n");
	}

	/*
	** GL_ARB_texture_compression
	*/
	if (gl_arb_texture_compression->integer) {
		if (ExtensionFound (glConfig.extensionString, "GL_ARB_texture_compression")) {
			Com_Printf (0, "...enabling GL_ARB_texture_compression\n");
			glConfig.extArbTexCompression = qTrue;
		}
		else
			Com_Printf (0, "...GL_ARB_texture_compression not found\n");
	}
	else
		Com_Printf (0, "...ignoring GL_ARB_texture_compression\n");

	/*
	** GL_ARB_texture_cube_map
	*/
	if (gl_arb_texture_cube_map->integer) {
		if (ExtensionFound (glConfig.extensionString, "GL_ARB_texture_cube_map")) {
			qglGetIntegerv (GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &glConfig.maxCMTexSize);

			if (glConfig.maxCMTexSize <= 0) {
				Com_Printf (0, S_COLOR_RED "GL_ARB_texture_cube_map not properly supported!\n");
				glConfig.maxCMTexSize = 0;
			}
			else {
				Q_NearestPow (&glConfig.maxCMTexSize, qTrue);

				Com_Printf (0, "...enabling GL_ARB_texture_cube_map\n");
				Com_Printf (0, "...* Max cubemap texture size: %i\n", glConfig.maxCMTexSize);
				glConfig.extArbTexCubeMap = qTrue;
			}
		}
		else
			Com_Printf (0, "...GL_ARB_texture_cube_map not found\n");
	}
	else
		Com_Printf (0, "...ignoring GL_ARB_texture_cube_map\n");

	/*
	** GL_ARB_texture_env_add
	*/
	if (gl_ext_texture_env_add->integer) {
		if (ExtensionFound (glConfig.extensionString, "GL_ARB_texture_env_add")) {
			if (glConfig.extSGISMultiTexture || glConfig.extArbMultitexture) {
				Com_Printf (0, "...enabling GL_ARB_texture_env_add\n");
				glConfig.extTexEnvAdd = qTrue;
			}
			else
				Com_Printf (0, "...ignoring GL_ARB_texture_env_add (no multitexture)\n");
		}
		else
			Com_Printf (0, "...GL_ARB_texture_env_add not found\n");
	}
	else
		Com_Printf (0, "...ignoring GL_ARB_texture_env_add\n");

	/*
	** GL_ARB_texture_env_combine
	** GL_EXT_texture_env_combine
	*/
	if (gl_ext_texture_env_combine->integer) {
		if (ExtensionFound (glConfig.extensionString, "GL_ARB_texture_env_combine") ||
			ExtensionFound (glConfig.extensionString, "GL_EXT_texture_env_combine")) {
			if (glConfig.extSGISMultiTexture || glConfig.extArbMultitexture) {
				Com_Printf (0, "...enabling GL_ARB/EXT_texture_env_combine\n");
				glConfig.extTexEnvCombine = qTrue;
			}
			else
				Com_Printf (0, "...ignoring GL_ARB/EXT_texture_env_combine (no multitexture)\n");
		}
		else
			Com_Printf (0, "...GL_ARB/EXT_texture_env_combine not found\n");
	}
	else
		Com_Printf (0, "...ignoring GL_ARB/EXT_texture_env_combine\n");

	/*
	** GL_NV_texture_env_combine4
	*/
	if (gl_nv_texture_env_combine4->integer) {
		if (ExtensionFound (glConfig.extensionString, "NV_texture_env_combine4")) {
			if (glConfig.extTexEnvCombine) {
				Com_Printf (0, "...enabling GL_NV_texture_env_combine4\n");
				glConfig.extNVTexEnvCombine4 = qTrue;
			}
			else
				Com_Printf (0, "...ignoring GL_NV_texture_env_combine4 (no combine)\n");
		}
		else
			Com_Printf (0, "...GL_NV_texture_env_combine4 not found\n");
	}
	else
		Com_Printf (0, "...ignoring GL_NV_texture_env_combine4\n");

	/*
	** GL_ARB_texture_env_dot3
	*/
	if (gl_ext_texture_env_dot3->integer) {
		if (ExtensionFound (glConfig.extensionString, "GL_ARB_texture_env_dot3")) {
			if (glConfig.extTexEnvCombine) {
				Com_Printf (0, "...enabling GL_ARB_texture_env_dot3\n");
				glConfig.extTexEnvDot3 = qTrue;
			}
			else
				Com_Printf (0, "...ignoring GL_ARB_texture_env_dot3 (no combine)\n");
		}
		else
			Com_Printf (0, "...GL_ARB_texture_env_dot3 not found\n");
	}
	else
		Com_Printf (0, "...ignoring GL_ARB_texture_env_dot3\n");

	/*
	** GL_ARB_vertex_program
	*/
	if (gl_ext_vertex_program->integer) {
		if (ExtensionFound (glConfig.extensionString, "GL_ARB_vertex_program")) {

			qglBindProgramARB = (void *) QGL_GetProcAddress("glBindProgramARB");
			if (qglBindProgramARB)				qglDeleteProgramsARB = (void *) QGL_GetProcAddress("glDeleteProgramsARB");
			if (qglDeleteProgramsARB)			qglGenProgramsARB = (void *) QGL_GetProcAddress("glGenProgramsARB");
			if (qglGenProgramsARB)				qglProgramStringARB = (void *) QGL_GetProcAddress("glProgramStringARB");
			if (qglProgramStringARB)			qglProgramEnvParameter4fARB = (void *) QGL_GetProcAddress("glProgramEnvParameter4fARB");
			if (qglProgramEnvParameter4fARB)	qglProgramLocalParameter4fARB = (void *) QGL_GetProcAddress("glProgramLocalParameter4fARB");

			if (!qglProgramLocalParameter4fARB) {
				Com_Printf (0, S_COLOR_RED "GL_ARB_vertex_program not properly supported!\n");
				qglBindProgramARB = NULL;
				qglDeleteProgramsARB = NULL;
				qglGenProgramsARB = NULL;
				qglProgramStringARB = NULL;
				qglProgramEnvParameter4fARB = NULL;
				qglProgramLocalParameter4fARB = NULL;
			}
			else {
				Com_Printf (0, "...enabling GL_ARB_vertex_program\n");
				glConfig.extVertexProgram = qTrue;
			}
		}
		else
			Com_Printf (0, "...GL_ARB_vertex_program not found\n");
	}
	else
		Com_Printf (0, "...ignoring GL_ARB_vertex_program\n");

	/*
	** GL_ARB_fragment_program
	*/
	if (gl_ext_fragment_program->integer) {
		if (ExtensionFound (glConfig.extensionString, "GL_ARB_fragment_program")) {
			qglGetIntegerv (GL_MAX_TEXTURE_COORDS_ARB, &glConfig.maxTexCoords);
			qglGetIntegerv (GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &glConfig.maxTexImageUnits);

			qglBindProgramARB = (void *) QGL_GetProcAddress("glBindProgramARB");
			if (qglBindProgramARB)				qglDeleteProgramsARB = (void *) QGL_GetProcAddress("glDeleteProgramsARB");
			if (qglDeleteProgramsARB)			qglGenProgramsARB = (void *) QGL_GetProcAddress("glGenProgramsARB");
			if (qglGenProgramsARB)				qglProgramStringARB = (void *) QGL_GetProcAddress("glProgramStringARB");
			if (qglProgramStringARB)			qglProgramEnvParameter4fARB = (void *) QGL_GetProcAddress("glProgramEnvParameter4fARB");
			if (qglProgramEnvParameter4fARB)	qglProgramLocalParameter4fARB = (void *) QGL_GetProcAddress("glProgramLocalParameter4fARB");

			if (!qglProgramLocalParameter4fARB) {
				Com_Printf (0, S_COLOR_RED "GL_ARB_fragment_program not properly supported!\n");
				qglBindProgramARB = NULL;
				qglDeleteProgramsARB = NULL;
				qglGenProgramsARB = NULL;
				qglProgramStringARB = NULL;
				qglProgramEnvParameter4fARB = NULL;
				qglProgramLocalParameter4fARB = NULL;
			}
			else {
				Com_Printf (0, "...enabling GL_ARB_fragment_program\n");
				Com_Printf (0, "...* Max texture coordinates: %i\n", glConfig.maxTexCoords);
				Com_Printf (0, "...* Max texture image units: %i\n", glConfig.maxTexImageUnits);
				glConfig.extFragmentProgram = qTrue;
			}
		}
		else
			Com_Printf (0, "...GL_ARB_fragment_program not found\n");
	}
	else
		Com_Printf (0, "...ignoring GL_ARB_fragment_program\n");

	/*
	** GL_ARB_vertex_buffer_object
	*/
	if (gl_ext_vertex_buffer_object->integer) {
		if (ExtensionFound (glConfig.extensionString, "GL_ARB_vertex_buffer_object")) {
			qglBindBufferARB = (void *) QGL_GetProcAddress ("glBindBufferARB");
			if (qglBindBufferARB)		qglDeleteBuffersARB = (void *) QGL_GetProcAddress ("glDeleteBuffersARB");
			if (qglDeleteBuffersARB)	qglGenBuffersARB = (void *) QGL_GetProcAddress ("glGenBuffersARB");
			if (qglGenBuffersARB)		qglIsBufferARB = (void *) QGL_GetProcAddress ("glIsBufferARB");
			if (qglIsBufferARB)			qglMapBufferARB = (void *) QGL_GetProcAddress ("glMapBufferARB");
			if (qglMapBufferARB)		qglUnmapBufferARB = (void *) QGL_GetProcAddress ("glUnmapBufferARB");
			if (qglUnmapBufferARB)		qglBufferDataARB = (void *) QGL_GetProcAddress ("glBufferDataARB");
			if (qglBufferDataARB)		qglBufferSubDataARB = (void *) QGL_GetProcAddress ("glBufferSubDataARB");

			if (!qglBufferDataARB) {
				Com_Printf (0, S_COLOR_RED "GL_ARB_vertex_buffer_object not properly supported!\n");
				qglBindBufferARB	= NULL;
				qglDeleteBuffersARB	= NULL;
				qglGenBuffersARB	= NULL;
				qglIsBufferARB		= NULL;
				qglMapBufferARB		= NULL;
				qglUnmapBufferARB	= NULL;
				qglBufferDataARB	= NULL;
				qglBufferSubDataARB	= NULL;
			}
			else {
				Com_Printf (0, "...enabling GL_ARB_vertex_buffer_object\n");
				glConfig.extVertexBufferObject = qTrue;
			}
		}
		else
			Com_Printf (0, "...GL_ARB_vertex_buffer_object not found\n");
	}
	else
		Com_Printf (0, "...ignoring GL_ARB_vertex_buffer_object\n");

	/*
	** GL_EXT_compiled_vertex_array
	** GL_SGI_compiled_vertex_array
	*/
	if (gl_ext_compiled_vertex_array->integer) {
		if (ExtensionFound (glConfig.extensionString, "GL_EXT_compiled_vertex_array") || 
			ExtensionFound (glConfig.extensionString, "GL_SGI_compiled_vertex_array")) {
			qglLockArraysEXT = (void *) QGL_GetProcAddress ("glLockArraysEXT");
			if (qglLockArraysEXT)	qglUnlockArraysEXT = (void *) QGL_GetProcAddress ("glUnlockArraysEXT");

			if (!qglUnlockArraysEXT) {
				Com_Printf (0, "..." S_COLOR_RED "GL_EXT/SGI_compiled_vertex_array not properly supported!\n");
				qglLockArraysEXT	= NULL;
				qglUnlockArraysEXT	= NULL;
			}
			else {
				Com_Printf (0, "...enabling GL_EXT/SGI_compiled_vertex_array\n");
				glConfig.extCompiledVertArray = qTrue;
			}
		}
		else
			Com_Printf (0, "...GL_EXT/SGI_compiled_vertex_array not found\n");
	}
	else
		Com_Printf (0, "...ignoring GL_EXT/SGI_compiled_vertex_array\n");

	/*
	** GL_EXT_draw_range_elements
	*/
	if (gl_ext_draw_range_elements->integer) {
		if (ExtensionFound (glConfig.extensionString, "GL_EXT_draw_range_elements")) {
			qglGetIntegerv (GL_MAX_ELEMENTS_VERTICES_EXT, &glConfig.maxElementVerts);
			qglGetIntegerv (GL_MAX_ELEMENTS_INDICES_EXT, &glConfig.maxElementIndices);

			if (glConfig.maxElementVerts < 0 || glConfig.maxElementIndices < 0) {
				glConfig.maxElementVerts = 0;
				glConfig.maxElementIndices = 0;
			}
			else {
				qglDrawRangeElementsEXT = (void *) QGL_GetProcAddress ("glDrawRangeElementsEXT");
				if (!qglDrawRangeElementsEXT)
					qglDrawRangeElementsEXT = (void *) QGL_GetProcAddress ("glDrawRangeElements");
			}

			if (!qglDrawRangeElementsEXT) {
				Com_Printf (0, "..." S_COLOR_RED "GL_EXT_draw_range_elements not properly supported!\n");
				qglDrawRangeElementsEXT = NULL;
				glConfig.maxElementIndices = 0;
				glConfig.maxElementVerts = 0;
			}
			else {
				Com_Printf (0, "...enabling GL_EXT_draw_range_elements\n");
				Com_Printf (0, "...* Max element vertices: %i\n", glConfig.maxElementVerts);
				Com_Printf (0, "...* Max element indices: %i\n", glConfig.maxElementIndices);
				glConfig.extDrawRangeElements = qTrue;
			}
		}
		else
			Com_Printf (0, "...GL_EXT_draw_range_elements not found\n");
	}
	else
		Com_Printf (0, "...ignoring GL_EXT_draw_range_elements\n");

	/*
	** GL_EXT_texture_edge_clamp
	*/
	if (gl_ext_texture_edge_clamp->integer) {
		if (ExtensionFound (glConfig.extensionString, "GL_EXT_texture_edge_clamp")) {
			Com_Printf (0, "...enabling GL_EXT_texture_edge_clamp\n");
			glConfig.extTexEdgeClamp = qTrue;
		}
		else
			Com_Printf (0, "...GL_EXT_texture_edge_clamp not found\n");
	}
	else
		Com_Printf (0, "...ignoring GL_EXT_texture_edge_clamp\n");

	/*
	** GL_EXT_texture_filter_anisotropic
	*/
	if (gl_ext_texture_filter_anisotropic->integer) {
		if (ExtensionFound (glConfig.extensionString, "GL_EXT_texture_filter_anisotropic")) {
			qglGetIntegerv (GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxAniso);
			if (glConfig.maxAniso <= 0) {
				Com_Printf (0, "..." S_COLOR_RED "GL_EXT_texture_filter_anisotropic not properly supported!\n");
				glConfig.maxAniso = 0;
			}
			else {
				Com_Printf (0, "...enabling GL_EXT_texture_filter_anisotropic\n");
				Com_Printf (0, "...* Max texture anisotropy: %i\n", glConfig.maxAniso);
				glConfig.extTexFilterAniso = qTrue;
			}
		}
		else
			Com_Printf (0, "...GL_EXT_texture_filter_anisotropic not found\n");
	}
	else
		Com_Printf (0, "...ignoring GL_EXT_texture_filter_anisotropic\n");

	/*
	** GL_SGIS_generate_mipmap
	*/
	if (gl_sgis_generate_mipmap->integer) {
		if (ExtensionFound (glConfig.extensionString, "GL_SGIS_generate_mipmap")) {
			if (gl_sgis_generate_mipmap->integer != 2
			&& (glConfig.renderClass == REND_CLASS_ATI || glConfig.renderClass == REND_CLASS_ATI_RADEON)) {
				Com_Printf (0, "...ignoring GL_SGIS_generate_mipmap because ATi sucks\n");
			}
			else {
				Com_Printf (0, "...enabling GL_SGIS_generate_mipmap\n");
				glConfig.extSGISGenMipmap = qTrue;
			}
		}
		else
			Com_Printf (0, "...GL_SGIS_generate_mipmap not found\n");
	}
	else
		Com_Printf (0, "...ignoring GL_SGIS_generate_mipmap\n");

#ifdef _WIN32
	/*
	** WGL_3DFX_gamma_control
	*/
	if (ExtensionFound (glConfig.extensionString, "WGL_3DFX_gamma_control")) {
		qwglGetDeviceGammaRamp3DFX = (BOOL (WINAPI *)(HDC, WORD *)) QGL_GetProcAddress ("wglGetDeviceGammaRamp3DFX");
		qwglSetDeviceGammaRamp3DFX = (BOOL (WINAPI *)(HDC, WORD *)) QGL_GetProcAddress ("wglSetDeviceGammaRamp3DFX");

		if (!qwglGetDeviceGammaRamp3DFX || !qwglSetDeviceGammaRamp3DFX) {
			Com_Printf (0, "..." S_COLOR_RED "WGL_3DFX_gamma_control not properly supported!\n");
		}
	}

	/*
	** WGL_EXT_swap_control
	*/
	if (gl_ext_swapinterval->integer) {
		if (ExtensionFound (glConfig.extensionString, "WGL_EXT_swap_control")) {
			if (!glConfig.extWinSwapInterval) {
				qwglSwapIntervalEXT = (BOOL (WINAPI *)(int)) QGL_GetProcAddress ("wglSwapIntervalEXT");

				if (qwglSwapIntervalEXT) {
					Com_Printf (0, "...enabling WGL_EXT_swap_control\n");
					glConfig.extWinSwapInterval = qTrue;
				}
				else
					Com_Printf (0, "..." S_COLOR_RED "WGL_EXT_swap_control not properly supported!\n");
			}
		}
		else
			Com_Printf (0, "...WGL_EXT_swap_control not found\n");
	}
	else
		Com_Printf (0, "...ignoring WGL_EXT_swap_control\n");
#endif // _WIN32
}


/*
==================
R_Register

Registers the renderer's cvars/commands and gets
the latched ones during a vid_restart
==================
*/
static void R_Register (void)
{
	Cvar_GetLatchedVars (CVAR_LATCH_VIDEO);

	con_font			= Cvar_Get ("con_font",				"conchars",		CVAR_ARCHIVE);

	e_test_0			= Cvar_Get ("e_test_0",				"0",			0);
	e_test_1			= Cvar_Get ("e_test_1",				"0",			0);

	gl_3dlabs_broken	= Cvar_Get ("gl_3dlabs_broken",		"1",			CVAR_ARCHIVE);
	gl_bitdepth			= Cvar_Get ("gl_bitdepth",			"0",			CVAR_LATCH_VIDEO);
	gl_clear			= Cvar_Get ("gl_clear",				"0",			0);
	gl_cull				= Cvar_Get ("gl_cull",				"1",			0);
	gl_drawbuffer		= Cvar_Get ("gl_drawbuffer",		"GL_BACK",		0);
	gl_driver			= Cvar_Get ("gl_driver",			GL_DRIVERNAME,	CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_dynamic			= Cvar_Get ("gl_dynamic",			"1",			0);
	gl_errorcheck		= Cvar_Get ("gl_errorcheck",		"1",			CVAR_ARCHIVE);

	gl_extensions					= Cvar_Get ("gl_extensions",				"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_arb_texture_compression		= Cvar_Get ("gl_arb_texture_compression",	"0",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_arb_texture_cube_map			= Cvar_Get ("gl_arb_texture_cube_map",		"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_ext_compiled_vertex_array	= Cvar_Get ("gl_ext_compiled_vertex_array",	"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_ext_draw_range_elements		= Cvar_Get ("gl_ext_draw_range_elements",	"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_ext_fragment_program			= Cvar_Get ("gl_ext_fragment_program",		"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_ext_max_anisotropy			= Cvar_Get ("gl_ext_max_anisotropy",		"2",		CVAR_ARCHIVE);
	gl_ext_swapinterval				= Cvar_Get ("gl_ext_swapinterval",			"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_ext_multitexture				= Cvar_Get ("gl_ext_multitexture",			"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_ext_texture_env_add			= Cvar_Get ("gl_ext_texture_env_add",		"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_ext_texture_edge_clamp		= Cvar_Get ("gl_ext_texture_edge_clamp",	"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_ext_texture_env_combine		= Cvar_Get ("gl_ext_texture_env_combine",	"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_nv_texture_env_combine4		= Cvar_Get ("gl_nv_texture_env_combine4",	"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_ext_texture_env_dot3			= Cvar_Get ("gl_ext_texture_env_dot3",		"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_ext_texture_filter_anisotropic=Cvar_Get ("gl_ext_texture_filter_anisotropic","0",	CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_ext_vertex_buffer_object		= Cvar_Get ("gl_ext_vertex_buffer_object",	"0",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_ext_vertex_program			= Cvar_Get ("gl_ext_vertex_program",		"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_sgis_generate_mipmap			= Cvar_Get ("gl_sgis_generate_mipmap",		"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);

	gl_ext_max_anisotropy->modified	= qFalse; // So that setting the anisotropy texparam is not done twice on init

	gl_finish			= Cvar_Get ("gl_finish",			"0",			CVAR_ARCHIVE);
	gl_flashblend		= Cvar_Get ("gl_flashblend",		"0",			CVAR_ARCHIVE);
	gl_lightmap			= Cvar_Get ("gl_lightmap",			"0",			0);
	gl_lockpvs			= Cvar_Get ("gl_lockpvs",			"0",			0);
	gl_log				= Cvar_Get ("gl_log",				"0",			0);
	gl_maxTexSize		= Cvar_Get ("gl_maxTexSize",		"0",			CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_mode				= Cvar_Get ("gl_mode",				"3",			CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_modulate			= Cvar_Get ("gl_modulate",			"1",			CVAR_ARCHIVE);

	gl_polyblend		= Cvar_Get ("gl_polyblend",			"1",			0);
	gl_shadows			= Cvar_Get ("gl_shadows",			"0",			CVAR_ARCHIVE);
	gl_shownormals		= Cvar_Get ("gl_shownormals",		"0",			0);
	gl_showtris			= Cvar_Get ("gl_showtris",			"0",			0);
	gl_swapinterval		= Cvar_Get ("gl_swapinterval",		"1",			CVAR_ARCHIVE);

	qgl_debug			= Cvar_Get ("qgl_debug",			"0",			0);

	r_bumpScale			= Cvar_Get ("r_bumpScale",			"1",			CVAR_ARCHIVE);
	r_caustics			= Cvar_Get ("r_caustics",			"1",			CVAR_ARCHIVE);
	r_detailTextures	= Cvar_Get ("r_detailTextures",		"1",			CVAR_ARCHIVE);
	r_displayFreq		= Cvar_Get ("r_displayfreq",		"0",			CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_drawEntities		= Cvar_Get ("r_drawEntities",		"1",			0);
	r_drawworld			= Cvar_Get ("r_drawworld",			"1",			0);
	r_fontscale			= Cvar_Get ("r_fontscale",			"1",			CVAR_ARCHIVE);
	r_fullbright		= Cvar_Get ("r_fullbright",			"0",			0);
	r_hwGamma			= Cvar_Get ("r_hwGamma",			"0",			CVAR_ARCHIVE);
	r_lerpmodels		= Cvar_Get ("r_lerpmodels",			"1",			0);
	r_lightlevel		= Cvar_Get ("r_lightlevel",			"0",			0);
	r_noCull			= Cvar_Get ("r_noCull",				"0",			0);
	r_noRefresh			= Cvar_Get ("r_noRefresh",			"0",			0);
	r_noVis				= Cvar_Get ("r_noVis",				"0",			0);
	r_speeds			= Cvar_Get ("r_speeds",				"0",			0);
	r_sphereCull		= Cvar_Get ("r_sphereCull",			"1",			0);

	// Add the various commands
	cmd_gfxInfo		= Cmd_AddCommand ("gfxinfo",		GL_GfxInfo_f,			"Prints out renderer information");
	cmd_eglRenderer	= Cmd_AddCommand ("egl_renderer",	GL_RendererMsg_f,		"Spams to the server your renderer information");
	cmd_eglVersion	= Cmd_AddCommand ("egl_version",	GL_VersionMsg_f,		"Spams to the server your client version");
}


/*
===============
R_Init
===============
*/
qBool R_Init (void *hInstance, void *hWnd)
{
	char	*rendererBuffer;
	char	*vendorBuffer;
	int		refInitTime;

	refInitTime = Sys_Milliseconds ();
	Com_Printf (0, "\n-------- Refresh Initialization --------\n");

	r_frameCount = 1;
	r_registrationFrame = 1;

	// Register renderer cvars
	R_Register ();

	// Set extension/max defaults
	glConfig.extArbMultitexture = qFalse;
	glConfig.extArbTexCompression = qFalse;
	glConfig.extArbTexCubeMap = qFalse;
	glConfig.extCompiledVertArray = qFalse;
	glConfig.extDrawRangeElements = qFalse;
	glConfig.extFragmentProgram = qFalse;
	glConfig.extSGISGenMipmap = qFalse;
	glConfig.extSGISMultiTexture = qFalse;
	glConfig.extTexEdgeClamp = qFalse;
	glConfig.extTexEnvAdd = qFalse;
	glConfig.extTexEnvCombine = qFalse;
	glConfig.extNVTexEnvCombine4 = qFalse;
	glConfig.extTexEnvDot3 = qFalse;
	glConfig.extTexFilterAniso = qFalse;
	glConfig.extVertexBufferObject = qFalse;
	glConfig.extVertexProgram = qFalse;
	glConfig.extWinSwapInterval = qFalse;

	glConfig.maxElementVerts = 0;
	glConfig.maxElementIndices = 0;

	glConfig.maxAniso = 0;
	glConfig.maxCMTexSize = 0;
	glConfig.maxTexSize = 256;
	glConfig.maxTexUnits = 1;

	glConfig.useStencil = qFalse;
	glConfig.cColorBits = 0;
	glConfig.cAlphaBits = 0;
	glConfig.cDepthBits = 0;
	glConfig.cStencilBits = 0;

	// Set our "safe" mode
	glConfig.safeMode = 3;

	// Initialize our QGL dynamic bindings
	if (!QGL_Init (gl_driver->string)) {
		QGL_Shutdown ();
		Com_Printf (0, "..." S_COLOR_RED "could not load \"%s\"\n", gl_driver->string);
		return qFalse;
	}

	// Initialize OS-specific parts of OpenGL
	if (!GLimp_Init (hInstance, hWnd)) {
		Com_Printf (0, "..." S_COLOR_RED "unable to init gl implementation\n");
		QGL_Shutdown ();
		return qFalse;
	}

	// Create the window and set up the context
	if (!R_SetMode ()) {
		Com_Printf (0, "..." S_COLOR_RED "could not set video mode\n");
		QGL_Shutdown ();
		return qFalse;
	}

	// Vendor string
	glConfig.vendorString = qglGetString (GL_VENDOR);
	Com_Printf (0, "GL_VENDOR: %s\n", glConfig.vendorString);

	vendorBuffer = Mem_StrDup (glConfig.vendorString);
	Q_strlwr (vendorBuffer);

	// Renderer string
	glConfig.rendererString = qglGetString (GL_RENDERER);
	Com_Printf (0, "GL_RENDERER: %s\n", glConfig.rendererString);

	rendererBuffer = Mem_StrDup (glConfig.rendererString);
	Q_strlwr (rendererBuffer);

	// Version string
	glConfig.versionString = qglGetString (GL_VERSION);
	Com_Printf (0, "GL_VERSION: %s\n", glConfig.versionString);

	// Extension string
	glConfig.extensionString = qglGetString (GL_EXTENSIONS);

	// Decide on a renderer class
	if (strstr (rendererBuffer, "glint"))			glConfig.renderClass = REND_CLASS_3DLABS_GLINT_MX;
	else if (strstr (rendererBuffer, "permedia"))	glConfig.renderClass = REND_CLASS_3DLABS_PERMEDIA;
	else if (strstr (rendererBuffer, "glzicd"))		glConfig.renderClass = REND_CLASS_3DLABS_REALIZM;
	else if (strstr (vendorBuffer, "ati ")) {
		if (strstr (vendorBuffer, "radeon"))
			glConfig.renderClass = REND_CLASS_ATI_RADEON;
		else
			glConfig.renderClass = REND_CLASS_ATI;
	}
	else if (strstr (vendorBuffer, "intel"))		glConfig.renderClass = REND_CLASS_INTEL;
	else if (strstr (vendorBuffer, "nvidia")) {
		if (strstr (rendererBuffer, "geforce"))
			glConfig.renderClass = REND_CLASS_NVIDIA_GEFORCE;
		else
			glConfig.renderClass = REND_CLASS_NVIDIA;
	}
	else if (strstr	(rendererBuffer, "pmx"))		glConfig.renderClass = REND_CLASS_PMX;
	else if (strstr	(rendererBuffer, "pcx1"))		glConfig.renderClass = REND_CLASS_POWERVR_PCX1;
	else if (strstr	(rendererBuffer, "pcx2"))		glConfig.renderClass = REND_CLASS_POWERVR_PCX2;
	else if (strstr	(rendererBuffer, "verite"))		glConfig.renderClass = REND_CLASS_RENDITION;
	else if (strstr (vendorBuffer, "s3"))			glConfig.renderClass = REND_CLASS_S3;
	else if (strstr (rendererBuffer, "prosavage"))	glConfig.renderClass = REND_CLASS_S3;
	else if (strstr (rendererBuffer, "twister"))	glConfig.renderClass = REND_CLASS_S3;
	else if (strstr	(vendorBuffer, "sgi"))			glConfig.renderClass = REND_CLASS_SGI;
	else if (strstr	(vendorBuffer, "sis"))			glConfig.renderClass = REND_CLASS_SIS;
	else if (strstr (rendererBuffer, "voodoo"))		glConfig.renderClass = REND_CLASS_VOODOO;
	else {
		if (strstr (rendererBuffer, "gdi generic")) {
			glConfig.renderClass = REND_CLASS_MCD;

			// MCD has buffering issues
			Cvar_VariableForceSetValue (gl_finish, 1);
		}
		else
			glConfig.renderClass = REND_CLASS_DEFAULT;
	}

#ifdef GL_FORCEFINISH
	Cvar_VariableForceSetValue (gl_finish, 1);
#endif

	// Check stencil buffer availability and usability
	Com_Printf (0, "...stencil buffer ");
	if (gl_stencilbuffer->integer && glConfig.cStencilBits > 0) {
		if (glConfig.renderClass == REND_CLASS_VOODOO)
			Com_Printf (0, "ignored\n");
		else {
			Com_Printf (0, "available\n");
			glConfig.useStencil = qTrue;
		}
	}
	else {
		Com_Printf (0, "disabled\n");
	}

#ifdef _WIN32
	// Check if CDS is allowed
	switch (glConfig.renderClass) {
	case REND_CLASS_3DLABS_GLINT_MX:
	case REND_CLASS_3DLABS_PERMEDIA:
	case REND_CLASS_3DLABS_REALIZM:
		if (gl_3dlabs_broken->integer)
			glConfig.allowCDS = qFalse;
		else
			glConfig.allowCDS = qTrue;
		break;

	default:
		glConfig.allowCDS = qTrue;
		break;
	}

	// Notify if CDS is allowed
	if (!glConfig.allowCDS)
		Com_Printf (0, "...disabling CDS\n");
#endif // _WIN32

	// Grab opengl extensions
	if (gl_extensions->integer)
		GL_InitExtensions ();
	else
		Com_Printf (0, "...ignoring OpenGL extensions\n");

	// Set the default state
	GL_SetDefaultState ();

	// Retreive generic information
	if (gl_maxTexSize->integer > 0) {
		glConfig.maxTexSize = gl_maxTexSize->integer;
		Q_NearestPow (&glConfig.maxTexSize, qTrue);

		Com_Printf (0, "Using forced maximum texture size of: %ix%i\n", glConfig.maxTexSize, glConfig.maxTexSize);
	}
	else {
		qglGetIntegerv (GL_MAX_TEXTURE_SIZE, &glConfig.maxTexSize);
		Q_NearestPow (&glConfig.maxTexSize, qTrue);
		if (glConfig.maxTexSize < 256) {
			Com_Printf (0, "Maximum texture size forced up to 256x256 from %i\n", glConfig.maxTexSize);
			glConfig.maxTexSize = 256;
		}
		else
			Com_Printf (0, "Using video card maximum texture size of %ix%i\n", glConfig.maxTexSize, glConfig.maxTexSize);
	}

	Com_Printf (0, "----------------------------------------\n");

	// Sub-system init
	R_ImageInit ();
	R_ProgramInit ();
	R_ShaderInit ();
	R_MediaInit ();
	R_ModelInit ();
	R_WorldInit ();
	RB_Init ();

	Com_Printf (0, "init time: %.2fs\n", (Sys_Milliseconds () - refInitTime)*0.001f);
	Com_Printf (0, "----------------------------------------\n");

	// Check for gl errors
	GL_CheckForError ("R_Init");

	Mem_Free (rendererBuffer);
	Mem_Free (vendorBuffer);

	// Apply cvar settings
	R_UpdateCvars ();

	return qTrue;
}


/*
===============
R_Shutdown
===============
*/
void R_Shutdown (void)
{
	// Remove commands
	Cmd_RemoveCommand ("gfxinfo", cmd_gfxInfo);
	Cmd_RemoveCommand ("egl_renderer", cmd_eglRenderer);
	Cmd_RemoveCommand ("egl_version", cmd_eglVersion);

	// Shutdown subsystems
	R_ShaderShutdown ();
	R_ProgramShutdown ();
	R_ImageShutdown ();
	R_ModelShutdown ();
	R_WorldShutdown ();
	RB_Shutdown ();

	// Shutdown OS specific OpenGL stuff like contexts, etc
	GLimp_Shutdown ();

	// Shutdown our QGL subsystem
	QGL_Shutdown ();
}
