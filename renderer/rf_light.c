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
// rf_light.c
// Dynamic lights
// Lightmaps
// Alias model lighting
//

#include "rf_local.h"

/*
=============================================================================

	QUAKE II DYNAMIC LIGHTS

=============================================================================
*/

// The lightmap texture data needs to be kept in
// main memory so texsubimage can update properly
static float	r_q2_blockLights[34*34*3];

static vec3_t	r_q2_pointColor;
static vec3_t	r_q2_lightSpot;

/*
=============
R_Q2BSP_MarkWorldLights
=============
*/
static void R_Q2BSP_r_MarkWorldLights (mBspNode_t *node, refDLight_t *lt, uint32 bit)
{
	mBspSurface_t	**mark, *surf;
	float			dist;

loc0:
	if (node->c.q2_contents != -1)
		return;
	if (node->c.visFrame != ri.scn.visFrameCount)
		return;

	dist = PlaneDiff (lt->origin, node->c.plane);
	if (dist > lt->intensity) {
		node = node->children[0];
		goto loc0;
	}
	if (dist < -lt->intensity) {
		node = node->children[1];
		goto loc0;
	}
	if (!BoundsIntersect (node->c.mins, node->c.maxs, lt->mins, lt->maxs))
		return;

	// Mark the polygons
	if (node->q2_firstLitSurface) {
		mark = node->q2_firstLitSurface;
		do {
			surf = *mark++;
			if (!BoundsIntersect (surf->mins, surf->maxs, lt->mins, lt->maxs))
				continue;

			if (surf->dLightFrame != ri.frameCount) {
				surf->dLightFrame = ri.frameCount;
				surf->dLightBits = 0;
			}
			surf->dLightBits |= bit;
		} while (*mark);
	}

	R_Q2BSP_r_MarkWorldLights (node->children[0], lt, bit);
	R_Q2BSP_r_MarkWorldLights (node->children[1], lt, bit);
}
void R_Q2BSP_MarkWorldLights (void)
{
	refDLight_t	*lt;
	uint32		i;

	if (gl_flashblend->intVal)
		return;

	for (lt=ri.scn.dLightList, i=0 ; i<ri.scn.numDLights ; i++, lt++)
		R_Q2BSP_r_MarkWorldLights (ri.scn.worldModel->bspModel.nodes, lt, 1<<i);
}


/*
=============
R_Q2BSP_MarkBModelLights
=============
*/
static void R_Q2BSP_r_MarkBModelLights (mBspNode_t *node, refDLight_t *lt, uint32 bit)
{
	mBspSurface_t	**mark, *surf;
	float			dist;

loc0:
	if (node->c.q2_contents != -1)
		return;

	dist = PlaneDiff (lt->origin, node->c.plane);
	if (dist > lt->intensity) {
		node = node->children[0];
		goto loc0;
	}
	if (dist < -lt->intensity) {
		node = node->children[1];
		goto loc0;
	}
	if (!BoundsIntersect (node->c.mins, node->c.maxs, lt->mins, lt->maxs))
		return;

	// Mark the polygons
	if (node->q2_firstLitSurface) {
		mark = node->q2_firstLitSurface;
		do {
			surf = *mark++;
			if (!BoundsIntersect (surf->mins, surf->maxs, lt->mins, lt->maxs))
				continue;

			if (surf->dLightFrame != ri.frameCount) {
				surf->dLightFrame = ri.frameCount;
				surf->dLightBits = 0;
			}
			surf->dLightBits |= bit;
		} while (*mark);
	}

	R_Q2BSP_r_MarkBModelLights (node->children[0], lt, bit);
	R_Q2BSP_r_MarkBModelLights (node->children[1], lt, bit);
}
void R_Q2BSP_MarkBModelLights (refEntity_t *ent, vec3_t mins, vec3_t maxs)
{
	refDLight_t		*lt;
	mBspNode_t		*node;
	uint32			i;

	if (!ri.scn.numDLights || gl_flashblend->intVal || !gl_dynamic->intVal || r_fullbright->intVal)
		return;

	node = ent->model->bspModel.nodes + ent->model->q2BspModel.firstNode;
	for (i=0, lt=ri.scn.dLightList ; i<ri.scn.numDLights ; lt++, i++) {
		if (!BoundsIntersect (mins, maxs, lt->mins, lt->maxs))
			continue;

		R_Q2BSP_r_MarkBModelLights (node, lt, 1<<i);
	}
}

/*
=============================================================================

	QUAKE II LIGHT SAMPLING

=============================================================================
*/

/*
===============
R_Q2BSP_RecursiveLightPoint
===============
*/
static int Q2BSP_RecursiveLightPoint (mBspNode_t *node, vec3_t start, vec3_t end)
{
	float			front, back, frac;
	int				i, s, t, ds, dt, r;
	int				side, map;
	cBspPlane_t		*plane;
	vec3_t			mid;
	mBspSurface_t	**mark, *surf;
	byte			*lightmap;

	// Didn't hit anything
	if (node->c.q2_contents != -1)
		return -1;
	
	// Calculate mid point
	plane = node->c.plane;
	if (plane->type < 3) {
		front = start[plane->type] - plane->dist;
		back = end[plane->type] - plane->dist;
	}
	else {
		front = DotProduct (start, plane->normal) - plane->dist;
		back = DotProduct (end, plane->normal) - plane->dist;
	}

	side = front < 0;
	if ((back < 0) == side)
		return Q2BSP_RecursiveLightPoint (node->children[side], start, end);
	
	frac = front / (front - back);
	mid[0] = start[0] + (end[0] - start[0]) * frac;
	mid[1] = start[1] + (end[1] - start[1]) * frac;
	mid[2] = start[2] + (end[2] - start[2]) * frac;
	
	// Go down front side
	r = Q2BSP_RecursiveLightPoint (node->children[side], start, mid);
	if (r >= 0)
		return r;	// Hit something
		
	if ((back < 0) == side)
		return -1;	// Didn't hit anything
		
	// Check for impact on this node
	Vec3Copy (mid, r_q2_lightSpot);

	if (node->q2_firstLitSurface) {
		mark = node->q2_firstLitSurface;
		do {
			surf = *mark++;

			s = (int) (DotProduct (mid, surf->q2_texInfo->vecs[0]) + surf->q2_texInfo->vecs[0][3]);
			t = (int) (DotProduct (mid, surf->q2_texInfo->vecs[1]) + surf->q2_texInfo->vecs[1][3]);
			if (s < surf->q2_textureMins[0] || t < surf->q2_textureMins[1])
				continue;

			ds = s - surf->q2_textureMins[0];
			dt = t - surf->q2_textureMins[1];
			if (ds > surf->q2_extents[0] || dt > surf->q2_extents[1])
				continue;

			ds >>= 4;
			dt >>= 4;

			lightmap = surf->q2_lmSamples;
			Vec3Clear (r_q2_pointColor);
			if (lightmap) {
				vec3_t scale;

				lightmap += 3 * (dt*surf->q2_lmWidth + ds);

				for (map=0 ; map<surf->q2_numStyles ; map++) {
					Vec3Scale (ri.scn.lightStyles[surf->q2_styles[map]].rgb, gl_modulate->floatVal, scale);
					for (i=0 ; i<3 ; i++)
						r_q2_pointColor[i] += lightmap[i] * scale[i] * (1.0f/255.0f);

					lightmap += 3*surf->q2_lmWidth*surf->q2_lmWidth;
				}
			}
			
			return 1;
		} while (*mark);
	}

	// Go down back side
	return Q2BSP_RecursiveLightPoint (node->children[!side], mid, end);
}
static qBool R_Q2BSP_RecursiveLightPoint (vec3_t point, vec3_t end)
{
	int	r;

	if (ri.def.rdFlags & RDF_NOWORLDMODEL || !ri.scn.worldModel->q2BspModel.lightData) {
		Vec3Set (r_q2_pointColor, 1, 1, 1);
		return qFalse;
	}

	r = Q2BSP_RecursiveLightPoint (ri.scn.worldModel->bspModel.nodes, point, end);
	
	if (r == -1) {
		Vec3Clear (r_q2_pointColor);
		return qFalse;
	}

	return qTrue;
}


/*
===============
R_Q2BSP_ShadowForEntity
===============
*/
static qBool R_Q2BSP_ShadowForEntity (refEntity_t *ent, vec3_t shadowSpot)
{
	vec3_t		end;

	Vec3Set (end, ent->origin[0], ent->origin[1], ent->origin[2] - 2048);

	if (R_Q2BSP_RecursiveLightPoint (ent->origin, end)) {
		// Found!
		Vec3Copy (r_q2_lightSpot, shadowSpot);
		return qTrue;
	}

	// Not found!
	Vec3Clear (shadowSpot);
	return qFalse;

}


/*
===============
R_Q2BSP_LightForEntity
===============
*/
static void R_Q2BSP_LightForEntity (refEntity_t *ent, int numVerts, byte *bArray)
{
	static vec3_t	tempColorsArray[RB_MAX_VERTS];
	float			*cArray;
	vec3_t			end;
	vec3_t			ambientLight;
	vec3_t			directedLight;
	int				r, g, b, i;
	vec3_t			dir, direction;
	float			dot;

	if (!(ent->flags & RF_WEAPONMODEL) && (r_fullbright->intVal || ent->flags & RF_FULLBRIGHT))
		goto fullBright;

	//
	// Get the lighting from below
	//
	Vec3Set (end, ent->origin[0], ent->origin[1], ent->origin[2] - 2048);
	if (!R_Q2BSP_RecursiveLightPoint (ent->origin, end)) {
		end[2] = ent->origin[2] + 16;
		if (!(ent->flags & RF_WEAPONMODEL) && !R_Q2BSP_RecursiveLightPoint (ent->origin, end)) {
			// Not found!
			Vec3Copy (r_q2_pointColor, ambientLight);
			Vec3Copy (r_q2_pointColor, directedLight);
		}
		else {
			// Found!
			Vec3Copy (r_q2_pointColor, directedLight);
			Vec3Scale (r_q2_pointColor, 0.6f, ambientLight);
		}
	}
	else {
		// Found!
		Vec3Copy (r_q2_pointColor, directedLight);
		Vec3Scale (r_q2_pointColor, 0.6f, ambientLight);
	}

	// Save off light value for server to look at (BIG HACK!)
	if (ent->flags & RF_WEAPONMODEL) {
		// Pick the greatest component, which should be
		// the same as the mono value returned by software
		if (r_q2_pointColor[0] > r_q2_pointColor[1]) {
			if (r_q2_pointColor[0] > r_q2_pointColor[2])
				Cvar_VariableSetValue (r_lightlevel, 150 * r_q2_pointColor[0], qTrue);
			else
				Cvar_VariableSetValue (r_lightlevel, 150 * r_q2_pointColor[2], qTrue);
		}
		else {
			if (r_q2_pointColor[1] > r_q2_pointColor[2])
				Cvar_VariableSetValue (r_lightlevel, 150 * r_q2_pointColor[1], qTrue);
			else
				Cvar_VariableSetValue (r_lightlevel, 150 * r_q2_pointColor[2], qTrue);
		}

	}

	// Fullbright entity
	if (r_fullbright->intVal || ent->flags & RF_FULLBRIGHT) {
fullBright:
		for (i=0 ; i<numVerts ; i++, bArray+=4)
			*(int *)bArray = *(int *)ent->color;
		return;
	}

	//
	// Flag effects
	//
	if (ent->flags & RF_MINLIGHT) {
		for (i=0 ; i<3 ; i++)
			if (ambientLight[i] > 0.1f)
				break;

		if (i == 3) {
			ambientLight[0] += 0.1f;
			ambientLight[1] += 0.1f;
			ambientLight[2] += 0.1f;
		}
	}

	if (ent->flags & RF_GLOW) {
		float	scale;
		float	min;

		// Bonus items will pulse with time
		scale = 0.1f * (float)sin (ri.def.time * 7);
		for (i=0 ; i<3 ; i++) {
			min = ambientLight[i] * 0.8f;
			ambientLight[i] += scale;
			if (ambientLight[i] < min)
				ambientLight[i] = min;
		}
	}

	//
	// Add ambient lights
	//
	Vec3Set (dir, -1, 0, 1);
	Matrix3_TransformVector (ent->axis, dir, direction);

	for (i=0 ; i<numVerts; i++) {
		dot = DotProduct (rb.batch.normals[i], direction);
		if (dot <= 0)
			Vec3Copy (ambientLight, tempColorsArray[i]);
		else
			Vec3MA (ambientLight, dot, directedLight, tempColorsArray[i]);
	}

	//
	// Add dynamic lights
	//
	if (gl_dynamic->intVal && ri.scn.numDLights) {
		refDLight_t	*lt;
		float		dist, add, intensity8, intensity;
		vec3_t		dlOrigin;
		uint32		num;

		for (lt=ri.scn.dLightList, num=0 ; num<ri.scn.numDLights ; num++, lt++) {
			// FIXME: use BoundsIntersect for a performance boost, though this will meen storing bounds in the entity or something...
			if (!BoundsAndSphereIntersect (lt->mins, lt->maxs, ent->origin, ent->model->radius * ent->scale))
				continue;

			// Translate
			Vec3Subtract (lt->origin, ent->origin, dir);
			dist = Vec3Length (dir);

			if (!dist || dist > lt->intensity + ent->model->radius * ent->scale)
				continue;

			// Rotate
			Matrix3_TransformVector (ent->axis, dir, dlOrigin);

			// Calculate intensity
			intensity = lt->intensity - dist;
			if (intensity <= 0)
				continue;
			intensity8 = lt->intensity * 8;

			for (i=0 ; i<numVerts ; i++) {
				Vec3Subtract (dlOrigin, rb.batch.vertices[i], dir);
				add = DotProduct (rb.batch.normals[i], dir);

				// Add some ambience
				Vec3MA (tempColorsArray[i], intensity * 0.4f * (1.0f/256.0f), lt->color, tempColorsArray[i]);

				// Shade the verts
				if (add > 0) {
					dot = DotProduct (dir, dir);
					add *= (intensity8 / dot) * Q_RSqrtf (dot);
					if (add > 255.0f)
						add = 255.0f / add;

					Vec3MA (tempColorsArray[i], add, lt->color, tempColorsArray[i]);
				}
			}
		}
	}

	//
	// Clamp
	//
	cArray = tempColorsArray[0];
	for (i=0 ; i<numVerts ; i++, bArray+=4, cArray+=3) {
		r = (int) (cArray[0] * ent->color[0]);
		g = (int) (cArray[1] * ent->color[1]);
		b = (int) (cArray[2] * ent->color[2]);

		bArray[0] = clamp (r, 0, 255);
		bArray[1] = clamp (g, 0, 255);
		bArray[2] = clamp (b, 0, 255);
	}
}


/*
===============
R_Q2BSP_LightPoint
===============
*/
static void R_Q2BSP_LightPoint (vec3_t point, vec3_t light)
{
	vec3_t		end;
	vec3_t		dist;
	refDLight_t	*lt;
	float		add;
	uint32		num;

	Vec3Set (end, point[0], point[1], point[2] - 2048);
	if (!R_Q2BSP_RecursiveLightPoint (point, end)) {
		end[2] = point[2] + 16;
		R_Q2BSP_RecursiveLightPoint (point, end);
	}
	Vec3Copy (r_q2_pointColor, light);

	//
	// Add dynamic lights
	//
	for (lt=ri.scn.dLightList, num=0 ; num<ri.scn.numDLights ; num++, lt++) {
		Vec3Subtract (point, lt->origin, dist);
		add = (lt->intensity - Vec3Length (dist)) * (1.0f/256.0f);

		if (add > 0)
			Vec3MA (light, add, lt->color, light);
	}
}


/*
====================
R_Q2BSP_SetLightLevel

Save off light value for server to look at (BIG HACK!)
====================
*/
static void R_Q2BSP_SetLightLevel (void)
{
	vec3_t		shadelight;

	R_Q2BSP_LightPoint (ri.def.viewOrigin, shadelight);

	// Pick the greatest component, which should be
	// the same as the mono value returned by software
	if (shadelight[0] > shadelight[1]) {
		if (shadelight[0] > shadelight[2])
			Cvar_VariableSetValue (r_lightlevel, 150 * shadelight[0], qTrue);
		else
			Cvar_VariableSetValue (r_lightlevel, 150 * shadelight[2], qTrue);
	}
	else {
		if (shadelight[1] > shadelight[2])
			Cvar_VariableSetValue (r_lightlevel, 150 * shadelight[1], qTrue);
		else
			Cvar_VariableSetValue (r_lightlevel, 150 * shadelight[2], qTrue);
	}

}

/*
=============================================================================

	QUAKE II LIGHTMAP

=============================================================================
*/

static byte		*r_q2_lmBuffer;
static int		r_q2_lmNumUploaded;
static int		*r_q2_lmAllocated;
int				r_q2_lmSize;

/*
===============
R_Q2BSP_AddDynamicLights
===============
*/
static void R_Q2BSP_AddDynamicLights (mBspSurface_t *surf)
{
	int			sd, td, s, t;
	float		fDist, fDist2, fRad;
	float		scale, sl, st;
	float		fsacc, ftacc;
	float		*bl;
	vec3_t		impact;
	refDLight_t	*lt;
	uint32		num;

	for (num=0, lt=ri.scn.dLightList ; num<ri.scn.numDLights ; num++, lt++) {
		if (!(surf->dLightBits & (1<<num)))
			continue;	// Not lit by this light

		fDist = PlaneDiff (lt->origin, surf->q2_plane);
		fRad = lt->intensity - (float)fabs (fDist); // fRad is now the highest intensity on the plane
		if (fRad < 0)
			continue;

		impact[0] = lt->origin[0] - (surf->q2_plane->normal[0] * fDist);
		impact[1] = lt->origin[1] - (surf->q2_plane->normal[1] * fDist);
		impact[2] = lt->origin[2] - (surf->q2_plane->normal[2] * fDist);

		sl = DotProduct (impact, surf->q2_texInfo->vecs[0]) + surf->q2_texInfo->vecs[0][3] - surf->q2_textureMins[0];
		st = DotProduct (impact, surf->q2_texInfo->vecs[1]) + surf->q2_texInfo->vecs[1][3] - surf->q2_textureMins[1];

		bl = r_q2_blockLights;
		for (t=0, ftacc=0 ; t<surf->q2_lmHeight ; t++) {
			td = (int) (st - ftacc);
			if (td < 0)
				td = -td;

			for (s=0, fsacc=0 ; s<surf->q2_lmWidth ; s++) {
				sd = (int) (sl - fsacc);
				if (sd < 0)
					sd = -sd;

				if (sd > td) {
					fDist = (float)(sd + (td>>1));
					fDist2 = (float)(sd + (td<<1));
				}
				else {
					fDist = (float)(td + (sd>>1));
					fDist2 = (float)(td + (sd<<1));
				}

				if (fDist < fRad) {
					scale = fRad - fDist;

					bl[0] += lt->color[0] * scale;
					bl[1] += lt->color[1] * scale;
					bl[2] += lt->color[2] * scale;

					// Amplify the center a little
					if (fDist2 < fRad) {
						scale = fRad - fDist2;
						bl[0] += lt->color[0] * scale * 0.5f;
						bl[1] += lt->color[1] * scale * 0.5f;
						bl[2] += lt->color[2] * scale * 0.5f;
					}
				}

				fsacc += 16;
				bl += 3;
			}

			ftacc += 16;
		}
	}
}


/*
===============
R_Q2BSP_BuildLightMap

Combine and scale multiple lightmaps into the floating format in blocklights
===============
*/
static void R_Q2BSP_BuildLightMap (mBspSurface_t *surf, byte *dest, int stride)
{
	int			i, j, size;
	int			map;
	float		*bl, max;
	vec3_t		scale;
	byte		*lightMap;

	if (surf->q2_texInfo->flags & (SURF_TEXINFO_SKY|SURF_TEXINFO_WARP))
		Com_Error (ERR_DROP, "LM_BuildLightMap called for non-lit surface");

	size = surf->q2_lmWidth*surf->q2_lmHeight;
	if (size > sizeof (r_q2_blockLights) >> 4)
		Com_Error (ERR_DROP, "Bad r_q2_blockLights size");

	// Set to full bright if no light data
	if (!surf->q2_lmSamples || r_fullbright->intVal) {
		for (i=0 ; i<size*3 ; i++)
			r_q2_blockLights[i] = 255.0f;
	}
	else {
		lightMap = surf->q2_lmSamples;

		// Add all the lightmaps
		if (surf->q2_numStyles == 1) {
			bl = r_q2_blockLights;

			// Optimal case
			Vec3Scale (ri.scn.lightStyles[surf->q2_styles[0]].rgb, gl_modulate->floatVal, scale);
			if (scale[0] == 1.0f && scale[1] == 1.0f && scale[2] == 1.0f) {
				for (i=0 ; i<size ; i++) {
					bl[0] = lightMap[i*3+0];
					bl[1] = lightMap[i*3+1];
					bl[2] = lightMap[i*3+2];

					bl += 3;
				}
			}
			else {
				for (i=0 ; i<size ; i++) {
					bl[0] = lightMap[i*3+0] * scale[0];
					bl[1] = lightMap[i*3+1] * scale[1];
					bl[2] = lightMap[i*3+2] * scale[2];

					bl += 3;
				}
			}

			// Skip to next lightmap
			lightMap += size*3;
		}
		else {
			map = 0;
			bl = r_q2_blockLights;
			Vec3Scale (ri.scn.lightStyles[surf->q2_styles[map]].rgb, gl_modulate->floatVal, scale);
			if (scale[0] == 1.0f && scale[1] == 1.0f && scale[2] == 1.0f) {
				for (i=0 ; i<size ; i++, bl+=3) {
					bl[0] = lightMap[i*3+0];
					bl[1] = lightMap[i*3+1];
					bl[2] = lightMap[i*3+2];
				}
			}
			else {
				for (i=0 ; i<size ; i++, bl+=3) {
					bl[0] = lightMap[i*3+0] * scale[0];
					bl[1] = lightMap[i*3+1] * scale[1];
					bl[2] = lightMap[i*3+2] * scale[2];
				}
			}

			// Skip to next lightmap
			lightMap += size*3;

			for (map=1 ; map<surf->q2_numStyles ; map++) {
				bl = r_q2_blockLights;

				Vec3Scale (ri.scn.lightStyles[surf->q2_styles[map]].rgb, gl_modulate->floatVal, scale);
				if (scale[0] == 1.0f && scale[1] == 1.0f && scale[2] == 1.0f) {
					for (i=0 ; i<size ; i++, bl+=3) {
						bl[0] += lightMap[i*3+0];
						bl[1] += lightMap[i*3+1];
						bl[2] += lightMap[i*3+2];
					}
				}
				else {
					for (i=0 ; i<size ; i++, bl+=3) {
						bl[0] += lightMap[i*3+0] * scale[0];
						bl[1] += lightMap[i*3+1] * scale[1];
						bl[2] += lightMap[i*3+2] * scale[2];
					}
				}

				// Skip to next lightmap
				lightMap += size*3;
			}
		}

		// Add all the dynamic lights
		if (surf->dLightFrame == ri.frameCount)
			R_Q2BSP_AddDynamicLights (surf);
	}

	// Put into texture format
	stride -= (surf->q2_lmWidth << 2);
	bl = r_q2_blockLights;

	for (i=0 ; i<surf->q2_lmHeight ; i++) {
		for (j=0 ; j<surf->q2_lmWidth ; j++) {
			// Catch negative lights
			if (bl[0] < 0)
				bl[0] = 0;
			if (bl[1] < 0)
				bl[1] = 0;
			if (bl[2] < 0)
				bl[2] = 0;

			// Determine the brightest of the three color components
			max = bl[0];
			if (bl[1] > max)
				max = bl[1];
			if (bl[2] > max)
				max = bl[2];

			// Normalize the color components to the highest channel
			if (max > 255) {
				max = 255.0f / max;

				dest[0] = (byte)(bl[0]*max);
				dest[1] = (byte)(bl[1]*max);
				dest[2] = (byte)(bl[2]*max);
				dest[3] = (byte)(255*max);
			}
			else {
				dest[0] = (byte)bl[0];
				dest[1] = (byte)bl[1];
				dest[2] = (byte)bl[2];
				dest[3] = 255;
			}

			bl += 3;
			dest += 4;
		}

		dest += stride;
	}
}


/*
===============
R_Q2BSP_SetLMCacheState
===============
*/
static void R_Q2BSP_SetLMCacheState (mBspSurface_t *surf)
{
	int		map;

	for (map=0 ; map<surf->q2_numStyles ; map++)
		surf->q2_cachedLight[map] = ri.scn.lightStyles[surf->q2_styles[map]].white;
}


/*
=======================
R_Q2BSP_UpdateLightmap
=======================
*/
void R_Q2BSP_UpdateLightmap (mBspSurface_t *surf)
{
	static uint32	temp[Q2LIGHTMAP_WIDTH*Q2LIGHTMAP_WIDTH];
	int				map;

	// Don't attempt a surface more than once a frame
	// FIXME: This is just a nasty work-around at best
	if (surf->q2_lmFrame == ri.frameCount)
		return;
	surf->q2_lmFrame = ri.frameCount;

	// Is this surface allowed to have a lightmap?
	if (surf->q2_texInfo->flags & (SURF_TEXINFO_SKY|SURF_TEXINFO_WARP)) {
		surf->q2_lmTexNumActive = -1;
		return;
	}

	// Dynamic this frame or dynamic previously
	if (gl_dynamic->intVal) {
		for (map=0 ; map<surf->q2_numStyles ; map++) {
			if (ri.scn.lightStyles[surf->q2_styles[map]].white != surf->q2_cachedLight[map])
				goto dynamic;
		}

		if (surf->dLightFrame == ri.frameCount)
			goto dynamic;
	}

	// No need to update
	surf->q2_lmTexNumActive = surf->lmTexNum;
	return;

dynamic:
	// Update texture
	R_Q2BSP_BuildLightMap (surf, (void *)temp, surf->q2_lmWidth*4);
	if ((surf->q2_styles[map] >= 32 || surf->q2_styles[map] == 0) && surf->dLightFrame != ri.frameCount) {
		R_Q2BSP_SetLMCacheState (surf);

		RB_BindTexture (r_lmTextures[surf->lmTexNum]);
		surf->q2_lmTexNumActive = surf->lmTexNum;
	}
	else {
		RB_BindTexture (r_lmTextures[0]);
		surf->q2_lmTexNumActive = 0;
	}

	qglTexSubImage2D (GL_TEXTURE_2D, 0,
					surf->q2_lmCoords[0], surf->q2_lmCoords[1],
					surf->q2_lmWidth, surf->q2_lmHeight,
					GL_RGBA,
					GL_UNSIGNED_BYTE,
					temp);
}


/*
================
R_Q2BSP_UploadLMBlock
================
*/
static void R_Q2BSP_UploadLMBlock (void)
{
	if (r_q2_lmNumUploaded+1 >= R_MAX_LIGHTMAPS)
		Com_Error (ERR_DROP, "R_Q2BSP_UploadLMBlock: - R_MAX_LIGHTMAPS exceeded\n");

	r_lmTextures[r_q2_lmNumUploaded++] = R_Load2DImage (Q_VarArgs ("*lm%i", r_q2_lmNumUploaded), (byte **)(&r_q2_lmBuffer),
		r_q2_lmSize, r_q2_lmSize, IF_NOPICMIP|IF_NOMIPMAP_LINEAR|IF_NOGAMMA|IF_NOINTENS|IF_NOCOMPRESS|IT_LIGHTMAP, 3);
}


/*
================
R_Q2BSP_AllocLMBlock

Returns a texture number and the position inside it
================
*/
static qBool R_Q2BSP_AllocLMBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;

	best = r_q2_lmSize;
	for (i=0 ; i<r_q2_lmSize-w ; i++) {
		best2 = 0;

		for (j=0 ; j<w ; j++) {
			if (r_q2_lmAllocated[i+j] >= best)
				break;

			if (r_q2_lmAllocated[i+j] > best2)
				best2 = r_q2_lmAllocated[i+j];
		}

		if (j == w) {
			// This is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if (best + h > r_q2_lmSize)
		return qFalse;

	for (i=0 ; i<w ; i++)
		r_q2_lmAllocated[*x + i] = best + h;

	return qTrue;
}


/*
==================
R_Q2BSP_BeginBuildingLightmaps
==================
*/
void R_Q2BSP_BeginBuildingLightmaps (void)
{
	int		size, i;

	// Should be no lightmaps at this point
	r_q2_lmNumUploaded = 0;

	// Find the maximum size
	for (size=1 ; size<Q2LIGHTMAP_WIDTH && size<ri.config.maxTexSize ; size<<=1);
	r_q2_lmSize = size;

	// Allocate buffers and clear values
	r_q2_lmAllocated = Mem_PoolAllocExt (sizeof (int) * r_q2_lmSize, qTrue, ri.lightSysPool, 0);
	r_q2_lmBuffer = Mem_PoolAllocExt (r_q2_lmSize*r_q2_lmSize*4, qFalse, ri.lightSysPool, 0);
	memset (r_q2_lmBuffer, 255, r_q2_lmSize*r_q2_lmSize*4);

	// Setup the base light styles
	for (i=0 ; i<MAX_CS_LIGHTSTYLES ; i++) {
		Vec3Set (ri.scn.lightStyles[i].rgb, 1, 1, 1);
		ri.scn.lightStyles[i].white = 3;
	}

	// Initialize the base dynamic lightmap texture
	R_Q2BSP_UploadLMBlock ();
}


/*
========================
R_Q2BSP_CreateSurfaceLightmap
========================
*/
void R_Q2BSP_CreateSurfaceLightmap (mBspSurface_t *surf)
{
	byte	*base;

	if (!R_Q2BSP_AllocLMBlock (surf->q2_lmWidth, surf->q2_lmHeight, &surf->q2_lmCoords[0], &surf->q2_lmCoords[1])) {
		R_Q2BSP_UploadLMBlock ();
		memset (r_q2_lmAllocated, 0, sizeof (int) * r_q2_lmSize);

		if (!R_Q2BSP_AllocLMBlock (surf->q2_lmWidth, surf->q2_lmHeight, &surf->q2_lmCoords[0], &surf->q2_lmCoords[1]))
			Com_Error (ERR_FATAL, "Consecutive calls to R_Q2BSP_AllocLMBlock (%d, %d) failed\n", surf->q2_lmWidth, surf->q2_lmHeight);
	}

	surf->lmTexNum = r_q2_lmNumUploaded;
	surf->q2_lmTexNumActive = -1;
	surf->q2_lmFrame = ri.frameCount - 1;	// Force an update

	base = r_q2_lmBuffer + ((surf->q2_lmCoords[1] * r_q2_lmSize + surf->q2_lmCoords[0]) * 4);

	R_Q2BSP_SetLMCacheState (surf);
	R_Q2BSP_BuildLightMap (surf, base, r_q2_lmSize*4);
}


/*
=======================
R_Q2BSP_EndBuildingLightmaps
=======================
*/
void R_Q2BSP_EndBuildingLightmaps (void)
{
	// Upload the final block
	R_Q2BSP_UploadLMBlock ();

	// Release allocated memory
	Mem_Free (r_q2_lmAllocated);
	Mem_Free (r_q2_lmBuffer);
}


/*
=======================
R_Q2BSP_TouchLightmaps
=======================
*/
static void R_Q2BSP_TouchLightmaps (void)
{
	int		i;

	for (i=0 ; i<r_q2_lmNumUploaded ; i++)
		R_TouchImage (r_lmTextures[i]);
}

/*
=============================================================================

	QUAKE III DYNAMIC LIGHTS

=============================================================================
*/

#define Q3_DLIGHT_SCALE 0.5f

/*
=================
R_Q3BSP_MarkLitSurfaces
=================
*/
static void R_Q3BSP_MarkLitSurfaces (refDLight_t *lt, float lightIntensity, uint32 bit, mBspNode_t *node)
{
	mBspLeaf_t		*leaf;
	mBspSurface_t	*surf, **mark;
	float			dist;

	for ( ; ; ) {
		if (node->c.visFrame != ri.scn.visFrameCount)
			return;
		if (node->c.plane == NULL)
			break;

		dist = PlaneDiff (lt->origin, node->c.plane);
		if (dist > lightIntensity) {
			node = node->children[0];
			continue;
		}

		if (dist >= -lightIntensity)
			R_Q3BSP_MarkLitSurfaces (lt, lightIntensity, bit, node->children[0]);
		node = node->children[1];
	}

	leaf = (mBspLeaf_t *)node;

	// Check for door connected areas
	if (ri.def.areaBits) {
		if (!(ri.def.areaBits[leaf->area>>3] & (1<<(leaf->area&7))))
			return;		// Not visible
	}
	if (!leaf->q3_firstLitSurface)
		return;
	if (!BoundsIntersect (leaf->c.mins, leaf->c.maxs, lt->mins, lt->maxs))
		return;

	mark = leaf->q3_firstLitSurface;
	do {
		surf = *mark++;
		if (!BoundsIntersect (surf->mins, surf->maxs, lt->mins, lt->maxs))
			continue;

		if (surf->dLightFrame != ri.frameCount) {
			surf->dLightBits = 0;
			surf->dLightFrame = ri.frameCount;
		}
		surf->dLightBits |= bit;
	} while (*mark);
}


/*
=================
R_Q3BSP_MarkWorldLights
=================
*/
void R_Q3BSP_MarkWorldLights (void)
{
	refDLight_t	*lt;
	uint32		i;

	if (gl_flashblend->intVal || !gl_dynamic->intVal || !ri.scn.numDLights || r_vertexLighting->intVal || r_fullbright->intVal)
		return;

	lt = ri.scn.dLightList;
	for (i=0 ; i<ri.scn.numDLights ; i++, lt++)
		R_Q3BSP_MarkLitSurfaces (lt, lt->intensity*Q3_DLIGHT_SCALE, 1<<i, ri.scn.worldModel->bspModel.nodes);
}


/*
=================
R_Q3BSP_MarkBModelLights
=================
*/
void R_Q3BSP_MarkBModelLights (refEntity_t *ent, vec3_t mins, vec3_t maxs)
{
	refDLight_t		*lt;
	refModel_t		*model = ent->model;
	mBspSurface_t	*surf;
	uint32			i;
	int				j;

	if (!gl_dynamic->intVal || !ri.scn.numDLights || r_fullbright->intVal)
		return;

	for (i=0, lt=ri.scn.dLightList ; i<ri.scn.numDLights ; i++, lt++) {
		if (!BoundsIntersect (mins, maxs, lt->mins, lt->maxs))
			continue;

		for (j=0, surf=model->bspModel.firstModelSurface ; j<model->bspModel.numModelSurfaces ; j++, surf++) {
			if (R_Q3BSP_SurfPotentiallyLit (surf)) {
				if (surf->dLightFrame != ri.frameCount) {
					surf->dLightBits = 0;
					surf->dLightFrame = ri.frameCount;
				}
				surf->dLightBits |= 1<<i;
			}
		}
	}
}

/*
=============================================================================

	QUAKE III LIGHT SAMPLING

=============================================================================
*/

/*
===============
R_Q3BSP_LightForEntity
===============
*/
static void R_Q3BSP_LightForEntity (refEntity_t *ent, int numVerts, byte *bArray)
{
	static vec3_t	tempColorsArray[RB_MAX_VERTS];
	vec3_t			vf, vf2;
	float			*cArray;
	float			t[8], direction_uv[2], dot;
	int				r, g, b, vi[3], i, j, index[4];
	vec3_t			dlorigin, ambient, diffuse, dir, direction;
	float			*gridSize, *gridMins;
	int				*gridBounds;

	// Fullbright entity
	if (r_fullbright->intVal || ent->flags & RF_FULLBRIGHT) {
		for (i=0 ; i<numVerts ; i++, bArray+=4)
			*(int *)bArray = *(int *)ent->color;
		return;
	}

	// Probably a weird shader, see mpteam4 for example
	if (!ent->model || ent->model->type == MODEL_Q3BSP) {
		memset (bArray, 0, sizeof (bvec4_t)*numVerts);
		return;
	}

	Vec3Set (ambient, 0, 0, 0);
	Vec3Set (diffuse, 0, 0, 0);
	Vec3Set (direction, 1, 1, 1);

	gridSize = ri.scn.worldModel->q3BspModel.gridSize;
	gridMins = ri.scn.worldModel->q3BspModel.gridMins;
	gridBounds = ri.scn.worldModel->q3BspModel.gridBounds;

	if (!ri.scn.worldModel->q3BspModel.lightGrid || !ri.scn.worldModel->q3BspModel.numLightGridElems)
		goto dynamic;

	for (i=0 ; i<3 ; i++) {
		vf[i] = (ent->origin[i] - ri.scn.worldModel->q3BspModel.gridMins[i]) / ri.scn.worldModel->q3BspModel.gridSize[i];
		vi[i] = (int)vf[i];
		vf[i] = vf[i] - floor(vf[i]);
		vf2[i] = 1.0f - vf[i];
	}

	index[0] = vi[2]*ri.scn.worldModel->q3BspModel.gridBounds[3] + vi[1]*ri.scn.worldModel->q3BspModel.gridBounds[0] + vi[0];
	index[1] = index[0] + ri.scn.worldModel->q3BspModel.gridBounds[0];
	index[2] = index[0] + ri.scn.worldModel->q3BspModel.gridBounds[3];
	index[3] = index[2] + ri.scn.worldModel->q3BspModel.gridBounds[0];
	for (i=0 ; i<4 ; i++) {
		if (index[i] < 0 || index[i] >= ri.scn.worldModel->q3BspModel.numLightGridElems-1)
			goto dynamic;
	}

	t[0] = vf2[0] * vf2[1] * vf2[2];
	t[1] = vf[0] * vf2[1] * vf2[2];
	t[2] = vf2[0] * vf[1] * vf2[2];
	t[3] = vf[0] * vf[1] * vf2[2];
	t[4] = vf2[0] * vf2[1] * vf[2];
	t[5] = vf[0] * vf2[1] * vf[2];
	t[6] = vf2[0] * vf[1] * vf[2];
	t[7] = vf[0] * vf[1] * vf[2];

	for (j=0 ; j<3 ; j++) {
		ambient[j] = 0;
		diffuse[j] = 0;

		for (i=0 ; i<4 ; i++) {
			ambient[j] += t[i*2] * ri.scn.worldModel->q3BspModel.lightGrid[index[i]].ambient[j];
			ambient[j] += t[i*2+1] * ri.scn.worldModel->q3BspModel.lightGrid[index[i]+1].ambient[j];

			diffuse[j] += t[i*2] * ri.scn.worldModel->q3BspModel.lightGrid[index[i]].diffuse[j];
			diffuse[j] += t[i*2+1] * ri.scn.worldModel->q3BspModel.lightGrid[index[i]+1].diffuse[j];
		}
	}

	for (j=0 ; j<2 ; j++) {
		direction_uv[j] = 0;

		for (i=0 ; i<4 ; i++) {
			direction_uv[j] += t[i*2] * ri.scn.worldModel->q3BspModel.lightGrid[index[i]].direction[j];
			direction_uv[j] += t[i*2+1] * ri.scn.worldModel->q3BspModel.lightGrid[index[i]+1].direction[j];
		}

		direction_uv[j] = AngleModf (direction_uv[j]);
	}

	dot = bound(0.0f, /*r_ambientscale->floatVal*/ 1.0f, 1.0f) * ri.pow2MapOvrbr;
	Vec3Scale (ambient, dot, ambient);

	dot = bound(0.0f, /*r_directedscale->floatVal*/ 1.0f, 1.0f) * ri.pow2MapOvrbr;
	Vec3Scale (diffuse, dot, diffuse);

	if (ent->flags & RF_MINLIGHT) {
		for (i=0 ; i<3 ; i++)
			if (ambient[i] > 0.1)
				break;

		if (i == 3) {
			ambient[0] = 0.1f;
			ambient[1] = 0.1f;
			ambient[2] = 0.1f;
		}
	}

	dot = direction_uv[0] * (1.0 / 255.0);
	t[0] = RB_FastSin (dot + 0.25f);
	t[1] = RB_FastSin (dot);

	dot = direction_uv[1] * (1.0 / 255.0);
	t[2] = RB_FastSin (dot + 0.25f);
	t[3] = RB_FastSin (dot);

	Vec3Set (dir, t[2] * t[1], t[3] * t[1], t[0]);

	// Rotate direction
	Matrix3_TransformVector (ent->axis, dir, direction);

	cArray = tempColorsArray[0];
	for (i=0 ; i<numVerts ; i++, cArray+=3) {
		dot = DotProduct (rb.batch.normals[i], direction);
		
		if (dot <= 0)
			Vec3Copy (ambient, cArray);
		else
			Vec3MA (ambient, dot, diffuse, cArray);
	}

dynamic:
	//
	// Add dynamic lights
	//
	if (gl_dynamic->intVal && ri.scn.numDLights) {
		refDLight_t	*dl;
		float		dist, add, intensity8;
		uint32		num;

		for (num=0, dl=ri.scn.dLightList ; num<ri.scn.numDLights ; dl++, num++) {
			// FIXME: use BoundsIntersect for a performance boost, though this will meen storing bounds in the entity or something...
			if (!BoundsAndSphereIntersect (dl->mins, dl->maxs, ent->origin, ent->model->radius * ent->scale))
				continue;

			// Translate
			Vec3Subtract ( dl->origin, ent->origin, dir );
			dist = Vec3Length ( dir );

			if (dist > dl->intensity + ent->model->radius * ent->scale)
				continue;

			// Rotate
			Matrix3_TransformVector ( ent->axis, dir, dlorigin );

			intensity8 = dl->intensity * 8;

			cArray = tempColorsArray[0];
			for (i=0 ; i<numVerts ; i++, cArray+=3) {
				Vec3Subtract (dlorigin, rb.batch.vertices[i], dir);
				add = DotProduct (rb.batch.normals[i], dir);

				if (add > 0) {
					dot = DotProduct (dir, dir);
					add *= (intensity8 / dot) * Q_RSqrtf (dot);
					Vec3MA (cArray, add, dl->color, cArray);
				}
			}
		}
	}

	cArray = tempColorsArray[0];
	for (i=0 ; i<numVerts ; i++, bArray+=4, cArray+=3) {
		r = (int) (cArray[0] * ent->color[0]);
		g = (int) (cArray[1] * ent->color[1]);
		b = (int) (cArray[2] * ent->color[2]);

		bArray[0] = clamp (r, 0, 255);
		bArray[1] = clamp (g, 0, 255);
		bArray[2] = clamp (b, 0, 255);
	}
}

/*
=============================================================================

	QUAKE III LIGHTMAP

=============================================================================
*/

static byte		*r_q3_lmBuffer;
static int		r_q3_lmBufferSize;
static int		r_q3_lmNumUploaded;
static int		r_q3_lmMaxBlockSize;

/*
=======================
R_Q3BSP_BuildLightmap
=======================
*/
static void R_Q3BSP_BuildLightmap (int w, int h, const byte *data, byte *dest, int blockWidth)
{
	int		x, y;
	float	scale;
	byte	*rgba;
	float	scaled[3];

	if (!data || r_fullbright->intVal) {
		for (y=0 ; y<h ; y++, dest)
			memset (dest + y * blockWidth, 255, w * 4);
		return;
	}

	scale = ri.pow2MapOvrbr;
	for (y=0 ; y<h ; y++) {
		for (x=0, rgba=dest+y*blockWidth ; x<w ; x++, rgba+=4) {
			scaled[0] = data[(y*w+x) * Q3LIGHTMAP_BYTES+0] * scale;
			scaled[1] = data[(y*w+x) * Q3LIGHTMAP_BYTES+1] * scale;
			scaled[2] = data[(y*w+x) * Q3LIGHTMAP_BYTES+2] * scale;

			ColorNormalizeb (scaled, rgba);
		}
	}
}


/*
=======================
R_Q3BSP_PackLightmaps
=======================
*/
static int R_Q3BSP_PackLightmaps (int num, int w, int h, int size, const byte *data, mQ3BspLightmapRect_t *rects)
{
	int i, x, y, root;
	byte *block;
	image_t *image;
	int	rectX, rectY, rectSize;
	int maxX, maxY, max, xStride;
	double tw, th, tx, ty;

	maxX = r_q3_lmMaxBlockSize / w;
	maxY = r_q3_lmMaxBlockSize / h;
	max = maxY;
	if (maxY > maxX)
		max = maxX;

	if (r_q3_lmNumUploaded >= R_MAX_LIGHTMAPS-1)
		Com_Error (ERR_DROP, "R_Q3BSP_PackLightmaps: - R_MAX_LIGHTMAPS exceeded\n");

	Com_DevPrintf (0, "Packing %i lightmap(s) -> ", num);

	if (!max || num == 1 || !r_lmPacking->intVal) {
		// Process as it is
		R_Q3BSP_BuildLightmap (w, h, data, r_q3_lmBuffer, w * 4);

		image = R_Load2DImage (Q_VarArgs ("*lm%i", r_q3_lmNumUploaded), (byte **)(&r_q3_lmBuffer),
			w, h, IF_CLAMP|IF_NOPICMIP|IF_NOMIPMAP_LINEAR|IF_NOGAMMA|IF_NOINTENS|IF_NOCOMPRESS|IT_LIGHTMAP, Q3LIGHTMAP_BYTES);

		r_lmTextures[r_q3_lmNumUploaded] = image;
		rects[0].texNum = r_q3_lmNumUploaded;

		rects[0].w = 1; rects[0].x = 0;
		rects[0].h = 1; rects[0].y = 0;

		Com_DevPrintf (0, "%ix%i\n", 1, 1);

		r_q3_lmNumUploaded++;
		return 1;
	}

	// Find the nearest square block size
	root = (int)sqrt (num);
	if (root > max)
		root = max;

	// Keep row size a power of two
	for (i=1 ; i<root ; i <<= 1);
	if (i > root)
		i >>= 1;
	root = i;

	num -= root * root;
	rectX = rectY = root;

	if (maxY > maxX) {
		for ( ; num>=root && rectY<maxY ; rectY++, num-=root);

		// Sample down if not a power of two
		for (y=1 ; y<rectY ; y <<= 1);
		if (y > rectY)
			y >>= 1;
		rectY = y;
	}
	else {
		for ( ; num>=root && rectX<maxX ; rectX++, num-=root);

		// Sample down if not a power of two
		for (x=1 ; x<rectX ; x<<=1);
		if (x > rectX)
			x >>= 1;
		rectX = x;
	}

	tw = 1.0 / (double)rectX;
	th = 1.0 / (double)rectY;

	xStride = w * 4;
	rectSize = (rectX * w) * (rectY * h) * 4;
	if (rectSize > r_q3_lmBufferSize) {
		if (r_q3_lmBuffer)
			Mem_Free (r_q3_lmBuffer);
		r_q3_lmBuffer = Mem_PoolAllocExt (rectSize, qFalse, ri.lightSysPool, 0);
		memset (r_q3_lmBuffer, 255, rectSize);
		r_q3_lmBufferSize = rectSize;
	}

	block = r_q3_lmBuffer;
	for (y=0, ty=0.0f, num=0 ; y<rectY ; y++, ty+=th, block+=rectX*xStride*h) {
		for (x=0, tx=0.0f ; x<rectX ; x++, tx+=tw, num++) {
			R_Q3BSP_BuildLightmap (w, h, data + num * size, block + x * xStride, rectX * xStride);

			rects[num].w = tw; rects[num].x = tx;
			rects[num].h = th; rects[num].y = ty;
		}
	}

	image = R_Load2DImage (Q_VarArgs ("*lm%i", r_q3_lmNumUploaded), (byte **)(&r_q3_lmBuffer),
		rectX * w, rectY * h, IF_CLAMP|IF_NOPICMIP|IF_NOMIPMAP_LINEAR|IF_NOGAMMA|IF_NOINTENS|IF_NOCOMPRESS|IT_LIGHTMAP, Q3LIGHTMAP_BYTES);

	r_lmTextures[r_q3_lmNumUploaded] = image;
	for (i=0 ; i<num ; i++)
		rects[i].texNum = r_q3_lmNumUploaded;

	Com_DevPrintf (0, "%ix%i\n", rectX, rectY);

	r_q3_lmNumUploaded++;
	return num;
}


/*
=======================
R_Q3BSP_BuildLightmaps
=======================
*/
void R_Q3BSP_BuildLightmaps (int numLightmaps, int w, int h, const byte *data, mQ3BspLightmapRect_t *rects)
{
	int		i;
	int		size;

	// Pack lightmaps
	for (size=1 ; size<r_lmMaxBlockSize->intVal && size<ri.config.maxTexSize ; size<<=1);

	r_q3_lmNumUploaded = 0;
	r_q3_lmMaxBlockSize = size;
	size = w * h * Q3LIGHTMAP_BYTES;
	r_q3_lmBufferSize = w * h * 4;
	r_q3_lmBuffer = Mem_PoolAllocExt (r_q3_lmBufferSize, qFalse, ri.lightSysPool, 0);

	for (i=0 ; i<numLightmaps ; )
		i += R_Q3BSP_PackLightmaps (numLightmaps - i, w, h, size, data + i * size, &rects[i]);

	if (r_q3_lmBuffer)
		Mem_Free (r_q3_lmBuffer);

	Com_DevPrintf (0, "Packed %i lightmaps into %i texture(s)\n", numLightmaps, r_q3_lmNumUploaded);
}


/*
=======================
R_Q3BSP_TouchLightmaps
=======================	
*/
static void R_Q3BSP_TouchLightmaps (void)
{
	int		i;

	for (i=0 ; i<r_q3_lmNumUploaded ; i++)
		R_TouchImage (r_lmTextures[i]);
}

/*
=============================================================================

	FUNCTION WRAPPING

=============================================================================
*/

/*
=============
R_LightBounds
=============
*/
void R_LightBounds (const vec3_t origin, float intensity, vec3_t mins, vec3_t maxs)
{
	if (ri.scn.worldModel->type == MODEL_Q2BSP) {
		Vec3Set (mins, origin[0] - (intensity * 2), origin[1] - (intensity * 2), origin[2] - (intensity * 2));
		Vec3Set (maxs, origin[0] + (intensity * 2), origin[1] + (intensity * 2), origin[2] + (intensity * 2));
		return;
	}

	Vec3Set (mins, origin[0] - intensity, origin[1] - intensity, origin[2] - intensity);
	Vec3Set (maxs, origin[0] + intensity, origin[1] + intensity, origin[2] + intensity);
}


/*
=============
R_LightPoint
=============
*/
void R_LightPoint (vec3_t point, vec3_t light)
{
	if (ri.def.rdFlags & RDF_NOWORLDMODEL)
		return;

	if (ri.scn.worldModel->type == MODEL_Q2BSP) {
		R_Q2BSP_LightPoint (point, light);
		return;
	}

	// FIXME
	Vec3Set (light, 1, 1, 1);
}


/*
=============
R_SetLightLevel
=============
*/
void R_SetLightLevel (void)
{
	if (ri.def.rdFlags & RDF_NOWORLDMODEL)
		return;

	if (ri.scn.worldModel->type == MODEL_Q2BSP) {
		R_Q2BSP_SetLightLevel ();
		return;
	}
}


/*
=======================
R_TouchLightmaps
=======================
*/
void R_TouchLightmaps (void)
{
	if (ri.def.rdFlags & RDF_NOWORLDMODEL)
		return;

	if (ri.scn.worldModel->type == MODEL_Q2BSP) {
		R_Q2BSP_TouchLightmaps ();
		return;
	}

	R_Q3BSP_TouchLightmaps ();
}


/*
=============
R_ShadowForEntity
=============
*/
qBool R_ShadowForEntity (refEntity_t *ent, vec3_t shadowSpot)
{
	if (ri.def.rdFlags & RDF_NOWORLDMODEL)
		return qFalse;

	if (ri.scn.worldModel->type == MODEL_Q2BSP)
		return R_Q2BSP_ShadowForEntity (ent, shadowSpot);

	return qFalse;
}


/*
=============
R_LightForEntity
=============
*/
void R_LightForEntity (refEntity_t *ent, int numVerts, byte *bArray)
{
	if (ri.def.rdFlags & RDF_NOWORLDMODEL || ri.scn.worldModel->type == MODEL_Q2BSP) {
		R_Q2BSP_LightForEntity (ent, numVerts, bArray);
		return;
	}

	R_Q3BSP_LightForEntity (ent, numVerts, bArray);
}
