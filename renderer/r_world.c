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

model_t				*r_worldModel;
entity_t			r_worldEntity;

uLong				r_visFrameCount;	// bumped when going to a new PVS

int					r_oldViewCluster;
int					r_viewCluster;

typedef struct skyState_s {
	qBool			loaded;

	char			baseName[MAX_QPATH];
	float			rotation;
	vec3_t			axis;

	mesh_t			meshes[6];
	shader_t		*shaders[6];

	vec2_t			coords[6][4];
	vec3_t			verts[6][4];
} skyState_t;

static skyState_t		r_skyState;

/*
=============================================================================

	SKY

=============================================================================
*/

static const float	r_skyClip[6][3] = {
	{1, 1, 0},		{1, -1, 0},		{0, -1, 1},		{0, 1, 1},		{1, 0, 1},		{-1, 0, 1} 
};
static const int	r_skySTToVec[6][3] = {
	{3, -1, 2},		{-3, 1, 2},		{1, 3, 2},		{-1, -3, 2},	{-2, -1, 3},	{2, -1, -3}
};
static const int	r_skyVecToST[6][3] = {
	{-2, 3, 1},		{2, 3, -1},		{1, 3, 2},		{-1, 3, -2},	{-2, -1, 3},	{-2, 1, -3}
};
static const int	r_skyTexOrder[6] = {0, 2, 1, 3, 4, 5};

#define	SKY_MAXCLIPVERTS	64
#define SKY_BOXSIZE			8192

/*
=================
R_AddSkySurface
=================
*/
static void ClipSkyPolygon (int nump, vec3_t vecs, int stage)
{
	const float	*norm;
	float	*v, d, e;
	float	dists[SKY_MAXCLIPVERTS];
	int		sides[SKY_MAXCLIPVERTS];
	int		newc[2], i, j;
	vec3_t	newv[2][SKY_MAXCLIPVERTS];
	qBool	front, back;

	if (nump > SKY_MAXCLIPVERTS-2)
		Com_Error (ERR_DROP, "ClipSkyPolygon: SKY_MAXCLIPVERTS");

	if (stage == 6) {
		// Fully clipped, so draw it
		int		i, j;
		vec3_t	v, av;
		float	s, t, dv;
		int		axis;
		float	*vp;

		// Decide which face it maps to
		VectorClear (v);
		for (i=0, vp=vecs ; i<nump ; i++, vp+=3)
			VectorAdd (vp, v, v);

		VectorSet (av, (float)fabs (v[0]), (float)fabs (v[1]), (float)fabs (v[2]));

		if (av[0] > av[1] && av[0] > av[2])
			axis = (v[0] < 0) ? 1 : 0;
		else if (av[1] > av[2] && av[1] > av[0])
			axis = (v[1] < 0) ? 3 : 2;
		else
			axis = (v[2] < 0) ? 5 : 4;

		// Project new texture coords
		for (i=0 ; i<nump ; i++, vecs+=3) {
			j = r_skyVecToST[axis][2];
			dv = (j > 0) ? vecs[j - 1] : -vecs[-j - 1];

			if (dv < 0.001)
				continue;	// Don't divide by zero

			j = r_skyVecToST[axis][0];
			s = (j < 0) ? -vecs[-j -1] / dv : vecs[j-1] / dv;

			j = r_skyVecToST[axis][1];
			t = (j < 0) ? -vecs[-j -1] / dv : vecs[j-1] / dv;

			if (s < r_currentList->skyMins[axis][0])
				r_currentList->skyMins[axis][0] = s;
			if (t < r_currentList->skyMins[axis][1])
				r_currentList->skyMins[axis][1] = t;

			if (s > r_currentList->skyMaxs[axis][0])
				r_currentList->skyMaxs[axis][0] = s;
			if (t > r_currentList->skyMaxs[axis][1])
				r_currentList->skyMaxs[axis][1] = t;
		}

		return;
	}

	front = back = qFalse;
	norm = r_skyClip[stage];
	for (i=0, v=vecs ; i<nump ; i++, v+=3) {
		d = DotProduct (v, norm);
		if (d > LARGE_EPSILON) {
			front = qTrue;
			sides[i] = SIDE_FRONT;
		}
		else if (d < -LARGE_EPSILON) {
			back = qTrue;
			sides[i] = SIDE_BACK;
		}
		else
			sides[i] = SIDE_ON;
		dists[i] = d;
	}

	if (!front || !back) {
		// Not clipped
		ClipSkyPolygon (nump, vecs, stage+1);
		return;
	}

	// Clip it
	sides[i] = sides[0];
	dists[i] = dists[0];
	VectorCopy (vecs, (vecs+(i*3)));
	newc[0] = newc[1] = 0;

	for (i=0, v=vecs ; i<nump ; i++, v+=3) {
		switch (sides[i]) {
		case SIDE_FRONT:
			VectorCopy (v, newv[0][newc[0]]);
			newc[0]++;
			break;

		case SIDE_BACK:
			VectorCopy (v, newv[1][newc[1]]);
			newc[1]++;
			break;

		case SIDE_ON:
			VectorCopy (v, newv[0][newc[0]]);
			newc[0]++;
			VectorCopy (v, newv[1][newc[1]]);
			newc[1]++;
			break;
		}

		if (sides[i] == SIDE_ON || sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;

		d = dists[i] / (dists[i] - dists[i+1]);
		for (j=0 ; j<3 ; j++) {
			e = v[j] + d * (v[j+3] - v[j]);
			newv[0][newc[0]][j] = e;
			newv[1][newc[1]][j] = e;
		}
		newc[0]++;
		newc[1]++;
	}

	// Continue
	ClipSkyPolygon (newc[0], newv[0][0], stage+1);
	ClipSkyPolygon (newc[1], newv[1][0], stage+1);
}
static void R_AddSkySurface (mBspSurface_t *surf)
{
	vec3_t		verts[SKY_MAXCLIPVERTS];
	mBspPoly_t	*p;
	int			i;

	// Calculate vertex values for sky box
	for (p=surf->polys ; p ; p=p->next) {
		for (i=0 ; i<p->mesh.numVerts && i<SKY_MAXCLIPVERTS ; i++) {
			VectorSubtract (p->mesh.vertexArray[i], r_refDef.viewOrigin, verts[i]);
		}

		ClipSkyPolygon (p->mesh.numVerts, verts[0], 0);
	}
}


/*
==============
R_ClearSky
==============
*/
static void R_ClearSky (void)
{
	int		i;

	if (!r_skyState.loaded)
		return;

	r_currentList->skyDrawn = qFalse;
	for (i=0 ; i<6 ; i++) {
		r_currentList->skyMins[i][0] = 9999;
		r_currentList->skyMins[i][1] = 9999;

		r_currentList->skyMaxs[i][0] = -9999;
		r_currentList->skyMaxs[i][1] = -9999;
	}
}


/*
==============
R_AddSkyToList
==============
*/
static void R_AddSkyToList (void)
{
	if (!r_skyState.loaded)
		return;

	R_AddMeshToList (r_skyState.shaders[r_skyTexOrder[0]], NULL, MBT_SKY, NULL);
}


/*
==============
R_DrawSky
==============
*/
static void DrawSkyVec (int side, int vertNum, float s, float t)
{
	vec3_t		v, b;
	int			j, k;

	// Coords
	r_skyState.coords[side][vertNum][0] = clamp ((s + 1) * 0.5f, 0, 1);
	r_skyState.coords[side][vertNum][1] = 1.0f - clamp ((t + 1) * 0.5f, 0, 1);

	// Verts
	VectorSet (b, s * SKY_BOXSIZE, t * SKY_BOXSIZE, SKY_BOXSIZE);

	for (j=0 ; j<3 ; j++) {
		k = r_skySTToVec[side][j];
		if (k < 0)
			v[j] = -b[-k - 1];
		else
			v[j] = b[k - 1];
	}

	r_skyState.verts[side][vertNum][0] = v[0] + r_refDef.viewOrigin[0];
	r_skyState.verts[side][vertNum][1] = v[1] + r_refDef.viewOrigin[1];
	r_skyState.verts[side][vertNum][2] = v[2] + r_refDef.viewOrigin[2];
}
void R_DrawSky (meshBuffer_t *mb)
{
	int			i;

	if (r_skyState.rotation) {
		// Check for sky visibility
		for (i=0 ; i<6 ; i++) {
			if (r_currentList->skyMins[i][0] < r_currentList->skyMaxs[i][0] && r_currentList->skyMins[i][1] < r_currentList->skyMaxs[i][1])
				break;
		}

		if (i == 6)
			return;	// Nothing visible

		// Rotation matrix
		qglPushMatrix ();
		qglRotatef (r_refDef.time * r_skyState.rotation, r_skyState.axis[0], r_skyState.axis[1], r_skyState.axis[2]);
	}

	for (i=0 ; i<6 ; i++) {
		if (r_skyState.rotation) {
			// Hack, forces full sky to draw when rotating
			r_currentList->skyMins[i][0] = -1;
			r_currentList->skyMins[i][1] = -1;
			r_currentList->skyMaxs[i][0] = 1;
			r_currentList->skyMaxs[i][1] = 1;
		}
		else {
			if (r_currentList->skyMins[i][0] >= r_currentList->skyMaxs[i][0]
			|| r_currentList->skyMins[i][1] >= r_currentList->skyMaxs[i][1])
				continue;
		}

		// Push and render
		DrawSkyVec (i, 0, r_currentList->skyMins[i][0], r_currentList->skyMins[i][1]);
		DrawSkyVec (i, 1, r_currentList->skyMins[i][0], r_currentList->skyMaxs[i][1]);
		DrawSkyVec (i, 2, r_currentList->skyMaxs[i][0], r_currentList->skyMaxs[i][1]);
		DrawSkyVec (i, 3, r_currentList->skyMaxs[i][0], r_currentList->skyMins[i][1]);

		mb->shader = r_skyState.shaders[r_skyTexOrder[i]];
		RB_PushMesh (&r_skyState.meshes[i], MF_NONBATCHED|MF_TRIFAN|mb->shader->features);
		RB_RenderMeshBuffer (mb, qFalse);
	}

	if (r_skyState.rotation)
		qglPopMatrix ();
}


/*
=================
R_CheckLoadSky

Returns qTrue if there are ANY sky surfaces in the map, called on map load
=================
*/
static qBool R_CheckLoadSky (mBspNode_t *node)
{
	mBspSurface_t	*surf, **mark;
	mBspLeaf_t		*leaf;
	int				i;

	if (node->contents == CONTENTS_SOLID)
		return qFalse;		// Solid

	// Recurse down the children
	if (node->contents == -1)
		return R_CheckLoadSky (node->children[0]) || R_CheckLoadSky (node->children[1]);

	// If this is a leaf node, draw it
	leaf = (mBspLeaf_t *)node;
	if (!leaf->numMarkSurfaces)
		return qFalse;

	// Search
	for (i=0, mark=leaf->firstMarkSurface ; i<leaf->numMarkSurfaces ; i++, mark++) {
		surf = *mark;
		if (surf->texInfo->flags & SURF_TEXINFO_SKY)
			return qTrue;
	}

	return qFalse;
}


/*
============
R_SetSky
============
*/
void R_SetSky (char *name, float rotate, vec3_t axis)
{
	char	pathName[MAX_QPATH];
	int		i;

	r_skyState.loaded = R_CheckLoadSky (r_worldModel->nodes);
	if (!r_skyState.loaded)
		return;

	Q_strncpyz (r_skyState.baseName, name, sizeof (r_skyState.baseName));
	r_skyState.rotation = rotate;
	VectorCopy (axis, r_skyState.axis);

	for (i=0 ; i<6 ; i++) {
		Q_snprintfz (pathName, sizeof (pathName), "env/%s%s.tga", r_skyState.baseName, sky_NameSuffix[i]);
		r_skyState.shaders[i] = R_RegisterSky (pathName);

		if (!r_skyState.shaders[i])
			r_skyState.shaders[i] = r_noShader;
	}
}

/*
=============================================================================

	WORLD MODEL

=============================================================================
*/

/*
================
R_AddWorldSurface
================
*/
static void R_AddWorldSurface (mBspSurface_t *surf, shader_t *shader, entity_t *entity)
{
	meshBuffer_t	*mb;

	// Add to list
	mb = R_AddMeshToList (shader, entity, MBT_MODEL_BSP, surf);
	if (!mb)
		return;

	// Caustics
	if (surf->lmTexNum && r_caustics->integer && surf->flags & SURF_UNDERWATER) {
		if (surf->flags & SURF_LAVA && glMedia.worldLavaCaustics)
			R_AddMeshToList (glMedia.worldLavaCaustics, entity, MBT_MODEL_BSP, surf);
		else if (surf->flags & SURF_SLIME && glMedia.worldSlimeCaustics)
			R_AddMeshToList (glMedia.worldSlimeCaustics, entity, MBT_MODEL_BSP, surf);
		else if (glMedia.worldWaterCaustics)
			R_AddMeshToList (glMedia.worldWaterCaustics, entity, MBT_MODEL_BSP, surf);
	}
}


/*
================
R_SurfShader
================
*/
static inline shader_t *R_SurfShader (mBspSurface_t *surf)
{
	mBspTexInfo_t	*texInfo;
	int				i;

	// Doesn't animate
	if (!surf->texInfo->next)
		return surf->texInfo->shader;

	// Animates
	texInfo = surf->texInfo;
	for (i=((int)(r_refDef.time * 2))%texInfo->numFrames ; i ; i--)
		texInfo = texInfo->next;

	return texInfo->shader;
}


/*
================
R_CullSurface
================
*/
static inline qBool R_CullSurface (mBspSurface_t *surf, shader_t *shader, float dist, int clipFlags)
{
	// See if it's been touched, if not, touch it
	if (surf->surfFrame == r_frameCount)
		return qTrue;	// Already touched this frame
	surf->surfFrame = r_frameCount;

	// Surface culling
	if (r_noCull->integer)
		return qFalse;

	// Check if it even has any passes
	if (!shader || !shader->numPasses)
		return qTrue;

	// Side culling
	switch (shader->cullType) {
	case SHADER_CULL_BACK:
		if (surf->flags & SURF_PLANEBACK) {
			if (dist <= SMALL_EPSILON)
				return qTrue;	// Wrong side
		}
		else {
			if (dist >= -SMALL_EPSILON)
				return qTrue;	// Wrong side
		}
		break;

	case SHADER_CULL_FRONT:
		if (surf->flags & SURF_PLANEBACK) {
			if (dist >= -SMALL_EPSILON)
				return qTrue;	// Wrong side
		}
		else {
			if (dist <= SMALL_EPSILON)
				return qTrue;	// Wrong side
		}
		break;
	}

	// Cull bounding box
	return (clipFlags && R_CullBox (surf->mins, surf->maxs, clipFlags));
}


/*
================
R_RecursiveWorldNode
================
*/
static void R_RecursiveWorldNode (mBspNode_t *node, int clipFlags)
{
	cBspPlane_t		*plane;
	mBspSurface_t	*surf, **mark;
	mBspLeaf_t		*leaf;
	shader_t		*shader;
	int				side, clipped, i;
	float			dist;

	if (node->contents == CONTENTS_SOLID)
		return;		// Solid
	if (node->visFrame != r_visFrameCount)
		return;		// Node not visible this frame

	// Cull
	if (clipFlags) {
		for (i=0, plane=r_viewFrustum ; i<4 ; i++, plane++) {
			if (!(clipFlags & (1<<i)))
				continue;

			clipped = BoxOnPlaneSide (node->mins, node->maxs, plane);
			switch (clipped) {
			case 1:
				clipFlags &= ~(1<<i);
				break;

			case 2:
				return;
			}
		}
	}

	// If a leaf node, draw stuff
	if (node->contents != -1) {
		leaf = (mBspLeaf_t *)node;

		// Check for door connected areas
		if (r_refDef.areaBits) {
			if (!(r_refDef.areaBits[leaf->area>>3] & (1<<(leaf->area&7))))
				return;		// not visible
		}

		// Mark visible surfaces
		mark = leaf->firstMarkSurface;
		for (i=leaf->numMarkSurfaces ; i ; ) {
			do {
				(*mark)->surfFrame = -1;
				mark++;
			} while (--i);
		}

		glSpeeds.worldLeafs++;
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
	R_RecursiveWorldNode (node->children[!side], clipFlags);

	// Draw stuff
	for (i=node->numSurfaces, surf=r_worldModel->surfaces+node->firstSurface ; i ; i--, surf++) {
		// Get the shader
		shader = R_SurfShader (surf);

		// Cull
		if (R_CullSurface (surf, shader, dist, clipFlags))
			continue;

		// Sky surface
		if (surf->texInfo->flags & SURF_TEXINFO_SKY) {
			R_AddSkySurface (surf);
			continue;
		}

		// World surface
		R_AddWorldSurface (surf, shader, &r_worldEntity);
	}

	// Recurse down the front side
	R_RecursiveWorldNode (node->children[side], clipFlags);
}


/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current cluster
===============
*/
static void R_MarkLeaves (void)
{
	static int	r_oldViewCluster2;
	static int	r_viewCluster2;
	byte		*vis, fatVis[BSP_MAX_VIS];
	int			i, c;
	mBspNode_t	*node;
	mBspLeaf_t	*leaf;

	// Current viewcluster
	if (!(r_refDef.rdFlags & RDF_NOWORLDMODEL)) {
		vec3_t	temp;
		VectorCopy (r_refDef.viewOrigin, temp);

		r_oldViewCluster = r_viewCluster;
		r_oldViewCluster2 = r_viewCluster2;

		leaf = R_PointInBSPLeaf (r_refDef.viewOrigin, r_worldModel);
		r_viewCluster = r_viewCluster2 = leaf->cluster;

		// Check above and below so crossing solid water doesn't draw wrong
		if (!leaf->contents) {
			// Look down a bit
			temp[2] -= 16;
			leaf = R_PointInBSPLeaf (temp, r_worldModel);
			if (!(leaf->contents & CONTENTS_SOLID) && leaf->cluster != r_viewCluster2)
				r_viewCluster2 = leaf->cluster;
		}
		else {
			// Look up a bit
			temp[2] += 16;
			leaf = R_PointInBSPLeaf (temp, r_worldModel);
			if (!(leaf->contents & CONTENTS_SOLID) && leaf->cluster != r_viewCluster2)
				r_viewCluster2 = leaf->cluster;
		}
	}

	if (r_oldViewCluster == r_viewCluster &&
		r_oldViewCluster2 == r_viewCluster2 &&
		!r_noVis->integer && r_viewCluster != -1)
		return;

	// Development aid to let you run around and see exactly where the pvs ends
	if (gl_lockpvs->integer)
		return;

	r_visFrameCount++;
	r_oldViewCluster = r_viewCluster;
	r_oldViewCluster2 = r_viewCluster2;

	if (r_noVis->integer || r_viewCluster == -1 || !r_worldModel->vis) {
		// Mark everything
		for (i=0 ; i<r_worldModel->numLeafs ; i++)
			r_worldModel->leafs[i].visFrame = r_visFrameCount;
		for (i=0 ; i<r_worldModel->numNodes ; i++)
			r_worldModel->nodes[i].visFrame = r_visFrameCount;
		return;
	}

	vis = R_BSPClusterPVS (r_viewCluster, r_worldModel);

	// May have to combine two clusters because of solid water boundaries
	if (r_viewCluster2 != r_viewCluster) {
		memcpy (fatVis, vis, (r_worldModel->numLeafs+7)/8);
		vis = R_BSPClusterPVS (r_viewCluster2, r_worldModel);
		c = (r_worldModel->numLeafs+31)/32;
		for (i=0 ; i<c ; i++)
			((int *)fatVis)[i] |= ((int *)vis)[i];
		vis = fatVis;
	}

	for (i=0, leaf=r_worldModel->leafs ; i<r_worldModel->numLeafs ; i++, leaf++) {
		if (leaf->cluster == -1)
			continue;
		if (!(vis[leaf->cluster>>3] & (1<<(leaf->cluster&7))))
			continue;

		node = (mBspNode_t *)leaf;
		do {
			if (node->visFrame == r_visFrameCount)
				break;

			node->visFrame = r_visFrameCount;
			node = node->parent;
		} while (node);
	}
}


/*
=============
R_AddWorldToList
=============
*/
void R_AddWorldToList (void)
{
	R_ClearSky ();
	R_MarkLeaves ();
	R_PushDLights ();

	if (!r_drawworld->integer)
		return;
	if (!r_worldModel)
		return;
	if (r_refDef.rdFlags & RDF_NOWORLDMODEL)
		return;

	R_RecursiveWorldNode (r_worldModel->nodes, (r_noCull->integer) ? 0 : 15);
	R_AddSkyToList ();
}

/*
=============================================================================

	BRUSH MODEL

=============================================================================
*/

/*
=================
R_AddBrushModelToList
=================
*/
void R_AddBrushModelToList (entity_t *ent)
{
	mBspSurface_t	*surf;
	shader_t		*shader;
	vec3_t			mins, maxs;
	vec3_t			origin, temp;
	float			dist;
	int				i;

	// No surfaces
	if (ent->model->numModelSurfaces == 0)
		return;

	// Cull
	VectorSubtract (r_refDef.viewOrigin, ent->origin, origin);
	if (!r_noCull->integer) {
		if (!Matrix_Compare (ent->axis, axisIdentity)) {
			mins[0] = ent->origin[0] - ent->model->radius * ent->scale;
			mins[1] = ent->origin[1] - ent->model->radius * ent->scale;
			mins[2] = ent->origin[2] - ent->model->radius * ent->scale;

			maxs[0] = ent->origin[0] + ent->model->radius * ent->scale;
			maxs[1] = ent->origin[1] + ent->model->radius * ent->scale;
			maxs[2] = ent->origin[2] + ent->model->radius * ent->scale;

			if (R_CullSphere (ent->origin, ent->model->radius, 15))
				return;

			VectorCopy (origin, temp);
			Matrix_TransformVector (ent->axis, temp, origin);
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
	if (gl_flashblend->integer == 2 || !gl_flashblend->integer) {
		dLight_t		*light;
		mBspNode_t		*node;

		node = ent->model->nodes + ent->model->firstNode;

		for (light=r_dLightList, i=0 ; i<r_numDLights ; i++, light++) {
			if (!BoundsAndSphereIntersect (mins, maxs, light->origin, light->intensity))
				continue;

			R_MarkLights (qFalse, light, 1<<i, node);
		}
	}

	// Draw the surfaces
	surf = ent->model->surfaces + ent->model->firstModelSurface;
	for (i=0 ; i<ent->model->numModelSurfaces ; i++, surf++) {
		// These aren't drawn here, ever.
		if (surf->texInfo->flags & SURF_TEXINFO_SKY)
			continue;

		// Find which side of the node we are on
		if (surf->plane->type < 3)
			dist = origin[surf->plane->type] - surf->plane->dist;
		else
			dist = DotProduct (origin, surf->plane->normal) - surf->plane->dist;

		// Get the shader
		shader = R_SurfShader (surf);

		// Cull
		if (R_CullSurface (surf, shader, dist, 0))
			continue;

		// World surface
		R_AddWorldSurface (surf, shader, ent);
	}
}


/*
=============================================================================

	CONSOLE COMMANDS

=============================================================================
*/

/*
=================
R_SetSky_f

Set a specific sky and rotation speed
=================
*/
static void R_SetSky_f (void)
{
	float	rotate;
	vec3_t	axis;

	if (!r_skyState.loaded) {
		Com_Printf (0, "No sky surfaces!\n");
		return;
	}

	if (Cmd_Argc () < 2) {
		Com_Printf (0, "Usage: sky <basename> <rotate> [axis x y z]\n");
		Com_Printf (0, "Currently: sky <%s> <%.1f> [%.1f %.1f %.1f]\n", r_skyState.baseName, r_skyState.rotation, r_skyState.axis[0], r_skyState.axis[1], r_skyState.axis[2]);
		return;
	}

	if (Cmd_Argc () > 2)
		rotate = (float)atof (Cmd_Argv (2));
	else
		rotate = 0;

	if (Cmd_Argc () == 6)
		VectorSet (axis, (float)atof (Cmd_Argv (3)), (float)atof (Cmd_Argv (4)), (float)atof (Cmd_Argv (5)));
	else
		VectorSet (axis, 0, 0, 1);

	R_SetSky (Cmd_Argv (1), rotate, axis);
}

/*
=============================================================================

	INIT / SHUTDOWN

=============================================================================
*/

static cmdFunc_t	*cmd_sky;

/*
==================
R_WorldInit
==================
*/
void R_WorldInit (void)
{
	int		i;

	// Commands
	cmd_sky = Cmd_AddCommand ("sky",		R_SetSky_f,		"Changes the sky env basename");

	// Init sky meshes
	for (i=0 ; i<6 ; i++) {
		r_skyState.meshes[i].numIndexes = QUAD_INDEXES;
		r_skyState.meshes[i].numVerts = 4;

		r_skyState.meshes[i].colorArray = NULL;
		r_skyState.meshes[i].coordArray = r_skyState.coords[i];
		r_skyState.meshes[i].indexArray = rb_quadIndices;
		r_skyState.meshes[i].lmCoordArray = NULL;
		r_skyState.meshes[i].normalsArray = NULL;
		r_skyState.meshes[i].sVectorsArray = NULL;
		r_skyState.meshes[i].tVectorsArray = NULL;
		r_skyState.meshes[i].trNeighborsArray = NULL;
		r_skyState.meshes[i].trNormalsArray = NULL;
		r_skyState.meshes[i].vertexArray = r_skyState.verts[i];
	}
}


/*
==================
R_WorldShutdown
==================
*/
void R_WorldShutdown (void)
{
	// Remove commands
	Cmd_RemoveCommand ("sky", cmd_sky);
}
