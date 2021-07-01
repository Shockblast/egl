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
// r_world.c
// World surface related refresh code
//

#include "r_local.h"

/*
=============================================================================

	WORLD MODEL

=============================================================================
*/

/*
================
R_AddQ2Surface
================
*/
static void R_AddQ2Surface (mBspSurface_t *surf, mQ2BspTexInfo_t *texInfo, entity_t *entity)
{
	meshBuffer_t	*mb;

	// Add to list
	mb = R_AddMeshToList (texInfo->shader, entity->shaderTime, entity, NULL, MBT_Q2BSP, surf);
	if (!mb)
		return;

	// Caustics
	if (surf->lmTexNum && r_caustics->integer && surf->q2_flags & SURF_UNDERWATER) {
		if (surf->q2_flags & SURF_LAVA && glMedia.worldLavaCaustics)
			R_AddMeshToList (glMedia.worldLavaCaustics, entity->shaderTime, entity, NULL, MBT_Q2BSP, surf);
		else if (surf->q2_flags & SURF_SLIME && glMedia.worldSlimeCaustics)
			R_AddMeshToList (glMedia.worldSlimeCaustics, entity->shaderTime, entity, NULL, MBT_Q2BSP, surf);
		else if (glMedia.worldWaterCaustics)
			R_AddMeshToList (glMedia.worldWaterCaustics, entity->shaderTime, entity, NULL, MBT_Q2BSP, surf);
	}
}


/*
================
R_Q2SurfShader
================
*/
static inline mQ2BspTexInfo_t *R_Q2SurfShader (mBspSurface_t *surf)
{
	mQ2BspTexInfo_t	*texInfo;
	int				i;

	// Doesn't animate
	if (!surf->q2_texInfo->next)
		return surf->q2_texInfo;

	// Animates
	texInfo = surf->q2_texInfo;
	for (i=((int)(r_refDef.time * 2))%texInfo->numFrames ; i ; i--)
		texInfo = texInfo->next;

	return texInfo;
}


/*
================
R_CullQ2SurfacePlanar
================
*/
static qBool R_CullQ2SurfacePlanar (mBspSurface_t *surf, shader_t *shader, float dist)
{
	// See if it's been touched, if not, touch it
	if (surf->q2_surfFrame == r_refScene.frameCount)
		return qTrue;	// Already touched this frame
	surf->q2_surfFrame = r_refScene.frameCount;

	// Check if it even has any passes
	if (!shader || !shader->numPasses)
		return qTrue;

	// Side culling
	if (r_facePlaneCull->integer) {
		switch (shader->cullType) {
		case SHADER_CULL_BACK:
			if (surf->q2_flags & SURF_PLANEBACK) {
				if (dist <= SMALL_EPSILON) {
					r_refStats.cullPlanar[qTrue]++;
					return qTrue;	// Wrong side
				}
			}
			else {
				if (dist >= -SMALL_EPSILON) {
					r_refStats.cullPlanar[qTrue]++;
					return qTrue;	// Wrong side
				}
			}
			break;

		case SHADER_CULL_FRONT:
			if (surf->q2_flags & SURF_PLANEBACK) {
				if (dist >= -SMALL_EPSILON) {
					r_refStats.cullPlanar[qTrue]++;
					return qTrue;	// Wrong side
				}
			}
			else {
				if (dist <= SMALL_EPSILON) {
					r_refStats.cullPlanar[qTrue]++;
					return qTrue;	// Wrong side
				}
			}
			break;
		}
	}

	r_refStats.cullPlanar[qFalse]++;
	return qFalse;
}


/*
================
R_CullQ2SurfaceBounds
================
*/
#define R_CullQ2SurfaceBounds(surf,clipFlags) R_CullBox((surf)->mins,(surf)->maxs,(clipFlags))


/*
===============
R_MarkQ2Leaves

Mark the leaves and nodes that are in the PVS for the current cluster
===============
*/
static void R_MarkQ2Leaves (void)
{
	static int	oldViewCluster2;
	static int	viewCluster2;
	byte		*vis, fatVis[Q2BSP_MAX_VIS];
	int			i, c;
	mBspNode_t	*node;
	mBspLeaf_t	*leaf;
	vec3_t		temp;

	// Current viewcluster
	VectorCopy (r_refDef.viewOrigin, temp);

	r_refScene.oldViewCluster = r_refScene.viewCluster;
	oldViewCluster2 = viewCluster2;

	leaf = R_PointInQ2BSPLeaf (r_refDef.viewOrigin, r_refScene.worldModel);
	r_refScene.viewCluster = viewCluster2 = leaf->cluster;

	// Check above and below so crossing solid water doesn't draw wrong
	if (!leaf->q2_contents) {
		// Look down a bit
		temp[2] -= 16;
		leaf = R_PointInQ2BSPLeaf (temp, r_refScene.worldModel);
		if (!(leaf->q2_contents & CONTENTS_SOLID) && leaf->cluster != viewCluster2)
			viewCluster2 = leaf->cluster;
	}
	else {
		// Look up a bit
		temp[2] += 16;
		leaf = R_PointInQ2BSPLeaf (temp, r_refScene.worldModel);
		if (!(leaf->q2_contents & CONTENTS_SOLID) && leaf->cluster != viewCluster2)
			viewCluster2 = leaf->cluster;
	}

	if (r_refScene.oldViewCluster == r_refScene.viewCluster &&
		oldViewCluster2 == viewCluster2 &&
		!r_noVis->integer && r_refScene.viewCluster != -1)
		return;

	// Development aid to let you run around and see exactly where the pvs ends
	if (gl_lockpvs->integer)
		return;

	r_refScene.visFrameCount++;
	r_refScene.oldViewCluster = r_refScene.viewCluster;
	oldViewCluster2 = viewCluster2;

	if (r_noVis->integer || r_refScene.viewCluster == -1 || !r_refScene.worldModel->q2BspModel.vis) {
		// Mark everything
		for (i=0 ; i<r_refScene.worldModel->bspModel.numLeafs ; i++)
			r_refScene.worldModel->bspModel.leafs[i].visFrame = r_refScene.visFrameCount;
		for (i=0 ; i<r_refScene.worldModel->bspModel.numNodes ; i++)
			r_refScene.worldModel->bspModel.nodes[i].visFrame = r_refScene.visFrameCount;
		return;
	}

	vis = R_Q2BSPClusterPVS (r_refScene.viewCluster, r_refScene.worldModel);

	// May have to combine two clusters because of solid water boundaries
	if (viewCluster2 != r_refScene.viewCluster) {
		memcpy (fatVis, vis, (r_refScene.worldModel->bspModel.numLeafs+7)/8);
		vis = R_Q2BSPClusterPVS (viewCluster2, r_refScene.worldModel);
		c = (r_refScene.worldModel->bspModel.numLeafs+31)/32;
		for (i=0 ; i<c ; i++)
			((int *)fatVis)[i] |= ((int *)vis)[i];
		vis = fatVis;
	}

	for (i=0, leaf=r_refScene.worldModel->bspModel.leafs ; i<r_refScene.worldModel->bspModel.numLeafs ; i++, leaf++) {
		if (leaf->cluster == -1)
			continue;
		if (!(vis[leaf->cluster>>3] & (1<<(leaf->cluster&7))))
			continue;

		node = (mBspNode_t *)leaf;
		do {
			if (node->visFrame == r_refScene.visFrameCount)
				break;

			node->visFrame = r_refScene.visFrameCount;
			node = node->parent;
		} while (node);
	}
}


/*
================
R_RecursiveQ2WorldNode
================
*/
static void R_RecursiveQ2WorldNode (mBspNode_t *node, int clipFlags)
{
	cBspPlane_t		*plane;
	mBspSurface_t	*surf, **mark;
	mBspLeaf_t		*leaf;
	mQ2BspTexInfo_t	*texInfo;
	int				side, clipped, i;
	float			dist;

	if (node->q2_contents == CONTENTS_SOLID)
		return;		// Solid
	if (node->visFrame != r_refScene.visFrameCount)
		return;		// Node not visible this frame

	// Cull
	if (clipFlags) {
		for (i=0, plane=r_refScene.viewFrustum ; i<4 ; i++, plane++) {
			if (!(clipFlags & (1<<i)))
				continue;

			clipped = BoxOnPlaneSide (node->mins, node->maxs, plane);
			switch (clipped) {
			case 1:
				clipFlags &= ~(1<<i);
				break;

			case 2:
				r_refStats.cullBounds[qTrue]++;
				return;
			}
		}
		r_refStats.cullBounds[qFalse]++;
	}

	// If a leaf node, draw stuff
	if (node->q2_contents != -1) {
		leaf = (mBspLeaf_t *)node;

		// Check for door connected areas
		if (r_refDef.areaBits) {
			if (!(r_refDef.areaBits[leaf->area>>3] & (1<<(leaf->area&7))))
				return;		// not visible
		}

		// Mark visible surfaces
		mark = leaf->q2_firstMarkSurface;
		for (i=leaf->q2_numMarkSurfaces ; i ; ) {
			do {
				(*mark)->q2_surfFrame = 0;
				mark++;
			} while (--i);
		}

		return;
	}

	// Node is just a decision point, so go down the apropriate sides
	plane = node->plane;

	// Find which side of the node we are on
	if (plane->type < 3)
		dist = r_refDef.viewOrigin[plane->type] - plane->dist;
	else
		dist = DotProduct (r_refDef.viewOrigin, plane->normal) - plane->dist;
	side = (dist >= 0) ? 0 : 1;

	// Recurse down the children, back side first
	R_RecursiveQ2WorldNode (node->children[!side], clipFlags);

	// Draw stuff
	for (i=node->q2_numSurfaces, surf=r_refScene.worldModel->bspModel.surfaces+node->q2_firstSurface ; i ; i--, surf++) {
		// Get the shader
		texInfo = R_Q2SurfShader (surf);

		// Cull
		if (R_CullQ2SurfacePlanar (surf, texInfo->shader, dist))
			continue;
		if (R_CullQ2SurfaceBounds (surf, clipFlags))
			continue;

		// Sky surface
		if (surf->q2_texInfo->flags & SURF_TEXINFO_SKY) {
			R_ClipSkySurface (surf);
			continue;
		}

		// World surface
		R_AddQ2Surface (surf, texInfo, &r_refScene.worldEntity);
	}

	// Recurse down the front side
	R_RecursiveQ2WorldNode (node->children[side], clipFlags);
}


/*
=============
R_AddWorldToList
=============
*/
void R_AddWorldToList (void)
{
	R_ClearSky ();

	if (r_refDef.rdFlags & RDF_NOWORLDMODEL)
		return;

	if (r_refScene.worldModel && r_refScene.worldModel->type == MODEL_Q3BSP) {
		R_AddQ3WorldToList ();
		return;
	}

	R_MarkQ2Leaves ();

	if (!r_drawworld->integer)
		return;

	R_Q2BSP_PushDLights ();
	R_RecursiveQ2WorldNode (r_refScene.worldModel->bspModel.nodes, (r_noCull->integer) ? 0 : 15);
	R_AddSkyToList ();
}

/*
=============================================================================

	BRUSH MODEL

=============================================================================
*/

/*
=================
R_AddQ2BrushModel
=================
*/
void R_AddQ2BrushModel (entity_t *ent)
{
	mBspSurface_t	*surf;
	mQ2BspTexInfo_t	*texInfo;
	vec3_t			mins, maxs;
	vec3_t			origin, temp;
	float			dist;
	int				i;

	// No surfaces
	if (!ent->model->bspModel.numModelSurfaces)
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

	// Calculate dynamic lighting for bmodel
	if (!gl_flashblend->integer) {
		dLight_t		*lt;
		mBspNode_t		*node;

		node = ent->model->bspModel.nodes + ent->model->q2BspModel.firstNode;
		for (lt=r_refScene.dLightList, i=0 ; i<r_refScene.numDLights ; i++, lt++) {
			if (!BoundsAndSphereIntersect (mins, maxs, lt->origin, lt->intensity))
				continue;

			R_Q2BSP_MarkLights (ent->model, node, lt, 1<<i, qFalse);
		}
	}

	// Draw the surfaces
	surf = ent->model->bspModel.firstModelSurface;
	for (i=0 ; i<ent->model->bspModel.numModelSurfaces ; i++, surf++) {
		// These aren't drawn here, ever.
		if (surf->q2_texInfo->flags & SURF_TEXINFO_SKY)
			continue;

		// Find which side of the node we are on
		if (surf->q2_plane->type < 3)
			dist = origin[surf->q2_plane->type] - surf->q2_plane->dist;
		else
			dist = DotProduct (origin, surf->q2_plane->normal) - surf->q2_plane->dist;

		// Get the shader
		texInfo = R_Q2SurfShader (surf);

		// Cull
		if (R_CullQ2SurfacePlanar (surf, texInfo->shader, dist))
			continue;

		// World surface
		R_AddQ2Surface (surf, texInfo, ent);
	}
}

/*
=============================================================================

	INIT / SHUTDOWN

=============================================================================
*/

/*
==================
R_WorldInit
==================
*/
void R_WorldInit (void)
{
	R_SkyInit ();
}


/*
==================
R_WorldShutdown
==================
*/
void R_WorldShutdown (void)
{
	R_SkyShutdown ();
}
