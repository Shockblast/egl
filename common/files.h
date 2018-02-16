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

#ifndef __FILES_H__
#define __FILES_H__

//
// files.h: quake file formats
// This file must be identical in the quake and utils directories
//

/*
=============================================================================

	PAK FILE FORMAT

	The .pak files are just a linear collapse of a directory tree
=============================================================================
*/

#define PAK_HEADER		(('K'<<24)+('C'<<16)+('A'<<8)+'P')	// little-endian "PACK"
#define PAK_HEADERSTR	"PACK"

#define PAK_MAX_FILES	4096

typedef struct dPackFile_s {
	char		name[56];

	int			filePos;
	int			fileLen;
} dPackFile_t;

typedef struct dPackHeader_s {
	int			ident;		// == PAK_HEADER
	int			dirOfs;
	int			dirLen;
} dPackHeader_t;

/*
=============================================================================

	PKZ FILE FORMAT

	Essentially a zip file, PK2/PK3/PK4 are evidentally the same
=============================================================================
*/

#define PKZ_MAX_FILES	32768

/*
=============================================================================

	PCX IMAGE FILE FORMAT

=============================================================================
*/

typedef struct pcxHeader_s {
	char				manufacturer;
	char				version;
	char				encoding;
	char				bitsPerPixel;

	uint16				xMin, yMin, xMax, yMax;
	uint16				hRes, vRes;
	byte				palette[48];

	char				reserved;
	char				colorPlanes;

	uint16				bytesPerLine;
	uint16				paletteType;

	char				filler[58];

	byte				data;			// unbounded
} pcxHeader_t;

/*
==============================================================================

	TGA IMAGE FILE FORMAT

==============================================================================
*/

typedef struct tgaHeader_s {
	byte	idLength;
	byte	colorMapType;
	byte	imageType;

	uint16	colorMapIndex;
	uint16	colorMapLength;
	byte	colorMapSize;

	uint16	xOrigin;
	uint16	yOrigin;
	uint16	width;
	uint16	height;

	byte	pixelSize;

	byte	attributes;
} tgaHeader_t;

/*
==============================================================================

	WAL IMAGE FILE FORMAT

==============================================================================
*/

typedef struct walTex_s {
	char		name[32];
	uint32		width;
	uint32		height;
	uint32		offsets[4];				// four mip maps stored
	char		animName[32];			// next frame in animation chain
	int			flags;
	int			contents;
	int			value;
} walTex_t;

/*
=============================================================================

	MD2 MODEL FILE FORMAT

=============================================================================
*/

#define MD2_HEADER		(('2'<<24)+('P'<<16)+('D'<<8)+'I')	// little-endian "IDP2"
#define MD2_HEADERSTR	"IDP2"

#define MD2_MODEL_VERSION	8

#define MD2_MAX_TRIANGLES	4096
#define MD2_MAX_VERTS		2048
#define MD2_MAX_FRAMES		512
#define MD2_MAX_SKINS		32
#define MD2_MAX_SKINNAME	64

typedef struct dMd2Coord_s {
	int16			s;
	int16			t;
} dMd2Coord_t;

typedef struct dMd2Triangle_s {
	int16			vertsIndex[3];
	int16			stIndex[3];
} dMd2Triangle_t;

#define DTRIVERTX_V0   0
#define DTRIVERTX_V1   1
#define DTRIVERTX_V2   2
#define DTRIVERTX_LNI  3
#define DTRIVERTX_SIZE 4

typedef struct dMd2TriVertX_s {
	byte			v[3];			// scaled byte to fit in frame mins/maxs
	byte			normalIndex;
} dMd2TriVertX_t;

typedef struct dMd2Frame_s {
	float			scale[3];		// multiply byte verts by this
	float			translate[3];	// then add this
	char			name[16];		// frame name from grabbing
	dMd2TriVertX_t	verts[1];		// variable sized
} dMd2Frame_t;

// the glcmd format:
// a positive integer starts a tristrip command, followed by that many
// vertex structures.
// a negative integer starts a trifan command, followed by -x vertexes
// a zero indicates the end of the command list.
// a vertex consists of a floating point s, a floating point t,
// and an integer vertex index.

typedef struct dMd2Header_s {
	int				ident;
	int				version;

	int				skinWidth;
	int				skinHeight;
	int				frameSize;		// byte size of each frame

	int				numSkins;
	int				numVerts;
	int				numST;			// greater than numVerts for seams
	int				numTris;
	int				numGLCmds;		// dwords in strip/fan command list
	int				numFrames;

	int				ofsSkins;		// each skin is a MD2_MAX_SKINNAME string
	int				ofsST;			// byte offset from start for stverts
	int				ofsTris;		// offset for dtriangles
	int				ofsFrames;		// offset for first frame
	int				ofsGLCmds;
	int				ofsEnd;			// end of file
} dMd2Header_t;

/*
=============================================================================

	MD3 MODEL FILE FORMAT

=============================================================================
*/

#define MD3_HEADER		(('3'<<24)+('P'<<16)+('D'<<8)+'I')	// little-endian "IDP3"
#define MD3_HEADERSTR	"IDP3"

#define MD3_MODEL_VERSION	15

#define MD3_MAX_TRIANGLES	8192		// per mesh
#define MD3_MAX_VERTS		4096		// per mesh
#define MD3_MAX_SHADERS		256			// per mesh
#define MD3_MAX_FRAMES		1024		// per model
#define MD3_MAX_MESHES		32			// per model
#define MD3_MAX_TAGS		16			// per frame

#define MD3_XYZ_SCALE		(1.0f/64)	// vertex scales

typedef struct dMd3Coord_s {
	float			st[2];
} dMd3Coord_t;

typedef struct dMd3Vertex_s {
	int16			point[3];
	byte			norm[2];
} dMd3Vertex_t;

typedef struct dMd3Frame_s {
	vec3_t			mins;
	vec3_t			maxs;

	vec3_t			translate;
	float			radius;
	char			creator[16];
} dMd3Frame_t;

typedef struct dMd3Tag_s {
	char			tagName[MAX_QPATH];
	float			origin[3];
	float			axis[3][3];
} dMd3Tag_t;

typedef struct dMd3Skin_s {
	char			name[MAX_QPATH];
	int				unused;					// shader
} dMd3Skin_t;

typedef struct dMd3Mesh_s {
	char			ident[4];

	char			meshName[MAX_QPATH];

	int				flags;

	int				numFrames;
	int				numSkins;
	int				numVerts;
	int				numTris;

	int				ofsIndexes;
	int				ofsSkins;
	int				ofsTCs;
	int				ofsVerts;

	int				meshSize;
} dMd3Mesh_t;

typedef struct dMd3Header_s {
	int				ident;
	int				version;

	char			fileName[MAX_QPATH];

	int				flags;

	int				numFrames;
	int				numTags;
	int				numMeshes;
	int				numSkins;

	int				ofsFrames;
	int				ofsTags;
	int				ofsMeshes;
	int				ofsEnd;
} dMd3Header_t;

/*
=============================================================================

	SP2 SPRITE MODEL FILE FORMAT

=============================================================================
*/

#define SP2_HEADER		(('2'<<24)+('S'<<16)+('D'<<8)+'I')	// little-endian "IDS2"
#define SP2_HEADERSTR	"IDS2"

#define SP2_VERSION		2

#define SP2_MAX_FRAMES	32

typedef struct dSpriteFrame_s {
	int			width;
	int			height;

	// raster coordinates inside pic
	int			originX;
	int			originY;

	char		name[MAX_QPATH];			// name of pcx file
} dSpriteFrame_t;

typedef struct dSpriteHeader_s {
	int				ident;
	int				version;

	int				numFrames;
	dSpriteFrame_t	frames[SP2_MAX_FRAMES];	// variable sized
} dSpriteHeader_t;

/*
=============================================================================

	BSP FILE FORMAT COMMON

=============================================================================
*/

// Key / value pair sizes
#define MAX_KEY					32
#define MAX_VALUE				1024

/*
=============================================================================

	QUAKE2 BSP FILE FORMAT

=============================================================================
*/

// Header information
#define Q2BSP_HEADER	"IBSP"
#define Q2BSP_VERSION	38

// Upper design bounds
// leaffaces, leafbrushes, planes, and verts are still bounded by 16 bit int16(short) limits
#define Q2BSP_MAX_MODELS		1024
#define Q2BSP_MAX_BRUSHES		8192
#define Q2BSP_MAX_ENTITIES		2048
#define Q2BSP_MAX_ENTSTRING		0x40000
#define Q2BSP_MAX_TEXINFO		8192

#define Q2BSP_MAX_AREAPORTALS	1024
#define Q2BSP_MAX_PLANES		65536
#define Q2BSP_MAX_NODES			65536
#define Q2BSP_MAX_BRUSHSIDES	65536
#define Q2BSP_MAX_LEAFS			65536
#define Q2BSP_MAX_VERTS			65536
#define Q2BSP_MAX_FACES			65536
#define Q2BSP_MAX_LEAFFACES		65536
#define Q2BSP_MAX_LEAFBRUSHES	65536
#define Q2BSP_MAX_PORTALS		65536
#define Q2BSP_MAX_EDGES			128000
#define Q2BSP_MAX_SURFEDGES		256000
#define Q2BSP_MAX_LIGHTING		0x200000
#define Q2BSP_MAX_VISIBILITY	0x100000

#define Q2BSP_MAX_VIS			8192	// (Q2BSP_MAX_LEAFS / 8)
#define Q2BSP_MAX_LIGHTMAPS		4

// ==========================================================================

#define Q2BSP_LUMP_ENTITIES		0
#define Q2BSP_LUMP_PLANES		1
#define Q2BSP_LUMP_VERTEXES		2
#define Q2BSP_LUMP_VISIBILITY	3
#define Q2BSP_LUMP_NODES		4
#define Q2BSP_LUMP_TEXINFO		5
#define Q2BSP_LUMP_FACES		6
#define Q2BSP_LUMP_LIGHTING		7
#define Q2BSP_LUMP_LEAFS		8
#define Q2BSP_LUMP_LEAFFACES	9
#define Q2BSP_LUMP_LEAFBRUSHES	10
#define Q2BSP_LUMP_EDGES		11
#define Q2BSP_LUMP_SURFEDGES	12
#define Q2BSP_LUMP_MODELS		13
#define Q2BSP_LUMP_BRUSHES		14
#define Q2BSP_LUMP_BRUSHSIDES	15
#define Q2BSP_LUMP_POP			16
#define Q2BSP_LUMP_AREAS		17
#define Q2BSP_LUMP_AREAPORTALS	18
#define Q2BSP_LUMP_TOTAL		19

typedef struct dQ2BspLump_s {
	int				fileOfs;
	int				fileLen;
} dQ2BspLump_t;

typedef struct dQ2BspHeader_s {
	int				ident;
	int				version;
	
	dQ2BspLump_t	lumps[Q2BSP_LUMP_TOTAL];
} dQ2BspHeader_t;

typedef struct dQ2BspModel_s {
	float			mins[3], maxs[3];

	float			origin[3];		// for sounds or lights

	int				headNode;

	int				firstFace;	// submodels just draw faces
	int				numFaces;	// without walking the bsp tree
} dQ2BspModel_t;

typedef struct dQ2BspVertex_s {
	float			point[3];
} dQ2BspVertex_t;

// planes (x&~1) and (x&~1)+1 are always opposites
typedef struct dQ2BspPlane_s {
	float			normal[3];
	float			dist;
	int				type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} dQ2BspPlane_t;

typedef struct dQ2BspNode_s {
	int				planeNum;
	int				children[2];	// negative numbers are -(leafs+1), not nodes
	int16			mins[3];		// for frustom culling
	int16			maxs[3];
	uint16			firstFace;
	uint16			numFaces;	// counting both sides
} dQ2BspNode_t;

typedef struct dQ2BspTexInfo_s {
	float			vecs[2][4];		// [s/t][xyz offset]
	int				flags;			// miptex flags + overrides
	int				value;			// light emission, etc
	char			texture[32];	// texture name (textures/*.wal)
	int				nextTexInfo;	// for animations, -1 = end of chain
} dQ2BspTexInfo_t;

// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
typedef struct dQ2BspEdge_s {
	uint16			v[2];		// vertex numbers
} dQ2BspEdge_t;

typedef struct dQ2BspSurface_s {
	uint16			planeNum;
	int16			side;

	int				firstEdge;		// we must support > 64k edges
	int16			numEdges;	
	int16			texInfo;

	// lighting info
	byte			styles[Q2BSP_MAX_LIGHTMAPS];
	int				lightOfs;		// start of [numstyles*surfsize] samples
} dQ2BspSurface_t;

typedef struct dQ2BspLeaf_s {
	int				contents;			// OR of all brushes (not needed?)

	int16			cluster;
	int16			area;

	int16			mins[3];			// for frustum culling
	int16			maxs[3];

	uint16			firstLeafFace;
	uint16			numLeafFaces;

	uint16			firstLeafBrush;
	uint16			numLeafBrushes;
} dQ2BspLeaf_t;

typedef struct dQ2BspBrushSide_s {
	uint16			planeNum;		// facing out of the leaf
	int16			texInfo;
} dQ2BspBrushSide_t;

typedef struct dQ2BspBrush_s {
	int				firstSide;
	int				numSides;
	int				contents;
} dQ2BspBrush_t;

// the visibility lump consists of a header with a count, then byte offsets for
// the PVS and PHS of each cluster, then the raw compressed bit vectors
#define Q2BSP_VIS_PVS	0
#define Q2BSP_VIS_PHS	1
typedef struct dQ2BspVis_s {
	int				numClusters;
	int				bitOfs[8][2];	// bitofs[numclusters][2]
} dQ2BspVis_t;

// each area has a list of portals that lead into other areas when portals are closed,
// other areas may not be visible or hearable even if the vis info says that it should be
typedef struct dQ2BspAreaPortal_s {
	int				portalNum;
	int				otherArea;
} dQ2BspAreaPortal_t;

typedef struct dQ2BspArea_s {
	int				numAreaPortals;
	int				firstAreaPortal;
} dQ2BspArea_t;

/*
=============================================================================

	QUAKE3 BSP FILE FORMAT

=============================================================================
*/

// Header information
#define Q3BSP_HEADER	"IBSP"
#define Q3BSP_VERSION	46

// Lightmap information
#define Q3LIGHTMAP_WIDTH	128				// lightmaps are square
#define Q3LIGHTMAP_BYTES	3
#define Q3LIGHTMAP_SIZE		(Q3LIGHTMAP_WIDTH*Q3LIGHTMAP_WIDTH*3)

// There shouldn't be any problem with increasing these values at the
// expense of more memory allocation in the utilities
#define Q3BSP_MAX_MODELS		0x400	// Same as Q2BSP_MAX_MODELS
#define Q3BSP_MAX_BRUSHES		0x8000
#define Q3BSP_MAX_ENTITIES		0x800
#define Q3BSP_MAX_ENTSTRING		0x40000
#define Q3BSP_MAX_SHADERS		0x400

#define Q3BSP_MAX_AREAS			0x100
#define Q3BSP_MAX_FOGS			0x100
#define Q3BSP_MAX_PLANES		0x20000
#define Q3BSP_MAX_NODES			0x20000
#define Q3BSP_MAX_BRUSHSIDES	0x30000
#define Q3BSP_MAX_LEAFS			0x20000
#define Q3BSP_MAX_VERTEXES		0x80000
#define Q3BSP_MAX_FACES			0x20000
#define Q3BSP_MAX_LEAFFACES		0x40000	// Increased from 0x20000...
#define Q3BSP_MAX_LEAFBRUSHES	0x40000
#define Q3BSP_MAX_PORTALS		0x20000
#define Q3BSP_MAX_INDICES		0x80000
#define Q3BSP_MAX_LIGHTING		0x800000
#define Q3BSP_MAX_VISIBILITY	0x200000

#define Q3BSP_MAX_VIS			(Q3BSP_MAX_LEAFS/8)
#define Q3BSP_MAX_LIGHTMAPS		4

// ==========================================================================

#define Q3BSP_LUMP_ENTITIES		0
#define Q3BSP_LUMP_SHADERREFS	1
#define Q3BSP_LUMP_PLANES		2
#define Q3BSP_LUMP_NODES		3
#define Q3BSP_LUMP_LEAFS		4
#define Q3BSP_LUMP_LEAFFACES	5
#define Q3BSP_LUMP_LEAFBRUSHES	6
#define Q3BSP_LUMP_MODELS		7
#define Q3BSP_LUMP_BRUSHES		8
#define Q3BSP_LUMP_BRUSHSIDES	9
#define Q3BSP_LUMP_VERTEXES		10
#define Q3BSP_LUMP_INDEXES		11
#define Q3BSP_LUMP_FOGS			12
#define Q3BSP_LUMP_FACES		13
#define Q3BSP_LUMP_LIGHTING		14
#define Q3BSP_LUMP_LIGHTGRID	15
#define Q3BSP_LUMP_VISIBILITY	16
#define Q3BSP_LUMP_LIGHTARRAY	17
#define Q3BSP_LUMP_TOTAL		17		// 16 for IDBSP

typedef struct dQ3BspLump_s {
	int				fileOfs;
	int				fileLen;
} dQ3BspLump_t;

typedef struct dQ3BspHeader_t {
	int				ident;
	int				version;
	
	dQ3BspLump_t	lumps[Q3BSP_LUMP_TOTAL];
} dQ3BspHeader_t;

typedef struct dQ3BspModel_s {
	float			mins[3], maxs[3];
	int				firstFace, numFaces;	// submodels just draw faces
											// without walking the bsp tree
    int				firstBrush, numBrushes;
} dQ3BspModel_t;

typedef struct dQ3BspVertex_s {
	float			point[3];
    float			texCoords[2];		// texture coords
	float			lmCoords[2];		// lightmap texture coords
    float			normal[3];			// normal
    byte			color[4];			// color used for vertex lighting
} dQ3BspVertex_t;

// planes (x&~1) and (x&~1)+1 are always opposites
typedef struct dQ3BspPlane_s {
	float	normal[3];
	float	dist;
} dQ3BspPlane_t;

typedef struct dQ3BspNode_s {
	int				planeNum;
	int				children[2];	// negative numbers are -(leafs+1), not nodes
	int				mins[3];		// for frustum culling
	int				maxs[3];
} dQ3BspNode_t;

typedef struct dQ3BspShaderRef_s {
	char			name[MAX_QPATH];
	int				flags;
	int				contents;
} dQ3BspShaderRef_t;

enum {
	FACETYPE_PLANAR   = 1,
	FACETYPE_PATCH    = 2,
	FACETYPE_TRISURF  = 3,
	FACETYPE_FLARE    = 4
};

typedef struct dQ3BspFace_s {
	int				shaderNum;
	int				fogNum;
	int				faceType;

    int				firstVert;
	int				numVerts;
	uint32			firstIndex;
	int				numIndexes;

    int				lmTexNum;		// lightmap info
    int				lmOffset[2];
    int				lmSize[2];

    float			origin[3];		// FACETYPE_FLARE only

    float			mins[3];
    float			maxs[3];		// FACETYPE_PATCH and FACETYPE_TRISURF only
    float			normal[3];		// FACETYPE_PLANAR only

    int				patch_cp[2];	// patch control point dimensions
} dQ3BspFace_t;

typedef struct dQ3BspLeaf_s {
	int				cluster;
	int				area;

	int				mins[3];
	int				maxs[3];

	int				firstLeafFace;
	int				numLeafFaces;

	int				firstLeafBrush;
	int				numLeafBrushes;
} dQ3BspLeaf_t;

typedef struct dQ3BspBrushSide_s {
	int				planeNum;
	int				shaderNum;
} dQ3BspBrushSide_t;

typedef struct dQ3BspBrush_s {
	int				firstSide;
	int				numSides;
	int				shaderNum;
} dQ3BspBrush_t;

typedef struct dQ3BspFog_s {
	char			shader[MAX_QPATH];
	int				brushNum;
	int				visibleSide;
} dQ3BspFog_t;

typedef struct dQ3BspVis_s {
	int				numClusters;
	int				rowSize;
	byte			data[1];
} dQ3BspVis_t;

typedef struct dQ3BspGridLight_s {
	byte			ambient[3];
	byte			diffuse[3];
	byte			direction[2];
} dQ3BspGridLight_t;

/*
=============================================================================

	FUNCTIONS

=============================================================================
*/

#define FS_FreeFile(buffer) _FS_FreeFile ((buffer),__FILE__,__LINE__)
#define FS_FreeFileList(list,num) _FS_FreeFileList ((list),(num),__FILE__,__LINE__)

int			FS_ZLibDecompress (byte *in, int inlen, byte *out, int outlen, int wbits);
int			FS_ZLibCompressChunk (byte *in, int len_in, byte *out, int len_out, int method, int wbits);

void		FS_CreatePath (char *path);
void		FS_CopyFile (char *src, char *dst);

int			FS_FileLength (fileHandle_t fileNum);
int			FS_Tell (fileHandle_t fileNum);

int			FS_Read (void *buffer, int len, fileHandle_t fileNum);
int			FS_Write (void *buffer, int size, fileHandle_t fileNum);
void		FS_Seek (fileHandle_t fileNum, int offset, fsSeekOrigin_t seekOrigin);
int			FS_OpenFile (char *fileName, fileHandle_t *fileNum, fsOpenMode_t openMode);
void		FS_CloseFile (fileHandle_t fileNum);

int			FS_LoadFile (char *path, void **buffer, char *terminate);
void		_FS_FreeFile (void *buffer, const char *fileName, const int fileLine);

int			FS_FileExists (char *path);

char		*FS_Gamedir (void);
void		FS_SetGamedir (char *dir, qBool firstTime);

void		FS_ExecAutoexec (void);

int			FS_FindFiles (char *path, char *filter, char *extension, char **fileList, int maxFiles, qBool addGameDir, qBool recurse);
void		_FS_FreeFileList (char **list, int num, const char *fileName, const int fileLine);

char		*FS_NextPath (char *prevPath);

void		FS_Init (void);

#endif // __FILES_H__
