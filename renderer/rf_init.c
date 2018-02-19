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
// rf_init.c
//

#include "rf_local.h"

cVar_t	*e_test_0;
cVar_t	*e_test_1;

cVar_t	*gl_bitdepth;
cVar_t	*gl_clear;
cVar_t	*gl_cull;
cVar_t	*gl_drawbuffer;
cVar_t	*gl_dynamic;
cVar_t	*gl_errorcheck;

cVar_t	*r_allowExtensions;
cVar_t	*r_ext_BGRA;
cVar_t	*r_ext_compiledVertexArray;
cVar_t	*r_ext_drawRangeElements;
cVar_t	*r_ext_fragmentProgram;
cVar_t	*r_ext_generateMipmap;
cVar_t	*r_ext_maxAnisotropy;
cVar_t	*r_ext_multitexture;
cVar_t	*r_ext_stencilTwoSide;
cVar_t	*r_ext_stencilWrap;
cVar_t	*r_ext_swapInterval;
cVar_t	*r_ext_texture3D;
cVar_t	*r_ext_textureCompression;
cVar_t	*r_ext_textureCubeMap;
cVar_t	*r_ext_textureEdgeClamp;
cVar_t	*r_ext_textureEnvAdd;
cVar_t	*r_ext_textureEnvCombine;
cVar_t	*r_ext_textureEnvCombineNV4;
cVar_t	*r_ext_textureEnvDot3;
cVar_t	*r_ext_textureFilterAnisotropic;
cVar_t	*r_ext_vertexBufferObject;
cVar_t	*r_ext_vertexProgram;

cVar_t	*gl_finish;
cVar_t	*gl_flashblend;
cVar_t	*gl_lightmap;
cVar_t	*gl_lockpvs;
cVar_t	*gl_log;
cVar_t	*gl_maxTexSize;
cVar_t	*gl_mode;
cVar_t	*gl_modulate;

cVar_t	*gl_shadows;
cVar_t	*gl_shownormals;
cVar_t	*gl_showtris;

cVar_t	*qgl_debug;

cVar_t	*r_caustics;
cVar_t	*r_colorMipLevels;
cVar_t	*r_debugBatching;
cVar_t	*r_debugCulling;
cVar_t	*r_debugLighting;
cVar_t	*r_debugSorting;
cVar_t	*r_defaultFont;
cVar_t	*r_detailTextures;
cVar_t	*r_displayFreq;
cVar_t	*r_drawDecals;
cVar_t	*r_drawEntities;
cVar_t	*r_drawPolys;
cVar_t	*r_drawworld;
cVar_t	*r_facePlaneCull;
cVar_t	*r_flares;
cVar_t	*r_flareFade;
cVar_t	*r_flareSize;
cVar_t	*r_fontScale;
cVar_t	*r_fullbright;
cVar_t	*r_hwGamma;
cVar_t	*r_lerpmodels;
cVar_t	*r_lightlevel;
cVar_t	*r_lmMaxBlockSize;
cVar_t	*r_lmModulate;
cVar_t	*r_lmPacking;
cVar_t	*r_noCull;
cVar_t	*r_noRefresh;
cVar_t	*r_noVis;
cVar_t	*r_offsetFactor;
cVar_t	*r_offsetUnits;
cVar_t	*r_patchDivLevel;
cVar_t	*r_roundImagesDown;
cVar_t	*r_skipBackend;
cVar_t	*r_speeds;
cVar_t	*r_sphereCull;
cVar_t	*r_swapInterval;
cVar_t	*r_textureBits;
cVar_t	*r_times;
cVar_t	*r_vertexLighting;
cVar_t	*r_zFarAbs;
cVar_t	*r_zFarMin;
cVar_t	*r_zNear;

cVar_t	*r_alphabits;
cVar_t	*r_colorbits;
cVar_t	*r_depthbits;
cVar_t	*r_stencilbits;
cVar_t	*cl_stereo;
cVar_t	*gl_allow_software;
cVar_t	*gl_stencilbuffer;

cVar_t	*vid_gamma;
cVar_t	*vid_gammapics;
cVar_t	*vid_width;
cVar_t	*vid_height;

cVar_t	*intensity;

cVar_t	*gl_nobind;
cVar_t	*gl_picmip;
cVar_t	*gl_screenshot;
cVar_t	*gl_texturemode;

static void	*cmd_gfxInfo;
static void	*cmd_rendererClass;
static void	*cmd_eglRenderer;
static void	*cmd_eglVersion;

/*
=============================================================================

	CONSOLE COMMANDS

=============================================================================
*/

/*
==================
R_RendererClass
==================
*/
static char *R_RendererClass (void)
{
	switch (ri.renderClass) {
	case REND_CLASS_DEFAULT:			return "Default";
	case REND_CLASS_MCD:				return "MCD";

	case REND_CLASS_3DLABS_GLINT_MX:	return "3DLabs GLIntMX";
	case REND_CLASS_3DLABS_PERMEDIA:	return "3DLabs Permedia";
	case REND_CLASS_3DLABS_REALIZM:		return "3DLabs Realizm";
	case REND_CLASS_ATI:				return "ATi";
	case REND_CLASS_ATI_RADEON:			return "ATi Radeon";
	case REND_CLASS_INTEL:				return "Intel";
	case REND_CLASS_NVIDIA:				return "nVidia";
	case REND_CLASS_NVIDIA_GEFORCE:		return "nVidia GeForce";
	case REND_CLASS_PMX:				return "PMX";
	case REND_CLASS_POWERVR_PCX1:		return "PowerVR PCX1";
	case REND_CLASS_POWERVR_PCX2:		return "PowerVR PCX2";
	case REND_CLASS_RENDITION:			return "Rendition";
	case REND_CLASS_S3:					return "S3";
	case REND_CLASS_SGI:				return "SGI";
	case REND_CLASS_SIS:				return "SiS";
	case REND_CLASS_VOODOO:				return "Voodoo";
	}

	return "";
}


/*
==================
R_RendererClass_f
==================
*/
static void R_RendererClass_f (void)
{
	Com_Printf (0, "Renderer Class: %s\n", R_RendererClass ());
}


/*
==================
R_GfxInfo_f
==================
*/
static void R_GfxInfo_f (void)
{
	Com_Printf (0, "----------------------------------------\n");

	Com_Printf (0, "EGL v%s:\n" "GL_PFD: c(%d-bits) a(%d-bits) z(%d-bit) s(%d-bit)\n",
				EGL_VERSTR,
				ri.cColorBits, ri.cAlphaBits, ri.cDepthBits, ri.cStencilBits);

	Com_Printf (0, "Renderer Class: %s\n", R_RendererClass ());

	Com_Printf (0, "----------------------------------------\n");

	Com_Printf (0, "GL_VENDOR: %s\n",		ri.vendorString);
	Com_Printf (0, "GL_RENDERER: %s\n",		ri.rendererString);
	Com_Printf (0, "GL_VERSION: %s\n",		ri.versionString);
	Com_Printf (0, "GL_EXTENSIONS: %s\n",	ri.extensionString);

	Com_Printf (0, "----------------------------------------\n");

	Com_Printf (0, "Extensions:\n");
	Com_Printf (0, "...ARB Multitexture: %s\n",				ri.config.extArbMultitexture ? "On" : "Off");

	Com_Printf (0, "...BGRA: %s\n",							ri.config.extBGRA ? "On" : "Off");
	Com_Printf (0, "...Compiled Vertex Array: %s\n",		ri.config.extCompiledVertArray ? "On" : "Off");

	Com_Printf (0, "...Draw Range Elements: %s\n",			ri.config.extDrawRangeElements ? "On" : "Off");
	if (ri.config.extDrawRangeElements) {
		Com_Printf (0, "...* Max element vertices: %i\n",	ri.config.maxElementVerts);
		Com_Printf (0, "...* Max element indices: %i\n",	ri.config.maxElementIndices);
	}

	Com_Printf (0, "...Fragment programs: %s\n",			ri.config.extFragmentProgram ? "On" : "Off");
	if (ri.config.extFragmentProgram) {
		Com_Printf (0, "...* Max texture coordinates: %i\n",ri.config.maxTexCoords);
		Com_Printf (0, "...* Max texture image units: %i\n",ri.config.maxTexImageUnits);
	}

	Com_Printf (0, "...nVidia Texture Env Combine4: %s\n",	ri.config.extNVTexEnvCombine4 ? "On" : "Off");
	Com_Printf (0, "...SGIS Mipmap Generation: %s\n",		ri.config.extSGISGenMipmap ? "On" : "Off");
	Com_Printf (0, "...SGIS Multitexture: %s\n",			ri.config.extSGISMultiTexture ? "On" : ri.config.extArbMultitexture ? "Deprecated for ARB Multitexture" : "Off");
	Com_Printf (0, "...Stencil Two Side: %s\n",				ri.config.extStencilTwoSide ? "On" : "Off");
	Com_Printf (0, "...Stencil Wrap: %s\n",					ri.config.extStencilWrap ? "On" : "Off");

	Com_Printf (0, "...Texture Cube Map: %s\n",				ri.config.extTexCubeMap ? "On" : "Off");
	if (ri.config.extTexCubeMap)
		Com_Printf (0, "...* Max cubemap texture size: %i\n",ri.config.maxCMTexSize);

	Com_Printf (0, "...Texture Compression: %s\n",			ri.config.extTexCompression ? "On" : "Off");
	Com_Printf (0, "...Texture 3D: %s\n",					ri.config.extTex3D ? "On" : "Off");
	Com_Printf (0, "...Texture Edge Clamp: %s\n",			ri.config.extTexEdgeClamp ? "On" : "Off");
	Com_Printf (0, "...Texture Env Add: %s\n",				ri.config.extTexEnvAdd ? "On" : "Off");
	Com_Printf (0, "...Texture Env Combine: %s\n",			ri.config.extTexEnvCombine ? "On" : "Off");
	Com_Printf (0, "...Texture Env DOT3: %s\n",				ri.config.extTexEnvDot3 ? "On" : "Off");

	Com_Printf (0, "...Texture Filter Anisotropic: %s\n",	ri.config.extTexFilterAniso ? "On" : "Off");
	if (ri.config.extTexFilterAniso)
		Com_Printf (0, "...* Max texture anisotropy: %i\n",	ri.config.maxAniso);

	Com_Printf (0, "...Vertex Buffer Objects: %s\n",		ri.config.extVertexBufferObject ? "On" : "Off");
	Com_Printf (0, "...Vertex programs: %s\n",				ri.config.extVertexProgram ? "On" : "Off");

	Com_Printf (0, "----------------------------------------\n");

	GL_TextureMode (qTrue, qTrue);
	GL_TextureBits (qTrue, qTrue);

	Com_Printf (0, "----------------------------------------\n");

	Com_Printf (0, "Max texture size: %i\n", ri.config.maxTexSize);
	Com_Printf (0, "Max texture units: %i\n", ri.config.maxTexUnits);

	Com_Printf (0, "----------------------------------------\n");
}


/*
==================
R_RendererMsg_f
==================
*/
static void R_RendererMsg_f (void)
{
	Cbuf_AddText (Q_VarArgs ("say [EGL v%s]: [%s: %s v%s] GL_PFD[c%d/a%d/z%d/s%d] RES[%dx%dx%d]\n",
		EGL_VERSTR,
		ri.vendorString, ri.rendererString, ri.versionString,
		ri.cColorBits, ri.cAlphaBits, ri.cDepthBits, ri.cStencilBits,
		ri.config.vidWidth, ri.config.vidHeight, ri.config.vidBitDepth));
}


/*
==================
R_VersionMsg_f
==================
*/
static void R_VersionMsg_f (void)
{
	Cbuf_AddText (Q_VarArgs ("say [EGL v%s (%s-%s) by Echon] [http://egl.quakedev.com/]\n",
		EGL_VERSTR, BUILDSTRING, CPUSTRING));
}

/*
=============================================================================

	INIT / SHUTDOWN

=============================================================================
*/

/*
==================
R_MediaInit
==================
*/
void R_MediaInit (void)
{
	// Chars image/materials
	R_CheckFont ();

	// World Caustic materials
	ri.media.worldLavaCaustics = R_RegisterTexture ("egl/lavacaustics", -1);
	ri.media.worldSlimeCaustics = R_RegisterTexture ("egl/slimecaustics", -1);
	ri.media.worldWaterCaustics = R_RegisterTexture ("egl/watercaustics", -1);
}


/*
===============
ExtensionFound
===============
*/
static qBool ExtensionFound (const byte *extensionList, const char *extension)
{
	const byte	*start;
	byte		*where, *terminator;

	// Extension names should not have spaces
	where = (byte *) strchr (extension, ' ');
	if (where || *extension == '\0')
		return qFalse;

	start = extensionList;
	for ( ; ; ) {
		where = (byte *) strstr ((const char *)start, extension);
		if (!where)
			break;
		terminator = where + strlen (extension);
		if (where == start || (*(where - 1) == ' ')) {
			if (*terminator == ' ' || *terminator == '\0') {
				return qTrue;
			}
		}
		start = terminator;
	}
	return qFalse;
}


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
#define SAFE_MODE	3
static qBool R_SetMode (void)
{
	int		width, height;
	qBool	fullScreen;

	Com_Printf (0, "Setting video mode\n");

	// Find the mode info
	fullScreen = vid_fullscreen->intVal ? qTrue : qFalse;
	if (vid_width->intVal > 0 && vid_height->intVal > 0) {
		width = vid_width->intVal;
		height = vid_height->intVal;
	}
	else if (!R_GetInfoForMode (gl_mode->intVal, &width, &height)) {
		Com_Printf (PRNT_ERROR, "...bad mode '%i', forcing safe mode\n", gl_mode->intVal);
		Cvar_VariableSetValue (gl_mode, (float)SAFE_MODE, qTrue);
		if (!R_GetInfoForMode (SAFE_MODE, &width, &height))
			return qFalse;	// This should *never* happen if SAFE_MODE is a sane value
	}

	// Attempt the desired mode
	if (GLimp_AttemptMode (fullScreen, width, height)) {
		Cvar_VariableSetValue (vid_fullscreen, (float)ri.config.vidFullScreen, qTrue);
		return qTrue;
	}

	// Bad mode, fall out of fullscreen if it was attempted
	if (fullScreen) {
		Com_Printf (PRNT_ERROR, "...failed to set fullscreen, attempting windowed\n");

		if (GLimp_AttemptMode (qFalse, width, height)) {
			Cvar_VariableSetValue (vid_fullscreen, (float)ri.config.vidFullScreen, qTrue);
			return qTrue;
		}
	}

	// Don't attempt the last valid safe mode if the user is already using it
	if (ri.lastValidMode != -1 && ri.lastValidMode != gl_mode->intVal) {
		Com_Printf (PRNT_ERROR, "...failed to set mode, attempted the last valid mode\n");
		Cvar_VariableSetValue (gl_mode, (float)ri.lastValidMode, qTrue);

		if (GLimp_AttemptMode (qFalse, width, height)) {
			Cvar_VariableSetValue (vid_fullscreen, (float)ri.config.vidFullScreen, qTrue);
			return qTrue;
		}
	}

	// Don't attempt safe mode if the user is already using it
	if (gl_mode->intVal == SAFE_MODE) {
		Com_Printf (PRNT_ERROR, "...already using the safe mode, exiting\n");
		return qFalse;
	}

	// Bad mode period, fall back to safe mode
	Com_Printf (PRNT_ERROR, "...failed to set mode, attempting safe mode '%d'\n", SAFE_MODE);
	Cvar_VariableSetValue (gl_mode, (float)SAFE_MODE, qTrue);

	// Try setting it back to something safe
	R_GetInfoForMode (gl_mode->intVal, &width, &height);
	if (GLimp_AttemptMode (fullScreen, width, height)) {
		Cvar_VariableSetValue (vid_fullscreen, (float)ri.config.vidFullScreen, qTrue);
		return qTrue;
	}

	Com_Printf (PRNT_ERROR, "...could not revert to safe mode\n");
	return qFalse;
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
	if (r_ext_multitexture->intVal) {
		// GL_ARB_multitexture
		if (ExtensionFound (ri.extensionString, "GL_ARB_multitexture")) {
			qglActiveTextureARB = QGL_GetProcAddress ("glActiveTextureARB");
			if (qglActiveTextureARB)	qglClientActiveTextureARB = QGL_GetProcAddress ("glClientActiveTextureARB");

			if (!qglClientActiveTextureARB) {
				Com_Printf (PRNT_ERROR, "...GL_ARB_multitexture not properly supported!\n");
				qglActiveTextureARB			= NULL;
				qglClientActiveTextureARB	= NULL;
			}
			else {
				Com_Printf (0, "...enabling GL_ARB_multitexture\n");
				ri.config.extArbMultitexture = qTrue;
			}
		}
		else
			Com_Printf (0, "...GL_ARB_multitexture not found\n");

		// GL_SGIS_multitexture
		if (!ri.config.extArbMultitexture) {
			Com_Printf (0, "...attempting GL_SGIS_multitexture\n");

			if (ExtensionFound (ri.extensionString, "GL_SGIS_multitexture")) {
				qglSelectTextureSGIS = QGL_GetProcAddress ("glSelectTextureSGIS");

				if (!qglSelectTextureSGIS) {
					Com_Printf (PRNT_ERROR, "...GL_SGIS_multitexture not properly supported!\n");
					qglSelectTextureSGIS	= NULL;
				}
				else {
					Com_Printf (0, "...enabling GL_SGIS_multitexture\n");
					ri.config.extSGISMultiTexture = qTrue;
				}
			}
			else
				Com_Printf (0, "...GL_SGIS_multitexture not found\n");
		}
	}
	else {
		qglActiveTextureARB			= NULL;
		qglClientActiveTextureARB	= NULL;
		qglSelectTextureSGIS		= NULL;

		Com_Printf (0, "...ignoring GL_ARB/SGIS_multitexture\n");
		Com_Printf (PRNT_WARNING, "WARNING: Disabling multitexture is not recommended!\n");
	}

	// Keep texture unit counts in check
	if (ri.config.extSGISMultiTexture || ri.config.extArbMultitexture) {
		qglGetIntegerv (GL_MAX_TEXTURE_UNITS, &ri.config.maxTexUnits);

		if (ri.config.maxTexUnits < 2) {
			Com_Printf (0, "...not using GL_ARB/SGIS_multitexture, < 2 texture units\n");

			ri.config.maxTexUnits = 1;
			qglActiveTextureARB				= NULL;
			qglClientActiveTextureARB		= NULL;
			qglSelectTextureSGIS			= NULL;
			ri.config.extArbMultitexture	= qFalse;
			ri.config.extSGISMultiTexture	= qFalse;
		}
		else {
			if (ri.config.extSGISMultiTexture && ri.config.maxTexUnits > 2) {
				// GL_SGIS_multitexture doesn't support more than 2 units does it?
				ri.config.maxTexUnits = 2;
				Com_Printf (0, "...* GL_SGIS_multitexture clamped to 2 texture units\n");
			}
			else if (ri.config.maxTexUnits > MAX_TEXUNITS) {
				// Clamp at the maximum amount the engine supports
				ri.config.maxTexUnits = MAX_TEXUNITS;
				Com_Printf (0, "...* clamped to engine maximum of %i texture units\n", ri.config.maxTexUnits);
			}
			else
				Com_Printf (0, "...* using video card maximum of %i texture units\n", ri.config.maxTexUnits);
		}
	}
	else {
		ri.config.maxTexUnits = 1;
	}

	/*
	** GL_ARB_texture_compression
	** GL_EXT_texture_compression_s3tc
	** GL_S3_s3tc
	*/
	if (r_ext_textureCompression->intVal) {
		while (r_ext_textureCompression->intVal) {
			switch (r_ext_textureCompression->intVal) {
			case 1:
				if (!ExtensionFound (ri.extensionString, "GL_ARB_texture_compression")) {
					Com_Printf (0, "...GL_ARB_texture_compression not found\n");
					Cvar_VariableSetValue (r_ext_textureCompression, 2, qTrue);
					break;
				}

				Com_Printf (0, "...enabling GL_ARB_texture_compression\n");
				ri.config.extTexCompression = qTrue;

				ri.rgbFormatCompressed = GL_COMPRESSED_RGB_ARB;
				ri.rgbaFormatCompressed = GL_COMPRESSED_RGBA_ARB;
				break;

			case 2:
			case 3:
			case 4:
				if (!ExtensionFound (ri.extensionString, "GL_EXT_texture_compression_s3tc")) {
					Com_Printf (0, "...GL_EXT_texture_compression_s3tc not found\n");
					Cvar_VariableSetValue (r_ext_textureCompression, 5, qTrue);
					break;
				}

				Com_Printf (0, "...enabling GL_EXT_texture_compression_s3tc\n");
				ri.config.extTexCompression = qTrue;

				ri.rgbFormatCompressed = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
				switch (r_ext_textureCompression->intVal) {
				case 2:
					ri.rgbaFormatCompressed = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
					Com_Printf (0, "...* using S3TC_DXT1\n");
					break;

				case 3:
					ri.rgbaFormatCompressed = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
					Com_Printf (0, "...* using S3TC_DXT3\n");
					break;

				case 4:
					ri.rgbaFormatCompressed = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
					Com_Printf (0, "...* using S3TC_DXT5\n");
					break;
				}
				break;

			case 5:
				if (!ExtensionFound (ri.extensionString, "GL_S3_s3tc")) {
					Com_Printf (0, "...GL_S3_s3tc not found\n");
					Cvar_VariableSetValue (r_ext_textureCompression, 0, qTrue);
					break;
				}

				Com_Printf (0, "...enabling GL_S3_s3tc\n");
				ri.config.extTexCompression = qTrue;

				ri.rgbFormatCompressed = GL_RGB_S3TC;
				ri.rgbaFormatCompressed = GL_RGBA_S3TC;
				break;

			default:
				Cvar_VariableSetValue (r_ext_textureCompression, 0, qTrue);
				break;
			}

			if (ri.config.extTexCompression || !r_ext_textureCompression->intVal)
				break;
		}
	}
	else {
		Com_Printf (0, "...ignoring GL_ARB_texture_compression\n");
		Com_Printf (0, "...ignoring GL_EXT_texture_compression_s3tc\n");
		Com_Printf (0, "...ignoring GL_S3_s3tc\n");
	}

	/*
	** GL_ARB_texture_cube_map
	*/
	if (r_ext_textureCubeMap->intVal) {
		if (ExtensionFound (ri.extensionString, "GL_ARB_texture_cube_map")) {
			qglGetIntegerv (GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &ri.config.maxCMTexSize);

			if (ri.config.maxCMTexSize <= 0) {
				Com_Printf (PRNT_ERROR, "GL_ARB_texture_cube_map not properly supported!\n");
				ri.config.maxCMTexSize = 0;
			}
			else {
				Q_NearestPow (&ri.config.maxCMTexSize, qTrue);

				Com_Printf (0, "...enabling GL_ARB_texture_cube_map\n");
				Com_Printf (0, "...* Max cubemap texture size: %i\n", ri.config.maxCMTexSize);
				ri.config.extTexCubeMap = qTrue;
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
	if (r_ext_textureEnvAdd->intVal) {
		if (ExtensionFound (ri.extensionString, "GL_ARB_texture_env_add")) {
			if (ri.config.extSGISMultiTexture || ri.config.extArbMultitexture) {
				Com_Printf (0, "...enabling GL_ARB_texture_env_add\n");
				ri.config.extTexEnvAdd = qTrue;
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
	if (r_ext_textureEnvCombine->intVal) {
		if (ExtensionFound (ri.extensionString, "GL_ARB_texture_env_combine") ||
			ExtensionFound (ri.extensionString, "GL_EXT_texture_env_combine")) {
			if (ri.config.extSGISMultiTexture || ri.config.extArbMultitexture) {
				Com_Printf (0, "...enabling GL_ARB/EXT_texture_env_combine\n");
				ri.config.extTexEnvCombine = qTrue;
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
	if (r_ext_textureEnvCombineNV4->intVal) {
		if (ExtensionFound (ri.extensionString, "NV_texture_env_combine4")) {
			if (ri.config.extTexEnvCombine) {
				Com_Printf (0, "...enabling GL_NV_texture_env_combine4\n");
				ri.config.extNVTexEnvCombine4 = qTrue;
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
	if (r_ext_textureEnvDot3->intVal) {
		if (ExtensionFound (ri.extensionString, "GL_ARB_texture_env_dot3")) {
			if (ri.config.extTexEnvCombine) {
				Com_Printf (0, "...enabling GL_ARB_texture_env_dot3\n");
				ri.config.extTexEnvDot3 = qTrue;
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
	if (r_ext_vertexProgram->intVal) {
		if (ExtensionFound (ri.extensionString, "GL_ARB_vertex_program")) {
			qglVertexAttribPointerARB = QGL_GetProcAddress("glVertexAttribPointerARB");
			if (qglVertexAttribPointerARB)		qglEnableVertexAttribArrayARB = QGL_GetProcAddress("glEnableVertexAttribArrayARB");
			if (qglEnableVertexAttribArrayARB)	qglDisableVertexAttribArrayARB = QGL_GetProcAddress("glDisableVertexAttribArrayARB");
			if (qglDisableVertexAttribArrayARB)	qglBindProgramARB = QGL_GetProcAddress("glBindProgramARB");
			if (qglBindProgramARB)				qglDeleteProgramsARB = QGL_GetProcAddress("glDeleteProgramsARB");
			if (qglDeleteProgramsARB)			qglGenProgramsARB = QGL_GetProcAddress("glGenProgramsARB");
			if (qglGenProgramsARB)				qglProgramStringARB = QGL_GetProcAddress("glProgramStringARB");
			if (qglProgramStringARB)			qglProgramEnvParameter4fARB = QGL_GetProcAddress("glProgramEnvParameter4fARB");
			if (qglProgramEnvParameter4fARB)	qglProgramEnvParameter4fvARB = QGL_GetProcAddress("glProgramEnvParameter4fvARB");
			if (qglProgramEnvParameter4fvARB)	qglProgramLocalParameter4fARB = QGL_GetProcAddress("glProgramLocalParameter4fARB");
			if (qglProgramLocalParameter4fARB)	qglProgramLocalParameter4fvARB = QGL_GetProcAddress("glProgramLocalParameter4fvARB");
			if (qglProgramLocalParameter4fvARB)	qglGetProgramivARB = QGL_GetProcAddress("glGetProgramivARB");

			if (!qglGetProgramivARB) {
				Com_Printf (PRNT_ERROR, "GL_ARB_vertex_program not properly supported!\n");
				qglVertexAttribPointerARB		= NULL;
				qglEnableVertexAttribArrayARB	= NULL;
				qglDisableVertexAttribArrayARB	= NULL;
				qglBindProgramARB				= NULL;
				qglDeleteProgramsARB			= NULL;
				qglGenProgramsARB				= NULL;
				qglProgramStringARB				= NULL;
				qglProgramEnvParameter4fARB		= NULL;
				qglProgramEnvParameter4fvARB	= NULL;
				qglProgramLocalParameter4fARB	= NULL;
				qglProgramLocalParameter4fvARB	= NULL;
				qglGetProgramivARB				= NULL;
			}
			else {
				Com_Printf (0, "...enabling GL_ARB_vertex_program\n");
				ri.config.extVertexProgram = qTrue;
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
	if (r_ext_fragmentProgram->intVal) {
		if (ExtensionFound (ri.extensionString, "GL_ARB_fragment_program")) {
			qglGetIntegerv (GL_MAX_TEXTURE_COORDS_ARB, &ri.config.maxTexCoords);
			qglGetIntegerv (GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &ri.config.maxTexImageUnits);

			qglBindProgramARB = QGL_GetProcAddress("glBindProgramARB");
			if (qglBindProgramARB)				qglDeleteProgramsARB = QGL_GetProcAddress("glDeleteProgramsARB");
			if (qglDeleteProgramsARB)			qglGenProgramsARB = QGL_GetProcAddress("glGenProgramsARB");
			if (qglGenProgramsARB)				qglProgramStringARB = QGL_GetProcAddress("glProgramStringARB");
			if (qglProgramStringARB)			qglProgramEnvParameter4fARB = QGL_GetProcAddress("glProgramEnvParameter4fARB");
			if (qglProgramEnvParameter4fARB)	qglProgramEnvParameter4fvARB = QGL_GetProcAddress("glProgramEnvParameter4fvARB");
			if (qglProgramEnvParameter4fvARB)	qglProgramLocalParameter4fARB = QGL_GetProcAddress("glProgramLocalParameter4fARB");
			if (qglProgramLocalParameter4fARB)	qglProgramLocalParameter4fvARB = QGL_GetProcAddress("glProgramLocalParameter4fvARB");
			if (qglProgramLocalParameter4fvARB)	qglGetProgramivARB = QGL_GetProcAddress("glGetProgramivARB");

			if (!qglGetProgramivARB) {
				Com_Printf (PRNT_ERROR, "GL_ARB_fragment_program not properly supported!\n");
				qglBindProgramARB				= NULL;
				qglDeleteProgramsARB			= NULL;
				qglGenProgramsARB				= NULL;
				qglProgramStringARB				= NULL;
				qglProgramEnvParameter4fARB		= NULL;
				qglProgramEnvParameter4fvARB	= NULL;
				qglProgramLocalParameter4fARB	= NULL;
				qglProgramLocalParameter4fvARB	= NULL;
				qglGetProgramivARB				= NULL;
			}
			else {
				Com_Printf (0, "...enabling GL_ARB_fragment_program\n");
				Com_Printf (0, "...* Max texture coordinates: %i\n", ri.config.maxTexCoords);
				Com_Printf (0, "...* Max texture image units: %i\n", ri.config.maxTexImageUnits);
				ri.config.extFragmentProgram = qTrue;
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
	if (r_ext_vertexBufferObject->intVal) {
		if (ExtensionFound (ri.extensionString, "GL_ARB_vertex_buffer_object")) {
			qglBindBufferARB = QGL_GetProcAddress ("glBindBufferARB");
			if (qglBindBufferARB)		qglDeleteBuffersARB = QGL_GetProcAddress ("glDeleteBuffersARB");
			if (qglDeleteBuffersARB)	qglGenBuffersARB = QGL_GetProcAddress ("glGenBuffersARB");
			if (qglGenBuffersARB)		qglIsBufferARB = QGL_GetProcAddress ("glIsBufferARB");
			if (qglIsBufferARB)			qglMapBufferARB = QGL_GetProcAddress ("glMapBufferARB");
			if (qglMapBufferARB)		qglUnmapBufferARB = QGL_GetProcAddress ("glUnmapBufferARB");
			if (qglUnmapBufferARB)		qglBufferDataARB = QGL_GetProcAddress ("glBufferDataARB");
			if (qglBufferDataARB)		qglBufferSubDataARB = QGL_GetProcAddress ("glBufferSubDataARB");

			if (!qglBufferSubDataARB) {
				Com_Printf (PRNT_ERROR, "GL_ARB_vertex_buffer_object not properly supported!\n");
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
				ri.config.extVertexBufferObject = qTrue;
			}
		}
		else
			Com_Printf (0, "...GL_ARB_vertex_buffer_object not found\n");
	}
	else
		Com_Printf (0, "...ignoring GL_ARB_vertex_buffer_object\n");

	/*
	** GL_EXT_bgra
	*/
	if (r_ext_BGRA->intVal) {
		if (ExtensionFound (ri.extensionString, "GL_EXT_bgra")) {
			Com_Printf (0, "...enabling GL_EXT_bgra\n");
			ri.config.extBGRA = qTrue;
		}
		else
			Com_Printf (0, "...GL_EXT_bgra not found\n");
	}
	else
		Com_Printf (0, "...ignoring GL_EXT_bgra\n");

	/*
	** GL_EXT_compiled_vertex_array
	** GL_SGI_compiled_vertex_array
	*/
	if (r_ext_compiledVertexArray->intVal) {
		if (ExtensionFound (ri.extensionString, "GL_EXT_compiled_vertex_array")
		|| ExtensionFound (ri.extensionString, "GL_SGI_compiled_vertex_array")) {
			if (r_ext_compiledVertexArray->intVal != 2
			&& (ri.renderClass == REND_CLASS_INTEL || ri.renderClass == REND_CLASS_S3 || ri.renderClass == REND_CLASS_SIS)) {
				Com_Printf (PRNT_WARNING, "...forcibly ignoring GL_EXT/SGI_compiled_vertex_array\n"
								"...* Your card is known for not supporting it properly\n"
								"...* If you would like it enabled, set r_ext_compiledVertexArray to 2\n");
			}
			else {
				qglLockArraysEXT = QGL_GetProcAddress ("glLockArraysEXT");
				if (qglLockArraysEXT)	qglUnlockArraysEXT = QGL_GetProcAddress ("glUnlockArraysEXT");

				if (!qglUnlockArraysEXT) {
					Com_Printf (PRNT_ERROR, "...GL_EXT/SGI_compiled_vertex_array not properly supported!\n");
					qglLockArraysEXT	= NULL;
					qglUnlockArraysEXT	= NULL;
				}
				else {
					Com_Printf (0, "...enabling GL_EXT/SGI_compiled_vertex_array\n");
					ri.config.extCompiledVertArray = qTrue;
				}
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
	if (r_ext_drawRangeElements->intVal) {
		if (ExtensionFound (ri.extensionString, "GL_EXT_draw_range_elements")) {
			// These are not actual maximums, but rather recommendations for performance...
			qglGetIntegerv (GL_MAX_ELEMENTS_VERTICES_EXT, &ri.config.maxElementVerts);
			qglGetIntegerv (GL_MAX_ELEMENTS_INDICES_EXT, &ri.config.maxElementIndices);

			if (ri.config.maxElementVerts > 0 && ri.config.maxElementIndices > 0) {
				qglDrawRangeElementsEXT = QGL_GetProcAddress ("glDrawRangeElementsEXT");
				if (!qglDrawRangeElementsEXT)
					qglDrawRangeElementsEXT = QGL_GetProcAddress ("glDrawRangeElements");
			}

			if (!qglDrawRangeElementsEXT) {
				Com_Printf (PRNT_ERROR, "...GL_EXT_draw_range_elements not properly supported!\n");
				qglDrawRangeElementsEXT		= NULL;
				ri.config.maxElementIndices	= 0;
				ri.config.maxElementVerts	= 0;
			}
			else {
				Com_Printf (0, "...enabling GL_EXT_draw_range_elements\n");
				Com_Printf (0, "...* Max element vertices: %i\n", ri.config.maxElementVerts);
				Com_Printf (0, "...* Max element indices: %i\n", ri.config.maxElementIndices);
				ri.config.extDrawRangeElements = qTrue;
			}
		}
		else
			Com_Printf (0, "...GL_EXT_draw_range_elements not found\n");
	}
	else
		Com_Printf (0, "...ignoring GL_EXT_draw_range_elements\n");

	/*
	** GL_EXT_texture3D
	*/
	if (r_ext_texture3D->intVal) {
		if (ExtensionFound (ri.extensionString, "GL_EXT_texture3D")) {
			qglTexImage3D = QGL_GetProcAddress ("glTexImage3D");
			if (qglTexImage3D) qglTexSubImage3D = QGL_GetProcAddress ("glTexSubImage3D");
			if (qglTexSubImage3D) qglGetIntegerv (GL_MAX_3D_TEXTURE_SIZE, &ri.config.max3DTexSize);

			if (!ri.config.max3DTexSize) {
				Com_Printf (PRNT_ERROR, "...GL_EXT_texture3D not properly supported!\n");
				qglTexImage3D		= NULL;
				qglTexSubImage3D	= NULL;
			}
			else {
				Com_Printf (0, "...enabling GL_EXT_texture3D\n");
				ri.config.extTex3D = qTrue;
			}
		}
		else
			Com_Printf (0, "...GL_EXT_texture3D not found\n");
	}
	else
		Com_Printf (0, "...ignoring GL_EXT_texture3D\n");

	/*
	** GL_EXT_texture_edge_clamp
	*/
	if (r_ext_textureEdgeClamp->intVal) {
		if (ExtensionFound (ri.extensionString, "GL_EXT_texture_edge_clamp")) {
			Com_Printf (0, "...enabling GL_EXT_texture_edge_clamp\n");
			ri.config.extTexEdgeClamp = qTrue;
		}
		else
			Com_Printf (0, "...GL_EXT_texture_edge_clamp not found\n");
	}
	else
		Com_Printf (0, "...ignoring GL_EXT_texture_edge_clamp\n");

	/*
	** GL_EXT_texture_filter_anisotropic
	*/
	if (r_ext_textureFilterAnisotropic->intVal) {
		if (ExtensionFound (ri.extensionString, "GL_EXT_texture_filter_anisotropic")) {
			qglGetIntegerv (GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &ri.config.maxAniso);
			if (ri.config.maxAniso <= 0) {
				Com_Printf (PRNT_ERROR, "...GL_EXT_texture_filter_anisotropic not properly supported!\n");
				ri.config.maxAniso = 0;
			}
			else {
				Com_Printf (0, "...enabling GL_EXT_texture_filter_anisotropic\n");
				Com_Printf (0, "...* Max texture anisotropy: %i\n", ri.config.maxAniso);
				ri.config.extTexFilterAniso = qTrue;
			}
		}
		else
			Com_Printf (0, "...GL_EXT_texture_filter_anisotropic not found\n");
	}
	else {
		if (ExtensionFound (ri.extensionString, "GL_EXT_texture_filter_anisotropic"))
			qglGetIntegerv (GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &ri.config.maxAniso);

		Com_Printf (0, "...ignoring GL_EXT_texture_filter_anisotropic\n");
	}

	/*
	** GL_SGIS_generate_mipmap
	*/
	if (r_ext_generateMipmap->intVal) {
		if (ExtensionFound (ri.extensionString, "GL_SGIS_generate_mipmap")) {
			if (r_ext_generateMipmap->intVal != 2
			&& (ri.renderClass == REND_CLASS_ATI || ri.renderClass == REND_CLASS_ATI_RADEON)) {
				Com_Printf (PRNT_WARNING, "...forcibly ignoring GL_SGIS_generate_mipmap\n"
								"...* ATi is known for not supporting it properly\n"
								"...* If you would like it enabled, set r_ext_generateMipmap to 2\n");
			}
			else {
				if (r_colorMipLevels->intVal) {
					Com_Printf (PRNT_WARNING, "...ignoring GL_SGIS_generate_mipmap because of r_colorMipLevels\n");
				}
				else {
					Com_Printf (0, "...enabling GL_SGIS_generate_mipmap\n");
					ri.config.extSGISGenMipmap = qTrue;
				}
			}
		}
		else
			Com_Printf (0, "...GL_SGIS_generate_mipmap not found\n");
	}
	else
		Com_Printf (0, "...ignoring GL_SGIS_generate_mipmap\n");

	/*
	** GL_EXT_stencil_two_side
	*/
	if (r_ext_stencilTwoSide->intVal) {
		if (ExtensionFound (ri.extensionString, "GL_EXT_stencil_two_side")) {
			qglActiveStencilFaceEXT = QGL_GetProcAddress ("glActiveStencilFaceEXT");

			if (!qglActiveStencilFaceEXT) {
				Com_Printf (PRNT_ERROR, "...GL_EXT_stencil_two_side not properly supported!\n");
				qglActiveStencilFaceEXT		= NULL;
			}
			else {
				Com_Printf (0, "...enabling GL_EXT_stencil_two_side\n");
				ri.config.extStencilTwoSide = qTrue;
			}
		}
		else
			Com_Printf (0, "...GL_EXT_stencil_two_side not found\n");
	}
	else
		Com_Printf (0, "...ignoring GL_EXT_stencil_two_side\n");

	/*
	** GL_EXT_stencil_wrap
	*/
	if (r_ext_stencilWrap->intVal) {
		if (ExtensionFound (ri.extensionString, "GL_EXT_stencil_wrap")) {
			Com_Printf (0, "...enabling GL_EXT_stencil_wrap\n");
			ri.config.extStencilWrap = qTrue;
		}
		else
			Com_Printf (0, "...GL_EXT_stencil_wrap not found\n");
	}
	else
		Com_Printf (0, "...ignoring GL_EXT_stencil_wrap\n");

#ifdef _WIN32
	/*
	** WGL_3DFX_gamma_control
	*/
	if (ExtensionFound (ri.extensionString, "WGL_3DFX_gamma_control")) {
		qwglGetDeviceGammaRamp3DFX = QGL_GetProcAddress ("wglGetDeviceGammaRamp3DFX");
		if (qwglGetDeviceGammaRamp3DFX) qwglSetDeviceGammaRamp3DFX = QGL_GetProcAddress ("wglSetDeviceGammaRamp3DFX");

		if (!qwglSetDeviceGammaRamp3DFX) {
			Com_Printf (PRNT_ERROR, "...WGL_3DFX_gamma_control not properly supported!\n");
			qwglGetDeviceGammaRamp3DFX		= NULL;
			qwglSetDeviceGammaRamp3DFX		= NULL;
		}
	}

	/*
	** WGL_EXT_swap_control
	*/
	if (r_ext_swapInterval->intVal) {
		if (ExtensionFound (ri.extensionString, "WGL_EXT_swap_control")) {
			if (!ri.config.extWinSwapInterval) {
				qwglSwapIntervalEXT = QGL_GetProcAddress ("wglSwapIntervalEXT");

				if (!qwglSwapIntervalEXT) {
					Com_Printf (PRNT_ERROR, "...WGL_EXT_swap_control not properly supported!\n");
					qwglSwapIntervalEXT		= NULL;
				}
				else {
					Com_Printf (0, "...enabling WGL_EXT_swap_control\n");
					ri.config.extWinSwapInterval = qTrue;
				}
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

	e_test_0			= Cvar_Register ("e_test_0",			"0",			0);
	e_test_1			= Cvar_Register ("e_test_1",			"0",			0);

	gl_bitdepth			= Cvar_Register ("gl_bitdepth",			"0",			CVAR_LATCH_VIDEO);
	gl_clear			= Cvar_Register ("gl_clear",			"0",			0);
	gl_cull				= Cvar_Register ("gl_cull",				"1",			0);
	gl_drawbuffer		= Cvar_Register ("gl_drawbuffer",		"GL_BACK",		0);
	gl_dynamic			= Cvar_Register ("gl_dynamic",			"1",			0);
	gl_errorcheck		= Cvar_Register ("gl_errorcheck",		"1",			CVAR_ARCHIVE);

	r_allowExtensions				= Cvar_Register ("r_allowExtensions",				"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_ext_BGRA						= Cvar_Register ("r_ext_BGRA",						"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_ext_compiledVertexArray		= Cvar_Register ("r_ext_compiledVertexArray",		"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_ext_drawRangeElements			= Cvar_Register ("r_ext_drawRangeElements",			"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_ext_fragmentProgram			= Cvar_Register ("r_ext_fragmentProgram",			"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_ext_generateMipmap			= Cvar_Register ("r_ext_generateMipmap",			"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_ext_maxAnisotropy				= Cvar_Register ("r_ext_maxAnisotropy",				"2",		CVAR_ARCHIVE);
	r_ext_multitexture				= Cvar_Register ("r_ext_multitexture",				"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_ext_stencilTwoSide			= Cvar_Register ("r_ext_stencilTwoSide",			"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_ext_stencilWrap				= Cvar_Register ("r_ext_stencilWrap",				"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_ext_swapInterval				= Cvar_Register ("r_ext_swapInterval",				"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_ext_texture3D					= Cvar_Register ("r_ext_texture3D",					"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_ext_textureCompression		= Cvar_Register ("r_ext_textureCompression",		"0",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_ext_textureCubeMap			= Cvar_Register ("r_ext_textureCubeMap",			"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_ext_textureEdgeClamp			= Cvar_Register ("r_ext_textureEdgeClamp",			"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_ext_textureEnvAdd				= Cvar_Register ("r_ext_textureEnvAdd",				"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_ext_textureEnvCombine			= Cvar_Register ("r_ext_textureEnvCombine",			"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_ext_textureEnvCombineNV4		= Cvar_Register ("r_ext_textureEnvCombineNV4",		"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_ext_textureEnvDot3			= Cvar_Register ("r_ext_textureEnvDot3",			"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_ext_textureFilterAnisotropic	= Cvar_Register ("r_ext_textureFilterAnisotropic",	"0",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_ext_vertexBufferObject		= Cvar_Register ("r_ext_vertexBufferObject",		"0",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_ext_vertexProgram				= Cvar_Register ("r_ext_vertexProgram",				"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);

	gl_finish			= Cvar_Register ("gl_finish",			"0",			CVAR_ARCHIVE);
	gl_flashblend		= Cvar_Register ("gl_flashblend",		"0",			CVAR_ARCHIVE);
	gl_lightmap			= Cvar_Register ("gl_lightmap",			"0",			CVAR_CHEAT);
	gl_lockpvs			= Cvar_Register ("gl_lockpvs",			"0",			CVAR_CHEAT);
	gl_log				= Cvar_Register ("gl_log",				"0",			0);
	gl_maxTexSize		= Cvar_Register ("gl_maxTexSize",		"0",			CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_mode				= Cvar_Register ("gl_mode",				"3",			CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_modulate			= Cvar_Register ("gl_modulate",			"1",			CVAR_ARCHIVE);

	gl_shadows			= Cvar_Register ("gl_shadows",			"0",			CVAR_ARCHIVE);
	gl_shownormals		= Cvar_Register ("gl_shownormals",		"0",			CVAR_CHEAT);
	gl_showtris			= Cvar_Register ("gl_showtris",			"0",			CVAR_CHEAT);

	qgl_debug			= Cvar_Register ("qgl_debug",			"0",			0);

	r_caustics			= Cvar_Register ("r_caustics",			"1",			CVAR_ARCHIVE);
	r_colorMipLevels	= Cvar_Register ("r_colorMipLevels",	"0",			CVAR_CHEAT|CVAR_LATCH_VIDEO);
	r_debugBatching		= Cvar_Register ("r_debugBatching",		"0",			0);
	r_debugCulling		= Cvar_Register ("r_debugCulling",		"0",			CVAR_CHEAT);
	r_debugLighting		= Cvar_Register ("r_debugLighting",		"0",			CVAR_CHEAT);
	r_debugSorting		= Cvar_Register ("r_debugSorting",		"0",			CVAR_CHEAT);
	r_defaultFont		= Cvar_Register ("r_defaultFont",		"default",		CVAR_ARCHIVE);
	r_detailTextures	= Cvar_Register ("r_detailTextures",	"1",			CVAR_ARCHIVE);
	r_displayFreq		= Cvar_Register ("r_displayfreq",		"0",			CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_drawDecals		= Cvar_Register ("r_drawDecals",		"1",			CVAR_CHEAT);
	r_drawEntities		= Cvar_Register ("r_drawEntities",		"1",			CVAR_CHEAT);
	r_drawPolys			= Cvar_Register ("r_drawPolys",			"1",			CVAR_CHEAT);
	r_drawworld			= Cvar_Register ("r_drawworld",			"1",			CVAR_CHEAT);
	r_facePlaneCull		= Cvar_Register ("r_facePlaneCull",		"1",			0);
	r_flares			= Cvar_Register ("r_flares",			"1",			CVAR_ARCHIVE);
	r_flareFade			= Cvar_Register ("r_flareFade",			"7",			CVAR_ARCHIVE);
	r_flareSize			= Cvar_Register ("r_flareSize",			"40",			CVAR_ARCHIVE);
	r_fontScale			= Cvar_Register ("r_fontScale",			"1",			CVAR_ARCHIVE);
	r_fullbright		= Cvar_Register ("r_fullbright",		"0",			CVAR_CHEAT);
	r_hwGamma			= Cvar_Register ("r_hwGamma",			"0",			CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_lerpmodels		= Cvar_Register ("r_lerpmodels",		"1",			0);
	r_lightlevel		= Cvar_Register ("r_lightlevel",		"0",			0);
	r_lmMaxBlockSize	= Cvar_Register ("r_lmMaxBlockSize",	"4096",			CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_lmModulate		= Cvar_Register ("r_lmModulate",		"2",			CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_lmPacking			= Cvar_Register ("r_lmPacking",			"1",			CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_noCull			= Cvar_Register ("r_noCull",			"0",			0);
	r_noRefresh			= Cvar_Register ("r_noRefresh",			"0",			0);
	r_noVis				= Cvar_Register ("r_noVis",				"0",			0);
	r_offsetFactor		= Cvar_Register ("r_offsetFactor",		"-1",			CVAR_CHEAT);
	r_offsetUnits		= Cvar_Register ("r_offsetUnits",		"-2",			CVAR_CHEAT);
	r_patchDivLevel		= Cvar_Register ("r_patchDivLevel",		"4",			CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_roundImagesDown	= Cvar_Register ("r_roundImagesDown",	"0",			CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_skipBackend		= Cvar_Register ("r_skipBackend",		"0",			CVAR_CHEAT);
	r_speeds			= Cvar_Register ("r_speeds",			"0",			0);
	r_sphereCull		= Cvar_Register ("r_sphereCull",		"1",			0);
	r_swapInterval		= Cvar_Register ("r_swapInterval",		"0",			CVAR_ARCHIVE);
	r_textureBits		= Cvar_Register ("r_textureBits",		"default",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_times				= Cvar_Register ("r_times",				"0",			0);
	r_vertexLighting	= Cvar_Register ("r_vertexLighting",	"0",			CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_zFarAbs			= Cvar_Register ("r_zFarAbs",			"0",			CVAR_CHEAT);
	r_zFarMin			= Cvar_Register ("r_zFarMin",			"256",			CVAR_CHEAT);
	r_zNear				= Cvar_Register ("r_zNear",				"4",			CVAR_CHEAT);

	r_alphabits			= Cvar_Register ("r_alphabits",			"0",			CVAR_LATCH_VIDEO);
	r_colorbits			= Cvar_Register ("r_colorbits",			"0",			CVAR_LATCH_VIDEO);
	r_depthbits			= Cvar_Register ("r_depthbits",			"0",			CVAR_LATCH_VIDEO);
	r_stencilbits		= Cvar_Register ("r_stencilbits",		"8",			CVAR_LATCH_VIDEO);
	cl_stereo			= Cvar_Register ("cl_stereo",			"0",			CVAR_LATCH_VIDEO);
	gl_allow_software	= Cvar_Register ("gl_allow_software",	"0",			CVAR_LATCH_VIDEO);
	gl_stencilbuffer	= Cvar_Register ("gl_stencilbuffer",	"1",			CVAR_ARCHIVE|CVAR_LATCH_VIDEO);

	vid_gamma			= Cvar_Register ("vid_gamma",			"1.0",						CVAR_ARCHIVE);
	vid_gammapics		= Cvar_Register ("vid_gammapics",		"0",						CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	vid_width			= Cvar_Register ("vid_width",			"0",						CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	vid_height			= Cvar_Register ("vid_height",			"0",						CVAR_ARCHIVE|CVAR_LATCH_VIDEO);

	intensity			= Cvar_Register ("intensity",			"1",						CVAR_ARCHIVE);

	gl_nobind			= Cvar_Register ("gl_nobind",			"0",						CVAR_CHEAT);
	gl_picmip			= Cvar_Register ("gl_picmip",			"0",						CVAR_LATCH_VIDEO);
	gl_screenshot		= Cvar_Register ("gl_screenshot",		"tga",						CVAR_ARCHIVE);

	gl_texturemode		= Cvar_Register ("gl_texturemode",		"GL_LINEAR_MIPMAP_NEAREST",	CVAR_ARCHIVE);

	// Force these to update next endframe
	r_swapInterval->modified = qTrue;
	gl_drawbuffer->modified = qTrue;
	gl_texturemode->modified = qTrue;
	r_ext_maxAnisotropy->modified = qTrue;
	r_defaultFont->modified = qTrue;
	vid_gamma->modified = qTrue;

	// Add the various commands
	cmd_gfxInfo			= Cmd_AddCommand ("gfxinfo",		R_GfxInfo_f,			"Prints out renderer information");
	cmd_rendererClass	= Cmd_AddCommand ("rendererclass",	R_RendererClass_f,		"Prints out the renderer class");
	cmd_eglRenderer		= Cmd_AddCommand ("egl_renderer",	R_RendererMsg_f,		"Spams to the server your renderer information");
	cmd_eglVersion		= Cmd_AddCommand ("egl_version",	R_VersionMsg_f,			"Spams to the server your client version");
}


/*
===============
R_Init
===============
*/
rInit_t R_Init (void)
{
	char	*rendererBuffer;
	char	*vendorBuffer;
	uint32	initTime;

	initTime = Sys_UMilliseconds ();
	Com_Printf (0, "\n-------- Refresh Initialization --------\n");

	ri.frameCount = 1;
	ri.reg.registerFrame = 1;

	// Register renderer cvars
	R_Register ();

	// Set extension/max defaults
	ri.config.extArbMultitexture = qFalse;
	ri.config.extBGRA = qFalse;
	ri.config.extCompiledVertArray = qFalse;
	ri.config.extDrawRangeElements = qFalse;
	ri.config.extFragmentProgram = qFalse;
	ri.config.extSGISGenMipmap = qFalse;
	ri.config.extSGISMultiTexture = qFalse;
	ri.config.extStencilTwoSide = qFalse;
	ri.config.extStencilWrap = qFalse;
	ri.config.extTex3D = qFalse;
	ri.config.extTexCompression = qFalse;
	ri.config.extTexCubeMap = qFalse;
	ri.config.extTexEdgeClamp = qFalse;
	ri.config.extTexEnvAdd = qFalse;
	ri.config.extTexEnvCombine = qFalse;
	ri.config.extNVTexEnvCombine4 = qFalse;
	ri.config.extTexEnvDot3 = qFalse;
	ri.config.extTexFilterAniso = qFalse;
	ri.config.extVertexBufferObject = qFalse;
	ri.config.extVertexProgram = qFalse;
	ri.config.extWinSwapInterval = qFalse;

	ri.config.max3DTexSize = 0;
	ri.config.maxAniso = 0;
	ri.config.maxCMTexSize = 0;
	ri.config.maxElementVerts = 0;
	ri.config.maxElementIndices = 0;
	ri.config.maxTexCoords = 0;
	ri.config.maxTexImageUnits = 0;
	ri.config.maxTexSize = 256;
	ri.config.maxTexUnits = 1;

	// Reset static refresh info
	ri.lastValidMode = -1;
	ri.useStencil = qFalse;
	ri.cColorBits = 0;
	ri.cAlphaBits = 0;
	ri.cDepthBits = 0;
	ri.cStencilBits = 0;

	// Create memory pools
	ri.decalSysPool = Mem_CreatePool ("Refresh: Decal system");
	ri.fontSysPool = Mem_CreatePool ("Refresh: Font system");
	ri.genericPool = Mem_CreatePool ("Refresh: Generic");
	ri.imageSysPool = Mem_CreatePool ("Refresh: Image system");
	ri.lightSysPool = Mem_CreatePool ("Refresh: Light system");
	ri.modelSysPool = Mem_CreatePool ("Refresh: Model system");
	ri.programSysPool = Mem_CreatePool ("Refresh: Program system");
	ri.matSysPool = Mem_CreatePool ("Refresh: Material system");

	// Initialize our QGL dynamic bindings
	if (!QGL_Init (GL_DRIVERNAME)) {
		Com_Printf (PRNT_ERROR, "...could not load \"%s\"\n", GL_DRIVERNAME);
		QGL_Shutdown ();
		return R_INIT_QGL_FAIL;
	}

	// Initialize OS-specific parts of OpenGL
	if (!GLimp_Init ()) {
		Com_Printf (PRNT_ERROR, "...unable to init gl implementation\n");
		QGL_Shutdown ();
		return R_INIT_OS_FAIL;
	}

	// Create the window and set up the context
	if (!R_SetMode ()) {
		Com_Printf (PRNT_ERROR, "...could not set video mode\n");
		QGL_Shutdown ();
		return R_INIT_MODE_FAIL;
	}

	// Vendor string
	ri.vendorString = qglGetString (GL_VENDOR);
	Com_Printf (0, "GL_VENDOR: %s\n", ri.vendorString);

	vendorBuffer = Mem_PoolStrDup ((char *)ri.vendorString, ri.genericPool, 0);
	Q_strlwr (vendorBuffer);

	// Renderer string
	ri.rendererString = qglGetString (GL_RENDERER);
	Com_Printf (0, "GL_RENDERER: %s\n", ri.rendererString);

	rendererBuffer = Mem_PoolStrDup ((char *)ri.rendererString, ri.genericPool, 0);
	Q_strlwr (rendererBuffer);

	// Version string
	ri.versionString = qglGetString (GL_VERSION);
	Com_Printf (0, "GL_VERSION: %s\n", ri.versionString);

	// Extension string
	ri.extensionString = qglGetString (GL_EXTENSIONS);

	// Decide on a renderer class
	if (strstr (rendererBuffer, "glint"))			ri.renderClass = REND_CLASS_3DLABS_GLINT_MX;
	else if (strstr (rendererBuffer, "permedia"))	ri.renderClass = REND_CLASS_3DLABS_PERMEDIA;
	else if (strstr (rendererBuffer, "glzicd"))		ri.renderClass = REND_CLASS_3DLABS_REALIZM;
	else if (strstr (vendorBuffer, "ati ")) {
		if (strstr (vendorBuffer, "radeon"))
			ri.renderClass = REND_CLASS_ATI_RADEON;
		else
			ri.renderClass = REND_CLASS_ATI;
	}
	else if (strstr (vendorBuffer, "intel"))		ri.renderClass = REND_CLASS_INTEL;
	else if (strstr (vendorBuffer, "nvidia")) {
		if (strstr (rendererBuffer, "geforce"))
			ri.renderClass = REND_CLASS_NVIDIA_GEFORCE;
		else
			ri.renderClass = REND_CLASS_NVIDIA;
	}
	else if (strstr	(rendererBuffer, "pmx"))		ri.renderClass = REND_CLASS_PMX;
	else if (strstr	(rendererBuffer, "pcx1"))		ri.renderClass = REND_CLASS_POWERVR_PCX1;
	else if (strstr	(rendererBuffer, "pcx2"))		ri.renderClass = REND_CLASS_POWERVR_PCX2;
	else if (strstr	(rendererBuffer, "verite"))		ri.renderClass = REND_CLASS_RENDITION;
	else if (strstr (vendorBuffer, "s3"))			ri.renderClass = REND_CLASS_S3;
	else if (strstr (rendererBuffer, "prosavage"))	ri.renderClass = REND_CLASS_S3;
	else if (strstr (rendererBuffer, "twister"))	ri.renderClass = REND_CLASS_S3;
	else if (strstr	(vendorBuffer, "sgi"))			ri.renderClass = REND_CLASS_SGI;
	else if (strstr	(vendorBuffer, "sis"))			ri.renderClass = REND_CLASS_SIS;
	else if (strstr (rendererBuffer, "voodoo"))		ri.renderClass = REND_CLASS_VOODOO;
	else {
		if (strstr (rendererBuffer, "gdi generic")) {
			ri.renderClass = REND_CLASS_MCD;

			// MCD has buffering issues
			Cvar_VariableSetValue (gl_finish, 1, qTrue);
		}
		else
			ri.renderClass = REND_CLASS_DEFAULT;
	}

	// Print the renderer class
	Com_Printf (0, "Renderer Class: %s\n", R_RendererClass ());

#ifdef GL_FORCEFINISH
	Cvar_VariableSetValue (gl_finish, 1, qTrue);
#endif

	// Check stencil buffer availability and usability
	Com_Printf (0, "...stencil buffer ");
	if (gl_stencilbuffer->intVal && ri.cStencilBits > 0) {
		if (ri.renderClass == REND_CLASS_VOODOO)
			Com_Printf (0, "ignored\n");
		else {
			Com_Printf (0, "available\n");
			ri.useStencil = qTrue;
		}
	}
	else {
		Com_Printf (0, "disabled\n");
	}

	// Grab opengl extensions
	if (r_allowExtensions->intVal)
		GL_InitExtensions ();
	else
		Com_Printf (0, "...ignoring OpenGL extensions\n");

	// Map overbrights
	ri.pow2MapOvrbr = r_lmModulate->intVal;
	if (ri.pow2MapOvrbr > 0)
		ri.pow2MapOvrbr = pow (2, ri.pow2MapOvrbr) / 255.0f;
	else
		ri.pow2MapOvrbr = 1.0f / 255.0f;

	// Retreive generic information
	if (gl_maxTexSize->intVal >= 256) {
		ri.config.maxTexSize = gl_maxTexSize->intVal;
		Q_NearestPow (&ri.config.maxTexSize, qTrue);

		Com_Printf (0, "Using forced maximum texture size of: %ix%i\n", ri.config.maxTexSize, ri.config.maxTexSize);
	}
	else {
		qglGetIntegerv (GL_MAX_TEXTURE_SIZE, &ri.config.maxTexSize);
		Q_NearestPow (&ri.config.maxTexSize, qTrue);
		if (ri.config.maxTexSize < 256) {
			Com_Printf (0, "Maximum texture size forced up to 256x256 from %i\n", ri.config.maxTexSize);
			ri.config.maxTexSize = 256;
		}
		else
			Com_Printf (0, "Using video card maximum texture size of %ix%i\n", ri.config.maxTexSize, ri.config.maxTexSize);
	}

	Com_Printf (0, "----------------------------------------\n");

	// Set the default state
	RB_SetDefaultState ();

	// Sub-system init
	R_ImageInit ();
	R_ProgramInit ();
	R_MaterialInit ();
	R_FontInit ();
	R_MediaInit ();
	R_ModelInit ();
	R_EntityInit ();
	R_WorldInit ();
	R_PolyInit ();
	R_DecalInit ();
	RB_Init ();
	RF_2DInit ();

	// Check for gl errors
	GL_CheckForError ("R_Init");

	Com_Printf (0, "----- Refresh Initialized %6ums -----\n", Sys_UMilliseconds()-initTime);

	Mem_Free (rendererBuffer);
	Mem_Free (vendorBuffer);

	return R_INIT_SUCCESS;
}


/*
===============
R_Shutdown
===============
*/
void R_Shutdown (qBool full)
{
	Com_Printf (0, "\n----------- Refresh Shutdown -----------\n");

	// Remove commands
	Cmd_RemoveCommand ("gfxinfo", cmd_gfxInfo);
	Cmd_RemoveCommand ("rendererclass", cmd_rendererClass);
	Cmd_RemoveCommand ("egl_renderer", cmd_eglRenderer);
	Cmd_RemoveCommand ("egl_version", cmd_eglVersion);

	// Shutdown subsystems
	R_FontShutdown ();
	R_MaterialShutdown ();
	R_ProgramShutdown ();
	R_ImageShutdown ();
	R_ModelShutdown ();
	R_WorldShutdown ();
	RB_Shutdown ();

	Com_Printf (0, "----------------------------------------\n");

	// Shutdown OS specific OpenGL stuff like contexts, etc
	GLimp_Shutdown (full);

	// Shutdown our QGL subsystem
	QGL_Shutdown ();

	Com_Printf (0, "----------------------------------------\n");
}
