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
static void				(*RB_ModifyTextureCoords) (shaderPass_t *pass, int texUnit);

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
static qBool			rb_enableNormals;
static qBool			rb_triangleOutlines;

static int				rb_currentLmTexNum;
static int				rb_currentMeshFeatures;
static int				rb_currentMeshType;
static shader_t			*rb_currentShader;
static entity_t			*rb_currentEntity;
static model_t			*rb_currentModel;

static int				rb_numColors;
static int				rb_numPasses;
static float			rb_shaderTime;

static int				rb_identityLighting;
static shaderPass_t		rb_lightMapPass;

#define FTABLE_SIZE		1024
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
static inline float R_FastSin (float t)
{
	return FTABLE_EVALUATE(rb_sinTable, t);
}


/*
==============
R_TableForFunc
==============
*/
static float *R_TableForFunc (uInt func)
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
	rb_enableNormals = qFalse;
	rb_arraysLocked = qFalse;

	rb_currentLmTexNum = -1;
	rb_currentMeshFeatures = 0;
	rb_currentMeshType = 0;
	rb_currentShader = NULL;
	rb_currentEntity = NULL;
	rb_currentModel = NULL;

	rb_numColors = 0;
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
RB_SetupColor
=============
*/
static void RB_SetupColor (const shaderPass_t *pass)
{
	int		r, g, b;
	float	*table, c, a;
	byte	*bArray, *inArray;
	vec3_t	t, v;
	int		i;
	const shaderFunc_t *rgbGenFunc, *alphaGenFunc;

	rgbGenFunc = &pass->rgbGen.func;
	alphaGenFunc = &pass->alphaGen.func;

	// Optimal case
	if (pass->flags & SHADER_PASS_NOCOLORARRAY)
		rb_numColors = 1;
	else
		rb_numColors = rb_numVerts;

	// Color generation
	bArray = rb_outColorArray[0];
	inArray = rb_inColorArray[0];
	switch (pass->rgbGen.type) {
	case RGB_GEN_IDENTITY:
		if (rb_numColors == 1) {
			bArray[0] = 255;
			bArray[1] = 255;
			bArray[2] = 255;
		}
		else {
			memset (bArray, 255, sizeof (bvec4_t) * rb_numColors);
		}
		break;

	case RGB_GEN_IDENTITY_LIGHTING:
		if (rb_numColors == 1) {
			bArray[0] = rb_identityLighting;
			bArray[1] = rb_identityLighting;
			bArray[2] = rb_identityLighting;
		}
		else {
			memset (bArray, rb_identityLighting, sizeof (bvec4_t) * rb_numColors);
		}
		break;

	case RGB_GEN_CONST:
		r = FloatToByte (pass->rgbGen.args[0]);
		g = FloatToByte (pass->rgbGen.args[1]);
		b = FloatToByte (pass->rgbGen.args[2]);

		for (i=0 ; i<rb_numColors ; i++, bArray += 4) {
			bArray[0] = r;
			bArray[1] = g;
			bArray[2] = b;
		}
		break;

	case RGB_GEN_COLORWAVE:
		table = R_TableForFunc (rgbGenFunc->type);
		c = rb_shaderTime * rgbGenFunc->args[3] + rgbGenFunc->args[2];
		c = FTABLE_EVALUATE(table, c) * rgbGenFunc->args[1] + rgbGenFunc->args[0];
		a = pass->rgbGen.args[0] * c; r = FloatToByte (bound (0, a, 1));
		a = pass->rgbGen.args[1] * c; g = FloatToByte (bound (0, a, 1));
		a = pass->rgbGen.args[2] * c; b = FloatToByte (bound (0, a, 1));

		for (i=0 ; i<rb_numColors ; i++, bArray += 4) {
			bArray[0] = r;
			bArray[1] = g;
			bArray[2] = b;
		}
		break;

	case RGB_GEN_ENTITY:
		r = *(int *)rb_currentEntity->color;
		for (i=0 ; i<rb_numColors ; i++, bArray += 4)
			*(int *)bArray = r;
		break;

	case RGB_GEN_ONE_MINUS_ENTITY:
		for (i=0 ; i<rb_numColors ; i++, bArray += 4) {
			bArray[0] = 255 - rb_currentEntity->color[0];
			bArray[1] = 255 - rb_currentEntity->color[1];
			bArray[2] = 255 - rb_currentEntity->color[2];
		}
		break;

	case RGB_GEN_VERTEX:
		if (intensity->integer > 0) {
			for (i=0 ; i<rb_numColors ; i++, bArray += 4, inArray += 4) {
				bArray[0] = inArray[0] >> (intensity->integer / 2);
				bArray[1] = inArray[1] >> (intensity->integer / 2);
				bArray[2] = inArray[2] >> (intensity->integer / 2);
			}
			break;
		}

	// FALL THROUGH
	case RGB_GEN_EXACT_VERTEX:
		if (rb_numColors == 1) {
			bArray[0] = inArray[0];
			bArray[1] = inArray[1];
			bArray[2] = inArray[2];
		}
		else {
			memcpy (bArray, inArray, sizeof (bvec4_t) * rb_numColors);
		}
		break;

	case RGB_GEN_ONE_MINUS_VERTEX:
		if (intensity->integer > 0) {
			for (i=0 ; i<rb_numColors ; i++, bArray += 4, inArray += 4) {
				bArray[0] = 255 - (inArray[0] >> (intensity->integer / 2));
				bArray[1] = 255 - (inArray[1] >> (intensity->integer / 2));
				bArray[2] = 255 - (inArray[2] >> (intensity->integer / 2));
			}
			break;
		}

	// FALL THROUGH
	case RGB_GEN_ONE_MINUS_EXACT_VERTEX:
		for (i=0 ; i<rb_numColors ; i++, bArray += 4, inArray += 4) {
			bArray[0] = 255 - inArray[0];
			bArray[1] = 255 - inArray[1];
			bArray[2] = 255 - inArray[2];
		}
		break;

	case RGB_GEN_LIGHTING_DIFFUSE:
		if (rb_currentEntity)
			R_LightForEntity (rb_currentEntity, rb_numColors, bArray);
		else {
			if (rb_numColors == 1) {
				bArray[0] = 255;
				bArray[1] = 255;
				bArray[2] = 255;
			}
			else {
				memset (bArray, 255, sizeof (bvec4_t) * rb_numColors);
			}
		}
		break;
	}

	// Alpha generation
	bArray = rb_outColorArray[0];
	inArray = rb_inColorArray[0];
	switch (pass->alphaGen.type) {
	case ALPHA_GEN_IDENTITY:
		for (i=0 ; i<rb_numColors ; i++, bArray += 4) {
			bArray[3] = 255;
		}
		break;

	case ALPHA_GEN_CONST:
		b = FloatToByte (pass->alphaGen.args[0]);
		for (i=0 ; i<rb_numColors ; i++, bArray += 4)
			bArray[3] = b;
		break;

	case ALPHA_GEN_WAVE:
		table = R_TableForFunc (alphaGenFunc->type);
		a = alphaGenFunc->args[2] + rb_shaderTime * alphaGenFunc->args[3];
		a = FTABLE_EVALUATE(table, a) * alphaGenFunc->args[1] + alphaGenFunc->args[0];
		b = FloatToByte (bound (0.0f, a, 1.0f));
		for (i=0 ; i<rb_numColors ; i++, bArray += 4)
			bArray[3] = b;
		break;

	case ALPHA_GEN_PORTAL:
		VectorAdd (rb_inVertexArray[0], rb_currentEntity->origin, v);
		VectorSubtract (r_refDef.viewOrigin, v, t);
		a = VectorLength (t) * pass->alphaGen.args[0];
		clamp (a, 0.0f, 1.0f);
		b = FloatToByte (a);

		for (i=0 ; i<rb_numColors ; i++, bArray += 4)
			bArray[3] = b;
		break;

	case ALPHA_GEN_VERTEX:
		for (i=0 ; i<rb_numColors; i++, bArray += 4, inArray += 4)
			bArray[3] = inArray[3];
		break;

	case ALPHA_GEN_ENTITY:
		for (i=0 ; i<rb_numColors; i++, bArray += 4)
			bArray[3] = rb_currentEntity->color[3];
		break;

	case ALPHA_GEN_SPECULAR:
		VectorSubtract (r_refDef.viewOrigin, rb_currentEntity->origin, t);
		if (!Matrix_Compare (rb_currentEntity->axis, axisIdentity))
			Matrix_TransformVector (rb_currentEntity->axis, t, v );
		else
			VectorCopy (t, v);

		for (i=0 ; i<rb_numColors ; i++, bArray += 4) {
			VectorSubtract (v, rb_inVertexArray[i], t);
			a = DotProduct (t, rb_inNormalsArray[i]) * Q_RSqrt (DotProduct (t, t));
			a = a * a * a * a * a;
			bArray[3] = FloatToByte (bound (0.0f, a, 1.0f));
		}
		break;

	case ALPHA_GEN_DOT:
		if (!Matrix_Compare (rb_currentEntity->axis, axisIdentity))
			Matrix_TransformVector (rb_currentEntity->axis, r_forwardVec, v);
		else
			VectorCopy (r_forwardVec, v);

		for (i=0 ; i<rb_numColors ; i++, bArray += 4) {
			a = DotProduct (v, rb_inNormalsArray[i]); if (a < 0) a = -a;
			bArray[3] = FloatToByte (bound (pass->alphaGen.args[0], a, pass->alphaGen.args[1]));
		}
		break;

	case ALPHA_GEN_ONE_MINUS_DOT:
		if (!Matrix_Compare (rb_currentEntity->axis, axisIdentity))
			Matrix_TransformVector (rb_currentEntity->axis, r_forwardVec, v);
		else
			VectorCopy (r_forwardVec, v);

		for (i=0 ; i<rb_numColors ; i++, bArray += 4) {
			a = DotProduct (v, rb_inNormalsArray[i]); if (a < 0) a = -a; a = 1.0f - a;
			bArray[3] = FloatToByte (bound (pass->alphaGen.args[0], a, pass->alphaGen.args[1]));
		}
		break;
	}
}


/*
=============
RB_ModifyTextureCoordsGeneric

Standard path
=============
*/
static const float r_warpSinTable[] = {
	0.000000f,	  0.098165f,	  0.196270f,	  0.294259f,	  0.392069f,	  0.489643f,	  0.586920f,	  0.683850f,
	0.780360f,	  0.876405f,	  0.971920f,	  1.066850f,	  1.161140f,	  1.254725f,	  1.347560f,	  1.439580f,
	1.530735f,	  1.620965f,	  1.710220f,	  1.798445f,	  1.885585f,	  1.971595f,	  2.056410f,	  2.139990f,
	2.222280f,	  2.303235f,	  2.382795f,	  2.460925f,	  2.537575f,	  2.612690f,	  2.686235f,	  2.758160f,
	2.828425f,	  2.896990f,	  2.963805f,	  3.028835f,	  3.092040f,	  3.153385f,	  3.212830f,	  3.270340f,
	3.325880f,	  3.379415f,	  3.430915f,	  3.480350f,	  3.527685f,	  3.572895f,	  3.615955f,	  3.656840f,
	3.695520f,	  3.731970f,	  3.766175f,	  3.798115f,	  3.827760f,	  3.855105f,	  3.880125f,	  3.902810f,
	3.923140f,	  3.941110f,	  3.956705f,	  3.969920f,	  3.980740f,	  3.989160f,	  3.995180f,	  3.998795f,
	4.000000f,	  3.998795f,	  3.995180f,	  3.989160f,	  3.980740f,	  3.969920f,	  3.956705f,	  3.941110f,
	3.923140f,	  3.902810f,	  3.880125f,	  3.855105f,	  3.827760f,	  3.798115f,	  3.766175f,	  3.731970f,
	3.695520f,	  3.656840f,	  3.615955f,	  3.572895f,	  3.527685f,	  3.480350f,	  3.430915f,	  3.379415f,
	3.325880f,	  3.270340f,	  3.212830f,	  3.153385f,	  3.092040f,	  3.028835f,	  2.963805f,	  2.896990f,
	2.828425f,	  2.758160f,	  2.686235f,	  2.612690f,	  2.537575f,	  2.460925f,	  2.382795f,	  2.303235f,
	2.222280f,	  2.139990f,	  2.056410f,	  1.971595f,	  1.885585f,	  1.798445f,	  1.710220f,	  1.620965f,
	1.530735f,	  1.439580f,	  1.347560f,	  1.254725f,	  1.161140f,	  1.066850f,	  0.971920f,	  0.876405f,
	0.780360f,	  0.683850f,	  0.586920f,	  0.489643f,	  0.392069f,	  0.294259f,	  0.196270f,	  0.098165f,
	0.000000f,	  -0.098165f,	 -0.196270f,	 -0.294259f,	 -0.392069f,	 -0.489643f,	 -0.586920f,	 -0.683850f,
	-0.780360f,	 -0.876405f,	 -0.971920f,	 -1.066850f,	 -1.161140f,	 -1.254725f,	 -1.347560f,	 -1.439580f,
	-1.530735f,	 -1.620965f,	 -1.710220f,	 -1.798445f,	 -1.885585f,	 -1.971595f,	 -2.056410f,	 -2.139990f,
	-2.222280f,	 -2.303235f,	 -2.382795f,	 -2.460925f,	 -2.537575f,	 -2.612690f,	 -2.686235f,	 -2.758160f,
	-2.828425f,	 -2.896990f,	 -2.963805f,	 -3.028835f,	 -3.092040f,	 -3.153385f,	 -3.212830f,	 -3.270340f,
	-3.325880f,	 -3.379415f,	 -3.430915f,	 -3.480350f,	 -3.527685f,	 -3.572895f,	 -3.615955f,	 -3.656840f,
	-3.695520f,	 -3.731970f,	 -3.766175f,	 -3.798115f,	 -3.827760f,	 -3.855105f,	 -3.880125f,	 -3.902810f,
	-3.923140f,	 -3.941110f,	 -3.956705f,	 -3.969920f,	 -3.980740f,	 -3.989160f,	 -3.995180f,	 -3.998795f,
	-4.000000f,	 -3.998795f,	 -3.995180f,	 -3.989160f,	 -3.980740f,	 -3.969920f,	 -3.956705f,	 -3.941110f,
	-3.923140f,	 -3.902810f,	 -3.880125f,	 -3.855105f,	 -3.827760f,	 -3.798115f,	 -3.766175f,	 -3.731970f,
	-3.695520f,	 -3.656840f,	 -3.615955f,	 -3.572895f,	 -3.527685f,	 -3.480350f,	 -3.430915f,	 -3.379415f,
	-3.325880f,	 -3.270340f,	 -3.212830f,	 -3.153385f,	 -3.092040f,	 -3.028835f,	 -2.963805f,	 -2.896990f,
	-2.828425f,	 -2.758160f,	 -2.686235f,	 -2.612690f,	 -2.537575f,	 -2.460925f,	 -2.382795f,	 -2.303235f,
	-2.222280f,	 -2.139990f,	 -2.056410f,	 -1.971595f,	 -1.885585f,	 -1.798445f,	 -1.710220f,	 -1.620965f,
	-1.530735f,	 -1.439580f,	 -1.347560f,	 -1.254725f,	 -1.161140f,	 -1.066850f,	 -0.971920f,	 -0.876405f,
	-0.780360f,	 -0.683850f,	 -0.586920f,	 -0.489643f,	 -0.392069f,	 -0.294259f,	 -0.196270f,	 -0.098165f
};
static void RB_VertexTCBaseGeneric (shaderPass_t *pass, int texUnit)
{
	float   *outCoords, depth;
	vec3_t  transform;
	vec3_t  n, projection;
	vec3_t  inverseAxis[3];
	int			 i;

	outCoords = rb_outCoordArray[texUnit][0];

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
			if (rb_currentModel == r_worldModel || rb_currentModel->type == MODEL_BSP) {
				VectorSubtract (vec3Origin, r_refDef.viewOrigin, transform);
			}
			else {
				VectorSubtract (rb_currentEntity->origin, r_refDef.viewOrigin, transform);
				Matrix_Transpose (rb_currentEntity->axis, inverseAxis);
			}
		}
		else {
			VectorClear (transform);
			Matrix_Transpose (axisIdentity, inverseAxis);
		}

		for (i=0 ; i<rb_numVerts ; i++, outCoords += 2) {
			VectorAdd (rb_inVertexArray[i], transform, projection);
			VectorNormalizeFast (projection);

			// Project vector
			if (rb_currentModel == r_worldModel || rb_currentModel->type == MODEL_BSP)
				VectorCopy (rb_inNormalsArray[i], n);
			else
				Matrix_TransformVector (inverseAxis, rb_inNormalsArray[i], n);

			depth = -2.0f * DotProduct (n, projection);
			VectorMA (projection, depth, n, projection);
			depth = Q_FastSqrt (DotProduct (projection, projection) * 4);

			outCoords[0] = -((projection[1] * depth) + 0.5f);
			outCoords[1] =  ((projection[2] * depth) + 0.5f);
		}
		break;

	case TC_GEN_VECTOR:
		for (i=0 ; i<rb_numVerts ; i++, outCoords += 2) {
			outCoords[0] = DotProduct (pass->tcGenVec[0], rb_inVertexArray[i]) + pass->tcGenVec[0][3];
			outCoords[1] = DotProduct (pass->tcGenVec[1], rb_inVertexArray[i]) + pass->tcGenVec[1][3];
		}
		break;

	case TC_GEN_REFLECTION:
		rb_enableNormals = qTrue;
		qglEnable (GL_TEXTURE_GEN_S);
		qglEnable (GL_TEXTURE_GEN_T);
		qglEnable (GL_TEXTURE_GEN_R);
		qglTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
		qglTexGeni (GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
		qglTexGeni (GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
		break;


	case TC_GEN_WARP:
		for (i=0 ; i<rb_numVerts ; i++, outCoords += 2) {
			outCoords[0] = rb_inCoordArray[i][0] + (r_warpSinTable[Q_ftol (((rb_inCoordArray[i][1]*8.0f + rb_shaderTime) * (256.0f / (M_PI * 2.0f)))) & 255] * (1.0/64));
			outCoords[1] = rb_inCoordArray[i][1] + (r_warpSinTable[Q_ftol (((rb_inCoordArray[i][0]*8.0f + rb_shaderTime) * (256.0f / (M_PI * 2.0f)))) & 255] * (1.0/64));
		}
		break;
	}

	qglTexCoordPointer (2, GL_FLOAT, 0, outCoords);
}
static void RB_ModifyTextureCoordsGeneric (shaderPass_t *pass, int texUnit)
{
	int		 i, j;
	float   *table;
	float   t1, t2, sint, cost;
	float   *tcArray;
	tcMod_t *tcMod;

	RB_VertexTCBaseGeneric (pass, texUnit);

	for (i=0, tcMod=pass->tcMods ; i<pass->numTCMods ; tcMod++, i++) {
		tcArray = rb_outCoordArray[texUnit][0];

		switch (tcMod->type) {
		case TC_MOD_ROTATE:
			cost = tcMod->args[0] * rb_shaderTime;
			sint = R_FastSin (cost);
			cost = R_FastSin (cost + 0.25);

			for (j=0 ; j<rb_numVerts ; j++, tcArray += 2) {
				t1 = tcArray[0];
				t2 = tcArray[1];
				tcArray[0] = cost * (t1 - 0.5f) - sint * (t2 - 0.5f) + 0.5f;
				tcArray[1] = cost * (t2 - 0.5f) + sint * (t1 - 0.5f) + 0.5f;
			}
			break;

		case TC_MOD_SCALE:
			t1 = tcMod->args[0];
			t2 = tcMod->args[1];

			for (j=0 ; j<rb_numVerts ; j++, tcArray += 2) {
				tcArray[0] = tcArray[0] * t1;
				tcArray[1] = tcArray[1] * t2;
			}
			break;

		case TC_MOD_TURB:
			t1 = tcMod->args[1];
			t2 = tcMod->args[2] + rb_shaderTime * tcMod->args[3];

			for (j=0 ; j<rb_numVerts ; j++, tcArray += 2) {
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

			for (j=0 ; j<rb_numVerts ; j++, tcArray += 2) {
				tcArray[0] = tcArray[0] * t1 + t2;
				tcArray[1] = tcArray[1] * t1 + t2;
			}
			break;

		case TC_MOD_SCROLL:
			t1 = tcMod->args[0] * rb_shaderTime; t1 = t1 - floor (t1);
			t2 = tcMod->args[1] * rb_shaderTime; t2 = t2 - floor (t2);

			for (j=0 ; j<rb_numVerts ; j++, tcArray += 2) {
				tcArray[0] = tcArray[0] + t1;
				tcArray[1] = tcArray[1] + t2;
			}
			break;

		case TC_MOD_TRANSFORM:
			for (j=0 ; j<rb_numVerts ; j++, tcArray += 2) {
				t1 = tcArray[0];
				t2 = tcArray[1];
				tcArray[0] = t1 * tcMod->args[0] + t2 * tcMod->args[2] + tcMod->args[4];
				tcArray[1] = t2 * tcMod->args[1] + t1 * tcMod->args[3] + tcMod->args[5];
			}
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
	mat4x4_t m1, m2;
	float   t1, t2, sint, cost;
	float   *table;
	tcMod_t *tcMod;
	int	     i;

	for (i=0, tcMod=pass->tcMods ; i<pass->numTCMods ; tcMod++, i++) {
		switch (tcMod->type) {
		case TC_MOD_ROTATE:
			cost = tcMod->args[0] * rb_shaderTime;
			sint = R_FastSin (cost);
			cost = R_FastSin (cost + 0.25);
			m2[0] = 1, m2[1] = 0, m2[4] = 0, m2[5] = 1, m2[12] = -0.5f, m2[13] = -0.5f;
			Matrix4_Copy (result, m1);
			Matrix4_MultiplyFast2 (m2, m1, result);
			m2[0] = cost, m2[1] = sint, m2[4] = -sint, m2[5] = cost, m2[12] = 0, m2[13] = 0;
			Matrix4_Copy (result, m1);
			Matrix4_MultiplyFast2 (m2, m1, result);
			m2[0] = 1, m2[1] = 0, m2[4] = 0, m2[5] = 1, m2[12] = 0.5f, m2[13] = 0.5f;
			Matrix4_Copy (result, m1);
			Matrix4_MultiplyFast2 (m2, m1, result);
			break;

		case TC_MOD_SCALE:
			m2[0] = tcMod->args[0], m2[1] = 0, m2[4] = 0, m2[5] = tcMod->args[1], m2[12] = 0, m2[13] = 0;
			Matrix4_Copy (result, m1);
			Matrix4_MultiplyFast2 (m2, m1, result);
			break;

		case TC_MOD_TURB:
			t1 = (1.0 / 4.0);
			t2 = tcMod->args[2] + rb_shaderTime * tcMod->args[3];
			m2[0] = 1 + (tcMod->args[1] * R_FastSin (t2) + tcMod->args[0]) * t1, m2[1] = 0, m2[4] = 0, 
				m2[5] = 1 + (tcMod->args[1] * R_FastSin (t2 + 0.25) + tcMod->args[0]) * t1, m2[12] = 0, m2[13] = 0;
			Matrix4_Copy (result, m1);
			Matrix4_MultiplyFast2 (m2, m1, result);
			break;

		case TC_MOD_STRETCH:
			table = R_TableForFunc (tcMod->args[0]);
			t2 = tcMod->args[3] + rb_shaderTime * tcMod->args[4];
			t1 = FTABLE_EVALUATE (table, t2) * tcMod->args[2] + tcMod->args[1];
			t1 = t1 ? 1.0f / t1 : 1.0f;
			t2 = 0.5f - 0.5f * t1;
			m2[0] = t1, m2[1] = 0, m2[4] = 0, m2[5] = t1, m2[12] = t2, m2[13] = t2;
			Matrix4_Copy (result, m1);
			Matrix4_MultiplyFast2 (m2, m1, result);
			break;

		case TC_MOD_SCROLL:
			t1 = tcMod->args[0] * rb_shaderTime;
			t2 = tcMod->args[1] * rb_shaderTime;
			m2[0] = 1, m2[1] = 0, m2[4] = 0, m2[5] = 1, m2[12] = t1 - floor ( t1 ), m2[13] = t2 - floor (t2);
			Matrix4_Copy (result, m1);
			Matrix4_MultiplyFast2 (m2, m1, result);
			break;

		case TC_MOD_TRANSFORM:
			m2[0] = tcMod->args[0], m2[1] = tcMod->args[3], m2[4] = tcMod->args[2], 
				m2[5] = tcMod->args[1], m2[12] = tcMod->args[4], m2[13] = tcMod->args[5];
			Matrix4_Copy (result, m1);
			Matrix4_MultiplyFast2 (m2, m1, result);
			break;
		}
	}
}
static qBool RB_VertexTCBaseMatrix (shaderPass_t *pass, int texUnit, mat4x4_t matrix)
{
	vec3_t  transform;
	vec3_t  n, projection;
	vec3_t  inverseAxis[3];
	float   depth;
	int	     i;

	Matrix4_Identity (matrix);

	// tcGen
	switch (pass->tcGen) {
	case TC_GEN_BASE:
		qglTexCoordPointer (2, GL_FLOAT, 0, rb_inCoordArray);
		return qTrue;

	case TC_GEN_LIGHTMAP:
		qglTexCoordPointer (2, GL_FLOAT, 0, rb_inLMCoordArray);
		return qTrue;

	case TC_GEN_ENVIRONMENT:
		if (glState.in2D) {
			qglTexCoordPointer (2, GL_FLOAT, 0, rb_inCoordArray);
			return qTrue;
		}

		if (rb_currentModel) {
			if (rb_currentModel == r_worldModel || rb_currentModel->type == MODEL_BSP) {
				VectorSubtract (vec3Origin, r_refDef.viewOrigin, transform);
			}
			else {
				VectorSubtract (rb_currentEntity->origin, r_refDef.viewOrigin, transform);
				Matrix_Transpose (rb_currentEntity->axis, inverseAxis);
			}
		}
		else {
			VectorClear (transform);
			Matrix_Transpose (axisIdentity, inverseAxis);
		}

		for (i=0 ; i<rb_numVerts ; i++) {
			VectorAdd (rb_inVertexArray[i], transform, projection);
			VectorNormalizeFast (projection);

			// Project vector
			if (rb_currentModel == r_worldModel || rb_currentModel->type == MODEL_BSP)
				VectorCopy (rb_inNormalsArray[i], n);
			else
				Matrix_TransformVector (inverseAxis, rb_inNormalsArray[i], n);


			depth = -2.0f * DotProduct (n, projection);
			VectorMA (projection, depth, n, projection);
			depth = Q_FastSqrt (DotProduct (projection, projection) * 4);


			rb_outCoordArray[texUnit][i][0] = -((projection[1] * depth) + 0.5f);
			rb_outCoordArray[texUnit][i][1] =  ((projection[2] * depth) + 0.5f);
		}

		qglTexCoordPointer (2, GL_FLOAT, 0, rb_outCoordArray[texUnit][0]);
		return qTrue;

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

		qglEnable (GL_TEXTURE_GEN_S);
		qglEnable (GL_TEXTURE_GEN_T);
		qglTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		qglTexGeni (GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		qglTexGenfv (GL_S, GL_OBJECT_PLANE, genVector[0]);
		qglTexGenfv (GL_T, GL_OBJECT_PLANE, genVector[1]);
		return qFalse;
	}

	case TC_GEN_REFLECTION:
		rb_enableNormals = qTrue;
		qglEnable (GL_TEXTURE_GEN_S);
		qglEnable (GL_TEXTURE_GEN_T);
		qglEnable (GL_TEXTURE_GEN_R);
		qglTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
		qglTexGeni (GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
		qglTexGeni (GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
		return qTrue;

	case TC_GEN_WARP:
		for (i=0 ; i<rb_numVerts ; i++) {
			rb_outCoordArray[texUnit][i][0] = rb_inCoordArray[i][0] + (r_warpSinTable[Q_ftol (((rb_inCoordArray[i][1]*8.0f + rb_shaderTime) * (256.0f / (M_PI * 2.0f)))) & 255] * (1.0/64));
			rb_outCoordArray[texUnit][i][1] = rb_inCoordArray[i][1] + (r_warpSinTable[Q_ftol (((rb_inCoordArray[i][0]*8.0f + rb_shaderTime) * (256.0f / (M_PI * 2.0f)))) & 255] * (1.0/64));
		}

		qglTexCoordPointer (2, GL_FLOAT, 0, rb_outCoordArray[texUnit][0]);
		return qTrue;
	}

	return qTrue;
}
static void RB_ModifyTextureCoordsMatrix (shaderPass_t *pass, int texUnit)
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
		Matrix4_Transpose (r_modelViewMatrix, m1);
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

				VectorNormalizeFast (rb_inNormalsArray[j]);
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
				vec3_t m0[3], m1[3], m2[3], result[3];

				if (rb_numIndexes % 6)
					break;

				if (rb_currentEntity && rb_currentModel != r_worldModel && rb_currentModel->type != MODEL_BSP)
					Matrix4_Matrix3 (r_modelViewMatrix, m1);
				else
					Matrix4_Matrix3 (r_worldViewMatrix, m1);

				Matrix_Transpose (m1, m2);

				for (k=0 ; k<rb_numIndexes ; k += 6) {
					quad[0] = (float *)(rb_outVertexArray + rb_inIndexArray[k+0]);
					quad[1] = (float *)(rb_outVertexArray + rb_inIndexArray[k+1]);
					quad[2] = (float *)(rb_outVertexArray + rb_inIndexArray[k+2]);

					for (j=2 ; j>=0 ; j--) {
						quad[3] = (float *)(rb_outVertexArray + rb_inIndexArray[k+3+j]);
						if (!VectorCompare (quad[3], quad[0]) && 
							!VectorCompare (quad[3], quad[1]) &&
							!VectorCompare (quad[3], quad[2])) {
							break;
						}
					}

					Matrix_FromPoints (quad[0], quad[1], quad[2], m0);

					// Swap m0[0] an m0[1] - FIXME?
					VectorCopy (m0[1], rot_centre);
					VectorCopy (m0[0], m0[1]);
					VectorCopy (rot_centre, m0[0]);

					Matrix_Multiply (m2, m0, result);

					for (j=0 ; j<3 ; j++)
						rot_centre[j] = (quad[0][j] + quad[1][j] + quad[2][j] + quad[3][j]) * 0.25;

					for (j=0 ; j<4 ; j++) {
						VectorSubtract (quad[j], rot_centre, tv);
						Matrix_TransformVector (result, tv, quad[j]);
						VectorAdd (rot_centre, quad[j], quad[j]);
					}
				}
			}
			break;

		case DEFORMV_AUTOSPRITE2:
			if (rb_numIndexes % 6)
				break;

			for (k=0 ; k<rb_numIndexes ; k += 6) {
				int long_axis, short_axis;
				vec3_t axis, tmp;
				float len[3];
				vec3_t m0[3], m1[3], m2[3], result[3];

				quad[0] = (float *)(rb_outVertexArray + rb_inIndexArray[k+0]);
				quad[1] = (float *)(rb_outVertexArray + rb_inIndexArray[k+1]);
				quad[2] = (float *)(rb_outVertexArray + rb_inIndexArray[k+2]);

				for (j=2 ; j>=0 ; j--) {
					quad[3] = (float *)(rb_outVertexArray + rb_inIndexArray[k+3+j]);

					if( !VectorCompare (quad[3], quad[0]) && 
						!VectorCompare (quad[3], quad[1]) &&
						!VectorCompare (quad[3], quad[2])) {
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

				if (!len[long_axis])
					break;
				len[long_axis] = Q_RSqrt (len[long_axis]);
				VectorScale (m0[long_axis], len[long_axis], axis);

				if (DotProduct (m0[long_axis], m0[short_axis])) {
					VectorCopy (axis, m0[1]);
					if (axis[0] || axis[1])
						MakeNormalVectors (m0[1], m0[0], m0[2]);
					else
						MakeNormalVectors (m0[1], m0[2], m0[0]);
				} else {
					if (!len[short_axis])
						break;
					len[short_axis] = Q_RSqrt (len[short_axis]);
					VectorScale (m0[short_axis], len[short_axis], m0[0]);
					VectorCopy (axis, m0[1]);
					CrossProduct (m0[0], m0[1], m0[2]);
				}

				for (j=0 ; j<3 ; j++)
					rot_centre[j] = (quad[0][j] + quad[1][j] + quad[2][j] + quad[3][j]) * 0.25;

				if (rb_currentEntity && rb_currentModel != r_worldModel && rb_currentModel->type != MODEL_BSP) {
					VectorAdd (rb_currentEntity->origin, rot_centre, tv);
					VectorSubtract (r_refDef.viewOrigin, tv, tmp);
					Matrix_TransformVector (rb_currentEntity->axis, tmp, tv);
				}
				else {
					VectorCopy (rot_centre, tv);
					VectorSubtract (r_refDef.viewOrigin, tv, tv);
				}

				// Filter any longest-axis-parts off the camera-direction
				deflect = -DotProduct (tv, axis);

				VectorMA (tv, deflect, axis, m1[2]);
				VectorNormalizeFast (m1[2]);
				VectorCopy (axis, m1[1]);
				CrossProduct (m1[1], m1[2], m1[0]);

				Matrix_Transpose (m1, m2);
				Matrix_Multiply (m2, m0, result);

				for (j=0 ; j<4 ; j++) {
					VectorSubtract (quad[j], rot_centre, tv);
					Matrix_TransformVector (result, tv, quad[j]);
					VectorAdd (rot_centre, quad[j], quad[j]);
				}
			}
			break;

		case DEFORMV_PROJECTION_SHADOW:
			break;

		case DEFORMV_AUTOPARTICLE:
			{
				float scale;
				vec3_t m0[3], m1[3], m2[3], result[3];

				if (rb_numIndexes % 6)
					break;

				memcpy (&rb_outVertexArray, rb_inVertexArray, sizeof (rb_outVertexArray)); // FIXME
				if (rb_currentEntity && rb_currentModel != r_worldModel && rb_currentModel->type != MODEL_BSP)
					Matrix4_Matrix3 (r_modelViewMatrix, m1);
				else
					Matrix4_Matrix3 (r_worldViewMatrix, m1);

				Matrix_Transpose (m1, m2);

				for (k=0 ; k<rb_numIndexes ; k += 6) {
					quad[0] = (float *)(rb_outVertexArray + rb_inIndexArray[k+0]);
					quad[1] = (float *)(rb_outVertexArray + rb_inIndexArray[k+1]);
					quad[2] = (float *)(rb_outVertexArray + rb_inIndexArray[k+2]);

					for (j=2 ; j>=0 ; j--) {
						quad[3] = (float *)(rb_outVertexArray + rb_inIndexArray[k+3+j]);

						if (!VectorCompare (quad[3], quad[0]) && 
							!VectorCompare (quad[3], quad[1]) &&
							!VectorCompare (quad[3], quad[2])) {
							break;
						}
					}

					Matrix_FromPoints (quad[0], quad[1], quad[2], m0);
					Matrix_Multiply (m2, m0, result);

					// Hack a scale up to keep particles from disappearing
					scale = (quad[0][0] - r_refDef.viewOrigin[0]) * r_forwardVec[0] +
							(quad[0][1] - r_refDef.viewOrigin[1]) * r_forwardVec[1] +
							(quad[0][2] - r_refDef.viewOrigin[2]) * r_forwardVec[2];
					if (scale < 20)
						scale = 1.5;
					else
						scale = 1.5 + scale * 0.006f;

					for (j=0 ; j<3 ; j++)
						rot_centre[j] = (quad[0][j] + quad[1][j] + quad[2][j] + quad[3][j]) * 0.25;

					for (j=0 ; j<4 ; j++) {
						VectorSubtract (quad[j], rot_centre, tv);
						Matrix_TransformVector (result, tv, quad[j]);
						VectorMA (rot_centre, scale, quad[j], quad[j]);
					}
				}
			}
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
	if (!rb_numVerts || !rb_numIndexes)
		return;

	// Normals
	if (rb_enableNormals) {
		qglEnableClientState (GL_NORMAL_ARRAY);
		qglNormalPointer (GL_FLOAT, 12, rb_inNormalsArray);
	}

	// Color
	if (rb_numColors) {
		if (rb_numColors > 1) {
			qglEnableClientState (GL_COLOR_ARRAY);
			qglColorPointer (4, GL_UNSIGNED_BYTE, 0, rb_outColorArray);
		}
		else if (rb_numColors == 1)
			qglColor4ubv (rb_outColorArray[0]);
	}

	// Draw
	if (glConfig.extDrawRangeElements)
		qglDrawRangeElementsEXT (GL_TRIANGLES, 0, rb_numVerts, rb_numIndexes, GL_UNSIGNED_INT, rb_inIndexArray);
	else
		qglDrawElements (GL_TRIANGLES, rb_numIndexes, GL_UNSIGNED_INT, rb_inIndexArray);

	// Color
	if (rb_numColors > 1)
		qglDisableClientState (GL_COLOR_ARRAY);

	// Normals
	if (rb_enableNormals) {
		qglDisableClientState (GL_NORMAL_ARRAY);
		rb_enableNormals = qFalse;
	}

	glSpeeds.numVerts += rb_numVerts;
	glSpeeds.numTris += rb_numIndexes/3;
	glSpeeds.numElements++;

	if (rb_currentMeshType == MBT_MODEL_BSP)
		glSpeeds.worldElements++;
}


/*
=============
RB_CleanUpTextureUnits
=============
*/
static void RB_CleanUpTextureUnits (void)
{
	int		i;

	for (i=rb_numPasses-1 ; i>0 ; i--) {
		GL_SelectTexture (i);
		qglDisable (GL_TEXTURE_2D);
		qglDisable (GL_TEXTURE_GEN_S);
		qglDisable (GL_TEXTURE_GEN_T);
		qglDisable (GL_TEXTURE_GEN_R);
		qglDisable (GL_TEXTURE_CUBE_MAP_ARB);
		qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
	}

	GL_SelectTexture (0);
	qglDisable (GL_TEXTURE_GEN_S);
	qglDisable (GL_TEXTURE_GEN_T);
	qglDisable (GL_TEXTURE_GEN_R);
	qglDisable (GL_TEXTURE_CUBE_MAP_ARB);
	qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
}


/*
=============
RB_BindShaderPass
=============
*/
static void RB_BindShaderPass (shaderPass_t *pass, image_t *image, int texUnit)
{
	if (!image) {
		// Find the texture
		if (pass->flags & SHADER_PASS_LIGHTMAP)
			image = &r_lmTextures[rb_currentLmTexNum];
		else if (pass->flags & SHADER_PASS_ANIMMAP)
			image = pass->animFrames[(int)(pass->animFPS * rb_shaderTime) % pass->animNumFrames];
		else
			image = pass->animFrames[0];
	}

	// Bind the texture
	GL_SelectTexture (texUnit);
	GL_BindTexture (image);
	qglEnable (GL_TEXTURE_2D);
	if (image->flags & IT_CUBEMAP)
		qglEnable (GL_TEXTURE_CUBE_MAP_ARB);
	else
		qglEnableClientState (GL_TEXTURE_COORD_ARRAY);

	// Modify coordinates
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
	if (pass->flags & SHADER_PASS_VERTEXPROGRAM) {
		program = pass->vertProg;

		qglEnable (GL_VERTEX_PROGRAM_ARB);
		qglBindProgramARB (GL_VERTEX_PROGRAM_ARB, program->progNum);

		qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 0, r_refDef.viewOrigin[0], r_refDef.viewOrigin[1], r_refDef.viewOrigin[2], 0);
		qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 1, r_refDef.viewAxis[0][0], r_refDef.viewAxis[0][1], r_refDef.viewAxis[0][2], 0);
		qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 2, r_refDef.viewAxis[1][0], r_refDef.viewAxis[1][1], r_refDef.viewAxis[1][2], 0);
		qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 3, r_refDef.viewAxis[2][0], r_refDef.viewAxis[2][1], r_refDef.viewAxis[2][2], 0);
		if (rb_currentEntity) {
			qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 4, rb_currentEntity->origin[0], rb_currentEntity->origin[1], rb_currentEntity->origin[2], 0);
			qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 5, rb_currentEntity->axis[0][0], rb_currentEntity->axis[0][1], rb_currentEntity->axis[0][2], 0);
			qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 6, rb_currentEntity->axis[1][0], rb_currentEntity->axis[1][1], rb_currentEntity->axis[1][2], 0);
			qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 7, rb_currentEntity->axis[2][0], rb_currentEntity->axis[2][1], rb_currentEntity->axis[2][2], 0);
		}
		qglProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 8, rb_shaderTime, 0, 0, 0);
	}
	else
		qglDisable (GL_VERTEX_PROGRAM_ARB);

	// Fragment program
	if (pass->flags & SHADER_PASS_FRAGMENTPROGRAM) {
		program = pass->fragProg;

		qglEnable (GL_FRAGMENT_PROGRAM_ARB);
		qglBindProgramARB (GL_FRAGMENT_PROGRAM_ARB, program->progNum);

		qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 0, r_refDef.viewOrigin[0], r_refDef.viewOrigin[1], r_refDef.viewOrigin[2], 0);
		qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 1, r_refDef.viewAxis[0][0], r_refDef.viewAxis[0][1], r_refDef.viewAxis[0][2], 0);
		qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 2, r_refDef.viewAxis[1][0], r_refDef.viewAxis[1][1], r_refDef.viewAxis[1][2], 0);
		qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 3, r_refDef.viewAxis[2][0], r_refDef.viewAxis[2][1], r_refDef.viewAxis[2][2], 0);
		if (rb_currentEntity) {
			qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 4, rb_currentEntity->origin[0], rb_currentEntity->origin[1], rb_currentEntity->origin[2], 0);
			qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 5, rb_currentEntity->axis[0][0], rb_currentEntity->axis[0][1], rb_currentEntity->axis[0][2], 0);
			qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 6, rb_currentEntity->axis[1][0], rb_currentEntity->axis[1][1], rb_currentEntity->axis[1][2], 0);
			qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 7, rb_currentEntity->axis[2][0], rb_currentEntity->axis[2][1], rb_currentEntity->axis[2][2], 0);
		}
		qglProgramLocalParameter4fARB (GL_FRAGMENT_PROGRAM_ARB, 8, rb_shaderTime, 0, 0, 0);
	}
	else
		qglDisable (GL_FRAGMENT_PROGRAM_ARB);

	// Blending
	if (pass->flags & SHADER_PASS_BLEND) {
		qglEnable (GL_BLEND);
		GL_BlendFunc (pass->blendSource, pass->blendDest);
	}
	else if (rb_currentEntity && rb_currentEntity->flags & RF_TRANSLUCENT) {
		qglEnable (GL_BLEND);
		GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else {
		if (mTex && pass->blendMode != GL_REPLACE && pass->blendMode != GL_DOT3_RGB_ARB)
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
	default:
		qglDisable (GL_ALPHA_TEST);
		break;
	}

	// Nasty hack!!!
	if (!glState.in2D) {
		qglDepthFunc (pass->depthFunc);
		if (rb_currentShader->flags & SHADER_DEPTHWRITE && !(rb_currentEntity && rb_currentEntity->flags & RF_TRANSLUCENT)) // FIXME
			qglDepthMask (GL_TRUE);
		else
			qglDepthMask (GL_FALSE);
	}

	// Mask colors
	qglColorMask (!pass->maskRed, !pass->maskGreen, !pass->maskBlue, !pass->maskAlpha);
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

	RB_DrawElements ();
	RB_CleanUpTextureUnits ();
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
			GL_TextureEnv (pass->blendMode);
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
			}
		}
	}

	RB_DrawElements ();
	RB_CleanUpTextureUnits ();
}


/*
================
RB_RenderDot3Combined
================
*/
void RB_RenderDot3Combined (void)
{
	shaderPass_t	*pass = rb_accumPasses[0];
	vec4_t			colorBlackNoAlpha = { 0, 0, 0, 0 };
	vec4_t			ambient, diffuse;
	qBool			doAdd, doModulate;

	if (!rb_currentEntity)
		return;

	doAdd = qTrue;
	doModulate = qTrue;
	rb_numColors = rb_numVerts;

	RB_BindShaderPass (pass, pass->animFrames[0], 0);
	//R_LightForEntityDot3 (rb_currentEntity, rb_colorArray[0], ambient, diffuse);
	ambient[0] = 127;
	ambient[1] = 127;
	ambient[2] = 127;
	ambient[3] = 127;
	diffuse[0] = 255;
	diffuse[1] = 255;
	diffuse[2] = 255;
	diffuse[3] = 255;
	R_LightForEntity (rb_currentEntity, rb_numVerts, rb_outColorArray[0]);
	RB_SetupPassState (pass, qTrue);

	GL_TextureEnv (GL_COMBINE_ARB);
	qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_DOT3_RGB_ARB);
	qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
	qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
	qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
	qglEnableClientState (GL_TEXTURE_COORD_ARRAY);

	if (!pass->animFrames[1]) {
		RB_DrawElements ();
		qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
		return;
	}

	GL_SelectTexture (1);
	qglEnable (GL_TEXTURE_2D);
	GL_TextureEnv (GL_COMBINE_ARB);
	qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
	qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT_ARB);
	qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
	qglTexEnvfv (GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, diffuse);

	if (glConfig.maxTexUnits > 2) {
		doAdd = qFalse;
		GL_SelectTexture (2);
		qglEnable (GL_TEXTURE_2D);
		GL_TextureEnv (GL_COMBINE_ARB);
		qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD);
		qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
		qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT_ARB);
		qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
		qglTexEnvfv (GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, ambient);

		if (glConfig.maxTexUnits > 3) {
			doModulate = qFalse;
			RB_BindShaderPass (pass, pass->animFrames[1], 3);
			GL_TextureEnv (GL_MODULATE);
			qglBlendFunc (GL_ZERO, GL_SRC_COLOR);
		}
	}
	RB_DrawElements ();

	if (glConfig.maxTexUnits > 2) {
		if (glConfig.maxTexUnits > 3) {
			qglDisable (GL_TEXTURE_2D);
			qglDisable (GL_TEXTURE_COORD_ARRAY);
		}
		GL_SelectTexture (2);
		GL_TextureEnv (GL_MODULATE);
		qglTexEnvfv (GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, colorBlackNoAlpha);
		qglDisable (GL_TEXTURE_2D);
	}
	GL_SelectTexture (1);
	GL_TextureEnv (GL_MODULATE);
	qglTexEnvfv (GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, colorBlackNoAlpha);
	qglDisable (GL_TEXTURE_2D);

	GL_SelectTexture (0);

	if (doAdd || doModulate) {
		rb_numColors = 0;
		GL_BindTexture (pass->animFrames[1]);
		GL_TextureEnv (GL_MODULATE);
		qglEnable (GL_BLEND);

		if (doModulate) {
			qglBlendFunc (GL_ZERO, GL_SRC_COLOR);
			qglColor4fv (colorWhite);
			RB_DrawElements ();
		}
		if (doAdd) {
			qglBlendFunc (GL_ONE, GL_ONE);
			qglColor4fv (ambient);
			RB_DrawElements ();
		}
		qglDisable (GL_BLEND);
	}
	qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
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

	RB_DrawElements ();
	RB_CleanUpTextureUnits ();
}

/*
===============================================================================

	MESH PUSHING

===============================================================================
*/

/*
=============
RB_BackendOverflow
=============
*/
qBool RB_BackendOverflow (const mesh_t *mesh)
{
	return (rb_numVerts + mesh->numVerts > VERTARRAY_MAX_VERTS || 
		rb_numIndexes + mesh->numIndexes > VERTARRAY_MAX_INDEXES);
}


/*
=============
RB_InvalidMesh
=============
*/
qBool RB_InvalidMesh (const mesh_t *mesh)
{
	return (!mesh->numVerts || !mesh->numIndexes || 
		mesh->numVerts > VERTARRAY_MAX_VERTS || mesh->numIndexes > VERTARRAY_MAX_INDEXES);
}


/*
=============
RB_PushTriangleIndexes
=============
*/
#if SHADOW_VOLUMES
static void RB_PushTriangleIndexes (index_t *indexes, int *neighbors, vec3_t *trNormals, int count, int meshFeatures)
{
	int		numTris;
#else
static void RB_PushTriangleIndexes (index_t *indexes, int count, int meshFeatures)
{
#endif
	index_t	*currentIndex;

	if (meshFeatures & MF_NONBATCHED) {
		rb_numIndexes = count;
		rb_inIndexArray = indexes;

#ifdef SHADOW_VOLUMES
		if (neighbors) {
			rb_inNeighborsArray = neighbors;
			rb_currentTrNeighbor = rb_inNeighborsArray + rb_numIndexes;
		}

		if (meshFeatures & MF_TRNORMALS && trNormals) {
			numTris = rb_numIndexes / 3;
			rb_inTrNormalsArray = trNormals;
			rb_currentTrNormal = rb_inTrNormalsArray[0] + numTris;
		}
#endif
	}
	else {
#ifdef SHADOW_VOLUMES
		numTris = rb_numIndexes / 3;
#endif
		currentIndex = rb_indexArray + rb_numIndexes;
		rb_numIndexes += count;
		rb_inIndexArray = rb_indexArray;

		// The following code assumes that R_PushIndexes is fed with triangles
		for ( ; count>0 ; count -= 3, indexes += 3, currentIndex += 3) {
			currentIndex[0] = rb_numVerts + indexes[0];
			currentIndex[1] = rb_numVerts + indexes[1];
			currentIndex[2] = rb_numVerts + indexes[2];

#if SHADOW_VOLUMES
			if (neighbors) {
				rb_currentTrNeighbor[0] = numTris + neighbors[0];
				rb_currentTrNeighbor[1] = numTris + neighbors[1];
				rb_currentTrNeighbor[2] = numTris + neighbors[2];

				neighbors += 3;
				rb_currentTrNeighbor += 3;
			}

			if (meshFeatures & MF_TRNORMALS && trNormals) {
				VectorCopy (*trNormals, rb_currentTrNormal);
				trNormals++;
				rb_currentTrNormal += 3;
			}
#endif
		}
	}
}

/*
=============
R_PushTrifanIndexes
=============
*/
static void R_PushTrifanIndexes (int numVerts)
{
	index_t	*currentIndex;
	int		count;

	currentIndex = rb_indexArray + rb_numIndexes;
	rb_numIndexes += numVerts + numVerts + numVerts - 6;
	rb_inIndexArray = rb_indexArray;

	for (count=2 ; count<numVerts ; count++, currentIndex += 3) {
		currentIndex[0] = rb_numVerts;
		currentIndex[1] = rb_numVerts + count - 1;
		currentIndex[2] = rb_numVerts + count;
	}
}


/*
=============
RB_PushMesh
=============
*/
void RB_PushMesh (mesh_t *mesh, int meshFeatures)
{
	rb_currentMeshFeatures = meshFeatures;

	// Push indexes
	if (meshFeatures & MF_TRIFAN)
		R_PushTrifanIndexes (mesh->numVerts);
	else
#if SHADOW_VOLUMES
		RB_PushTriangleIndexes (mesh->indexArray, mesh->trNeighborsArray, mesh->trNormalsArray, mesh->numIndexes, meshFeatures);
#else
		RB_PushTriangleIndexes (mesh->indexArray, mesh->numIndexes, meshFeatures);
#endif

	// Push the rest
	if (meshFeatures & MF_NONBATCHED) {
		rb_numVerts = 0;

		// Vertexes and normals
		if (meshFeatures & MF_DEFORMVS) {
			if (mesh->vertexArray != rb_vertexArray)
				memcpy (rb_vertexArray[rb_numVerts], mesh->vertexArray, mesh->numVerts * sizeof (vec3_t));
			rb_inVertexArray = rb_vertexArray;

			if (meshFeatures & MF_NORMALS && mesh->normalsArray) {
				if (mesh->normalsArray != rb_normalsArray)
					memcpy (rb_normalsArray[rb_numVerts], mesh->normalsArray, mesh->numVerts * sizeof (vec3_t));
				rb_inNormalsArray = rb_normalsArray;
			}
		}
		else {
			rb_inVertexArray = mesh->vertexArray;

			if (meshFeatures & MF_NORMALS && mesh->normalsArray)
				rb_inNormalsArray = mesh->normalsArray;
		}

		// Texture coordinates
		if (meshFeatures & MF_STCOORDS && mesh->coordArray)
			rb_inCoordArray = mesh->coordArray;

		// Lightmap texture coordinates
		if (meshFeatures & MF_LMCOORDS && mesh->lmCoordArray)
			rb_inLMCoordArray = mesh->lmCoordArray;

		// STVectors
		if (meshFeatures & MF_STVECTORS) {
			if (mesh->sVectorsArray)
				rb_inSVectorsArray = mesh->sVectorsArray;
			if (mesh->tVectorsArray)
				rb_inTVectorsArray = mesh->tVectorsArray;
		}
	}
	else {
		// Vertexes
		memcpy (rb_vertexArray[rb_numVerts], mesh->vertexArray, mesh->numVerts * sizeof (vec3_t));
		rb_inVertexArray = rb_vertexArray;

		// Normals
		if (meshFeatures & MF_NORMALS && mesh->normalsArray) {
			memcpy (rb_normalsArray[rb_numVerts], mesh->normalsArray, mesh->numVerts * sizeof (vec3_t));
			rb_inNormalsArray = rb_normalsArray;
		}

		// Texture coordinates
		if (meshFeatures & MF_STCOORDS && mesh->coordArray) {
			memcpy (rb_coordArray[rb_numVerts], mesh->coordArray, mesh->numVerts * sizeof (vec2_t));
			rb_inCoordArray = rb_coordArray;
		}

		// Lightmap texture coordinates
		if (meshFeatures & MF_LMCOORDS && mesh->lmCoordArray) {
			memcpy (rb_lMCoordArray[rb_numVerts], mesh->lmCoordArray, mesh->numVerts * sizeof (vec2_t));
			rb_inLMCoordArray = rb_lMCoordArray;
		}

		// STVectors
		if (meshFeatures & MF_STVECTORS) {
			if (mesh->sVectorsArray) {
				memcpy (rb_sVectorsArray[rb_numVerts], mesh->sVectorsArray, mesh->numVerts * sizeof (vec3_t));
				rb_inSVectorsArray = mesh->sVectorsArray;
			}
			if (mesh->tVectorsArray) {
				memcpy (rb_tVectorsArray[rb_numVerts], mesh->tVectorsArray, mesh->numVerts * sizeof (vec3_t));
				rb_inTVectorsArray = mesh->tVectorsArray;
			}
		}
	}

	// Colors
	if (meshFeatures & MF_COLORS && mesh->colorArray) {
		memcpy (rb_colorArray[rb_numVerts], mesh->colorArray, mesh->numVerts * sizeof (bvec4_t));
		rb_inColorArray = rb_colorArray;
	}

	rb_numVerts += mesh->numVerts;
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
	if (rb_accumPasses[0]->blendMode == GL_DOT3_RGB_ARB)
		RB_RenderDot3Combined ();
	else if (rb_numPasses == 1)
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

	if (!rb_numPasses) {
		rb_accumPasses[rb_numPasses++] = pass;
		return;
	}

	// Check bounds
	accum = (rb_numPasses < glConfig.maxTexUnits);

	// Accumulate?
	if (accum) {
		// Compare against previous pass properties
		prevPass = rb_accumPasses[rb_numPasses - 1];

		if (prevPass->depthFunc != pass->depthFunc
		|| prevPass->totalMask != pass->totalMask
		|| pass->alphaFunc != ALPHA_FUNC_NONE
		|| pass->rgbGen.type != RGB_GEN_IDENTITY
		|| pass->alphaGen.type != ALPHA_GEN_IDENTITY
		|| (prevPass->flags != ALPHA_FUNC_NONE && pass->depthFunc != GL_EQUAL))
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
	rb_accumPasses[rb_numPasses++] = pass;
}


/*
=============
RB_SetOutlineColor
=============
*/
static inline void RB_SetOutlineColor (void)
{
	switch (rb_currentMeshType) {
	case MBT_MODEL_ALIAS:
	case MBT_MODEL_SP2:
	case MBT_SKY:
		qglColor4fv (colorRed);
		break;

	case MBT_MODEL_BSP:
		qglColor4fv (colorWhite);
		break;

	case MBT_POLY:
		qglColor4fv (colorGreen);
		break;
	}
}


/*
=============
RB_DrawNormals
=============
*/
static inline void RB_DrawNormals (void)
{
	int		i;

	if (!rb_inNormalsArray)
		return;

	if (gl_shownormals->integer == 2)
		RB_SetOutlineColor ();
	else
		qglColor4ub (255, 255, 255, 255);

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
=============
RB_DrawTriangles
=============
*/
static inline void RB_DrawTriangles (void)
{
	int		i;

	if (gl_showtris->integer == 2)
		RB_SetOutlineColor ();
	else
		qglColor4ub (255, 255, 255, 255);

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
RB_SetupShaderState
=============
*/
static void RB_SetupShaderState (shader_t *shader)
{
	// Culling
	if (!gl_cull->integer || rb_currentMeshFeatures & MF_NOCULL) {
		qglDisable (GL_CULL_FACE);
	}
	else {
		switch (rb_currentShader->cullType) {
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
		}
	}

	// Polygon offset
	if (rb_currentShader->flags & SHADER_POLYGONOFFSET) {
		qglEnable (GL_POLYGON_OFFSET_FILL);
		qglPolygonOffset (rb_currentShader->offsetFactor, rb_currentShader->offsetUnits);
	}
	else
		qglDisable (GL_POLYGON_OFFSET_FILL);

	// Depth range
	if (rb_currentShader->flags & SHADER_DEPTHRANGE) {
		qglDepthRange (rb_currentShader->depthNear, rb_currentShader->depthFar);
	}
	else {
		if (rb_currentEntity && rb_currentEntity->flags & RF_DEPTHHACK)
			qglDepthRange (0, 0.3f);
		else
			qglDepthRange (0, 1);
	}
}


/*
=============
RB_RenderMeshBuffer
=============
*/
void RB_RenderMeshBuffer (meshBuffer_t *mb, qBool shadowPass)
{
	shaderPass_t	*pass;
	int				i;

	// Copy values
	rb_currentMeshType = mb->sortKey & 7;
	rb_currentEntity = mb->entity;
	rb_currentModel = mb->entity ? mb->entity->model : NULL;
	rb_currentShader = mb->shader;
	rb_currentLmTexNum = (rb_currentMeshType == MBT_MODEL_BSP) ? ((mBspSurface_t *)mb->mesh)->lmTexNumActive : -1;

	// Shader time
	if (glState.in2D) {
		rb_shaderTime = sys_currTime * 0.001f;
	}
	else {
		rb_shaderTime = r_refDef.time;

		if (rb_currentEntity) {
			rb_shaderTime -= rb_currentEntity->shaderTime;
			if (rb_shaderTime < 0)
				rb_shaderTime = 0;
		}
	}

	// State
	if (!rb_triangleOutlines)
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

	// Render outlines if desired
	if (rb_triangleOutlines) {
		RB_LockArrays (rb_numVerts);

		if (gl_showtris->integer)
			RB_DrawTriangles ();
		if (gl_shownormals->integer)
			RB_DrawNormals ();

		RB_UnlockArrays ();
		RB_ResetPointers ();
		return;
	}

	// Accumulate passes and render
	RB_LockArrays (rb_numVerts);
	for (i=0, pass=rb_currentShader->passes ; i<rb_currentShader->numPasses ; pass++, i++) {
		if (pass->flags & SHADER_PASS_LIGHTMAP) {
			if (rb_currentLmTexNum < 0)
				continue;
		}
		else if (!pass->animNumFrames)
			continue;
		if (pass->flags & SHADER_PASS_DETAIL && !r_detailTextures->integer)
			continue;

		RB_AccumulatePass (pass);

		glSpeeds.shaderPasses++;
	}

	// Draw lightmaps if desired
	if (gl_lightmap->integer && rb_currentLmTexNum >= 0) {
		RB_AccumulatePass (&rb_lightMapPass);
		glSpeeds.shaderPasses++;
	}

	if (rb_numPasses)
		RB_RenderAccumulatedPasses ();

	// Reset
	GL_LoadIdentityTexMatrix ();
	qglMatrixMode (GL_MODELVIEW);

	RB_UnlockArrays ();

	// Nasty hack!!!
	if (!glState.in2D)
		qglDepthMask (GL_TRUE);

	glSpeeds.shaderCount++;

	RB_ResetPointers ();
}


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
===============================================================================

	FRAME HANDLING

===============================================================================
*/

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

	if (prevupdate > (sys_currTime % rupdate)) {
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
	prevupdate = (sys_currTime % rupdate);
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

		if (t < 0.25)
			rb_triangleTable[i] = t * 4.0f;
		else if (t < 0.75)
			rb_triangleTable[i] = 2 - 4.0f * t;
		else
			rb_triangleTable[i] = (t - 0.75f) * 4.0f - 1.0f;

		if (t < 0.5)
			rb_squareTable[i] = 1.0f;
		else
			rb_squareTable[i] = -1.0f;

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

	// Togglable solid lightmap overlay
	rb_lightMapPass.alphaFunc = ALPHA_FUNC_NONE;
	rb_lightMapPass.flags = SHADER_PASS_LIGHTMAP|SHADER_PASS_NOCOLORARRAY;
	rb_lightMapPass.tcGen = TC_GEN_LIGHTMAP;
	rb_lightMapPass.depthFunc = GL_LEQUAL;
	rb_lightMapPass.blendMode = GL_REPLACE;
	rb_lightMapPass.rgbGen.type = RGB_GEN_IDENTITY;
	rb_lightMapPass.alphaGen.type = ALPHA_GEN_IDENTITY;
	rb_lightMapPass.numTCMods = 0;

	// Find rendering paths
	switch (glConfig.renderClass) {
	case REND_CLASS_INTEL:
	case REND_CLASS_S3:
	case REND_CLASS_SIS:
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
