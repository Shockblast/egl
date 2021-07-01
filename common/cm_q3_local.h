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
// cm_q3_local.h
// Quake3 BSP map model collision header
//

#include "cm_common.h"

// ==========================================================================

#define MAX_CM_AREAPORTALS		(MAX_CS_EDICTS)
#define MAX_CM_AREAS			(Q3BSP_MAX_AREAS)
#define MAX_CM_BRUSHSIDES		(Q3BSP_MAX_BRUSHSIDES << 1)
#define MAX_CM_SHADERS			(Q3BSP_MAX_SHADERS)
#define MAX_CM_PLANES			(Q3BSP_MAX_PLANES << 2)
#define MAX_CM_NODES			(Q3BSP_MAX_NODES)
#define MAX_CM_LEAFS			(Q3BSP_MAX_LEAFS)
#define MAX_CM_LEAFBRUSHES		(Q3BSP_MAX_LEAFBRUSHES)
#define MAX_CM_MODELS			(Q3BSP_MAX_MODELS)
#define MAX_CM_BRUSHES			(Q3BSP_MAX_BRUSHES << 1)
#define MAX_CM_VISIBILITY		(Q3BSP_MAX_VISIBILITY)
#define MAX_CM_FACES			(Q3BSP_MAX_FACES)
#define MAX_CM_LEAFFACES		(Q3BSP_MAX_LEAFFACES)
#define MAX_CM_VERTEXES			(Q3BSP_MAX_VERTEXES)
#define MAX_CM_PATCHES			(0x10000)
#define MAX_CM_PATCH_VERTS		(4096)
#define MAX_CM_ENTSTRING		(Q3BSP_MAX_ENTSTRING)

#define CM_SUBDIVLEVEL			(15)

typedef struct cface_s {
	int					faceType;

	int					numVerts;
	int					firstVert;

	int					shaderNum;
	int					patch_cp[2];
} cface_t;

typedef struct cnode_t {
	cBspPlane_t			*plane;
	int					children[2];	// negative numbers are leafs
} cnode_t;

typedef struct cbrushside_s {
	cBspPlane_t			*plane;
	cBspSurface_t		*surface;
} cbrushside_t;

typedef struct cleaf_s {
	int					contents;
	int					cluster;
	int					area;

	int					firstLeafFace;
	int					numLeafFaces;

	int					firstLeafBrush;
	int					numLeafBrushes;

	int					firstLeafPatch;
	int					numLeafPatches;
} cleaf_t;

typedef struct cbrush_s {
	int					contents;
	int					numSides;
	int					firstBrushSide;
	int					checkCount;		// to avoid repeated testings
} cbrush_t;

typedef struct cpatch_s {
	vec3_t				absMins, absMaxs;

	int					numBrushes;
	cbrush_t			*brushes;

	cBspSurface_t		*surface;
	int					checkCount;		// to avoid repeated testings
} cpatch_t;

typedef struct careaportal_s {
	qBool				open;
	int					area;
	int					otherArea;
} careaportal_t;

typedef struct carea_s {
	int					numAreaPortals;
	int					areaPortals[MAX_CM_AREAS];
	int					floodNum;		// if two areas have equal floodnums, they are connected
	int					floodValid;
} carea_t;

// ==========================================================================

extern int					checkcount;

extern int					numbrushsides;
extern cbrushside_t			map_brushsides[MAX_CM_BRUSHSIDES+6];	// extra for box hull

extern int					numshaderrefs;
extern cBspSurface_t		map_surfaces[MAX_CM_SHADERS];

extern int					numplanes;
extern cBspPlane_t			map_planes[MAX_CM_PLANES+12];			// extra for box hull

extern int					numnodes;
extern cnode_t				map_nodes[MAX_CM_NODES+6];				// extra for box hull

extern int					numleafs;								// allow leaf funcs to be called without a map
extern cleaf_t				map_leafs[MAX_CM_LEAFS];

extern int					numleafbrushes;
extern int					map_leafbrushes[MAX_CM_LEAFBRUSHES+1];	// extra for box hull

extern int					numbrushes;
extern cbrush_t				map_brushes[MAX_CM_BRUSHES+1];			// extra for box hull

extern byte					map_hearability[MAX_CM_VISIBILITY];

extern int					numvisibility;
extern byte					map_visibility[MAX_CM_VISIBILITY];
extern dQ3BspVis_t			*map_pvs;

extern dQ3BspVis_t			*map_phs;

extern byte					nullrow[MAX_CM_LEAFS/8];

extern int					numentitychars;
extern char					map_entitystring[MAX_CM_ENTSTRING];

extern int					numareaportals;
extern careaportal_t		map_areaportals[MAX_CM_AREAPORTALS];

extern int					numareas;
extern carea_t				map_areas[MAX_CM_AREAS];

extern cBspSurface_t		nullsurface;

extern int					emptyleaf;

extern cpatch_t				map_patches[MAX_CM_PATCHES];
extern int					numpatches;

extern int					map_leafpatches[MAX_CM_LEAFFACES];
extern int					numleafpatches;

extern vec4_t				*map_verts;
extern int					numvertexes;

extern cface_t				*map_faces;
extern int					numfaces;

extern int					*map_leaffaces;
extern int					numleaffaces;

// ==========================================================================

void		CM_Q3BSP_InitBoxHull (void);
void		CM_Q3BSP_FloodAreaConnections (void);
