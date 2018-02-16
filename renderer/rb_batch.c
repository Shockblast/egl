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
// rb_batch.c
//

#include "rb_local.h"

/*
=============
RB_BackendOverflow
=============
*/
qBool RB_BackendOverflow (int numVerts, int numIndexes)
{
	return (rb.numVerts + numVerts > RB_MAX_VERTS
		|| rb.numIndexes + numIndexes > RB_MAX_INDEXES);
}


/*
=============
RB_InvalidMesh
=============
*/
qBool RB_InvalidMesh (const mesh_t *mesh)
{
	return (!mesh->numVerts
		|| !mesh->numIndexes
		|| mesh->numVerts > RB_MAX_VERTS
		|| mesh->numIndexes > RB_MAX_INDEXES);
}


/*
=============
RB_PushTriangleIndexes
=============
*/
#if SHADOW_VOLUMES
static inline void RB_PushTriangleIndexes (index_t *indexes, int *neighbors, vec3_t *trNormals, int count, meshFeatures_t meshFeatures)
{
	int		numTris;
#else
static inline void RB_PushTriangleIndexes (index_t *indexes, int count, meshFeatures_t meshFeatures)
{
#endif
	index_t	*currentIndex;

	if (meshFeatures & MF_NONBATCHED) {
		rb.numIndexes = count;
		rb.inIndices = indexes;

#ifdef SHADOW_VOLUMES
		if (neighbors) {
			rb.inNeighbors = neighbors;
			rb.curTrNeighbor = rb.inNeighbors + rb.numIndexes;
		}

		if (meshFeatures & MF_TRNORMALS && trNormals) {
			numTris = rb.numIndexes / 3;
			rb.inTrNormals = trNormals;
			rb.curTrNormal = rb.inTrNormals[0] + numTris;
		}
#endif
	}
	else {
#ifdef SHADOW_VOLUMES
		numTris = rb.numIndexes / 3;
#endif
		currentIndex = rb.batch.indices + rb.numIndexes;
		rb.numIndexes += count;
		rb.inIndices = rb.batch.indices;

		// The following code assumes that R_PushIndexes is fed with triangles
		for ( ; count>0 ; count-=3, indexes+=3, currentIndex+=3) {
			currentIndex[0] = rb.numVerts + indexes[0];
			currentIndex[1] = rb.numVerts + indexes[1];
			currentIndex[2] = rb.numVerts + indexes[2];

#if SHADOW_VOLUMES
			if (neighbors) {
				rb.curTrNeighbor[0] = numTris + neighbors[0];
				rb.curTrNeighbor[1] = numTris + neighbors[1];
				rb.curTrNeighbor[2] = numTris + neighbors[2];

				neighbors += 3;
				rb.curTrNeighbor += 3;
			}

			if (meshFeatures & MF_TRNORMALS && trNormals) {
				Vec3Copy (*trNormals, rb.curTrNormal);
				trNormals++;
				rb.curTrNormal += 3;
			}
#endif
		}
	}
}


/*
=============
R_PushTrifanIndexes
=============
*/
static inline void R_PushTrifanIndexes (int numVerts)
{
	index_t	*currentIndex;
	int		count;

	currentIndex = rb.batch.indices + rb.numIndexes;
	rb.numIndexes += (numVerts - 2) * 3;
	rb.inIndices = rb.batch.indices;

	for (count=2 ; count<numVerts ; count++, currentIndex+=3) {
		currentIndex[0] = rb.numVerts;
		currentIndex[1] = rb.numVerts + count - 1;
		currentIndex[2] = rb.numVerts + count;
	}
}


/*
=============
RB_PushMesh
=============
*/
void RB_PushMesh (mesh_t *mesh, meshFeatures_t meshFeatures)
{
	int		i;

	assert (mesh->numVerts);

	rb.curMeshFeatures = meshFeatures;

	// Push indexes
	if (meshFeatures & MF_TRIFAN)
		R_PushTrifanIndexes (mesh->numVerts);
	else
#if SHADOW_VOLUMES
		RB_PushTriangleIndexes (mesh->indexArray, mesh->trNeighborsArray, mesh->trNormalsArray, mesh->numIndexes, meshFeatures);
#else
		RB_PushTriangleIndexes (mesh->indexArray, mesh->numIndexes, meshFeatures);
#endif

	// Push the rest
	if (meshFeatures & MF_NONBATCHED) {
		rb.numVerts = 0;

		// Vertexes and normals
		if (meshFeatures & MF_DEFORMVS) {
			if (mesh->vertexArray != rb.batch.vertices) {
				for (i=0 ; i<mesh->numVerts ; i++) {
					rb.batch.vertices[rb.numVerts+i][0] = mesh->vertexArray[i][0];
					rb.batch.vertices[rb.numVerts+i][1] = mesh->vertexArray[i][1];
					rb.batch.vertices[rb.numVerts+i][2] = mesh->vertexArray[i][2];
				}
			}
			rb.inVertices = rb.batch.vertices;

			if (meshFeatures & MF_NORMALS && mesh->normalsArray) {
				if (mesh->normalsArray != rb.batch.normals) {
					for (i=0 ; i<mesh->numVerts ; i++) {
						rb.batch.normals[rb.numVerts+i][0] = mesh->normalsArray[i][0];
						rb.batch.normals[rb.numVerts+i][1] = mesh->normalsArray[i][1];
						rb.batch.normals[rb.numVerts+i][2] = mesh->normalsArray[i][2];
					}
				}
				rb.inNormals = rb.batch.normals;
			}
		}
		else {
			rb.inVertices = mesh->vertexArray;

			if (meshFeatures & MF_NORMALS && mesh->normalsArray)
				rb.inNormals = mesh->normalsArray;
		}

		// Texture coordinates
		if (meshFeatures & MF_STCOORDS && mesh->coordArray)
			rb.inCoords = mesh->coordArray;

		// Lightmap texture coordinates
		if (meshFeatures & MF_LMCOORDS && mesh->lmCoordArray)
			rb.inLMCoords = mesh->lmCoordArray;

		// STVectors
		if (meshFeatures & MF_STVECTORS) {
			if (mesh->sVectorsArray)
				rb.inSVectors = mesh->sVectorsArray;
			if (mesh->tVectorsArray)
				rb.inTVectors = mesh->tVectorsArray;
		}

		// Colors
		if (meshFeatures & MF_COLORS && mesh->colorArray) {
			for (i=0 ; i<mesh->numVerts ; i++) {
				rb.batch.colors[rb.numVerts+i][0] = mesh->colorArray[i][0];
				rb.batch.colors[rb.numVerts+i][1] = mesh->colorArray[i][1];
				rb.batch.colors[rb.numVerts+i][2] = mesh->colorArray[i][2];
				rb.batch.colors[rb.numVerts+i][3] = mesh->colorArray[i][3];
			}
			rb.inColors = rb.batch.colors;
		}
	}
	else {
		ri.pc.meshBatches++;

		// Vertexes and colors
		if (meshFeatures & MF_COLORS && mesh->colorArray) {
			for (i=0 ; i<mesh->numVerts ; i++) {
				rb.batch.vertices[rb.numVerts+i][0] = mesh->vertexArray[i][0];
				rb.batch.vertices[rb.numVerts+i][1] = mesh->vertexArray[i][1];
				rb.batch.vertices[rb.numVerts+i][2] = mesh->vertexArray[i][2];

				rb.batch.colors[rb.numVerts+i][0] = mesh->colorArray[i][0];
				rb.batch.colors[rb.numVerts+i][1] = mesh->colorArray[i][1];
				rb.batch.colors[rb.numVerts+i][2] = mesh->colorArray[i][2];
				rb.batch.colors[rb.numVerts+i][3] = mesh->colorArray[i][3];
			}
			rb.inColors = rb.batch.colors;
		}
		else {
			for (i=0 ; i<mesh->numVerts ; i++) {
				rb.batch.vertices[rb.numVerts+i][0] = mesh->vertexArray[i][0];
				rb.batch.vertices[rb.numVerts+i][1] = mesh->vertexArray[i][1];
				rb.batch.vertices[rb.numVerts+i][2] = mesh->vertexArray[i][2];
			}
		}
		rb.inVertices = rb.batch.vertices;

		// Normals
		if (meshFeatures & MF_NORMALS && mesh->normalsArray) {
			for (i=0 ; i<mesh->numVerts ; i++) {
				rb.batch.normals[rb.numVerts+i][0] = mesh->normalsArray[i][0];
				rb.batch.normals[rb.numVerts+i][1] = mesh->normalsArray[i][1];
				rb.batch.normals[rb.numVerts+i][2] = mesh->normalsArray[i][2];
			}
			rb.inNormals = rb.batch.normals;
		}

		// Texture coordinates
		if (meshFeatures & MF_STCOORDS && mesh->coordArray) {
			for (i=0 ; i<mesh->numVerts ; i++) {
				rb.batch.coords[rb.numVerts+i][0] = mesh->coordArray[i][0];
				rb.batch.coords[rb.numVerts+i][1] = mesh->coordArray[i][1];
			}
			rb.inCoords = rb.batch.coords;
		}

		// Lightmap texture coordinates
		if (meshFeatures & MF_LMCOORDS && mesh->lmCoordArray) {
			for (i=0 ; i<mesh->numVerts ; i++) {
				rb.batch.lmCoords[rb.numVerts+i][0] = mesh->lmCoordArray[i][0];
				rb.batch.lmCoords[rb.numVerts+i][1] = mesh->lmCoordArray[i][1];
			}
			rb.inLMCoords = rb.batch.lmCoords;
		}

		// STVectors
		if (meshFeatures & MF_STVECTORS) {
			if (mesh->sVectorsArray && mesh->tVectorsArray) {
				for (i=0 ; i<mesh->numVerts ; i++) {
					rb.batch.sVectors[rb.numVerts+i][0] = mesh->sVectorsArray[i][0];
					rb.batch.sVectors[rb.numVerts+i][1] = mesh->sVectorsArray[i][1];
					rb.batch.sVectors[rb.numVerts+i][2] = mesh->sVectorsArray[i][2];
					rb.batch.tVectors[rb.numVerts+i][0] = mesh->tVectorsArray[i][0];
					rb.batch.tVectors[rb.numVerts+i][1] = mesh->tVectorsArray[i][1];
					rb.batch.tVectors[rb.numVerts+i][2] = mesh->tVectorsArray[i][2];
				}
				rb.inSVectors = rb.batch.sVectors;
				rb.inTVectors = rb.batch.tVectors;
			}
			else {
				if (mesh->sVectorsArray) {
					for (i=0 ; i<mesh->numVerts ; i++) {
						rb.batch.sVectors[rb.numVerts+i][0] = mesh->sVectorsArray[i][0];
						rb.batch.sVectors[rb.numVerts+i][1] = mesh->sVectorsArray[i][1];
						rb.batch.sVectors[rb.numVerts+i][2] = mesh->sVectorsArray[i][2];
					}
					rb.inSVectors = rb.batch.sVectors;
				}
				if (mesh->tVectorsArray) {
					for (i=0 ; i<mesh->numVerts ; i++) {
						rb.batch.tVectors[rb.numVerts+i][0] = mesh->tVectorsArray[i][0];
						rb.batch.tVectors[rb.numVerts+i][1] = mesh->tVectorsArray[i][1];
						rb.batch.tVectors[rb.numVerts+i][2] = mesh->tVectorsArray[i][2];
					}
					rb.inTVectors = rb.batch.tVectors;
				}
			}
		}
	}

	rb.numVerts += mesh->numVerts;
}
