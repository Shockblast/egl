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
// rf_cull.c
//

#include "rf_local.h"

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
	Vec3Copy (ri.def.viewAxis[0], ri.scn.viewFrustum[0].normal);
	RotatePointAroundVector (ri.scn.viewFrustum[1].normal, ri.def.viewAxis[2], ri.def.viewAxis[0], -(90-ri.def.fovX / 2));
	RotatePointAroundVector (ri.scn.viewFrustum[2].normal, ri.def.viewAxis[2], ri.def.viewAxis[0], 90-ri.def.fovX / 2);
	RotatePointAroundVector (ri.scn.viewFrustum[3].normal, ri.def.rightVec, ri.def.viewAxis[0], 90-ri.def.fovY / 2);
	RotatePointAroundVector (ri.scn.viewFrustum[4].normal, ri.def.rightVec, ri.def.viewAxis[0], -(90 - ri.def.fovY / 2));

	for (i=0 ; i<5 ; i++) {
		ri.scn.viewFrustum[i].type = PLANE_NON_AXIAL;
		ri.scn.viewFrustum[i].dist = DotProduct (ri.def.viewOrigin, ri.scn.viewFrustum[i].normal);
		ri.scn.viewFrustum[i].signBits = SignbitsForPlane (&ri.scn.viewFrustum[i]);
	}

	ri.scn.viewFrustum[0].dist += r_zNear->floatVal;
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

	if (r_noCull->intVal)
		return qFalse;

	for (i=0, p=ri.scn.viewFrustum ; i<5 ; p++, i++) {
		if (!(clipFlags & (1<<i)))
			continue;

		switch (p->signBits) {
		case 0:
			if (p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist) {
				ri.pc.cullBounds[CULL_PASS]++;
				return qTrue;
			}
			break;

		case 1:
			if (p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist) {
				ri.pc.cullBounds[CULL_PASS]++;
				return qTrue;
			}
			break;

		case 2:
			if (p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist) {
				ri.pc.cullBounds[CULL_PASS]++;
				return qTrue;
			}
			break;

		case 3:
			if (p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist) {
				ri.pc.cullBounds[CULL_PASS]++;
				return qTrue;
			}
			break;

		case 4:
			if (p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist) {
				ri.pc.cullBounds[CULL_PASS]++;
				return qTrue;
			}
			break;

		case 5:
			if (p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist) {
				ri.pc.cullBounds[CULL_PASS]++;
				return qTrue;
			}
			break;

		case 6:
			if (p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist) {
				ri.pc.cullBounds[CULL_PASS]++;
				return qTrue;
			}
			break;

		case 7:
			if (p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist) {
				ri.pc.cullBounds[CULL_PASS]++;
				return qTrue;
			}
			break;

		default:
			assert (0);
			return qFalse;
		}
	}

	ri.pc.cullBounds[CULL_FAIL]++;
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

	if (r_noCull->intVal)
		return qFalse;

	for (i=0, p=ri.scn.viewFrustum ; i<5 ; p++, i++) {
		if (!(clipFlags & (1<<i)))
			continue;

		if (DotProduct(origin, p->normal)-p->dist <= -radius) {
			ri.pc.cullRadius[CULL_PASS]++;
			return qTrue;
		}
	}

	ri.pc.cullRadius[CULL_FAIL]++;
	return qFalse;
}

/*
=================
R_PointOccluded

Returns qTrue if the origin is not visible via trace
=================
*/
qBool R_PointOccluded (const vec3_t origin)
{
	trace_t tr;
	
	tr = CM_Trace(ri.def.viewOrigin, (float *) origin, 1, CONTENTS_SOLID);

	return tr.fraction < 1.0;
}

/*
=============================================================================

	MAP VISIBILITY CULLING

=============================================================================
*/

/*
===============
R_CullNode

Returns qTrue if this node hasn't been touched this frame
===============
*/
qBool R_CullNode (mBspNode_t *node)
{
	if (r_noCull->intVal)
		return qFalse;

	if (!node || node->c.visFrame == ri.scn.visFrameCount) {
		ri.pc.cullVis[CULL_FAIL]++;
		return qFalse;
	}

	ri.pc.cullVis[CULL_PASS]++;
	return qTrue;
}


/*
===============
R_CullSurface

Returns qTrue if this surface hasn't been touched this frame
===============
*/
qBool R_CullSurface (mBspSurface_t *surf)
{
	if (r_noCull->intVal)
		return qFalse;

	if (!surf || surf->visFrame == ri.frameCount) {
		ri.pc.cullSurf[CULL_FAIL]++;
		return qFalse;
	}

	ri.pc.cullSurf[CULL_PASS]++;
	return qTrue;
}
