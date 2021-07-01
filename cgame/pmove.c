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
// pmove.c
//

#include "cg_local.h"

// all of the locals will be zeroed before each pmove, just to make damn sure
// we don't have any differences when running on client or server

typedef struct pMoveLocal_s {
	vec3_t			origin;			// full float precision
	vec3_t			velocity;		// full float precision

	vec3_t			forward, right, up;
	float			frameTime;

	cBspSurface_t	*groundSurface;
	int				groundContents;

	svec3_t			previousOrigin;
	qBool			ladder;
} pMoveLocal_t;

static pMoveNew_t	*pm;
static pMoveLocal_t	pml;

static float	pmAirAcceleration = 0;

#define STEPSIZE			18
#define MIN_STEP_NORMAL		0.7f		// can't step up onto very steep slopes
#define MAX_CLIP_PLANES		5

// movement parameters
#define PM_STOPSPEED		100.0f
#define PM_MAXSPEED			300.0f
#define PM_DUCKSPEED		100.0f
#define PM_ACCELERATE		10.0f
#define PM_WATERACCELERATE	10.0f
#define PM_FRICTION			6.0f
#define PM_WATERFRICTION	1.0f
#define PM_WATERSPEED		400.0f

/*
==================
PM_ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
static void PM_ClipVelocity (vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	float	backoff;
	float	change;
	int		i;
	
	backoff = DotProduct (in, normal) * overbounce;

	for (i=0 ; i<3 ; i++) {
		change = normal[i]*backoff;
		out[i] = in[i] - change;
		if ((out[i] > -LARGE_EPSILON) && (out[i] < LARGE_EPSILON))
			out[i] = 0;
	}
}


/*
==================
PM_StepSlideMove

Each intersection will try to step over the obstruction instead of
sliding along it.

Returns a new origin, velocity, and contact entity
Does not modify any world state?

Returns qTrue if going onto a step.
==================
*/
static qBool PM_StepSlideMove_ (void)
{
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numPlanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity;
	int			i, j;
	trace_t		trace;
	vec3_t		end;
	float		time_left;
	qBool		step = qFalse;
	
	numbumps = 4;
	
	VectorCopy (pml.velocity, primal_velocity);
	numPlanes = 0;
	
	time_left = pml.frameTime;

	for (bumpcount=0 ; bumpcount<numbumps ; bumpcount++) {
		end[0] = pml.origin[0] + time_left * pml.velocity[0];
		end[1] = pml.origin[1] + time_left * pml.velocity[1];
		end[2] = pml.origin[2] + time_left * pml.velocity[2];

		trace = pm->trace (pml.origin, pm->mins, pm->maxs, end);

		if (trace.allSolid) {
			// Entity is trapped in another solid
			pml.velocity[2] = 0;	// Don't build up falling damage
			return 0;
		}

		if (trace.fraction > 0) {
			// Actually covered some distance
			VectorCopy (trace.endPos, pml.origin);
			numPlanes = 0;
		}

		if (trace.fraction == 1)
			 break;		// Moved the entire distance

		// Save entity for contact
		if (pm->numTouch < MAXTOUCH && trace.ent) {
			pm->touchEnts[pm->numTouch] = trace.ent;
			pm->numTouch++;
		}

		if (trace.plane.normal[2] < MIN_STEP_NORMAL)
			step = qTrue;	// Step
		
		time_left -= time_left * trace.fraction;

		// Slide along this plane
		if (numPlanes >= MAX_CLIP_PLANES) {
			// This shouldn't really happen
			VectorClear (pml.velocity);
			break;
		}

		VectorCopy (trace.plane.normal, planes[numPlanes]);
		numPlanes++;

		// modify original_velocity so it parallels all of the clip planes
		for (i=0 ; i<numPlanes ; i++) {
			PM_ClipVelocity (pml.velocity, planes[i], pml.velocity, 1.01f);
			for (j=0 ; j<numPlanes ; j++) {
				if (j != i) {
					if (DotProduct (pml.velocity, planes[j]) < 0)
						break;	// not ok
				}
			}
			if (j == numPlanes)
				break;
		}
		
		if (i != numPlanes) {
			// Go along this plane
		}
		else {
			// Go along the crease
			if (numPlanes != 2) {
				VectorClear (pml.velocity);
				break;
			}
			CrossProduct (planes[0], planes[1], dir);
			d = DotProduct (dir, pml.velocity);
			VectorScale (dir, d, pml.velocity);
		}

		/*
		** if velocity is against the original velocity, stop dead
		** to avoid tiny occilations in sloping corners
		*/
		if (DotProduct (pml.velocity, primal_velocity) <= 0) {
			VectorClear (pml.velocity);
			break;
		}
	}

	if (pm->state.pmTime)
		VectorCopy (primal_velocity, pml.velocity);

	return step;
}

static void PM_StepSlideMove (void)
{
	vec3_t		start_o, start_v;
	vec3_t		down_o, down_v;
	trace_t		trace;
	float		down_dist, up_dist;
	vec3_t		up, down;
	qBool		step;

	VectorCopy (pml.origin, start_o);
	VectorCopy (pml.velocity, start_v);

	step = PM_StepSlideMove_ ();

	VectorCopy (pml.origin, down_o);
	VectorCopy (pml.velocity, down_v);

	VectorCopy (start_o, up);
	up[2] += STEPSIZE;

	trace = pm->trace (up, pm->mins, pm->maxs, up);
	if (trace.allSolid)
		return;	// Can't step up

	// Try sliding above
	VectorCopy (up, pml.origin);
	VectorCopy (start_v, pml.velocity);

	PM_StepSlideMove_ ();

	// Push down the final amount
	VectorCopy (pml.origin, down);
	down[2] -= STEPSIZE;
	trace = pm->trace (pml.origin, pm->mins, pm->maxs, down);
	if (!trace.allSolid)
		VectorCopy (trace.endPos, pml.origin);

	VectorCopy (pml.origin, up);

	// Decide which one went farther
	down_dist = (down_o[0] - start_o[0])*(down_o[0] - start_o[0])
		+ (down_o[1] - start_o[1])*(down_o[1] - start_o[1]);
	up_dist = (up[0] - start_o[0])*(up[0] - start_o[0])
		+ (up[1] - start_o[1])*(up[1] - start_o[1]);

	if (down_dist > up_dist || trace.plane.normal[2] < MIN_STEP_NORMAL) {
		VectorCopy (down_o, pml.origin);
		VectorCopy (down_v, pml.velocity);
		return;
	}

	if (step || trace.plane.normal[2] == 1)
		pm->step = qTrue;

	// !! Special case !!
	// if we were walking along a plane, then we need to copy the Z over
	pml.velocity[2] = down_v[2];
}


/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
static void PM_Friction (void)
{
	float	*vel;
	float	speed, newspeed, control;
	float	friction;
	float	drop;
	
	vel = pml.velocity;
	
	speed = VectorLength (vel);
	if (speed < 1) {
		vel[0] = 0;
		vel[1] = 0;
		return;
	}

	drop = 0;

	// Apply ground friction
	if ((pm->groundEntity && pml.groundSurface && !(pml.groundSurface->flags & SURF_TEXINFO_SLICK)) || pml.ladder) {
		friction = PM_FRICTION;
		control = (speed < PM_STOPSPEED) ? PM_STOPSPEED : speed;
		drop += control*friction*pml.frameTime;
	}

	// Apply water friction
	if (pm->waterLevel && !pml.ladder)
		drop += speed*PM_WATERFRICTION*pm->waterLevel*pml.frameTime;

	// Scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0)
		newspeed = 0;

	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}


/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
static void PM_Accelerate (vec3_t wishdir, float wishspeed, float accel)
{
	float		addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct (pml.velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;

	accelspeed = accel*pml.frameTime*wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	pml.velocity[0] += accelspeed*wishdir[0];
	pml.velocity[1] += accelspeed*wishdir[1];
	pml.velocity[2] += accelspeed*wishdir[2];
}


/*
==============
PM_AirAccelerate

Handles user intended air acceleration
==============
*/
static void PM_AirAccelerate (vec3_t wishdir, float wishspeed, float accel)
{
	float		addspeed, accelspeed;
	float		currentspeed, wishspd = wishspeed;

	if (wishspd > 30)
		wishspd = 30;
	currentspeed = DotProduct (pml.velocity, wishdir);
	addspeed = wishspd - currentspeed;
	if (addspeed <= 0)
		return;

	accelspeed = accel * wishspeed * pml.frameTime;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	pml.velocity[0] += accelspeed*wishdir[0];
	pml.velocity[1] += accelspeed*wishdir[1];
	pml.velocity[2] += accelspeed*wishdir[2];	
}


/*
=============
PM_AddCurrents
=============
*/
static void PM_AddCurrents (vec3_t	wishvel)
{
	vec3_t	v;
	float	s;

	// Account for ladders
	if (pml.ladder && fabs (pml.velocity[2]) <= 200) {
		if ((pm->viewAngles[PITCH] <= -15) && (pm->cmd.forwardMove > 0))
			wishvel[2] = 200;
		else if ((pm->viewAngles[PITCH] >= 15) && (pm->cmd.forwardMove > 0))
			wishvel[2] = -200;
		else if (pm->cmd.upMove > 0)
			wishvel[2] = 200;
		else if (pm->cmd.upMove < 0)
			wishvel[2] = -200;
		else
			wishvel[2] = 0;

		// Limit horizontal speed when on a ladder
		if (wishvel[0] < -25)
			wishvel[0] = -25;
		else if (wishvel[0] > 25)
			wishvel[0] = 25;

		if (wishvel[1] < -25)
			wishvel[1] = -25;
		else if (wishvel[1] > 25)
			wishvel[1] = 25;
	}

	// Add water currents
	if (pm->waterType & MASK_CURRENT) {
		VectorClear (v);

		if (pm->waterType & CONTENTS_CURRENT_0)		v[0] += 1;
		if (pm->waterType & CONTENTS_CURRENT_90)	v[1] += 1;
		if (pm->waterType & CONTENTS_CURRENT_180)	v[0] -= 1;
		if (pm->waterType & CONTENTS_CURRENT_270)	v[1] -= 1;
		if (pm->waterType & CONTENTS_CURRENT_UP)	v[2] += 1;
		if (pm->waterType & CONTENTS_CURRENT_DOWN)	v[2] -= 1;

		s = PM_WATERSPEED;
		if ((pm->waterLevel == 1) && (pm->groundEntity))
			s /= 2;

		VectorMA (wishvel, s, v, wishvel);
	}

	// Add conveyor belt velocities
	if (pm->groundEntity) {
		VectorClear (v);

		if (pml.groundContents & CONTENTS_CURRENT_0)	v[0] += 1;
		if (pml.groundContents & CONTENTS_CURRENT_90)	v[1] += 1;
		if (pml.groundContents & CONTENTS_CURRENT_180)	v[0] -= 1;
		if (pml.groundContents & CONTENTS_CURRENT_270)	v[1] -= 1;
		if (pml.groundContents & CONTENTS_CURRENT_UP)	v[2] += 1;
		if (pml.groundContents & CONTENTS_CURRENT_DOWN)	v[2] -= 1;

		VectorMA (wishvel, 100 /* pm->groundEntity->speed */, v, wishvel);
	}
}


/*
===================
PM_WaterMove
===================
*/
static void PM_WaterMove (void)
{
	vec3_t	wishvel, wishdir;
	float	wishspeed;

	// User intentions
	wishvel[0] = pml.forward[0]*pm->cmd.forwardMove + pml.right[0]*pm->cmd.sideMove;
	wishvel[1] = pml.forward[1]*pm->cmd.forwardMove + pml.right[1]*pm->cmd.sideMove;
	wishvel[2] = pml.forward[2]*pm->cmd.forwardMove + pml.right[2]*pm->cmd.sideMove;

	if (!pm->cmd.forwardMove && !pm->cmd.sideMove && !pm->cmd.upMove)
		wishvel[2] -= 60;		// Drift towards bottom
	else
		wishvel[2] += pm->cmd.upMove;

	PM_AddCurrents (wishvel);

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalizef (wishdir, wishdir);

	if (wishspeed > PM_MAXSPEED) {
		VectorScale (wishvel, PM_MAXSPEED/wishspeed, wishvel);
		wishspeed = PM_MAXSPEED;
	}
	wishspeed *= 0.5f;

	PM_Accelerate (wishdir, wishspeed, PM_WATERACCELERATE);

	PM_StepSlideMove ();
}


/*
===================
PM_AirMove
===================
*/
static void PM_AirMove (void)
{
	vec3_t		wishvel, wishdir;
	float		fmove, smove;
	float		wishspeed;
	float		maxspeed;

	fmove = pm->cmd.forwardMove;
	smove = pm->cmd.sideMove;

	wishvel[0] = pml.forward[0]*fmove + pml.right[0]*smove;
	wishvel[1] = pml.forward[1]*fmove + pml.right[1]*smove;
	wishvel[2] = 0;

	PM_AddCurrents (wishvel);

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalizef (wishdir, wishdir);

	// Clamp to server defined max speed
	maxspeed = (pm->state.pmFlags & PMF_DUCKED) ? PM_DUCKSPEED : PM_MAXSPEED;

	if (wishspeed > maxspeed) {
		VectorScale (wishvel, maxspeed/wishspeed, wishvel);
		wishspeed = maxspeed;
	}
	
	if (pml.ladder) {
		PM_Accelerate (wishdir, wishspeed, PM_ACCELERATE);
		if (!wishvel[2]) {
			if (pml.velocity[2] > 0) {
				pml.velocity[2] -= pm->state.gravity * pml.frameTime;
				if (pml.velocity[2] < 0)
					pml.velocity[2]  = 0;
			}
			else {
				pml.velocity[2] += pm->state.gravity * pml.frameTime;
				if (pml.velocity[2] > 0)
					pml.velocity[2]  = 0;
			}
		}

		PM_StepSlideMove ();
	}
	else if (pm->groundEntity) {
		// Walking on ground
		pml.velocity[2] = 0; //!!! this is before the accel
		PM_Accelerate (wishdir, wishspeed, PM_ACCELERATE);

		if (pm->state.gravity > 0)
			pml.velocity[2] = 0;
		else
			pml.velocity[2] -= pm->state.gravity * pml.frameTime;

		if (!pml.velocity[0] && !pml.velocity[1])
			return;
		PM_StepSlideMove ();
	}
	else {
		// Not on ground, so little effect on velocity
		if (pmAirAcceleration)
			PM_AirAccelerate (wishdir, wishspeed, PM_ACCELERATE);
		else
			PM_Accelerate (wishdir, wishspeed, 1);

		// Add gravity
		pml.velocity[2] -= pm->state.gravity * pml.frameTime;
		PM_StepSlideMove ();
	}
}


/*
=============
PM_CatagorizePosition
=============
*/
static void PM_CatagorizePosition (void)
{
	vec3_t		point;
	int			cont;
	trace_t		trace;
	float		sample1;
	float		sample2;

	// If the player hull point one unit down is solid, the player is on ground
	// see if standing on something solid
	point[0] = pml.origin[0];
	point[1] = pml.origin[1];
	point[2] = pml.origin[2] - 0.25;

	if (pml.velocity[2] > 180) {
		pm->state.pmFlags &= ~PMF_ON_GROUND;
		pm->groundEntity = NULL;
	}
	else {
		trace = pm->trace (pml.origin, pm->mins, pm->maxs, point);
		pml.groundSurface = trace.surface;
		pml.groundContents = trace.contents;

		if (!trace.ent || (trace.plane.normal[2] < MIN_STEP_NORMAL && !trace.startSolid)) {
			pm->groundEntity = NULL;
			pm->state.pmFlags &= ~PMF_ON_GROUND;
		}
		else {
			pm->groundEntity = trace.ent;

			// Hitting solid ground will end a waterjump
			if (pm->state.pmFlags & PMF_TIME_WATERJUMP) {
				pm->state.pmFlags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_TELEPORT);
				pm->state.pmTime = 0;
			}

			if (!(pm->state.pmFlags & PMF_ON_GROUND)) {
				// Just hit the ground
				pm->state.pmFlags |= PMF_ON_GROUND;
				// Don't do landing time if we were just going down a slope
				if (pml.velocity[2] < -200 && !pm->strafeHack) {
					pm->state.pmFlags |= PMF_TIME_LAND;
					// Don't allow another jump for a little while
					if (pml.velocity[2] < -400)
						pm->state.pmTime = 25;
					else
						pm->state.pmTime = 18;
				}
			}
		}

		if (pm->numTouch < MAXTOUCH && trace.ent) {
			pm->touchEnts[pm->numTouch] = trace.ent;
			pm->numTouch++;
		}
	}

	// Get waterLevel, accounting for ducking
	pm->waterLevel = 0;
	pm->waterType = 0;

	sample2 = pm->viewHeight - pm->mins[2];
	sample1 = sample2 / 2;

	point[2] = pml.origin[2] + pm->mins[2] + 1;	
	cont = pm->pointContents (point);

	if (cont & MASK_WATER) {
		pm->waterType = cont;
		pm->waterLevel = 1;
		point[2] = pml.origin[2] + pm->mins[2] + sample1;
		cont = pm->pointContents (point);
		if (cont & MASK_WATER) {
			pm->waterLevel = 2;
			point[2] = pml.origin[2] + pm->mins[2] + sample2;
			cont = pm->pointContents (point);
			if (cont & MASK_WATER)
				pm->waterLevel = 3;
		}
	}
}


/*
=============
PM_CheckJump
=============
*/
static void PM_CheckJump (void)
{
	if (pm->state.pmFlags & PMF_TIME_LAND) {
		// Hasn't been long enough since landing to jump again
		return;
	}

	if (pm->cmd.upMove < 10) {
		// Not holding jump
		pm->state.pmFlags &= ~PMF_JUMP_HELD;
		return;
	}

	// Must wait for jump to be released
	if (pm->state.pmFlags & PMF_JUMP_HELD)
		return;

	if (pm->state.pmType == PMT_DEAD)
		return;

	if (pm->waterLevel >= 2) {
		// Swimming, not jumping
		pm->groundEntity = NULL;

		if (pml.velocity[2] <= -300)
			return;

		if (pm->waterType == CONTENTS_WATER)
			pml.velocity[2] = 100;
		else if (pm->waterType == CONTENTS_SLIME)
			pml.velocity[2] = 80;
		else
			pml.velocity[2] = 50;
		return;
	}

	if (pm->groundEntity == NULL)
		return;		// In air, so no effect

	pm->state.pmFlags |= PMF_JUMP_HELD;

	pm->groundEntity = NULL;
	pml.velocity[2] += 270;
	if (pml.velocity[2] < 270)
		pml.velocity[2] = 270;

	// This is so hacky my eyes hurt just looking at it
	if (!cgi.Cvar_GetIntegerValue ("dedicated") && glm_jumppred->value && !cgi.Com_ServerState () && cg.currGameMod == GAME_MOD_GLOOM) {
		switch (cg.gloomClassType) {
		case GLM_HATCHLING:	pml.velocity[2] *= 1.5f;	break;
		case GLM_DRONE:		pml.velocity[2] *= 1.4f;	break;
		case GLM_KAMIKAZE:	pml.velocity[2] *= 1.4f;	break;
		case GLM_STINGER:	pml.velocity[2] *= 1.35f;	break;
		case GLM_GUARDIAN:	pml.velocity[2] *= 1.2f;	break;
		case GLM_STALKER:	pml.velocity[2] *= 0.5f;	break;
		case GLM_BREEDER:	pml.velocity[2] *= 0.7f;	break;
		case GLM_HT:		pml.velocity[2] *= 0.8f;	break;
		case GLM_COMMANDO:	pml.velocity[2] *= 1.2f;	break;
		case GLM_EXTERM:	pml.velocity[2] *= 0.9f;	break;
		case GLM_MECH:		pml.velocity[2] *= 0.4f;	break;
		case GLM_WRAITH:	pml.velocity[2] *= 1.4f;	break;

		case GLM_GRUNT:
		case GLM_ST:
		case GLM_ENGINEER:
		case GLM_BIOTECH:
		case GLM_DEFAULT:
		case GLM_OBSERVER:
		default:
			break;
		}
	}
}


/*
=============
PM_CheckSpecialMovement
=============
*/
static void PM_CheckSpecialMovement (void)
{
	vec3_t	spot;
	int		cont;
	vec3_t	flatforward;
	trace_t	trace;

	if (pm->state.pmTime)
		return;

	pml.ladder = qFalse;

	// Check for ladder
	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	VectorNormalizef (flatforward, flatforward);

	VectorMA (pml.origin, 1, flatforward, spot);
	trace = pm->trace (pml.origin, pm->mins, pm->maxs, spot);
	if ((trace.fraction < 1) && (trace.contents & CONTENTS_LADDER))
		pml.ladder = qTrue;

	// Check for water jump
	if (pm->waterLevel != 2)
		return;

	VectorMA (pml.origin, 30, flatforward, spot);
	spot[2] += 4;
	cont = pm->pointContents (spot);
	if (!(cont & CONTENTS_SOLID))
		return;

	spot[2] += 16;
	cont = pm->pointContents (spot);
	if (cont)
		return;

	// Jump out of water
	VectorScale (flatforward, 50, pml.velocity);
	pml.velocity[2] = 350;

	pm->state.pmFlags |= PMF_TIME_WATERJUMP;
	pm->state.pmTime = 255;
}


/*
===============
PM_FlyMove
===============
*/
static void PM_FlyMove (qBool doClip)
{
	float	speed, drop, friction, control, newspeed;
	float	currentspeed, addspeed, accelspeed;
	vec3_t	wishvel, wishdir, end;
	float	fmove, smove, wishspeed;
	trace_t	trace;

	pm->viewHeight = 22;

	// friction
	speed = VectorLength (pml.velocity);
	if (speed < 1)
		VectorClear (pml.velocity);
	else {
		drop = 0;

		friction = PM_FRICTION*1.5;	// extra friction
		control = (speed < PM_STOPSPEED) ? PM_STOPSPEED : speed;
		drop += control*friction*pml.frameTime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		VectorScale (pml.velocity, newspeed, pml.velocity);
	}

	// accelerate
	fmove = pm->cmd.forwardMove;
	smove = pm->cmd.sideMove;

	VectorNormalizef (pml.forward, pml.forward);
	VectorNormalizef (pml.right, pml.right);

	wishvel[0] = pml.forward[0]*fmove + pml.right[0]*smove;
	wishvel[1] = pml.forward[1]*fmove + pml.right[1]*smove;
	wishvel[2] = pml.forward[2]*fmove + pml.right[2]*smove + pm->cmd.upMove;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalizef (wishdir, wishdir);

	// clamp to server defined max speed
	if (wishspeed > PM_MAXSPEED) {
		VectorScale (wishvel, PM_MAXSPEED/wishspeed, wishvel);
		wishspeed = PM_MAXSPEED;
	}

	currentspeed = DotProduct (pml.velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;

	accelspeed = PM_ACCELERATE*pml.frameTime*wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	pml.velocity[0] += accelspeed*wishdir[0];
	pml.velocity[1] += accelspeed*wishdir[1];
	pml.velocity[2] += accelspeed*wishdir[2];	

	if (doClip) {
		end[0] = pml.origin[0] + pml.frameTime * pml.velocity[0];
		end[1] = pml.origin[1] + pml.frameTime * pml.velocity[1];
		end[2] = pml.origin[2] + pml.frameTime * pml.velocity[2];

		trace = pm->trace (pml.origin, pm->mins, pm->maxs, end);

		VectorCopy (trace.endPos, pml.origin);
	}
	else {
		// move
		VectorMA (pml.origin, pml.frameTime, pml.velocity, pml.origin);
	}
}


/*
==============
PM_CheckDuck

Sets mins, maxs, and pm->viewHeight
==============
*/
static void PM_CheckDuck (void)
{
	trace_t	trace;

	pm->mins[0] = -16;
	pm->mins[1] = -16;

	pm->maxs[0] = 16;
	pm->maxs[1] = 16;

	if (pm->state.pmType == PMT_GIB) {
		pm->mins[2] = 0;
		pm->maxs[2] = 16;
		pm->viewHeight = 8;
		return;
	}

	pm->mins[2] = -24;

	if (pm->state.pmType == PMT_DEAD) {
		pm->state.pmFlags |= PMF_DUCKED;
	}
	else if (pm->cmd.upMove < 0 && (pm->state.pmFlags & PMF_ON_GROUND)) {
		// duck
		pm->state.pmFlags |= PMF_DUCKED;
	}
	else {
		// stand up if possible
		if (pm->state.pmFlags & PMF_DUCKED) {
			// try to stand up
			pm->maxs[2] = 32;
			trace = pm->trace (pml.origin, pm->mins, pm->maxs, pml.origin);
			if (!trace.allSolid)
				pm->state.pmFlags &= ~PMF_DUCKED;
		}
	}

	if (pm->state.pmFlags & PMF_DUCKED) {
		pm->maxs[2] = 4;
		pm->viewHeight = -2;
	}
	else {
		pm->maxs[2] = 32;
		pm->viewHeight = 22;
	}
}


/*
==============
PM_DeadMove
==============
*/
static void PM_DeadMove (void)
{
	float	forward;

	if (!pm->groundEntity)
		return;

	// extra friction
	forward = VectorLength (pml.velocity);
	forward -= 20;
	if (forward <= 0)
		VectorClear (pml.velocity);
	else {
		VectorNormalizef (pml.velocity, pml.velocity);
		VectorScale (pml.velocity, forward, pml.velocity);
	}
}


/*
================
PM_GoodPosition
================
*/
static qBool PM_GoodPosition (void)
{
	trace_t	trace;
	vec3_t	origin, end;

	if (pm->state.pmType == PMT_SPECTATOR)
		return qTrue;

	origin[0] = end[0] = pm->state.origin[0]*(1.0f/8.0f);
	origin[1] = end[1] = pm->state.origin[1]*(1.0f/8.0f);
	origin[2] = end[2] = pm->state.origin[2]*(1.0f/8.0f);
	trace = pm->trace (origin, pm->mins, pm->maxs, end);

	return !trace.allSolid;
}


/*
================
PM_SnapPosition

On exit, the origin will have a value that is pre-quantized to the (1.0f/8.0f)
precision of the network channel and in a valid position.
================
*/
static void PM_SnapPosition (void)
{
	int		sign[3];
	int		i, j, bits;
	int16	base[3];
	// try all single bits first
	static int jitterbits[8] = {0,4,1,2,3,5,6,7};

	// snap velocity to eigths
	pm->state.velocity[0] = (int)(pml.velocity[0]*8);
	pm->state.velocity[1] = (int)(pml.velocity[1]*8);
	pm->state.velocity[2] = (int)(pml.velocity[2]*8);

	for (i=0 ; i<3 ; i++) {
		if (pml.origin[i] >= 0)
			sign[i] = 1;
		else 
			sign[i] = -1;
		pm->state.origin[i] = (int)(pml.origin[i]*8);
		if (pm->state.origin[i]*(1.0f/8.0f) == pml.origin[i])
			sign[i] = 0;
	}
	VectorCopy (pm->state.origin, base);

	// try all combinations
	for (j=0 ; j<8 ; j++) {
		bits = jitterbits[j];
		VectorCopy (base, pm->state.origin);
		for (i=0 ; i<3 ; i++)
			if (bits & (1<<i))
				pm->state.origin[i] += sign[i];

		if (PM_GoodPosition ())
			return;
	}

	// go back to the last position
	VectorCopy (pml.previousOrigin, pm->state.origin);
}


/*
================
PM_InitialSnapPosition
================
*/
static void PM_InitialSnapPosition (void)
{
	int			x, y, z;
	int16		base[3];
	static int	offset[3] = { 0, -1, 1 };

	VectorCopy (pm->state.origin, base);

	for (z=0 ; z<3 ; z++) {
		pm->state.origin[2] = base[2] + offset[z];
		for (y=0 ; y<3 ; y++) {
			pm->state.origin[1] = base[1] + offset[y];
			for (x=0 ; x<3 ; x++) {
				pm->state.origin[0] = base[0] + offset[x];
				if (PM_GoodPosition ()) {
					pml.origin[0] = pm->state.origin[0]*(1.0f/8.0f);
					pml.origin[1] = pm->state.origin[1]*(1.0f/8.0f);
					pml.origin[2] = pm->state.origin[2]*(1.0f/8.0f);
					VectorCopy (pm->state.origin, pml.previousOrigin);
					return;
				}
			}
		}
	}

	Com_DevPrintf (PRNT_WARNING, "Bad InitialSnapPosition\n");
}


/*
================
PM_ClampAngles
================
*/
static void PM_ClampAngles (void)
{
	int16	temp;
	int		i;

	if (pm->state.pmFlags & PMF_TIME_TELEPORT) {
		pm->viewAngles[YAW] = SHORT2ANGLE (pm->cmd.angles[YAW] + pm->state.deltaAngles[YAW]);
		pm->viewAngles[PITCH] = 0;
		pm->viewAngles[ROLL] = 0;
	}
	else {
		// circularly clamp the angles with deltas
		for (i=0 ; i<3 ; i++) {
			temp = pm->cmd.angles[i] + pm->state.deltaAngles[i];
			pm->viewAngles[i] = SHORT2ANGLE(temp);
		}

		// don't let the player look up or down more than 90 degrees
		if ((pm->viewAngles[PITCH] > 89) && (pm->viewAngles[PITCH] < 180))
			pm->viewAngles[PITCH] = 89;
		else if ((pm->viewAngles[PITCH] < 271) && (pm->viewAngles[PITCH] >= 180))
			pm->viewAngles[PITCH] = 271;
	}

	Angles_Vectors (pm->viewAngles, pml.forward, pml.right, pml.up);
}


/*
================
Pmove

Can be called by either the server or the client
================
*/
void Pmove (pMoveNew_t *pMove, float airAcceleration)
{
	pm = pMove;
	pmAirAcceleration = airAcceleration;

	// clear results
	pm->numTouch = 0;
	VectorClear (pm->viewAngles);
	pm->viewHeight = 0;
	pm->groundEntity = 0;
	pm->waterType = 0;
	pm->waterLevel = 0;
	pm->step = qFalse;

	// clear all pmove local vars
	memset (&pml, 0, sizeof (pml));

	// convert origin and velocity to float values
	pml.origin[0] = pm->state.origin[0]*(1.0f/8.0f);
	pml.origin[1] = pm->state.origin[1]*(1.0f/8.0f);
	pml.origin[2] = pm->state.origin[2]*(1.0f/8.0f);

	pml.velocity[0] = pm->state.velocity[0]*(1.0f/8.0f);
	pml.velocity[1] = pm->state.velocity[1]*(1.0f/8.0f);
	pml.velocity[2] = pm->state.velocity[2]*(1.0f/8.0f);

	// save old org in case we get stuck
	VectorCopy (pm->state.origin, pml.previousOrigin);

	pml.frameTime = pm->cmd.msec * 0.001;

	PM_ClampAngles ();

	if (pm->state.pmType == PMT_SPECTATOR) {
		pml.frameTime *= pm->multiplier;
		PM_FlyMove (qFalse);
		PM_SnapPosition ();
		return;
	}

	if (pm->state.pmType >= PMT_DEAD) {
		pm->cmd.forwardMove = 0;
		pm->cmd.sideMove = 0;
		pm->cmd.upMove = 0;
	}

	if (pm->state.pmType == PMT_FREEZE)
		return;		// no movement at all

	// set mins, maxs, and viewHeight
	PM_CheckDuck ();

	if (pm->snapInitial)
		PM_InitialSnapPosition ();

	// set groundEntity, waterType, and waterLevel
	PM_CatagorizePosition ();

	if (pm->state.pmType == PMT_DEAD)
		PM_DeadMove ();

	PM_CheckSpecialMovement ();

	// drop timing counter
	if (pm->state.pmTime) {
		int		msec;

		msec = pm->cmd.msec >> 3;
		if (!msec)
			msec = 1;
		if (msec >= pm->state.pmTime) {
			pm->state.pmFlags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_TELEPORT);
			pm->state.pmTime = 0;
		}
		else
			pm->state.pmTime -= msec;
	}

	if (pm->state.pmFlags & PMF_TIME_TELEPORT) {
		// teleport pause stays exactly in place
	}
	else if (pm->state.pmFlags & PMF_TIME_WATERJUMP) {
		// waterjump has no control, but falls
		pml.velocity[2] -= pm->state.gravity * pml.frameTime;
		if (pml.velocity[2] < 0) {
			// cancel as soon as we are falling down again
			pm->state.pmFlags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_TELEPORT);
			pm->state.pmTime = 0;
		}

		PM_StepSlideMove ();
	}
	else {
		PM_CheckJump ();

		PM_Friction ();

		if (pm->waterLevel >= 2)
			PM_WaterMove ();
		else {
			vec3_t	angles;

			VectorCopy(pm->viewAngles, angles);
			if (angles[PITCH] > 180)
				angles[PITCH] = angles[PITCH] - 360;
			angles[PITCH] /= 3;

			Angles_Vectors (angles, pml.forward, pml.right, pml.up);

			PM_AirMove ();
		}
	}

	// set groundEntity, waterType, and waterLevel for final spot
	PM_CatagorizePosition ();

	PM_SnapPosition ();
}
