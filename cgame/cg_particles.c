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
// cg_particles.c
//

#include "cg_local.h"

static cgParticle_t		*cgActiveParticles;
static cgParticle_t		*cgFreeParticles;
static cgParticle_t		cgParticles[MAX_PARTICLES];

/*
=============================================================================

	PARTICLE IMAGING

=============================================================================
*/

int pSmokeCnt			= 2;
int pSmokeTable[]		= { PT_SMOKE, PT_SMOKE2 };
int pRandSmoke (void)	{ return pSmokeTable[(rand()%pSmokeCnt)]; }

int pEmbersCnt			= 3;
int pEmbersTable[]		= { PT_EMBERS1, PT_EMBERS2, PT_EMBERS3 };
int pRandEmbers (void)	{ return pEmbersTable[(rand()%pEmbersCnt)]; }

int pFireCnt			= 4;
int pFireTable[]		= { PT_FIRE1, PT_FIRE2, PT_FIRE3, PT_FIRE4 };
int pRandFire (void)	{ return pFireTable[(rand()%pFireCnt)]; }

/*
=============================================================================

	PARTICLE MANAGEMENT

=============================================================================
*/

/*
===============
makePart
===============
*/
void __fastcall makePart (double org0,					double org1,					double org2,
						double angle0,					double angle1,					double angle2,
						double vel0,					double vel1,					double vel2,
						double accel0,					double accel1,					double accel2,
						double red,						double green,					double blue,
						double redVel,					double greenVel,				double blueVel,
						double alpha,					double alphaVel,
						double size,					double sizeVel,
						int type,						int flags,
						int sFactor,					int dFactor,
						void (*think)(struct cgParticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time),
						qBool thinkNext,
						double orient) {
	cgParticle_t		*p = NULL;

	if (!cgFreeParticles)
		return;

	p = cgFreeParticles;
	cgFreeParticles = p->next;
	p->next = cgActiveParticles;
	cgActiveParticles = p;

	p->time = cgs.realTime;

	VectorSet (p->org, org0, org1, org2);
	VectorCopy (p->org, p->oldOrigin);

	VectorSet (p->angle, angle0, angle1, angle2);
	VectorSet (p->vel, vel0, vel1, vel2);
	VectorSet (p->accel, accel0, accel1, accel2);

	Vector4Set (p->color, red, green, blue, alpha);
	Vector4Set (p->colorVel, redVel, greenVel, blueVel, alphaVel);

	p->image = cgMedia.mediaTable[type%PDT_PICTOTAL];
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
		dist = VectorDistanceFast (cg.refDef.viewOrigin, org);
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
float __fastcall pDecDegree (vec3_t org, float dec, float scale, qBool lod) {
	float	deg = scale, value, dist;

	// cvar degree control
	deg = clamp (deg, 0, 1);

	value = (dec*(1+deg)) - ((dec*deg)*clamp (part_degree->value, 0, 2));

	if (value < 1) value = 1;
	else if (part_lod->integer && lod) {
		// distance calculations
		dist = VectorDistanceFast (cg.refDef.viewOrigin, org);
		value *= ((dist/1400) < 1)?1 : (dist/1400);
	}

	return value;
}


/*
===============
pIncDegree
===============
*/
float __fastcall pIncDegree (vec3_t org, float inc, float scale, qBool lod) {
	float	deg = scale, value, dist;

	// cvar degree control
	deg = clamp (deg, 0, 1);

	value = (inc*(1-deg)) + ((inc*deg)*clamp (part_degree->value, 0, 2));

	if (part_lod->integer && lod) {
		// distance calculations
		dist = VectorDistanceFast (cg.refDef.viewOrigin, org);
		value /= ((dist/1400) < 1)?1 : (dist/1400);
	}

	return value;
}


/*
===============
CG_FreeParticle
===============
*/
void CG_FreeParticle (cgParticle_t *p) {
	p->next = cgFreeParticles;
	cgFreeParticles = p;
}


/*
===============
CG_ClearParticles
===============
*/
void CG_ClearParticles (void) {
	int		i;

	// hulk smash
	memset (cgParticles, 0, sizeof (cgParticles));

	cgFreeParticles = &cgParticles[0];
	cgActiveParticles = NULL;

	for (i=0 ; i<MAX_PARTICLES-1 ; i++)
		cgParticles[i].next = &cgParticles[i + 1];
	cgParticles[MAX_PARTICLES-1].next = NULL;

	CG_ClearSustains ();
}


/*
===============
CG_AddParticles
===============
*/
void CG_AddParticles (void) {
	cgParticle_t	*p, *next;
	cgParticle_t	*active, *tail;
	vec3_t			org, temp;
	dvec4_t			color;
	double			size, orient;
	double			time, time2, dist;
	int				i, j, pointBits;
	float			lightest;
	vec3_t			shade;

	active = NULL;
	tail = NULL;

	CG_AddMapFX ();
	CG_AddSustains ();
	if (!cl_add_particles->integer)
		return;

	for (p=cgActiveParticles ; p ; p=next) {
		next = p->next;

		time = 1;
		if (p->colorVel[3] >= PART_INSTANT) {
			time = (cgs.realTime - p->time)*0.001;
			color[3] = p->color[3] + time*p->colorVel[3];

			if (color[3] <= 0) {
				// faded out
				CG_FreeParticle (p);
				continue;
			}
		}
		else
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
		if ((p->colorVel[3] >= PART_INSTANT) && (p->size != p->sizeVel)) {
			if (p->size > p->sizeVel) // shrink
				size = p->size - ((p->size - p->sizeVel) * (p->color[3] - color[3]));
			else // grow
				size = p->size + ((p->sizeVel - p->size) * (p->color[3] - color[3]));
		}

		// colorVel calcs
		VectorCopy (p->color, color);
		if (p->colorVel[3] >= PART_INSTANT) {
			for (i=0 ; i<3 ; i++) {
				if (p->color[i] != p->colorVel[i]) {
					if (p->color[i] > p->colorVel[i])
						color[i] = p->color[i] - ((p->color[i] - p->colorVel[i]) * (p->color[3] - color[3]));
					else
						color[i] = p->color[i] + ((p->colorVel[i] - p->color[i]) * (p->color[3] - color[3]));
				}

				color[i] = clamp (color[i], 0, 255);
			}
		}

		// origin
		time2 = time*time;

		org[0] = p->org[0] + p->vel[0]*time + p->accel[0]*time2;
		org[1] = p->org[1] + p->vel[1]*time + p->accel[1]*time2;
		org[2] = p->org[2] + p->vel[2]*time + p->accel[2]*time2;

		if (p->flags & PF_GRAVITY)
			org[2] += (time2 * -PART_GRAVITY);

		// culling
		if (part_cull->integer && !(p->flags & (PF_BEAM|PF_SPIRAL|PF_DIRECTION))) {
			// lessen fillrate consumption
			if (!(p->flags & PF_NOCLOSECULL)) {
				dist = VectorDistanceFast (cg.refDef.viewOrigin, org);
				if (dist <= p->size * 0.8)
					color[3] = 0;
			}

			// kill particles behind the view
			VectorSubtract (org, cg.refDef.viewOrigin, temp);
			VectorNormalize (temp, temp);

			if (DotProduct (temp, cg.refDef.viewAxis[0]) < 0)
				continue;
		}

		// particle shading
		if ((p->flags & PF_SHADE) && part_shade->integer) {
			cgi.R_LightPoint (p->org, shade);

			lightest = 0;
			for (j=0 ; j<3 ; j++) {
				color[j] = ((0.7 * clamp (shade[j], 0.0, 1.0)) + 0.3) * p->color[j];
				if (color[j] > lightest)
					lightest = color[j];
			}

			if (lightest > 255.0) {
				color[0] *= 255.0 / lightest;
				color[1] *= 255.0 / lightest;
				color[2] *= 255.0 / lightest;
			}
		}

		// alpha*color
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
				if (CGI_CM_PointContents (org, 0) & pointBits) {
					color[3] = 0;
					p->color[3] = 0;
					p->colorVel[3] = 0;
					continue;
				}
			}
			else {
				if (p->flags & PF_LAVAONLY)		pointBits |= CONTENTS_LAVA;
				if (p->flags & PF_SLIMEONLY)	pointBits |= CONTENTS_SLIME;
				if (p->flags & PF_WATERONLY)	pointBits |= CONTENTS_WATER;

				if (pointBits) {
					if (!(CGI_CM_PointContents (org, 0) & pointBits)) {
						color[3] = 0;
						p->color[3] = 0;
						p->colorVel[3] = 0;
						continue;
					}
				}
			}

			if (size && (color[3] > 0))
				cgi.R_AddParticle (org, p->angle, color, size, p->image, p->flags, p->sFactor, p->dFactor, orient);
		}

		VectorCopy (org, p->oldOrigin);

		if (p->colorVel[3] < PART_INSTANT) {
			p->color[3] = 0;
			p->colorVel[3] = 0;
		}
	}

	cgActiveParticles = active;
}
