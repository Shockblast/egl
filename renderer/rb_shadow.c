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
// rb_shadow.c
//

#include "rb_local.h"

/*
=============================================================================

	VOLUMETRIC SHADOWS

=============================================================================
*/

#ifdef SHADOW_VOLUMES
#define MAX_SHADOWVOLUME_INDEXES	RB_MAX_INDEXES*4

static qBool	rb_triFacingLight[RB_MAX_TRIANGLES];
static index_t	rb_shadowVolIndexes[MAX_SHADOWVOLUME_INDEXES];
static int		rb_numShadowVolTris;

/*
=============
RB_BuildShadowVolumeTriangles
=============
*/
static int RB_BuildShadowVolumeTriangles (void)
{
	int		i, j, tris;
	int		*neighbors = rb.inNeighbors;
	index_t	*indexes = rb.inIndices;
	index_t	*out = rb_shadowVolIndexes;

	// check each frontface for bordering backfaces,
	// and cast shadow polygons from those edges,
	// also create front and back caps for shadow volume
	for (i=0, j=0, tris=0 ; i<rb.numIndexes ; i += 3, j++, indexes += 3, neighbors += 3) {
		if (!rb_triFacingLight[j])
			continue;

		// triangle is frontface and therefore casts shadow,
		// output front and back caps for shadow volume front cap
		out[0] = indexes[0];
		out[1] = indexes[1];
		out[2] = indexes[2];

		// rear cap (with flipped winding order)
		out[3] = indexes[0] + rb.numVerts;
		out[4] = indexes[2] + rb.numVerts;
		out[5] = indexes[1] + rb.numVerts;
		out += 6;
		tris += 2;

		// check the edges
		if (neighbors[0] < 0 || !rb_triFacingLight[neighbors[0]]) {
			out[0] = indexes[1];
			out[1] = indexes[0];
			out[2] = indexes[0] + rb.numVerts;
			out[3] = indexes[1];
			out[4] = indexes[0] + rb.numVerts;
			out[5] = indexes[1] + rb.numVerts;
			out += 6;
			tris += 2;
		}

		if (neighbors[1] < 0 || !rb_triFacingLight[neighbors[1]]) {
			out[0] = indexes[2];
			out[1] = indexes[1];
			out[2] = indexes[1] + rb.numVerts;
			out[3] = indexes[2];
			out[4] = indexes[1] + rb.numVerts;
			out[5] = indexes[2] + rb.numVerts;
			out += 6;
			tris += 2;
		}

		if (neighbors[2] < 0 || !rb_triFacingLight[neighbors[2]]) {
			out[0] = indexes[0];
			out[1] = indexes[2];
			out[2] = indexes[2] + rb.numVerts;
			out[3] = indexes[0];
			out[4] = indexes[2] + rb.numVerts;
			out[5] = indexes[0] + rb.numVerts;
			out += 6;
			tris += 2;
		}
	}

	return tris;
}


/*
=============
RB_MakeTriangleShadowFlagsFromScratch
=============
*/
static void RB_MakeTriangleShadowFlagsFromScratch (vec3_t lightDist, float lightRadius)
{
	float	f;
	int		i, j;
	float	*v0, *v1, *v2;
	vec3_t	dir0, dir1, temp;
	float	*trnormal = rb.inTrNormals[0];
	index_t	*indexes = rb.inIndices;

	for (i=0, j=0 ; i<rb.numIndexes ; i += 3, j++, trnormal += 3, indexes += 3) {
		// Calculate triangle facing flag
		v0 = (float *)(rb.inVertices + indexes[0]);
		v1 = (float *)(rb.inVertices + indexes[1]);
		v2 = (float *)(rb.inVertices + indexes[2]);

		// Calculate two mostly perpendicular edge directions
		Vec3Subtract (v0, v1, dir0);
		Vec3Subtract (v2, v1, dir1);

		// We have two edge directions, we can calculate a third vector from
		// them, which is the direction of the surface normal (it's magnitude
		// is not 1 however)
		CrossProduct (dir0, dir1, temp);

		// Compare distance of light along normal, with distance of any point
		// of the triangle along the same normal (the triangle is planar,
		// I.E. flat, so all points give the same answer)
		f = (lightDist[0] - v0[0]) * temp[0] + (lightDist[1] - v0[1]) * temp[1] + (lightDist[2] - v0[2]) * temp[2];
		rb_triFacingLight[j] = (f > 0) ? qTrue : qFalse;
	}
}


/*
=============
RB_MakeTriangleShadowFlags
=============
*/
static void RB_MakeTriangleShadowFlags (vec3_t lightDist, float lightRadius)
{
	int		i, j;
	float	f;
	float	*v0;
	float	*trnormal = rb.inTrNormals[0];
	index_t	*indexes = rb.inIndices;

	for (i=0, j=0 ; i<rb.numIndexes ; i += 3, j++, trnormal += 3, indexes += 3) {
		v0 = (float *)(rb.inVertices + indexes[0]);

		// compare distance of light along normal, with distance of any point
		// of the triangle along the same normal (the triangle is planar,
		// I.E. flat, so all points give the same answer)
		f = (lightDist[0] - v0[0]) * trnormal[0] + (lightDist[1] - v0[1]) * trnormal[1] + (lightDist[2] - v0[2]) * trnormal[2];
		if (f > 0) {
			rb_triFacingLight[j] = qTrue;
		}
		else {
			rb_triFacingLight[j] = qFalse;
		}
	}
}


/*
=============
RB_ShadowProjectVertices
=============
*/
static void RB_ShadowProjectVertices (vec3_t lightDist, float projectDistance)
{
	vec3_t	diff;
	float	*in, *out;
	int		i;

	in = (float *)(rb.inVertices[0]);
	out = (float *)(rb.inVertices[rb.numVerts]);
	for (i=0 ; i<rb.numVerts ; i++, in += 3, out += 3) {
		Vec3Subtract (in, lightDist, diff);
		VectorNormalizef (diff, diff);
		Vec3MA (in, projectDistance, diff, out);
	}
}


/*
=============
RB_BuildShadowVolume
=============
*/
static void RB_BuildShadowVolume (vec3_t lightDist, float projectDistance)
{
	if (rb.curTrNormal != rb.inTrNormals[0])
		RB_MakeTriangleShadowFlags (lightDist, projectDistance);
	else
		RB_MakeTriangleShadowFlagsFromScratch (lightDist, projectDistance);

	RB_ShadowProjectVertices (lightDist, projectDistance);
	rb_numShadowVolTris = RB_BuildShadowVolumeTriangles ();
}


/*
=============
RB_DrawShadowVolume
=============
*/
static void RB_DrawShadowVolume (void)
{
	if (ri.config.extDrawRangeElements)
		qglDrawRangeElementsEXT (GL_TRIANGLES, 0, rb.numVerts * 2, rb_numShadowVolTris * 3, GL_UNSIGNED_INT, rb_shadowVolIndexes);
	else
		qglDrawElements (GL_TRIANGLES, rb_numShadowVolTris * 3, GL_UNSIGNED_INT, rb_shadowVolIndexes);
}


/*
=============
RB_CheckLightBoundaries
=============
*/
static qBool RB_CheckLightBoundaries (vec3_t mins, vec3_t maxs, vec3_t lightOrigin, float intensity2)
{
	vec3_t	v;

	v[0] = bound (mins[0], lightOrigin[0], maxs[0]);
	v[1] = bound (mins[1], lightOrigin[1], maxs[1]);
	v[2] = bound (mins[2], lightOrigin[2], maxs[2]);

	return (DotProduct(v, v) < intensity2);
}


/*
=============
RB_CastShadowVolume
=============
*/
static void RB_CastShadowVolume (refEntity_t *ent, vec3_t mins, vec3_t maxs, float radius, vec3_t lightOrigin, float intensity)
{
	float		projectDistance, intensity2;
	vec3_t		lightDist, lightDist2;

	if (R_CullSphere (lightOrigin, intensity, 31))
		return;

	intensity2 = intensity * intensity;
	Vec3Subtract (lightOrigin, ent->origin, lightDist2);

	if (!RB_CheckLightBoundaries (mins, maxs, lightDist2, intensity2))
		return;

	projectDistance = radius - (float)Vec3Length (lightDist2);
	if (projectDistance > 0)
		return;		// Light is inside the bbox

	projectDistance += intensity;
	if (projectDistance <= 0.1)
		return;		// Too far away

	// Rotate
	if (!Matrix3_Compare (ent->axis, axisIdentity))
		Matrix3_TransformVector (ent->axis, lightDist2, lightDist);
	else
		Vec3Copy (lightDist2, lightDist);

	RB_BuildShadowVolume (lightDist, projectDistance);

	RB_LockArrays (rb.numVerts * 2);

	if (gl_shadows->intVal == SHADOW_VOLUMES) {
		if (ri.useStencil) {
			if (ri.config.extStencilTwoSide) {
				qglEnable (GL_STENCIL_TEST_TWO_SIDE_EXT);

				qglActiveStencilFaceEXT (GL_BACK);
				qglStencilOp (GL_KEEP, GL_INCR, GL_KEEP);
				qglActiveStencilFaceEXT (GL_FRONT);
				qglStencilOp (GL_KEEP, GL_DECR, GL_KEEP);

				RB_DrawShadowVolume ();

				qglDisable (GL_STENCIL_TEST_TWO_SIDE_EXT);
			}
			else {
				qglCullFace (GL_BACK);		// Quake is backwards, this culls front faces
				qglStencilOp (GL_KEEP, GL_INCR, GL_KEEP);
				RB_DrawShadowVolume ();

				// Decrement stencil if frontface is behind depthbuffer
				qglCullFace (GL_FRONT);		// Quake is backwards, this culls back faces
				qglStencilOp (GL_KEEP, GL_DECR, GL_KEEP);
				RB_DrawShadowVolume ();
			}
		}
		else {
			qglCullFace (GL_BACK);		// Quake is backwards, this culls front faces
			RB_DrawShadowVolume ();

			// Decrement stencil if frontface is behind depthbuffer
			qglCullFace (GL_FRONT);		// Quake is backwards, this culls back faces
			RB_DrawShadowVolume ();
		}
	}
	else {
		RB_DrawShadowVolume ();
	}

	RB_UnlockArrays ();
}


/*
=============
RB_SetShadowState
=============
*/
void RB_SetShadowState (qBool start)
{
	if (start) {
		// Clear state
		RB_FinishRendering ();

		// Set the mode
		RB_SelectTexture (0);
		RB_TextureTarget (0);
		switch (gl_shadows->intVal) {
		case 1:
			RB_StateForBits (SB1_CULL_FRONT|SB1_BLEND_ON|SB1_DEFAULT);

			qglColor4f (0, 0, 0, SHADOW_ALPHA);

			qglDepthFunc (GL_LEQUAL);

			if (!ri.useStencil)
				break;
			qglEnable (GL_STENCIL_TEST);
			qglStencilFunc (GL_EQUAL, 128, 0xFF);
			qglStencilOp (GL_KEEP, GL_KEEP, GL_INCR);
			qglStencilMask (255);
			break;

#ifdef SHADOW_VOLUMES
		case SHADOW_VOLUMES:
			if (ri.config.extStencilTwoSide)
				RB_StateForBits (SB1_BLEND_ON|SB1_DEFAULT);
			else
				RB_StateForBits (SB1_CULL_FRONT|SB1_BLEND_ON|SB1_DEFAULT);

			qglColor4f (1, 1, 1, 1);
			qglColorMask (GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

			qglDepthFunc (GL_LESS);

			if (!ri.useStencil)
				break;
			qglEnable (GL_STENCIL_TEST);
			qglStencilFunc (GL_ALWAYS, 128, 255);
			qglStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);
			qglStencilMask (255);
			break;

		case 3:
			RB_StateForBits (SB1_DEPTHTEST_ON|SB1_BLEND_ON|SB1_BLENDSRC_ONE|SB1_BLENDDST_ONE);

			qglColor3f (1.0f, 0.1f, 0.1f);

			qglDepthFunc (GL_LEQUAL);

			if (!ri.useStencil)
				break;
			qglDisable (GL_STENCIL_TEST);
			break;
#endif
		}
		return;
	}

	// Reset
	switch (gl_shadows->intVal) {
	case 1:
		if (!ri.useStencil)
			break;
		qglDisable (GL_STENCIL_TEST);
		break;

#ifdef SHADOW_VOLUMES
	case SHADOW_VOLUMES:
		qglColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		qglDepthFunc (GL_LEQUAL);

		if (!ri.useStencil)
			break;
		qglStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);
		qglDisable (GL_STENCIL_TEST);
		break;

	default:
		break;
#endif
	}

	RB_TextureTarget (GL_TEXTURE_2D);
	RB_StateForBits (SB1_DEPTHMASK_ON|SB1_DEFAULT);
}


/*
=============
RB_DrawShadowVolumes
=============
*/
void RB_DrawShadowVolumes (mesh_t *mesh, refEntity_t *ent, vec3_t mins, vec3_t maxs, float radius)
{
	refDLight_t	*dLight;
	vec3_t		hack;
	uint32		i;

	if (ri.def.rdFlags & RDF_NOWORLDMODEL) {
		RB_ResetPointers ();
		return;
	}

	// FIXME: HACK
	Vec3Copy (ent->origin, hack);
	hack[2] += 128;
	hack[1] += 128;
	RB_CastShadowVolume (ent, mins, maxs, radius, hack, 500);

	// Dynamic light shadows
	dLight = ri.scn.dLightList;
	for (i=0 ; i<ri.scn.numDLights ; i++, dLight++)
		RB_CastShadowVolume (ent, mins, maxs, radius, dLight->origin, dLight->intensity);

	RB_ResetPointers ();
}


/*
=============
RB_ShadowBlend
=============
*/
void RB_ShadowBlend (void)
{
	if (gl_shadows->intVal != SHADOW_VOLUMES || !ri.useStencil)
		return;

	qglMatrixMode (GL_PROJECTION);
    qglLoadIdentity ();
	qglOrtho (0, 1, 1, 0, -99999, 99999);

	qglMatrixMode (GL_MODELVIEW);
    qglLoadIdentity ();

	RB_StateForBits (SB1_BLEND_ON|SB1_BLENDSRC_SRC_ALPHA|SB1_BLENDDST_ONE_MINUS_SRC_ALPHA);
	RB_TextureTarget (0);

	qglColor4f (0, 0, 0, SHADOW_ALPHA);

	qglEnable (GL_STENCIL_TEST);
	qglStencilFunc (GL_NOTEQUAL, 128, 255);
	qglStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);

	qglBegin (GL_TRIANGLES);
	qglVertex2f (-5, -5);
	qglVertex2f (10, -5);
	qglVertex2f (-5, 10);
	qglEnd ();

	RB_StateForBits (SB1_CULL_FRONT|SB1_DEFAULT);
	qglDisable (GL_STENCIL_TEST);
	RB_TextureTarget (GL_TEXTURE_2D);

	qglColor4f (1, 1, 1, 1);
}
#endif


/*
=============================================================================

	SIMPLE SHADOWS

	The mesh is simply pushed flat.
=============================================================================
*/

/*
=============
RB_SimpleShadow
=============
*/
void RB_SimpleShadow (refEntity_t *ent, vec3_t shadowSpot)
{
	float	height;
	int		i;

	// Set the height
	height = (ent->origin[2] - shadowSpot[2]) * -1;
	for (i=0 ; i<rb.numVerts ; i++)
		rb.inVertices[i][2] = height + 1;

	qglColor4f (0, 0, 0, SHADOW_ALPHA);

	// Draw it
	RB_LockArrays (rb.numVerts);

	if (ri.config.extDrawRangeElements)
		qglDrawRangeElementsEXT (GL_TRIANGLES, 0, rb.numVerts, rb.numIndexes, GL_UNSIGNED_INT, rb.inIndices);
	else
		qglDrawElements (GL_TRIANGLES, rb.numIndexes, GL_UNSIGNED_INT, rb.inIndices);

	RB_UnlockArrays ();
	RB_ResetPointers ();
}
