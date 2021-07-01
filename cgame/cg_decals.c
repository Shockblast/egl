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

static decal_t	*cg_activeDecals;
static decal_t	*cg_freeDecals;
static decal_t	cg_decalList[MAX_DECALS];

/*
=============================================================================

	DECAL IMAGING

=============================================================================
*/

static int dBloodCnt		= 6;
static int dBloodTable[]	= { DT_BLOOD, DT_BLOOD2, DT_BLOOD3, DT_BLOOD4, DT_BLOOD5, DT_BLOOD6 };
int dRandBloodMark (void)	{ return dBloodTable[(rand()%dBloodCnt)]; }

static int dGrnBloodCnt		= 6;
static int dGrnBloodTable[]	= { DT_GRNBLOOD, DT_GRNBLOOD2, DT_GRNBLOOD3, DT_GRNBLOOD4, DT_GRNBLOOD5, DT_GRNBLOOD6 };
int dRandGrnBloodMark (void){ return dBloodTable[(rand()%dGrnBloodCnt)]; }

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
decal_t		*makeDecal (float org0,					float org1,					float org2,
						float dir0,					float dir1,					float dir2,
						float red,					float green,				float blue,
						float redVel,				float greenVel,				float blueVel,
						float alpha,				float alphaVel,
						float size,
						int type,					int flags,
						void (*think)(struct decal_s *d, vec4_t color, int *type, int *flags),
						qBool thinkNext,
						float lifeTime,				 float angle)
{
	int			j;
	uInt		i, numFragments;
	vec3_t		verts[MAX_POLY_VERTS];
	vec3_t		v, origin, dir;
	fragment_t	*fr, fragments[MAX_POLY_FRAGMENTS];
	vec3_t		axis[3];

	decal_t	*d = NULL;

	// Decal toggling
	if (!cl_add_decals->integer || !cl_decal_life->value || !cg_freeDecals)
		return NULL;

	// Copy values
	VectorSet (dir, dir0, dir1, dir2);
	VectorSet (origin, org0, org1, org2);

	// Invalid decal!
	if (size == 0 || VectorCompare (dir, vec3Origin))
		return NULL;

	// Negativity check
	if (size < 0)
		size *= -1;

	// Calculate orientation matrix
	VectorNormalize (dir, axis[0]);
	PerpendicularVector (axis[0], axis[1]);
	RotatePointAroundVector (axis[2], axis[0], axis[1], angle);
	CrossProduct (axis[0], axis[2], axis[1]);

	// Clip it
	numFragments = cgi.R_GetClippedFragments (origin, size, axis,
										MAX_POLY_VERTS, verts,
										MAX_POLY_FRAGMENTS, fragments);

	// No valid fragments
	if (!numFragments)
		return NULL;
	if (numFragments > cl_decal_max->value)
		return NULL;
	if (numFragments >= MAX_DECALS)
		return NULL;

	VectorScale (axis[1], 0.5f / size, axis[1]);
	VectorScale (axis[2], 0.5f / size, axis[2]);

	for (i=0, fr=fragments ; i<numFragments ; i++, fr++) {
		vec_t	*coords;
		vec_t	*vertices;
		byte	*buffer;

		if (fr->numVerts > MAX_POLY_VERTS)
			fr->numVerts = MAX_POLY_VERTS;
		else if (fr->numVerts <= 0)
			continue;

		if (!cg_freeDecals)
			break;

		d = cg_freeDecals;
		cg_freeDecals = d->next;
		d->next = cg_activeDecals;
		cg_activeDecals = d;

		d->time = (float)cgs.realTime;
		d->lifeTime = lifeTime;

		VectorSet (d->org, org0, org1, org2);
		VectorSet (d->dir, dir0, dir1, dir2);

		Vector4Set (d->color, red, green, blue, alpha);
		Vector4Set (d->colorVel, redVel, greenVel, blueVel, alphaVel);

		d->angle = angle;

		d->size = size;

		d->shader = cgMedia.decalTable[type%DT_PICTOTAL];
		d->flags = flags;

		d->think = think;
		d->thinkNext = thinkNext;

		d->numVerts = fr->numVerts;
		d->node = fr->node;

		// Allocate coords and vertices
		buffer = CG_MemAlloc ((fr->numVerts * sizeof (vec2_t)) + (fr->numVerts * sizeof (vec3_t)) + (fr->numVerts * sizeof (bvec4_t)));
		coords = d->coords = (vec_t *)buffer;

		buffer += fr->numVerts * sizeof (vec2_t);
		vertices = d->vertices = (vec_t *)buffer;

		buffer += fr->numVerts * sizeof (vec3_t);
		d->colors = (bvec4_t *)buffer;
		for (j=0 ; j<fr->numVerts ; j++) {
			vertices[0] = verts[fr->firstVert+j][0];
			vertices[1] = verts[fr->firstVert+j][1];
			vertices[2] = verts[fr->firstVert+j][2];

			v[0] = vertices[0] - origin[0];
			v[1] = vertices[1] - origin[1];
			v[2] = vertices[2] - origin[2];

			coords[0] = clamp (DotProduct (v, axis[1]) + 0.5f, 0.0f, 1.0f);
			coords[1] = clamp (DotProduct (v, axis[2]) + 0.5f, 0.0f, 1.0f);

			coords += 2;
			vertices += 3;
		}
	}

	return d;
}


/*
===============
CG_FreeDecal
===============
*/
static inline void CG_FreeDecal (decal_t *d)
{
	if (d->coords) {
		CG_MemFree (d->coords);
		d->coords = NULL;
	}

	d->next = cg_freeDecals;
	cg_freeDecals = d;
}


/*
===============
CG_ClearDecals
===============
*/
void CG_ClearDecals (void)
{
	int			i;

	cg_freeDecals = &cg_decalList[0];
	cg_activeDecals = NULL;

	for (i=0 ; i<MAX_DECALS-1 ; i++) {
		if (cg_decalList[i].coords) {
			CG_MemFree (cg_decalList[i].coords);
			cg_decalList[i].coords = NULL;
		}

		cg_decalList[i].next = &cg_decalList[i + 1];
	}
	cg_decalList[MAX_DECALS-1].next = NULL;
}


/*
===============
CG_AddDecals
===============
*/
void CG_AddDecals (void)
{
	decal_t		*d, *next;
	decal_t		*active, *tail;
	float		lifeTime;
	int			i, flags, type;
	vec4_t		color;
	vec3_t		temp;
	int			pointBits;
	poly_t		poly;
	bvec4_t		outColor;

	if (!cl_add_decals->integer)
		return;

	active = NULL;
	tail = NULL;

	for (d=cg_activeDecals ; d ; d=next) {
		next = d->next;
		if (d->colorVel[3] > DECAL_INSTANT) {
			lifeTime = d->lifeTime;
			if (!(d->flags & DF_NOTIMESCALE))
				lifeTime += cl_decal_life->value;

			// Calculate alpha ramp
			color[3] = clamp (((lifeTime - ((cgs.realTime - d->time) * 0.001f)) / lifeTime), -1, 1);
		}
		else {
			color[3] = d->color[3];
		}

		// Faded out
		if (color[3] <= 0.0001f) {
			CG_FreeDecal (d);
			continue;
		}

		if (color[3] > 1.0f)
			color[3] = 1.0f;

		d->next = NULL;
		if (!tail)
			active = tail = d;
		else {
			tail->next = d;
			tail = d;
		}

		// Skip if not going to be visible
		if (!d->node || cgi.R_CullVisNode (d->node))
			goto nextDecal;

		// Frustum cull the largest possible size
		if (cgi.R_CullSphere (d->org, d->size + 1, 15))
			goto nextDecal;

		// Small decal lod
		if (cl_decal_lod->integer && (d->size < 12)) {
			VectorSubtract (cg.refDef.viewOrigin, d->org, temp);
			if ((DotProduct (temp, temp) / 15000) > (100 * d->size))
				goto nextDecal;
		}

		// ColorVel calcs
		VectorCopy (d->color, color);
		if (d->colorVel[3] > DECAL_INSTANT) {
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

		// Adjust ramp to desired initial and final alpha settings
		color[3] = (color[3] * d->color[3]) + ((1 - color[3]) * d->colorVel[3]);

		if (d->flags & DF_ALPHACOLOR)
			VectorScale (color, color[3], color);

		// Think func
		flags = d->flags;
		if (d->think && d->thinkNext) {
			d->thinkNext = qFalse;
			d->think (d, color, &type, &flags);
		}

		// Contents requirements
		pointBits = 0;
		if (d->flags & DF_AIRONLY) {
			pointBits |= (CONTENTS_LAVA|CONTENTS_SLIME|CONTENTS_WATER);
			if (CGI_CM_PointContents (d->org, 0) & pointBits) {
				color[3] = 0;
				d->color[3] = 0;
				d->colorVel[3] = 0;
				goto nextDecal;
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
					goto nextDecal;
				}
			}
		}

		if (color[3] <= 0.0f)
			goto nextDecal;

		// Render it
		outColor[0] = color[0];
		outColor[1] = color[1];
		outColor[2] = color[2];
		outColor[3] = color[3] * 255;
		for (i=0 ; i<d->numVerts ; i++)
			*(int *)d->colors[i] = *(int *)outColor;

		poly.numVerts = d->numVerts;
		poly.colors = (bvec4_t *)d->colors;
		poly.texCoords = (vec2_t *)d->coords;
		poly.vertices = (vec3_t *)d->vertices;
		poly.shader = d->shader;

		cgi.R_AddPoly (&poly);

nextDecal:
		// Kill if instant
		if (d->colorVel[3] <= DECAL_INSTANT) {
			d->color[3] = 0.0;
			d->colorVel[3] = 0.0;
		}
	}

	cg_activeDecals = active;
}
