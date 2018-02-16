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
// rf_world.c
// World surface related refresh code
//

#include "rf_local.h"

/*
=============================================================================

	QUAKE II WORLD MODEL

=============================================================================
*/

/*
================
R_AddQ2Surface
================
*/
static void R_AddQ2Surface (mBspSurface_t *surf, mQ2BspTexInfo_t *texInfo, refEntity_t *entity)
{
	meshBuffer_t	*mb;

	surf->visFrame = ri.frameCount;

	// Add to list
	mb = R_AddMeshToList (texInfo->shader, entity->shaderTime, entity, NULL, MBT_Q2BSP, surf);
	if (!mb)
		return;

	// Caustics
	if (surf->lmTexNum && r_caustics->intVal && surf->q2_flags & SURF_UNDERWATER) {
		if (surf->q2_flags & SURF_LAVA && ri.media.worldLavaCaustics)
			R_AddMeshToList (ri.media.worldLavaCaustics, entity->shaderTime, entity, NULL, MBT_Q2BSP, surf);
		else if (surf->q2_flags & SURF_SLIME && ri.media.worldSlimeCaustics)
			R_AddMeshToList (ri.media.worldSlimeCaustics, entity->shaderTime, entity, NULL, MBT_Q2BSP, surf);
		else if (ri.media.worldWaterCaustics)
			R_AddMeshToList (ri.media.worldWaterCaustics, entity->shaderTime, entity, NULL, MBT_Q2BSP, surf);
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
	for (i=((int)(ri.def.time * 2))%texInfo->numFrames ; i ; i--)
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
	// Side culling
	if (r_facePlaneCull->intVal) {
		switch (shader->cullType) {
		case SHADER_CULL_BACK:
			if (surf->q2_flags & SURF_PLANEBACK) {
				if (dist <= SMALL_EPSILON) {
					ri.pc.cullPlanar[CULL_PASS]++;
					return qTrue;	// Wrong side
				}
			}
			else {
				if (dist >= -SMALL_EPSILON) {
					ri.pc.cullPlanar[CULL_PASS]++;
					return qTrue;	// Wrong side
				}
			}
			break;

		case SHADER_CULL_FRONT:
			if (surf->q2_flags & SURF_PLANEBACK) {
				if (dist >= -SMALL_EPSILON) {
					ri.pc.cullPlanar[CULL_PASS]++;
					return qTrue;	// Wrong side
				}
			}
			else {
				if (dist <= SMALL_EPSILON) {
					ri.pc.cullPlanar[CULL_PASS]++;
					return qTrue;	// Wrong side
				}
			}
			break;
		}
	}

	ri.pc.cullPlanar[CULL_FAIL]++;
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
	Vec3Copy (ri.def.viewOrigin, temp);

	ri.scn.oldViewCluster = ri.scn.viewCluster;
	oldViewCluster2 = viewCluster2;

	leaf = R_PointInQ2BSPLeaf (ri.def.viewOrigin, ri.scn.worldModel);
	ri.scn.viewCluster = viewCluster2 = leaf->cluster;

	// Check above and below so crossing solid water doesn't draw wrong
	if (!leaf->c.q2_contents) {
		// Look down a bit
		temp[2] -= 16;
		leaf = R_PointInQ2BSPLeaf (temp, ri.scn.worldModel);
		if (!(leaf->c.q2_contents & CONTENTS_SOLID) && leaf->cluster != viewCluster2)
			viewCluster2 = leaf->cluster;
	}
	else {
		// Look up a bit
		temp[2] += 16;
		leaf = R_PointInQ2BSPLeaf (temp, ri.scn.worldModel);
		if (!(leaf->c.q2_contents & CONTENTS_SOLID) && leaf->cluster != viewCluster2)
			viewCluster2 = leaf->cluster;
	}

	if (ri.def.areaChanged)
		ri.def.areaChanged = qFalse;
	else if (ri.scn.oldViewCluster == ri.scn.viewCluster
	&& oldViewCluster2 == viewCluster2
	&& !r_noVis->intVal && ri.scn.viewCluster != -1)
		return;

	// Development aid to let you run around and see exactly where the pvs ends
	if (gl_lockpvs->intVal)
		return;

	ri.scn.visFrameCount++;
	ri.scn.oldViewCluster = ri.scn.viewCluster;
	oldViewCluster2 = viewCluster2;

	if (r_noVis->intVal || ri.scn.viewCluster == -1 || !ri.scn.worldModel->q2BspModel.vis) {
		// Mark everything
		for (i=0 ; i<ri.scn.worldModel->bspModel.numLeafs ; i++)
			ri.scn.worldModel->bspModel.leafs[i].c.visFrame = ri.scn.visFrameCount;
		for (i=0 ; i<ri.scn.worldModel->bspModel.numNodes ; i++)
			ri.scn.worldModel->bspModel.nodes[i].c.visFrame = ri.scn.visFrameCount;
		return;
	}

	vis = R_Q2BSPClusterPVS (ri.scn.viewCluster, ri.scn.worldModel);

	// May have to combine two clusters because of solid water boundaries
	if (viewCluster2 != ri.scn.viewCluster) {
		memcpy (fatVis, vis, (ri.scn.worldModel->bspModel.numLeafs+7)/8);
		vis = R_Q2BSPClusterPVS (viewCluster2, ri.scn.worldModel);
		c = (ri.scn.worldModel->bspModel.numLeafs+31)/32;
		for (i=0 ; i<c ; i++)
			((int *)fatVis)[i] |= ((int *)vis)[i];
		vis = fatVis;
	}

	for (i=0, leaf=ri.scn.worldModel->bspModel.leafs ; i<ri.scn.worldModel->bspModel.numLeafs ; i++, leaf++) {
		if (leaf->cluster == -1)
			continue;
		if (!(vis[leaf->cluster>>3] & (1<<(leaf->cluster&7))))
			continue;

		// Check for door connected areas
		if (ri.def.areaBits) {
			if (!(ri.def.areaBits[leaf->area>>3] & (1<<(leaf->area&7))))
				continue;		// Not visible
		}

		node = (mBspNode_t *)leaf;
		do {
			if (node->c.visFrame == ri.scn.visFrameCount)
				break;

			node->c.visFrame = ri.scn.visFrameCount;
			node = node->c.parent;
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
	cBspPlane_t		*p;
	mBspSurface_t	**mark, *surf;
	mQ2BspTexInfo_t	*texInfo;
	int				side, clipped, i;
	float			dist;

	if (node->c.q2_contents == CONTENTS_SOLID)
		return;		// Solid
	if (R_CullNode (node))
		return;		// Node not visible this frame

	// Cull
	if (clipFlags && !node->c.badBounds) {
		for (i=0, p=ri.scn.viewFrustum ; i<5 ; i++, p++) {
			if (!(clipFlags & (1<<i)))
				continue;

			clipped = BoxOnPlaneSide (node->c.mins, node->c.maxs, p);
			switch (clipped) {
			case 1:
				clipFlags &= ~(1<<i);
				break;

			case 2:
				ri.pc.cullBounds[CULL_PASS]++;
				return;
			}
		}
		ri.pc.cullBounds[CULL_FAIL]++;
	}

	// If a leaf node, draw stuff
	if (node->c.q2_contents != -1)
		return;

	// Node is just a decision point, so go down the apropriate sides
	// Find which side of the node we are on
	dist = PlaneDiff (ri.def.viewOrigin, node->c.plane);
	side = (dist >= 0) ? 0 : 1;

	// Recurse down the children, back side first
	R_RecursiveQ2WorldNode (node->children[!side], clipFlags);

	// Draw stuff
	if (node->q2_firstVisSurface) {
		mark = node->q2_firstVisSurface;
		do {
			surf = *mark++;

			// See if it's been touched
			if (surf->visFrame == ri.frameCount)
				continue;

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
			R_AddQ2Surface (surf, texInfo, ri.scn.worldEntity);
		} while (*mark);
	}

	// Recurse down the front side
	R_RecursiveQ2WorldNode (node->children[side], clipFlags);
}

/*
=============================================================================

	QUAKE II BRUSH MODELS

=============================================================================
*/

/*
=================
R_AddQ2BrushModel
=================
*/
void R_AddQ2BrushModel (refEntity_t *ent)
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
	Vec3Subtract (ri.def.viewOrigin, ent->origin, origin);
	if (!r_noCull->intVal) {
		if (!Matrix3_Compare (ent->axis, axisIdentity)) {
			mins[0] = ent->origin[0] - ent->model->radius * ent->scale;
			mins[1] = ent->origin[1] - ent->model->radius * ent->scale;
			mins[2] = ent->origin[2] - ent->model->radius * ent->scale;

			maxs[0] = ent->origin[0] + ent->model->radius * ent->scale;
			maxs[1] = ent->origin[1] + ent->model->radius * ent->scale;
			maxs[2] = ent->origin[2] + ent->model->radius * ent->scale;

			if (R_CullSphere (ent->origin, ent->model->radius, 31))
				return;

			Vec3Copy (origin, temp);
			Matrix3_TransformVector (ent->axis, temp, origin);
		}
		else {
			// Calculate bounds
			Vec3MA (ent->origin, ent->scale, ent->model->mins, mins);
			Vec3MA (ent->origin, ent->scale, ent->model->maxs, maxs);

			if (R_CullBox (mins, maxs, 31))
				return;
		}
	}

	// Calculate dynamic lighting for bmodel
	R_Q2BSP_MarkBModelLights (ent, mins, maxs);

	// Draw the surfaces
	surf = ent->model->bspModel.firstModelSurface;
	for (i=0 ; i<ent->model->bspModel.numModelSurfaces ; i++, surf++) {
		// See if it's been touched
		if (surf->visFrame == ri.frameCount)
			continue;

		// These aren't drawn here, ever.
		if (surf->q2_texInfo->flags & SURF_TEXINFO_SKY)
			continue;

		// Find which side of the node we are on
		dist = PlaneDiff (origin, surf->q2_plane);

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

	QUAKE III WORLD MODEL

=============================================================================
*/

static qBool			r_q3_visChanged;
static uint32			r_q3_numVisSurfs;
static mBspSurface_t	*r_q3_visSurfs[Q3BSP_MAX_LEAFFACES];
static uint32			r_q3_numSkySurfs;
static mBspSurface_t	*r_q3_skySurfs[Q3BSP_MAX_LEAFFACES];

/*
================
R_AddQ3Surface
================
*/
static void R_AddQ3Surface (mBspSurface_t *surf, refEntity_t *ent, meshType_t meshType)
{
	// Add to list
	R_AddMeshToList (surf->q3_shaderRef->shader, ent->shaderTime, ent, surf->q3_fog, meshType, surf);

	// Surface is used this frame
	surf->visFrame = ri.frameCount;
}


/*
================
R_CullQ3FlareSurface
================
*/
static qBool R_CullQ3FlareSurface (mBspSurface_t *surf, refEntity_t *ent, int clipFlags)
{
	vec3_t	origin;

	// Check if flares/culling are disabled
	if (!r_flares->intVal || !r_flareFade->floatVal)
		return qTrue;

	// Find the origin
	if (ent == ri.scn.worldEntity) {
		Vec3Copy (surf->q3_origin, origin);
	}
	else {
		Matrix3_TransformVector (ent->axis, surf->q3_origin, origin);
		Vec3Add (origin, ent->origin, origin);
	}

	// Check if it's behind the camera
	if ((origin[0]-ri.def.viewOrigin[0])*ri.def.viewAxis[0][0]
	+ (origin[1]-ri.def.viewOrigin[1])*ri.def.viewAxis[0][1]
	+ (origin[2]-ri.def.viewOrigin[2])*ri.def.viewAxis[0][2] < 0) {
		ri.pc.cullRadius[CULL_PASS]++;
		return qTrue;
	}
	ri.pc.cullRadius[CULL_FAIL]++;

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
	if (!r_facePlaneCull->intVal
	|| Vec3Compare (surf->q3_origin, vec3Origin)
	|| shader->cullType == SHADER_CULL_NONE)
		return qFalse;

	// Plane culling
	if (surf->q3_origin[0] == 1.0f)
		dot = modelOrigin[0] - surf->mesh->vertexArray[0][0];
	else if (surf->q3_origin[1] == 1.0f)
		dot = modelOrigin[1] - surf->mesh->vertexArray[0][1];
	else if (surf->q3_origin[2] == 1.0f)
		dot = modelOrigin[2] - surf->mesh->vertexArray[0][2];
	else
		dot = (modelOrigin[0] - surf->mesh->vertexArray[0][0]) * surf->q3_origin[0]
			+ (modelOrigin[1] - surf->mesh->vertexArray[0][1]) * surf->q3_origin[1]
			+ (modelOrigin[2] - surf->mesh->vertexArray[0][2]) * surf->q3_origin[2];

	if (shader->cullType == SHADER_CULL_FRONT || ri.scn.mirrorView) {
		if (dot <= SMALL_EPSILON) {
			ri.pc.cullPlanar[CULL_PASS]++;
			return qTrue;
		}
	}
	else {
		if (dot >= -SMALL_EPSILON) {
			ri.pc.cullPlanar[CULL_PASS]++;
			return qTrue;
		}
	}

	ri.pc.cullPlanar[CULL_FAIL]++;
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
		if (node->c.visFrame != ri.scn.visFrameCount)
			return;
		if (!node->c.plane)
			break;

		R_MarkQ3Surfaces (node->children[0]);
		node = node->children[1];
	}

	// If a leaf node, draw stuff
	leaf = (mBspLeaf_t *)node;
	if (!leaf->q3_firstVisSurface)
		return;

	// Check for door connected areas
	if (ri.def.areaBits) {
		if (!(ri.def.areaBits[leaf->area>>3] & (1<<(leaf->area&7))))
			return;		// Not visible
	}

	mark = leaf->q3_firstVisSurface;
	do {
		surf = *mark++;

		// See if it's been touched, if not, touch it
		if (surf->q3_nodeFrame == ri.scn.visFrameCount)
			continue;	// Already touched this frame
		surf->q3_nodeFrame = ri.scn.visFrameCount;

		// Sky surface
		if (surf->q3_shaderRef->shader->flags & SHADER_SKY) {
			// See if there's room
			if (r_q3_numSkySurfs >= Q3BSP_MAX_LEAFFACES) {
				Com_Printf (PRNT_WARNING, "R_MarkQ3Surfaces: hit max surface count!\n");
				break;
			}

			// Add to list
			r_q3_skySurfs[r_q3_numSkySurfs++] = surf;
			continue;
		}

		// See if there's room
		if (r_q3_numVisSurfs >= Q3BSP_MAX_LEAFFACES) {
			Com_Printf (PRNT_WARNING, "R_MarkQ3Surfaces: hit max surface count!\n");
			break;
		}

		// Add to list
		r_q3_visSurfs[r_q3_numVisSurfs++] = surf;
	} while (*mark);
}


/*
=============
R_MarkQ3Leaves
=============
*/
static void R_MarkQ3Leaves (void)
{
	byte			*vis;
	int				i;
	mBspLeaf_t		*leaf;
	mBspNode_t		*node;
	int				cluster;
	// If this is true, it's because of a map change
	// Map changes should for a visibility set update
	r_q3_visChanged = !ri.scn.visFrameCount;

	// Current viewcluster
	if (ri.scn.worldModel && !ri.scn.mirrorView) {
		if (ri.scn.portalView) {
			ri.scn.oldViewCluster = -1;
			leaf = R_PointInQ3BSPLeaf (ri.scn.portalOrigin, ri.scn.worldModel);
		}
		else {
			ri.scn.oldViewCluster = ri.scn.viewCluster;
			leaf = R_PointInQ3BSPLeaf (ri.def.viewOrigin, ri.scn.worldModel);
		}

		ri.scn.viewCluster = leaf->cluster;
	}

	if (ri.def.areaChanged)
		ri.def.areaChanged = qFalse;
	else if (!r_noVis->intVal && !r_q3_visChanged
	&& ri.scn.viewCluster == ri.scn.oldViewCluster
	&& ri.scn.viewCluster != -1)
		return;

	// Development aid to let you run around and see exactly where the pvs ends
	if (gl_lockpvs->intVal)
		return;

	ri.scn.visFrameCount++;
	ri.scn.oldViewCluster = ri.scn.viewCluster;

	// Update visibility array
	r_q3_visChanged = qTrue;

	if (r_noVis->intVal || ri.scn.viewCluster == -1 || !ri.scn.worldModel->q3BspModel.vis) {
		// Mark everything
		for (i=0 ; i<ri.scn.worldModel->bspModel.numLeafs ; i++)
			ri.scn.worldModel->bspModel.leafs[i].c.visFrame = ri.scn.visFrameCount;
		for (i=0 ; i<ri.scn.worldModel->bspModel.numNodes ; i++)
			ri.scn.worldModel->bspModel.nodes[i].c.visFrame = ri.scn.visFrameCount;
		return;
	}

	vis = R_Q3BSPClusterPVS (ri.scn.viewCluster, ri.scn.worldModel);
	for (i=0, leaf=ri.scn.worldModel->bspModel.leafs ; i<ri.scn.worldModel->bspModel.numLeafs ; i++, leaf++) {
		cluster = leaf->cluster;
		if (cluster == -1)
			continue;

		if (vis[cluster>>3] & (1<<(cluster&7))) {
			node = (mBspNode_t *)leaf;
			do {
				if (node->c.visFrame == ri.scn.visFrameCount)
					break;
				node->c.visFrame = ri.scn.visFrameCount;
				node = node->c.parent;
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

	// No culling
	if (!cull) {
		// Clip sky surfaces
		for (i=0 ; i<r_q3_numSkySurfs ; i++) {
			surf = r_q3_skySurfs[i];
			R_ClipSkySurface (surf);
		}

		// Add world surfaces
		for (i=0 ; i<r_q3_numVisSurfs ; i++) {
			surf = r_q3_visSurfs[i];

			switch (surf->q3_faceType) {
			case FACETYPE_FLARE:
				if (!r_flares->intVal || !r_flareFade->floatVal)
					continue;

				R_AddQ3Surface (surf, ri.scn.worldEntity, MBT_Q3BSP_FLARE);
				break;

			default:
				R_AddQ3Surface (surf, ri.scn.worldEntity, MBT_Q3BSP);
				break;
			}
		}

		return;
	}

	// FIXME: these could be properly sorted by node, and frustum culling of nodes will be usable again

	// Clip sky surfaces
	for (i=0 ; i<r_q3_numSkySurfs ; i++) {
		surf = r_q3_skySurfs[i];
		if (R_CullQ3SurfacePlanar (surf, surf->q3_shaderRef->shader, ri.def.viewOrigin))
			continue;
		if (R_CullQ3SurfaceBounds (surf, 31))
			continue;

		R_ClipSkySurface (surf);
	}

	// Add world surfaces
	for (i=0 ; i<r_q3_numVisSurfs ; i++) {
		surf = r_q3_visSurfs[i];
		switch (surf->q3_faceType) {
		case FACETYPE_FLARE:
			if (R_CullQ3FlareSurface (surf, ri.scn.worldEntity, 31))
				continue;

			R_AddQ3Surface (surf, ri.scn.worldEntity, MBT_Q3BSP_FLARE);
			break;

		case FACETYPE_PLANAR:
			if (R_CullQ3SurfacePlanar (surf, surf->q3_shaderRef->shader, ri.def.viewOrigin))
				continue;
			// FALL THROUGH
		default:
			if (R_CullQ3SurfaceBounds (surf, 31))
				continue;

			R_AddQ3Surface (surf, ri.scn.worldEntity, MBT_Q3BSP);
			break;
		}
	}
}


/*
=============
R_AddQ3WorldToList
=============
*/
static void R_AddQ3WorldToList (void)
{
	uint32	startTime;

	if (r_times->intVal)
		startTime = Sys_UMilliseconds ();
	R_MarkQ3Leaves ();
	if (r_times->intVal)
		ri.pc.timeMarkLeaves += Sys_UMilliseconds () - startTime;

	if (!r_drawworld->intVal)
		return;

	if (r_q3_visChanged) {
		r_q3_visChanged = qFalse;
		r_q3_numVisSurfs = 0;
		r_q3_numSkySurfs = 0;
		R_MarkQ3Surfaces (ri.scn.worldModel->bspModel.nodes);
	}

	if (r_times->intVal)
		startTime = Sys_UMilliseconds ();
	R_Q3BSP_MarkWorldLights ();
	if (r_times->intVal)
		ri.pc.timeMarkLights += Sys_UMilliseconds () - startTime;

	if (r_times->intVal)
		startTime = Sys_UMilliseconds ();
	R_DrawQ3WorldList (!(r_noCull->intVal));
	if (r_times->intVal)
		ri.pc.timeRecurseWorld += Sys_UMilliseconds () - startTime;
}

/*
=============================================================================

	QUAKE III BRUSH MODELS

=============================================================================
*/

/*
=================
R_AddQ3BrushModel
=================
*/
void R_AddQ3BrushModel (refEntity_t *ent)
{
	mBspSurface_t	*surf;
	vec3_t			mins, maxs;
	vec3_t			origin, temp;
	int				i;

	// No surfaces
	if (ent->model->bspModel.numModelSurfaces == 0)
		return;

	// Cull
	Vec3Subtract (ri.def.viewOrigin, ent->origin, origin);
	if (!r_noCull->intVal) {
		if (!Matrix3_Compare (ent->axis, axisIdentity)) {
			mins[0] = ent->origin[0] - ent->model->radius * ent->scale;
			mins[1] = ent->origin[1] - ent->model->radius * ent->scale;
			mins[2] = ent->origin[2] - ent->model->radius * ent->scale;

			maxs[0] = ent->origin[0] + ent->model->radius * ent->scale;
			maxs[1] = ent->origin[1] + ent->model->radius * ent->scale;
			maxs[2] = ent->origin[2] + ent->model->radius * ent->scale;

			if (R_CullSphere (ent->origin, ent->model->radius, 31))
				return;

			Vec3Copy (origin, temp);
			Matrix3_TransformVector (ent->axis, temp, origin);
		}
		else {
			// Calculate bounds
			Vec3MA (ent->origin, ent->scale, ent->model->mins, mins);
			Vec3MA (ent->origin, ent->scale, ent->model->maxs, maxs);

			if (R_CullBox (mins, maxs, 31))
				return;
		}
	}

	// Mark lights
	R_Q3BSP_MarkBModelLights (ent, mins, maxs);

	// Draw the surfaces
	surf = ent->model->bspModel.firstModelSurface;
	for (i=0 ; i<ent->model->bspModel.numModelSurfaces ; i++, surf++) {
		// Don't draw twice
		if (surf->visFrame == ri.frameCount)
			continue;

		// These aren't drawn here, ever.
		if (surf->q3_shaderRef->flags & SHREF_SKY)
			continue;

		// See if it's visible
		if (!R_Q3BSP_SurfPotentiallyVisible (surf))
			continue;

		// Cull
		switch (surf->q3_faceType) {
		case FACETYPE_FLARE:
			if (!r_noCull->intVal && R_CullQ3FlareSurface (surf, ent, 0))
				continue;

			R_AddQ3Surface (surf, ent, MBT_Q3BSP_FLARE);
			break;

		case FACETYPE_PLANAR:
			if (!r_noCull->intVal && R_CullQ3SurfacePlanar (surf, surf->q3_shaderRef->shader, origin))
				continue;
			// FALL THROUGH
		default:
			R_AddQ3Surface (surf, ent, MBT_Q3BSP);
			break;
		}
	}
}

/*
=============================================================================

	FUNCTION WRAPPING

=============================================================================
*/

/*
=============
R_AddWorldToList
=============
*/
void R_AddWorldToList (void)
{
	uint32	startTime;

	R_ClearSky ();

	if (ri.def.rdFlags & RDF_NOWORLDMODEL)
		return;

	if (ri.scn.worldModel && ri.scn.worldModel->type == MODEL_Q3BSP) {
		R_AddQ3WorldToList ();
		return;
	}

	if (r_times->intVal)
		startTime = Sys_UMilliseconds ();
	R_MarkQ2Leaves ();
	if (r_times->intVal)
		ri.pc.timeMarkLeaves += Sys_UMilliseconds () - startTime;

	if (!r_drawworld->intVal)
		return;

	if (r_times->intVal)
		startTime = Sys_UMilliseconds ();
	R_Q2BSP_MarkWorldLights ();
	if (r_times->intVal)
		ri.pc.timeMarkLights += Sys_UMilliseconds () - startTime;

	if (r_times->intVal)
		startTime = Sys_UMilliseconds ();
	R_RecursiveQ2WorldNode (ri.scn.worldModel->bspModel.nodes, (r_noCull->intVal) ? 0 : 31);
	if (r_times->intVal)
		ri.pc.timeRecurseWorld += Sys_UMilliseconds () - startTime;
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
