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
// rb_entity.c
// Entity handling and null model rendering
//

#include "rb_local.h"

static uint32		rb_numNullEntities;
static refEntity_t	*rb_nullEntities[MAX_REF_ENTITIES];

/*
=============================================================================

	ENTITY ORIENTATION

=============================================================================
*/

/*
=================
RB_LoadModelIdentity

ala Vic
=================
*/
void RB_LoadModelIdentity (void)
{
	Matrix4_Copy (ri.scn.worldViewMatrix, ri.scn.modelViewMatrix);
	qglLoadMatrixf (ri.scn.worldViewMatrix);
}


/*
=================
RB_RotateForEntity

ala Vic
=================
*/
void RB_RotateForEntity (refEntity_t *ent)
{
	mat4x4_t	objectMatrix;

	Matrix3_Matrix4 (ent->axis, ent->origin, objectMatrix);
	if (ent->model && ent->model->isBspModel && ent->scale != 1.0f)
		Matrix4_Scale (objectMatrix, ent->scale, ent->scale, ent->scale);
	Matrix4_MultiplyFast (ri.scn.worldViewMatrix, objectMatrix, ri.scn.modelViewMatrix);

	qglLoadMatrixf (ri.scn.modelViewMatrix);
}


/*
=================
RB_RotateForAliasShadow

Not even sure if this is right...
=================
*/
void RB_RotateForAliasShadow (refEntity_t *ent)
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

	Matrix4_MultiplyFast (ri.scn.worldViewMatrix, objectMatrix, ri.scn.modelViewMatrix);

	qglLoadMatrixf (ri.scn.modelViewMatrix);
}


/*
=================
RB_TranslateForEntity

ala Vic
=================
*/
void RB_TranslateForEntity (refEntity_t *ent)
{
	mat4x4_t	objectMatrix;

	Matrix4_Identity (objectMatrix);

	objectMatrix[12] = ent->origin[0];
	objectMatrix[13] = ent->origin[1];
	objectMatrix[14] = ent->origin[2];

	Matrix4_MultiplyFast (ri.scn.worldViewMatrix, objectMatrix, ri.scn.modelViewMatrix);

	qglLoadMatrixf (ri.scn.modelViewMatrix);
}


/*
=============================================================================

	NULL MODEL ENTITY HANDLING

=============================================================================
*/

/*
=============
RB_AddNullModelToList

Used when a model isn't found for the current entity
=============
*/
void RB_AddNullModelToList (refEntity_t *ent)
{
	rb_nullEntities[rb_numNullEntities++] = ent;
}


/*
=============
RB_DrawNullModel

Used when a model isn't found for the current entity
=============
*/
static void RB_DrawNullModel (refEntity_t *ent)
{
	vec3_t	shadelight;
	int		i;

	if (ent->flags & (RF_VIEWERMODEL|RF_DEPTHHACK))
		return;

	if (ent->flags & RF_FULLBRIGHT)
		Vec3Set (shadelight, 1, 1, 1);
	else
		R_LightPoint (ent->origin, shadelight);

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
}


/*
=============
RB_DrawNullModelList
=============
*/
void RB_DrawNullModelList (void)
{
	uint32	i;

	if (!rb_numNullEntities)
		return;

	RB_StateForBits (SB1_DEPTHMASK_ON|SB1_DEFAULT);
	RB_TextureTarget (0);

	for (i=0 ; i<rb_numNullEntities ; i++) {
		if (ri.scn.mirrorView) {
			if (rb_nullEntities[i]->flags & RF_WEAPONMODEL) 
				continue;
		}
		else {
			if (rb_nullEntities[i]->flags & RF_VIEWERMODEL) 
				continue;
		}

		RB_RotateForEntity (rb_nullEntities[i]);
		RB_DrawNullModel (rb_nullEntities[i]);
	}

	RB_TextureTarget (GL_TEXTURE_2D);
	RB_StateForBits (SB1_DEFAULT);

	qglColor4f (1, 1, 1, 1);

	RB_LoadModelIdentity ();
	rb_numNullEntities = 0;
}
