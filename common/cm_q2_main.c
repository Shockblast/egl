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
//

#include "cm_q2_local.h"

static byte				*cm_mapBuffer;

static int				cm_numEntityChars;
static char				cm_mapEntityString[Q2BSP_MAX_ENTSTRING];

int						cm_numNodes;
cQ2BspNode_t			cm_mapNodes[Q2BSP_MAX_NODES+6];		// extra for box hull

int						cm_numBrushSides;
cQ2BspBrushSide_t		cm_mapBrushSides[Q2BSP_MAX_BRUSHSIDES];

int						cm_numLeafs = 1;					// allow leaf funcs to be called without a map
cQ2BspLeaf_t			cm_mapLeafs[Q2BSP_MAX_LEAFS];
int						cm_emptyLeaf;

int						cm_numLeafBrushes;
uint16					cm_mapLeafBrushes[Q2BSP_MAX_LEAFBRUSHES];

int						cm_numBrushes;
cQ2BspBrush_t			cm_mapBrushes[Q2BSP_MAX_BRUSHES];

int						cm_numAreas = 1;
cQ2BspArea_t			cm_mapAreas[Q2BSP_MAX_AREAS];

static int				cm_numTexInfo;
static cQ2MapSurface_t	cm_mapSurfaces[Q2BSP_MAX_TEXINFO];

cQ2MapSurface_t			cm_nullSurface;

int						cm_numPlanes;
cBspPlane_t				cm_mapPlanes[Q2BSP_MAX_PLANES+6];	// extra for box hull

int						cm_numVisibility;
byte					cm_mapVisibility[Q2BSP_MAX_VISIBILITY];
dQ2BspVis_t				*cm_mapVis = (dQ2BspVis_t *)cm_mapVisibility;

int						cm_numAreaPortals;
dQ2BspAreaPortal_t		cm_mapAreaPortals[Q2BSP_MAX_AREAPORTALS];
qBool					cm_portalOpen[Q2BSP_MAX_AREAPORTALS];

int						cm_numClusters = 1;

/*
=============================================================================

	QUAKE2 BSP LOADING

=============================================================================
*/

/*
=================
CM_LoadQ2BSPSurfaces
=================
*/
static void CM_LoadQ2BSPSurfaces (dQ2BspLump_t *l)
{
	dQ2BspTexInfo_t	*in;
	cQ2MapSurface_t	*out;
	int				i;

	in = (void *)(cm_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadQ2BSPSurfaces: funny lump size");

	cm_numTexInfo = l->fileLen / sizeof (*in);
	if (cm_numTexInfo < 1)
		Com_Error (ERR_DROP, "CM_LoadQ2BSPSurfaces: Map with no surfaces");
	if (cm_numTexInfo > Q2BSP_MAX_TEXINFO)
		Com_Error (ERR_DROP, "CM_LoadQ2BSPSurfaces: Map has too many surfaces");

	out = cm_mapSurfaces;
	for (i=0 ; i<cm_numTexInfo ; i++, in++, out++) {
		Q_strncpyz (out->c.name, in->texture, sizeof (out->c.name));
		Q_strncpyz (out->rname, in->texture, sizeof (out->rname));
		out->c.flags = LittleLong (in->flags);
		out->c.value = LittleLong (in->value);
	}
}


/*
=================
CM_LoadQ2BSPLeafs
=================
*/
static void CM_LoadQ2BSPLeafs (dQ2BspLump_t *l)
{
	int				i;
	cQ2BspLeaf_t	*out;
	dQ2BspLeaf_t	*in;
	
	in = (void *)(cm_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadQ2BSPLeafs: funny lump size");

	cm_numLeafs = l->fileLen / sizeof (*in);
	if (cm_numLeafs < 1)
		Com_Error (ERR_DROP, "CM_LoadQ2BSPLeafs: Map with no leafs");
	// need to save space for box planes
	if (cm_numLeafs > Q2BSP_MAX_PLANES)
		Com_Error (ERR_DROP, "CM_LoadQ2BSPLeafs: Map has too many planes");

	out = cm_mapLeafs;
	cm_numClusters = 0;

	for (i=0 ; i<cm_numLeafs ; i++, in++, out++) {
		out->contents = LittleLong (in->contents);
		out->cluster = LittleShort (in->cluster);
		out->area = LittleShort (in->area);
		out->firstLeafBrush = LittleShort (in->firstLeafBrush);
		out->numLeafBrushes = LittleShort (in->numLeafBrushes);

		if (out->cluster >= cm_numClusters)
			cm_numClusters = out->cluster + 1;
	}

	if (cm_mapLeafs[0].contents != CONTENTS_SOLID)
		Com_Error (ERR_DROP, "CM_LoadQ2BSPLeafs: Map leaf 0 is not CONTENTS_SOLID");
	cm_emptyLeaf = -1;
	for (i=1 ; i<cm_numLeafs ; i++) {
		if (!cm_mapLeafs[i].contents) {
			cm_emptyLeaf = i;
			break;
		}
	}
	if (cm_emptyLeaf == -1)
		Com_Error (ERR_DROP, "CM_LoadQ2BSPLeafs: Map does not have an empty leaf");
}


/*
=================
CM_LoadQ2BSPLeafBrushes
=================
*/
static void CM_LoadQ2BSPLeafBrushes (dQ2BspLump_t *l)
{
	int		i;
	uint16	*out;
	uint16	*in;
	
	in = (void *)(cm_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadQ2BSPLeafBrushes: funny lump size");

	cm_numLeafBrushes = l->fileLen / sizeof (*in);
	if (cm_numLeafBrushes < 1)
		Com_Error (ERR_DROP, "CM_LoadQ2BSPLeafBrushes: Map with no planes");

	// need to save space for box planes
	if (cm_numLeafBrushes > Q2BSP_MAX_LEAFBRUSHES)
		Com_Error (ERR_DROP, "CM_LoadQ2BSPLeafBrushes: Map has too many leafbrushes");

	out = cm_mapLeafBrushes;

	for (i=0 ; i<cm_numLeafBrushes ; i++, in++, out++)
		*out = LittleShort (*in);
}


/*
=================
CM_LoadQ2BSPPlanes
=================
*/
static void CM_LoadQ2BSPPlanes (dQ2BspLump_t *l)
{
	int				i, j;
	cBspPlane_t		*out;
	dQ2BspPlane_t	*in;
	int				bits;
	
	in = (void *)(cm_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadQ2BSPPlanes: funny lump size");

	cm_numPlanes = l->fileLen / sizeof (*in);
	if (cm_numPlanes < 1)
		Com_Error (ERR_DROP, "CM_LoadQ2BSPPlanes: Map with no planes");

	// need to save space for box planes
	if (cm_numPlanes > Q2BSP_MAX_PLANES)
		Com_Error (ERR_DROP, "CM_LoadQ2BSPPlanes: Map has too many planes");

	out = cm_mapPlanes;
	for (i=0 ; i<cm_numPlanes ; i++, in++, out++) {
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
CM_LoadQ2BSPBrushes
=================
*/
static void CM_LoadQ2BSPBrushes (dQ2BspLump_t *l)
{
	dQ2BspBrush_t	*in;
	cQ2BspBrush_t	*out;
	int				i;
	
	in = (void *)(cm_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadQ2BSPBrushes: funny lump size");

	cm_numBrushes = l->fileLen / sizeof (*in);
	if (cm_numBrushes > Q2BSP_MAX_BRUSHES)
		Com_Error (ERR_DROP, "CM_LoadQ2BSPBrushes: Map has too many brushes");

	out = cm_mapBrushes;
	for (i=0 ; i<cm_numBrushes ; i++, out++, in++) {
		out->firstBrushSide = LittleLong (in->firstSide);
		out->numSides = LittleLong (in->numSides);
		out->contents = LittleLong (in->contents);
	}
}


/*
=================
CM_LoadQ2BSPBrushSides
=================
*/
static void CM_LoadQ2BSPBrushSides (dQ2BspLump_t *l)
{
	int					i, j;
	cQ2BspBrushSide_t	*out;
	dQ2BspBrushSide_t	*in;
	int					num;

	in = (void *)(cm_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadQ2BSPBrushSides: funny lump size");

	cm_numBrushSides = l->fileLen / sizeof (*in);
	// need to save space for box planes
	if (cm_numBrushSides > Q2BSP_MAX_BRUSHSIDES)
		Com_Error (ERR_DROP, "CM_LoadQ2BSPBrushSides: Map has too many planes");

	out = cm_mapBrushSides;
	for (i=0 ; i<cm_numBrushSides ; i++, in++, out++) {
		num = LittleShort (in->planeNum);
		out->plane = &cm_mapPlanes[num];
		j = LittleShort (in->texInfo);
		if (j >= cm_numTexInfo)
			Com_Error (ERR_DROP, "CM_LoadQ2BSPBrushSides: Bad brushside texInfo");
		out->surface = &cm_mapSurfaces[j];
	}
}


/*
=================
CM_LoadQ2BSPSubmodels
=================
*/
static void CM_LoadQ2BSPSubmodels (dQ2BspLump_t *l)
{
	dQ2BspModel_t	*in;
	cBspModel_t		*out;
	int				i, j;

	in = (void *)(cm_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadQ2BSPSubmodels: funny lump size");

	cm_numCModels = l->fileLen / sizeof (*in);
	if (cm_numCModels < 1)
		Com_Error (ERR_DROP, "CM_LoadQ2BSPSubmodels: Map with no models");
	if (cm_numCModels > Q2BSP_MAX_MODELS)
		Com_Error (ERR_DROP, "CM_LoadQ2BSPSubmodels: Map has too many models");

	for (i=0 ; i<cm_numCModels ; i++, in++, out++) {
		out = &cm_mapCModels[i];

		for (j=0 ; j<3 ; j++) {
			// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
		}
		out->headNode = LittleLong (in->headNode);
	}
}


/*
=================
CM_LoadQ2BSPNodes
=================
*/
static void CM_LoadQ2BSPNodes (dQ2BspLump_t *l)
{
	dQ2BspNode_t	*in;
	cQ2BspNode_t	*out;
	int				child;
	int				i;

	in = (void *)(cm_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadQ2BSPNodes: funny lump size");

	cm_numNodes = l->fileLen / sizeof (*in);
	if (cm_numNodes < 1)
		Com_Error (ERR_DROP, "CM_LoadQ2BSPNodes: Map has no nodes");
	if (cm_numNodes > Q2BSP_MAX_NODES)
		Com_Error (ERR_DROP, "CM_LoadQ2BSPNodes: Map has too many nodes");

	out = cm_mapNodes;
	for (i=0 ; i<cm_numNodes ; i++, out++, in++) {
		out->plane = cm_mapPlanes + LittleLong (in->planeNum);

		child = LittleLong (in->children[0]);
		out->children[0] = child;

		child = LittleLong (in->children[1]);
		out->children[1] = child;
	}
}


/*
=================
CM_LoadQ2BSPAreas
=================
*/
static void CM_LoadQ2BSPAreas (dQ2BspLump_t *l)
{
	int				i;
	cQ2BspArea_t	*out;
	dQ2BspArea_t	*in;

	in = (void *)(cm_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadQ2BSPAreas: funny lump size");

	cm_numAreas = l->fileLen / sizeof (*in);
	if (cm_numAreas > Q2BSP_MAX_AREAS)
		Com_Error (ERR_DROP, "CM_LoadQ2BSPAreas: Map has too many areas");

	out = cm_mapAreas;
	for (i=0 ; i<cm_numAreas ; i++, in++, out++) {
		out->numAreaPortals = LittleLong (in->numAreaPortals);
		out->firstAreaPortal = LittleLong (in->firstAreaPortal);
		out->floodValid = 0;
		out->floodNum = 0;
	}
}


/*
=================
CM_LoadQ2BSPAreaPortals
=================
*/
static void CM_LoadQ2BSPAreaPortals (dQ2BspLump_t *l)
{
	int					i;
	dQ2BspAreaPortal_t	*out;
	dQ2BspAreaPortal_t	*in;

	in = (void *)(cm_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadQ2BSPAreaPortals: funny lump size");

	cm_numAreaPortals = l->fileLen / sizeof (*in);
	if (cm_numAreaPortals > Q2BSP_MAX_AREAS)
		Com_Error (ERR_DROP, "CM_LoadQ2BSPAreaPortals: Map has too many areas");

	out = cm_mapAreaPortals;
	for (i=0 ; i<cm_numAreaPortals ; i++, in++, out++) {
		out->portalNum = LittleLong (in->portalNum);
		out->otherArea = LittleLong (in->otherArea);
	}
}


/*
=================
CM_LoadQ2BSPVisibility
=================
*/
static void CM_LoadQ2BSPVisibility (dQ2BspLump_t *l)
{
	int		i;

	cm_numVisibility = l->fileLen;
	if (l->fileLen > Q2BSP_MAX_VISIBILITY)
		Com_Error (ERR_DROP, "CM_LoadQ2BSPVisibility: Map has too large visibility lump");

	memcpy (cm_mapVisibility, cm_mapBuffer + l->fileOfs, l->fileLen);

	cm_mapVis->numClusters = LittleLong (cm_mapVis->numClusters);
	for (i=0 ; i<cm_mapVis->numClusters ; i++) {
		cm_mapVis->bitOfs[i][0] = LittleLong (cm_mapVis->bitOfs[i][0]);
		cm_mapVis->bitOfs[i][1] = LittleLong (cm_mapVis->bitOfs[i][1]);
	}
}


/*
=================
CM_LoadQ2BSPEntityString
=================
*/
static void CM_LoadQ2BSPEntityString (dQ2BspLump_t *l)
{
	cm_numEntityChars = l->fileLen;
	if (l->fileLen > Q2BSP_MAX_ENTSTRING)
		Com_Error (ERR_DROP, "CM_LoadQ2BSPEntityString: Map has too large entity lump");

	memcpy (cm_mapEntityString, cm_mapBuffer + l->fileOfs, l->fileLen);
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
	// Byte swap
	//
	header = *(dQ2BspHeader_t *)buffer;
	for (i=0 ; i<sizeof (dQ2BspHeader_t)/4 ; i++)
		((int *)&header)[i] = LittleLong (((int *)&header)[i]);
	cm_mapBuffer = (byte *)buffer;

	//
	// Load into heap
	//
	CM_LoadQ2BSPSurfaces		(&header.lumps[Q2BSP_LUMP_TEXINFO]);
	CM_LoadQ2BSPLeafs			(&header.lumps[Q2BSP_LUMP_LEAFS]);
	CM_LoadQ2BSPLeafBrushes		(&header.lumps[Q2BSP_LUMP_LEAFBRUSHES]);
	CM_LoadQ2BSPPlanes			(&header.lumps[Q2BSP_LUMP_PLANES]);
	CM_LoadQ2BSPBrushes			(&header.lumps[Q2BSP_LUMP_BRUSHES]);
	CM_LoadQ2BSPBrushSides		(&header.lumps[Q2BSP_LUMP_BRUSHSIDES]);
	CM_LoadQ2BSPSubmodels		(&header.lumps[Q2BSP_LUMP_MODELS]);
	CM_LoadQ2BSPNodes			(&header.lumps[Q2BSP_LUMP_NODES]);
	CM_LoadQ2BSPAreas			(&header.lumps[Q2BSP_LUMP_AREAS]);
	CM_LoadQ2BSPAreaPortals		(&header.lumps[Q2BSP_LUMP_AREAPORTALS]);
	CM_LoadQ2BSPVisibility		(&header.lumps[Q2BSP_LUMP_VISIBILITY]);
	CM_LoadQ2BSPEntityString	(&header.lumps[Q2BSP_LUMP_ENTITIES]);

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
	memset (cm_portalOpen, 0, sizeof (cm_portalOpen));
	CM_Q2BSP_FloodAreaConnections ();
}


/*
==================
CM_Q2BSP_UnloadMap
==================
*/
void CM_Q2BSP_UnloadMap (void)
{
	memset (cm_mapEntityString, 0, Q2BSP_MAX_ENTSTRING);

	cm_numEntityChars = 0;

	cm_numAreaPortals = 0;
	cm_numAreas = 1;
	cm_numBrushes = 0;
	cm_numBrushSides = 0;
	cm_numClusters = 1;
	cm_numLeafs = 1;
	cm_numLeafBrushes = 0;
	cm_numNodes = 0;
	cm_numPlanes = 0;
	cm_numTexInfo = 0;
	cm_numVisibility = 0;
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
	return cm_mapEntityString;
}


/*
==================
CM_Q2BSP_SurfRName
==================
*/
char *CM_Q2BSP_SurfRName (int texNum)
{
	return cm_mapSurfaces[texNum].rname;
}


/*
==================
CM_Q2BSP_NumClusters
==================
*/
int CM_Q2BSP_NumClusters (void)
{
	return cm_numClusters;
}


/*
==================
CM_Q2BSP_NumTexInfo
==================
*/
int CM_Q2BSP_NumTexInfo (void)
{
	return cm_numTexInfo;
}
