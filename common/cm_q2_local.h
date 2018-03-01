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
	int				children[2];		// negative numbers are leafs
} cQ2BspNode_t;

typedef struct cQ2BspLeaf_s {
	int				contents;
	int				cluster;
	int				area;
	uint16			firstLeafBrush;
	uint16			numLeafBrushes;
} cQ2BspLeaf_t;

typedef struct cQ2BspBrushSide_s {
	cBspPlane_t		*plane;
	cBspSurface_t	*surface;
} cQ2BspBrushSide_t;

typedef struct cQ2BspBrush_s {
	int				contents;
	int				numSides;
	int				firstBrushSide;
	int				checkCount;			// to avoid repeated testings
} cQ2BspBrush_t;

typedef struct cQ2BspArea_s {
	int				numAreaPortals;
	int				firstAreaPortal;
	int				floodNum;			// if two areas have equal floodnums, they are connected
	int				floodValid;
} cQ2BspArea_t;

// ==========================================================================

extern int					cm_q2_numNodes;
extern cQ2BspNode_t			*cm_q2_nodes;

extern int					cm_q2_numBrushSides;
extern cQ2BspBrushSide_t	*cm_q2_brushSides;

extern int					cm_q2_numLeafs;
extern int					cm_q2_emptyLeaf;
extern cQ2BspLeaf_t			*cm_q2_leafs;

extern int					cm_q2_numLeafBrushes;
extern uint16				*cm_q2_leafBrushes;

extern int					cm_q2_numBrushes;
extern cQ2BspBrush_t		*cm_q2_brushes;

extern int					cm_q2_numAreas;
extern cQ2BspArea_t			*cm_q2_areas;

extern cBspSurface_t		cm_q2_nullSurface;

extern int					cm_q2_numPlanes;
extern cBspPlane_t			*cm_q2_planes;

extern int					cm_q2_numVisibility;
extern dQ2BspVis_t			*cm_q2_visData;

extern int					cm_q2_numAreaPortals;
extern dQ2BspAreaPortal_t	*cm_q2_areaPortals;
extern qBool				*cm_q2_portalOpen;

extern int					cm_q2_numClusters;

// ==========================================================================

void		CM_Q2BSP_InitBoxHull (void);
void		CM_Q2BSP_FloodAreaConnections (void);
