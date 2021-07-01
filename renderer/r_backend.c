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
// r_backend.c
//

#include "r_local.h"

/*
===============================================================================

	ARRAYS

===============================================================================
*/

// Rendering paths
static void				(*RB_ModifyTextureCoords) (shaderPass_t *pass, texUnit_t texUnit);

// Backend information
bvec4_t					*rb_inColorArray;
vec2_t					*rb_inCoordArray;
index_t					*rb_inIndexArray;
vec2_t					*rb_inLMCoordArray;
vec3_t					*rb_inNormalsArray;
vec3_t					*rb_inSVectorsArray;
vec3_t					*rb_inTVectorsArray;
vec3_t					*rb_inVertexArray;
int						rb_numIndexes;
int						rb_numVerts;

#ifdef SHADOW_VOLUMES
int						*rb_inNeighborsArray;
vec3_t					*rb_inTrNormalsArray;
int						*rb_currentTrNeighbor;
float					*rb_currentTrNormal;
#endif

// For rendering
bvec4_t					rb_colorArray[VERTARRAY_MAX_VERTS];
vec2_t					rb_coordArray[VERTARRAY_MAX_VERTS];
index_t					rb_indexArray[VERTARRAY_MAX_INDEXES];
vec2_t					rb_lMCoordArray[VERTARRAY_MAX_VERTS];
vec3_t					rb_normalsArray[VERTARRAY_MAX_VERTS];
vec3_t					rb_sVectorsArray[VERTARRAY_MAX_VERTS];
vec3_t					rb_tVectorsArray[VERTARRAY_MAX_VERTS];
vec3_t					rb_vertexArray[VERTARRAY_MAX_VERTS];

#ifdef SHADOW_VOLUMES
int						rb_neighborsArray[VERTARRAY_MAX_NEIGHBORS];
vec3_t					rb_trNormalsArray[VERTARRAY_MAX_TRIANGLES];
#endif

// Used whenever copying is necessary
static bvec4_t			rb_outColorArray[VERTARRAY_MAX_VERTS];
static vec2_t			rb_outCoordArray[MAX_TEXUNITS][VERTARRAY_MAX_VERTS];
static vec3_t			rb_outVertexArray[VERTARRAY_MAX_VERTS];

// Current item rendering settings
static shaderPass_t		*rb_accumPasses[MAX_TEXUNITS];
static qBool			rb_arraysLocked;
static qBool			rb_triangleOutlines;

static uint32			rb_currentDLightBits;
static entity_t			*rb_currentEntity;
static int				rb_currentLmTexNum;
meshFeatures_t			rb_currentMeshFeatures;
static meshType_t		rb_currentMeshType;
static model_t			*rb_currentModel;
static shader_t			*rb_currentShader;
static mQ3BspFog_t		*rb_texFog, *rb_colorFog;

static uint32			rb_patchWidth;
static uint32			rb_patchHeight;

static int				rb_numPasses;
static int				rb_numOldPasses;
static float			rb_shaderTime;

static int				rb_identityLighting;
static shaderPass_t		rb_dLightPass, rb_fogPass, rb_lightMapPass;

#define FTABLE_SIZE		2048
#define FTABLE_CLAMP(x)	(((int)((x)*FTABLE_SIZE) & (FTABLE_SIZE-1)))
#define FTABLE_EVALUATE(table,x) (table[FTABLE_CLAMP(x)])

static float			rb_sinTable[FTABLE_SIZE];
static float			rb_triangleTable[FTABLE_SIZE];
static float			rb_squareTable[FTABLE_SIZE];
static float			rb_sawtoothTable[FTABLE_SIZE];
static float			rb_inverseSawtoothTable[FTABLE_SIZE];
static float			rb_noiseTable[FTABLE_SIZE];

index_t					rb_quadIndices[QUAD_INDEXES] = {0, 1, 2, 0, 2, 3};
index_t					rb_triFanIndices[TRIFAN_INDEXES];

/*
==============
R_FastSin
==============
*/
float R_FastSin (float t)
{
	return FTABLE_EVALUATE(rb_sinTable, t);
}


/*
==============
R_TableForFunc
==============
*/
static float *R_TableForFunc (shTableFunc_t func)
{
	switch (func) {
	case SHADER_FUNC_SIN:				return rb_sinTable;
	case SHADER_FUNC_TRIANGLE:			return rb_triangleTable;
	case SHADER_FUNC_SQUARE:			return rb_squareTable;
	case SHADER_FUNC_SAWTOOTH:			return rb_sawtoothTable;
	case SHADER_FUNC_INVERSESAWTOOTH:	return rb_inverseSawtoothTable;
	case SHADER_FUNC_NOISE:				return rb_noiseTable;
	}

	// Assume error
	Com_Error (ERR_DROP, "R_TableForFunc: unknown function");

	return NULL;
}


/*
=============
RB_LockArrays
=============
*/
void RB_LockArrays (int numVerts)
{
	if (rb_arraysLocked)
		return;
	if (!glConfig.extCompiledVertArray)
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
	rb_arraysLocked = qFalse;

	rb_currentLmTexNum = -1;
	rb_currentMeshFeatures = 0;
	rb_currentMeshType = MBT_MAX;
	rb_currentShader = NULL;
	rb_currentEntity = NULL;
	rb_currentModel = NULL;

	rb_numIndexes = 0;
	rb_numPasses = 0;
	rb_numVerts = 0;

#ifdef SHADOW_VOLUMES
	rb_inNeighborsArray = rb_neighborsArray;
	rb_inTrNormalsArray = rb_trNormalsArray;

	rb_currentTrNeighbor = rb_inNeighborsArray;
	rb_currentTrNormal = rb_inTrNormalsArray[0];
#endif

	rb_inColorArray = NULL;
	rb_inCoordArray = NULL;
	rb_inIndexArray = NULL;
	rb_inLMCoordArray = NULL;
	rb_inNormalsArray = NULL;
	rb_inVertexArray = NULL;
	rb_inSVectorsArray = NULL;
	rb_inTVectorsArray = NULL;
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
static void RB_SetupColorFog (const shaderPass_t *pass, int numColors)
{
	byte	*bArray;
	double	dist, vdist;
	cBspPlane_t *fogPlane, globalFogPlane;
	vec3_t	viewtofog;
	double	fogNormal[3], vpnNormal[3];
	double	fogDist, vpnDist, fogShaderDist;
	int		fogptype;
	qBool	alphaFog;
	float	c, a;
	int		i;

	if ((pass->blendSource != GL_SRC_ALPHA && pass->blendDest != GL_SRC_ALPHA)
	&& (pass->blendSource != GL_ONE_MINUS_SRC_ALPHA && pass->blendDest != GL_ONE_MINUS_SRC_ALPHA))
		alphaFog = qFalse;
	else
		alphaFog = qTrue;

	fogPlane = rb_colorFog->visiblePlane;
	if (!fogPlane) {
		VectorSet (globalFogPlane.normal, 0, 0, 1);
		globalFogPlane.dist = r_refScene.worldModel->bspModel.nodes[0].maxs[2] + 1;
		globalFogPlane.type = PLANE_Z;
		fogPlane = &globalFogPlane;
	}

	fogShaderDist = rb_colorFog->shader->fogDist;
	dist = PlaneDiff (r_refDef.viewOrigin, fogPlane);

	if (rb_currentShader->flags & SHADER_SKY) {
		if (dist > 0)
			VectorScale (fogPlane->normal, -dist, viewtofog);
		else
			VectorClear (viewtofog);
	}
	else
		VectorCopy (rb_currentEntity->origin, viewtofog);

	vpnNormal[0] = DotProduct (rb_currentEntity->axis[0], r_refDef.forwardVec) * fogShaderDist * rb_currentEntity->scale;
	vpnNormal[1] = DotProduct (rb_currentEntity->axis[1], r_refDef.forwardVec) * fogShaderDist * rb_currentEntity->scale;
	vpnNormal[2] = DotProduct (rb_currentEntity->axis[2], r_refDef.forwardVec) * fogShaderDist * rb_currentEntity->scale;
	vpnDist = ((r_refDef.viewOrigin[0] - viewtofog[0]) * r_refDef.forwardVec[0]
				+ (r_refDef.viewOrigin[1] - viewtofog[1]) * r_refDef.forwardVec[1]
				+ (r_refDef.viewOrigin[2] - viewtofog[2]) * r_refDef.forwardVec[2])
				* fogShaderDist;

	bArray = rb_outColorArray[0];
	if (dist < 0) {
		// Camera is inside the fog
		for (i=0 ; i<numColors ; i++, bArray+=4) {
			c = DotProduct (rb_inVertexArray[i], vpnNormal) - vpnDist;
			a = (1.0f - bound (0, c, 1.0f)) * (1.0 / 255.0);

			if (alphaFog)
				bArray[3] = R_FloatToByte ((float)bArray[3]*a);
			else {
				bArray[0] = R_FloatToByte ((float)bArray[0]*a);
				bArray[1] = R_FloatToByte ((float)bArray[1]*a);
				bArray[2] = R_FloatToByte ((float)bArray[2]*a);
			}
		}
	}
	else {
		fogNormal[0] = DotProduct (rb_currentEntity->axis[0], fogPlane->normal) * rb_currentEntity->scale;
		fogNormal[1] = DotProduct (rb_currentEntity->axis[1], fogPlane->normal) * rb_currentEntity->scale;
		fogNormal[2] = DotProduct (rb_currentEntity->axis[2], fogPlane->normal) * rb_currentEntity->scale;
		fogptype = (fogNormal[0] == 1.0 ? PLANE_X : (fogNormal[1] == 1.0 ? PLANE_Y : (fogNormal[2] == 1.0 ? PLANE_Z : PLANE_NON_AXIAL)));
		if (fogptype > 2)
			VectorScale (fogNormal, fogShaderDist, fogNormal);
		fogDist = (fogPlane->dist - DotProduct (viewtofog, fogPlane->normal)) * fogShaderDist;
		dist *= fogShaderDist;

		for (i=0 ; i<numColors ; i++, bArray+=4) {
			if (fogptype < 3)
				vdist = rb_inVertexArray[i][fogptype] * fogShaderDist - fogDist;
			else
				vdist = DotProduct (rb_inVertexArray[i], fogNormal) - fogDist;

			if (vdist < 0) {
				c = (DotProduct (rb_inVertexArray[i], vpnNormal) - vpnDist) * vdist / (vdist - dist);
				a = (1.0f - bound (0, c, 1.0f)) * (1.0 / 255.0);

				if (alphaFog)
					bArray[3] = R_FloatToByte ((float)bArray[3]*a);
				else {
					bArray[0] = R_FloatToByte ((float)bArray[0]*a);
					bArray[1] = R_FloatToByte ((float)bArray[1]*a);
					bArray[2] = R_FloatToByte ((float)bArray[2]*a);
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
static qBool RB_SetupColorFast (const shaderPass_t *pass)
{
	const shaderFunc_t *rgbGenFunc, *alphaGenFunc;
	byte		*inArray;
	bvec4_t		color;
	float		*table, c, a;

	rgbGenFunc = &pass->rgbGen.func;
	alphaGenFunc = &pass->alphaGen.func;
	inArray = rb_inColorArray[0];

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
		color[0] = R_FloatToByte (pass->rgbGen.args[0]);
		color[1] = R_FloatToByte (pass->rgbGen.args[1]);
		color[2] = R_FloatToByte (pass->rgbGen.args[2]);
		break;

	case RGB_GEN_COLORWAVE:
		table = R_TableForFunc (rgbGenFunc->type);
		c = rb_shaderTime * rgbGenFunc->args[3] + rgbGenFunc->args[2];
		c = FTABLE_EVALUATE(table, c) * rgbGenFunc->args[1] + rgbGenFunc->args[0];
		a = pass->rgbGen.args[0] * c; color[0] = R_FloatToByte (bound (0, a, 1));
		a = pass->rgbGen.args[1] * c; color[1] = R_FloatToByte (bound (0, a, 1));
		a = pass->rgbGen.args[2] * c; color[2] = R_FloatToByte (bound (0, a, 1));
		break;

	case RGB_GEN_ENTITY:
		color[0] = rb_currentEntity->color[0];
		color[1] = rb_currentEntity->color[1];
		color[2] = rb_currentEntity->color[2];
		break;

	case RGB_GEN_ONE_MINUS_ENTITY:
		color[0] = 255 - rb_currentEntity->color[0];
		color[1] = 255 - rb_currentEntity->color[1];
		color[2] = 255 - rb_currentEntity->color[2];
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
		color[3] = R_FloatToByte (pass->alphaGen.args[0]);
		break;

	case ALPHA_GEN_ENTITY:
		color[3] = rb_currentEntity->color[3];
		break;

	case ALPHA_GEN_WAVE:
		table = R_TableForFunc (alphaGenFunc->type);
		a = alphaGenFunc->args[2] + rb_shaderTime * alphaGenFunc->args[3];
		a = FTABLE_EVALUATE(table, a) * alphaGenFunc->args[1] + alphaGenFunc->args[0];
		color[3] = R_FloatToByte (bound (0.0f, a, 1.0f));
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
static void RB_SetupColor (const shaderPass_t *pass)
{
	const shaderFunc_t *rgbGenFunc, *alphaGenFunc;
	int		r, g, b;
	float	*table, c, a;
	byte	*bArray, *inArray;
	int		numColors, i;
	vec3_t	t, v;

	rgbGenFunc = &pass->rgbGen.func;
	alphaGenFunc = &pass->alphaGen.func;

	// Optimal case
	if (pass->flags & SHADER_PASS_NOCOLORARRAY && !rb_colorFog) {
		if (RB_SetupColorFast (pass))
			return;
		numColors = 1;
	}
	else
		numColors = rb_numVerts;

	// Color generation
	bArray = rb_outColorArray[0];
	inArray = rb_inColorArray[0];
	switch (pass->rgbGen.type) {
	case RGB_GEN_UNKNOWN:
	case RGB_GEN_IDENTITY:
		memset (bArray, 255, sizeof (bvec4_t) * numColors);
		break;

	case RGB_GEN_IDENTITY_LIGHTING:
		memset (bArray, rb_identityLighting, sizeof (bvec4_t) * numColors);
		break;

	case RGB_GEN_CONST:
		r = R_FloatToByte (pass->rgbGen.args[0]);
		g = R_FloatToByte (pass->rgbGen.args[1]);
		b = R_FloatToByte (pass->rgbGen.args[2]);

		for (i=0 ; i<numColors ; i++, bArray+=4) {
			bArray[0] = r;
			bArray[1] = g;
			bArray[2] = b;
		}
		break;

	case RGB_GEN_COLORWAVE:
		table = R_TableForFunc (rgbGenFunc->type);
		c = rb_shaderTime * rgbGenFunc->args[3] + rgbGenFunc->args[2];
		c = FTABLE_EVALUATE(table, c) * rgbGenFunc->args[1] + rgbGenFunc->args[0];
		a = pass->rgbGen.args[0] * c; r = R_FloatToByte (bound (0, a, 1));
		a = pass->rgbGen.args[1] * c; g = R_FloatToByte (bound (0, a, 1));
		a = pass->rgbGen.args[2] * c; b = R_FloatToByte (bound (0, a, 1));

		for (i=0 ; i<numColors ; i++, bArray+=4) {
			bArray[0] = r;
			bArray[1] = g;
			bArray[2] = b;
		}
		break;

	case RGB_GEN_ENTITY:
		r = *(int *)rb_currentEntity->color;
		for (i=0 ; i<numColors ; i++, bArray+=4)
			*(int *)bArray = r;
		break;

	case RGB_GEN_ONE_MINUS_ENTITY:
		for (i=0 ; i<numColors ; i++, bArray+=4) {
			bArray[0] = 255 - rb_currentEntity->color[0];
			bArray[1] = 255 - rb_currentEntity->color[1];
			bArray[2] = 255 - rb_currentEntity->color[2];
		}
		break;

	case RGB_GEN_VERTEX:
		if (intensity->integer > 0) {
			for (i=0 ; i<numColors ; i++, bArray+=4, inArray+=4) {
				bArray[0] = inArray[0] >> (intensity->integer / 2);
				bArray[1] = inArray[1] >> (intensity->integer / 2);
				bArray[2] = inArray[2] >> (intensity->integer / 2);
			}
			break;
		}

	// FALL THROUGH
	case RGB_GEN_EXACT_VERTEX:
		memcpy (bArray, inArray, sizeof (bvec4_t) * numColors);
		break;

	case RGB_GEN_ONE_MINUS_VERTEX:
		if (intensity->integer > 0) {
			for (i=0 ; i<numColors ; i++, bArray+=4, inArray+=4) {
				bArray[0] = 255 - (inArray[0] >> (intensity->integer / 2));
				bArray[1] = 255 - (inArray[1] >> (intensity->integer / 2));
				bArray[2] = 255 - (inArray[2] >> (intensity->integer / 2));
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
		R_LightForEntity (rb_currentEntity, numColors, bArray);
		break;

	case RGB_GEN_FOG:
		for (i=0 ; i<numColors ; i++, bArray+=4) {
			bArray[0] = rb_texFog->shader->fogColor[0];
			bArray[1] = rb_texFog->shader->fogColor[1];
			bArray[2] = rb_texFog->shader->fogColor[2];
		}
		break;

	default:
		assert (0);
		break;
	}

	// Alpha generation
	bArray = rb_outColorArray[0];
	inArray = rb_inColorArray[0];
	switch (pass->alphaGen.type) {
	case ALPHA_GEN_UNKNOWN:
	case ALPHA_GEN_IDENTITY:
		for (i=0 ; i<numColors ; i++, bArray+=4) {
			bArray[3] = 255;
		}
		break;

	case ALPHA_GEN_CONST:
		b = R_FloatToByte (pass->alphaGen.args[0]);
		for (i=0 ; i<numColors ; i++, bArray+=4)
			bArray[3] = b;
		break;

	case ALPHA_GEN_WAVE:
		table = R_TableForFunc (alphaGenFunc->type);
		a = alphaGenFunc->args[2] + rb_shaderTime * alphaGenFunc->args[3];
		a = FTABLE_EVALUATE(table, a) * alphaGenFunc->args[1] + alphaGenFunc->args[0];
		b = R_FloatToByte (bound (0.0f, a, 1.0f));
		for (i=0 ; i<numColors ; i++, bArray+=4)
			bArray[3] = b;
		break;

	case ALPHA_GEN_PORTAL:
		VectorAdd (rb_inVertexArray[0], rb_currentEntity->origin, v);
		VectorSubtract (r_refDef.viewOrigin, v, t);
		a = VectorLength (t) * pass->alphaGen.args[0];
		b = R_FloatToByte (clamp (a, 0.0f, 1.0f));

		for (i=0 ; i<numColors ; i++, bArray+=4)
			bArray[3] = b;
		break;

	case ALPHA_GEN_VERTEX:
		for (i=0 ; i<numColors; i++, bArray+=4, inArray+=4)
			bArray[3] = inArray[3];
		break;

	case ALPHA_GEN_ENTITY:
		for (i=0 ; i<numColors; i++, bArray+=4)
			bArray[3] = rb_currentEntity->color[3];
		break;

	case ALPHA_GEN_SPECULAR:
		VectorSubtract (r_refDef.viewOrigin, rb_currentEntity->origin, t);
		if (!Matrix3_Compare (rb_currentEntity->axis, axisIdentity))
			Matrix3_TransformVector (rb_currentEntity->axis, t, v);
		else
			VectorCopy (t, v);

		for (i=0 ; i<numColors ; i++, bArray+=4) {
			VectorSubtract (v, rb_inVertexArray[i], t);
			a = DotProduct (t, rb_inNormalsArray[i]) * Q_RSqrt (DotProduct (t, t));
			a = a * a * a * a * a;
			bArray[3] = R_FloatToByte (bound (0.0f, a, 1.0f));
		}
		break;

	case ALPHA_GEN_DOT:
		if (!Matrix3_Compare (rb_currentEntity->axis, axisIdentity))
			Matrix3_TransformVector (rb_currentEntity->axis, r_refDef.forwardVec, v);
		else
			VectorCopy (r_refDef.forwardVec, v);

		for (i=0 ; i<numColors ; i++, bArray+=4) {
			a = DotProduct (v, rb_inNormalsArray[i]);
			if (a < 0)
				a = -a;
			bArray[3] = R_FloatToByte (bound (pass->alphaGen.args[0], a, pass->alphaGen.args[1]));
		}
		break;

	case ALPHA_GEN_ONE_MINUS_DOT:
		if (!Matrix3_Compare (rb_currentEntity->axis, axisIdentity))
			Matrix3_TransformVector (rb_currentEntity->axis, r_refDef.forwardVec, v);
		else
			VectorCopy (r_refDef.forwardVec, v);

		for (i=0 ; i<numColors ; i++, bArray+=4) {
			a = DotProduct (v, rb_inNormalsArray[i]);
			if (a < 0)
				a = -a;
			a = 1.0f - a;
			bArray[3] = R_FloatToByte (bound (pass->alphaGen.args[0], a, pass->alphaGen.args[1]));
		}
		break;

	case ALPHA_GEN_FOG:
		for (i=0 ; i<numColors ; i++, bArray+=4) {
			bArray[3] = rb_texFog->shader->fogColor[3];
		}
		break;

	default:
		assert (0);
		break;
	}

	// Colored fog
	if (rb_colorFog)
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
static void RB_VertexTCBaseGeneric (shaderPass_t *pass, texUnit_t texUnit)
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
			memcpy (outCoords, rb_inCoordArray[0], sizeof(float) * 2 * rb_numVerts);
			break;
		}

		qglTexCoordPointer (2, GL_FLOAT, 0, rb_inCoordArray);
		return;

	case TC_GEN_LIGHTMAP:
		if (pass->numTCMods) {
			memcpy (outCoords, rb_inLMCoordArray[0], sizeof(float) * 2 * rb_numVerts);
			break;
		}

		qglTexCoordPointer (2, GL_FLOAT, 0, rb_inLMCoordArray);
		return;

	case TC_GEN_ENVIRONMENT:
		if (glState.in2D) {
			memcpy (outCoords, rb_inCoordArray[0], sizeof(float) * 2 * rb_numVerts);
			return;
		}

		if (rb_currentModel) {
			if (rb_currentModel == r_refScene.worldModel) {
				VectorSubtract (vec3Origin, r_refDef.viewOrigin, transform);
			}
			else {
				VectorSubtract (rb_currentEntity->origin, r_refDef.viewOrigin, transform);
				Matrix3_Transpose (rb_currentEntity->axis, inverseAxis);
			}
		}
		else {
			VectorClear (transform);
			Matrix3_Transpose (axisIdentity, inverseAxis);
		}

		for (i=0 ; i<rb_numVerts ; i++, outCoords+=2) {
			VectorAdd (rb_inVertexArray[i], transform, projection);
			VectorNormalizeFastf (projection);

			// Project vector
			if (rb_currentModel && rb_currentModel == r_refScene.worldModel)
				VectorCopy (rb_inNormalsArray[i], n);
			else
				Matrix3_TransformVector (inverseAxis, rb_inNormalsArray[i], n);

			depth = -2.0f * DotProduct (n, projection);
			VectorMA (projection, depth, n, projection);
			depth = Q_FastSqrt (DotProduct (projection, projection) * 4);

			outCoords[0] = -((projection[1] * depth) + 0.5f);
			outCoords[1] = ((projection[2] * depth) + 0.5f);
		}
		break;

	case TC_GEN_VECTOR:
		for (i=0 ; i<rb_numVerts ; i++, outCoords+=2) {
			outCoords[0] = DotProduct (pass->tcGenVec[0], rb_inVertexArray[i]) + pass->tcGenVec[0][3];
			outCoords[1] = DotProduct (pass->tcGenVec[1], rb_inVertexArray[i]) + pass->tcGenVec[1][3];
		}
		break;

	case TC_GEN_REFLECTION:
		qglTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
		qglTexGeni (GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
		qglTexGeni (GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);

		qglNormalPointer (GL_FLOAT, 12, rb_inNormalsArray);
		break;


	case TC_GEN_WARP:
		for (i=0 ; i<rb_numVerts ; i++, outCoords+=2) {
			outCoords[0] = rb_inCoordArray[i][0] + (r_warpSinTable[Q_ftol (((rb_inCoordArray[i][1]*8.0f + rb_shaderTime) * (256.0f / (M_PI * 2.0f)))) & 255] * (1.0/64));
			outCoords[1] = rb_inCoordArray[i][1] + (r_warpSinTable[Q_ftol (((rb_inCoordArray[i][0]*8.0f + rb_shaderTime) * (256.0f / (M_PI * 2.0f)))) & 255] * (1.0/64));
		}
		break;

	case TC_GEN_FOG:
		{
			int			fogPtype;
			cBspPlane_t	*fogPlane, globalFogPlane;
			shader_t	*fogShader;
			vec3_t		viewtofog;
			double		fogNormal[3], vpnNormal[3];
			double		dist, vdist, fogDist, vpnDist;
			vec3_t		fogVPN;

			fogPlane = rb_texFog->visiblePlane;
			if (!fogPlane) {
				VectorSet (globalFogPlane.normal, 0, 0, 1);
				globalFogPlane.dist = r_refScene.worldModel->bspModel.nodes[0].maxs[2] + 1;
				globalFogPlane.type = PLANE_Z;
				fogPlane = &globalFogPlane;
			}
			fogShader = rb_texFog->shader;

			// Distance to fog
			dist = PlaneDiff (r_refDef.viewOrigin, fogPlane);
			if (rb_currentShader->flags & SHADER_SKY) {
				if (dist > 0)
					VectorMA (r_refDef.viewOrigin, -dist, fogPlane->normal, viewtofog);
				else
					VectorCopy (r_refDef.viewOrigin, viewtofog);
			}
			else
				VectorCopy (rb_currentEntity->origin, viewtofog);

			VectorScale (r_refDef.forwardVec, fogShader->fogDist, fogVPN);

			// Fog settings
			fogNormal[0] = DotProduct (rb_currentEntity->axis[0], fogPlane->normal) * rb_currentEntity->scale;
			fogNormal[1] = DotProduct (rb_currentEntity->axis[1], fogPlane->normal) * rb_currentEntity->scale;
			fogNormal[2] = DotProduct (rb_currentEntity->axis[2], fogPlane->normal) * rb_currentEntity->scale;
			fogPtype = (fogNormal[0] == 1.0 ? PLANE_X : (fogNormal[1] == 1.0 ? PLANE_Y : (fogNormal[2] == 1.0 ? PLANE_Z : PLANE_NON_AXIAL)));
			fogDist = (fogPlane->dist - DotProduct (viewtofog, fogPlane->normal));

			// Forward view normal/distance
			vpnNormal[0] = DotProduct (rb_currentEntity->axis[0], fogVPN) * rb_currentEntity->scale;
			vpnNormal[1] = DotProduct (rb_currentEntity->axis[1], fogVPN) * rb_currentEntity->scale;
			vpnNormal[2] = DotProduct (rb_currentEntity->axis[2], fogVPN) * rb_currentEntity->scale;
			vpnDist = ((r_refDef.viewOrigin[0] - viewtofog[0]) * fogVPN[0]
						+ (r_refDef.viewOrigin[1] - viewtofog[1]) * fogVPN[1]
						+ (r_refDef.viewOrigin[2] - viewtofog[2]) * fogVPN[2]);

			if (dist < 0) {
				// Camera is inside the fog brush
				if (fogPtype < 3) {
					for (i=0 ; i<rb_numVerts ; i++, outCoords+=2) {
						outCoords[0] = DotProduct (rb_inVertexArray[i], vpnNormal) - vpnDist;
						outCoords[1] = -(rb_inVertexArray[i][fogPtype] - fogDist);
					}
				}
				else {
					for (i=0 ; i<rb_numVerts ; i++, outCoords+=2) {
						outCoords[0] = DotProduct (rb_inVertexArray[i], vpnNormal) - vpnDist;
						outCoords[1] = -(DotProduct (rb_inVertexArray[i], fogNormal) - fogDist);
					}
				}
			}
			else {
				if (fogPtype < 3) {
					for (i=0 ; i<rb_numVerts ; i++, outCoords+=2) {
						vdist = rb_inVertexArray[i][fogPtype] - fogDist;
						outCoords[0] = ((vdist < 0) ? (DotProduct (rb_inVertexArray[i], vpnNormal) - vpnDist) * vdist / (vdist - dist) : 0.0f);
						outCoords[1] = -vdist;
					}
				}
				else {
					for (i=0 ; i<rb_numVerts ; i++, outCoords+=2) {
						vdist = DotProduct (rb_inVertexArray[i], fogNormal) - fogDist;
						outCoords[0] = ((vdist < 0) ? (DotProduct (rb_inVertexArray[i], vpnNormal) - vpnDist) * vdist / (vdist - dist) : 0.0f);
						outCoords[1] = -vdist;
					}
				}
			}

			outCoords = rb_outCoordArray[texUnit][0];
			for (i=0 ; i<rb_numVerts ; i++, outCoords+=2) {
				outCoords[1] *= fogShader->fogDist + 1.5f/(float)FOGTEX_HEIGHT;
			}
		}
		break;

	default:
		assert (0);
		break;
	}

	qglTexCoordPointer (2, GL_FLOAT, 0, rb_outCoordArray[texUnit][0]);
}
static void RB_ModifyTextureCoordsGeneric (shaderPass_t *pass, texUnit_t texUnit)
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
			cost = tcMod->args[0] * rb_shaderTime;
			sint = R_FastSin (cost);
			cost = R_FastSin (cost + 0.25);

			for (j=0 ; j<rb_numVerts ; j++, tcArray+=2) {
				t1 = tcArray[0];
				t2 = tcArray[1];
				tcArray[0] = cost * (t1 - 0.5f) - sint * (t2 - 0.5f) + 0.5f;
				tcArray[1] = cost * (t2 - 0.5f) + sint * (t1 - 0.5f) + 0.5f;
			}
			break;

		case TC_MOD_SCALE:
			t1 = tcMod->args[0];
			t2 = tcMod->args[1];

			for (j=0 ; j<rb_numVerts ; j++, tcArray+=2) {
				tcArray[0] = tcArray[0] * t1;
				tcArray[1] = tcArray[1] * t2;
			}
			break;

		case TC_MOD_TURB:
			t1 = tcMod->args[1];
			t2 = tcMod->args[2] + rb_shaderTime * tcMod->args[3];

			for (j=0 ; j<rb_numVerts ; j++, tcArray+=2) {
				tcArray[0] = tcArray[0] + t1 * R_FastSin (tcArray[0] * t1 + t2);
				tcArray[1] = tcArray[1] + t1 * R_FastSin (tcArray[1] * t1 + t2);
			}
			break;

		case TC_MOD_STRETCH:
			table = R_TableForFunc (tcMod->args[0]);
			t2 = tcMod->args[3] + rb_shaderTime * tcMod->args[4];
			t1 = FTABLE_EVALUATE(table, t2) * tcMod->args[2] + tcMod->args[1];
			t1 = t1 ? 1.0f / t1 : 1.0f;
			t2 = 0.5f - 0.5f * t1;

			for (j=0 ; j<rb_numVerts ; j++, tcArray+=2) {
				tcArray[0] = tcArray[0] * t1 + t2;
				tcArray[1] = tcArray[1] * t1 + t2;
			}
			break;

		case TC_MOD_SCROLL:
			t1 = tcMod->args[0] * rb_shaderTime; t1 = t1 - floor (t1);
			t2 = tcMod->args[1] * rb_shaderTime; t2 = t2 - floor (t2);

			for (j=0 ; j<rb_numVerts ; j++, tcArray+=2) {
				tcArray[0] = tcArray[0] + t1;
				tcArray[1] = tcArray[1] + t2;
			}
			break;

		case TC_MOD_TRANSFORM:
			for (j=0 ; j<rb_numVerts ; j++, tcArray+=2) {
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
static void R_ApplyTCModsMatrix (shaderPass_t *pass, mat4x4_t result)
{
	mat4x4_t	m1, m2;
	float		t1, t2, sint, cost;
	float		*table;
	tcMod_t		*tcMod;
	int			i;

	for (i=0, tcMod=pass->tcMods ; i<pass->numTCMods ; tcMod++, i++) {
		switch (tcMod->type) {
		case TC_MOD_ROTATE:
			cost = tcMod->args[0] * rb_shaderTime;
			sint = R_FastSin (cost);
			cost = R_FastSin (cost + 0.25);
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
			t2 = tcMod->args[2] + rb_shaderTime * tcMod->args[3];
			Matrix4_Scale2D (result, 1 + (tcMod->args[1] * R_FastSin (t2) + tcMod->args[0]) * t1, 1 + (tcMod->args[1] * R_FastSin (t2 + 0.25f) + tcMod->args[0]) * t1);
			break;

		case TC_MOD_STRETCH:
			table = R_TableForFunc (tcMod->args[0]);
			t2 = tcMod->args[3] + rb_shaderTime * tcMod->args[4];
			t1 = FTABLE_EVALUATE (table, t2) * tcMod->args[2] + tcMod->args[1];
			t1 = t1 ? 1.0f / t1 : 1.0f;
			t2 = 0.5f - 0.5f * t1;
			Matrix4_Stretch2D (result, t1, t2);
			break;

		case TC_MOD_SCROLL:
			t1 = tcMod->args[0] * rb_shaderTime; t1 = t1 - floor (t1);
			t2 = tcMod->args[1] * rb_shaderTime; t2 = t2 - floor (t2);
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
static qBool RB_VertexTCBaseMatrix (shaderPass_t *pass, texUnit_t texUnit, mat4x4_t matrix)
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
		qglTexCoordPointer (2, GL_FLOAT, 0, rb_inCoordArray);
		return qTrue;

	case TC_GEN_LIGHTMAP:
		qglTexCoordPointer (2, GL_FLOAT, 0, rb_inLMCoordArray);
		return qTrue;

	case TC_GEN_ENVIRONMENT:
		if (glState.in2D)
			return qTrue;

		matrix[0] = matrix[12] = -0.5f;
		matrix[5] = matrix[13] = 0.5f;

		VectorSubtract (r_refDef.viewOrigin, rb_currentEntity->origin, transform);
		Matrix3_Transpose (rb_currentEntity->axis, inverseAxis);

		if (Matrix3_Compare (inverseAxis, axisIdentity)) {
			for (i=0 ; i<rb_numVerts ; i++) {
				VectorSubtract (rb_inVertexArray[i], transform, projection);
				VectorNormalizeFastf (projection);
				VectorCopy (rb_inNormalsArray[i], n);

				// Project vector
				depth = -DotProduct (n, projection);
				depth += depth;
				VectorMA (projection, depth, n, projection);
				depth = Q_RSqrt (DotProduct (projection, projection));

				rb_outCoordArray[texUnit][i][0] = projection[1] * depth;
				rb_outCoordArray[texUnit][i][1] = projection[2] * depth;
			}
		}
		else {
			for (i=0 ; i<rb_numVerts ; i++) {
				VectorSubtract (rb_inVertexArray[i], transform, projection);
				VectorNormalizeFastf (projection);
				Matrix3_TransformVector (inverseAxis, rb_inNormalsArray[i], n);

				// Project vector
				depth = -DotProduct (n, projection);
				depth += depth;
				VectorMA (projection, depth, n, projection);
				depth = Q_RSqrt (DotProduct (projection, projection));

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

		qglNormalPointer (GL_FLOAT, 12, rb_inNormalsArray);
		return qTrue;

	case TC_GEN_WARP:
		for (i=0 ; i<rb_numVerts ; i++) {
			rb_outCoordArray[texUnit][i][0] = rb_inCoordArray[i][0] + (r_warpSinTable[Q_ftol (((rb_inCoordArray[i][1]*8.0f + rb_shaderTime) * (256.0f / (M_PI * 2.0f)))) & 255] * (1.0/64));
			rb_outCoordArray[texUnit][i][1] = rb_inCoordArray[i][1] + (r_warpSinTable[Q_ftol (((rb_inCoordArray[i][0]*8.0f + rb_shaderTime) * (256.0f / (M_PI * 2.0f)))) & 255] * (1.0/64));
		}

		qglTexCoordPointer (2, GL_FLOAT, 0, rb_outCoordArray[texUnit][0]);
		return qTrue;

	case TC_GEN_FOG:
		{
			int			fogPtype;
			cBspPlane_t	*fogPlane, globalFogPlane;
			shader_t	*fogShader;
			vec3_t		viewtofog;
			double		fogNormal[3], vpnNormal[3];
			double		dist, vdist, fogDist, vpnDist;
			float		*outCoords;

			fogPlane = rb_texFog->visiblePlane;
			if (!fogPlane) {
				VectorSet (globalFogPlane.normal, 0, 0, 1);
				globalFogPlane.dist = r_refScene.worldModel->bspModel.nodes[0].maxs[2] + 1;
				globalFogPlane.type = PLANE_Z;
				fogPlane = &globalFogPlane;
			}
			fogShader = rb_texFog->shader;

			matrix[0] = matrix[5] = fogShader->fogDist;
			matrix[13] = 1.5/(float)FOGTEX_HEIGHT;

			// Distance to fog
			dist = PlaneDiff (r_refDef.viewOrigin, fogPlane);

			if (rb_currentShader->flags & SHADER_SKY) {
				if (dist > 0)
					VectorMA (r_refDef.viewOrigin, -dist, fogPlane->normal, viewtofog);
				else
					VectorCopy (r_refDef.viewOrigin, viewtofog);
			}
			else
				VectorCopy (rb_currentEntity->origin, viewtofog);

			// Fog settings
			fogNormal[0] = DotProduct (rb_currentEntity->axis[0], fogPlane->normal) * rb_currentEntity->scale;
			fogNormal[1] = DotProduct (rb_currentEntity->axis[1], fogPlane->normal) * rb_currentEntity->scale;
			fogNormal[2] = DotProduct (rb_currentEntity->axis[2], fogPlane->normal) * rb_currentEntity->scale;
			fogPtype = (fogNormal[0] == 1.0 ? PLANE_X : (fogNormal[1] == 1.0 ? PLANE_Y : (fogNormal[2] == 1.0 ? PLANE_Z : PLANE_NON_AXIAL)));
			fogDist = (fogPlane->dist - DotProduct (viewtofog, fogPlane->normal));

			// Forward view normal/distance
			vpnNormal[0] = DotProduct (rb_currentEntity->axis[0], r_refDef.forwardVec) * rb_currentEntity->scale;
			vpnNormal[1] = DotProduct (rb_currentEntity->axis[1], r_refDef.forwardVec) * rb_currentEntity->scale;
			vpnNormal[2] = DotProduct (rb_currentEntity->axis[2], r_refDef.forwardVec) * rb_currentEntity->scale;
			vpnDist = ((r_refDef.viewOrigin[0] - viewtofog[0]) * r_refDef.forwardVec[0]
						+ (r_refDef.viewOrigin[1] - viewtofog[1]) * r_refDef.forwardVec[1]
						+ (r_refDef.viewOrigin[2] - viewtofog[2]) * r_refDef.forwardVec[2]);

			outCoords = rb_outCoordArray[texUnit][0];
			if (dist < 0) {
				// Camera is inside the fog brush
				if (fogPtype < 3) {
					for (i=0 ; i<rb_numVerts ; i++, outCoords+=2) {
						outCoords[0] = DotProduct (rb_inVertexArray[i], vpnNormal) - vpnDist;
						outCoords[1] = -(rb_inVertexArray[i][fogPtype] - fogDist);
					}
				}
				else {
					for (i=0 ; i<rb_numVerts ; i++, outCoords+=2) {
						outCoords[0] = DotProduct (rb_inVertexArray[i], vpnNormal) - vpnDist;
						outCoords[1] = -(DotProduct (rb_inVertexArray[i], fogNormal) - fogDist);
					}
				}
			}
			else {
				if (fogPtype < 3) {
					for (i=0 ; i<rb_numVerts ; i++, outCoords+=2) {
						vdist = rb_inVertexArray[i][fogPtype] - fogDist;
						outCoords[0] = ((vdist < 0) ? (DotProduct (rb_inVertexArray[i], vpnNormal) - vpnDist) * vdist / (vdist - dist) : 0.0f);
						outCoords[1] = -vdist;
					}
				}
				else {
					for (i=0 ; i<rb_numVerts ; i++, outCoords+=2) {
						vdist = DotProduct (rb_inVertexArray[i], fogNormal) - fogDist;
						outCoords[0] = ((vdist < 0) ? (DotProduct (rb_inVertexArray[i], vpnNormal) - vpnDist) * vdist / (vdist - dist) : 0.0f);
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
static void RB_ModifyTextureCoordsMatrix (shaderPass_t *pass, texUnit_t texUnit)
{
	mat4x4_t	m1, m2, result;
	qBool		identityMatrix;

	// Texture coordinate base
	identityMatrix = RB_VertexTCBaseMatrix (pass, texUnit, result);

	// Texture coordinate modifications
	qglMatrixMode (GL_TEXTURE);

	if (pass->numTCMods) {
		identityMatrix = qFalse;
		R_ApplyTCModsMatrix (pass, result);
	}

	if (pass->tcGen == TC_GEN_REFLECTION) {
		Matrix4_Transpose (r_refScene.modelViewMatrix, m1);
		Matrix4_Copy (result, m2);
		Matrix4_Multiply (m2, m1, result);
		GL_LoadTexMatrix (result);
		return;
	}

	// Load identity
	if (identityMatrix)
		GL_LoadIdentityTexMatrix ();
	else
		GL_LoadTexMatrix (result);
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
	float			*quad[4], *table, now;
	int				i, j, k;
	uint32			l, m, p;
	vec3_t			tv, rot_centre;

	// Deformations
	vertDeform = &rb_currentShader->deforms[0];
	for (i=0 ; i<rb_currentShader->numDeforms ; i++, vertDeform++) {
		switch (vertDeform->type) {
		case DEFORMV_NONE:
			break;

		case DEFORMV_WAVE:
			table = R_TableForFunc (vertDeform->func.type);
			now = vertDeform->func.args[2] + vertDeform->func.args[3] * rb_shaderTime;

			for (j=0 ; j<rb_numVerts ; j++) {
				deflect = (rb_inVertexArray[j][0] + rb_inVertexArray[j][1] + rb_inVertexArray[j][2]) * vertDeform->args[0] + now;
				deflect = FTABLE_EVALUATE (table, deflect) * vertDeform->func.args[1] + vertDeform->func.args[0];

				// Deflect vertex along its normal by wave amount
				VectorMA (rb_inVertexArray[j], deflect, rb_inNormalsArray[j], rb_outVertexArray[j]);
			}
			break;

		case DEFORMV_NORMAL:
			args[0] = vertDeform->args[1] * rb_shaderTime;

			for (j=0 ; j<rb_numVerts ; j++) {
				args[1] = rb_inNormalsArray[j][2] * args[0];

				deflect = vertDeform->args[0] * R_FastSin (args[1]);
				rb_inNormalsArray[j][0] *= deflect;
				deflect = vertDeform->args[0] * R_FastSin (args[1] + 0.25);
				rb_inNormalsArray[j][1] *= deflect;

				VectorNormalizeFastf (rb_inNormalsArray[j]);
			}
			break;

		case DEFORMV_BULGE:
			args[0] = vertDeform->args[0] / (float)rb_patchHeight;
			args[1] = vertDeform->args[1];
			args[2] = rb_shaderTime / (vertDeform->args[2] * rb_patchWidth);

			for (l=0, p=0 ; l<rb_patchHeight ; l++) {
				deflect = R_FastSin ((float)l * args[0] + args[2]) * args[1];
				for (m=0 ; m<rb_patchWidth ; m++, p++)
					VectorMA (rb_inVertexArray[p], deflect, rb_inNormalsArray[p], rb_outVertexArray[p]);
			}
			break;

		case DEFORMV_MOVE:
			table = R_TableForFunc (vertDeform->func.type);
			deflect = vertDeform->func.args[2] + rb_shaderTime * vertDeform->func.args[3];
			deflect = FTABLE_EVALUATE(table, deflect) * vertDeform->func.args[1] + vertDeform->func.args[0];

			for (j=0 ; j<rb_numVerts ; j++)
				VectorMA (rb_inVertexArray[j], deflect, vertDeform->args, rb_outVertexArray[j]);
			break;

		case DEFORMV_AUTOSPRITE:
			{
				mat3x3_t	m0, m1, m2, result;

				if (rb_numIndexes % 6)
					break;

				memcpy (&rb_outVertexArray, rb_inVertexArray, sizeof (rb_outVertexArray)); // FIXME
				if (rb_currentModel && rb_currentModel == r_refScene.worldModel)
					Matrix4_Matrix3 (r_refScene.worldViewMatrix, m1);
				else
					Matrix4_Matrix3 (r_refScene.modelViewMatrix, m1);

				Matrix3_Transpose (m1, m2);

				for (k=0 ; k<rb_numIndexes ; k+=6) {
					quad[0] = (float *)(rb_outVertexArray + rb_inIndexArray[k+0]);
					quad[1] = (float *)(rb_outVertexArray + rb_inIndexArray[k+1]);
					quad[2] = (float *)(rb_outVertexArray + rb_inIndexArray[k+2]);

					for (j=2 ; j>=0 ; j--) {
						quad[3] = (float *)(rb_outVertexArray + rb_inIndexArray[k+3+j]);
						if (!VectorCompare (quad[3], quad[0])
						&& !VectorCompare (quad[3], quad[1])
						&& !VectorCompare (quad[3], quad[2])) {
							break;
						}
					}

					Matrix3_FromPoints (quad[0], quad[1], quad[2], m0);

					// Swap m0[0] an m0[1] - FIXME?
					VectorCopy (m0[1], rot_centre);
					VectorCopy (m0[0], m0[1]);
					VectorCopy (rot_centre, m0[0]);

					Matrix3_Multiply (m2, m0, result);

					rot_centre[0] = (quad[0][0] + quad[1][0] + quad[2][0] + quad[3][0]) * 0.25;
					rot_centre[1] = (quad[0][1] + quad[1][1] + quad[2][1] + quad[3][1]) * 0.25;
					rot_centre[2] = (quad[0][2] + quad[1][2] + quad[2][2] + quad[3][2]) * 0.25;

					for (j=0 ; j<4 ; j++) {
						VectorSubtract (quad[j], rot_centre, tv);
						Matrix3_TransformVector (result, tv, quad[j]);
						VectorAdd (rot_centre, quad[j], quad[j]);
					}
				}
			}
			break;

		case DEFORMV_AUTOSPRITE2:
			if (rb_numIndexes % 6)
				break;

			for (k=0 ; k<rb_numIndexes ; k+=6) {
				int			long_axis, short_axis;
				vec3_t		axis, tmp, len;
				mat3x3_t	m0, m1, m2, result;

				memcpy (&rb_outVertexArray, rb_inVertexArray, sizeof (rb_outVertexArray)); // FIXME

				quad[0] = (float *)(rb_outVertexArray + rb_inIndexArray[k+0]);
				quad[1] = (float *)(rb_outVertexArray + rb_inIndexArray[k+1]);
				quad[2] = (float *)(rb_outVertexArray + rb_inIndexArray[k+2]);

				for (j=2 ; j>=0 ; j--) {
					quad[3] = (float *)(rb_outVertexArray + rb_inIndexArray[k+3+j]);

					if (!VectorCompare (quad[3], quad[0])
					&& !VectorCompare (quad[3], quad[1])
					&& !VectorCompare (quad[3], quad[2])) {
						break;
					}
				}

				// Build a matrix were the longest axis of the billboard is the Y-Axis
				VectorSubtract (quad[1], quad[0], m0[0]);
				VectorSubtract (quad[2], quad[0], m0[1]);
				VectorSubtract (quad[2], quad[1], m0[2]);
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
				len[long_axis] = Q_RSqrt (len[long_axis]);
				VectorScale (m0[long_axis], len[long_axis], axis);

				if (DotProduct (m0[long_axis], m0[short_axis])) {
					VectorCopy (axis, m0[1]);
					if (axis[0] || axis[1])
						MakeNormalVectorsd (m0[1], m0[0], m0[2]);
					else
						MakeNormalVectorsd (m0[1], m0[2], m0[0]);
				}
				else {
					if (!len[short_axis])
						break;
					len[short_axis] = Q_RSqrt (len[short_axis]);
					VectorScale (m0[short_axis], len[short_axis], m0[0]);
					VectorCopy (axis, m0[1]);
					CrossProduct (m0[0], m0[1], m0[2]);
				}

				for (j=0 ; j<3 ; j++)
					rot_centre[j] = (quad[0][j] + quad[1][j] + quad[2][j] + quad[3][j]) * 0.25;

				if (rb_currentModel && rb_currentModel == r_refScene.worldModel) {
					VectorCopy (rot_centre, tv);
					VectorSubtract (r_refDef.viewOrigin, tv, tv);
				}
				else {
					VectorAdd (rb_currentEntity->origin, rot_centre, tv);
					VectorSubtract (r_refDef.viewOrigin, tv, tmp);
					Matrix3_TransformVector (rb_currentEntity->axis, tmp, tv);
				}

				// Filter any longest-axis-parts off the camera-direction
				deflect = -DotProduct (tv, axis);

				VectorMA (tv, deflect, axis, m1[2]);
				VectorNormalizeFastd (m1[2]);
				VectorCopy (axis, m1[1]);
				CrossProduct (m1[1], m1[2], m1[0]);

				Matrix3_Transpose (m1, m2);
				Matrix3_Multiply (m2, m0, result);

				for (j=0 ; j<4 ; j++) {
					VectorSubtract (quad[j], rot_centre, tv);
					Matrix3_TransformVector (result, tv, quad[j]);
					VectorAdd (rot_centre, quad[j], quad[j]);
				}
			}
			break;

		case DEFORMV_PROJECTION_SHADOW:
			break;

		case DEFORMV_AUTOPARTICLE:
			{
				mat3x3_t	m0, m1, m2, result;
				float		scale;

				if (rb_numIndexes % 6)
					break;

				memcpy (&rb_outVertexArray, rb_inVertexArray, sizeof (rb_outVertexArray)); // FIXME
				if (rb_currentModel && rb_currentModel == r_refScene.worldModel)
					Matrix4_Matrix3 (r_refScene.worldViewMatrix, m1);
				else
					Matrix4_Matrix3 (r_refScene.modelViewMatrix, m1);

				Matrix3_Transpose (m1, m2);

				for (k=0 ; k<rb_numIndexes ; k+=6) {
					quad[0] = (float *)(rb_outVertexArray + rb_inIndexArray[k+0]);
					quad[1] = (float *)(rb_outVertexArray + rb_inIndexArray[k+1]);
					quad[2] = (float *)(rb_outVertexArray + rb_inIndexArray[k+2]);

					for (j=2 ; j>=0 ; j--) {
						quad[3] = (float *)(rb_outVertexArray + rb_inIndexArray[k+3+j]);

						if (!VectorCompare (quad[3], quad[0])
						&& !VectorCompare (quad[3], quad[1])
						&& !VectorCompare (quad[3], quad[2])) {
							break;
						}
					}

					Matrix3_FromPoints (quad[0], quad[1], quad[2], m0);
					Matrix3_Multiply (m2, m0, result);

					// Hack a scale up to keep particles from disappearing
					scale = (quad[0][0] - r_refDef.viewOrigin[0]) * r_refDef.forwardVec[0] +
							(quad[0][1] - r_refDef.viewOrigin[1]) * r_refDef.forwardVec[1] +
							(quad[0][2] - r_refDef.viewOrigin[2]) * r_refDef.forwardVec[2];
					if (scale < 20)
						scale = 1.5;
					else
						scale = 1.5 + scale * 0.006f;

					for (j=0 ; j<3 ; j++)
						rot_centre[j] = (quad[0][j] + quad[1][j] + quad[2][j] + quad[3][j]) * 0.25;

					for (j=0 ; j<4 ; j++) {
						VectorSubtract (quad[j], rot_centre, tv);
						Matrix3_TransformVector (result, tv, quad[j]);
						VectorMA (rot_centre, scale, quad[j], quad[j]);
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
void RB_DrawElements (meshType_t meshType)
{
	if (!rb_numVerts || !rb_numIndexes)
		return;

	// Flush
	if (glConfig.extDrawRangeElements)
		qglDrawRangeElementsEXT (GL_TRIANGLES, 0, rb_numVerts, rb_numIndexes, GL_UNSIGNED_INT, rb_inIndexArray);
	else
		qglDrawElements (GL_TRIANGLES, rb_numIndexes, GL_UNSIGNED_INT, rb_inIndexArray);

	// Increment performance counters
	if (rb_triangleOutlines || !r_speeds->integer)
		return;

	r_refStats.numVerts += rb_numVerts;
	r_refStats.numTris += rb_numIndexes/3;
	r_refStats.numElements++;

	switch (meshType) {
	case MBT_Q2BSP:
	case MBT_Q3BSP:
		r_refStats.worldElements++;
		break;

	case MBT_ALIAS:
		r_refStats.aliasElements++;
		r_refStats.aliasPolys += rb_numIndexes/3;
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

	for (i=rb_numOldPasses ; i>rb_numPasses ; i--) {
		GL_SelectTexture (i-1);
		qglDisable (GL_TEXTURE_2D);
		qglDisable (GL_TEXTURE_GEN_S);
		qglDisable (GL_TEXTURE_GEN_T);
		qglDisable (GL_TEXTURE_GEN_R);
		qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
		if (glConfig.extTexCubeMap)
			qglDisable (GL_TEXTURE_CUBE_MAP_ARB);
	}

	rb_numOldPasses = rb_numPasses;
}


/*
=============
RB_BindShaderPass
=============
*/
static void RB_BindShaderPass (shaderPass_t *pass, image_t *image, texUnit_t texUnit)
{
	if (!image) {
		// Find the texture
		if (pass->flags & SHADER_PASS_LIGHTMAP)
			image = r_lmTextures[rb_currentLmTexNum];
		else if (pass->flags & SHADER_PASS_ANIMMAP)
			image = pass->animFrames[(int)(pass->animFPS * rb_shaderTime) % pass->animNumFrames];
		else
			image = pass->animFrames[0];

		if (!image)
			image = r_noTexture;
	}

	// Bind the texture
	GL_SelectTexture (texUnit);
	GL_BindTexture (image);
	qglEnable (GL_TEXTURE_2D);
	if (image->flags & IT_CUBEMAP) {
		qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
		qglEnable (GL_TEXTURE_CUBE_MAP_ARB);
	}
	else {
		qglEnableClientState (GL_TEXTURE_COORD_ARRAY);
		if (glConfig.extTexCubeMap)
			qglDisable (GL_TEXTURE_CUBE_MAP_ARB);
	}

	// Modify the texture coordinates
	RB_ModifyTextureCoords (pass, texUnit);
}


/*
=============
RB_SetupPassState
=============
*/
static void RB_SetupPassState (shaderPass_t *pass, qBool mTex)
{
	program_t	*program;

	// Vertex program
	if (glConfig.extVertexProgram) {
		if (pass->flags & SHADER_PASS_VERTEXPROGRAM) {
			program = pass->vertProg;

			qglEnable (GL_VERTEX_PROGRAM_ARB);
			R_BindProgram (program);

			qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 0, r_refDef.viewOrigin[0], r_refDef.viewOrigin[1], r_refDef.viewOrigin[2], 0);
			qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 1, r_refDef.forwardVec[0], r_refDef.forwardVec[1], r_refDef.forwardVec[2], 0);
			qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 2, r_refDef.rightVec[0], r_refDef.rightVec[1], r_refDef.rightVec[2], 0);
			qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 3, r_refDef.upVec[0], r_refDef.upVec[1], r_refDef.upVec[2], 0);
			qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 4, rb_currentEntity->origin[0], rb_currentEntity->origin[1], rb_currentEntity->origin[2], 0);
			qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 5, rb_currentEntity->axis[0][0], rb_currentEntity->axis[0][1], rb_currentEntity->axis[0][2], 0);
			qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 6, rb_currentEntity->axis[1][0], rb_currentEntity->axis[1][1], rb_currentEntity->axis[1][2], 0);
			qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 7, rb_currentEntity->axis[2][0], rb_currentEntity->axis[2][1], rb_currentEntity->axis[2][2], 0);
			qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 8, rb_shaderTime, 0, 0, 0);
		}
		else
			qglDisable (GL_VERTEX_PROGRAM_ARB);
	}

	// Fragment program
	if (glConfig.extFragmentProgram) {
		if (pass->flags & SHADER_PASS_FRAGMENTPROGRAM) {
			program = pass->fragProg;

			qglEnable (GL_FRAGMENT_PROGRAM_ARB);
			R_BindProgram (program);

			qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 0, r_refDef.viewOrigin[0], r_refDef.viewOrigin[1], r_refDef.viewOrigin[2], 0);
			qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 1, r_refDef.forwardVec[0], r_refDef.forwardVec[1], r_refDef.forwardVec[2], 0);
			qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 2, r_refDef.rightVec[0], r_refDef.rightVec[1], r_refDef.rightVec[2], 0);
			qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 3, r_refDef.upVec[0], r_refDef.upVec[1], r_refDef.upVec[2], 0);
			qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 4, rb_currentEntity->origin[0], rb_currentEntity->origin[1], rb_currentEntity->origin[2], 0);
			qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 5, rb_currentEntity->axis[0][0], rb_currentEntity->axis[0][1], rb_currentEntity->axis[0][2], 0);
			qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 6, rb_currentEntity->axis[1][0], rb_currentEntity->axis[1][1], rb_currentEntity->axis[1][2], 0);
			qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 7, rb_currentEntity->axis[2][0], rb_currentEntity->axis[2][1], rb_currentEntity->axis[2][2], 0);
			qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 8, rb_shaderTime, 0, 0, 0);
		}
		else
			qglDisable (GL_FRAGMENT_PROGRAM_ARB);
	}

	// Blending
	if (pass->flags & SHADER_PASS_BLEND) {
		qglEnable (GL_BLEND);
		GL_BlendFunc (pass->blendSource, pass->blendDest);
	}
	else if (rb_currentEntity->flags & RF_TRANSLUCENT) {
		qglEnable (GL_BLEND);
		GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else {
		if (pass->blendMode != GL_REPLACE && mTex)
			GL_BlendFunc (pass->blendSource, pass->blendDest);
		qglDisable (GL_BLEND);
	}

	// Alphafunc
	switch (pass->alphaFunc) {
	case ALPHA_FUNC_GT0:
		qglEnable (GL_ALPHA_TEST);
		qglAlphaFunc (GL_GREATER, 0);
		break;

	case ALPHA_FUNC_LT128:
		qglEnable (GL_ALPHA_TEST);
		qglAlphaFunc (GL_LESS, 0.5f);
		break;

	case ALPHA_FUNC_GE128:
		qglEnable (GL_ALPHA_TEST);
		qglAlphaFunc (GL_GEQUAL, 0.5f);
		break;

	case ALPHA_FUNC_NONE:
		qglDisable (GL_ALPHA_TEST);
		break;

	default:
		assert (0);
		break;
	}

	// Nasty hack!!!
	if (!glState.in2D) {
		qglDepthFunc (pass->depthFunc);
		if (pass->flags & SHADER_PASS_DEPTHWRITE && !(rb_currentEntity->flags & RF_TRANSLUCENT)) // FIXME
			qglDepthMask (GL_TRUE);
		else
			qglDepthMask (GL_FALSE);
	}

	// Mask colors
	if (pass->totalMask)
		qglColorMask (!pass->maskRed, !pass->maskGreen, !pass->maskBlue, !pass->maskAlpha);
	else
		qglColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}


/*
=============
RB_RenderGeneric
=============
*/
static void RB_RenderGeneric (void)
{
	shaderPass_t	*pass;

	pass = rb_accumPasses[0];

	RB_BindShaderPass (pass, NULL, 0);
	RB_SetupColor (pass);
	RB_SetupPassState (pass, qFalse);
	if (pass->blendMode == GL_REPLACE)
		GL_TextureEnv (GL_REPLACE);
	else
		GL_TextureEnv (GL_MODULATE);

	RB_DrawElements (rb_currentMeshType);
}


/*
=============
RB_RenderCombine
=============
*/
static void RB_RenderCombine (void)
{
	shaderPass_t	*pass;
	int				i;

	pass = rb_accumPasses[0];

	RB_BindShaderPass (pass, NULL, 0);
	RB_SetupColor (pass);
	RB_SetupPassState (pass, qTrue);
	GL_TextureEnv (GL_MODULATE);

	for (i=1 ; i<rb_numPasses ; i++) {
		pass = rb_accumPasses[i];
		RB_BindShaderPass (pass, NULL, i);

		switch (pass->blendMode) {
		case GL_REPLACE:
		case GL_MODULATE:
			GL_TextureEnv (GL_MODULATE);
			break;

		case GL_ADD:
			// These modes are best set with TexEnv, Combine4 would need much more setup
			GL_TextureEnv (GL_ADD);
			break;

		case GL_DECAL:
			// Mimics Alpha-Blending in upper texture stage, but instead of multiplying the alpha-channel, they´re added
			// this way it can be possible to use GL_DECAL in both texture-units, while still looking good
			// normal mutlitexturing would multiply the alpha-channel which looks ugly
			GL_TextureEnv (GL_COMBINE_ARB);
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
			GL_TextureEnv (GL_COMBINE4_NV);

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

	RB_DrawElements (rb_currentMeshType);
}


/*
=============
RB_RenderMTex
=============
*/
static void RB_RenderMTex (void)
{
	shaderPass_t	*pass;
	int				i;

	pass = rb_accumPasses[0];

	RB_BindShaderPass (pass, NULL, 0);
	RB_SetupColor (pass);
	RB_SetupPassState (pass, qTrue);
	GL_TextureEnv (GL_MODULATE);

	for (i=1 ; i<rb_numPasses ; i++) {
		pass = rb_accumPasses[i];
		RB_BindShaderPass (pass, NULL, i);
		GL_TextureEnv (pass->blendMode);
	}

	RB_DrawElements (rb_currentMeshType);
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
	RB_CleanUpTextureUnits ();

	if (rb_numPasses == 1)
		RB_RenderGeneric ();
	else if (glConfig.extTexEnvCombine)
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
static void RB_AccumulatePass (shaderPass_t *pass)
{
	shaderPass_t	*prevPass;
	qBool			accum;

	r_refStats.meshPasses++;

	if (!rb_numPasses) {
		rb_accumPasses[rb_numPasses++] = pass;
		return;
	}

	// Check bounds
	accum = (rb_numPasses < glConfig.maxTexUnits) && !(pass->flags & SHADER_PASS_DLIGHT);

	// Accumulate?
	if (accum) {
		// Compare against previous pass properties
		prevPass = rb_accumPasses[rb_numPasses - 1];

		if (prevPass->depthFunc != pass->depthFunc
		|| prevPass->totalMask != pass->totalMask
		|| pass->alphaFunc != ALPHA_FUNC_NONE
		|| pass->rgbGen.type != RGB_GEN_IDENTITY
		|| pass->alphaGen.type != ALPHA_GEN_IDENTITY
		|| (prevPass->alphaFunc != ALPHA_FUNC_NONE && pass->depthFunc != GL_EQUAL))
			accum = qFalse;

		// Check the blend modes
		if (accum) {
			if (!pass->blendMode) {
				accum = (prevPass->blendMode == GL_REPLACE) && glConfig.extNVTexEnvCombine4;
			}
			else switch (prevPass->blendMode) {
			case GL_REPLACE:
				if (glConfig.extTexEnvCombine)
					accum = (pass->blendMode == GL_ADD) ? glConfig.extTexEnvAdd : qTrue;
				else
					accum = (pass->blendMode == GL_ADD) ? glConfig.extTexEnvAdd : (pass->blendMode != GL_DECAL);
				break;

			case GL_ADD:
				accum = (pass->blendMode == GL_ADD) && glConfig.extTexEnvAdd;
				break;

			case GL_MODULATE:
				accum = (pass->blendMode == GL_MODULATE || pass->blendMode == GL_REPLACE);
				break;

			default:
				accum = qFalse;
				break;
			}
		}
	}

	if (!accum)
		RB_RenderAccumulatedPasses ();

	if (pass->flags & SHADER_PASS_DLIGHT) {
		RB_CleanUpTextureUnits ();
		R_Q3BSP_AddDynamicLights (rb_currentDLightBits, rb_currentEntity, pass->depthFunc, pass->blendSource, pass->blendDest);
	}
	else
		rb_accumPasses[rb_numPasses++] = pass;
}

/*
===============================================================================

	MESH RENDERING SUPPORTING FUNCTIONS

===============================================================================
*/

/*
=============
RB_SetupShaderState
=============
*/
static void RB_SetupShaderState (shader_t *shader)
{
	// Culling
	if (!gl_cull->integer || rb_currentMeshFeatures & MF_NOCULL) {
		qglDisable (GL_CULL_FACE);
	}
	else switch (shader->cullType) {
	case SHADER_CULL_FRONT:
		qglEnable (GL_CULL_FACE);
		qglCullFace (GL_FRONT);
		break;

	case SHADER_CULL_BACK:
		qglEnable (GL_CULL_FACE);
		qglCullFace (GL_BACK);
		break;

	case SHADER_CULL_NONE:
		qglDisable (GL_CULL_FACE);
		break;

	default:
		assert (0);
		break;
	}

	if (rb_triangleOutlines)
		return;

	// Polygon offset
	if (shader->flags & SHADER_POLYGONOFFSET) {
		qglEnable (GL_POLYGON_OFFSET_FILL);
		qglPolygonOffset (shader->offsetFactor, shader->offsetUnits);
	}
	else
		qglDisable (GL_POLYGON_OFFSET_FILL);

	// Depth range
	if (shader->flags & SHADER_DEPTHRANGE) {
		qglDepthRange (shader->depthNear, shader->depthFar);
	}
	else {
		if (rb_currentEntity->flags & RF_DEPTHHACK)
			qglDepthRange (0, 0.3f);
		else
			qglDepthRange (0, 1);
	}

	// Depth testing
	// FIXME: SHADER_DEPTHTEST option?
	if (shader->flags & SHADER_FLARE || glState.in2D)
		qglDisable (GL_DEPTH_TEST);
	else
		qglEnable (GL_DEPTH_TEST);
}


/*
=============
RB_SetOutlineColor
=============
*/
static inline void RB_SetOutlineColor (void)
{
	switch (rb_currentMeshType) {
	case MBT_2D:
	case MBT_ALIAS:
	case MBT_SP2:
	case MBT_SKY:
		qglColor4fv (colorRed);
		break;

	case MBT_Q2BSP:
	case MBT_Q3BSP:
		qglColor4fv (colorWhite);
		break;

	case MBT_POLY:
		qglColor4fv (colorGreen);
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
	switch (gl_showtris->integer) {
	case 1:
		qglColor4ub (255, 255, 255, 255);
		break;

	case 2:
		RB_SetOutlineColor ();
		break;
	}

	// Draw
	for (i=0 ; i<rb_numIndexes ; i+=3) {
		qglBegin (GL_LINE_STRIP);
		qglArrayElement (rb_inIndexArray[i]);
		qglArrayElement (rb_inIndexArray[i+1]);
		qglArrayElement (rb_inIndexArray[i+2]);
		qglArrayElement (rb_inIndexArray[i]);
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

	if (!rb_inNormalsArray)
		return;

	// Set color
	switch (gl_shownormals->integer) {
	case 1:
		qglColor4ub (255, 255, 255, 255);
		break;

	case 2:
		RB_SetOutlineColor ();
		break;
	}

	// Draw
	qglBegin (GL_LINES);
	for (i=0 ; i<rb_numVerts ; i++) {
		qglVertex3f(rb_inVertexArray[i][0],
					rb_inVertexArray[i][1],
					rb_inVertexArray[i][2]);
		qglVertex3f(rb_inVertexArray[i][0] + rb_inNormalsArray[i][0]*2,
					rb_inVertexArray[i][1] + rb_inNormalsArray[i][1]*2,
					rb_inVertexArray[i][2] + rb_inNormalsArray[i][2]*2);
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
	shaderPass_t	*pass;
	qBool			debugLightmap, addDlights;
	int				i;

	if (r_skipBackend->integer)
		return;

	// Collect mesh buffer values
	rb_currentMeshType = mb->sortKey & 7;
	rb_currentShader = mb->shader;
	if (mb->entity) {
		rb_currentEntity = mb->entity;
		rb_currentModel = mb->entity->model;
	}
	else {
		rb_currentEntity = &r_refScene.defaultEntity;
		rb_currentModel = NULL;
	}

	switch (rb_currentMeshType) {
	case MBT_Q2BSP:
		rb_currentLmTexNum = ((mBspSurface_t *)mb->mesh)->q2_lmTexNumActive;
		rb_patchWidth = rb_patchHeight = 0;

		surf = (mBspSurface_t *)mb->mesh;
		rb_currentDLightBits = 0; // Only used in Q3BSP
		addDlights = qFalse;
		break;

	case MBT_Q3BSP:
		rb_currentLmTexNum = ((mBspSurface_t *)mb->mesh)->lmTexNum;
		rb_patchWidth = ((mBspSurface_t *)mb->mesh)->q3_patchWidth;
		rb_patchHeight = ((mBspSurface_t *)mb->mesh)->q3_patchHeight;

		surf = (mBspSurface_t *)mb->mesh;
		if (!(rb_currentShader->flags & SHADER_FLARE) && surf->dLightFrame == r_refScene.frameCount) {
			rb_currentDLightBits = surf->dLightBits;
			addDlights = qTrue;
		}
		else {
			rb_currentDLightBits = 0;
			addDlights = qFalse;
		}
		break;

	default:
		rb_currentLmTexNum = -1;
		rb_patchWidth = rb_patchHeight = 0;
		rb_currentDLightBits = 0;
		addDlights = qFalse;
		break;
	}

	// Set time
	if (glState.in2D)
		rb_shaderTime = Sys_Milliseconds () * 0.001f;
	else
		rb_shaderTime = r_refDef.time;
	rb_shaderTime -= mb->shaderTime;
	if (rb_shaderTime < 0)
		rb_shaderTime = 0;

	// State
	RB_SetupShaderState (rb_currentShader);

	// Setup vertices
	if (rb_currentShader->numDeforms) {
		RB_DeformVertices ();
		qglVertexPointer (3, GL_FLOAT, 0, rb_outVertexArray);
	}
	else {
		qglVertexPointer (3, GL_FLOAT, 0, rb_inVertexArray);
	}

	if (!rb_numIndexes || shadowPass)
		return;

	RB_LockArrays (rb_numVerts);

	// Render outlines if desired
	if (rb_triangleOutlines) {
		if (gl_showtris->integer)
			RB_ShowTriangles ();
		if (gl_shownormals->integer)
			RB_ShowNormals ();

		RB_UnlockArrays ();
		RB_ResetPointers ();
		return;
	}

	// Set fog shaders
	if (mb->fog && mb->fog->shader) {
		if ((rb_currentShader->sortKey <= SHADER_SORT_PARTICLE+1 && rb_currentShader->flags & (SHADER_DEPTHWRITE|SHADER_SKY)) || rb_currentShader->fogDist) {
			rb_texFog = mb->fog;
			rb_colorFog = NULL;
		}
		else {
			rb_texFog = NULL;
			rb_colorFog = mb->fog;
		}
	}
	else {
		rb_texFog = NULL;
		rb_colorFog = NULL;
	}

	// Accumulate passes and render
	debugLightmap = qFalse;
	for (i=0, pass=mb->shader->passes ; i<mb->shader->numPasses ; pass++, i++) {
		if (pass->flags & SHADER_PASS_LIGHTMAP) {
			if (rb_currentLmTexNum < 0)
				continue;
			debugLightmap = (gl_lightmap->integer);
		}
		else if (!pass->animNumFrames)
			continue;
		if (r_detailTextures->integer) {
			if (pass->flags & SHADER_PASS_NOTDETAIL)
				continue;
		}
		else if (pass->flags & SHADER_PASS_DETAIL)
			continue;

		// Accumulate
		RB_AccumulatePass (pass);

		if (pass->flags & SHADER_PASS_LIGHTMAP && rb_currentDLightBits) {
			RB_AccumulatePass (&rb_dLightPass);
			addDlights = qFalse;
		}
	}

	if (debugLightmap) {
		// Accumulate a lightmap pass for debugging purposes
		RB_AccumulatePass (&rb_lightMapPass);
	}
	else if (rb_texFog && rb_texFog->shader) {
		// Accumulate fog
		rb_fogPass.animFrames[0] = r_fogTexture;
		if (!mb->shader->numPasses || rb_currentShader->fogDist || rb_currentShader->flags & SHADER_SKY)
			rb_fogPass.depthFunc = GL_LEQUAL;
		else
			rb_fogPass.depthFunc = GL_EQUAL;
		RB_AccumulatePass (&rb_fogPass);
	}

	if (rb_numPasses)
		RB_RenderAccumulatedPasses ();

	// Render dlights
	if (addDlights || debugLightmap) {
		RB_CleanUpTextureUnits ();
		R_Q3BSP_AddDynamicLights (rb_currentDLightBits, rb_currentEntity, GL_EQUAL, GL_DST_COLOR, GL_ONE);
	}

	// Reset
	RB_UnlockArrays ();

	GL_LoadIdentityTexMatrix ();
	qglMatrixMode (GL_MODELVIEW);

	if (!rb_triangleOutlines)
		RB_ResetPointers ();

	r_refStats.meshCount++;
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

	for (i=glConfig.maxTexUnits-1 ; i>0 ; i--) {
		GL_SelectTexture (i);
		qglDisable (GL_TEXTURE_2D);
		qglDisable (GL_TEXTURE_GEN_S);
		qglDisable (GL_TEXTURE_GEN_T);
		qglDisable (GL_TEXTURE_GEN_R);
		qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
		if (glConfig.extTexCubeMap)
			qglDisable (GL_TEXTURE_CUBE_MAP_ARB);
	}

	GL_SelectTexture (0);
	qglEnable (GL_TEXTURE_2D);
	qglDisable (GL_TEXTURE_GEN_S);
	qglDisable (GL_TEXTURE_GEN_T);
	qglDisable (GL_TEXTURE_GEN_R);
	qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
	if (glConfig.extTexCubeMap)
		qglDisable (GL_TEXTURE_CUBE_MAP_ARB);

	qglDisableClientState (GL_NORMAL_ARRAY);
	qglDisableClientState (GL_COLOR_ARRAY);

	qglColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	qglDisable (GL_ALPHA_TEST);

	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDisable (GL_BLEND);

	qglDepthFunc (GL_LEQUAL);
	qglDepthMask (GL_TRUE);

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

	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	qglDepthMask (GL_FALSE);
	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_TEXTURE_2D);
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

	qglEnable (GL_TEXTURE_2D);
	qglEnable (GL_DEPTH_TEST);
	qglDepthMask (GL_TRUE);
}


/*
=============
RB_BeginFrame

Does any pre-frame backend actions
=============
*/
void RB_BeginFrame (void)
{
	static int prevupdate;
	static int rupdate = 300;

	rb_numVerts = 0;
	rb_numIndexes = 0;

	if (prevupdate > (Sys_Milliseconds () % rupdate)) {
		int i, j, k;
		float t;

		j = rand() * (FTABLE_SIZE/4);
		k = rand() * (FTABLE_SIZE/2);

		for (i=0 ; i<FTABLE_SIZE ; i++) {
			if (i >= j && i < (j+k)) {
				t = (double)((i-j)) / (double)(k);
				rb_noiseTable[i] = R_FastSin (t + 0.25);
			}
			else {
				rb_noiseTable[i] = 1;
			}
		}

		rupdate = 300 + rand() % 300;
	}
	prevupdate = (Sys_Milliseconds () % rupdate);
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
void RB_Init (void)
{
	int		i;
	double	t;
	index_t	*index;

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

	// Generate trifan indices
	index = rb_triFanIndices;
	for (i=0 ; i<VERTARRAY_MAX_VERTS-2 ; i++) {
		index[0] = 0;
		index[1] = i + 1;
		index[2] = i + 2;

		index += 3;
	}

	// Identity lighting
	rb_identityLighting = glState.invIntens * 255;
	if (rb_identityLighting > 255)
		rb_identityLighting = 255;

	// Quake3 BSP specific dynamic light pass
	memset (&rb_dLightPass, 0, sizeof (shaderPass_t));
	rb_dLightPass.flags = SHADER_PASS_DLIGHT|SHADER_PASS_BLEND;
	rb_dLightPass.depthFunc = GL_EQUAL;
	rb_dLightPass.blendSource = GL_DST_COLOR;
	rb_dLightPass.blendDest = GL_ONE;

	// Create the fog pass
	memset (&rb_fogPass, 0, sizeof (shaderPass_t));
	rb_fogPass.flags = SHADER_PASS_BLEND|SHADER_PASS_NOCOLORARRAY;
	rb_fogPass.tcGen = TC_GEN_FOG;
	rb_fogPass.blendMode = GL_DECAL;
	rb_fogPass.blendSource = GL_SRC_ALPHA;
	rb_fogPass.blendDest = GL_ONE_MINUS_SRC_ALPHA;
	rb_fogPass.rgbGen.type = RGB_GEN_FOG;
	rb_fogPass.alphaGen.type = ALPHA_GEN_FOG;

	// Togglable solid lightmap overlay
	memset (&rb_lightMapPass, 0, sizeof (shaderPass_t));
	rb_lightMapPass.flags = SHADER_PASS_LIGHTMAP|SHADER_PASS_NOCOLORARRAY;
	rb_lightMapPass.tcGen = TC_GEN_LIGHTMAP;
	rb_lightMapPass.depthFunc = GL_EQUAL;
	rb_lightMapPass.blendMode = GL_REPLACE;
	rb_lightMapPass.rgbGen.type = RGB_GEN_IDENTITY;
	rb_lightMapPass.alphaGen.type = ALPHA_GEN_IDENTITY;

	// Find rendering paths
	switch (glStatic.renderClass) {
	case REND_CLASS_INTEL:
	case REND_CLASS_S3:
	case REND_CLASS_SIS:
		// These models are known for not supporting texture matrices properly/at all
		RB_ModifyTextureCoords = RB_ModifyTextureCoordsGeneric;
		break;

	default:
		RB_ModifyTextureCoords = RB_ModifyTextureCoordsMatrix;
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
