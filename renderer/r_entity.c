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

// r_entity.c

#include "r_local.h"

/*
=============================================================

	ENTITIES

=============================================================
*/

/*
=================
R_LoadModelIdentity

ala Vic
=================
*/
void R_LoadModelIdentity (void) {
	Matrix4_Copy (r_WorldViewMatrix, r_ModelViewMatrix);
	qglLoadMatrixf (r_WorldViewMatrix);
}

/*
=================
R_RotateForEntity

ala Vic
=================
*/
void R_RotateForEntity (entity_t *ent) {
	mat4_t	objectMatrix;

	if ((ent->scale != 1.0f) && ent->model && (ent->model->type == MODEL_BSP)) {
		objectMatrix[ 0] = ent->axis[0][0] * ent->scale;
		objectMatrix[ 1] = ent->axis[0][1] * ent->scale;
		objectMatrix[ 2] = ent->axis[0][2] * ent->scale;

		objectMatrix[ 4] = ent->axis[1][0] * ent->scale;
		objectMatrix[ 5] = ent->axis[1][1] * ent->scale;
		objectMatrix[ 6] = ent->axis[1][2] * ent->scale;

		objectMatrix[ 8] = ent->axis[2][0] * ent->scale;
		objectMatrix[ 9] = ent->axis[2][1] * ent->scale;
		objectMatrix[10] = ent->axis[2][2] * ent->scale;
	} else {
		objectMatrix[ 0] = ent->axis[0][0];
		objectMatrix[ 1] = ent->axis[0][1];
		objectMatrix[ 2] = ent->axis[0][2];

		objectMatrix[ 4] = ent->axis[1][0];
		objectMatrix[ 5] = ent->axis[1][1];
		objectMatrix[ 6] = ent->axis[1][2];

		objectMatrix[ 8] = ent->axis[2][0];
		objectMatrix[ 9] = ent->axis[2][1];
		objectMatrix[10] = ent->axis[2][2];
	}

	objectMatrix[ 3] = 0;
	objectMatrix[ 7] = 0;
	objectMatrix[11] = 0;
	objectMatrix[12] = ent->origin[0];
	objectMatrix[13] = ent->origin[1];
	objectMatrix[14] = ent->origin[2];
	objectMatrix[15] = 1.0;

	Matrix4_MultiplyFast (r_WorldViewMatrix, objectMatrix, r_ModelViewMatrix);

	qglLoadMatrixf (r_ModelViewMatrix);
}


/*
=================
R_RotateForAliasShadow

Not even sure if this is right...
=================
*/
void R_RotateForAliasShadow (entity_t *ent) {
	mat4_t	objectMatrix;

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

	Matrix4_MultiplyFast (r_WorldViewMatrix, objectMatrix, r_ModelViewMatrix);

	qglLoadMatrixf (r_ModelViewMatrix);
}


/*
=================
R_TranslateForEntity

ala Vic
=================
*/
void R_TranslateForEntity (entity_t *ent) {
	mat4_t	objectMatrix;

	Matrix4_Identity (objectMatrix);

	objectMatrix[12] = ent->origin[0];
	objectMatrix[13] = ent->origin[1];
	objectMatrix[14] = ent->origin[2];

	Matrix4_MultiplyFast (r_WorldViewMatrix, objectMatrix, r_ModelViewMatrix);

	qglLoadMatrixf (r_ModelViewMatrix);
}


/*
=============
R_DrawNullModel

Used when a model isn't found for the current entity
=============
*/
void R_DrawNullModel (entity_t *ent)
{
	vec3_t	shadelight;
	int		i;

	if (ent->flags & RF_VIEWERMODEL)
		return;

	if (ent->flags & RF_FULLBRIGHT)
		VectorSet (shadelight, 1, 1, 1);
	else
		R_LightPoint (ent->origin, shadelight);

	qglDisable (GL_TEXTURE_2D);
	qglColor3fv (shadelight);

	qglBegin (GL_TRIANGLE_FAN);

	qglVertex3f (0, 0, -16);
	for (i=0 ; i<=4 ; i++)
		qglVertex3f (16 * cos (i * M_PI_DIV2), 16 * sin (i * M_PI_DIV2), 0);
	qglEnd ();

	qglBegin (GL_TRIANGLE_FAN);
	qglVertex3f (0, 0, 16);
	for (i=4 ; i>=0 ; i--)
		qglVertex3f (16 * cos (i * M_PI_DIV2), 16 * sin (i * M_PI_DIV2), 0);

	qglEnd ();

	qglEnable (GL_TEXTURE_2D);
	qglColor4f (1, 1, 1, 1);
}


/*
=============
R_DrawEntities

Passes entities to their rendering function, non-transparent first
=============
*/
int entitycmpfnc (const entity_t *a, const entity_t *b) {
	// all other models are sorted by model then skin
	if (a->model == b->model)
		return ((int)a->skin - (int) b->skin);
	else
		return ((int)a->model - (int) b->model);
}
void R_DrawEntities (qBool vWeapOnly) {
	int			i;

	if (!r_drawentities->integer)
		return;

	// sort entities for better cache locality
	qsort (r_Entities, r_NumEntities, sizeof (r_Entities[0]), (int (*)(const void *, const void *))entitycmpfnc);

	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// draw non-transparent first
	for (i=0 ; i<r_NumEntities ; i++) {
		r_CurrEntity = &r_Entities[i];

		if (r_CurrEntity->flags & (RF_BEAM|RF_TRANSLUCENT))
			continue;

		if (vWeapOnly) {
			if (!(r_CurrEntity->flags & RF_WEAPONMODEL))
				continue;
		} else if (r_CurrEntity->flags & RF_WEAPONMODEL)
			continue;

		GL_ToggleOverbrights (qTrue);
		r_CurrModel = r_CurrEntity->model;
		if (!r_CurrModel) {
			R_DrawNullModel (r_CurrEntity);
			continue;
		}

		R_RotateForEntity (r_CurrEntity);
		switch (r_CurrModel->type) {
		case MODEL_MD2:
		case MODEL_MD3:
			R_DrawAliasModel (r_CurrEntity);
			break;
		case MODEL_BSP:
			R_DrawBrushModel (r_CurrEntity);
			break;
		case MODEL_SP2:
			R_DrawSpriteModel (r_CurrEntity);
			break;
		default:
			Com_Error (ERR_DROP, "Bad modeltype");
			break;
		}
		GL_ToggleOverbrights (qFalse);
	}

	// draw transparent entities
	qglDepthMask (GL_FALSE);
	qglEnable (GL_BLEND);
	for (i=0 ; i<r_NumEntities ; i++) {
		r_CurrEntity = &r_Entities[i];

		if (r_CurrEntity->flags & RF_BEAM)				continue;
		if (!(r_CurrEntity->flags & RF_TRANSLUCENT))	continue;

		if (vWeapOnly) {
			if (!(r_CurrEntity->flags & RF_WEAPONMODEL))
				continue;
		} else if (r_CurrEntity->flags & RF_WEAPONMODEL)
			continue;

		GL_ToggleOverbrights (qTrue);
		r_CurrModel = r_CurrEntity->model;
		if (!r_CurrModel) {
			R_DrawNullModel (r_CurrEntity);
			continue;
		}

		R_RotateForEntity (r_CurrEntity);
		switch (r_CurrModel->type) {
		case MODEL_MD2:
		case MODEL_MD3:
			R_DrawAliasModel (r_CurrEntity);
			break;
		case MODEL_BSP:
			R_DrawBrushModel (r_CurrEntity);
			break;
		case MODEL_SP2:
			R_DrawSpriteModel (r_CurrEntity);
			break;
		default:
			Com_Error (ERR_DROP, "Bad modeltype");
			break;
		}
		GL_ToggleOverbrights (qFalse);
	}
	qglDisable (GL_BLEND);
	qglDepthMask (GL_TRUE);

	R_LoadModelIdentity ();
}
