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
// rb_render.c
//

#include "rb_local.h"

/*
===============================================================================

	ARRAYS

===============================================================================
*/

// Rendering paths
static void				(*RB_ModifyTextureCoords) (matPass_t *pass, texUnit_t texUnit);

// Backend data
rbData_t				rb;

// Temporary spaces
static bvec4_t			rb_outColorArray[RB_MAX_VERTS];
static vec2_t			rb_outCoordArray[MAX_TEXUNITS][RB_MAX_VERTS];
static vec3_t			rb_outVertexArray[RB_MAX_VERTS];

// Dynamic data
static matPass_t		*rb_accumPasses[MAX_TEXUNITS];
static int				rb_numPasses;
static int				rb_numOldPasses;

static qBool			rb_arraysLocked;
static qBool			rb_triangleOutlines;
static float			rb_matTime;
static uint32			rb_stateBits1;

// Static data
static qBool			rb_matrixCoords;		// Texture coordinate math done using texture matrices
static int				rb_identityLighting;

static matPass_t		rb_dLightPass;
static matPass_t		rb_fogPass;
static matPass_t		rb_lightMapPass;

float					rb_sinTable[FTABLE_SIZE];
float					rb_triangleTable[FTABLE_SIZE];
float					rb_squareTable[FTABLE_SIZE];
float					rb_sawtoothTable[FTABLE_SIZE];
float					rb_inverseSawtoothTable[FTABLE_SIZE];
float					rb_noiseTable[FTABLE_SIZE];

/*
=============
RB_LockArrays
=============
*/
void RB_LockArrays (int numVerts)
{
	if (rb_arraysLocked)
		return;
	if (!ri.config.extCompiledVertArray)
		return;

	qglLockArraysEXT (0, numVerts);
	rb_arraysLocked = qTrue;
}


/*
=============
RB_UnlockArrays
=============
*/
void RB_UnlockArrays (void)
{
	if (!rb_arraysLocked)
		return;

	qglUnlockArraysEXT ();
	rb_arraysLocked = qFalse;
}


/*
=============
RB_ResetPointers
=============
*/
void RB_ResetPointers (void)
{
	rb.inColors = NULL;
	rb.inCoords = NULL;
	rb.inIndices = NULL;
	rb.inLMCoords = NULL;
	rb.inNormals = NULL;
	rb.inSVectors = NULL;
	rb.inTVectors = NULL;
	rb.inVertices = NULL;

#ifdef SHADOW_VOLUMES
	rb.inNeighbors = rb.batch.neighbors;
	rb.inTrNormals = rb.batch.trNormals;

	rb.curTrNeighbor = rb.inNeighbors;
	rb.curTrNormal = rb.inTrNormals[0];
#endif

	rb.curEntity = NULL;
	rb.curDLightBits = 0;
	rb.curLMTexNum = -1;
	rb.curMeshFeatures = 0;
	rb.curMeshType = MBT_MAX;
	rb.curModel = NULL;
	rb.curPatchWidth = 0;
	rb.curPatchHeight = 0;
	rb.curMat = NULL;

	rb.curTexFog = NULL;
	rb.curColorFog = NULL;

	rb.numIndexes = 0;
	rb.numVerts = 0;

	rb_arraysLocked = qFalse;
	rb_numPasses = 0;
}

/*
===============================================================================

	PASS HANDLING

===============================================================================
*/

/*
=============
RB_SetupColorFog
=============
*/
static void RB_SetupColorFog (const matPass_t *pass, int numColors)
{
	byte	*bArray;
	double	dist, vdist;
	cBspPlane_t *fogPlane, globalFogPlane;
	vec3_t	viewtofog;
	double	fogNormal[3], vpnNormal[3];
	double	fogDist, vpnDist, fogMatDist;
	int		fogptype;
	qBool	alphaFog;
	float	c, a;
	int		i;

	if ((pass->blendSource != GL_SRC_ALPHA && pass->blendDest != GL_SRC_ALPHA)
	&& (pass->blendSource != GL_ONE_MINUS_SRC_ALPHA && pass->blendDest != GL_ONE_MINUS_SRC_ALPHA))
		alphaFog = qFalse;
	else
		alphaFog = qTrue;

	fogPlane = rb.curColorFog->visiblePlane;
	if (!fogPlane) {
		Vec3Set (globalFogPlane.normal, 0, 0, 1);
		globalFogPlane.dist = ri.scn.worldModel->bspModel.nodes[0].c.maxs[2] + 1;
		globalFogPlane.type = PLANE_Z;
		fogPlane = &globalFogPlane;
	}

	fogMatDist = rb.curColorFog->mat->fogDist;
	dist = PlaneDiff (ri.def.viewOrigin, fogPlane);

	if (rb.curMat->flags & MAT_SKY) {
		if (dist > 0)
			Vec3Scale (fogPlane->normal, -dist, viewtofog);
		else
			Vec3Clear (viewtofog);
	}
	else
		Vec3Copy (rb.curEntity->origin, viewtofog);

	vpnNormal[0] = DotProduct (rb.curEntity->axis[0], ri.def.viewAxis[0]) * fogMatDist * rb.curEntity->scale;
	vpnNormal[1] = DotProduct (rb.curEntity->axis[1], ri.def.viewAxis[0]) * fogMatDist * rb.curEntity->scale;
	vpnNormal[2] = DotProduct (rb.curEntity->axis[2], ri.def.viewAxis[0]) * fogMatDist * rb.curEntity->scale;
	vpnDist = ((ri.def.viewOrigin[0] - viewtofog[0]) * ri.def.viewAxis[0][0]
				+ (ri.def.viewOrigin[1] - viewtofog[1]) * ri.def.viewAxis[0][1]
				+ (ri.def.viewOrigin[2] - viewtofog[2]) * ri.def.viewAxis[0][2])
				* fogMatDist;

	bArray = rb_outColorArray[0];
	if (dist < 0) {
		// Camera is inside the fog
		for (i=0 ; i<numColors ; i++, bArray+=4) {
			c = DotProduct (rb.inVertices[i], vpnNormal) - vpnDist;
			a = (1.0f - bound (0, c, 1.0f)) * (1.0 / 255.0);

			if (alphaFog) {
				bArray[3] = FloatToByte ((float)bArray[3]*a);
			}
			else {
				bArray[0] = FloatToByte ((float)bArray[0]*a);
				bArray[1] = FloatToByte ((float)bArray[1]*a);
				bArray[2] = FloatToByte ((float)bArray[2]*a);
			}
		}
	}
	else {
		fogNormal[0] = DotProduct (rb.curEntity->axis[0], fogPlane->normal) * rb.curEntity->scale;
		fogNormal[1] = DotProduct (rb.curEntity->axis[1], fogPlane->normal) * rb.curEntity->scale;
		fogNormal[2] = DotProduct (rb.curEntity->axis[2], fogPlane->normal) * rb.curEntity->scale;
		fogptype = (fogNormal[0] == 1.0 ? PLANE_X : (fogNormal[1] == 1.0 ? PLANE_Y : (fogNormal[2] == 1.0 ? PLANE_Z : PLANE_NON_AXIAL)));
		if (fogptype > 2)
			Vec3Scale (fogNormal, fogMatDist, fogNormal);
		fogDist = (fogPlane->dist - DotProduct (viewtofog, fogPlane->normal)) * fogMatDist;
		dist *= fogMatDist;

		for (i=0 ; i<numColors ; i++, bArray+=4) {
			if (fogptype < 3)
				vdist = rb.inVertices[i][fogptype] * fogMatDist - fogDist;
			else
				vdist = DotProduct (rb.inVertices[i], fogNormal) - fogDist;

			if (vdist < 0) {
				c = (DotProduct (rb.inVertices[i], vpnNormal) - vpnDist) * vdist / (vdist - dist);
				a = (1.0f - bound (0, c, 1.0f)) * (1.0 / 255.0);

				if (alphaFog) {
					bArray[3] = FloatToByte ((float)bArray[3]*a);
				}
				else {
					bArray[0] = FloatToByte ((float)bArray[0]*a);
					bArray[1] = FloatToByte ((float)bArray[1]*a);
					bArray[2] = FloatToByte ((float)bArray[2]*a);
				}
			}
		}
	}
}


/*
=============
RB_SetupColorFast
=============
*/
static qBool RB_SetupColorFast (const matPass_t *pass)
{
	const materialFunc_t *rgbGenFunc, *alphaGenFunc;
	byte		*inArray;
	bvec4_t		color;
	float		*table, c, a;

	rgbGenFunc = &pass->rgbGen.func;
	alphaGenFunc = &pass->alphaGen.func;
	inArray = rb.inColors[0];

	// Get the RGB
	switch (pass->rgbGen.type) {
	case RGB_GEN_UNKNOWN:
	case RGB_GEN_IDENTITY:
		color[0] = color[1] = color[2] = 255;
		break;

	case RGB_GEN_IDENTITY_LIGHTING:
		color[0] = color[1] = color[2] = rb_identityLighting;
		break;

	case RGB_GEN_CONST:
		color[0] = pass->rgbGen.bArgs[0];
		color[1] = pass->rgbGen.bArgs[1];
		color[2] = pass->rgbGen.bArgs[2];
		break;

	case RGB_GEN_COLORWAVE:
		table = RB_TableForFunc (rgbGenFunc->type);
		c = rb_matTime * rgbGenFunc->args[3] + rgbGenFunc->args[2];
		c = FTABLE_EVALUATE(table, c) * rgbGenFunc->args[1] + rgbGenFunc->args[0];
		a = pass->rgbGen.fArgs[0] * c; color[0] = FloatToByte (bound (0, a, 1));
		a = pass->rgbGen.fArgs[1] * c; color[1] = FloatToByte (bound (0, a, 1));
		a = pass->rgbGen.fArgs[2] * c; color[2] = FloatToByte (bound (0, a, 1));
		break;

	case RGB_GEN_ENTITY:
		color[0] = rb.curEntity->color[0];
		color[1] = rb.curEntity->color[1];
		color[2] = rb.curEntity->color[2];
		break;

	case RGB_GEN_ONE_MINUS_ENTITY:
		color[0] = 255 - rb.curEntity->color[0];
		color[1] = 255 - rb.curEntity->color[1];
		color[2] = 255 - rb.curEntity->color[2];
		break;

	default:
		return qFalse;
	}

	// Get the alpha
	switch (pass->alphaGen.type) {
	case ALPHA_GEN_UNKNOWN:
	case ALPHA_GEN_IDENTITY:
		color[3] = 255;
		break;

	case ALPHA_GEN_CONST:
		color[3] = FloatToByte (pass->alphaGen.args[0]);
		break;

	case ALPHA_GEN_ENTITY:
		color[3] = rb.curEntity->color[3];
		break;

	case ALPHA_GEN_WAVE:
		table = RB_TableForFunc (alphaGenFunc->type);
		a = alphaGenFunc->args[2] + rb_matTime * alphaGenFunc->args[3];
		a = FTABLE_EVALUATE(table, a) * alphaGenFunc->args[1] + alphaGenFunc->args[0];
		color[3] = FloatToByte (bound (0.0f, a, 1.0f));
		break;

	default:
		return qFalse;
	}

	qglDisableClientState (GL_COLOR_ARRAY);
	qglColor4ubv (color);
	return qTrue;
}


/*
=============
RB_SetupColor
=============
*/
static void RB_SetupColor (const matPass_t *pass)
{
	const materialFunc_t *rgbGenFunc, *alphaGenFunc;
	int		r, g, b;
	float	*table, c, a;
	byte	*bArray, *inArray;
	int		numColors, i;
	vec3_t	t, v;

	rgbGenFunc = &pass->rgbGen.func;
	alphaGenFunc = &pass->alphaGen.func;

	// Optimal case
	if (pass->flags & MAT_PASS_NOCOLORARRAY && !rb.curColorFog) {
		if (RB_SetupColorFast (pass))
			return;
		numColors = 1;
	}
	else
		numColors = rb.numVerts;

	// Color generation
	bArray = rb_outColorArray[0];
	inArray = rb.inColors[0];
	switch (pass->rgbGen.type) {
	case RGB_GEN_UNKNOWN:
	case RGB_GEN_IDENTITY:
		for (i=0 ; i<numColors ; i++, bArray+=4) {
			bArray[0] = 255;
			bArray[1] = 255;
			bArray[2] = 255;
		}
		break;

	case RGB_GEN_IDENTITY_LIGHTING:
		for (i=0 ; i<numColors ; i++, bArray+=4) {
			bArray[0] = rb_identityLighting;
			bArray[1] = rb_identityLighting;
			bArray[2] = rb_identityLighting;
		}
		break;

	case RGB_GEN_CONST:
		r = pass->rgbGen.bArgs[0];
		g = pass->rgbGen.bArgs[1];
		b = pass->rgbGen.bArgs[2];

		for (i=0 ; i<numColors ; i++, bArray+=4) {
			bArray[0] = r;
			bArray[1] = g;
			bArray[2] = b;
		}
		break;

	case RGB_GEN_COLORWAVE:
		table = RB_TableForFunc (rgbGenFunc->type);
		c = rb_matTime * rgbGenFunc->args[3] + rgbGenFunc->args[2];
		c = FTABLE_EVALUATE(table, c) * rgbGenFunc->args[1] + rgbGenFunc->args[0];
		a = pass->rgbGen.fArgs[0] * c; r = FloatToByte (bound (0, a, 1));
		a = pass->rgbGen.fArgs[1] * c; g = FloatToByte (bound (0, a, 1));
		a = pass->rgbGen.fArgs[2] * c; b = FloatToByte (bound (0, a, 1));

		for (i=0 ; i<numColors ; i++, bArray+=4) {
			bArray[0] = r;
			bArray[1] = g;
			bArray[2] = b;
		}
		break;

	case RGB_GEN_ENTITY:
		r = *(int *)rb.curEntity->color;
		for (i=0 ; i<numColors ; i++, bArray+=4)
			*(int *)bArray = r;
		break;

	case RGB_GEN_ONE_MINUS_ENTITY:
		for (i=0 ; i<numColors ; i++, bArray+=4) {
			bArray[0] = 255 - rb.curEntity->color[0];
			bArray[1] = 255 - rb.curEntity->color[1];
			bArray[2] = 255 - rb.curEntity->color[2];
		}
		break;

	case RGB_GEN_VERTEX:
		if (intensity->intVal > 0) {
			for (i=0 ; i<numColors ; i++, bArray+=4, inArray+=4) {
				bArray[0] = inArray[0] >> (intensity->intVal / 2);
				bArray[1] = inArray[1] >> (intensity->intVal / 2);
				bArray[2] = inArray[2] >> (intensity->intVal / 2);
			}
			break;
		}

	// FALL THROUGH
	case RGB_GEN_EXACT_VERTEX:
		for (i=0 ; i<numColors ; i++, bArray+=4, inArray+=4) {
			bArray[0] = inArray[0];
			bArray[1] = inArray[1];
			bArray[2] = inArray[2];
		}
		break;

	case RGB_GEN_ONE_MINUS_VERTEX:
		if (intensity->intVal > 0) {
			for (i=0 ; i<numColors ; i++, bArray+=4, inArray+=4) {
				bArray[0] = 255 - (inArray[0] >> (intensity->intVal / 2));
				bArray[1] = 255 - (inArray[1] >> (intensity->intVal / 2));
				bArray[2] = 255 - (inArray[2] >> (intensity->intVal / 2));
			}
			break;
		}

	// FALL THROUGH
	case RGB_GEN_ONE_MINUS_EXACT_VERTEX:
		for (i=0 ; i<numColors ; i++, bArray+=4, inArray+=4) {
			bArray[0] = 255 - inArray[0];
			bArray[1] = 255 - inArray[1];
			bArray[2] = 255 - inArray[2];
		}
		break;

	case RGB_GEN_LIGHTING_DIFFUSE:
		R_LightForEntity (rb.curEntity, numColors, bArray);
		break;

	case RGB_GEN_FOG:
		for (i=0 ; i<numColors ; i++, bArray+=4) {
			bArray[0] = rb.curTexFog->mat->fogColor[0];
			bArray[1] = rb.curTexFog->mat->fogColor[1];
			bArray[2] = rb.curTexFog->mat->fogColor[2];
		}
		break;

	default:
		assert (0);
		break;
	}

	// Alpha generation
	bArray = rb_outColorArray[0];
	inArray = rb.inColors[0];
	switch (pass->alphaGen.type) {
	case ALPHA_GEN_UNKNOWN:
	case ALPHA_GEN_IDENTITY:
		for (i=0 ; i<numColors ; i++, bArray+=4) {
			bArray[3] = 255;
		}
		break;

	case ALPHA_GEN_CONST:
		b = FloatToByte (pass->alphaGen.args[0]);
		for (i=0 ; i<numColors ; i++, bArray+=4)
			bArray[3] = b;
		break;

	case ALPHA_GEN_WAVE:
		table = RB_TableForFunc (alphaGenFunc->type);
		a = alphaGenFunc->args[2] + rb_matTime * alphaGenFunc->args[3];
		a = FTABLE_EVALUATE(table, a) * alphaGenFunc->args[1] + alphaGenFunc->args[0];
		b = FloatToByte (bound (0.0f, a, 1.0f));
		for (i=0 ; i<numColors ; i++, bArray+=4)
			bArray[3] = b;
		break;

	case ALPHA_GEN_PORTAL:
		Vec3Add (rb.inVertices[0], rb.curEntity->origin, v);
		Vec3Subtract (ri.def.viewOrigin, v, t);
		a = Vec3Length (t) * pass->alphaGen.args[0];
		b = FloatToByte (clamp (a, 0.0f, 1.0f));

		for (i=0 ; i<numColors ; i++, bArray+=4)
			bArray[3] = b;
		break;

	case ALPHA_GEN_VERTEX:
		for (i=0 ; i<numColors ; i++, bArray+=4, inArray+=4)
			bArray[3] = inArray[3];
		break;

	case ALPHA_GEN_ONE_MINUS_VERTEX:
		for (i=0 ; i<numColors ; i++, bArray+=4, inArray+=4)
			bArray[3] = 255 - inArray[3];

	case ALPHA_GEN_ENTITY:
		for (i=0 ; i<numColors ; i++, bArray+=4)
			bArray[3] = rb.curEntity->color[3];
		break;

	case ALPHA_GEN_SPECULAR:
		Vec3Subtract (ri.def.viewOrigin, rb.curEntity->origin, t);
		if (!Matrix3_Compare (rb.curEntity->axis, axisIdentity))
			Matrix3_TransformVector (rb.curEntity->axis, t, v);
		else
			Vec3Copy (t, v);

		for (i=0 ; i<numColors ; i++, bArray+=4) {
			Vec3Subtract (v, rb.inVertices[i], t);
			a = DotProduct (t, rb.inNormals[i]) * Q_RSqrtf (DotProduct (t, t));
			a = a * a * a * a * a;
			bArray[3] = FloatToByte (bound (0.0f, a, 1.0f));
		}
		break;

	case ALPHA_GEN_DOT:
		if (!Matrix3_Compare (rb.curEntity->axis, axisIdentity))
			Matrix3_TransformVector (rb.curEntity->axis, ri.def.viewAxis[0], v);
		else
			Vec3Copy (ri.def.viewAxis[0], v);

		for (i=0 ; i<numColors ; i++, bArray+=4) {
			a = DotProduct (v, rb.inNormals[i]);
			if (a < 0)
				a = -a;
			bArray[3] = FloatToByte (bound (pass->alphaGen.args[0], a, pass->alphaGen.args[1]));
		}
		break;

	case ALPHA_GEN_ONE_MINUS_DOT:
		if (!Matrix3_Compare (rb.curEntity->axis, axisIdentity))
			Matrix3_TransformVector (rb.curEntity->axis, ri.def.viewAxis[0], v);
		else
			Vec3Copy (ri.def.viewAxis[0], v);

		for (i=0 ; i<numColors ; i++, bArray+=4) {
			a = DotProduct (v, rb.inNormals[i]);
			if (a < 0)
				a = -a;
			a = 1.0f - a;
			bArray[3] = FloatToByte (bound (pass->alphaGen.args[0], a, pass->alphaGen.args[1]));
		}
		break;

	case ALPHA_GEN_FOG:
		for (i=0 ; i<numColors ; i++, bArray+=4)
			bArray[3] = rb.curTexFog->mat->fogColor[3];
		break;

	default:
		assert (0);
		break;
	}

	// Colored fog
	if (rb.curColorFog)
		RB_SetupColorFog (pass, numColors);

	// Set color
	if (numColors == 1) {
		qglDisableClientState (GL_COLOR_ARRAY);
		qglColor4ubv (rb_outColorArray[0]);
	}
	else {
		qglEnableClientState (GL_COLOR_ARRAY);
		qglColorPointer (4, GL_UNSIGNED_BYTE, 0, rb_outColorArray);
	}
}


/*
=============
RB_ModifyTextureCoordsGeneric

Standard path
=============
*/
static const float r_warpSinTable[] = {
	0.000000f,	0.098165f,	0.196270f,	0.294259f,	0.392069f,	0.489643f,	0.586920f,	0.683850f,
	0.780360f,	0.876405f,	0.971920f,	1.066850f,	1.161140f,	1.254725f,	1.347560f,	1.439580f,
	1.530735f,	1.620965f,	1.710220f,	1.798445f,	1.885585f,	1.971595f,	2.056410f,	2.139990f,
	2.222280f,	2.303235f,	2.382795f,	2.460925f,	2.537575f,	2.612690f,	2.686235f,	2.758160f,
	2.828425f,	2.896990f,	2.963805f,	3.028835f,	3.092040f,	3.153385f,	3.212830f,	3.270340f,
	3.325880f,	3.379415f,	3.430915f,	3.480350f,	3.527685f,	3.572895f,	3.615955f,	3.656840f,
	3.695520f,	3.731970f,	3.766175f,	3.798115f,	3.827760f,	3.855105f,	3.880125f,	3.902810f,
	3.923140f,	3.941110f,	3.956705f,	3.969920f,	3.980740f,	3.989160f,	3.995180f,	3.998795f,
	4.000000f,	3.998795f,	3.995180f,	3.989160f,	3.980740f,	3.969920f,	3.956705f,	3.941110f,
	3.923140f,	3.902810f,	3.880125f,	3.855105f,	3.827760f,	3.798115f,	3.766175f,	3.731970f,
	3.695520f,	3.656840f,	3.615955f,	3.572895f,	3.527685f,	3.480350f,	3.430915f,	3.379415f,
	3.325880f,	3.270340f,	3.212830f,	3.153385f,	3.092040f,	3.028835f,	2.963805f,	2.896990f,
	2.828425f,	2.758160f,	2.686235f,	2.612690f,	2.537575f,	2.460925f,	2.382795f,	2.303235f,
	2.222280f,	2.139990f,	2.056410f,	1.971595f,	1.885585f,	1.798445f,	1.710220f,	1.620965f,
	1.530735f,	1.439580f,	1.347560f,	1.254725f,	1.161140f,	1.066850f,	0.971920f,	0.876405f,
	0.780360f,	0.683850f,	0.586920f,	0.489643f,	0.392069f,	0.294259f,	0.196270f,	0.098165f,
	0.000000f,	-0.098165f,	-0.196270f,	-0.294259f,	-0.392069f,	-0.489643f,	-0.586920f,	-0.683850f,
	-0.780360f,	-0.876405f,	-0.971920f,	-1.066850f,	-1.161140f,	-1.254725f,	-1.347560f,	-1.439580f,
	-1.530735f,	-1.620965f,	-1.710220f,	-1.798445f,	-1.885585f,	-1.971595f,	-2.056410f,	-2.139990f,
	-2.222280f,	-2.303235f,	-2.382795f,	-2.460925f,	-2.537575f,	-2.612690f,	-2.686235f,	-2.758160f,
	-2.828425f,	-2.896990f,	-2.963805f,	-3.028835f,	-3.092040f,	-3.153385f,	-3.212830f,	-3.270340f,
	-3.325880f,	-3.379415f,	-3.430915f,	-3.480350f,	-3.527685f,	-3.572895f,	-3.615955f,	 -3.656840f,
	-3.695520f,	-3.731970f,	-3.766175f,	-3.798115f,	-3.827760f,	-3.855105f,	-3.880125f,	-3.902810f,
	-3.923140f,	-3.941110f,	-3.956705f,	-3.969920f,	-3.980740f,	-3.989160f,	-3.995180f,	-3.998795f,
	-4.000000f,	-3.998795f,	-3.995180f,	-3.989160f,	-3.980740f,	-3.969920f,	-3.956705f,	-3.941110f,
	-3.923140f,	-3.902810f,	-3.880125f,	-3.855105f,	-3.827760f,	-3.798115f,	-3.766175f,	-3.731970f,
	-3.695520f,	-3.656840f,	-3.615955f,	-3.572895f,	-3.527685f,	-3.480350f,	-3.430915f,	-3.379415f,
	-3.325880f,	-3.270340f,	-3.212830f,	-3.153385f,	-3.092040f,	-3.028835f,	-2.963805f,	-2.896990f,
	-2.828425f,	-2.758160f,	-2.686235f,	-2.612690f,	-2.537575f,	-2.460925f,	-2.382795f,	-2.303235f,
	-2.222280f,	-2.139990f,	-2.056410f,	-1.971595f,	-1.885585f,	-1.798445f,	-1.710220f,	-1.620965f,
	-1.530735f,	-1.439580f,	-1.347560f,	-1.254725f,	-1.161140f,	-1.066850f,	-0.971920f,	-0.876405f,
	-0.780360f,	-0.683850f,	-0.586920f,	-0.489643f,	-0.392069f,	 -0.294259f,-0.196270f,	-0.098165f
};
static void RB_VertexTCBaseGeneric (matPass_t *pass, texUnit_t texUnit)
{
	float		*outCoords, depth;
	vec3_t		transform;
	vec3_t		n, projection;
	mat3x3_t	inverseAxis;
	int			i;

	outCoords = rb_outCoordArray[texUnit][0];

	// State
	switch (pass->tcGen) {
	case TC_GEN_REFLECTION:
		qglEnable (GL_TEXTURE_GEN_S);
		qglEnable (GL_TEXTURE_GEN_T);
		qglEnable (GL_TEXTURE_GEN_R);
		qglEnableClientState (GL_NORMAL_ARRAY);
		break;

	default:
		qglDisable (GL_TEXTURE_GEN_S);
		qglDisable (GL_TEXTURE_GEN_T);
		qglDisable (GL_TEXTURE_GEN_R);
		qglDisableClientState (GL_NORMAL_ARRAY);
		break;
	}

	// tcGen
	switch (pass->tcGen) {
	case TC_GEN_BASE:
		if (pass->numTCMods) {
			for (i=0 ; i<rb.numVerts ; i++) {
				rb_outCoordArray[texUnit][i][0] = rb.inCoords[i][0];
				rb_outCoordArray[texUnit][i][1] = rb.inCoords[i][1];
			}
			break;
		}

		qglTexCoordPointer (2, GL_FLOAT, 0, rb.inCoords);
		return;

	case TC_GEN_LIGHTMAP:
		if (pass->numTCMods) {
			for (i=0 ; i<rb.numVerts ; i++) {
				rb_outCoordArray[texUnit][i][0] = rb.inLMCoords[i][0];
				rb_outCoordArray[texUnit][i][1] = rb.inLMCoords[i][1];
			}
			break;
		}

		qglTexCoordPointer (2, GL_FLOAT, 0, rb.inLMCoords);
		return;

	case TC_GEN_ENVIRONMENT:
		if (rb_glState.in2D) {
			for (i=0 ; i<rb.numVerts ; i++) {
				rb_outCoordArray[texUnit][i][0] = rb.inCoords[i][0];
				rb_outCoordArray[texUnit][i][1] = rb.inCoords[i][1];
			}
			return;
		}

		if (rb.curModel) {
			if (rb.curModel == ri.scn.worldModel) {
				Vec3Subtract (vec3Origin, ri.def.viewOrigin, transform);
			}
			else {
				Vec3Subtract (rb.curEntity->origin, ri.def.viewOrigin, transform);
				Matrix3_Transpose (rb.curEntity->axis, inverseAxis);
			}
		}
		else {
			Vec3Clear (transform);
			Matrix3_Transpose (axisIdentity, inverseAxis);
		}

		for (i=0 ; i<rb.numVerts ; i++, outCoords+=2) {
			Vec3Add (rb.inVertices[i], transform, projection);
			VectorNormalizeFastf (projection);

			// Project vector
			if (rb.curModel && rb.curModel == ri.scn.worldModel)
				Vec3Copy (rb.inNormals[i], n);
			else
				Matrix3_TransformVector (inverseAxis, rb.inNormals[i], n);

			depth = -2.0f * DotProduct (n, projection);
			Vec3MA (projection, depth, n, projection);
			depth = sqrt (DotProduct (projection, projection) * 4);

			outCoords[0] = -((projection[1] * depth) + 0.5f);
			outCoords[1] = ((projection[2] * depth) + 0.5f);
		}
		break;

	case TC_GEN_VECTOR:
		for (i=0 ; i<rb.numVerts ; i++, outCoords+=2) {
			outCoords[0] = DotProduct (pass->tcGenVec[0], rb.inVertices[i]) + pass->tcGenVec[0][3];
			outCoords[1] = DotProduct (pass->tcGenVec[1], rb.inVertices[i]) + pass->tcGenVec[1][3];
		}
		break;

	case TC_GEN_REFLECTION:
		qglTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
		qglTexGeni (GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
		qglTexGeni (GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);

		qglNormalPointer (GL_FLOAT, 12, rb.inNormals);
		break;


	case TC_GEN_WARP:
		for (i=0 ; i<rb.numVerts ; i++, outCoords+=2) {
			outCoords[0] = rb.inCoords[i][0] + (r_warpSinTable[(uint8_t) (((rb.inCoords[i][1]*8.0f + rb_matTime) * (256.0f / (M_PI * 2.0f)))) & 255] * (1.0/64));
			outCoords[1] = rb.inCoords[i][1] + (r_warpSinTable[(uint8_t) (((rb.inCoords[i][0]*8.0f + rb_matTime) * (256.0f / (M_PI * 2.0f)))) & 255] * (1.0/64));
		}
		break;

	case TC_GEN_DLIGHT:
		return;

	case TC_GEN_FOG:
		{
			int			fogPtype;
			cBspPlane_t	*fogPlane, globalFogPlane;
			material_t	*fogMat;
			vec3_t		viewtofog;
			double		fogNormal[3], vpnNormal[3];
			double		dist, vdist, fogDist, vpnDist;
			vec3_t		fogVPN;

			fogPlane = rb.curTexFog->visiblePlane;
			if (!fogPlane) {
				Vec3Set (globalFogPlane.normal, 0, 0, 1);
				globalFogPlane.dist = ri.scn.worldModel->bspModel.nodes[0].c.maxs[2] + 1;
				globalFogPlane.type = PLANE_Z;
				fogPlane = &globalFogPlane;
			}
			fogMat = rb.curTexFog->mat;

			// Distance to fog
			dist = PlaneDiff (ri.def.viewOrigin, fogPlane);
			if (rb.curMat->flags & MAT_SKY) {
				if (dist > 0)
					Vec3MA (ri.def.viewOrigin, -dist, fogPlane->normal, viewtofog);
				else
					Vec3Copy (ri.def.viewOrigin, viewtofog);
			}
			else
				Vec3Copy (rb.curEntity->origin, viewtofog);

			Vec3Scale (ri.def.viewAxis[0], fogMat->fogDist, fogVPN);

			// Fog settings
			fogNormal[0] = DotProduct (rb.curEntity->axis[0], fogPlane->normal) * rb.curEntity->scale;
			fogNormal[1] = DotProduct (rb.curEntity->axis[1], fogPlane->normal) * rb.curEntity->scale;
			fogNormal[2] = DotProduct (rb.curEntity->axis[2], fogPlane->normal) * rb.curEntity->scale;
			fogPtype = (fogNormal[0] == 1.0 ? PLANE_X : (fogNormal[1] == 1.0 ? PLANE_Y : (fogNormal[2] == 1.0 ? PLANE_Z : PLANE_NON_AXIAL)));
			fogDist = (fogPlane->dist - DotProduct (viewtofog, fogPlane->normal));

			// Forward view normal/distance
			vpnNormal[0] = DotProduct (rb.curEntity->axis[0], fogVPN) * rb.curEntity->scale;
			vpnNormal[1] = DotProduct (rb.curEntity->axis[1], fogVPN) * rb.curEntity->scale;
			vpnNormal[2] = DotProduct (rb.curEntity->axis[2], fogVPN) * rb.curEntity->scale;
			vpnDist = ((ri.def.viewOrigin[0] - viewtofog[0]) * fogVPN[0]
						+ (ri.def.viewOrigin[1] - viewtofog[1]) * fogVPN[1]
						+ (ri.def.viewOrigin[2] - viewtofog[2]) * fogVPN[2]);

			if (dist < 0) {
				// Camera is inside the fog brush
				if (fogPtype < 3) {
					for (i=0 ; i<rb.numVerts ; i++, outCoords+=2) {
						outCoords[0] = DotProduct (rb.inVertices[i], vpnNormal) - vpnDist;
						outCoords[1] = -(rb.inVertices[i][fogPtype] - fogDist);
					}
				}
				else {
					for (i=0 ; i<rb.numVerts ; i++, outCoords+=2) {
						outCoords[0] = DotProduct (rb.inVertices[i], vpnNormal) - vpnDist;
						outCoords[1] = -(DotProduct (rb.inVertices[i], fogNormal) - fogDist);
					}
				}
			}
			else {
				if (fogPtype < 3) {
					for (i=0 ; i<rb.numVerts ; i++, outCoords+=2) {
						vdist = rb.inVertices[i][fogPtype] - fogDist;
						outCoords[0] = ((vdist < 0) ? (DotProduct (rb.inVertices[i], vpnNormal) - vpnDist) * vdist / (vdist - dist) : 0.0f);
						outCoords[1] = -vdist;
					}
				}
				else {
					for (i=0 ; i<rb.numVerts ; i++, outCoords+=2) {
						vdist = DotProduct (rb.inVertices[i], fogNormal) - fogDist;
						outCoords[0] = ((vdist < 0) ? (DotProduct (rb.inVertices[i], vpnNormal) - vpnDist) * vdist / (vdist - dist) : 0.0f);
						outCoords[1] = -vdist;
					}
				}
			}

			outCoords = rb_outCoordArray[texUnit][0];
			for (i=0 ; i<rb.numVerts ; i++, outCoords+=2)
				outCoords[1] *= fogMat->fogDist + 1.5f/(float)FOGTEX_HEIGHT;
		}
		break;

	default:
		assert (0);
		break;
	}

	qglTexCoordPointer (2, GL_FLOAT, 0, rb_outCoordArray[texUnit][0]);
}
static void RB_ModifyTextureCoordsGeneric (matPass_t *pass, texUnit_t texUnit)
{
	int		i, j;
	float	*table;
	float	t1, t2, sint, cost;
	float	*tcArray;
	tcMod_t	*tcMod;

	RB_VertexTCBaseGeneric (pass, texUnit);

	for (i=0, tcMod=pass->tcMods ; i<pass->numTCMods ; tcMod++, i++) {
		tcArray = rb_outCoordArray[texUnit][0];

		switch (tcMod->type) {
		case TC_MOD_ROTATE:
			cost = tcMod->args[0] * rb_matTime;
			sint = RB_FastSin (cost);
			cost = RB_FastSin (cost + 0.25);

			for (j=0 ; j<rb.numVerts ; j++, tcArray+=2) {
				t1 = tcArray[0];
				t2 = tcArray[1];
				tcArray[0] = cost * (t1 - 0.5f) - sint * (t2 - 0.5f) + 0.5f;
				tcArray[1] = cost * (t2 - 0.5f) + sint * (t1 - 0.5f) + 0.5f;
			}
			break;

		case TC_MOD_SCALE:
			t1 = tcMod->args[0];
			t2 = tcMod->args[1];

			for (j=0 ; j<rb.numVerts ; j++, tcArray+=2) {
				tcArray[0] = tcArray[0] * t1;
				tcArray[1] = tcArray[1] * t2;
			}
			break;

		case TC_MOD_TURB:
			t1 = tcMod->args[1];
			t2 = tcMod->args[2] + rb_matTime * tcMod->args[3];

			for (j=0 ; j<rb.numVerts ; j++, tcArray+=2) {
				tcArray[0] = tcArray[0] + t1 * RB_FastSin (tcArray[0] * t1 + t2);
				tcArray[1] = tcArray[1] + t1 * RB_FastSin (tcArray[1] * t1 + t2);
			}
			break;

		case TC_MOD_STRETCH:
			table = RB_TableForFunc (tcMod->args[0]);
			t2 = tcMod->args[3] + rb_matTime * tcMod->args[4];
			t1 = FTABLE_EVALUATE(table, t2) * tcMod->args[2] + tcMod->args[1];
			t1 = t1 ? 1.0f / t1 : 1.0f;
			t2 = 0.5f - 0.5f * t1;

			for (j=0 ; j<rb.numVerts ; j++, tcArray+=2) {
				tcArray[0] = tcArray[0] * t1 + t2;
				tcArray[1] = tcArray[1] * t1 + t2;
			}
			break;

		case TC_MOD_SCROLL:
			t1 = tcMod->args[0] * rb_matTime; t1 = t1 - floor (t1);
			t2 = tcMod->args[1] * rb_matTime; t2 = t2 - floor (t2);

			for (j=0 ; j<rb.numVerts ; j++, tcArray+=2) {
				tcArray[0] = tcArray[0] + t1;
				tcArray[1] = tcArray[1] + t2;
			}
			break;

		case TC_MOD_TRANSFORM:
			for (j=0 ; j<rb.numVerts ; j++, tcArray+=2) {
				t1 = tcArray[0];
				t2 = tcArray[1];
				tcArray[0] = t1 * tcMod->args[0] + t2 * tcMod->args[2] + tcMod->args[4];
				tcArray[1] = t2 * tcMod->args[1] + t1 * tcMod->args[3] + tcMod->args[5];
			}
			break;

		default:
			assert (0);
			break;
		}
	}
}


/*
=============
RB_ModifyTextureCoordsMatrix

Matrix path
=============
*/
static void RB_ApplyTCModsMatrix (matPass_t *pass, mat4x4_t result)
{
	mat4x4_t	m1, m2;
	float		t1, t2, sint, cost;
	float		*table;
	tcMod_t		*tcMod;
	int			i;

	for (i=0, tcMod=pass->tcMods ; i<pass->numTCMods ; tcMod++, i++) {
		switch (tcMod->type) {
		case TC_MOD_ROTATE:
			cost = tcMod->args[0] * rb_matTime;
			sint = RB_FastSin (cost);
			cost = RB_FastSin (cost + 0.25);
			m2[0] =  cost, m2[1] = sint, m2[12] =  0.5f * (sint - cost + 1);
			m2[4] = -sint, m2[5] = cost, m2[13] = -0.5f * (sint + cost - 1);
			Matrix4_Copy2D (result, m1);
			Matrix4_Multiply2D (m2, m1, result);
			break;

		case TC_MOD_SCALE:
			Matrix4_Scale2D (result, tcMod->args[0], tcMod->args[1]);
			break;

		case TC_MOD_TURB:
			t1 = (1.0f / 4.0f);
			t2 = tcMod->args[2] + rb_matTime * tcMod->args[3];
			Matrix4_Scale2D (result, 1 + (tcMod->args[1] * RB_FastSin (t2) + tcMod->args[0]) * t1, 1 + (tcMod->args[1] * RB_FastSin (t2 + 0.25f) + tcMod->args[0]) * t1);
			break;

		case TC_MOD_STRETCH:
			table = RB_TableForFunc (tcMod->args[0]);
			t2 = tcMod->args[3] + rb_matTime * tcMod->args[4];
			t1 = FTABLE_EVALUATE (table, t2) * tcMod->args[2] + tcMod->args[1];
			t1 = t1 ? 1.0f / t1 : 1.0f;
			t2 = 0.5f - 0.5f * t1;
			Matrix4_Stretch2D (result, t1, t2);
			break;

		case TC_MOD_SCROLL:
			t1 = tcMod->args[0] * rb_matTime; t1 = t1 - floor (t1);
			t2 = tcMod->args[1] * rb_matTime; t2 = t2 - floor (t2);
			Matrix4_Translate2D (result, t1, t2);
			break;

		case TC_MOD_TRANSFORM:
			m2[0] = tcMod->args[0], m2[1] = tcMod->args[2], m2[12] = tcMod->args[4],
			m2[5] = tcMod->args[1], m2[4] = tcMod->args[3], m2[13] = tcMod->args[5]; 
			Matrix4_Copy2D (result, m1);
			Matrix4_Multiply2D (m2, m1, result);
			break;

		default:
			assert (0);
			break;
		}
	}
}
static qBool RB_VertexTCBaseMatrix (matPass_t *pass, texUnit_t texUnit, mat4x4_t matrix)
{
	vec3_t		transform;
	vec3_t		n, projection;
	mat3x3_t	inverseAxis;
	float		depth;
	int			i;

	Matrix4_Identity (matrix);

	// State
	switch (pass->tcGen) {
	case TC_GEN_REFLECTION:
		qglEnable (GL_TEXTURE_GEN_S);
		qglEnable (GL_TEXTURE_GEN_T);
		qglEnable (GL_TEXTURE_GEN_R);
		qglEnableClientState (GL_NORMAL_ARRAY);
		break;

	case TC_GEN_VECTOR:
		qglEnable (GL_TEXTURE_GEN_S);
		qglEnable (GL_TEXTURE_GEN_T);
		qglDisable (GL_TEXTURE_GEN_R);
		qglDisableClientState (GL_NORMAL_ARRAY);
		break;

	default:
		qglDisable (GL_TEXTURE_GEN_S);
		qglDisable (GL_TEXTURE_GEN_T);
		qglDisable (GL_TEXTURE_GEN_R);
		qglDisableClientState (GL_NORMAL_ARRAY);
		break;
	}

	// tcGen
	switch (pass->tcGen) {
	case TC_GEN_BASE:
		qglTexCoordPointer (2, GL_FLOAT, 0, rb.inCoords);
		return qTrue;

	case TC_GEN_LIGHTMAP:
		qglTexCoordPointer (2, GL_FLOAT, 0, rb.inLMCoords);
		return qTrue;

	case TC_GEN_ENVIRONMENT:
		if (rb_glState.in2D)
			return qTrue;

		matrix[0] = matrix[12] = -0.5f;
		matrix[5] = matrix[13] = 0.5f;

		Vec3Subtract (ri.def.viewOrigin, rb.curEntity->origin, transform);
		Matrix3_Transpose (rb.curEntity->axis, inverseAxis);

		if (Matrix3_Compare (inverseAxis, axisIdentity)) {
			for (i=0 ; i<rb.numVerts ; i++) {
				Vec3Subtract (rb.inVertices[i], transform, projection);
				VectorNormalizeFastf (projection);
				Vec3Copy (rb.inNormals[i], n);

				// Project vector
				depth = -DotProduct (n, projection);
				depth += depth;
				Vec3MA (projection, depth, n, projection);
				depth = Q_RSqrtf (DotProduct (projection, projection));

				rb_outCoordArray[texUnit][i][0] = projection[1] * depth;
				rb_outCoordArray[texUnit][i][1] = projection[2] * depth;
			}
		}
		else {
			for (i=0 ; i<rb.numVerts ; i++) {
				Vec3Subtract (rb.inVertices[i], transform, projection);
				VectorNormalizeFastf (projection);
				Matrix3_TransformVector (inverseAxis, rb.inNormals[i], n);

				// Project vector
				depth = -DotProduct (n, projection);
				depth += depth;
				Vec3MA (projection, depth, n, projection);
				depth = Q_RSqrtf (DotProduct (projection, projection));

				rb_outCoordArray[texUnit][i][0] = projection[1] * depth;
				rb_outCoordArray[texUnit][i][1] = projection[2] * depth;
			}
		}

		qglTexCoordPointer (2, GL_FLOAT, 0, rb_outCoordArray[texUnit][0]);
		return qFalse;

	case TC_GEN_VECTOR:
		{
			GLfloat genVector[2][4];

			for (i=0 ; i<3 ; i++) {
				genVector[0][i] = pass->tcGenVec[0][i];
				genVector[1][i] = pass->tcGenVec[1][i];
				genVector[0][3] = genVector[1][3] = 0;
			}

			matrix[12] = pass->tcGenVec[0][3];
			matrix[13] = pass->tcGenVec[1][3];

			qglTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			qglTexGeni (GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			qglTexGenfv (GL_S, GL_OBJECT_PLANE, genVector[0]);
			qglTexGenfv (GL_T, GL_OBJECT_PLANE, genVector[1]);
			return qFalse;
		}

	case TC_GEN_REFLECTION:
		qglTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
		qglTexGeni (GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
		qglTexGeni (GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);

		qglNormalPointer (GL_FLOAT, 12, rb.inNormals);
		return qTrue;

	case TC_GEN_WARP:
		for (i=0 ; i<rb.numVerts ; i++) {
			rb_outCoordArray[texUnit][i][0] = rb.inCoords[i][0] + (r_warpSinTable[(uint8_t) (((rb.inCoords[i][1]*8.0f + rb_matTime) * (256.0f / (M_PI * 2.0f)))) & 255] * (1.0/64));
			rb_outCoordArray[texUnit][i][1] = rb.inCoords[i][1] + (r_warpSinTable[(uint8_t) (((rb.inCoords[i][0]*8.0f + rb_matTime) * (256.0f / (M_PI * 2.0f)))) & 255] * (1.0/64));
		}

		qglTexCoordPointer (2, GL_FLOAT, 0, rb_outCoordArray[texUnit][0]);
		return qTrue;

	case TC_GEN_DLIGHT:
		return qTrue;

	case TC_GEN_FOG:
		{
			int			fogPtype;
			cBspPlane_t	*fogPlane, globalFogPlane;
			material_t	*fogMat;
			vec3_t		viewtofog;
			double		fogNormal[3], vpnNormal[3];
			double		dist, vdist, fogDist, vpnDist;
			float		*outCoords;

			fogPlane = rb.curTexFog->visiblePlane;
			if (!fogPlane) {
				Vec3Set (globalFogPlane.normal, 0, 0, 1);
				globalFogPlane.dist = ri.scn.worldModel->bspModel.nodes[0].c.maxs[2] + 1;
				globalFogPlane.type = PLANE_Z;
				fogPlane = &globalFogPlane;
			}
			fogMat = rb.curTexFog->mat;

			matrix[0] = matrix[5] = fogMat->fogDist;
			matrix[13] = 1.5/(float)FOGTEX_HEIGHT;

			// Distance to fog
			dist = PlaneDiff (ri.def.viewOrigin, fogPlane);

			if (rb.curMat->flags & MAT_SKY) {
				if (dist > 0)
					Vec3MA (ri.def.viewOrigin, -dist, fogPlane->normal, viewtofog);
				else
					Vec3Copy (ri.def.viewOrigin, viewtofog);
			}
			else
				Vec3Copy (rb.curEntity->origin, viewtofog);

			// Fog settings
			fogNormal[0] = DotProduct (rb.curEntity->axis[0], fogPlane->normal) * rb.curEntity->scale;
			fogNormal[1] = DotProduct (rb.curEntity->axis[1], fogPlane->normal) * rb.curEntity->scale;
			fogNormal[2] = DotProduct (rb.curEntity->axis[2], fogPlane->normal) * rb.curEntity->scale;
			fogPtype = (fogNormal[0] == 1.0 ? PLANE_X : (fogNormal[1] == 1.0 ? PLANE_Y : (fogNormal[2] == 1.0 ? PLANE_Z : PLANE_NON_AXIAL)));
			fogDist = (fogPlane->dist - DotProduct (viewtofog, fogPlane->normal));

			// Forward view normal/distance
			vpnNormal[0] = DotProduct (rb.curEntity->axis[0], ri.def.viewAxis[0]) * rb.curEntity->scale;
			vpnNormal[1] = DotProduct (rb.curEntity->axis[1], ri.def.viewAxis[0]) * rb.curEntity->scale;
			vpnNormal[2] = DotProduct (rb.curEntity->axis[2], ri.def.viewAxis[0]) * rb.curEntity->scale;
			vpnDist = ((ri.def.viewOrigin[0] - viewtofog[0]) * ri.def.viewAxis[0][0]
						+ (ri.def.viewOrigin[1] - viewtofog[1]) * ri.def.viewAxis[0][1]
						+ (ri.def.viewOrigin[2] - viewtofog[2]) * ri.def.viewAxis[0][2]);

			outCoords = rb_outCoordArray[texUnit][0];
			if (dist < 0) {
				// Camera is inside the fog brush
				if (fogPtype < 3) {
					for (i=0 ; i<rb.numVerts ; i++, outCoords+=2) {
						outCoords[0] = DotProduct (rb.inVertices[i], vpnNormal) - vpnDist;
						outCoords[1] = -(rb.inVertices[i][fogPtype] - fogDist);
					}
				}
				else {
					for (i=0 ; i<rb.numVerts ; i++, outCoords+=2) {
						outCoords[0] = DotProduct (rb.inVertices[i], vpnNormal) - vpnDist;
						outCoords[1] = -(DotProduct (rb.inVertices[i], fogNormal) - fogDist);
					}
				}
			}
			else {
				if (fogPtype < 3) {
					for (i=0 ; i<rb.numVerts ; i++, outCoords+=2) {
						vdist = rb.inVertices[i][fogPtype] - fogDist;
						outCoords[0] = ((vdist < 0) ? (DotProduct (rb.inVertices[i], vpnNormal) - vpnDist) * vdist / (vdist - dist) : 0.0f);
						outCoords[1] = -vdist;
					}
				}
				else {
					for (i=0 ; i<rb.numVerts ; i++, outCoords+=2) {
						vdist = DotProduct (rb.inVertices[i], fogNormal) - fogDist;
						outCoords[0] = ((vdist < 0) ? (DotProduct (rb.inVertices[i], vpnNormal) - vpnDist) * vdist / (vdist - dist) : 0.0f);
						outCoords[1] = -vdist;
					}
				}
			}

			qglTexCoordPointer (2, GL_FLOAT, 0, rb_outCoordArray[texUnit][0]);
			return qFalse;
		}
	}

	// Should never reach here...
	assert (0);
	return qTrue;
}
static void RB_ModifyTextureCoordsMatrix (matPass_t *pass, texUnit_t texUnit)
{
	mat4x4_t	m1, m2, result;
	qBool		identityMatrix;

	// Texture coordinate base
	identityMatrix = RB_VertexTCBaseMatrix (pass, texUnit, result);

	// Texture coordinate modifications
	qglMatrixMode (GL_TEXTURE);

	if (pass->numTCMods) {
		identityMatrix = qFalse;
		RB_ApplyTCModsMatrix (pass, result);
	}

	if (pass->tcGen == TC_GEN_REFLECTION) {
		Matrix4_Transpose (ri.scn.modelViewMatrix, m1);
		Matrix4_Copy (result, m2);
		Matrix4_Multiply (m2, m1, result);
		RB_LoadTexMatrix (result);
		return;
	}

	// Load identity
	if (identityMatrix)
		RB_LoadIdentityTexMatrix ();
	else
		RB_LoadTexMatrix (result);
}


/*
=============
RB_DeformVertices
=============
*/
static void RB_DeformVertices (void)
{
	vertDeform_t	*vertDeform;
	float			args[4], deflect;
	float			*quadIn[4], *quadOut[4], *table, now;
	int				i, j, k;
	uint32			l, m, p;
	vec3_t			tv, rot_centre;

	// Deformations
	vertDeform = &rb.curMat->deforms[0];
	for (i=0 ; i<rb.curMat->numDeforms ; i++, vertDeform++) {
		switch (vertDeform->type) {
		case DEFORMV_NONE:
			break;

		case DEFORMV_WAVE:
			table = RB_TableForFunc (vertDeform->func.type);
			now = vertDeform->func.args[2] + vertDeform->func.args[3] * rb_matTime;

			for (j=0 ; j<rb.numVerts ; j++) {
				deflect = (rb.inVertices[j][0] + rb.inVertices[j][1] + rb.inVertices[j][2]) * vertDeform->args[0] + now;
				deflect = FTABLE_EVALUATE (table, deflect) * vertDeform->func.args[1] + vertDeform->func.args[0];

				// Deflect vertex along its normal by wave amount
				Vec3MA (rb.inVertices[j], deflect, rb.inNormals[j], rb_outVertexArray[j]);
			}
			break;

		case DEFORMV_NORMAL:
			args[0] = vertDeform->args[1] * rb_matTime;

			for (j=0 ; j<rb.numVerts ; j++) {
				args[1] = rb.inNormals[j][2] * args[0];

				deflect = vertDeform->args[0] * RB_FastSin (args[1]);
				rb.inNormals[j][0] *= deflect;
				deflect = vertDeform->args[0] * RB_FastSin (args[1] + 0.25);
				rb.inNormals[j][1] *= deflect;

				VectorNormalizeFastf (rb.inNormals[j]);
			}
			break;

		case DEFORMV_BULGE:
			args[0] = vertDeform->args[0] / (float)rb.curPatchHeight;
			args[1] = vertDeform->args[1];
			args[2] = rb_matTime / (vertDeform->args[2] * rb.curPatchWidth);

			for (l=0, p=0 ; l<rb.curPatchHeight ; l++) {
				deflect = RB_FastSin ((float)l * args[0] + args[2]) * args[1];
				for (m=0 ; m<rb.curPatchWidth ; m++, p++)
					Vec3MA (rb.inVertices[p], deflect, rb.inNormals[p], rb_outVertexArray[p]);
			}
			break;

		case DEFORMV_MOVE:
			table = RB_TableForFunc (vertDeform->func.type);
			deflect = vertDeform->func.args[2] + rb_matTime * vertDeform->func.args[3];
			deflect = FTABLE_EVALUATE(table, deflect) * vertDeform->func.args[1] + vertDeform->func.args[0];

			for (j=0 ; j<rb.numVerts ; j++)
				Vec3MA (rb.inVertices[j], deflect, vertDeform->args, rb_outVertexArray[j]);
			break;

		case DEFORMV_AUTOSPRITE:
			{
				mat3x3_t	m0, m1, m2, result;

				if (rb.numIndexes % 6)
					break;

				if (rb.curModel && rb.curModel == ri.scn.worldModel)
					Matrix4_Matrix3 (ri.scn.worldViewMatrix, m1);
				else
					Matrix4_Matrix3 (ri.scn.modelViewMatrix, m1);

				Matrix3_Transpose (m1, m2);

				for (k=0 ; k<rb.numIndexes ; k+=6) {
					quadIn[0] = (float *)(rb.inVertices + rb.inIndices[k+0]);
					quadIn[1] = (float *)(rb.inVertices + rb.inIndices[k+1]);
					quadIn[2] = (float *)(rb.inVertices + rb.inIndices[k+2]);

					quadOut[0] = (float *)(rb_outVertexArray + rb.inIndices[k+0]);
					quadOut[1] = (float *)(rb_outVertexArray + rb.inIndices[k+1]);
					quadOut[2] = (float *)(rb_outVertexArray + rb.inIndices[k+2]);

					for (j=2 ; j>=0 ; j--) {
						quadIn[3] = (float *)(rb.inVertices + rb.inIndices[k+3+j]);
						if (!Vec3Compare (quadIn[3], quadIn[0])
						&& !Vec3Compare (quadIn[3], quadIn[1])
						&& !Vec3Compare (quadIn[3], quadIn[2])) {
							quadOut[3] = (float *)(rb_outVertexArray + rb.inIndices[k+3+j]);
							break;
						}
					}

					Matrix3_FromPoints (quadIn[0], quadIn[1], quadIn[2], m0);

					// Swap m0[0] an m0[1] - FIXME?
					Vec3Copy (m0[1], rot_centre);
					Vec3Copy (m0[0], m0[1]);
					Vec3Copy (rot_centre, m0[0]);

					Matrix3_Multiply (m2, m0, result);

					rot_centre[0] = (quadIn[0][0] + quadIn[1][0] + quadIn[2][0] + quadIn[3][0]) * 0.25f;
					rot_centre[1] = (quadIn[0][1] + quadIn[1][1] + quadIn[2][1] + quadIn[3][1]) * 0.25f;
					rot_centre[2] = (quadIn[0][2] + quadIn[1][2] + quadIn[2][2] + quadIn[3][2]) * 0.25f;

					for (j=0 ; j<4 ; j++) {
						Vec3Subtract (quadIn[j], rot_centre, tv);
						Matrix3_TransformVector (result, tv, quadOut[j]);
						Vec3Add (rot_centre, quadOut[j], quadOut[j]);
					}
				}
			}
			break;

		case DEFORMV_AUTOSPRITE2:
			if (rb.numIndexes % 6)
				break;

			for (k=0 ; k<rb.numIndexes ; k+=6) {
				int			long_axis, short_axis;
				vec3_t		axis, tmp, len;
				mat3x3_t	m0, m1, m2, result;

				quadIn[0] = (float *)(rb.inVertices + rb.inIndices[k+0]);
				quadIn[1] = (float *)(rb.inVertices + rb.inIndices[k+1]);
				quadIn[2] = (float *)(rb.inVertices + rb.inIndices[k+2]);

				quadOut[0] = (float *)(rb_outVertexArray + rb.inIndices[k+0]);
				quadOut[1] = (float *)(rb_outVertexArray + rb.inIndices[k+1]);
				quadOut[2] = (float *)(rb_outVertexArray + rb.inIndices[k+2]);

				for (j=2 ; j>=0 ; j--) {
					quadIn[3] = (float *)(rb.inVertices + rb.inIndices[k+3+j]);

					if (!Vec3Compare (quadIn[3], quadIn[0])
					&& !Vec3Compare (quadIn[3], quadIn[1])
					&& !Vec3Compare (quadIn[3], quadIn[2])) {
						quadOut[3] = (float *)(rb_outVertexArray + rb.inIndices[k+3+j]);
						break;
					}
				}

				// Build a matrix were the longest axis of the billboard is the Y-Axis
				Vec3Subtract (quadIn[1], quadIn[0], m0[0]);
				Vec3Subtract (quadIn[2], quadIn[0], m0[1]);
				Vec3Subtract (quadIn[2], quadIn[1], m0[2]);
				len[0] = DotProduct (m0[0], m0[0]);
				len[1] = DotProduct (m0[1], m0[1]);
				len[2] = DotProduct (m0[2], m0[2]);

				if (len[2] > len[1] && len[2] > len[0]) {
					if (len[1] > len[0]) {
						long_axis = 1;
						short_axis = 0;
					}
					else {
						long_axis = 0;
						short_axis = 1;
					}
				}
				else if (len[1] > len[2] && len[1] > len[0]) {
					if (len[2] > len[0]) {
						long_axis = 2;
						short_axis = 0;
					}
					else {
						long_axis = 0;
						short_axis = 2;
					}
				}
				else if (len[0] > len[1] && len[0] > len[2]) {
					if (len[2] > len[1]) {
						long_axis = 2;
						short_axis = 1;
					}
					else {
						long_axis = 1;
						short_axis = 2;
					}
				}
				else {
					// FIXME
					long_axis = 0;
					short_axis = 0;
				}

				if (!len[long_axis])
					break;
				len[long_axis] = Q_RSqrtf (len[long_axis]);
				Vec3Scale (m0[long_axis], len[long_axis], axis);

				if (DotProduct (m0[long_axis], m0[short_axis])) {
					Vec3Copy (axis, m0[1]);
					if (axis[0] || axis[1])
						MakeNormalVectorsf (m0[1], m0[0], m0[2]);
					else
						MakeNormalVectorsf (m0[1], m0[2], m0[0]);
				}
				else {
					if (!len[short_axis])
						break;
					len[short_axis] = Q_RSqrtf (len[short_axis]);
					Vec3Scale (m0[short_axis], len[short_axis], m0[0]);
					Vec3Copy (axis, m0[1]);
					CrossProduct (m0[0], m0[1], m0[2]);
				}

				rot_centre[0] = (quadIn[0][0] + quadIn[1][0] + quadIn[2][0] + quadIn[3][0]) * 0.25f;
				rot_centre[1] = (quadIn[0][1] + quadIn[1][1] + quadIn[2][1] + quadIn[3][1]) * 0.25f;
				rot_centre[2] = (quadIn[0][2] + quadIn[1][2] + quadIn[2][2] + quadIn[3][2]) * 0.25f;

				if (rb.curModel && rb.curModel == ri.scn.worldModel) {
					Vec3Copy (rot_centre, tv);
					Vec3Subtract (ri.def.viewOrigin, tv, tv);
				}
				else {
					Vec3Add (rb.curEntity->origin, rot_centre, tv);
					Vec3Subtract (ri.def.viewOrigin, tv, tmp);
					Matrix3_TransformVector (rb.curEntity->axis, tmp, tv);
				}

				// Filter any longest-axis-parts off the camera-direction
				deflect = -DotProduct (tv, axis);

				Vec3MA (tv, deflect, axis, m1[2]);
				VectorNormalizeFastf (m1[2]);
				Vec3Copy (axis, m1[1]);
				CrossProduct (m1[1], m1[2], m1[0]);

				Matrix3_Transpose (m1, m2);
				Matrix3_Multiply (m2, m0, result);

				for (j=0 ; j<4 ; j++) {
					Vec3Subtract (quadIn[j], rot_centre, tv);
					Matrix3_TransformVector (result, tv, quadOut[j]);
					Vec3Add (rot_centre, quadOut[j], quadOut[j]);
				}
			}
			break;

		case DEFORMV_PROJECTION_SHADOW:
			break;

		case DEFORMV_AUTOPARTICLE:
			{
				mat3x3_t	m0, m1, m2, result;
				float		scale;

				if (rb.numIndexes % 6)
					break;

				if (rb.curModel && rb.curModel == ri.scn.worldModel)
					Matrix4_Matrix3 (ri.scn.worldViewMatrix, m1);
				else
					Matrix4_Matrix3 (ri.scn.modelViewMatrix, m1);

				Matrix3_Transpose (m1, m2);

				for (k=0 ; k<rb.numIndexes ; k+=6) {
					quadIn[0] = (float *)(rb.inVertices + rb.inIndices[k+0]);
					quadIn[1] = (float *)(rb.inVertices + rb.inIndices[k+1]);
					quadIn[2] = (float *)(rb.inVertices + rb.inIndices[k+2]);

					quadOut[0] = (float *)(rb_outVertexArray + rb.inIndices[k+0]);
					quadOut[1] = (float *)(rb_outVertexArray + rb.inIndices[k+1]);
					quadOut[2] = (float *)(rb_outVertexArray + rb.inIndices[k+2]);

					for (j=2 ; j>=0 ; j--) {
						quadIn[3] = (float *)(rb.inVertices + rb.inIndices[k+3+j]);

						if (!Vec3Compare (quadIn[3], quadIn[0])
						&& !Vec3Compare (quadIn[3], quadIn[1])
						&& !Vec3Compare (quadIn[3], quadIn[2])) {
							quadOut[3] = (float *)(rb_outVertexArray + rb.inIndices[k+3+j]);
							break;
						}
					}

					Matrix3_FromPoints (quadIn[0], quadIn[1], quadIn[2], m0);
					Matrix3_Multiply (m2, m0, result);

					// Hack a scale up to keep particles from disappearing
					scale = (quadIn[0][0] - ri.def.viewOrigin[0]) * ri.def.viewAxis[0][0] +
							(quadIn[0][1] - ri.def.viewOrigin[1]) * ri.def.viewAxis[0][1] +
							(quadIn[0][2] - ri.def.viewOrigin[2]) * ri.def.viewAxis[0][2];
					if (scale < 20)
						scale = 1.5;
					else
						scale = 1.5 + scale * 0.006f;

					rot_centre[0] = (quadIn[0][0] + quadIn[1][0] + quadIn[2][0] + quadIn[3][0]) * 0.25f;
					rot_centre[1] = (quadIn[0][1] + quadIn[1][1] + quadIn[2][1] + quadIn[3][1]) * 0.25f;
					rot_centre[2] = (quadIn[0][2] + quadIn[1][2] + quadIn[2][2] + quadIn[3][2]) * 0.25f;

					for (j=0 ; j<4 ; j++) {
						Vec3Subtract (quadIn[j], rot_centre, tv);
						Matrix3_TransformVector (result, tv, quadOut[j]);
						Vec3MA (rot_centre, scale, quadOut[j], quadOut[j]);
					}
				}
			}
			break;

		default:
			assert (0);
			break;
		}
	}
}

/*
===============================================================================

	MESH RENDERING

===============================================================================
*/

/*
=============
RB_DrawElements
=============
*/
static void RB_DrawElements (void)
{
	if (!rb.numVerts || !rb.numIndexes)
		return;

	// Flush
	if (ri.config.extDrawRangeElements)
		qglDrawRangeElementsEXT (GL_TRIANGLES, 0, rb.numVerts, rb.numIndexes, GL_UNSIGNED_INT, rb.inIndices);
	else
		qglDrawElements (GL_TRIANGLES, rb.numIndexes, GL_UNSIGNED_INT, rb.inIndices);

	// Increment performance counters
	if (rb_triangleOutlines || !r_speeds->intVal)
		return;

	ri.pc.numVerts += rb.numVerts;
	ri.pc.numTris += rb.numIndexes/3;
	ri.pc.numElements++;

	switch (rb.curMeshType) {
	case MBT_Q2BSP:
	case MBT_Q3BSP:
		ri.pc.worldElements++;
		break;

	case MBT_ALIAS:
		ri.pc.aliasElements++;
		ri.pc.aliasPolys += rb.numIndexes/3;
		break;
	}
}


/*
=============
RB_CleanUpTextureUnits

This cleans up the texture units not planned to be used in the next render.
=============
*/
static void RB_CleanUpTextureUnits (void)
{
	texUnit_t		i;

	if (rb_matrixCoords)
		qglMatrixMode (GL_TEXTURE);

	for (i=rb_numOldPasses ; i>rb_numPasses ; i--) {
		RB_SelectTexture (i-1);
		RB_TextureTarget (0);
		qglDisable (GL_TEXTURE_GEN_S);
		qglDisable (GL_TEXTURE_GEN_T);
		qglDisable (GL_TEXTURE_GEN_R);
		qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
		RB_LoadIdentityTexMatrix ();
	}

	if (rb_matrixCoords)
		qglMatrixMode (GL_MODELVIEW);

	rb_numOldPasses = rb_numPasses;
}


/*
=============
RB_BindMaterialPass
=============
*/
static void RB_BindMaterialPass (matPass_t *pass, image_t *image, texUnit_t texUnit)
{
	if (!image) {
		// Find the texture
		if (pass->flags & MAT_PASS_LIGHTMAP)
			image = r_lmTextures[rb.curLMTexNum];
		else if (pass->flags & MAT_PASS_ANIMMAP)
			image = pass->animImages[(int)(pass->animFPS * rb_matTime) % pass->animNumImages];
		else
			image = pass->animImages[0];

		if (!image)
			image = ri.noTexture;
	}

	// Bind the texture
	RB_SelectTexture (texUnit);
	RB_BindTexture (image);

	// FIXME: This is kind of hacky
	RB_TextureTarget (image->target);
	if (image->flags & (IT_3D|IT_CUBEMAP))
		qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
	else
		qglEnableClientState (GL_TEXTURE_COORD_ARRAY);

	// Modify the texture coordinates
	RB_ModifyTextureCoords (pass, texUnit);
}


/*
=============
RB_SetupPassState
=============
*/
static void RB_SetupPassState (matPass_t *pass, qBool mTex)
{
	program_t	*program;
	uint32		sb1;

	sb1 = rb_stateBits1|pass->stateBits1;

	// Vertex program
	if (pass->flags & MAT_PASS_VERTEXPROGRAM) {
		program = pass->vertProgPtr;

		qglEnable (GL_VERTEX_PROGRAM_ARB);
		RB_BindProgram (program);

		qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 0, ri.def.viewOrigin[0], ri.def.viewOrigin[1], ri.def.viewOrigin[2], 0);
		qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 1, ri.def.viewAxis[0][0], ri.def.viewAxis[0][1], ri.def.viewAxis[0][2], 0);
		qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 2, ri.def.rightVec[0], ri.def.rightVec[1], ri.def.rightVec[2], 0);
		qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 3, ri.def.viewAxis[2][0], ri.def.viewAxis[2][1], ri.def.viewAxis[2][2], 0);
		qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 4, rb.curEntity->origin[0], rb.curEntity->origin[1], rb.curEntity->origin[2], 0);
		qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 5, rb.curEntity->axis[0][0], rb.curEntity->axis[0][1], rb.curEntity->axis[0][2], 0);
		qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 6, rb.curEntity->axis[1][0], rb.curEntity->axis[1][1], rb.curEntity->axis[1][2], 0);
		qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 7, rb.curEntity->axis[2][0], rb.curEntity->axis[2][1], rb.curEntity->axis[2][2], 0);
		qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 8, rb_matTime, 0, 0, 0);
	}
	else if (ri.config.extVertexProgram)
		qglDisable (GL_VERTEX_PROGRAM_ARB);

	// Fragment program
	if (pass->flags & MAT_PASS_FRAGMENTPROGRAM) {
		program = pass->fragProgPtr;

		qglEnable (GL_FRAGMENT_PROGRAM_ARB);
		RB_BindProgram (program);

		qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 0, ri.def.viewOrigin[0], ri.def.viewOrigin[1], ri.def.viewOrigin[2], 0);
		qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 1, ri.def.viewAxis[0][0], ri.def.viewAxis[0][1], ri.def.viewAxis[0][2], 0);
		qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 2, ri.def.rightVec[0], ri.def.rightVec[1], ri.def.rightVec[2], 0);
		qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 3, ri.def.viewAxis[2][0], ri.def.viewAxis[2][1], ri.def.viewAxis[2][2], 0);
		qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 4, rb.curEntity->origin[0], rb.curEntity->origin[1], rb.curEntity->origin[2], 0);
		qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 5, rb.curEntity->axis[0][0], rb.curEntity->axis[0][1], rb.curEntity->axis[0][2], 0);
		qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 6, rb.curEntity->axis[1][0], rb.curEntity->axis[1][1], rb.curEntity->axis[1][2], 0);
		qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 7, rb.curEntity->axis[2][0], rb.curEntity->axis[2][1], rb.curEntity->axis[2][2], 0);
		qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 8, rb_matTime, 0, 0, 0);
	}
	else if (ri.config.extFragmentProgram)
		qglDisable (GL_FRAGMENT_PROGRAM_ARB);

	// Blending
	if (pass->flags & MAT_PASS_BLEND) {
		sb1 |= SB1_BLEND_ON;
	}
	else if (rb.curEntity->flags & RF_TRANSLUCENT) {
		// FIXME: necessary Quake2 hack
		sb1 &= ~(SB1_BLENDSRC_BITS|SB1_BLENDDST_BITS);
		sb1 |= SB1_BLEND_ON|SB1_BLENDSRC_SRC_ALPHA|SB1_BLENDDST_ONE_MINUS_SRC_ALPHA;
	}

	// Nasty hack!!!
	if (!rb_glState.in2D) {
		qglDepthFunc (pass->depthFunc);
		if (pass->flags & MAT_PASS_DEPTHWRITE && !(rb.curEntity->flags & RF_TRANSLUCENT)) // FIXME: necessary Quake2 hack
			sb1 |= SB1_DEPTHMASK_ON;
	}

	// Mask colors
	if (pass->totalMask)
		qglColorMask (!pass->maskRed, !pass->maskGreen, !pass->maskBlue, !pass->maskAlpha);
	else
		qglColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	// Commit
	RB_StateForBits (sb1);
}


/*
=================
RB_RenderDLights

Special case for rendering Q3BSP dynamic light attenuation.
=================
*/
static void RB_RenderDLights (void)
{
	int				j;
	refDLight_t		*light;
	byte			*outColor;
	float			dist;
	vec3_t			tempVec, lightOrigin;
	float			scale;
	GLfloat			s[4], t[4], r[4];
	matPass_t	*pass;
	uint32			num;

	// Set state
	pass = rb_accumPasses[0];
	RB_BindMaterialPass (pass, ri.dLightTexture, 0);
	RB_SetupPassState (pass, qFalse);
	RB_TextureEnv (GL_MODULATE);

	// Texture state
	if (ri.config.extTex3D) {
		qglEnable (GL_TEXTURE_GEN_S);
		qglTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		s[1] = s[2] = 0;

		qglEnable (GL_TEXTURE_GEN_T);
		qglTexGeni (GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		t[0] = t[2] = 0;

		qglEnable (GL_TEXTURE_GEN_R);
		qglTexGeni (GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		r[0] = r[1] = 0;

		qglDisableClientState (GL_COLOR_ARRAY);
	}
	else {
		qglEnable (GL_TEXTURE_GEN_S);
		qglTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		s[1] = s[2] = 0;

		qglEnable (GL_TEXTURE_GEN_T);
		qglTexGeni (GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		t[0] = t[2] = 0;

		qglDisable (GL_TEXTURE_GEN_R);

		qglEnableClientState (GL_COLOR_ARRAY);
		qglColorPointer (4, GL_UNSIGNED_BYTE, 0, rb.batch.colors);
	}

	for (num=0, light=ri.scn.dLightList ; num<ri.scn.numDLights ; num++, light++) {
		if (!(rb.curDLightBits & (1<<num)))
			continue;	// Not lit by this light

		// Transform
		Vec3Subtract (light->origin, rb.curEntity->origin, lightOrigin);
		if (!Matrix3_Compare (rb.curEntity->axis, axisIdentity)) {
			Vec3Copy (lightOrigin, tempVec);
			Matrix3_TransformVector (rb.curEntity->axis, tempVec, lightOrigin);
		}

		scale = 1.0f / light->intensity;

		// Calculate coordinates
		s[0] = scale;
		s[3] = (-lightOrigin[0] * scale) + 0.5f;
		qglTexGenfv (GL_S, GL_OBJECT_PLANE, s);

		t[1] = scale;
		t[3] = (-lightOrigin[1] * scale) + 0.5f;
		qglTexGenfv (GL_T, GL_OBJECT_PLANE, t);

		if (ri.config.extTex3D) {
			// Color
			qglColor3fv (light->color);

			// Depth coordinate
			r[2] = scale;
			r[3] = (-lightOrigin[2] * scale) + 0.5f;
			qglTexGenfv (GL_R, GL_OBJECT_PLANE, r);
		}
		else {
			// Color
			outColor = rb.batch.colors[0];
			for (j=0 ; j<rb.numVerts ; j++) {
				dist = (rb.inVertices[j][2] - lightOrigin[2]) * 2;
				if (dist < 0)
					dist = -dist;

				if (dist < light->intensity) {
					outColor[0] = FloatToByte (light->color[0]);
					outColor[1] = FloatToByte (light->color[1]);
					outColor[2] = FloatToByte (light->color[2]);
				}
				else {
					dist = Q_RSqrtf (dist * dist - light->intensity * light->intensity);
					dist = clamp (dist, 0, 1);

					outColor[0] = FloatToByte (dist * light->color[0]);
					outColor[1] = FloatToByte (dist * light->color[1]);
					outColor[2] = FloatToByte (dist * light->color[2]);
				}
				outColor[3] = 255;

				// Next
				outColor += 4;
			}
		}

		// Render
		RB_DrawElements ();
	}
}


/*
=============
RB_RenderGeneric
=============
*/
static void RB_RenderGeneric (void)
{
	matPass_t	*pass;

	pass = rb_accumPasses[0];

	RB_BindMaterialPass (pass, NULL, 0);
	RB_SetupColor (pass);
	RB_SetupPassState (pass, qFalse);
	if (pass->blendMode == GL_REPLACE)
		RB_TextureEnv (GL_REPLACE);
	else
		RB_TextureEnv (GL_MODULATE);

	RB_DrawElements ();
}


/*
=============
RB_RenderCombine
=============
*/
static void RB_RenderCombine (void)
{
	matPass_t	*pass;
	int				i;

	pass = rb_accumPasses[0];

	RB_BindMaterialPass (pass, NULL, 0);
	RB_SetupColor (pass);
	RB_SetupPassState (pass, qTrue);
	RB_TextureEnv (GL_MODULATE);

	for (i=1 ; i<rb_numPasses ; i++) {
		pass = rb_accumPasses[i];
		RB_BindMaterialPass (pass, NULL, i);

		switch (pass->blendMode) {
		case GL_REPLACE:
		case GL_MODULATE:
			RB_TextureEnv (GL_MODULATE);
			break;

		case GL_ADD:
			// These modes are best set with TexEnv, Combine4 would need much more setup
			RB_TextureEnv (GL_ADD);
			break;

		case GL_DECAL:
			// Mimics Alpha-Blending in upper texture stage, but instead of multiplying the alpha-channel, they´re added
			// this way it can be possible to use GL_DECAL in both texture-units, while still looking good
			// normal mutlitexturing would multiply the alpha-channel which looks ugly
			RB_TextureEnv (GL_COMBINE_ARB);
			qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_INTERPOLATE_ARB);
			qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_ADD);

			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
			qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
			qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
			qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PREVIOUS_ARB);
			qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_SRC_ALPHA);

			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_TEXTURE);
			qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_SRC_ALPHA);
			break;

		default:
			RB_TextureEnv (GL_COMBINE4_NV);

			qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD);
			qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_ADD);

			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
			qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
			qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

			switch (pass->blendSource) {
			case GL_ONE:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_ZERO);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_ONE_MINUS_SRC_COLOR);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_ZERO);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case GL_ZERO:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_ZERO);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_ZERO);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_SRC_ALPHA);
				break;
			case GL_DST_COLOR:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PREVIOUS_ARB);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_SRC_ALPHA);
				break;
			case GL_ONE_MINUS_DST_COLOR:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_ONE_MINUS_SRC_COLOR);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PREVIOUS_ARB);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case GL_SRC_ALPHA:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_ALPHA);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_TEXTURE);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_SRC_ALPHA);
				break;
			case GL_ONE_MINUS_SRC_ALPHA:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_ONE_MINUS_SRC_ALPHA);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_TEXTURE);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case GL_DST_ALPHA:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PREVIOUS_ARB);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_ALPHA);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_SRC_ALPHA);
				break;
			case GL_ONE_MINUS_DST_ALPHA:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PREVIOUS_ARB);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_ONE_MINUS_SRC_ALPHA);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_ONE_MINUS_SRC_ALPHA);
				break;
			default:
				assert (0);
				break;
			}

			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_PREVIOUS_ARB);
			qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_SRC_COLOR);
			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_ARB, GL_PREVIOUS_ARB);	
			qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND2_ALPHA_ARB, GL_SRC_ALPHA);

			switch (pass->blendDest) {
			case GL_ONE:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_RGB_NV, GL_ZERO);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_RGB_NV, GL_ONE_MINUS_SRC_COLOR);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_ALPHA_NV, GL_ZERO);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_ALPHA_NV, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case GL_ZERO:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_RGB_NV, GL_ZERO);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_RGB_NV, GL_SRC_COLOR);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_ALPHA_NV, GL_ZERO);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_ALPHA_NV, GL_SRC_ALPHA);
				break;
			case GL_SRC_COLOR:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_RGB_NV, GL_TEXTURE);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_RGB_NV, GL_SRC_COLOR);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_ALPHA_NV, GL_TEXTURE);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_ALPHA_NV, GL_SRC_ALPHA);
				break;
			case GL_ONE_MINUS_SRC_COLOR:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_RGB_NV, GL_TEXTURE);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_RGB_NV, GL_ONE_MINUS_SRC_COLOR);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_ALPHA_NV, GL_TEXTURE);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_ALPHA_NV, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case GL_SRC_ALPHA:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_RGB_NV, GL_TEXTURE);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_RGB_NV, GL_SRC_ALPHA);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_ALPHA_NV, GL_TEXTURE);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_ALPHA_NV, GL_SRC_ALPHA);
				break;
			case GL_ONE_MINUS_SRC_ALPHA:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_RGB_NV, GL_TEXTURE);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_RGB_NV, GL_ONE_MINUS_SRC_ALPHA);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_ALPHA_NV, GL_TEXTURE);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_ALPHA_NV, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case GL_DST_ALPHA:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_RGB_NV, GL_PREVIOUS_ARB);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_RGB_NV, GL_SRC_ALPHA);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_ALPHA_NV, GL_PREVIOUS_ARB);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_ALPHA_NV, GL_SRC_ALPHA);
				break;
			case GL_ONE_MINUS_DST_ALPHA:
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_RGB_NV, GL_PREVIOUS_ARB);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_RGB_NV, GL_ONE_MINUS_SRC_ALPHA);
				qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE3_ALPHA_NV, GL_PREVIOUS_ARB);
				qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND3_ALPHA_NV, GL_ONE_MINUS_SRC_ALPHA);
				break;
			default:
				assert (0);
				break;
			}
		}
	}

	RB_DrawElements ();
}


/*
=============
RB_RenderMTex
=============
*/
static void RB_RenderMTex (void)
{
	matPass_t	*pass;
	int				i;

	pass = rb_accumPasses[0];

	RB_BindMaterialPass (pass, NULL, 0);
	RB_SetupColor (pass);
	RB_SetupPassState (pass, qTrue);
	RB_TextureEnv (GL_MODULATE);

	for (i=1 ; i<rb_numPasses ; i++) {
		pass = rb_accumPasses[i];
		RB_BindMaterialPass (pass, NULL, i);
		RB_TextureEnv (pass->blendMode);
	}

	RB_DrawElements ();
}

/*
===============================================================================

	PASS ACCUMULATION

===============================================================================
*/

/*
=============
RB_RenderAccumulatedPasses
=============
*/
static void RB_RenderAccumulatedPasses (void)
{
	// Clean up texture units not used in this flush
	RB_CleanUpTextureUnits ();

	// Flush
	if (rb_numPasses == 1)
		RB_RenderGeneric ();
	else if (ri.config.extTexEnvCombine)
		RB_RenderCombine ();
	else
		RB_RenderMTex ();
	rb_numPasses = 0;
}


/*
=============
RB_AccumulatePass
=============
*/
static void RB_AccumulatePass (matPass_t *pass)
{
	matPass_t	*prevPass;
	qBool			accum;

	ri.pc.meshPasses++;

	// First pass always accumulates
	if (!rb_numPasses) {
		rb_accumPasses[rb_numPasses++] = pass;
		return;
	}

	// Dynamic lights never accumulate
	if (pass->flags & MAT_PASS_DLIGHT) {
		if (rb.curDLightBits && ri.scn.numDLights) {
			RB_RenderAccumulatedPasses ();

			// Clean up texture units not used in this flush
			RB_CleanUpTextureUnits ();

			// Flush
			rb_accumPasses[rb_numPasses++] = pass;
			RB_RenderDLights ();
			rb_numPasses = 0;
		}
		return;
	}

	// Check bounds
	if (rb_numPasses >= ri.config.maxTexUnits) {
		RB_RenderAccumulatedPasses ();
		rb_accumPasses[rb_numPasses++] = pass;
		return;
	}

	// Compare against previous pass properties
	prevPass = rb_accumPasses[rb_numPasses-1];
	if (prevPass->depthFunc != pass->depthFunc
	|| prevPass->totalMask != pass->totalMask
	|| !pass->canAccumulate
	|| (prevPass->alphaFunc != ALPHA_FUNC_NONE && pass->depthFunc != GL_EQUAL)) {
		RB_RenderAccumulatedPasses ();
		rb_accumPasses[rb_numPasses++] = pass;
		return;
	}

	// Check the blend modes
	if (!pass->blendMode) {
		accum = (prevPass->blendMode == GL_REPLACE) && ri.config.extNVTexEnvCombine4;
	}
	else switch (prevPass->blendMode) {
	case GL_REPLACE:
		if (ri.config.extTexEnvCombine)
			accum = (pass->blendMode == GL_ADD) ? ri.config.extTexEnvAdd : qTrue;
		else
			accum = (pass->blendMode == GL_ADD) ? ri.config.extTexEnvAdd : (pass->blendMode != GL_DECAL);
		break;

	case GL_ADD:
		accum = (pass->blendMode == GL_ADD) && ri.config.extTexEnvAdd;
		break;

	case GL_MODULATE:
		accum = (pass->blendMode == GL_MODULATE || pass->blendMode == GL_REPLACE);
		break;

	default:
		accum = qFalse;
		break;
	}

	if (!accum)
		RB_RenderAccumulatedPasses ();
	rb_accumPasses[rb_numPasses++] = pass;
}

/*
===============================================================================

	MESH RENDERING SUPPORTING FUNCTIONS

===============================================================================
*/

/*
=============
RB_SetupMaterialState
=============
*/
static void RB_SetupMaterialState (material_t *mat)
{
	rb_stateBits1 = 0;

	// Culling
	if (gl_cull->intVal && !(rb.curMeshFeatures & MF_NOCULL)) {
		switch (mat->cullType) {
		case MAT_CULL_FRONT:
			rb_stateBits1 |= SB1_CULL_FRONT;
			break;
		case MAT_CULL_BACK:
			rb_stateBits1 |= SB1_CULL_BACK;
			break;
		case MAT_CULL_NONE:
			break;
		default:
			assert (0);
			break;
		}
	}

	if (rb_triangleOutlines)
		return;

	// Polygon offset
	if (mat->flags & MAT_POLYGONOFFSET)
		rb_stateBits1 |= SB1_POLYOFFSET_ON;

	// Depth range
	if (mat->flags & MAT_DEPTHRANGE) {
		qglDepthRange (mat->depthNear, mat->depthFar);
	}
	else if (rb.curEntity->flags & RF_DEPTHHACK)
		qglDepthRange (0, 0.3f);
	else
		qglDepthRange (0, 1);

	// Depth testing
	// FIXME: MAT_NODEPTH option?
	if (!(mat->flags & MAT_FLARE) && !rb_glState.in2D)
		rb_stateBits1 |= SB1_DEPTHTEST_ON;
}


/*
=============
RB_SetOutlineColor
=============
*/
static inline void RB_SetOutlineColor (void)
{
	switch (rb.curMeshType) {
	case MBT_2D:
	case MBT_ALIAS:
	case MBT_SP2:
	case MBT_SKY:
		qglColor4fv (Q_colorRed);
		break;

	case MBT_Q2BSP:
	case MBT_Q3BSP:
	case MBT_Q3BSP_FLARE:
		qglColor4fv (Q_colorWhite);
		break;

	case MBT_DECAL:
		qglColor4fv (Q_colorYellow);
		break;

	case MBT_POLY:
		qglColor4fv (Q_colorGreen);
		break;

	default:
		assert (0);
		break;
	}
}


/*
=============
RB_ShowTriangles
=============
*/
static inline void RB_ShowTriangles (void)
{
	int			i;

	// Set color
	switch (gl_showtris->intVal) {
	case 1:
		qglColor4ub (255, 255, 255, 255);
		break;

	case 2:
		RB_SetOutlineColor ();
		break;
	}

	// Draw
	for (i=0 ; i<rb.numIndexes ; i+=3) {
		qglBegin (GL_LINE_STRIP);
		qglArrayElement (rb.inIndices[i]);
		qglArrayElement (rb.inIndices[i+1]);
		qglArrayElement (rb.inIndices[i+2]);
		qglArrayElement (rb.inIndices[i]);
		qglEnd ();
	}
}


/*
=============
RB_ShowNormals
=============
*/
static inline void RB_ShowNormals (void)
{
	int		i;

	if (!rb.inNormals)
		return;

	// Set color
	switch (gl_shownormals->intVal) {
	case 1:
		qglColor4ub (255, 255, 255, 255);
		break;

	case 2:
		RB_SetOutlineColor ();
		break;
	}

	// Draw
	qglBegin (GL_LINES);
	for (i=0 ; i<rb.numVerts ; i++) {
		qglVertex3f(rb.inVertices[i][0],
					rb.inVertices[i][1],
					rb.inVertices[i][2]);
		qglVertex3f(rb.inVertices[i][0] + rb.inNormals[i][0]*2,
					rb.inVertices[i][1] + rb.inNormals[i][1]*2,
					rb.inVertices[i][2] + rb.inNormals[i][2]*2);
	}
	qglEnd ();
}

/*
===============================================================================

	MESH BUFFER RENDERING

===============================================================================
*/

/*
=============
RB_RenderMeshBuffer

This is the entry point for rendering just about everything
=============
*/
void RB_RenderMeshBuffer (meshBuffer_t *mb, qBool shadowPass)
{
	mBspSurface_t	*surf;
	matPass_t	*pass;
	qBool			debugLightmap, addDlights;
	int				i;

	if (r_skipBackend->intVal)
		return;

	// Collect mesh buffer values
	rb.curMeshType = mb->sortKey & (MBT_MAX-1);
	rb.curMat = mb->mat;
	rb.curEntity = mb->entity;
	rb.curModel = mb->entity->model;

	switch (rb.curMeshType) {
	case MBT_Q2BSP:
		rb.curLMTexNum = ((mBspSurface_t *)mb->mesh)->q2_lmTexNumActive;

		surf = (mBspSurface_t *)mb->mesh;
		addDlights = qFalse;
		break;

	case MBT_Q3BSP:
		rb.curLMTexNum = ((mBspSurface_t *)mb->mesh)->lmTexNum;
		rb.curPatchWidth = ((mBspSurface_t *)mb->mesh)->q3_patchWidth;
		rb.curPatchHeight = ((mBspSurface_t *)mb->mesh)->q3_patchHeight;

		surf = (mBspSurface_t *)mb->mesh;
		if (!(rb.curMat->flags & MAT_FLARE) && surf->dLightFrame == ri.frameCount) {
			rb.curDLightBits = surf->dLightBits;
			addDlights = qTrue;
		}
		else {
			addDlights = qFalse;
		}
		break;

	default:
		addDlights = qFalse;
		break;
	}

	// Set time
	if (rb_glState.in2D)
		rb_matTime = Sys_Milliseconds () * 0.001f;
	else
		rb_matTime = ri.def.time;
	rb_matTime -= mb->matTime;
	if (rb_matTime < 0)
		rb_matTime = 0;

	// State
	RB_SetupMaterialState (rb.curMat);

	// Setup vertices
	if (rb.curMat->numDeforms) {
		RB_DeformVertices ();
		qglVertexPointer (3, GL_FLOAT, 0, rb_outVertexArray);
	}
	else {
		qglVertexPointer (3, GL_FLOAT, 0, rb.inVertices);
	}

	if (!rb.numIndexes || shadowPass)
		return;

	RB_LockArrays (rb.numVerts);

	// Render outlines if desired
	if (rb_triangleOutlines) {
		if (gl_showtris->intVal)
			RB_ShowTriangles ();
		if (gl_shownormals->intVal)
			RB_ShowNormals ();

		RB_UnlockArrays ();
		RB_ResetPointers ();
		return;
	}

	// Set fog materials
	if (mb->fog && mb->fog->mat) {
		if ((rb.curMat->sortKey <= MAT_SORT_PARTICLE+1 && rb.curMat->flags & (MAT_DEPTHWRITE|MAT_SKY)) || rb.curMat->fogDist)
			rb.curTexFog = mb->fog;
		else
			rb.curColorFog = mb->fog;
	}

	// Accumulate passes and render
	debugLightmap = qFalse;
	for (i=0, pass=mb->mat->passes ; i<mb->mat->numPasses ; pass++, i++) {
		if (pass->flags & MAT_PASS_LIGHTMAP) {
			if (rb.curLMTexNum < 0)
				continue;
			debugLightmap = (gl_lightmap->intVal);
		}
		else if (!pass->animNumImages)
			continue;
		if (r_detailTextures->intVal) {
			if (pass->flags & MAT_PASS_NOTDETAIL)
				continue;
		}
		else if (pass->flags & MAT_PASS_DETAIL)
			continue;

		// Accumulate
		RB_AccumulatePass (pass);

		if (pass->flags & MAT_PASS_LIGHTMAP) {
			RB_AccumulatePass (&rb_dLightPass);
			addDlights = qFalse;
		}
	}

	if (debugLightmap) {
		// Accumulate a lightmap pass for debugging purposes
		RB_AccumulatePass (&rb_lightMapPass);
	}
	else if (rb.curTexFog && rb.curTexFog->mat) {
		// Accumulate fog
		rb_fogPass.animImages[0] = ri.fogTexture;
		if (!mb->mat->numPasses || rb.curMat->fogDist || rb.curMat->flags & MAT_SKY)
			rb_fogPass.depthFunc = GL_LEQUAL;
		else
			rb_fogPass.depthFunc = GL_EQUAL;
		RB_AccumulatePass (&rb_fogPass);
	}

	// Render dlights
	if (addDlights || debugLightmap)
		RB_AccumulatePass (&rb_dLightPass);

	// Make sure we've flushed
	if (rb_numPasses)
		RB_RenderAccumulatedPasses ();

	// Unlock arrays
	RB_UnlockArrays ();

	// Reset the texture matrices
	if (rb_matrixCoords)
		qglMatrixMode (GL_MODELVIEW);

	// Reset backend information
	if (!rb_triangleOutlines)
		RB_ResetPointers ();

	// Update performance counters
	ri.pc.meshCount++;
}


/*
=============
RB_FinishRendering

This is called after rendering the mesh list and individual items that pass through the
rendering backend. It resets states so that anything rendered through another system
doesn't catch a leaked state.
=============
*/
void RB_FinishRendering (void)
{
	texUnit_t		i;

	if (rb_matrixCoords)
		qglMatrixMode (GL_TEXTURE);

	for (i=ri.config.maxTexUnits-1 ; i>0 ; i--) {
		RB_SelectTexture (i);
		RB_TextureTarget (0);
		qglDisable (GL_TEXTURE_GEN_S);
		qglDisable (GL_TEXTURE_GEN_T);
		qglDisable (GL_TEXTURE_GEN_R);
		qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
		RB_LoadIdentityTexMatrix ();
	}

	RB_SelectTexture (0);
	RB_TextureTarget (GL_TEXTURE_2D);
	qglDisable (GL_TEXTURE_GEN_S);
	qglDisable (GL_TEXTURE_GEN_T);
	qglDisable (GL_TEXTURE_GEN_R);
	qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
	RB_LoadIdentityTexMatrix ();

	if (rb_matrixCoords)
		qglMatrixMode (GL_MODELVIEW);

	qglDisableClientState (GL_NORMAL_ARRAY);
	qglDisableClientState (GL_COLOR_ARRAY);

	qglColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	RB_StateForBits (SB1_DEPTHMASK_ON|SB1_DEFAULT);

	qglDepthFunc (GL_LEQUAL);

	rb_numOldPasses = 0;
}

/*
===============================================================================

	FRAME HANDLING

===============================================================================
*/

/*
=============
RB_BeginTriangleOutlines
=============
*/
void RB_BeginTriangleOutlines (void)
{
	rb_triangleOutlines = qTrue;

	RB_StateForBits (SB1_BLENDSRC_SRC_ALPHA|SB1_BLENDDST_ONE_MINUS_SRC_ALPHA);
	RB_TextureTarget (0);

	qglColor4ub (255, 255, 255, 255);
}


/*
=============
RB_EndTriangleOutlines
=============
*/
void RB_EndTriangleOutlines (void)
{
	rb_triangleOutlines = qFalse;

	RB_StateForBits (SB1_DEPTHMASK_ON|SB1_DEFAULT);
	RB_TextureTarget (GL_TEXTURE_2D);
}


/*
=============
RB_BeginFrame

Does any pre-frame backend actions
=============
*/
void RB_BeginFrame (void)
{
	static uint32	prevUpdate;
	static uint32	interval = 300;

	rb.numVerts = 0;
	rb.numIndexes = 0;

	rb_numOldPasses = 0;

	// Update the noise table
	if (prevUpdate > Sys_UMilliseconds()%interval) {
		int		i, j, k;
		float	t;

		j = rand()*(FTABLE_SIZE/4);
		k = rand()*(FTABLE_SIZE/2);

		for (i=0 ; i<FTABLE_SIZE ; i++) {
			if (i >= j && i < j+k) {
				t = (double)((i-j)) / (double)(k);
				rb_noiseTable[i] = RB_FastSin (t + 0.25f);
			}
			else {
				rb_noiseTable[i] = 1;
			}
		}

		interval = 300 + rand() % 300;
	}
	prevUpdate = Sys_UMilliseconds () % interval;
}


/*
=============
RB_EndFrame
=============
*/
void RB_EndFrame (void)
{
}

/*
===============================================================================

	INIT / SHUTDOWN

===============================================================================
*/

/*
=============
RB_Init
=============
*/
void R_PassStateBits (matPass_t *pass);
void RB_Init (void)
{
	int		i;
	double	t;

	// Set defaults
	rb_triangleOutlines = qFalse;
	RB_ResetPointers ();

	qglEnableClientState (GL_VERTEX_ARRAY);

	// Build lookup tables
	for (i=0 ; i<FTABLE_SIZE ; i++) {
		t = (double)i / (double)FTABLE_SIZE;

		rb_sinTable[i] = sin (t * (M_PI * 2.0f));
		rb_triangleTable[i] = (t < 0.25) ? t * 4.0f : (t < 0.75) ? 2 - 4.0f * t : (t - 0.75f) * 4.0f - 1.0f;
		rb_squareTable[i] = (t < 0.5) ? 1.0f : -1.0f;
		rb_sawtoothTable[i] = t;
		rb_inverseSawtoothTable[i] = 1.0 - t;
	}

	// Identity lighting
	rb_identityLighting = ri.inverseIntensity * 255;
	if (rb_identityLighting > 255)
		rb_identityLighting = 255;

	// Quake3 BSP specific dynamic light pass
	memset (&rb_dLightPass, 0, sizeof (matPass_t));
	rb_dLightPass.flags = MAT_PASS_DLIGHT|MAT_PASS_BLEND;
	rb_dLightPass.tcGen = TC_GEN_DLIGHT,
	rb_dLightPass.depthFunc = GL_EQUAL;
	rb_dLightPass.blendSource = GL_DST_COLOR;
	rb_dLightPass.blendDest = GL_ONE;
	rb_dLightPass.canAccumulate = qFalse;
	R_PassStateBits (&rb_dLightPass);

	// Create the fog pass
	memset (&rb_fogPass, 0, sizeof (matPass_t));
	rb_fogPass.flags = MAT_PASS_BLEND|MAT_PASS_NOCOLORARRAY;
	rb_fogPass.tcGen = TC_GEN_FOG;
	rb_fogPass.blendMode = GL_DECAL;
	rb_fogPass.blendSource = GL_SRC_ALPHA;
	rb_fogPass.blendDest = GL_ONE_MINUS_SRC_ALPHA;
	rb_fogPass.rgbGen.type = RGB_GEN_FOG;
	rb_fogPass.alphaGen.type = ALPHA_GEN_FOG;
	rb_fogPass.canAccumulate = qFalse;
	R_PassStateBits (&rb_fogPass);

	// Togglable solid lightmap overlay
	memset (&rb_lightMapPass, 0, sizeof (matPass_t));
	rb_lightMapPass.flags = MAT_PASS_LIGHTMAP|MAT_PASS_NOCOLORARRAY;
	rb_lightMapPass.tcGen = TC_GEN_LIGHTMAP;
	rb_lightMapPass.depthFunc = GL_EQUAL;
	rb_lightMapPass.blendMode = GL_REPLACE;
	rb_lightMapPass.rgbGen.type = RGB_GEN_IDENTITY;
	rb_lightMapPass.alphaGen.type = ALPHA_GEN_IDENTITY;
	rb_lightMapPass.canAccumulate = qTrue;
	R_PassStateBits (&rb_lightMapPass);

	// Find rendering paths
	switch (ri.renderClass) {
	case REND_CLASS_INTEL:
	case REND_CLASS_S3:
	case REND_CLASS_SIS:
		// These models are known for not supporting texture matrices properly/at all
		RB_ModifyTextureCoords = RB_ModifyTextureCoordsGeneric;
		rb_matrixCoords = qFalse;
		break;

	default:
		RB_ModifyTextureCoords = RB_ModifyTextureCoordsMatrix;
		rb_matrixCoords = qTrue;
		break;
	}
}


/*
=============
RB_Shutdown
=============
*/
void RB_Shutdown (void)
{
}
