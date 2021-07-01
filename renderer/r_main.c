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

// r_main.c

#include "r_local.h"
#include "warpsin.h"

glConfig_t	glConfig;
glMedia_t	glMedia;
glSpeeds_t	glSpeeds;
glState_t	glState;

vec3_t		v_Forward;
vec3_t		v_Right;
vec3_t		v_Up;

mat4_t		r_ModelViewMatrix;
mat4_t		r_ProjectionMatrix;
mat4_t		r_WorldViewMatrix;

refDef_t	r_RefDef;
vidDef_t	vidDef;

qBool		r_FirstTime;

//
// scene items
//

entity_t		*r_CurrEntity;
model_t			*r_CurrModel;

int				r_NumEntities;
entity_t		r_Entities[MAX_ENTITIES];

int				r_NumParticles;
particle_t		r_Particles[MAX_PARTICLES];

int				r_NumDecals;
decal_t			r_Decals[MAX_DECALS];

int				r_NumDLights;
dLight_t		r_DLights[MAX_DLIGHTS];

lightStyle_t	r_LightStyles[MAX_LIGHTSTYLES];

//
// cvars
//

cvar_t	*con_font;

cvar_t	*e_test_0;
cvar_t	*e_test_1;

cvar_t	*gl_3dlabs_broken;
cvar_t	*gl_bitdepth;
cvar_t	*gl_clear;
cvar_t	*gl_cull;

cvar_t	*gl_extensions;
cvar_t	*gl_arb_texture_compression;
cvar_t	*gl_arb_texture_cube_map;
cvar_t	*gl_arb_vertex_buffer_object;
cvar_t	*gl_ext_bgra;
cvar_t	*gl_ext_compiled_vertex_array;
cvar_t	*gl_ext_draw_range_elements;
cvar_t	*gl_ext_max_anisotropy;
cvar_t	*gl_ext_mtexcombine;
cvar_t	*gl_ext_multitexture;
cvar_t	*gl_ext_swapinterval;
cvar_t	*gl_ext_texture_edge_clamp;
cvar_t	*gl_ext_texture_env_add;
cvar_t	*gl_ext_texture_filter_anisotropic;
cvar_t	*gl_nv_multisample_filter_hint;
cvar_t	*gl_nv_texture_shader;
cvar_t	*gl_sgis_generate_mipmap;

cvar_t	*gl_drawbuffer;
cvar_t	*gl_driver;
cvar_t	*gl_dynamic;

cvar_t	*gl_finish;
cvar_t	*gl_flashblend;
cvar_t	*gl_lightmap;
cvar_t	*gl_lockpvs;
cvar_t	*gl_log;
cvar_t	*gl_mode;
cvar_t	*gl_modulate;

cvar_t	*gl_polyblend;
cvar_t	*gl_saturatelighting;
cvar_t	*gl_shadows;
cvar_t	*gl_showtris;
cvar_t	*gl_swapinterval;

cvar_t	*part_shade;

cvar_t	*r_advir;
cvar_t	*r_caustics;
cvar_t	*r_displayfreq;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;

cvar_t	*r_fog;
cvar_t	*r_fog_start;
cvar_t	*r_fog_end;
cvar_t	*r_fog_density;
cvar_t	*r_fog_red;
cvar_t	*r_fog_green;
cvar_t	*r_fog_blue;
cvar_t	*r_fog_water;

cvar_t	*r_fontscale;
cvar_t	*r_fullbright;
cvar_t	*r_hudscale;
cvar_t	*r_lerpmodels;
cvar_t	*r_lightlevel;
cvar_t	*r_nocull;
cvar_t	*r_norefresh;
cvar_t	*r_novis;
cvar_t	*r_overbrightbits;
cvar_t	*r_speeds;

/*
=============================================================

	FRUSTUM

=============================================================
*/

cBspPlane_t	r_Frustum[4];

/*
=================
R_CullBox

Returns qTrue if the box is completely outside the frustum
=================
*/
qBool R_CullBox (vec3_t mins, vec3_t maxs) {
	int			i;
	cBspPlane_t	*p;

	if (r_nocull->integer)
		return qFalse;

	for (i=0, p=r_Frustum ; i<4 ; i++, p++) {
		switch (p->signBits) {
		case 0:
			if (p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist)
				return qTrue;
			break;
		case 1:
			if (p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist)
				return qTrue;
			break;
		case 2:
			if (p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist)
				return qTrue;
			break;
		case 3:
			if (p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist)
				return qTrue;
			break;
		case 4:
			if (p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist)
				return qTrue;
			break;
		case 5:
			if (p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist)
				return qTrue;
			break;
		case 6:
			if (p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist)
				return qTrue;
			break;
		case 7:
			if (p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist)
				return qTrue;
			break;
		default:
			assert (0);
			return qFalse;
		}
	}

	return qFalse;
}


/*
=================
R_CullSphere

Returns qTrue if the sphere is completely outside the frustum
=================
*/
qBool R_CullSphere (const vec3_t origin, const float radius, const int clipFlags) {
	int			i;
	cBspPlane_t	*p;

	if (r_nocull->integer)
		return qFalse;

	for (i=0, p=r_Frustum ; i<4 ; i++, p++) {
		if (!(clipFlags & (1<<i)))
			continue;

		if (DotProduct (origin, p->normal) - p->dist <= -radius)
			return qTrue;
	}

	return qFalse;
}


/*
===============
R_SetFrustum
===============
*/
static void R_SetFrustum (void) {
	int		i;

	// build the transformation matrix for the given view angles
	AngleVectors (r_RefDef.viewAngles, v_Forward, v_Right, v_Up);

	RotatePointAroundVector (r_Frustum[0].normal, v_Up,		v_Forward,	-(90-r_RefDef.fovX / 2));
	RotatePointAroundVector (r_Frustum[1].normal, v_Up,		v_Forward,	90-r_RefDef.fovX / 2);
	RotatePointAroundVector (r_Frustum[2].normal, v_Right,	v_Forward,	90-r_RefDef.fovY / 2);
	RotatePointAroundVector (r_Frustum[3].normal, v_Right,	v_Forward,	-(90 - r_RefDef.fovY / 2));

	for (i=0 ; i<4 ; i++) {
		r_Frustum[i].type = PLANE_ANYZ;
		r_Frustum[i].dist = DotProduct (r_RefDef.viewOrigin, r_Frustum[i].normal);
		r_Frustum[i].signBits = SignbitsForPlane (&r_Frustum[i]);
	}
}

/*
=============================================================

	RENDERING

=============================================================
*/

/*
============
R_PolyBlend

Fixed by Vic
============
*/
static void R_PolyBlend (void) {
	if (!gl_polyblend->integer)
		return;
	if (r_RefDef.vBlend[3] < 0.01f)
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

	qglColor4fv (r_RefDef.vBlend);
	qglBegin (GL_TRIANGLES);

	qglVertex2f (-5, -5);
	qglVertex2f (10, -5);
	qglVertex2f (-5, 10);

	qglEnd ();

	qglDisable (GL_BLEND);
	qglEnable (GL_TEXTURE_2D);

	qglColor4f (1, 1, 1, 1);
}


/*
================
R_DrawRSpeeds

Dev information printed to the screen
================
*/
static void R_DrawRSpeeds (void) {
	if (r_speeds->integer) {
		char	string[128];
		int		vOffset = 4;
		vec4_t	color = { colorCyan[0], colorCyan[1], colorCyan[2], Cvar_VariableValue ("scr_hudalpha") };

		// shader specific
		if (r_shaders && r_shaders->integer) {
			sprintf (string, "%4i shader %5i pass (%1i cube)",
							glSpeeds.shaderCount,
							glSpeeds.shaderPasses,
							glSpeeds.shaderCMPasses);

			Draw_String(vidDef.width - (strlen (string) * HUD_FTSIZE) - 2,
						vidDef.height - ((HUD_FTSIZE + FT_SHAOFFSET) * vOffset++),
						FS_SHADOW, HUD_SCALE, string, color);
		}

		// lighting
		sprintf (string, "%1i dlight %1i lmaps",
						r_NumDLights,
						glSpeeds.visibleLightmaps);

		Draw_String(vidDef.width - (strlen (string) * HUD_FTSIZE) - 2,
					vidDef.height - ((HUD_FTSIZE + FT_SHAOFFSET) * vOffset++),
					FS_SHADOW, HUD_SCALE, string, color);

		// world
		sprintf (string, "%5i wpoly %3i leaf %2i sky %1i tex",
						glSpeeds.worldPolys,
						glSpeeds.worldLeafs,
						glSpeeds.skyPolys,
						glSpeeds.visibleTextures);

		Draw_String(vidDef.width - (strlen (string) * HUD_FTSIZE) - 2,
					vidDef.height - ((HUD_FTSIZE + FT_SHAOFFSET) * vOffset++),
					FS_SHADOW, HUD_SCALE, string, color);

		// particle/decal
		sprintf (string, "%5i part %4i decal",
						r_NumParticles,
						r_NumDecals);

		Draw_String(vidDef.width - (strlen (string) * HUD_FTSIZE) - 2,
					vidDef.height - ((HUD_FTSIZE + FT_SHAOFFSET) * vOffset++),
					FS_SHADOW, HUD_SCALE, string, color);

		// alias
		sprintf (string, "%i/%i vis/tot ent %5i epoly",
						glSpeeds.numEntities,
						r_NumEntities,
						glSpeeds.aliasPolys);

		Draw_String(vidDef.width - (strlen (string) * HUD_FTSIZE) - 2,
					vidDef.height - ((HUD_FTSIZE + FT_SHAOFFSET) * vOffset++),
					FS_SHADOW, HUD_SCALE, string, color);

		glSpeeds.numEntities = 0;
		glSpeeds.aliasPolys = 0;

		glSpeeds.worldPolys = 0;
		glSpeeds.worldLeafs = 0;
		glSpeeds.skyPolys = 0;
		glSpeeds.visibleTextures = 0;

		glSpeeds.shaderCount = 0;
		glSpeeds.shaderPasses = 0;
		glSpeeds.shaderCMPasses = 0;
	}
}


/*
================
R_SetupFog
================
*/
static void R_SetupFog (void) {
	if (r_fog->integer) {
		GLfloat	fogColor[3];
		qBool	inWater;

		if (r_fog_water->integer && (r_RefDef.rdFlags & RDF_UNDERWATER)) {
			int		pointContents;

			pointContents = CM_PointContents (r_RefDef.viewOrigin, 0);
			VectorSet (fogColor, 0.5, 0.5, 0.5);
			if (pointContents & CONTENTS_LAVA) {
				inWater = qTrue;
				fogColor[0] = 1;
			}
			if (pointContents & CONTENTS_SLIME) {
				inWater = qTrue;
				fogColor[1] = 1;
			}
			if (pointContents & CONTENTS_WATER) {
				inWater = qTrue;
				fogColor[2] = 1;
			}

			if (inWater) {
				qglFogi (GL_FOG_MODE,	GL_LINEAR);

				qglFogf (GL_FOG_START,	32);
				qglFogf (GL_FOG_END,	1024);
			}
		} else
			inWater = qFalse;

		if (!inWater) {
			VectorSet (fogColor, r_fog_red->value, r_fog_green->value, r_fog_blue->value);

			switch (r_fog->integer) {
			case 3:		qglFogi (GL_FOG_MODE, GL_EXP2);		break;
			case 2:		qglFogi (GL_FOG_MODE, GL_EXP);		break;
			default:	qglFogi (GL_FOG_MODE, GL_LINEAR);	break;
			}

			qglFogf (GL_FOG_START,		r_fog_start->value);
			qglFogf (GL_FOG_END,		r_fog_end->value);
			qglFogf (GL_FOG_DENSITY,	r_fog_density->value);
		}

		qglFogfv (GL_FOG_COLOR,		fogColor);
		qglEnable (GL_FOG);
	}
}

/*
=============================================================

	FRAME RENDERING

=============================================================
*/

/*
================
R_RenderScene

r_RefDef must be set before the first call
================
*/
static void R_RenderScene (refDef_t *fd) {
	if (r_norefresh->integer)
		return;

	r_RefDef = *fd;

	if (!r_WorldModel && !(r_RefDef.rdFlags & RDF_NOWORLDMODEL))
		Com_Error (ERR_DROP, "R_RenderScene: NULL worldmodel");

	GL_SetupGL3D ();

	R_PushDLights ();

	R_SetFrustum ();
	R_MarkLeaves ();

	R_SetupFog ();
	R_DrawWorld ();
	R_DrawDecals ();
	R_DrawEntities (qFalse);
	R_RenderDLights ();
	R_DrawParticles ();
	R_DrawAlphaSurfaces ();
	R_DrawEntities (qTrue);

	R_PolyBlend ();

	qglDisable (GL_FOG);
}


/*
==================
R_RenderFrame
==================
*/
void R_RenderFrame (refDef_t *fd) {
	glState.in2D = qFalse;

	R_RenderScene (fd);
	R_SetLightLevel ();
	GL_SetupGL2D ();

	if (r_WorldModel && !(r_RefDef.rdFlags & RDF_NOWORLDMODEL))
		R_DrawRSpeeds ();
}


/*
==================
R_BeginFrame
==================
*/
void R_BeginFrame (float cameraSeparation) {
	glState.cameraSeparation = cameraSeparation;
	glState.realTime = (Sys_Milliseconds () * 0.001f);

	// frame logging
	if (gl_log->modified)
		QGL_ToggleLogging (gl_log->integer);

	if (gl_log->integer)
		QGL_LogBeginFrame ();

	// check cvar settings
	R_UpdateCvars ();

	// setup the frame for rendering
	GLimp_BeginFrame ();

	// go into 2D mode
	GL_SetupGL2D ();
}


/*
==================
R_EndFrame
==================
*/
void R_EndFrame (void) {
	if (gl_finish->integer)
		qglFinish ();

	GLimp_EndFrame ();

	if (gl_log->integer)
		QGL_LogEndFrame ();

	r_FrameCount++;
}


/*
====================
R_ClearFrame
====================
*/
void R_ClearFrame (void) {
	r_NumEntities = 0;
	r_NumParticles = 0;
	r_NumDecals = 0;
	r_NumDLights = 0;
}


/*
=====================
R_AddEntity
=====================
*/
void R_AddEntity (entity_t *ent) {
	if (r_NumEntities >= MAX_ENTITIES)
		return;

	if (!(ent->flags & RF_TRANSLUCENT) && (ent->color[3] < 1.0f))
		ent->flags |= RF_TRANSLUCENT;

	if (ent->color[3] <= 0.0f)
		return;

	r_Entities[r_NumEntities++] = *ent;
}


/*
=====================
R_AddParticle
=====================
*/
void R_AddParticle (vec3_t org, vec3_t angle, dvec4_t color, double size,
					image_t *image, int flags, int sFactor, int dFactor, double orient) {
	particle_t	*p;

	if (r_NumParticles >= MAX_PARTICLES)
		return;

	if (color[3] <= 0.0f)
		return;

	p = &r_Particles[r_NumParticles++];

	VectorCopy (org, p->origin);
	VectorCopy (angle, p->angle);

	Vector4Copy (color, p->color);
	
	p->image = image;
	p->flags = flags;
	
	p->size = size;

	p->sFactor = sFactor;
	p->dFactor = dFactor;

	p->orient = orient;
}


/*
=====================
R_AddDecal
=====================
*/
void R_AddDecal (vec2_t *coords, vec3_t *verts, int numVerts, struct mNode_s *node,
				dvec4_t color, image_t *image, int flags, int sFactor, int dFactor) {
	decal_t		*d;

	if (r_NumDecals >= MAX_DECALS)
		return;

	if (color[3] <= 0.0f)
		return;

	d = &r_Decals[r_NumDecals++];

	Vector4Copy (color, d->color);

	d->image = image;
	d->flags = flags;

	d->sFactor = sFactor;
	d->dFactor = dFactor;

	d->numVerts = numVerts;
	d->node = node;
	d->coords = coords;
	d->verts = verts;
}


/*
=====================
R_AddLight
=====================
*/
void R_AddLight (vec3_t org, float intensity, float r, float g, float b) {
	dLight_t	*dl;

	if (r_NumDLights >= MAX_DLIGHTS)
		return;

	if (intensity == 0.0f)
		return;

	dl = &r_DLights[r_NumDLights++];

	VectorCopy (org, dl->origin);
	VectorSet (dl->color, r, g, b);
	dl->intensity = intensity;
}


/*
=====================
R_AddLightStyle
=====================
*/
void R_AddLightStyle (int style, float r, float g, float b) {
	lightStyle_t	*ls;

	if ((style < 0) || (style > MAX_LIGHTSTYLES)) {
		Com_Error (ERR_DROP, "Bad light style %i", style);
		return;
	}

	ls = &r_LightStyles[style];

	ls->white	= r+g+b;
	VectorSet (ls->rgb, r, g, b);
}

/*
=============================================================

	MISC

=============================================================
*/

/*
==================
GL_CheckForError
==================
*/
static const char *GetGLErrorString (GLenum error) {
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
void GL_CheckForError (void) {
	int		err;

	err = qglGetError ();
	if (err != GL_NO_ERROR)
		Com_Printf (PRINT_ALL, S_COLOR_RED "GL_ERROR: glGetError (): '%s' (0x%x)\n", GetGLErrorString (err), err);
}


/*
==================
GL_GfxInfo_f
==================
*/
static void GL_GfxInfo_f (void) {
	Com_Printf (PRINT_ALL, "----------------------------------------\n");

	Com_Printf (PRINT_ALL, "EGL v%s:\n" "%s PFD: c(%d-bits) a(%d-bits) z(%d-bit) s(%d-bit)\n",
				EGL_VERSTR,
				glConfig.wglPFD ? "WGL" : "GL",
				glConfig.cColorBits, glConfig.cAlphaBits, glConfig.cDepthBits, glConfig.cStencilBits);

	Com_Printf (PRINT_ALL, "Renderer Class:");
	if (glConfig.rendType & GLREND_DEFAULT)		Com_Printf (PRINT_ALL, " [Default]");

	if (glConfig.rendType & GLREND_VOODOO)		Com_Printf (PRINT_ALL, " [Voodoo]");

	if (glConfig.rendType & GLREND_PCX1)		Com_Printf (PRINT_ALL, " [PCX1]");
	if (glConfig.rendType & GLREND_PCX2)		Com_Printf (PRINT_ALL, " [PCX2]");
	if (glConfig.rendType & GLREND_PMX)			Com_Printf (PRINT_ALL, " [PMX]");
	if (glConfig.rendType & GLREND_POWERVR)		Com_Printf (PRINT_ALL, " [POWERVR]");
	if (glConfig.rendType & GLREND_PERMEDIA2)	Com_Printf (PRINT_ALL, " [PERMEDIA2]");
	if (glConfig.rendType & GLREND_GLINT_MX)	Com_Printf (PRINT_ALL, " [GLINT MX]");
	if (glConfig.rendType & GLREND_3DLABS)		Com_Printf (PRINT_ALL, " [3DLabs]");
	if (glConfig.rendType & GLREND_REALIZM)		Com_Printf (PRINT_ALL, " [Realizm]");
	if (glConfig.rendType & GLREND_RENDITION)	Com_Printf (PRINT_ALL, " [Rendition]");
	if (glConfig.rendType & GLREND_SGI)			Com_Printf (PRINT_ALL, " [SGI]");
	if (glConfig.rendType & GLREND_MCD)			Com_Printf (PRINT_ALL, " [MCD]");
	if (glConfig.rendType & GLREND_SIS)			Com_Printf (PRINT_ALL, " [SiS]");

	if (glConfig.rendType & GLREND_NVIDIA)		Com_Printf (PRINT_ALL, " [nVidia]");
	if (glConfig.rendType & GLREND_GEFORCE)		Com_Printf (PRINT_ALL, " [GeForce]");

	if (glConfig.rendType & GLREND_ATI)			Com_Printf (PRINT_ALL, " [ATI]");
	if (glConfig.rendType & GLREND_RADEON)		Com_Printf (PRINT_ALL, " [Radeon]");
	Com_Printf (PRINT_ALL, "\n");

	Com_Printf (PRINT_ALL, "----------------------------------------\n");

	Com_Printf (PRINT_ALL, "GL_VENDOR: %s\n",		glConfig.vendString);
	Com_Printf (PRINT_ALL, "GL_RENDERER: %s\n",		glConfig.rendString);
	Com_Printf (PRINT_ALL, "GL_VERSION: %s\n",		glConfig.versString);
	Com_Printf (PRINT_ALL, "GL_EXTENSIONS: %s\n",	glConfig.extsString);

	Com_Printf (PRINT_ALL, "----------------------------------------\n");

	Com_Printf (PRINT_ALL, "CDS: %s\n", (glConfig.allowCDS) ? "Enabled" : "Disabled");
	Com_Printf (PRINT_ALL, "Extensions:\n");
	Com_Printf (PRINT_ALL, "...ARB Multitexture: %s\n",				glConfig.extArbMultitexture ? "On" : "Off");
	Com_Printf (PRINT_ALL, "...ARB Texture Compression: %s\n",		glConfig.extArbTexCompression ? "On" : "Off");
	Com_Printf (PRINT_ALL, "...ARB Texture Cube Map: %s\n",			glConfig.extArbTexCubeMap ? "On" : "Off");
	Com_Printf (PRINT_ALL, "...ARB Vertex Buffer Object: %s\n",		glConfig.extArbVertBufferObject ? "On" : "Off");
	Com_Printf (PRINT_ALL, "...RGBA -> BGRA: %s\n",					glConfig.extBGRA ? "On" : "Off");
	Com_Printf (PRINT_ALL, "...Compiled Vertex Array: %s\n",		glConfig.extCompiledVertArray ? "On" : "Off");
	Com_Printf (PRINT_ALL, "...Draw Range Elements: %s\n",			glConfig.extDrawRangeElements ? "On" : "Off");
	Com_Printf (PRINT_ALL, "...Multitexture Combine: %s\n",			glConfig.extMTexCombine ? "On" : "Off");
	Com_Printf (PRINT_ALL, "...nVidia Multisample Hint: %s\n",		glConfig.extNVMulSampleHint ? "On" : "Off");
	Com_Printf (PRINT_ALL, "...nVidia Texture Shader: %s\n",		glConfig.extNVTexShader ? "On" : "Off");
	Com_Printf (PRINT_ALL, "...SGIS Mipmap Generation: %s\n",		glConfig.extSGISGenMipmap ? "On" : "Off");
	Com_Printf (PRINT_ALL, "...SGIS Multitexture: %s\n",			glConfig.extSGISMultiTexture ? "On" : glConfig.extArbMultitexture ? "Deprecated for ARB Multitexture" : "Off");
	Com_Printf (PRINT_ALL, "...Texture Edge Clamp: %s\n",			glConfig.extTexEdgeClamp ? "On" : "Off");
	Com_Printf (PRINT_ALL, "...Texture Env Add: %s\n",				glConfig.extTexEnvAdd ? "On" : "Off");
	Com_Printf (PRINT_ALL, "...Texture Filter Anisotropic: %s\n",	glConfig.extTexFilterAniso ? "On" : "Off");

	Com_Printf (PRINT_ALL, "----------------------------------------\n");

	GL_TextureMode ();
	GL_TextureBits ();

	Com_Printf (PRINT_ALL, "----------------------------------------\n");

	if (glConfig.maxElementVerts)	Com_Printf (PRINT_ALL, "GL_MAX_ELEMENTS_VERTICES_EXT: %i\n", glConfig.maxElementVerts);
	if (glConfig.maxElementIndices)	Com_Printf (PRINT_ALL, "GL_MAX_ELEMENTS_INDICES_EXT: %i\n", glConfig.maxElementIndices);
	if (glConfig.maxAniso)			Com_Printf (PRINT_ALL, "GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT: %i\n", glConfig.maxAniso);
	if (glConfig.maxCMTexSize)		Com_Printf (PRINT_ALL, "GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB: %i\n", glConfig.maxCMTexSize);
	Com_Printf (PRINT_ALL, "GL_MAX_TEXTURE_SIZE: %i\n", glConfig.maxTexSize);
	Com_Printf (PRINT_ALL, "GL_MAX_TEXTURE_UNITS: %i\n", glConfig.maxTexUnits);

	Com_Printf (PRINT_ALL, "----------------------------------------\n");
}


/*
==================
GL_RendererMsg_f
==================
*/
void GL_RendererMsg_f (void) {
	int		width;
	int		height;

	R_GetInfoForMode (gl_mode->integer, &width, &height);

	Cbuf_AddText (va("say [EGL v%s]: [%s: %s v%s] %s PFD[c%d/a%d/z%d/s%d] RES[%dx%dx%d]\n",
				EGL_VERSTR,
				glConfig.vendString, glConfig.rendString, glConfig.versString,
				glConfig.wglPFD ? "WGL" : "GL",
				glConfig.cColorBits, glConfig.cAlphaBits, glConfig.cDepthBits, glConfig.cStencilBits,
				width, height, glConfig.vidBitDepth));
}


/*
==================
GL_VersionMsg_f
==================
*/
void GL_VersionMsg_f (void) {
	Cbuf_AddText (va("say [EGL v%s (%s-%s) by Echon] [www.echon.org]\n",
				EGL_VERSTR, BUILDSTRING, CPUSTRING));
}


/*
===============
R_ExtensionAvailable
===============
*/
qBool R_ExtensionAvailable (const char *extensions, const char *extension) {
	const byte	*start;
	byte		*where, *terminator;

	// extension names should not have spaces
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
==================
R_UpdateCvars

Locks/limits/checks certain cvars before rendering
==================
*/
static void R_UpdateCvars (void) {
	// multisampling update
	if (gl_nv_multisample_filter_hint->modified) {
		gl_nv_multisample_filter_hint->modified = qFalse;
		if (glConfig.extMultiSample && glConfig.extNVMulSampleHint) {
			if (!strcmp (gl_nv_multisample_filter_hint->string, "nicest"))
				qglHint (GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
			else
				qglHint (GL_MULTISAMPLE_FILTER_HINT_NV, GL_FASTEST);
		}
	}

	// lock r_overbrightbits to allowed values
	if (r_overbrightbits->modified) {
		r_overbrightbits->modified = qFalse;

		if (glConfig.extMTexCombine) {
			if (r_overbrightbits->integer >= 4)
				Cvar_ForceSetValue ("r_overbrightbits", 4);
			else if (r_overbrightbits->integer >= 2)
				Cvar_ForceSetValue ("r_overbrightbits", 2);
			else
				Cvar_ForceSetValue ("r_overbrightbits", 0);
		} else
			Cvar_ForceSetValue ("r_overbrightbits", 0);
	}

	// draw buffer stuff
	if (gl_drawbuffer->modified) {
		gl_drawbuffer->modified = qFalse;
		if (!glState.cameraSeparation || !glState.stereoEnabled) {
			if (!Q_stricmp (gl_drawbuffer->string, "GL_FRONT"))
				qglDrawBuffer (GL_FRONT);
			else
				qglDrawBuffer (GL_BACK);
		}
	}

	// update the swap interval*/
	if (gl_swapinterval->modified) {
		gl_swapinterval->modified = qFalse;

#ifdef _WIN32
		if (!glState.stereoEnabled && qwglSwapIntervalEXT) 
			qwglSwapIntervalEXT (gl_swapinterval->integer);
#endif
	}

	// make sure fullscreen matches
	if (vid_fullscreen->modified) {
		vid_fullscreen->modified = qFalse;
		glConfig.vidFullscreen = (vid_fullscreen->integer != 0) ? qTrue : qFalse;
	}

	// no need to do the following stuff twice
	if (r_FirstTime)
		return;

	// texturemode stuff
	if (gl_texturemode->modified)
		GL_TextureMode ();

	// update anisotropy
	if (gl_ext_max_anisotropy->modified)
		GL_ResetAnisotropy ();

	// update font
	if (con_font->modified)
		Draw_CheckFont ();
}

/*
=============================================================

	REGISTRATION

=============================================================
*/

/*
==================
R_BeginRegistration

Starts refresh registration on map load
==================
*/
void R_BeginRegistration (char *mapName) {
	Mod_BeginRegistration (mapName);
	Img_BeginRegistration ();

	R_RegisterMedia ();
}


/*
==================
R_EndRegistration

Called at the end of all registration by the client
==================
*/
void R_EndRegistration (void) {
	Mod_EndRegistration ();
	RS_EndRegistration ();
	Img_EndRegistration ();
}


/*
==================
R_InitMedia

Imagery that only needs to be setup on intialization
==================
*/
void R_InitMedia (void) {
	/*
	** chars image/shaders
	*/
	Draw_CheckFont ();

	glMedia.uiCharsImage = Img_FindImage ("ui/uichars.pcx", IF_NOFLUSH|IF_NOMIPMAP|IF_NOPICMIP|IF_NOGAMMA|IF_NOINTENS|IF_CLAMP);
	if (!glMedia.uiCharsImage)
		glMedia.uiCharsImage = glMedia.charsImage;

	/*
	** model_ShellTexture
	** Shader-less fallback
	*/
	glMedia.model_ShellTexture = Img_FindImage ("egl/shell_god.tga", IF_NOFLUSH|IF_NOGAMMA|IF_NOINTENS);
	if (!glMedia.model_ShellTexture)
		glMedia.model_ShellTexture = r_WhiteTexture;

	// and the rest
	R_RegisterMedia ();
	CL_InitMedia ();
}


/*
==================
R_RegisterMedia

Imagery/shader re-setup on map change and loaded on init
==================
*/
void R_RegisterMedia (void) {
	/*
	** model_Shell shaders
	*/
	glMedia.model_ShellGod		= RS_RegisterShader ("shell_god");
	glMedia.model_ShellHalfDam	= RS_RegisterShader ("shell_halfdam");
	glMedia.model_ShellDouble	= RS_RegisterShader ("shell_double");
	glMedia.model_ShellRed		= RS_RegisterShader ("shell_red");
	glMedia.model_ShellGreen	= RS_RegisterShader ("shell_green");
	glMedia.model_ShellBlue		= RS_RegisterShader ("shell_blue");
	glMedia.model_ShellIR		= RS_RegisterShader ("shell_ir");

	/*
	** world_Caustic shaders
	*/
	glMedia.world_LavaCaustics	= RS_RegisterShader ("egl/lavacaustics");
	glMedia.world_SlimeCaustics	= RS_RegisterShader ("egl/slimecaustics");
	glMedia.world_WaterCaustics	= RS_RegisterShader ("egl/watercaustics");
}

/*
=============================================================

	INIT / SHUTDOWN

=============================================================
*/

/*
============
R_GetInfoForMode
============
*/
typedef struct {
	char		*info;

	int			width;
	int			height;

	int			mode;
} vidMode_t;

static vidMode_t r_VidModes[] = {
	{"Mode 0: 320 x 240",		320,	240,	0 },
	{"Mode 1: 400 x 300",		400,	300,	1 },
	{"Mode 2: 512 x 384",		512,	384,	2 },
	{"Mode 3: 640 x 480",		640,	480,	3 },
	{"Mode 4: 800 x 600",		800,	600,	4 },
	{"Mode 5: 960 x 720",		960,	720,	5 },
	{"Mode 6: 1024 x 768",		1024,	768,	6 },
	{"Mode 7: 1152 x 864",		1152,	864,	7 },
	{"Mode 8: 1280 x 960",		1280,	960,	8 },
	{"Mode 9: 1600 x 1200",		1600,	1200,	9 },
	{"Mode 10: 1920 x 1440",	1920,	1440,	10},
	{"Mode 11: 2048 x 1536",	2048,	1536,	11},

	{"Mode 12: 1280 x 800 (ws)",	1280,	800,	12},
	{"Mode 13: 1440 x 900 (ws)",	1440,	900,	13}
};

#define NUM_VIDMODES (sizeof (r_VidModes) / sizeof (r_VidModes[0]))
qBool R_GetInfoForMode (int mode, int *width, int *height) {
	if ((mode < 0) || (mode >= NUM_VIDMODES))
		return qFalse;

	*width  = r_VidModes[mode].width;
	*height = r_VidModes[mode].height;
	return qTrue;
}

/*
==================
R_SetMode
==================
*/
static qBool R_SetMode (void) {
	int		err;

	// check if ChangeDisplaySettings is allowed (only fullscreen uses CDS in win32)
	if (vid_fullscreen->modified && !glConfig.allowCDS) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "CDS not allowed with this driver\n");
		Cvar_ForceSetValue ("vid_fullscreen", !vid_fullscreen->integer);
		vid_fullscreen->modified = qFalse;
	}

	vid_fullscreen->modified = qFalse;
	gl_mode->modified = qFalse;

	// set defaults
	vidDef.width = 0;
	vidDef.height = 0;
	glConfig.vidBitDepth = 0;
	glConfig.vidFrequency = 0;
	glConfig.vidFullscreen = (vid_fullscreen->integer != 0) ? qTrue : qFalse;

	if ((err = GLimp_AttemptMode (gl_mode->integer)) == RSErr_ok) {
		// it worked! set this as the safe mode
		glConfig.safeMode = gl_mode->integer;
	} else {
		// there was an error
		switch (err) {
		case RSErr_InvalidFullscreen:
			// fullscreen failed
			Com_Printf (PRINT_ALL, "..." S_COLOR_YELLOW "fullscreen unavailable in this mode\n");
			Cvar_ForceSetValue ("vid_fullscreen", 0);
			vid_fullscreen->modified = qFalse;
			glConfig.vidFullscreen = qFalse;
			if ((err = GLimp_AttemptMode (gl_mode->integer)) == RSErr_ok)
				return qTrue;
			break;

		case RSErr_InvalidMode:
			// fall to the last known safe mode
			Com_Printf (PRINT_ALL, "..." S_COLOR_YELLOW "invalid mode, attempting safe mode '%d'\n", glConfig.safeMode);
			Cvar_ForceSetValue ("gl_mode", glConfig.safeMode);
			gl_mode->modified = qFalse;
			break;

		case RSErr_unknown:
			// this should never happen!
			Com_Printf (PRINT_ALL, "..." S_COLOR_RED "unkown error!\n");
			break;
		}

		// set defaults
		vidDef.width = 0;
		vidDef.height = 0;
		glConfig.vidBitDepth = 0;
		glConfig.vidFrequency = 0;
		glConfig.vidFullscreen = (vid_fullscreen->integer != 0) ? qTrue : qFalse;

		// try setting it back to something safe
		if ((err = GLimp_AttemptMode (glConfig.safeMode)) != RSErr_ok) {
			Com_Printf (PRINT_ALL, "..." S_COLOR_RED "could not revert to safe mode\n");
			return qFalse;
		}
	}

	return qTrue;
}


/*
===============
GL_InitExtensions
===============
*/
static void GL_InitExtensions (void) {
	// check for gl errors
	GL_CheckForError ();

	/*
	** GL_ARB_multitexture
	** GL_SGIS_multitexture
	*/
	if (gl_ext_multitexture->integer) {
		// GL_ARB_multitexture
		if (R_ExtensionAvailable (glConfig.extsString, "GL_ARB_multitexture")) {
			qglMTexCoord2f = (void *) QGL_GetProcAddress ("glMultiTexCoord2fARB");
			if (qglMTexCoord2f)			qglActiveTextureARB = (void *) QGL_GetProcAddress ("glActiveTextureARB");
			if (qglActiveTextureARB)	qglClientActiveTextureARB = (void *) QGL_GetProcAddress ("glClientActiveTextureARB");

			if (!qglClientActiveTextureARB) {
				Com_Printf (PRINT_ALL, "..." S_COLOR_RED "GL_ARB_multitexture not properly supported!\n");
				qglMTexCoord2f				= NULL;
				qglActiveTextureARB			= NULL;
				qglClientActiveTextureARB	= NULL;
			} else {
				Com_Printf (PRINT_ALL, "...enabling GL_ARB_multitexture\n");
				glConfig.extArbMultitexture = qTrue;
			}
		} else
			Com_Printf (PRINT_ALL, "...GL_ARB_multitexture not found\n");

		// GL_SGIS_multitexture
		if (!glConfig.extArbMultitexture) {
			Com_Printf (PRINT_ALL, "...attempting GL_SGIS_multitexture\n");

			if (R_ExtensionAvailable (glConfig.extsString, "GL_SGIS_multitexture")) {
				qglMTexCoord2f = (void *) QGL_GetProcAddress ("glMTexCoord2fSGIS");
				if (qglMTexCoord2f)		qglSelectTextureSGIS = (void *) QGL_GetProcAddress ("glSelectTextureSGIS");

				if (!qglSelectTextureSGIS) {
					Com_Printf (PRINT_ALL, "..." S_COLOR_RED "GL_SGIS_multitexture not properly supported!\n");
					qglMTexCoord2f			= NULL;
					qglSelectTextureSGIS	= NULL;
				} else {
					Com_Printf (PRINT_ALL, "...enabling GL_SGIS_multitexture\n");
					glConfig.extSGISMultiTexture = qTrue;
				}
			} else
				Com_Printf (PRINT_ALL, "...GL_SGIS_multitexture not found\n");
		}
	} else {
		Com_Printf (PRINT_ALL, "...ignoring GL_ARB/SGIS_multitexture\n");
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "WARNING: Disabling multitexture is not recommended!\n");
	}

	// keep texture unit counts in check
	if (glConfig.extSGISMultiTexture || glConfig.extArbMultitexture) {
		qglGetIntegerv (GL_MAX_TEXTURE_UNITS, &glConfig.maxTexUnits);
		if (glConfig.maxTexUnits < 1)
			glConfig.maxTexUnits = 1;
		else {
			// sgis mtex doesn't support more than 2 units does it?
			if (glConfig.extSGISMultiTexture && (glConfig.maxTexUnits > 2))
				glConfig.maxTexUnits = 2;

			if (glConfig.maxTexUnits > GL_MAXTU)
				glConfig.maxTexUnits = GL_MAXTU;
		}
	} else
		glConfig.maxTexUnits = 1;

	/*
	** GL_ARB_texture_compression
	*/
	if (gl_arb_texture_compression->integer) {
		if (R_ExtensionAvailable (glConfig.extsString, "GL_ARB_texture_compression")) {
				Com_Printf (PRINT_ALL, "...enabling GL_ARB_texture_compression\n");
				glConfig.extArbTexCompression = qTrue;
		} else
			Com_Printf (PRINT_ALL, "...GL_ARB_texture_compression not found\n");
	} else
		Com_Printf (PRINT_ALL, "...ignoring GL_ARB_texture_compression\n");

	/*
	** GL_ARB_texture_cube_map
	*/
	if (gl_arb_texture_cube_map->integer) {
		if (R_ExtensionAvailable (glConfig.extsString, "GL_ARB_texture_cube_map")) {
				qglGetIntegerv (GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &glConfig.maxCMTexSize);

				if (glConfig.maxCMTexSize <= 0) {
					Com_Printf (PRINT_ALL, S_COLOR_RED "GL_ARB_texture_cube_map not properly supported!\n");
					glConfig.maxCMTexSize = 0;
				} else {
					Com_Printf (PRINT_ALL, "...enabling GL_ARB_texture_cube_map\n");
					glConfig.extArbTexCubeMap = qTrue;
					Q_NearestPow (&glConfig.maxCMTexSize, qTrue);
				}
		} else
			Com_Printf (PRINT_ALL, "...GL_ARB_texture_cube_map not found\n");
	} else
		Com_Printf (PRINT_ALL, "...ignoring GL_ARB_texture_cube_map\n");

	/*
	** GL_ARB_texture_env_add
	*/
	if (gl_ext_texture_env_add->integer) {
		if (R_ExtensionAvailable (glConfig.extsString, "GL_ARB_texture_env_add")) {
			if (glConfig.extSGISMultiTexture || glConfig.extArbMultitexture) {
				Com_Printf (PRINT_ALL, "...enabling GL_ARB_texture_env_add\n");
				glConfig.extTexEnvAdd = qTrue;
			} else
				Com_Printf (PRINT_ALL, "...ignoring GL_ARB_texture_env_add (no multitexture)\n");
		} else
			Com_Printf (PRINT_ALL, "...GL_ARB_texture_env_add not found\n");
	} else
		Com_Printf (PRINT_ALL, "...ignoring GL_ARB_texture_env_add\n");

	/*
	** GL_ARB_texture_env_combine
	** GL_EXT_texture_env_combine
	*/
	if (gl_ext_mtexcombine->integer) {
		if (R_ExtensionAvailable (glConfig.extsString, "GL_ARB_texture_env_combine") ||
			R_ExtensionAvailable (glConfig.extsString, "GL_EXT_texture_env_combine")) {
			if (glConfig.extSGISMultiTexture || glConfig.extArbMultitexture) {
				Com_Printf (PRINT_ALL, "...enabling GL_ARB/EXT_texture_env_combine\n");
				glConfig.extMTexCombine = qTrue;
			} else
				Com_Printf (PRINT_ALL, "...ignoring GL_ARB/EXT_texture_env_combine (no multitexture)\n");
		} else
			Com_Printf (PRINT_ALL, "...GL_ARB/EXT_texture_env_combine not found\n");
	} else
		Com_Printf (PRINT_ALL, "...ignoring GL_ARB/EXT_texture_env_combine\n");

	/*
	** GL_ARB_vertex_buffer_object
	*/
	if (gl_arb_vertex_buffer_object->integer) {
		if (R_ExtensionAvailable (glConfig.extsString, "GL_ARB_vertex_buffer_object")) {
			qglBindBufferARB = (void *) QGL_GetProcAddress ("glBindBufferARB");
			if (qglBindBufferARB)		qglDeleteBuffersARB = (void *) QGL_GetProcAddress ("glDeleteBuffersARB");
			if (qglDeleteBuffersARB)	qglGenBuffersARB = (void *) QGL_GetProcAddress ("glGenBuffersARB");
			if (qglGenBuffersARB)		qglIsBufferARB = (void *) QGL_GetProcAddress ("glIsBufferARB");
			if (qglIsBufferARB)			qglMapBufferARB = (void *) QGL_GetProcAddress ("glMapBufferARB");
			if (qglMapBufferARB)		qglUnmapBufferARB = (void *) QGL_GetProcAddress ("glUnmapBufferARB");
			if (qglUnmapBufferARB)		qglBufferDataARB = (void *) QGL_GetProcAddress ("glBufferDataARB");
			if (qglBufferDataARB)		qglBufferSubDataARB = (void *) QGL_GetProcAddress ("glBufferSubDataARB");

			if (!qglBufferDataARB) {
				Com_Printf (PRINT_ALL, S_COLOR_RED "GL_ARB_vertex_buffer_object not properly supported!\n");
				qglBindBufferARB	= NULL;
				qglDeleteBuffersARB	= NULL;
				qglGenBuffersARB	= NULL;
				qglIsBufferARB		= NULL;
				qglMapBufferARB		= NULL;
				qglUnmapBufferARB	= NULL;
				qglBufferDataARB	= NULL;
				qglBufferSubDataARB	= NULL;
			} else {
				Com_Printf (PRINT_ALL, "...enabling GL_ARB_vertex_buffer_object\n");
				glConfig.extArbVertBufferObject = qTrue;
			}
		} else
			Com_Printf (PRINT_ALL, "...GL_ARB_vertex_buffer_object not found\n");
	} else
		Com_Printf (PRINT_ALL, "...ignoring GL_ARB_vertex_buffer_object\n");

	/*
	** GL_EXT_bgra
	*/
	if (gl_ext_bgra->integer) {
		if (R_ExtensionAvailable (glConfig.extsString, "GL_EXT_bgra")) {
				Com_Printf (PRINT_ALL, "...enabling GL_EXT_bgra\n");
				glConfig.extBGRA = qTrue;
		} else
			Com_Printf (PRINT_ALL, "...GL_EXT_bgra not found\n");
	} else
		Com_Printf (PRINT_ALL, "...ignoring GL_EXT_bgra\n");

	/*
	** GL_EXT_compiled_vertex_array
	** GL_SGI_compiled_vertex_array
	*/
	if (gl_ext_compiled_vertex_array->integer) {
		if (R_ExtensionAvailable (glConfig.extsString, "GL_EXT_compiled_vertex_array") || 
			R_ExtensionAvailable (glConfig.extsString, "GL_SGI_compiled_vertex_array")) {
			qglLockArraysEXT = (void *) QGL_GetProcAddress ("glLockArraysEXT");
			if (qglLockArraysEXT)	qglUnlockArraysEXT = (void *) QGL_GetProcAddress ("glUnlockArraysEXT");

			if (!qglUnlockArraysEXT) {
				Com_Printf (PRINT_ALL, "..." S_COLOR_RED "GL_EXT/SGI_compiled_vertex_array not properly supported!\n");
				qglLockArraysEXT	= NULL;
				qglUnlockArraysEXT	= NULL;
			} else {
				Com_Printf (PRINT_ALL, "...enabling GL_EXT/SGI_compiled_vertex_array\n");
				glConfig.extCompiledVertArray = qTrue;
			}
		} else
			Com_Printf (PRINT_ALL, "...GL_EXT/SGI_compiled_vertex_array not found\n");
	} else
		Com_Printf (PRINT_ALL, "...ignoring GL_EXT/SGI_compiled_vertex_array\n");

	/*
	** GL_EXT_draw_range_elements
	*/
	if (gl_ext_draw_range_elements->integer) {
		if (R_ExtensionAvailable (glConfig.extsString, "GL_EXT_draw_range_elements")) {
			qglGetIntegerv (GL_MAX_ELEMENTS_VERTICES_EXT, &glConfig.maxElementVerts);
			qglGetIntegerv (GL_MAX_ELEMENTS_INDICES_EXT, &glConfig.maxElementIndices);

			if (glConfig.maxElementVerts < 0)
				glConfig.maxElementVerts = 0;
			if (glConfig.maxElementIndices < 0)
				glConfig.maxElementIndices = 0;

			if (glConfig.maxElementIndices && glConfig.maxElementVerts)
				qglDrawRangeElementsEXT = (void *) QGL_GetProcAddress ("glDrawRangeElementsEXT");
			if (!qglDrawRangeElementsEXT)
				qglDrawRangeElementsEXT = (void *) QGL_GetProcAddress ("glDrawRangeElements");

			if (!qglDrawRangeElementsEXT) {
				Com_Printf (PRINT_ALL, "..." S_COLOR_RED "GL_EXT_draw_range_elements not properly supported!\n");
				qglDrawRangeElementsEXT = NULL;
				glConfig.maxElementIndices	= 0;
				glConfig.maxElementVerts		= 0;
			} else {
				Com_Printf (PRINT_ALL, "...enabling GL_EXT_draw_range_elements\n");
				glConfig.extDrawRangeElements = qTrue;
			}
		} else
			Com_Printf (PRINT_ALL, "...GL_EXT_draw_range_elements not found\n");
	} else
		Com_Printf (PRINT_ALL, "...ignoring GL_EXT_draw_range_elements\n");

	/*
	** GL_EXT_texture_edge_clamp
	*/
	if (gl_ext_texture_edge_clamp->integer) {
		if (R_ExtensionAvailable (glConfig.extsString, "GL_EXT_texture_edge_clamp")) {
				Com_Printf (PRINT_ALL, "...enabling GL_EXT_texture_edge_clamp\n");
				glConfig.extTexEdgeClamp = qTrue;
		} else
			Com_Printf (PRINT_ALL, "...GL_EXT_texture_edge_clamp not found\n");
	} else
		Com_Printf (PRINT_ALL, "...ignoring GL_EXT_texture_edge_clamp\n");

	/*
	** GL_EXT_texture_filter_anisotropic
	*/
	if (gl_ext_texture_filter_anisotropic->integer) {
		if (R_ExtensionAvailable (glConfig.extsString, "GL_EXT_texture_filter_anisotropic")) {
			qglGetIntegerv (GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxAniso);
			if (glConfig.maxAniso <= 0) {
				Com_Printf (PRINT_ALL, "..." S_COLOR_RED "GL_EXT_texture_filter_anisotropic not properly supported!\n");
				glConfig.maxAniso = 0;
			} else {
				Com_Printf (PRINT_ALL, "...enabling GL_EXT_texture_filter_anisotropic (%dx max)\n", glConfig.maxAniso);
				glConfig.extTexFilterAniso = qTrue;
			}
		} else
			Com_Printf (PRINT_ALL, "...GL_EXT_texture_filter_anisotropic not found\n");
	} else
		Com_Printf (PRINT_ALL, "...ignoring GL_EXT_texture_filter_anisotropic\n");

	/*
	** GL_NV_multisample_filter_hint
	*/
	if (R_ExtensionAvailable (glConfig.extsString, "GL_NV_multisample_filter_hint")) {
		Com_Printf (PRINT_ALL, "...enabling GL_NV_multisample_filter_hint\n");
		glConfig.extNVMulSampleHint = qTrue;
	} else
		Com_Printf (PRINT_ALL, "...GL_NV_multisample_filter_hint not found\n");

	/*
	** GL_NV_texture_shader
	*/
	if (gl_nv_texture_shader->integer) {
		if (R_ExtensionAvailable (glConfig.extsString, "GL_NV_texture_shader")) {
			Com_Printf (PRINT_ALL, "...enabling GL_NV_texture_shader\n");
			glConfig.extNVTexShader = qTrue;
		} else
			Com_Printf (PRINT_ALL, "...GL_NV_texture_shader not found\n");
	} else
		Com_Printf (PRINT_ALL, "...ignoring GL_NV_texture_shader\n");

	/*
	** GL_SGIS_generate_mipmap
	*/
	if (gl_sgis_generate_mipmap->integer) {
		if (R_ExtensionAvailable (glConfig.extsString, "GL_SGIS_generate_mipmap")) {
			Com_Printf (PRINT_ALL, "...enabling GL_SGIS_generate_mipmap\n");
			glConfig.extSGISGenMipmap = qTrue;
		} else
			Com_Printf (PRINT_ALL, "...GL_SGIS_generate_mipmap not found\n");
	} else
		Com_Printf (PRINT_ALL, "...ignoring GL_SGIS_generate_mipmap\n");

#ifdef _WIN32
	/*
	** WGL_EXT_swap_control
	** WGL Extension lookup will also load this, so check for it already being present
	*/
	if (gl_ext_swapinterval->integer) {
		if (R_ExtensionAvailable (glConfig.extsString, "WGL_EXT_swap_control")) {
			if (!glConfig.extWinSwapInterval) {
				qwglSwapIntervalEXT = (BOOL (WINAPI *)(int)) QGL_GetProcAddress ("wglSwapIntervalEXT");

				if (qwglSwapIntervalEXT) {
					Com_Printf (PRINT_ALL, "...enabling WGL_EXT_swap_control\n");
					glConfig.extWinSwapInterval = qTrue;
				} else
					Com_Printf (PRINT_ALL, "..." S_COLOR_RED "WGL_EXT_swap_control not properly supported!\n");
			}
		} else
			Com_Printf (PRINT_ALL, "...WGL_EXT_swap_control not found\n");
	} else
		Com_Printf (PRINT_ALL, "...ignoring WGL_EXT_swap_control\n");
#endif // _WIN32
}


/*
==================
R_Register

Registers the renderer's cvars/commands and gets
the latched ones during a vid_restart
==================
*/
static void R_Register (void) {
	Cvar_GetLatchedVars (CVAR_LATCHVIDEO);

	con_font			= Cvar_Get ("con_font",				"conchars",		CVAR_ARCHIVE);

	e_test_0			= Cvar_Get ("e_test_0",				"0",			0);
	e_test_1			= Cvar_Get ("e_test_1",				"0",			0);

	gl_3dlabs_broken	= Cvar_Get ("gl_3dlabs_broken",		"1",			CVAR_ARCHIVE);
	gl_bitdepth			= Cvar_Get ("gl_bitdepth",			"0",			CVAR_LATCHVIDEO);
	gl_clear			= Cvar_Get ("gl_clear",				"0",			0);
	gl_cull				= Cvar_Get ("gl_cull",				"1",			0);
	gl_drawbuffer		= Cvar_Get ("gl_drawbuffer",		"GL_BACK",		0);
	gl_driver			= Cvar_Get ("gl_driver",			GL_DRIVERNAME,	CVAR_ARCHIVE|CVAR_LATCHVIDEO);
	gl_dynamic			= Cvar_Get ("gl_dynamic",			"1",			0);

	gl_extensions					= Cvar_Get ("gl_extensions",				"1",		CVAR_ARCHIVE|CVAR_LATCHVIDEO);
	gl_arb_texture_compression		= Cvar_Get ("gl_arb_texture_compression",	"0",		CVAR_ARCHIVE|CVAR_LATCHVIDEO);
	gl_arb_texture_cube_map			= Cvar_Get ("gl_arb_texture_cube_map",		"1",		CVAR_ARCHIVE|CVAR_LATCHVIDEO);
	gl_arb_vertex_buffer_object		= Cvar_Get ("gl_arb_vertex_buffer_object",	"1",		CVAR_ARCHIVE|CVAR_LATCHVIDEO);
	gl_ext_bgra						= Cvar_Get ("gl_ext_bgra",					"1",		CVAR_ARCHIVE|CVAR_LATCHVIDEO);
	gl_ext_compiled_vertex_array	= Cvar_Get ("gl_ext_compiled_vertex_array",	"1",		CVAR_ARCHIVE|CVAR_LATCHVIDEO);
	gl_ext_draw_range_elements		= Cvar_Get ("gl_ext_draw_range_elements",	"1",		CVAR_ARCHIVE|CVAR_LATCHVIDEO);
	gl_ext_max_anisotropy			= Cvar_Get ("gl_ext_max_anisotropy",		"2",		CVAR_ARCHIVE);
	gl_ext_mtexcombine				= Cvar_Get ("gl_ext_mtexcombine",			"1",		CVAR_ARCHIVE|CVAR_LATCHVIDEO);
	gl_ext_swapinterval				= Cvar_Get ("gl_ext_swapinterval",			"1",		CVAR_ARCHIVE|CVAR_LATCHVIDEO);
	gl_ext_multitexture				= Cvar_Get ("gl_ext_multitexture",			"1",		CVAR_ARCHIVE|CVAR_LATCHVIDEO);
	gl_ext_texture_filter_anisotropic =	Cvar_Get ("gl_ext_texture_filter_anisotropic","0",	CVAR_ARCHIVE|CVAR_LATCHVIDEO);
	gl_ext_texture_env_add			= Cvar_Get ("gl_ext_texture_env_add",		"1",		CVAR_ARCHIVE|CVAR_LATCHVIDEO);
	gl_ext_texture_edge_clamp		= Cvar_Get ("gl_ext_texture_edge_clamp",	"1",		CVAR_ARCHIVE|CVAR_LATCHVIDEO);
	gl_nv_multisample_filter_hint	= Cvar_Get ("gl_nv_multisample_filter_hint","fastest",	CVAR_ARCHIVE);
	gl_nv_texture_shader			= Cvar_Get ("gl_nv_texture_shader",			"1",		CVAR_ARCHIVE);
	gl_sgis_generate_mipmap			= Cvar_Get ("gl_sgis_generate_mipmap",		"1",		CVAR_ARCHIVE|CVAR_LATCHVIDEO);

	gl_ext_max_anisotropy->modified	= qFalse; // so that setting the anisotropy texparam is not done twice on init

	gl_finish			= Cvar_Get ("gl_finish",			"0",			CVAR_ARCHIVE);
	gl_flashblend		= Cvar_Get ("gl_flashblend",		"0",			CVAR_ARCHIVE);
	gl_lightmap			= Cvar_Get ("gl_lightmap",			"0",			0);
	gl_lockpvs			= Cvar_Get ("gl_lockpvs",			"0",			0);
	gl_log				= Cvar_Get ("gl_log",				"0",			0);
	gl_mode				= Cvar_Get ("gl_mode",				"3",			CVAR_ARCHIVE|CVAR_LATCHVIDEO);
	gl_modulate			= Cvar_Get ("gl_modulate",			"1",			CVAR_ARCHIVE);

	gl_polyblend		= Cvar_Get ("gl_polyblend",			"1",			0);
	gl_saturatelighting	= Cvar_Get ("gl_saturatelighting",	"0",			0);
	gl_shadows			= Cvar_Get ("gl_shadows",			"0",			CVAR_ARCHIVE);
	gl_showtris			= Cvar_Get ("gl_showtris",			"0",			0);
	gl_swapinterval		= Cvar_Get ("gl_swapinterval",		"1",			CVAR_ARCHIVE);

	part_shade			= Cvar_Get ("part_shade",			"1",			CVAR_ARCHIVE);

	r_advir				= Cvar_Get ("r_advir",				"1",			CVAR_ARCHIVE);
	r_caustics			= Cvar_Get ("r_caustics",			"1",			CVAR_ARCHIVE);
	r_displayfreq		= Cvar_Get ("r_displayfreq",		"0",			CVAR_ARCHIVE|CVAR_LATCHVIDEO);
	r_drawentities		= Cvar_Get ("r_drawentities",		"1",			0);
	r_drawworld			= Cvar_Get ("r_drawworld",			"1",			0);

	r_fog				= Cvar_Get ("r_fog",				"0",			CVAR_ARCHIVE);
	r_fog_start			= Cvar_Get ("r_fog_start",			"300",			CVAR_ARCHIVE);
	r_fog_density		= Cvar_Get ("r_fog_density",		"0.0005",		CVAR_ARCHIVE);
	r_fog_end			= Cvar_Get ("r_fog_end",			"3000",			CVAR_ARCHIVE);
	r_fog_red			= Cvar_Get ("r_fog_red",			"1",			CVAR_ARCHIVE);
	r_fog_green			= Cvar_Get ("r_fog_green",			"1",			CVAR_ARCHIVE);
	r_fog_blue			= Cvar_Get ("r_fog_blue",			"1",			CVAR_ARCHIVE);
	r_fog_water			= Cvar_Get ("r_fog_water",			"1",			CVAR_ARCHIVE);

	r_fontscale			= Cvar_Get ("r_fontscale",			"1",			CVAR_ARCHIVE);
	r_fullbright		= Cvar_Get ("r_fullbright",			"0",			0);
	r_hudscale			= Cvar_Get ("r_hudscale",			"1",			CVAR_ARCHIVE);
	r_lerpmodels		= Cvar_Get ("r_lerpmodels",			"1",			0);
	r_lightlevel		= Cvar_Get ("r_lightlevel",			"0",			0);
	r_nocull			= Cvar_Get ("r_nocull",				"0",			0);
	r_norefresh			= Cvar_Get ("r_norefresh",			"0",			0);
	r_novis				= Cvar_Get ("r_novis",				"0",			0);
	r_overbrightbits	= Cvar_Get ("r_overbrightbits",		"0",			CVAR_ARCHIVE);
	r_speeds			= Cvar_Get ("r_speeds",				"0",			0);

	// add the various commands
	Cmd_AddCommand ("gfxinfo",		GL_GfxInfo_f);
	Cmd_AddCommand ("egl_renderer",	GL_RendererMsg_f);
	Cmd_AddCommand ("egl_version",	GL_VersionMsg_f);
}


/*
===============
R_Unregister
===============
*/
static void R_Unregister (void) {
	Cmd_RemoveCommand ("gfxinfo");
	Cmd_RemoveCommand ("egl_version");
	Cmd_RemoveCommand ("egl_renderer");
}


/*
===============
R_Init
===============
*/
qBool R_Init (void *hInstance, void *hWnd) {
	char	renderbuf[2048];
	char	vendorbuf[2048];
	int		refInitTime;

	refInitTime = Sys_Milliseconds ();
	Com_Printf (PRINT_ALL, "\n-------- Refresh Initialization --------\n");

	r_FirstTime = qTrue;

	// register renderer cvars
	R_Register ();

	// set defaults
	glConfig.extArbMultitexture = qFalse;
	glConfig.extArbTexCompression = qFalse;
	glConfig.extArbTexCubeMap = qFalse;
	glConfig.extArbVertBufferObject = qFalse;
	glConfig.extBGRA = qFalse;
	glConfig.extCompiledVertArray = qFalse;
	glConfig.extDrawRangeElements = qFalse;
	glConfig.extMTexCombine = qFalse;
	glConfig.extMultiSample = qFalse;
	glConfig.extNVMulSampleHint = qFalse;
	glConfig.extNVTexShader = qFalse;
	glConfig.extSGISGenMipmap = qFalse;
	glConfig.extSGISMultiTexture = qFalse;
	glConfig.extTexEdgeClamp = qFalse;
	glConfig.extTexEnvAdd = qFalse;
	glConfig.extTexFilterAniso = qFalse;
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

	// set our "safe" mode
	glConfig.safeMode = 3;

	// initialize our QGL dynamic bindings
	if (!QGL_Init (gl_driver->string)) {
		QGL_Shutdown ();
		Com_Printf (PRINT_ALL, "..." S_COLOR_RED "could not load \"%s\"\n", gl_driver->string);
		return qFalse;
	}

	// initialize OS-specific parts of OpenGL
	if (!GLimp_Init (hInstance, hWnd)) {
		Com_Printf (PRINT_ALL, "..." S_COLOR_RED "unable to init gl implimentation\n");
		QGL_Shutdown ();
		return qFalse;
	}

	// create the window and set up the context
	if (!R_SetMode ()) {
		Com_Printf (PRINT_ALL, "..." S_COLOR_RED "could not set video mode\n");
		QGL_Shutdown ();
		return qFalse;
	}

	// check stencil buffer availability and usability
	Com_Printf (PRINT_ALL, "...stencil buffer ");
	if (gl_stencilbuffer->integer && (glConfig.cStencilBits > 0)) {
		if (glConfig.rendType & GLREND_VOODOO)
			Com_Printf (PRINT_ALL, "ignored\n");
		else {
			Com_Printf (PRINT_ALL, "available\n");
			glConfig.useStencil = qTrue;
		}
	} else
		Com_Printf (PRINT_ALL, "disabled\n");

	// get our various GL strings
	glConfig.vendString = qglGetString (GL_VENDOR);
	Com_Printf (PRINT_ALL, "GL_VENDOR: %s\n", glConfig.vendString);
	Q_strncpyz (vendorbuf, glConfig.vendString, sizeof (vendorbuf));
	Q_strlwr (vendorbuf);

	glConfig.rendString = qglGetString (GL_RENDERER);
	Com_Printf (PRINT_ALL, "GL_RENDERER: %s\n", glConfig.rendString);
	Q_strncpyz (renderbuf, glConfig.rendString, sizeof (renderbuf));
	Q_strlwr (renderbuf);

	glConfig.versString = qglGetString (GL_VERSION);
	Com_Printf (PRINT_ALL, "GL_VERSION: %s\n", glConfig.versString);

	glConfig.extsString = qglGetString (GL_EXTENSIONS);

	// find out the renderer model
	if (strstr	(vendorbuf, "nvidia")) {
		glConfig.rendType = GLREND_NVIDIA;
		if (strstr	(renderbuf, "geforce"))		glConfig.rendType |= GLREND_GEFORCE;
	} else if (strstr	(vendorbuf, "sgi"))				glConfig.rendType = GLREND_SGI;
	else if (strstr	(renderbuf, "permedia"))			glConfig.rendType = GLREND_PERMEDIA2;
	else if (strstr	(renderbuf, "glint"))				glConfig.rendType = GLREND_GLINT_MX;
	else if (strstr	(renderbuf, "glzicd"))				glConfig.rendType = GLREND_REALIZM;
	else if (strstr	(renderbuf, "pcx1"))				glConfig.rendType = GLREND_PCX1;
	else if (strstr	(renderbuf, "pcx2"))				glConfig.rendType = GLREND_PCX2;
	else if (strstr	(renderbuf, "pmx"))					glConfig.rendType = GLREND_PMX;
	else if (strstr	(renderbuf, "verite"))				glConfig.rendType = GLREND_RENDITION;
	else if (strstr	(vendorbuf, "sis"))					glConfig.rendType = GLREND_SIS;
	else if (strstr (renderbuf, "voodoo"))				glConfig.rendType = GLREND_VOODOO;
	else if (strstr (vendorbuf, "ati")) {
		glConfig.rendType = GLREND_ATI;
		if (strstr	(vendorbuf, "radeon"))		glConfig.rendType |= GLREND_RADEON;
	} else {
		if (strstr	(renderbuf, "gdi generic")) {
			// MCD has buffering issues
			glConfig.rendType = GLREND_MCD;
			Cvar_ForceSetValue ("gl_finish", 1);
		} else
			glConfig.rendType = GLREND_DEFAULT;
	}

#ifdef GL_FORCEFINISH
	Cvar_ForceSetValue ("gl_finish", 1);
#endif

	if (glConfig.rendType & GLREND_3DLABS) {
		if (gl_3dlabs_broken->integer)
			glConfig.allowCDS = qFalse;
		else
			glConfig.allowCDS = qTrue;
	} else
		glConfig.allowCDS = qTrue;

	// notify if CDS is allowed
	if (!glConfig.allowCDS)
		Com_Printf (PRINT_ALL, "...disabling CDS\n");

	// grab opengl extensions
	if (gl_extensions->integer)
		GL_InitExtensions ();
	else
		Com_Printf (PRINT_ALL, "...ignoring OpenGL extensions\n");

	// check cvar settings
	R_UpdateCvars ();

	// set the default state
	GL_SetDefaultState ();

	// retreive generic information
	qglGetIntegerv (GL_MAX_TEXTURE_SIZE, &glConfig.maxTexSize);
	Q_NearestPow (&glConfig.maxTexSize, qTrue);
	if (glConfig.maxTexSize < 256)
		glConfig.maxTexSize = 256;

	Com_Printf (PRINT_ALL, "----------------------------------------\n");

	// sub-system init
	RS_Init ();
	Img_Init ();
	R_InitMedia ();
	Mod_Init ();
	Sky_Init ();
	RB_Init ();

	Com_Printf (PRINT_ALL, "init time: %dms\n", Sys_Milliseconds () - refInitTime);
	Com_Printf (PRINT_ALL, "----------------------------------------\n");

	// check for gl errors
	GL_CheckForError ();

	r_FirstTime = qFalse;

	return qTrue;
}


/*
===============
R_Shutdown
===============
*/
void R_Shutdown (void) {
	R_Unregister ();

	// shutdown subsystems
	RS_Shutdown ();
	Img_Shutdown ();
	Mod_Shutdown ();
	Sky_Shutdown ();
	RB_Shutdown ();

	// shut down OS specific OpenGL stuff like contexts, etc
	GLimp_Shutdown ();

	// shutdown our QGL subsystem
	QGL_Shutdown ();
}
