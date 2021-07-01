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

// cmodel.c
// - model loading

#include "common.h"

typedef struct {
	cBspPlane_t		*plane;
	int				children[2];	// negative numbers are leafs
} cBspNode_t;

typedef struct {
	int				contents;
	int				cluster;
	int				area;
	unsigned short	firstLeafBrush;
	unsigned short	numLeafBrushes;
} cBspLeaf_t;

typedef struct {
	cBspPlane_t		*plane;
	mapSurface_t	*surface;
} cBspBrushSide_t;

typedef struct {
	int				contents;
	int				numSides;
	int				firstBrushSide;
	int				checkCount;		// to avoid repeated testings
} cBspBrush_t;

typedef struct {
	int				numAreaPortals;
	int				firstAreaPortal;
	int				floodNum;			// if two areas have equal floodnums, they are connected
	int				floodValid;
} cBspArea_t;

static char		cm_MapName[MAX_QPATH];
static unsigned	cm_MapChecksum;

static int		cm_NumEntityChars;
static char		cm_MapEntityString[BSP_MAX_ENTSTRING];

static int				cm_NumNodes;
static cBspNode_t		cm_MapNodes[BSP_MAX_NODES+6];		// extra for box hull

static int				cm_NumBrushSides;
static cBspBrushSide_t	cm_MapBrushSides[BSP_MAX_BRUSHSIDES];

static int				cm_NumLeafs = 1;					// allow leaf funcs to be called without a map
static cBspLeaf_t		cm_MapLeafs[BSP_MAX_LEAFS];
static int				cm_EmptyLeaf;

static int				cm_NumLeafBrushes;
static unsigned short	cm_MapLeafBrushes[BSP_MAX_LEAFBRUSHES];

static int				cm_NumBrushes;
static cBspBrush_t		cm_MapBrushes[BSP_MAX_BRUSHES];

static int				cm_NumAreas = 1;
static cBspArea_t		cm_MapAreas[BSP_MAX_AREAS];

int						cm_NumTexInfo;
mapSurface_t			cm_MapSurfaces[BSP_MAX_TEXINFO];
static mapSurface_t		cm_NullSurface;

static int				cm_NumPlanes;
static cBspPlane_t		cm_MapPlanes[BSP_MAX_PLANES+6];	// extra for box hull

static int				cm_NumCModels;
static cBspModel_t		cm_MapCModels[BSP_MAX_MODELS];

static int				cm_NumVisibility;
static byte				cm_MapVisibility[BSP_MAX_VISIBILITY];
static dBspVis_t		*cm_MapVis = (dBspVis_t *)cm_MapVisibility;

static int				cm_NumAreaPortals;
static dBspAreaPortal_t	cm_MapAreaPortals[BSP_MAX_AREAPORTALS];
static qBool			cm_PortalOpen[BSP_MAX_AREAPORTALS];

static int		cm_NumClusters = 1;

static int		cm_CheckCount;
static int		cm_FloodValid;

static cvar_t	*map_noareas;

void	CM_InitBoxHull (void);
void	FloodAreaConnections (void);

int		c_Traces, c_BrushTraces;
int		c_PointContents;

/*
===============================================================================

	MAP LOADING

===============================================================================
*/

byte	*cm_base;

/*
=================
CM_LoadSurfaces
=================
*/
void CM_LoadSurfaces (dBspLump_t *l) {
	dBspTexInfo_t	*in;
	mapSurface_t	*out;
	int				i;

	in = (void *)(cm_base + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadSurfaces: funny lump size");

	cm_NumTexInfo = l->fileLen / sizeof (*in);
	if (cm_NumTexInfo < 1)
		Com_Error (ERR_DROP, "CM_LoadSurfaces: Map with no surfaces");
	if (cm_NumTexInfo > BSP_MAX_TEXINFO)
		Com_Error (ERR_DROP, "CM_LoadSurfaces: Map has too many surfaces");

	out = cm_MapSurfaces;

	for (i=0 ; i<cm_NumTexInfo ; i++, in++, out++) {
		strncpy (out->c.name, in->texture, sizeof (out->c.name)-1);
		strncpy (out->rname, in->texture, sizeof (out->rname)-1);
		out->c.flags = LittleLong (in->flags);
		out->c.value = LittleLong (in->value);
	}
}


/*
=================
CM_LoadLeafs
=================
*/
void CM_LoadLeafs (dBspLump_t *l) {
	int			i;
	cBspLeaf_t	*out;
	dBspLeaf_t	*in;
	
	in = (void *)(cm_base + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadLeafs: funny lump size");

	cm_NumLeafs = l->fileLen / sizeof (*in);
	if (cm_NumLeafs < 1)
		Com_Error (ERR_DROP, "CM_LoadLeafs: Map with no leafs");
	// need to save space for box planes
	if (cm_NumLeafs > BSP_MAX_PLANES)
		Com_Error (ERR_DROP, "CM_LoadLeafs: Map has too many planes");

	out = cm_MapLeafs;
	cm_NumClusters = 0;

	for (i=0 ; i<cm_NumLeafs ; i++, in++, out++) {
		out->contents = LittleLong (in->contents);
		out->cluster = LittleShort (in->cluster);
		out->area = LittleShort (in->area);
		out->firstLeafBrush = LittleShort (in->firstLeafBrush);
		out->numLeafBrushes = LittleShort (in->numLeafBrushes);

		if (out->cluster >= cm_NumClusters)
			cm_NumClusters = out->cluster + 1;
	}

	if (cm_MapLeafs[0].contents != CONTENTS_SOLID)
		Com_Error (ERR_DROP, "CM_LoadLeafs: Map leaf 0 is not CONTENTS_SOLID");
	cm_EmptyLeaf = -1;
	for (i=1 ; i<cm_NumLeafs ; i++) {
		if (!cm_MapLeafs[i].contents) {
			cm_EmptyLeaf = i;
			break;
		}
	}
	if (cm_EmptyLeaf == -1)
		Com_Error (ERR_DROP, "CM_LoadLeafs: Map does not have an empty leaf");
}


/*
=================
CM_LoadLeafBrushes
=================
*/
void CM_LoadLeafBrushes (dBspLump_t *l) {
	int				i;
	unsigned short	*out;
	unsigned short	*in;
	
	in = (void *)(cm_base + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadLeafBrushes: funny lump size");

	cm_NumLeafBrushes = l->fileLen / sizeof (*in);
	if (cm_NumLeafBrushes < 1)
		Com_Error (ERR_DROP, "CM_LoadLeafBrushes: Map with no planes");
	// need to save space for box planes
	if (cm_NumLeafBrushes > BSP_MAX_LEAFBRUSHES)
		Com_Error (ERR_DROP, "CM_LoadLeafBrushes: Map has too many leafbrushes");

	out = cm_MapLeafBrushes;

	for (i=0 ; i<cm_NumLeafBrushes ; i++, in++, out++)
		*out = LittleShort (*in);
}


/*
=================
CM_LoadPlanes
=================
*/
void CM_LoadPlanes (dBspLump_t *l) {
	int			i, j;
	cBspPlane_t	*out;
	dBspPlane_t	*in;
	int			bits;
	
	in = (void *)(cm_base + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadPlanes: funny lump size");

	cm_NumPlanes = l->fileLen / sizeof (*in);
	if (cm_NumPlanes < 1)
		Com_Error (ERR_DROP, "CM_LoadPlanes: Map with no planes");
	// need to save space for box planes
	if (cm_NumPlanes > BSP_MAX_PLANES)
		Com_Error (ERR_DROP, "CM_LoadPlanes: Map has too many planes");

	out = cm_MapPlanes;

	for (i=0 ; i<cm_NumPlanes ; i++, in++, out++) {
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
CM_LoadBrushes
=================
*/
void CM_LoadBrushes (dBspLump_t *l) {
	dBspBrush_t	*in;
	cBspBrush_t	*out;
	int			i;
	
	in = (void *)(cm_base + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadBrushes: funny lump size");

	cm_NumBrushes = l->fileLen / sizeof (*in);
	if (cm_NumBrushes > BSP_MAX_BRUSHES)
		Com_Error (ERR_DROP, "CM_LoadBrushes: Map has too many brushes");

	out = cm_MapBrushes;

	for (i=0 ; i<cm_NumBrushes ; i++, out++, in++) {
		out->firstBrushSide = LittleLong (in->firstSide);
		out->numSides = LittleLong (in->numSides);
		out->contents = LittleLong (in->contents);
	}
}


/*
=================
CM_LoadBrushSides
=================
*/
void CM_LoadBrushSides (dBspLump_t *l) {
	int				i, j;
	cBspBrushSide_t	*out;
	dBspBrushSide_t	*in;
	int				num;

	in = (void *)(cm_base + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadBrushSides: funny lump size");

	cm_NumBrushSides = l->fileLen / sizeof (*in);
	// need to save space for box planes
	if (cm_NumBrushSides > BSP_MAX_BRUSHSIDES)
		Com_Error (ERR_DROP, "CM_LoadBrushSides: Map has too many planes");

	out = cm_MapBrushSides;

	for (i=0 ; i<cm_NumBrushSides ; i++, in++, out++) {
		num = LittleShort (in->planeNum);
		out->plane = &cm_MapPlanes[num];
		j = LittleShort (in->texInfo);
		if (j >= cm_NumTexInfo)
			Com_Error (ERR_DROP, "CM_LoadBrushSides: Bad brushside texInfo");
		out->surface = &cm_MapSurfaces[j];
	}
}


/*
=================
CM_LoadSubmodels
=================
*/
void CM_LoadSubmodels (dBspLump_t *l) {
	dBspModel_t	*in;
	cBspModel_t	*out;
	int			i, j;

	in = (void *)(cm_base + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadSubmodels: funny lump size");

	cm_NumCModels = l->fileLen / sizeof (*in);
	if (cm_NumCModels < 1)
		Com_Error (ERR_DROP, "CM_LoadSubmodels: Map with no models");
	if (cm_NumCModels > BSP_MAX_MODELS)
		Com_Error (ERR_DROP, "CM_LoadSubmodels: Map has too many models");

	for (i=0 ; i<cm_NumCModels ; i++, in++, out++) {
		out = &cm_MapCModels[i];

		for (j=0 ; j<3 ; j++) {
			// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
			out->origin[j] = LittleFloat (in->origin[j]);
		}
		out->headNode = LittleLong (in->headNode);
	}
}


/*
=================
CM_LoadNodes
=================
*/
void CM_LoadNodes (dBspLump_t *l) {
	dBspNode_t		*in;
	cBspNode_t		*out;
	int				child;
	int				i, j;

	in = (void *)(cm_base + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadNodes: funny lump size");

	cm_NumNodes = l->fileLen / sizeof (*in);
	if (cm_NumNodes < 1)
		Com_Error (ERR_DROP, "CM_LoadNodes: Map has no nodes");
	if (cm_NumNodes > BSP_MAX_NODES)
		Com_Error (ERR_DROP, "CM_LoadNodes: Map has too many nodes");

	out = cm_MapNodes;

	for (i=0 ; i<cm_NumNodes ; i++, out++, in++) {
		out->plane = cm_MapPlanes + LittleLong (in->planeNum);
		for (j=0 ; j<2 ; j++) {
			child = LittleLong (in->children[j]);
			out->children[j] = child;
		}
	}
}


/*
=================
CM_LoadAreas
=================
*/
void CM_LoadAreas (dBspLump_t *l) {
	int			i;
	cBspArea_t	*out;
	dBspArea_t	*in;

	in = (void *)(cm_base + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadAreas: funny lump size");

	cm_NumAreas = l->fileLen / sizeof (*in);
	if (cm_NumAreas > BSP_MAX_AREAS)
		Com_Error (ERR_DROP, "CM_LoadAreas: Map has too many areas");

	out = cm_MapAreas;

	for (i=0 ; i<cm_NumAreas ; i++, in++, out++) {
		out->numAreaPortals = LittleLong (in->numAreaPortals);
		out->firstAreaPortal = LittleLong (in->firstAreaPortal);
		out->floodValid = 0;
		out->floodNum = 0;
	}
}


/*
=================
CM_LoadAreaPortals
=================
*/
void CM_LoadAreaPortals (dBspLump_t *l) {
	int					i;
	dBspAreaPortal_t	*out;
	dBspAreaPortal_t	*in;

	in = (void *)(cm_base + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadAreaPortals: funny lump size");

	cm_NumAreaPortals = l->fileLen / sizeof (*in);
	if (cm_NumAreaPortals > BSP_MAX_AREAS)
		Com_Error (ERR_DROP, "CM_LoadAreaPortals: Map has too many areas");

	out = cm_MapAreaPortals;

	for (i=0 ; i<cm_NumAreaPortals ; i++, in++, out++) {
		out->portalNum = LittleLong (in->portalNum);
		out->otherArea = LittleLong (in->otherArea);
	}
}


/*
=================
CM_LoadVisibility
=================
*/
void CM_LoadVisibility (dBspLump_t *l) {
	int		i;

	cm_NumVisibility = l->fileLen;
	if (l->fileLen > BSP_MAX_VISIBILITY)
		Com_Error (ERR_DROP, "CM_LoadVisibility: Map has too large visibility lump");

	memcpy (cm_MapVisibility, cm_base + l->fileOfs, l->fileLen);

	cm_MapVis->numClusters = LittleLong (cm_MapVis->numClusters);
	for (i=0 ; i<cm_MapVis->numClusters ; i++) {
		cm_MapVis->bitOfs[i][0] = LittleLong (cm_MapVis->bitOfs[i][0]);
		cm_MapVis->bitOfs[i][1] = LittleLong (cm_MapVis->bitOfs[i][1]);
	}
}


/*
=================
CM_LoadEntityString
=================
*/
void CM_LoadEntityString (dBspLump_t *l) {
	cm_NumEntityChars = l->fileLen;
	if (l->fileLen > BSP_MAX_ENTSTRING)
		Com_Error (ERR_DROP, "CM_LoadEntityString: Map has too large entity lump");

	memcpy (cm_MapEntityString, cm_base + l->fileOfs, l->fileLen);
}


/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
cBspModel_t *CM_LoadMap (char *name, qBool clientload, unsigned *checksum) {
	unsigned		*buf;
	int				i;
	dBspHeader_t	header;
	int				length;

	map_noareas = Cvar_Get ("map_noareas", "0", 0);

	if (!strcmp (cm_MapName, name) && (clientload || !Cvar_VariableInteger ("flushmap"))) {
		*checksum = cm_MapChecksum;
		if (!clientload) {
			memset (cm_PortalOpen, 0, sizeof (cm_PortalOpen));
			FloodAreaConnections ();
		}
		return &cm_MapCModels[0];		// still have the right version
	}

	// free old stuff
	CM_UnloadMap ();

	if (!name || !name[0]) {
		CM_UnloadMap ();
		*checksum = cm_MapChecksum;
		return &cm_MapCModels[0];			// cinematic servers won't have anything at all
	}

	//
	// load the file
	//
	length = FS_LoadFile (name, (void **)&buf);
	if (!buf)
		Com_Error (ERR_DROP, "CM_LoadMap: Couldn't load %s", name);

	cm_MapChecksum = LittleLong (Com_BlockChecksum (buf, length));
	*checksum = cm_MapChecksum;

	header = *(dBspHeader_t *)buf;
	for (i=0 ; i<sizeof (dBspHeader_t)/4 ; i++)
		((int *)&header)[i] = LittleLong (((int *)&header)[i]);

	if (header.version != BSP_VERSION)
		Com_Error (ERR_DROP, "CM_LoadMap: %s has wrong version number (%i should be %i)", name, header.version, BSP_VERSION);

	cm_base = (byte *)buf;

	// load into heap
	CM_LoadSurfaces		(&header.lumps[LUMP_TEXINFO]);
	CM_LoadLeafs		(&header.lumps[LUMP_LEAFS]);
	CM_LoadLeafBrushes	(&header.lumps[LUMP_LEAFBRUSHES]);
	CM_LoadPlanes		(&header.lumps[LUMP_PLANES]);
	CM_LoadBrushes		(&header.lumps[LUMP_BRUSHES]);
	CM_LoadBrushSides	(&header.lumps[LUMP_BRUSHSIDES]);
	CM_LoadSubmodels	(&header.lumps[LUMP_MODELS]);
	CM_LoadNodes		(&header.lumps[LUMP_NODES]);
	CM_LoadAreas		(&header.lumps[LUMP_AREAS]);
	CM_LoadAreaPortals	(&header.lumps[LUMP_AREAPORTALS]);
	CM_LoadVisibility	(&header.lumps[LUMP_VISIBILITY]);
	CM_LoadEntityString	(&header.lumps[LUMP_ENTITIES]);

	FS_FreeFile (buf);

	CM_InitBoxHull ();

	memset (cm_PortalOpen, 0, sizeof (cm_PortalOpen));
	FloodAreaConnections ();

	strcpy (cm_MapName, name);

	return &cm_MapCModels[0];
}


/*
==================
CM_UnloadMap
==================
*/
void CM_UnloadMap (void) {
	cm_NumEntityChars = 0;
	cm_MapEntityString[0] = 0;
	cm_MapName[0] = 0;
	cm_MapChecksum = 0;

	cm_NumAreaPortals = 0;
	cm_NumAreas = 1;
	cm_NumBrushes = 0;
	cm_NumBrushSides = 0;
	cm_NumClusters = 1;
	cm_NumCModels = 0;
	cm_NumLeafs = 1;
	cm_NumLeafBrushes = 0;
	cm_NumNodes = 0;
	cm_NumPlanes = 0;
	cm_NumTexInfo = 0;
	cm_NumVisibility = 0;

	c_Traces = 0;
	c_BrushTraces = 0;
	c_PointContents = 0;
}


/*
==================
CM_InlineModel
==================
*/
cBspModel_t *CM_InlineModel (char *name) {
	int		num;

	if (!name || (name[0] != '*'))
		Com_Error (ERR_DROP, "CM_InlineModel: bad name");
	num = atoi (name+1);
	if ((num < 1) || (num >= cm_NumCModels))
		Com_Error (ERR_DROP, "CM_InlineModel: bad number");

	return &cm_MapCModels[num];
}

//=======================================================================

/*
==================
CM_EntityString
==================
*/
char *CM_EntityString (void) {
	return cm_MapEntityString;
}


/*
==================
CM_NumClusters
==================
*/
int CM_NumClusters (void) {
	return cm_NumClusters;
}


/*
==================
CM_NumInlineModels
==================
*/
int CM_NumInlineModels (void) {
	return cm_NumCModels;
}


/*
==================
CM_LeafArea
==================
*/
int CM_LeafArea (int leafNum) {
	if ((leafNum < 0) || (leafNum >= cm_NumLeafs))
		Com_Error (ERR_DROP, "CM_LeafArea: bad number");

	return cm_MapLeafs[leafNum].area;
}


/*
==================
CM_LeafCluster
==================
*/
int CM_LeafCluster (int leafNum) {
	if ((leafNum < 0) || (leafNum >= cm_NumLeafs))
		Com_Error (ERR_DROP, "CM_LeafCluster: bad number");

	return cm_MapLeafs[leafNum].cluster;
}


/*
==================
CM_LeafContents
==================
*/
int CM_LeafContents (int leafNum) {
	if ((leafNum < 0) || (leafNum >= cm_NumLeafs))
		Com_Error (ERR_DROP, "CM_LeafContents: bad number");

	return cm_MapLeafs[leafNum].contents;
}

//=======================================================================

cBspPlane_t	*boxPlanes;
int			boxHeadNode;
cBspBrush_t	*boxBrush;
cBspLeaf_t	*boxLeaf;

/*
===================
CM_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
void CM_InitBoxHull (void) {
	int				i;
	int				side;
	cBspNode_t		*c;
	cBspPlane_t		*p;
	cBspBrushSide_t	*s;

	boxHeadNode = cm_NumNodes;
	boxPlanes = &cm_MapPlanes[cm_NumPlanes];
	if ((cm_NumNodes+6 > BSP_MAX_NODES)
		|| (cm_NumBrushes+1 > BSP_MAX_BRUSHES)
		|| (cm_NumLeafBrushes+1 > BSP_MAX_LEAFBRUSHES)
		|| (cm_NumBrushSides+6 > BSP_MAX_BRUSHSIDES)
		|| (cm_NumPlanes+12 > BSP_MAX_PLANES))
		Com_Error (ERR_DROP, "CM_InitBoxHull: Not enough room for box tree");

	boxBrush = &cm_MapBrushes[cm_NumBrushes];
	boxBrush->numSides = 6;
	boxBrush->firstBrushSide = cm_NumBrushSides;
	boxBrush->contents = CONTENTS_MONSTER;

	boxLeaf = &cm_MapLeafs[cm_NumLeafs];
	boxLeaf->contents = CONTENTS_MONSTER;
	boxLeaf->firstLeafBrush = cm_NumLeafBrushes;
	boxLeaf->numLeafBrushes = 1;

	cm_MapLeafBrushes[cm_NumLeafBrushes] = cm_NumBrushes;

	for (i=0 ; i<6 ; i++) {
		side = i&1;

		// brush sides
		s = &cm_MapBrushSides[cm_NumBrushSides+i];
		s->plane = cm_MapPlanes + (cm_NumPlanes+i*2+side);
		s->surface = &cm_NullSurface;

		// nodes
		c = &cm_MapNodes[boxHeadNode+i];
		c->plane = cm_MapPlanes + (cm_NumPlanes+i*2);
		c->children[side] = -1 - cm_EmptyLeaf;
		if (i != 5)
			c->children[side^1] = boxHeadNode+i + 1;
		else
			c->children[side^1] = -1 - cm_NumLeafs;

		// planes
		p = &boxPlanes[i*2];
		p->type = i>>1;
		p->signBits = 0;
		VectorIdentity (p->normal);
		p->normal[i>>1] = 1;

		p = &boxPlanes[i*2+1];
		p->type = 3 + (i>>1);
		p->signBits = 0;
		VectorIdentity (p->normal);
		p->normal[i>>1] = -1;
	}	
}


/*
===================
CM_HeadnodeForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*/
int	CM_HeadnodeForBox (vec3_t mins, vec3_t maxs) {
	boxPlanes[0].dist	= maxs[0];
	boxPlanes[1].dist	= -maxs[0];
	boxPlanes[2].dist	= mins[0];
	boxPlanes[3].dist	= -mins[0];
	boxPlanes[4].dist	= maxs[1];
	boxPlanes[5].dist	= -maxs[1];
	boxPlanes[6].dist	= mins[1];
	boxPlanes[7].dist	= -mins[1];
	boxPlanes[8].dist	= maxs[2];
	boxPlanes[9].dist	= -maxs[2];
	boxPlanes[10].dist	= mins[2];
	boxPlanes[11].dist	= -mins[2];

	return boxHeadNode;
}


/*
==================
CM_PointLeafnum_r
==================
*/
int CM_PointLeafnum_r (vec3_t p, int num) {
	float		d;
	cBspNode_t	*node;
	cBspPlane_t	*plane;

	while (num >= 0) {
		node = cm_MapNodes + num;
		plane = node->plane;
		
		if (plane->type < 3)
			d = p[plane->type] - plane->dist;
		else
			d = DotProduct (plane->normal, p) - plane->dist;

		if (d < 0)
			num = node->children[1];
		else
			num = node->children[0];
	}

	c_PointContents++;	// optimize counter

	return -1 - num;
}


/*
==================
CM_PointLeafnum
==================
*/
int CM_PointLeafnum (vec3_t p) {
	if (!cm_NumPlanes)
		return 0;	// sound may call this without map loaded
	return CM_PointLeafnum_r (p, 0);
}


/*
=============
CM_BoxLeafnums

Fills in a list of all the leafs touched
=============
*/
int		leafCount, leafMaxCount;
int		*leafList;
float	*leafMins, *leafMaxs;
int		leafTopNode;
void CM_BoxLeafnums_r (int nodenum) {
	cBspPlane_t	*plane;
	cBspNode_t	*node;
	int		s;

	for ( ; ; ) {
		if (nodenum < 0) {
			if (leafCount >= leafMaxCount)
				return;

			leafList[leafCount++] = -1 - nodenum;
			return;
		}
	
		node = &cm_MapNodes[nodenum];
		plane = node->plane;
		s = BOX_ON_PLANE_SIDE (leafMins, leafMaxs, plane);
		if (s == 1)
			nodenum = node->children[0];
		else if (s == 2)
			nodenum = node->children[1];
		else {
			// go down both
			if (leafTopNode == -1)
				leafTopNode = nodenum;
			CM_BoxLeafnums_r (node->children[0]);
			nodenum = node->children[1];
		}
	}
}

/*
==================
CM_BoxLeafnumsHeadNode
==================
*/
int	CM_BoxLeafnumsHeadNode (vec3_t mins, vec3_t maxs, int *list, int listsize, int headnode, int *topnode) {
	leafList = list;
	leafCount = 0;
	leafMaxCount = listsize;
	leafMins = mins;
	leafMaxs = maxs;
	leafTopNode = -1;

	CM_BoxLeafnums_r (headnode);

	if (topnode)
		*topnode = leafTopNode;

	return leafCount;
}


/*
==================
CM_BoxLeafnums
==================
*/
int	CM_BoxLeafnums (vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode) {
	return CM_BoxLeafnumsHeadNode (mins, maxs, list, listsize, cm_MapCModels[0].headNode, topnode);
}


/*
==================
CM_PointContents
==================
*/
int CM_PointContents (vec3_t p, int headnode) {
	int		l;

	if (!cm_NumNodes)	// map not loaded
		return 0;

	l = CM_PointLeafnum_r (p, headnode);

	return cm_MapLeafs[l].contents;
}


/*
==================
CM_TransformedPointContents

Handles offseting and rotation of the end points for moving and rotating entities
==================
*/
int	CM_TransformedPointContents (vec3_t p, int headnode, vec3_t origin, vec3_t angles) {
	vec3_t	p_l;
	vec3_t	temp;
	vec3_t	forward, right, up;
	int		l;

	/* subtract origin offset */
	VectorSubtract (p, origin, p_l);

	/* rotate start and end into the models frame of reference */
	if (headnode != boxHeadNode && (angles[0] || angles[1] || angles[2])) {
		AngleVectors (angles, forward, right, up);

		VectorCopy (p_l, temp);
		p_l[0] = DotProduct (temp, forward);
		p_l[1] = -DotProduct (temp, right);
		p_l[2] = DotProduct (temp, up);
	}

	l = CM_PointLeafnum_r (p_l, headnode);

	return cm_MapLeafs[l].contents;
}

/*
===============================================================================

	BOX TRACING

===============================================================================
*/

// 1/32 epsilon to keep floating point happy
#define	DIST_EPSILON	(0.03125)

vec3_t	trace_start, trace_end;
vec3_t	trace_mins, trace_maxs;
vec3_t	trace_extents;

trace_t	trace_trace;
int		trace_contents;
qBool	trace_ispoint;		// optimized case

/*
================
CM_ClipBoxToBrush
================
*/
void CM_ClipBoxToBrush (vec3_t mins, vec3_t maxs, vec3_t p1, vec3_t p2, trace_t *trace, cBspBrush_t *brush) {
	int				i, j;
	cBspPlane_t		*plane, *clipplane;
	float			dist, dot1, dot2, f;
	float			enterfrac, leavefrac;
	vec3_t			ofs;
	qBool			getout, startout;
	cBspBrushSide_t	*side, *leadside;

	enterfrac = -1;
	leavefrac = 1;
	clipplane = NULL;

	if (!brush->numSides)
		return;

	c_BrushTraces++;

	getout = qFalse;
	startout = qFalse;
	leadside = NULL;

	for (i=0 ; i<brush->numSides ; i++) 	{
		side = &cm_MapBrushSides[brush->firstBrushSide+i];
		plane = side->plane;

		// FIXME: special case for axial
		if (!trace_ispoint) {
			// general box case
			// push the plane out apropriately for mins/maxs
			// FIXME: use signBits into 8 way lookup for each mins/maxs
			for (j=0 ; j<3 ; j++)
			{
				if (plane->normal[j] < 0)
					ofs[j] = maxs[j];
				else
					ofs[j] = mins[j];
			}
			dist = DotProduct (ofs, plane->normal);
			dist = plane->dist - dist;
		} else {
			// special point case
			dist = plane->dist;
		}

		dot1 = DotProduct (p1, plane->normal) - dist;
		dot2 = DotProduct (p2, plane->normal) - dist;

		if (dot2 > 0)
			getout = qTrue;	// endpoint is not in solid
		if (dot1 > 0)
			startout = qTrue;

		// if completely in front of face, no intersection
		if ((dot1 > 0) && (dot2 >= dot1))
			return;

		if ((dot1 <= 0) && (dot2 <= 0))
			continue;

		// crosses face
		if (dot1 > dot2) {
			// enter
			f = (dot1-DIST_EPSILON) / (dot1-dot2);
			if (f > enterfrac) {
				enterfrac = f;
				clipplane = plane;
				leadside = side;
			}
		} else {
			// leave
			f = (dot1+DIST_EPSILON) / (dot1-dot2);
			if (f < leavefrac)
				leavefrac = f;
		}
	}

	if (!startout) {
		// original point was inside brush
		trace->startSolid = qTrue;
		if (!getout)
			trace->allSolid = qTrue;
		return;
	}

	if (enterfrac < leavefrac) {
		if ((enterfrac > -1) && (enterfrac < trace->fraction)) {
			if (enterfrac < 0)
				enterfrac = 0;

			trace->fraction = enterfrac;
			trace->plane = *clipplane;
			trace->surface = &(leadside->surface->c);
			trace->contents = brush->contents;
		}
	}
}


/*
================
CM_TestBoxInBrush
================
*/
void CM_TestBoxInBrush (vec3_t mins, vec3_t maxs, vec3_t p1, trace_t *trace, cBspBrush_t *brush) {
	int				i, j;
	vec3_t			ofs;
	cBspPlane_t		*plane;
	cBspBrushSide_t	*side;
	float			dist, dot;

	if (!brush->numSides)
		return;

	for (i=0 ; i<brush->numSides ; i++) {
		side = &cm_MapBrushSides[brush->firstBrushSide+i];
		plane = side->plane;

		// FIXME: special case for axial
		// general box case
		// push the plane out apropriately for mins/maxs
		// FIXME: use signBits into 8 way lookup for each mins/maxs
		for (j=0 ; j<3 ; j++) {
			if (plane->normal[j] < 0)
				ofs[j] = maxs[j];
			else
				ofs[j] = mins[j];
		}

		dist = plane->dist - DotProduct (ofs, plane->normal);
		dot = DotProduct (p1, plane->normal) - dist;

		// if completely in front of face, no intersection
		if (dot > 0)
			return;
	}

	// inside this brush
	trace->startSolid = trace->allSolid = qTrue;
	trace->fraction = 0;
	trace->contents = brush->contents;
}


/*
================
CM_TraceToLeaf
================
*/
void CM_TraceToLeaf (int leafnum) {
	int			k;
	int			brushnum;
	cBspLeaf_t	*leaf;
	cBspBrush_t	*b;

	leaf = &cm_MapLeafs[leafnum];
	if (!(leaf->contents & trace_contents))
		return;

	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numLeafBrushes ; k++) {
		brushnum = cm_MapLeafBrushes[leaf->firstLeafBrush+k];
		b = &cm_MapBrushes[brushnum];
		if (b->checkCount == cm_CheckCount)
			continue;	// already checked this brush in another leaf

		b->checkCount = cm_CheckCount;

		if (!(b->contents & trace_contents))
			continue;

		CM_ClipBoxToBrush (trace_mins, trace_maxs, trace_start, trace_end, &trace_trace, b);
		if (!trace_trace.fraction)
			return;
	}
}


/*
================
CM_TestInLeaf
================
*/
void CM_TestInLeaf (int leafnum) {
	int			k;
	int			brushnum;
	cBspLeaf_t	*leaf;
	cBspBrush_t	*b;

	leaf = &cm_MapLeafs[leafnum];
	if (!(leaf->contents & trace_contents))
		return;

	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numLeafBrushes ; k++) {
		brushnum = cm_MapLeafBrushes[leaf->firstLeafBrush+k];
		b = &cm_MapBrushes[brushnum];
		if (b->checkCount == cm_CheckCount)
			continue;	// already checked this brush in another leaf

		b->checkCount = cm_CheckCount;

		if (!(b->contents & trace_contents))
			continue;

		CM_TestBoxInBrush (trace_mins, trace_maxs, trace_start, &trace_trace, b);
		if (!trace_trace.fraction)
			return;
	}
}


/*
==================
CM_RecursiveHullCheck
==================
*/
void CM_RecursiveHullCheck (int num, float p1f, float p2f, vec3_t p1, vec3_t p2) {
	cBspNode_t	*node;
	cBspPlane_t	*plane;
	float		t1, t2, offset;
	float		frac, frac2;
	float		idist;
	int			i, side;
	vec3_t		mid;
	float		midf;

	if (trace_trace.fraction <= p1f)
		return;		// already hit something nearer

	// if < 0, we are in a leaf node
	if (num < 0) {
		CM_TraceToLeaf (-1-num);
		return;
	}

	/*
	** find the point distances to the seperating plane
	** and the offset for the size of the box
	*/
	node = cm_MapNodes + num;
	plane = node->plane;

	if (plane->type < 3) {
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
		offset = trace_extents[plane->type];
	} else {
		t1 = DotProduct (plane->normal, p1) - plane->dist;
		t2 = DotProduct (plane->normal, p2) - plane->dist;
		if (trace_ispoint)
			offset = 0;
		else
			offset = fabs (trace_extents[0]*plane->normal[0]) +
				fabs (trace_extents[1]*plane->normal[1]) +
				fabs (trace_extents[2]*plane->normal[2]);
	}

	// see which sides we need to consider
	if ((t1 >= offset) && (t2 >= offset)) {
		CM_RecursiveHullCheck (node->children[0], p1f, p2f, p1, p2);
		return;
	}
	if ((t1 < -offset) && (t2 < -offset)) {
		CM_RecursiveHullCheck (node->children[1], p1f, p2f, p1, p2);
		return;
	}

	// put the crosspoint DIST_EPSILON pixels on the near side
	if (t1 < t2) {
		idist = 1.0/(t1-t2);
		side = 1;
		frac2 = (t1 + offset + DIST_EPSILON)*idist;
		frac = (t1 - offset + DIST_EPSILON)*idist;
	} else if (t1 > t2) {
		idist = 1.0/(t1-t2);
		side = 0;
		frac2 = (t1 - offset - DIST_EPSILON)*idist;
		frac = (t1 + offset + DIST_EPSILON)*idist;
	} else {
		side = 0;
		frac = 1;
		frac2 = 0;
	}

	// move up to the node
	if (frac < 0)
		frac = 0;
	if (frac > 1)
		frac = 1;
		
	midf = p1f + (p2f - p1f )* frac;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac * (p2[i] - p1[i]);

	CM_RecursiveHullCheck (node->children[side], p1f, midf, p1, mid);

	// go past the node
	if (frac2 < 0)
		frac2 = 0;
	if (frac2 > 1)
		frac2 = 1;
		
	midf = p1f + (p2f - p1f) * frac2;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac2 * (p2[i] - p1[i]);

	CM_RecursiveHullCheck (node->children[side^1], midf, p2f, mid, p2);
}

//======================================================================

/*
====================
CL_Trace
====================
*/
trace_t CL_Trace (vec3_t start, vec3_t end, float size, int contentmask) {
	vec3_t maxs, mins;

	VectorSet (maxs, size, size, size);
	VectorSet (mins, -size, -size, -size);

	return CM_BoxTrace (start, end, mins, maxs, 0, contentmask);
}


/*
==================
CM_BoxTrace
==================
*/
trace_t CM_BoxTrace (vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int headnode, int brushmask) {
	int		i;

	cm_CheckCount++;	// for multi-check avoidance
	c_Traces++;			// for statistics, may be zeroed

	// fill in a default trace
	memset (&trace_trace, 0, sizeof (trace_trace));
	trace_trace.fraction = 1;
	trace_trace.surface = &(cm_NullSurface.c);

	if (!cm_NumNodes)	// map not loaded
		return trace_trace;

	trace_contents = brushmask;
	VectorCopy (start, trace_start);
	VectorCopy (end, trace_end);
	VectorCopy (mins, trace_mins);
	VectorCopy (maxs, trace_maxs);

	// check for position test special case
	if (VectorCompare (start, end)) {
		int		leafs[1024];
		int		i, numLeafs;
		vec3_t	c1, c2;
		int		topnode;

		VectorAdd (start, mins, c1);
		VectorAdd (start, maxs, c2);
		for (i=0 ; i<3 ; i++) {
			c1[i] -= 1;
			c2[i] += 1;
		}

		numLeafs = CM_BoxLeafnumsHeadNode (c1, c2, leafs, 1024, headnode, &topnode);
		for (i=0 ; i<numLeafs ; i++) {
			CM_TestInLeaf (leafs[i]);
			if (trace_trace.allSolid)
				break;
		}
		VectorCopy (start, trace_trace.endPos);
		return trace_trace;
	}

	// check for point special case
	if (VectorCompare (mins, vec3Identity) && VectorCompare (maxs, vec3Identity)) {
		trace_ispoint = qTrue;
		VectorIdentity (trace_extents);
	} else {
		trace_ispoint = qFalse;
		trace_extents[0] = -mins[0] > maxs[0] ? -mins[0] : maxs[0];
		trace_extents[1] = -mins[1] > maxs[1] ? -mins[1] : maxs[1];
		trace_extents[2] = -mins[2] > maxs[2] ? -mins[2] : maxs[2];
	}

	// general sweeping through world
	CM_RecursiveHullCheck (headnode, 0, 1, start, end);

	if (trace_trace.fraction == 1) {
		VectorCopy (end, trace_trace.endPos);
	} else {
		for (i=0 ; i<3 ; i++)
			trace_trace.endPos[i] = start[i] + trace_trace.fraction * (end[i] - start[i]);
	}
	return trace_trace;
}


/*
==================
CM_TransformedBoxTrace

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
#ifdef _WIN32
#pragma optimize ("", off)
#endif
trace_t CM_TransformedBoxTrace (vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs,
						  int headnode, int brushmask, vec3_t origin, vec3_t angles) {
	trace_t		trace;
	vec3_t		start_l, end_l;
	vec3_t		forward, right, up;
	vec3_t		temp, a;
	qBool		rotated;

	// subtract origin offset
	VectorSubtract (start, origin, start_l);
	VectorSubtract (end, origin, end_l);

	// rotate start and end into the models frame of reference
	if ((headnode != boxHeadNode) && (angles[0] || angles[1] || angles[2]))
		rotated = qTrue;
	else
		rotated = qFalse;

	if (rotated) {
		AngleVectors (angles, forward, right, up);

		VectorCopy (start_l, temp);
		start_l[0] = DotProduct (temp, forward);
		start_l[1] = -DotProduct (temp, right);
		start_l[2] = DotProduct (temp, up);

		VectorCopy (end_l, temp);
		end_l[0] = DotProduct (temp, forward);
		end_l[1] = -DotProduct (temp, right);
		end_l[2] = DotProduct (temp, up);
	}

	// sweep the box through the model
	trace = CM_BoxTrace (start_l, end_l, mins, maxs, headnode, brushmask);

	if (rotated && trace.fraction != 1.0) {
		// FIXME: figure out how to do this with existing angles
		VectorNegate (angles, a);
		AngleVectors (a, forward, right, up);

		VectorCopy (trace.plane.normal, temp);
		trace.plane.normal[0] = DotProduct (temp, forward);
		trace.plane.normal[1] = -DotProduct (temp, right);
		trace.plane.normal[2] = DotProduct (temp, up);
	}

	trace.endPos[0] = start[0] + trace.fraction * (end[0] - start[0]);
	trace.endPos[1] = start[1] + trace.fraction * (end[1] - start[1]);
	trace.endPos[2] = start[2] + trace.fraction * (end[2] - start[2]);

	return trace;
}
#ifdef _WIN32
#pragma optimize ("", on)
#endif

/*
===============================================================================

	PVS / PHS

===============================================================================
*/

/*
===================
CM_DecompressVis
===================
*/
void CM_DecompressVis (byte *in, byte *out) {
	int		c;
	byte	*out_p;
	int		row;

	row = (cm_NumClusters + 7) >> 3;	
	out_p = out;

	if (!in || !cm_NumVisibility) {
		// no vis info, so make all visible
		while (row) {
			*out_p++ = 0xff;
			row--;
		}
		return;		
	}

	do {
		if (*in) {
			*out_p++ = *in++;
			continue;
		}
	
		c = in[1];
		in += 2;
		if ((out_p - out) + c > row) {
			c = row - (out_p - out);
			Com_Printf (PRINT_DEV, S_COLOR_YELLOW "Warning: Vis decompression overrun\n");
		}

		while (c) {
			*out_p++ = 0;
			c--;
		}
	} while (out_p - out < row);
}


/*
===================
CM_ClusterPVS
===================
*/
byte	pvsRow[BSP_MAX_VIS];
byte *CM_ClusterPVS (int cluster) {
	if (cluster == -1)
		memset (pvsRow, 0, (cm_NumClusters + 7) >> 3);
	else
		CM_DecompressVis (cm_MapVisibility + cm_MapVis->bitOfs[cluster][BSP_VIS_PVS], pvsRow);
	return pvsRow;
}


/*
===================
CM_ClusterPHS
===================
*/
byte	phsRow[BSP_MAX_VIS];
byte *CM_ClusterPHS (int cluster) {
	if (cluster == -1)
		memset (phsRow, 0, (cm_NumClusters + 7) >> 3);
	else
		CM_DecompressVis (cm_MapVisibility + cm_MapVis->bitOfs[cluster][BSP_VIS_PHS], phsRow);
	return phsRow;
}

/*
===============================================================================

	AREAPORTALS

===============================================================================
*/

/*
====================
FloodAreaConnections
====================
*/
void FloodArea_r (cBspArea_t *area, int floodNum) {
	dBspAreaPortal_t	*p;
	int					i;

	if (area->floodValid == cm_FloodValid) {
		if (area->floodNum == floodNum)
			return;
		Com_Error (ERR_DROP, "FloodArea_r: reflooded");
	}

	area->floodNum = floodNum;
	area->floodValid = cm_FloodValid;
	p = &cm_MapAreaPortals[area->firstAreaPortal];
	for (i=0 ; i<area->numAreaPortals ; i++, p++) {
		if (cm_PortalOpen[p->portalNum])
			FloodArea_r (&cm_MapAreas[p->otherArea], floodNum);
	}
}
void FloodAreaConnections (void) {
	int			i;
	cBspArea_t	*area;
	int			floodNum;

	// all current floods are now invalid
	cm_FloodValid++;
	floodNum = 0;

	// area 0 is not used
	for (i=1 ; i<cm_NumAreas ; i++) {
		area = &cm_MapAreas[i];
		if (area->floodValid == cm_FloodValid)
			continue;		// already flooded into
		floodNum++;
		FloodArea_r (area, floodNum);
	}

}

void CM_SetAreaPortalState (int portalnum, qBool open) {
	if (portalnum > cm_NumAreaPortals)
		Com_Error (ERR_DROP, "CM_SetAreaPortalState: areaportal > numAreaPortals");

	cm_PortalOpen[portalnum] = open;
	FloodAreaConnections ();
}

qBool CM_AreasConnected (int area1, int area2) {
	if (map_noareas->value)
		return qTrue;

	if ((area1 > cm_NumAreas) || (area2 > cm_NumAreas))
		Com_Error (ERR_DROP, "CM_AreasConnected: area > cm_NumAreas");

	if (cm_MapAreas[area1].floodNum == cm_MapAreas[area2].floodNum)
		return qTrue;
	return qFalse;
}


/*
=================
CM_WriteAreaBits

Writes a length byte followed by a bit vector of all the areas
that area in the same flood as the area parameter

This is used by the client refreshes to cull visibility
=================
*/
int CM_WriteAreaBits (byte *buffer, int area) {
	int		i;
	int		floodNum;
	int		bytes;

	bytes = (cm_NumAreas+7)>>3;

	if (map_noareas->value) {
		// for debugging, send everything
		memset (buffer, 255, bytes);
	} else {
		memset (buffer, 0, bytes);

		floodNum = cm_MapAreas[area].floodNum;
		for (i=0 ; i<cm_NumAreas ; i++) {
			if ((cm_MapAreas[i].floodNum == floodNum) || !area)
				buffer[i>>3] |= 1<<(i&7);
		}
	}

	return bytes;
}


/*
===================
CM_WritePortalState

Writes the portal state to a savegame file
===================
*/
void CM_WritePortalState (FILE *f) {
	fwrite (cm_PortalOpen, sizeof (cm_PortalOpen), 1, f);
}


/*
===================
CM_ReadPortalState

Reads the portal state from a savegame file
and recalculates the area connections
===================
*/
void CM_ReadPortalState (FILE *f) {
	FS_Read (cm_PortalOpen, sizeof (cm_PortalOpen), f);
	FloodAreaConnections ();
}


/*
=============
CM_HeadnodeVisible

Returns qTrue if any leaf under headnode has a cluster that
is potentially visible
=============
*/
qBool CM_HeadnodeVisible (int nodenum, byte *visbits) {
	int			leafnum;
	int			cluster;
	cBspNode_t	*node;

	if (nodenum < 0) {
		leafnum = -1-nodenum;
		cluster = cm_MapLeafs[leafnum].cluster;
		if (cluster == -1)
			return qFalse;
		if (visbits[cluster>>3] & (1<<(cluster&7)))
			return qTrue;
		return qFalse;
	}

	node = &cm_MapNodes[nodenum];
	if (CM_HeadnodeVisible (node->children[0], visbits))
		return qTrue;
	return CM_HeadnodeVisible (node->children[1], visbits);
}
