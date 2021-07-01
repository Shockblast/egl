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
// r_model.h
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
	shader_t		*skin;
} mAliasSkin_t;

typedef struct mAliasTag_s {
	char			name[MAX_QPATH];
	quat_t			quat;
	vec3_t			origin;
} mAliasTag_t;

typedef struct mAliasVertex_s {
	short			point[3];
	byte			latLong[2];
} mAliasVertex_t;

typedef struct mAliasMesh_s {
	char			name[MAX_QPATH];

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
	shader_t		*skin;
} mSpriteFrame_t;

typedef struct mSpriteModel_s {
	int				numFrames;
	mSpriteFrame_t	*frames;			// variable sized
} mSpriteModel_t;

/*
==============================================================================

	BRUSH MODELS

==============================================================================
*/

#define	SIDE_FRONT			0
#define	SIDE_BACK			1
#define	SIDE_ON				2

#define	SURF_PLANEBACK		2
#define	SURF_DRAWSKY		4
#define SURF_DRAWTURB		0x0010
#define SURF_DRAWBACKGROUND	0x0040
#define SURF_UNDERWATER		0x0080
#define SURF_LAVA			0x0100
#define SURF_SLIME			0x0200
#define SURF_WATER			0x0400

typedef struct mBspVertex_s {
	vec3_t					position;
} mBspVertex_t;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct mBspEdge_s {
	uShort					v[2];
	uInt					cachedEdgeOffset;
} mBspEdge_t;

typedef struct mBspTexInfo_s {
	char					texName[MAX_QPATH];
	shader_t				*shader;

	float					vecs[2][4];
	int						flags;
	int						numFrames;

	int						surfParams;

	struct mBspTexInfo_s	*next;		// animation chain
} mBspTexInfo_t;

typedef struct mBspPoly_s {
	struct mBspPoly_s		*next;
	struct mesh_s			mesh;
} mBspPoly_t;

typedef struct mBspSurface_s {
	uLong					surfFrame;		// should be drawn when node is crossed

	cBspPlane_t				*plane;
	int						flags;

	int						firstEdge;		// look up in model->surfedges[], negative numbers
	int						numEdges;		// are backwards edges

	vec3_t					mins, maxs;

	svec2_t					textureMins;
	svec2_t					extents;

	mBspPoly_t				*polys;			// multiple if subdivided
	mBspTexInfo_t			*texInfo;
	
	// Lighting info
	uLong					dLightFrame;
	int						dLightBits;
	ivec2_t					dLightCoords;	// gl lightmap coordinates for dynamic lightmaps

	int						lmTexNumActive;	// Updated lightmap being used this frame for this surface
	int						lmTexNum;
	ivec2_t					lmCoords;		// gl lightmap coordinates
	int						lmWidth;
	int						lmHeight;
	byte					*lmSamples;		// [numstyles*surfsize]

	int						numStyles;
	byte					styles[BSP_MAX_LIGHTMAPS];
	float					cachedLight[BSP_MAX_LIGHTMAPS];	// values currently used in lightmap

	// Decals
	uLong					fragmentFrame;
} mBspSurface_t;

typedef struct mBspNode_s {
	// common with leaf
	int						contents;			// -1, to differentiate from leafs
	uLong					visFrame;			// node needs to be traversed if current

	// for bounding box culling
	vec3_t					mins;
	vec3_t					maxs;

	struct mBspNode_s		*parent;

	// node specific
	cBspPlane_t				*plane;
	struct mBspNode_s		*children[2];	

	uShort					firstSurface;
	uShort					numSurfaces;
} mBspNode_t;

typedef struct mBspLeaf_s {
	// common with node
	int						contents;			// not -1, to differentiate from nodes
	uLong					visFrame;			// node needs to be traversed if current

	// for bounding box culling
	vec3_t					mins;
	vec3_t					maxs;

	struct mBspNode_s		*parent;

	// leaf specific
	int						cluster;
	int						area;

	mBspSurface_t			**firstMarkSurface;
	int						numMarkSurfaces;
} mBspLeaf_t;

typedef struct mBspHeader_s {
	vec3_t					mins, maxs;
	vec3_t					origin;		// for sounds or lights
	float					radius;
	int						headNode;
	int						visLeafs;	// not including the solid leaf 0
	int						firstFace;
	int						numFaces;
} mBspHeader_t;

/*
==============================================================================

	WHOLE MODEL

==============================================================================
*/

enum {
	MODEL_BAD,

	MODEL_BSP,
	MODEL_MD2,
	MODEL_MD3,
	MODEL_SP2
};

typedef struct model_s {
	char			name[MAX_QPATH];

	uLong			touchFrame;
	qBool			finishedLoading;

	uInt			memTag;		// memory tag
	int				memSize;	// size in memory

	uInt			type;

	//
	// volume occupied by the model graphics
	//		
	vec3_t			mins;
	vec3_t			maxs;
	float			radius;

	//
	// brush models
	//
	int				firstModelSurface;
	int				numModelSurfaces;

	int				numSubModels;
	mBspHeader_t	*subModels;

	int				numPlanes;
	cBspPlane_t		*planes;

	int				numLeafs;		// number of visible leafs, not counting 0
	mBspLeaf_t		*leafs;

	int				numVertexes;
	mBspVertex_t	*vertexes;

	int				numEdges;
	mBspEdge_t		*edges;

	int				numNodes;
	int				firstNode;
	mBspNode_t		*nodes;

	int				numTexInfo;
	mBspTexInfo_t	*texInfo;

	int				numSurfaces;
	mBspSurface_t	*surfaces;

	int				numSurfEdges;
	int				*surfEdges;

	int				numMarkSurfaces;
	mBspSurface_t	**markSurfaces;

	dBspVis_t		*vis;

	byte			*lightData;

	//
	// alias models
	//
	mAliasModel_t	*aliasModel;

	//
	// sprite models
	//
	mSpriteModel_t	*spriteModel;
} model_t;
