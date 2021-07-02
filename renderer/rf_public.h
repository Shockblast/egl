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
// rf_public.h
//

/*
=============================================================================

	MESH BUFFERING

=============================================================================
*/

#define MAX_MESH_BUFFER			8192
#define MAX_ADDITIVE_BUFFER		8192
#define MAX_POSTPROC_BUFFER		64

#define MAX_MESH_KEYS			(SHADER_SORT_OPAQUE+1)
#define MAX_ADDITIVE_KEYS		(SHADER_SORT_NEAREST-SHADER_SORT_OPAQUE)

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
	MBT_DECAL,
	MBT_POLY,
	MBT_Q2BSP,
	MBT_Q3BSP,
	MBT_Q3BSP_FLARE,
	MBT_SKY,
	MBT_SP2,

	MBT_MAX				= 16
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

	refEntity_t				*entity;
	shader_t				*shader;
	struct mQ3BspFog_s		*fog;
	void					*mesh;
} meshBuffer_t;

typedef struct meshList_s {
	qBool					skyDrawn;
	float					skyMins[6][2];
	float					skyMaxs[6][2];

	int						numMeshes[MAX_MESH_KEYS];
	meshBuffer_t			meshBuffer[MAX_MESH_KEYS][MAX_MESH_BUFFER];

	int						numAdditiveMeshes[MAX_ADDITIVE_KEYS];
	meshBuffer_t			meshBufferAdditive[MAX_ADDITIVE_KEYS][MAX_ADDITIVE_BUFFER];

	int						numPostProcessMeshes;
	meshBuffer_t			meshBufferPostProcess[MAX_POSTPROC_BUFFER];
} meshList_t;

extern meshList_t	r_portalList;
extern meshList_t	r_worldList;
extern meshList_t	*r_currentList;

void	R_SortMeshList (void);
void	R_DrawMeshList (qBool triangleOutlines);
void	R_DrawMeshOutlines (void);
