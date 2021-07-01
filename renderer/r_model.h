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

typedef struct mAliasVertex_s {
	short			point[3];
	byte			latlong[2];		// use bytes to keep 8-byte alignment
} mAliasVertex_t;

typedef struct mAliasFrame_s {
	vec3_t			mins;
	vec3_t			maxs;

	vec3_t			scale;
	vec3_t			translate;
	float			radius;
} mAliasFrame_t;

typedef struct mAliasTag_s {
	char			tagName[MAX_QPATH];
	quat_t			quat;
	vec3_t			origin;
} mAliasTag_t;

typedef struct mAliasSkin_s {
	char			name[MAX_QPATH];
	image_t			*image;
	shader_t		*shader;
} mAliasSkin_t;

typedef struct mAliasMesh_s {
	char			meshName[MAX_QPATH];

	int				numVerts;
	mAliasVertex_t	*vertexes;
	vec2_t			*coords;

	int				numTris;
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
	image_t			*skin;
	shader_t		*shader;
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
#define SURF_DRAWTURB		0x10
#define SURF_DRAWBACKGROUND	0x40
#define SURF_UNDERWATER		0x80
#define SURF_LAVA			0x00100
#define SURF_SLIME			0x00200
#define SURF_WATER			0x00400

typedef struct mBspVertex_s {
	vec3_t				position;
} mBspVertex_t;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct mBspEdge_s {
	uShort	v[2];
	uInt	cachedEdgeOffset;
} mBspEdge_t;

typedef struct mBspTexInfo_s {
	float				vecs[2][4];
	int					flags;
	int					numFrames;

	struct				mBspTexInfo_s	*next;		// animation chain

	image_t				*image;
	shader_t			*shader;
} mBspTexInfo_t;

#define	VERTEXSIZE	7
typedef struct mBspPoly_s {
	struct mBspPoly_s	*next;
	struct mBspPoly_s	*chain;

	int					numverts;

	int					flags;
	float				verts[4][VERTEXSIZE];	// variable sized (xyz s1t1 s2t2)
} mBspPoly_t;

typedef struct mBspSurface_s {
	int					visFrame;		// should be drawn when node is crossed

	cBspPlane_t			*plane;
	int					flags;

	int					firstEdge;		// look up in model->surfedges[], negative numbers
	int					numEdges;		// are backwards edges
	
	short				textureMins[2];
	short				extents[2];

	mBspPoly_t			*polys;			// multiple if warped

	struct				mBspSurface_s	*textureChain;
	struct				mBspSurface_s	*lightMapChain;

	mBspTexInfo_t		*texInfo;
	
	// lighting info
	int					dLightFrame;
	int					dLightBits;
	ivec2_t				dLightCoords;	// gl lightmap coordinates for dynamic lightmaps

	float				c_s;
	float				c_t;

	int					lmTexNum;
	ivec2_t				lmCoords;		// gl lightmap coordinates
	int					lmWidth;
	int					lmHeight;
	byte				*lmSamples;		// [numstyles*surfsize]

	int					numStyles;
	byte				styles[BSP_MAX_LIGHTMAPS];
	float				cachedLight[BSP_MAX_LIGHTMAPS];	// values currently used in lightmap

	// spaz moving alpha surfs fix
	entity_t			*entity;

	// Vic's awesome decals
	int					fragmentFrame;
} mBspSurface_t;

typedef struct mBspNode_s {
	// common with leaf
	int					contents;			// -1, to differentiate from leafs
	int					visFrame;			// node needs to be traversed if current

	// for bounding box culling 
	float				mins[3];
	float				maxs[3];

	struct mBspNode_s	*parent;

	// node specific
	cBspPlane_t			*plane;
	struct mBspNode_s	*children[2];	

	uShort				firstSurface;
	uShort				numSurfaces;
} mBspNode_t;

typedef struct mBspLeaf_s {
	// common with node
	int					contents;			// wil be a negative contents number
	int					visFrame;			// node needs to be traversed if current

	// for bounding box culling
	float				mins[3];
	float				maxs[3];

	struct mBspNode_s	*parent;

	// leaf specific
	int					cluster;
	int					area;

	mBspSurface_t		**firstMarkSurface;
	int					numMarkSurfaces;
} mBspLeaf_t;

typedef struct mBspHeader_s {
	vec3_t				mins, maxs;
	vec3_t				origin;		// for sounds or lights
	float				radius;
	int					headNode;
	int					visLeafs;	// not including the solid leaf 0
	int					firstFace;
	int					numFaces;
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

	uInt			mRegistrationFrame;

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
