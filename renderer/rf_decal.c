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
// rf_decal.c
// FIXME TODO:
// - Clean up CG_SpawnDecal parms
// - could be re-worked so that decals are only added to the list when their owner surface is
// - r_decal_maxFragments r_decal_maxVerts?
//

#include "rf_local.h"

#define MAX_DECAL_VERTS		512
#define MAX_DECAL_FRAGMENTS	384

typedef struct refFragment_s {
	int					firstVert;
	int					numVerts;

	vec3_t				normal;

	mBspSurface_t		*surf;
} refFragment_t;

static mesh_t		r_decalMesh;

/*
==============================================================================

	REFRESH FUNCTIONS

==============================================================================
*/

/*
===============
R_AddDecalsToList
===============
*/
void R_AddDecalsToList (void)
{
	refDecal_t		*d;
	mQ3BspFog_t		*fog;
	mBspSurface_t	*surf;
	uint32			i, j;

	if (!r_drawDecals->intVal)
		return;

	// Add decal meshes to list
	for (i=0 ; i<ri.scn.numDecals ; i++) {
		d = ri.scn.decalList[i];

		// Check the surface visibility
		fog = NULL;
		if (d->numSurfaces) {
			for (j=0 ; j<d->numSurfaces ; j++) {
				surf = d->surfaces[j];
				if (!R_CullSurface (surf)) {
					if (!fog && surf->q3_fog)
						fog = surf->q3_fog;	// FIXME: test me!
					break;
				}
			}
			if (j == d->numSurfaces)
				continue;
		}

		// Frustum cull
		if (R_CullSphere (d->origin, d->radius, 31))
			continue;

		// Add to the list
		R_AddMeshToList (d->poly.mat, d->poly.matTime, NULL, fog, MBT_DECAL, d);
		ri.scn.drawnDecals++;
	}
}


/*
================
R_PushDecal
================
*/
void R_PushDecal (meshBuffer_t *mb, meshFeatures_t features)
{
	refDecal_t	*d;

	d = (refDecal_t *)mb->mesh;
	if (d->poly.numVerts > RB_MAX_VERTS)
		return;

	r_decalMesh.numIndexes = d->numIndexes;
	r_decalMesh.indexArray = d->indexes;

	r_decalMesh.numVerts = d->poly.numVerts;
	r_decalMesh.colorArray = d->poly.colors;
	r_decalMesh.coordArray = (vec2_t *)d->poly.texCoords;
	r_decalMesh.normalsArray = (vec3_t *)d->normals;
	r_decalMesh.vertexArray = (vec3_t *)d->poly.vertices;

	RB_PushMesh (&r_decalMesh, features);
}


/*
================
R_DecalOverflow
================
*/
qBool R_DecalOverflow (meshBuffer_t *mb)
{
	refDecal_t	*d;

	d = (refDecal_t *)mb->mesh;
	return RB_BackendOverflow (d->poly.numVerts, d->numIndexes);
}


/*
================
R_DecalInit
================
*/
void R_DecalInit (void)
{
	r_decalMesh.lmCoordArray = NULL;
	r_decalMesh.sVectorsArray = NULL;
	r_decalMesh.tVectorsArray = NULL;
	r_decalMesh.trNeighborsArray = NULL;
	r_decalMesh.trNormalsArray = NULL;
}

/*
==============================================================================

	FRAGMENT CLIPPING

==============================================================================
*/

static uint32			r_numFragmentVerts;
static vec3_t			r_fragmentVerts[MAX_DECAL_VERTS];
static vec3_t			r_fragmentNormals[MAX_DECAL_VERTS];

static uint32			r_numClippedFragments;
static refFragment_t	r_clippedFragments[MAX_DECAL_FRAGMENTS];

static uint32			r_fragmentFrame = 0;
static cBspPlane_t		r_fragmentPlanes[6];

static vec3_t			r_decalOrigin;
static vec3_t			r_decalNormal;
static float			r_decalRadius;

/*
==============================================================================

	QUAKE II FRAGMENT CLIPPING

==============================================================================
*/

/*
=================
R_Q2BSP_ClipPoly
=================
*/
static void R_Q2BSP_ClipPoly (int nump, vec4_t vecs, int stage, refFragment_t *fr)
{
	cBspPlane_t	*plane;
	qBool		front, back;
	vec4_t		newv[MAX_DECAL_VERTS];
	int			sides[MAX_DECAL_VERTS];
	float		dists[MAX_DECAL_VERTS];
	float		*v, d;
	int			newc, i, j;

	if (nump > MAX_DECAL_VERTS - 2) {
		Com_Printf (PRNT_ERROR, "R_Q2BSP_ClipPoly: nump > MAX_DECAL_VERTS - 2");
		return;
	}

	if (stage == 6) {
		// Fully clipped
		if (nump > 2) {
			fr->numVerts = nump;
			fr->firstVert = r_numFragmentVerts;

			if (r_numFragmentVerts+nump >= MAX_DECAL_VERTS)
				nump = MAX_DECAL_VERTS - r_numFragmentVerts;

			for (i=0, v=vecs ; i<nump ; i++, v+=4) {
				Vec3Copy (fr->normal, r_fragmentNormals[r_numFragmentVerts + i]);
				Vec3Copy (v, r_fragmentVerts[r_numFragmentVerts + i]);
			}

			r_numFragmentVerts += nump;
		}

		return;
	}

	front = back = qFalse;
	plane = &r_fragmentPlanes[stage];
	for (i=0, v=vecs ; i<nump ; i++ , v+=4) {
		d = PlaneDiff (v, plane);
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

	if (!front)
		return;

	// Clip it
	sides[i] = sides[0];
	dists[i] = dists[0];
	Vec3Copy (vecs, (vecs + (i * 4)));
	newc = 0;

	for (i=0, v=vecs ; i<nump ; i++, v+=4) {
		switch (sides[i]) {
		case SIDE_FRONT:
			Vec3Copy (v, newv[newc]);
			newc++;
			break;
		case SIDE_BACK:
			break;
		case SIDE_ON:
			Vec3Copy (v, newv[newc]);
			newc++;
			break;
		}

		if (sides[i] == SIDE_ON
		|| sides[i+1] == SIDE_ON
		|| sides[i+1] == sides[i])
			continue;

		d = dists[i] / (dists[i] - dists[i+1]);
		for (j=0 ; j<3 ; j++)
			newv[newc][j] = v[j] + d * (v[j+4] - v[j]);
		newc++;
	}

	// Continue
	R_Q2BSP_ClipPoly (newc, newv[0], stage+1, fr);
}


/*
=================
R_Q2BSP_PlanarClipFragment
=================
*/
static void R_Q2BSP_PlanarClipFragment (mBspNode_t *node, mBspSurface_t *surf)
{
	int				i;
	float			*v, *v2, *v3;
	refFragment_t	*fr;
	vec4_t			verts[MAX_DECAL_VERTS];

	v = surf->mesh->vertexArray[0];

	// Copy vertex data and clip to each triangle
	for (i=0; i<surf->mesh->numVerts-2 ; i++) {
		fr = &r_clippedFragments[r_numClippedFragments];
		fr->numVerts = 0;
		fr->surf = surf;
		Vec3Copy (surf->mesh->normalsArray[i], fr->normal);

		v2 = surf->mesh->vertexArray[0] + (i+1) * 3;
		v3 = surf->mesh->vertexArray[0] + (i+2) * 3;

		Vec3Copy (v , verts[0]);
		Vec3Copy (v2, verts[1]);
		Vec3Copy (v3, verts[2]);

		R_Q2BSP_ClipPoly (3, verts[0], 0, fr);
		if (fr->numVerts && (r_numFragmentVerts >= MAX_DECAL_VERTS || ++r_numClippedFragments >= MAX_DECAL_FRAGMENTS))
			return;
	}
}


/*
=================
R_Q2BSP_FragmentNode
=================
*/
static void R_Q2BSP_FragmentNode (mBspNode_t *node)
{
	float			dist;
	mBspLeaf_t		*leaf;
	mBspSurface_t	*surf, **mark;

mark0:
	if (r_numFragmentVerts >= MAX_DECAL_VERTS || r_numClippedFragments >= MAX_DECAL_FRAGMENTS)
		return;	// Already reached the limit somewhere else

	if (node->c.q2_contents != -1) {
		if (node->c.q2_contents == CONTENTS_SOLID)
			return;

		// Leaf
		leaf = (mBspLeaf_t *)node;
		if (!leaf->q2_firstDecalSurface)
			return;

		mark = leaf->q2_firstDecalSurface;
		do {
			if (r_numFragmentVerts >= MAX_DECAL_VERTS || r_numClippedFragments >= MAX_DECAL_FRAGMENTS)
				return;

			surf = *mark++;
			if (!surf)
				continue;

			if (surf->fragmentFrame == r_fragmentFrame)
				continue;		// Already touched
			surf->fragmentFrame = r_fragmentFrame;

			if (surf->q2_numEdges < 3)
				continue;		// Bogus face

			if (surf->q2_flags & SURF_PLANEBACK) {
				if (DotProduct(r_decalNormal, surf->q2_plane->normal) > -0.5f)
					continue;	// Greater than 60 degrees
			}
			else {
				if (DotProduct(r_decalNormal, surf->q2_plane->normal) < 0.5f)
					continue;	// Greater than 60 degrees
			}

			// Clip
			R_Q2BSP_PlanarClipFragment (node, surf);
		} while (*mark);

		return;
	}

	dist = PlaneDiff (r_decalOrigin, node->c.plane);
	if (dist > r_decalRadius) {
		node = node->children[0];
		goto mark0;
	}
	if (dist < -r_decalRadius) {
		node = node->children[1];
		goto mark0;
	}

	R_Q2BSP_FragmentNode (node->children[0]);
	R_Q2BSP_FragmentNode (node->children[1]);
}

/*
==============================================================================

	QUAKE III FRAGMENT CLIPPING

==============================================================================
*/

/*
=================
R_Q3BSP_WindingClipFragment

This function operates on windings (convex polygons without 
any points inside) like triangles, quads, etc. The output is 
a convex fragment (polygon, trifan) which the result of clipping 
the input winding by six fragment planes.
=================
*/
static void R_Q3BSP_WindingClipFragment (vec3_t *wVerts, int numVerts, refFragment_t *fr)
{
	int				i, j;
	int				stage, newc, numv;
	cBspPlane_t		*plane;
	qBool			front;
	float			*v, *nextv, d;
	static float	dists[MAX_DECAL_VERTS+1];
	static int		sides[MAX_DECAL_VERTS+1];
	vec3_t			*verts, *newverts;
	static vec3_t	newv[2][MAX_DECAL_VERTS];

	numv = numVerts;
	verts = wVerts;

	for (stage=0, plane=r_fragmentPlanes ; stage<6 ; stage++, plane++) {
		for (i=0, v=verts[0], front=qFalse ; i<numv ; i++, v+=3) {
			d = PlaneDiff (v, plane);

			if (d > LARGE_EPSILON) {
				front = qTrue;
				sides[i] = SIDE_FRONT;
			}
			else if (d < -LARGE_EPSILON) {
				sides[i] = SIDE_BACK;
			}
			else {
				front = qTrue;
				sides[i] = SIDE_ON;
			}
			dists[i] = d;
		}

		if (!front)
			return;

		// Clip it
		sides[i] = sides[0];
		dists[i] = dists[0];

		newc = 0;
		newverts = newv[stage & 1];

		for (i=0, v=verts[0] ; i<numv ; i++, v+=3) {
			switch (sides[i]) {
			case SIDE_FRONT:
				if (newc == MAX_DECAL_VERTS)
					return;
				Vec3Copy (v, newverts[newc]);
				newc++;
				break;

			case SIDE_BACK:
				break;

			case SIDE_ON:
				if (newc == MAX_DECAL_VERTS)
					return;
				Vec3Copy (v, newverts[newc]);
				newc++;
				break;
			}

			if (sides[i] == SIDE_ON
			|| sides[i+1] == SIDE_ON
			|| sides[i+1] == sides[i])
				continue;
			if (newc == MAX_DECAL_VERTS)
				return;

			d = dists[i] / (dists[i] - dists[i+1]);
			nextv = (i == numv - 1) ? verts[0] : v + 3;
			for (j=0 ; j<3 ; j++)
				newverts[newc][j] = v[j] + d * (nextv[j] - v[j]);

			newc++;
		}

		if (newc <= 2)
			return;

		// Continue with new verts
		numv = newc;
		verts = newverts;
	}

	// Fully clipped
	if (r_numFragmentVerts + numv > MAX_DECAL_VERTS)
		return;

	fr->numVerts = numv;
	fr->firstVert = r_numFragmentVerts;

	for (i=0, v=verts[0] ; i<numv ; i++, v+=3) {
		Vec3Copy (fr->normal, r_fragmentNormals[r_numFragmentVerts + i]);
		Vec3Copy (v, r_fragmentVerts[r_numFragmentVerts + i]);
	}
	r_numFragmentVerts += numv;
}


/*
=================
R_Q3BSP_PlanarSurfClipFragment

NOTE: one might want to combine this function with 
R_Q3BSP_WindingClipFragment for special cases like trifans (q1 and
q2 polys) or tristrips for ultra-fast clipping, providing there's 
enough stack space (depending on MAX_DECAL_VERTS value).
=================
*/
static void R_Q3BSP_PlanarSurfClipFragment (mBspSurface_t *surf, mBspNode_t *node)
{
	int				i;
	mesh_t			*mesh;
	index_t			*index;
	vec3_t			*normals, *verts, tri[3];
	refFragment_t	*fr;

	if (DotProduct (r_decalNormal, surf->q3_origin) < 0.5)
		return;		// Greater than 60 degrees

	mesh = surf->mesh;

	// Clip each triangle individually
	index = mesh->indexArray;
	normals = mesh->normalsArray;
	verts = mesh->vertexArray;
	for (i=0 ; i<mesh->numIndexes ; i+=3, index+=3) {
		fr = &r_clippedFragments[r_numClippedFragments];
		fr->numVerts = 0;
		fr->surf = surf;

		Vec3Copy (normals[index[0]], fr->normal);

		Vec3Copy (verts[index[0]], tri[0]);
		Vec3Copy (verts[index[1]], tri[1]);
		Vec3Copy (verts[index[2]], tri[2]);

		R_Q3BSP_WindingClipFragment (tri, 3, fr);
		if (fr->numVerts && (r_numFragmentVerts == MAX_DECAL_VERTS || ++r_numClippedFragments == MAX_DECAL_FRAGMENTS))
			return;
	}
}


/*
=================
R_Q3BSP_PatchSurfClipFragment
=================
*/
static void R_Q3BSP_PatchSurfClipFragment (mBspSurface_t *surf, mBspNode_t *node)
{
	int				i;
	mesh_t			*mesh;
	index_t			*index;
	vec3_t			*normals, *verts, tri[3];
	vec3_t			dir1, dir2, snorm;
	refFragment_t	*fr;

	mesh = surf->mesh;

	// Clip each triangle individually
	index = mesh->indexArray;
	normals = mesh->normalsArray;
	verts = mesh->vertexArray;
	for (i=0 ; i<mesh->numIndexes ; i+=3, index+=3) {
		fr = &r_clippedFragments[r_numClippedFragments];
		fr->numVerts = 0;
		fr->surf = surf;

		Vec3Copy (normals[index[0]], fr->normal);

		Vec3Copy (verts[index[0]], tri[0]);
		Vec3Copy (verts[index[1]], tri[1]);
		Vec3Copy (verts[index[2]], tri[2]);

		// Calculate two mostly perpendicular edge directions
		Vec3Subtract (tri[0], tri[1], dir1);
		Vec3Subtract (tri[2], tri[1], dir2);

		// We have two edge directions, we can calculate a third vector from
		// them, which is the direction of the triangle normal
		CrossProduct (dir1, dir2, snorm);

		// We multiply 0.5 by length of snorm to avoid normalizing
		if (DotProduct(r_decalNormal, snorm) < 0.5f * Vec3Length(snorm))
			continue;	// Greater than 60 degrees

		R_Q3BSP_WindingClipFragment (tri, 3, fr);
		if (fr->numVerts && (r_numFragmentVerts == MAX_DECAL_VERTS || ++r_numClippedFragments == MAX_DECAL_FRAGMENTS))
			return;
	}
}


/*
=================
R_Q3BSP_FragmentNode
=================
*/
static void R_Q3BSP_FragmentNode (void)
{
	int					stackdepth = 0;
	float				dist;
	mBspNode_t			*node;
	static mBspNode_t	*localStack[2048];
	mBspLeaf_t			*leaf;
	mBspSurface_t		*surf, **mark;

	node = ri.scn.worldModel->bspModel.nodes;
	for (stackdepth=0 ; ; ) {
		if (node->c.plane == NULL) {
			leaf = (mBspLeaf_t *)node;
			if (!leaf->q3_firstFragmentSurface)
				goto nextNodeOnStack;

			mark = leaf->q3_firstFragmentSurface;
			do {
				if (r_numFragmentVerts == MAX_DECAL_VERTS || r_numClippedFragments == MAX_DECAL_FRAGMENTS)
					return;		// Already reached the limit

				surf = *mark++;
				if (surf->fragmentFrame == r_fragmentFrame)
					continue;
				surf->fragmentFrame = r_fragmentFrame;

				if (surf->q3_faceType == FACETYPE_PLANAR)
					R_Q3BSP_PlanarSurfClipFragment (surf, node);
				else
					R_Q3BSP_PatchSurfClipFragment (surf, node);
			} while (*mark);

			if (r_numFragmentVerts == MAX_DECAL_VERTS || r_numClippedFragments == MAX_DECAL_FRAGMENTS)
				return;		// Already reached the limit

nextNodeOnStack:
			if (!stackdepth)
				break;
			node = localStack[--stackdepth];
			continue;
		}

		dist = PlaneDiff (r_decalOrigin, node->c.plane);
		if (dist > r_decalRadius) {
			node = node->children[0];
			continue;
		}

		if (dist >= -r_decalRadius && (stackdepth < sizeof (localStack) / sizeof (mBspNode_t *)))
			localStack[stackdepth++] = node->children[0];
		node = node->children[1];
	}
}

// ===========================================================================

/*
=================
R_GetClippedFragments
=================
*/
static uint32 R_GetClippedFragments (vec3_t origin, float radius, vec3_t axis[3])
{
	int		i;
	float	d;

	if (ri.def.rdFlags & RDF_NOWORLDMODEL)
		return 0;
	if (!ri.scn.worldModel->bspModel.nodes)
		return 0;

	r_fragmentFrame++;

	// Store data
	Vec3Copy (origin, r_decalOrigin);
	Vec3Copy (axis[0], r_decalNormal);
	r_decalRadius = radius;

	// Initialize fragments
	r_numFragmentVerts = 0;
	r_numClippedFragments = 0;

	// Calculate clipping planes
	for (i=0 ; i<3; i++) {
		d = DotProduct (origin, axis[i]);

		Vec3Copy (axis[i], r_fragmentPlanes[i*2].normal);
		r_fragmentPlanes[i*2].dist = d - radius;
		r_fragmentPlanes[i*2].type = PlaneTypeForNormal (r_fragmentPlanes[i*2].normal);

		Vec3Negate (axis[i], r_fragmentPlanes[i*2+1].normal);
		r_fragmentPlanes[i*2+1].dist = -d - radius;
		r_fragmentPlanes[i*2+1].type = PlaneTypeForNormal (r_fragmentPlanes[i*2+1].normal);
	}

	if (ri.scn.worldModel->type == MODEL_Q3BSP)
		R_Q3BSP_FragmentNode ();
	else
		R_Q2BSP_FragmentNode (ri.scn.worldModel->bspModel.nodes);

	return r_numClippedFragments;
}

/*
==============================================================================

	EXPORT FUNCTIONS

==============================================================================
*/

/*
===============
R_CreateDecal
===============
*/
qBool R_CreateDecal (refDecal_t *d, struct material_s *material, vec3_t origin, vec3_t direction, float angle, float size)
{
	vec3_t			*clipNormals, *clipVerts;
	refFragment_t	*fr, *clipFragments;
	vec3_t			axis[3];
	uint32			numFragments, i, k;
	byte			*buffer;
	uint32			totalIndexes;
	index_t			*outIndexes;
	uint32			totalVerts;
	float			*outNormals;
	float			*outVerts;
	float			*outCoords;
	uint32			totalSurfaces;
	mBspSurface_t	**outSurfs;
	vec3_t			mins, maxs;
	vec3_t			temp;
	int				j;

	if (!d)
		return qFalse;

	// See if there's room and it's valid
	if (!size || Vec3Compare (direction, vec3Origin)) {
		Com_DevPrintf (PRNT_WARNING, "WARNING: attempted to create a decal with an invalid %s\n", !size ? "size" : "direction");
		return qFalse;
	}

	// Negativity check
	if (size < 0)
		size *= -1;

	// Calculate orientation matrix
	VectorNormalizef (direction, axis[0]);
	PerpendicularVector (axis[0], axis[1]);
	RotatePointAroundVector (axis[2], axis[0], axis[1], angle);
	CrossProduct (axis[0], axis[2], axis[1]);

	// Clip it
	clipNormals = r_fragmentNormals;
	clipVerts = r_fragmentVerts;
	clipFragments = r_clippedFragments;
	numFragments = R_GetClippedFragments (origin, size, axis);
	if (!numFragments)
		return qFalse;	// No valid fragments

	// Find the total allocation size
	totalIndexes = 0;
	totalVerts = 0;
	totalSurfaces = 0;
	for (i=0, fr=clipFragments ; i<numFragments ; fr++, i++) {
		totalIndexes += (fr->numVerts - 2) * 3;
		totalVerts += fr->numVerts;

		// NULL out duplicate surfaces to save redundant cull attempts
		if (fr->surf) {
			for (k=i+1 ; k<numFragments ; k++) {
				if (clipFragments[k].surf == fr->surf)
					fr->surf = NULL;
			}

			if (fr->surf)
				totalSurfaces++;
		}
	}
	assert (totalIndexes && totalVerts);

	// Store values
	Vec3Copy (origin, d->origin);
	d->poly.mat = material;
	d->numIndexes = totalIndexes;
	d->poly.numVerts = totalVerts;
	d->numSurfaces = totalSurfaces;

	// Allocate space
	buffer = Mem_PoolAlloc ((d->poly.numVerts * sizeof (vec3_t) * 2)
							+ (d->numIndexes * sizeof (index_t))
							+ (d->poly.numVerts * sizeof (vec2_t))
							+ (d->poly.numVerts * sizeof (bvec4_t))
							+ (d->numSurfaces * sizeof (struct mBspSurface_s *)), ri.decalSysPool, 0);
	outVerts = (float *)buffer;
	d->poly.vertices = (vec3_t *)buffer;

	buffer += d->poly.numVerts * sizeof (vec3_t);
	outNormals = (float *)buffer;
	d->normals = (vec3_t *)buffer;

	buffer += d->poly.numVerts * sizeof (vec3_t);
	outIndexes = d->indexes = (int *)buffer;

	buffer += d->numIndexes * sizeof (index_t);
	outCoords = (float *)buffer;
	d->poly.texCoords = (vec2_t *)buffer;

	buffer += d->poly.numVerts * sizeof (vec2_t);
	d->poly.colors = (bvec4_t *)buffer;

	buffer += d->poly.numVerts * sizeof (bvec4_t);
	d->surfaces = outSurfs = (struct mBspSurface_s **)buffer;

	// Store vertex data
	ClearBounds (mins, maxs);
	totalVerts = 0;
	totalIndexes = 0;

	size = 0.5f / size;
	Vec3Scale (axis[1], size, axis[1]);
	Vec3Scale (axis[2], size, axis[2]);
	for (i=0, fr=clipFragments ; i<numFragments ; fr++, i++) {
		if (fr->surf)
			*outSurfs++ = fr->surf;

		// Indexes
		outIndexes = d->indexes + totalIndexes;
		totalIndexes += (fr->numVerts - 2) * 3;
		for (j=2 ; j<fr->numVerts ; j++) {
			outIndexes[0] = totalVerts;
			outIndexes[1] = totalVerts + j - 1;
			outIndexes[2] = totalVerts + j;

			outIndexes += 3;
		}

		for (j=0 ; j<fr->numVerts ; j++) {
			// Vertices
			outVerts[0] = clipVerts[fr->firstVert+j][0];
			outVerts[1] = clipVerts[fr->firstVert+j][1];
			outVerts[2] = clipVerts[fr->firstVert+j][2];

			// Normals
			outNormals[0] = clipNormals[fr->firstVert+j][0];
			outNormals[1] = clipNormals[fr->firstVert+j][1];
			outNormals[2] = clipNormals[fr->firstVert+j][2];

			// Bounds
			AddPointToBounds (outVerts, mins, maxs);

			// Coords
			Vec3Subtract (outVerts, origin, temp);
			outCoords[0] = DotProduct (temp, axis[1]) + 0.5f;
			outCoords[1] = DotProduct (temp, axis[2]) + 0.5f;

			outVerts += 3;
			outNormals += 3;
			outCoords += 2;
		}

		totalVerts += fr->numVerts;
	}

	// Calculate radius
	d->radius = RadiusFromBounds (mins, maxs);
	assert (d->radius);
	return qTrue;
}


/*
===============
R_FreeDecal

Releases decal memory for index and vertex data.
===============
*/
qBool R_FreeDecal (refDecal_t *d)
{
	if (!d || !d->poly.vertices)
		return qFalse;

	Mem_Free (d->poly.vertices);
	d->poly.vertices = NULL;
	return qTrue;
}
