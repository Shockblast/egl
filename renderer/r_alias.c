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
// r_alias.c
// Alias model rendering (MD2 and MD3)
//

#include "r_local.h"

static shader_t			*alias_meshShader;
static image_t			*alias_meshSkin;

static mAliasModel_t	*alias_Model;
static mAliasMesh_t		alias_Mesh;
static mAliasFrame_t	*alias_Frame;
static mAliasFrame_t	*alias_OldFrame;

static vec3_t			alias_modMins;
static vec3_t			alias_modMaxs;
static float			alias_modRadius;

static qBool			alias_hasShell;
static qBool			alias_hasShadow;
static vec3_t			alias_shadowSpot;

/*
===============================================================================

	ALIAS RENDERING

===============================================================================
*/

/*
=============
R_DrawAliasShell
=============
*/
void R_DrawAliasShell (shader_t *shader) {
	int				i, j;
	vec3_t			diff;
	shaderStage_t	*stage;

	if (shader && r_shaders->integer) {
		float	alpha, rsalpha;
		vec2_t	coords, scrollCoords;
		image_t	*stageImage;

		alpha = r_CurrEntity->color[3];
		stage = shader->stage;
		if (alias_meshShader && alias_meshShader->noShell)
			return;

		do {
			if (stage->lightMapOnly)
				stageImage = r_WhiteTexture;
			else if (stage->animCount) {
				image_t		*animImg = RS_Animate (stage);
				if (!animImg)
					continue;
				else
					stageImage = animImg;
			} else if (!stage->texture)
				continue;
			else
				stageImage = stage->texture;
			GL_Bind (0, stageImage);

			RS_Scroll (stage, scrollCoords);
			RS_BlendFunc (stage, (r_CurrEntity->flags&RF_TRANSLUCENT) ? qTrue : qFalse, (r_CurrEntity->flags&RF_TRANSLUCENT) ? qFalse : qTrue);
			RS_AlphaShift (stage, &rsalpha);

			alpha = rsalpha * alpha;
			if (alpha <= 0)
				continue;

			if ((alpha < 1) && !(r_CurrEntity->flags & RF_TRANSLUCENT))
				qglDepthMask (GL_FALSE);

			if (stage->alphaMask)
				qglEnable (GL_ALPHA_TEST);

			if (stage->cubeMap) {
				qglEnable (GL_TEXTURE_CUBE_MAP_ARB);

				qglEnable (GL_TEXTURE_GEN_S);
				qglEnable (GL_TEXTURE_GEN_T);
				qglEnable (GL_TEXTURE_GEN_R);
				qglTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
				qglTexGeni (GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
				qglTexGeni (GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);

				glSpeeds.shaderCMPasses++;
			}

			qglBegin (GL_TRIANGLES);
			for(i=0 ; i<alias_Mesh.numTris ; i++) {
				for (j=0 ; j<3 ; j++) {
					if (stage->lightMap || stage->lightMapOnly) {
						qglColor4f (colorArray[alias_Mesh.indexes[3*i+j]][0],
									colorArray[alias_Mesh.indexes[3*i+j]][1],
									colorArray[alias_Mesh.indexes[3*i+j]][2],
									alpha);
					} else
						qglColor4f (1, 1, 1, alpha);

					coords[0] = coordArray[alias_Mesh.indexes[3*i+j]][0];
					coords[1] = coordArray[alias_Mesh.indexes[3*i+j]][1];

					if (stage->envMap) {
						VectorAdd (r_CurrEntity->origin, vertexArray[alias_Mesh.indexes[3*i+j]], diff);
						RS_SetEnvmap (r_CurrEntity->origin, r_CurrEntity->axis, vertexArray[alias_Mesh.indexes[3*i+j]], normalsArray[alias_Mesh.indexes[3*i+j]], coords);
					}

					RS_SetTexcoords2D (stage, coords);

					qglTexCoord2f (coords[0]+scrollCoords[0], coords[1]+scrollCoords[1]);
					qglNormal3fv (normalsArray[alias_Mesh.indexes[3*i+j]]);
					qglVertex3fv (vertexArray[alias_Mesh.indexes[3*i+j]]);
				}
			}
			qglEnd ();

			if (stage->cubeMap) {
				qglDisable (GL_TEXTURE_GEN_S);
				qglDisable (GL_TEXTURE_GEN_T);
				qglDisable (GL_TEXTURE_GEN_R);

				qglDisable (GL_TEXTURE_CUBE_MAP_ARB);
			}

			if (stage->alphaMask)
				qglDisable (GL_ALPHA_TEST);

			if (!(r_CurrEntity->flags&RF_TRANSLUCENT)) {
				qglDisable (GL_BLEND);
				if (alpha < 1)
					qglDepthMask (GL_TRUE);
			}

			glSpeeds.aliasPolys += alias_Mesh.numTris;
			glSpeeds.shaderPasses++;

			stage = stage->next;
		} while (stage);
		glSpeeds.shaderCount++;
	} else {
		if (!(r_CurrEntity->flags&RF_TRANSLUCENT)) {
			qglEnable (GL_BLEND);
			qglDepthMask (GL_FALSE);
		}

		GL_Bind (0, glMedia.model_ShellTexture);
		GL_BlendFunc (GL_SRC_ALPHA, GL_ONE);

		qglBegin (GL_TRIANGLES);
		for(i=0 ; i<alias_Mesh.numTris ; i++) {
			for (j=0 ; j<3 ; j++) {
				qglTexCoord2f (coordArray[alias_Mesh.indexes[3*i+j]][0] - (r_RefDef.time * 0.5),
							coordArray[alias_Mesh.indexes[3*i+j]][1] - (r_RefDef.time * 0.5));
				qglNormal3fv (normalsArray[alias_Mesh.indexes[3*i+j]]);
				qglVertex3fv (vertexArray[alias_Mesh.indexes[3*i+j]]);
			}
		}
		qglEnd ();

		if (!(r_CurrEntity->flags&RF_TRANSLUCENT)) {
			qglDisable (GL_BLEND);
			qglDepthMask (GL_TRUE);
		}

		glSpeeds.aliasPolys += alias_Mesh.numTris;
	}
}


/*
=============
R_DrawAliasFrameLerp
=============
*/
void R_DrawAliasFrameLerp (void) {
	float			alpha;
	int				i, j;
	vec3_t			diff;
	shaderStage_t	*stage;

	alpha = r_CurrEntity->color[3];
	if (!alias_meshShader || !r_shaders->integer) {
		GL_Bind (0, alias_meshSkin);

		qglBegin (GL_TRIANGLES);
		for(i=0 ; i<alias_Mesh.numTris ; i++) {
			for (j=0 ; j<3 ; j++) {
				qglColor4f (colorArray[alias_Mesh.indexes[3*i+j]][0],
							colorArray[alias_Mesh.indexes[3*i+j]][1],
							colorArray[alias_Mesh.indexes[3*i+j]][2],
							alpha);
				qglTexCoord2fv (coordArray[alias_Mesh.indexes[3*i+j]]);
				qglNormal3fv (normalsArray[alias_Mesh.indexes[3*i+j]]);
				qglVertex3fv (vertexArray[alias_Mesh.indexes[3*i+j]]);
			}
		}
		qglEnd ();

		glSpeeds.aliasPolys += alias_Mesh.numTris;
	} else {
		float	rsalpha;
		vec2_t	coords, scrollCoords;
		image_t	*stageImage;

		stage = alias_meshShader->stage;
		do {
			if (stage->lightMapOnly)
				stageImage = r_WhiteTexture;
			else if (stage->animCount) {
				image_t		*animImg = RS_Animate (stage);
				if (!animImg)
					continue;
				else
					stageImage = animImg;
			} else if (!stage->texture)
				continue;
			else
				stageImage = stage->texture;
			GL_Bind (0, stageImage);

			RS_Scroll (stage, scrollCoords);
			RS_BlendFunc (stage, (r_CurrEntity->flags&RF_TRANSLUCENT) ? qTrue : qFalse, (r_CurrEntity->flags&RF_TRANSLUCENT) ? qFalse : qTrue);
			RS_AlphaShift (stage, &rsalpha);

			alpha = rsalpha * alpha;
			if (alpha <= 0)
				continue;

			if ((alpha < 1) && !(r_CurrEntity->flags&RF_TRANSLUCENT))
				qglDepthMask (GL_FALSE);

			if (stage->alphaMask)
				qglEnable (GL_ALPHA_TEST);

			if (stage->cubeMap) {
				qglEnable (GL_TEXTURE_CUBE_MAP_ARB);

				qglEnable (GL_TEXTURE_GEN_S);
				qglEnable (GL_TEXTURE_GEN_T);
				qglEnable (GL_TEXTURE_GEN_R);
				qglTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
				qglTexGeni (GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
				qglTexGeni (GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);

				glSpeeds.shaderCMPasses++;
			}

			qglBegin (GL_TRIANGLES);
			for(i=0 ; i<alias_Mesh.numTris ; i++) {
				for (j=0 ; j<3 ; j++) {
					if (stage->lightMap || stage->lightMapOnly) {
						qglColor4f (colorArray[alias_Mesh.indexes[3*i+j]][0],
									colorArray[alias_Mesh.indexes[3*i+j]][1],
									colorArray[alias_Mesh.indexes[3*i+j]][2],
									alpha);
					} else
						qglColor4f (1, 1, 1, alpha);

					coords[0] = coordArray[alias_Mesh.indexes[3*i+j]][0];
					coords[1] = coordArray[alias_Mesh.indexes[3*i+j]][1];

					if (stage->envMap) {
						VectorAdd (r_CurrEntity->origin, vertexArray[alias_Mesh.indexes[3*i+j]], diff);
						RS_SetEnvmap (r_CurrEntity->origin, r_CurrEntity->axis, vertexArray[alias_Mesh.indexes[3*i+j]], normalsArray[alias_Mesh.indexes[3*i+j]], coords);
					}

					RS_SetTexcoords2D (stage, coords);

					qglTexCoord2f (coords[0]+scrollCoords[0], coords[1]+scrollCoords[1]);
					qglNormal3fv (normalsArray[alias_Mesh.indexes[3*i+j]]);
					qglVertex3fv (vertexArray[alias_Mesh.indexes[3*i+j]]);
				}
			}
			qglEnd ();

			if (stage->cubeMap) {
				qglDisable (GL_TEXTURE_GEN_S);
				qglDisable (GL_TEXTURE_GEN_T);
				qglDisable (GL_TEXTURE_GEN_R);

				qglDisable (GL_TEXTURE_CUBE_MAP_ARB);
			}

			if (stage->alphaMask)
				qglDisable (GL_ALPHA_TEST);

			if (!(r_CurrEntity->flags&RF_TRANSLUCENT)) {
				qglDisable (GL_BLEND);
				if (alpha < 1)
					qglDepthMask (GL_TRUE);
			}

			glSpeeds.aliasPolys += alias_Mesh.numTris;
			glSpeeds.shaderPasses++;

			stage = stage->next;
		} while (stage);
		glSpeeds.shaderCount++;
	}

	// shell rendering
	if (!alias_hasShell) {
		GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		return;
	}

	// IR SHELL
	if ((r_RefDef.rdFlags & RDF_IRGOGGLES) && (r_CurrEntity->flags & RF_IR_VISIBLE)) {
		if (glMedia.model_ShellIR) {
			R_DrawAliasShell (glMedia.model_ShellIR);
		} else {
			qglColor4f (1, 0, 0, 0.75 * alpha);
			R_DrawAliasShell (NULL);
		}
	} else {
		// RF_SHELL_DOUBLE
		if (r_CurrEntity->flags & RF_SHELL_DOUBLE) {
			if (glMedia.model_ShellDouble) {
				R_DrawAliasShell (glMedia.model_ShellDouble);
			} else {
				qglColor4f (0.9, 0.7, 0, 0.75 * alpha);
				R_DrawAliasShell (NULL);
			}
		}

		// RF_SHELL_HALF_DAM
		if (r_CurrEntity->flags & RF_SHELL_HALF_DAM) {
			if (glMedia.model_ShellHalfDam) {
				R_DrawAliasShell (glMedia.model_ShellHalfDam);
			} else {
				qglColor4f (0.56, 0.59, 0.45, 0.75 * alpha);
				R_DrawAliasShell (NULL);
			}
		}

		// GOD MODE
		if ((r_CurrEntity->flags & RF_SHELL_RED) && (r_CurrEntity->flags & RF_SHELL_GREEN) && (r_CurrEntity->flags & RF_SHELL_BLUE)) {
			if (glMedia.model_ShellGod) {
				R_DrawAliasShell (glMedia.model_ShellGod);
			} else {
				qglColor4f (1, 1, 1, 0.75 * alpha);
				R_DrawAliasShell (NULL);
			}
		} else {
			// RF_SHELL_RED
			if (r_CurrEntity->flags & RF_SHELL_RED) {
				if (glMedia.model_ShellRed) {
					R_DrawAliasShell (glMedia.model_ShellRed);
				} else {
					qglColor4f (1, 0, 0, 0.75 * alpha);
					R_DrawAliasShell (NULL);
				}
			}

			// RF_SHELL_GREEN
			if (r_CurrEntity->flags & RF_SHELL_GREEN) {
				if (glMedia.model_ShellGreen) {
					R_DrawAliasShell (glMedia.model_ShellGreen);
				} else {
					qglColor4f (0, 1, 0, 0.75 * alpha);
					R_DrawAliasShell (NULL);
				}
			}

			// RF_SHELL_BLUE
			if (r_CurrEntity->flags & RF_SHELL_BLUE) {
				if (glMedia.model_ShellBlue) {
					R_DrawAliasShell (glMedia.model_ShellBlue);
				} else {
					qglColor4f (0, 0, 1, 0.75 * alpha);
					R_DrawAliasShell (NULL);
				}
			}
		}
	}

	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

/*
===============================================================================

	ALIAS SHADOWS

===============================================================================
*/

/*
=============
R_DrawAliasShadow
=============
*/
// kthx jesus christ this code is ugly
void R_DrawAliasShadow (void) {
	float	height, dist, alpha;
	vec3_t	newPoint;
	int		i, j;

	// where to put it
	height = (r_CurrEntity->origin[2] - alias_shadowSpot[2]) * -1;

	// distance
	VectorCopy (r_CurrEntity->origin, newPoint);
	newPoint[2] = height;

	dist = clamp (VectorDistance (r_CurrEntity->origin, newPoint), 0, 401);
	if (dist > 400)
		return;

	// scaling
	alpha = ((405 - dist) / 800.0);
	qglScalef ((alpha + 0.69), (alpha + 0.69), 1);

	// coloring
	qglColor4f (0, 0, 0, alpha);

	qglBegin (GL_TRIANGLES);
	for(i=0 ; i<alias_Mesh.numTris ; i++) {
		for (j=0 ; j<3 ; j++) {
			qglVertex3f(vertexArray[alias_Mesh.indexes[3*i+j]][0],
						vertexArray[alias_Mesh.indexes[3*i+j]][1],
						height);
		}
	}
	qglEnd ();

	glSpeeds.aliasPolys += alias_Mesh.numTris;
}

/*
===============================================================================

	ALIAS PROCESSING

===============================================================================
*/

/*
=============
R_AliasLatLongToNorm
=============
*/
void R_AliasLatLongToNorm (byte latlong[2], vec3_t out) {
	float sin_a, sin_b, cos_a, cos_b;

	sin_a = (float)latlong[0] * ONEDIV255;
	cos_a = sin (sin_a + 0.25);
	sin_a = sin (sin_a);
	sin_b = (float)latlong[1] * ONEDIV255;
	cos_b = sin (sin_b + 0.25);
	sin_b = sin (sin_b);

	VectorSet (out, cos_b * sin_a, sin_b * sin_a, cos_a);
}


/*
=============
R_AliasModelBBox
=============
*/
void R_AliasModelBBox (void) {
	int				i;
	float			*thisMins, *oldMins;
	float			*thisMaxs, *oldMaxs;

	// compute axially aligned mins and maxs
	if (alias_Frame == alias_OldFrame) {
		VectorCopy (alias_Frame->mins, alias_modMins);
		VectorCopy (alias_Frame->maxs, alias_modMaxs);
		alias_modRadius = alias_Frame->radius;
	} else {
		thisMins = alias_Frame->mins;
		thisMaxs = alias_Frame->maxs;

		oldMins  = alias_OldFrame->mins;
		oldMaxs  = alias_OldFrame->maxs;

		for (i=0 ; i<3 ; i++) {
			alias_modMins[i] = min (thisMins[i], oldMins[i]);
			alias_modMaxs[i] = max (thisMaxs[i], oldMaxs[i]);
		}

		alias_modRadius = RadiusFromBounds (thisMins, thisMaxs);
	}

	if (r_CurrEntity->scale != 1.0f) {
		VectorScale (alias_modMins, r_CurrEntity->scale, alias_modMins);
		VectorScale (alias_modMaxs, r_CurrEntity->scale, alias_modMaxs);
		alias_modRadius *= r_CurrEntity->scale;
	}
}


/*
=============
R_CullAliasModel
=============
*/
qBool R_CullAliasModel (void) {
	if (r_CurrEntity->flags & RF_WEAPONMODEL)
		return qFalse;
	if (r_CurrEntity->flags & RF_VIEWERMODEL)
		return qFalse;

	if (!Matrix_Compare (r_CurrEntity->axis, axisIdentity)) {
		if (R_CullSphere (r_CurrEntity->origin, alias_modRadius))
			return qTrue;
	} else {
		vec3_t	bbox[8];
		int		i, p, f, aggregateMask = ~0;

		// compute and rotate a full bounding box
		for (i=0 ; i<8 ; i++) {
			vec3_t tmp;

			tmp[0] = ((i & 1) ? alias_modMins[0] : alias_modMaxs[0]);
			tmp[1] = ((i & 2) ? alias_modMins[1] : alias_modMaxs[1]);
			tmp[2] = ((i & 4) ? alias_modMins[2] : alias_modMaxs[2]);

			Matrix_TransformVector (r_CurrEntity->axis, tmp, bbox[i]);
			bbox[i][0] += r_CurrEntity->origin[0];
			bbox[i][1] = -bbox[i][1] + r_CurrEntity->origin[1];
			bbox[i][2] += r_CurrEntity->origin[2];
		}

		for (p=0 ; p<8 ; p++) {
			int mask = 0;
			
			for (f=0 ; f<4 ; f++) {
				if (DotProduct (r_Frustum[f].normal, bbox[p]) < r_Frustum[f].dist)
					mask |= (1 << f);
			}

			aggregateMask &= mask;
		}

		if (aggregateMask)
			return qTrue;

		return qFalse;
	}

	return qFalse;
}


/*
=============
R_AliasModelSkin
=============
*/
void R_AliasModelSkin (void) {
	// find the skin for this mesh
	if (r_CurrEntity->skin) {
		// custom player skin
		alias_meshSkin = r_CurrEntity->skin;
		alias_meshShader = RS_FindShader (r_CurrEntity->skin->name);
	} else {
		if (r_CurrEntity->skinNum >= MD2_MAX_SKINS) {
			// server's only send MD2 skinNums anyways
			alias_meshSkin = alias_Mesh.skins[0].image;
			alias_meshShader = alias_Mesh.skins[0].shader;
		} else if ((r_CurrEntity->skinNum >= 0) && (r_CurrEntity->skinNum < alias_Mesh.numSkins)) {
			alias_meshSkin = alias_Mesh.skins[r_CurrEntity->skinNum].image;
			alias_meshShader = alias_Mesh.skins[r_CurrEntity->skinNum].shader;

			if (!alias_meshSkin) {
				alias_meshSkin = alias_Mesh.skins[0].image;
				alias_meshShader = alias_Mesh.skins[0].shader;
			}
		} else {
			alias_meshSkin = alias_Mesh.skins[0].image;
			alias_meshShader = alias_Mesh.skins[0].shader;
		}
	}

	if (!alias_meshSkin) {
		alias_meshSkin = r_NoTexture;
		alias_meshShader = 0;
	}
}


/*
=============
R_DrawAliasModel
=============
*/
void R_DrawAliasModel (entity_t *ent) {
	mAliasVertex_t	*verts, *oldVerts;
	vec3_t			move, delta;
	vec3_t			normal, oldNormal;
	vec3_t			back, front;
	float			frontLerp, backLerp;
	int				meshNum;
	int				i, j;

	alias_Model = ent->model->aliasModel;

	alias_hasShell = (ent->flags & RF_SHELLMASK) ? qTrue : qFalse;
	alias_hasShadow = (!gl_shadows->integer || (ent->flags & RF_NOSHADOWMASK) || (r_RefDef.rdFlags & RDF_NOWORLDMODEL)) ? qFalse : qTrue;

	// sanity checks
	if ((ent->frame >= alias_Model->numFrames) || (ent->frame < 0)) {
		Com_Printf (PRNT_DEV, S_COLOR_YELLOW "R_DrawAliasModel: '%s' no such frame '%d'\n", ent->model->name, ent->frame);
		ent->frame = 0;
	}

	if ((ent->oldFrame >= alias_Model->numFrames) || (ent->oldFrame < 0)) {
		Com_Printf (PRNT_DEV, S_COLOR_YELLOW "R_DrawAliasModel: '%s' no such oldFrame '%d'\n", ent->model->name, ent->oldFrame);
		ent->oldFrame = 0;
	}

	alias_Frame = alias_Model->frames + ent->frame;
	alias_OldFrame = alias_Model->frames + ent->oldFrame;

	// culling
	R_AliasModelBBox ();
	if (!alias_hasShadow && R_CullAliasModel ())
		return;

	glSpeeds.numEntities++;

	// depth hacking
	if (ent->flags & RF_DEPTHHACK)
		qglDepthRange (0, 0.3);

	// flip it for lefty
	if (ent->flags & RF_CULLHACK)
		qglFrontFace (GL_CW);

	// interpolation calculations
	backLerp = ent->backLerp;
	if (!r_lerpmodels->integer)
		backLerp = 0;
	frontLerp = 1.0 - backLerp;

	VectorSubtract (ent->oldOrigin, ent->origin, delta);
	Matrix_TransformVector (ent->axis, delta, move);
	VectorAdd (move, alias_OldFrame->translate, move);

	for (i=0 ; i<3 ; i++) {
		move[i] = ((backLerp * move[i]) + (frontLerp * alias_Frame->translate[i])) * ent->scale;
		back[i] = (backLerp * alias_OldFrame->scale[i]) * ent->scale;
		front[i] = ((1.0f - backLerp) * alias_Frame->scale[i]) * ent->scale;
	}

	for (meshNum=0; meshNum<alias_Model->numMeshes; meshNum++) {
		alias_Mesh = alias_Model->meshes[meshNum];

		verts = alias_Mesh.vertexes + (ent->frame * alias_Mesh.numVerts);
		oldVerts = alias_Mesh.vertexes + (ent->oldFrame * alias_Mesh.numVerts);

		for (j=0 ; j<alias_Mesh.numVerts ; j++, verts++, oldVerts++) {
			coordArray[j][0] = alias_Mesh.coords[j][0];
			coordArray[j][1] = alias_Mesh.coords[j][1];

			R_AliasLatLongToNorm (verts->latlong, normal);
			R_AliasLatLongToNorm (oldVerts->latlong, oldNormal);

			normalsArray[j][0] = normal[0] + (oldNormal[0] - normal[0]) * backLerp;
			normalsArray[j][1] = normal[1] + (oldNormal[1] - normal[1]) * backLerp;
			normalsArray[j][2] = normal[2] + (oldNormal[2] - normal[2]) * backLerp;

			vertexArray[j][0] = move[0] + verts->point[0]*front[0] + oldVerts->point[0]*back[0];
			vertexArray[j][1] = move[1] + verts->point[1]*front[1] + oldVerts->point[1]*back[1];
			vertexArray[j][2] = move[2] + verts->point[2]*front[2] + oldVerts->point[2]*back[2];
		}

		// lighting
		R_LightForEntity (ent, alias_Mesh.numVerts, alias_hasShell, alias_hasShadow, alias_shadowSpot);

		// only needed to lerp yourself for your shadow
		if (ent->flags & RF_VIEWERMODEL)
			continue;

		// find the skin and shader
		R_AliasModelSkin ();

		// draw the model
		R_DrawAliasFrameLerp ();
	}

	// flip it for lefty
	if (ent->flags & RF_CULLHACK)
		qglFrontFace (GL_CCW);

	// depth hacking
	if (ent->flags & RF_DEPTHHACK)
		qglDepthRange (0, 1);

	// throw in a shadow
	if (alias_hasShadow) {
		R_RotateForAliasShadow (ent);
		for (meshNum=0 ; meshNum<alias_Model->numMeshes ; meshNum++) {
			alias_Mesh = alias_Model->meshes[meshNum];

			// find the skin and shader
			R_AliasModelSkin ();

			if (alias_meshShader && alias_meshShader->noShadow)
				continue;
			
			qglDisable (GL_TEXTURE_2D);
			qglEnable (GL_POLYGON_OFFSET_FILL);

			if (!(ent->flags & RF_TRANSLUCENT)) {
				qglDepthMask (GL_FALSE);
				qglEnable (GL_BLEND);
			}

			if (glConfig.useStencil && (gl_shadows->integer > 1)) {
				qglEnable (GL_STENCIL_TEST);
				qglStencilFunc (GL_EQUAL, 1, 2);
				qglStencilOp (GL_KEEP, GL_KEEP, GL_INCR);
			}

			// draw it
			R_DrawAliasShadow ();

			if (glConfig.useStencil && (gl_shadows->integer > 1))
				qglDisable (GL_STENCIL_TEST);

			if (!(ent->flags & RF_TRANSLUCENT)) {
				qglDepthMask (GL_TRUE);
				qglDisable (GL_BLEND);
			}

			qglDisable (GL_POLYGON_OFFSET_FILL);
			qglEnable (GL_TEXTURE_2D);
		}
	}
}
