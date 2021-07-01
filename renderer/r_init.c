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
// r_init.c
//

#include "r_local.h"

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

cVar_t	*r_allowExtensions;
cVar_t	*r_ext_BGRA;
cVar_t	*r_ext_compiledVertexArray;
cVar_t	*r_ext_drawRangeElements;
cVar_t	*r_ext_fragmentProgram;
cVar_t	*r_ext_generateMipmap;
cVar_t	*r_ext_maxAnisotropy;
cVar_t	*r_ext_multitexture;
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
cVar_t	*r_debugCulling;
cVar_t	*r_debugLighting;
cVar_t	*r_debugSorting;
cVar_t	*r_detailTextures;
cVar_t	*r_displayFreq;
cVar_t	*r_drawEntities;
cVar_t	*r_drawPolys;
cVar_t	*r_drawworld;
cVar_t	*r_facePlaneCull;
cVar_t	*r_flares;
cVar_t	*r_flareFade;
cVar_t	*r_flareSize;
cVar_t	*r_fontscale;
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
cVar_t	*r_patchDivLevel;
cVar_t	*r_roundImagesDown;
cVar_t	*r_skipBackend;
cVar_t	*r_speeds;
cVar_t	*r_sphereCull;
cVar_t	*r_swapInterval;
cVar_t	*r_textureBits;
cVar_t	*r_vertexLighting;

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

cVar_t	*gl_jpgquality;
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
GL_RendererClass_f
==================
*/
static char *R_RendererClass (void)
{
	switch (glStatic.renderClass) {
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
GL_RendererClass_f
==================
*/
static void GL_RendererClass_f (void)
{
	Com_Printf (0, "Renderer Class: %s\n", R_RendererClass ());
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
				glStatic.cColorBits, glStatic.cAlphaBits, glStatic.cDepthBits, glStatic.cStencilBits);

	Com_Printf (0, "Renderer Class: %s\n", R_RendererClass ());

	Com_Printf (0, "----------------------------------------\n");

	Com_Printf (0, "GL_VENDOR: %s\n",		glStatic.vendorString);
	Com_Printf (0, "GL_RENDERER: %s\n",		glStatic.rendererString);
	Com_Printf (0, "GL_VERSION: %s\n",		glStatic.versionString);
	Com_Printf (0, "GL_EXTENSIONS: %s\n",	glStatic.extensionString);

	Com_Printf (0, "----------------------------------------\n");

#ifdef WIN32
	Com_Printf (0, "CDS: %s\n", (glStatic.allowCDS) ? "Enabled" : "Disabled");
#endif // WIN32

	Com_Printf (0, "Extensions:\n");
	Com_Printf (0, "...ARB Multitexture: %s\n",				glConfig.extArbMultitexture ? "On" : "Off");

	Com_Printf (0, "...BGRA: %s\n",							glConfig.extBGRA ? "On" : "Off");
	Com_Printf (0, "...Compiled Vertex Array: %s\n",			glConfig.extCompiledVertArray ? "On" : "Off");

	Com_Printf (0, "...Draw Range Elements: %s\n",			glConfig.extDrawRangeElements ? "On" : "Off");
	if (glConfig.extDrawRangeElements) {
		Com_Printf (0, "...* Max element vertices: %i\n",	glConfig.maxElementVerts);
		Com_Printf (0, "...* Max element indices: %i\n",		glConfig.maxElementIndices);
	}

	Com_Printf (0, "...Fragment programs: %s\n",				glConfig.extFragmentProgram ? "On" : "Off");
	if (glConfig.extFragmentProgram) {
		Com_Printf (0, "...* Max texture coordinates: %i\n", glConfig.maxTexCoords);
		Com_Printf (0, "...* Max texture image units: %i\n", glConfig.maxTexImageUnits);
	}

	Com_Printf (0, "...nVidia Texture Env Combine4: %s\n",	glConfig.extNVTexEnvCombine4 ? "On" : "Off");
	Com_Printf (0, "...SGIS Mipmap Generation: %s\n",		glConfig.extSGISGenMipmap ? "On" : "Off");
	Com_Printf (0, "...SGIS Multitexture: %s\n",				glConfig.extSGISMultiTexture ? "On" : glConfig.extArbMultitexture ? "Deprecated for ARB Multitexture" : "Off");

	Com_Printf (0, "...Texture Cube Map: %s\n",				glConfig.extTexCubeMap ? "On" : "Off");
	if (glConfig.extTexCubeMap)
		Com_Printf (0, "...* Max cubemap texture size: %i\n",glConfig.maxCMTexSize);

	Com_Printf (0, "...Texture Compression: %s\n",			glConfig.extTexCompression ? "On" : "Off");
	Com_Printf (0, "...Texture 3D: %s\n",					glConfig.extTex3D ? "On" : "Off");
	Com_Printf (0, "...Texture Edge Clamp: %s\n",			glConfig.extTexEdgeClamp ? "On" : "Off");
	Com_Printf (0, "...Texture Env Add: %s\n",				glConfig.extTexEnvAdd ? "On" : "Off");
	Com_Printf (0, "...Texture Env Combine: %s\n",			glConfig.extTexEnvCombine ? "On" : "Off");
	Com_Printf (0, "...Texture Env DOT3: %s\n",				glConfig.extTexEnvDot3 ? "On" : "Off");

	Com_Printf (0, "...Texture Filter Anisotropic: %s\n",	glConfig.extTexFilterAniso ? "On" : "Off");
	if (glConfig.extTexFilterAniso)
		Com_Printf (0, "...* Max texture anisotropy: %i\n",	glConfig.maxAniso);

	Com_Printf (0, "...Vertex Buffer Objects: %s\n",			glConfig.extVertexBufferObject ? "On" : "Off");
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
				glStatic.vendorString, glStatic.rendererString, glStatic.versionString,
				glStatic.cColorBits, glStatic.cAlphaBits, glStatic.cDepthBits, glStatic.cStencilBits,
				glConfig.vidWidth, glConfig.vidHeight, glConfig.vidBitDepth));
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
=============================================================================

	INIT / SHUTDOWN

=============================================================================
*/

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

#ifdef WIN32
	// Check if ChangeDisplaySettings is allowed (only fullscreen uses CDS in win32)
	if (vid_fullscreen->modified && !glStatic.allowCDS) {
		Com_Printf (PRNT_WARNING, "CDS not allowed with this driver\n");
		Cvar_VariableSetValue (vid_fullscreen, 0, qTrue);
		vid_fullscreen->modified = qFalse;
	}
#endif // WIN32

	// Set defaults
	glConfig.vidWidth = 0;
	glConfig.vidHeight = 0;
	glConfig.vidBitDepth = 0;
	glConfig.vidFrequency = 0;
	glConfig.vidFullScreen = qFalse;

	// Find the mode info
	fullScreen = vid_fullscreen->integer ? qTrue : qFalse;
	if (vid_width->integer > 0 && vid_height->integer > 0) {
		width = vid_width->integer;
		height = vid_height->integer;
	}
	else if (!R_GetInfoForMode (gl_mode->integer, &width, &height)) {
		Com_Printf (PRNT_ERROR, "R_SetMode: bad mode '%i', forcing safe mode\n", gl_mode->integer);
		Cvar_VariableSetValue (gl_mode, (float)SAFE_MODE, qTrue);
		if (!R_GetInfoForMode (gl_mode->integer, &width, &height))
			return qFalse; // This should *never* happen if SAFE_MODE is a sane value
	}

	// Attempt the desired mode
	if (!GLimp_AttemptMode (fullScreen, width, height)) {
		// Bad mode, fall back to safe mode
		Com_Printf (PRNT_WARNING, "...invalid mode, attempting safe mode '%d'\n", SAFE_MODE);
		Cvar_VariableSetValue (gl_mode, (float)SAFE_MODE, qTrue);

		// Set defaults
		glConfig.vidWidth = 0;
		glConfig.vidHeight = 0;
		glConfig.vidBitDepth = 0;
		glConfig.vidFrequency = 0;
		glConfig.vidFullScreen = qFalse;
		Cvar_VariableSetValue (vid_width, 0, qTrue);
		Cvar_VariableSetValue (vid_height, 0, qTrue);

		// Try setting it back to something safe
		R_GetInfoForMode (gl_mode->integer, &width, &height);
		if (!GLimp_AttemptMode (fullScreen, width, height)) {
			Com_Printf (PRNT_ERROR, "...could not revert to safe mode\n");
			return qFalse;
		}
	}

	Cvar_VariableSetValue (vid_fullscreen, (float)glConfig.vidFullScreen, qTrue);
	vid_fullscreen->modified = qFalse;
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
	if (r_ext_multitexture->integer) {
		// GL_ARB_multitexture
		if (ExtensionFound (glStatic.extensionString, "GL_ARB_multitexture")) {
			qglMTexCoord2f = (void *) QGL_GetProcAddress ("glMultiTexCoord2fARB");
			if (qglMTexCoord2f)			qglActiveTextureARB = (void *) QGL_GetProcAddress ("glActiveTextureARB");
			if (qglActiveTextureARB)	qglClientActiveTextureARB = (void *) QGL_GetProcAddress ("glClientActiveTextureARB");

			if (!qglClientActiveTextureARB) {
				Com_Printf (PRNT_ERROR, "...GL_ARB_multitexture not properly supported!\n");
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

			if (ExtensionFound (glStatic.extensionString, "GL_SGIS_multitexture")) {
				qglMTexCoord2f = (void *) QGL_GetProcAddress ("glMTexCoord2fSGIS");
				if (qglMTexCoord2f)		qglSelectTextureSGIS = (void *) QGL_GetProcAddress ("glSelectTextureSGIS");

				if (!qglSelectTextureSGIS) {
					Com_Printf (PRNT_ERROR, "...GL_SGIS_multitexture not properly supported!\n");
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
		Com_Printf (PRNT_WARNING, "WARNING: Disabling multitexture is not recommended!\n");
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
	** GL_EXT_texture_compression_s3tc
	** GL_S3_s3tc
	*/
	if (r_ext_textureCompression->integer) {
		while (r_ext_textureCompression->integer) {
			switch (r_ext_textureCompression->integer) {
			case 1:
				if (!ExtensionFound (glStatic.extensionString, "GL_ARB_texture_compression")) {
					Com_Printf (0, "...GL_ARB_texture_compression not found\n");
					Cvar_VariableSetValue (r_ext_textureCompression, 2, qTrue);
					break;
				}

				Com_Printf (0, "...enabling GL_ARB_texture_compression\n");
				glConfig.extTexCompression = qTrue;

				glStatic.rgbFormatCompressed = GL_COMPRESSED_RGB_ARB;
				glStatic.rgbaFormatCompressed = GL_COMPRESSED_RGBA_ARB;
				break;

			case 2:
			case 3:
			case 4:
				if (!ExtensionFound (glStatic.extensionString, "GL_EXT_texture_compression_s3tc")) {
					Com_Printf (0, "...GL_EXT_texture_compression_s3tc not found\n");
					Cvar_VariableSetValue (r_ext_textureCompression, 5, qTrue);
					break;
				}

				Com_Printf (0, "...enabling GL_EXT_texture_compression_s3tc\n");
				glConfig.extTexCompression = qTrue;

				glStatic.rgbFormatCompressed = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
				switch (r_ext_textureCompression->integer) {
				case 2:
					glStatic.rgbaFormatCompressed = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
					Com_Printf (0, "...* using S3TC_DXT1\n");
					break;

				case 3:
					glStatic.rgbaFormatCompressed = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
					Com_Printf (0, "...* using S3TC_DXT3\n");
					break;

				case 4:
					glStatic.rgbaFormatCompressed = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
					Com_Printf (0, "...* using S3TC_DXT5\n");
					break;
				}
				break;

			case 5:
				if (!ExtensionFound (glStatic.extensionString, "GL_S3_s3tc")) {
					Com_Printf (0, "...GL_S3_s3tc not found\n");
					Cvar_VariableSetValue (r_ext_textureCompression, 0, qTrue);
					break;
				}

				Com_Printf (0, "...enabling GL_S3_s3tc\n");
				glConfig.extTexCompression = qTrue;

				glStatic.rgbFormatCompressed = GL_RGB_S3TC;
				glStatic.rgbaFormatCompressed = GL_RGBA_S3TC;
				break;

			default:
				Cvar_VariableSetValue (r_ext_textureCompression, 0, qTrue);
				break;
			}

			if (glConfig.extTexCompression || !r_ext_textureCompression->integer)
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
	if (r_ext_textureCubeMap->integer) {
		if (ExtensionFound (glStatic.extensionString, "GL_ARB_texture_cube_map")) {
			qglGetIntegerv (GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &glConfig.maxCMTexSize);

			if (glConfig.maxCMTexSize <= 0) {
				Com_Printf (PRNT_ERROR, "GL_ARB_texture_cube_map not properly supported!\n");
				glConfig.maxCMTexSize = 0;
			}
			else {
				Q_NearestPow (&glConfig.maxCMTexSize, qTrue);

				Com_Printf (0, "...enabling GL_ARB_texture_cube_map\n");
				Com_Printf (0, "...* Max cubemap texture size: %i\n", glConfig.maxCMTexSize);
				glConfig.extTexCubeMap = qTrue;
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
	if (r_ext_textureEnvAdd->integer) {
		if (ExtensionFound (glStatic.extensionString, "GL_ARB_texture_env_add")) {
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
	if (r_ext_textureEnvCombine->integer) {
		if (ExtensionFound (glStatic.extensionString, "GL_ARB_texture_env_combine") ||
			ExtensionFound (glStatic.extensionString, "GL_EXT_texture_env_combine")) {
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
	if (r_ext_textureEnvCombineNV4->integer) {
		if (ExtensionFound (glStatic.extensionString, "NV_texture_env_combine4")) {
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
	if (r_ext_textureEnvDot3->integer) {
		if (ExtensionFound (glStatic.extensionString, "GL_ARB_texture_env_dot3")) {
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
	if (r_ext_vertexProgram->integer) {
		if (ExtensionFound (glStatic.extensionString, "GL_ARB_vertex_program")) {

			qglBindProgramARB = (void *) QGL_GetProcAddress("glBindProgramARB");
			if (qglBindProgramARB)				qglDeleteProgramsARB = (void *) QGL_GetProcAddress("glDeleteProgramsARB");
			if (qglDeleteProgramsARB)			qglGenProgramsARB = (void *) QGL_GetProcAddress("glGenProgramsARB");
			if (qglGenProgramsARB)				qglProgramStringARB = (void *) QGL_GetProcAddress("glProgramStringARB");
			if (qglProgramStringARB)			qglProgramEnvParameter4fARB = (void *) QGL_GetProcAddress("glProgramEnvParameter4fARB");
			if (qglProgramEnvParameter4fARB)	qglProgramLocalParameter4fARB = (void *) QGL_GetProcAddress("glProgramLocalParameter4fARB");

			if (!qglProgramLocalParameter4fARB) {
				Com_Printf (PRNT_ERROR, "GL_ARB_vertex_program not properly supported!\n");
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
	if (r_ext_fragmentProgram->integer) {
		if (ExtensionFound (glStatic.extensionString, "GL_ARB_fragment_program")) {
			qglGetIntegerv (GL_MAX_TEXTURE_COORDS_ARB, &glConfig.maxTexCoords);
			qglGetIntegerv (GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &glConfig.maxTexImageUnits);

			qglBindProgramARB = (void *) QGL_GetProcAddress("glBindProgramARB");
			if (qglBindProgramARB)				qglDeleteProgramsARB = (void *) QGL_GetProcAddress("glDeleteProgramsARB");
			if (qglDeleteProgramsARB)			qglGenProgramsARB = (void *) QGL_GetProcAddress("glGenProgramsARB");
			if (qglGenProgramsARB)				qglProgramStringARB = (void *) QGL_GetProcAddress("glProgramStringARB");
			if (qglProgramStringARB)			qglProgramEnvParameter4fARB = (void *) QGL_GetProcAddress("glProgramEnvParameter4fARB");
			if (qglProgramEnvParameter4fARB)	qglProgramLocalParameter4fARB = (void *) QGL_GetProcAddress("glProgramLocalParameter4fARB");

			if (!qglProgramLocalParameter4fARB) {
				Com_Printf (PRNT_ERROR, "GL_ARB_fragment_program not properly supported!\n");
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
	if (r_ext_vertexBufferObject->integer) {
		if (ExtensionFound (glStatic.extensionString, "GL_ARB_vertex_buffer_object")) {
			qglBindBufferARB = (void *) QGL_GetProcAddress ("glBindBufferARB");
			if (qglBindBufferARB)		qglDeleteBuffersARB = (void *) QGL_GetProcAddress ("glDeleteBuffersARB");
			if (qglDeleteBuffersARB)	qglGenBuffersARB = (void *) QGL_GetProcAddress ("glGenBuffersARB");
			if (qglGenBuffersARB)		qglIsBufferARB = (void *) QGL_GetProcAddress ("glIsBufferARB");
			if (qglIsBufferARB)			qglMapBufferARB = (void *) QGL_GetProcAddress ("glMapBufferARB");
			if (qglMapBufferARB)		qglUnmapBufferARB = (void *) QGL_GetProcAddress ("glUnmapBufferARB");
			if (qglUnmapBufferARB)		qglBufferDataARB = (void *) QGL_GetProcAddress ("glBufferDataARB");
			if (qglBufferDataARB)		qglBufferSubDataARB = (void *) QGL_GetProcAddress ("glBufferSubDataARB");

			if (!qglBufferDataARB) {
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
				glConfig.extVertexBufferObject = qTrue;
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
	if (r_ext_BGRA->integer) {
		if (ExtensionFound (glStatic.extensionString, "GL_EXT_bgra")) {
			Com_Printf (0, "...enabling GL_EXT_bgra\n");
			glConfig.extBGRA = qTrue;
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
	if (r_ext_compiledVertexArray->integer) {
		if (ExtensionFound (glStatic.extensionString, "GL_EXT_compiled_vertex_array")
		|| ExtensionFound (glStatic.extensionString, "GL_SGI_compiled_vertex_array")) {
			if (r_ext_compiledVertexArray->integer != 2
			&& (glStatic.renderClass == REND_CLASS_INTEL || glStatic.renderClass == REND_CLASS_S3 || glStatic.renderClass == REND_CLASS_SIS)) {
				Com_Printf (0, "...forcibly ignoring GL_EXT/SGI_compiled_vertex_array\n"
								"...* Your card is known for not supporting it properly\n"
								"...* If you would like it enabled, set r_ext_compiledVertexArray to 2\n");
			}
			else {
				qglLockArraysEXT = (void *) QGL_GetProcAddress ("glLockArraysEXT");
				if (qglLockArraysEXT)	qglUnlockArraysEXT = (void *) QGL_GetProcAddress ("glUnlockArraysEXT");

				if (!qglUnlockArraysEXT) {
					Com_Printf (PRNT_ERROR, "...GL_EXT/SGI_compiled_vertex_array not properly supported!\n");
					qglLockArraysEXT	= NULL;
					qglUnlockArraysEXT	= NULL;
				}
				else {
					Com_Printf (0, "...enabling GL_EXT/SGI_compiled_vertex_array\n");
					glConfig.extCompiledVertArray = qTrue;
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
	if (r_ext_drawRangeElements->integer) {
		if (ExtensionFound (glStatic.extensionString, "GL_EXT_draw_range_elements")) {
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
				Com_Printf (PRNT_ERROR, "...GL_EXT_draw_range_elements not properly supported!\n");
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
	** GL_EXT_texture3D
	*/
	if (r_ext_texture3D->integer) {
		if (ExtensionFound (glStatic.extensionString, "GL_EXT_texture3D")) {
			qglTexImage3D = (void *) QGL_GetProcAddress ("glTexImage3D");
			if (qglTexImage3D) qglTexSubImage3D = (void *) QGL_GetProcAddress ("glTexSubImage3D");
			if (qglTexSubImage3D) qglGetIntegerv (GL_MAX_3D_TEXTURE_SIZE, &glConfig.max3DTexSize);

			if (!glConfig.max3DTexSize) {
				Com_Printf (PRNT_ERROR, "...GL_EXT_texture3D not properly supported!\n");
				qglTexImage3D		= NULL;
				qglTexSubImage3D	= NULL;
			}
			else {
				Com_Printf (0, "...enabling GL_EXT_texture3D\n");
				glConfig.extTex3D = qTrue;
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
	if (r_ext_textureEdgeClamp->integer) {
		if (ExtensionFound (glStatic.extensionString, "GL_EXT_texture_edge_clamp")) {
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
	if (r_ext_textureFilterAnisotropic->integer) {
		if (ExtensionFound (glStatic.extensionString, "GL_EXT_texture_filter_anisotropic")) {
			qglGetIntegerv (GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxAniso);
			if (glConfig.maxAniso <= 0) {
				Com_Printf (PRNT_ERROR, "...GL_EXT_texture_filter_anisotropic not properly supported!\n");
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
	else {
		if (ExtensionFound (glStatic.extensionString, "GL_EXT_texture_filter_anisotropic"))
			qglGetIntegerv (GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxAniso);

		Com_Printf (0, "...ignoring GL_EXT_texture_filter_anisotropic\n");
	}

	/*
	** GL_SGIS_generate_mipmap
	*/
	if (r_ext_generateMipmap->integer) {
		if (ExtensionFound (glStatic.extensionString, "GL_SGIS_generate_mipmap")) {
			if (r_ext_generateMipmap->integer != 2
			&& (glStatic.renderClass == REND_CLASS_ATI || glStatic.renderClass == REND_CLASS_ATI_RADEON)) {
				Com_Printf (PRNT_ERROR, "...forcibly ignoring GL_SGIS_generate_mipmap\n"
								"...* ATi is known for not supporting it properly\n"
								"...* If you would like it enabled, set r_ext_generateMipmap to 2\n");
			}
			else {
				if (r_colorMipLevels->integer) {
					Com_Printf (PRNT_WARNING, "...ignoring GL_SGIS_generate_mipmap because of r_colorMipLevels\n");
				}
				else {
					Com_Printf (0, "...enabling GL_SGIS_generate_mipmap\n");
					glConfig.extSGISGenMipmap = qTrue;
				}
			}
		}
		else
			Com_Printf (0, "...GL_SGIS_generate_mipmap not found\n");
	}
	else
		Com_Printf (0, "...ignoring GL_SGIS_generate_mipmap\n");

#ifdef WIN32
	/*
	** WGL_3DFX_gamma_control
	*/
	if (ExtensionFound (glStatic.extensionString, "WGL_3DFX_gamma_control")) {
		qwglGetDeviceGammaRamp3DFX = (BOOL (WINAPI *)(HDC, WORD *)) QGL_GetProcAddress ("wglGetDeviceGammaRamp3DFX");
		qwglSetDeviceGammaRamp3DFX = (BOOL (WINAPI *)(HDC, WORD *)) QGL_GetProcAddress ("wglSetDeviceGammaRamp3DFX");

		if (!qwglGetDeviceGammaRamp3DFX || !qwglSetDeviceGammaRamp3DFX) {
			Com_Printf (PRNT_ERROR, "...WGL_3DFX_gamma_control not properly supported!\n");
		}
	}

	/*
	** WGL_EXT_swap_control
	*/
	if (r_ext_swapInterval->integer) {
		if (ExtensionFound (glStatic.extensionString, "WGL_EXT_swap_control")) {
			if (!glConfig.extWinSwapInterval) {
				qwglSwapIntervalEXT = (BOOL (WINAPI *)(int)) QGL_GetProcAddress ("wglSwapIntervalEXT");

				if (qwglSwapIntervalEXT) {
					Com_Printf (0, "...enabling WGL_EXT_swap_control\n");
					glConfig.extWinSwapInterval = qTrue;
				}
				else
					Com_Printf (PRNT_ERROR, "...WGL_EXT_swap_control not properly supported!\n");
			}
		}
		else
			Com_Printf (0, "...WGL_EXT_swap_control not found\n");
	}
	else
		Com_Printf (0, "...ignoring WGL_EXT_swap_control\n");
#endif // WIN32
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

	con_font			= Cvar_Register ("con_font",			"conchars",		CVAR_ARCHIVE);

	e_test_0			= Cvar_Register ("e_test_0",			"0",			0);
	e_test_1			= Cvar_Register ("e_test_1",			"0",			0);

	gl_3dlabs_broken	= Cvar_Register ("gl_3dlabs_broken",	"1",			CVAR_ARCHIVE);
	gl_bitdepth			= Cvar_Register ("gl_bitdepth",			"0",			CVAR_LATCH_VIDEO);
	gl_clear			= Cvar_Register ("gl_clear",			"0",			0);
	gl_cull				= Cvar_Register ("gl_cull",				"1",			0);
	gl_drawbuffer		= Cvar_Register ("gl_drawbuffer",		"GL_BACK",		0);
	gl_driver			= Cvar_Register ("gl_driver",			GL_DRIVERNAME,	CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
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
	r_debugCulling		= Cvar_Register ("r_debugCulling",		"0",			CVAR_CHEAT);
	r_debugLighting		= Cvar_Register ("r_debugLighting",		"0",			CVAR_CHEAT);
	r_debugSorting		= Cvar_Register ("r_debugSorting",		"0",			CVAR_CHEAT);
	r_detailTextures	= Cvar_Register ("r_detailTextures",	"1",			CVAR_ARCHIVE);
	r_displayFreq		= Cvar_Register ("r_displayfreq",		"0",			CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_drawEntities		= Cvar_Register ("r_drawEntities",		"1",			CVAR_CHEAT);
	r_drawPolys			= Cvar_Register ("r_drawPolys",			"1",			CVAR_CHEAT);
	r_drawworld			= Cvar_Register ("r_drawworld",			"1",			CVAR_CHEAT);
	r_facePlaneCull		= Cvar_Register ("r_facePlaneCull",		"1",			0);
	r_flares			= Cvar_Register ("r_flares",			"1",			CVAR_ARCHIVE);
	r_flareFade			= Cvar_Register ("r_flareFade",			"7",			CVAR_ARCHIVE);
	r_flareSize			= Cvar_Register ("r_flareSize",			"40",			CVAR_ARCHIVE);
	r_fontscale			= Cvar_Register ("r_fontscale",			"1",			CVAR_ARCHIVE);
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
	r_patchDivLevel		= Cvar_Register ("r_patchDivLevel",		"4",			CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_roundImagesDown	= Cvar_Register ("r_roundImagesDown",	"0",			CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_skipBackend		= Cvar_Register ("r_skipBackend",		"0",			CVAR_CHEAT);
	r_speeds			= Cvar_Register ("r_speeds",			"0",			0);
	r_sphereCull		= Cvar_Register ("r_sphereCull",		"1",			0);
	r_swapInterval		= Cvar_Register ("r_swapInterval",		"0",			CVAR_ARCHIVE);
	r_textureBits		= Cvar_Register ("r_textureBits",		"default",					CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	r_vertexLighting	= Cvar_Register ("r_vertexLighting",	"0",			CVAR_ARCHIVE|CVAR_LATCH_VIDEO);

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

	intensity			= Cvar_Register ("intensity",			"2",						CVAR_ARCHIVE);

	gl_jpgquality		= Cvar_Register ("gl_jpgquality",		"85",						CVAR_ARCHIVE);
	gl_nobind			= Cvar_Register ("gl_nobind",			"0",						CVAR_CHEAT);
	gl_picmip			= Cvar_Register ("gl_picmip",			"0",						CVAR_LATCH_VIDEO);
	gl_screenshot		= Cvar_Register ("gl_screenshot",		"tga",						CVAR_ARCHIVE);

	gl_texturemode		= Cvar_Register ("gl_texturemode",		"GL_LINEAR_MIPMAP_NEAREST",	CVAR_ARCHIVE);

	// Force these to update next endframe
	r_swapInterval->modified = qTrue;
	gl_drawbuffer->modified = qTrue;
	gl_texturemode->modified = qTrue;
	r_ext_maxAnisotropy->modified = qTrue;
	con_font->modified = qTrue;
	vid_gamma->modified = qTrue;

	// Add the various commands
	cmd_gfxInfo			= Cmd_AddCommand ("gfxinfo",		GL_GfxInfo_f,			"Prints out renderer information");
	cmd_rendererClass	= Cmd_AddCommand ("rendererclass",	GL_RendererClass_f,		"Prints out the renderer class");
	cmd_eglRenderer		= Cmd_AddCommand ("egl_renderer",	GL_RendererMsg_f,		"Spams to the server your renderer information");
	cmd_eglVersion		= Cmd_AddCommand ("egl_version",	GL_VersionMsg_f,		"Spams to the server your client version");
}


/*
===============
R_Init
===============
*/
rInit_t R_Init (void *hInstance, void *hWnd)
{
	char	*rendererBuffer;
	char	*vendorBuffer;
	int		refInitTime;

	refInitTime = Sys_Milliseconds ();
	Com_Printf (0, "\n-------- Refresh Initialization --------\n");

	r_refScene.frameCount = 1;
	r_refRegInfo.registerFrame = 1;

	// Register renderer cvars
	R_Register ();

	// Set extension/max defaults
	glConfig.extArbMultitexture = qFalse;
	glConfig.extBGRA = qFalse;
	glConfig.extCompiledVertArray = qFalse;
	glConfig.extDrawRangeElements = qFalse;
	glConfig.extFragmentProgram = qFalse;
	glConfig.extSGISGenMipmap = qFalse;
	glConfig.extSGISMultiTexture = qFalse;
	glConfig.extTex3D = qFalse;
	glConfig.extTexCompression = qFalse;
	glConfig.extTexCubeMap = qFalse;
	glConfig.extTexEdgeClamp = qFalse;
	glConfig.extTexEnvAdd = qFalse;
	glConfig.extTexEnvCombine = qFalse;
	glConfig.extNVTexEnvCombine4 = qFalse;
	glConfig.extTexEnvDot3 = qFalse;
	glConfig.extTexFilterAniso = qFalse;
	glConfig.extVertexBufferObject = qFalse;
	glConfig.extVertexProgram = qFalse;
	glConfig.extWinSwapInterval = qFalse;

	glConfig.max3DTexSize = 0;
	glConfig.maxAniso = 0;
	glConfig.maxCMTexSize = 0;
	glConfig.maxElementVerts = 0;
	glConfig.maxElementIndices = 0;
	glConfig.maxTexCoords = 0;
	glConfig.maxTexImageUnits = 0;
	glConfig.maxTexSize = 256;
	glConfig.maxTexUnits = 1;

	glStatic.useStencil = qFalse;
	glStatic.cColorBits = 0;
	glStatic.cAlphaBits = 0;
	glStatic.cDepthBits = 0;
	glStatic.cStencilBits = 0;

	// Initialize our QGL dynamic bindings
	if (!QGL_Init (gl_driver->string)) {
		Com_Printf (PRNT_ERROR, "...could not load \"%s\"\n", gl_driver->string);
		QGL_Shutdown ();
		return R_INIT_QGL_FAIL;
	}

	// Initialize OS-specific parts of OpenGL
	if (!GLimp_Init (hInstance, hWnd)) {
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
	glStatic.vendorString = qglGetString (GL_VENDOR);
	Com_Printf (0, "GL_VENDOR: %s\n", glStatic.vendorString);

	vendorBuffer = Mem_StrDup ((char *)glStatic.vendorString);
	Q_strlwr (vendorBuffer);

	// Renderer string
	glStatic.rendererString = qglGetString (GL_RENDERER);
	Com_Printf (0, "GL_RENDERER: %s\n", glStatic.rendererString);

	rendererBuffer = Mem_StrDup ((char *)glStatic.rendererString);
	Q_strlwr (rendererBuffer);

	// Version string
	glStatic.versionString = qglGetString (GL_VERSION);
	Com_Printf (0, "GL_VERSION: %s\n", glStatic.versionString);

	// Extension string
	glStatic.extensionString = qglGetString (GL_EXTENSIONS);

	// Decide on a renderer class
	if (strstr (rendererBuffer, "glint"))			glStatic.renderClass = REND_CLASS_3DLABS_GLINT_MX;
	else if (strstr (rendererBuffer, "permedia"))	glStatic.renderClass = REND_CLASS_3DLABS_PERMEDIA;
	else if (strstr (rendererBuffer, "glzicd"))		glStatic.renderClass = REND_CLASS_3DLABS_REALIZM;
	else if (strstr (vendorBuffer, "ati ")) {
		if (strstr (vendorBuffer, "radeon"))
			glStatic.renderClass = REND_CLASS_ATI_RADEON;
		else
			glStatic.renderClass = REND_CLASS_ATI;
	}
	else if (strstr (vendorBuffer, "intel"))		glStatic.renderClass = REND_CLASS_INTEL;
	else if (strstr (vendorBuffer, "nvidia")) {
		if (strstr (rendererBuffer, "geforce"))
			glStatic.renderClass = REND_CLASS_NVIDIA_GEFORCE;
		else
			glStatic.renderClass = REND_CLASS_NVIDIA;
	}
	else if (strstr	(rendererBuffer, "pmx"))		glStatic.renderClass = REND_CLASS_PMX;
	else if (strstr	(rendererBuffer, "pcx1"))		glStatic.renderClass = REND_CLASS_POWERVR_PCX1;
	else if (strstr	(rendererBuffer, "pcx2"))		glStatic.renderClass = REND_CLASS_POWERVR_PCX2;
	else if (strstr	(rendererBuffer, "verite"))		glStatic.renderClass = REND_CLASS_RENDITION;
	else if (strstr (vendorBuffer, "s3"))			glStatic.renderClass = REND_CLASS_S3;
	else if (strstr (rendererBuffer, "prosavage"))	glStatic.renderClass = REND_CLASS_S3;
	else if (strstr (rendererBuffer, "twister"))	glStatic.renderClass = REND_CLASS_S3;
	else if (strstr	(vendorBuffer, "sgi"))			glStatic.renderClass = REND_CLASS_SGI;
	else if (strstr	(vendorBuffer, "sis"))			glStatic.renderClass = REND_CLASS_SIS;
	else if (strstr (rendererBuffer, "voodoo"))		glStatic.renderClass = REND_CLASS_VOODOO;
	else {
		if (strstr (rendererBuffer, "gdi generic")) {
			glStatic.renderClass = REND_CLASS_MCD;

			// MCD has buffering issues
			Cvar_VariableSetValue (gl_finish, 1, qTrue);
		}
		else
			glStatic.renderClass = REND_CLASS_DEFAULT;
	}

	// Print the renderer class
	Com_Printf (0, "Renderer Class: %s\n", R_RendererClass ());

#ifdef GL_FORCEFINISH
	Cvar_VariableSetValue (gl_finish, 1, qTrue);
#endif

	// Check stencil buffer availability and usability
	Com_Printf (0, "...stencil buffer ");
	if (gl_stencilbuffer->integer && glStatic.cStencilBits > 0) {
		if (glStatic.renderClass == REND_CLASS_VOODOO)
			Com_Printf (0, "ignored\n");
		else {
			Com_Printf (0, "available\n");
			glStatic.useStencil = qTrue;
		}
	}
	else {
		Com_Printf (0, "disabled\n");
	}

#ifdef WIN32
	// Check if CDS is allowed
	switch (glStatic.renderClass) {
	case REND_CLASS_3DLABS_GLINT_MX:
	case REND_CLASS_3DLABS_PERMEDIA:
	case REND_CLASS_3DLABS_REALIZM:
		if (gl_3dlabs_broken->integer)
			glStatic.allowCDS = qFalse;
		else
			glStatic.allowCDS = qTrue;
		break;

	default:
		glStatic.allowCDS = qTrue;
		break;
	}

	// Notify if CDS is allowed
	if (!glStatic.allowCDS)
		Com_Printf (0, "...disabling CDS\n");
#endif // WIN32

	// Grab opengl extensions
	if (r_allowExtensions->integer)
		GL_InitExtensions ();
	else
		Com_Printf (0, "...ignoring OpenGL extensions\n");

	// Set the default state
	GL_SetDefaultState ();

	// Map overbrights
	glState.pow2MapOvrbr = r_lmModulate->integer;

	if (glState.pow2MapOvrbr > 0)
		glState.pow2MapOvrbr = pow (2, glState.pow2MapOvrbr) / 255.0f;
	else
		glState.pow2MapOvrbr = 1.0f / 255.0f;

	// Retreive generic information
	if (gl_maxTexSize->integer > 0 && gl_maxTexSize->integer >= 128) {
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
	R_EntityInit ();
	R_WorldInit ();
	RB_Init ();

	Com_Printf (0, "init time: %.2fs\n", (Sys_Milliseconds () - refInitTime)*0.001f);
	Com_Printf (0, "----------------------------------------\n");

	// Check for gl errors
	GL_CheckForError ("R_Init");

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
	// Remove commands
	Cmd_RemoveCommand ("gfxinfo", cmd_gfxInfo);
	Cmd_RemoveCommand ("rendererclass", cmd_rendererClass);
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
