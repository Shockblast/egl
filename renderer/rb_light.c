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
// rb_light.c
//

#include "rb_local.h"

/*
=============================================================================

	DYNAMIC LIGHTS

=============================================================================
*/

/*
=============
RB_DrawDLights
=============
*/
#define DEBUGLIGHT_SIZE	8
void RB_DrawDLights (void)
{
	int			k;
	refDLight_t	*light;
	float		a, rad;
	vec3_t		v;
	uint32		i;

	if (r_debugLighting->intVal) {
		RB_SelectTexture (0);
		RB_TextureTarget (0);
		RB_StateForBits (SB1_BLEND_ON|SB1_DEFAULT);

		for (light=ri.scn.dLightList, i=0 ; i<ri.scn.numDLights ; i++, light++) {
			//
			// Radius
			//
			qglColor4f (light->color[0], light->color[1], light->color[2], 0.2f);

			qglBegin (GL_TRIANGLE_FAN);
			qglVertex3f (light->origin[0] - (ri.def.viewAxis[0][0] * light->intensity),
				light->origin[1] - (ri.def.viewAxis[0][1] * light->intensity),
				light->origin[2] - (ri.def.viewAxis[0][2] * light->intensity));

			for (k=32 ; k>=0 ; k--) {
				a = (k / 32.0f) * (M_PI * 2.0f);

				v[0] = light->origin[0] + (-ri.def.viewAxis[1][0] * (float)cos (a) * light->intensity) + (ri.def.viewAxis[2][0] * (float)sin (a) * light->intensity);
				v[1] = light->origin[1] + (-ri.def.viewAxis[1][1] * (float)cos (a) * light->intensity) + (ri.def.viewAxis[2][1] * (float)sin (a) * light->intensity);
				v[2] = light->origin[2] + (-ri.def.viewAxis[1][2] * (float)cos (a) * light->intensity) + (ri.def.viewAxis[2][2] * (float)sin (a) * light->intensity);
				qglVertex3fv (v);
			}
			qglEnd ();

			//
			// Box
			//
			qglColor4f (light->color[0], light->color[1], light->color[2], 1);

			// Top
			qglBegin (GL_LINES);
			qglVertex3f (light->origin[0] + DEBUGLIGHT_SIZE, light->origin[1] + DEBUGLIGHT_SIZE, light->origin[2] + DEBUGLIGHT_SIZE);
			qglVertex3f (light->origin[0] - DEBUGLIGHT_SIZE, light->origin[1] + DEBUGLIGHT_SIZE, light->origin[2] + DEBUGLIGHT_SIZE);

			qglVertex3f (light->origin[0] - DEBUGLIGHT_SIZE, light->origin[1] + DEBUGLIGHT_SIZE, light->origin[2] + DEBUGLIGHT_SIZE);
			qglVertex3f (light->origin[0] - DEBUGLIGHT_SIZE, light->origin[1] - DEBUGLIGHT_SIZE, light->origin[2] + DEBUGLIGHT_SIZE);

			qglVertex3f (light->origin[0] - DEBUGLIGHT_SIZE, light->origin[1] - DEBUGLIGHT_SIZE, light->origin[2] + DEBUGLIGHT_SIZE);
			qglVertex3f (light->origin[0] + DEBUGLIGHT_SIZE, light->origin[1] - DEBUGLIGHT_SIZE, light->origin[2] + DEBUGLIGHT_SIZE);

			qglVertex3f (light->origin[0] + DEBUGLIGHT_SIZE, light->origin[1] - DEBUGLIGHT_SIZE, light->origin[2] + DEBUGLIGHT_SIZE);
			qglVertex3f (light->origin[0] + DEBUGLIGHT_SIZE, light->origin[1] + DEBUGLIGHT_SIZE, light->origin[2] + DEBUGLIGHT_SIZE);
			qglEnd ();

			// Bottom
			qglBegin (GL_LINES);
			qglVertex3f (light->origin[0] + DEBUGLIGHT_SIZE, light->origin[1] + DEBUGLIGHT_SIZE, light->origin[2] - DEBUGLIGHT_SIZE);
			qglVertex3f (light->origin[0] - DEBUGLIGHT_SIZE, light->origin[1] + DEBUGLIGHT_SIZE, light->origin[2] - DEBUGLIGHT_SIZE);

			qglVertex3f (light->origin[0] - DEBUGLIGHT_SIZE, light->origin[1] + DEBUGLIGHT_SIZE, light->origin[2] - DEBUGLIGHT_SIZE);
			qglVertex3f (light->origin[0] - DEBUGLIGHT_SIZE, light->origin[1] - DEBUGLIGHT_SIZE, light->origin[2] - DEBUGLIGHT_SIZE);

			qglVertex3f (light->origin[0] - DEBUGLIGHT_SIZE, light->origin[1] - DEBUGLIGHT_SIZE, light->origin[2] - DEBUGLIGHT_SIZE);
			qglVertex3f (light->origin[0] + DEBUGLIGHT_SIZE, light->origin[1] - DEBUGLIGHT_SIZE, light->origin[2] - DEBUGLIGHT_SIZE);

			qglVertex3f (light->origin[0] + DEBUGLIGHT_SIZE, light->origin[1] - DEBUGLIGHT_SIZE, light->origin[2] - DEBUGLIGHT_SIZE);
			qglVertex3f (light->origin[0] + DEBUGLIGHT_SIZE, light->origin[1] + DEBUGLIGHT_SIZE, light->origin[2] - DEBUGLIGHT_SIZE);
			qglEnd ();

			// Corners
			qglBegin (GL_LINES);
			qglVertex3f (light->origin[0] + DEBUGLIGHT_SIZE, light->origin[1] + DEBUGLIGHT_SIZE, light->origin[2] + DEBUGLIGHT_SIZE);
			qglVertex3f (light->origin[0] + DEBUGLIGHT_SIZE, light->origin[1] + DEBUGLIGHT_SIZE, light->origin[2] - DEBUGLIGHT_SIZE);

			qglVertex3f (light->origin[0] - DEBUGLIGHT_SIZE, light->origin[1] + DEBUGLIGHT_SIZE, light->origin[2] + DEBUGLIGHT_SIZE);
			qglVertex3f (light->origin[0] - DEBUGLIGHT_SIZE, light->origin[1] + DEBUGLIGHT_SIZE, light->origin[2] - DEBUGLIGHT_SIZE);

			qglVertex3f (light->origin[0] - DEBUGLIGHT_SIZE, light->origin[1] - DEBUGLIGHT_SIZE, light->origin[2] + DEBUGLIGHT_SIZE);
			qglVertex3f (light->origin[0] - DEBUGLIGHT_SIZE, light->origin[1] - DEBUGLIGHT_SIZE, light->origin[2] - DEBUGLIGHT_SIZE);

			qglVertex3f (light->origin[0] + DEBUGLIGHT_SIZE, light->origin[1] - DEBUGLIGHT_SIZE, light->origin[2] + DEBUGLIGHT_SIZE);
			qglVertex3f (light->origin[0] + DEBUGLIGHT_SIZE, light->origin[1] - DEBUGLIGHT_SIZE, light->origin[2] - DEBUGLIGHT_SIZE);
			qglEnd ();

			//
			// Bounds
			//
			qglColor4f (light->color[0], light->color[1], light->color[2], 1);

			// Top
			qglBegin (GL_LINES);
			qglVertex3f (light->maxs[0], light->maxs[1], light->maxs[2]);
			qglVertex3f (light->mins[0], light->maxs[1], light->maxs[2]);

			qglVertex3f (light->mins[0], light->maxs[1], light->maxs[2]);
			qglVertex3f (light->mins[0], light->mins[1], light->maxs[2]);

			qglVertex3f (light->mins[0], light->mins[1], light->maxs[2]);
			qglVertex3f (light->maxs[0], light->mins[1], light->maxs[2]);

			qglVertex3f (light->maxs[0], light->mins[1], light->maxs[2]);
			qglVertex3f (light->maxs[0], light->maxs[1], light->maxs[2]);
			qglEnd ();

			// Bottom
			qglBegin (GL_LINES);
			qglVertex3f (light->maxs[0], light->maxs[1], light->mins[2]);
			qglVertex3f (light->mins[0], light->maxs[1], light->mins[2]);

			qglVertex3f (light->mins[0], light->maxs[1], light->mins[2]);
			qglVertex3f (light->mins[0], light->mins[1], light->mins[2]);

			qglVertex3f (light->mins[0], light->mins[1], light->mins[2]);
			qglVertex3f (light->maxs[0], light->mins[1], light->mins[2]);

			qglVertex3f (light->maxs[0], light->mins[1], light->mins[2]);
			qglVertex3f (light->maxs[0], light->maxs[1], light->mins[2]);
			qglEnd ();

			// Corners
			qglBegin (GL_LINES);
			qglVertex3f (light->maxs[0], light->maxs[1], light->maxs[2]);
			qglVertex3f (light->maxs[0], light->maxs[1], light->mins[2]);

			qglVertex3f (light->mins[0], light->maxs[1], light->maxs[2]);
			qglVertex3f (light->mins[0], light->maxs[1], light->mins[2]);

			qglVertex3f (light->mins[0], light->mins[1], light->maxs[2]);
			qglVertex3f (light->mins[0], light->mins[1], light->mins[2]);

			qglVertex3f (light->maxs[0], light->mins[1], light->maxs[2]);
			qglVertex3f (light->maxs[0], light->mins[1], light->mins[2]);
			qglEnd ();
		}

		RB_StateForBits (SB1_DEPTHMASK_ON|SB1_DEFAULT);
		RB_TextureTarget (GL_TEXTURE_2D);
	}

	if (!gl_flashblend->intVal)
		return;

	RB_SelectTexture (0);
	RB_TextureTarget (0);
	RB_StateForBits (SB1_DEPTHTEST_ON|SB1_BLEND_ON|SB1_BLENDSRC_ONE|SB1_BLENDDST_ONE);

	for (light=ri.scn.dLightList, i=0 ; i<ri.scn.numDLights ; i++, light++) {
		rad = light->intensity * 0.7f;
		qglBegin (GL_TRIANGLE_FAN);

		qglColor3f (light->color[0] * 0.2f, light->color[1] * 0.2f, light->color[2] * 0.2f);

		v[0] = light->origin[0] - (ri.def.viewAxis[0][0] * rad);
		v[1] = light->origin[1] - (ri.def.viewAxis[0][1] * rad);
		v[2] = light->origin[2] - (ri.def.viewAxis[0][2] * rad);
		qglVertex3fv (v);

		qglColor3f (0, 0, 0);
		for (k=32 ; k>=0 ; k--) {
			a = (k / 32.0f) * (M_PI * 2.0f);

			v[0] = light->origin[0] + (ri.def.rightVec[0] * (float)cos (a) * rad) + (ri.def.viewAxis[2][0] * (float)sin (a) * rad);
			v[1] = light->origin[1] + (ri.def.rightVec[1] * (float)cos (a) * rad) + (ri.def.viewAxis[2][1] * (float)sin (a) * rad);
			v[2] = light->origin[2] + (ri.def.rightVec[2] * (float)cos (a) * rad) + (ri.def.viewAxis[2][2] * (float)sin (a) * rad);
			qglVertex3fv (v);
		}

		qglEnd ();
	}

	RB_StateForBits (SB1_DEPTHMASK_ON|SB1_DEFAULT);
	RB_TextureTarget (GL_TEXTURE_2D);
}
