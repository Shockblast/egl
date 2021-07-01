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
=================
R_AddSpriteModel
=================
*/
void R_AddSpriteModel (entity_t *ent) {
	mSpriteModel_t	*spriteModel;
	mSpriteFrame_t	*spriteFrame;
	shader_t		*shader;
	vec3_t			point;
	rbMesh_t		mesh;

	spriteModel = ent->model->spriteModel;
	spriteFrame = &spriteModel->frames[ent->frame % spriteModel->numFrames];

	// culling
	if (R_CullSphere (ent->origin, spriteFrame->radius, 15))
		return;

	glSpeeds.visEntities++;

	shader = spriteFrame->shader;

	R_LoadModelIdentity ();

	// rendering
	if (!shader || !r_shaders->integer) {
		// regular shader-less sprites are just fullbright by default
		Vector4Set (colorArray[0], 1, 1, 1, ent->color[3]);

		if ((ent->color[3] != 1.0f) && (!(ent->flags & RF_TRANSLUCENT)))
			qglEnable (GL_BLEND);
		if (ent->color[3] == 1.0f)
			qglEnable (GL_ALPHA_TEST);
	}
	else {
		vec3_t		shadeLight;

		R_LightPoint (ent->origin, shadeLight);
		Vector4Set (colorArray[0], shadeLight[0], shadeLight[1], shadeLight[2], ent->color[3]);
	}

	//
	// create verts/normals/coords
	//
	VectorMA (ent->origin, -spriteFrame->originY, v_Up, point);
	VectorMA (point, -spriteFrame->originX, v_Right, point);
	Vector2Set (coordArray[0], 0, 1);
	VectorSet (normalsArray[0], 0, 1, 0);
	VectorCopy (point, vertexArray[0]);


	VectorMA (ent->origin, spriteFrame->height - spriteFrame->originY, v_Up, point);
	VectorMA (point, -spriteFrame->originX, v_Right, point);
	Vector2Set (coordArray[1], 0, 0);
	VectorSet (normalsArray[1], 0, 1, 0);
	VectorCopy (point, vertexArray[1]);


	VectorMA (ent->origin, spriteFrame->height - spriteFrame->originY, v_Up, point);
	VectorMA (point, spriteFrame->width - spriteFrame->originX, v_Right, point);
	Vector2Set (coordArray[2], 1, 0);
	VectorSet (normalsArray[2], 0, 1, 0);
	VectorCopy (point, vertexArray[2]);


	VectorMA (ent->origin, -spriteFrame->originY, v_Up, point);
	VectorMA (point, spriteFrame->width - spriteFrame->originX, v_Right, point);
	Vector2Set (coordArray[3], 1, 1);
	VectorSet (normalsArray[3], 0, 1, 0);
	VectorCopy (point, vertexArray[3]);

	//
	// create mesh item
	//
	mesh.numColors = 1;
	mesh.numIndexes = QUAD_INDEXES;
	mesh.numVerts = QUAD_INDEXES;

	mesh.colorArray = colorArray;
	mesh.coordArray = coordArray;
	mesh.lmCoordArray = NULL;
	mesh.indexArray = rbQuadIndices;
	mesh.normalsArray = normalsArray;
	mesh.vertexArray = vertexArray;

	mesh.surface = NULL;

	mesh.texture = spriteFrame->skin;
	mesh.lmTexture = NULL;
	mesh.shader = shader;

	//
	// push
	//
	RB_RenderMesh (&mesh);

	if (!shader || !r_shaders->value) {
		if ((ent->color[3] != 1.0f) && (!(ent->flags & RF_TRANSLUCENT)))
			qglDisable (GL_BLEND);
		if (ent->color[3] == 1.0f)
			qglDisable (GL_ALPHA_TEST);
	}

	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
