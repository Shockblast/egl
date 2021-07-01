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
// r_poly.c
// Mesh handling, generally passed from ui/cgame
//

#include "r_local.h"

/*
==============================================================================

	POLYGON BACKEND
	Generic meshes can be passed from CGAME to be rendered here

==============================================================================
*/

static mesh_t	r_polyMesh;

static bvec4_t	r_polyColors[VERTARRAY_MAX_VERTS];
static vec2_t	r_polyCoords[VERTARRAY_MAX_VERTS];
static vec3_t	r_polyVerts[VERTARRAY_MAX_VERTS];

/*
================
R_AddPolysToList
================
*/
void R_AddPolysToList (void)
{
	poly_t		*p;
	mQ3BspFog_t	*fog;
	int			i;

	if (!r_drawPolys->integer)
		return;

	// Add poly meshes to list
	for (p=r_refScene.polyList, i=0 ; i<r_refScene.numPolys ; i++, p++) {
		fog = R_FogForSphere (p->origin, p->radius);
		R_AddMeshToList (p->shader, p->shaderTime, NULL, fog, MBT_POLY, p);
	}
}


/*
================
R_PushPoly
================
*/
void R_PushPoly (meshBuffer_t *mb)
{
	poly_t			*p;
	meshFeatures_t	features;
	int				i;

	p = (poly_t *)mb->mesh;
	if (p->numVerts > VERTARRAY_MAX_VERTS)
		return;

	for (i=0 ; i<p->numVerts ; i++) {
		VectorCopy (p->vertices[i], r_polyVerts[i]);
		Vector2Copy (p->texCoords[i], r_polyCoords[i]);
		*(int *)r_polyColors[i] = *(int *)p->colors[i];
	}

	r_polyMesh.numIndexes = (p->numVerts - 2) * 3;
	r_polyMesh.numVerts = p->numVerts;

	r_polyMesh.colorArray = r_polyColors;
	r_polyMesh.coordArray = r_polyCoords;
	r_polyMesh.indexArray = rb_triFanIndices;
	r_polyMesh.lmCoordArray = NULL;
	r_polyMesh.normalsArray = NULL;
	r_polyMesh.sVectorsArray = NULL;
	r_polyMesh.tVectorsArray = NULL;
	r_polyMesh.trNeighborsArray = NULL;
	r_polyMesh.trNormalsArray = NULL;
	r_polyMesh.vertexArray = r_polyVerts;

	features = MF_TRIFAN;
	if (p->shader) {
		features |= p->shader->features;
		if (!(p->shader->flags & SHADER_ENTITY_MERGABLE))
			features |= MF_NONBATCHED;
	}

	RB_PushMesh (&r_polyMesh, features);
}


/*
================
R_PolyOverflow
================
*/
qBool R_PolyOverflow (meshBuffer_t *mb)
{
	poly_t	*p;

	p = (poly_t *)mb->mesh;
	return (rb_numVerts + p->numVerts > VERTARRAY_MAX_VERTS);
}

/*
==============================================================================

	FRAGMENT CLIPPING

==============================================================================
*/

static uint32		r_numFragmentVerts;
static uint32		r_maxFragmentVerts;
static vec3_t		*r_fragmentVerts;

static uint32		r_numClippedFragments;
static uint32		r_maxClippedFragments;
static fragment_t	*r_clippedFragments;

static uint32		r_fragmentFrame = 0;
static cBspPlane_t	r_fragmentPlanes[6];

/*
==============================================================================

	QUAKE2 FRAGMENT CLIPPING

==============================================================================
*/

/*
=================
R_Q2BSP_ClipPoly
=================
*/
static void R_Q2BSP_ClipPoly (int nump, vec4_t vecs, int stage, fragment_t *fr)
{
	cBspPlane_t	*plane;
	qBool		front, back;
	vec4_t		newv[MAX_POLY_VERTS];
	int			sides[MAX_POLY_VERTS];
	float		dists[MAX_POLY_VERTS];
	float		*v, d;
	int			newc, i, j;

	if (nump > MAX_POLY_VERTS - 2) {
		Com_Printf (PRNT_ERROR, "R_Q2BSP_ClipPoly: nump > MAX_POLY_VERTS - 2");
		return;
	}

	if (stage == 6) {
		// Fully clipped
		if (nump > 2) {
			fr->numVerts = nump;
			fr->firstVert = r_numFragmentVerts;

			if (r_numFragmentVerts+nump >= r_maxFragmentVerts)
				nump = r_maxFragmentVerts - r_numFragmentVerts;

			for (i=0, v=vecs ; i<nump ; i++, v+=4)
				VectorCopy (v, r_fragmentVerts[r_numFragmentVerts + i]);

			r_numFragmentVerts += nump;
		}

		return;
	}

	front = back = qFalse;
	plane = &r_fragmentPlanes[stage];
	for (i=0, v=vecs ; i<nump ; i++ , v+= 4) {
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
	VectorCopy (vecs, (vecs + (i * 4)));
	newc = 0;

	for (i=0, v=vecs ; i<nump ; i++, v+=4) {
		switch (sides[i]) {
		case SIDE_FRONT:
			VectorCopy (v, newv[newc]);
			newc++;
			break;
		case SIDE_BACK:
			break;
		case SIDE_ON:
			VectorCopy (v, newv[newc]);
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
static void R_Q2BSP_PlanarClipFragment (mBspNode_t *node, mBspSurface_t *surf, vec3_t normal)
{
	int			i;
	float		*v, *v2, *v3;
	fragment_t	*fr;
	vec4_t		verts[MAX_POLY_VERTS];

	// Bogus face
	if (surf->q2_numEdges < 3)
		return;

	// Greater than 60 degrees
	if (surf->q2_flags & SURF_PLANEBACK) {
		if (-DotProduct (normal, surf->q2_plane->normal) < 0.5)
			return;
	}
	else {
		if (DotProduct (normal, surf->q2_plane->normal) < 0.5)
			return;
	}

	v = surf->q2_polys->mesh.vertexArray[0];
	// Copy vertex data and clip to each triangle
	for (i=0; i<surf->q2_polys->mesh.numVerts-2 ; i++) {
		fr = &r_clippedFragments[r_numClippedFragments];
		fr->numVerts = 0;
		fr->node = node;

		v2 = surf->q2_polys->mesh.vertexArray[0] + (i+1) * 3;
		v3 = surf->q2_polys->mesh.vertexArray[0] + (i+2) * 3;

		VectorCopy (v , verts[0]);
		VectorCopy (v2, verts[1]);
		VectorCopy (v3, verts[2]);

		R_Q2BSP_ClipPoly (3, verts[0], 0, fr);

		if (fr->numVerts) {
			r_numClippedFragments++;

			if (r_numFragmentVerts >= r_maxFragmentVerts || r_numClippedFragments >= r_maxClippedFragments)
				return;
		}
	}
}


/*
=================
R_Q2BSP_FragmentNode
=================
*/
static void R_Q2BSP_FragmentNode (mBspNode_t *node, vec3_t origin, float radius, vec3_t normal)
{
	float		dist;
	cBspPlane_t	*plane;

mark0:
	if (r_numFragmentVerts >= r_maxFragmentVerts || r_numClippedFragments >= r_maxClippedFragments)
		return;	// Already reached the limit somewhere else
	if (node->q2_contents == CONTENTS_SOLID)
		return;

	if (node->q2_contents != -1) {
		// Leaf
		int				c;
		mBspLeaf_t		*leaf;
		mBspSurface_t	*surf, **mark;

		leaf = (mBspLeaf_t *)node;
		if (!(c = leaf->q2_numMarkSurfaces))
			return;

		mark = leaf->q2_firstMarkSurface;
		do {
			if (r_numFragmentVerts >= r_maxFragmentVerts || r_numClippedFragments >= r_maxClippedFragments)
				return;

			surf = *mark++;
			if (!surf)
				continue;

			if (surf->fragmentFrame == r_fragmentFrame)
				continue;
			surf->fragmentFrame = r_fragmentFrame;

			if (surf->q2_texInfo->flags & SURF_TEXINFO_NODRAW)
				continue;
			if (surf->q2_texInfo->shader && surf->q2_texInfo->shader->flags & SHADER_NOMARK)
				continue;

			R_Q2BSP_PlanarClipFragment (node, surf, normal);
		} while (--c);

		return;
	}

	plane = node->plane;
	dist = PlaneDiff (origin, plane);

	if (dist > radius) {
		node = node->children[0];
		goto mark0;
	}
	if (dist < -radius) {
		node = node->children[1];
		goto mark0;
	}

	R_Q2BSP_FragmentNode (node->children[0], origin, radius, normal);
	R_Q2BSP_FragmentNode (node->children[1], origin, radius, normal);
}

/*
==============================================================================

	QUAKE3 FRAGMENT CLIPPING

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
static void R_Q3BSP_WindingClipFragment (vec3_t *wVerts, int numVerts, fragment_t *fr)
{
	int			i, j;
	int			stage, newc, numv;
	cBspPlane_t	*plane;
	qBool		front;
	float		*v, *nextv, d;
	float		dists[MAX_POLY_VERTS+1];
	int			sides[MAX_POLY_VERTS+1];
	vec3_t		*verts, *newverts, newv[2][MAX_POLY_VERTS];

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
				if (newc == MAX_POLY_VERTS)
					return;
				VectorCopy (v, newverts[newc]);
				newc++;
				break;

			case SIDE_BACK:
				break;

			case SIDE_ON:
				if (newc == MAX_POLY_VERTS)
					return;
				VectorCopy (v, newverts[newc]);
				newc++;
				break;
			}

			if (sides[i] == SIDE_ON
			|| sides[i+1] == SIDE_ON
			|| sides[i+1] == sides[i])
				continue;
			if (newc == MAX_POLY_VERTS)
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
	if (r_numFragmentVerts + numv > r_maxFragmentVerts)
		return;

	fr->numVerts = numv;
	fr->firstVert = r_numFragmentVerts;

	for (i=0, v=verts[0] ; i<numv ; i++, v+=3)
		VectorCopy (v, r_fragmentVerts[r_numFragmentVerts + i]);
	r_numFragmentVerts += numv;
}


/*
=================
R_Q3BSP_PlanarSurfClipFragment

NOTE: one might want to combine this function with 
R_Q3BSP_WindingClipFragment for special cases like trifans (q1 and
q2 polys) or tristrips for ultra-fast clipping, providing there's 
enough stack space (depending on MAX_FRAGMENT_VERTS value).
=================
*/
static void R_Q3BSP_PlanarSurfClipFragment (mBspSurface_t *surf, mBspNode_t *node, vec3_t normal)
{
	int			i;
	mesh_t		*mesh;
	index_t		*index;
	vec3_t		*verts, tri[3];
	fragment_t	*fr;

	if (DotProduct (normal, surf->q3_origin) < 0.5)
		return;		// Greater than 60 degrees

	mesh = surf->q3_mesh;
	fr = &r_clippedFragments[r_numClippedFragments];
	fr->node = node;
	fr->numVerts = 0;

	// Clip each triangle individually
	index = mesh->indexArray;
	verts = mesh->vertexArray;
	for (i=0 ; i<mesh->numIndexes ; i+=3, index+=3) {
		VectorCopy (verts[index[0]], tri[0]);
		VectorCopy (verts[index[1]], tri[1]);
		VectorCopy (verts[index[2]], tri[2]);

		R_Q3BSP_WindingClipFragment (tri, 3, fr);

		if (fr->numVerts) {
			if (r_numFragmentVerts == r_maxFragmentVerts || ++r_numClippedFragments == r_maxClippedFragments)
				return;

			fr++;
			fr->numVerts = 0;
			fr->node = node;
		}
	}
}

/*
=================
R_Q3BSP_PatchSurfClipFragment
=================
*/
static void R_Q3BSP_PatchSurfClipFragment (mBspSurface_t *surf, mBspNode_t *node, vec3_t normal)
{
	int			i;
	mesh_t		*mesh;
	index_t		*index;
	vec3_t		*verts, tri[3];
	vec3_t		dir1, dir2, snorm;
	fragment_t	*fr;

	mesh = surf->q3_mesh;
	fr = &r_clippedFragments[r_numClippedFragments];
	fr->node = node;
	fr->numVerts = 0;

	// Clip each triangle individually
	index = mesh->indexArray;
	verts = mesh->vertexArray;
	for (i=0 ; i<mesh->numIndexes ; i+=3, index+=3) {
		VectorCopy (verts[index[0]], tri[0]);
		VectorCopy (verts[index[1]], tri[1]);
		VectorCopy (verts[index[2]], tri[2]);

		// Calculate two mostly perpendicular edge directions
		VectorSubtract (tri[0], tri[1], dir1);
		VectorSubtract (tri[2], tri[1], dir2);

		// We have two edge directions, we can calculate a third vector from
		// them, which is the direction of the triangle normal
		CrossProduct (dir1, dir2, snorm);

		// We multiply 0.5 by length of snorm to avoid normalizing
		if (DotProduct (normal, snorm) < 0.5 * VectorLength (snorm))
			continue;	// Greater than 60 degrees

		R_Q3BSP_WindingClipFragment (tri, 3, fr);

		if (fr->numVerts) {
			if (r_numFragmentVerts == r_maxFragmentVerts || ++r_numClippedFragments == r_maxClippedFragments)
				return;

			fr++;
			fr->numVerts = 0;
			fr->node = node;
		}
	}
}


/*
=================
R_Q3BSP_FragmentNode
=================
*/
static void R_Q3BSP_FragmentNode (vec3_t origin, float radius, vec3_t normal)
{
	int				stackdepth = 0;
	float			dist;
	mBspNode_t		*node, *localStack[2048];
	mBspLeaf_t		*leaf;
	mBspSurface_t	*surf, **mark;

	node = r_refScene.worldModel->bspModel.nodes;
	for (stackdepth=0 ; ; ) {
		if (node->plane == NULL) {
			leaf = (mBspLeaf_t *)node;
			if (!leaf->q3_firstFragmentSurface)
				goto nextNodeOnStack;

			mark = leaf->q3_firstFragmentSurface;
			do {
				if (r_numFragmentVerts == r_maxFragmentVerts || r_numClippedFragments == r_maxClippedFragments)
					return;		// Already reached the limit

				surf = *mark++;
				if (surf->fragmentFrame == r_fragmentFrame)
					continue;
				surf->fragmentFrame = r_fragmentFrame;

				if (surf->q3_faceType == FACETYPE_PLANAR)
					R_Q3BSP_PlanarSurfClipFragment (surf, node, normal);
				else
					R_Q3BSP_PatchSurfClipFragment (surf, node, normal);
			} while (*mark);

			if (r_numFragmentVerts == r_maxFragmentVerts || r_numClippedFragments == r_maxClippedFragments)
				return;		// Already reached the limit

nextNodeOnStack:
			if (!stackdepth)
				break;
			node = localStack[--stackdepth];
			continue;
		}

		dist = PlaneDiff (origin, node->plane);
		if (dist > radius) {
			node = node->children[0];
			continue;
		}

		if (dist >= -radius && (stackdepth < sizeof (localStack) / sizeof (mBspNode_t *)))
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
uint32 R_GetClippedFragments (vec3_t origin, float radius, vec3_t axis[3],
						   int maxFragmentVerts, vec3_t *fragmentVerts,
						   int maxFragments, fragment_t *fragments)
{
	int		i;
	float	d;

	if (r_refDef.rdFlags & RDF_NOWORLDMODEL)
		return 0;
	if (!r_refScene.worldModel->bspModel.nodes)
		return 0;

	r_fragmentFrame++;

	// Initialize fragments
	r_numFragmentVerts = 0;
	r_maxFragmentVerts = maxFragmentVerts;
	r_fragmentVerts = fragmentVerts;

	r_numClippedFragments = 0;
	r_maxClippedFragments = maxFragments;
	r_clippedFragments = fragments;

	// Calculate clipping planes
	for (i=0 ; i<3; i++) {
		d = DotProduct (origin, axis[i]);

		VectorCopy (axis[i], r_fragmentPlanes[i*2].normal);
		r_fragmentPlanes[i*2].dist = d - radius;
		r_fragmentPlanes[i*2].type = PlaneTypeForNormal (r_fragmentPlanes[i*2].normal);

		VectorNegate (axis[i], r_fragmentPlanes[i*2+1].normal);
		r_fragmentPlanes[i*2+1].dist = -d - radius;
		r_fragmentPlanes[i*2+1].type = PlaneTypeForNormal (r_fragmentPlanes[i*2+1].normal);
	}

	if (r_refScene.worldModel->type == MODEL_Q3BSP)
		R_Q3BSP_FragmentNode (origin, radius, axis[0]);
	else
		R_Q2BSP_FragmentNode (r_refScene.worldModel->bspModel.nodes, origin, radius, axis[0]);

	return r_numClippedFragments;
}
