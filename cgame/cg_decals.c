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

static cgDecal_t	*cgActiveDecals;
static cgDecal_t	*cgFreeDecals;
static cgDecal_t	cgDecals[MAX_DECALS];

/*
=============================================================================

	DECAL IMAGING

=============================================================================
*/

static int dBloodCnt		= 6;
static int dBloodTable[]	= { DT_BLOOD, DT_BLOOD2, DT_BLOOD3, DT_BLOOD4, DT_BLOOD5, DT_BLOOD6 };
int dRandBloodMark (void)	{ return dBloodTable[(rand()%dBloodCnt)]; }

static int dExploMarkCnt	= 3;
static int dExploMarkTable[]= { DT_EXPLOMARK, DT_EXPLOMARK2, DT_EXPLOMARK3 };
int dRandExploMark (void)	{ return dExploMarkTable[(rand()%dExploMarkCnt)]; }

static int dSlashCnt		= 3;
static int dSlashTable[]	= { DT_SLASH, DT_SLASH2, DT_SLASH3 };
int dRandSlashMark (void)	{ return dSlashTable[(rand()%dSlashCnt)]; }

/*
=============================================================================

	DECAL MANAGEMENT

=============================================================================
*/

/*
===============
makeDecal
===============
*/
cgDecal_t	*makeDecal (double org0,					double org1,					double org2,
						double dir0,					double dir1,					double dir2,
						double red,						double green,					double blue,
						double redVel,					double greenVel,				double blueVel,
						double alpha,					double alphaVel,
						double size,					double sizeVel,
						int type,						int flags,
						int sFactor,					int dFactor,
						void (*think)(struct cgDecal_s *d, dvec4_t color, int *type, int *flags),
						qBool thinkNext,
						double lifeTime,				 double angle) {
	int			i, j;
	uInt		numFragments;
	vec3_t		verts[MAX_DECAL_VERTS];
	vec3_t		v, origin, dir;
	fragment_t	*fr, fragments[MAX_DECAL_FRAGMENTS];
	vec3_t		axis[3];

	cgDecal_t	*d = NULL;

	// decal toggling
	if (!cl_add_decals->integer || !cl_decal_life->value || !cgFreeDecals)
		return NULL;

	// copy values
	VectorSet (dir, dir0, dir1, dir2);
	VectorSet (origin, org0, org1, org2);

	// invalid decal!
	if ((size == 0) || VectorCompare (dir, vec3Origin))
		return NULL;

	// negativity check
	if (size < 0)
		size *= -1;

	// calculate orientation matrix
	VectorNormalize (dir, axis[0]);
	PerpendicularVector (axis[0], axis[1]);
	RotatePointAroundVector (axis[2], axis[0], axis[1], angle);
	CrossProduct (axis[0], axis[2], axis[1]);

	// clip it
	numFragments = cgi.R_GetClippedFragments (origin, size, axis,
										MAX_DECAL_VERTS, verts,
										MAX_DECAL_FRAGMENTS, fragments);

	// no valid fragments
	if (!numFragments)
		return NULL;

	if (numFragments > cl_decal_max->value)
		return NULL;

	if (numFragments > MAX_DECALS)
		return NULL;

	VectorScale (axis[1], 0.5f / size, axis[1]);
	VectorScale (axis[2], 0.5f / size, axis[2]);

	for (i=0, fr=fragments ; i<numFragments ; i++, fr++) {
		if (fr->numVerts > MAX_DECAL_VERTS)
			fr->numVerts = MAX_DECAL_VERTS;
		else if (fr->numVerts <= 0)
			continue;

		if (!cgFreeDecals)
			break;

		d = cgFreeDecals;
		cgFreeDecals = d->next;
		d->next = cgActiveDecals;
		cgActiveDecals = d;

		d->time = cgs.realTime;
		d->lifeTime = lifeTime;

		VectorSet (d->org, org0, org1, org2);
		VectorSet (d->dir, dir0, dir1, dir2);

		Vector4Set (d->color, red, green, blue, alpha);
		Vector4Set (d->colorVel, redVel, greenVel, blueVel, alphaVel);

		d->angle = angle;

		d->size = size;
		d->sizeVel = sizeVel;

		d->image = cgMedia.mediaTable[type%PDT_PICTOTAL];
		d->flags = flags;

		d->sFactor = sFactor;
		d->dFactor = dFactor;

		d->think = think;
		d->thinkNext = thinkNext;

		d->numVerts = fr->numVerts;
		d->node = fr->node;

		d->coords = CG_MemAlloc (sizeof (vec2_t) * fr->numVerts);
		d->verts = CG_MemAlloc (sizeof (vec3_t) * fr->numVerts);
		for (j=0 ; j<fr->numVerts ; j++) {
			VectorCopy (verts[fr->firstVert+j], d->verts[j]);
			VectorSubtract (d->verts[j], origin, v);

			d->coords[j][0] = clamp (DotProduct (v, axis[1]) + 0.5f, 0.0, 1.0);
			d->coords[j][1] = clamp (DotProduct (v, axis[2]) + 0.5f, 0.0, 1.0);
		}
	}

	return d;
}


/*
===============
CG_FreeDecal
===============
*/
void CG_FreeDecal (cgDecal_t *d) {
	if (d->coords)
		CG_MemFree (d->coords);
	if (d->verts)
		CG_MemFree (d->verts);

	d->next = cgFreeDecals;
	cgFreeDecals = d;
}


/*
===============
CG_ClearDecals
===============
*/
void CG_ClearDecals (void) {
	int			i;

	// hulk smash
	memset (cgDecals, 0, sizeof (cgDecals));

	cgFreeDecals = &cgDecals[0];
	cgActiveDecals = NULL;

	for (i=0 ; i<MAX_DECALS-1 ; i++) {
		if (cgDecals[i].coords)
			CG_MemFree (cgDecals[i].coords);
		if (cgDecals[i].verts)
			CG_MemFree (cgDecals[i].verts);

		cgDecals[i].next = &cgDecals[i + 1];
	}
	cgDecals[MAX_DECALS-1].next = NULL;
}


/*
===============
CG_AddDecals
===============
*/
void CG_AddDecals (void) {
	cgDecal_t	*d, *next;
	cgDecal_t	*active, *tail;
	double		lifeTime;
	int			i, flags, type;
	int			sFactor, dFactor;
	dvec4_t		color;
	vec3_t		temp;
	int			pointBits;

	if (!cl_add_decals->integer)
		return;

	active = NULL;
	tail = NULL;

	for (d=cgActiveDecals ; d ; d=next) {
		next = d->next;
		if (d->colorVel[3] != DECAL_INSTANT) {
			lifeTime = d->lifeTime;
			if (!(d->flags & DF_NOTIMESCALE))
				lifeTime += cl_decal_life->value;

			// calculate alpha ramp
			color[3] = clamp (((lifeTime - ((cgs.realTime - d->time) * 0.001)) / lifeTime), -1, 1);

			if (color[3] <= 0) {
				// faded out
				CG_FreeDecal (d);
				continue;
			}
		}
		else
			color[3] = d->color[3];

		d->next = NULL;
		if (!tail)
			active = tail = d;
		else {
			tail->next = d;
			tail = d;
		}

		if (color[3] > 1.0)
			color[3] = 1;

		// skip if not going to be visible
		if ((d->node == NULL) || cgi.R_CullVisNode (d->node))
			continue;

		// frustum cull the largest possible size
		if (cgi.R_CullSphere (d->org, max (d->size, d->sizeVel) + 1, 15))
			continue;

		// small decal lod
		if (cl_decal_lod->integer && (d->size < 12)) {
			VectorSubtract (cg.refDef.viewOrigin, d->org, temp);
			if ((DotProduct (temp, temp) / 15000) > (100 * d->size))
				continue;
		}

		// colorVel calcs
		VectorCopy (d->color, color);
		if (d->colorVel[3] != DECAL_INSTANT) {
			for (i=0 ; i<3 ; i++) {
				if (d->color[i] != d->colorVel[i]) {
					if (d->color[i] > d->colorVel[i])
						color[i] = d->color[i] - ((d->color[i] - d->colorVel[i]) * (d->color[3] - color[3]));
					else
						color[i] = d->color[i] + ((d->colorVel[i] - d->color[i]) * (d->color[3] - color[3]));
				}

				color[i] = clamp (color[i], 0, 255);
			}
		}

		// adjust ramp to desired initial and final alpha settings
		color[3] = (color[3] * d->color[3]) + ((1 - color[3]) * d->colorVel[3]);

		if (d->flags & DF_ALPHACOLOR)
			VectorScale (color, color[3], color);

		// think func
		flags = d->flags;
		sFactor = d->sFactor;
		dFactor = d->dFactor;
		if (d->think && d->thinkNext) {
			d->thinkNext = qFalse;
			d->think (d, color, &type, &flags);
		}

		// contents requirements
		pointBits = 0;
		if (d->flags & DF_AIRONLY) {
			pointBits |= (CONTENTS_LAVA|CONTENTS_SLIME|CONTENTS_WATER);
			if (CGI_CM_PointContents (d->org, 0) & pointBits) {
				color[3] = 0;
				d->color[3] = 0;
				d->colorVel[3] = 0;
				continue;
			}
		}
		else {
			if (d->flags & DF_LAVAONLY)		pointBits |= CONTENTS_LAVA;
			if (d->flags & DF_SLIMEONLY)	pointBits |= CONTENTS_SLIME;
			if (d->flags & DF_WATERONLY)	pointBits |= CONTENTS_WATER;

			if (pointBits) {
				if (!(CGI_CM_PointContents (d->org, 0) & pointBits)) {
					color[3] = 0;
					d->color[3] = 0;
					d->colorVel[3] = 0;
					continue;
				}
			}
		}

		// add to be rendered
		if (color[3] > 0)
			cgi.R_AddDecal (d->coords, d->verts, d->numVerts, d->node, color, d->image, flags, sFactor, dFactor);

		if (d->colorVel[3] == DECAL_INSTANT) {
			d->color[3] = 0.0;
			d->colorVel[3] = 0.0;
		}
	}

	cgActiveDecals = active;
}
