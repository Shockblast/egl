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
// r_fx.c
// Particle rendering
// Decal clipping and rendering
//

#include "r_local.h"

#define FX_USERB	0//!e_test_1->integer //orly


/*
==============================================================================

	DECAL RENDERING
 
==============================================================================
*/

/*
===============
R_DrawDecals
===============
*/
void R_DrawDecals (void) {
	decal_t		*d;
	bvec4_t		color;
	int			i, j;

	qglDepthMask (GL_FALSE);
	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglEnable (GL_POLYGON_OFFSET_FILL);

	for (d=r_Decals, i=0 ; i<r_NumDecals ; i++, d++) {
		// texturing
		if (!FX_USERB)
			GL_Bind (d->image);

		// coloring
		VectorCopy (d->color, color);
		color[3] = d->color[3] * 255.0;
		if (FX_USERB) {
			colorArray[0][0] = color[0] * ONEDIV255;
			colorArray[0][1] = color[1] * ONEDIV255;
			colorArray[0][2] = color[2] * ONEDIV255;
			colorArray[0][3] = d->color[3];
		}
		else
			qglColor4ubv (color);

		// rendering
		qglEnable (GL_BLEND);
		GL_BlendFunc (d->sFactor, d->dFactor);

		if (r_fog->integer) {
			// don't fog w/ glowing blendfuncs (looks horrible)
			if ((d->sFactor != GL_ONE_MINUS_SRC_ALPHA) && (d->dFactor != GL_SRC_ALPHA))
				qglDisable (GL_FOG);
			else
				qglEnable (GL_FOG);
		}

		if (FX_USERB) {
			rbMesh_t	mesh;

			for (j=0 ; j<d->numVerts ; j++) {
				VectorSet (normalsArray[j], 0, 1, 0); // kthx actual normal
			}

			mesh.numColors = 1;
			mesh.numIndexes = (d->numVerts-2)*3;
			mesh.numVerts = d->numVerts;

			mesh.colorArray = colorArray;
			mesh.coordArray = d->coords;
			mesh.lmCoordArray = NULL;
			mesh.indexArray = rbTriFanIndices;
			mesh.normalsArray = normalsArray;
			mesh.vertexArray = d->verts;

			mesh.surface = NULL;

			mesh.texture = d->image;
			mesh.lmTexture = NULL;
			mesh.shader = NULL;

			RB_RenderMesh (&mesh);
		}
		else {
			qglBegin (GL_TRIANGLE_FAN);
			for (j=0 ; j<d->numVerts ; j++) {
				qglTexCoord2fv (d->coords[j]);
				qglVertex3fv (d->verts[j]);
			}
			qglEnd ();
		}
	}
	glSpeeds.numDecals += r_NumDecals;

	// finish up
	qglDisable (GL_POLYGON_OFFSET_FILL);

	if (r_fog->integer)
		qglEnable (GL_FOG);

	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDisable (GL_BLEND);
	qglDepthMask (GL_TRUE);
}

/*
==============================================================================

	DECAL CLIPPING
 
==============================================================================
*/

static uInt			numFragmentVerts;
static uInt			maxFragmentVerts;
static vec3_t		*fragmentVerts;

static uInt			numClippedFragments;
static uInt			maxClippedFragments;
static fragment_t	*clippedFragments;

static uLong		fragmentFrame = 0;
static cBspPlane_t	fragmentPlanes[6];

/*
=================
R_ClipPoly
=================
*/
void R_ClipPoly (int nump, vec4_t vecs, int stage, fragment_t *fr) {
	cBspPlane_t	*plane;
	qBool		front, back;
	vec4_t		newv[MAX_DECAL_VERTS];
	int			sides[MAX_DECAL_VERTS];
	float		dists[MAX_DECAL_VERTS];
	float		*v, d;
	int			newc, i, j;

	if (nump > (MAX_DECAL_VERTS - 2)) {
		Com_Printf (0, S_COLOR_RED "R_ClipPoly: nump > MAX_DECAL_VERTS - 2");
		return;
	}

	if (stage == 6) {
		// fully clipped
		if (nump > 2) {
			fr->numVerts = nump;
			fr->firstVert = numFragmentVerts;

			if (numFragmentVerts+nump >= maxFragmentVerts)
				nump = maxFragmentVerts - numFragmentVerts;

			for (i=0, v=vecs ; i<nump ; i++, v+=4)
				VectorCopy (v, fragmentVerts[numFragmentVerts + i]);

			numFragmentVerts += nump;
		}

		return;
	}

	front = back = qFalse;
	plane = &fragmentPlanes[stage];
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

	// clip it
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

	// continue
	R_ClipPoly (newc, newv[0], stage+1, fr);
}


/*
=================
R_PlanarSurfClipFragment
=================
*/
void R_PlanarSurfClipFragment (mBspNode_t *node, mBspSurface_t *surf, vec3_t normal) {
	int			i;
	float		*v, *v2, *v3;
	fragment_t	*fr;
	vec4_t		verts[MAX_DECAL_VERTS];

	// bogus face
	if (surf->numEdges < 3)
		return;

	// greater than 60 degrees
	if (surf->flags & SURF_PLANEBACK) {
		if (-DotProduct (normal, surf->plane->normal) < 0.5)
			return;
	}
	else {
		if (DotProduct (normal, surf->plane->normal) < 0.5)
			return;
	}

	v = surf->polys->vertices[0];
	// copy vertex data and clip to each triangle
	for (i=0; i<surf->polys->numVerts-2 ; i++) {
		fr = &clippedFragments[numClippedFragments];
		fr->numVerts = 0;
		fr->node = node;

		v2 = surf->polys->vertices[0] + (i+1) * 3;
		v3 = surf->polys->vertices[0] + (i+2) * 3;

		VectorCopy (v , verts[0]);
		VectorCopy (v2, verts[1]);
		VectorCopy (v3, verts[2]);

		R_ClipPoly (3, verts[0], 0, fr);

		if (fr->numVerts) {
			numClippedFragments++;

			if ((numFragmentVerts >= maxFragmentVerts) || (numClippedFragments >= maxClippedFragments))
				return;
		}
	}
}


/*
=================
R_RecursiveFragmentNode
=================
*/
void R_RecursiveFragmentNode (mBspNode_t *node, vec3_t origin, float radius, vec3_t normal) {
	float		dist;
	cBspPlane_t	*plane;

mark0:
	if ((numFragmentVerts >= maxFragmentVerts) || (numClippedFragments >= maxClippedFragments))
		return;	// already reached the limit somewhere else

	if (node->contents == CONTENTS_SOLID)
		return;

	if (node->contents != -1) {
		// leaf
		int				c;
		mBspLeaf_t		*leaf;
		mBspSurface_t	*surf, **mark;

		leaf = (mBspLeaf_t *)node;
		if (!(c = leaf->numMarkSurfaces))
			return;

		mark = leaf->firstMarkSurface;
		do {
			if ((numFragmentVerts >= maxFragmentVerts) || (numClippedFragments >= maxClippedFragments))
				return;

			surf = *mark++;
			if (!surf)
				continue;

			if (surf->fragmentFrame == fragmentFrame)
				continue;

			surf->fragmentFrame = fragmentFrame;
			if (surf->texInfo->flags & (SURF_SKY|SURF_WARP|SURF_NODRAW))
				continue;

			if (surf->texInfo && surf->texInfo->shader && surf->texInfo->shader->noMark)
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
						   int maxfverts, vec3_t *fverts,
						   int maxfragments, fragment_t *fragments) {
	int		i;
	float	d;

	fragmentFrame++;

	// initialize fragments
	numFragmentVerts = 0;
	maxFragmentVerts = maxfverts;
	fragmentVerts = fverts;

	numClippedFragments = 0;
	maxClippedFragments = maxfragments;
	clippedFragments = fragments;

	// calculate clipping planes
	for (i=0 ; i<3; i++) {
		d = DotProduct (origin, axis[i]);

		VectorCopy (axis[i], fragmentPlanes[i*2].normal);
		fragmentPlanes[i*2].dist = d - radius;
		fragmentPlanes[i*2].type = PlaneTypeForNormal (fragmentPlanes[i*2].normal);

		VectorNegate (axis[i], fragmentPlanes[i*2+1].normal);
		fragmentPlanes[i*2+1].dist = -d - radius;
		fragmentPlanes[i*2+1].type = PlaneTypeForNormal (fragmentPlanes[i*2+1].normal);
	}

	R_RecursiveFragmentNode (r_WorldModel->nodes, origin, radius, axis[0]);

	return numClippedFragments;
}

/*
==============================================================================

	PARTICLE RENDERING
 
==============================================================================
*/

/*
===============
R_DrawParticles
===============
*/
void R_DrawParticles (void) {
	particle_t	*p;
	int			i, j;
	vec3_t		p_Up, p_Right;
	vec3_t		tmp_Up, tmp_Right;
	vec3_t		vertcoords[4];
	vec3_t		point, width;
	vec3_t		move;
	bvec4_t		color;
	float		scale;

	qglDepthMask (GL_FALSE);
	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (gl_cull->integer)
		qglDisable (GL_CULL_FACE);

	VectorScale (v_Up,		0.75,	p_Up);
	VectorScale (v_Right,	0.75,	p_Right);

	VectorAdd (p_Up, p_Right, vertcoords[0]);
	VectorSubtract (p_Right, p_Up, vertcoords[1]);
	VectorNegate (vertcoords[0], vertcoords[2]);
	VectorNegate (vertcoords[1], vertcoords[3]);

	for (p=r_Particles, i=0 ; i<r_NumParticles ; i++, p++) {
		// texturing
		if (!FX_USERB)
			GL_Bind (p->image);

		// scaling
		if (p->flags & PF_SCALED) {
			scale = (p->origin[0] - r_RefDef.viewOrigin[0]) * v_Forward[0] +
					(p->origin[1] - r_RefDef.viewOrigin[1]) * v_Forward[1] +
					(p->origin[2] - r_RefDef.viewOrigin[2]) * v_Forward[2];

			scale = (scale < 20) ? 1 : 1 + scale * 0.004;
		}
		else
			scale = 1;

		scale = (scale - 1) + p->size;

		// coloring
		VectorCopy (p->color, color);
		color[3] = p->color[3] * 255.0;

		for (j=0 ; j<4 ; j++) {
			if (color[j] > 255)
				color[j] = 255;
			colorArray[0][j] = color[j] / 255.0;
		}

		if (!FX_USERB)
			qglColor4ubv (color);

		// rendering
		qglEnable (GL_BLEND);
		GL_BlendFunc (p->sFactor, p->dFactor);

		if (r_fog->integer) {
			// don't fog w/ glowing blendfuncs (looks horrible)
			if ((p->sFactor != GL_ONE_MINUS_SRC_ALPHA) && (p->dFactor != GL_SRC_ALPHA))
				qglDisable (GL_FOG);
			else
				qglEnable (GL_FOG);
		}

		if (p->flags & PF_DEPTHHACK)
			qglDepthRange (0, 0.3);

		if (p->flags & PF_BEAM) {
			VectorSubtract (p->origin, r_RefDef.viewOrigin, point);
			CrossProduct (point, p->angle, width);
			VectorNormalize (width, width);
			VectorScale (width, scale, width);

			if (FX_USERB) {
				Vector2Set (coordArray[0], 1, 1);
				VectorSet (normalsArray[0], 0, 1, 0);
				VectorSet (vertexArray[0],	p->origin[0] + width[0],
											p->origin[1] + width[1],
											p->origin[2] + width[2]);

				Vector2Set (coordArray[1], 0, 0);
				VectorSet (normalsArray[1], 0, 1, 0);
				VectorSet (vertexArray[1],	p->origin[0] - width[0],
											p->origin[1] - width[1],
											p->origin[2] - width[2]);
			}
			else {
				qglBegin (GL_QUADS);
				qglTexCoord2f (1, 1);
				qglVertex3f(p->origin[0] + width[0],
							p->origin[1] + width[1],
							p->origin[2] + width[2]);

				qglTexCoord2f (0, 0);
				qglVertex3f(p->origin[0] - width[0],
							p->origin[1] - width[1],
							p->origin[2] - width[2]);
			}

			VectorAdd (point, p->angle, point);
			CrossProduct (point, p->angle, width);
			VectorNormalize (width, width);
			VectorScale (width, scale, width);

			if (FX_USERB) {
				rbMesh_t	mesh;

				Vector2Set (coordArray[2], 0, 0);
				VectorSet (normalsArray[2], 0, 1, 0);
				VectorSet (vertexArray[2],	p->origin[0] + p->angle[0] - width[0],
											p->origin[1] + p->angle[1] - width[1],
											p->origin[2] + p->angle[2] - width[2]);

				Vector2Set (coordArray[3], 1, 1);
				VectorSet (normalsArray[3], 0, 1, 0);
				VectorSet (vertexArray[3],	p->origin[0] + p->angle[0] + width[0],
											p->origin[1] + p->angle[1] + width[1],
											p->origin[2] + p->angle[2] + width[2]);

				mesh.numColors = 1;
				mesh.numIndexes = QUAD_INDEXES;
				mesh.numVerts = QUAD_INDEXES;

				mesh.colorArray = colorArray;
				mesh.coordArray = coordArray;
				mesh.lmCoordArray = NULL;
				mesh.indexArray = rbQuadIndices;
				mesh.normalsArray = normalsArray;
				mesh.vertexArray = vertexArray;

				mesh.surface = NULL;

				mesh.texture = p->image;
				mesh.lmTexture = NULL;
				mesh.shader = NULL;

				RB_RenderMesh (&mesh);
			}
			else {
				qglTexCoord2f (0, 0);
				qglVertex3f(p->origin[0] + p->angle[0] - width[0],
							p->origin[1] + p->angle[1] - width[1],
							p->origin[2] + p->angle[2] - width[2]);

				qglTexCoord2f (1, 1);
				qglVertex3f(p->origin[0] + p->angle[0] + width[0],
							p->origin[1] + p->angle[1] + width[1],
							p->origin[2] + p->angle[2] + width[2]);
				qglEnd ();
			}
		}
 else if (p->flags & PF_SPIRAL) {
			vec3_t	vec, dir1, dir2, dir3, spdir;
			float	len, loc, c, d, s;

			VectorCopy (p->origin, move);
			VectorCopy (p->angle, vec);
			len = VectorNormalize (vec, vec);
			MakeNormalVectors (vec, tmp_Right, tmp_Up);

			for (loc = p->orient; loc<(len + p->orient) ; loc++) {
				d = loc * 0.2;
				c = cos (d);
				s = sin (d);

				VectorScale (tmp_Right, c * scale, dir1);
				VectorMA (dir1, s * scale, tmp_Up, dir1);

				d = (loc + 1) * 0.2;
				c = cos (d);
				s = sin (d);

				VectorScale (tmp_Right, c * scale, dir2);
				VectorMA (dir2, s * scale, tmp_Up, dir2);
				VectorAdd (dir2, vec, dir2);

				d = (loc + 2) * 0.2;
				c = cos (d);
				s = sin (d);

				VectorScale (tmp_Right, c * scale, dir3);
				VectorMA (dir3, s * scale, tmp_Up, dir3);
				VectorMA (dir3, 2, vec, dir3);

				VectorAdd (move, dir1, point);

				VectorSubtract (dir2, dir1, spdir);

				VectorSubtract (point, r_RefDef.viewOrigin, point);
				CrossProduct (point, spdir, width);

				if (VectorLengthFast (width))
					VectorNormalize (width, width);
				else
					VectorCopy (p_Up, width);

				if (FX_USERB) {
					Vector2Set (coordArray[0], 0.5, 1);
					VectorSet (normalsArray[0], 0, 1, 0);
					VectorSet (vertexArray[0],	point[0] + width[0] + r_RefDef.viewOrigin[0],
												point[1] + width[1] + r_RefDef.viewOrigin[1],
												point[2] + width[2] + r_RefDef.viewOrigin[2]);

					Vector2Set (coordArray[1], 0.5, 0);
					VectorSet (normalsArray[1], 0, 1, 0);
					VectorSet (vertexArray[1],	point[0] - width[0] + r_RefDef.viewOrigin[0],
												point[1] - width[1] + r_RefDef.viewOrigin[1],
												point[2] - width[2] + r_RefDef.viewOrigin[2]);
				}
				else {
					qglBegin (GL_QUADS);
					qglTexCoord2f (0.5, 1);
					qglVertex3f(point[0] + width[0] + r_RefDef.viewOrigin[0],
								point[1] + width[1] + r_RefDef.viewOrigin[1],
								point[2] + width[2] + r_RefDef.viewOrigin[2]);

					qglTexCoord2f (0.5, 0);
					qglVertex3f(point[0] - width[0] + r_RefDef.viewOrigin[0],
								point[1] - width[1] + r_RefDef.viewOrigin[1],
								point[2] - width[2] + r_RefDef.viewOrigin[2]);
				}

				VectorAdd (move, dir2, point);
				VectorSubtract (dir3, dir2, spdir);

				VectorSubtract (point, r_RefDef.viewOrigin, point);
				CrossProduct (point, spdir, width);

				if (VectorLengthFast (width))
					VectorNormalize (width, width);
				else
					VectorCopy (p_Up, width);

				if (FX_USERB) {
					rbMesh_t	mesh;

					Vector2Set (coordArray[2], 0.5, 0);
					VectorSet (normalsArray[2], 0, 1, 0);
					VectorSet (vertexArray[2],	point[0] - width[0] + r_RefDef.viewOrigin[0],
												point[1] - width[1] + r_RefDef.viewOrigin[1],
												point[2] - width[2] + r_RefDef.viewOrigin[2]);

					Vector2Set (coordArray[3], 0.5, 1);
					VectorSet (normalsArray[3], 0, 1, 0);
					VectorSet (vertexArray[3],	point[0] + width[0] + r_RefDef.viewOrigin[0],
												point[1] + width[1] + r_RefDef.viewOrigin[1],
												point[2] + width[2] + r_RefDef.viewOrigin[2]);

					mesh.numColors = 1;
					mesh.numIndexes = QUAD_INDEXES;
					mesh.numVerts = QUAD_INDEXES;

					mesh.colorArray = colorArray;
					mesh.coordArray = coordArray;
					mesh.lmCoordArray = NULL;
					mesh.indexArray = rbQuadIndices;
					mesh.normalsArray = normalsArray;
					mesh.vertexArray = vertexArray;

					mesh.surface = NULL;

					mesh.texture = p->image;
					mesh.lmTexture = NULL;
					mesh.shader = NULL;

					RB_RenderMesh (&mesh);
				}
				else {
					qglTexCoord2f (0.5, 0);
					qglVertex3f(point[0] - width[0] + r_RefDef.viewOrigin[0],
								point[1] - width[1] + r_RefDef.viewOrigin[1],
								point[2] - width[2] + r_RefDef.viewOrigin[2]);

					qglTexCoord2f (0.5, 1);
					qglVertex3f(point[0] + width[0] + r_RefDef.viewOrigin[0],
								point[1] + width[1] + r_RefDef.viewOrigin[1],
								point[2] + width[2] + r_RefDef.viewOrigin[2]);
					qglEnd ();
				}

				VectorAdd (move, vec, move);
			}
		}
		else if (p->flags & PF_DIRECTION) {
			vec3_t	delta, vdelta;

			VectorAdd (p->angle, p->origin, vdelta);

			VectorSubtract (p->origin, vdelta, move);
			VectorNormalize (move, move);

			VectorCopy (move, tmp_Up);
			VectorSubtract (r_RefDef.viewOrigin, vdelta, delta);
			CrossProduct (tmp_Up, delta, tmp_Right);

			VectorNormalize (tmp_Right, tmp_Right);

			VectorScale (tmp_Right, 0.75f, tmp_Right);
			VectorScale (tmp_Up, 0.75f * VectorLengthFast (p->angle), tmp_Up);
			if (FX_USERB) {
				rbMesh_t	mesh;

				// top left
				Vector2Set (coordArray[0], 0, 0);
				VectorSet (normalsArray[0], p->angle[0], p->angle[1], p->angle[2]); // kthx actual normal?
				VectorSet (vertexArray[0],	p->origin[0] + tmp_Up[0]*scale - tmp_Right[0]*scale,
											p->origin[1] + tmp_Up[1]*scale - tmp_Right[1]*scale,
											p->origin[2] + tmp_Up[2]*scale - tmp_Right[2]*scale);

				// bottom left
				Vector2Set (coordArray[1], 0, 1);
				VectorSet (normalsArray[1], p->angle[0], p->angle[1], p->angle[2]); // kthx actual normal?
				VectorSet (vertexArray[1],	p->origin[0] - tmp_Up[0]*scale - tmp_Right[0]*scale,
											p->origin[1] - tmp_Up[1]*scale - tmp_Right[1]*scale,
											p->origin[2] - tmp_Up[2]*scale - tmp_Right[2]*scale);

				// bottom right
				Vector2Set (coordArray[2], 1, 1);
				VectorSet (normalsArray[2], p->angle[0], p->angle[1], p->angle[2]); // kthx actual normal?
				VectorSet (vertexArray[2],	p->origin[0] - tmp_Up[0]*scale + tmp_Right[0]*scale,
											p->origin[1] - tmp_Up[1]*scale + tmp_Right[1]*scale,
											p->origin[2] - tmp_Up[2]*scale + tmp_Right[2]*scale);

				// top right
				Vector2Set (coordArray[3], 1, 0);
				VectorSet (normalsArray[3], p->angle[0], p->angle[1], p->angle[2]); // kthx actual normal?
				VectorSet (vertexArray[3],	p->origin[0] + tmp_Up[0]*scale + tmp_Right[0]*scale,
											p->origin[1] + tmp_Up[1]*scale + tmp_Right[1]*scale,
											p->origin[2] + tmp_Up[2]*scale + tmp_Right[2]*scale);

				mesh.numColors = 1;
				mesh.numIndexes = QUAD_INDEXES;
				mesh.numVerts = QUAD_INDEXES;

				mesh.colorArray = colorArray;
				mesh.coordArray = coordArray;
				mesh.lmCoordArray = NULL;
				mesh.indexArray = rbQuadIndices;
				mesh.normalsArray = normalsArray;
				mesh.vertexArray = vertexArray;

				mesh.surface = NULL;

				mesh.texture = p->image;
				mesh.lmTexture = NULL;
				mesh.shader = NULL;

				RB_RenderMesh (&mesh);
			}
			else {
				qglBegin (GL_QUADS);
				qglTexCoord2f (0, 0);	// top left
				qglVertex3f(p->origin[0] + tmp_Up[0]*scale - tmp_Right[0]*scale,
							p->origin[1] + tmp_Up[1]*scale - tmp_Right[1]*scale,
							p->origin[2] + tmp_Up[2]*scale - tmp_Right[2]*scale);

				qglTexCoord2f (0, 1);	// bottom left
				qglVertex3f(p->origin[0] - tmp_Up[0]*scale - tmp_Right[0]*scale,
							p->origin[1] - tmp_Up[1]*scale - tmp_Right[1]*scale,
							p->origin[2] - tmp_Up[2]*scale - tmp_Right[2]*scale);

				qglTexCoord2f (1, 1);	// bottom right
				qglVertex3f(p->origin[0] - tmp_Up[0]*scale + tmp_Right[0]*scale,
							p->origin[1] - tmp_Up[1]*scale + tmp_Right[1]*scale,
							p->origin[2] - tmp_Up[2]*scale + tmp_Right[2]*scale);

				qglTexCoord2f (1, 0);	// top right
				qglVertex3f(p->origin[0] + tmp_Up[0]*scale + tmp_Right[0]*scale,
							p->origin[1] + tmp_Up[1]*scale + tmp_Right[1]*scale,
							p->origin[2] + tmp_Up[2]*scale + tmp_Right[2]*scale);
				qglEnd ();
			}
		}
		else {
			if (p->orient) {
				RotatePointAroundVector (tmp_Right, v_Forward, p_Right, p->orient);
				CrossProduct (v_Forward, tmp_Right, tmp_Up);
			}
			else {
				VectorCopy (p_Right, tmp_Right);
				VectorCopy (p_Up, tmp_Up);
			}

			if (FX_USERB) {
				rbMesh_t	mesh;

				// top left
				Vector2Set (coordArray[0], 0, 0);
				VectorSet (normalsArray[0], 0, 1, 0);
				VectorSet (vertexArray[0],	p->origin[0] + tmp_Up[0]*scale - tmp_Right[0]*scale,
											p->origin[1] + tmp_Up[1]*scale - tmp_Right[1]*scale,
											p->origin[2] + tmp_Up[2]*scale - tmp_Right[2]*scale);

				// bottom left
				Vector2Set (coordArray[1], 0, 1);
				VectorSet (normalsArray[1], 0, 1, 0);
				VectorSet (vertexArray[1],	p->origin[0] - tmp_Up[0]*scale - tmp_Right[0]*scale,
											p->origin[1] - tmp_Up[1]*scale - tmp_Right[1]*scale,
											p->origin[2] - tmp_Up[2]*scale - tmp_Right[2]*scale);

				// bottom right
				Vector2Set (coordArray[2], 1, 1);
				VectorSet (normalsArray[2], 0, 1, 0);
				VectorSet (vertexArray[2],	p->origin[0] - tmp_Up[0]*scale + tmp_Right[0]*scale,
											p->origin[1] - tmp_Up[1]*scale + tmp_Right[1]*scale,
											p->origin[2] - tmp_Up[2]*scale + tmp_Right[2]*scale);

				// top right
				Vector2Set (coordArray[3], 1, 0);
				VectorSet (normalsArray[3], 0, 1, 0);
				VectorSet (vertexArray[3],	p->origin[0] + tmp_Up[0]*scale + tmp_Right[0]*scale,
											p->origin[1] + tmp_Up[1]*scale + tmp_Right[1]*scale,
											p->origin[2] + tmp_Up[2]*scale + tmp_Right[2]*scale);

				mesh.numColors = 1;
				mesh.numIndexes = QUAD_INDEXES;
				mesh.numVerts = QUAD_INDEXES;

				mesh.colorArray = colorArray;
				mesh.coordArray = coordArray;
				mesh.lmCoordArray = NULL;
				mesh.indexArray = rbQuadIndices;
				mesh.normalsArray = normalsArray;
				mesh.vertexArray = vertexArray;

				mesh.surface = NULL;

				mesh.texture = p->image;
				mesh.lmTexture = NULL;
				mesh.shader = NULL;

				RB_RenderMesh (&mesh);
			}
			else {
				qglBegin (GL_QUADS);
				qglTexCoord2f (0, 0);	// top left
				qglVertex3f(p->origin[0] + tmp_Up[0]*scale - tmp_Right[0]*scale,
							p->origin[1] + tmp_Up[1]*scale - tmp_Right[1]*scale,
							p->origin[2] + tmp_Up[2]*scale - tmp_Right[2]*scale);

				qglTexCoord2f (0, 1);	// bottom left
				qglVertex3f(p->origin[0] - tmp_Up[0]*scale - tmp_Right[0]*scale,
							p->origin[1] - tmp_Up[1]*scale - tmp_Right[1]*scale,
							p->origin[2] - tmp_Up[2]*scale - tmp_Right[2]*scale);

				qglTexCoord2f (1, 1);	// bottom right
				qglVertex3f(p->origin[0] - tmp_Up[0]*scale + tmp_Right[0]*scale,
							p->origin[1] - tmp_Up[1]*scale + tmp_Right[1]*scale,
							p->origin[2] - tmp_Up[2]*scale + tmp_Right[2]*scale);

				qglTexCoord2f (1, 0);	// top right
				qglVertex3f(p->origin[0] + tmp_Up[0]*scale + tmp_Right[0]*scale,
							p->origin[1] + tmp_Up[1]*scale + tmp_Right[1]*scale,
							p->origin[2] + tmp_Up[2]*scale + tmp_Right[2]*scale);
				qglEnd ();
			}
		}

		if (p->flags & PF_DEPTHHACK)
			qglDepthRange (0, 1);
	}
	glSpeeds.numParticles += r_NumParticles;

	// finish up
	qglDisable (GL_BLEND);
	qglDepthMask (GL_TRUE);
	qglLineWidth (1);
	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (r_fog->integer)
		qglEnable (GL_FOG);
	if (gl_cull->integer)
		qglEnable (GL_CULL_FACE);
}