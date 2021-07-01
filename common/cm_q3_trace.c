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
// cm_q3_trace.c
// Quake3 BSP map model tracing
//

#include "cm_q3_local.h"

cBspPlane_t	*box_planes;
int			box_headnode;
cbrush_t	*box_brush;
cleaf_t		*box_leaf;

int			floodvalid;

/*
=============================================================================

	TRACING

=============================================================================
*/

/*
==================
CM_Q3BSP_LeafArea
==================
*/
int CM_Q3BSP_LeafArea (int leafNum)
{
	if (leafNum < 0 || leafNum >= numleafs)
		Com_Error (ERR_DROP, "CM_Q3BSP_LeafArea: bad number");

	return map_leafs[leafNum].area;
}


/*
==================
CM_Q3BSP_LeafCluster
==================
*/
int CM_Q3BSP_LeafCluster (int leafNum)
{
	if (leafNum < 0 || leafNum >= numleafs)
		Com_Error (ERR_DROP, "CM_Q3BSP_LeafCluster: bad number");

	return map_leafs[leafNum].cluster;
}


/*
==================
CM_Q3BSP_LeafContents
==================
*/
int CM_Q3BSP_LeafContents (int leafNum)
{
	if (leafNum < 0 || leafNum >= numleafs)
		Com_Error (ERR_DROP, "CM_Q3BSP_LeafContents: bad number");

	return map_leafs[leafNum].contents;
}

// ==========================================================================

/*
===================
CM_Q3BSP_InitBoxHull
===================
*/
void CM_Q3BSP_InitBoxHull (void)
{
	int				i;
	int				side;
	cnode_t			*c;
	cBspPlane_t		*p;
	cbrushside_t	*s;

	box_headnode = numnodes;
	box_planes = &map_planes[numplanes];
	if (numnodes > MAX_CM_NODES
		|| numbrushes > MAX_CM_BRUSHES
		|| numleafbrushes > MAX_CM_LEAFBRUSHES
		|| numbrushsides > MAX_CM_BRUSHSIDES
		|| numplanes > MAX_CM_PLANES)
		Com_Error (ERR_DROP, "Not enough room for box tree");

	box_brush = &map_brushes[numbrushes];
	box_brush->numSides = 6;
	box_brush->firstBrushSide = numbrushsides;
	box_brush->contents = Q3CNTNTS_BODY;

	box_leaf = &map_leafs[numleafs];
	box_leaf->contents = Q3CNTNTS_BODY;
	box_leaf->firstLeafBrush = numleafbrushes;
	box_leaf->numLeafBrushes = 1;

	map_leafbrushes[numleafbrushes] = numbrushes;

	for (i=0 ; i<6 ; i++)
	{
		side = i&1;

		// brush sides
		s = &map_brushsides[numbrushsides+i];
		s->plane = map_planes + (numplanes+i*2+side);
		s->surface = &nullsurface;

		// nodes
		c = &map_nodes[box_headnode+i];
		c->plane = map_planes + (numplanes+i*2);
		c->children[side] = -1 - emptyleaf;
		if (i != 5)
			c->children[side^1] = box_headnode+i + 1;
		else
			c->children[side^1] = -1 - numleafs;

		// planes
		p = &box_planes[i*2];
		p->type = i>>1;
		p->signBits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = 1;

		p = &box_planes[i*2+1];
		p->type = 3 + (i>>1);
		p->signBits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = -1;
	}
}


/*
===================
CM_Q3BSP_HeadnodeForBox
===================
*/
int	CM_Q3BSP_HeadnodeForBox (vec3_t mins, vec3_t maxs)
{
	box_planes[0].dist = maxs[0];
	box_planes[1].dist = -maxs[0];
	box_planes[2].dist = mins[0];
	box_planes[3].dist = -mins[0];
	box_planes[4].dist = maxs[1];
	box_planes[5].dist = -maxs[1];
	box_planes[6].dist = mins[1];
	box_planes[7].dist = -mins[1];
	box_planes[8].dist = maxs[2];
	box_planes[9].dist = -maxs[2];
	box_planes[10].dist = mins[2];
	box_planes[11].dist = -mins[2];

	return box_headnode;
}


/*
==================
CM_Q3BSP_PointLeafnum
==================
*/
int CM_PointLeafnum_r (vec3_t p, int num)
{
	float		d;
	cnode_t		*node;
	cBspPlane_t	*plane;

	while (num >= 0)
	{
		node = map_nodes + num;
		plane = node->plane;
		d = PlaneDiff (p, plane);

		if (d < 0)
			num = node->children[1];
		else
			num = node->children[0];
	}

	cm_numPointContents++;		// optimize counter

	return -1 - num;
}
int CM_Q3BSP_PointLeafnum (vec3_t p)
{
	if (!numplanes)
		return 0;		// sound may call this without map loaded
	return CM_PointLeafnum_r (p, 0);
}


/*
==================
CM_Q3BSP_BoxLeafnums
==================
*/
int		leaf_count, leaf_maxcount;
int		*leaf_list;
float	*leaf_mins, *leaf_maxs;
int		leaf_topnode;

void CM_BoxLeafnums_r (int nodenum)
{
	cnode_t	*node;
	int		s;

	while (1)
	{
		if (nodenum < 0)
		{
			if (leaf_count >= leaf_maxcount)
			{
				return;
			}
			leaf_list[leaf_count++] = -1 - nodenum;
			return;
		}
	
		node = &map_nodes[nodenum];
		s = BoxOnPlaneSide(leaf_mins, leaf_maxs, node->plane);

		if (s == 1)
			nodenum = node->children[0];
		else if (s == 2)
			nodenum = node->children[1];
		else
		{	// go down both
			if (leaf_topnode == -1)
				leaf_topnode = nodenum;
			CM_BoxLeafnums_r (node->children[0]);
			nodenum = node->children[1];
		}
	}
}
int	CM_BoxLeafnums_headnode (vec3_t mins, vec3_t maxs, int *list, int listsize, int headnode, int *topnode)
{
	leaf_list = list;
	leaf_count = 0;
	leaf_maxcount = listsize;
	leaf_mins = mins;
	leaf_maxs = maxs;

	leaf_topnode = -1;

	CM_BoxLeafnums_r (headnode);

	if (topnode)
		*topnode = leaf_topnode;

	return leaf_count;
}
int	CM_Q3BSP_BoxLeafnums (vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode)
{
	return CM_BoxLeafnums_headnode (mins, maxs, list,
		listsize, cm_mapCModels[0].headNode, topnode);
}


/*
==================
CM_Q3BSP_PointContents
==================
*/
int CM_Q3BSP_PointContents (vec3_t p, int headNode)
{
	int				i, j, contents;
	cleaf_t			*leaf;
	cbrush_t		*brush;
	cbrushside_t	*brushside;

	if (!numnodes)	// map not loaded
		return 0;

	i = CM_PointLeafnum_r (p, headNode);
	leaf = &map_leafs[i];

	if (leaf->contents & Q3CNTNTS_NODROP)
		contents = Q3CNTNTS_NODROP;
	else
		contents = 0;

	for (i=0 ; i<leaf->numLeafBrushes ; i++) {
		brush = &map_brushes[map_leafbrushes[leaf->firstLeafBrush + i]];

		// Check if brush actually adds something to contents
		if ((contents & brush->contents) == brush->contents)
			continue;
		
		brushside = &map_brushsides[brush->firstBrushSide];
		for (j=0 ; j<brush->numSides ; j++, brushside++) {
			if (PlaneDiff (p, brushside->plane) > 0)
				break;
		}

		if (j == brush->numSides) 
			contents |= brush->contents;
	}

	return contents;
}


/*
==================
CM_Q3BSP_TransformedPointContents
==================
*/
int	CM_Q3BSP_TransformedPointContents (vec3_t p, int headNode, vec3_t origin, vec3_t angles)
{
	vec3_t		p_l;
	vec3_t		temp;
	vec3_t		forward, right, up;

	if (!numnodes)	// map not loaded
		return 0;

	// Subtract origin offset
	VectorSubtract (p, origin, p_l);

	// Rotate start and end into the models frame of reference
	if (headNode != box_headnode && (angles[0] || angles[1] || angles[2])) {
		Angles_Vectors (angles, forward, right, up);

		VectorCopy (p_l, temp);
		p_l[0] = DotProduct (temp, forward);
		p_l[1] = -DotProduct (temp, right);
		p_l[2] = DotProduct (temp, up);
	}

	return CM_PointContents (p_l, headNode);
}

/*
=============================================================================

	BOX TRACING

=============================================================================
*/

// 1/32 epsilon to keep floating point happy
#define DIST_EPSILON	(0.03125)

vec3_t	trace_start, trace_end;
vec3_t	trace_mins, trace_maxs;
vec3_t	trace_absmins, trace_absmaxs;
vec3_t	trace_extents;

trace_t	trace_trace;
int		trace_contents;
qBool	trace_ispoint;		// optimized case

/*
================
CM_ClipBoxToBrush
================
*/
void CM_ClipBoxToBrush (vec3_t mins, vec3_t maxs, vec3_t p1, vec3_t p2, trace_t *trace, cbrush_t *brush)
{
	int			i, j;
	cBspPlane_t	*plane, *clipplane;
	float		enterfrac, leavefrac;
	vec3_t		ofs;
	float		d1, d2;
	qBool		getout, startout;
	float		f;
	cbrushside_t	*side, *leadside;

	enterfrac = -1;
	leavefrac = 1;
	clipplane = NULL;

	if (!brush->numSides)
		return;

	cm_numBrushTraces++;

	getout = qFalse;
	startout = qFalse;
	leadside = NULL;

	for (i=0 ; i<brush->numSides ; i++)
	{
		side = &map_brushsides[brush->firstBrushSide+i];
		plane = side->plane;

		if (!trace_ispoint)
		{	// general box case

			// push the plane out apropriately for mins/maxs

			// FIXME: use signbits into 8 way lookup for each mins/maxs
			for (j=0 ; j<3 ; j++)
			{
				if (plane->normal[j] < 0)
					ofs[j] = maxs[j];
				else
					ofs[j] = mins[j];
			}

			if ( plane->type < 3 ) {
				d1 = p1[plane->type] + ofs[plane->type] - plane->dist;
				d2 = p2[plane->type] + ofs[plane->type] - plane->dist;
			}
			else {
				d1 = ( p1[0] + ofs[0] ) * plane->normal[0] + 
					 ( p1[1] + ofs[1] ) * plane->normal[1] +
					 ( p1[2] + ofs[2] ) * plane->normal[2] - plane->dist;
				d2 = ( p2[0] + ofs[0] ) * plane->normal[0] + 
					 ( p2[1] + ofs[1] ) * plane->normal[1] +
					 ( p2[2] + ofs[2] ) * plane->normal[2] - plane->dist;
			}
		}
		else
		{	// special point case
			if ( plane->type < 3 ) {
				d1 = p1[plane->type] - plane->dist;
				d2 = p2[plane->type] - plane->dist;
			} else {
				d1 = DotProduct (p1, plane->normal) - plane->dist;
				d2 = DotProduct (p2, plane->normal) - plane->dist;
			}
		}

		if (d2 > 0)
			getout = qTrue;	// endpoint is not in solid
		if (d1 > 0)
			startout = qTrue;

		// if completely in front of face, no intersection
		if (d1 > 0 && d2 >= d1)
			return;

		if (d1 <= 0 && d2 <= 0)
			continue;

		// crosses face
		if (d1 > d2)
		{	// enter
			f = (d1-DIST_EPSILON) / (d1-d2);
			if (f > enterfrac)
			{
				enterfrac = f;
				clipplane = plane;
				leadside = side;
			}
		}
		else
		{	// leave
			f = (d1+DIST_EPSILON) / (d1-d2);
			if (f < leavefrac)
				leavefrac = f;
		}
	}

	if (!startout)
	{	// original point was inside brush
		trace->startSolid = qTrue;
		if (!getout)
			trace->allSolid = qTrue;
		return;
	}
	if (enterfrac < leavefrac)
	{
		if (enterfrac > -1 && enterfrac < trace->fraction)
		{
			if (enterfrac < 0)
				enterfrac = 0;
			trace->fraction = enterfrac;
			trace->plane = *clipplane;
			trace->surface = leadside->surface;
			trace->contents = brush->contents;
		}
	}
}


/*
================
CM_ClipBoxToPatch
================
*/
void CM_ClipBoxToPatch (vec3_t mins, vec3_t maxs, vec3_t p1, vec3_t p2, trace_t *trace, cbrush_t *brush)
{
	int			i, j;
	cBspPlane_t	*plane, *clipplane;
	float		enterfrac, leavefrac;
	vec3_t		ofs;
	float		d1, d2;
	qBool		startout;
	float		f;
	cbrushside_t	*side, *leadside;

	if (!brush->numSides)
		return;

	cm_numBrushTraces++;

	enterfrac = -1;
	leavefrac = 1;
	clipplane = NULL;
	startout = qFalse;
	leadside = NULL;

	for (i=0 ; i<brush->numSides ; i++)
	{
		side = &map_brushsides[brush->firstBrushSide+i];
		plane = side->plane;

		if (!trace_ispoint)
		{	// general box case

			// push the plane out apropriately for mins/maxs

			// FIXME: use signbits into 8 way lookup for each mins/maxs
			for (j=0 ; j<3 ; j++)
			{
				if (plane->normal[j] < 0)
					ofs[j] = maxs[j];
				else
					ofs[j] = mins[j];
			}

			if ( plane->type < 3 ) {
				d1 = p1[plane->type] + ofs[plane->type] - plane->dist;
				d2 = p2[plane->type] + ofs[plane->type] - plane->dist;
			} else {
				d1 = ( p1[0] + ofs[0] ) * plane->normal[0] + 
					 ( p1[1] + ofs[1] ) * plane->normal[1] +
					 ( p1[2] + ofs[2] ) * plane->normal[2] - plane->dist;
				d2 = ( p2[0] + ofs[0] ) * plane->normal[0] + 
					 ( p2[1] + ofs[1] ) * plane->normal[1] +
					 ( p2[2] + ofs[2] ) * plane->normal[2] - plane->dist;
			}
		}
		else
		{	// special point case
			if ( plane->type < 3 ) {
				d1 = p1[plane->type] - plane->dist;
				d2 = p2[plane->type] - plane->dist;
			} else {
				d1 = DotProduct (p1, plane->normal) - plane->dist;
				d2 = DotProduct (p2, plane->normal) - plane->dist;
			}
		}

		if (d1 > 0)
			startout = qTrue;

		// if completely in front of face, no intersection
		if (d1 > 0 && d2 >= d1)
			return;

		if (d1 <= 0 && d2 <= 0)
			continue;

		// crosses face
		if (d1 > d2)
		{	// enter
			f = (d1-DIST_EPSILON) / (d1-d2);
			if (f > enterfrac)
			{
				enterfrac = f;
				clipplane = plane;
				leadside = side;
			}
		}
		else
		{	// leave
			f = (d1 /*+ DIST_EPSILON*/) / (d1-d2);
			if (f < leavefrac)
				leavefrac = f;
		}
	}

	if (!startout)
		return;		// original point is inside the patch

	if (enterfrac < leavefrac)
	{
		if (leadside && leadside->surface
			&& enterfrac < trace->fraction)
		{
			if (enterfrac < 0)
				enterfrac = 0;
			trace->fraction = enterfrac;
			trace->plane = *clipplane;
			trace->surface = leadside->surface;
			trace->contents = brush->contents;
		}
	}
}


/*
================
CM_TestBoxInBrush
================
*/
void CM_TestBoxInBrush (vec3_t mins, vec3_t maxs, vec3_t p1,
					  trace_t *trace, cbrush_t *brush)
{
	int			i, j;
	cBspPlane_t	*plane;
	vec3_t		ofs;
	float		d1;
	cbrushside_t	*side;

	if (!brush->numSides)
		return;

	for (i=0 ; i<brush->numSides ; i++)
	{
		side = &map_brushsides[brush->firstBrushSide+i];
		plane = side->plane;

		// general box case

		// push the plane out apropriately for mins/maxs

		// FIXME: use signbits into 8 way lookup for each mins/maxs
		for (j=0 ; j<3 ; j++)
		{
			if (plane->normal[j] < 0)
				ofs[j] = maxs[j];
			else
				ofs[j] = mins[j];
		}

		if ( plane->type < 3 ) {
			d1 = p1[plane->type] + ofs[plane->type] - plane->dist;
		} else {
			d1 = ( p1[0] + ofs[0] ) * plane->normal[0] +
				 ( p1[1] + ofs[1] ) * plane->normal[1] +
				 ( p1[2] + ofs[2] ) * plane->normal[2] - plane->dist;
		}

		// if completely in front of face, no intersection
		if (d1 > 0)
			return;
	}

	// inside this brush
	trace->startSolid = trace->allSolid = qTrue;
	trace->fraction = 0;
	trace->contents = brush->contents;
}


/*
================
CM_TestBoxInPatch
================
*/
void CM_TestBoxInPatch (vec3_t mins, vec3_t maxs, vec3_t p1, trace_t *trace, cbrush_t *brush)
{
	int			i, j;
	cBspPlane_t	*plane;
	vec3_t		ofs;
	float		d1, maxdist;
	cbrushside_t	*side;

	if (!brush->numSides)
		return;

	maxdist = -9999;

	for (i=0 ; i<brush->numSides ; i++)
	{
		side = &map_brushsides[brush->firstBrushSide+i];
		plane = side->plane;

		// general box case

		// push the plane out apropriately for mins/maxs

		// FIXME: use signbits into 8 way lookup for each mins/maxs
		for (j=0 ; j<3 ; j++)
		{
			if (plane->normal[j] < 0)
				ofs[j] = maxs[j];
			else
				ofs[j] = mins[j];
		}

		if ( plane->type < 3 ) {
			d1 = p1[plane->type] + ofs[plane->type] - plane->dist;
		} else {
			d1 = ( p1[0] + ofs[0] ) * plane->normal[0] +
				 ( p1[1] + ofs[1] ) * plane->normal[1] +
				 ( p1[2] + ofs[2] ) * plane->normal[2] - plane->dist;
		}

		// if completely in front of face, no intersection
		if (d1 > 0)
			return;

		if (side->surface && d1 > maxdist)
			maxdist = d1;
	}

// FIXME
//	if (maxdist < -0.25)
//		return;		// deep inside the patch

	// inside this patch
	trace->startSolid = trace->allSolid = qTrue;
	trace->fraction = 0;
	trace->contents = brush->contents;
}


/*
================
CM_TraceToLeaf
================
*/
void CM_TraceToLeaf (int leafnum)
{
	int			i, j;
	int			brushnum, patchnum;
	cleaf_t		*leaf;
	cbrush_t	*b;
	cpatch_t	*patch;

	leaf = &map_leafs[leafnum];
	if (!(leaf->contents & trace_contents))
		return;

	// trace line against all brushes in the leaf
	for (i=0 ; i<leaf->numLeafBrushes ; i++)
	{
		brushnum = map_leafbrushes[leaf->firstLeafBrush+i];

		b = &map_brushes[brushnum];
		if (b->checkCount == checkcount)
			continue;	// already checked this brush in another leaf
		b->checkCount = checkcount;
		if (!(b->contents & trace_contents))
			continue;
		CM_ClipBoxToBrush (trace_mins, trace_maxs, trace_start, trace_end, &trace_trace, b);
		if (!trace_trace.fraction)
			return;
	}

	if (cm_noCurves->value)
		return;

	// trace line against all patches in the leaf
	for (i = 0; i < leaf->numLeafPatches; i++)
	{
		patchnum = map_leafpatches[leaf->firstLeafPatch+i];

		patch = &map_patches[patchnum];
		if (patch->checkCount == checkcount)
			continue;	// already checked this patch in another leaf
		patch->checkCount = checkcount;
		if ( !(patch->surface->contents & trace_contents) )
			continue;
		if ( !BoundsIntersect(patch->absMins, patch->absMaxs, trace_absmins, trace_absmaxs) )
			continue;
		for (j = 0; j < patch->numBrushes; j++)
		{
			CM_ClipBoxToPatch (trace_mins, trace_maxs, trace_start, trace_end, &trace_trace, &patch->brushes[j]);
			if (!trace_trace.fraction)
				return;
		}
	}
}


/*
================
CM_TestInLeaf
================
*/
void CM_TestInLeaf (int leafnum)
{
	int			i, j;
	int			brushnum, patchnum;
	cleaf_t		*leaf;
	cbrush_t	*b;
	cpatch_t	*patch;

	leaf = &map_leafs[leafnum];
	if ( !(leaf->contents & trace_contents))
		return;

	// trace line against all brushes in the leaf
	for (i=0 ; i<leaf->numLeafBrushes ; i++)
	{
		brushnum = map_leafbrushes[leaf->firstLeafBrush+i];

		b = &map_brushes[brushnum];
		if (b->checkCount == checkcount)
			continue;	// already checked this brush in another leaf
		b->checkCount = checkcount;
		if ( !(b->contents & trace_contents))
			continue;
		CM_TestBoxInBrush (trace_mins, trace_maxs, trace_start, &trace_trace, b);
		if (!trace_trace.fraction)
			return;
	}

	if (cm_noCurves->value)
		return;

	// trace line against all patches in the leaf
	for (i = 0; i < leaf->numLeafPatches; i++)
	{
		patchnum = map_leafpatches[leaf->firstLeafPatch+i];

		patch = &map_patches[patchnum];
		if (patch->checkCount == checkcount)
			continue;	// already checked this patch in another leaf
		patch->checkCount = checkcount;
		if ( !(patch->surface->contents & trace_contents) )
			continue;
		if ( !BoundsIntersect(patch->absMins, patch->absMaxs, trace_absmins, trace_absmaxs) )
			continue;
		for (j = 0; j < patch->numBrushes; j++)
		{
			CM_TestBoxInPatch (trace_mins, trace_maxs, trace_start, &trace_trace, &patch->brushes[j]);
			if (!trace_trace.fraction)
				return;
		}
	}
}


/*
==================
CM_RecursiveHullCheck

==================
*/
void CM_RecursiveHullCheck (int num, float p1f, float p2f, vec3_t p1, vec3_t p2)
{
	cnode_t		*node;
	cBspPlane_t	*plane;
	float		t1, t2, offset;
	float		frac, frac2;
	float		idist;
	int			i;
	vec3_t		mid;
	int			side;
	float		midf;

	if (trace_trace.fraction <= p1f)
		return;		// already hit something nearer

	// if < 0, we are in a leaf node
	if (num < 0)
	{
		CM_TraceToLeaf (-1-num);
		return;
	}

	//
	// find the point distances to the seperating plane
	// and the offset for the size of the box
	//
	node = map_nodes + num;
	plane = node->plane;

	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
		offset = trace_extents[plane->type];
	}
	else
	{
		t1 = DotProduct (plane->normal, p1) - plane->dist;
		t2 = DotProduct (plane->normal, p2) - plane->dist;
		if (trace_ispoint)
			offset = 0;
		else
			offset = fabs(trace_extents[0]*plane->normal[0]) +
				fabs(trace_extents[1]*plane->normal[1]) +
				fabs(trace_extents[2]*plane->normal[2]);
	}


	// see which sides we need to consider
	if (t1 >= offset && t2 >= offset)
	{
		CM_RecursiveHullCheck (node->children[0], p1f, p2f, p1, p2);
		return;
	}
	if (t1 < -offset && t2 < -offset)
	{
		CM_RecursiveHullCheck (node->children[1], p1f, p2f, p1, p2);
		return;
	}

	// put the crosspoint DIST_EPSILON pixels on the near side
	if (t1 < t2)
	{
		idist = 1.0/(t1-t2);
		side = 1;
		frac2 = (t1 + offset + DIST_EPSILON)*idist;
		frac = (t1 - offset + DIST_EPSILON)*idist;
	}
	else if (t1 > t2)
	{
		idist = 1.0/(t1-t2);
		side = 0;
		frac2 = (t1 - offset - DIST_EPSILON)*idist;
		frac = (t1 + offset + DIST_EPSILON)*idist;
	}
	else
	{
		side = 0;
		frac = 1;
		frac2 = 0;
	}

	// move up to the node
	clamp ( frac, 0, 1 );

	midf = p1f + (p2f - p1f)*frac;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac*(p2[i] - p1[i]);

	CM_RecursiveHullCheck (node->children[side], p1f, midf, p1, mid);

	// go past the node
	clamp ( frac2, 0, 1 );

	midf = p1f + (p2f - p1f)*frac2;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac2*(p2[i] - p1[i]);

	CM_RecursiveHullCheck (node->children[side^1], midf, p2f, mid, p2);
}

// ==========================================================================

/*
====================
CM_Q3BSP_Trace
====================
*/
trace_t CM_Q3BSP_Trace (vec3_t start, vec3_t end, float size, int contentMask)
{
	vec3_t maxs, mins;

	VectorSet (maxs, size, size, size);
	VectorSet (mins, -size, -size, -size);

	return CM_Q3BSP_BoxTrace (start, end, mins, maxs, 0, contentMask);
}


/*
==================
CM_Q3BSP_BoxTrace
==================
*/
trace_t CM_Q3BSP_BoxTrace (vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int headNode, int brushMask)
{
	int		i;
	vec3_t	point;

	checkcount++;		// for multi-check avoidance

	cm_numTraces++;			// for statistics, may be zeroed

	// fill in a default trace
	memset (&trace_trace, 0, sizeof(trace_trace));
	trace_trace.fraction = 1;
	trace_trace.surface = &nullsurface;

	if (!numnodes)	// map not loaded
		return trace_trace;

	trace_contents = brushMask;
	VectorCopy (start, trace_start);
	VectorCopy (end, trace_end);
	VectorCopy (mins, trace_mins);
	VectorCopy (maxs, trace_maxs);

	// build a bounding box of the entire move
	ClearBounds (trace_absmins, trace_absmaxs);
	VectorAdd (start, trace_mins, point);
	AddPointToBounds (point, trace_absmins, trace_absmaxs);
	VectorAdd (start, trace_maxs, point);
	AddPointToBounds (point, trace_absmins, trace_absmaxs);
	VectorAdd (end, trace_mins, point);
	AddPointToBounds (point, trace_absmins, trace_absmaxs);
	VectorAdd (end, trace_maxs, point);
	AddPointToBounds (point, trace_absmins, trace_absmaxs);

	//
	// check for position test special case
	//
	if (start[0] == end[0] && start[1] == end[1] && start[2] == end[2])
	{
		int		leafs[1024];
		int		i, numleafs;
		vec3_t	c1, c2;
		int		topnode;

		VectorAdd (start, mins, c1);
		VectorAdd (start, maxs, c2);
		for (i=0 ; i<3 ; i++)
		{
			c1[i] -= 1;
			c2[i] += 1;
		}

		numleafs = CM_BoxLeafnums_headnode (c1, c2, leafs, 1024, headNode, &topnode);
		for (i=0 ; i<numleafs ; i++)
		{
			CM_TestInLeaf (leafs[i]);
			if (trace_trace.allSolid)
				break;
		}
		VectorCopy (start, trace_trace.endPos);
		return trace_trace;
	}

	//
	// check for point special case
	//
	if (mins[0] == 0 && mins[1] == 0 && mins[2] == 0
		&& maxs[0] == 0 && maxs[1] == 0 && maxs[2] == 0)
	{
		trace_ispoint = qTrue;
		VectorClear (trace_extents);
	}
	else
	{
		trace_ispoint = qFalse;
		trace_extents[0] = -mins[0] > maxs[0] ? -mins[0] : maxs[0];
		trace_extents[1] = -mins[1] > maxs[1] ? -mins[1] : maxs[1];
		trace_extents[2] = -mins[2] > maxs[2] ? -mins[2] : maxs[2];
	}

	//
	// general sweeping through world
	//
	CM_RecursiveHullCheck (headNode, 0, 1, start, end);

	if (trace_trace.fraction == 1)
	{
		VectorCopy (end, trace_trace.endPos);
	}
	else
	{
		for (i=0 ; i<3 ; i++)
			trace_trace.endPos[i] = start[i] + trace_trace.fraction * (end[i] - start[i]);
	}
	return trace_trace;
}


/*
==================
CM_Q3BSP_TransformedBoxTrace
==================
*/
#ifdef WIN32
#pragma optimize( "", off )
#endif
trace_t CM_Q3BSP_TransformedBoxTrace (vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs,
						  int headNode, int brushMask, vec3_t origin, vec3_t angles)
{
	trace_t		trace;
	vec3_t		start_l, end_l;
	vec3_t		a;
	vec3_t		forward, right, up;
	vec3_t		temp;
	qBool		rotated;

	// subtract origin offset
	VectorSubtract (start, origin, start_l);
	VectorSubtract (end, origin, end_l);

	// rotate start and end into the models frame of reference
	if (headNode != box_headnode && 
	(angles[0] || angles[1] || angles[2]) )
		rotated = qTrue;
	else
		rotated = qFalse;

	if (rotated)
	{
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
	trace = CM_BoxTrace (start_l, end_l, mins, maxs, headNode, brushMask);

	if (rotated && trace.fraction != 1.0)
	{
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
#pragma optimize( "", on )
#endif

/*
=============================================================================

	PVS / PHS

=============================================================================
*/

/*
===================
CM_Q3BSP_ClusterPVS
===================
*/
byte *CM_Q3BSP_ClusterPVS (int cluster)
{
	if (cluster != -1 && map_pvs->numClusters)
		return (byte *)map_pvs->data + cluster * map_pvs->rowSize;

	return nullrow;
}


/*
===================
CM_Q3BSP_ClusterPHS
===================
*/
byte *CM_Q3BSP_ClusterPHS (int cluster)
{
	if (cluster != -1 && map_phs->numClusters)
		return (byte *)map_phs->data + cluster * map_phs->rowSize;

	return nullrow;
}

/*
=============================================================================

	AREAPORTALS

=============================================================================
*/

/*
=================
CM_Q3BSP_AddAreaPortal
=================
*/
static qBool CM_Q3BSP_AddAreaPortal (int portalnum, int area, int otherarea)
{
	carea_t *a;
	careaportal_t *ap;

	if (portalnum >= MAX_CM_AREAPORTALS)
		return qFalse;
	if (!area || area > numareas || !otherarea || otherarea > numareas)
		return qFalse;

	ap = &map_areaportals[portalnum];
	ap->area = area;
	ap->otherArea = otherarea;

	a = &map_areas[area];
	a->areaPortals[a->numAreaPortals++] = portalnum;

	a = &map_areas[otherarea];
	a->areaPortals[a->numAreaPortals++] = portalnum;

	numareaportals++;

	return qTrue;
}


/*
=================
CM_Q3BSP_FloodArea_r
=================
*/
static void CM_Q3BSP_FloodArea_r ( int areanum, int floodnum )
{
	int				i;
	carea_t			*area;
	careaportal_t	*p;

	area = &map_areas[areanum];
	if (area->floodValid == floodvalid) {
		if (area->floodNum == floodnum)
			return;
		Com_Error (ERR_DROP, "CM_Q3BSP_FloodArea_r: reflooded");
	}

	area->floodNum = floodnum;
	area->floodValid = floodvalid;
	for (i=0 ; i<area->numAreaPortals ; i++) {
		p = &map_areaportals[area->areaPortals[i]];
		if (!p->open)
			continue;

		if (p->area == areanum)
			CM_Q3BSP_FloodArea_r (p->otherArea, floodnum);
		else if (p->otherArea == areanum)
			CM_Q3BSP_FloodArea_r (p->area, floodnum);
	}
}


/*
====================
CM_Q3BSP_FloodAreaConnections
====================
*/
void CM_Q3BSP_FloodAreaConnections (void)
{
	int		i;
	int		floodnum;

	// all current floods are now invalid
	floodvalid++;
	floodnum = 0;

	// area 0 is not used
	for (i=1 ; i<numareas ; i++) {
		if (map_areas[i].floodValid == floodvalid)
			continue;		// already flooded into
		floodnum++;
		CM_Q3BSP_FloodArea_r (i, floodnum);
	}
}


/*
====================
CM_Q3BSP_SetAreaPortalState
====================
*/
void CM_Q3BSP_SetAreaPortalState (int portalNum, int area, int otherArea, qBool open)
{
	if (portalNum >= MAX_CM_AREAPORTALS)
		Com_Error (ERR_DROP, "portalNum >= MAX_CM_AREAPORTALS");

	if (!map_areaportals[portalNum].area) {
		// add new areaportal if it doesn't exist
		if (!CM_Q3BSP_AddAreaPortal (portalNum, area, otherArea))
			return;
	}

	map_areaportals[portalNum].open = open;
	CM_Q3BSP_FloodAreaConnections ();
}


/*
====================
CM_Q3BSP_AreasConnected
====================
*/
qBool CM_Q3BSP_AreasConnected (int area1, int area2)
{
	if (cm_noAreas->value)
		return qTrue;
	if (area1 > numareas || area2 > numareas)
		Com_Error (ERR_DROP, "CM_AreasConnected: area > numareas");

	if (map_areas[area1].floodNum == map_areas[area2].floodNum)
		return qTrue;
	return qFalse;
}


/*
=================
CM_Q3BSP_WriteAreaBits
=================
*/
int CM_Q3BSP_WriteAreaBits (byte *buffer, int area)
{
	int		i;
	int		bytes;

	bytes = (numareas+7)>>3;

	if (cm_noAreas->value)
	{	// for debugging, send everything
		memset (buffer, 255, bytes);
	}
	else
	{
		memset (buffer, 0, bytes);

		for (i=1 ; i<numareas ; i++)
		{
			if (!area || CM_AreasConnected ( i, area ) || i == area)
				buffer[i>>3] |= 1<<(i&7);
		}
	}

	return bytes;
}


/*
===================
CM_Q3BSP_WritePortalState
===================
*/
void CM_Q3BSP_WritePortalState (fileHandle_t fileNum)
{
}


/*
===================
CM_Q3BSP_ReadPortalState
===================
*/
void CM_Q3BSP_ReadPortalState (fileHandle_t fileNum)
{
}


/*
=============
CM_Q3BSP_HeadnodeVisible
=============
*/
qBool CM_Q3BSP_HeadnodeVisible (int nodeNum, byte *visBits)
{
	int		leafnum;
	int		cluster;
	cnode_t	*node;

	if (nodeNum < 0)
	{
		leafnum = -1-nodeNum;
		cluster = map_leafs[leafnum].cluster;
		if (cluster == -1)
			return qFalse;
		if (visBits[cluster>>3] & (1<<(cluster&7)))
			return qTrue;
		return qFalse;
	}

	node = &map_nodes[nodeNum];
	if (CM_HeadnodeVisible(node->children[0], visBits))
		return qTrue;
	return CM_HeadnodeVisible(node->children[1], visBits);
}
