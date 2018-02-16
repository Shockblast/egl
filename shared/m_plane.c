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
// m_plane.c
//

#include "shared.h"

/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
int BoxOnPlaneSide (vec3_t mins, vec3_t maxs, cBspPlane_t *p)
{
	float	dist1, dist2;
	int		sides;

	// Fast axial cases
	if (p->type < 3) {
		if (p->dist <= mins[p->type])
			return 1;
		if (p->dist >= maxs[p->type])
			return 2;
		return 3;
	}
	
	// General case
	switch (p->signBits) {
	case 0:
		dist1 = p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2];
		dist2 = p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2];
		break;

	case 1:
		dist1 = p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2];
		dist2 = p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2];
		break;

	case 2:
		dist1 = p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2];
		dist2 = p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2];
		break;

	case 3:
		dist1 = p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2];
		dist2 = p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2];
		break;

	case 4:
		dist1 = p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2];
		dist2 = p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2];
		break;

	case 5:
		dist1 = p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2];
		dist2 = p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2];
		break;

	case 6:
		dist1 = p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2];
		dist2 = p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2];
		break;

	case 7:
		dist1 = p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2];
		dist2 = p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2];
		break;

	default:
		dist1 = 0;	// Shut up compiler
		dist2 = 0;	// Shut up compiler
		break;
	}

	sides = 0;
	if (dist1 >= p->dist)
		sides = 1;
	if (dist2 < p->dist)
		sides |= 2;

	return sides;
}


/*
=================
PlaneTypeForNormal
=================
*/
int	PlaneTypeForNormal (vec3_t normal)
{
	float	ax, ay, az;

	// NOTE: should these have an epsilon around 1.0?		
	if (normal[0] >= 1.0)
		return PLANE_X;
	if (normal[1] >= 1.0)
		return PLANE_Y;
	if (normal[2] >= 1.0)
		return PLANE_Z;

	ax = (float)fabs (normal[0]);
	ay = (float)fabs (normal[1]);
	az = (float)fabs (normal[2]);

	if (ax >= ay && ax >= az)
		return PLANE_ANYX;
	if (ay >= ax && ay >= az)
		return PLANE_ANYY;
	return PLANE_ANYZ;
}


/*
=================
CategorizePlane

A slightly simplier version of SignbitsForPlane and PlaneTypeForNormal
=================
*/
void CategorizePlane (cBspPlane_t *plane)
{
	byte	i;

	plane->signBits = 0;
	plane->type = PLANE_NON_AXIAL;
	for (i=0 ; i<3 ; i++) {
		if (plane->normal[i] < 0)
			plane->signBits |= 1<<i;
		if (plane->normal[i] == 1.0f)
			plane->type = i;
	}
}


/*
=================
PlaneFromPoints
=================
*/
void PlaneFromPoints (vec3_t verts[3], cBspPlane_t *plane)
{
	vec3_t	v1, v2;

	Vec3Subtract (verts[1], verts[0], v1);
	Vec3Subtract (verts[2], verts[0], v2);
	CrossProduct (v2, v1, plane->normal);
	VectorNormalizef (plane->normal, plane->normal);
	plane->dist = DotProduct (verts[0], plane->normal);
}

#define PLANE_NORMAL_EPSILON	0.00001
#define PLANE_DIST_EPSILON		0.01

/*
=================
ComparePlanes
=================
*/
qBool ComparePlanes (const vec3_t p1normal, float p1dist, const vec3_t p2normal, float p2dist)
{
	if (fabs (p1normal[0] - p2normal[0]) < PLANE_NORMAL_EPSILON
	&& fabs (p1normal[1] - p2normal[1]) < PLANE_NORMAL_EPSILON
	&& fabs (p1normal[2] - p2normal[2]) < PLANE_NORMAL_EPSILON
	&& fabs (p1dist - p2dist) < PLANE_DIST_EPSILON)
		return qTrue;

	return qFalse;
}


/*
==============
SnapVector
==============
*/
void SnapVector (vec3_t normal)
{
	int		i;

	for (i=0 ; i<3 ; i++) {
		if (fabs (normal[i] - 1) < PLANE_NORMAL_EPSILON) {
			Vec3Clear (normal);
			normal[i] = 1;
			break;
		}
		if (fabs (normal[i] - -1) < PLANE_NORMAL_EPSILON) {
			Vec3Clear (normal);
			normal[i] = -1;
			break;
		}
	}
}


/*
===============
ProjectPointOnPlane
===============
*/
void ProjectPointOnPlane (vec3_t dst, vec3_t p, vec3_t normal)
{
	float	d;
	vec3_t	n;
	float	invDenom;

	invDenom = 1.0f / DotProduct (normal, normal);

	d = DotProduct (normal, p) * invDenom;

	n[0] = normal[0] * invDenom;
	n[1] = normal[1] * invDenom;
	n[2] = normal[2] * invDenom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}


/*
===============
SignbitsForPlane
===============
*/
int SignbitsForPlane (cBspPlane_t *out)
{
	int		bits;

	// For fast box on planeside test
	bits = 0;
	if (out->normal[0] < 0)
		bits |= 1 << 0;
	if (out->normal[1] < 0)
		bits |= 1 << 1;
	if (out->normal[2] < 0)
		bits |= 1 << 2;

	return bits;
}
