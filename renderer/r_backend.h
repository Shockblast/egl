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
// r_backend.h
// Refresh backend header
//

/*
=============================================================================

	MESH BUFFERING

=============================================================================
*/

#define MAX_MESH_BUFFER				16384

enum { // meshFeatures_t
	MF_NONBATCHED		= 1 << 0,
	MF_NORMALS			= 1 << 1,
	MF_STCOORDS			= 1 << 2,
	MF_LMCOORDS			= 1 << 3,
	MF_COLORS			= 1 << 4,
	MF_TRNORMALS		= 1 << 5,
	MF_NOCULL			= 1 << 6,
	MF_DEFORMVS			= 1 << 7,
	MF_STVECTORS		= 1 << 8,
	MF_TRIFAN			= 1 << 9,
	MF_STATIC_MESH		= 1 << 10,
};

enum { // meshType_t
	MBT_2D,
	MBT_ALIAS,
	MBT_POLY,
	MBT_Q2BSP,
	MBT_Q3BSP,
	MBT_Q3BSP_FLARE,
	MBT_SKY,
	MBT_SP2,

	MBT_MAX
};

typedef struct mesh_s {
	int						numIndexes;
	int						numVerts;

	bvec4_t					*colorArray;
	vec2_t					*coordArray;
	vec2_t					*lmCoordArray;
	index_t					*indexArray;
	vec3_t					*normalsArray;
	vec3_t					*sVectorsArray;
	vec3_t					*tVectorsArray;
	index_t					*trNeighborsArray;
	vec3_t					*trNormalsArray;
	vec3_t					*vertexArray;
} mesh_t;

typedef struct meshBuffer_s {
	uint32					sortKey;
	float					shaderTime;

	entity_t				*entity;
	shader_t				*shader;
	struct mQ3BspFog_s		*fog;
	void					*mesh;
} meshBuffer_t;

#define MAX_MESH_KEYS		(SHADER_SORT_OPAQUE+1)
#define MAX_ADDITIVE_KEYS	(SHADER_SORT_NEAREST-SHADER_SORT_OPAQUE)

typedef struct meshList_s {
	qBool					skyDrawn;
	float					skyMins[6][2];
	float					skyMaxs[6][2];

	int						numMeshes[MAX_MESH_KEYS];
	meshBuffer_t			meshBuffer[MAX_MESH_KEYS][MAX_MESH_BUFFER];

	int						numAdditiveMeshes[MAX_ADDITIVE_KEYS];
	meshBuffer_t			meshBufferAdditive[MAX_ADDITIVE_KEYS][MAX_MESH_BUFFER];

	int						numPostProcessMeshes;
	meshBuffer_t			meshBufferPostProcess[MAX_MESH_BUFFER];
} meshList_t;

extern meshList_t	r_portalList;
extern meshList_t	r_worldList;
extern meshList_t	*r_currentList;

meshBuffer_t	*R_AddMeshToList (shader_t *shader, float shaderTime, entity_t *ent, struct mQ3BspFog_s *fog, meshType_t meshType, void *mesh);
void	R_SortMeshList (void);
void	R_DrawMeshList (qBool triangleOutlines);
void	R_DrawMeshOutlines (void);

/*
=============================================================================

	BACKEND

=============================================================================
*/

#define VERTARRAY_MAX_VERTS			4096
#define VERTARRAY_MAX_INDEXES		VERTARRAY_MAX_VERTS*6
#define VERTARRAY_MAX_TRIANGLES		VERTARRAY_MAX_INDEXES/3
#define VERTARRAY_MAX_NEIGHBORS		VERTARRAY_MAX_TRIANGLES*3

#define QUAD_INDEXES				6
#define TRIFAN_INDEXES				((VERTARRAY_MAX_VERTS-2)*3)

extern bvec4_t			rb_colorArray[VERTARRAY_MAX_VERTS];
extern vec2_t			rb_coordArray[VERTARRAY_MAX_VERTS];
extern index_t			rb_indexArray[VERTARRAY_MAX_INDEXES];
extern vec2_t			rb_lMCoordArray[VERTARRAY_MAX_VERTS];
extern vec3_t			rb_normalsArray[VERTARRAY_MAX_VERTS];
extern vec3_t			rb_sVectorsArray[VERTARRAY_MAX_VERTS];
extern vec3_t			rb_tVectorsArray[VERTARRAY_MAX_VERTS];
extern vec3_t			rb_vertexArray[VERTARRAY_MAX_VERTS];

#ifdef SHADOW_VOLUMES
extern int				rb_neighborsArray[VERTARRAY_MAX_NEIGHBORS];
extern vec3_t			rb_trNormalsArray[VERTARRAY_MAX_TRIANGLES];
#endif

extern bvec4_t			*rb_inColorArray;
extern vec2_t			*rb_inCoordArray;
extern index_t			*rb_inIndexArray;
extern vec2_t			*rb_inLMCoordArray;
extern vec3_t			*rb_inNormalsArray;
extern vec3_t			*rb_inSVectorsArray;
extern vec3_t			*rb_inTVectorsArray;
extern vec3_t			*rb_inVertexArray;
extern int				rb_numIndexes;
extern int				rb_numVerts;

#ifdef SHADOW_VOLUMES
extern int				*rb_inNeighborsArray;
extern vec3_t			*rb_inTrNormalsArray;
extern int				*rb_currentTrNeighbor;
extern float			*rb_currentTrNormal;
#endif

extern meshFeatures_t	rb_currentMeshFeatures;

extern index_t			rb_quadIndices[QUAD_INDEXES];
extern index_t			rb_triFanIndices[TRIFAN_INDEXES];

float	R_FastSin (float t);

void	RB_LockArrays (int numVerts);
void	RB_UnlockArrays (void);
void	RB_ResetPointers (void);

void	RB_DrawElements (meshType_t meshType);
void	RB_RenderMeshBuffer (meshBuffer_t *mb, qBool shadowPass);
void	RB_FinishRendering (void);

void	RB_BeginTriangleOutlines (void);
void	RB_EndTriangleOutlines (void);

void	RB_BeginFrame (void);
void	RB_EndFrame (void);

void	RB_Init (void);
void	RB_Shutdown (void);

/*
===============================================================================

	MESH PUSHING

===============================================================================
*/

/*
=============
RB_BackendOverflow
=============
*/
static inline qBool RB_BackendOverflow (const mesh_t *mesh)
{
	return (rb_numVerts + mesh->numVerts > VERTARRAY_MAX_VERTS || 
		rb_numIndexes + mesh->numIndexes > VERTARRAY_MAX_INDEXES);
}


/*
=============
RB_InvalidMesh
=============
*/
static inline qBool RB_InvalidMesh (const mesh_t *mesh)
{
	return (!mesh->numVerts || !mesh->numIndexes || 
		mesh->numVerts > VERTARRAY_MAX_VERTS || mesh->numIndexes > VERTARRAY_MAX_INDEXES);
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
		rb_numIndexes = count;
		rb_inIndexArray = indexes;

#ifdef SHADOW_VOLUMES
		if (neighbors) {
			rb_inNeighborsArray = neighbors;
			rb_currentTrNeighbor = rb_inNeighborsArray + rb_numIndexes;
		}

		if (meshFeatures & MF_TRNORMALS && trNormals) {
			numTris = rb_numIndexes / 3;
			rb_inTrNormalsArray = trNormals;
			rb_currentTrNormal = rb_inTrNormalsArray[0] + numTris;
		}
#endif
	}
	else {
#ifdef SHADOW_VOLUMES
		numTris = rb_numIndexes / 3;
#endif
		currentIndex = rb_indexArray + rb_numIndexes;
		rb_numIndexes += count;
		rb_inIndexArray = rb_indexArray;

		// The following code assumes that R_PushIndexes is fed with triangles
		for ( ; count>0 ; count -= 3, indexes += 3, currentIndex += 3) {
			currentIndex[0] = rb_numVerts + indexes[0];
			currentIndex[1] = rb_numVerts + indexes[1];
			currentIndex[2] = rb_numVerts + indexes[2];

#if SHADOW_VOLUMES
			if (neighbors) {
				rb_currentTrNeighbor[0] = numTris + neighbors[0];
				rb_currentTrNeighbor[1] = numTris + neighbors[1];
				rb_currentTrNeighbor[2] = numTris + neighbors[2];

				neighbors += 3;
				rb_currentTrNeighbor += 3;
			}

			if (meshFeatures & MF_TRNORMALS && trNormals) {
				VectorCopy (*trNormals, rb_currentTrNormal);
				trNormals++;
				rb_currentTrNormal += 3;
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

	currentIndex = rb_indexArray + rb_numIndexes;
	rb_numIndexes += numVerts + numVerts + numVerts - 6;
	rb_inIndexArray = rb_indexArray;

	for (count=2 ; count<numVerts ; count++, currentIndex += 3) {
		currentIndex[0] = rb_numVerts;
		currentIndex[1] = rb_numVerts + count - 1;
		currentIndex[2] = rb_numVerts + count;
	}
}


/*
=============
RB_PushMesh
=============
*/
static inline RB_PushMesh (mesh_t *mesh, meshFeatures_t meshFeatures)
{
	rb_currentMeshFeatures = meshFeatures;

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
		rb_numVerts = 0;

		// Vertexes and normals
		if (meshFeatures & MF_DEFORMVS) {
			if (mesh->vertexArray != rb_vertexArray)
				memcpy (rb_vertexArray[rb_numVerts], mesh->vertexArray, mesh->numVerts * sizeof (vec3_t));
			rb_inVertexArray = rb_vertexArray;

			if (meshFeatures & MF_NORMALS && mesh->normalsArray) {
				if (mesh->normalsArray != rb_normalsArray)
					memcpy (rb_normalsArray[rb_numVerts], mesh->normalsArray, mesh->numVerts * sizeof (vec3_t));
				rb_inNormalsArray = rb_normalsArray;
			}
		}
		else {
			rb_inVertexArray = mesh->vertexArray;

			if (meshFeatures & MF_NORMALS && mesh->normalsArray)
				rb_inNormalsArray = mesh->normalsArray;
		}

		// Texture coordinates
		if (meshFeatures & MF_STCOORDS && mesh->coordArray)
			rb_inCoordArray = mesh->coordArray;

		// Lightmap texture coordinates
		if (meshFeatures & MF_LMCOORDS && mesh->lmCoordArray)
			rb_inLMCoordArray = mesh->lmCoordArray;

		// STVectors
		if (meshFeatures & MF_STVECTORS) {
			if (mesh->sVectorsArray)
				rb_inSVectorsArray = mesh->sVectorsArray;
			if (mesh->tVectorsArray)
				rb_inTVectorsArray = mesh->tVectorsArray;
		}
	}
	else {
		// Vertexes
		memcpy (rb_vertexArray[rb_numVerts], mesh->vertexArray, mesh->numVerts * sizeof (vec3_t));
		rb_inVertexArray = rb_vertexArray;

		// Normals
		if (meshFeatures & MF_NORMALS && mesh->normalsArray) {
			memcpy (rb_normalsArray[rb_numVerts], mesh->normalsArray, mesh->numVerts * sizeof (vec3_t));
			rb_inNormalsArray = rb_normalsArray;
		}

		// Texture coordinates
		if (meshFeatures & MF_STCOORDS && mesh->coordArray) {
			memcpy (rb_coordArray[rb_numVerts], mesh->coordArray, mesh->numVerts * sizeof (vec2_t));
			rb_inCoordArray = rb_coordArray;
		}

		// Lightmap texture coordinates
		if (meshFeatures & MF_LMCOORDS && mesh->lmCoordArray) {
			memcpy (rb_lMCoordArray[rb_numVerts], mesh->lmCoordArray, mesh->numVerts * sizeof (vec2_t));
			rb_inLMCoordArray = rb_lMCoordArray;
		}

		// STVectors
		if (meshFeatures & MF_STVECTORS) {
			if (mesh->sVectorsArray) {
				memcpy (rb_sVectorsArray[rb_numVerts], mesh->sVectorsArray, mesh->numVerts * sizeof (vec3_t));
				rb_inSVectorsArray = mesh->sVectorsArray;
			}
			if (mesh->tVectorsArray) {
				memcpy (rb_tVectorsArray[rb_numVerts], mesh->tVectorsArray, mesh->numVerts * sizeof (vec3_t));
				rb_inTVectorsArray = mesh->tVectorsArray;
			}
		}
	}

	// Colors
	if (meshFeatures & MF_COLORS && mesh->colorArray) {
		memcpy (rb_colorArray[rb_numVerts], mesh->colorArray, mesh->numVerts * sizeof (bvec4_t));
		rb_inColorArray = rb_colorArray;
	}

	rb_numVerts += mesh->numVerts;
}
