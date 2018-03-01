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
// rb_public.h
// Refresh backend header
//

/*
=============================================================================

	BACKEND

=============================================================================
*/

#define RB_MAX_VERTS			4096
#define RB_MAX_INDEXES			RB_MAX_VERTS*6
#define RB_MAX_TRIANGLES		RB_MAX_INDEXES/3
#define RB_MAX_NEIGHBORS		RB_MAX_TRIANGLES*3

// FIXME: this could be made local to the backend
typedef struct rbData_batch_s {
	bvec4_t					colors[RB_MAX_VERTS];
	vec2_t					coords[RB_MAX_VERTS];
	index_t					indices[RB_MAX_INDEXES];
	vec2_t					lmCoords[RB_MAX_VERTS];
	vec3_t					normals[RB_MAX_VERTS];
	vec3_t					sVectors[RB_MAX_VERTS];
	vec3_t					tVectors[RB_MAX_VERTS];
	vec3_t					vertices[RB_MAX_VERTS];

#ifdef SHADOW_VOLUMES
	int						neighbors[RB_MAX_NEIGHBORS];
	vec3_t					trNormals[RB_MAX_TRIANGLES];
#endif
} rbData_batch_t;

typedef struct rbData_s {
	// Batch buffers are used for MAT_ENTITY_MERGABLE materials and for
	// storage to pass to the backend on non-MAT_ENTITY_MERGABLE materials.
	rbData_batch_t			batch;

	// Input data pointers for rendering a mesh buffer
	bvec4_t					*inColors;
	vec2_t					*inCoords;
	index_t					*inIndices;
	vec2_t					*inLMCoords;
	vec3_t					*inNormals;
	vec3_t					*inSVectors;
	vec3_t					*inTVectors;
	vec3_t					*inVertices;

#ifdef SHADOW_VOLUMES
	int						*inNeighbors;
	vec3_t					*inTrNormals;
	int						*curTrNeighbor;
	float					*curTrNormal;
#endif

	refEntity_t				*curEntity;
	uint32					curDLightBits;
	int						curLMTexNum;
	meshFeatures_t			curMeshFeatures;
	meshType_t				curMeshType;
	struct refModel_s		*curModel;
	uint32					curPatchWidth;
	uint32					curPatchHeight;
	material_t				*curMat;

	struct mQ3BspFog_s		*curColorFog;
	struct mQ3BspFog_s		*curTexFog;

	int						numIndexes;
	int						numVerts;
} rbData_t;

extern rbData_t	rb;

/*
=============================================================================

	STATE COMPRESSION

=============================================================================
*/

enum {
	SB1_BLENDSRC_ZERO					= 0x1,
	SB1_BLENDSRC_ONE					= 0x2,
	SB1_BLENDSRC_DST_COLOR				= 0x3,
	SB1_BLENDSRC_ONE_MINUS_DST_COLOR	= 0x4,
	SB1_BLENDSRC_SRC_ALPHA				= 0x5,
	SB1_BLENDSRC_ONE_MINUS_SRC_ALPHA	= 0x6,
	SB1_BLENDSRC_DST_ALPHA				= 0x7,
	SB1_BLENDSRC_ONE_MINUS_DST_ALPHA	= 0x8,
	SB1_BLENDSRC_SRC_ALPHA_SATURATE		= 0x9,
	SB1_BLENDSRC_BITS					= 0xf,

	SB1_BLENDDST_ZERO					= 0x10,
	SB1_BLENDDST_ONE					= 0x20,
	SB1_BLENDDST_SRC_COLOR				= 0x30,
	SB1_BLENDDST_ONE_MINUS_SRC_COLOR	= 0x40,
	SB1_BLENDDST_SRC_ALPHA				= 0x50,
	SB1_BLENDDST_ONE_MINUS_SRC_ALPHA	= 0x60,
	SB1_BLENDDST_DST_ALPHA				= 0x70,
	SB1_BLENDDST_ONE_MINUS_DST_ALPHA	= 0x80,
	SB1_BLENDDST_BITS					= 0xf0,

	SB1_BLEND_ON						= 0x100,
	SB1_DEPTHMASK_ON					= 0x1000,
	SB1_DEPTHTEST_ON					= 0x10000,
	SB1_POLYOFFSET_ON					= 0x100000,

	SB1_CULL_FRONT						= 0x1000000,
	SB1_CULL_BACK						= 0x2000000,
	SB1_CULL_BITS						= 0xf000000,

	SB1_ATEST_GT0						= 0x10000000,
	SB1_ATEST_LT128						= 0x20000000,
	SB1_ATEST_GE128						= 0x40000000,
	SB1_ATEST_BITS						= 0xf0000000,
};

#define SB1_DEFAULT		(SB1_BLENDSRC_SRC_ALPHA|SB1_BLENDDST_ONE_MINUS_SRC_ALPHA|SB1_DEPTHTEST_ON)

/*
=============================================================================

	ENTITY HANDLING

=============================================================================
*/

void		RB_LoadModelIdentity (void);
void		RB_RotateForEntity (refEntity_t *ent);
void		RB_RotateForAliasShadow (refEntity_t *ent);
void		RB_TranslateForEntity (refEntity_t *ent);

void		RB_AddNullModelToList (refEntity_t *ent);
void		RB_DrawNullModelList (void);
