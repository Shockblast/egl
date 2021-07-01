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

trace_t pTrace (vec3_t start, vec3_t end, float size) {
	return cgi.CM_Trace (start, end, size, 1);
}

void pCalcPartVelocity (cgParticle_t *p, double scale, double *time, vec3_t velocity, double gravscale) {
	double time1 = *time;
	double time2 = time1*time1;

	velocity[0] = scale * (p->vel[0] * time1 + (p->accel[0]) * time2);
	velocity[1] = scale * (p->vel[1] * time1 + (p->accel[1]) * time2);
	velocity[2] = scale;

	if (p->flags & PF_GRAVITY)
		velocity[2] *= p->vel[2] * time1 + (p->accel[2]-(PART_GRAVITY * gravscale)) * time2;
	else
		velocity[2] *= p->vel[2] * time1 + (p->accel[2]) * time2;
}

void pClipVelocity (vec3_t in, vec3_t normal, vec3_t out) {
	double	backoff, change;
	int		i;
	
	backoff = VectorLengthFast (in) * 0.25 + DotProduct (in, normal) * 3.0f;

	for (i=0 ; i<3 ; i++) {
		change = normal[i] * backoff;
		out[i] = in[i] - change;
		if ((out[i] > -LARGE_EPSILON) && (out[i] < LARGE_EPSILON))
			out[i] = 0;
	}
}

/*
==============================================================

	PARTICLE INTELLIGENCE

==============================================================
*/

void pBloodThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time) {
	trace_t		tr;
	double		clipsize, sizescale;

	p->thinkNext = qTrue;

	// make a decal
	clipsize = *size*0.1;
	if (clipsize<0.25) clipsize = 0.25;
	tr = pTrace (p->oldOrigin, org, clipsize);

	if (tr.fraction < 1) {
		if (!(p->flags & PF_NODECAL)) {
			sizescale = clamp ((p->size < p->sizeVel) ? (p->sizeVel / *size) : (p->size / *size), 0.75, 1.25);

			makeDecal (
				org[0],							org[1],							org[2],
				tr.plane.normal[0],				tr.plane.normal[1],				tr.plane.normal[2],
				255,							255,							255,
				0,								0,								0,
				clamp (color[3]*3,0,p->color[3]),0,
				(13 + (crand()*3)) * sizescale,	0,
				dRandBloodMark (),				DF_ALPHACOLOR,
				p->sFactor,						p->dFactor,
				0,								qFalse,
				0,								frand () * 360);
		}

		p->color[3] = 0;
		p->thinkNext = qFalse;
	}
}

void pBloodDripThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time) {
	double	length;

	pCalcPartVelocity (p, 0.33, time, angle, *orient);

	length = VectorNormalize (angle, angle);
	if (length > *orient)
		length = *orient;
	VectorScale (angle, -length, angle);

	pBloodThink (p, org, angle, color, size, orient, time);
}

#define pBounceMaxVelocity 100
void pBounceThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time) {
	double	clipsize, length;
	trace_t	tr;
	vec3_t	velocity;

	p->thinkNext = qTrue;

	clipsize = *size*0.5;
	if (clipsize<0.25) clipsize = 0.25;
	tr = pTrace (p->oldOrigin, org, clipsize);

	if (tr.fraction < 1) {
		pCalcPartVelocity (p, 0.9, time, velocity, 1);
		pClipVelocity (velocity, tr.plane.normal, p->vel);

		p->color[3]	= color[3];
		p->size		= *size;
		p->time		= cgs.realTime;

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

	length = VectorNormalize (p->vel, p->vel);
	if (length > pBounceMaxVelocity)
		VectorScale (p->vel, pBounceMaxVelocity, p->vel);
	else
		VectorScale (p->vel, length, p->vel);
}

void pExploAnimThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time) {
	cgi.R_AddLight (org, 375 * ((color[3] / p->color[3]) + (frand () * 0.05)), 1, 0.8, 0.6);

	if (color[3] > (p->color[3] * 0.95))
		p->image = cgMedia.mediaTable[PT_EXPLO1];
	else if (color[3] > (p->color[3] * 0.9))
		p->image = cgMedia.mediaTable[PT_EXPLO2];
	else if (color[3] > (p->color[3] * 0.8))
		p->image = cgMedia.mediaTable[PT_EXPLO3];
	else if (color[3] > (p->color[3] * 0.65))
		p->image = cgMedia.mediaTable[PT_EXPLO4];
	else if (color[3] > (p->color[3] * 0.3))
		p->image = cgMedia.mediaTable[PT_EXPLO5];
	else if (color[3] > (p->color[3] * 0.15))
		p->image = cgMedia.mediaTable[PT_EXPLO6];
	else
		p->image = cgMedia.mediaTable[PT_EXPLO7];

	p->thinkNext = qTrue;
}

void pExploWaveThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time) {
	double	length;

	pCalcPartVelocity (p, *orient, time, angle, *orient);

	length = VectorNormalize (angle, angle);
	if (length > *orient)
		length = *orient;
	VectorScale (angle, -length, angle);

	p->thinkNext = qTrue;
}

void pFastSmokeThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time) {
	if (p->orient < 180)
		*orient -= cgs.realTime*0.02;
	else
		*orient += cgs.realTime*0.02;

	p->thinkNext = qTrue;
}

void pFireThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time) {
	*orient = (frand () * 360) * (color[3] * color[3]);

	p->thinkNext = qTrue;
}

void pFireTrailThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time) {
	double	length;

	pCalcPartVelocity (p, 10, time, angle, *orient);

	length = VectorNormalize (angle, angle);
	if (length > *orient)
		length = *orient;
	VectorScale (angle, -length, angle);

	p->thinkNext = qTrue;
}

void pFlareThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time) {
	float	dist;

	dist = VectorDistanceFast (cg.refDef.viewOrigin, org);
	*orient = dist * 0.4;

	if (p->flags & PF_SCALED)
		*size = clamp (*size * (dist / 1000.0), *size, *size*10);
}

void pRailSpiralThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time) {
	*orient += cgs.realTime * 0.075;

	p->thinkNext = qTrue;
}

void pRicochetSparkThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time) {
	double	length;

	pCalcPartVelocity (p, 6, time, angle, *orient);

	length = VectorNormalize (angle, angle);
	if (length > *orient)
		length = *orient;
	VectorScale (angle, -length, angle);

	p->thinkNext = qTrue;
}

void pSlowFireThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time) {
	if (p->orient < 180)
		*orient -= cgs.realTime*0.01;
	else
		*orient += cgs.realTime*0.01;

	p->thinkNext = qTrue;
}

void pSmokeThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time) {
	if (p->orient < 180)
		*orient -= cgs.realTime*0.01;
	else
		*orient += cgs.realTime*0.01;

	p->thinkNext = qTrue;
}

void pSparkGrowThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time) {
	double length;

	pCalcPartVelocity (p, 6, time, angle, *orient);

	length = VectorNormalize (angle, angle);
	if (length > *orient)
		length = *orient;
	VectorScale (angle, -length, angle);

	p->thinkNext = qTrue;
}

void pSplashThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time) {
	double	length;

	pCalcPartVelocity (p, 0.7, time, angle, *orient);

	length = VectorNormalize (angle, angle);
	if (length > *orient)
		length = *orient;
	VectorScale (angle, -length, angle);

	p->thinkNext = qTrue;
}

void pSplashPlumeThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time) {
	double	length;

	pCalcPartVelocity (p, 2, time, angle, *orient);

	length = VectorNormalize (angle, angle);
	if (length > *orient)
		length = *orient;
	VectorScale (angle, -length, angle);

	p->thinkNext = qTrue;
}

void pSmokeTrailThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time) {
	if (p->orient < 180)
		*orient -= cgs.realTime*0.02;
	else
		*orient += cgs.realTime*0.02;

	if (color[3] > (p->color[3]*0.5))
		color[3] = (color[3] - p->color[3]) * -1;

	p->thinkNext = qTrue;
}
