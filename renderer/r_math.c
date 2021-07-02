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
// r_math.c
// Renderer math library
//

#include "r_local.h"

/*
=============
R_SetupProjectionMatrix
=============
*/
static inline float R_CalcZFar (void)
{
	float	forwardDist, viewDist;
	float	forwardClip;
	float	skySize;
	int		i;

	if (ri.scn.worldModel->type == MODEL_Q3BSP) {
		// FIXME: adjust for skydome height * 2!
		skySize = SKY_BOXSIZE * 2;
	}
	else {
		// FIXME: make this dependant on if there's a skybox or not
		skySize = SKY_BOXSIZE * 2;
	}

	forwardDist = DotProduct (ri.def.viewOrigin, ri.def.viewAxis[0]);
	forwardClip = skySize + forwardDist;

	viewDist = 0;
	for (i=0 ; i<3 ; i++) {
		if (ri.def.viewAxis[0][i] < 0)
			viewDist += ri.scn.worldModel->bspModel.nodes[0].c.mins[i] * ri.def.viewAxis[0][i];
		else
			viewDist += ri.scn.worldModel->bspModel.nodes[0].c.maxs[i] * ri.def.viewAxis[0][i];
	}
	if (viewDist > forwardClip)
		forwardClip = viewDist;

	// Bias by 256 pixels
	return max (forwardClip - forwardDist + 256, ri.scn.zFar);
}
void R_SetupProjectionMatrix (refDef_t *rd, mat4x4_t m)
{
	GLfloat		xMin, xMax;
	GLfloat		yMin, yMax;
	GLfloat		zNear;
	GLfloat		vAspect = (float)rd->width/(float)rd->height;

	// Near/far clip
	zNear = r_zNear->intVal;
	if (r_zFarAbs->intVal) {
		ri.scn.zFar = r_zFarAbs->intVal;
	}
	else {
		if (ri.def.rdFlags & RDF_NOWORLDMODEL)
			ri.scn.zFar = 2048;
		else
			ri.scn.zFar = r_zFarMin->intVal + R_CalcZFar ();
	}

	// Calculate aspect
	yMax = zNear * (float)tan ((rd->fovY * M_PI) / 360.0f);
	yMin = -yMax;

	xMin = yMin * vAspect;
	xMax = yMax * vAspect;

	xMin += -(2 * ri.cameraSeparation) / zNear;
	xMax += -(2 * ri.cameraSeparation) / zNear;

	// Apply to matrix
	m[0] = (2.0f * zNear) / (xMax - xMin);
	m[1] = 0.0f;
	m[2] = 0.0f;
	m[3] = 0.0f;
	m[4] = 0.0f;
	m[5] = (2.0f * zNear) / (yMax - yMin);
	m[6] = 0.0f;
	m[7] = 0.0f;
	m[8] = (xMax + xMin) / (xMax - xMin);
	m[9] = (yMax + yMin) / (yMax - yMin);
	m[10] = -(ri.scn.zFar + zNear) / (ri.scn.zFar - zNear);
	m[11] = -1.0f;
	m[12] = 0.0f;
	m[13] = 0.0f;
	m[14] = -(2.0f * ri.scn.zFar * zNear) / (ri.scn.zFar - zNear);
	m[15] = 0.0f;
}


/*
=============
R_SetupModelviewMatrix
=============
*/
void R_SetupModelviewMatrix (refDef_t *rd, mat4x4_t m)
{
#if 0
	Matrix4_Identity (m);
	Matrix4_Rotate (m, -90, 1, 0, 0);
	Matrix4_Rotate (m,  90, 0, 0, 1);
#else
	Vec4Set (&m[0], 0, 0, -1, 0);
	Vec4Set (&m[4], -1, 0, 0, 0);
	Vec4Set (&m[8], 0, 1, 0, 0);
	Vec4Set (&m[12], 0, 0, 0, 1);
#endif

	Matrix4_Rotate (m, -rd->viewAngles[2], 1, 0, 0);
	Matrix4_Rotate (m, -rd->viewAngles[0], 0, 1, 0);
	Matrix4_Rotate (m, -rd->viewAngles[1], 0, 0, 1);
	Matrix4_Translate (m, -rd->viewOrigin[0], -rd->viewOrigin[1], -rd->viewOrigin[2]);
}


/*
===============
Matrix4_Multiply_Vector
===============
*/
void Matrix4_Multiply_Vector (const mat4x4_t m, const vec4_t v, vec4_t out)
{
	out[0] = m[0] * v[0] + m[4] * v[1] + m[8] * v[2] + m[12] * v[3];
	out[1] = m[1] * v[0] + m[5] * v[1] + m[9] * v[2] + m[13] * v[3];
	out[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14] * v[3];
	out[3] = m[3] * v[0] + m[7] * v[1] + m[11] * v[2] + m[15] * v[3];
}
