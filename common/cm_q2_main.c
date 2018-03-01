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
// cm_q2_main.c
// Quake2 BSP map model loading
// FIXME TODO:
// - A lot of these allocations can be better sized (entity string for example).
//

#include "cm_q2_local.h"

static byte				*cm_q2_mapBuffer;

static int				cm_q2_numEntityChars;
static char				cm_q2_emptyEntityString[1];
static char				*cm_q2_entityString;

static int				cm_q2_numTexInfo;
static cBspSurface_t	*cm_q2_surfaces;
static int				cm_q2_numTexInfoUnique;
static cBspSurface_t	**cm_q2_surfacesUnique;

int						cm_q2_numNodes;
cQ2BspNode_t			*cm_q2_nodes;

int						cm_q2_numBrushSides;
cQ2BspBrushSide_t		*cm_q2_brushSides;

int						cm_q2_numLeafs = 1;									// allow leaf funcs to be called without a map
int						cm_q2_emptyLeaf;
cQ2BspLeaf_t			*cm_q2_leafs;

int						cm_q2_numLeafBrushes;
uint16					*cm_q2_leafBrushes;

int						cm_q2_numBrushes;
cQ2BspBrush_t			*cm_q2_brushes;

int						cm_q2_numAreas = 1;
cQ2BspArea_t			*cm_q2_areas;

cBspSurface_t			cm_q2_nullSurface;

int						cm_q2_numPlanes;
cBspPlane_t				*cm_q2_planes;

int						cm_q2_numVisibility;
dQ2BspVis_t				*cm_q2_visData;

int						cm_q2_numAreaPortals;
dQ2BspAreaPortal_t		*cm_q2_areaPortals;
qBool					*cm_q2_portalOpen;

int						cm_q2_numClusters = 1;

/*
=============================================================================

	QUAKE2 BSP LOADING

=============================================================================
*/

/*
=================
CM_Q2BSP_LoadSurfaces
=================
*/
static void CM_Q2BSP_LoadSurfaces (dQ2BspLump_t *l)
{
	dQ2BspTexInfo_t	*in;
	cBspSurface_t	*out;
	int				i, j;
	int				unique;

	// Sanity check lump size
	in = (void *)(cm_q2_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadSurfaces: funny lump size");

	// Total
	cm_q2_numTexInfo = l->fileLen / sizeof (*in);
	if (cm_q2_numTexInfo < 1)
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadSurfaces: Map with no surfaces");
	if (cm_q2_numTexInfo > Q2BSP_MAX_TEXINFO)
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadSurfaces: Map has too many surfaces");
	cm_q2_surfaces = Mem_PoolAlloc (sizeof(cBspSurface_t) * cm_q2_numTexInfo, com_cmodelSysPool, 0);

	// Byte swap
	out = cm_q2_surfaces;
	for (i=0 ; i<cm_q2_numTexInfo ; i++, in++, out++) {
		Q_strncpyz (out->name, in->texture, sizeof (out->name));
		Q_strncpyz (out->rname, in->texture, sizeof (out->rname));
		out->flags = LittleLong (in->flags);
		out->value = LittleLong (in->value);
	}

	// Find the total unique
	unique = 0;
	for (i=0 ; i<cm_q2_numTexInfo ; i++) {
		for (j=i-1 ; j>=0 ; j--) {
			if (!Q_stricmp (cm_q2_surfaces[i].rname, cm_q2_surfaces[j].rname))
				break;
		}
		if (j == -1)
			unique++;
	}

	// Shouldn't really happen...
	if (!unique)
		return;

	cm_q2_numTexInfoUnique = unique;
	cm_q2_surfacesUnique = Mem_PoolAlloc (sizeof(cBspSurface_t *) * cm_q2_numTexInfoUnique, com_cmodelSysPool, 0);
	for (i=0, unique=0 ; i<cm_q2_numTexInfo ; i++) {
		for (j=i-1 ; j>=0 ; j--) {
			if (!Q_stricmp (cm_q2_surfaces[i].rname, cm_q2_surfaces[j].rname))
				break;
		}
		if (j == -1)
			cm_q2_surfacesUnique[unique++] = &cm_q2_surfaces[i];
	}
}


/*
=================
CM_Q2BSP_LoadLeafs
=================
*/
static void CM_Q2BSP_LoadLeafs (dQ2BspLump_t *l)
{
	int				i;
	cQ2BspLeaf_t	*out;
	dQ2BspLeaf_t	*in;

	// Sanity check lump size
	in = (void *)(cm_q2_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadLeafs: funny lump size");

	// Find the size and allocate (with extra for box hull)
	cm_q2_numLeafs = l->fileLen / sizeof (*in);
	if (cm_q2_numLeafs < 1)
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadLeafs: Map with no leafs");
	if (cm_q2_numLeafs > Q2BSP_MAX_PLANES)
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadLeafs: Map has too many planes");
	cm_q2_leafs = Mem_PoolAlloc (sizeof(cQ2BspLeaf_t) * (cm_q2_numLeafs+6), com_cmodelSysPool, 0);

	// Byte swap
	out = cm_q2_leafs;
	cm_q2_numClusters = 0;

	for (i=0 ; i<cm_q2_numLeafs ; i++, in++, out++) {
		out->contents = LittleLong (in->contents);
		out->cluster = LittleShort (in->cluster);
		out->area = LittleShort (in->area);
		out->firstLeafBrush = LittleShort (in->firstLeafBrush);
		out->numLeafBrushes = LittleShort (in->numLeafBrushes);

		if (out->cluster >= cm_q2_numClusters)
			cm_q2_numClusters = out->cluster + 1;
	}

	if (cm_q2_leafs[0].contents != CONTENTS_SOLID)
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadLeafs: Map leaf 0 is not CONTENTS_SOLID");

	cm_q2_emptyLeaf = -1;
	for (i=1 ; i<cm_q2_numLeafs ; i++) {
		if (!cm_q2_leafs[i].contents) {
			cm_q2_emptyLeaf = i;
			break;
		}
	}
	if (cm_q2_emptyLeaf == -1)
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadLeafs: Map does not have an empty leaf");
}


/*
=================
CM_Q2BSP_LoadLeafBrushes
=================
*/
static void CM_Q2BSP_LoadLeafBrushes (dQ2BspLump_t *l)
{
	int		i;
	uint16	*out;
	uint16	*in;

	// Sanity check lump size
	in = (void *)(cm_q2_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadLeafBrushes: funny lump size");

	// Find the size and allocate (with extra for box hull)
	cm_q2_numLeafBrushes = l->fileLen / sizeof (*in);
	if (cm_q2_numLeafBrushes < 1)
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadLeafBrushes: Map with no planes");
	if (cm_q2_numLeafBrushes > Q2BSP_MAX_LEAFBRUSHES)
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadLeafBrushes: Map has too many leafbrushes");
	cm_q2_leafBrushes = Mem_PoolAlloc (sizeof(uint16) * (cm_q2_numLeafBrushes+1), com_cmodelSysPool, 0);

	// Byte swap
	out = cm_q2_leafBrushes;
	for (i=0 ; i<cm_q2_numLeafBrushes ; i++, in++, out++)
		*out = LittleShort (*in);
}


/*
=================
CM_Q2BSP_LoadPlanes
=================
*/
static void CM_Q2BSP_LoadPlanes (dQ2BspLump_t *l)
{
	int				i, j;
	cBspPlane_t		*out;
	dQ2BspPlane_t	*in;
	int				bits;

	// Sanity check lump size
	in = (void *)(cm_q2_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadPlanes: funny lump size");

	// Find the size and allocate (with extra for box hull)
	cm_q2_numPlanes = l->fileLen / sizeof (*in);
	if (cm_q2_numPlanes < 1)
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadPlanes: Map with no planes");
	if (cm_q2_numPlanes > Q2BSP_MAX_PLANES)
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadPlanes: Map has too many planes");
	cm_q2_planes = Mem_PoolAlloc (sizeof(cBspPlane_t) * (cm_q2_numPlanes+12), com_cmodelSysPool, 0);

	// Byte swap
	out = cm_q2_planes;
	for (i=0 ; i<cm_q2_numPlanes ; i++, in++, out++) {
		bits = 0;
		for (j=0 ; j<3 ; j++) {
			out->normal[j] = LittleFloat (in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1<<j;
		}

		out->dist = LittleFloat (in->dist);
		out->type = LittleLong (in->type);
		out->signBits = bits;
	}
}


/*
=================
CM_Q2BSP_LoadBrushes
=================
*/
static void CM_Q2BSP_LoadBrushes (dQ2BspLump_t *l)
{
	dQ2BspBrush_t	*in;
	cQ2BspBrush_t	*out;
	int				i;

	// Sanity check lump size
	in = (void *)(cm_q2_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadBrushes: funny lump size");

	// Find the size and allocate (with extra for box hull)
	cm_q2_numBrushes = l->fileLen / sizeof (*in);
	if (cm_q2_numBrushes > Q2BSP_MAX_BRUSHES)
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadBrushes: Map has too many brushes");
	cm_q2_brushes = Mem_PoolAlloc (sizeof(cQ2BspBrush_t) * (cm_q2_numBrushes+1), com_cmodelSysPool, 0);

	// Byte swap
	out = cm_q2_brushes;
	for (i=0 ; i<cm_q2_numBrushes ; i++, out++, in++) {
		out->firstBrushSide = LittleLong (in->firstSide);
		out->numSides = LittleLong (in->numSides);
		out->contents = LittleLong (in->contents);
	}
}


/*
=================
CM_Q2BSP_LoadBrushSides
=================
*/
static void CM_Q2BSP_LoadBrushSides (dQ2BspLump_t *l)
{
	int					i, j;
	cQ2BspBrushSide_t	*out;
	dQ2BspBrushSide_t	*in;
	int					num;

	// Sanity check lump size
	in = (void *)(cm_q2_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadBrushSides: funny lump size");

	// Find the size and allocate (with extra for box hull)
	cm_q2_numBrushSides = l->fileLen / sizeof (*in);
	if (cm_q2_numBrushSides > Q2BSP_MAX_BRUSHSIDES)
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadBrushSides: Map has too many planes");
	cm_q2_brushSides = Mem_PoolAlloc (sizeof(cQ2BspBrushSide_t) * (cm_q2_numBrushSides+6), com_cmodelSysPool, 0);

	// Byte swap
	out = cm_q2_brushSides;
	for (i=0 ; i<cm_q2_numBrushSides ; i++, in++, out++) {
		num = LittleShort (in->planeNum);
		out->plane = &cm_q2_planes[num];
		j = LittleShort (in->texInfo);
		if (j >= cm_q2_numTexInfo)
			Com_Error (ERR_DROP, "CM_Q2BSP_LoadBrushSides: Bad brushside texInfo");
		out->surface = &cm_q2_surfaces[j];
	}
}


/*
=================
CM_Q2BSP_LoadSubmodels
=================
*/
static void CM_Q2BSP_LoadSubmodels (dQ2BspLump_t *l)
{
	dQ2BspModel_t	*in;
	cBspModel_t		*out;
	int				i;

	// Sanity check lump size
	in = (void *)(cm_q2_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadSubmodels: funny lump size");

	// Find the size
	cm_numCModels = l->fileLen / sizeof (*in);
	if (cm_numCModels < 1)
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadSubmodels: Map with no models");
	if (cm_numCModels > Q2BSP_MAX_MODELS)
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadSubmodels: Map has too many models");

	// Byte swap
	for (i=0 ; i<cm_numCModels ; i++, in++, out++) {
		out = &cm_mapCModels[i];

		// Spread the mins / maxs by a pixel
		out->mins[0] = LittleFloat (in->mins[0]) - 1;
		out->mins[1] = LittleFloat (in->mins[1]) - 1;
		out->mins[2] = LittleFloat (in->mins[2]) - 1;
		out->maxs[0] = LittleFloat (in->maxs[0]) + 1;
		out->maxs[1] = LittleFloat (in->maxs[1]) + 1;
		out->maxs[2] = LittleFloat (in->maxs[2]) + 1;

		// Head node
		out->headNode = LittleLong (in->headNode);
	}
}


/*
=================
CM_Q2BSP_LoadNodes
=================
*/
static void CM_Q2BSP_LoadNodes (dQ2BspLump_t *l)
{
	dQ2BspNode_t	*in;
	cQ2BspNode_t	*out;
	int				i;

	// Sanity check lump size
	in = (void *)(cm_q2_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadNodes: funny lump size");

	// Find the size and allocate (with extra for box hull)
	cm_q2_numNodes = l->fileLen / sizeof (*in);
	if (cm_q2_numNodes < 1)
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadNodes: Map has no nodes");
	if (cm_q2_numNodes > Q2BSP_MAX_NODES)
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadNodes: Map has too many nodes");
	cm_q2_nodes = Mem_PoolAlloc (sizeof(cQ2BspNode_t) * (cm_q2_numNodes+6), com_cmodelSysPool, 0);

	// Byte swap
	out = cm_q2_nodes;
	for (i=0 ; i<cm_q2_numNodes ; i++, out++, in++) {
		out->plane = cm_q2_planes + LittleLong (in->planeNum);
		out->children[0] = LittleLong (in->children[0]);
		out->children[1] = LittleLong (in->children[1]);
	}
}


/*
=================
CM_Q2BSP_LoadAreas
=================
*/
static void CM_Q2BSP_LoadAreas (dQ2BspLump_t *l)
{
	int				i;
	cQ2BspArea_t	*out;
	dQ2BspArea_t	*in;

	// Sanity check lump size
	in = (void *)(cm_q2_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadAreas: funny lump size");

	// Find the size and allocate
	cm_q2_numAreas = l->fileLen / sizeof (*in);
	if (cm_q2_numAreas > Q2BSP_MAX_AREAS)
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadAreas: Map has too many areas");
	cm_q2_areas = Mem_PoolAlloc (sizeof(cQ2BspArea_t) * cm_q2_numAreas, com_cmodelSysPool, 0);

	// Byte swap
	out = cm_q2_areas;
	for (i=0 ; i<cm_q2_numAreas ; i++, in++, out++) {
		out->numAreaPortals = LittleLong (in->numAreaPortals);
		out->firstAreaPortal = LittleLong (in->firstAreaPortal);
		out->floodValid = 0;
		out->floodNum = 0;
	}
}


/*
=================
CM_Q2BSP_LoadAreaPortals
=================
*/
static void CM_Q2BSP_LoadAreaPortals (dQ2BspLump_t *l)
{
	int					i;
	dQ2BspAreaPortal_t	*out;
	dQ2BspAreaPortal_t	*in;

	// Sanity check lump size
	in = (void *)(cm_q2_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadAreaPortals: funny lump size");

	// Find the size and allocate
	cm_q2_numAreaPortals = l->fileLen / sizeof (*in);
	if (cm_q2_numAreaPortals > Q2BSP_MAX_AREAS)
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadAreaPortals: Map has too many areas");
	cm_q2_areaPortals = Mem_PoolAlloc (sizeof(dQ2BspAreaPortal_t) * cm_q2_numAreaPortals, com_cmodelSysPool, 0);

	// Byte swap
	out = cm_q2_areaPortals;
	for (i=0 ; i<cm_q2_numAreaPortals ; i++, in++, out++) {
		out->portalNum = LittleLong (in->portalNum);
		out->otherArea = LittleLong (in->otherArea);
	}
}


/*
=================
CM_Q2BSP_LoadVisibility
=================
*/
static void CM_Q2BSP_LoadVisibility (dQ2BspLump_t *l)
{
	int		i;

	// Find the size and allocate
	cm_q2_numVisibility = l->fileLen;
	if (l->fileLen > Q2BSP_MAX_VISIBILITY)
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadVisibility: Map has too large visibility lump");

	// If there's no visibility, just store the number of clusters
	if (!cm_q2_numVisibility) {
		cm_q2_visData = Mem_PoolAlloc (sizeof(int), com_cmodelSysPool, 0);
		cm_q2_visData->numClusters = 0;
		return;
	}

	// Byte swap
	cm_q2_visData = Mem_PoolAlloc (sizeof(int) + (sizeof(byte) * cm_q2_numVisibility), com_cmodelSysPool, 0);
	memcpy (cm_q2_visData, cm_q2_mapBuffer + l->fileOfs, l->fileLen);
	cm_q2_visData->numClusters = LittleLong (cm_q2_visData->numClusters);
	for (i=0 ; i<cm_q2_visData->numClusters ; i++) {
		cm_q2_visData->bitOfs[i][0] = LittleLong (cm_q2_visData->bitOfs[i][0]);
		cm_q2_visData->bitOfs[i][1] = LittleLong (cm_q2_visData->bitOfs[i][1]);
	}
}


/*
=================
CM_Q2BSP_LoadEntityString
=================
*/
static void CM_Q2BSP_LoadEntityString (dQ2BspLump_t *l)
{
	// Find the size and allocate (with extra for NULL termination)
	cm_q2_numEntityChars = l->fileLen;
	if (l->fileLen > Q2BSP_MAX_ENTSTRING)
		Com_Error (ERR_DROP, "CM_Q2BSP_LoadEntityString: Map has too large entity lump");
	cm_q2_entityString = Mem_PoolAlloc (sizeof(char) * (cm_q2_numEntityChars+1), com_cmodelSysPool, 0);

	// Copy data
	memcpy (cm_q2_entityString, cm_q2_mapBuffer + l->fileOfs, l->fileLen);
}

// ==========================================================================

/*
==================
CM_Q2BSP_LoadMap

Loads in the map and all submodels
==================
*/
cBspModel_t *CM_Q2BSP_LoadMap (uint32 *buffer)
{
	dQ2BspHeader_t	header;
	int				i;

	//
	// Allocate space
	//
	cm_q2_portalOpen = Mem_PoolAlloc (sizeof(qBool) * Q2BSP_MAX_AREAPORTALS, com_cmodelSysPool, 0);

	//
	// Byte swap
	//
	header = *(dQ2BspHeader_t *)buffer;
	for (i=0 ; i<sizeof (dQ2BspHeader_t)/4 ; i++)
		((int *)&header)[i] = LittleLong (((int *)&header)[i]);
	cm_q2_mapBuffer = (byte *)buffer;

	//
	// Load into heap
	//
	CM_Q2BSP_LoadSurfaces		(&header.lumps[Q2BSP_LUMP_TEXINFO]);
	CM_Q2BSP_LoadLeafs			(&header.lumps[Q2BSP_LUMP_LEAFS]);
	CM_Q2BSP_LoadLeafBrushes	(&header.lumps[Q2BSP_LUMP_LEAFBRUSHES]);
	CM_Q2BSP_LoadPlanes			(&header.lumps[Q2BSP_LUMP_PLANES]);
	CM_Q2BSP_LoadBrushes		(&header.lumps[Q2BSP_LUMP_BRUSHES]);
	CM_Q2BSP_LoadBrushSides		(&header.lumps[Q2BSP_LUMP_BRUSHSIDES]);
	CM_Q2BSP_LoadSubmodels		(&header.lumps[Q2BSP_LUMP_MODELS]);
	CM_Q2BSP_LoadNodes			(&header.lumps[Q2BSP_LUMP_NODES]);
	CM_Q2BSP_LoadAreas			(&header.lumps[Q2BSP_LUMP_AREAS]);
	CM_Q2BSP_LoadAreaPortals	(&header.lumps[Q2BSP_LUMP_AREAPORTALS]);
	CM_Q2BSP_LoadVisibility		(&header.lumps[Q2BSP_LUMP_VISIBILITY]);
	CM_Q2BSP_LoadEntityString	(&header.lumps[Q2BSP_LUMP_ENTITIES]);

	CM_Q2BSP_InitBoxHull ();
	CM_Q2BSP_PrepMap ();

	return &cm_mapCModels[0];
}


/*
==================
CM_Q2BSP_PrepMap
==================
*/
void CM_Q2BSP_PrepMap (void)
{
	if (!cm_q2_portalOpen)
		return;

	memset (cm_q2_portalOpen, 0, sizeof(qBool) * Q2BSP_MAX_AREAPORTALS);
	CM_Q2BSP_FloodAreaConnections ();
}


/*
==================
CM_Q2BSP_UnloadMap
==================
*/
void CM_Q2BSP_UnloadMap (void)
{
	cm_q2_areaPortals = NULL;
	cm_q2_areas = NULL;
	cm_q2_brushes = NULL;
	cm_q2_brushSides = NULL;
	cm_q2_entityString = cm_q2_emptyEntityString;
	cm_q2_leafBrushes = NULL;
	cm_q2_leafs = NULL;
	cm_q2_nodes = NULL;
	cm_q2_planes = NULL;
	cm_q2_portalOpen = NULL;
	cm_q2_surfaces = NULL;
	cm_q2_surfacesUnique = NULL;
	cm_q2_visData = NULL;

	cm_q2_numAreaPortals = 0;
	cm_q2_numAreas = 1;
	cm_q2_numBrushes = 0;
	cm_q2_numBrushSides = 0;
	cm_q2_numClusters = 1;
	cm_q2_numEntityChars = 0;
	cm_q2_numLeafBrushes = 0;
	cm_q2_numLeafs = 1;
	cm_q2_numNodes = 0;
	cm_q2_numPlanes = 0;
	cm_q2_numTexInfo = 0;
	cm_q2_numTexInfoUnique = 0;
	cm_q2_numVisibility = 0;
}

/*
=============================================================================

	QUAKE2 BSP INFORMATION

=============================================================================
*/

/*
==================
CM_Q2BSP_EntityString
==================
*/
char *CM_Q2BSP_EntityString (void)
{
	return cm_q2_entityString;
}


/*
==================
CM_Q2BSP_SurfRName
==================
*/
char *CM_Q2BSP_SurfRName (int texNum)
{
	return cm_q2_surfacesUnique ? cm_q2_surfacesUnique[texNum]->rname : NULL;
}


/*
==================
CM_Q2BSP_NumClusters
==================
*/
int CM_Q2BSP_NumClusters (void)
{
	return cm_q2_numClusters;
}


/*
==================
CM_Q2BSP_NumTexInfo
==================
*/
int CM_Q2BSP_NumTexInfo (void)
{
	return cm_q2_numTexInfoUnique;
}
