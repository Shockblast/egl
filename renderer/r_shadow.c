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
// r_shadow.c
//

#include "r_local.h"

/*
=============================================================================

	VOLUMETRIC SHADOWS

=============================================================================
*/

#ifdef SHADOW_VOLUMES
#define MAX_SHADOWVOLUME_INDEXES	VERTARRAY_MAX_INDEXES*4

static qBool	r_triangleFacingLight[VERTARRAY_MAX_TRIANGLES];
static index_t	r_shadowVolumeIndexes[MAX_SHADOWVOLUME_INDEXES];
static int		r_numShadowVolumeTris;

/*
=============
R_BuildShadowVolumeTriangles
=============
*/
static int R_BuildShadowVolumeTriangles (void)
{
	int		i, j, tris;
	int		*neighbors = rb_inNeighborsArray;
	index_t	*indexes = rb_inIndexArray;
	index_t	*out = r_shadowVolumeIndexes;

	// check each frontface for bordering backfaces,
	// and cast shadow polygons from those edges,
	// also create front and back caps for shadow volume
	for (i=0, j=0, tris=0 ; i<rb_numIndexes ; i += 3, j++, indexes += 3, neighbors += 3) {
		if (!r_triangleFacingLight[j])
			continue;

		// triangle is frontface and therefore casts shadow,
		// output front and back caps for shadow volume front cap
		out[0] = indexes[0];
		out[1] = indexes[1];
		out[2] = indexes[2];

		// rear cap (with flipped winding order)
		out[3] = indexes[0] + rb_numVerts;
		out[4] = indexes[2] + rb_numVerts;
		out[5] = indexes[1] + rb_numVerts;
		out += 6;
		tris += 2;

		// check the edges
		if (neighbors[0] < 0 || !r_triangleFacingLight[neighbors[0]]) {
			out[0] = indexes[1];
			out[1] = indexes[0];
			out[2] = indexes[0] + rb_numVerts;
			out[3] = indexes[1];
			out[4] = indexes[0] + rb_numVerts;
			out[5] = indexes[1] + rb_numVerts;
			out += 6;
			tris += 2;
		}

		if (neighbors[1] < 0 || !r_triangleFacingLight[neighbors[1]]) {
			out[0] = indexes[2];
			out[1] = indexes[1];
			out[2] = indexes[1] + rb_numVerts;
			out[3] = indexes[2];
			out[4] = indexes[1] + rb_numVerts;
			out[5] = indexes[2] + rb_numVerts;
			out += 6;
			tris += 2;
		}

		if (neighbors[2] < 0 || !r_triangleFacingLight[neighbors[2]]) {
			out[0] = indexes[0];
			out[1] = indexes[2];
			out[2] = indexes[2] + rb_numVerts;
			out[3] = indexes[0];
			out[4] = indexes[2] + rb_numVerts;
			out[5] = indexes[0] + rb_numVerts;
			out += 6;
			tris += 2;
		}
	}

	return tris;
}


/*
=============
R_MakeTriangleShadowFlagsFromScratch
=============
*/
static void R_MakeTriangleShadowFlagsFromScratch (vec3_t lightDist, float lightRadius)
{
	float	f;
	int		i, j;
	float	*v0, *v1, *v2;
	vec3_t	dir0, dir1, temp;
	float	*trnormal = rb_inTrNormalsArray[0];
	index_t	*indexes = rb_inIndexArray;

	for (i=0, j=0 ; i<rb_numIndexes ; i += 3, j++, trnormal += 3, indexes += 3) {
		// Calculate triangle facing flag
		v0 = (float *)(rb_inVertexArray + indexes[0]);
		v1 = (float *)(rb_inVertexArray + indexes[1]);
		v2 = (float *)(rb_inVertexArray + indexes[2]);

		// Calculate two mostly perpendicular edge directions
		VectorSubtract (v0, v1, dir0);
		VectorSubtract (v2, v1, dir1);

		// We have two edge directions, we can calculate a third vector from
		// them, which is the direction of the surface normal (it's magnitude
		// is not 1 however)
		CrossProduct (dir0, dir1, temp);

		// Compare distance of light along normal, with distance of any point
		// of the triangle along the same normal (the triangle is planar,
		// I.E. flat, so all points give the same answer)
		f = (lightDist[0] - v0[0]) * temp[0] + (lightDist[1] - v0[1]) * temp[1] + (lightDist[2] - v0[2]) * temp[2];
		r_triangleFacingLight[j] = (f > 0) ? qTrue : qFalse;
	}
}


/*
=============
R_MakeTriangleShadowFlags
=============
*/
static void R_MakeTriangleShadowFlags (vec3_t lightDist, float lightRadius)
{
	int		i, j;
	float	f;
	float	*v0;
	float	*trnormal = rb_inTrNormalsArray[0];
	index_t	*indexes = rb_inIndexArray;

	for (i=0, j=0 ; i<rb_numIndexes ; i += 3, j++, trnormal += 3, indexes += 3) {
		v0 = (float *)(rb_inVertexArray + indexes[0]);

		// compare distance of light along normal, with distance of any point
		// of the triangle along the same normal (the triangle is planar,
		// I.E. flat, so all points give the same answer)
		f = (lightDist[0] - v0[0]) * trnormal[0] + (lightDist[1] - v0[1]) * trnormal[1] + (lightDist[2] - v0[2]) * trnormal[2];
		if (f > 0) {
			r_triangleFacingLight[j] = qTrue;
		}
		else {
			r_triangleFacingLight[j] = qFalse;
		}
	}
}


/*
=============
R_ShadowProjectVertices
=============
*/
static void R_ShadowProjectVertices (vec3_t lightDist, float projectDistance)
{
	vec3_t	diff;
	float	*in, *out;
	int		i;

	in = (float *)(rb_inVertexArray[0]);
	out = (float *)(rb_inVertexArray[rb_numVerts]);

	for (i=0 ; i<rb_numVerts ; i++, in += 3, out += 3) {
		VectorSubtract (in, lightDist, diff);
		VectorNormalizef (diff, diff);
		VectorMA (in, projectDistance, diff, out);
//		VectorMA (in, r_shadows_nudge->value, diff, in );
	}
}


/*
=============
R_BuildShadowVolume
=============
*/
static void R_BuildShadowVolume (vec3_t lightDist, float projectDistance)
{
	if (rb_currentTrNormal != rb_inTrNormalsArray[0])
		R_MakeTriangleShadowFlags (lightDist, projectDistance);
	else
		R_MakeTriangleShadowFlagsFromScratch (lightDist, projectDistance);

	R_ShadowProjectVertices (lightDist, projectDistance);
	r_numShadowVolumeTris = R_BuildShadowVolumeTriangles ();
}


/*
=============
R_DrawShadowVolume
=============
*/
static void R_DrawShadowVolume (void)
{
	if (glConfig.extDrawRangeElements)
		qglDrawRangeElementsEXT (GL_TRIANGLES, 0, rb_numVerts * 2, r_numShadowVolumeTris * 3, GL_UNSIGNED_INT, r_shadowVolumeIndexes);
	else
		qglDrawElements (GL_TRIANGLES, r_numShadowVolumeTris * 3, GL_UNSIGNED_INT, r_shadowVolumeIndexes);
}


/*
=============
R_CheckLightBoundaries
=============
*/
static qBool R_CheckLightBoundaries (vec3_t mins, vec3_t maxs, vec3_t lightOrigin, float intensity2)
{
	vec3_t	v;

	v[0] = bound (mins[0], lightOrigin[0], maxs[0]);
	v[1] = bound (mins[1], lightOrigin[1], maxs[1]);
	v[2] = bound (mins[2], lightOrigin[2], maxs[2]);

	return (DotProduct(v, v) < intensity2);
}


/*
=============
R_CastShadowVolume
=============
*/
static void R_CastShadowVolume (entity_t *ent, vec3_t mins, vec3_t maxs, float radius, vec3_t lightOrigin, float intensity)
{
	float		projectDistance, intensity2;
	vec3_t		lightDist, lightDist2;

	if (R_CullSphere (lightOrigin, intensity, 15))
		return;

	intensity2 = intensity * intensity;
	VectorSubtract (lightOrigin, ent->origin, lightDist2);

	if (!R_CheckLightBoundaries (mins, maxs, lightDist2, intensity2))
		return;

	projectDistance = radius - (float)VectorLength (lightDist2);
	if (projectDistance > 0)
		return;		// Light is inside the bbox

	projectDistance += intensity;
	if (projectDistance <= 0.1)
		return;		// Too far away

	// Rotate
	if (!Matrix3_Compare (ent->axis, axisIdentity))
		Matrix3_TransformVector (ent->axis, lightDist2, lightDist);
	else
		VectorCopy (lightDist2, lightDist);

	R_BuildShadowVolume (lightDist, projectDistance);

	RB_LockArrays (rb_numVerts * 2);

	if (gl_shadows->integer == SHADOW_VOLUMES) {
		qglCullFace (GL_BACK);		// Quake is backwards, this culls front faces
		if (glStatic.useStencil)
			qglStencilOp (GL_KEEP, GL_INCR, GL_KEEP);
		R_DrawShadowVolume ();

		// Decrement stencil if frontface is behind depthbuffer
		qglCullFace (GL_FRONT);		// Quake is backwards, this culls back faces
		if (glStatic.useStencil)
			qglStencilOp (GL_KEEP, GL_DECR, GL_KEEP);
	}

	R_DrawShadowVolume ();

	RB_UnlockArrays ();
}


/*
=============
R_DrawShadowVolumes
=============
*/
void R_DrawShadowVolumes (mesh_t *mesh, entity_t *ent, vec3_t mins, vec3_t maxs, float radius)
{
	dLight_t	*dLight;
	vec3_t		hack;
	int			i;

	if (r_refDef.rdFlags & RDF_NOWORLDMODEL) {
		RB_ResetPointers ();
		return;
	}

	// FIXME: HACK
	VectorCopy (ent->origin, hack);
	hack[2] += 128;
	hack[1] += 128;
	R_CastShadowVolume (ent, mins, maxs, radius, hack, 500);

	// Dynamic light shadows
	dLight = r_refScene.dLightList;
	for (i=0 ; i<r_refScene.numDLights ; i++, dLight++)
		R_CastShadowVolume (ent, mins, maxs, radius, dLight->origin, dLight->intensity);

	RB_ResetPointers ();
}


/*
=============
R_ShadowBlend
=============
*/
void R_ShadowBlend (void)
{
	if (gl_shadows->integer != SHADOW_VOLUMES || !glStatic.useStencil)
		return;

	qglMatrixMode (GL_PROJECTION);
    qglLoadIdentity ();
	qglOrtho (0, 1, 1, 0, -99999, 99999);

	qglMatrixMode (GL_MODELVIEW);
    qglLoadIdentity ();

	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDisable (GL_ALPHA_TEST);
	qglEnable (GL_BLEND);
	qglDisable (GL_CULL_FACE);
	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_TEXTURE_2D);

	qglColor4f (0, 0, 0, SHADOW_ALPHA);

	qglEnable (GL_STENCIL_TEST);
	qglStencilFunc (GL_NOTEQUAL, 128, 255);//0xFF);
	qglStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);

	qglBegin (GL_TRIANGLES);
	qglVertex2f (-5, -5);
	qglVertex2f (10, -5);
	qglVertex2f (-5, 10);
	qglEnd ();

	qglDisable (GL_STENCIL_TEST);
	qglDisable (GL_BLEND);
	qglEnable (GL_TEXTURE_2D);
	qglEnable (GL_CULL_FACE);
	qglEnable (GL_DEPTH_TEST);

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
R_SimpleShadow
=============
*/
void R_SimpleShadow (entity_t *ent, vec3_t shadowSpot)
{
	float	height;
	int		i;

	// Set the height
	height = (ent->origin[2] - shadowSpot[2]) * -1;
	for (i=0 ; i<rb_numVerts ; i++)
		rb_inVertexArray[i][2] = height + 1;

	qglColor4f (0, 0, 0, SHADOW_ALPHA);

	// Draw it
	RB_LockArrays (rb_numVerts);

	if (glConfig.extDrawRangeElements)
		qglDrawRangeElementsEXT (GL_TRIANGLES, 0, rb_numVerts, rb_numIndexes, GL_UNSIGNED_INT, rb_inIndexArray);
	else
		qglDrawElements (GL_TRIANGLES, rb_numIndexes, GL_UNSIGNED_INT, rb_inIndexArray);

	RB_UnlockArrays ();
	RB_ResetPointers ();
}
