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
// r_sprite.c
// sprite model rendering
//

#include "r_local.h"

/*
=============================================================================

	SP2 MODELS

=============================================================================
*/

static bvec4_t	r_spriteColors[4];
static vec2_t	r_spriteCoords[4];
static vec3_t	r_spriteVerts[4];
static vec3_t	r_spriteNormals[4];

/*
=================
R_AddSP2ModelToList
=================
*/
void R_AddSP2ModelToList (entity_t *ent)
{
	mSpriteModel_t	*spriteModel;
	mSpriteFrame_t	*spriteFrame;
	shader_t		*shader;

	spriteModel = ent->model->spriteModel;
	spriteFrame = &spriteModel->frames[ent->frame % spriteModel->numFrames];
	shader = spriteFrame->skin;

	if (!shader) {
		Com_DevPrintf (PRNT_WARNING, "R_AddSP2ModelToList: '%s' has a NULL shader\n", ent->model->name);
		return;
	}

	R_AddMeshToList (shader, ent->shaderTime, ent, R_FogForSphere (ent->origin, spriteFrame->radius), MBT_SP2, spriteModel);
}


/*
=================
R_DrawSP2Model
=================
*/
void R_DrawSP2Model (meshBuffer_t *mb)
{
	mSpriteModel_t	*spriteModel;
	mSpriteFrame_t	*spriteFrame;
	vec3_t			point;
	mesh_t			mesh;
	meshFeatures_t	features;

	spriteModel = (mSpriteModel_t *)mb->mesh;
	spriteFrame = &spriteModel->frames[mb->entity->frame % spriteModel->numFrames];

	//
	// Culling
	//
	if ((mb->entity->origin[0] - r_refDef.viewOrigin[0]) * r_refDef.forwardVec[0]
	+ (mb->entity->origin[1] - r_refDef.viewOrigin[1]) * r_refDef.forwardVec[1]
	+ (mb->entity->origin[2] - r_refDef.viewOrigin[2]) * r_refDef.forwardVec[2] < 0)
		return;

	//
	// create verts/normals/coords
	//
	Vector4Set (r_spriteColors[0], mb->entity->color[0], mb->entity->color[1], mb->entity->color[2], mb->entity->color[3]);
	VectorMA (mb->entity->origin, -spriteFrame->originY, r_refDef.upVec, point);
	VectorMA (point, -spriteFrame->originX, r_refDef.rightVec, point);
	Vector2Set (r_spriteCoords[0], 0, 1);
	VectorSet (r_spriteNormals[0], 0, 1, 0);
	VectorCopy (point, r_spriteVerts[0]);

	Vector4Set (r_spriteColors[1], mb->entity->color[0], mb->entity->color[1], mb->entity->color[2], mb->entity->color[3]);
	VectorMA (mb->entity->origin, spriteFrame->height - spriteFrame->originY, r_refDef.upVec, point);
	VectorMA (point, -spriteFrame->originX, r_refDef.rightVec, point);
	Vector2Set (r_spriteCoords[1], 0, 0);
	VectorSet (r_spriteNormals[1], 0, 1, 0);
	VectorCopy (point, r_spriteVerts[1]);

	Vector4Set (r_spriteColors[2], mb->entity->color[0], mb->entity->color[1], mb->entity->color[2], mb->entity->color[3]);
	VectorMA (mb->entity->origin, spriteFrame->height - spriteFrame->originY, r_refDef.upVec, point);
	VectorMA (point, spriteFrame->width - spriteFrame->originX, r_refDef.rightVec, point);
	Vector2Set (r_spriteCoords[2], 1, 0);
	VectorSet (r_spriteNormals[2], 0, 1, 0);
	VectorCopy (point, r_spriteVerts[2]);

	Vector4Set (r_spriteColors[3], mb->entity->color[0], mb->entity->color[1], mb->entity->color[2], mb->entity->color[3]);
	VectorMA (mb->entity->origin, -spriteFrame->originY, r_refDef.upVec, point);
	VectorMA (point, spriteFrame->width - spriteFrame->originX, r_refDef.rightVec, point);
	Vector2Set (r_spriteCoords[3], 1, 1);
	VectorSet (r_spriteNormals[3], 0, 1, 0);
	VectorCopy (point, r_spriteVerts[3]);

	//
	// Create mesh item
	//
	mesh.numIndexes = QUAD_INDEXES;
	mesh.numVerts = 4;

	mesh.colorArray = r_spriteColors;
	mesh.coordArray = r_spriteCoords;
	mesh.lmCoordArray = NULL;
	mesh.indexArray = rb_quadIndices;
	mesh.normalsArray = r_spriteNormals;
	mesh.sVectorsArray = NULL;
	mesh.tVectorsArray = NULL;
	mesh.trNeighborsArray = NULL;
	mesh.trNormalsArray = NULL;
	mesh.vertexArray = r_spriteVerts;

	//
	// Push
	//
	features = MF_TRIFAN|MF_NOCULL|MF_NONBATCHED|mb->shader->features;
	if (gl_shownormals->integer)
		features |= MF_NORMALS;

	RB_PushMesh (&mesh, features);
	RB_RenderMeshBuffer (mb, qFalse);
}

/*
=============================================================================

	Q3BSP FLARE

=============================================================================
*/

static vec3_t		r_flareVerts[4];
static vec2_t		r_flareCoords[4] = { {0, 1}, {0, 0}, {1,0}, {1,1} };
static bvec4_t		r_flareColors[4];
static mesh_t		r_flareMesh;

/*
=================
R_FlareOverflow
=================
*/
qBool R_FlareOverflow (void)
{
	return RB_BackendOverflow (&r_flareMesh);
}


/*
=================
R_PushFlare
=================
*/
void R_TransformToScreen_Vec3 (vec3_t in, vec3_t out); // FIXME
void R_PushFlare (meshBuffer_t *mb)
{
	vec4_t			color;
	vec3_t			origin, point, v;
	float			radius = r_flareSize->value, flarescale, depth;
	float			up = radius, down = -radius, left = -radius, right = radius;
	mBspSurface_t	*surf = (mBspSurface_t *)mb->mesh;

	if (mb->entity && mb->entity->model && !mb->entity->model->isBspModel) {
		Matrix3_TransformVector (mb->entity->axis, surf->q3_origin, origin);
		VectorAdd(origin, mb->entity->origin, origin);
	}
	else {
		VectorCopy (surf->q3_origin, origin);
	}
	R_TransformToScreen_Vec3 (origin, v);

	if (v[0] < r_refDef.x || v[0] > r_refDef.x + r_refDef.width)
		return;
	if (v[1] < r_refDef.y || v[1] > r_refDef.y + r_refDef.height)
		return;

	qglReadPixels ((int)(v[0]), (int)(v[1]), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
	if (depth + 1e-4 < v[2])
		return;		// Occluded

	VectorCopy (origin, origin);

	VectorMA (origin, down, r_refDef.upVec, point);
	VectorMA (point, -left, r_refDef.rightVec, r_flareVerts[0]);
	VectorMA (point, -right, r_refDef.rightVec, r_flareVerts[3]);

	VectorMA (origin, up, r_refDef.upVec, point);
	VectorMA (point, -left, r_refDef.rightVec, r_flareVerts[1]);
	VectorMA (point, -right, r_refDef.rightVec, r_flareVerts[2]);

	flarescale = 1.0f / r_flareFade->value;
	Vector4Set (color,
		COLOR_R (surf->dLightBits) * flarescale,
		COLOR_G (surf->dLightBits) * flarescale,
		COLOR_B (surf->dLightBits) * flarescale, 255);

	Vector4Copy (color, r_flareColors[0]);
	Vector4Copy (color, r_flareColors[1]);
	Vector4Copy (color, r_flareColors[2]);
	Vector4Copy (color, r_flareColors[3]);

	r_flareMesh.numIndexes = 6;
	r_flareMesh.numVerts = 4;
	r_flareMesh.vertexArray = r_flareVerts;
	r_flareMesh.coordArray = r_flareCoords;
	r_flareMesh.colorArray = r_flareColors;

	RB_PushMesh (&r_flareMesh, MF_NOCULL|MF_TRIFAN|mb->shader->features);
}
