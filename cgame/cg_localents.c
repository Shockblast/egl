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
// cg_localents.c
// Local entities. Things like bullet casings...
//

#include "cg_local.h"

typedef struct localEnt_s {
	struct localEnt_s	*next;
	struct localEnt_s	*prev;

	int					time;
	leType_t			type;

	refEntity_t			refEnt;

	vec3_t				angles;
	vec3_t				avel;

	vec3_t				velocity;
	vec3_t				mins, maxs;

	vec3_t				pubParms;
	vec3_t				privFloatParms;
	qBool				privBoolParms[3];
	int					privIntParms[3];

	qBool				remove;
} localEnt_t;

static localEnt_t	*cg_freeLEnts;
static localEnt_t	cg_leHeadNode, cg_leList[MAX_LENTS];
static int			cg_numLEnts;

/*
=============================================================================

	LOCAL ENTITY THINKING

=============================================================================
*/

/*
===============
LE_ClipVelocity
===============
*/
#define	STOP_EPSILON	0.1f
static int LE_ClipVelocity (vec3_t in, vec3_t normal, vec3_t out, float overBounce)
{
	float		backOff, change;
	int			blocked, i;

	if (normal[2] >= 0)
		blocked = 1;	// Floor
	else
		blocked = 0;
	if (!normal[2])
		blocked |= 2;	// Step

	backOff = DotProduct(in, normal) * overBounce;
	for (i=0 ; i<3 ; i++) {
		change = normal[i] * backOff;
		out[i] = in[i] - change;
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
	}

	return blocked;
}


/*
===============
LE_ColorUpdate
===============
*/
static void LE_ColorUpdate (localEnt_t *le)
{
	vec3_t	color;

	cgi.R_LightPoint (le->refEnt.origin, color);
	ColorNormalizeb (color, le->refEnt.color);
}


/*
===============
LE_BrassThink

pubParms0: gravity

privFloatParms[0-2]: last angle (used when privIntParms[0] is >= 1)
privIntParms[0]: 0 = run, 1-3 = interpolate, 4 = set angle, 5 = rest
===============
*/
static void LE_BrassThink (localEnt_t *le)
{
	trace_t		tr;
	float		timeSq;
	vec3_t		normal, perpNormal;
	vec3_t		perpPos;
	vec3_t		angles;

	// Check if time is up
	if (cg.realTime >= le->time+(cg_brassTime->floatVal*1000)) {
		le->remove = qTrue;
		return;
	}

	// Check if we stopped moving
	switch (le->privIntParms[0]) {
	case 0:
		// Run a frame
		timeSq = (cg.realTime - le->time) * 0.001f;
		timeSq *= timeSq;

		// Copy origin to old
		Vec3Copy (le->refEnt.origin, le->refEnt.oldOrigin);

		// Move angles
		Vec3MA (le->angles, cg.refreshFrameTime, le->avel, le->angles);
		Angles_Matrix3 (le->angles, le->refEnt.axis);

		// Scale velocity and add gravity
		le->refEnt.origin[0] += le->velocity[0] * cg.refreshFrameTime;
		le->refEnt.origin[1] += le->velocity[1] * cg.refreshFrameTime;
		le->refEnt.origin[2] += le->velocity[2] * cg.refreshFrameTime;
		le->refEnt.origin[2] -= le->pubParms[0] * timeSq;

		// Check for collision
		CG_PMTrace (&tr, le->refEnt.oldOrigin, le->mins, le->maxs, le->refEnt.origin, qFalse, qFalse);
		if (tr.allSolid || tr.startSolid) {
			le->remove = qTrue;
			return;
		}

		if (tr.fraction != 1.0f) {
			Vec3Copy (tr.endPos, le->refEnt.origin);

			// Check if it's time to stop
			LE_ClipVelocity (le->velocity, tr.plane.normal, le->velocity, 1.0f + (frand () * 0.5f));
			if (tr.plane.normal[2] > 0.7f) {
				// Play crash sound
				if (le->type == LE_SGSHELL)
					cgi.Snd_StartSound (tr.endPos, 0, 0, cgMedia.sfx.sgShell[(rand()&1)], 1, ATTN_NORM, 0);
				else
					cgi.Snd_StartSound (tr.endPos, 0, 0, cgMedia.sfx.mgShell[(rand()&1)], 1, ATTN_NORM, 0);

				// Store current angles
				Vec3Copy (le->angles, le->privFloatParms);

				// Orient flat and rotate randomly on the plane
				VectorNormalizef (tr.plane.normal, normal);
				PerpendicularVector (normal, perpNormal);
				ProjectPointOnPlane (perpPos, tr.endPos, perpNormal);
				RotatePointAroundVector (le->angles, perpNormal, perpPos, frand()*360);

				// Found our new home
				le->privIntParms[0]++;
			}
		}
		break;

	case 1:
	case 2:
	case 3:
		// Interpolate between last and final
		angles[0] = LerpAngle (le->angles[0], le->privFloatParms[0], 0.25f * le->privIntParms[0]);
		angles[1] = LerpAngle (le->angles[1], le->privFloatParms[1], 0.25f * le->privIntParms[0]);
		angles[2] = LerpAngle (le->angles[2], le->privFloatParms[2], 0.25f * le->privIntParms[0]);
		Angles_Matrix3 (angles, le->refEnt.axis);
		le->privIntParms[0]++;
		break;

	case 4:
		// Set final
		Angles_Matrix3 (le->angles, le->refEnt.axis);
		le->privIntParms[0]++;
		break;

	case 5:
		// Rest
		break;

	default:
		assert (0);
		break;
	}

	// Update color
	LE_ColorUpdate (le);
}

/*
=============================================================================

	LOCAL ENTITY MANAGEMENT

=============================================================================
*/

/*
===============
CG_AllocLEnt
===============
*/
static localEnt_t *CG_AllocLEnt (void)
{
	localEnt_t	*le;

	// Take a free particle spot if possible, otherwise steal the oldest one
	if (cg_freeLEnts) {
		le = cg_freeLEnts;
		cg_freeLEnts = le->next;
	}
	else {
		le = cg_leHeadNode.prev;
		le->prev->next = le->next;
		le->next->prev = le->prev;

		cg_numLEnts--;
	}

	// Move to the beginning of the list
	le->prev = &cg_leHeadNode;
	le->next = cg_leHeadNode.next;
	le->next->prev = le;
	le->prev->next = le;

	cg_numLEnts++;
	return le;
}


/*
===============
CG_FreeLEnt
===============
*/
static inline void CG_FreeLEnt (localEnt_t *le)
{
	// Remove from linked active list
	le->prev->next = le->next;
	le->next->prev = le->prev;

	// Insert into linked free list
	le->next = cg_freeLEnts;
	cg_freeLEnts = le;

	cg_numLEnts--;
}


/*
===============
CG_SpawnLocalEnt
===============
*/
qBool CG_SpawnLocalEnt (float org0,						float org1,						float org2,
						float vel0,						float vel1,						float vel2,
						float angle0,					float angle1,					float angle2,
						float avel0,					float avel1,					float avel2,
						float parm0,					float parm1,					float parm2,
						leType_t type)
{
	localEnt_t	*le;

	// Allocate an entity and store values
	le = CG_AllocLEnt ();
	le->time = cg.realTime;
	Vec3Set (le->refEnt.origin, org0, org1, org2);
	Vec3Set (le->refEnt.oldOrigin, org0, org1, org2);
	Vec4Set (le->refEnt.color, 255, 255, 255, 255);

	Vec3Set (le->angles, angle0, angle1, angle2);
	Vec3Set (le->avel, avel0, avel1, avel2);
	Angles_Matrix3 (le->angles, le->refEnt.axis);

	Vec3Set (le->velocity, vel0, vel1, vel2);
	Vec3Set (le->pubParms, parm0, parm1, parm2);

	Vec3Clear (le->privFloatParms);
	le->privBoolParms[0] = le->privBoolParms[1] = le->privBoolParms[2] = qFalse;
	le->privIntParms[0] = le->privIntParms[1] = le->privIntParms[2] = 0;

	// Set up the refEnt
	le->refEnt.flags = RF_NOSHADOW;
	le->type = type;
	switch (type) {
	case LE_MGSHELL:
		le->refEnt.model = cgMedia.brassMGModel;
		le->refEnt.flags |= RF_FULLBRIGHT;
		break;

	case LE_SGSHELL:
		le->refEnt.model = cgMedia.brassSGModel;
		le->refEnt.flags |= RF_FULLBRIGHT;
		break;
	}
	le->refEnt.scale = 1.0f;
	cgi.R_ModelBounds (le->refEnt.model, le->mins, le->maxs);

	le->remove = !(le->refEnt.model);

	return qTrue;
}


/*
===============
CG_ClearLocalEnts
===============
*/
void CG_ClearLocalEnts (void)
{
	int		i;

	// Link particles
	cg_freeLEnts = &cg_leList[0];
	cg_leHeadNode.prev = &cg_leHeadNode;
	cg_leHeadNode.next = &cg_leHeadNode;
	for (i=0 ; i<MAX_LENTS-1 ; i++)
		cg_leList[i].next = &cg_leList[i+1];
	cg_leList[MAX_LENTS-1].next = NULL;
}


/*
===============
CG_AddLocalEnts
===============
*/
void CG_AddLocalEnts (void)
{
	localEnt_t	*le, *next, *hNode;

	hNode = &cg_leHeadNode;
	for (le=hNode->prev ; le!=hNode ; le=next) {
		next = le->prev;

		// Run physics and other per-frame things
		switch (le->type) {
		case LE_MGSHELL:
		case LE_SGSHELL:
			LE_BrassThink (le);
			break;
		}

		// Remove if desired
		if (le->remove) {
			CG_FreeLEnt (le);
			continue;
		}

		// Add to refresh
		cgi.R_AddEntity (&le->refEnt);
	}
}
