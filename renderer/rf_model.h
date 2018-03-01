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
// rf_model.h
// Memory representation of the different model types
//

/*
==============================================================================

	ALIAS MODELS

==============================================================================
*/

#define ALIAS_MAX_VERTS		4096
#define ALIAS_MAX_LODS		4

typedef struct mAliasFrame_s {
	vec3_t			mins;
	vec3_t			maxs;

	vec3_t			scale;
	vec3_t			translate;
	float			radius;
} mAliasFrame_t;

typedef struct mAliasSkin_s {
	char			name[MAX_QPATH];
	material_t		*material;
} mAliasSkin_t;

typedef struct mAliasTag_s {
	char			name[MAX_QPATH];
	quat_t			quat;
	vec3_t			origin;
} mAliasTag_t;

typedef struct mAliasVertex_s {
	int16			point[3];
	byte			latLong[2];
} mAliasVertex_t;

typedef struct mAliasMesh_s {
	char			name[MAX_QPATH];

	vec3_t			*mins;
	vec3_t			*maxs;
	float			*radius;

	int				numVerts;
	mAliasVertex_t	*vertexes;
	vec2_t			*coords;

	int				numTris;
	int				*neighbors;
	index_t			*indexes;

	int				numSkins;
	mAliasSkin_t	*skins;
} mAliasMesh_t;

typedef struct mAliasModel_s {
	int				numFrames;
	mAliasFrame_t	*frames;

	int				numTags;
	mAliasTag_t		*tags;

	int				numMeshes;
	mAliasMesh_t	*meshes;
} mAliasModel_t;

/*
========================================================================

	SPRITE MODELS

========================================================================
*/

typedef struct mSpriteFrame_s {
	// dimensions
	int				width;
	int				height;

	float			radius;

	// raster coordinates inside pic
	int				originX;
	int				originY;

	// texturing
	char			name[MAX_QPATH];	// name of pcx file
	material_t		*material;
} mSpriteFrame_t;

typedef struct mSpriteModel_s {
	int				numFrames;
	mSpriteFrame_t	*frames;			// variable sized
} mSpriteModel_t;

/*
==============================================================================

	QUAKE2 BSP BRUSH MODELS

==============================================================================
*/

#define SIDE_FRONT			0
#define SIDE_BACK			1
#define SIDE_ON				2

#define SURF_PLANEBACK		2
#define SURF_DRAWSKY		4
#define SURF_DRAWTURB		0x0010
#define SURF_DRAWBACKGROUND	0x0040
#define SURF_UNDERWATER		0x0080
#define SURF_LAVA			0x0100
#define SURF_SLIME			0x0200
#define SURF_WATER			0x0400

// ===========================================================================
//
// Q3BSP
//

typedef struct mQ3BspShaderRef_s
{
	char					name[MAX_QPATH];
	int						flags;
	int						contents;
	material_t				*mat;
} mQ3BspShaderRef_t;

typedef struct mQ3BspFog_s {
	char					name[MAX_QPATH];
	material_t				*mat;

	cBspPlane_t				*visiblePlane;

	int						numPlanes;
	cBspPlane_t				*planes;
} mQ3BspFog_t;

typedef struct mQ3BspLight_s {
	vec3_t					origin;
	vec3_t					color;
	float					intensity;
} mQ3BspLight_t;

typedef struct mQ3BspGridLight_s {
	byte					ambient[3];
	byte					diffuse[3];
	byte					direction[2];
} mQ3BspGridLight_t;

typedef struct mQ3BspLightmapRect_s {
	int						texNum;

	float					x, y;
	float					w, h;
} mQ3BspLightmapRect_t;

// ===========================================================================
//
// Q2BSP
//

typedef struct mQ2BspVertex_s {
	vec3_t					position;
} mQ2BspVertex_t;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct mQ2BspEdge_s {
	uint16					v[2];
	uint32					cachedEdgeOffset;
} mQ2BspEdge_t;

typedef struct mQ2BspTexInfo_s {
	char					texName[MAX_QPATH];
	material_t				*mat;

	int						width;
	int						height;

	float					vecs[2][4];
	int						flags;
	int						numFrames;

	int						surfParams;

	struct mQ2BspTexInfo_s	*next;		// animation chain
} mQ2BspTexInfo_t;

typedef struct mQ2BspPoly_s {
	struct mQ2BspPoly_s		*next;
	struct mesh_s			mesh;
} mQ2BspPoly_t;

// ===========================================================================
//
// Q2 Q3 BSP COMMON
//

typedef struct mBspSurface_s {
	// Quake2 BSP specific
	cBspPlane_t				*q2_plane;
	int						q2_flags;

	int						q2_firstEdge;		// look up in model->surfedges[], negative numbers
	int						q2_numEdges;		// are backwards edges

	svec2_t					q2_textureMins;
	svec2_t					q2_extents;

	mQ2BspPoly_t			*q2_polys;			// multiple if subdivided
	mQ2BspTexInfo_t			*q2_texInfo;

	ivec2_t					q2_dLightCoords;	// gl lightmap coordinates for dynamic lightmaps

	uint32					q2_lmFrame;
	int						q2_lmTexNumActive;	// Updated lightmap being used this frame for this surface
	ivec2_t					q2_lmCoords;		// gl lightmap coordinates
	int						q2_lmWidth;
	int						q2_lmHeight;
	byte					*q2_lmSamples;		// [numstyles*surfsize]

	int						q2_numStyles;
	byte					q2_styles[Q2BSP_MAX_LIGHTMAPS];
	float					q2_cachedLight[Q2BSP_MAX_LIGHTMAPS];	// values currently used in lightmap
	float					*q2_blockLights;

	// Quake3 BSP specific
	uint32					q3_nodeFrame;		// used so we don't have to recurse EVERY frame

	int						q3_faceType;

	mQ3BspFog_t				*q3_fog;
	mQ3BspShaderRef_t		*q3_shaderRef;

    vec3_t					q3_origin;

	uint32					q3_patchWidth;
	uint32					q3_patchHeight;

	// Common between Quake2 and Quake3 BSP formats
	uint32					visFrame;			// should be drawn when node is crossed

	vec3_t					mins, maxs;

	mesh_t					*mesh;

	uint32					fragmentFrame;

	int						lmTexNum;
	uint32					dLightFrame;
	uint32					dLightBits;
} mBspSurface_t;

typedef struct mNodeLeafShared_s {
	uint32					visFrame;			// node needs to be traversed if current

	cBspPlane_t				*plane;				// Only Q3BSP uses this in leafs
	int						q2_contents;		// -1, to differentiate from leafs

	// For bounding box culling
	qBool					badBounds;
	vec3_t					mins;
	vec3_t					maxs;

	struct mBspNode_s		*parent;
} mNodeLeafShared_t;

typedef struct mBspNode_s {
	// Common with leaf
	mNodeLeafShared_t		c;

	// Node specific
	struct mBspNode_s		*children[2];	

	mBspSurface_t			**q2_firstVisSurface;
	mBspSurface_t			**q2_firstLitSurface;
} mBspNode_t;

typedef struct mBspLeaf_s {
	// Common with node
	mNodeLeafShared_t		c;

	// Leaf specific
	int						cluster;
	int						area;

	mBspSurface_t			**q2_firstDecalSurface;
	mBspSurface_t			**q2_firstMarkSurface;
	int						q2_numMarkSurfaces;

	mBspSurface_t			**q3_firstVisSurface;
	mBspSurface_t			**q3_firstLitSurface;
	mBspSurface_t			**q3_firstFragmentSurface;
} mBspLeaf_t;

typedef struct mBspHeader_s {
	vec3_t					mins, maxs;
	float					radius;
	vec3_t					origin;		// for sounds or lights

	int						firstFace;
	int						numFaces;

	// Quake2 BSP specific
	int						headNode;
	int						visLeafs;	// not including the solid leaf 0
} mBspHeader_t;

/*
==============================================================================

	BASE MODEL STRUCT

==============================================================================
*/

enum { // modelType_t;
	MODEL_BAD,

	MODEL_Q2BSP,
	MODEL_Q3BSP,
	MODEL_MD2,
	MODEL_MD3,
	MODEL_SP2
};

typedef struct mBspModel_s {
	// Common between Quake2 and Quake3 BSP formats
	uint32					numSubModels;
	mBspHeader_t			*subModels;
	struct refModel_s		*inlineModels;

	int						numModelSurfaces;
	mBspSurface_t			*firstModelSurface;

	int						numLeafs;		// number of visible leafs, not counting 0
	mBspLeaf_t				*leafs;

	int						numNodes;
	mBspNode_t				*nodes;

	int						numPlanes;
	cBspPlane_t				*planes;

	int						numSurfaces;
	mBspSurface_t			*surfaces;
} mBspModel_t;

typedef struct mQ2BspModel_s {
	int						numVertexes;
	mQ2BspVertex_t			*vertexes;

	int						numEdges;
	mQ2BspEdge_t			*edges;

	int						firstNode;

	int						numTexInfo;
	mQ2BspTexInfo_t			*texInfo;

	int						numSurfEdges;
	int						*surfEdges;

	int						numMarkSurfaces;
	mBspSurface_t			**markSurfaces;

	dQ2BspVis_t				*vis;

	byte					*lightData;
} mQ2BspModel_t;

typedef struct mQ3BspModel_s {
	vec3_t					gridSize;
	vec3_t					gridMins;
	int						gridBounds[4];

	int						numVertexes;
	vec3_t					*vertexArray;
	vec3_t					*normalsArray;		// normals
	vec2_t					*coordArray;		// texture coords		
	vec2_t					*lmCoordArray;		// lightmap texture coords
	bvec4_t					*colorArray;		// colors used for vertex lighting

	int						numSurfIndexes;
	int						*surfIndexes;

	int						numShaderRefs;
	mQ3BspShaderRef_t		*shaderRefs;

	int						numLightGridElems;
	mQ3BspGridLight_t		*lightGrid;

	int						numFogs;
	mQ3BspFog_t				*fogs;

	int						numWorldLights;
	mQ3BspLight_t			*worldLights;

	int						numLightmaps;
	mQ3BspLightmapRect_t	*lightmapRects;

	dQ3BspVis_t				*vis;
} mQ3BspModel_t;

typedef struct refModel_s {
	char					name[MAX_QPATH];
	char					bareName[MAX_QPATH];

	uint32					touchFrame;
	qBool					isBspModel;

	uint32					memTag;		// memory tag
	size_t					memSize;	// size in memory

	uint32					hashValue;
	struct refModel_s		*hashNext;

	modelType_t				type;

	//
	// volume occupied by the model graphics
	//		
	vec3_t					mins;
	vec3_t					maxs;
	float					radius;

	//
	// brush models
	//
	mBspModel_t				bspModel;

	//
	// q2 brush models
	//
	mQ2BspModel_t			q2BspModel;

	//
	// q3 brush models
	//
	mQ3BspModel_t			q3BspModel;

	//
	// alias models
	//
	mAliasModel_t			*aliasModel;

	//
	// sprite models
	//
	mSpriteModel_t			*spriteModel;
} refModel_t;

//
// rf_alias.c
//

void		R_AddAliasModelToList (refEntity_t *ent);
void		R_DrawAliasModel (meshBuffer_t *mb, qBool shadowPass);

//
// rf_model.c
//

byte		*R_Q2BSPClusterPVS (int cluster, refModel_t *model);
mBspLeaf_t	*R_PointInQ2BSPLeaf (float *point, refModel_t *model);

mQ3BspFog_t	*R_FogForSphere (const vec3_t center, const float radius);

byte		*R_Q3BSPClusterPVS (int cluster, refModel_t *model);
mBspLeaf_t	*R_PointInQ3BSPLeaf (vec3_t p, refModel_t *model);

qBool		R_Q3BSP_SurfPotentiallyFragmented (mBspSurface_t *surf);
qBool		R_Q3BSP_SurfPotentiallyLit (mBspSurface_t *surf);
qBool		R_Q3BSP_SurfPotentiallyVisible (mBspSurface_t *surf);

void		R_EndModelRegistration (void);

void		R_ModelInit (void);
void		R_ModelShutdown (void);

//
// rf_sprite.c
//

void		R_AddSP2ModelToList (refEntity_t *ent);
void		R_DrawSP2Model (meshBuffer_t *mb);

qBool		R_FlareOverflow (void);
void		R_PushFlare (meshBuffer_t *mb);
