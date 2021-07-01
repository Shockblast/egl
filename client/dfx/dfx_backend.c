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

// cl_decal.c
// - decal stuff on the client side

#include "../cl_local.h"

/*
==============================================================

	DECAL IMAGING

==============================================================
*/

int dBloodCnt		= 6;
int dBloodTable[]	= { DT_BLOOD, DT_BLOOD2, DT_BLOOD3, DT_BLOOD4, DT_BLOOD5, DT_BLOOD6 };
int dRandBloodMark (void)	{ return dBloodTable[(rand()%dBloodCnt)]; }

int dBurnCnt		= 3;
int dBurnTable[]	= { DT_BURNMARK, DT_BURNMARK2, DT_BURNMARK3 };
int dRandBurnMark (void)	{ return dBurnTable[(rand()%dBurnCnt)]; }

int dSlashCnt		= 3;
int dSlashTable[]	= { DT_SLASH, DT_SLASH2, DT_SLASH3 };
int dRandSlashMark (void)	{ return dSlashTable[(rand()%dSlashCnt)]; }

/*
==============================================================

	DECAL MANAGEMENT

==============================================================
*/

cdecal_t	*cl_ActiveDecals;
cdecal_t	*cl_FreeDecals;
cdecal_t	cl_Decals[MAX_DECALS];

/*
===============
makeDecal
===============
*/
cdecal_t	*makeDecal (double org0,					double org1,					double org2,
						double dir0,					double dir1,					double dir2,
						double red,						double green,					double blue,
						double redVel,					double greenVel,				double blueVel,
						double alpha,					double alphaVel,
						double size,					double sizeVel,
						int type,						int flags,
						int sFactor,					int dFactor,
						void (*think)(struct cdecal_s *d, dvec4_t color, int *type, int *flags),
						qBool thinkNext,
						double lifeTime,				 double angle) {
	int			i, j;
	int			numFragments;
	vec3_t		verts[MAX_DECAL_VERTS];
	vec3_t		v, origin, dir;
	fragment_t	*fr, fragments[MAX_DECAL_FRAGMENTS];
	vec3_t		axis[3];

	cdecal_t	*d = NULL;

	if (!cl_add_decals->integer || !cl_decal_life->value || !cl_FreeDecals)
		return NULL;

	VectorSet (dir, dir0, dir1, dir2);
	VectorSet (origin, org0, org1, org2);

	// invalid decal
	if ((size == 0) || VectorCompare (dir, vec3Identity))
		return NULL;

	if (size < 0)
		size *= -1;

	// calculate orientation matrix
	VectorNormalize (dir, axis[0]);
	PerpendicularVector (axis[0], axis[1]);
	RotatePointAroundVector (axis[2], axis[0], axis[1], angle);
	CrossProduct (axis[0], axis[2], axis[1]);

	// clip it
	numFragments = R_GetClippedFragments (origin, size, axis,
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

		if (!cl_FreeDecals)
			break;

		d = cl_FreeDecals;
		cl_FreeDecals = d->next;
		d->next = cl_ActiveDecals;
		cl_ActiveDecals = d;

		d->time = cls.realTime;
		d->lifeTime = lifeTime;

		VectorSet (d->org, org0, org1, org2);
		VectorSet (d->dir, dir0, dir1, dir2);

		Vector4Set (d->color, red, green, blue, alpha);
		Vector4Set (d->colorVel, redVel, greenVel, blueVel, alphaVel);

		d->angle = angle;

		d->size = size;
		d->sizeVel = sizeVel;

		d->image = clMedia.mediaTable[type%PDT_PICTOTAL];
		d->flags = flags;

		d->sFactor = sFactor;
		d->dFactor = dFactor;

		d->think = think;
		d->thinkNext = thinkNext;

		d->numVerts = fr->numVerts;
		d->node = fr->node;

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
CL_FreeDecal
===============
*/
void CL_FreeDecal (cdecal_t *d) {
	d->next = cl_FreeDecals;
	cl_FreeDecals = d;
}


/*
===============
CL_ClearDecals
===============
*/
void CL_ClearDecals (void) {
	int			i;

	// hulk smash
	cl_FreeDecals = &cl_Decals[0];
	cl_ActiveDecals = NULL;

	for (i=0 ; i<MAX_DECALS ; i++)
		cl_Decals[i].next = &cl_Decals[i + 1];

	cl_Decals[MAX_DECALS - 1].next = NULL;
}


/*
===============
CL_AddDecals
===============
*/
// need this for decal node crap
#include "../../renderer/r_local.h"
extern int	r_VisFrameCount;
void CL_AddDecals (void) {
	cdecal_t	*d, *next;
	cdecal_t	*active, *tail;
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

	for (d=cl_ActiveDecals ; d ; d=next) {
		next = d->next;
		if (d->colorVel[3] != DECAL_INSTANT) {
			lifeTime = d->lifeTime;
			if (!(d->flags & DF_NOTIMESCALE))
				lifeTime += cl_decal_life->value;

			// calculate alpha ramp
			color[3] = clamp (((lifeTime - ((cls.realTime - d->time) * 0.001)) / lifeTime), -1, 1);

			if (color[3] <= 0) {
				// faded out
				CL_FreeDecal (d);
				continue;
			}
		} else
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
		if ((d->node == NULL) || (d->node->visFrame != r_VisFrameCount))
			continue;

		// small decal lod
		if (cl_decal_lod->integer && (d->size < 12)) {
			VectorSubtract (cl.refDef.viewOrigin, d->org, temp);
			if ((DotProduct (temp, temp) / 15000) > (100 * d->size))
				continue;
		}

		// colorVel calcs
		VectorCopy (d->color, color);
		for (i=0 ; i<3 ; i++) {
			if ((d->colorVel[3] != DECAL_INSTANT) && (d->color[i] != d->colorVel[i])) {
				if (d->color[i] > d->colorVel[i])
					color[i] = d->color[i] - ((d->color[i] - d->colorVel[i]) * (d->color[3] - color[3]));
				else
					color[i] = d->color[i] + ((d->colorVel[i] - d->color[i]) * (d->color[3] - color[3]));
			}

			color[i] = clamp (color[i], 0, 255);
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
			if (CL_PMPointContents (d->org) & pointBits) {
				color[3] = 0;
				d->color[3] = 0;
				d->colorVel[3] = 0;
				continue;
			}
		} else {
			if (d->flags & DF_LAVAONLY)		pointBits |= CONTENTS_LAVA;
			if (d->flags & DF_SLIMEONLY)	pointBits |= CONTENTS_SLIME;
			if (d->flags & DF_WATERONLY)	pointBits |= CONTENTS_WATER;

			if (pointBits) {
				if (!(CL_PMPointContents (d->org) & pointBits)) {
					color[3] = 0;
					d->color[3] = 0;
					d->colorVel[3] = 0;
					continue;
				}
			}
		}

		// add to be rendered
		if (color[3] > 0)
			R_AddDecal (d->coords, d->verts, d->numVerts, d->node, color, d->image, flags, sFactor, dFactor);

		if (d->colorVel[3] == DECAL_INSTANT) {
			d->color[3] = 0.0;
			d->colorVel[3] = 0.0;
		}
	}

	cl_ActiveDecals = active;
}
