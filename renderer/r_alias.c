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

static const float r_sinTable[256] = {
	0.000000f,	0.024541f,	0.049068f,	0.073565f,	0.098017f,	0.122411f,	0.146730f,	0.170962f,
	0.195090f,	0.219101f,	0.242980f,	0.266713f,	0.290285f,	0.313682f,	0.336890f,	0.359895f,
	0.382683f,	0.405241f,	0.427555f,	0.449611f,	0.471397f,	0.492898f,	0.514103f,	0.534998f,
	0.555570f,	0.575808f,	0.595699f,	0.615232f,	0.634393f,	0.653173f,	0.671559f,	0.689541f,
	0.707107f,	0.724247f,	0.740951f,	0.757209f,	0.773010f,	0.788346f,	0.803208f,	0.817585f,
	0.831470f,	0.844854f,	0.857729f,	0.870087f,	0.881921f,	0.893224f,	0.903989f,	0.914210f,
	0.923880f,	0.932993f,	0.941544f,	0.949528f,	0.956940f,	0.963776f,	0.970031f,	0.975702f,
	0.980785f,	0.985278f,	0.989177f,	0.992480f,	0.995185f,	0.997290f,	0.998795f,	0.999699f,
	1.000000f,	0.999699f,	0.998795f,	0.997290f,	0.995185f,	0.992480f,	0.989177f,	0.985278f,
	0.980785f,	0.975702f,	0.970031f,	0.963776f,	0.956940f,	0.949528f,	0.941544f,	0.932993f,
	0.923880f,	0.914210f,	0.903989f,	0.893224f,	0.881921f,	0.870087f,	0.857729f,	0.844854f,
	0.831470f,	0.817585f,	0.803208f,	0.788346f,	0.773010f,	0.757209f,	0.740951f,	0.724247f,
	0.707107f,	0.689541f,	0.671559f,	0.653173f,	0.634393f,	0.615232f,	0.595699f,	0.575808f,
	0.555570f,	0.534998f,	0.514103f,	0.492898f,	0.471397f,	0.449611f,	0.427555f,	0.405241f,
	0.382683f,	0.359895f,	0.336890f,	0.313682f,	0.290285f,	0.266713f,	0.242980f,	0.219101f,
	0.195090f,	0.170962f,	0.146731f,	0.122411f,	0.098017f,	0.073564f,	0.049068f,	0.024541f,
	-0.000000f,	-0.024541f,	-0.049068f,	-0.073565f,	-0.098017f,	-0.122411f,	-0.146730f,	-0.170962f,
	-0.195090f,	-0.219101f,	-0.242980f,	-0.266713f,	-0.290285f,	-0.313682f,	-0.336890f,	-0.359895f,
	-0.382683f,	-0.405241f,	-0.427555f,	-0.449611f,	-0.471397f,	-0.492898f,	-0.514103f,	-0.534998f,
	-0.555570f,	-0.575808f,	-0.595699f,	-0.615232f,	-0.634393f,	-0.653173f,	-0.671559f,	-0.689541f,
	-0.707107f,	-0.724247f,	-0.740951f,	-0.757209f,	-0.773010f,	-0.788346f,	-0.803208f,	-0.817585f,
	-0.831469f,	-0.844853f,	-0.857729f,	-0.870087f,	-0.881921f,	-0.893224f,	-0.903989f,	-0.914210f,
	-0.923880f,	-0.932993f,	-0.941544f,	-0.949528f,	-0.956940f,	-0.963776f,	-0.970031f,	-0.975702f,
	-0.980785f,	-0.985278f,	-0.989177f,	-0.992480f,	-0.995185f,	-0.997290f,	-0.998795f,	-0.999699f,
	-1.000000f,	-0.999699f,	-0.998795f,	-0.997290f,	-0.995185f,	-0.992480f,	-0.989177f,	-0.985278f,
	-0.980785f,	-0.975702f,	-0.970031f,	-0.963776f,	-0.956940f,	-0.949528f,	-0.941544f,	-0.932993f,
	-0.923879f,	-0.914210f,	-0.903989f,	-0.893224f,	-0.881921f,	-0.870087f,	-0.857729f,	-0.844853f,
	-0.831470f,	-0.817585f,	-0.803208f,	-0.788346f,	-0.773010f,	-0.757209f,	-0.740951f,	-0.724247f,
	-0.707107f,	-0.689541f,	-0.671559f,	-0.653173f,	-0.634393f,	-0.615231f,	-0.595699f,	-0.575808f,
	-0.555570f,	-0.534998f,	-0.514103f,	-0.492898f,	-0.471397f,	-0.449612f,	-0.427555f,	-0.405241f,
	-0.382683f,	-0.359895f,	-0.336890f,	-0.313682f,	-0.290285f,	-0.266713f,	-0.242980f,	-0.219101f,
	-0.195090f,	-0.170962f,	-0.146730f,	-0.122411f,	-0.098017f,	-0.073565f,	-0.049068f,	-0.024541f
};

static const float r_cosTable[256] = {
	1.000000f,	0.999699f,	0.998795f,	0.997290f,	0.995185f,	0.992480f,	0.989177f,	0.985278f,
	0.980785f,	0.975702f,	0.970031f,	0.963776f,	0.956940f,	0.949528f,	0.941544f,	0.932993f,
	0.923880f,	0.914210f,	0.903989f,	0.893224f,	0.881921f,	0.870087f,	0.857729f,	0.844854f,
	0.831470f,	0.817585f,	0.803208f,	0.788346f,	0.773010f,	0.757209f,	0.740951f,	0.724247f,
	0.707107f,	0.689541f,	0.671559f,	0.653173f,	0.634393f,	0.615232f,	0.595699f,	0.575808f,
	0.555570f,	0.534998f,	0.514103f,	0.492898f,	0.471397f,	0.449611f,	0.427555f,	0.405241f,
	0.382683f,	0.359895f,	0.336890f,	0.313682f,	0.290285f,	0.266713f,	0.242980f,	0.219101f,
	0.195090f,	0.170962f,	0.146730f,	0.122411f,	0.098017f,	0.073565f,	0.049068f,	0.024541f,
	-0.000000f,	-0.024541f,	-0.049068f,	-0.073565f,	-0.098017f,	-0.122411f,	-0.146730f,	-0.170962f,
	-0.195090f,	-0.219101f,	-0.242980f,	-0.266713f,	-0.290285f,	-0.313682f,	-0.336890f,	-0.359895f,
	-0.382683f,	-0.405241f,	-0.427555f,	-0.449611f,	-0.471397f,	-0.492898f,	-0.514103f,	-0.534998f,
	-0.555570f,	-0.575808f,	-0.595699f,	-0.615232f,	-0.634393f,	-0.653173f,	-0.671559f,	-0.689541f,
	-0.707107f,	-0.724247f,	-0.740951f,	-0.757209f,	-0.773010f,	-0.788346f,	-0.803208f,	-0.817585f,
	-0.831470f,	-0.844854f,	-0.857729f,	-0.870087f,	-0.881921f,	-0.893224f,	-0.903989f,	-0.914210f,
	-0.923880f,	-0.932993f,	-0.941544f,	-0.949528f,	-0.956940f,	-0.963776f,	-0.970031f,	-0.975702f,
	-0.980785f,	-0.985278f,	-0.989177f,	-0.992480f,	-0.995185f,	-0.997290f,	-0.998795f,	-0.999699f,
	-1.000000f,	-0.999699f,	-0.998795f,	-0.997290f,	-0.995185f,	-0.992480f,	-0.989177f,	-0.985278f,
	-0.980785f,	-0.975702f,	-0.970031f,	-0.963776f,	-0.956940f,	-0.949528f,	-0.941544f,	-0.932993f,
	-0.923880f,	-0.914210f,	-0.903989f,	-0.893224f,	-0.881921f,	-0.870087f,	-0.857729f,	-0.844854f,
	-0.831470f,	-0.817585f,	-0.803208f,	-0.788346f,	-0.773011f,	-0.757209f,	-0.740951f,	-0.724247f,
	-0.707107f,	-0.689541f,	-0.671559f,	-0.653173f,	-0.634393f,	-0.615232f,	-0.595699f,	-0.575808f,
	-0.555570f,	-0.534998f,	-0.514103f,	-0.492898f,	-0.471397f,	-0.449611f,	-0.427555f,	-0.405242f,
	-0.382684f,	-0.359895f,	-0.336890f,	-0.313682f,	-0.290285f,	-0.266713f,	-0.242980f,	-0.219101f,
	-0.195090f,	-0.170962f,	-0.146730f,	-0.122411f,	-0.098017f,	-0.073565f,	-0.049068f,	-0.024541f,
	0.000000f,	0.024541f,	0.049068f,	0.073565f,	0.098017f,	0.122411f,	0.146730f,	0.170962f,
	0.195090f,	0.219101f,	0.242980f,	0.266713f,	0.290285f,	0.313682f,	0.336890f,	0.359895f,
	0.382684f,	0.405241f,	0.427555f,	0.449611f,	0.471397f,	0.492898f,	0.514103f,	0.534998f,
	0.555570f,	0.575808f,	0.595699f,	0.615232f,	0.634393f,	0.653173f,	0.671559f,	0.689540f,
	0.707107f,	0.724247f,	0.740951f,	0.757209f,	0.773011f,	0.788347f,	0.803207f,	0.817585f,
	0.831470f,	0.844854f,	0.857729f,	0.870087f,	0.881921f,	0.893224f,	0.903989f,	0.914210f,
	0.923880f,	0.932993f,	0.941544f,	0.949528f,	0.956940f,	0.963776f,	0.970031f,	0.975702f,
	0.980785f,	0.985278f,	0.989177f,	0.992480f,	0.995185f,	0.997290f,	0.998795f,	0.999699f
};

static vec3_t	r_aliasMins;
static vec3_t	r_aliasMaxs;
static float	r_aliasRadius;

/*
===============================================================================

	ALIAS PROCESSING

===============================================================================
*/

/*
===============
R_BuildTangentVectors
===============
*/
static void R_BuildTangentVectors (int numVertexes, vec3_t *xyzArray, vec2_t *stArray, int numTris, index_t *indexes, vec3_t *sVectorsArray, vec3_t *tVectorsArray)
{
	float	d, *v[3], *tc[3];
	vec3_t	stVec[3], normal;
	int		i, j;

	// assuming arrays have already been allocated
	// this also does some nice precaching
	memset (sVectorsArray, 0, numVertexes * sizeof (*sVectorsArray));
	memset (tVectorsArray, 0, numVertexes * sizeof (*tVectorsArray));

	for (i=0 ; i<numTris ; i++, indexes+=3) {
		for (j=0 ; j<3 ; j++) {
			v[j] = (float *)(xyzArray + indexes[j]);
			tc[j] = (float *)(stArray + indexes[j]);
		}

		// calculate two mostly perpendicular edge directions
		VectorSubtract (v[0], v[1], stVec[0]);
		VectorSubtract (v[2], v[1], stVec[1]);

		// we have two edge directions, we can calculate the normal then
		CrossProduct (stVec[0], stVec[1], normal);
		VectorNormalize (normal, normal);

		for (j=0 ; j<3 ; j++) {
			stVec[0][j] = ((tc[1][1] - tc[0][1]) * (v[2][j] - v[0][j]) - (tc[2][1] - tc[0][1]) * (v[1][j] - v[0][j]));
			stVec[1][j] = ((tc[1][0] - tc[0][0]) * (v[2][j] - v[0][j]) - (tc[2][0] - tc[0][0]) * (v[1][j] - v[0][j]));
		}

		// keep s\t vectors orthogonal
		for (j=0 ; j<2 ; j++) {
			d = -DotProduct (stVec[j], normal);
			VectorMA (stVec[j], d, normal, stVec[j]);
			VectorNormalize (stVec[j], stVec[j]);
		}

		// inverse tangent vectors if needed
		CrossProduct (stVec[1], stVec[0], stVec[2]);
		if (DotProduct (stVec[2], normal) < 0) {
			VectorInverse (stVec[0]);
			VectorInverse (stVec[1]);
		}

		for (j=0 ; j<3 ; j++) {
			VectorAdd (sVectorsArray[indexes[j]], stVec[0], sVectorsArray[indexes[j]]);
			VectorAdd (tVectorsArray[indexes[j]], stVec[1], tVectorsArray[indexes[j]]);
		}
	}

	// normalize
	for (i=0 ; i<numVertexes ; i++) {
		VectorNormalize (sVectorsArray[i], sVectorsArray[i]);
		VectorNormalize (tVectorsArray[i], tVectorsArray[i]);
	}
}


/*
=============
R_AliasModelBBox
=============
*/
static void R_AliasModelBBox (entity_t *ent, model_t *model)
{
	mAliasModel_t	*aliasModel;
	mAliasFrame_t	*frame;
	mAliasFrame_t	*oldFrame;

	aliasModel = model->aliasModel;
	frame = aliasModel->frames + ent->frame;
	oldFrame = aliasModel->frames + ent->oldFrame;

	// Compute axially aligned mins and maxs
	if (frame == oldFrame) {
		VectorCopy (frame->mins, r_aliasMins);
		VectorCopy (frame->maxs, r_aliasMaxs);
		r_aliasRadius = frame->radius;
	}
	else {
		// Find the greatest mins/maxs between the current and last frame
		r_aliasMins[0] = min (frame->mins[0], oldFrame->mins[0]);
		r_aliasMins[1] = min (frame->mins[1], oldFrame->mins[1]);
		r_aliasMins[2] = min (frame->mins[2], oldFrame->mins[2]);

		r_aliasMaxs[0] = max (frame->maxs[0], oldFrame->maxs[0]);
		r_aliasMaxs[1] = max (frame->maxs[1], oldFrame->maxs[1]);
		r_aliasMaxs[2] = max (frame->maxs[2], oldFrame->maxs[2]);

		r_aliasRadius = RadiusFromBounds (r_aliasMins, r_aliasMaxs);
	}

	if (ent->scale != 1.0f) {
		VectorScale (r_aliasMins, ent->scale, r_aliasMins);
		VectorScale (r_aliasMaxs, ent->scale, r_aliasMaxs);
		r_aliasRadius *= ent->scale;
	}
}


/*
=============
R_CullAliasModel
=============
*/
static qBool R_CullAliasModel (entity_t *ent)
{
	if (ent->flags & RF_WEAPONMODEL)
		return qFalse;
	if (ent->flags & RF_VIEWERMODEL)
		return qFalse;
	if (r_noCull->integer)
		return qFalse;

	// Cull
	if (r_sphereCull->integer) {
		if (R_CullSphere (ent->origin, r_aliasRadius, 15))
			return qTrue;
	}
	else if (!r_noCull->integer) {
		vec3_t	bbox[8];
		vec3_t	tmp;
		int		i, p, f, mask;
		int		aggregateMask = ~0;

		// Compute and rotate a full bounding box
		for (i=0 ; i<8 ; i++) {
			tmp[0] = ((i & 1) ? r_aliasMins[0] : r_aliasMaxs[0]);
			tmp[1] = ((i & 2) ? r_aliasMins[1] : r_aliasMaxs[1]);
			tmp[2] = ((i & 4) ? r_aliasMins[2] : r_aliasMaxs[2]);

			Matrix_TransformVector (ent->axis, tmp, bbox[i]);
			bbox[i][0] += ent->origin[0];
			bbox[i][1] = -bbox[i][1] + ent->origin[1];
			bbox[i][2] += ent->origin[2];
		}

		for (p=0 ; p<8 ; p++) {
			mask = 0;
			
			for (f=0 ; f<4 ; f++) {
				if (DotProduct (r_viewFrustum[f].normal, bbox[p]) < r_viewFrustum[f].dist)
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
R_AddAliasModelToList
=============
*/
void R_AddAliasModelToList (entity_t *ent)
{
	mAliasModel_t	*model;
	mAliasMesh_t	*aliasMesh;
	shader_t		*shader;
	int				meshNum;

	model = ent->model->aliasModel;

	// Sanity checks
	if (ent->frame >= model->numFrames || ent->frame < 0) {
		Com_Printf (PRNT_DEV, S_COLOR_YELLOW "R_AddAliasModelToList: '%s' no such frame '%d'\n", ent->model->name, ent->frame);
		ent->frame = 0;
	}

	if (ent->oldFrame >= model->numFrames || ent->oldFrame < 0) {
		Com_Printf (PRNT_DEV, S_COLOR_YELLOW "R_AddAliasModelToList: '%s' no such oldFrame '%d'\n", ent->model->name, ent->oldFrame);
		ent->oldFrame = 0;
	}

	// Culling
	R_AliasModelBBox (ent, ent->model);
	if (!gl_shadows->integer && R_CullAliasModel (ent))
		return;

	// Add to list
	for (meshNum=0; meshNum<model->numMeshes; meshNum++) {
		aliasMesh = &model->meshes[meshNum];

		// Find the skin for this mesh
		if (ent->skin) {
			// Custom player skin
			shader = ent->skin;
		}
		else {
			if (ent->skinNum >= MD2_MAX_SKINS) {
				// Server's only send MD2 skinNums anyways
				shader = aliasMesh->skins[0].skin;
			}
			else if (ent->skinNum >= 0 && ent->skinNum < aliasMesh->numSkins) {
				shader = aliasMesh->skins[ent->skinNum].skin;
				if (!shader)
					shader = aliasMesh->skins[0].skin;
			}
			else {
				shader = aliasMesh->skins[0].skin;
			}
		}

		if (!shader) {
			shader = r_noShader;
			Com_Printf (0, S_COLOR_YELLOW "R_AddAliasModelToList: '%s' has a NULL shader\n", ent->model->name);
		}

		R_AddMeshToList (shader, ent, MBT_MODEL_ALIAS, aliasMesh);
	}
}


/*
=============
R_DrawAliasModel
=============
*/
void R_DrawAliasModel (meshBuffer_t *mb, qBool shadowPass)
{
	mAliasModel_t	*model;
	mAliasVertex_t	*verts, *oldVerts;
	mAliasMesh_t	*aliasMesh;
	mAliasFrame_t	*frame, *oldFrame;
	vec3_t			move, delta;
	vec3_t			oldScale, scale;
	float			frontLerp, backLerp;
	vec3_t			shadowSpot;
	int				features, i;
	qBool			calcNormals;
	qBool			calcSTVectors;
	mesh_t			mesh;
	entity_t		*ent;

	ent = mb->entity;
	model = ent->model->aliasModel;

	if (!shadowPass && ent->flags & RF_VIEWERMODEL)
		return;

	aliasMesh = (mAliasMesh_t *)mb->mesh;

	if (shadowPass) {
#ifdef SHADOW_VOLUMES
		if (!aliasMesh->neighbors)
			return;
#endif
		if (mb->shader->flags & SHADER_NOSHADOW)
			return;
	}

	frame = model->frames + ent->frame;
	oldFrame = model->frames + ent->oldFrame;

	// Determine if it will have a shadow
	if (shadowPass) {
		// Find where to put the shadow
		shadowPass = R_ShadowForEntity (ent, shadowSpot);
		if (!shadowPass)
			return;
	}

	// Depth hacking
	if (ent->flags & RF_DEPTHHACK)
		qglDepthRange (0, 0.3f);

	// Flip it for lefty
	if (ent->flags & RF_CULLHACK)
		qglFrontFace (GL_CW);

	// Interpolation calculations
	backLerp = ent->backLerp;
	if (!r_lerpmodels->integer)
		backLerp = 0;
	frontLerp = 1.0f - backLerp;

	VectorSubtract (ent->oldOrigin, ent->origin, delta);
	Matrix_TransformVector (ent->axis, delta, move);
	VectorAdd (move, oldFrame->translate, move);

	move[0] = frame->translate[0] + (move[0] - frame->translate[0]) * backLerp;
	move[1] = frame->translate[1] + (move[1] - frame->translate[1]) * backLerp;
	move[2] = frame->translate[2] + (move[2] - frame->translate[2]) * backLerp;

	// Mesh features
	features = MF_NONBATCHED | mb->shader->features;
	if (mb->shader->features & SHADER_AUTOSPRITE)
		features |= MF_NOCULL;

	if (shadowPass) {
		calcNormals = qFalse;
		calcSTVectors = qFalse;
		features |= MF_DEFORMVS;
	}
	else {
		if (gl_shownormals->integer) {
			features |= MF_NORMALS;
			calcNormals = qTrue;
		}
		else {
			calcNormals = (features & MF_NORMALS);
		}

		calcSTVectors = (features & MF_STVECTORS);
	}

	// Optimal route
	if (ent->frame == ent->oldFrame) {
		scale[0] = frame->scale[0] * ent->scale;
		scale[1] = frame->scale[1] * ent->scale;
		scale[2] = frame->scale[2] * ent->scale;

		// Store vertexes
		verts = aliasMesh->vertexes + (ent->frame * aliasMesh->numVerts);
		for (i=0 ; i<aliasMesh->numVerts ; i++, verts++) {
			rb_vertexArray[i][0] = move[0] + verts->point[0]*scale[0];
			rb_vertexArray[i][1] = move[1] + verts->point[1]*scale[1];
			rb_vertexArray[i][2] = move[2] + verts->point[2]*scale[2];
		}

		// Calculate normals
		if (calcNormals) {
			verts = aliasMesh->vertexes + (ent->frame * aliasMesh->numVerts);
			for (i=0 ; i<aliasMesh->numVerts ; i++, verts++) {
				rb_normalsArray[i][0] = r_sinTable[verts->latLong[0]] * r_cosTable[verts->latLong[1]];
				rb_normalsArray[i][1] = r_sinTable[verts->latLong[0]] * r_sinTable[verts->latLong[1]];
				rb_normalsArray[i][2] = r_cosTable[verts->latLong[0]];

				VectorNormalizeFast (rb_normalsArray[i]);
			}
		}
	}
	else {
		verts = aliasMesh->vertexes + (ent->frame * aliasMesh->numVerts);
		oldVerts = aliasMesh->vertexes + (ent->oldFrame * aliasMesh->numVerts);

		scale[0] = (frontLerp * frame->scale[0]) * ent->scale;
		scale[1] = (frontLerp * frame->scale[1]) * ent->scale;
		scale[2] = (frontLerp * frame->scale[2]) * ent->scale;

		oldScale[0] = (backLerp * oldFrame->scale[0]) * ent->scale;
		oldScale[1] = (backLerp * oldFrame->scale[1]) * ent->scale;
		oldScale[2] = (backLerp * oldFrame->scale[2]) * ent->scale;

		// Store vertexes
		for (i=0 ; i<aliasMesh->numVerts ; i++, verts++, oldVerts++) {
			rb_vertexArray[i][0] = move[0] + verts->point[0]*scale[0] + oldVerts->point[0]*oldScale[0];
			rb_vertexArray[i][1] = move[1] + verts->point[1]*scale[1] + oldVerts->point[1]*oldScale[1];
			rb_vertexArray[i][2] = move[2] + verts->point[2]*scale[2] + oldVerts->point[2]*oldScale[2];

			VectorNormalizeFast (rb_normalsArray[i]);
		}

		// Calculate normals
		if (calcNormals) {
			vec3_t		normal, oldNormal;

			verts = aliasMesh->vertexes + (ent->frame * aliasMesh->numVerts);
			oldVerts = aliasMesh->vertexes + (ent->oldFrame * aliasMesh->numVerts);
			for (i=0 ; i<aliasMesh->numVerts ; i++, verts++, oldVerts++) {
				normal[0] = r_sinTable[verts->latLong[0]] * r_cosTable[verts->latLong[1]];
				normal[1] = r_sinTable[verts->latLong[0]] * r_sinTable[verts->latLong[1]];
				normal[2] = r_cosTable[verts->latLong[0]];

				oldNormal[0] = r_sinTable[oldVerts->latLong[0]] * r_cosTable[oldVerts->latLong[1]];
				oldNormal[1] = r_sinTable[oldVerts->latLong[0]] * r_sinTable[oldVerts->latLong[1]];
				oldNormal[2] = r_cosTable[oldVerts->latLong[0]];

				rb_normalsArray[i][0] = normal[0] + (oldNormal[0] - normal[0]) * backLerp;
				rb_normalsArray[i][1] = normal[1] + (oldNormal[1] - normal[1]) * backLerp;
				rb_normalsArray[i][2] = normal[2] + (oldNormal[2] - normal[2]) * backLerp;

				VectorNormalizeFast (rb_normalsArray[i]);
			}
		}
	}

	// Build stVectors
	if (calcSTVectors) {
		R_BuildTangentVectors (aliasMesh->numVerts, rb_vertexArray, aliasMesh->coords, aliasMesh->numTris, aliasMesh->indexes, rb_sVectorsArray, rb_tVectorsArray);
		mesh.sVectorsArray = rb_sVectorsArray;
		mesh.tVectorsArray = rb_tVectorsArray;
	}
	else {
		mesh.sVectorsArray = NULL;
		mesh.tVectorsArray = NULL;
	}

	// Fill out mesh properties
	mesh.numIndexes = aliasMesh->numTris * 3;
	mesh.numVerts = aliasMesh->numVerts;

	mesh.colorArray = NULL;
	mesh.coordArray = aliasMesh->coords;
	mesh.indexArray = aliasMesh->indexes;
	mesh.lmCoordArray = NULL;
	mesh.normalsArray = rb_normalsArray;
#ifdef SHADOW_VOLUMES
	mesh.trNeighborsArray = aliasMesh->neighbors;
	mesh.trNormalsArray = NULL;
#endif
	mesh.vertexArray = rb_vertexArray;

	// Push the mesh
	if (!RB_InvalidMesh (&mesh)) {
		RB_PushMesh (&mesh, features);
		RB_RenderMeshBuffer (mb, shadowPass);

		// Throw in a shadow if this is the pass for it
		if (shadowPass) {
			if (gl_shadows->integer == 1) {
				R_RotateForAliasShadow (ent);
				R_SimpleShadow (ent, shadowSpot);
			}
			else {
#ifdef SHADOW_VOLUMES
				R_AliasModelBBox (ent, ent->model);
				R_DrawShadowVolumes (&mesh, ent, r_aliasMins, r_aliasMaxs, r_aliasRadius);
#endif
			}
		}
	}

	// Flip it for lefty
	if (ent->flags & RF_CULLHACK)
		qglFrontFace (GL_CCW);

	// Depth hacking
	if (ent->flags & RF_DEPTHHACK)
		qglDepthRange (0, 1);

	glSpeeds.aliasPolys += aliasMesh->numTris;
}
