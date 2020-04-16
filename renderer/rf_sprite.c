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
// rf_sprite.c
// sprite model rendering
//

#include "rf_local.h"

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
void R_AddSP2ModelToList (refEntity_t *ent)
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
	mesh_t			mesh;
	meshFeatures_t	features;

	spriteModel = (mSpriteModel_t *)mb->mesh;
	spriteFrame = &spriteModel->frames[mb->entity->frame % spriteModel->numFrames];

	//
	// Culling
	//
	if ((mb->entity->origin[0] - ri.def.viewOrigin[0]) * ri.def.viewAxis[0][0]
	+ (mb->entity->origin[1] - ri.def.viewOrigin[1]) * ri.def.viewAxis[0][1]
	+ (mb->entity->origin[2] - ri.def.viewOrigin[2]) * ri.def.viewAxis[0][2] < 0)
		return;

	//
	// create verts/normals/coords
	//
	Vec4Copy (mb->entity->color, r_spriteColors[0]);
	r_spriteVerts[0][0] = mb->entity->origin[0] + (ri.def.viewAxis[2][0] * -spriteFrame->originY) + (ri.def.rightVec[0] * -spriteFrame->originX);
	r_spriteVerts[0][1] = mb->entity->origin[1] + (ri.def.viewAxis[2][1] * -spriteFrame->originY) + (ri.def.rightVec[1] * -spriteFrame->originX);
	r_spriteVerts[0][2] = mb->entity->origin[2] + (ri.def.viewAxis[2][2] * -spriteFrame->originY) + (ri.def.rightVec[2] * -spriteFrame->originX);
	Vec2Set (r_spriteCoords[0], 0, 1);
	Vec3Set (r_spriteNormals[0], 0, 1, 0);

	Vec4Copy (mb->entity->color, r_spriteColors[1]);
	r_spriteVerts[1][0] = mb->entity->origin[0] + (ri.def.viewAxis[2][0] * (spriteFrame->height-spriteFrame->originY)) + (ri.def.rightVec[0] * -spriteFrame->originX);
	r_spriteVerts[1][1] = mb->entity->origin[1] + (ri.def.viewAxis[2][1] * (spriteFrame->height-spriteFrame->originY)) + (ri.def.rightVec[1] * -spriteFrame->originX);
	r_spriteVerts[1][2] = mb->entity->origin[2] + (ri.def.viewAxis[2][2] * (spriteFrame->height-spriteFrame->originY)) + (ri.def.rightVec[2] * -spriteFrame->originX);
	Vec2Set (r_spriteCoords[1], 0, 0);
	Vec3Set (r_spriteNormals[1], 0, 1, 0);

	Vec4Copy (mb->entity->color, r_spriteColors[2]);
	r_spriteVerts[2][0] = mb->entity->origin[0] + (ri.def.viewAxis[2][0] * (spriteFrame->height-spriteFrame->originY)) + (ri.def.rightVec[0] * (spriteFrame->width-spriteFrame->originX));
	r_spriteVerts[2][1] = mb->entity->origin[1] + (ri.def.viewAxis[2][1] * (spriteFrame->height-spriteFrame->originY)) + (ri.def.rightVec[1] * (spriteFrame->width-spriteFrame->originX));
	r_spriteVerts[2][2] = mb->entity->origin[2] + (ri.def.viewAxis[2][2] * (spriteFrame->height-spriteFrame->originY)) + (ri.def.rightVec[2] * (spriteFrame->width-spriteFrame->originX));
	Vec2Set (r_spriteCoords[2], 1, 0);
	Vec3Set (r_spriteNormals[2], 0, 1, 0);

	Vec4Copy (mb->entity->color, r_spriteColors[3]);
	r_spriteVerts[3][0] = mb->entity->origin[0] + (ri.def.viewAxis[2][0] * -spriteFrame->originY) + (ri.def.rightVec[0] * (spriteFrame->width-spriteFrame->originX));
	r_spriteVerts[3][1] = mb->entity->origin[1] + (ri.def.viewAxis[2][1] * -spriteFrame->originY) + (ri.def.rightVec[1] * (spriteFrame->width-spriteFrame->originX));
	r_spriteVerts[3][2] = mb->entity->origin[2] + (ri.def.viewAxis[2][2] * -spriteFrame->originY) + (ri.def.rightVec[2] * (spriteFrame->width-spriteFrame->originX));
	Vec2Set (r_spriteCoords[3], 1, 1);
	Vec3Set (r_spriteNormals[3], 0, 1, 0);

	//
	// Create mesh item
	//
	mesh.numIndexes = 0;
	mesh.numVerts = 4;

	mesh.colorArray = r_spriteColors;
	mesh.coordArray = r_spriteCoords;
	mesh.lmCoordArray = NULL;
	mesh.indexArray = NULL;
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
	if (gl_shownormals->intVal)
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
	// Only need to check the current one, because the next one will have the same vertex/index count anyways.
	return RB_BackendOverflow (4, 6);
}


/*
=================
R_PushFlare
=================
*/
void R_PushFlare (meshBuffer_t *mb)
{
	vec4_t			color;
	vec3_t			origin, point, v;
	float			radius = r_flareSize->floatVal, flarescale, depth;
	float			up = radius, down = -radius, left = -radius, right = radius;
	mBspSurface_t	*surf = (mBspSurface_t *)mb->mesh;
	meshFeatures_t	features;

	if (mb->entity && mb->entity->model && !mb->entity->model->isBspModel) {
		Matrix3_TransformVector (mb->entity->axis, surf->q3_origin, origin);
		Vec3Add(origin, mb->entity->origin, origin);
	}
	else {
		Vec3Copy (surf->q3_origin, origin);
	}
	R_TransformToScreen_Vec3 (origin, v);

	if (v[0] < ri.def.x || v[0] > ri.def.x + ri.def.width)
		return;
	if (v[1] < ri.def.y || v[1] > ri.def.y + ri.def.height)
		return;

	qglReadPixels ((int)(v[0]), (int)(v[1]), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
	if (depth + 1e-4 < v[2])
		return;		// Occluded

	Vec3Copy (origin, origin);

	Vec3MA (origin, down, ri.def.viewAxis[2], point);
	Vec3MA (point, -left, ri.def.rightVec, r_flareVerts[0]);
	Vec3MA (point, -right, ri.def.rightVec, r_flareVerts[3]);

	Vec3MA (origin, up, ri.def.viewAxis[2], point);
	Vec3MA (point, -left, ri.def.rightVec, r_flareVerts[1]);
	Vec3MA (point, -right, ri.def.rightVec, r_flareVerts[2]);

	flarescale = 1.0f / r_flareFade->floatVal;
	Vec4Set (color,
		COLOR_R (surf->dLightBits) * flarescale,
		COLOR_G (surf->dLightBits) * flarescale,
		COLOR_B (surf->dLightBits) * flarescale, 255);

	Vec4Copy (color, r_flareColors[0]);
	Vec4Copy (color, r_flareColors[1]);
	Vec4Copy (color, r_flareColors[2]);
	Vec4Copy (color, r_flareColors[3]);

	r_flareMesh.numIndexes = 6;
	r_flareMesh.numVerts = 4;
	r_flareMesh.vertexArray = r_flareVerts;
	r_flareMesh.coordArray = r_flareCoords;
	r_flareMesh.colorArray = r_flareColors;

	features = MF_NOCULL|MF_TRIFAN|mb->shader->features;
	if (r_debugBatching->intVal == 2)
		features |= MF_NONBATCHED;

	RB_PushMesh (&r_flareMesh, features);
}
