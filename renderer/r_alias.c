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
// Alias model rendering
//

#include "r_local.h"

static shader_t			*aliasMeshShader;
static image_t			*aliasMeshSkin;

static mAliasModel_t	*aliasModel;
static mAliasMesh_t		aliasMesh;
static mAliasFrame_t	*aliasFrame;
static mAliasFrame_t	*aliasOldFrame;

static vec3_t			aliasModMins;
static vec3_t			aliasModMaxs;
static float			aliasModRadius;

static qBool			aliasHasShell;
static qBool			aliasHasShadow;
static vec3_t			aliasShadowSpot;

/*
===============================================================================

	ALIAS MODEL RENDERING

===============================================================================
*/

/*
=============
R_DrawAliasShell
=============
*/
void R_DrawAliasShell (shader_t *shader) {
	int				i, j;
	rbMesh_t		mesh;

	if (shader && r_shaders->integer) {
		mesh.numColors = aliasMesh.numVerts;
		mesh.numIndexes = aliasMesh.numTris*3;
		mesh.numVerts = aliasMesh.numVerts;

		mesh.colorArray = colorArray;
		mesh.coordArray = aliasMesh.coords;
		mesh.lmCoordArray = NULL;
		mesh.indexArray = aliasMesh.indexes;
		mesh.normalsArray = normalsArray;
		mesh.vertexArray = vertexArray;

		mesh.surface = NULL;

		mesh.texture = glMedia.modelShellTexture;
		mesh.lmTexture = NULL;
		mesh.shader = shader;

		RB_RenderMesh (&mesh);
	}
	else {
		if (!(rbCurrEntity->flags&RF_TRANSLUCENT)) {
			qglEnable (GL_BLEND);
			qglDepthMask (GL_FALSE);
		}

		GL_Bind (glMedia.modelShellTexture);
		GL_BlendFunc (GL_SRC_ALPHA, GL_ONE);

		qglBegin (GL_TRIANGLES);
		for (i=0 ; i<aliasMesh.numTris ; i++) {
			for (j=0 ; j<3 ; j++) {
				qglTexCoord2f (aliasMesh.coords[aliasMesh.indexes[3*i+j]][0] - (r_RefDef.time * 0.5),
							aliasMesh.coords[aliasMesh.indexes[3*i+j]][1] - (r_RefDef.time * 0.5));
				qglNormal3fv (normalsArray[aliasMesh.indexes[3*i+j]]);
				qglVertex3fv (vertexArray[aliasMesh.indexes[3*i+j]]);
			}
		}
		qglEnd ();

		if (!(rbCurrEntity->flags&RF_TRANSLUCENT)) {
			qglDisable (GL_BLEND);
			qglDepthMask (GL_TRUE);
		}
	}

	glSpeeds.aliasPolys += aliasMesh.numTris;
}


/*
=============
R_DrawAliasFrameLerp
=============
*/
void R_DrawAliasFrameLerp (void) {
	rbMesh_t		mesh;

	mesh.numColors = aliasMesh.numVerts;
	mesh.numIndexes = aliasMesh.numTris*3;
	mesh.numVerts = aliasMesh.numVerts;

	mesh.colorArray = colorArray;
	mesh.coordArray = aliasMesh.coords;
	mesh.lmCoordArray = NULL;
	mesh.indexArray = aliasMesh.indexes;
	mesh.normalsArray = normalsArray;
	mesh.vertexArray = vertexArray;

	mesh.surface = NULL;

	mesh.texture = aliasMeshSkin;
	mesh.lmTexture = NULL;
	mesh.shader = aliasMeshShader;

	RB_RenderMesh (&mesh);

	glSpeeds.aliasPolys += aliasMesh.numTris;

	// shell rendering
	if (!aliasHasShell)
		return;

	// IR SHELL
	if ((r_RefDef.rdFlags & RDF_IRGOGGLES) && (rbCurrEntity->flags & RF_IR_VISIBLE)) {
		if (glMedia.modelShellIR) {
			R_DrawAliasShell (glMedia.modelShellIR);
		}
		else {
			qglColor4f (1, 0, 0, 0.75 * rbCurrEntity->color[3]);
			R_DrawAliasShell (NULL);
		}
	}
	else {
		// RF_SHELL_DOUBLE
		if (rbCurrEntity->flags & RF_SHELL_DOUBLE) {
			if (glMedia.modelShellDouble) {
				R_DrawAliasShell (glMedia.modelShellDouble);
			}
			else {
				qglColor4f (0.9, 0.7, 0, 0.75 * rbCurrEntity->color[3]);
				R_DrawAliasShell (NULL);
			}
		}

		// RF_SHELL_HALF_DAM
		if (rbCurrEntity->flags & RF_SHELL_HALF_DAM) {
			if (glMedia.modelShellHalfDam) {
				R_DrawAliasShell (glMedia.modelShellHalfDam);
			}
			else {
				qglColor4f (0.56, 0.59, 0.45, 0.75 * rbCurrEntity->color[3]);
				R_DrawAliasShell (NULL);
			}
		}

		// GOD MODE
		if ((rbCurrEntity->flags & RF_SHELL_RED) && (rbCurrEntity->flags & RF_SHELL_GREEN) && (rbCurrEntity->flags & RF_SHELL_BLUE)) {
			if (glMedia.modelShellGod) {
				R_DrawAliasShell (glMedia.modelShellGod);
			}
			else {
				qglColor4f (1, 1, 1, 0.75 * rbCurrEntity->color[3]);
				R_DrawAliasShell (NULL);
			}
		}
		else {
			// RF_SHELL_RED
			if (rbCurrEntity->flags & RF_SHELL_RED) {
				if (glMedia.modelShellRed) {
					R_DrawAliasShell (glMedia.modelShellRed);
				}
				else {
					qglColor4f (1, 0, 0, 0.75 * rbCurrEntity->color[3]);
					R_DrawAliasShell (NULL);
				}
			}

			// RF_SHELL_GREEN
			if (rbCurrEntity->flags & RF_SHELL_GREEN) {
				if (glMedia.modelShellGreen) {
					R_DrawAliasShell (glMedia.modelShellGreen);
				}
				else {
					qglColor4f (0, 1, 0, 0.75 * rbCurrEntity->color[3]);
					R_DrawAliasShell (NULL);
				}
			}

			// RF_SHELL_BLUE
			if (rbCurrEntity->flags & RF_SHELL_BLUE) {
				if (glMedia.modelShellBlue) {
					R_DrawAliasShell (glMedia.modelShellBlue);
				}
				else {
					qglColor4f (0, 0, 1, 0.75 * rbCurrEntity->color[3]);
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
	height = (rbCurrEntity->origin[2] - aliasShadowSpot[2]) * -1;

	// distance
	VectorCopy (rbCurrEntity->origin, newPoint);
	newPoint[2] = height;

	dist = clamp (VectorDistance (rbCurrEntity->origin, newPoint), 0, 401);
	if (dist > 400)
		return;

	// scaling
	alpha = ((405 - dist) / 800.0);
	qglScalef ((alpha + 0.69), (alpha + 0.69), 1);

	// coloring
	qglColor4f (0, 0, 0, alpha);

	qglBegin (GL_TRIANGLES);
	for (i=0 ; i<aliasMesh.numTris ; i++) {
		for (j=0 ; j<3 ; j++) {
			qglVertex3f(vertexArray[aliasMesh.indexes[3*i+j]][0],
						vertexArray[aliasMesh.indexes[3*i+j]][1],
						height);
		}
	}
	qglEnd ();

	glSpeeds.aliasPolys += aliasMesh.numTris;
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
	// compute axially aligned mins and maxs
	if (aliasFrame == aliasOldFrame) {
		VectorCopy (aliasFrame->mins, aliasModMins);
		VectorCopy (aliasFrame->maxs, aliasModMaxs);
		aliasModRadius = aliasFrame->radius;
	}
	else {
		// find the greatest mins/maxs between the current and last frame
		aliasModMins[0] = min (aliasFrame->mins[0], aliasOldFrame->mins[0]);
		aliasModMins[1] = min (aliasFrame->mins[1], aliasOldFrame->mins[1]);
		aliasModMins[2] = min (aliasFrame->mins[2], aliasOldFrame->mins[2]);

		aliasModMaxs[0] = max (aliasFrame->maxs[0], aliasOldFrame->maxs[0]);
		aliasModMaxs[1] = max (aliasFrame->maxs[1], aliasOldFrame->maxs[1]);
		aliasModMaxs[2] = max (aliasFrame->maxs[2], aliasOldFrame->maxs[2]);

		aliasModRadius = RadiusFromBounds (aliasModMins, aliasModMaxs);
	}

	if (rbCurrEntity->scale != 1.0f) {
		VectorScale (aliasModMins, rbCurrEntity->scale, aliasModMins);
		VectorScale (aliasModMaxs, rbCurrEntity->scale, aliasModMaxs);
		aliasModRadius *= rbCurrEntity->scale;
	}
}


/*
=============
R_CullAliasModel
=============
*/
qBool R_CullAliasModel (void) {
	if (rbCurrEntity->flags & RF_WEAPONMODEL)
		return qFalse;
	if (rbCurrEntity->flags & RF_VIEWERMODEL)
		return qFalse;

	if (r_spherecull->integer) {
		if (R_CullSphere (rbCurrEntity->origin, aliasModRadius, 15))
			return qTrue;
	}
	else if (!r_nocull->integer) {
		vec3_t	bbox[8];
		int		i, p, f, aggregateMask = ~0;

		// compute and rotate a full bounding box
		for (i=0 ; i<8 ; i++) {
			vec3_t tmp;

			tmp[0] = ((i & 1) ? aliasModMins[0] : aliasModMaxs[0]);
			tmp[1] = ((i & 2) ? aliasModMins[1] : aliasModMaxs[1]);
			tmp[2] = ((i & 4) ? aliasModMins[2] : aliasModMaxs[2]);

			Matrix_TransformVector (rbCurrEntity->axis, tmp, bbox[i]);
			bbox[i][0] += rbCurrEntity->origin[0];
			bbox[i][1] = -bbox[i][1] + rbCurrEntity->origin[1];
			bbox[i][2] += rbCurrEntity->origin[2];
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
	if (rbCurrEntity->skin) {
		// custom player skin
		aliasMeshSkin = rbCurrEntity->skin;
		aliasMeshShader = RS_FindShader (rbCurrEntity->skin->name);
	}
	else {
		if (rbCurrEntity->skinNum >= MD2_MAX_SKINS) {
			// server's only send MD2 skinNums anyways
			aliasMeshSkin = aliasMesh.skins[0].image;
			aliasMeshShader = aliasMesh.skins[0].shader;
		}
		else if ((rbCurrEntity->skinNum >= 0) && (rbCurrEntity->skinNum < aliasMesh.numSkins)) {
			aliasMeshSkin = aliasMesh.skins[rbCurrEntity->skinNum].image;
			aliasMeshShader = aliasMesh.skins[rbCurrEntity->skinNum].shader;

			if (!aliasMeshSkin) {
				aliasMeshSkin = aliasMesh.skins[0].image;
				aliasMeshShader = aliasMesh.skins[0].shader;
			}
		}
		else {
			aliasMeshSkin = aliasMesh.skins[0].image;
			aliasMeshShader = aliasMesh.skins[0].shader;
		}
	}

	if (!aliasMeshSkin) {
		aliasMeshSkin = r_NoTexture;
		aliasMeshShader = 0;
	}
}


/*
=============
R_AddAliasModel
=============
*/
void R_AddAliasModel (entity_t *ent) {
	mAliasVertex_t	*verts, *oldVerts;
	vec3_t			move, delta;
	vec3_t			normal, oldNormal;
	vec3_t			oldScale, scale;
	float			frontLerp, backLerp;
	int				*neighbors;
	int				meshNum;
	int				i;

	aliasModel = ent->model->aliasModel;

	aliasHasShell = (ent->flags & RF_SHELLMASK) ? qTrue : qFalse;
	aliasHasShadow = (!gl_shadows->integer || (ent->flags & RF_NOSHADOWMASK) || (r_RefDef.rdFlags & RDF_NOWORLDMODEL)) ? qFalse : qTrue;

	// sanity checks
	if ((ent->frame >= aliasModel->numFrames) || (ent->frame < 0)) {
		Com_Printf (PRNT_DEV, S_COLOR_YELLOW "R_AddAliasModel: '%s' no such frame '%d'\n", ent->model->name, ent->frame);
		ent->frame = 0;
	}

	if ((ent->oldFrame >= aliasModel->numFrames) || (ent->oldFrame < 0)) {
		Com_Printf (PRNT_DEV, S_COLOR_YELLOW "R_AddAliasModel: '%s' no such oldFrame '%d'\n", ent->model->name, ent->oldFrame);
		ent->oldFrame = 0;
	}

	aliasFrame = aliasModel->frames + ent->frame;
	aliasOldFrame = aliasModel->frames + ent->oldFrame;

	// culling
	R_AliasModelBBox ();
	if (!aliasHasShadow && R_CullAliasModel ())
		return;

	glSpeeds.visEntities++;

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
	VectorAdd (move, aliasOldFrame->translate, move);

	move[0] = ((backLerp * move[0]) + (frontLerp * aliasFrame->translate[0])) * ent->scale;
	move[1] = ((backLerp * move[1]) + (frontLerp * aliasFrame->translate[1])) * ent->scale;
	move[2] = ((backLerp * move[2]) + (frontLerp * aliasFrame->translate[2])) * ent->scale;

	oldScale[0] = (backLerp * aliasOldFrame->scale[0]) * ent->scale;
	oldScale[1] = (backLerp * aliasOldFrame->scale[1]) * ent->scale;
	oldScale[2] = (backLerp * aliasOldFrame->scale[2]) * ent->scale;

	scale[0] = ((1.0f - backLerp) * aliasFrame->scale[0]) * ent->scale;
	scale[1] = ((1.0f - backLerp) * aliasFrame->scale[1]) * ent->scale;
	scale[2] = ((1.0f - backLerp) * aliasFrame->scale[2]) * ent->scale;

	for (meshNum=0; meshNum<aliasModel->numMeshes; meshNum++) {
		aliasMesh = aliasModel->meshes[meshNum];

		verts = aliasMesh.vertexes + (ent->frame * aliasMesh.numVerts);
		oldVerts = aliasMesh.vertexes + (ent->oldFrame * aliasMesh.numVerts);

		neighbors = aliasMesh.neighbors + (ent->frame * aliasMesh.numVerts);

		for (i=0 ; i<aliasMesh.numVerts ; i++, verts++, oldVerts++) {
			R_AliasLatLongToNorm (verts->latLong, normal);
			R_AliasLatLongToNorm (oldVerts->latLong, oldNormal);

			normalsArray[i][0] = normal[0] + (oldNormal[0] - normal[0]) * backLerp;
			normalsArray[i][1] = normal[1] + (oldNormal[1] - normal[1]) * backLerp;
			normalsArray[i][2] = normal[2] + (oldNormal[2] - normal[2]) * backLerp;

			vertexArray[i][0] = move[0] + verts->point[0]*scale[0] + oldVerts->point[0]*oldScale[0];
			vertexArray[i][1] = move[1] + verts->point[1]*scale[1] + oldVerts->point[1]*oldScale[1];
			vertexArray[i][2] = move[2] + verts->point[2]*scale[2] + oldVerts->point[2]*oldScale[2];
		}

		// lighting
		R_LightForEntity (ent, aliasMesh.numVerts, &aliasHasShell, &aliasHasShadow, aliasShadowSpot);

		// only needed to lerp yourself for your shadow
		if (ent->flags & RF_VIEWERMODEL)
			continue;

		// find the skin and shader
		R_AliasModelSkin ();

		// draw the model
		R_DrawAliasFrameLerp ();

		neighbors += 3;
	}

	// flip it for lefty
	if (ent->flags & RF_CULLHACK)
		qglFrontFace (GL_CCW);

	// depth hacking
	if (ent->flags & RF_DEPTHHACK)
		qglDepthRange (0, 1);

	// throw in a shadow
	if (aliasHasShadow) {
		R_RotateForAliasShadow (ent);
		for (meshNum=0 ; meshNum<aliasModel->numMeshes ; meshNum++) {
			aliasMesh = aliasModel->meshes[meshNum];

			// find the skin and shader
			R_AliasModelSkin ();

			if (aliasMeshShader && aliasMeshShader->noShadow)
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
