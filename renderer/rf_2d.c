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
// rf_2d.c
//

#include "rf_local.h"

/*
===============================================================================

	2D HANDLING

===============================================================================
*/

static mesh_t		rb_2DMesh;
static meshBuffer_t	rb_2DMBuffer;

static vec3_t		rb_2DVertices[4];
static vec3_t		rb_2DNormals[4] = { {0,1,0}, {0,1,0}, {0,1,0}, {0,1,0} };
static vec2_t		rb_2DTexCoords[4];
static bvec4_t		rb_2DColors[4];

/*
=============
R_DrawPic
=============
*/
void R_DrawPic (material_t *mat, float matTime, float x, float y, int w, int h, float s1, float t1, float s2, float t2, vec4_t color)
{
	meshFeatures_t	features;
	int				bColor;

	if (!mat)
		return;

	// FIXME: Normalize and FloatToByte?
	rb_2DColors[0][0] = (color[0] * 255);
	rb_2DColors[0][1] = (color[1] * 255);
	rb_2DColors[0][2] = (color[2] * 255);
	rb_2DColors[0][3] = (color[3] * 255);
	bColor = *(int *)rb_2DColors[0];

	rb_2DVertices[0][0] = x;
	rb_2DVertices[0][1] = y;
	rb_2DTexCoords[0][0] = s1;
	rb_2DTexCoords[0][1] = t1;

	rb_2DVertices[1][0] = x+w;
	rb_2DVertices[1][1] = y;
	rb_2DTexCoords[1][0] = s2;
	rb_2DTexCoords[1][1] = t1;
	*(int *)rb_2DColors[1] = bColor;

	rb_2DVertices[2][0] = x+w;
	rb_2DVertices[2][1] = y+h;
	rb_2DTexCoords[2][0] = s2;
	rb_2DTexCoords[2][1] = t2;
	*(int *)rb_2DColors[2] = bColor;

	rb_2DVertices[3][0] = x;
	rb_2DVertices[3][1] = y+h;
	rb_2DTexCoords[3][0] = s1;
	rb_2DTexCoords[3][1] = t2;
	*(int *)rb_2DColors[3] = bColor;

	rb_2DMBuffer.mat = mat;
	rb_2DMBuffer.matTime = matTime;

	features = MF_TRIFAN|mat->features;
	if (gl_shownormals->intVal)
		features |= MF_NORMALS;
//	if (!(mat->flags & MAT_ENTITY_MERGABLE) || r_debugBatching->intVal == 2)
		features |= MF_NONBATCHED;

	RB_PushMesh (&rb_2DMesh, features);
	RB_RenderMeshBuffer (&rb_2DMBuffer, qFalse);
}


/*
=============
R_DrawRectangle
=============
*/
void R_DrawRectangle (material_t *mat, float matTime, vec2_t tl, vec2_t tr, vec2_t br, vec2_t bl, float s1, float t1, float s2, float t2, vec4_t color)
{
	meshFeatures_t	features;
	int				bColor;

	if (!mat)
		return;

	// FIXME: Normalize and FloatToByte?
	rb_2DColors[0][0] = (color[0] * 255);
	rb_2DColors[0][1] = (color[1] * 255);
	rb_2DColors[0][2] = (color[2] * 255);
	rb_2DColors[0][3] = (color[3] * 255);
	bColor = *(int *)rb_2DColors[0];

	rb_2DVertices[0][0] = tl[0];
	rb_2DVertices[0][1] = tl[1];
	rb_2DTexCoords[0][0] = s1;
	rb_2DTexCoords[0][1] = t1;

	rb_2DVertices[1][0] = tr[0];
	rb_2DVertices[1][1] = tr[1];
	rb_2DTexCoords[1][0] = s2;
	rb_2DTexCoords[1][1] = t1;
	*(int *)rb_2DColors[1] = bColor;

	rb_2DVertices[2][0] = br[0];
	rb_2DVertices[2][1] = br[1];
	rb_2DTexCoords[2][0] = s2;
	rb_2DTexCoords[2][1] = t2;
	*(int *)rb_2DColors[2] = bColor;

	rb_2DVertices[3][0] = bl[0];
	rb_2DVertices[3][1] = bl[1];
	rb_2DTexCoords[3][0] = s1;
	rb_2DTexCoords[3][1] = t2;
	*(int *)rb_2DColors[3] = bColor;

	rb_2DMBuffer.mat = mat;
	rb_2DMBuffer.matTime = matTime;

	features = MF_TRIFAN|mat->features;
	if (gl_shownormals->intVal)
		features |= MF_NORMALS;
//	if (!(mat->flags & MAT_ENTITY_MERGABLE) || r_debugBatching->intVal == 2)
		features |= MF_NONBATCHED;

	RB_PushMesh (&rb_2DMesh, features);
	RB_RenderMeshBuffer (&rb_2DMBuffer, qFalse);
}

/*
===============================================================================

	INIT / SHUTDOWN

===============================================================================
*/

/*
=============
RF_2DInit
=============
*/
void RF_2DInit (void)
{
	rb_2DVertices[0][2] = 1;
	rb_2DVertices[1][2] = 1;
	rb_2DVertices[2][2] = 1;
	rb_2DVertices[3][2] = 1;

	rb_2DMesh.numIndexes = 0;
	rb_2DMesh.numVerts = 4;

	rb_2DMesh.colorArray = rb_2DColors;
	rb_2DMesh.coordArray = rb_2DTexCoords;
	rb_2DMesh.indexArray = NULL;
	rb_2DMesh.lmCoordArray = NULL;
	rb_2DMesh.normalsArray = rb_2DNormals;
	rb_2DMesh.sVectorsArray = NULL;
	rb_2DMesh.tVectorsArray = NULL;
	rb_2DMesh.trNeighborsArray = NULL;
	rb_2DMesh.trNormalsArray = NULL;
	rb_2DMesh.vertexArray = rb_2DVertices;

	rb_2DMBuffer.sortKey = 0;
	rb_2DMBuffer.entity = ri.scn.defaultEntity;
	rb_2DMBuffer.mesh = NULL;
}
