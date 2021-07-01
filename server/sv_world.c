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

// sv_world.c
// - world query functions

#include "sv_local.h"

/*
===============================================================================

	ENTITY AREA CHECKING
- FIXME: this use of "area" is different from the bsp file use

===============================================================================
*/

// (type *)STRUCT_FROM_LINK(link_t *link, type, member)
// ent = STRUCT_FROM_LINK(link,entity_t,order)
// FIXME: remove this mess!
#define	STRUCT_FROM_LINK(l,t,m) ((t *)((byte *)l - (int)&(((t *)0)->m)))

#define	EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,edict_t,area)

typedef struct areaNode_s {
	int			axis;		// -1 = leaf node
	float		dist;
	struct		areaNode_s		*children[2];
	link_t		trigger_edicts;
	link_t		solid_edicts;
} areaNode_t;

#define	AREA_DEPTH	4
#define	AREA_NODES	32

areaNode_t	sv_areanodes[AREA_NODES];
int			sv_numareanodes;

float	*area_mins, *area_maxs;
edict_t	**area_list;
int		area_count, area_maxcount;
int		area_type;

int SV_HullForEntity (edict_t *ent);


// ClearLink is used for new headNodes
void ClearLink (link_t *l) {
	l->prev = l->next = l;
}

void RemoveLink (link_t *l) {
	l->next->prev = l->prev;
	l->prev->next = l->next;
}

void InsertLinkBefore (link_t *l, link_t *before) {
	l->next = before;
	l->prev = before->prev;
	l->prev->next = l;
	l->next->prev = l;
}

/*
===============
SV_CreateAreaNode

Builds a uniformly subdivided tree for the given world size
===============
*/
areaNode_t *SV_CreateAreaNode (int depth, vec3_t mins, vec3_t maxs) {
	areaNode_t	*anode;
	vec3_t		size;
	vec3_t		mins1, maxs1, mins2, maxs2;

	anode = &sv_areanodes[sv_numareanodes];
	sv_numareanodes++;

	ClearLink (&anode->trigger_edicts);
	ClearLink (&anode->solid_edicts);
	
	if (depth == AREA_DEPTH) {
		anode->axis = -1;
		anode->children[0] = anode->children[1] = NULL;
		return anode;
	}
	
	VectorSubtract (maxs, mins, size);
	if (size[0] > size[1])
		anode->axis = 0;
	else
		anode->axis = 1;
	
	anode->dist = 0.5 * (maxs[anode->axis] + mins[anode->axis]);
	VectorCopy (mins, mins1);	
	VectorCopy (mins, mins2);	
	VectorCopy (maxs, maxs1);	
	VectorCopy (maxs, maxs2);	
	
	maxs1[anode->axis] = mins2[anode->axis] = anode->dist;
	
	anode->children[0] = SV_CreateAreaNode (depth+1, mins2, maxs2);
	anode->children[1] = SV_CreateAreaNode (depth+1, mins1, maxs1);

	return anode;
}


/*
===============
SV_ClearWorld
===============
*/
void SV_ClearWorld (void) {
	memset (sv_areanodes, 0, sizeof (sv_areanodes));
	sv_numareanodes = 0;
	SV_CreateAreaNode (0, sv.models[1]->mins, sv.models[1]->maxs);
}


/*
===============
SV_UnlinkEdict
===============
*/
void SV_UnlinkEdict (edict_t *ent) {
	if (!ent->area.prev)
		return;		// not linked in anywhere
	RemoveLink (&ent->area);
	ent->area.prev = ent->area.next = NULL;
}


/*
===============
SV_LinkEdict
===============
*/
#define MAX_TOTAL_ENT_LEAFS		128
void SV_LinkEdict (edict_t *ent) {
	areaNode_t	*node;
	int			leafs[MAX_TOTAL_ENT_LEAFS];
	int			clusters[MAX_TOTAL_ENT_LEAFS];
	int			num_leafs;
	int			i, j, k;
	int			area;
	int			topnode;

	if (ent->area.prev)
		SV_UnlinkEdict (ent);	// unlink from old position
		
	if (ent == ge->edicts)
		return;		// don't add the world

	if (!ent->inUse)
		return;

	/* set the size */
	VectorSubtract (ent->maxs, ent->mins, ent->size);
	
	/* encode the size into the entity_state for client prediction */
	if (ent->solid == SOLID_BBOX && !(ent->svFlags & SVF_DEADMONSTER)) {
		/* assume that x/y are equal and symetric */
		i = ent->maxs[0]/8;
		if (i<1)
			i = 1;
		if (i>31)
			i = 31;

		/* z is not symetric */
		j = (-ent->mins[2])/8;
		if (j<1)
			j = 1;
		if (j>31)
			j = 31;

		/* and z maxs can be negative... */
		k = (ent->maxs[2]+32)/8;
		if (k<1)
			k = 1;
		if (k>63)
			k = 63;

		ent->s.solid = (k<<10) | (j<<5) | i;
	} else if (ent->solid == SOLID_BSP) {
		ent->s.solid = 31;		// a solid_bbox will never create this value
	} else
		ent->s.solid = 0;

	/* set the abs box */
	if (ent->solid == SOLID_BSP && (ent->s.angles[0] || ent->s.angles[1] || ent->s.angles[2])) {
		/* expand for rotation */
		float		rMax, v;
		int			i;

		rMax = 0;
		for (i=0 ; i<3 ; i++) {
			v =fabs (ent->mins[i]);
			if (v > rMax)
				rMax = v;
			v =fabs (ent->maxs[i]);
			if (v > rMax)
				rMax = v;
		}

		for (i=0 ; i<3 ; i++) {
			ent->absMin[i] = ent->s.origin[i] - rMax;
			ent->absMax[i] = ent->s.origin[i] + rMax;
		}
	} else {
		/* normal */
		VectorAdd (ent->s.origin, ent->mins, ent->absMin);	
		VectorAdd (ent->s.origin, ent->maxs, ent->absMax);
	}

	/*
	** because movement is clipped an epsilon away from an actual edge,
	** we must fully check even when bounding boxes don't quite touch
	*/
	ent->absMin[0] -= 1;
	ent->absMin[1] -= 1;
	ent->absMin[2] -= 1;
	ent->absMax[0] += 1;
	ent->absMax[1] += 1;
	ent->absMax[2] += 1;

	/* link to PVS leafs */
	ent->numClusters = 0;
	ent->areaNum = 0;
	ent->areaNum2 = 0;

	/* get all leafs, including solids */
	num_leafs = CM_BoxLeafnums (ent->absMin, ent->absMax,
		leafs, MAX_TOTAL_ENT_LEAFS, &topnode);

	/* set areas */
	for (i=0 ; i<num_leafs ; i++) {
		clusters[i] = CM_LeafCluster (leafs[i]);
		area = CM_LeafArea (leafs[i]);
		if (area) {
			/*
			** doors may legally straggle two areas,
			** but nothing should evern need more than that
			*/
			if (ent->areaNum && (ent->areaNum != area)) {
				if (ent->areaNum2 && (ent->areaNum2 != area) && (sv.state == SS_LOADING))
					Com_Printf (PRINT_DEV, "Object touching 3 areas at %f %f %f\n",
					ent->absMin[0], ent->absMin[1], ent->absMin[2]);
				ent->areaNum2 = area;
			} else
				ent->areaNum = area;
		}
	}

	if (num_leafs >= MAX_TOTAL_ENT_LEAFS) {
		/* assume we missed some leafs, and mark by headNode */
		ent->numClusters = -1;
		ent->headNode = topnode;
	} else {
		ent->numClusters = 0;
		for (i=0 ; i<num_leafs ; i++) {
			if (clusters[i] == -1)
				continue;		// not a visible leaf
			for (j=0 ; j<i ; j++)
				if (clusters[j] == clusters[i])
					break;
			if (j == i) {
				if (ent->numClusters == MAX_ENT_CLUSTERS) {
					/* assume we missed some leafs, and mark by headNode */
					ent->numClusters = -1;
					ent->headNode = topnode;
					break;
				}

				ent->clusterNums[ent->numClusters++] = clusters[i];
			}
		}
	}

	/* if first time, make sure oldOrigin is valid */
	if (!ent->linkCount)
		VectorCopy (ent->s.origin, ent->s.oldOrigin);

	ent->linkCount++;

	if (ent->solid == SOLID_NOT)
		return;

	/* find the first node that the ent's box crosses */
	node = sv_areanodes;
	for ( ; ; ) {
		if (node->axis == -1)
			break;
		if (ent->absMin[node->axis] > node->dist)
			node = node->children[0];
		else if (ent->absMax[node->axis] < node->dist)
			node = node->children[1];
		else
			break;	// crosses the node
	}
	
	/* link it in */	
	if (ent->solid == SOLID_TRIGGER)
		InsertLinkBefore (&ent->area, &node->trigger_edicts);
	else
		InsertLinkBefore (&ent->area, &node->solid_edicts);
}


/*
====================
SV_AreaEdicts_r
====================
*/
void SV_AreaEdicts_r (areaNode_t *node) {
	link_t		*l, *next, *start;
	edict_t		*check;
	int			count;

	count = 0;

	/* touch linked edicts */
	if (area_type == AREA_SOLID)
		start = &node->solid_edicts;
	else
		start = &node->trigger_edicts;

	for (l=start->next  ; l != start ; l = next) {
		next = l->next;
		check = EDICT_FROM_AREA(l);

		if (check->solid == SOLID_NOT)
			continue;		// deactivated
		if (check->absMin[0] > area_maxs[0]
		|| check->absMin[1] > area_maxs[1]
		|| check->absMin[2] > area_maxs[2]
		|| check->absMax[0] < area_mins[0]
		|| check->absMax[1] < area_mins[1]
		|| check->absMax[2] < area_mins[2])
			continue;		// not touching

		if (area_count == area_maxcount) {
			Com_Printf (PRINT_ALL, "SV_AreaEdicts: MAXCOUNT\n");
			return;
		}

		area_list[area_count] = check;
		area_count++;
	}
	
	if (node->axis == -1)
		return;		// terminal node

	/* recurse down both sides */
	if (area_maxs[node->axis] > node->dist)
		SV_AreaEdicts_r (node->children[0]);
	if (area_mins[node->axis] < node->dist)
		SV_AreaEdicts_r (node->children[1]);
}


/*
================
SV_AreaEdicts
================
*/
int SV_AreaEdicts (vec3_t mins, vec3_t maxs, edict_t **list, int maxcount, int areatype) {
	area_mins = mins;
	area_maxs = maxs;
	area_list = list;
	area_count = 0;
	area_maxcount = maxcount;
	area_type = areatype;

	SV_AreaEdicts_r (sv_areanodes);

	return area_count;
}

//===========================================================================

/*
=============
SV_PointContents
=============
*/
int SV_PointContents (vec3_t p) {
	edict_t		*touch[MAX_EDICTS], *hit;
	int			i, num;
	int			contents;
	int			headNode;
	float		*angles;

	/* get base contents from world */
	contents = CM_PointContents (p, sv.models[1]->headNode);

	/* or in contents from all the other entities */
	num = SV_AreaEdicts (p, p, touch, MAX_EDICTS, AREA_SOLID);

	for (i=0 ; i<num ; i++) {
		hit = touch[i];

		/* might intersect, so do an exact clip */
		headNode = SV_HullForEntity (hit);
		angles = hit->s.angles;
		if (hit->solid != SOLID_BSP)
			angles = vec3Identity;	// boxes don't rotate

		contents |= CM_TransformedPointContents (p, headNode, hit->s.origin, hit->s.angles);
	}

	return contents;
}


/*
================
SV_HullForEntity

Returns a headNode that can be used for testing or clipping an
object of mins/maxs size.
Offset is filled in to contain the adjustment that must be added to the
testing object's origin to get a point to use with the returned hull.
================
*/
int SV_HullForEntity (edict_t *ent) {
	cBspModel_t	*model;

	/* decide which clipping hull to use, based on the size */
	if (ent->solid == SOLID_BSP) {
		/* explicit hulls in the BSP model */
		model = sv.models[ent->s.modelIndex];

		if (!model)
			Com_Error (ERR_FATAL, "MOVETYPE_PUSH with a non bsp model");

		return model->headNode;
	}

	/* create a temp hull from bounding box sizes */
	return CM_HeadnodeForBox (ent->mins, ent->maxs);
}

//===========================================================================

typedef struct {
	vec3_t		boxmins, boxmaxs;// enclose the test object along entire move
	float		*mins, *maxs;	// size of the moving object
	vec3_t		mins2, maxs2;	// size when clipping against mosnters
	float		*start, *end;
	trace_t		trace;
	edict_t		*passedict;
	int			contentmask;
} moveClip_t;

/*
====================
SV_ClipMoveToEntities
====================
*/
void SV_ClipMoveToEntities (moveClip_t *clip) {
	int			i, num;
	edict_t		*touchlist[MAX_EDICTS], *touch;
	trace_t		trace;
	int			headNode;
	float		*angles;

	num = SV_AreaEdicts (clip->boxmins, clip->boxmaxs, touchlist, MAX_EDICTS, AREA_SOLID);

	/*
	** be careful, it is possible to have an entity in this
	** list removed before we get to it (killtriggered)
	*/
	for (i=0 ; i<num ; i++) {
		touch = touchlist[i];
		if (touch->solid == SOLID_NOT)
			continue;
		if (touch == clip->passedict)
			continue;
		if (clip->trace.allSolid)
			return;
		if (clip->passedict) {
			if (touch->owner == clip->passedict)
				continue;	// don't clip against own missiles
			if (clip->passedict->owner == touch)
				continue;	// don't clip against owner
		}

		if (!(clip->contentmask & CONTENTS_DEADMONSTER) && (touch->svFlags & SVF_DEADMONSTER))
			continue;

		/* might intersect, so do an exact clip */
		headNode = SV_HullForEntity (touch);
		angles = touch->s.angles;
		if (touch->solid != SOLID_BSP)
			angles = vec3Identity;	// boxes don't rotate

		if (touch->svFlags & SVF_MONSTER)
			trace = CM_TransformedBoxTrace (clip->start, clip->end,
				clip->mins2, clip->maxs2, headNode, clip->contentmask,
				touch->s.origin, angles);
		else
			trace = CM_TransformedBoxTrace (clip->start, clip->end,
				clip->mins, clip->maxs, headNode,  clip->contentmask,
				touch->s.origin, angles);

		if (trace.allSolid || trace.startSolid || (trace.fraction < clip->trace.fraction)) {
			trace.ent = touch;
			if (clip->trace.startSolid) {
				clip->trace = trace;
				clip->trace.startSolid = qTrue;
			} else
				clip->trace = trace;
		} else if (trace.startSolid)
			clip->trace.startSolid = qTrue;
	}
}


/*
==================
SV_TraceBounds
==================
*/
void SV_TraceBounds (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, vec3_t boxmins, vec3_t boxmaxs) {
	int		i;
	
	for (i=0 ; i<3 ; i++) {
		if (end[i] > start[i]) {
			boxmins[i] = start[i] + mins[i] - 1;
			boxmaxs[i] = end[i] + maxs[i] + 1;
		} else {
			boxmins[i] = end[i] + mins[i] - 1;
			boxmaxs[i] = start[i] + maxs[i] + 1;
		}
	}
}


/*
==================
SV_Trace

Moves the given mins/maxs volume through the world from start to end.

Passedict and edicts owned by passedict are explicitly not checked.
==================
*/
trace_t SV_Trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, edict_t *passedict, int contentmask) {
	moveClip_t	clip;

	if (!mins)
		mins = vec3Identity;
	if (!maxs)
		maxs = vec3Identity;

	memset (&clip, 0, sizeof (moveClip_t));

	/* clip to world */
	clip.trace = CM_BoxTrace (start, end, mins, maxs, 0, contentmask);
	clip.trace.ent = ge->edicts;
	if (clip.trace.fraction == 0)
		return clip.trace;		// blocked by the world

	clip.contentmask = contentmask;
	clip.start = start;
	clip.end = end;
	clip.mins = mins;
	clip.maxs = maxs;
	clip.passedict = passedict;

	VectorCopy (mins, clip.mins2);
	VectorCopy (maxs, clip.maxs2);
	
	/* create the bounding box of the entire move */
	SV_TraceBounds (start, clip.mins2, clip.maxs2, end, clip.boxmins, clip.boxmaxs);

	/* clip to other solid entities */
	SV_ClipMoveToEntities (&clip);

	return clip.trace;
}
