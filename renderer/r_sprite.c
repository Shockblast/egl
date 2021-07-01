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

static bvec4_t	r_spriteColors[4];
static vec2_t	r_spriteCoords[4];
static vec3_t	r_spriteVerts[4];
static vec3_t	r_spriteNormals[4];

/*
=================
R_AddSpriteModelToList
=================
*/
void R_AddSpriteModelToList (entity_t *ent)
{
	mSpriteModel_t	*spriteModel;
	mSpriteFrame_t	*spriteFrame;
	shader_t		*shader;

	spriteModel = ent->model->spriteModel;
	spriteFrame = &spriteModel->frames[ent->frame % spriteModel->numFrames];
	shader = spriteFrame->skin;

	R_AddMeshToList (shader, ent, MBT_MODEL_SP2, spriteModel);
}


/*
=================
R_DrawSpriteModel
=================
*/
void R_DrawSpriteModel (meshBuffer_t *mb)
{
	mSpriteModel_t	*spriteModel;
	mSpriteFrame_t	*spriteFrame;
	vec3_t			point;
	mesh_t			mesh;
	int				features;

	spriteModel = (mSpriteModel_t *)mb->mesh;
	spriteFrame = &spriteModel->frames[mb->entity->frame % spriteModel->numFrames];

	//
	// Culling
	//
	if ((mb->entity->origin[0] - r_refDef.viewOrigin[0]) * r_forwardVec[0] +
		(mb->entity->origin[1] - r_refDef.viewOrigin[1]) * r_forwardVec[1] + 
		(mb->entity->origin[2] - r_refDef.viewOrigin[2]) * r_forwardVec[2] < 0)
		return;

	//
	// create verts/normals/coords
	//
	Vector4Set (r_spriteColors[0], mb->entity->color[0], mb->entity->color[1], mb->entity->color[2], mb->entity->color[3]);
	VectorMA (mb->entity->origin, -spriteFrame->originY, r_upVec, point);
	VectorMA (point, -spriteFrame->originX, r_rightVec, point);
	Vector2Set (r_spriteCoords[0], 0, 1);
	VectorSet (r_spriteNormals[0], 0, 1, 0);
	VectorCopy (point, r_spriteVerts[0]);

	Vector4Set (r_spriteColors[1], mb->entity->color[0], mb->entity->color[1], mb->entity->color[2], mb->entity->color[3]);
	VectorMA (mb->entity->origin, spriteFrame->height - spriteFrame->originY, r_upVec, point);
	VectorMA (point, -spriteFrame->originX, r_rightVec, point);
	Vector2Set (r_spriteCoords[1], 0, 0);
	VectorSet (r_spriteNormals[1], 0, 1, 0);
	VectorCopy (point, r_spriteVerts[1]);

	Vector4Set (r_spriteColors[2], mb->entity->color[0], mb->entity->color[1], mb->entity->color[2], mb->entity->color[3]);
	VectorMA (mb->entity->origin, spriteFrame->height - spriteFrame->originY, r_upVec, point);
	VectorMA (point, spriteFrame->width - spriteFrame->originX, r_rightVec, point);
	Vector2Set (r_spriteCoords[2], 1, 0);
	VectorSet (r_spriteNormals[2], 0, 1, 0);
	VectorCopy (point, r_spriteVerts[2]);

	Vector4Set (r_spriteColors[3], mb->entity->color[0], mb->entity->color[1], mb->entity->color[2], mb->entity->color[3]);
	VectorMA (mb->entity->origin, -spriteFrame->originY, r_upVec, point);
	VectorMA (point, spriteFrame->width - spriteFrame->originX, r_rightVec, point);
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
