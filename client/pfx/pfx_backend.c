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

// pfx_backend.c

#include "../cl_local.h"

/*
==============================================================

	PARTICLE IMAGING

==============================================================
*/

int pSmokeCnt		= 2;
int pSmokeTable[]	= { PT_SMOKE, PT_SMOKE2 };
int pRandSmoke (void)	{ return pSmokeTable[(rand()%pSmokeCnt)]; }

/*
==============================================================

	PARTICLE MANAGEMENT

==============================================================
*/

cparticle_t	*cl_ActiveParticles;
cparticle_t	*cl_FreeParticles;
cparticle_t	cl_Particles[MAX_PARTICLES];

/*
===============
makePart
===============
*/
void FASTCALL makePart (double org0,					double org1,					double org2,
						double angle0,					double angle1,					double angle2,
						double vel0,					double vel1,					double vel2,
						double accel0,					double accel1,					double accel2,
						double red,						double green,					double blue,
						double redVel,					double greenVel,				double blueVel,
						double alpha,					double alphaVel,
						double size,					double sizeVel,
						int type,						int flags,
						int sFactor,					int dFactor,
						void (*think)(struct cparticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time),
						qBool thinkNext,
						double orient) {
	cparticle_t		*p = NULL;

	if (!cl_FreeParticles)
		return;

	p = cl_FreeParticles;
	cl_FreeParticles = p->next;
	p->next = cl_ActiveParticles;
	cl_ActiveParticles = p;

	p->time = cls.realTime;

	VectorSet (p->org, org0, org1, org2);
	VectorCopy (p->org, p->oldOrigin);

	VectorSet (p->angle, angle0, angle1, angle2);
	VectorSet (p->vel, vel0, vel1, vel2);
	VectorSet (p->accel, accel0, accel1, accel2);

	Vector4Set (p->color, red, green, blue, alpha);
	Vector4Set (p->colorVel, redVel, greenVel, blueVel, alphaVel);

	p->image = clMedia.mediaTable[type%PDT_PICTOTAL];
	p->flags = flags;

	p->size = size;
	p->sizeVel = sizeVel;

	p->sFactor = sFactor;
	p->dFactor = dFactor;

	if (think)
		p->think = think;
	else
		p->think = NULL;

	p->thinkNext = thinkNext;

	p->orient = orient;
}


/*
===============
pDegree
===============
*/
qBool pDegree (vec3_t org, qBool lod) {
	float	dist, value;

	if (((rand()%41)/20) > clamp (part_degree->value, 0, 2))
		return qTrue;

	if (lod && part_lod->integer) {
		// distance calculations
		dist = VectorDistance (cl.refDef.viewOrigin, org);
		value = ((dist/1400) < 0.85)?0.85 : (dist/1400);

		if (value > clamp (part_degree->value, 0, 2))
			return qTrue;
	}

	return qFalse;
}


/*
===============
pDecDegree
===============
*/
float FASTCALL pDecDegree (vec3_t org, float dec, float scale, qBool lod) {
	float	deg = scale, value, dist;

	// cvar degree control
	deg = clamp (deg, 0, 1);

	value = (dec*(1+deg)) - ((dec*deg)*clamp (part_degree->value, 0, 2));

	if (value < 1) value = 1;
	else if (part_lod->integer && lod) {
		// distance calculations
		dist = VectorDistance (cl.refDef.viewOrigin, org);
		value *= ((dist/1400) < 1)?1 : (dist/1400);
	}

	return value;
}


/*
===============
pIncDegree
===============
*/
float FASTCALL pIncDegree (vec3_t org, float inc, float scale, qBool lod) {
	float	deg = scale, value, dist;

	// cvar degree control
	deg = clamp (deg, 0, 1);

	value = (inc*(1-deg)) + ((inc*deg)*clamp (part_degree->value, 0, 2));

	if (part_lod->integer && lod) {
		// distance calculations
		dist = VectorDistance (cl.refDef.viewOrigin, org);
		value /= ((dist/1400) < 1)?1 : (dist/1400);
	}

	return value;
}


/*
===============
CL_FreeParticle
===============
*/
void CL_FreeParticle (cparticle_t *p) {
	p->next = cl_FreeParticles;
	cl_FreeParticles = p;
}


/*
===============
CL_ClearParticles
===============
*/
void CL_ClearParticles (void) {
	int		i;

	// hulk smash
	cl_FreeParticles = &cl_Particles[0];
	cl_ActiveParticles = NULL;

	for (i=0 ; i<MAX_PARTICLES ; i++)
		cl_Particles[i].next = &cl_Particles[i + 1];

	cl_Particles[MAX_PARTICLES - 1].next = NULL;

	CL_ClearSustains ();
}


/*
===============
CL_AddParticles
===============
*/
void CL_AddParticles (void) {
	cparticle_t	*p, *next;
	cparticle_t	*active, *tail;
	vec3_t		org, temp;
	dvec4_t		color;
	double		size, orient;
	double		time, time2, dist;
	int			i;
	int			pointBits;

	active = NULL;
	tail = NULL;

	CL_ProcessSustains ();
	if (!cl_add_particles->integer)
		return;

	for (p=cl_ActiveParticles ; p ; p=next) {
		next = p->next;

		// PMM - added PART_INSTANT handling for heat beam
		time = 1;
		if (p->colorVel[3] != PART_INSTANT) {
			time = (cls.realTime - p->time)*0.001;
			color[3] = p->color[3] + time*p->colorVel[3];

			if (color[3] <= 0) {
				// faded out
				CL_FreeParticle (p);
				continue;
			}
		} else
			color[3] = p->color[3];

		p->next = NULL;
		if (!tail)
			active = tail = p;
		else {
			tail->next = p;
			tail = p;
		}

		if (color[3] > 1.0)
			color[3] = 1;

		// sizeVel calcs
		size = p->size;
		if ((p->colorVel[3] != PART_INSTANT) && (p->size != p->sizeVel)) {
			if (p->size > p->sizeVel) // shrink
				size = p->size - ((p->size - p->sizeVel) * (p->color[3] - color[3]));
			else // grow
				size = p->size + ((p->sizeVel - p->size) * (p->color[3] - color[3]));
		}

		// colorVel calcs
		VectorCopy (p->color, color);
		for (i=0 ; i<3 ; i++) {
			if ((p->colorVel[3] != PART_INSTANT) && (p->color[i] != p->colorVel[i])) {
				if (p->color[i] > p->colorVel[i])
					color[i] = p->color[i] - ((p->color[i] - p->colorVel[i]) * (p->color[3] - color[3]));
				else
					color[i] = p->color[i] + ((p->colorVel[i] - p->color[i]) * (p->color[3] - color[3]));
			}

			color[i] = clamp (color[i], 0, 255);
		}

		// origin
		time2 = time*time;
		for (i=0 ; i<3 ; i++)
			org[i] = p->org[i] + p->vel[i]*time + p->accel[i]*time2;

		if (p->flags & PF_GRAVITY)
			org[2] += (time2 * -PART_GRAVITY);

		// culling
		if (part_cull->integer && !(p->flags & (PF_BEAM|PF_SPIRAL|PF_DEPTHHACK|PF_DIRECTION))) {
			// lessen fillrate consumption
			dist = VectorDistance (cl.refDef.viewOrigin, org);
			if (dist <= p->size * 0.8)
				color[3] = 0;

			// kill particles behind the view
			VectorSubtract (org, cl.refDef.viewOrigin, temp);
			VectorNormalize (temp, temp);

			if (DotProduct (temp, cl.refDef.viewAxis[0]) < 0)
				continue;
		}

		if (p->flags & PF_ALPHACOLOR)
			VectorScale (color, color[3], color);

		// think function
		if (color[3] > 0) {
			orient = p->orient;

			if (p->thinkNext && p->think) {
				p->thinkNext = qFalse;
				p->think (p, org, p->angle, color, &size, &orient, &time);
			}

			// contents requirements
			pointBits = 0;
			if (p->flags & PF_AIRONLY) {
				pointBits |= (CONTENTS_LAVA|CONTENTS_SLIME|CONTENTS_WATER);
				if (CL_PMPointContents (org) & pointBits) {
					color[3] = 0;
					p->color[3] = 0;
					p->colorVel[3] = 0;
					continue;
				}
			} else {
				if (p->flags & PF_LAVAONLY)		pointBits |= CONTENTS_LAVA;
				if (p->flags & PF_SLIMEONLY)	pointBits |= CONTENTS_SLIME;
				if (p->flags & PF_WATERONLY)	pointBits |= CONTENTS_WATER;

				if (pointBits) {
					if (!(CL_PMPointContents (org) & pointBits)) {
						color[3] = 0;
						p->color[3] = 0;
						p->colorVel[3] = 0;
						continue;
					}
				}
			}

			R_AddParticle (org, p->angle, color, size, p->image, p->flags, p->sFactor, p->dFactor, orient);
		}

		VectorCopy (org, p->oldOrigin);

		if (p->colorVel[3] == PART_INSTANT) {
			p->color[3] = 0;
			p->colorVel[3] = 0;
		}
	}

	cl_ActiveParticles = active;
}
