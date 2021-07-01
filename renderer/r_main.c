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
glStatic_t	glStatic;

refDef_t	r_refDef;
refRegist_t	r_refRegInfo;
refScene_t	r_refScene;
refStats_t	r_refStats;

/*
=============================================================================

	SCENE

=============================================================================
*/

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
		if (!glState.cameraSeparation || !glConfig.stereoEnabled) {
			if (!Q_stricmp (gl_drawbuffer->string, "GL_FRONT"))
				qglDrawBuffer (GL_FRONT);
			else
				qglDrawBuffer (GL_BACK);
		}
	}

	// Update the swap interval
	if (r_swapInterval->modified) {
		r_swapInterval->modified = qFalse;

#ifdef WIN32
		if (!glConfig.stereoEnabled && glConfig.extWinSwapInterval)
			qwglSwapIntervalEXT (r_swapInterval->integer);
#endif
	}

	// Texturemode stuff
	if (gl_texturemode->modified)
		GL_TextureMode (qFalse, qFalse);

	// Update anisotropy
	if (r_ext_maxAnisotropy->modified)
		GL_ResetAnisotropy ();

	// Update font
	if (con_font->modified)
		R_CheckFont ();

	// Gamma ramp
	if (glConfig.hwGammaInUse && vid_gamma->modified)
		R_UpdateGammaRamp ();
}


/*
==================
R_SetupGL2D
==================
*/
static void R_SetupGL2D (void)
{
	glState.in2D = qTrue;

	// Set 2D virtual screen size
	qglViewport (0, 0, glConfig.vidWidth, glConfig.vidHeight);
	qglScissor (0, 0, glConfig.vidWidth, glConfig.vidHeight);

	qglMatrixMode (GL_PROJECTION);
	qglLoadIdentity ();
	qglOrtho (0, glConfig.vidWidth, glConfig.vidHeight, 0, -99999, 99999);

	qglMatrixMode (GL_MODELVIEW);
	qglLoadIdentity ();
}


/*
=============
R_SetupProjectionMatrix
=============
*/
static inline float R_CalcZFar (void)
{
	float	forwardDist, viewDist;
	float	forwardClip;
	float	minimum;
	int		i;

	if (r_refScene.worldModel->type == MODEL_Q3BSP) {
		// FIXME: adjust for skydome height * 2!
		minimum = 0;
	}
	else {
		// FIXME: make this dependant on if there's a skybox or not
		minimum = SKY_BOXSIZE * 2;
	}
	if (minimum < 256)
		minimum = 256;

	forwardDist = DotProduct (r_refDef.viewOrigin, r_refDef.forwardVec);
	forwardClip = minimum + forwardDist;

	viewDist = 0;
	for (i=0 ; i<3 ; i++) {
		if (r_refDef.forwardVec[i] < 0)
			viewDist += r_refScene.worldModel->bspModel.nodes[0].mins[i] * r_refDef.forwardVec[i];
		else
			viewDist += r_refScene.worldModel->bspModel.nodes[0].maxs[i] * r_refDef.forwardVec[i];
	}
	if (viewDist > forwardClip)
		forwardClip = viewDist;

	// Bias by 256 pixels
	return max (forwardClip - forwardDist + 256, r_refScene.zFar);
}
static void R_SetupProjectionMatrix (refDef_t *rd, mat4x4_t m)
{
	GLfloat		xMin, xMax;
	GLfloat		yMin, yMax;
	GLfloat		zNear, zFar;
	GLfloat		vAspect = (float)rd->width/(float)rd->height;

	// Near/far clip
	if (r_refDef.rdFlags & RDF_NOWORLDMODEL)
		r_refScene.zFar = 2048;
	else
		r_refScene.zFar = R_CalcZFar ();

	zNear = 4;
	zFar = r_refScene.zFar;

	// Calculate aspect
	yMax = zNear * (float)tan ((rd->fovY * M_PI) / 360.0f);
	yMin = -yMax;

	xMin = yMin * vAspect;
	xMax = yMax * vAspect;

	xMin += -(2 * glState.cameraSeparation) / zNear;
	xMax += -(2 * glState.cameraSeparation) / zNear;

	// Apply to matrix
	m[0] = (2.0f * zNear) / (xMax - xMin);
	m[1] = 0.0f;
	m[2] = 0.0f;
	m[3] = 0.0f;
	m[4] = 0.0f;
	m[5] = (2.0f * zNear) / (yMax - yMin);
	m[6] = 0.0f;
	m[7] = 0.0f;
	m[8] = (xMax + xMin) / (xMax - xMin);
	m[9] = (yMax + yMin) / (yMax - yMin);
	m[10] = -(zFar + zNear) / (zFar - zNear);
	m[11] = -1.0f;
	m[12] = 0.0f;
	m[13] = 0.0f;
	m[14] = -(2.0f * zFar * zNear) / (zFar - zNear);
	m[15] = 0.0f;
}


/*
=============
R_SetupModelviewMatrix
=============
*/
static void R_SetupModelviewMatrix (refDef_t *rd, mat4x4_t m)
{
#if 0
	Matrix4_Identity (m);
	Matrix4_Rotate (m, -90, 1, 0, 0);
	Matrix4_Rotate (m,  90, 0, 0, 1);
#else
	Vector4Set (&m[0], 0, 0, -1, 0);
	Vector4Set (&m[4], -1, 0, 0, 0);
	Vector4Set (&m[8], 0, 1, 0, 0);
	Vector4Set (&m[12], 0, 0, 0, 1);
#endif

	Matrix4_Rotate (m, -rd->viewAngles[2], 1, 0, 0);
	Matrix4_Rotate (m, -rd->viewAngles[0], 0, 1, 0);
	Matrix4_Rotate (m, -rd->viewAngles[1], 0, 0, 1);
	Matrix4_Translate (m, -rd->viewOrigin[0], -rd->viewOrigin[1], -rd->viewOrigin[2]);
}


/*
=============
R_SetupGL3D
=============
*/
static void R_SetupGL3D (void)
{
	// Set up viewport
	if (!r_refScene.mirrorView && !r_refScene.portalView) {
		qglScissor (r_refDef.x, glConfig.vidHeight - r_refDef.height - r_refDef.y, r_refDef.width, r_refDef.height);
		qglViewport (r_refDef.x, glConfig.vidHeight - r_refDef.height - r_refDef.y, r_refDef.width, r_refDef.height);
		qglClear (GL_DEPTH_BUFFER_BIT);
	}

	// Set up projection matrix
	R_SetupProjectionMatrix (&r_refDef, r_refScene.projectionMatrix);
	if (r_refScene.mirrorView)
		r_refScene.projectionMatrix[0] = -r_refScene.projectionMatrix[0];

	qglMatrixMode (GL_PROJECTION);
	qglLoadMatrixd (r_refScene.projectionMatrix);

	// Set up the world view matrix
	R_SetupModelviewMatrix (&r_refDef, r_refScene.worldViewMatrix);

	qglMatrixMode (GL_MODELVIEW);
	qglLoadMatrixd (r_refScene.worldViewMatrix);

	// Handle portal/mirror rendering
	if (r_refScene.mirrorView || r_refScene.portalView) {
		GLdouble	clip[4];

		clip[0] = r_refScene.clipPlane.normal[0];
		clip[1] = r_refScene.clipPlane.normal[1];
		clip[2] = r_refScene.clipPlane.normal[2];
		clip[3] = -r_refScene.clipPlane.dist;

		qglClipPlane (GL_CLIP_PLANE0, clip);
		qglEnable (GL_CLIP_PLANE0);
	}
}


/*
================
R_Clear
================
*/
static void R_Clear (void)
{
	int			clearBits;

	clearBits = GL_DEPTH_BUFFER_BIT;
	if (gl_clear->integer) {
		qglClearColor (0.5f, 0.5f, 0.5f, 1.0f);
		clearBits |= GL_COLOR_BUFFER_BIT;
	}

	if (glStatic.useStencil && gl_shadows->integer) {
		qglClearStencil (128);
		clearBits |= GL_STENCIL_BUFFER_BIT;
	}

	qglClear (clearBits);

	qglDepthRange (0, 1);
}

// ==========================================================

/*
================
R_RenderToList

Adds scene items to the desired list
================
*/
void R_RenderToList (refDef_t *rd, meshList_t *list)
{
	int		i;

	r_refDef = *rd;
	r_currentList = list;

	for (i=0 ; i<MAX_MESH_KEYS ; i++)
		r_currentList->numMeshes[i] = 0;
	for (i=0 ; i<MAX_ADDITIVE_KEYS ; i++)
		r_currentList->numAdditiveMeshes[i] = 0;
	r_currentList->skyDrawn = qFalse;

	R_SetupGL3D ();
	R_SetupFrustum ();

	R_AddWorldToList ();
	R_AddPolysToList ();
	R_AddEntitiesToList ();
	R_SortMeshList ();
	R_DrawMeshList (qFalse);
	R_DrawMeshOutlines ();
	R_DrawNullModelList ();
	R_AddDLightsToList ();

	if (r_refScene.mirrorView || r_refScene.portalView)
		qglDisable (GL_CLIP_PLANE0);
}


/*
================
R_RenderScene
================
*/
void R_RenderScene (refDef_t *rd)
{
	glState.in2D = qFalse;
	if (r_noRefresh->integer)
		return;

	if (!r_refScene.worldModel->finishedLoading && !(rd->rdFlags & RDF_NOWORLDMODEL))
		Com_Error (ERR_DROP, "R_RenderScene: NULL worldmodel");

	r_refScene.zFar = 0;
	r_refScene.mirrorView = qFalse;
	r_refScene.portalView = qFalse;

	if (gl_finish->integer)
		qglFinish ();

	RB_BeginFrame ();

	R_RenderToList (rd, &r_worldList);
	R_SetLightLevel ();

#ifdef SHADOW_VOLUMES
	R_ShadowBlend ();
#endif

	RB_EndFrame ();

	R_SetupGL2D ();
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

	// Setup the frame for rendering
	GLimp_BeginFrame ();

	// Go into 2D mode
	R_SetupGL2D ();

	// Apply cvar settings
	R_UpdateCvars ();

	// Clear the scene if desired
	R_Clear ();
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
	R_SetupGL2D ();

	// Frame logging
	QGL_LogEndFrame ();

	// Rendering speeds
	if (r_speeds->integer || r_debugCulling->integer) {
		// General rendering information
		if (r_speeds->integer) {
			Com_Printf (0, "---------------------------------\n");
			Com_Printf (0, "%3i ent %3i aelem %4i apoly %4i poly %3i dlight\n",
				r_refScene.numEntities, r_refStats.aliasElements, r_refStats.aliasPolys,
				r_refScene.numPolys, r_refScene.numDLights);

			Com_Printf (0, "%4i tex %.2f mtexel %3i unit %3i env %4i binds\n",
				r_refStats.texturesInUse, r_refStats.texelsInUse/1000000.0f, r_refStats.textureUnitChanges, r_refStats.textureEnvChanges, r_refStats.textureBinds);

			if (r_refScene.worldModel && !(r_refDef.rdFlags & RDF_NOWORLDMODEL)) {
				Com_Printf (0, "%4i wpoly %4i welem %6.f zfar\n",
				r_refStats.worldPolys, r_refStats.worldElements, r_refScene.zFar);
			}

			Com_Printf (0, "%4i vert %4i tris %4i elem %3i mesh %3i pass\n",
				r_refStats.numVerts, r_refStats.numTris, r_refStats.numElements,
				r_refStats.meshCount, r_refStats.meshPasses);
		}

		// Cull information
		if (r_debugCulling->integer && !r_noCull->integer) {
			Com_Printf (0, "bounds[%3i/%3i] planar[%3i/%3i] radii[%3i/%3i] vis[%3i/%3i]\n",
				r_refStats.cullBounds[qTrue], r_refStats.cullBounds[qFalse],
				r_refStats.cullPlanar[qTrue], r_refStats.cullPlanar[qFalse],
				r_refStats.cullRadius[qTrue], r_refStats.cullRadius[qFalse],
				r_refStats.cullVis[qTrue], r_refStats.cullVis[qFalse]);
		}

		memset (&r_refStats, 0, sizeof (r_refStats));
	}

	// Next frame
	r_refScene.frameCount++;
}

// ==========================================================

/*
====================
R_ClearScene
====================
*/
void R_ClearScene (void)
{
	r_refScene.numDLights = 0;
	r_refScene.numEntities = 0;
	r_refScene.numPolys = 0;
}


/*
=====================
R_AddEntity
=====================
*/
void R_AddEntity (entity_t *ent)
{
	if (r_refScene.numEntities >= MAX_ENTITIES)
		return;
	if (ent->color[3] <= 0)
		return;

	r_refScene.entityList[r_refScene.numEntities] = *ent;
	if (ent->color[3] < 255)
		r_refScene.entityList[r_refScene.numEntities].flags |= RF_TRANSLUCENT;

	r_refScene.numEntities++;
}


/*
=====================
R_AddPoly
=====================
*/
void R_AddPoly (poly_t *poly)
{
	if (r_refScene.numPolys >= MAX_POLYS)
		return;

	r_refScene.polyList[r_refScene.numPolys] = *poly;
	if (!r_refScene.polyList[r_refScene.numPolys].shader)
		r_refScene.polyList[r_refScene.numPolys].shader = r_noShader;

	r_refScene.numPolys++;
}


/*
=====================
R_AddLight
=====================
*/
void R_AddLight (vec3_t origin, float intensity, float r, float g, float b)
{
	dLight_t	*dl;

	if (r_refScene.numDLights >= MAX_DLIGHTS)
		return;

	if (!intensity)
		return;

	dl = &r_refScene.dLightList[r_refScene.numDLights++];

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

	ls = &r_refScene.lightStyles[style];

	ls->white = r+g+b;
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
	case GL_INVALID_ENUM:		return "INVALID ENUM";
	case GL_INVALID_OPERATION:	return "INVALID OPERATION";
	case GL_INVALID_VALUE:		return "INVALID VALUE";
	case GL_NO_ERROR:			return "NO ERROR";
	case GL_OUT_OF_MEMORY:		return "OUT OF MEMORY";
	case GL_STACK_OVERFLOW:		return "STACK OVERFLOW";
	case GL_STACK_UNDERFLOW:	return "STACK UNDERFLOW";
	}

	return "unknown";
}
void GL_CheckForError (char *where)
{
	GLenum		error;

	error = qglGetError ();
	if (error != GL_NO_ERROR) {
		Com_Printf (PRNT_ERROR, "GL_ERROR: glGetError (): '%s' (0x%x)", GetGLErrorString (error), error);
		if (where)
			Com_Printf (0, " %s\n", where);
		else
			Com_Printf (0, "\n");
	}
}


/*
=============
R_GetGLConfig
=============
*/
void R_GetGLConfig (glConfig_t *outConfig)
{
	*outConfig = glConfig;
}


/*
=============
R_TransformToScreen_Vec3
=============
*/
void R_TransformToScreen_Vec3 (vec3_t in, vec3_t out)
{
	vec4_t temp, temp2;

	temp[0] = in[0];
	temp[1] = in[1];
	temp[2] = in[2];
	temp[3] = 1.0f;
	Matrix4_Multiply_Vector (r_refScene.worldViewMatrix, temp, temp2);
	Matrix4_Multiply_Vector (r_refScene.projectionMatrix, temp2, temp);

	if (!temp[3])
		return;
	out[0] = r_refDef.x + (temp[0] / temp[3] + 1.0f) * r_refDef.width * 0.5f;
	out[1] = r_refDef.y + (temp[1] / temp[3] + 1.0f) * r_refDef.height * 0.5f;
	out[2] = (temp[2] / temp[3] + 1.0f) * 0.5f;
}


/*
=============
R_TransformVectorToScreen
=============
*/
void R_TransformVectorToScreen (refDef_t *rd, vec3_t in, vec2_t out)
{
	mat4x4_t	p, m;
	vec4_t		temp, temp2;

	if (!rd || !in || !out)
		return;

	temp[0] = in[0];
	temp[1] = in[1];
	temp[2] = in[2];
	temp[3] = 1.0f;

	R_SetupProjectionMatrix (rd, p);
	R_SetupModelviewMatrix (rd, m);

	Matrix4_Multiply_Vector (m, temp, temp2);
	Matrix4_Multiply_Vector (p, temp2, temp);

	if (!temp[3])
		return;
	out[0] = rd->x + (temp[0] / temp[3] + 1.0f) * rd->width * 0.5f;
	out[1] = rd->y + (temp[1] / temp[3] + 1.0f) * rd->height * 0.5f;
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
	// Clear the scene so that old scene object pointers are cleared
	R_ClearScene ();

	// Clear old registration values
	r_refRegInfo.modelsReleased = 0;
	r_refRegInfo.modelsSearched = 0;
	r_refRegInfo.modelsTouched = 0;
	r_refRegInfo.shadersReleased = 0;
	r_refRegInfo.shadersSearched = 0;
	r_refRegInfo.shadersTouched = 0;
	r_refRegInfo.fragProgsReleased = 0;
	r_refRegInfo.vertProgsReleased = 0;
	r_refRegInfo.imagesReleased = 0;
	r_refRegInfo.imagesResampled = 0;
	r_refRegInfo.imagesSearched = 0;
	r_refRegInfo.imagesTouched = 0;

	// Begin sub-system registration
	r_refRegInfo.inSequence = qTrue;
	r_refRegInfo.registerFrame++;

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
	R_EndShaderRegistration ();	// Register first so programs and images are touched
	R_EndImageRegistration ();
	R_EndProgramRegistration ();

	r_refRegInfo.inSequence = qFalse;

	// Print registration info
	Com_Printf (0, "Registration sequence completed...\n");
	Com_Printf (0, "Models   rel/touch/search: %i/%i/%i\n", r_refRegInfo.modelsReleased, r_refRegInfo.modelsTouched, r_refRegInfo.modelsSearched);
	Com_Printf (0, "Shaders  rel/touch/search: %i/%i/%i\n", r_refRegInfo.shadersReleased, r_refRegInfo.shadersTouched, r_refRegInfo.shadersSearched);
	Com_Printf (0, "Programs released fragment/vertex: %i/%i\n", r_refRegInfo.fragProgsReleased, r_refRegInfo.vertProgsReleased);
	Com_Printf (0, "Images   rel/resamp/search/touch: %i/%i/%i/%i\n", r_refRegInfo.imagesReleased, r_refRegInfo.imagesResampled, r_refRegInfo.imagesSearched, r_refRegInfo.imagesTouched);
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
