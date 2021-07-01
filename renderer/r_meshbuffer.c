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
// r_meshbuffer.c
//

#include "r_local.h"

#define 	QSORT_MAX_STACKDEPTH	4096

meshList_t	r_portalList;
meshList_t	r_worldList;
meshList_t	*r_currentList;

#define R_MBCopy(in,out) \
	(\
		(out).sortKey = (in).sortKey, \
		(out).shaderTime = (in).shaderTime, \
		(out).entity = (in).entity, \
		(out).shader = (in).shader, \
		(out).fog = (in).fog, \
		(out).mesh = (in).mesh \
	)

/*
=============
R_AddMeshToList
=============
*/
meshBuffer_t *R_AddMeshToList (shader_t *shader, float shaderTime, entity_t *ent, struct mQ3BspFog_s *fog, meshType_t meshType, void *mesh)
{
	meshBuffer_t	*mb;

	// Check if it qualifies to be added to the list
	if (!shader)
		return NULL;
	else if (!shader->numPasses)
		return NULL;
	if (!ent)
		ent = &r_refScene.defaultEntity;

	// Choose the buffer to append to
	switch (shader->sortKey) {
	case SHADER_SORT_SKY:
	case SHADER_SORT_OPAQUE:
		if (r_currentList->numMeshes[shader->sortKey] >= MAX_MESH_BUFFER)
			return NULL;
		mb = &r_currentList->meshBuffer[shader->sortKey][r_currentList->numMeshes[shader->sortKey]++];
		break;

	case SHADER_SORT_POSTPROCESS:
		if (r_currentList->numPostProcessMeshes >= MAX_MESH_BUFFER)
			return NULL;
		mb = &r_currentList->meshBufferPostProcess[r_currentList->numPostProcessMeshes];
		break;

	case SHADER_SORT_PORTAL:
		if (r_refScene.mirrorView || r_refScene.portalView)
			return NULL;
		// Fall through

	default:
		if (r_currentList->numAdditiveMeshes[shader->sortKey - MAX_MESH_KEYS] >= MAX_MESH_BUFFER)
			return NULL;
		mb = &r_currentList->meshBufferAdditive[shader->sortKey - MAX_MESH_KEYS][r_currentList->numAdditiveMeshes[shader->sortKey - MAX_MESH_KEYS]++];
		break;
	}

	// Fill it in
	mb->entity = ent;
	mb->shader = shader;
	mb->shaderTime = shaderTime;
	mb->fog = fog;
	mb->mesh = mesh;

	// Set the sort key
	mb->sortKey = meshType;
	if (ent->flags & RF_TRANSLUCENT)
		mb->sortKey |= 1 << 26;

#ifdef _DEBUG
	assert ((mb->sortKey & 7) == meshType);
	assert (meshType >= 0 && meshType < MBT_MAX);
#endif // _DEBUG

	return mb;
}


/*
================
R_QSortMeshBuffers

Quicksort
================
*/
static void R_QSortMeshBuffers (meshBuffer_t *meshes, int Li, int Ri)
{
	int		li, ri, stackDepth, total;
	meshBuffer_t median, tempbuf;
	int		localStack[QSORT_MAX_STACKDEPTH];

	stackDepth = 0;
	total = Ri + 1;

mark0:
	if (Ri - Li < 48) {
		li = Li;
		ri = Ri;

		R_MBCopy (meshes[(Li+Ri) >> 1], median);

		if (meshes[Li].sortKey > median.sortKey) {
			if (meshes[Ri].sortKey > meshes[Li].sortKey) 
				R_MBCopy (meshes[Li], median);
		}
		else if (median.sortKey > meshes[Ri].sortKey) {
			R_MBCopy (meshes[Ri], median);
		}

		while (li < ri) {
			while (median.sortKey > meshes[li].sortKey)
				li++;
			while (meshes[ri].sortKey > median.sortKey)
				ri--;

			if (li <= ri) {
				R_MBCopy (meshes[ri], tempbuf);
				R_MBCopy (meshes[li], meshes[ri]);
				R_MBCopy (tempbuf, meshes[li]);

				li++;
				ri--;
			}
		}

		if (Li < ri && stackDepth < QSORT_MAX_STACKDEPTH) {
			localStack[stackDepth++] = li;
			localStack[stackDepth++] = Ri;
			li = Li;
			Ri = ri;
			goto mark0;
		}

		if (li < Ri) {
			Li = li;
			goto mark0;
		}
	}

	if (stackDepth) {
		Ri = ri = localStack[--stackDepth];
		Li = li = localStack[--stackDepth];
		goto mark0;
	}

	for (li=1 ; li<total ; li++) {
		R_MBCopy (meshes[li], tempbuf);
		ri = li - 1;

		while (ri >= 0 && meshes[ri].sortKey > tempbuf.sortKey) {
			R_MBCopy (meshes[ri], meshes[ri+1]);
			ri--;
		}
		if (li != ri+1)
			R_MBCopy (tempbuf, meshes[ri+1]);
	}
}


/*
================
R_ISortMeshes

Insertion sort
================
*/
static void R_ISortMeshBuffers (meshBuffer_t *meshes, int numMeshes)
{
	int				i, j;
	meshBuffer_t	tempbuf;

	for (i=1 ; i<numMeshes ; i++) {
		R_MBCopy (meshes[i], tempbuf);
		j = i - 1;

		while (j >= 0 && meshes[j].sortKey > tempbuf.sortKey) {
			R_MBCopy (meshes[j], meshes[j+1]);
			j--;
		}
		if (i != j+1)
			R_MBCopy (tempbuf, meshes[j+1]);
	}
}


/*
=============
R_SortMeshList
=============
*/
void R_SortMeshList (void)
{
	int		i;

	if (r_debugSorting->integer)
		return;

	// Sort meshes
	for (i=0 ; i<MAX_MESH_KEYS ; i++) {
		if (r_currentList->numMeshes[i])
			R_QSortMeshBuffers (r_currentList->meshBuffer[i], 0, r_currentList->numMeshes[i] - 1);
	}

	// Sort additive meshes
	for (i=0 ; i<MAX_ADDITIVE_KEYS ; i++) {
		if (r_currentList->numAdditiveMeshes[i])
			R_ISortMeshBuffers (r_currentList->meshBufferAdditive[i], r_currentList->numAdditiveMeshes[i]);
	}

	// Sort post-process meshes
	if (r_currentList->numPostProcessMeshes)
		R_ISortMeshBuffers (r_currentList->meshBufferPostProcess, r_currentList->numPostProcessMeshes);
}


/*
=============
R_BatchMeshBuffer
=============
*/
static inline void R_BatchMeshBuffer (meshBuffer_t *mb, meshBuffer_t *nextMB, qBool shadowPass, qBool triangleOutlines)
{
	mBspSurface_t	*surf, *nextSurf;
	mQ2BspPoly_t	*poly;
	meshFeatures_t	features;
	meshType_t		meshType;
	meshType_t		nextMeshType;

	// Check if it's a sky surface
	if (mb->shader->flags & SHADER_SKY) {
		if (!r_currentList->skyDrawn) {
			R_DrawSky (mb);
			r_currentList->skyDrawn = qTrue;
		}
		return;
	}

	if (!shadowPass) {
		// Check if it's a portal surface
		if (mb->shader->sortKey == SHADER_SORT_PORTAL && !triangleOutlines) {
		}
	}
	else {
		// If in shadowPass, check if it's allowed
		if (mb->entity->flags & (RF_WEAPONMODEL|RF_NOSHADOW))
			return;
	}

	// Render it!
	meshType = mb->sortKey & 7;
	nextMeshType = (nextMB) ? nextMB->sortKey & 7 : -1;
	switch (meshType) {
	case MBT_ALIAS:
		R_RotateForEntity (mb->entity);
		R_DrawAliasModel (mb, shadowPass);

		if (nextMeshType != MBT_ALIAS)
			R_LoadModelIdentity ();
		break;

	case MBT_Q2BSP: 
		if (shadowPass)
			break;

		// Find the surface
		surf = (mBspSurface_t *)mb->mesh;
		R_Q2BSP_UpdateLightmap (surf);

		// Set features
		features = MF_TRIFAN|mb->shader->features;
		if (gl_shownormals->integer && !shadowPass)
			features |= MF_NORMALS;
		if (mb->shader->flags & SHADER_AUTOSPRITE)
			features |= MF_NOCULL;
		if (!(mb->shader->flags & SHADER_ENTITY_MERGABLE))
			features |= MF_NONBATCHED;

		// Push the mesh
		for (poly=surf->q2_polys ; poly ; poly=poly->next) {
			if (!triangleOutlines)
				r_refStats.worldPolys++;

			if (RB_InvalidMesh (&poly->mesh))
				continue;

			RB_PushMesh (&poly->mesh, features);

			if (!(features & MF_NONBATCHED)
			&& poly->next
			&& !RB_BackendOverflow (&poly->next->mesh))
				continue;

			if (mb->entity->model != r_refScene.worldModel)
				R_RotateForEntity (mb->entity);

			RB_RenderMeshBuffer (mb, shadowPass);

			if (mb->entity->model != r_refScene.worldModel)
				R_LoadModelIdentity ();
		}
		break;

	case MBT_Q3BSP:
		if (shadowPass)
			break;
		if (!triangleOutlines)
			r_refStats.worldPolys++;

		surf = (mBspSurface_t *)mb->mesh;
		nextSurf = nextMB ? (mBspSurface_t *)nextMB->mesh : NULL;

		features = mb->shader->features;
		if (gl_shownormals->integer && !shadowPass)
			features |= MF_NORMALS;
		if (mb->shader->flags & SHADER_AUTOSPRITE)
			features |= MF_NOCULL;
		if (!(mb->shader->flags & SHADER_ENTITY_MERGABLE))
			features |= MF_NONBATCHED;
		RB_PushMesh (surf->q3_mesh, features);

		if (features & MF_NONBATCHED
		|| mb->shader->flags & SHADER_DEFORMV_BULGE
		|| !nextMB
		|| nextMeshType != MBT_Q3BSP
		|| nextMB->sortKey != mb->sortKey
		|| nextMB->shader != mb->shader
		|| nextSurf->dLightBits != surf->dLightBits
		|| nextSurf->lmTexNum != surf->lmTexNum
		|| nextMB->shader->flags & SHADER_DEFORMV_BULGE
		|| RB_BackendOverflow (nextSurf->q3_mesh)) {
			if (mb->entity->model != r_refScene.worldModel)
				R_RotateForEntity (mb->entity);

			RB_RenderMeshBuffer (mb, shadowPass);

			if (mb->entity->model != r_refScene.worldModel)
				R_LoadModelIdentity ();
		}
		break;

	case MBT_Q3BSP_FLARE:
		if (shadowPass)
			break;
		if (!triangleOutlines)
			r_refStats.worldPolys++;

		surf = (mBspSurface_t *)mb->mesh;
		nextSurf = nextMB ? (mBspSurface_t *)nextMB->mesh : NULL;

		features = mb->shader->features;
		if (gl_shownormals->integer && !shadowPass)
			features |= MF_NORMALS;
		if (mb->shader->flags & SHADER_AUTOSPRITE)
			features |= MF_NOCULL;
		if (!(mb->shader->flags & SHADER_ENTITY_MERGABLE))
			features |= MF_NONBATCHED;
		R_PushFlare (mb);

		if (features & MF_NONBATCHED
		|| mb->shader->flags & SHADER_DEFORMV_BULGE
		|| !nextMB
		|| nextMeshType != MBT_Q3BSP_FLARE
		|| nextMB->sortKey != mb->sortKey
		|| nextMB->shader != mb->shader
		|| nextSurf->dLightBits != surf->dLightBits
		|| nextSurf->lmTexNum != surf->lmTexNum
		|| nextMB->shader->flags & SHADER_DEFORMV_BULGE
		|| R_FlareOverflow ())
			RB_RenderMeshBuffer (mb, shadowPass);
		break;

	case MBT_SP2:
		if (shadowPass)
			break;

		R_DrawSP2Model (mb);
		break;

	case MBT_POLY:
		if (shadowPass)
			break;

		R_PushPoly (mb);

		if (!nextMB
		|| nextMB->sortKey != mb->sortKey
		|| nextMB->shader != mb->shader
		|| !(mb->shader->flags & SHADER_ENTITY_MERGABLE)
		|| R_PolyOverflow (nextMB)) {
			RB_RenderMeshBuffer (mb, shadowPass);
		}
		break;
	}
}


/*
=============
R_DrawMeshList
=============
*/
static void R_SetShadowState (qBool start)
{
	if (start) {
		// Load identity
		R_LoadModelIdentity ();

		// Clear state
		RB_FinishRendering ();

		// Set the mode
		GL_SelectTexture (0);
		qglDisable (GL_TEXTURE_2D);
		switch (gl_shadows->integer) {
		case 1:
			qglEnable (GL_CULL_FACE);
			qglEnable (GL_BLEND);

			qglColor4f (0, 0, 0, SHADOW_ALPHA);

			qglDepthFunc (GL_LEQUAL);

			if (!glStatic.useStencil)
				break;
			qglEnable (GL_STENCIL_TEST);
			qglStencilFunc (GL_EQUAL, 128, 0xFF);
			qglStencilOp (GL_KEEP, GL_KEEP, GL_INCR);
			qglStencilMask (255);
			break;

#ifdef SHADOW_VOLUMES
		case SHADOW_VOLUMES:
			qglEnable (GL_CULL_FACE);
			qglEnable (GL_BLEND);

			qglColor4f (1, 1, 1, 1);
			qglColorMask (GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

			qglDepthFunc (GL_LESS);

			if (!glStatic.useStencil)
				break;
			qglEnable (GL_STENCIL_TEST);
			qglStencilFunc (GL_ALWAYS, 128, 255);
			qglStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);
			qglStencilMask (255);
			break;

		case 3:
			qglDisable (GL_CULL_FACE);
			qglEnable (GL_BLEND);

			GL_BlendFunc (GL_ONE, GL_ONE);
			qglColor3f (1.0f, 0.1f, 0.1f);

			qglDepthFunc (GL_LEQUAL);

			if (!glStatic.useStencil)
				break;
			qglDisable (GL_STENCIL_TEST);
			break;
#endif
		}
		qglDisable (GL_ALPHA_TEST);
		qglDepthMask (GL_FALSE);
		return;
	}

	// Reset
	switch (gl_shadows->integer) {
	case 1:
		qglDisable (GL_BLEND);

		if (!glStatic.useStencil)
			break;
		qglDisable (GL_STENCIL_TEST);
		break;

#ifdef SHADOW_VOLUMES
	case SHADOW_VOLUMES:
		if (!glStatic.useStencil)
			break;
		qglStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);
		qglDisable (GL_STENCIL_TEST);
		qglColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		qglDepthFunc (GL_LEQUAL);
		break;

	default:
		qglDisable (GL_BLEND);
		break;
#endif
	}

	qglEnable (GL_TEXTURE_2D);
	qglDepthMask (GL_TRUE);
}

void R_DrawMeshList (qBool triangleOutlines)
{
	meshBuffer_t	*mb;
	int				i, j;

	// Draw meshes
	for (j=0 ; j<MAX_MESH_KEYS ; j++) {
		if (r_currentList->numMeshes[j]) {
			mb = r_currentList->meshBuffer[j];
			for (i=0 ; i<r_currentList->numMeshes[j]-1 ; i++, mb++)
				R_BatchMeshBuffer (mb, mb+1, qFalse, triangleOutlines);
			R_BatchMeshBuffer (mb, NULL, qFalse, triangleOutlines);
		}
	}

	// Draw additive meshes
	for (j=0 ; j<MAX_ADDITIVE_KEYS ; j++) {
		if (r_currentList->numAdditiveMeshes[j]) {
			mb = r_currentList->meshBufferAdditive[j];
			for (i=0 ; i<r_currentList->numAdditiveMeshes[j]-1 ; i++, mb++)
				R_BatchMeshBuffer (mb, mb+1, qFalse, triangleOutlines);
			R_BatchMeshBuffer (mb, NULL, qFalse, triangleOutlines);
		}
	}

	// Draw mesh shadows
	if (gl_shadows->integer) {
		R_SetShadowState (qTrue);

		// Draw the meshes
		for (j=0 ; j<MAX_MESH_KEYS ; j++) {
			if (r_currentList->numMeshes[j]) {
				mb = r_currentList->meshBuffer[j];
				for (i=0 ; i<r_currentList->numMeshes[j]-1 ; i++, mb++)
					R_BatchMeshBuffer (mb, mb+1, qTrue, triangleOutlines);
				R_BatchMeshBuffer (mb, NULL, qTrue, triangleOutlines);
			}
		}

		for (j=0 ; j<MAX_ADDITIVE_KEYS ; j++) {
			if (r_currentList->numAdditiveMeshes[j]) {
				mb = r_currentList->meshBufferAdditive[j];
				for (i=0 ; i<r_currentList->numAdditiveMeshes[j]-1 ; i++, mb++)
					R_BatchMeshBuffer (mb, mb+1, qTrue, triangleOutlines);
				R_BatchMeshBuffer (mb, NULL, qTrue, triangleOutlines);
			}
		}

		if (r_currentList->numPostProcessMeshes) {
			mb = r_currentList->meshBufferPostProcess;
			for (i=0 ; i<r_currentList->numPostProcessMeshes-1 ; i++, mb++)
				R_BatchMeshBuffer (mb, mb+1, qTrue, triangleOutlines);
			R_BatchMeshBuffer (mb, NULL, qTrue, triangleOutlines);
		}

		R_SetShadowState (qFalse);
	}

	// Draw post process meshes
	if (r_currentList->numPostProcessMeshes) {
		mb = r_currentList->meshBufferPostProcess;
		for (i=0 ; i<r_currentList->numPostProcessMeshes-1 ; i++, mb++)
			R_BatchMeshBuffer (mb, mb+1, qFalse, triangleOutlines);
		R_BatchMeshBuffer (mb, NULL, qFalse, triangleOutlines);
	}

	// Load identity
	R_LoadModelIdentity ();

	// Clear state
	RB_FinishRendering ();
}


/*
=============
R_DrawMeshOutlines
=============
*/
void R_DrawMeshOutlines (void)
{
	if (!gl_showtris->integer && !gl_shownormals->integer)
		return;

	RB_BeginTriangleOutlines ();
	R_DrawMeshList (qTrue);
	RB_EndTriangleOutlines ();
}
