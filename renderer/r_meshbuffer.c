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

#define		QSORT_MAX_STACKDEPTH	4096

meshList_t	r_worldList;
meshList_t	*r_currentList;

#define R_MBCopy(in,out) \
	(\
		(out).sortKey = (in).sortKey, \
		(out).entity = (in).entity, \
		(out).shader = (in).shader, \
		(out).mesh = (in).mesh \
	)

/*
=============
R_AddMeshToList
=============
*/
meshBuffer_t *R_AddMeshToList (shader_t *shader, entity_t *ent, int meshType, void *mesh)
{
	meshBuffer_t	*mb;

	if (!shader)
		shader = r_noShader;

	// Choose the buffer to append to
	switch (shader->sortKey) {
	case SHADER_SORT_SKY:
	case SHADER_SORT_OPAQUE:
		if (r_currentList->numMeshes >= MAX_MESH_BUFFER)
			return NULL;
		mb = &r_currentList->meshBuffer[r_currentList->numMeshes++];
		break;

	case SHADER_SORT_POSTPROCESS:
		if (r_currentList->numPostProcessMeshes >= MAX_MESH_BUFFER)
			return NULL;
		mb = &r_currentList->meshBufferPostProcess[r_currentList->numPostProcessMeshes];
		break;

	default:
		if (r_currentList->numAdditiveMeshes >= MAX_MESH_BUFFER)
			return NULL;
		mb = &r_currentList->meshBufferAdditive[r_currentList->numAdditiveMeshes++];
		break;
	}

	// Fill it in
	mb->entity = ent;
	mb->shader = shader;
	mb->mesh = mesh;

	// Set the sort key
	mb->sortKey = ((shader->sortKey + 1) << 27) | meshType;
	if (ent && ent->flags & RF_TRANSLUCENT)
		mb->sortKey |= 1 << 26;

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
	// Sort meshes
	if (r_currentList->numMeshes)
		R_QSortMeshBuffers (r_currentList->meshBuffer, 0, r_currentList->numMeshes - 1);

	// Sort additive meshes
	if (r_currentList->numAdditiveMeshes)
		R_ISortMeshBuffers (r_currentList->meshBufferAdditive, r_currentList->numAdditiveMeshes);

	// Sort post-process meshes
	if (r_currentList->numPostProcessMeshes)
		R_ISortMeshBuffers (r_currentList->meshBufferPostProcess, r_currentList->numPostProcessMeshes);
}


/*
=============
R_BatchMeshBuffer
=============
*/
static inline void R_BatchMeshBuffer (meshBuffer_t *mb, meshBuffer_t *nextMB, qBool shadowPass)
{
	shader_t		*shader;
	mBspSurface_t	*surf;
	mBspPoly_t		*poly;
	int				features;
	int				meshType;
	int				nextMeshType;

	// Copy values
	shader = mb->shader;

	// If in shadowPass, check if it's allowed
	if (shadowPass && mb->entity && mb->entity->flags & (RF_WEAPONMODEL|RF_NOSHADOW))
		return;

	// Render it!
	meshType = mb->sortKey & 7;
	nextMeshType = (nextMB) ? nextMB->sortKey & 7 : -1;
	switch (meshType) {
	case MBT_MODEL_ALIAS:
		R_RotateForEntity (mb->entity);
		R_DrawAliasModel (mb, shadowPass);

		if (nextMeshType != MBT_MODEL_ALIAS)
			R_LoadModelIdentity ();
		break;

	case MBT_MODEL_BSP: 
		if (shadowPass)
			break;

		// Find the surface
		surf = (mBspSurface_t *)mb->mesh;
		LM_UpdateLightmap (surf);

		// Set features
		features = MF_TRIFAN|mb->shader->features;
		if (gl_shownormals->integer && !shadowPass)
			features |= MF_NORMALS;
		if (shader->flags & SHADER_AUTOSPRITE)
			features |= MF_NOCULL;
		if (!(shader->flags & SHADER_ENTITY_MERGABLE))
			features |= MF_NONBATCHED;

		// Push the mesh
		for (poly=surf->polys ; poly ; poly=poly->next) {
			glSpeeds.worldPolys++;

			if (RB_InvalidMesh (&poly->mesh))
				continue;

			RB_PushMesh (&poly->mesh, features);

			if (!(features & MF_NONBATCHED)) {
				if (poly->next
				&& !RB_BackendOverflow (&poly->next->mesh))
					continue;
			}

			if (mb->entity->model != r_worldModel)
				R_RotateForEntity (mb->entity);

			RB_RenderMeshBuffer (mb, shadowPass);

			if (mb->entity->model != r_worldModel)
				R_LoadModelIdentity ();
		}

		break;

	case MBT_MODEL_SP2:
		if (shadowPass)
			break;

		R_DrawSpriteModel (mb);
		break;

	case MBT_POLY:
		if (shadowPass)
			break;

		R_PushPoly (mb);

		if (!nextMB
		|| nextMB->sortKey != mb->sortKey
		|| nextMB->shader != mb->shader
		|| !(shader->flags & SHADER_ENTITY_MERGABLE)
		|| R_PolyOverflow (nextMB)) {
			R_DrawPoly (mb);
		}
		break;

	case MBT_SKY:
		if (shadowPass)
			break;
		if (r_currentList->skyDrawn)
			break;

		R_DrawSky (mb);
		r_currentList->skyDrawn = qTrue;
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
	int				i;

	// Draw meshes
	if (r_currentList->numMeshes) {
		mb = r_currentList->meshBuffer;
		for (i=0 ; i<r_currentList->numMeshes-1 ; i++, mb++)
			R_BatchMeshBuffer (mb, mb+1, qFalse);
		R_BatchMeshBuffer (mb, NULL, qFalse);
	}

	// Draw additive meshes
	if (r_currentList->numAdditiveMeshes) {
		mb = r_currentList->meshBufferAdditive;
		for (i=0 ; i<r_currentList->numAdditiveMeshes-1 ; i++, mb++)
			R_BatchMeshBuffer (mb, mb+1, qFalse);
		R_BatchMeshBuffer (mb, NULL, qFalse);
	}

	// Draw mesh shadows
	if (gl_shadows->integer) {
		// Set the mode
		if (!triangleOutlines) {
			qglDisable (GL_TEXTURE_2D);

			if (gl_shadows->integer == 1) {
				qglEnable (GL_CULL_FACE);
				qglEnable (GL_BLEND);

				GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				qglColor4f (0, 0, 0, SHADOW_ALPHA);

				qglDepthFunc (GL_LEQUAL);

				qglEnable (GL_STENCIL_TEST);
				qglStencilFunc (GL_EQUAL, 128, 0xFF);
				qglStencilOp (GL_KEEP, GL_KEEP, GL_INCR);
			}
#ifdef SHADOW_VOLUMES
			else if (gl_shadows->integer == SHADOW_VOLUMES) {
				qglEnable (GL_CULL_FACE);
				qglEnable (GL_BLEND);

				qglColor4f (1, 1, 1, 1);
				qglColorMask (0, 0, 0, 0);

				qglDepthFunc (GL_LESS);

				qglEnable (GL_STENCIL_TEST);
				qglStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);
				qglStencilFunc (GL_ALWAYS, 128, 0xFF);
			}
			else {
				qglDisable (GL_STENCIL_TEST);
				qglDisable (GL_CULL_FACE);
				qglEnable (GL_BLEND);

				GL_BlendFunc (GL_ONE, GL_ONE);
				qglColor3f (1.0f, 0.1f, 0.1f);

				qglDepthFunc (GL_LEQUAL);
			}
#endif
			qglDisable (GL_ALPHA_TEST);
			qglDepthMask (GL_FALSE);
		}

		// Draw the meshes
		if (r_currentList->numMeshes) {
			mb = r_currentList->meshBuffer;
			for (i=0 ; i<r_currentList->numMeshes-1 ; i++, mb++)
				R_BatchMeshBuffer (mb, mb+1, qTrue);
			R_BatchMeshBuffer (mb, NULL, qTrue);
		}

		if (r_currentList->numAdditiveMeshes) {
			mb = r_currentList->meshBufferAdditive;
			for (i=0 ; i<r_currentList->numAdditiveMeshes-1 ; i++, mb++)
				R_BatchMeshBuffer (mb, mb+1, qTrue);
			R_BatchMeshBuffer (mb, NULL, qTrue);
		}

		if (r_currentList->numPostProcessMeshes) {
			mb = r_currentList->meshBufferPostProcess;
			for (i=0 ; i<r_currentList->numPostProcessMeshes-1 ; i++, mb++)
				R_BatchMeshBuffer (mb, mb+1, qTrue);
			R_BatchMeshBuffer (mb, NULL, qTrue);
		}

		// Reset
		if (!triangleOutlines) {
			if (gl_shadows->integer == 1) {
				qglDisable (GL_STENCIL_TEST);
				qglDisable (GL_BLEND);
			}
#ifdef SHADOW_VOLUMES
			else if (gl_shadows->integer == SHADOW_VOLUMES) {
				qglStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);
				qglDisable (GL_STENCIL_TEST);
				qglColorMask (1, 1, 1, 1);
				qglDepthFunc (GL_LEQUAL);
			}
			else {
				qglDisable (GL_BLEND);
			}
#endif
			qglEnable (GL_TEXTURE_2D);
			qglDepthMask (GL_TRUE);
		}
	}

	// Draw post process meshes
	if (r_currentList->numPostProcessMeshes) {
		mb = r_currentList->meshBufferPostProcess;
		for (i=0 ; i<r_currentList->numPostProcessMeshes-1 ; i++, mb++)
			R_BatchMeshBuffer (mb, mb+1, qFalse);
		R_BatchMeshBuffer (mb, NULL, qFalse);
	}

	// Load identity
	R_LoadModelIdentity ();

	qglDisable (GL_POLYGON_OFFSET_FILL);
	qglDisable (GL_ALPHA_TEST);
	qglDisable (GL_BLEND);
	qglDepthFunc (GL_LEQUAL);

	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDepthMask (GL_TRUE);
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
