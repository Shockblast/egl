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
// cm_q2_local.h
// Quake2 BSP map model collision header
//

#include "cm_common.h"

// ==========================================================================

typedef struct cQ2BspNode_s {
	cBspPlane_t		*plane;
	int				children[2];	// negative numbers are leafs
} cQ2BspNode_t;

typedef struct cQ2BspLeaf_s {
	int				contents;
	int				cluster;
	int				area;
	uint16			firstLeafBrush;
	uint16			numLeafBrushes;
} cQ2BspLeaf_t;

typedef struct cQ2MapSurface_s {	// used internally due to name len probs - ZOID
	cBspSurface_t	c;
	char			rname[32];
} cQ2MapSurface_t;

typedef struct cQ2BspBrushSide_s {
	cBspPlane_t		*plane;
	cQ2MapSurface_t	*surface;
} cQ2BspBrushSide_t;

typedef struct cQ2BspBrush_s {
	int				contents;
	int				numSides;
	int				firstBrushSide;
	int				checkCount;		// to avoid repeated testings
} cQ2BspBrush_t;

typedef struct cQ2BspArea_s {
	int				numAreaPortals;
	int				firstAreaPortal;
	int				floodNum;			// if two areas have equal floodnums, they are connected
	int				floodValid;
} cQ2BspArea_t;

// ==========================================================================

extern int					cm_numNodes;
extern cQ2BspNode_t			cm_mapNodes[Q2BSP_MAX_NODES+6];		// extra for box hull

extern int					cm_numBrushSides;
extern cQ2BspBrushSide_t	cm_mapBrushSides[Q2BSP_MAX_BRUSHSIDES];

extern int					cm_numLeafs;
extern cQ2BspLeaf_t			cm_mapLeafs[Q2BSP_MAX_LEAFS];
extern int					cm_emptyLeaf;

extern int					cm_numLeafBrushes;
extern uint16				cm_mapLeafBrushes[Q2BSP_MAX_LEAFBRUSHES];

extern int					cm_numBrushes;
extern cQ2BspBrush_t		cm_mapBrushes[Q2BSP_MAX_BRUSHES];

extern int					cm_numAreas;
extern cQ2BspArea_t			cm_mapAreas[Q2BSP_MAX_AREAS];

extern cQ2MapSurface_t		cm_nullSurface;

extern int					cm_numPlanes;
extern cBspPlane_t			cm_mapPlanes[Q2BSP_MAX_PLANES+6];	// extra for box hull

extern cBspModel_t			cm_mapCModels[Q2BSP_MAX_MODELS];

extern int					cm_numVisibility;
extern byte					cm_mapVisibility[Q2BSP_MAX_VISIBILITY];
extern dQ2BspVis_t			*cm_mapVis;

extern int					cm_numAreaPortals;
extern dQ2BspAreaPortal_t	cm_mapAreaPortals[Q2BSP_MAX_AREAPORTALS];
extern qBool				cm_portalOpen[Q2BSP_MAX_AREAPORTALS];

extern int					cm_numClusters;

// ==========================================================================

void		CM_Q2BSP_InitBoxHull (void);
void		CM_Q2BSP_FloodAreaConnections (void);
