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
// cg_decals.c
//

#include "cg_local.h"

static cgDecal_t	*cg_freeDecals;
static cgDecal_t	cg_decalHeadNode, cg_decalList[MAX_REF_DECALS]; // FIXME: 1.2MB array!
static int			cg_numDecals;

/*
=============================================================================

	DECAL IMAGING

=============================================================================
*/

int dRandBloodMark (void)	{ return DT_BLOOD01 + (rand()&15); }
int dRandGrnBloodMark (void){ return DT_BLOOD01_GRN + (rand()&15); }
int dRandExploMark (void)	{ return DT_EXPLOMARK + (rand()%3); }
int dRandSlashMark (void)	{ return DT_SLASH + (rand()%3); }

/*
=============================================================================

	DECAL MANAGEMENT

=============================================================================
*/

/*
===============
CG_AllocDecal
===============
*/
static cgDecal_t *CG_AllocDecal (void)
{
	cgDecal_t	*d;

	// Take a free decal spot if possible, otherwise steal the oldest one
	if (cg_freeDecals && cg_numDecals+1 < cg_decalMax->intVal) {
		d = cg_freeDecals;
		cg_freeDecals = d->next;
	}
	else {
		d = cg_decalHeadNode.prev;
		d->prev->next = d->next;
		d->next->prev = d->prev;

		cgi.R_FreeDecal (&d->refDecal);
		cg_numDecals--;
	}

	// Move to the beginning of the list
	d->prev = &cg_decalHeadNode;
	d->next = cg_decalHeadNode.next;
	d->next->prev = d;
	d->prev->next = d;

	cg_numDecals++;
	return d;
}


/*
===============
CG_FreeDecal
===============
*/
static inline void CG_FreeDecal (cgDecal_t *d)
{
	// Remove from linked active list
	d->prev->next = d->next;
	d->next->prev = d->prev;

	// Insert into linked free list
	d->next = cg_freeDecals;
	cg_freeDecals = d;

	// Free in renderer
	cgi.R_FreeDecal (&d->refDecal);
	cg_numDecals--;
}


/*
===============
CG_SpawnDecal
===============
*/
cgDecal_t *CG_SpawnDecal (float org0,				float org1,					float org2,
						float dir0,					float dir1,					float dir2,
						float red,					float green,				float blue,
						float redVel,				float greenVel,				float blueVel,
						float alpha,				float alphaVel,
						float size,
						int type,					uint32 flags,
						void (*think)(struct cgDecal_s *d, vec4_t color, int *type, uint32 *flags),
						qBool thinkNext,
						float lifeTime,				float angle)
{
	vec3_t		origin, dir;
	cgDecal_t	*d;

	// Decal toggling
	if (!cg_decals->intVal)
		return NULL;
	if (flags & DF_USE_BURNLIFE) {
		if (!cg_decalBurnLife->floatVal)
			return NULL;
	}
	else if (!cg_decalLife->floatVal)
		return NULL;

	// Copy values
	Vec3Set (dir, dir0, dir1, dir2);
	Vec3Set (origin, org0, org1, org2);

	// Create the decal
	d = CG_AllocDecal ();
	if (!cgi.R_CreateDecal (&d->refDecal, cgMedia.decalTable[type%DT_PICTOTAL], origin, dir, angle, size)) {
		CG_FreeDecal (d);
		return NULL;
	}

	// Store values
	d->time = (float)cg.realTime;
	d->lifeTime = lifeTime;

	Vec4Set (d->color, red, green, blue, alpha);
	Vec4Set (d->colorVel, redVel, greenVel, blueVel, alphaVel);

	d->size = size;

	d->flags = flags;

	d->think = think;
	d->thinkNext = thinkNext;
	return d;
}


/*
===============
CG_ClearDecals
===============
*/
void CG_ClearDecals (void)
{
	int		i;

	cg_numDecals = 0;

	// Link decals
	cg_freeDecals = &cg_decalList[0];
	cg_decalHeadNode.prev = &cg_decalHeadNode;
	cg_decalHeadNode.next = &cg_decalHeadNode;
	for (i=0 ; i<MAX_REF_DECALS ; i++) {
		if (i < MAX_REF_DECALS-1)
			cg_decalList[i].next = &cg_decalList[i+1];

		cgi.R_FreeDecal (&cg_decalList[i].refDecal);
	}
	cg_decalList[MAX_REF_DECALS-1].next = NULL;
}


/*
===============
CG_AddDecals
===============
*/
void CG_AddDecals (void)
{
	cgDecal_t	*d, *next, *hNode;
	float		lifeTime, finalTime;
	float		fade;
	int			i, type;
	uint32		flags;
	vec4_t		color;
	vec3_t		temp;
	bvec4_t		outColor;
	int			num;

	if (!cg_decals->intVal)
		return;

	// Add to list
	num = 0;
	hNode = &cg_decalHeadNode;
	for (d=hNode->prev ; d!=hNode ; d=next) {
		next = d->prev;
		num++;

		if (d->colorVel[3] > DECAL_INSTANT) {
			// Determine how long this decal shall live for
			if (d->flags & DF_FIXED_LIFE)
				lifeTime = d->lifeTime;
			else if (d->flags & DF_USE_BURNLIFE)
				lifeTime = d->lifeTime + cg_decalBurnLife->floatVal;
			else
				lifeTime = d->lifeTime + cg_decalLife->floatVal;

			// Start fading
			finalTime = d->time + (lifeTime * 1000);
			if ((float)cg.realTime > finalTime)  {
				// Finished the life, fade for cg_decalFadeTime
				if (cg_decalFadeTime->floatVal) {
					lifeTime = cg_decalFadeTime->floatVal;

					// final alpha * ((fade time - time since death) / fade time)
					color[3] = d->colorVel[3] * ((lifeTime - (((float)cg.realTime - finalTime) * 0.001f)) / lifeTime);
				}
				else
					color[3] = 0.0f;
			}
			else {
				// Not done living, fade between start/final alpha
				fade = (lifeTime - (((float)cg.realTime - d->time) * 0.001f)) / lifeTime;
				color[3] = (fade * d->color[3]) + ((1.0f - fade) * d->colorVel[3]);
			}
		}
		else {
			color[3] = d->color[3];
		}

		// Faded out
		if (color[3] <= 0.0001f || num > cg_decalMax->intVal) {
			CG_FreeDecal (d);
			continue;
		}

		if (color[3] > 1.0f)
			color[3] = 1.0f;

		// Small decal lod
		if (cg_decalLOD->intVal && d->size < 12) {
			Vec3Subtract (cg.refDef.viewOrigin, d->refDecal.origin, temp);
			if (DotProduct(temp, temp)/15000 > 100*d->size)
				goto nextDecal;
		}

		// ColorVel calcs
		if (d->color[3] > DECAL_INSTANT) {
			for (i=0 ; i<3 ; i++) {
				if (d->color[i] != d->colorVel[i]) {
					if (d->color[i] > d->colorVel[i])
						color[i] = d->color[i] - ((d->color[i] - d->colorVel[i]) * (d->color[3] - color[3]));
					else
						color[i] = d->color[i] + ((d->colorVel[i] - d->color[i]) * (d->color[3] - color[3]));
				}
				else {
					color[i] = d->color[i];
				}

				color[i] = clamp (color[i], 0, 255);
			}
		}
		else {
			Vec3Copy (d->color, color);
		}

		// Adjust ramp to desired initial and final alpha settings
		color[3] = (color[3] * d->color[3]) + ((1 - color[3]) * d->colorVel[3]);

		if (d->flags & DF_ALPHACOLOR)
			Vec3Scale (color, color[3], color);

		// Think func
		flags = d->flags;
		if (d->think && d->thinkNext) {
			d->thinkNext = qFalse;
			d->think (d, color, &type, &flags);
		}

		if (color[3] <= 0.0f)
			goto nextDecal;

		// Render it
		outColor[0] = color[0];
		outColor[1] = color[1];
		outColor[2] = color[2];
		outColor[3] = color[3] * 255;

		cgi.R_AddDecal (&d->refDecal, outColor, 0);

nextDecal:
		// Kill if instant
		if (d->colorVel[3] <= DECAL_INSTANT) {
			d->color[3] = 0.0;
			d->colorVel[3] = 0.0;
		}
	}
}
