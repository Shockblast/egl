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

	// Calculate the view frustum
	RotatePointAroundVector (r_refScene.viewFrustum[0].normal, r_refDef.upVec,		r_refDef.forwardVec,	-(90-r_refDef.fovX / 2));
	RotatePointAroundVector (r_refScene.viewFrustum[1].normal, r_refDef.upVec,		r_refDef.forwardVec,	90-r_refDef.fovX / 2);
	RotatePointAroundVector (r_refScene.viewFrustum[2].normal, r_refDef.rightVec,	r_refDef.forwardVec,	90-r_refDef.fovY / 2);
	RotatePointAroundVector (r_refScene.viewFrustum[3].normal, r_refDef.rightVec,	r_refDef.forwardVec,	-(90 - r_refDef.fovY / 2));

	for (i=0 ; i<4 ; i++) {
		r_refScene.viewFrustum[i].type = PLANE_ANYZ;
		r_refScene.viewFrustum[i].dist = DotProduct (r_refDef.viewOrigin, r_refScene.viewFrustum[i].normal);
		r_refScene.viewFrustum[i].signBits = SignbitsForPlane (&r_refScene.viewFrustum[i]);
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

	for (i=0, p=r_refScene.viewFrustum ; i<4 ; i++, p++) {
		if (!(clipFlags & (1<<i)))
			continue;

		switch (p->signBits) {
		case 0:
			if (p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist) {
				r_refStats.cullBounds[qTrue]++;
				return qTrue;
			}
			break;

		case 1:
			if (p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist) {
				r_refStats.cullBounds[qTrue]++;
				return qTrue;
			}
			break;

		case 2:
			if (p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist) {
				r_refStats.cullBounds[qTrue]++;
				return qTrue;
			}
			break;

		case 3:
			if (p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist) {
				r_refStats.cullBounds[qTrue]++;
				return qTrue;
			}
			break;

		case 4:
			if (p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist) {
				r_refStats.cullBounds[qTrue]++;
				return qTrue;
			}
			break;

		case 5:
			if (p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist) {
				r_refStats.cullBounds[qTrue]++;
				return qTrue;
			}
			break;

		case 6:
			if (p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist) {
				r_refStats.cullBounds[qTrue]++;
				return qTrue;
			}
			break;

		case 7:
			if (p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist) {
				r_refStats.cullBounds[qTrue]++;
				return qTrue;
			}
			break;

		default:
			assert (0);
			return qFalse;
		}
	}

	r_refStats.cullBounds[qFalse]++;
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

	if (r_noCull->integer)
		return qFalse;

	for (i=0, p=r_refScene.viewFrustum ; i<4 ; i++, p++) {
		if (!(clipFlags & (1<<i)))
			continue;

		if (DotProduct (origin, p->normal) - p->dist <= -radius) {
			r_refStats.cullRadius[qTrue]++;
			return qTrue;
		}
	}

	r_refStats.cullRadius[qFalse]++;
	return qFalse;
}

/*
=============================================================================

	MAP VISIBILITY CULLING

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
	if (r_noCull->integer)
		return qFalse;

	if (!node || node->visFrame == r_refScene.visFrameCount) {
		r_refStats.cullVis[qFalse]++;
		return qFalse;
	}

	r_refStats.cullVis[qTrue]++;
	return qTrue;
}
