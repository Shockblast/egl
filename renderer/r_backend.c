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
// TODO Rendering backend (hehehe)
//

#include "r_local.h"

vec4_t					colorArray[VERTARRAY_MAX_VERTS];
vec2_t					coordArray[VERTARRAY_MAX_VERTS];
vec3_t					normalsArray[VERTARRAY_MAX_VERTS];
vec3_t					vertexArray[VERTARRAY_MAX_VERTS];

vec2_t					lmCoordArray[VERTARRAY_MAX_VERTS];

static vec4_t			tempColorArray[VERTARRAY_MAX_VERTS];
static vec2_t			tempCoordArray[VERTARRAY_MAX_VERTS];
static vec3_t			tempNormalsArray[VERTARRAY_MAX_VERTS];
static vec3_t			tempVertexArray[VERTARRAY_MAX_VERTS];

static vec4_t			*inColorArray;
static vec2_t			*inCoordArray;
static vec3_t			*inNormalsArray;
static vec3_t			*inVertexArray;

static vec2_t			*inLmCoordArray;

static index_t			*inIndexArray;
static int				inNumColors;
static int				inNumIndexes;
static int				inNumVerts;

static qBool			rbEnableNormals;
static qBool			rbMultiTexture;

static image_t			*rbTextures[TU_MAX];
static shader_t			*rbShader;
static mBspSurface_t	*rbSurface;

static qBool			rbArraysLocked;

index_t					rbQuadIndices[QUAD_INDEXES] = {0, 1, 2, 0, 2, 3};
index_t					rbTriFanIndices[TRIFAN_INDEXES];

entity_t				*rbCurrEntity;
model_t					*rbCurrModel;

/*
=============
RB_ResetPointers
=============
*/
static inline void RB_ResetPointers (void) {
	int		i;

	inColorArray = colorArray;
	inCoordArray = coordArray;
	inNormalsArray = normalsArray;
	inVertexArray = vertexArray;
	inIndexArray = NULL;

	inLmCoordArray = NULL;

	inNumColors = 0;
	inNumIndexes = 0;
	inNumVerts = 0;

	rbEnableNormals = qFalse;
	rbMultiTexture = qFalse;

	for (i=0 ; i<TU_MAX ; i++)
		rbTextures[i] = NULL;
	rbShader = NULL;
	rbSurface = NULL;

	qglTexCoordPointer (2, GL_FLOAT, 0, inCoordArray);
	qglVertexPointer (3, GL_FLOAT, 0, inVertexArray);
}


/*
=============
RB_LockArrays
=============
*/
static inline void RB_LockArrays (int numVerts) {
	if (rbArraysLocked)
		return;
	if (!glConfig.extCompiledVertArray)
		return;

	qglLockArraysEXT (0, numVerts);
	rbArraysLocked = qTrue;
}


/*
=============
RB_UnlockArrays
=============
*/
static inline void RB_UnlockArrays (void) {
	if (!rbArraysLocked)
		return;

	qglUnlockArraysEXT ();
	rbArraysLocked = qFalse;
}

/*
===============================================================================

	SHADER HANDLING

===============================================================================
*/

/*
=============
RB_ModifyColor
=============
*/
static qBool RB_ModifyColor (shaderStage_t *stage) {
	float	colorScale;
	float	surfAlpha;
	float	scriptAlpha;
	int		i;

	colorScale = 1.0f;
	surfAlpha = 1.0f;

	// surface forced alpha/color
	if (rbSurface) {
		qBool	trans33 = (rbSurface->texInfo->flags & SURF_TRANS33) ? qTrue : qFalse;
		qBool	trans66 = (rbSurface->texInfo->flags & SURF_TRANS66) ? qTrue : qFalse;

		if (trans33 || trans66) {
			surfAlpha = 0.0f;
			if (trans33)
				surfAlpha = 0.33f;
			if (trans66)
				surfAlpha = 0.66f;
			colorScale = glState.invIntens;
		}

		if (rbSurface->flags & SURF_DRAWTURB) {
			colorScale = glState.invIntens;
		}
	}

	// shader alpha
	if (stage) {
		RS_AlphaShift (stage, &scriptAlpha);
		if (scriptAlpha <= 0)
			return qFalse;
	}
	else
		scriptAlpha = 1.0f;

	// lighting
	if (!stage || (stage->lightMap || stage->lightMapOnly)) {
		for (i=0 ; i<inNumColors ; i++) {
			tempColorArray[i][0] = inColorArray[i][0] * colorScale;
			tempColorArray[i][1] = inColorArray[i][1] * colorScale;
			tempColorArray[i][2] = inColorArray[i][2] * colorScale;
			tempColorArray[i][3] = inColorArray[i][3] * scriptAlpha * surfAlpha;
		}
	}
	else {
		for (i=0 ; i<inNumColors ; i++) {
			tempColorArray[i][0] = 1 * colorScale;
			tempColorArray[i][1] = 1 * colorScale;
			tempColorArray[i][2] = 1 * colorScale;
			tempColorArray[i][3] = inColorArray[i][3] * scriptAlpha * surfAlpha;
		}
	}

	// FIXME inColorArray[0] is used because I need to see if an entity is translucent
	// later on we can remove the secondary loop for transparent entities
	// which will let me remove the surrounding blending enable/disable
	// same could be done with alpha surfaces
	if ((stage && stage->blendFunc.blend) || (tempColorArray[0][3] < 1.0f)) {
		if (stage && stage->blendFunc.blend)
			GL_BlendFunc (stage->blendFunc.source, stage->blendFunc.dest);
		else
			GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		qglEnable (GL_BLEND);
	}

	// FIXME don't touch depthmask until it's a shader option
	/*if (rbCurrModel && (rbCurrModel != r_WorldModel) && (rbCurrModel->type != MODEL_BSP)) {
		if ((scriptAlpha < 1) && !rbForceBlend)
			qglDepthMask (GL_FALSE);
	}*/

//	GL_TexEnv (GL_MODULATE);
	/*if (stage && glConfig.extMTexCombine) {
		GL_TexEnv (GL_COMBINE4_NV);

		qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD);
		qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_ADD);

		qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
		qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
		qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

		switch (stage->blendFunc.source) {
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

		switch (stage->blendFunc.dest) {
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
	}*/

	return qTrue;
}


/*
=============
RB_ModifyTextureCoords
=============
*/
#define ROT_DEGREE (-stage->rotateSpeed * glState.realTime * 0.0087266388888888888888888888888889)
static qBool RB_ModifyTextureCoords (shaderStage_t *stage, int stageNum) {
	int		i;
	vec2_t	scrollCoords, coords;
	vec3_t	n, transform, projection;
	vec3_t	inverseAxis[3];
	float	*mat = r_WorldViewMatrix;
	float	depth, scroll = 0;
	vec3_t	tcv;
	qBool	envMapPass;

	qglTexCoordPointer (2, GL_FLOAT, 0, tempCoordArray);

	//
	// setup
	//
	if (stage) {
		envMapPass = (stage->envMap) ? qTrue : qFalse;

		// x scroll
		if (stage->scroll.speedX) {
			switch (stage->scroll.typeX) {
			// sine
			case STAGE_SINE:
				scrollCoords[0] = sin (glState.realTime * stage->scroll.speedX);
				break;

			// cosine
			case STAGE_COSINE:
				scrollCoords[0] = cos (glState.realTime * stage->scroll.speedX);
				break;

			// default
			default:
			case STAGE_STATIC:
				scrollCoords[0] = glState.realTime * stage->scroll.speedX;
				break;
			}
		}
		else
			scrollCoords[0] = 0;

		// y scroll
		if (stage->scroll.speedY) {
			switch (stage->scroll.typeY) {
			// sine
			case STAGE_SINE:
				scrollCoords[1] = sin (glState.realTime * stage->scroll.speedY);
				break;

			// cosine
			case STAGE_COSINE:
				scrollCoords[1] = cos (glState.realTime * stage->scroll.speedY);
				break;

			// default
			default:
			case STAGE_STATIC:
				scrollCoords[1] = glState.realTime * stage->scroll.speedY;
				break;
			}
		}
		else
			scrollCoords[1] = 0;
	}
	else {
		envMapPass = qFalse;

		Vector2Clear (scrollCoords);
	}

	// scrolling
	if (rbSurface && (rbSurface->texInfo->flags & SURF_FLOWING)) {
		if (rbSurface->flags & SURF_DRAWTURB)
			scroll = -64 * ((r_RefDef.time*0.5) - (int)(r_RefDef.time*0.5));
		else {
			scroll = -64 * ((r_RefDef.time*0.025) - (int)(r_RefDef.time*0.025));
			if (scroll == 0.0)
				scroll = -64.0;
		}
	}

	// envmap
	if (envMapPass) {
		if (rbCurrModel) {
			if (rbCurrModel == r_WorldModel) {
				VectorSubtract (vec3Origin, r_RefDef.viewOrigin, transform);
			}
			else if (rbCurrModel->type == MODEL_BSP) {
				VectorSubtract (rbCurrEntity->origin, r_RefDef.viewOrigin, transform);
				Matrix_Transpose (rbCurrEntity->axis, inverseAxis);
			}
			else {
				VectorSubtract (rbCurrEntity->origin, r_RefDef.viewOrigin, transform);
				Matrix_Transpose (rbCurrEntity->axis, inverseAxis);
			}
		}
		else {
			VectorClear (transform);
			Matrix_Transpose (axisIdentity, inverseAxis);
		}
	}

	// push into temp array
	for (i=0 ; i<inNumVerts ; i++) {
		coords[0] = inCoordArray[i][0];
		coords[1] = inCoordArray[i][1];

		if (stage) {
			// x scale
			if (stage->scale.scaleX) {
				switch (stage->scale.typeX) {
				// sine
				case STAGE_SINE:
					coords[0] *= stage->scale.scaleX * sin (glState.realTime * 0.05);
					break;

				// cosine
				case STAGE_COSINE:
					coords[0] *= stage->scale.scaleX * cos (glState.realTime * 0.05);
					break;

				// default
				default:
				case STAGE_STATIC:
					coords[0] *= stage->scale.scaleX;
					break;
				}
			}

			// y scale
			if (stage->scale.scaleY) {
				switch (stage->scale.typeY) {
				// sine
				case STAGE_SINE:
					coords[1] *= stage->scale.scaleY * sin (glState.realTime * 0.05);
					break;

				// cosine
				case STAGE_COSINE:
					coords[1] *= stage->scale.scaleY * cos (glState.realTime * 0.05);
					break;

				// default
				default:
				case STAGE_STATIC:
					coords[1] *= stage->scale.scaleY;
					break;
				}
			}

			// rotation
			if (stage->rotateSpeed) {
				float	cost = cos (ROT_DEGREE);
				float	sint = sin (ROT_DEGREE);

				if (rbSurface) {
					coords[0] = cost * (coords[0] - rbSurface->c_s) + sint * (rbSurface->c_t - coords[1]) + rbSurface->c_s;
					coords[1] = cost * (coords[1] - rbSurface->c_t) + sint * (coords[0] - rbSurface->c_s) + rbSurface->c_t;
				}
				else {
					coords[0] = cost * (coords[0] - 0.5) + sint * (0.5 - coords[1]) + 0.5;
					coords[1] = cost * (coords[1] - 0.5) + sint * (coords[0] - 0.5) + 0.5;
				}
			}
		}

		// envmap
		if (envMapPass) {
			VectorAdd (inVertexArray[i], transform, projection);
			VectorNormalize (projection, projection);

			// project vector
			if (rbCurrModel && (rbCurrModel == r_WorldModel))
				VectorCopy (inNormalsArray[i], n);
			else
				Matrix_TransformVector (inverseAxis, inNormalsArray[i], n);

			depth = -2.0f * DotProduct (n, projection);
			VectorMA (projection, depth, n, projection);
			depth = Q_FastSqrt (DotProduct (projection, projection) * 4);

			coords[0] = -((projection[1] * depth) + 0.5f);
			coords[1] =  ((projection[2] * depth) + 0.5f);

			// partially include the view vectors
			tcv[0] = (inVertexArray[i][0] * mat[0 ]) +
					 (inVertexArray[i][1] * mat[4 ]) +
					 (inVertexArray[i][2] * mat[8 ]) + mat[12];
			tcv[1] = (inVertexArray[i][0] * mat[1 ]) +
					 (inVertexArray[i][1] * mat[5 ]) +
					 (inVertexArray[i][2] * mat[9 ]) + mat[13];
			tcv[2] = (inVertexArray[i][0] * mat[2 ]) +
					 (inVertexArray[i][1] * mat[6 ]) +
					 (inVertexArray[i][2] * mat[10]) + mat[14];

			VectorNormalize (tcv, tcv);

			coords[0] += tcv[0] * 0.25;
			coords[1] += tcv[1] * 0.25;
		}
		else {
			if (rbSurface && (rbSurface->flags & SURF_DRAWTURB)) {
				coords[0] = (coords[0]+scroll) * ONEDIV64;
				coords[1] = coords[1] * ONEDIV64;
			}
			else {
				coords[0] += scroll;
			}
		}

		tempCoordArray[i][0] = coords[0]+scrollCoords[0];
		tempCoordArray[i][1] = coords[1]+scrollCoords[1];
	}

	return qTrue;
}


/*
=============
RB_ModifyVertexes
=============
*/
static qBool RB_ModifyVertexes (shaderStage_t *stage) {
	int		i;
	vec3_t	wv;
	float	scale, time;
	qBool	warpSmooth;

	qglVertexPointer (3, GL_FLOAT, 0, tempVertexArray);

	if (rbShader && rbSurface && rbShader->warpSmooth) {
		warpSmooth = qTrue;
		time = glState.realTime * rbShader->warpSpeed;
	}
	else {
		warpSmooth = qFalse;
		time = 0;
	}

	for (i=0 ; i<inNumVerts ; i++) {
		tempNormalsArray[i][0] = inNormalsArray[i][0];
		tempNormalsArray[i][1] = inNormalsArray[i][1];
		tempNormalsArray[i][2] = inNormalsArray[i][2];

		tempVertexArray[i][0] = inVertexArray[i][0];
		tempVertexArray[i][1] = inVertexArray[i][1];
		tempVertexArray[i][2] = inVertexArray[i][2];

		if (warpSmooth) {
			scale = rbShader->warpDist *
					sin (tempVertexArray[i][0]*rbShader->warpSmooth + time) *
					sin (tempVertexArray[i][1]*rbShader->warpSmooth + time) *
					sin (tempVertexArray[i][2]*rbShader->warpSmooth + time);

			VectorMA (tempVertexArray[i], scale, rbSurface->plane->normal, wv);
			tempVertexArray[i][0] = wv[0];
			tempVertexArray[i][1] = wv[1];
			tempVertexArray[i][2] = wv[2];
		}
	}

	return qTrue;
}


/*
=============
RB_BeginStage
=============
*/
static inline qBool RB_BeginStage (shaderStage_t *stage) {
	if (!stage)
		return qFalse;

	//
	// texture
	//
	if (stage->lightMapOnly) {
		if (rbTextures[1])
			rbTextures[0] = rbTextures[1];
		else
			rbTextures[0] = r_WhiteTexture;
		rbMultiTexture = qFalse;
	}
	else if (stage->animCount) {
		image_t *animImg = RS_Animate (stage);
		if (!animImg)
			return qFalse;
		else
			rbTextures[0] = animImg;
	}
	else if (!stage->texture) {
		stage = stage->next;
		return qFalse;
	}
	else
		rbTextures[0] = stage->texture;

	//
	// cubemap
	//
	if (stage->cubeMap) {
		qglEnable (GL_TEXTURE_CUBE_MAP_ARB);

		qglEnable (GL_TEXTURE_GEN_S);
		qglEnable (GL_TEXTURE_GEN_T);
		qglEnable (GL_TEXTURE_GEN_R);
		qglTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
		qglTexGeni (GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
		qglTexGeni (GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);

		rbEnableNormals = qTrue;

		glSpeeds.shaderCMPasses++;
	}

	//
	// alphamask
	//
	if (stage->alphaMask)
		qglEnable (GL_ALPHA_TEST);

	return qTrue;
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
static void RB_DrawElements (void) {
	if (!inIndexArray || !inNumVerts || !inNumIndexes)
		return;

	//
	// set
	//
	if (rbEnableNormals) {
		qglEnableClientState (GL_NORMAL_ARRAY);
		qglNormalPointer (GL_FLOAT, 12, tempNormalsArray);
	}

	GL_SelectTexture (0);
	GL_Bind (rbTextures[0]);
	qglEnableClientState (GL_TEXTURE_COORD_ARRAY);

	if (inNumColors > 1) {
		qglEnableClientState (GL_COLOR_ARRAY);
		qglColorPointer (4, GL_FLOAT, 0, tempColorArray);
	}
	else if (inNumColors == 1)
		qglColor4f (tempColorArray[0][0], tempColorArray[0][1], tempColorArray[0][2], tempColorArray[0][3]);

	GL_SelectTexture (1);
	if (rbMultiTexture) {
		qglEnable (GL_TEXTURE_2D);
		if (gl_lightmap->integer)
			GL_TexEnv (GL_REPLACE);
		else 
			GL_TexEnv (GL_MODULATE);

		GL_Bind (rbTextures[1]);
		qglEnableClientState (GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer (2, GL_FLOAT, 0, inLmCoordArray);
	}
	else {
		qglDisable (GL_TEXTURE_2D);
		GL_TexEnv (GL_MODULATE);

		qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
	}

	//
	// draw
	//
	if (glConfig.extDrawRangeElements
	&& (inNumVerts < glConfig.maxElementVerts)
	&& (inNumIndexes < glConfig.maxElementIndices)) {
		qglDrawRangeElementsEXT (GL_TRIANGLES, 0, inNumVerts, inNumIndexes, GL_UNSIGNED_INT, inIndexArray);
	}
	else {
		qglDrawElements (GL_TRIANGLES, inNumIndexes, GL_UNSIGNED_INT, inIndexArray);
	}

	//
	// reset
	//
	if (rbMultiTexture) {
		GL_SelectTexture (1);
		qglDisable (GL_TEXTURE_2D);
		GL_TexEnv (GL_MODULATE);

		qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
		rbMultiTexture = qFalse;
	}

	GL_SelectTexture (0);
	qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
	if (rbEnableNormals) {
		qglDisableClientState (GL_NORMAL_ARRAY);
		rbEnableNormals = qFalse;
	}

	if (inNumColors > 1)
		qglDisableClientState (GL_COLOR_ARRAY);

	qglDisable (GL_TEXTURE_GEN_S);
	qglDisable (GL_TEXTURE_GEN_T);
	qglDisable (GL_TEXTURE_GEN_R);
	qglDisable (GL_TEXTURE_CUBE_MAP_ARB);
	qglDisable (GL_ALPHA_TEST);
	qglDisable (GL_BLEND);

	glSpeeds.rbVerts += inNumVerts;
	glSpeeds.rbTris += inNumIndexes/3;
	glSpeeds.rbNumElements++;
}


/*
=============
RB_DrawNormals
=============
*/
static inline void RB_DrawNormals (void) {
	int		i;

	if (!inNumVerts || !inVertexArray || !inNormalsArray)
		return;

	qglBegin (GL_LINES);
	for (i=0 ; i<inNumVerts ; i++) {
		qglVertex3f(inVertexArray[i][0],
					inVertexArray[i][1],
					inVertexArray[i][2]);
		qglVertex3f(inVertexArray[i][0] + inNormalsArray[i][0]*2,
					inVertexArray[i][1] + inNormalsArray[i][1]*2,
					inVertexArray[i][2] + inNormalsArray[i][2]*2);
	}
	qglEnd ();
}


/*
=============
RB_DrawTriangles
=============
*/
static inline void RB_DrawTriangles (void) {
	int		i;

	if (!inNumIndexes || !inIndexArray)
		return;

	for (i=0 ; i<inNumIndexes ; i+=3) {
		qglBegin (GL_LINE_STRIP);
		qglArrayElement (inIndexArray[i]);
		qglArrayElement (inIndexArray[i+1]);
		qglArrayElement (inIndexArray[i+2]);
		qglArrayElement (inIndexArray[i]);
		qglEnd ();
	}
}


/*
=============
RB_RenderMesh
=============
*/
void RB_RenderMesh (rbMesh_t *mesh) {
	if (!mesh->numColors || !mesh->numIndexes || !mesh->numVerts || !mesh->indexArray)
		return;

	// check values
	if (mesh->numColors > VERTARRAY_MAX_VERTS)
		Com_Error (ERR_FATAL, "RB_RenderMesh: numColors(%i) > VERTARRAY_MAX_VERTS(%i)", mesh->numColors, VERTARRAY_MAX_VERTS);
	if (mesh->numIndexes > VERTARRAY_MAX_INDEXES)
		Com_Error (ERR_FATAL, "RB_RenderMesh: numIndexes(%i) > VERTARRAY_MAX_INDEXES(%i)", mesh->numIndexes, VERTARRAY_MAX_INDEXES);
	if (mesh->numVerts > VERTARRAY_MAX_VERTS)
		Com_Error (ERR_FATAL, "RB_RenderMesh: numVerts(%i) > VERTARRAY_MAX_VERTS(%i)", mesh->numVerts, VERTARRAY_MAX_VERTS);

	// copy values
	inNumColors = mesh->numColors;
	inNumIndexes = mesh->numIndexes;
	inNumVerts = mesh->numVerts;

	inColorArray = mesh->colorArray;
	inCoordArray = mesh->coordArray;
	inLmCoordArray = mesh->lmCoordArray;
	inIndexArray = mesh->indexArray;
	inNormalsArray = mesh->normalsArray;
	inVertexArray = mesh->vertexArray;

	rbSurface = mesh->surface;

	rbShader = mesh->shader;

	rbTextures[1] = mesh->lmTexture;

	if (glConfig.extArbMultitexture || glConfig.extSGISMultiTexture) {
		if (mesh->lmTexture) {
			rbMultiTexture = qTrue;
		}
	}

	// start
	if (!r_shaders->integer || !rbShader) {
		if (mesh->texture) { // FIXME: default to r_NoTexture?
			if (rbMultiTexture)
				RB_LockArrays (mesh->numVerts);

			rbTextures[0] = mesh->texture;

			// begin
			RB_BeginStage (NULL);

			// modify
			RB_ModifyVertexes (NULL);
			RB_ModifyColor (NULL);
			RB_ModifyTextureCoords (NULL, 0);

			// flush
			RB_DrawElements ();

			if (rbMultiTexture)
				RB_UnlockArrays ();
		}
	}
	else {
		shaderStage_t	*stage;
		int				stageNum;

		RB_LockArrays (mesh->numVerts);

		for (stageNum=0, stage=rbShader->stage ; stage ; stage=stage->next, stageNum++) {
			// begin
			if (!RB_BeginStage (stage))
				continue;

			// modify
			if (!RB_ModifyVertexes (stage) ||
				!RB_ModifyColor (stage) ||
				!RB_ModifyTextureCoords (stage, stageNum))
				continue;

			if (stageNum > glConfig.maxTexUnits)
				stageNum = 0;

			// flush
			RB_DrawElements ();

			glSpeeds.shaderPasses++;
		}

		glSpeeds.shaderCount++;
	}

	// render lightmap if not in multitexture
	if (!(glConfig.extArbMultitexture || glConfig.extSGISMultiTexture)) {
		if (mesh->lmTexture) {
			// don't bother writing Z
			qglDepthMask (GL_FALSE);

			// set the appropriate blending mode unless we're only looking at the lightmaps
			if (!gl_lightmap->value) {
				qglEnable (GL_BLEND);

				if (gl_saturatelighting->value)
					GL_BlendFunc (GL_ONE, GL_ONE);
				else
					GL_BlendFunc (GL_ZERO, GL_SRC_COLOR);
			}

			rbTextures[0] = rbTextures[1];

			qglTexCoordPointer (2, GL_FLOAT, 0, mesh->lmCoordArray);

			RB_DrawElements ();

			// restore state
			qglDisable (GL_BLEND);
			qglDepthMask (GL_TRUE);
		}
	}

	RB_UnlockArrays ();
	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//
	// lame surface type shaders
	//
	if (rbSurface && mesh->lmTexture && (rbSurface->flags & SURF_UNDERWATER)
	&& (mesh->shader != glMedia.worldLavaCaustics)
	&& (mesh->shader != glMedia.worldSlimeCaustics)
	&& (mesh->shader != glMedia.worldWaterCaustics)
	&& r_shaders->integer && r_caustics->integer) {
		rbMesh_t	tempMesh;

		qglDepthMask (GL_FALSE); // FIXME

		// caustics
		tempMesh.shader = NULL;
		if (rbSurface->flags & SURF_UNDERWATER) {
			if ((rbSurface->flags & SURF_LAVA) && glMedia.worldLavaCaustics)
				tempMesh.shader = glMedia.worldLavaCaustics;
			else if ((rbSurface->flags & SURF_SLIME) && glMedia.worldSlimeCaustics)
				tempMesh.shader = glMedia.worldSlimeCaustics;
			else if (glMedia.worldWaterCaustics)
				tempMesh.shader = glMedia.worldWaterCaustics;
		}

		tempMesh.numColors = inNumColors;
		tempMesh.numIndexes = inNumIndexes;
		tempMesh.numVerts = inNumVerts;

		tempMesh.colorArray = inColorArray;
		tempMesh.coordArray = inCoordArray;
		tempMesh.lmCoordArray = inLmCoordArray;
		tempMesh.indexArray = inIndexArray;
		tempMesh.normalsArray = inNormalsArray;
		tempMesh.vertexArray = inVertexArray;

		tempMesh.surface = rbSurface;

		tempMesh.texture = NULL;
		tempMesh.lmTexture = mesh->lmTexture;

		if (tempMesh.shader)
			RB_RenderMesh (&tempMesh);

		qglDepthMask (GL_TRUE);  // FIXME
	}

	if (gl_showtris->integer || gl_shownormals->integer) {
		RB_LockArrays (mesh->numVerts);

		qglDepthMask (GL_FALSE);
		qglDisable (GL_DEPTH_TEST);
		qglDisable (GL_TEXTURE_2D);
		qglColor4f (1, 1, 1, 1);

		if (gl_showtris->integer)
			RB_DrawTriangles ();
		if (gl_shownormals->integer)
			RB_DrawNormals ();

		qglEnable (GL_TEXTURE_2D);
		qglEnable (GL_DEPTH_TEST);
		qglDepthMask (GL_TRUE);

		RB_UnlockArrays ();
	}
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
void RB_BeginFrame (void) {
}


/*
=============
RB_EndFrame

Finishes frame backend operations
Goes into 2D mode
=============
*/
#define DRAWRIGHT(x) Draw_String(NULL, vidDef.width - (strlen (string) * FT_SIZE) - 2,	\
					vidDef.height - ((FT_SIZE + FT_SHAOFFSET) * vOffset++),				\
					FS_SHADOW, FT_SCALE, string, (x));

void RB_EndFrame (void) {
	GL_Setup2D ();

	if (r_speeds->integer && r_WorldModel && !(r_RefDef.rdFlags & RDF_NOWORLDMODEL)) {
		char	string[128];
		int		vOffset = 9;

		//
		// totals
		//
		Q_snprintfz (string, sizeof (string), "%4i elements", glSpeeds.rbNumElements);		DRAWRIGHT(colorCyan);
		Q_snprintfz (string, sizeof (string), "%6i verts", glSpeeds.rbVerts);				DRAWRIGHT(colorCyan);
		Q_snprintfz (string, sizeof (string), "%5i tris", glSpeeds.rbTris);					DRAWRIGHT(colorCyan);

		Q_snprintfz (string, sizeof (string), "Totals");			DRAWRIGHT(colorYellow);
		vOffset++;

		//
		// image system
		//
		Q_snprintfz (string, sizeof (string), "%3i unitChanges", glSpeeds.texUnitChanges);	DRAWRIGHT(colorCyan);
		Q_snprintfz (string, sizeof (string), "%3i envChanges", glSpeeds.texEnvChanges);	DRAWRIGHT(colorCyan);
		Q_snprintfz (string, sizeof (string), "%4i binds", glSpeeds.textureBinds);			DRAWRIGHT(colorCyan);

		Q_snprintfz (string, sizeof (string), "Texture");			DRAWRIGHT(colorYellow);
		vOffset++;

		//
		// shader specific
		//
		if (r_shaders && r_shaders->integer) {
			Q_snprintfz (string, sizeof (string), "%1i cube", glSpeeds.shaderCMPasses);		DRAWRIGHT(colorCyan);
			Q_snprintfz (string, sizeof (string), "%5i passes", glSpeeds.shaderPasses);		DRAWRIGHT(colorCyan);
			Q_snprintfz (string, sizeof (string), "%4i shaders", glSpeeds.shaderCount);		DRAWRIGHT(colorCyan);

			Q_snprintfz (string, sizeof (string), "Shaders");		DRAWRIGHT(colorYellow);
			vOffset++;
		}

		//
		// world
		//
		Q_snprintfz (string, sizeof (string), "%2i dlights", glSpeeds.numDLights);			DRAWRIGHT(colorCyan);
		Q_snprintfz (string, sizeof (string), "%2i skyPoly", glSpeeds.skyPolys);			DRAWRIGHT(colorCyan);
		Q_snprintfz (string, sizeof (string), "%3i leaf", glSpeeds.worldLeafs);				DRAWRIGHT(colorCyan);
		Q_snprintfz (string, sizeof (string), "%5i worldPoly", glSpeeds.worldPolys);		DRAWRIGHT(colorCyan);

		Q_snprintfz (string, sizeof (string), "World");				DRAWRIGHT(colorYellow);
		vOffset++;

		//
		// particle/decal
		//
		Q_snprintfz (string, sizeof (string), "%4i decals", glSpeeds.numDecals);			DRAWRIGHT(colorCyan);
		Q_snprintfz (string, sizeof (string), "%5i particles", glSpeeds.numParticles);		DRAWRIGHT(colorCyan);

		Q_snprintfz (string, sizeof (string), "Particle/Decal");	DRAWRIGHT(colorYellow);
		vOffset++;

		//
		// alias
		//
		Q_snprintfz (string, sizeof (string), "%2i visible", glSpeeds.visEntities);			DRAWRIGHT(colorCyan);
		Q_snprintfz (string, sizeof (string), "%2i total", glSpeeds.numEntities);			DRAWRIGHT(colorCyan);
		Q_snprintfz (string, sizeof (string), "%5i epoly", glSpeeds.aliasPolys);			DRAWRIGHT(colorCyan);

		Q_snprintfz (string, sizeof (string), "Alias Models");		DRAWRIGHT(colorYellow);

		// reset
		memset (&glSpeeds, 0, sizeof (glSpeeds));
	}
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
void RB_Init (void) {
	int		i;
	index_t	*index;

	qglEnableClientState (GL_VERTEX_ARRAY);

	RB_ResetPointers ();

	rbArraysLocked = qFalse;

	// generate trifan indices
	index = rbTriFanIndices;
	for (i=0 ; i<VERTARRAY_MAX_VERTS-2 ; i++) {
		index[0] = 0;
		index[1] = i + 1;
		index[2] = i + 2;

		index+=3;
	}
}


/*
=============
RB_Shutdown
=============
*/
void RB_Shutdown (qBool full) {
}
