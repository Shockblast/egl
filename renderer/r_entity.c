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
// r_entity.c
// Entity handling and null model rendering
//

#include "r_local.h"

static int		r_numNullEntities;
static entity_t	*r_nullEntities[MAX_ENTITIES];

/*
=============================================================================

	ENTITIES

=============================================================================
*/

/*
=================
R_LoadModelIdentity

ala Vic
=================
*/
void R_LoadModelIdentity (void)
{
	Matrix4_Copy (r_refScene.worldViewMatrix, r_refScene.modelViewMatrix);
	qglLoadMatrixd (r_refScene.worldViewMatrix);
}


/*
=================
R_RotateForEntity

ala Vic
=================
*/
void R_RotateForEntity (entity_t *ent)
{
	mat4x4_t	objectMatrix;

	Matrix3_Matrix4 (ent->axis, ent->origin, objectMatrix);
	if (ent->model && ent->model->isBspModel && ent->scale != 1.0f)
		Matrix4_Scale (objectMatrix, ent->scale, ent->scale, ent->scale);
	Matrix4_MultiplyFast (r_refScene.worldViewMatrix, objectMatrix, r_refScene.modelViewMatrix);

	qglLoadMatrixd (r_refScene.modelViewMatrix);
}


/*
=================
R_RotateForAliasShadow

Not even sure if this is right...
=================
*/
void R_RotateForAliasShadow (entity_t *ent)
{
	mat4x4_t	objectMatrix;

	objectMatrix[ 0] = ent->axis[0][0];
	objectMatrix[ 1] = ent->axis[0][1];
	objectMatrix[ 2] = 0;

	objectMatrix[ 3] = 0;

	objectMatrix[ 4] = ent->axis[1][0];
	objectMatrix[ 5] = ent->axis[1][1];
	objectMatrix[ 6] = ent->axis[1][2];

	objectMatrix[ 7] = 0;

	objectMatrix[ 8] = 0;
	objectMatrix[ 9] = 0;
	objectMatrix[10] = ent->axis[2][2];

	objectMatrix[11] = 0;
	objectMatrix[12] = ent->origin[0];
	objectMatrix[13] = ent->origin[1];
	objectMatrix[14] = ent->origin[2];
	objectMatrix[15] = 1.0;

	Matrix4_MultiplyFast (r_refScene.worldViewMatrix, objectMatrix, r_refScene.modelViewMatrix);

	qglLoadMatrixd (r_refScene.modelViewMatrix);
}


/*
=================
R_TranslateForEntity

ala Vic
=================
*/
void R_TranslateForEntity (entity_t *ent)
{
	mat4x4_t	objectMatrix;

	Matrix4_Identity (objectMatrix);

	objectMatrix[12] = ent->origin[0];
	objectMatrix[13] = ent->origin[1];
	objectMatrix[14] = ent->origin[2];

	Matrix4_MultiplyFast (r_refScene.worldViewMatrix, objectMatrix, r_refScene.modelViewMatrix);

	qglLoadMatrixd (r_refScene.modelViewMatrix);
}


/*
=============
R_AddNullModelToList

Used when a model isn't found for the current entity
=============
*/
static void R_AddNullModelToList (entity_t *ent)
{
	r_nullEntities[r_numNullEntities++] = ent;
}


/*
=============
R_DrawNullModel

Used when a model isn't found for the current entity
=============
*/
static void R_DrawNullModel (entity_t *ent)
{
	vec3_t	shadelight;
	int		i;

	if (ent->flags & (RF_VIEWERMODEL|RF_DEPTHHACK))
		return;

	if (ent->flags & RF_FULLBRIGHT)
		VectorSet (shadelight, 1, 1, 1);
	else
		R_LightPoint (ent->origin, shadelight);

	qglDepthMask (GL_FALSE);
	qglDisable (GL_TEXTURE_2D);
	qglColor3fv (shadelight);

	qglBegin (GL_TRIANGLE_FAN);

	qglVertex3f (0, 0, -16);
	for (i=0 ; i<=4 ; i++)
		qglVertex3f (16 * (float)cos (i * (M_PI / 2.0f)), 16 * (float)sin (i * (M_PI / 2.0f)), 0);
	qglEnd ();

	qglBegin (GL_TRIANGLE_FAN);
	qglVertex3f (0, 0, 16);
	for (i=4 ; i>=0 ; i--)
		qglVertex3f (16 * (float)cos (i * (M_PI / 2.0f)), 16 * (float)sin (i * (M_PI / 2.0f)), 0);

	qglEnd ();

	qglEnable (GL_TEXTURE_2D);
	qglDepthMask (GL_TRUE);
	qglColor4f (1, 1, 1, 1);
}


/*
=============
R_DrawNullModelList
=============
*/
void R_DrawNullModelList (void)
{
	int		i;

	for (i=0 ; i<r_numNullEntities ; i++) {
		if (r_refScene.mirrorView) {
			if (r_nullEntities[i]->flags & RF_WEAPONMODEL) 
				continue;
		}
		else {
			if (r_nullEntities[i]->flags & RF_VIEWERMODEL) 
				continue;
		}

		R_RotateForEntity (r_nullEntities[i]);
		R_DrawNullModel (r_nullEntities[i]);
	}

	R_LoadModelIdentity ();
}


/*
=============
R_AddEntitiesToList
=============
*/
void R_AddEntitiesToList (void)
{
	int			i;

	r_numNullEntities = 0;
	if (!r_drawEntities->integer)
		return;

	// Add all entities to the list
	for (i=0 ; i<r_refScene.numEntities ; i++) {
		if (!r_refScene.entityList[i].model) {
			R_AddNullModelToList (&r_refScene.entityList[i]);
			continue;
		}

		if (r_refScene.mirrorView) {
			if (r_refScene.entityList[i].flags & RF_WEAPONMODEL)
				continue;
		}

		switch (r_refScene.entityList[i].model->type) {
		case MODEL_MD2:
		case MODEL_MD3:
			R_AddAliasModelToList (&r_refScene.entityList[i]);
			break;

		case MODEL_Q2BSP:
			R_AddQ2BrushModel (&r_refScene.entityList[i]);
			break;

		case MODEL_Q3BSP:
			R_AddQ3BrushModel (&r_refScene.entityList[i]);
			break;

		case MODEL_SP2:
			R_AddSP2ModelToList (&r_refScene.entityList[i]);
			break;

		case MODEL_BAD:
		default:
			R_AddNullModelToList (&r_refScene.entityList[i]);
			break;
		}
	}
}

/*
=============================================================================

	INITIALIZATION

=============================================================================
*/

/*
=============
R_EntityInit
=============
*/
void R_EntityInit (void)
{
	// Create the default entity
	memset (&r_refScene.defaultEntity, 0, sizeof (entity_t));
	r_refScene.defaultEntity.scale = 1.0f;
	Matrix3_Identity (r_refScene.defaultEntity.axis);
	VectorSet (r_refScene.defaultEntity.color, 255, 255, 255);
}
