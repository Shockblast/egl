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
// Quake2 BSP map model tracing
//

#include "cm_q2_local.h"

static int				cm_checkCount;
static int				cm_floodValid;

static cBspPlane_t		*cm_boxPlanes;
static int				cm_boxHeadNode;
static cQ2BspBrush_t	*cm_boxBrush;
static cQ2BspLeaf_t		*cm_boxLeaf;

static int				cm_leafCount, cm_leafMaxCount;
static int				*cm_leafList;
static float			*cm_leafMins, *cm_leafMaxs;
static int				cm_leafTopNode;

// 1/32 epsilon to keep floating point happy
#define DIST_EPSILON	(0.03125)

static vec3_t			cm_traceStart, cm_traceEnd;
static vec3_t			cm_traceMins, cm_traceMaxs;
static vec3_t			cm_traceExtents;

static trace_t			cm_currentTrace;
static int				cm_traceContents;
static qBool			cm_traceIsPoint;		// optimized case

/*
=============================================================================

	TRACING

=============================================================================
*/

/*
==================
CM_Q2BSP_LeafArea
==================
*/
int CM_Q2BSP_LeafArea (int leafNum)
{
	if (leafNum < 0 || leafNum >= cm_numLeafs)
		Com_Error (ERR_DROP, "CM_LeafArea: bad number");

	return cm_mapLeafs[leafNum].area;
}


/*
==================
CM_Q2BSP_LeafCluster
==================
*/
int CM_Q2BSP_LeafCluster (int leafNum)
{
	if (leafNum < 0 || leafNum >= cm_numLeafs)
		Com_Error (ERR_DROP, "CM_LeafCluster: bad number");

	return cm_mapLeafs[leafNum].cluster;
}


/*
==================
CM_Q2BSP_LeafContents
==================
*/
int CM_Q2BSP_LeafContents (int leafNum)
{
	if (leafNum < 0 || leafNum >= cm_numLeafs)
		Com_Error (ERR_DROP, "CM_LeafContents: bad number");

	return cm_mapLeafs[leafNum].contents;
}

// ==========================================================================

/*
===================
CM_Q2BSP_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
void CM_Q2BSP_InitBoxHull (void)
{
	int					i;
	int					side;
	cQ2BspNode_t		*c;
	cBspPlane_t			*p;
	cQ2BspBrushSide_t	*s;

	cm_boxHeadNode = cm_numNodes;
	cm_boxPlanes = &cm_mapPlanes[cm_numPlanes];
	if ((cm_numNodes+6 > Q2BSP_MAX_NODES)
		|| (cm_numBrushes+1 > Q2BSP_MAX_BRUSHES)
		|| (cm_numLeafBrushes+1 > Q2BSP_MAX_LEAFBRUSHES)
		|| (cm_numBrushSides+6 > Q2BSP_MAX_BRUSHSIDES)
		|| (cm_numPlanes+12 > Q2BSP_MAX_PLANES))
		Com_Error (ERR_DROP, "CM_Q2BSP_InitBoxHull: Not enough room for box tree");

	cm_boxBrush = &cm_mapBrushes[cm_numBrushes];
	cm_boxBrush->numSides = 6;
	cm_boxBrush->firstBrushSide = cm_numBrushSides;
	cm_boxBrush->contents = CONTENTS_MONSTER;

	cm_boxLeaf = &cm_mapLeafs[cm_numLeafs];
	cm_boxLeaf->contents = CONTENTS_MONSTER;
	cm_boxLeaf->firstLeafBrush = cm_numLeafBrushes;
	cm_boxLeaf->numLeafBrushes = 1;

	cm_mapLeafBrushes[cm_numLeafBrushes] = cm_numBrushes;

	for (i=0 ; i<6 ; i++) {
		side = i&1;

		// brush sides
		s = &cm_mapBrushSides[cm_numBrushSides+i];
		s->plane = cm_mapPlanes + (cm_numPlanes+i*2+side);
		s->surface = &cm_nullSurface;

		// nodes
		c = &cm_mapNodes[cm_boxHeadNode+i];
		c->plane = cm_mapPlanes + (cm_numPlanes+i*2);
		c->children[side] = -1 - cm_emptyLeaf;
		if (i != 5)
			c->children[side^1] = cm_boxHeadNode+i + 1;
		else
			c->children[side^1] = -1 - cm_numLeafs;

		// planes
		p = &cm_boxPlanes[i*2];
		p->type = i>>1;
		p->signBits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = 1;

		p = &cm_boxPlanes[i*2+1];
		p->type = 3 + (i>>1);
		p->signBits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = -1;
	}	
}


/*
===================
CM_Q2BSP_HeadnodeForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*/
int	CM_Q2BSP_HeadnodeForBox (vec3_t mins, vec3_t maxs)
{
	cm_boxPlanes[0].dist = maxs[0];
	cm_boxPlanes[1].dist = -maxs[0];
	cm_boxPlanes[2].dist = mins[0];
	cm_boxPlanes[3].dist = -mins[0];
	cm_boxPlanes[4].dist = maxs[1];
	cm_boxPlanes[5].dist = -maxs[1];
	cm_boxPlanes[6].dist = mins[1];
	cm_boxPlanes[7].dist = -mins[1];
	cm_boxPlanes[8].dist = maxs[2];
	cm_boxPlanes[9].dist = -maxs[2];
	cm_boxPlanes[10].dist = mins[2];
	cm_boxPlanes[11].dist = -mins[2];

	return cm_boxHeadNode;
}


/*
==================
CM_Q2BSP_PointLeafnum_r
==================
*/
static int CM_Q2BSP_PointLeafnum_r (vec3_t p, int num)
{
	float			d;
	cQ2BspNode_t	*node;
	cBspPlane_t		*plane;

	while (num >= 0) {
		node = cm_mapNodes + num;
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

	cm_numPointContents++;	// optimize counter

	return -1 - num;
}


/*
==================
CM_Q2BSP_PointLeafnum
==================
*/
int CM_Q2BSP_PointLeafnum (vec3_t p)
{
	if (!cm_numPlanes)
		return 0;	// sound may call this without map loaded
	return CM_Q2BSP_PointLeafnum_r (p, 0);
}


/*
=============
CM_Q2BSP_BoxLeafnums

Fills in a list of all the leafs touched
=============
*/
static void CM_Q2BSP_BoxLeafnums_r (int nodeNum)
{
	cBspPlane_t		*plane;
	cQ2BspNode_t	*node;
	int				s;

	for ( ; ; ) {
		if (nodeNum < 0) {
			if (cm_leafCount >= cm_leafMaxCount)
				return;

			cm_leafList[cm_leafCount++] = -1 - nodeNum;
			return;
		}
	
		node = &cm_mapNodes[nodeNum];
		plane = node->plane;
		s = BOX_ON_PLANE_SIDE (cm_leafMins, cm_leafMaxs, plane);
		if (s == 1)
			nodeNum = node->children[0];
		else if (s == 2)
			nodeNum = node->children[1];
		else {
			// go down both
			if (cm_leafTopNode == -1)
				cm_leafTopNode = nodeNum;
			CM_Q2BSP_BoxLeafnums_r (node->children[0]);
			nodeNum = node->children[1];
		}
	}
}


/*
==================
CM_Q2BSP_BoxLeafnumsHeadNode
==================
*/
static int CM_Q2BSP_BoxLeafnumsHeadNode (vec3_t mins, vec3_t maxs, int *list, int listSize, int headNode, int *topNode)
{
	cm_leafList = list;
	cm_leafCount = 0;
	cm_leafMaxCount = listSize;
	cm_leafMins = mins;
	cm_leafMaxs = maxs;
	cm_leafTopNode = -1;

	CM_Q2BSP_BoxLeafnums_r (headNode);

	if (topNode)
		*topNode = cm_leafTopNode;

	return cm_leafCount;
}


/*
==================
CM_Q2BSP_BoxLeafnums
==================
*/
int	CM_Q2BSP_BoxLeafnums (vec3_t mins, vec3_t maxs, int *list, int listSize, int *topNode)
{
	return CM_Q2BSP_BoxLeafnumsHeadNode (mins, maxs, list, listSize, cm_mapCModels[0].headNode, topNode);
}


/*
==================
CM_Q2BSP_PointContents
==================
*/
int CM_Q2BSP_PointContents (vec3_t p, int headNode)
{
	int		l;

	if (!cm_numNodes)	// map not loaded
		return 0;

	l = CM_Q2BSP_PointLeafnum_r (p, headNode);

	return cm_mapLeafs[l].contents;
}


/*
==================
CM_Q2BSP_TransformedPointContents

Handles offseting and rotation of the end points for moving and rotating entities
==================
*/
int	CM_Q2BSP_TransformedPointContents (vec3_t p, int headNode, vec3_t origin, vec3_t angles)
{
	vec3_t	dist;
	vec3_t	temp;
	vec3_t	forward, right, up;
	int		l;

	// subtract origin offset
	VectorSubtract (p, origin, dist);

	// rotate start and end into the models frame of reference
	if (headNode != cm_boxHeadNode && (angles[0] || angles[1] || angles[2])) {
		Angles_Vectors (angles, forward, right, up);

		VectorCopy (dist, temp);
		dist[0] = DotProduct (temp, forward);
		dist[1] = -DotProduct (temp, right);
		dist[2] = DotProduct (temp, up);
	}

	l = CM_Q2BSP_PointLeafnum_r (dist, headNode);

	return cm_mapLeafs[l].contents;
}

/*
=============================================================================

	BOX TRACING

=============================================================================
*/

/*
================
CM_Q2BSP_ClipBoxToBrush
================
*/
static void CM_Q2BSP_ClipBoxToBrush (vec3_t mins, vec3_t maxs, vec3_t p1, vec3_t p2, trace_t *trace, cQ2BspBrush_t *brush)
{
	int					i, j;
	cBspPlane_t			*plane, *clipplane;
	float				dist, dot1, dot2, f;
	float				enterfrac, leavefrac;
	vec3_t				ofs;
	qBool				getout, startout;
	cQ2BspBrushSide_t	*side, *leadside;

	enterfrac = -1;
	leavefrac = 1;
	clipplane = NULL;

	if (!brush->numSides)
		return;

	cm_numBrushTraces++;

	getout = qFalse;
	startout = qFalse;
	leadside = NULL;

	for (i=0 ; i<brush->numSides ; i++) 	{
		side = &cm_mapBrushSides[brush->firstBrushSide+i];
		plane = side->plane;

		// FIXME: special case for axial
		if (!cm_traceIsPoint) {
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
		if (dot1 > 0 && dot2 >= dot1)
			return;

		if (dot1 <= 0 && dot2 <= 0)
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
CM_Q2BSP_TestBoxInBrush
================
*/
static void CM_Q2BSP_TestBoxInBrush (vec3_t mins, vec3_t maxs, vec3_t p1, trace_t *trace, cQ2BspBrush_t *brush)
{
	int					i, j;
	vec3_t				ofs;
	cBspPlane_t			*plane;
	cQ2BspBrushSide_t	*side;
	float				dist, dot;

	if (!brush->numSides)
		return;

	for (i=0 ; i<brush->numSides ; i++) {
		side = &cm_mapBrushSides[brush->firstBrushSide+i];
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
CM_Q2BSP_TraceToLeaf
================
*/
static void CM_Q2BSP_TraceToLeaf (int leafNum)
{
	cQ2BspLeaf_t	*leaf;
	cQ2BspBrush_t	*b;
	int				brushNum;
	int				k;

	leaf = &cm_mapLeafs[leafNum];
	if (!(leaf->contents & cm_traceContents))
		return;

	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numLeafBrushes ; k++) {
		brushNum = cm_mapLeafBrushes[leaf->firstLeafBrush+k];
		b = &cm_mapBrushes[brushNum];
		if (b->checkCount == cm_checkCount)
			continue;	// already checked this brush in another leaf

		b->checkCount = cm_checkCount;

		if (!(b->contents & cm_traceContents))
			continue;

		CM_Q2BSP_ClipBoxToBrush (cm_traceMins, cm_traceMaxs, cm_traceStart, cm_traceEnd, &cm_currentTrace, b);
		if (!cm_currentTrace.fraction)
			return;
	}
}


/*
================
CM_Q2BSP_TestInLeaf
================
*/
static void CM_Q2BSP_TestInLeaf (int leafNum)
{
	cQ2BspLeaf_t	*leaf;
	cQ2BspBrush_t	*b;
	int				brushNum;
	int				k;

	leaf = &cm_mapLeafs[leafNum];
	if (!(leaf->contents & cm_traceContents))
		return;

	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numLeafBrushes ; k++) {
		brushNum = cm_mapLeafBrushes[leaf->firstLeafBrush+k];
		b = &cm_mapBrushes[brushNum];
		if (b->checkCount == cm_checkCount)
			continue;	// already checked this brush in another leaf

		b->checkCount = cm_checkCount;

		if (!(b->contents & cm_traceContents))
			continue;

		CM_Q2BSP_TestBoxInBrush (cm_traceMins, cm_traceMaxs, cm_traceStart, &cm_currentTrace, b);
		if (!cm_currentTrace.fraction)
			return;
	}
}


/*
==================
CM_Q2BSP_RecursiveHullCheck
==================
*/
static void CM_Q2BSP_RecursiveHullCheck (int num, float p1f, float p2f, vec3_t p1, vec3_t p2)
{
	cQ2BspNode_t	*node;
	cBspPlane_t		*plane;
	float			t1, t2, offset;
	float			frac, frac2;
	float			idist;
	int				i, side;
	vec3_t			mid;
	float			midf;

	if (cm_currentTrace.fraction <= p1f)
		return;		// already hit something nearer

	// if < 0, we are in a leaf node
	if (num < 0) {
		CM_Q2BSP_TraceToLeaf (-1-num);
		return;
	}

	/*
	** find the point distances to the seperating plane
	** and the offset for the size of the box
	*/
	node = cm_mapNodes + num;
	plane = node->plane;

	if (plane->type < 3) {
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
		offset = cm_traceExtents[plane->type];
	}
	else {
		t1 = DotProduct (plane->normal, p1) - plane->dist;
		t2 = DotProduct (plane->normal, p2) - plane->dist;
		if (cm_traceIsPoint)
			offset = 0;
		else
			offset = fabs (cm_traceExtents[0]*plane->normal[0]) +
				fabs (cm_traceExtents[1]*plane->normal[1]) +
				fabs (cm_traceExtents[2]*plane->normal[2]);
	}

	// see which sides we need to consider
	if (t1 >= offset && t2 >= offset) {
		CM_Q2BSP_RecursiveHullCheck (node->children[0], p1f, p2f, p1, p2);
		return;
	}
	if (t1 < -offset && t2 < -offset) {
		CM_Q2BSP_RecursiveHullCheck (node->children[1], p1f, p2f, p1, p2);
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

	CM_Q2BSP_RecursiveHullCheck (node->children[side], p1f, midf, p1, mid);

	// go past the node
	frac2 = clamp (frac2, 0, 1);
	midf = p1f + (p2f - p1f) * frac2;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac2 * (p2[i] - p1[i]);

	CM_Q2BSP_RecursiveHullCheck (node->children[side^1], midf, p2f, mid, p2);
}

// ==========================================================================

/*
====================
CM_Q2BSP_Trace
====================
*/
trace_t CM_Q2BSP_Trace (vec3_t start, vec3_t end, float size, int contentMask)
{
	vec3_t maxs, mins;

	VectorSet (maxs, size, size, size);
	VectorSet (mins, -size, -size, -size);

	return CM_Q2BSP_BoxTrace (start, end, mins, maxs, 0, contentMask);
}


/*
==================
CM_Q2BSP_BoxTrace
==================
*/
trace_t CM_Q2BSP_BoxTrace (vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int headNode, int brushMask)
{
	int		i;

	cm_checkCount++;	// for multi-check avoidance
	cm_numTraces++;		// for statistics, may be zeroed

	// fill in a default trace
	memset (&cm_currentTrace, 0, sizeof (cm_currentTrace));
	cm_currentTrace.fraction = 1;
	cm_currentTrace.surface = &(cm_nullSurface.c);

	if (!cm_numNodes)	// map not loaded
		return cm_currentTrace;

	cm_traceContents = brushMask;
	VectorCopy (start, cm_traceStart);
	VectorCopy (end, cm_traceEnd);
	VectorCopy (mins, cm_traceMins);
	VectorCopy (maxs, cm_traceMaxs);

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

		numLeafs = CM_Q2BSP_BoxLeafnumsHeadNode (c1, c2, leafs, 1024, headNode, &topNode);
		for (i=0 ; i<numLeafs ; i++) {
			CM_Q2BSP_TestInLeaf (leafs[i]);
			if (cm_currentTrace.allSolid)
				break;
		}
		VectorCopy (start, cm_currentTrace.endPos);
		return cm_currentTrace;
	}

	// check for point special case
	if (VectorCompare (mins, vec3Origin) && VectorCompare (maxs, vec3Origin)) {
		cm_traceIsPoint = qTrue;
		VectorClear (cm_traceExtents);
	}
	else {
		cm_traceIsPoint = qFalse;
		cm_traceExtents[0] = -mins[0] > maxs[0] ? -mins[0] : maxs[0];
		cm_traceExtents[1] = -mins[1] > maxs[1] ? -mins[1] : maxs[1];
		cm_traceExtents[2] = -mins[2] > maxs[2] ? -mins[2] : maxs[2];
	}

	// general sweeping through world
	CM_Q2BSP_RecursiveHullCheck (headNode, 0, 1, start, end);

	if (cm_currentTrace.fraction == 1) {
		VectorCopy (end, cm_currentTrace.endPos);
	}
	else {
		for (i=0 ; i<3 ; i++)
			cm_currentTrace.endPos[i] = start[i] + cm_currentTrace.fraction * (end[i] - start[i]);
	}

	return cm_currentTrace;
}


/*
==================
CM_Q2BSP_TransformedBoxTrace

Handles offseting and rotation of the end points for moving and rotating entities
==================
*/
#ifdef WIN32
#pragma optimize ("", off)
#endif
trace_t CM_Q2BSP_TransformedBoxTrace (vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs,
						  int headNode, int brushMask, vec3_t origin, vec3_t angles)
{
	trace_t		trace;
	vec3_t		start_l, end_l;
	vec3_t		forward, right, up;
	vec3_t		temp, a;
	qBool		rotated;

	// subtract origin offset
	VectorSubtract (start, origin, start_l);
	VectorSubtract (end, origin, end_l);

	// rotate start and end into the models frame of reference
	if (headNode != cm_boxHeadNode && (angles[0] || angles[1] || angles[2]))
		rotated = qTrue;
	else
		rotated = qFalse;

	if (rotated) {
		Angles_Vectors (angles, forward, right, up);

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
	trace = CM_Q2BSP_BoxTrace (start_l, end_l, mins, maxs, headNode, brushMask);

	if (rotated && trace.fraction != 1.0) {
		// FIXME: figure out how to do this with existing angles
		VectorNegate (angles, a);
		Angles_Vectors (a, forward, right, up);

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
#ifdef WIN32
#pragma optimize ("", on)
#endif

/*
=============================================================================

	PVS / PHS

=============================================================================
*/

/*
===================
CM_Q2BSP_DecompressVis
===================
*/
static void CM_Q2BSP_DecompressVis (byte *in, byte *out)
{
	int		c;
	byte	*outPtr;
	int		row;

	row = (cm_numClusters + 7) >> 3;	
	outPtr = out;

	if (!in || !cm_numVisibility) {
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
			Com_DevPrintf (PRNT_WARNING, "Warning: Vis decompression overrun\n");
		}

		while (c) {
			*outPtr++ = 0;
			c--;
		}
	} while (outPtr - out < row);
}


/*
===================
CM_Q2BSP_ClusterPVS
===================
*/
byte *CM_Q2BSP_ClusterPVS (int cluster)
{
	static byte		pvsRow[Q2BSP_MAX_VIS];

	if (cluster == -1)
		memset (pvsRow, 0, (cm_numClusters + 7) >> 3);
	else
		CM_Q2BSP_DecompressVis (cm_mapVisibility + cm_mapVis->bitOfs[cluster][Q2BSP_VIS_PVS], pvsRow);
	return pvsRow;
}


/*
===================
CM_Q2BSP_ClusterPHS
===================
*/
byte *CM_Q2BSP_ClusterPHS (int cluster)
{
	static byte		phsRow[Q2BSP_MAX_VIS];

	if (cluster == -1)
		memset (phsRow, 0, (cm_numClusters + 7) >> 3);
	else
		CM_Q2BSP_DecompressVis (cm_mapVisibility + cm_mapVis->bitOfs[cluster][Q2BSP_VIS_PHS], phsRow);
	return phsRow;
}

/*
=============================================================================

	AREAPORTALS

=============================================================================
*/

/*
====================
CM_Q2BSP_FloodAreaConnections
====================
*/
static void FloodArea_r (cQ2BspArea_t *area, int floodNum)
{
	dQ2BspAreaPortal_t	*p;
	int					i;

	if (area->floodValid == cm_floodValid) {
		if (area->floodNum == floodNum)
			return;
		Com_Error (ERR_DROP, "FloodArea_r: reflooded");
	}

	area->floodNum = floodNum;
	area->floodValid = cm_floodValid;
	p = &cm_mapAreaPortals[area->firstAreaPortal];
	for (i=0 ; i<area->numAreaPortals ; i++, p++) {
		if (cm_portalOpen[p->portalNum])
			FloodArea_r (&cm_mapAreas[p->otherArea], floodNum);
	}
}
void CM_Q2BSP_FloodAreaConnections (void)
{
	cQ2BspArea_t	*area;
	int				floodNum;
	int				i;

	// all current floods are now invalid
	cm_floodValid++;
	floodNum = 0;

	// area 0 is not used
	for (i=1 ; i<cm_numAreas ; i++) {
		area = &cm_mapAreas[i];
		if (area->floodValid == cm_floodValid)
			continue;		// already flooded into
		floodNum++;
		FloodArea_r (area, floodNum);
	}
}


/*
====================
CM_Q2BSP_SetAreaPortalState
====================
*/
void CM_Q2BSP_SetAreaPortalState (int portalNum, qBool open)
{
	if (portalNum > cm_numAreaPortals)
		Com_Error (ERR_DROP, "CM_SetAreaPortalState: areaportal > numAreaPortals");

	cm_portalOpen[portalNum] = open;
	CM_Q2BSP_FloodAreaConnections ();
}


/*
====================
CM_Q2BSP_AreasConnected
====================
*/
qBool CM_Q2BSP_AreasConnected (int area1, int area2)
{
	if (cm_noAreas->value)
		return qTrue;

	if (area1 > cm_numAreas || area2 > cm_numAreas)
		Com_Error (ERR_DROP, "CM_AreasConnected: area > cm_numAreas");

	if (cm_mapAreas[area1].floodNum == cm_mapAreas[area2].floodNum)
		return qTrue;
	return qFalse;
}


/*
=================
CM_Q2BSP_WriteAreaBits

Writes a length byte followed by a bit vector of all the areas
that area in the same flood as the area parameter

This is used by the client refreshes to cull visibility
=================
*/
int CM_Q2BSP_WriteAreaBits (byte *buffer, int area)
{
	int		i;
	int		floodNum;
	int		bytes;

	bytes = (cm_numAreas+7)>>3;

	if (cm_noAreas->value) {
		// for debugging, send everything
		memset (buffer, 255, bytes);
	}
	else {
		memset (buffer, 0, bytes);

		floodNum = cm_mapAreas[area].floodNum;
		for (i=0 ; i<cm_numAreas ; i++) {
			if ((cm_mapAreas[i].floodNum == floodNum) || !area)
				buffer[i>>3] |= 1<<(i&7);
		}
	}

	return bytes;
}


/*
===================
CM_Q2BSP_WritePortalState

Writes the portal state to a savegame file
===================
*/
void CM_Q2BSP_WritePortalState (fileHandle_t fileNum)
{
	FS_Write (cm_portalOpen, sizeof (cm_portalOpen), fileNum);
}


/*
===================
CM_Q2BSP_ReadPortalState

Reads the portal state from a savegame file and recalculates the area connections
===================
*/
void CM_Q2BSP_ReadPortalState (fileHandle_t fileNum)
{
	FS_Read (cm_portalOpen, sizeof (cm_portalOpen), fileNum);
	CM_Q2BSP_FloodAreaConnections ();
}


/*
=============
CM_Q2BSP_HeadnodeVisible

Returns qTrue if any leaf under headnode has a cluster that is potentially visible
=============
*/
qBool CM_Q2BSP_HeadnodeVisible (int nodeNum, byte *visBits)
{
	cQ2BspNode_t	*node;
	int				leafNum;
	int				cluster;

	if (nodeNum < 0) {
		leafNum = -1-nodeNum;
		cluster = cm_mapLeafs[leafNum].cluster;
		if (cluster == -1)
			return qFalse;
		if (visBits[cluster>>3] & (1<<(cluster&7)))
			return qTrue;
		return qFalse;
	}

	node = &cm_mapNodes[nodeNum];
	if (CM_Q2BSP_HeadnodeVisible (node->children[0], visBits))
		return qTrue;
	return CM_Q2BSP_HeadnodeVisible (node->children[1], visBits);
}
