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
// cg_partthink.c
//

#include "cg_local.h"

/*
===============
pTrace
===============
*/
static trace_t pTrace (vec3_t start, vec3_t end, float size)
{
	return cgi.CM_Trace (start, end, size, 1);
}


/*
===============
pCalcPartVelocity
===============
*/
static void pCalcPartVelocity (particle_t *p, float scale, float *time, vec3_t velocity, float gravityScale)
{
	float time1 = *time;
	float time2 = time1*time1;

	velocity[0] = scale * (p->vel[0] * time1 + (p->accel[0]) * time2);
	velocity[1] = scale * (p->vel[1] * time1 + (p->accel[1]) * time2);
	velocity[2] = scale;

	if (p->flags & PF_GRAVITY)
		velocity[2] *= p->vel[2] * time1 + (p->accel[2]-(PART_GRAVITY * gravityScale)) * time2;
	else
		velocity[2] *= p->vel[2] * time1 + (p->accel[2]) * time2;
}


/*
===============
pClipVelocity
===============
*/
static void pClipVelocity (vec3_t in, vec3_t normal, vec3_t out)
{
	float	backoff;
	int		i;
	
	backoff = VectorLengthFast (in) * 0.25f + DotProduct (in, normal) * 3.0f;

	for (i=0 ; i<3 ; i++) {
		out[i] = in[i] - (normal[i] * backoff);
		if ((out[i] > -LARGE_EPSILON) && (out[i] < LARGE_EPSILON))
			out[i] = 0;
	}
}

/*
=============================================================================

	PARTICLE INTELLIGENCE

=============================================================================
*/

/*
===============
pBloodThink
===============
*/
void pBloodThink (struct particle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time)
{
	trace_t		tr;
	float		clipsize, sizescale;
	qBool		isGreen;
	static int	sfxDelay = -1;

	isGreen = (p->flags & PF_GREENBLOOD);
	p->thinkNext = qTrue;

	// make a decal
	clipsize = *size * 0.1f;
	if (clipsize<0.25) clipsize = 0.25f;
	tr = pTrace (p->oldOrigin, org, clipsize);

	if (tr.startSolid || tr.allSolid || tr.fraction < 1) {
		if (!(p->flags & PF_NODECAL)) {
			sizescale = clamp ((p->size < p->sizeVel) ? (p->sizeVel / *size) : (p->size / *size), 0.75f, 1.25f);

			makeDecal (
				org[0],							org[1],							org[2],
				tr.plane.normal[0],				tr.plane.normal[1],				tr.plane.normal[2],
				isGreen ? 30.0f : 255.0f,		isGreen ? 70.0f : 255.0f,		isGreen ? 30.0f : 255.0f,
				0,								0,								0,
				clamp (color[3]*3, 0, p->color[3]),
				0,
				(13 + (crand()*4)) * sizescale,
				isGreen ? dRandGrnBloodMark () : dRandBloodMark (),
				DF_ALPHACOLOR,
				0,								qFalse,
				0,								frand () * 360.0f);

			if (!(p->flags & PF_NOSFX) && cg.realTime > sfxDelay) {
				sfxDelay = cg.realTime + 300;
				cgi.Snd_StartSound (org, 0, CHAN_AUTO, cgMedia.gibSplatSfx[rand () % 3], 0.33f, ATTN_IDLE, 0);
			}
		}

		p->color[3] = 0;
		p->thinkNext = qFalse;
	}
}


/*
===============
pBloodDripThink
===============
*/
void pBloodDripThink (struct particle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time)
{
	float	length;

	pCalcPartVelocity (p, 0.4f, time, angle, *orient);

	length = VectorNormalizef (angle, angle);
	if (length > *orient)
		length = *orient;
	VectorScale (angle, -length, angle);

	pBloodThink (p, org, angle, color, size, orient, time);
}


/*
===============
pBounceThink
===============
*/
#define pBounceMaxVelocity 100
void pBounceThink (struct particle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time)
{
	float	clipsize, length;
	trace_t	tr;
	vec3_t	velocity;

	p->thinkNext = qTrue;

	clipsize = *size*0.5f;
	if (clipsize<0.25) clipsize = 0.25;
	tr = pTrace (p->oldOrigin, org, clipsize);

	if (tr.startSolid || tr.allSolid) {
		// don't fall through
		VectorCopy (tr.endPos, p->org);
		VectorCopy (p->org, p->oldOrigin);
		if (p->flags & PF_GRAVITY)
			p->flags &= ~PF_GRAVITY;
		VectorClear (p->vel);
		VectorClear (p->accel);
		p->thinkNext = qFalse;
		return;
	}

	if (tr.fraction < 1) {
		pCalcPartVelocity (p, 0.9f, time, velocity, 1);
		pClipVelocity (velocity, tr.plane.normal, p->vel);

		p->color[3]	= color[3];
		p->size		= *size;
		p->time		= (float)cg.realTime;

		VectorClear (p->accel);
		VectorCopy (tr.endPos, p->org);
		VectorCopy (p->org, org);
		VectorCopy (p->org, p->oldOrigin);

		if ((tr.plane.normal[2] > 0.6) && (VectorLengthFast (p->vel) < 2)) {
			if (p->flags & PF_GRAVITY)
				p->flags &= ~PF_GRAVITY;
			VectorClear (p->vel);
			// more realism; if they're moving they "cool down" faster than when settled
			if (p->colorVel[3] != PART_INSTANT)
				p->colorVel[3] *= 0.5;

			p->thinkNext = qFalse;
		}
	}

	length = VectorNormalizef (p->vel, p->vel);
	if (length > pBounceMaxVelocity)
		VectorScale (p->vel, pBounceMaxVelocity, p->vel);
	else
		VectorScale (p->vel, length, p->vel);
}


/*
===============
pDropletThink
===============
*/
void pDropletThink (struct particle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time)
{
	if (p->orient < 180)
		*orient -= cg.realTime*0.02f;
	else
		*orient += cg.realTime*0.02f;

	p->thinkNext = qTrue;
}


/*
===============
pExploAnimThink
===============
*/
void pExploAnimThink (struct particle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time)
{
	cgi.R_AddLight (org, 375 * ((color[3] / p->color[3]) + (frand () * 0.05f)), 1, 0.8f, 0.6f);

	if (color[3] > (p->color[3] * 0.95))
		p->shader = cgMedia.particleTable[PT_EXPLO1];
	else if (color[3] > (p->color[3] * 0.9))
		p->shader = cgMedia.particleTable[PT_EXPLO2];
	else if (color[3] > (p->color[3] * 0.8))
		p->shader = cgMedia.particleTable[PT_EXPLO3];
	else if (color[3] > (p->color[3] * 0.65))
		p->shader = cgMedia.particleTable[PT_EXPLO4];
	else if (color[3] > (p->color[3] * 0.3))
		p->shader = cgMedia.particleTable[PT_EXPLO5];
	else if (color[3] > (p->color[3] * 0.15))
		p->shader = cgMedia.particleTable[PT_EXPLO6];
	else
		p->shader = cgMedia.particleTable[PT_EXPLO7];

	p->thinkNext = qTrue;
}


/*
===============
pFastSmokeThink
===============
*/
void pFastSmokeThink (struct particle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time)
{
	if (p->orient < 180)
		*orient -= cg.realTime*0.02f;
	else
		*orient += cg.realTime*0.02f;

	p->thinkNext = qTrue;
}


/*
===============
pFireThink
===============
*/
void pFireThink (struct particle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time)
{
	*orient = (frand () * 360) * (color[3] * color[3]);

	p->thinkNext = qTrue;
}


/*
===============
pFireTrailThink
===============
*/
void pFireTrailThink (struct particle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time)
{
	float	length;

	pCalcPartVelocity (p, 10, time, angle, *orient);

	length = VectorNormalizef (angle, angle);
	if (length > *orient)
		length = *orient;
	VectorScale (angle, -length, angle);

	p->thinkNext = qTrue;
}


/*
===============
pFlareThink
===============
*/
void pFlareThink (struct particle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time)
{
	float	dist;

	dist = VectorDistFast (cg.refDef.viewOrigin, org);
	*orient = dist * 0.4f;

	if (p->flags & PF_SCALED)
		*size = clamp (*size * (dist / 1000.0f), *size, *size*10);
}


/*
===============
pRailSpiralThink
===============
*/
void pRailSpiralThink (struct particle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time)
{
	*orient += cg.realTime * 0.075f;

	p->thinkNext = qTrue;
}


/*
===============
pRicochetSparkThink
===============
*/
void pRicochetSparkThink (struct particle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time)
{
	float	length;

	pCalcPartVelocity (p, 6, time, angle, *orient);

	length = VectorNormalizef (angle, angle);
	if (length > *orient)
		length = *orient;
	VectorScale (angle, -length, angle);

	p->thinkNext = qTrue;
}


/*
===============
pSlowFireThink
===============
*/
void pSlowFireThink (struct particle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time)
{
	if (p->orient < 180)
		*orient -= cg.realTime*0.01f;
	else
		*orient += cg.realTime*0.01f;

	p->thinkNext = qTrue;
}


/*
===============
pSmokeThink
===============
*/
void pSmokeThink (struct particle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time)
{
	if (p->orient < 180)
		*orient -= cg.realTime*0.01f;
	else
		*orient += cg.realTime*0.01f;

	p->thinkNext = qTrue;
}


/*
===============
pSparkGrowThink
===============
*/
void pSparkGrowThink (struct particle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time)
{
	float length;

	pCalcPartVelocity (p, 6, time, angle, *orient);

	length = VectorNormalizef (angle, angle);
	if (length > *orient)
		length = *orient;
	VectorScale (angle, -length, angle);

	p->thinkNext = qTrue;
}


/*
===============
pSplashThink
===============
*/
void pSplashThink (struct particle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time)
{
	float	length;

	pCalcPartVelocity (p, 0.7f, time, angle, *orient);

	length = VectorNormalizef (angle, angle);
	if (length > *orient)
		length = *orient;
	VectorScale (angle, -length, angle);

	p->thinkNext = qTrue;
}
