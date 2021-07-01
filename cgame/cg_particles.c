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

static particle_t		*cg_activeParticles;
static particle_t		*cg_freeParticles;
static particle_t		cg_particleList[MAX_PARTICLES];

/*
=============================================================================

	PARTICLE IMAGING

=============================================================================
*/

int pRandBloodDrip (void)	{ return PT_BLDDRIP01 + (rand()&1); }
int pRandGrnBloodDrip (void){ return PT_BLDDRIP01_GRN + (rand()&1); }
int pRandBloodMark (void)	{ return PT_BLOOD + (rand()%6); }
int pRandGrnBloodMark (void){ return PT_GRNBLOOD + (rand()%6); }
int pRandSmoke (void)		{ return PT_SMOKE + (rand()&1); }
int pRandGlowSmoke (void)	{ return PT_SMOKEGLOW + (rand()&1); }
int pRandEmbers (void)		{ return PT_EMBERS1 + (rand()%3); }
int pRandFire (void)		{ return PT_FIRE1 + (rand()&3); }

/*
=============================================================================

	PARTICLE MANAGEMENT

=============================================================================
*/

/*
===============
makePart

JESUS H FUNCTION PARAMATERS
===============
*/
void __fastcall makePart (float org0,					float org1,					float org2,
						float angle0,					float angle1,				float angle2,
						float vel0,						float vel1,					float vel2,
						float accel0,					float accel1,				float accel2,
						float red,						float green,				float blue,
						float redVel,					float greenVel,				float blueVel,
						float alpha,					float alphaVel,
						float size,						float sizeVel,
						int type,						int flags,
						void (*think)(struct particle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time),
						qBool thinkNext,
						byte style,
						float orient)
{
	particle_t		*p = NULL;

	if (!cg_freeParticles)
		return;

	p = cg_freeParticles;
	cg_freeParticles = p->next;
	p->next = cg_activeParticles;
	cg_activeParticles = p;

	p->time = (float)cg.realTime;

	VectorSet (p->org, org0, org1, org2);
	VectorCopy (p->org, p->oldOrigin);

	VectorSet (p->angle, angle0, angle1, angle2);
	VectorSet (p->vel, vel0, vel1, vel2);
	VectorSet (p->accel, accel0, accel1, accel2);

	Vector4Set (p->color, red, green, blue, alpha);
	Vector4Set (p->colorVel, redVel, greenVel, blueVel, alphaVel);

	p->shader = cgMedia.particleTable[type%PT_PICTOTAL];
	p->style = style;
	p->flags = flags;

	p->size = size;
	p->sizeVel = sizeVel;

	if (think)
		p->think = think;
	else
		p->think = NULL;

	p->thinkNext = thinkNext;

	p->orient = orient;
}


/*
===============
CG_FreeParticle
===============
*/
static inline void CG_FreeParticle (particle_t *p)
{
	p->next = cg_freeParticles;
	cg_freeParticles = p;
}


/*
===============
CG_ClearParticles
===============
*/
void CG_ClearParticles (void)
{
	int		i;

	// Hulk smash
	memset (cg_particleList, 0, sizeof (cg_particleList));

	cg_freeParticles = &cg_particleList[0];
	cg_activeParticles = NULL;

	for (i=0 ; i<MAX_PARTICLES-1 ; i++)
		cg_particleList[i].next = &cg_particleList[i + 1];
	cg_particleList[MAX_PARTICLES-1].next = NULL;

	CG_ClearSustains ();
}


/*
===============
CG_AddParticles
===============
*/
void CG_AddParticles (void)
{
	particle_t	*p, *next;
	particle_t	*active, *tail;
	vec3_t		org, temp;
	vec4_t		color;
	float		size, orient;
	float		time, time2, dist;
	int			i, j, pointBits;
	float		lightest;
	vec3_t		shade;
	float		scale;
	vec3_t		p_Up, p_Right;
	vec3_t		tmp_Up, tmp_Right;
	vec3_t		point, width, move;
	poly_t		poly;
	bvec4_t		outColor;
	vec3_t		delta, vdelta;

	active = NULL;
	tail = NULL;

	CG_AddMapFX ();
	CG_AddSustains ();
	if (!cl_add_particles->integer)
		return;

	VectorScale (cg.refDef.upVec, 0.75f, p_Up);
	VectorScale (cg.refDef.rightVec, 0.75f, p_Right);

	for (p=cg_activeParticles ; p ; p=next) {
		next = p->next;

		if (p->colorVel[3] > PART_INSTANT) {
			time = (cg.realTime - p->time)*0.001f;
			color[3] = p->color[3] + time*p->colorVel[3];
		}
		else {
			time = 1;
			color[3] = p->color[3];
		}

		// Faded out
		if (color[3] <= 0.0001f) {
			CG_FreeParticle (p);
			continue;
		}

		if (color[3] > 1.0)
			color[3] = 1.0f;

		p->next = NULL;
		if (!tail)
			active = tail = p;
		else {
			tail->next = p;
			tail = p;
		}

		// Origin
		time2 = time*time;

		org[0] = p->org[0] + p->vel[0]*time + p->accel[0]*time2;
		org[1] = p->org[1] + p->vel[1]*time + p->accel[1]*time2;
		org[2] = p->org[2] + p->vel[2]*time + p->accel[2]*time2;

		if (p->flags & PF_GRAVITY)
			org[2] -= (time2 * PART_GRAVITY);

		// Culling
		switch (p->style) {
		case PART_STYLE_ANGLED:
		case PART_STYLE_BEAM:
		case PART_STYLE_DIRECTION:
			break;

		default:
			if (cg_particleCulling->integer) {
				// Kill particles behind the view
				VectorSubtract (org, cg.refDef.viewOrigin, temp);
				VectorNormalizef (temp, temp);
				if (DotProduct (temp, cg.refDef.forwardVec) < 0)
					goto nextParticle;

				// Lessen fillrate consumption
				if (!(p->flags & PF_NOCLOSECULL)) {
					dist = VectorDistFast (cg.refDef.viewOrigin, org);
					if (dist <= 5)
						goto nextParticle;
				}
			}
			break;
		}

		// sizeVel calcs
		if (p->colorVel[3] > PART_INSTANT && p->size != p->sizeVel) {
			if (p->size > p->sizeVel) // shrink
				size = p->size - ((p->size - p->sizeVel) * (p->color[3] - color[3]));
			else // grow
				size = p->size + ((p->sizeVel - p->size) * (p->color[3] - color[3]));
		}
		else {
			size = p->size;
		}

		if (size < 0.0f)
			goto nextParticle;

		// colorVel calcs
		VectorCopy (p->color, color);
		if (p->colorVel[3] > PART_INSTANT) {
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

		// Particle shading
		if (p->flags & PF_SHADE && cg_particleShading->integer) {
			cgi.R_LightPoint (p->org, shade);

			lightest = 0;
			for (j=0 ; j<3 ; j++) {
				color[j] = ((0.7f * clamp (shade[j], 0.0f, 1.0f)) + 0.3f) * p->color[j];
				if (color[j] > lightest)
					lightest = color[j];
			}

			if (lightest > 255.0) {
				color[0] *= 255.0f / lightest;
				color[1] *= 255.0f / lightest;
				color[2] *= 255.0f / lightest;
			}
		}

		// Alpha*color
		if (p->flags & PF_ALPHACOLOR)
			VectorScale (color, color[3], color);

		// Think function
		orient = p->orient;
		if (p->thinkNext && p->think) {
			p->thinkNext = qFalse;
			p->think (p, org, p->angle, color, &size, &orient, &time);
		}

		if (color[3] <= 0.0f)
			goto nextParticle;

		// Contents requirements
		pointBits = 0;
		if (cg.currGameMod != GAME_MOD_LOX) { // FIXME: yay hack
			if (p->flags & PF_AIRONLY) {
				pointBits |= (CONTENTS_LAVA|CONTENTS_SLIME|CONTENTS_WATER);
				if (cgi.CM_PointContents (org, 0) & pointBits) {
					p->color[3] = 0;
					p->colorVel[3] = 0;
					goto nextParticle;
				}
			}
			else {
				if (p->flags & PF_LAVAONLY)		pointBits |= CONTENTS_LAVA;
				if (p->flags & PF_SLIMEONLY)	pointBits |= CONTENTS_SLIME;
				if (p->flags & PF_WATERONLY)	pointBits |= CONTENTS_WATER;

				if (pointBits) {
					if (!(cgi.CM_PointContents (org, 0) & pointBits)) {
						p->color[3] = 0;
						p->colorVel[3] = 0;
						goto nextParticle;
					}
				}
			}
		}

		// Add to be rendered
		if (p->flags & PF_SCALED) {
			scale = (org[0] - cg.refDef.viewOrigin[0]) * cg.refDef.forwardVec[0] +
					(org[1] - cg.refDef.viewOrigin[1]) * cg.refDef.forwardVec[1] +
					(org[2] - cg.refDef.viewOrigin[2]) * cg.refDef.forwardVec[2];

			scale = (scale < 20) ? 1 : 1 + scale * 0.004f;
		}
		else
			scale = 1;

		scale = (scale - 1) + size;

		// Rendering
		outColor[0] = color[0];
		outColor[1] = color[1];
		outColor[2] = color[2];
		outColor[3] = color[3] * 255;

		switch (p->style) {
		case PART_STYLE_ANGLED:
			Angles_Vectors (p->angle, NULL, tmp_Right, tmp_Up); 

			if (orient) {
				float c = (float)cos (DEG2RAD (orient)) * scale;
				float s = (float)sin (DEG2RAD (orient)) * scale;

				// Top left
				Vector2Set (p->outCoords[0], 0, 0);
				VectorSet (p->outVertices[0],	org[0] + tmp_Up[0]*s - tmp_Right[0]*c,
												org[1] + tmp_Up[1]*s - tmp_Right[1]*c,
												org[2] + tmp_Up[2]*s - tmp_Right[2]*c);

				// Bottom left
				Vector2Set (p->outCoords[1], 0, 1);
				VectorSet (p->outVertices[1],	org[0] - tmp_Up[0]*c - tmp_Right[0]*s,
												org[1] - tmp_Up[1]*c - tmp_Right[1]*s,
												org[2] - tmp_Up[2]*c - tmp_Right[2]*s);

				// Bottom right
				Vector2Set (p->outCoords[2], 1, 1);
				VectorSet (p->outVertices[2],	org[0] - tmp_Up[0]*s + tmp_Right[0]*c,
												org[1] - tmp_Up[1]*s + tmp_Right[1]*c,
												org[2] - tmp_Up[2]*s + tmp_Right[2]*c);

				// Top right
				Vector2Set (p->outCoords[3], 1, 0);
				VectorSet (p->outVertices[3],	org[0] + tmp_Up[0]*c + tmp_Right[0]*s,
												org[1] + tmp_Up[1]*c + tmp_Right[1]*s,
												org[2] + tmp_Up[2]*c + tmp_Right[2]*s);
			}
			else {
				// Top left
				Vector2Set (p->outCoords[0], 0, 0);
				VectorSet (p->outVertices[0],	org[0] + tmp_Up[0]*scale - tmp_Right[0]*scale,
												org[1] + tmp_Up[1]*scale - tmp_Right[1]*scale,
												org[2] + tmp_Up[2]*scale - tmp_Right[2]*scale);

				// Bottom left
				Vector2Set (p->outCoords[1], 0, 1);
				VectorSet (p->outVertices[1],	org[0] - tmp_Up[0]*scale - tmp_Right[0]*scale,
												org[1] - tmp_Up[1]*scale - tmp_Right[1]*scale,
												org[2] - tmp_Up[2]*scale - tmp_Right[2]*scale);

				// Bottom right
				Vector2Set (p->outCoords[2], 1, 1);
				VectorSet (p->outVertices[2],	org[0] - tmp_Up[0]*scale + tmp_Right[0]*scale,
												org[1] - tmp_Up[1]*scale + tmp_Right[1]*scale,
												org[2] - tmp_Up[2]*scale + tmp_Right[2]*scale);

				// Top right
				Vector2Set (p->outCoords[3], 1, 0);
				VectorSet (p->outVertices[3],	org[0] + tmp_Up[0]*scale + tmp_Right[0]*scale,
												org[1] + tmp_Up[1]*scale + tmp_Right[1]*scale,
												org[2] + tmp_Up[2]*scale + tmp_Right[2]*scale);
			}

			// Render it
			*(int *)p->outColor[0] = *(int *)outColor;
			*(int *)p->outColor[1] = *(int *)outColor;
			*(int *)p->outColor[2] = *(int *)outColor;
			*(int *)p->outColor[3] = *(int *)outColor;

			poly.numVerts = 4;
			poly.colors = p->outColor;
			poly.texCoords = p->outCoords;
			poly.vertices = p->outVertices;
			poly.shader = p->shader;
			poly.shaderTime = 0;

			VectorCopy (p->org, poly.origin);
			poly.radius = scale;

			cgi.R_AddPoly (&poly);
			break;

		case PART_STYLE_BEAM:
			VectorSubtract (org, cg.refDef.viewOrigin, point);
			CrossProduct (point, p->angle, width);
			VectorNormalizef (width, width);
			VectorScale (width, scale, width);

			dist = VectorDistFast (org, p->angle);

			Vector2Set (p->outCoords[0], 1, dist);
			VectorSet (p->outVertices[0], org[0] + width[0],
										org[1] + width[1],
										org[2] + width[2]);

			Vector2Set (p->outCoords[1], 0, 0);
			VectorSet (p->outVertices[1], org[0] - width[0],
										org[1] - width[1],
										org[2] - width[2]);

			VectorAdd (point, p->angle, point);
			CrossProduct (point, p->angle, width);
			VectorNormalizef (width, width);
			VectorScale (width, scale, width);

			Vector2Set (p->outCoords[2], 0, 0);
			VectorSet (p->outVertices[2], org[0] + p->angle[0] - width[0],
										org[1] + p->angle[1] - width[1],
										org[2] + p->angle[2] - width[2]);

			Vector2Set (p->outCoords[3], 1, dist);
			VectorSet (p->outVertices[3], org[0] + p->angle[0] + width[0],
										org[1] + p->angle[1] + width[1],
										org[2] + p->angle[2] + width[2]);

			// Render it
			*(int *)p->outColor[0] = *(int *)outColor;
			*(int *)p->outColor[1] = *(int *)outColor;
			*(int *)p->outColor[2] = *(int *)outColor;
			*(int *)p->outColor[3] = *(int *)outColor;

			poly.numVerts = 4;
			poly.colors = p->outColor;
			poly.texCoords = p->outCoords;
			poly.vertices = p->outVertices;
			poly.shader = p->shader;
			poly.shaderTime = 0;

			VectorCopy (p->org, poly.origin);
			poly.radius = scale; // FIXME this needs to be the length of the beam

			cgi.R_AddPoly (&poly);
			break;

		case PART_STYLE_DIRECTION:
			VectorAdd (p->angle, org, vdelta);

			VectorSubtract (org, vdelta, move);
			VectorNormalizef (move, move);

			VectorCopy (move, tmp_Up);
			VectorSubtract (cg.refDef.viewOrigin, vdelta, delta);
			CrossProduct (tmp_Up, delta, tmp_Right);

			VectorNormalizef (tmp_Right, tmp_Right);

			VectorScale (tmp_Right, 0.75f, tmp_Right);
			VectorScale (tmp_Up, 0.75f * VectorLengthFast (p->angle), tmp_Up);

			// Top left
			Vector2Set (p->outCoords[0], 0, 0);
			VectorSet (p->outVertices[0],org[0] + tmp_Up[0]*scale - tmp_Right[0]*scale,
										org[1] + tmp_Up[1]*scale - tmp_Right[1]*scale,
										org[2] + tmp_Up[2]*scale - tmp_Right[2]*scale);

			// Bottom left
			Vector2Set (p->outCoords[1], 0, 1);
			VectorSet (p->outVertices[1],org[0] - tmp_Up[0]*scale - tmp_Right[0]*scale,
										org[1] - tmp_Up[1]*scale - tmp_Right[1]*scale,
										org[2] - tmp_Up[2]*scale - tmp_Right[2]*scale);

			// Bottom right
			Vector2Set (p->outCoords[2], 1, 1);
			VectorSet (p->outVertices[2],org[0] - tmp_Up[0]*scale + tmp_Right[0]*scale,
										org[1] - tmp_Up[1]*scale + tmp_Right[1]*scale,
										org[2] - tmp_Up[2]*scale + tmp_Right[2]*scale);

			// Top right
			Vector2Set (p->outCoords[3], 1, 0);
			VectorSet (p->outVertices[3],org[0] + tmp_Up[0]*scale + tmp_Right[0]*scale,
										org[1] + tmp_Up[1]*scale + tmp_Right[1]*scale,
										org[2] + tmp_Up[2]*scale + tmp_Right[2]*scale);

			// Render it
			*(int *)p->outColor[0] = *(int *)outColor;
			*(int *)p->outColor[1] = *(int *)outColor;
			*(int *)p->outColor[2] = *(int *)outColor;
			*(int *)p->outColor[3] = *(int *)outColor;

			poly.numVerts = 4;
			poly.colors = p->outColor;
			poly.texCoords = p->outCoords;
			poly.vertices = p->outVertices;
			poly.shader = p->shader;
			poly.shaderTime = 0;

			VectorCopy (p->org, poly.origin);
			poly.radius = scale;

			cgi.R_AddPoly (&poly);
			break;

		default:
		case PART_STYLE_QUAD:
			if (orient) {
				float c = (float)cos (DEG2RAD (orient)) * scale;
				float s = (float)sin (DEG2RAD (orient)) * scale;

				// Top left
				Vector2Set (p->outCoords[0], 0.0, 0.0);
				VectorSet (p->outVertices[0],	org[0] - cg.refDef.rightVec[0]*c + cg.refDef.upVec[0]*s,
												org[1] - cg.refDef.rightVec[1]*c + cg.refDef.upVec[1]*s,
												org[2] - cg.refDef.rightVec[2]*c + cg.refDef.upVec[2]*s);

				// Bottom left
				Vector2Set (p->outCoords[1], 0.0, 1.0);
				VectorSet (p->outVertices[1],	org[0] + cg.refDef.rightVec[0]*s + cg.refDef.upVec[0]*c,
												org[1] + cg.refDef.rightVec[1]*s + cg.refDef.upVec[1]*c,
												org[2] + cg.refDef.rightVec[2]*s + cg.refDef.upVec[2]*c);

				// Bottom right
				Vector2Set (p->outCoords[2], 1.0, 1.0);
				VectorSet (p->outVertices[2],	org[0] + cg.refDef.rightVec[0]*c - cg.refDef.upVec[0]*s,
												org[1] + cg.refDef.rightVec[1]*c - cg.refDef.upVec[1]*s,
												org[2] + cg.refDef.rightVec[2]*c - cg.refDef.upVec[2]*s);

				// Top right
				Vector2Set (p->outCoords[3], 1.0, 0.0);
				VectorSet (p->outVertices[3],	org[0] - cg.refDef.rightVec[0]*s - cg.refDef.upVec[0]*c,
												org[1] - cg.refDef.rightVec[1]*s - cg.refDef.upVec[1]*c,
												org[2] - cg.refDef.rightVec[2]*s - cg.refDef.upVec[2]*c);
			}
			else {
				// Top left
				Vector2Set (p->outCoords[0], 0, 0);
				VectorSet (p->outVertices[0],	org[0] + cg.refDef.upVec[0]*scale - cg.refDef.rightVec[0]*scale,
												org[1] + cg.refDef.upVec[1]*scale - cg.refDef.rightVec[1]*scale,
												org[2] + cg.refDef.upVec[2]*scale - cg.refDef.rightVec[2]*scale);

				// Bottom left
				Vector2Set (p->outCoords[1], 0, 1);
				VectorSet (p->outVertices[1],	org[0] - cg.refDef.upVec[0]*scale - cg.refDef.rightVec[0]*scale,
												org[1] - cg.refDef.upVec[1]*scale - cg.refDef.rightVec[1]*scale,
												org[2] - cg.refDef.upVec[2]*scale - cg.refDef.rightVec[2]*scale);

				// Bottom right
				Vector2Set (p->outCoords[2], 1, 1);
				VectorSet (p->outVertices[2],	org[0] - cg.refDef.upVec[0]*scale + cg.refDef.rightVec[0]*scale,
												org[1] - cg.refDef.upVec[1]*scale + cg.refDef.rightVec[1]*scale,
												org[2] - cg.refDef.upVec[2]*scale + cg.refDef.rightVec[2]*scale);

				// Top right
				Vector2Set (p->outCoords[3], 1, 0);
				VectorSet (p->outVertices[3],	org[0] + cg.refDef.upVec[0]*scale + cg.refDef.rightVec[0]*scale,
												org[1] + cg.refDef.upVec[1]*scale + cg.refDef.rightVec[1]*scale,
												org[2] + cg.refDef.upVec[2]*scale + cg.refDef.rightVec[2]*scale);
			}

			// Render it
			*(int *)p->outColor[0] = *(int *)outColor;
			*(int *)p->outColor[1] = *(int *)outColor;
			*(int *)p->outColor[2] = *(int *)outColor;
			*(int *)p->outColor[3] = *(int *)outColor;

			poly.numVerts = 4;
			poly.colors = p->outColor;
			poly.texCoords = p->outCoords;
			poly.vertices = p->outVertices;
			poly.shader = p->shader;
			poly.shaderTime = 0;

			VectorCopy (p->org, poly.origin);
			poly.radius = scale;

			cgi.R_AddPoly (&poly);
			break;
		}

nextParticle:
		VectorCopy (org, p->oldOrigin);

		// Kill if instant
		if (p->colorVel[3] <= PART_INSTANT) {
			p->color[3] = 0;
			p->colorVel[3] = 0;
		}
	}

	cg_activeParticles = active;
}
