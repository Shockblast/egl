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
// r_cull.c
//

#include "r_local.h"

cBspPlane_t	r_viewFrustum[4];

/*
=============================================================================

	FRUSTUM CULLING

=============================================================================
*/

/*
===============
R_SetupFrustum
===============
*/
void R_SetupFrustum (void)
{
	int		i;

	// Build the transformation matrix for the given view angles
	AngleVectors (r_refDef.viewAngles, r_forwardVec, r_rightVec, r_upVec);

	// Calculate the view frustum
	RotatePointAroundVector (r_viewFrustum[0].normal, r_upVec,		r_forwardVec,	-(90-r_refDef.fovX / 2));
	RotatePointAroundVector (r_viewFrustum[1].normal, r_upVec,		r_forwardVec,	90-r_refDef.fovX / 2);
	RotatePointAroundVector (r_viewFrustum[2].normal, r_rightVec,	r_forwardVec,	90-r_refDef.fovY / 2);
	RotatePointAroundVector (r_viewFrustum[3].normal, r_rightVec,	r_forwardVec,	-(90 - r_refDef.fovY / 2));

	for (i=0 ; i<4 ; i++) {
		r_viewFrustum[i].type = PLANE_ANYZ;
		r_viewFrustum[i].dist = DotProduct (r_refDef.viewOrigin, r_viewFrustum[i].normal);
		r_viewFrustum[i].signBits = SignbitsForPlane (&r_viewFrustum[i]);
	}
}


/*
=================
R_CullBox

Returns qTrue if the box is completely outside the frustum
=================
*/
qBool R_CullBox (vec3_t mins, vec3_t maxs, int clipFlags)
{
	int			i;
	cBspPlane_t	*p;

	if (r_noCull->integer)
		return qFalse;

	for (i=0, p=r_viewFrustum ; i<4 ; i++, p++) {
		if (!(clipFlags & (1<<i)))
			continue;

		switch (p->signBits) {
		case 0:
			if (p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist)
				return qTrue;
			break;

		case 1:
			if (p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist)
				return qTrue;
			break;

		case 2:
			if (p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist)
				return qTrue;
			break;

		case 3:
			if (p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist)
				return qTrue;
			break;

		case 4:
			if (p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist)
				return qTrue;
			break;

		case 5:
			if (p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist)
				return qTrue;
			break;

		case 6:
			if (p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist)
				return qTrue;
			break;

		case 7:
			if (p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist)
				return qTrue;
			break;

		default:
			assert (0);
			return qFalse;
		}
	}

	return qFalse;
}


/*
=================
R_CullSphere

Returns qTrue if the sphere is completely outside the frustum
=================
*/
qBool R_CullSphere (const vec3_t origin, const float radius, int clipFlags)
{
	int			i;
	cBspPlane_t	*p;

	if (!r_noCull->integer)
		return qFalse;

	for (i=0, p=r_viewFrustum ; i<4 ; i++, p++) {
		if (!(clipFlags & (1<<i)))
			continue;

		if (DotProduct (origin, p->normal) - p->dist <= -radius)
			return qTrue;
	}

	return qFalse;
}

/*
=============================================================================

	MISC CULLING

=============================================================================
*/

/*
===============
R_CullVisNode

Returns qTrue if this node hasn't been touched this frame
===============
*/
qBool R_CullVisNode (struct mBspNode_s *node)
{
	if (!node || node->visFrame == r_visFrameCount)
		return qFalse;

	return qTrue;
}
