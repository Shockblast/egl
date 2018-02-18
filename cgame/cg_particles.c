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

static cgParticle_t		*cg_freeParticles;
static cgParticle_t		cg_particleHeadNode, cg_particleList[MAX_PARTICLES];
static int				cg_numParticles;

/*
=============================================================================

	PARTICLE MATERIAL RANDOMIZING

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
CG_AllocParticle
===============
*/
static cgParticle_t *CG_AllocParticle (void)
{
	cgParticle_t	*p;

	// Take a free particle spot if possible, otherwise steal the oldest one
	if (cg_freeParticles && cg_numParticles+1 < cg_particleMax->intVal) {
		p = cg_freeParticles;
		cg_freeParticles = p->next;
	}
	else {
		p = cg_particleHeadNode.prev;
		p->prev->next = p->next;
		p->next->prev = p->prev;

		cg_numParticles--;
	}

	// Move to the beginning of the list
	p->prev = &cg_particleHeadNode;
	p->next = cg_particleHeadNode.next;
	p->next->prev = p;
	p->prev->next = p;

	cg_numParticles++;
	return p;
}


/*
===============
CG_FreeParticle
===============
*/
static inline void CG_FreeParticle (cgParticle_t *p)
{
	// Remove from linked active list
	p->prev->next = p->next;
	p->next->prev = p->prev;

	// Insert into linked free list
	p->next = cg_freeParticles;
	cg_freeParticles = p;

	cg_numParticles--;
}


/*
===============
CG_SpawnParticle

FIXME: JESUS H FUNCTION PARAMATERS
===============
*/
void	CG_SpawnParticle (float org0,					float org1,					float org2,
						float angle0,					float angle1,				float angle2,
						float vel0,						float vel1,					float vel2,
						float accel0,					float accel1,				float accel2,
						float red,						float green,				float blue,
						float redVel,					float greenVel,				float blueVel,
						float alpha,					float alphaVel,
						float size,						float sizeVel,
						uint32 type,					uint32 flags,
						void (*think)(struct cgParticle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time),
						qBool thinkNext,
						byte style,
						float orient)
{
	cgParticle_t		*p = NULL;

	p = CG_AllocParticle ();
	p->time = (float)cg.realTime;

	Vec3Set (p->org, org0, org1, org2);
	Vec3Copy (p->org, p->oldOrigin);

	Vec3Set (p->angle, angle0, angle1, angle2);
	Vec3Set (p->vel, vel0, vel1, vel2);
	Vec3Set (p->accel, accel0, accel1, accel2);

	Vec4Set (p->color, red, green, blue, alpha);
	Vec4Set (p->colorVel, redVel, greenVel, blueVel, alphaVel);

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
CG_ClearParticles
===============
*/
void CG_ClearParticles (void)
{
	int		i;

	// Link particles
	cg_freeParticles = &cg_particleList[0];
	cg_particleHeadNode.prev = &cg_particleHeadNode;
	cg_particleHeadNode.next = &cg_particleHeadNode;
	for (i=0 ; i<MAX_PARTICLES ; i++) {
		if (i < MAX_PARTICLES-1)
			cg_particleList[i].next = &cg_particleList[i+1];

		// Store static poly info
		cg_particleList[i].outPoly.numVerts = 4;
		cg_particleList[i].outPoly.colors = cg_particleList[i].outColor;
		cg_particleList[i].outPoly.texCoords = cg_particleList[i].outCoords;
		cg_particleList[i].outPoly.vertices = cg_particleList[i].outVertices;
		cg_particleList[i].outPoly.shaderTime = 0;
	}

	cg_particleList[MAX_PARTICLES-1].next = NULL;
}


/*
===============
CG_AddParticles
===============
*/
void CG_AddParticles (void)
{
	cgParticle_t	*p, *next, *hNode;
	vec3_t			org, temp;
	vec4_t			color;
	float			size, orient;
	float			time, time2, dist;
	int				i, j, pointBits;
	float			lightest;
	vec3_t			shade;
	float			scale;
	vec3_t			p_upVec, p_rtVec;
	vec3_t			a_upVec, a_rtVec;
	vec3_t			point, width, move;
	bvec4_t			outColor;
	vec3_t			delta, vdelta;
	int				num;

	CG_AddMapFXToList ();
	CG_AddSustains ();
	if (!cl_add_particles->intVal)
		return;

	Vec3Scale (cg.refDef.viewAxis[2], 0.75f, p_upVec);
	Vec3Scale (cg.refDef.rightVec, 0.75f, p_rtVec);

	num = 0;
	hNode = &cg_particleHeadNode;
	for (p=hNode->prev ; p!=hNode ; p=next) {
		next = p->prev;
		num++;

		if (p->colorVel[3] > PART_INSTANT) {
			time = (cg.realTime - p->time)*0.001f;
			color[3] = p->color[3] + time*p->colorVel[3];
		}
		else {
			time = 1;
			color[3] = p->color[3];
		}

		// Faded out
		if (color[3] <= 0.0001f || num > cg_particleMax->intVal) {
			CG_FreeParticle (p);
			continue;
		}

		if (color[3] > 1.0)
			color[3] = 1.0f;

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
			if (cg_particleCulling->intVal) {
				// Kill particles behind the view
				Vec3Subtract (org, cg.refDef.viewOrigin, temp);
				VectorNormalizeFastf (temp);
				if (DotProduct (temp, cg.refDef.viewAxis[0]) < 0)
					goto nextParticle;

				// Lessen fillrate consumption
				if (!(p->flags & PF_NOCLOSECULL)) {
					dist = Vec3Dist (cg.refDef.viewOrigin, org);
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
		Vec3Copy (p->color, color);
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
		if ((p->flags & PF_SHADE) && cg_particleShading->intVal) {
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
			Vec3Scale (color, color[3], color);

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
		if (cg.currGameMod != GAME_MOD_LOX && cg.currGameMod != GAME_MOD_GIEX) { // FIXME: yay hack
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
			scale = (org[0] - cg.refDef.viewOrigin[0]) * cg.refDef.viewAxis[0][0] +
					(org[1] - cg.refDef.viewOrigin[1]) * cg.refDef.viewAxis[0][1] +
					(org[2] - cg.refDef.viewOrigin[2]) * cg.refDef.viewAxis[0][2];

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
			Angles_Vectors (p->angle, NULL, a_rtVec, a_upVec); 

			if (orient) {
				float c = (float)cos (DEG2RAD (orient)) * scale;
				float s = (float)sin (DEG2RAD (orient)) * scale;

				// Top left
				Vec2Set (p->outCoords[0], 0, 0);
				Vec3Set (p->outVertices[0],	org[0] + a_upVec[0]*s - a_rtVec[0]*c,
											org[1] + a_upVec[1]*s - a_rtVec[1]*c,
											org[2] + a_upVec[2]*s - a_rtVec[2]*c);

				// Bottom left
				Vec2Set (p->outCoords[1], 0, 1);
				Vec3Set (p->outVertices[1],	org[0] - a_upVec[0]*c - a_rtVec[0]*s,
											org[1] - a_upVec[1]*c - a_rtVec[1]*s,
											org[2] - a_upVec[2]*c - a_rtVec[2]*s);

				// Bottom right
				Vec2Set (p->outCoords[2], 1, 1);
				Vec3Set (p->outVertices[2],	org[0] - a_upVec[0]*s + a_rtVec[0]*c,
											org[1] - a_upVec[1]*s + a_rtVec[1]*c,
											org[2] - a_upVec[2]*s + a_rtVec[2]*c);

				// Top right
				Vec2Set (p->outCoords[3], 1, 0);
				Vec3Set (p->outVertices[3],	org[0] + a_upVec[0]*c + a_rtVec[0]*s,
											org[1] + a_upVec[1]*c + a_rtVec[1]*s,
											org[2] + a_upVec[2]*c + a_rtVec[2]*s);
			}
			else {
				// Top left
				Vec2Set (p->outCoords[0], 0, 0);
				Vec3Set (p->outVertices[0],	org[0] + a_upVec[0]*scale - a_rtVec[0]*scale,
											org[1] + a_upVec[1]*scale - a_rtVec[1]*scale,
											org[2] + a_upVec[2]*scale - a_rtVec[2]*scale);

				// Bottom left
				Vec2Set (p->outCoords[1], 0, 1);
				Vec3Set (p->outVertices[1],	org[0] - a_upVec[0]*scale - a_rtVec[0]*scale,
											org[1] - a_upVec[1]*scale - a_rtVec[1]*scale,
											org[2] - a_upVec[2]*scale - a_rtVec[2]*scale);

				// Bottom right
				Vec2Set (p->outCoords[2], 1, 1);
				Vec3Set (p->outVertices[2],	org[0] - a_upVec[0]*scale + a_rtVec[0]*scale,
											org[1] - a_upVec[1]*scale + a_rtVec[1]*scale,
											org[2] - a_upVec[2]*scale + a_rtVec[2]*scale);

				// Top right
				Vec2Set (p->outCoords[3], 1, 0);
				Vec3Set (p->outVertices[3],	org[0] + a_upVec[0]*scale + a_rtVec[0]*scale,
											org[1] + a_upVec[1]*scale + a_rtVec[1]*scale,
											org[2] + a_upVec[2]*scale + a_rtVec[2]*scale);
			}

			// Render it
			*(int *)p->outColor[0] = *(int *)outColor;
			*(int *)p->outColor[1] = *(int *)outColor;
			*(int *)p->outColor[2] = *(int *)outColor;
			*(int *)p->outColor[3] = *(int *)outColor;

			p->outPoly.shader = p->shader;
			Vec3Copy (p->org, p->outPoly.origin);
			p->outPoly.radius = scale;

			cgi.R_AddPoly (&p->outPoly);
			break;

		case PART_STYLE_BEAM:
			Vec3Subtract (org, cg.refDef.viewOrigin, point);
			CrossProduct (point, p->angle, width);
			VectorNormalizeFastf (width);
			Vec3Scale (width, scale, width);

			Vec3Add (org, p->angle, delta);

			dist = Vec3Dist (org, delta);

			Vec2Set (p->outCoords[0], 1, dist);
			Vec3Set (p->outVertices[0], org[0] + width[0],
										org[1] + width[1],
										org[2] + width[2]);

			Vec2Set (p->outCoords[1], 0, 0);
			Vec3Set (p->outVertices[1], org[0] - width[0],
										org[1] - width[1],
										org[2] - width[2]);

			Vec3Add (point, p->angle, point);
			CrossProduct (point, p->angle, width);
			VectorNormalizeFastf (width);
			Vec3Scale (width, scale, width);

			Vec2Set (p->outCoords[2], 0, 0);
			Vec3Set (p->outVertices[2], org[0] + p->angle[0] - width[0],
										org[1] + p->angle[1] - width[1],
										org[2] + p->angle[2] - width[2]);

			Vec2Set (p->outCoords[3], 1, dist);
			Vec3Set (p->outVertices[3], org[0] + p->angle[0] + width[0],
										org[1] + p->angle[1] + width[1],
										org[2] + p->angle[2] + width[2]);

			// Render it
			*(int *)p->outColor[0] = *(int *)outColor;
			*(int *)p->outColor[1] = *(int *)outColor;
			*(int *)p->outColor[2] = *(int *)outColor;
			*(int *)p->outColor[3] = *(int *)outColor;

			p->outPoly.shader = p->shader;
			Vec3Copy (p->org, p->outPoly.origin);
			p->outPoly.radius = Vec3Dist (org, delta);

			cgi.R_AddPoly (&p->outPoly);
			break;

		case PART_STYLE_DIRECTION:
			Vec3Add (p->angle, org, vdelta);

			Vec3Subtract (org, vdelta, move);
			VectorNormalizeFastf (move);

			Vec3Copy (move, a_upVec);
			Vec3Subtract (cg.refDef.viewOrigin, vdelta, delta);
			CrossProduct (a_upVec, delta, a_rtVec);

			VectorNormalizeFastf (a_rtVec);

			Vec3Scale (a_rtVec, 0.75f, a_rtVec);
			Vec3Scale (a_upVec, 0.75f * Vec3Length (p->angle), a_upVec);

			// Top left
			Vec2Set (p->outCoords[0], 0, 0);
			Vec3Set (p->outVertices[0], org[0] + a_upVec[0]*scale - a_rtVec[0]*scale,
										org[1] + a_upVec[1]*scale - a_rtVec[1]*scale,
										org[2] + a_upVec[2]*scale - a_rtVec[2]*scale);

			// Bottom left
			Vec2Set (p->outCoords[1], 0, 1);
			Vec3Set (p->outVertices[1], org[0] - a_upVec[0]*scale - a_rtVec[0]*scale,
										org[1] - a_upVec[1]*scale - a_rtVec[1]*scale,
										org[2] - a_upVec[2]*scale - a_rtVec[2]*scale);

			// Bottom right
			Vec2Set (p->outCoords[2], 1, 1);
			Vec3Set (p->outVertices[2], org[0] - a_upVec[0]*scale + a_rtVec[0]*scale,
										org[1] - a_upVec[1]*scale + a_rtVec[1]*scale,
										org[2] - a_upVec[2]*scale + a_rtVec[2]*scale);

			// Top right
			Vec2Set (p->outCoords[3], 1, 0);
			Vec3Set (p->outVertices[3], org[0] + a_upVec[0]*scale + a_rtVec[0]*scale,
										org[1] + a_upVec[1]*scale + a_rtVec[1]*scale,
										org[2] + a_upVec[2]*scale + a_rtVec[2]*scale);

			// Render it
			*(int *)p->outColor[0] = *(int *)outColor;
			*(int *)p->outColor[1] = *(int *)outColor;
			*(int *)p->outColor[2] = *(int *)outColor;
			*(int *)p->outColor[3] = *(int *)outColor;

			p->outPoly.shader = p->shader;
			Vec3Copy (p->org, p->outPoly.origin);
			p->outPoly.radius = scale;

			cgi.R_AddPoly (&p->outPoly);
			break;

		default:
		case PART_STYLE_QUAD:
			if (orient) {
				float c = (float)cos (DEG2RAD (orient)) * scale;
				float s = (float)sin (DEG2RAD (orient)) * scale;

				// Top left
				Vec2Set (p->outCoords[0], 0.0, 0.0);
				Vec3Set (p->outVertices[0],	org[0] + cg.refDef.viewAxis[1][0]*c + cg.refDef.viewAxis[2][0]*s,
											org[1] + cg.refDef.viewAxis[1][1]*c + cg.refDef.viewAxis[2][1]*s,
											org[2] + cg.refDef.viewAxis[1][2]*c + cg.refDef.viewAxis[2][2]*s);

				// Bottom left
				Vec2Set (p->outCoords[1], 0.0, 1.0);
				Vec3Set (p->outVertices[1],	org[0] - cg.refDef.viewAxis[1][0]*s + cg.refDef.viewAxis[2][0]*c,
											org[1] - cg.refDef.viewAxis[1][1]*s + cg.refDef.viewAxis[2][1]*c,
											org[2] - cg.refDef.viewAxis[1][2]*s + cg.refDef.viewAxis[2][2]*c);

				// Bottom right
				Vec2Set (p->outCoords[2], 1.0, 1.0);
				Vec3Set (p->outVertices[2],	org[0] - cg.refDef.viewAxis[1][0]*c - cg.refDef.viewAxis[2][0]*s,
											org[1] - cg.refDef.viewAxis[1][1]*c - cg.refDef.viewAxis[2][1]*s,
											org[2] - cg.refDef.viewAxis[1][2]*c - cg.refDef.viewAxis[2][2]*s);

				// Top right
				Vec2Set (p->outCoords[3], 1.0, 0.0);
				Vec3Set (p->outVertices[3],	org[0] + cg.refDef.viewAxis[1][0]*s - cg.refDef.viewAxis[2][0]*c,
											org[1] + cg.refDef.viewAxis[1][1]*s - cg.refDef.viewAxis[2][1]*c,
											org[2] + cg.refDef.viewAxis[1][2]*s - cg.refDef.viewAxis[2][2]*c);
			}
			else {
				// Top left
				Vec2Set (p->outCoords[0], 0, 0);
				Vec3Set (p->outVertices[0],	org[0] + cg.refDef.viewAxis[2][0]*scale + cg.refDef.viewAxis[1][0]*scale,
											org[1] + cg.refDef.viewAxis[2][1]*scale + cg.refDef.viewAxis[1][1]*scale,
											org[2] + cg.refDef.viewAxis[2][2]*scale + cg.refDef.viewAxis[1][2]*scale);

				// Bottom left
				Vec2Set (p->outCoords[1], 0, 1);
				Vec3Set (p->outVertices[1],	org[0] - cg.refDef.viewAxis[2][0]*scale + cg.refDef.viewAxis[1][0]*scale,
											org[1] - cg.refDef.viewAxis[2][1]*scale + cg.refDef.viewAxis[1][1]*scale,
											org[2] - cg.refDef.viewAxis[2][2]*scale + cg.refDef.viewAxis[1][2]*scale);

				// Bottom right
				Vec2Set (p->outCoords[2], 1, 1);
				Vec3Set (p->outVertices[2],	org[0] - cg.refDef.viewAxis[2][0]*scale - cg.refDef.viewAxis[1][0]*scale,
											org[1] - cg.refDef.viewAxis[2][1]*scale - cg.refDef.viewAxis[1][1]*scale,
											org[2] - cg.refDef.viewAxis[2][2]*scale - cg.refDef.viewAxis[1][2]*scale);

				// Top right
				Vec2Set (p->outCoords[3], 1, 0);
				Vec3Set (p->outVertices[3],	org[0] + cg.refDef.viewAxis[2][0]*scale - cg.refDef.viewAxis[1][0]*scale,
											org[1] + cg.refDef.viewAxis[2][1]*scale - cg.refDef.viewAxis[1][1]*scale,
											org[2] + cg.refDef.viewAxis[2][2]*scale - cg.refDef.viewAxis[1][2]*scale);
			}

			// Render it
			*(int *)p->outColor[0] = *(int *)outColor;
			*(int *)p->outColor[1] = *(int *)outColor;
			*(int *)p->outColor[2] = *(int *)outColor;
			*(int *)p->outColor[3] = *(int *)outColor;

			p->outPoly.shader = p->shader;
			Vec3Copy (p->org, p->outPoly.origin);
			p->outPoly.radius = scale;

			cgi.R_AddPoly (&p->outPoly);
			break;
		}

nextParticle:
		Vec3Copy (org, p->oldOrigin);

		// Kill if instant
		if (p->colorVel[3] <= PART_INSTANT) {
			p->color[3] = 0;
			p->colorVel[3] = 0;
		}
	}
}
