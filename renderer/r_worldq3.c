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
// r_worldq3.c
//

#include "r_local.h"

static qBool			r_visChanged;
static uint32			r_numVisSurfs;
static mBspSurface_t	*r_visSurfs[Q3BSP_MAX_VIS];
static uint32			r_numSkySurfs;
static mBspSurface_t	*r_skySurfs[Q3BSP_MAX_VIS];

/*
=============================================================================

	WORLD MODEL

=============================================================================
*/

/*
================
R_AddQ3Surface
================
*/
static void R_AddQ3Surface (mBspSurface_t *surf, entity_t *ent, meshType_t meshType)
{
	// Add to list
	R_AddMeshToList (surf->q3_shaderRef->shader, ent->shaderTime, ent, surf->q3_fog, meshType, surf);
}


/*
================
R_CullQ3FlareSurface
================
*/
static qBool R_CullQ3FlareSurface (mBspSurface_t *surf, entity_t *ent, int clipFlags)
{
	vec3_t	origin;

	// Check if flares/culling are disabled
	if (!r_flares->integer || !r_flareFade->value)
		return qTrue;

	// Find the origin
	if (ent == &r_refScene.worldEntity)
		VectorCopy (surf->q3_origin, origin);
	else {
		Matrix3_TransformVector (ent->axis, surf->q3_origin, origin);
		VectorAdd (origin, ent->origin, origin);
	}

	// Check if it's behind the camera
	if ((origin[0]-r_refDef.viewOrigin[0])*r_refDef.forwardVec[0]
	+ (origin[1]-r_refDef.viewOrigin[1])*r_refDef.forwardVec[1]
	+ (origin[2]-r_refDef.viewOrigin[2])*r_refDef.forwardVec[2] < 0) {
		r_refStats.cullRadius[qTrue]++;
		return qTrue;
	}
	r_refStats.cullRadius[qFalse]++;

	// Radius cull
	if (clipFlags && R_CullSphere (origin, 1, clipFlags))
		return qTrue;

	// Visible
	return qFalse;
}


/*
================
R_CullQ3SurfacePlanar
================
*/
static qBool R_CullQ3SurfacePlanar (mBspSurface_t *surf, shader_t *shader, vec3_t modelOrigin)
{
	float	dot;

	// Check if culling is disabled
	if (!r_facePlaneCull->integer
	|| VectorCompare (surf->q3_origin, vec3Origin)
	|| shader->cullType == SHADER_CULL_NONE)
		return qFalse;

	// Plane culling
	if (surf->q3_origin[0] == 1.0f)
		dot = modelOrigin[0] - surf->q3_mesh->vertexArray[0][0];
	else if (surf->q3_origin[1] == 1.0f)
		dot = modelOrigin[1] - surf->q3_mesh->vertexArray[0][1];
	else if (surf->q3_origin[2] == 1.0f)
		dot = modelOrigin[2] - surf->q3_mesh->vertexArray[0][2];
	else
		dot = (modelOrigin[0] - surf->q3_mesh->vertexArray[0][0]) * surf->q3_origin[0]
			+ (modelOrigin[1] - surf->q3_mesh->vertexArray[0][1]) * surf->q3_origin[1]
			+ (modelOrigin[2] - surf->q3_mesh->vertexArray[0][2]) * surf->q3_origin[2];

	if (shader->cullType == SHADER_CULL_FRONT || r_refScene.mirrorView) {
		if (dot <= SMALL_EPSILON) {
			r_refStats.cullPlanar[qTrue]++;
			return qTrue;
		}
	}
	else {
		if (dot >= -SMALL_EPSILON) {
			r_refStats.cullPlanar[qTrue]++;
			return qTrue;
		}
	}

	r_refStats.cullPlanar[qFalse]++;
	return qFalse;
}


/*
=============
R_CullQ3SurfaceBounds
=============
*/
#define R_CullQ3SurfaceBounds(surf,clipFlags) R_CullBox((surf)->mins,(surf)->maxs,(clipFlags))


/*
=============
R_MarkQ3Surfaces
=============
*/
static void R_MarkQ3Surfaces (mBspNode_t *node)
{
	mBspSurface_t	**mark, *surf;
	mBspLeaf_t		*leaf;

	for ( ; ; ) {
		if (node->visFrame != r_refScene.visFrameCount)
			return;
		if (!node->plane)
			break;

		R_MarkQ3Surfaces (node->children[0]);
		node = node->children[1];
	}

	// If a leaf node, draw stuff
	leaf = (mBspLeaf_t *)node;
	if (!leaf->q3_firstVisSurface)
		return;

	// Check for door connected areas
	if (r_refDef.areaBits) {
		if(!(r_refDef.areaBits[leaf->area>>3] & (1<<(leaf->area&7))))
			return;		// Not visible
	}

	mark = leaf->q3_firstVisSurface;
	do {
		surf = *mark++;

		// See if it's been touched, if not, touch it
		if (surf->q3_visFrame == r_refScene.frameCount)
			continue;	// Already touched this frame
		surf->q3_visFrame = r_refScene.frameCount;

		// Sky surface
		if (surf->q3_shaderRef->shader->flags & SHADER_SKY) {
			// See if there's room
			if (r_numSkySurfs >= Q3BSP_MAX_VIS) {
				Com_Printf (PRNT_WARNING, "R_MarkQ3Surfaces: uh oh\n");
				break;
			}

			// Add to list
			r_skySurfs[r_numSkySurfs++] = surf;
			continue;
		}

		// See if there's room
		if (r_numVisSurfs >= Q3BSP_MAX_VIS) {
			Com_Printf (PRNT_WARNING, "R_MarkQ3Surfaces: uh oh\n");
			break;
		}

		// Add to list
		r_visSurfs[r_numVisSurfs++] = surf;
	} while (*mark);
}


/*
=============
R_MarkQ3Leaves
=============
*/
static void R_MarkQ3Leaves (void)
{
	byte		*vis;
	int			i;
	mBspLeaf_t	*leaf;
	mBspNode_t	*node;
	int			cluster;

	// If this is true, it's because of a map change
	// Map changes should for a visibility set update
	r_visChanged = !r_refScene.visFrameCount;

	// Current viewcluster
	if (r_refScene.worldModel && !r_refScene.mirrorView) {
		if (r_refScene.portalView) {
			r_refScene.oldViewCluster = -1;
			leaf = R_PointInQ3BSPLeaf (r_refScene.portalOrigin, r_refScene.worldModel);
		}
		else {
			r_refScene.oldViewCluster = r_refScene.viewCluster;
			leaf = R_PointInQ3BSPLeaf (r_refDef.viewOrigin, r_refScene.worldModel);
		}

		r_refScene.viewCluster = leaf->cluster;
	}

	if (!r_noVis->integer && !r_visChanged
	&& r_refScene.viewCluster == r_refScene.oldViewCluster
	&& r_refScene.viewCluster != -1)
		return;

	// Development aid to let you run around and see exactly where the pvs ends
	if (gl_lockpvs->integer)
		return;

	r_refScene.visFrameCount++;
	r_refScene.oldViewCluster = r_refScene.viewCluster;

	// Update visibility array
	r_visChanged = qTrue;

	if (r_noVis->integer || r_refScene.viewCluster == -1 || !r_refScene.worldModel->q3BspModel.vis) {
		// Mark everything
		for (i=0 ; i<r_refScene.worldModel->bspModel.numLeafs ; i++)
			r_refScene.worldModel->bspModel.leafs[i].visFrame = r_refScene.visFrameCount;
		for (i=0 ; i<r_refScene.worldModel->bspModel.numNodes ; i++)
			r_refScene.worldModel->bspModel.nodes[i].visFrame = r_refScene.visFrameCount;
		return;
	}

	vis = R_Q3BSPClusterPVS (r_refScene.viewCluster, r_refScene.worldModel);
	for (i=0, leaf=r_refScene.worldModel->bspModel.leafs ; i<r_refScene.worldModel->bspModel.numLeafs ; i++, leaf++) {
		cluster = leaf->cluster;
		if (cluster == -1)
			continue;

		if (vis[cluster>>3] & (1<<(cluster&7))) {
			node = (mBspNode_t *)leaf;
			do {
				if (node->visFrame == r_refScene.visFrameCount)
					break;
				node->visFrame = r_refScene.visFrameCount;
				node = node->parent;
			} while (node);
		}
	}
}


/*
=============
R_DrawQ3WorldList
=============
*/
static void R_DrawQ3WorldList (qBool cull)
{
	mBspSurface_t	*surf;
	uint32			i;

	// FIXME: these could be properly sorted by node, and frustum culling of nodes will be usable again

	if (cull) {
		// Clip sky surfaces
		for (i=0 ; i<r_numSkySurfs ; i++) {
			surf = r_skySurfs[i];
			if (R_CullQ3SurfacePlanar (surf, surf->q3_shaderRef->shader, r_refDef.viewOrigin))
				continue;
			if (R_CullQ3SurfaceBounds (surf, 15))
				continue;

			R_ClipSkySurface (surf);
		}

		// Add world surfaces
		for (i=0 ; i<r_numVisSurfs ; i++) {
			surf = r_visSurfs[i];

			switch (surf->q3_faceType) {
			case FACETYPE_FLARE:
				if (R_CullQ3FlareSurface (surf, &r_refScene.worldEntity, 15))
					break;

				R_AddQ3Surface (surf, &r_refScene.worldEntity, MBT_Q3BSP_FLARE);
				break;

			case FACETYPE_PLANAR:
				if (R_CullQ3SurfacePlanar (surf, surf->q3_shaderRef->shader, r_refDef.viewOrigin))
					break;
				// FALL THROUGH
			default:
				if (R_CullQ3SurfaceBounds (surf, 15))
					continue;

				R_AddQ3Surface (surf, &r_refScene.worldEntity, MBT_Q3BSP);
				break;
			}
		}
	}
	else {
		// Clip sky surfaces
		for (i=0 ; i<r_numSkySurfs ; i++) {
			surf = r_skySurfs[i];
			R_ClipSkySurface (surf);
		}

		// Add world surfaces
		for (i=0 ; i<r_numVisSurfs ; i++) {
			surf = r_visSurfs[i];

			switch (surf->q3_faceType) {
			case FACETYPE_FLARE:
				if (!r_flares->integer || !r_flareFade->value)
					break;

				R_AddQ3Surface (surf, &r_refScene.worldEntity, MBT_Q3BSP_FLARE);
				break;

			default:
				R_AddQ3Surface (surf, &r_refScene.worldEntity, MBT_Q3BSP);
				break;
			}
		}
	}
}


/*
=============
R_AddQ3WorldToList
=============
*/
void R_AddQ3WorldToList (void)
{
	R_MarkQ3Leaves ();

	if (!r_drawworld->integer)
		return;

	if (r_visChanged) {
		r_visChanged = qFalse;
		r_numVisSurfs = 0;
		r_numSkySurfs = 0;
		R_MarkQ3Surfaces (r_refScene.worldModel->bspModel.nodes);
	}

	R_Q3BSP_MarkLights ();
	R_DrawQ3WorldList (!(r_noCull->integer));
}

/*
=============================================================================

	BRUSH MODEL

=============================================================================
*/

/*
=================
R_AddQ3BrushModel
=================
*/
void R_AddQ3BrushModel (entity_t *ent)
{
	mBspSurface_t	*surf;
	vec3_t			mins, maxs;
	vec3_t			origin, temp;
	int				i;

	// No surfaces
	if (ent->model->bspModel.numModelSurfaces == 0)
		return;

	// Cull
	VectorSubtract (r_refDef.viewOrigin, ent->origin, origin);
	if (!r_noCull->integer) {
		if (!Matrix3_Compare (ent->axis, axisIdentity)) {
			mins[0] = ent->origin[0] - ent->model->radius * ent->scale;
			mins[1] = ent->origin[1] - ent->model->radius * ent->scale;
			mins[2] = ent->origin[2] - ent->model->radius * ent->scale;

			maxs[0] = ent->origin[0] + ent->model->radius * ent->scale;
			maxs[1] = ent->origin[1] + ent->model->radius * ent->scale;
			maxs[2] = ent->origin[2] + ent->model->radius * ent->scale;

			if (R_CullSphere (ent->origin, ent->model->radius, 15))
				return;

			VectorCopy (origin, temp);
			Matrix3_TransformVector (ent->axis, temp, origin);
		}
		else {
			// Calculate bounds
			VectorMA (ent->origin, ent->scale, ent->model->mins, mins);
			VectorMA (ent->origin, ent->scale, ent->model->maxs, maxs);

			if (R_CullBox (mins, maxs, 15))
				return;
		}
	}

	// Mark lights
	R_Q3BSP_MarkLightsBModel (ent, mins, maxs);

	// Draw the surfaces
	surf = ent->model->bspModel.firstModelSurface;
	for (i=0 ; i<ent->model->bspModel.numModelSurfaces ; i++, surf++) {
		// See if it's been touched, if not, touch it
		if (surf->q3_visFrame == r_refScene.frameCount)
			continue;	// Already touched this frame
		surf->q3_visFrame = r_refScene.frameCount;

		// These aren't drawn here, ever.
		if (surf->q3_shaderRef->flags & SHREF_SKY)
			continue;

		// See if it's visible
		if (!R_Q3BSP_SurfPotentiallyVisible (surf))
			continue;

		// Cull
		switch (surf->q3_faceType) {
		case FACETYPE_FLARE:
			if (!r_noCull->integer && R_CullQ3FlareSurface (surf, ent, 0))
				break;

			R_AddQ3Surface (surf, ent, MBT_Q3BSP_FLARE);
			break;

		case FACETYPE_PLANAR:
			if (!r_noCull->integer && R_CullQ3SurfacePlanar (surf, surf->q3_shaderRef->shader, origin))
				break;
			// FALL THROUGH
		default:
			R_AddQ3Surface (surf, ent, MBT_Q3BSP);
			break;
		}
	}
}
