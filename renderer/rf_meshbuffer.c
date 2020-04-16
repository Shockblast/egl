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
// rf_meshbuffer.c
//

#include "rf_local.h"

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
meshBuffer_t *R_AddMeshToList (shader_t *shader, float shaderTime, refEntity_t *ent, struct mQ3BspFog_s *fog, meshType_t meshType, void *mesh)
{
	mBspSurface_t	*surf;
	meshBuffer_t	*mb;

	assert (meshType >= 0 && meshType < MBT_MAX);

	// Check if it qualifies to be added to the list
	if (!shader)
		return NULL;
	else if (!shader->numPasses)
		return NULL;
	if (!mesh)
		return NULL;
	if (!ent)
		ent = ri.scn.defaultEntity;

	// Choose the buffer to append to
	switch (shader->sortKey) {
	case SHADER_SORT_SKY:
	case SHADER_SORT_OPAQUE:
		if (r_currentList->numMeshes[shader->sortKey] >= MAX_MESH_BUFFER)
			return NULL;
		mb = &r_currentList->meshBuffer[shader->sortKey][r_currentList->numMeshes[shader->sortKey]++];
		break;

	case SHADER_SORT_POSTPROCESS:
		if (r_currentList->numPostProcessMeshes >= MAX_POSTPROC_BUFFER)
			return NULL;
		mb = &r_currentList->meshBufferPostProcess[r_currentList->numPostProcessMeshes];
		break;

	case SHADER_SORT_PORTAL:
		if (ri.scn.mirrorView || ri.scn.portalView)
			return NULL;
		// Fall through

	default:
		if (r_currentList->numAdditiveMeshes[shader->sortKey - MAX_MESH_KEYS] >= MAX_ADDITIVE_BUFFER)
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
	mb->sortKey = meshType & (MBT_MAX-1);						// [0 - 2^4] == [0 - 16]

	// Sort by entity
	mb->sortKey |= (((int)(ent - ri.scn.entityList)+1) << 5);	// [2^5 - 2^16] == [32 - 65536]

	// Sort by dlightbits
	switch (meshType) {
	case MBT_Q2BSP:
		surf = (mBspSurface_t *)mesh;
		if (surf->dLightBits)
			mb->sortKey |= 1 << 26;								// [2^26]
		break;

	case MBT_Q3BSP:
		surf = (mBspSurface_t *)mesh;
		if (surf->dLightBits)
			mb->sortKey |= 1 << 26;								// [2^26]

		// Sort by fog
		if (fog)
			mb->sortKey |= (((int)(fog - ri.scn.worldModel->q3BspModel.fogs)+1) << 17);	// [2^17 - 2^25] == [131072 - 33554432]
		break;
	}

	// Translucent models rendered first
	if (ent->flags & RF_TRANSLUCENT)
		mb->sortKey |= 1 << 27;									// [2^27]

	// Stupidity check
	assert ((mb->sortKey & (MBT_MAX-1)) == meshType);
	assert (&ri.scn.entityList[(mb->sortKey >> 5) & (MAX_REF_ENTITIES-1)] != ent);
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
	uint32	startTime;
	int		i;

	if (r_debugSorting->intVal)
		return;

	if (r_times->intVal)
		startTime = Sys_UMilliseconds ();

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

	if (r_times->intVal)
		ri.pc.timeSortList += Sys_UMilliseconds () - startTime;
}


/*
=============
R_BatchMeshBuffer
=============
*/
static inline void R_BatchMeshBuffer (meshBuffer_t *mb, meshBuffer_t *nextMB, qBool shadowPass, qBool triangleOutlines)
{
	mBspSurface_t	*surf, *nextSurf;
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
	meshType = mb->sortKey & (MBT_MAX-1);
	nextMeshType = (nextMB) ? nextMB->sortKey & (MBT_MAX-1) : -1;
	switch (meshType) {
	case MBT_ALIAS:
		RB_RotateForEntity (mb->entity);
		R_DrawAliasModel (mb, shadowPass);

		if (nextMeshType != MBT_ALIAS)
			RB_LoadModelIdentity ();
		break;

	case MBT_Q2BSP: 
		if (shadowPass)
			break;

		// Find the surface
		surf = (mBspSurface_t *)mb->mesh;
		R_Q2BSP_UpdateLightmap (surf);
		if (nextMeshType == MBT_Q2BSP) {
			nextSurf = (mBspSurface_t *)nextMB->mesh;
			R_Q2BSP_UpdateLightmap (nextSurf);
		}
		else
			nextSurf = NULL;

		// Set features
		features = mb->shader->features;
		if (gl_shownormals->intVal)
			features |= MF_NORMALS;
		if (mb->shader->flags & SHADER_AUTOSPRITE)
			features |= MF_NOCULL;
		if (!(mb->shader->flags & SHADER_ENTITY_MERGABLE) || r_debugBatching->intVal == 2)
			features |= MF_NONBATCHED;

		// Push the mesh
		if (!triangleOutlines)
			ri.pc.worldPolys++;

		RB_PushMesh (surf->mesh, features);

		if (features & MF_NONBATCHED
		|| mb->shader->flags & SHADER_DEFORMV_BULGE
		|| !nextMB
		|| nextMB->shader->flags & SHADER_DEFORMV_BULGE
		|| nextMB->entity != mb->entity
		|| nextMB->sortKey != mb->sortKey
		|| nextMB->shader != mb->shader
		|| nextMB->shaderTime != mb->shaderTime
		|| !nextSurf
		|| nextSurf->q2_lmTexNumActive != surf->q2_lmTexNumActive
		|| RB_BackendOverflow (nextSurf->mesh->numVerts, nextSurf->mesh->numIndexes)) {
			if (mb->entity->model != ri.scn.worldModel)
				RB_RotateForEntity (mb->entity);

			ri.pc.meshBatchFlush++;
			RB_RenderMeshBuffer (mb, shadowPass);

			if (mb->entity->model != ri.scn.worldModel)
				RB_LoadModelIdentity ();
		}
		break;

	case MBT_Q3BSP:
		if (shadowPass)
			break;
		if (!triangleOutlines)
			ri.pc.worldPolys++;

		surf = (mBspSurface_t *)mb->mesh;
		nextSurf = (nextMeshType == MBT_Q3BSP) ? (mBspSurface_t *)nextMB->mesh : NULL;

		features = mb->shader->features;
		if (gl_shownormals->intVal)
			features |= MF_NORMALS;
		if (mb->shader->flags & SHADER_AUTOSPRITE)
			features |= MF_NOCULL;
		if (!(mb->shader->flags & SHADER_ENTITY_MERGABLE) || r_debugBatching->intVal == 2)
			features |= MF_NONBATCHED;
		RB_PushMesh (surf->mesh, features);

		if (features & MF_NONBATCHED
		|| mb->shader->flags & SHADER_DEFORMV_BULGE
		|| !nextMB
		|| nextMB->entity != mb->entity
		|| nextMB->sortKey != mb->sortKey
		|| nextMB->shader != mb->shader
		|| nextMB->shaderTime != mb->shaderTime
		|| nextSurf->dLightBits != surf->dLightBits
		|| nextSurf->lmTexNum != surf->lmTexNum
		|| nextMB->shader->flags & SHADER_DEFORMV_BULGE
		|| RB_BackendOverflow (nextSurf->mesh->numVerts, nextSurf->mesh->numIndexes)) {
			if (mb->entity->model != ri.scn.worldModel)
				RB_RotateForEntity (mb->entity);

			ri.pc.meshBatchFlush++;
			RB_RenderMeshBuffer (mb, shadowPass);

			if (mb->entity->model != ri.scn.worldModel)
				RB_LoadModelIdentity ();
		}
		break;

	case MBT_Q3BSP_FLARE:
		if (shadowPass)
			break;
		if (!triangleOutlines)
			ri.pc.worldPolys++;

		surf = (mBspSurface_t *)mb->mesh;
		nextSurf = nextMB ? (mBspSurface_t *)nextMB->mesh : NULL;

		features = mb->shader->features;
		if (gl_shownormals->intVal)
			features |= MF_NORMALS;
		if (mb->shader->flags & SHADER_AUTOSPRITE)
			features |= MF_NOCULL;
		if (!(mb->shader->flags & SHADER_ENTITY_MERGABLE) || r_debugBatching->intVal == 2)
			features |= MF_NONBATCHED;
		R_PushFlare (mb);

		if (features & MF_NONBATCHED
		|| mb->shader->flags & SHADER_DEFORMV_BULGE
		|| !nextMB
		|| nextMB->sortKey != mb->sortKey
		|| nextMB->shader != mb->shader
		|| nextMB->shaderTime != mb->shaderTime
		|| nextSurf->dLightBits != surf->dLightBits
		|| nextSurf->lmTexNum != surf->lmTexNum
		|| nextMB->shader->flags & SHADER_DEFORMV_BULGE
		|| R_FlareOverflow ()) {
			ri.pc.meshBatchFlush++;
			RB_RenderMeshBuffer (mb, shadowPass);
		}
		break;

	case MBT_SP2:
		if (shadowPass)
			break;

		ri.pc.meshBatchFlush++;
		R_DrawSP2Model (mb);
		break;

	case MBT_DECAL:
		if (shadowPass)
			break;

		// FIXME: adjust for bmodel decals here

		features = mb->shader->features;
		if (gl_shownormals->intVal)
			features |= MF_NORMALS;
		if (!(mb->shader->flags & SHADER_ENTITY_MERGABLE) || r_debugBatching->intVal == 2)
			features |= MF_NONBATCHED;
		R_PushDecal (mb, features);

		if (features & MF_NONBATCHED
		|| !nextMB
		|| nextMB->sortKey != mb->sortKey
		|| nextMB->shader != mb->shader
		|| nextMB->shaderTime != mb->shaderTime
		|| R_DecalOverflow (nextMB)) {
			ri.pc.meshBatchFlush++;
			RB_RenderMeshBuffer (mb, shadowPass);
		}
		break;

	case MBT_POLY:
		if (shadowPass)
			break;

		features = MF_TRIFAN|mb->shader->features;
		if (!(mb->shader->flags & SHADER_ENTITY_MERGABLE) || r_debugBatching->intVal == 2)
			features |= MF_NONBATCHED;

		R_PushPoly (mb, features);

		if (features & MF_NONBATCHED
		|| !nextMB
		|| nextMB->sortKey != mb->sortKey
		|| nextMB->shader != mb->shader
		|| nextMB->shaderTime != mb->shaderTime
		|| R_PolyOverflow (nextMB)) {
			ri.pc.meshBatchFlush++;
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
void R_DrawMeshList (qBool triangleOutlines)
{
	meshBuffer_t	*mb;
	uint32			startTime;
	meshType_t		meshType;
	int				i, j;

	if (r_times->intVal)
		startTime = Sys_UMilliseconds ();

	// Draw meshes
	for (j=0 ; j<MAX_MESH_KEYS ; j++) {
		if (!r_currentList->numMeshes[j])
			continue;
		mb = r_currentList->meshBuffer[j];
		for (i=0 ; i<r_currentList->numMeshes[j]-1 ; i++, mb++)
			R_BatchMeshBuffer (mb, mb+1, qFalse, triangleOutlines);
		R_BatchMeshBuffer (mb, NULL, qFalse, triangleOutlines);
	}

	// Draw additive meshes
	for (j=0 ; j<MAX_ADDITIVE_KEYS ; j++) {
		if (!r_currentList->numAdditiveMeshes[j])
			continue;

		// Update lightmaps
		mb = r_currentList->meshBufferAdditive[j];
		for (i=0 ; i<r_currentList->numAdditiveMeshes[j] ; i++, mb++) {
			meshType = mb->sortKey & (MBT_MAX-1);
			if (meshType == MBT_Q2BSP)
				R_Q2BSP_UpdateLightmap ((mBspSurface_t *)mb->mesh);
		}

		// Render meshes
		mb = r_currentList->meshBufferAdditive[j];
		for (i=0 ; i<r_currentList->numAdditiveMeshes[j]-1 ; i++, mb++)
			R_BatchMeshBuffer (mb, mb+1, qFalse, triangleOutlines);
		R_BatchMeshBuffer (mb, NULL, qFalse, triangleOutlines);
	}

	// Draw mesh shadows
	if (gl_shadows->intVal) {
#ifdef SHADOW_VOLUMES
		RB_SetShadowState (qTrue);
#endif

		// Draw the meshes
		for (j=0 ; j<MAX_MESH_KEYS ; j++) {
			if (!r_currentList->numMeshes[j])
				continue;
			mb = r_currentList->meshBuffer[j];
			for (i=0 ; i<r_currentList->numMeshes[j]-1 ; i++, mb++)
				R_BatchMeshBuffer (mb, mb+1, qTrue, triangleOutlines);
			R_BatchMeshBuffer (mb, NULL, qTrue, triangleOutlines);
		}

		for (j=0 ; j<MAX_ADDITIVE_KEYS ; j++) {
			if (!r_currentList->numAdditiveMeshes[j])
				continue;
			mb = r_currentList->meshBufferAdditive[j];
			for (i=0 ; i<r_currentList->numAdditiveMeshes[j]-1 ; i++, mb++)
				R_BatchMeshBuffer (mb, mb+1, qTrue, triangleOutlines);
			R_BatchMeshBuffer (mb, NULL, qTrue, triangleOutlines);
		}

		if (r_currentList->numPostProcessMeshes) {
			mb = r_currentList->meshBufferPostProcess;
			for (i=0 ; i<r_currentList->numPostProcessMeshes-1 ; i++, mb++)
				R_BatchMeshBuffer (mb, mb+1, qTrue, triangleOutlines);
			R_BatchMeshBuffer (mb, NULL, qTrue, triangleOutlines);
		}

#ifdef SHADOW_VOLUMES
		RB_SetShadowState (qFalse);
#endif
	}

	// Draw post process meshes
	if (r_currentList->numPostProcessMeshes) {
		mb = r_currentList->meshBufferPostProcess;
		for (i=0 ; i<r_currentList->numPostProcessMeshes-1 ; i++, mb++)
			R_BatchMeshBuffer (mb, mb+1, qFalse, triangleOutlines);
		R_BatchMeshBuffer (mb, NULL, qFalse, triangleOutlines);
	}

	// Clear state
	RB_FinishRendering ();

	if (r_times->intVal)
		ri.pc.timeDrawList += Sys_UMilliseconds () - startTime;
}


/*
=============
R_DrawMeshOutlines
=============
*/
void R_DrawMeshOutlines (void)
{
	if (!gl_showtris->intVal && !gl_shownormals->intVal)
		return;

	RB_BeginTriangleOutlines ();
	R_DrawMeshList (qTrue);
	RB_EndTriangleOutlines ();
}
