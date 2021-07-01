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
	Generic meshes can be passed from CGAME/UI to be rendered here
 
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
	poly_t	*p;
	int		i;

	// Add poly meshes to list
	for (p=r_polyList, i=0 ; i<r_numPolys ; i++, p++) {
		if (!p->shader)
			continue;

		R_AddMeshToList (p->shader, NULL, MBT_POLY, p);
	}
}


/*
================
R_PushPoly
================
*/
void R_PushPoly (meshBuffer_t *mb)
{
	poly_t	*p;
	int		features, i;

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

	features = MF_TRIFAN|p->shader->features;
	if (!(p->shader->flags & SHADER_ENTITY_MERGABLE))
		features |= MF_NONBATCHED;

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
================
R_DrawPoly
================
*/
void R_DrawPoly (meshBuffer_t *mb)
{
	RB_RenderMeshBuffer (mb, qFalse);
}

/*
==============================================================================

	FRAGMENT CLIPPING
 
==============================================================================
*/

static uInt			r_numFragmentVerts;
static uInt			r_maxFragmentVerts;
static vec3_t		*r_fragmentVerts;

static uInt			r_numClippedFragments;
static uInt			r_maxClippedFragments;
static fragment_t	*r_clippedFragments;

static uLong		r_fragmentFrame = 0;
static cBspPlane_t	r_fragmentPlanes[6];

/*
=================
R_ClipPoly
=================
*/
void R_ClipPoly (int nump, vec4_t vecs, int stage, fragment_t *fr)
{
	cBspPlane_t	*plane;
	qBool		front, back;
	vec4_t		newv[MAX_POLY_VERTS];
	int			sides[MAX_POLY_VERTS];
	float		dists[MAX_POLY_VERTS];
	float		*v, d;
	int			newc, i, j;

	if (nump > MAX_POLY_VERTS - 2) {
		Com_Printf (0, S_COLOR_RED "R_ClipPoly: nump > MAX_POLY_VERTS - 2");
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

		if ((sides[i] == SIDE_ON) || (sides[i+1] == SIDE_ON) || (sides[i+1] == sides[i]))
			continue;

		d = dists[i] / (dists[i] - dists[i+1]);
		for (j=0 ; j<3 ; j++)
			newv[newc][j] = v[j] + d * (v[j+4] - v[j]);
		newc++;
	}

	// Continue
	R_ClipPoly (newc, newv[0], stage+1, fr);
}


/*
=================
R_PlanarSurfClipFragment
=================
*/
void R_PlanarSurfClipFragment (mBspNode_t *node, mBspSurface_t *surf, vec3_t normal)
{
	int			i;
	float		*v, *v2, *v3;
	fragment_t	*fr;
	vec4_t		verts[MAX_POLY_VERTS];

	// Bogus face
	if (surf->numEdges < 3)
		return;

	// Greater than 60 degrees
	if (surf->flags & SURF_PLANEBACK) {
		if (-DotProduct (normal, surf->plane->normal) < 0.5)
			return;
	}
	else {
		if (DotProduct (normal, surf->plane->normal) < 0.5)
			return;
	}

	v = surf->polys->mesh.vertexArray[0];
	// Copy vertex data and clip to each triangle
	for (i=0; i<surf->polys->mesh.numVerts-2 ; i++) {
		fr = &r_clippedFragments[r_numClippedFragments];
		fr->numVerts = 0;
		fr->node = node;

		v2 = surf->polys->mesh.vertexArray[0] + (i+1) * 3;
		v3 = surf->polys->mesh.vertexArray[0] + (i+2) * 3;

		VectorCopy (v , verts[0]);
		VectorCopy (v2, verts[1]);
		VectorCopy (v3, verts[2]);

		R_ClipPoly (3, verts[0], 0, fr);

		if (fr->numVerts) {
			r_numClippedFragments++;

			if (r_numFragmentVerts >= r_maxFragmentVerts || r_numClippedFragments >= r_maxClippedFragments)
				return;
		}
	}
}


/*
=================
R_RecursiveFragmentNode
=================
*/
void R_RecursiveFragmentNode (mBspNode_t *node, vec3_t origin, float radius, vec3_t normal)
{
	float		dist;
	cBspPlane_t	*plane;

mark0:
	if (r_numFragmentVerts >= r_maxFragmentVerts || r_numClippedFragments >= r_maxClippedFragments)
		return;	// Already reached the limit somewhere else
	if (node->contents == CONTENTS_SOLID)
		return;

	if (node->contents != -1) {
		// Leaf
		int				c;
		mBspLeaf_t		*leaf;
		mBspSurface_t	*surf, **mark;

		leaf = (mBspLeaf_t *)node;
		if (!(c = leaf->numMarkSurfaces))
			return;

		mark = leaf->firstMarkSurface;
		do {
			if (r_numFragmentVerts >= r_maxFragmentVerts || r_numClippedFragments >= r_maxClippedFragments)
				return;

			surf = *mark++;
			if (!surf)
				continue;

			if (surf->fragmentFrame == r_fragmentFrame)
				continue;
			surf->fragmentFrame = r_fragmentFrame;

			if (surf->texInfo->flags & (SURF_TEXINFO_SKY|SURF_TEXINFO_WARP|SURF_TEXINFO_NODRAW))
				continue;

			if (surf->texInfo && surf->texInfo->shader && surf->texInfo->shader->flags & SHADER_NOMARK)
				continue;

			R_PlanarSurfClipFragment (node, surf, normal);
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

	R_RecursiveFragmentNode (node->children[0], origin, radius, normal);
	R_RecursiveFragmentNode (node->children[1], origin, radius, normal);
}


/*
=================
R_GetClippedFragments
=================
*/
uInt R_GetClippedFragments (vec3_t origin, float radius, vec3_t axis[3],
						   int maxFragmentVerts, vec3_t *fragmentVerts,
						   int maxFragments, fragment_t *fragments)
{
	int		i;
	float	d;

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

	R_RecursiveFragmentNode (r_worldModel->nodes, origin, radius, axis[0]);

	return r_numClippedFragments;
}
