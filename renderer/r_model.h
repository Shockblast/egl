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
	shader_t		*skin;
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
	shader_t				*shader;
} mQ3BspShaderRef_t;

typedef struct mQ3BspFog_s {
	char					name[MAX_QPATH];
	shader_t				*shader;

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
	shader_t				*shader;

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
	uint32					q2_surfFrame;		// should be drawn when node is crossed

	cBspPlane_t				*q2_plane;
	int						q2_flags;

	int						q2_firstEdge;		// look up in model->surfedges[], negative numbers
	int						q2_numEdges;		// are backwards edges

	svec2_t					q2_textureMins;
	svec2_t					q2_extents;

	mQ2BspPoly_t			*q2_polys;			// multiple if subdivided
	mQ2BspTexInfo_t			*q2_texInfo;

	ivec2_t					q2_dLightCoords;	// gl lightmap coordinates for dynamic lightmaps

	int						q2_lmTexNumActive;	// Updated lightmap being used this frame for this surface
	ivec2_t					q2_lmCoords;		// gl lightmap coordinates
	int						q2_lmWidth;
	int						q2_lmHeight;
	byte					*q2_lmSamples;		// [numstyles*surfsize]

	int						q2_numStyles;
	byte					q2_styles[Q2BSP_MAX_LIGHTMAPS];
	float					q2_cachedLight[Q2BSP_MAX_LIGHTMAPS];	// values currently used in lightmap

	// Quake3 BSP specific
	uint32					q3_visFrame;		// should be drawn when node is crossed
	int						q3_faceType;

	mQ3BspShaderRef_t		*q3_shaderRef;

	mesh_t					*q3_mesh;
	mQ3BspFog_t				*q3_fog;

    vec3_t					q3_origin;

	uint32					q3_patchWidth;
	uint32					q3_patchHeight;

	// Common between Quake2 and Quake3 BSP formats
	vec3_t					mins, maxs;

	uint32					fragmentFrame;

	int						lmTexNum;
	uint32					dLightFrame;
	uint32					dLightBits;
} mBspSurface_t;

typedef struct mBspNode_s {
	// common with leaf
	cBspPlane_t				*plane;
	int						q2_contents;		// -1, to differentiate from leafs
	uint32					visFrame;			// node needs to be traversed if current

	// for bounding box culling
	vec3_t					mins;
	vec3_t					maxs;

	struct mBspNode_s		*parent;

	// node specific
	struct mBspNode_s		*children[2];	

	uint16					q2_firstSurface;
	uint16					q2_numSurfaces;
} mBspNode_t;

typedef struct mBspLeaf_s {
	// Common with node
	cBspPlane_t				*q3_plane;
	int						q2_contents;		// not -1, to differentiate from nodes
	uint32					visFrame;			// node needs to be traversed if current

	vec3_t					mins;
	vec3_t					maxs;

	struct mBspNode_s		*parent;

	// Leaf specific
	int						cluster;
	int						area;

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
	int						numSubModels;
	mBspHeader_t			*subModels;

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

typedef struct model_s {
	char					name[MAX_QPATH];

	uint32					touchFrame;
	qBool					finishedLoading;
	qBool					isBspModel;

	uint32					memTag;		// memory tag
	int						memSize;	// size in memory

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
} model_t;

//
// r_alias.c
//

void		R_AddAliasModelToList (entity_t *ent);
void		R_DrawAliasModel (meshBuffer_t *mb, qBool shadowPass);

//
// r_model.c
//

byte		*R_Q2BSPClusterPVS (int cluster, model_t *model);
mBspLeaf_t	*R_PointInQ2BSPLeaf (float *point, model_t *model);

mQ3BspFog_t	*R_FogForSphere (const vec3_t center, const float radius);

byte		*R_Q3BSPClusterPVS (int cluster, model_t *model);
mBspLeaf_t	*R_PointInQ3BSPLeaf (vec3_t p, model_t *model);

qBool		R_Q3BSP_SurfPotentiallyFragmented (mBspSurface_t *surf);
qBool		R_Q3BSP_SurfPotentiallyLit (mBspSurface_t *surf);
qBool		R_Q3BSP_SurfPotentiallyVisible (mBspSurface_t *surf);

void		R_RegisterMap (char *mapName);
model_t		*R_RegisterModel (char *name);

void		R_ModelBounds (model_t *model, vec3_t mins, vec3_t maxs);

void		R_BeginModelRegistration (void);
void		R_EndModelRegistration (void);

void		R_ModelInit (void);
void		R_ModelShutdown (void);

//
// r_sprite.c
//

void		R_AddSP2ModelToList (entity_t *ent);
void		R_DrawSP2Model (meshBuffer_t *mb);

qBool		R_FlareOverflow (void);
void		R_PushFlare (meshBuffer_t *mb);
