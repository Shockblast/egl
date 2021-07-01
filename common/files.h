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
========================================================================

	The .pak files are just a linear collapse of a directory tree

========================================================================
*/

#define PAK_HEADER		(('K'<<24)+('C'<<16)+('A'<<8)+'P')	// little-endian "PACK"
#define PAK_HEADERSTR	"PACK"

#define	PAK_MAXFILES	4096

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
========================================================================

	MD2 triangle model file format

========================================================================
*/

#define MD2_HEADER		(('2'<<24)+('P'<<16)+('D'<<8)+'I')	// little-endian "IDP2"
#define MD2_HEADERSTR	"IDP2"

#define MD2_ALIAS_VERSION	8

#define	MD2_MAX_TRIANGLES	4096
#define MD2_MAX_VERTS		2048
#define MD2_MAX_FRAMES		512
#define MD2_MAX_SKINS		32
#define	MD2_MAX_SKINNAME	64

typedef struct dMd2Coord_s {
	short			s;
	short			t;
} dMd2Coord_t;

typedef struct dMd2Triangle_s {
	short			vertsIndex[3];
	short			stIndex[3];
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
========================================================================

	MD3 model file format

========================================================================
*/

#define MD3_HEADER		(('3'<<24)+('P'<<16)+('D'<<8)+'I')	// little-endian "IDP3"
#define MD3_HEADERSTR	"IDP3"

#define MD3_ALIAS_VERSION	15

#define	MD3_MAX_TRIANGLES	8192		// per mesh
#define MD3_MAX_VERTS		4096		// per mesh
#define MD3_MAX_SHADERS		256			// per mesh
#define MD3_MAX_FRAMES		1024		// per model
#define	MD3_MAX_MESHES		32			// per model
#define MD3_MAX_TAGS		16			// per frame

#define	MD3_XYZ_SCALE		(1.0/64)	// vertex scales

typedef struct dMd3Coord_s {
	float			st[2];
} dMd3Coord_t;

typedef struct dMd3Vertex_s {
	short			point[3];
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
========================================================================

	SP2 sprite file format

========================================================================
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
==============================================================================

	BSP file format

==============================================================================
*/

#define BSP_HEADER		(('P'<<24)+('S'<<16)+('B'<<8)+'I')	// little-endian "IBSP"
#define BSP_HEADERSTR	"IBSP"

#define BSP_VERSION		38

// upper design bounds
// leaffaces, leafbrushes, planes, and verts are still bounded by
// 16 bit short limits
#define	BSP_MAX_MODELS		1024
#define	BSP_MAX_BRUSHES		8192
#define	BSP_MAX_ENTITIES	2048
#define	BSP_MAX_ENTSTRING	0x40000
#define	BSP_MAX_TEXINFO		8192

#define	BSP_MAX_AREAPORTALS	1024
#define	BSP_MAX_PLANES		65536
#define	BSP_MAX_NODES		65536
#define	BSP_MAX_BRUSHSIDES	65536
#define	BSP_MAX_LEAFS		65536
#define	BSP_MAX_VERTS		65536
#define	BSP_MAX_FACES		65536
#define	BSP_MAX_LEAFFACES	65536
#define	BSP_MAX_LEAFBRUSHES 65536
#define	BSP_MAX_PORTALS		65536
#define	BSP_MAX_EDGES		128000
#define	BSP_MAX_SURFEDGES	256000
#define	BSP_MAX_LIGHTING	0x200000
#define	BSP_MAX_VISIBILITY	0x100000

#define BSP_MAX_VIS			8192	// (BSP_MAX_LEAFS / 8)
#define	BSP_MAX_LIGHTMAPS	4

// key / value pair sizes
#define	MAX_KEY		32
#define	MAX_VALUE	1024

// ===========================================================================

#define	LUMP_ENTITIES		0
#define	LUMP_PLANES			1
#define	LUMP_VERTEXES		2
#define	LUMP_VISIBILITY		3
#define	LUMP_NODES			4
#define	LUMP_TEXINFO		5
#define	LUMP_FACES			6
#define	LUMP_LIGHTING		7
#define	LUMP_LEAFS			8
#define	LUMP_LEAFFACES		9
#define	LUMP_LEAFBRUSHES	10
#define	LUMP_EDGES			11
#define	LUMP_SURFEDGES		12
#define	LUMP_MODELS			13
#define	LUMP_BRUSHES		14
#define	LUMP_BRUSHSIDES		15
#define	LUMP_POP			16
#define	LUMP_AREAS			17
#define	LUMP_AREAPORTALS	18
#define	HEADER_LUMPS		19

typedef struct dBspLump_s {
	int		fileOfs;
	int		fileLen;
} dBspLump_t;

typedef struct dBspHeader_s {
	int				ident;
	int				version;
	
	dBspLump_t		lumps[HEADER_LUMPS];
} dBspHeader_t;

typedef struct dBspModel_s {
	float			mins[3], maxs[3];

	float			origin[3];		// for sounds or lights

	int				headNode;

	int				firstFace;	// submodels just draw faces
	int				numFaces;	// without walking the bsp tree
} dBspModel_t;

typedef struct dBspVertex_s {
	float			point[3];
} dBspVertex_t;

// planes (x&~1) and (x&~1)+1 are always opposites
typedef struct dBspPlane_s {
	float			normal[3];
	float			dist;
	int				type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} dBspPlane_t;

typedef struct dBspNode_s {
	int				planeNum;
	int				children[2];	// negative numbers are -(leafs+1), not nodes
	short			mins[3];		// for frustom culling
	short			maxs[3];
	uShort			firstFace;
	uShort			numFaces;	// counting both sides
} dBspNode_t;

typedef struct dBspTexInfo_s {
	float			vecs[2][4];		// [s/t][xyz offset]
	int				flags;			// miptex flags + overrides
	int				value;			// light emission, etc
	char			texture[32];	// texture name (textures/*.wal)
	int				nextTexInfo;	// for animations, -1 = end of chain
} dBspTexInfo_t;

// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
typedef struct dBspEdge_s {
	uShort			v[2];		// vertex numbers
} dBspEdge_t;

typedef struct dBspSurface_s {
	uShort			planeNum;
	short			side;

	int				firstEdge;		// we must support > 64k edges
	short			numEdges;	
	short			texInfo;

	// lighting info
	byte			styles[BSP_MAX_LIGHTMAPS];
	int				lightOfs;		// start of [numstyles*surfsize] samples
} dBspSurface_t;

typedef struct dBspLeaf_s {
	int				contents;			// OR of all brushes (not needed?)

	short			cluster;
	short			area;

	short			mins[3];			// for frustum culling
	short			maxs[3];

	uShort			firstLeafFace;
	uShort			numLeafFaces;

	uShort			firstLeafBrush;
	uShort			numLeafBrushes;
} dBspLeaf_t;

typedef struct dBspBrushSide_s {
	uShort			planeNum;		// facing out of the leaf
	short			texInfo;
} dBspBrushSide_t;

typedef struct dBspBrush_s {
	int				firstSide;
	int				numSides;
	int				contents;
} dBspBrush_t;

// the visibility lump consists of a header with a count, then byte offsets for
// the PVS and PHS of each cluster, then the raw compressed bit vectors
#define	BSP_VIS_PVS	0
#define	BSP_VIS_PHS	1
typedef struct dBspVis_s {
	int				numClusters;
	int				bitOfs[8][2];	// bitofs[numclusters][2]
} dBspVis_t;

// each area has a list of portals that lead into other areas when portals are closed,
// other areas may not be visible or hearable even if the vis info says that it should be
typedef struct dBspAreaPortal_s {
	int				portalNum;
	int				otherArea;
} dBspAreaPortal_t;

typedef struct dBspArea_s {
	int				numAreaPortals;
	int				firstAreaPortal;
} dBspArea_t;

/*
=============================================================================

	FUNCTIONS

=============================================================================
*/

void	FS_CreatePath (char *path);

void	FS_CopyFile (char *src, char *dst);
void	FS_FCloseFile (FILE *f);	// note: this can't be called from another DLL, due to MS libc issues
int		FS_FOpenFile (char *filename, FILE **file);
void	FS_Read (void *buffer, int len, FILE *f);	// properly handles partial reads

// a null buffer will just return the file length without loading a -1 length is not present
int		FS_LoadFile (char *path, void **buffer);
void	FS_FreeFile (void *buffer);
void	FS_FreeFileList (char **list, int n);

qBool	FS_CurrGame (char *name);
char	*FS_Gamedir (void);
void	FS_SetGamedir (char *dir, qBool firstTime);

void	FS_ExecAutoexec (void);

char	**FS_ListFiles (char *findName, int *numFiles, uInt mustHave, uInt cantHave);
char	*FS_NextPath (char *prevpath);
char	*FS_FindFirst (char *find);
char	*FS_FindNext (char *find);

void	FS_Init (void);
void	FS_Shutdown (void);

#endif // __FILES_H__
