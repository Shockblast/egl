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

#include "rf_local.h"

refInfo_t	ri;

/*
==============================================================================

	POLYGON BACKEND

	Generic meshes can be passed from CGAME to be rendered here
==============================================================================
*/

static mesh_t		r_polyMesh;

/*
================
R_AddPolysToList
================
*/
static void R_AddPolysToList (void)
{
	refPoly_t	*p;
	mQ3BspFog_t	*fog;
	uint32		i;

	if (!r_drawPolys->intVal)
		return;

	// Add poly meshes to list
	for (i=0 ; i<ri.scn.numPolys ; i++) {
		p = ri.scn.polyList[i];

		// Find fog
		fog = R_FogForSphere (p->origin, p->radius);

		// Add to the list
		R_AddMeshToList (p->mat, p->matTime, NULL, fog, MBT_POLY, p);
	}
}


/*
================
R_PushPoly
================
*/
void R_PushPoly (meshBuffer_t *mb, meshFeatures_t features)
{
	refPoly_t	*p;

	p = (refPoly_t *)mb->mesh;
	if (p->numVerts > RB_MAX_VERTS)
		return;

	r_polyMesh.numIndexes = (p->numVerts - 2) * 3;
	r_polyMesh.numVerts = p->numVerts;

	r_polyMesh.colorArray = p->colors;
	r_polyMesh.coordArray = p->texCoords;
	r_polyMesh.vertexArray = p->vertices;

	RB_PushMesh (&r_polyMesh, features);
}


/*
================
R_PolyOverflow
================
*/
qBool R_PolyOverflow (meshBuffer_t *mb)
{
	refPoly_t	*p;

	p = (refPoly_t *)mb->mesh;
	return RB_BackendOverflow (p->numVerts, (p->numVerts - 2) * 3);
}


/*
================
R_PolyInit
================
*/
void R_PolyInit (void)
{
	r_polyMesh.indexArray = NULL;
	r_polyMesh.lmCoordArray = NULL;
	r_polyMesh.normalsArray = NULL;
	r_polyMesh.sVectorsArray = NULL;
	r_polyMesh.tVectorsArray = NULL;
	r_polyMesh.trNeighborsArray = NULL;
	r_polyMesh.trNormalsArray = NULL;
}

/*
=============================================================================

	ENTITIES

=============================================================================
*/

/*
=============
R_AddEntitiesToList
=============
*/
static void R_AddEntitiesToList (void)
{
	refEntity_t	*ent;
	uint32		i;

	if (!r_drawEntities->intVal)
		return;

	// Add all entities to the list
	for (i=ENTLIST_OFFSET, ent=&ri.scn.entityList[ENTLIST_OFFSET] ; i<ri.scn.numEntities ; ent++, i++) {
		if (!ent->model) {
			RB_AddNullModelToList (ent);
			continue;
		}

		if (ri.scn.mirrorView) {
			if (ent->flags & RF_WEAPONMODEL)
				continue;
		}

		switch (ent->model->type) {
		case MODEL_MD2:
		case MODEL_MD3:
			R_AddAliasModelToList (ent);
			break;

		case MODEL_Q2BSP:
			R_AddQ2BrushModel (ent);
			break;

		case MODEL_Q3BSP:
			R_AddQ3BrushModel (ent);
			break;

		case MODEL_SP2:
			R_AddSP2ModelToList (ent);
			break;

		case MODEL_BAD:
		default:
			RB_AddNullModelToList (ent);
			break;
		}
	}
}


/*
=============
R_EntityInit
=============
*/
void R_EntityInit (void)
{
	// Reserve a spot for the default entity
	ri.scn.defaultEntity = &ri.scn.entityList[0];

	memset (ri.scn.defaultEntity, 0, sizeof (refEntity_t));
	ri.scn.defaultEntity->model = ri.scn.defaultModel;
	ri.scn.defaultEntity->scale = 1.0f;
	Matrix3_Identity (ri.scn.defaultEntity->axis);
	Vec4Set (ri.scn.defaultEntity->color, 255, 255, 255, 255);

	// And for the world entity
	ri.scn.worldEntity = &ri.scn.entityList[1];
	memcpy (ri.scn.worldEntity, ri.scn.defaultEntity, sizeof (refEntity_t));
}

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
		if (!ri.cameraSeparation || !ri.config.stereoEnabled) {
			if (!Q_stricmp (gl_drawbuffer->string, "GL_FRONT"))
				qglDrawBuffer (GL_FRONT);
			else
				qglDrawBuffer (GL_BACK);
		}
	}

	// Texturemode stuff
	if (gl_texturemode->modified)
		GL_TextureMode (qFalse, qFalse);

	// Update anisotropy
	if (r_ext_maxAnisotropy->modified)
		GL_ResetAnisotropy ();

	// Update font
	if (r_defaultFont->modified)
		R_CheckFont ();
	if (r_fontScale->modified) {
		r_fontScale->modified = qFalse;
		if (r_fontScale->floatVal <= 0)
			Cvar_VariableSetValue (r_fontScale, 1, qTrue);
	}

	// Gamma ramp
	if (ri.config.hwGammaInUse && vid_gamma->modified)
		R_UpdateGammaRamp ();

	// Clamp zFar
	if (r_zFarAbs->modified) {
		r_zFarAbs->modified = qFalse;
		if (r_zFarAbs->intVal < 0)
			Cvar_VariableSetValue (r_zFarAbs, 0, qTrue);
	}
	if (r_zFarMin->modified) {
		r_zFarMin->modified = qFalse;
		if (r_zFarMin->intVal <= 0)
			Cvar_VariableSetValue (r_zFarMin, 1, qTrue);
	}

	// Clamp zNear
	if (r_zNear->modified) {
		r_zNear->modified = qFalse;
		if (r_zNear->floatVal < 0.1f)
			Cvar_VariableSetValue (r_zNear, 4, qTrue);
	}
}


/*
================
R_RenderToList

Adds scene items to the desired list
================
*/
void R_RenderToList (refDef_t *rd, meshList_t *list)
{
	uint32	startTime = 0;
	int		i;

	if (r_times->intVal)
		startTime = Sys_UMilliseconds ();

	ri.def = *rd;
	r_currentList = list;

	for (i=0 ; i<MAX_MESH_KEYS ; i++)
		r_currentList->numMeshes[i] = 0;
	for (i=0 ; i<MAX_ADDITIVE_KEYS ; i++)
		r_currentList->numAdditiveMeshes[i] = 0;
	r_currentList->skyDrawn = qFalse;

	RB_SetupGL3D ();
	R_SetupFrustum ();

	R_AddSkyToList ();
	R_AddWorldToList ();
	R_AddDecalsToList ();
	R_AddPolysToList ();
	R_AddEntitiesToList ();
	R_SortMeshList ();
	R_DrawMeshList (qFalse);
	R_DrawMeshOutlines ();
	RB_DrawNullModelList ();
	RB_DrawDLights ();

	if (ri.scn.mirrorView || ri.scn.portalView)
		qglDisable (GL_CLIP_PLANE0);

	if (r_times->intVal)
		ri.pc.timeAddToList += Sys_UMilliseconds () - startTime;
}


/*
================
R_RenderScene
================
*/
void R_RenderScene (refDef_t *rd)
{
	if (r_noRefresh->intVal)
		return;

	if (!ri.scn.worldModel->touchFrame && !(rd->rdFlags & RDF_NOWORLDMODEL))
		Com_Error (ERR_DROP, "R_RenderScene: NULL worldmodel");

	ri.scn.zFar = 0;
	ri.scn.mirrorView = qFalse;
	ri.scn.portalView = qFalse;

	if (gl_finish->intVal)
		qglFinish ();

	R_RenderToList (rd, &r_worldList);
	R_SetLightLevel ();

#ifdef SHADOW_VOLUMES
	RB_ShadowBlend ();
#endif

	RB_SetupGL2D ();
}


/*
==================
R_BeginFrame
==================
*/
void R_BeginFrame (float cameraSeparation)
{
	ri.cameraSeparation = cameraSeparation;

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
	RB_SetupGL2D ();

	// Apply cvar settings
	R_UpdateCvars ();

	// Clear the scene if desired
	RB_ClearBuffers ();

	// Update the backend
	RB_BeginFrame ();
}


/*
==================
R_EndFrame
==================
*/
void R_EndFrame (void)
{
	// Update the backend
	RB_EndFrame ();

	// Swap buffers
	GLimp_EndFrame ();

	// Go into 2D mode
	RB_SetupGL2D ();

	// Frame logging
	QGL_LogEndFrame ();

	// Rendering speeds
	if (r_speeds->intVal || r_times->intVal || r_debugBatching->intVal || r_debugCulling->intVal) {
		// FIXME: Check ->modified and set to false, if it's modified to turn on then clear ri.pc and skip the first time for accuracy
		// General rendering information
		if (r_speeds->intVal) {
			Com_Printf (0, "\n");
			Com_Printf (0, "%3u ent %3u aelem %4u apoly %4u poly %3u dlight\n",
				ri.scn.numEntities-ENTLIST_OFFSET, ri.pc.aliasElements, ri.pc.aliasPolys,
				ri.scn.numPolys, ri.scn.numDLights);

			Com_Printf (0, "%.2f mtexel %3u unit %3u envchg %4u binds (%4u unique)\n",
				ri.pc.texelsInUse/1000000.0f, ri.pc.textureUnitChanges,
				ri.pc.textureEnvChanges, ri.pc.textureBinds, ri.pc.texturesInUse);

			if (ri.scn.worldModel->touchFrame && !(ri.def.rdFlags & RDF_NOWORLDMODEL)) {
				Com_Printf (0, "%4u wpoly %4u welem %4u decal %6.f zfar\n",
					ri.pc.worldPolys, ri.pc.worldElements, ri.scn.drawnDecals, ri.scn.zFar);
			}

			Com_Printf (0, "%5u vert %5u tris %4u elem %4u mesh %4u pass %3u gls\n",
				ri.pc.numVerts, ri.pc.numTris, ri.pc.numElements,
				ri.pc.meshCount, ri.pc.meshPasses,
				ri.pc.stateChanges);
		}

		// Time to process things
		if (r_times->intVal) {
			Com_Printf (0, "\n");
			Com_Printf (0, "%3u add %3u sort %3u draw\n",
				ri.pc.timeAddToList, ri.pc.timeSortList, ri.pc.timeDrawList);

			if (ri.scn.worldModel->touchFrame && !(ri.def.rdFlags & RDF_NOWORLDMODEL)) {
				Com_Printf (0, "%3u marklv %3u marklt %3u recurs\n",
					ri.pc.timeMarkLeaves, ri.pc.timeMarkLights, ri.pc.timeRecurseWorld);
			}
		}

		// Batch information
		if (r_debugBatching->intVal) {
			Com_Printf (0, "\n");
			Com_Printf (0, "%4i batch %4i flush\n",
				ri.pc.meshBatches, ri.pc.meshBatchFlush);

			if (ri.pc.meshBatches && ri.pc.meshBatchFlush)
				Com_Printf (0, "%5.2f efficiency\n",
					100.0f - ((float)ri.pc.meshBatchFlush/(float)ri.pc.meshBatches));
		}

		// Cull information
		if (r_debugCulling->intVal && !r_noCull->intVal) {
			Com_Printf (0, "\n");
			Com_Printf (0, "bounds[%3i/%3i] planar[%3i/%3i] radii[%3i/%3i] visFrame[%3i/%3i] surfFrame[%3i/%3i]\n",
				ri.pc.cullBounds[CULL_PASS], ri.pc.cullBounds[CULL_FAIL],
				ri.pc.cullPlanar[CULL_PASS], ri.pc.cullPlanar[CULL_FAIL],
				ri.pc.cullRadius[CULL_PASS], ri.pc.cullRadius[CULL_FAIL],
				ri.pc.cullVis[CULL_PASS], ri.pc.cullVis[CULL_FAIL],
				ri.pc.cullSurf[CULL_PASS], ri.pc.cullSurf[CULL_FAIL]);
		}

		memset (&ri.pc, 0, sizeof (refStats_t));
	}

	// Next frame
	ri.frameCount++;
}

// ==========================================================

/*
====================
R_ClearScene
====================
*/
void R_ClearScene (void)
{
	ri.scn.numDecals = 0;
	ri.scn.drawnDecals = 0;

	ri.scn.numDLights = 0;
	ri.scn.numEntities = ENTLIST_OFFSET;
	ri.scn.numPolys = 0;
}


/*
=====================
R_AddDecal
=====================
*/
void R_AddDecal (refDecal_t *decal, bvec4_t color, struct material_s *material, float materialTime)
{
	int			i;

	if (!decal || ri.scn.numDecals >= MAX_REF_DECALS)
		return;

	// Adjust color
	if (color) {
		for (i=0 ; i<decal->poly.numVerts ; i++)
			*(int *)decal->poly.colors[i] = *(int *)color;
	}

	// Material
	decal->poly.mat = material;
	if (!decal->poly.mat)
		decal->poly.mat = r_noMaterial;
	decal->poly.matTime = materialTime;

	// FIXME: adjust bmodel decals here

	// Store
	ri.scn.decalList[ri.scn.numDecals++] = decal;
}


/*
=====================
R_AddEntity
=====================
*/
void R_AddEntity (refEntity_t *ent)
{
	if (ri.scn.numEntities >= MAX_REF_ENTITIES)
		return;
	if (ent->color[3] <= 0)
		return;

	ri.scn.entityList[ri.scn.numEntities] = *ent;
	if (ent->color[3] < 255)
		ri.scn.entityList[ri.scn.numEntities].flags |= RF_TRANSLUCENT;

	ri.scn.numEntities++;
}


/*
=====================
R_AddPoly
=====================
*/
void R_AddPoly (refPoly_t *poly)
{
	if (ri.scn.numPolys >= MAX_REF_POLYS)
		return;

	// Material
	if (!poly->mat)
		poly->mat = r_noMaterial;

	// Store
	ri.scn.polyList[ri.scn.numPolys++] = poly;
}


/*
=====================
R_AddLight
=====================
*/
void R_AddLight (vec3_t origin, float intensity, float r, float g, float b)
{
	refDLight_t	*dl;

	if (ri.scn.numDLights >= MAX_REF_DLIGHTS)
		return;

	if (!intensity)
		return;

	dl = &ri.scn.dLightList[ri.scn.numDLights++];

	Vec3Copy (origin, dl->origin);
	Vec3Set (dl->color, r, g, b);
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
	refLightStyle_t	*ls;

	if (style < 0 || style > MAX_CS_LIGHTSTYLES) {
		Com_Error (ERR_DROP, "Bad light style %i", style);
		return;
	}

	ls = &ri.scn.lightStyles[style];

	ls->white = r+g+b;
	Vec3Set (ls->rgb, r, g, b);
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
		Com_Printf (PRNT_ERROR, "GL_ERROR: '%s' (0x%x)", GetGLErrorString (error), error);
		if (where)
			Com_Printf (0, " %s\n", where);
		else
			Com_Printf (0, "\n");
	}
}


/*
=============
R_GetRefConfig
=============
*/
void R_GetRefConfig (refConfig_t *outConfig)
{
	*outConfig = ri.config;
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
	Matrix4_Multiply_Vector (ri.scn.worldViewMatrix, temp, temp2);
	Matrix4_Multiply_Vector (ri.scn.projectionMatrix, temp2, temp);

	if (!temp[3])
		return;
	out[0] = ri.def.x + (temp[0] / temp[3] + 1.0f) * ri.def.width * 0.5f;
	out[1] = ri.def.y + (temp[1] / temp[3] + 1.0f) * ri.def.height * 0.5f;
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
	ri.reg.fontsReleased = 0;
	ri.reg.fontsSeaked = 0;
	ri.reg.fontsTouched = 0;
	ri.reg.imagesReleased = 0;
	ri.reg.imagesResampled = 0;
	ri.reg.imagesSeaked = 0;
	ri.reg.imagesTouched = 0;
	ri.reg.modelsReleased = 0;
	ri.reg.modelsSeaked = 0;
	ri.reg.modelsTouched = 0;
	ri.reg.matsReleased = 0;
	ri.reg.matsSeaked = 0;
	ri.reg.matsTouched = 0;

	// Begin sub-system registration
	ri.reg.inSequence = qTrue;
	ri.reg.registerFrame++;

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
	R_EndFontRegistration (); // Register first so materials are touched
	R_EndModelRegistration (); // Register first so materials are touched
	R_EndMaterialRegistration ();	// Register first so programs and images are touched
	R_EndImageRegistration ();

	ri.reg.inSequence = qFalse;

	// Print registration info
	Com_Printf (PRNT_CONSOLE, "Registration sequence completed...\n");
	Com_Printf (PRNT_CONSOLE, "Fonts      rel/touch/seak: %i/%i/%i\n", ri.reg.fontsReleased, ri.reg.fontsTouched, ri.reg.fontsSeaked);
	Com_Printf (PRNT_CONSOLE, "Models     rel/touch/seak: %i/%i/%i\n", ri.reg.modelsReleased, ri.reg.modelsTouched, ri.reg.modelsSeaked);
	Com_Printf (PRNT_CONSOLE, "Materials  rel/touch/seak: %i/%i/%i\n", ri.reg.matsReleased, ri.reg.matsTouched, ri.reg.matsSeaked);
	Com_Printf (PRNT_CONSOLE, "Images     rel/resamp/seak/touch: %i/%i/%i/%i\n", ri.reg.imagesReleased, ri.reg.imagesResampled, ri.reg.imagesSeaked, ri.reg.imagesTouched);
}
