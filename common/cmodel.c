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
// cmodel.c
// Map model loading and tracing
//

#define USE_BYTESWAP
#include "common.h"

typedef struct cBspNode_s {
	cBspPlane_t		*plane;
	int				children[2];	// negative numbers are leafs
} cBspNode_t;

typedef struct cBspLeaf_s {
	int				contents;
	int				cluster;
	int				area;
	uShort			firstLeafBrush;
	uShort			numLeafBrushes;
} cBspLeaf_t;

typedef struct cMapSurface_s {	// used internally due to name len probs - ZOID
	cBspSurface_t	c;
	char			rname[32];
} cMapSurface_t;

typedef struct cBspBrushSide_s {
	cBspPlane_t		*plane;
	cMapSurface_t	*surface;
} cBspBrushSide_t;

typedef struct cBspBrush_s {
	int				contents;
	int				numSides;
	int				firstBrushSide;
	int				checkCount;		// to avoid repeated testings
} cBspBrush_t;

typedef struct cBspArea_s {
	int				numAreaPortals;
	int				firstAreaPortal;
	int				floodNum;			// if two areas have equal floodnums, they are connected
	int				floodValid;
} cBspArea_t;

static char				cmMapName[MAX_QPATH];
static uInt				cmMapChecksum;
static qBool			cmClientLoad;

static int				cmNumEntityChars;
static char				cmMapEntityString[BSP_MAX_ENTSTRING];

static int				cmNumNodes;
static cBspNode_t		cmMapNodes[BSP_MAX_NODES+6];		// extra for box hull

static int				cmNumBrushSides;
static cBspBrushSide_t	cmMapBrushSides[BSP_MAX_BRUSHSIDES];

static int				cmNumLeafs = 1;					// allow leaf funcs to be called without a map
static cBspLeaf_t		cmMapLeafs[BSP_MAX_LEAFS];
static int				cmEmptyLeaf;

static int				cmNumLeafBrushes;
static uShort			cmMapLeafBrushes[BSP_MAX_LEAFBRUSHES];

static int				cmNumBrushes;
static cBspBrush_t		cmMapBrushes[BSP_MAX_BRUSHES];

static int				cmNumAreas = 1;
static cBspArea_t		cmMapAreas[BSP_MAX_AREAS];

static int				cmNumTexInfo;
static cMapSurface_t	cmMapSurfaces[BSP_MAX_TEXINFO];
static cMapSurface_t	cmNullSurface;

static int				cmNumPlanes;
static cBspPlane_t		cmMapPlanes[BSP_MAX_PLANES+6];	// extra for box hull

static int				cmNumCModels;
static cBspModel_t		cmMapCModels[BSP_MAX_MODELS];

static int				cmNumVisibility;
static byte				cmMapVisibility[BSP_MAX_VISIBILITY];
static dBspVis_t		*cmMapVis = (dBspVis_t *)cmMapVisibility;

static int				cmNumAreaPortals;
static dBspAreaPortal_t	cmMapAreaPortals[BSP_MAX_AREAPORTALS];
static qBool			cmPortalOpen[BSP_MAX_AREAPORTALS];

static int		cmNumClusters = 1;

static int		cmCheckCount;
static int		cmFloodValid;

static cVar_t	*flushmap;
static cVar_t	*map_noareas;

void	CM_InitBoxHull (void);
void	FloodAreaConnections (void);

int		cmTraces, cmBrushTraces;
int		cmPointContents;

/*
=============================================================================

	MAP LOADING

=============================================================================
*/

byte	*cmbase;

/*
=================
CM_LoadSurfaces
=================
*/
void CM_LoadSurfaces (dBspLump_t *l) {
	dBspTexInfo_t	*in;
	cMapSurface_t	*out;
	int				i;

	in = (void *)(cmbase + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadSurfaces: funny lump size");

	cmNumTexInfo = l->fileLen / sizeof (*in);
	if (cmNumTexInfo < 1)
		Com_Error (ERR_DROP, "CM_LoadSurfaces: Map with no surfaces");
	if (cmNumTexInfo > BSP_MAX_TEXINFO)
		Com_Error (ERR_DROP, "CM_LoadSurfaces: Map has too many surfaces");

	out = cmMapSurfaces;

	for (i=0 ; i<cmNumTexInfo ; i++, in++, out++) {
		Q_strncpyz (out->c.name, in->texture, sizeof (out->c.name));
		Q_strncpyz (out->rname, in->texture, sizeof (out->rname));
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
	
	in = (void *)(cmbase + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadLeafs: funny lump size");

	cmNumLeafs = l->fileLen / sizeof (*in);
	if (cmNumLeafs < 1)
		Com_Error (ERR_DROP, "CM_LoadLeafs: Map with no leafs");
	// need to save space for box planes
	if (cmNumLeafs > BSP_MAX_PLANES)
		Com_Error (ERR_DROP, "CM_LoadLeafs: Map has too many planes");

	out = cmMapLeafs;
	cmNumClusters = 0;

	for (i=0 ; i<cmNumLeafs ; i++, in++, out++) {
		out->contents = LittleLong (in->contents);
		out->cluster = LittleShort (in->cluster);
		out->area = LittleShort (in->area);
		out->firstLeafBrush = LittleShort (in->firstLeafBrush);
		out->numLeafBrushes = LittleShort (in->numLeafBrushes);

		if (out->cluster >= cmNumClusters)
			cmNumClusters = out->cluster + 1;
	}

	if (cmMapLeafs[0].contents != CONTENTS_SOLID)
		Com_Error (ERR_DROP, "CM_LoadLeafs: Map leaf 0 is not CONTENTS_SOLID");
	cmEmptyLeaf = -1;
	for (i=1 ; i<cmNumLeafs ; i++) {
		if (!cmMapLeafs[i].contents) {
			cmEmptyLeaf = i;
			break;
		}
	}
	if (cmEmptyLeaf == -1)
		Com_Error (ERR_DROP, "CM_LoadLeafs: Map does not have an empty leaf");
}


/*
=================
CM_LoadLeafBrushes
=================
*/
void CM_LoadLeafBrushes (dBspLump_t *l) {
	int		i;
	uShort	*out;
	uShort	*in;
	
	in = (void *)(cmbase + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadLeafBrushes: funny lump size");

	cmNumLeafBrushes = l->fileLen / sizeof (*in);
	if (cmNumLeafBrushes < 1)
		Com_Error (ERR_DROP, "CM_LoadLeafBrushes: Map with no planes");

	// need to save space for box planes
	if (cmNumLeafBrushes > BSP_MAX_LEAFBRUSHES)
		Com_Error (ERR_DROP, "CM_LoadLeafBrushes: Map has too many leafbrushes");

	out = cmMapLeafBrushes;

	for (i=0 ; i<cmNumLeafBrushes ; i++, in++, out++)
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
	
	in = (void *)(cmbase + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadPlanes: funny lump size");

	cmNumPlanes = l->fileLen / sizeof (*in);
	if (cmNumPlanes < 1)
		Com_Error (ERR_DROP, "CM_LoadPlanes: Map with no planes");

	// need to save space for box planes
	if (cmNumPlanes > BSP_MAX_PLANES)
		Com_Error (ERR_DROP, "CM_LoadPlanes: Map has too many planes");

	out = cmMapPlanes;

	for (i=0 ; i<cmNumPlanes ; i++, in++, out++) {
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
	
	in = (void *)(cmbase + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadBrushes: funny lump size");

	cmNumBrushes = l->fileLen / sizeof (*in);
	if (cmNumBrushes > BSP_MAX_BRUSHES)
		Com_Error (ERR_DROP, "CM_LoadBrushes: Map has too many brushes");

	out = cmMapBrushes;

	for (i=0 ; i<cmNumBrushes ; i++, out++, in++) {
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

	in = (void *)(cmbase + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadBrushSides: funny lump size");

	cmNumBrushSides = l->fileLen / sizeof (*in);
	// need to save space for box planes
	if (cmNumBrushSides > BSP_MAX_BRUSHSIDES)
		Com_Error (ERR_DROP, "CM_LoadBrushSides: Map has too many planes");

	out = cmMapBrushSides;

	for (i=0 ; i<cmNumBrushSides ; i++, in++, out++) {
		num = LittleShort (in->planeNum);
		out->plane = &cmMapPlanes[num];
		j = LittleShort (in->texInfo);
		if (j >= cmNumTexInfo)
			Com_Error (ERR_DROP, "CM_LoadBrushSides: Bad brushside texInfo");
		out->surface = &cmMapSurfaces[j];
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

	in = (void *)(cmbase + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadSubmodels: funny lump size");

	cmNumCModels = l->fileLen / sizeof (*in);
	if (cmNumCModels < 1)
		Com_Error (ERR_DROP, "CM_LoadSubmodels: Map with no models");
	if (cmNumCModels > BSP_MAX_MODELS)
		Com_Error (ERR_DROP, "CM_LoadSubmodels: Map has too many models");

	for (i=0 ; i<cmNumCModels ; i++, in++, out++) {
		out = &cmMapCModels[i];

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
	int				i;

	in = (void *)(cmbase + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadNodes: funny lump size");

	cmNumNodes = l->fileLen / sizeof (*in);
	if (cmNumNodes < 1)
		Com_Error (ERR_DROP, "CM_LoadNodes: Map has no nodes");
	if (cmNumNodes > BSP_MAX_NODES)
		Com_Error (ERR_DROP, "CM_LoadNodes: Map has too many nodes");

	out = cmMapNodes;

	for (i=0 ; i<cmNumNodes ; i++, out++, in++) {
		out->plane = cmMapPlanes + LittleLong (in->planeNum);

		child = LittleLong (in->children[0]);
		out->children[0] = child;

		child = LittleLong (in->children[1]);
		out->children[1] = child;
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

	in = (void *)(cmbase + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadAreas: funny lump size");

	cmNumAreas = l->fileLen / sizeof (*in);
	if (cmNumAreas > BSP_MAX_AREAS)
		Com_Error (ERR_DROP, "CM_LoadAreas: Map has too many areas");

	out = cmMapAreas;

	for (i=0 ; i<cmNumAreas ; i++, in++, out++) {
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

	in = (void *)(cmbase + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadAreaPortals: funny lump size");

	cmNumAreaPortals = l->fileLen / sizeof (*in);
	if (cmNumAreaPortals > BSP_MAX_AREAS)
		Com_Error (ERR_DROP, "CM_LoadAreaPortals: Map has too many areas");

	out = cmMapAreaPortals;

	for (i=0 ; i<cmNumAreaPortals ; i++, in++, out++) {
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

	cmNumVisibility = l->fileLen;
	if (l->fileLen > BSP_MAX_VISIBILITY)
		Com_Error (ERR_DROP, "CM_LoadVisibility: Map has too large visibility lump");

	memcpy (cmMapVisibility, cmbase + l->fileOfs, l->fileLen);

	cmMapVis->numClusters = LittleLong (cmMapVis->numClusters);
	for (i=0 ; i<cmMapVis->numClusters ; i++) {
		cmMapVis->bitOfs[i][0] = LittleLong (cmMapVis->bitOfs[i][0]);
		cmMapVis->bitOfs[i][1] = LittleLong (cmMapVis->bitOfs[i][1]);
	}
}


/*
=================
CM_LoadEntityString
=================
*/
void CM_LoadEntityString (dBspLump_t *l) {
	cmNumEntityChars = l->fileLen;
	if (l->fileLen > BSP_MAX_ENTSTRING)
		Com_Error (ERR_DROP, "CM_LoadEntityString: Map has too large entity lump");

	memcpy (cmMapEntityString, cmbase + l->fileOfs, l->fileLen);
}


/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
cBspModel_t *CM_LoadMap (char *name, qBool clientLoad, uInt *checksum) {
	uInt			*buf;
	dBspHeader_t	header;
	int				fileLen, i;

	flushmap	= Cvar_Get ("flushmap",		"0",		0);
	map_noareas = Cvar_Get ("map_noareas",	"0",		0);

	cmClientLoad = clientLoad;

	if (!strcmp (cmMapName, name) && (clientLoad || !flushmap->integer)) {
		*checksum = cmMapChecksum;
		if (!clientLoad) {
			memset (cmPortalOpen, 0, sizeof (cmPortalOpen));
			FloodAreaConnections ();
		}
		return &cmMapCModels[0];		// still have the right version
	}

	//
	// free old stuff
	//
	CM_UnloadMap ();

	if (!name || !name[0]) {
		CM_UnloadMap ();
		*checksum = cmMapChecksum;
		return &cmMapCModels[0];		// cinematic servers won't have anything at all
	}

	//
	// load the file
	//
	fileLen = FS_LoadFile (name, (void **)&buf);
	if (fileLen == -1)
		Com_Error (ERR_DROP, "CM_LoadMap: Couldn't find %s", name);
	if (!buf)
		Com_Error (ERR_DROP, "CM_LoadMap: Couldn't load %s", name);

	cmMapChecksum = LittleLong (Com_BlockChecksum (buf, fileLen));
	*checksum = cmMapChecksum;

	header = *(dBspHeader_t *)buf;
	for (i=0 ; i<sizeof (dBspHeader_t)/4 ; i++)
		((int *)&header)[i] = LittleLong (((int *)&header)[i]);

	if (header.version != BSP_VERSION)
		Com_Error (ERR_DROP, "CM_LoadMap: %s has wrong version number (%i should be %i)", name, header.version, BSP_VERSION);

	cmbase = (byte *)buf;

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

	memset (cmPortalOpen, 0, sizeof (cmPortalOpen));
	FloodAreaConnections ();

	Q_strncpyz (cmMapName, name, sizeof (cmMapName));

	return &cmMapCModels[0];
}


/*
==================
CM_UnloadMap
==================
*/
void CM_UnloadMap (void) {
	memset (cmMapEntityString, 0, BSP_MAX_ENTSTRING);
	memset (cmMapName, 0, MAX_QPATH);

	cmNumEntityChars = 0;
	cmMapChecksum = 0;

	cmNumAreaPortals = 0;
	cmNumAreas = 1;
	cmNumBrushes = 0;
	cmNumBrushSides = 0;
	cmNumClusters = 1;
	cmNumCModels = 0;
	cmNumLeafs = 1;
	cmNumLeafBrushes = 0;
	cmNumNodes = 0;
	cmNumPlanes = 0;
	cmNumTexInfo = 0;
	cmNumVisibility = 0;

	cmTraces = 0;
	cmBrushTraces = 0;
	cmPointContents = 0;
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
	if ((num < 1) || (num >= cmNumCModels))
		Com_Error (ERR_DROP, "CM_InlineModel: bad number");

	return &cmMapCModels[num];
}

// ==========================================================================

qBool CM_ClientLoad (void)		{ return cmClientLoad; }

char *CM_EntityString (void)	{ return cmMapEntityString; }
char *CM_SurfRName (int texNum)	{ return cmMapSurfaces[texNum].rname; }

int CM_NumClusters (void)		{ return cmNumClusters; }
int CM_NumInlineModels (void)	{ return cmNumCModels; }
int CM_NumTexInfo (void)		{ return cmNumTexInfo; }

// ==========================================================================

/*
==================
CM_LeafArea
==================
*/
int CM_LeafArea (int leafNum) {
	if ((leafNum < 0) || (leafNum >= cmNumLeafs))
		Com_Error (ERR_DROP, "CM_LeafArea: bad number");

	return cmMapLeafs[leafNum].area;
}


/*
==================
CM_LeafCluster
==================
*/
int CM_LeafCluster (int leafNum) {
	if ((leafNum < 0) || (leafNum >= cmNumLeafs))
		Com_Error (ERR_DROP, "CM_LeafCluster: bad number");

	return cmMapLeafs[leafNum].cluster;
}


/*
==================
CM_LeafContents
==================
*/
int CM_LeafContents (int leafNum) {
	if ((leafNum < 0) || (leafNum >= cmNumLeafs))
		Com_Error (ERR_DROP, "CM_LeafContents: bad number");

	return cmMapLeafs[leafNum].contents;
}

// ==========================================================================

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

	boxHeadNode = cmNumNodes;
	boxPlanes = &cmMapPlanes[cmNumPlanes];
	if ((cmNumNodes+6 > BSP_MAX_NODES)
		|| (cmNumBrushes+1 > BSP_MAX_BRUSHES)
		|| (cmNumLeafBrushes+1 > BSP_MAX_LEAFBRUSHES)
		|| (cmNumBrushSides+6 > BSP_MAX_BRUSHSIDES)
		|| (cmNumPlanes+12 > BSP_MAX_PLANES))
		Com_Error (ERR_DROP, "CM_InitBoxHull: Not enough room for box tree");

	boxBrush = &cmMapBrushes[cmNumBrushes];
	boxBrush->numSides = 6;
	boxBrush->firstBrushSide = cmNumBrushSides;
	boxBrush->contents = CONTENTS_MONSTER;

	boxLeaf = &cmMapLeafs[cmNumLeafs];
	boxLeaf->contents = CONTENTS_MONSTER;
	boxLeaf->firstLeafBrush = cmNumLeafBrushes;
	boxLeaf->numLeafBrushes = 1;

	cmMapLeafBrushes[cmNumLeafBrushes] = cmNumBrushes;

	for (i=0 ; i<6 ; i++) {
		side = i&1;

		// brush sides
		s = &cmMapBrushSides[cmNumBrushSides+i];
		s->plane = cmMapPlanes + (cmNumPlanes+i*2+side);
		s->surface = &cmNullSurface;

		// nodes
		c = &cmMapNodes[boxHeadNode+i];
		c->plane = cmMapPlanes + (cmNumPlanes+i*2);
		c->children[side] = -1 - cmEmptyLeaf;
		if (i != 5)
			c->children[side^1] = boxHeadNode+i + 1;
		else
			c->children[side^1] = -1 - cmNumLeafs;

		// planes
		p = &boxPlanes[i*2];
		p->type = i>>1;
		p->signBits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = 1;

		p = &boxPlanes[i*2+1];
		p->type = 3 + (i>>1);
		p->signBits = 0;
		VectorClear (p->normal);
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
		node = cmMapNodes + num;
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

	cmPointContents++;	// optimize counter

	return -1 - num;
}


/*
==================
CM_PointLeafnum
==================
*/
int CM_PointLeafnum (vec3_t p) {
	if (!cmNumPlanes)
		return 0;	// sound may call this without map loaded
	return CM_PointLeafnum_r (p, 0);
}


/*
=============
CM_BoxLeafnums

Fills in a list of all the leafs touched
=============
*/
static int		leafCount, leafMaxCount;
static int		*leafList;
static float	*leafMins, *leafMaxs;
static int		leafTopNode;

void CM_BoxLeafnums_r (int nodeNum) {
	cBspPlane_t	*plane;
	cBspNode_t	*node;
	int		s;

	for ( ; ; ) {
		if (nodeNum < 0) {
			if (leafCount >= leafMaxCount)
				return;

			leafList[leafCount++] = -1 - nodeNum;
			return;
		}
	
		node = &cmMapNodes[nodeNum];
		plane = node->plane;
		s = BOX_ON_PLANE_SIDE (leafMins, leafMaxs, plane);
		if (s == 1)
			nodeNum = node->children[0];
		else if (s == 2)
			nodeNum = node->children[1];
		else {
			// go down both
			if (leafTopNode == -1)
				leafTopNode = nodeNum;
			CM_BoxLeafnums_r (node->children[0]);
			nodeNum = node->children[1];
		}
	}
}

int	CM_BoxLeafnumsHeadNode (vec3_t mins, vec3_t maxs, int *list, int listSize, int headNode, int *topNode) {
	leafList = list;
	leafCount = 0;
	leafMaxCount = listSize;
	leafMins = mins;
	leafMaxs = maxs;
	leafTopNode = -1;

	CM_BoxLeafnums_r (headNode);

	if (topNode)
		*topNode = leafTopNode;

	return leafCount;
}


int	CM_BoxLeafnums (vec3_t mins, vec3_t maxs, int *list, int listSize, int *topNode) {
	return CM_BoxLeafnumsHeadNode (mins, maxs, list, listSize, cmMapCModels[0].headNode, topNode);
}


/*
==================
CM_PointContents
==================
*/
int CM_PointContents (vec3_t p, int headNode) {
	int		l;

	if (!cmNumNodes)	// map not loaded
		return 0;

	l = CM_PointLeafnum_r (p, headNode);

	return cmMapLeafs[l].contents;
}


/*
==================
CM_TransformedPointContents

Handles offseting and rotation of the end points for moving and rotating entities
==================
*/
int	CM_TransformedPointContents (vec3_t p, int headNode, vec3_t origin, vec3_t angles) {
	vec3_t	dist;
	vec3_t	temp;
	vec3_t	forward, right, up;
	int		l;

	// subtract origin offset
	VectorSubtract (p, origin, dist);

	// rotate start and end into the models frame of reference
	if (headNode != boxHeadNode && (angles[0] || angles[1] || angles[2])) {
		AngleVectors (angles, forward, right, up);

		VectorCopy (dist, temp);
		dist[0] = DotProduct (temp, forward);
		dist[1] = -DotProduct (temp, right);
		dist[2] = DotProduct (temp, up);
	}

	l = CM_PointLeafnum_r (dist, headNode);

	return cmMapLeafs[l].contents;
}

/*
=============================================================================

	BOX TRACING

=============================================================================
*/

// 1/32 epsilon to keep floating point happy
#define	DIST_EPSILON	(0.03125)

vec3_t	traceStart, traceEnd;
vec3_t	traceMins, traceMaxs;
vec3_t	traceExtents;

trace_t	traceTrace;
int		traceContents;
qBool	traceIsPoint;		// optimized case

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

	cmBrushTraces++;

	getout = qFalse;
	startout = qFalse;
	leadside = NULL;

	for (i=0 ; i<brush->numSides ; i++) 	{
		side = &cmMapBrushSides[brush->firstBrushSide+i];
		plane = side->plane;

		// FIXME: special case for axial
		if (!traceIsPoint) {
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
		}
		else {
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
		}
		else {
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
		side = &cmMapBrushSides[brush->firstBrushSide+i];
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
void CM_TraceToLeaf (int leafNum) {
	int			k;
	int			brushnum;
	cBspLeaf_t	*leaf;
	cBspBrush_t	*b;

	leaf = &cmMapLeafs[leafNum];
	if (!(leaf->contents & traceContents))
		return;

	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numLeafBrushes ; k++) {
		brushnum = cmMapLeafBrushes[leaf->firstLeafBrush+k];
		b = &cmMapBrushes[brushnum];
		if (b->checkCount == cmCheckCount)
			continue;	// already checked this brush in another leaf

		b->checkCount = cmCheckCount;

		if (!(b->contents & traceContents))
			continue;

		CM_ClipBoxToBrush (traceMins, traceMaxs, traceStart, traceEnd, &traceTrace, b);
		if (!traceTrace.fraction)
			return;
	}
}


/*
================
CM_TestInLeaf
================
*/
void CM_TestInLeaf (int leafNum) {
	int			k;
	int			brushnum;
	cBspLeaf_t	*leaf;
	cBspBrush_t	*b;

	leaf = &cmMapLeafs[leafNum];
	if (!(leaf->contents & traceContents))
		return;

	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numLeafBrushes ; k++) {
		brushnum = cmMapLeafBrushes[leaf->firstLeafBrush+k];
		b = &cmMapBrushes[brushnum];
		if (b->checkCount == cmCheckCount)
			continue;	// already checked this brush in another leaf

		b->checkCount = cmCheckCount;

		if (!(b->contents & traceContents))
			continue;

		CM_TestBoxInBrush (traceMins, traceMaxs, traceStart, &traceTrace, b);
		if (!traceTrace.fraction)
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

	if (traceTrace.fraction <= p1f)
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
	node = cmMapNodes + num;
	plane = node->plane;

	if (plane->type < 3) {
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
		offset = traceExtents[plane->type];
	}
	else {
		t1 = DotProduct (plane->normal, p1) - plane->dist;
		t2 = DotProduct (plane->normal, p2) - plane->dist;
		if (traceIsPoint)
			offset = 0;
		else
			offset = fabs (traceExtents[0]*plane->normal[0]) +
				fabs (traceExtents[1]*plane->normal[1]) +
				fabs (traceExtents[2]*plane->normal[2]);
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
	}
	else if (t1 > t2) {
		idist = 1.0/(t1-t2);
		side = 0;
		frac2 = (t1 - offset - DIST_EPSILON)*idist;
		frac = (t1 + offset + DIST_EPSILON)*idist;
	}
	else {
		side = 0;
		frac = 1;
		frac2 = 0;
	}

	// move up to the node
	frac = clamp (frac, 0, 1);
	midf = p1f + (p2f - p1f ) * frac;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac * (p2[i] - p1[i]);

	CM_RecursiveHullCheck (node->children[side], p1f, midf, p1, mid);

	// go past the node
	frac2 = clamp (frac2, 0, 1);
	midf = p1f + (p2f - p1f) * frac2;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac2 * (p2[i] - p1[i]);

	CM_RecursiveHullCheck (node->children[side^1], midf, p2f, mid, p2);
}

// ==========================================================================

/*
====================
CM_Trace
====================
*/
trace_t CM_Trace (vec3_t start, vec3_t end, float size, int contentMask) {
	vec3_t maxs, mins;

	VectorSet (maxs, size, size, size);
	VectorSet (mins, -size, -size, -size);

	return CM_BoxTrace (start, end, mins, maxs, 0, contentMask);
}


/*
==================
CM_BoxTrace
==================
*/
trace_t CM_BoxTrace (vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int headNode, int brushMask) {
	int		i;

	cmCheckCount++;	// for multi-check avoidance
	cmTraces++;			// for statistics, may be zeroed

	// fill in a default trace
	memset (&traceTrace, 0, sizeof (traceTrace));
	traceTrace.fraction = 1;
	traceTrace.surface = &(cmNullSurface.c);

	if (!cmNumNodes)	// map not loaded
		return traceTrace;

	traceContents = brushMask;
	VectorCopy (start, traceStart);
	VectorCopy (end, traceEnd);
	VectorCopy (mins, traceMins);
	VectorCopy (maxs, traceMaxs);

	// check for position test special case
	if (VectorCompare (start, end)) {
		int		leafs[1024];
		int		i, numLeafs;
		vec3_t	c1, c2;
		int		topNode;

		VectorAdd (start, mins, c1);
		VectorAdd (start, maxs, c2);
		for (i=0 ; i<3 ; i++) {
			c1[i] -= 1;
			c2[i] += 1;
		}

		numLeafs = CM_BoxLeafnumsHeadNode (c1, c2, leafs, 1024, headNode, &topNode);
		for (i=0 ; i<numLeafs ; i++) {
			CM_TestInLeaf (leafs[i]);
			if (traceTrace.allSolid)
				break;
		}
		VectorCopy (start, traceTrace.endPos);
		return traceTrace;
	}

	// check for point special case
	if (VectorCompare (mins, vec3Origin) && VectorCompare (maxs, vec3Origin)) {
		traceIsPoint = qTrue;
		VectorClear (traceExtents);
	}
	else {
		traceIsPoint = qFalse;
		traceExtents[0] = -mins[0] > maxs[0] ? -mins[0] : maxs[0];
		traceExtents[1] = -mins[1] > maxs[1] ? -mins[1] : maxs[1];
		traceExtents[2] = -mins[2] > maxs[2] ? -mins[2] : maxs[2];
	}

	// general sweeping through world
	CM_RecursiveHullCheck (headNode, 0, 1, start, end);

	if (traceTrace.fraction == 1) {
		VectorCopy (end, traceTrace.endPos);
	}
	else {
		for (i=0 ; i<3 ; i++)
			traceTrace.endPos[i] = start[i] + traceTrace.fraction * (end[i] - start[i]);
	}
	return traceTrace;
}


/*
==================
CM_TransformedBoxTrace

Handles offseting and rotation of the end points for moving and rotating entities
==================
*/
#ifdef _WIN32
#pragma optimize ("", off)
#endif
trace_t CM_TransformedBoxTrace (vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs,
						  int headNode, int brushMask, vec3_t origin, vec3_t angles) {
	trace_t		trace;
	vec3_t		start_l, end_l;
	vec3_t		forward, right, up;
	vec3_t		temp, a;
	qBool		rotated;

	// subtract origin offset
	VectorSubtract (start, origin, start_l);
	VectorSubtract (end, origin, end_l);

	// rotate start and end into the models frame of reference
	if ((headNode != boxHeadNode) && (angles[0] || angles[1] || angles[2]))
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
	trace = CM_BoxTrace (start_l, end_l, mins, maxs, headNode, brushMask);

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
=============================================================================

	PVS / PHS

=============================================================================
*/

/*
===================
CM_DecompressVis
===================
*/
void CM_DecompressVis (byte *in, byte *out) {
	int		c;
	byte	*outPtr;
	int		row;

	row = (cmNumClusters + 7) >> 3;	
	outPtr = out;

	if (!in || !cmNumVisibility) {
		// no vis info, so make all visible
		while (row) {
			*outPtr++ = 0xff;
			row--;
		}
		return;		
	}

	do {
		if (*in) {
			*outPtr++ = *in++;
			continue;
		}
	
		c = in[1];
		in += 2;
		if ((outPtr - out) + c > row) {
			c = row - (outPtr - out);
			Com_Printf (PRNT_DEV, S_COLOR_YELLOW "Warning: Vis decompression overrun\n");
		}

		while (c) {
			*outPtr++ = 0;
			c--;
		}
	} while (outPtr - out < row);
}


/*
===================
CM_ClusterPVS
===================
*/
byte *CM_ClusterPVS (int cluster) {
	static byte		pvsRow[BSP_MAX_VIS];

	if (cluster == -1)
		memset (pvsRow, 0, (cmNumClusters + 7) >> 3);
	else
		CM_DecompressVis (cmMapVisibility + cmMapVis->bitOfs[cluster][BSP_VIS_PVS], pvsRow);
	return pvsRow;
}


/*
===================
CM_ClusterPHS
===================
*/
byte *CM_ClusterPHS (int cluster) {
	static byte		phsRow[BSP_MAX_VIS];

	if (cluster == -1)
		memset (phsRow, 0, (cmNumClusters + 7) >> 3);
	else
		CM_DecompressVis (cmMapVisibility + cmMapVis->bitOfs[cluster][BSP_VIS_PHS], phsRow);
	return phsRow;
}

/*
=============================================================================

	AREAPORTALS

=============================================================================
*/

/*
====================
FloodAreaConnections
====================
*/
void FloodArea_r (cBspArea_t *area, int floodNum) {
	dBspAreaPortal_t	*p;
	int					i;

	if (area->floodValid == cmFloodValid) {
		if (area->floodNum == floodNum)
			return;
		Com_Error (ERR_DROP, "FloodArea_r: reflooded");
	}

	area->floodNum = floodNum;
	area->floodValid = cmFloodValid;
	p = &cmMapAreaPortals[area->firstAreaPortal];
	for (i=0 ; i<area->numAreaPortals ; i++, p++) {
		if (cmPortalOpen[p->portalNum])
			FloodArea_r (&cmMapAreas[p->otherArea], floodNum);
	}
}
void FloodAreaConnections (void) {
	int			i;
	cBspArea_t	*area;
	int			floodNum;

	// all current floods are now invalid
	cmFloodValid++;
	floodNum = 0;

	// area 0 is not used
	for (i=1 ; i<cmNumAreas ; i++) {
		area = &cmMapAreas[i];
		if (area->floodValid == cmFloodValid)
			continue;		// already flooded into
		floodNum++;
		FloodArea_r (area, floodNum);
	}

}

void CM_SetAreaPortalState (int portalnum, qBool open) {
	if (portalnum > cmNumAreaPortals)
		Com_Error (ERR_DROP, "CM_SetAreaPortalState: areaportal > numAreaPortals");

	cmPortalOpen[portalnum] = open;
	FloodAreaConnections ();
}

qBool CM_AreasConnected (int area1, int area2) {
	if (map_noareas->value)
		return qTrue;

	if ((area1 > cmNumAreas) || (area2 > cmNumAreas))
		Com_Error (ERR_DROP, "CM_AreasConnected: area > cmNumAreas");

	if (cmMapAreas[area1].floodNum == cmMapAreas[area2].floodNum)
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

	bytes = (cmNumAreas+7)>>3;

	if (map_noareas->value) {
		// for debugging, send everything
		memset (buffer, 255, bytes);
	}
	else {
		memset (buffer, 0, bytes);

		floodNum = cmMapAreas[area].floodNum;
		for (i=0 ; i<cmNumAreas ; i++) {
			if ((cmMapAreas[i].floodNum == floodNum) || !area)
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
	fwrite (cmPortalOpen, sizeof (cmPortalOpen), 1, f);
}


/*
===================
CM_ReadPortalState

Reads the portal state from a savegame file
and recalculates the area connections
===================
*/
void CM_ReadPortalState (FILE *f) {
	FS_Read (cmPortalOpen, sizeof (cmPortalOpen), f);
	FloodAreaConnections ();
}


/*
=============
CM_HeadnodeVisible

Returns qTrue if any leaf under headnode has a cluster that
is potentially visible
=============
*/
qBool CM_HeadnodeVisible (int nodeNum, byte *visbits) {
	int			leafNum;
	int			cluster;
	cBspNode_t	*node;

	if (nodeNum < 0) {
		leafNum = -1-nodeNum;
		cluster = cmMapLeafs[leafNum].cluster;
		if (cluster == -1)
			return qFalse;
		if (visbits[cluster>>3] & (1<<(cluster&7)))
			return qTrue;
		return qFalse;
	}

	node = &cmMapNodes[nodeNum];
	if (CM_HeadnodeVisible (node->children[0], visbits))
		return qTrue;
	return CM_HeadnodeVisible (node->children[1], visbits);
}
