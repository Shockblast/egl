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
// r_light.c
// Dynamic lights
// Lightmaps
// Alias model lighting
//

#include "r_local.h"

static float	r_blockLights[34*34*3];

vec3_t			r_pointColor;
cBspPlane_t		*r_lightPlane;		// Used for the shadow plane
vec3_t			r_lightSpot;

/*
=============================================================================

	DYNAMIC LIGHTS

=============================================================================
*/

/*
=============
R_AddDLightsToList
=============
*/
void R_AddDLightsToList (void)
{
	int			i, k;
	dLight_t	*light;
	float		a, rad;
	vec3_t		v;

	if (!gl_flashblend->integer)
		return;

	qglDepthMask (GL_FALSE);
	qglDisable (GL_TEXTURE_2D);
	qglEnable (GL_BLEND);
	GL_BlendFunc (GL_ONE, GL_ONE);

	for (light=r_dLightList, i=0 ; i<r_numDLights ; i++, light++) {
		rad = light->intensity * 0.7f;
		qglBegin (GL_TRIANGLE_FAN);

		if (gl_flashblend->integer == 2)
			qglColor3f (light->color[0] * 0.05f, light->color[1] * 0.05f, light->color[2] * 0.05f);
		else
			qglColor3f (light->color[0] * 0.2f, light->color[1] * 0.2f, light->color[2] * 0.2f);

		v[0] = light->origin[0] - (r_forwardVec[0] * rad);
		v[1] = light->origin[1] - (r_forwardVec[1] * rad);
		v[2] = light->origin[2] - (r_forwardVec[2] * rad);
		qglVertex3fv (v);

		qglColor3f (0, 0, 0);
		for (k=32 ; k>=0 ; k--) {
			a = (k / 32.0f) * (M_PI * 2.0f);

			v[0] = light->origin[0] + (r_rightVec[0] * (float)cos (a) * rad) + (r_upVec[0] * (float)sin (a) * rad);
			v[1] = light->origin[1] + (r_rightVec[1] * (float)cos (a) * rad) + (r_upVec[1] * (float)sin (a) * rad);
			v[2] = light->origin[2] + (r_rightVec[2] * (float)cos (a) * rad) + (r_upVec[2] * (float)sin (a) * rad);
			qglVertex3fv (v);
		}

		qglEnd ();
	}

	qglDisable (GL_BLEND);
	qglEnable (GL_TEXTURE_2D);
	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDepthMask (GL_TRUE);
}


/*
=============
R_MarkLights
=============
*/
void R_MarkLights (qBool visCull, dLight_t *light, int bit, mBspNode_t *node)
{
	cBspPlane_t		*plane;
	mBspSurface_t	*surf;
	float			dist;
	int				j;
	
	if (node->contents != -1)
		return;
	if (visCull && node->visFrame != r_visFrameCount)
		return;

	plane = node->plane;
	if (plane->type < 3)
		dist = light->origin[plane->type] - plane->dist;
	else
		dist = DotProduct(light->origin, plane->normal) - plane->dist;

	if (dist > light->intensity) {
		R_MarkLights (visCull, light, bit, node->children[0]);
		return;
	}
	if (dist < -light->intensity) {
		R_MarkLights (visCull, light, bit, node->children[1]);
		return;
	}

	// Mark the polygons
	surf = r_worldModel->surfaces + node->firstSurface;
	for (j=0 ; j<node->numSurfaces ; j++, surf++) {
		if (surf->dLightFrame != r_frameCount) {
			surf->dLightBits = 0;
			surf->dLightFrame = r_frameCount;
		}
		surf->dLightBits |= bit;
	}

	R_MarkLights (visCull, light, bit, node->children[0]);
	R_MarkLights (visCull, light, bit, node->children[1]);
}


/*
=============
R_PushDLights
=============
*/
void R_PushDLights (void)
{
	int			i;
	dLight_t	*dl;

	if (gl_flashblend->integer != 2 && gl_flashblend->integer)
		return;

	for (dl=r_dLightList, i=0 ; i<r_numDLights ; i++, dl++)
		R_MarkLights (qTrue, dl, 1 << i, r_worldModel->nodes);
}


/*
=============
R_LightBounds
=============
*/
void R_LightBounds (const vec3_t origin, float intensity, vec3_t mins, vec3_t maxs)
{
	VectorSet (mins, origin[0] - intensity, origin[1] - intensity, origin[2] - intensity);
	VectorSet (maxs, origin[0] + intensity, origin[1] + intensity, origin[2] + intensity);
}


/*
===============
R_AddDynamicLights
===============
*/
static void R_AddDynamicLights (mBspSurface_t *surf)
{
	int				lightNum, sd, td, s, t;
	float			fDist, fRad;
	float			scale, sl, st;
	float			fsacc, ftacc;
	float			*bl;
	vec3_t			impact;
	dLight_t		*dl;

	for (lightNum=0, dl=r_dLightList ; lightNum<r_numDLights ; lightNum++, dl++) {
		if (!(surf->dLightBits & (1<<lightNum)))
			continue;	// Not lit by this light

		if (surf->plane->type < 3)
			fDist = dl->origin[surf->plane->type] - surf->plane->dist;
		else
			fDist = DotProduct (dl->origin, surf->plane->normal) - surf->plane->dist;
		fRad = dl->intensity - (float)fabs (fDist); // fRad is now the highest intensity on the plane
		if (fRad < 0)
			continue;

		impact[0] = dl->origin[0] - (surf->plane->normal[0] * fDist);
		impact[1] = dl->origin[1] - (surf->plane->normal[1] * fDist);
		impact[2] = dl->origin[2] - (surf->plane->normal[2] * fDist);

		sl = DotProduct (impact, surf->texInfo->vecs[0]) + surf->texInfo->vecs[0][3] - surf->textureMins[0];
		st = DotProduct (impact, surf->texInfo->vecs[1]) + surf->texInfo->vecs[1][3] - surf->textureMins[1];

		bl = r_blockLights;
		for (t=0, ftacc=0 ; t<surf->lmHeight ; t++) {
			td = Q_ftol (st - ftacc);
			if (td < 0)
				td = -td;

			for (s=0, fsacc=0 ; s<surf->lmWidth ; s++) {
				sd = Q_ftol (sl - fsacc);
				if (sd < 0)
					sd = -sd;

				if (sd > td)
					fDist = (float)(sd + (td>>1));
				else
					fDist = (float)(td + (sd>>1));

				if (fDist < fRad) {
					scale = fRad - fDist;

					bl[0] += dl->color[0] * scale;
					bl[1] += dl->color[1] * scale;
					bl[2] += dl->color[2] * scale;
				}

				fsacc += 16;
				bl += 3;
			}

			ftacc += 16;
		}
	}
}

/*
=============================================================================

	LIGHT SAMPLING

=============================================================================
*/

/*
===============
R_RecursiveLightPoint
===============
*/
static int RecursiveLightPoint (mBspNode_t *node, vec3_t start, vec3_t end)
{
	float			front, back, frac;
	int				i, s, t, ds, dt, r;
	int				side, map;
	cBspPlane_t		*plane;
	vec3_t			mid;
	mBspSurface_t	*surf;
	mBspTexInfo_t	*tex;
	byte			*lightmap;

	// Didn't hit anything
	if (node->contents != -1)
		return -1;
	
	// Calculate mid point
	plane = node->plane;
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
		return RecursiveLightPoint (node->children[side], start, end);
	
	frac = front / (front - back);
	mid[0] = start[0] + (end[0] - start[0]) * frac;
	mid[1] = start[1] + (end[1] - start[1]) * frac;
	mid[2] = start[2] + (end[2] - start[2]) * frac;
	
	// Go down front side
	r = RecursiveLightPoint (node->children[side], start, mid);
	if (r >= 0)
		return r;	// hit something
		
	if ((back < 0) == side)
		return -1;	// didn't hit anything
		
	// Check for impact on this node
	VectorCopy (mid, r_lightSpot);
	r_lightPlane = plane;

	surf = r_worldModel->surfaces + node->firstSurface;
	for (i=0 ; i<node->numSurfaces ; i++, surf++) {
		if (surf->texInfo->flags & (SURF_TEXINFO_SKY|SURF_TEXINFO_WARP)) 
			continue;	// No lightmaps

		tex = surf->texInfo;
		
		s = (int)(DotProduct (mid, tex->vecs[0]) + tex->vecs[0][3]);
		t = (int)(DotProduct (mid, tex->vecs[1]) + tex->vecs[1][3]);

		if ((s < surf->textureMins[0]) || (t < surf->textureMins[1]))
			continue;
		
		ds = s - surf->textureMins[0];
		dt = t - surf->textureMins[1];
		
		if ((ds > surf->extents[0]) || (dt > surf->extents[1]))
			continue;

		if (!surf->lmSamples)
			return 0;

		ds >>= 4;
		dt >>= 4;

		lightmap = surf->lmSamples;
		VectorClear (r_pointColor);
		if (lightmap) {
			vec3_t scale;

			lightmap += 3 * (dt*surf->lmWidth + ds);

			for (map=0 ; map<surf->numStyles ; map++) {
				VectorScale (r_lightStyles[surf->styles[map]].rgb, gl_modulate->value, scale);
				for (i=0 ; i<3 ; i++)
					r_pointColor[i] += lightmap[i] * scale[i] * (1.0f/255.0f);

				lightmap += 3*surf->lmWidth*surf->lmWidth;
			}
		}
		
		return 1;
	}

	// Go down back side
	return RecursiveLightPoint (node->children[!side], mid, end);
}
qBool R_RecursiveLightPoint (vec3_t point, vec3_t end, vec3_t color)
{
	int	r;

	if (r_refDef.rdFlags & RDF_NOWORLDMODEL || !r_worldModel || !r_worldModel->lightData) {
		VectorSet (color, 1, 1, 1);
		return qFalse;
	}

	r = RecursiveLightPoint (r_worldModel->nodes, point, end);
	
	if (r == -1) {
		VectorClear (color);
		return qFalse;
	}
	else {
		VectorCopy (r_pointColor, color);
		return qTrue;
	}

	return qFalse;
}


/*
===============
R_ShadowForEntity
===============
*/
qBool R_ShadowForEntity (entity_t *ent, vec3_t shadowSpot)
{
	vec3_t		end;
	vec3_t		lightColor;

	VectorSet (end, ent->origin[0], ent->origin[1], ent->origin[2] - 2048);

	if (R_RecursiveLightPoint (ent->origin, end, lightColor)) {
		// Found!
		VectorCopy (r_lightSpot, shadowSpot);
		return qTrue;
	}

	// Not found!
	VectorClear (shadowSpot);
	return qFalse;

}


/*
===============
R_LightForEntity
===============
*/
void R_LightForEntity (entity_t *ent, int numVerts, byte *bArray)
{
	static vec3_t	tempColorsArray[VERTARRAY_MAX_VERTS];
	vec3_t			end;
	vec3_t			lightColor;
	vec3_t			ambientLight;
	vec3_t			directedLight;
	int				r, g, b, a, i;

	//
	// Get the lighting from below
	// FIXME: Compile a list of world lights, then light using them just as you do dlights
	//
	VectorSet (end, ent->origin[0], ent->origin[1], ent->origin[2] - 2048);
	if (R_RecursiveLightPoint (ent->origin, end, lightColor)) {
		// Found!
		VectorCopy (lightColor, directedLight);
		VectorScale (lightColor, 0.6f, ambientLight);
	}
	else {
		// Not found!
		VectorCopy (lightColor, ambientLight);
		VectorCopy (lightColor, directedLight);
	}

	// Save off light value for server to look at (BIG HACK!)
	if (ent->flags & RF_WEAPONMODEL) {
		// Pick the greatest component, which should be
		// the same as the mono value returned by software
		if (lightColor[0] > lightColor[1]) {
			if (lightColor[0] > lightColor[2])
				Cvar_VariableForceSetValue (r_lightlevel, 150 * lightColor[0]);
			else
				Cvar_VariableForceSetValue (r_lightlevel, 150 * lightColor[2]);
		}
		else {
			if (lightColor[1] > lightColor[2])
				Cvar_VariableForceSetValue (r_lightlevel, 150 * lightColor[1]);
			else
				Cvar_VariableForceSetValue (r_lightlevel, 150 * lightColor[2]);
		}

	}

	if (r_fullbright->integer || ent->flags & RF_FULLBRIGHT) {
		for (i=0 ; i<numVerts ; i++) {
			tempColorsArray[i][0] = 1;
			tempColorsArray[i][1] = 1;
			tempColorsArray[i][2] = 1;
			tempColorsArray[i][3] = ent->color[3];
		}
	}
	else {
		vec3_t		dir, direction;
		float		dot;

		// Reset
		for (i=0 ; i<numVerts ; i++) {
			tempColorsArray[i][0] = 0.0f;
			tempColorsArray[i][1] = 0.0f;
			tempColorsArray[i][2] = 0.0f;
			tempColorsArray[i][3] = 1.0f;
		}

		// Only needed to get the location for your own shadow
		if (ent->flags & RF_VIEWERMODEL)
			return;

		//
		// Flag effects
		//
		if (ent->flags & RF_MINLIGHT) {
			for (i=0 ; i<3 ; i++)
				if (ambientLight[i] > 0.1)
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
			scale = 0.1f * (float)sin (r_refDef.time * 7);
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
		VectorSet (dir, -1, 0, 1);
		Matrix_TransformVector (ent->axis, dir, direction);

		for (i=0 ; i<numVerts; i++) {
			dot = DotProduct (rb_normalsArray[i], direction);
			if (dot <= 0)
				VectorCopy (ambientLight, tempColorsArray[i]);
			else
				VectorMA (ambientLight, dot, directedLight, tempColorsArray[i]);
		}

		//
		// Add dynamic lights
		//
		if (gl_dynamic->integer && r_numDLights) {
			int			lightNum;
			dLight_t	*dl;
			float		dist, add, intensity8, intensity;
			vec3_t		dlOrigin;

			for (dl=r_dLightList, lightNum=0 ; lightNum<r_numDLights ; lightNum++, dl++) {
				if (!BoundsAndSphereIntersect (dl->mins, dl->maxs, ent->origin, ent->model->radius * ent->scale))
					continue;

				// Translate
				VectorSubtract (dl->origin, ent->origin, dir);
				dist = VectorLengthFast (dir);

				if (!dist || dist > dl->intensity + ent->model->radius * ent->scale)
					continue;

				// Rotate
				Matrix_TransformVector (ent->axis, dir, dlOrigin);

				// Calculate intensity
				intensity = dl->intensity - dist;
				if (intensity <= 0)
					continue;
				intensity8 = dl->intensity * 8;

				for (i=0 ; i<numVerts ; i++) {
					VectorSubtract (dlOrigin, rb_vertexArray[i], dir);
					add = DotProduct (rb_normalsArray[i], dir);

					// Add some ambience
					VectorMA (tempColorsArray[i], intensity * 0.4f * (1.0f/256.0f), dl->color, tempColorsArray[i]);

					// Shade the verts
					if (add > 0) {
						dot = DotProduct (dir, dir);
						add *= (intensity8 / dot) * Q_RSqrt (dot);
						if (add > 255.0f)
							add = 255.0f / add;

						VectorMA (tempColorsArray[i], add, dl->color, tempColorsArray[i]);
					}
				}
			}
		}
	}

	//
	// Clamp
	//
	for (i=0 ; i<numVerts ; i++, bArray+=4) {
		r = Q_ftol (tempColorsArray[i][0] * ent->color[0]);
		g = Q_ftol (tempColorsArray[i][1] * ent->color[1]);
		b = Q_ftol (tempColorsArray[i][2] * ent->color[2]);
		a = Q_ftol (tempColorsArray[i][3] * ent->color[3]);

		bArray[0] = clamp (r, 0, 255);
		bArray[1] = clamp (g, 0, 255);
		bArray[2] = clamp (b, 0, 255);
		bArray[3] = clamp (a, 0, 255);
	}
}


/*
===============
R_LightPoint
===============
*/
void R_LightPoint (vec3_t point, vec3_t light)
{
	vec3_t		end;
	vec3_t		dist;
	dLight_t	*dl;
	float		add;
	int			lightNum;

	VectorSet (end, point[0], point[1], point[2] - 2048);
	R_RecursiveLightPoint (point, end, light);

	//
	// Add dynamic lights
	//
	for (dl=r_dLightList, lightNum=0 ; lightNum<r_numDLights ; lightNum++, dl++) {
		VectorSubtract (point, dl->origin, dist);
		add = (dl->intensity - VectorLength (dist)) * (1.0f/256.0f);

		if (add > 0)
			VectorMA (light, add, dl->color, light);
	}
}


/*
====================
R_SetLightLevel

Save off light value for server to look at (BIG HACK!)
====================
*/
void R_SetLightLevel (void)
{
	vec3_t		shadelight;

	if (r_refDef.rdFlags & RDF_NOWORLDMODEL)
		return;

	R_LightPoint (r_refDef.viewOrigin, shadelight);

	// Pick the greatest component, which should be
	// the same as the mono value returned by software
	if (shadelight[0] > shadelight[1]) {
		if (shadelight[0] > shadelight[2])
			Cvar_VariableForceSetValue (r_lightlevel, 150 * shadelight[0]);
		else
			Cvar_VariableForceSetValue (r_lightlevel, 150 * shadelight[2]);
	}
	else {
		if (shadelight[1] > shadelight[2])
			Cvar_VariableForceSetValue (r_lightlevel, 150 * shadelight[1]);
		else
			Cvar_VariableForceSetValue (r_lightlevel, 150 * shadelight[2]);
	}

}

/*
=============================================================================

	LIGHTMAP

=============================================================================
*/

typedef struct lightMapState_s {
	int			currTexNum;

	byte		buffer[LIGHTMAP_SIZE*LIGHTMAP_SIZE*LIGHTMAP_BYTES];

	// the lightmap texture data needs to be kept in
	// main memory so texsubimage can update properly
	int			allocated[LIGHTMAP_SIZE];
} lightMapState_t;

static lightMapState_t	lmState;

/*
===============
LM_BuildLightMap

Combine and scale multiple lightmaps into the floating format in blocklights
===============
*/
static void LM_BuildLightMap (mBspSurface_t *surf, byte *dest, int stride)
{
	int			i, j, size;
	int			map;
	float		*bl, max;
	vec3_t		scale;
	byte		*lightMap;

	if (surf->texInfo->flags & (SURF_TEXINFO_SKY|SURF_TEXINFO_WARP))
		Com_Error (ERR_DROP, "LM_BuildLightMap called for non-lit surface");

	size = surf->lmWidth*surf->lmHeight;
	if (size > sizeof (r_blockLights) >> 4)
		Com_Error (ERR_DROP, "Bad r_blockLights size");

	// Set to full bright if no light data
	if (!surf->lmSamples) {
		for (i=0 ; i<size*3 ; i++)
			r_blockLights[i] = 255.0f;
	}
	else {
		lightMap = surf->lmSamples;

		// Add all the lightmaps
		if (surf->numStyles == 1) {
			bl = r_blockLights;

			// Optimal case
			VectorScale (r_lightStyles[surf->styles[0]].rgb, gl_modulate->value, scale);
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
			bl = r_blockLights;
			VectorScale (r_lightStyles[surf->styles[map]].rgb, gl_modulate->value, scale);
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

			for (map=1 ; map<surf->numStyles ; map++) {
				bl = r_blockLights;

				VectorScale (r_lightStyles[surf->styles[map]].rgb, gl_modulate->value, scale);
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
		if (surf->dLightFrame == r_frameCount)
			R_AddDynamicLights (surf);
	}

	// Put into texture format
	stride -= (surf->lmWidth << 2);
	bl = r_blockLights;

	for (i=0 ; i<surf->lmHeight ; i++) {
		for (j=0 ; j<surf->lmWidth ; j++) {
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
				max = 255.0F / max;

				dest[0] = (byte)Q_ftol (bl[0]*max);
				dest[1] = (byte)Q_ftol (bl[1]*max);
				dest[2] = (byte)Q_ftol (bl[2]*max);
				dest[3] = (byte)Q_ftol (255*max);
			}
			else {
				dest[0] = (byte)Q_ftol (bl[0]);
				dest[1] = (byte)Q_ftol (bl[1]);
				dest[2] = (byte)Q_ftol (bl[2]);
				dest[3] = 255;
			}

			bl += 3;
			dest += 4;
		}

		dest += stride;
	}
}


/*
================
LM_UploadBlock
================
*/
static void LM_UploadBlock (void)
{
	GL_BindTexture (&r_lmTextures[lmState.currTexNum]);

	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	qglTexImage2D (GL_TEXTURE_2D,
					0,
					r_lmTextures[lmState.currTexNum].format,
					LIGHTMAP_SIZE, LIGHTMAP_SIZE,
					0,
					GL_LIGHTMAP_FORMAT,
					GL_UNSIGNED_BYTE,
					lmState.buffer);

	if (++lmState.currTexNum >= MAX_LIGHTMAPS)
		Com_Error (ERR_DROP, "LM_UploadBlock: - MAX_LIGHTMAPS exceeded\n");
}


/*
================
LM_AllocBlock

Returns a texture number and the position inside it
================
*/
static qBool LM_AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;

	best = LIGHTMAP_SIZE;
	for (i=0 ; i<LIGHTMAP_SIZE-w ; i++) {
		best2 = 0;

		for (j=0 ; j<w ; j++) {
			if (lmState.allocated[i+j] >= best)
				break;

			if (lmState.allocated[i+j] > best2)
				best2 = lmState.allocated[i+j];
		}

		if (j == w) {
			// This is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if (best + h > LIGHTMAP_SIZE)
		return qFalse;

	for (i=0 ; i<w ; i++) {
		lmState.allocated[*x + i] = best + h;
	}

	return qTrue;
}


/*
===============
LM_SetCacheState
===============
*/
static void LM_SetCacheState (mBspSurface_t *surf)
{
	int		map;

	for (map=0 ; map<surf->numStyles ; map++) {
		surf->cachedLight[map] = r_lightStyles[surf->styles[map]].white;
	}
}


/*
=======================
LM_UpdateLightmap
=======================
*/
void LM_UpdateLightmap (mBspSurface_t *surf)
{
	static uInt	temp[LIGHTMAP_SIZE*LIGHTMAP_SIZE];
	image_t		*lightMap;
	int			map;

	// Is this surface allowed to have a lightmap?
	if (surf->texInfo->flags & (SURF_TEXINFO_SKY|SURF_TEXINFO_WARP)) {
		surf->lmTexNumActive = -1;
		return;
	}

	// Dynamic this frame or dynamic previously
	if (gl_dynamic->integer) {
		for (map=0 ; map<surf->numStyles ; map++) {
			if (r_lightStyles[surf->styles[map]].white != surf->cachedLight[map])
				goto dynamic;
		}

		if (surf->dLightFrame == r_frameCount)
			goto dynamic;
	}

	// No need to update
	surf->lmTexNumActive = surf->lmTexNum;
	return;

dynamic:
	// Update texture
	LM_BuildLightMap (surf, (void *)temp, surf->lmWidth*4);

	if ((surf->styles[map] >= 32 || surf->styles[map] == 0) && surf->dLightFrame != r_frameCount) {
		LM_SetCacheState (surf);

		lightMap = &r_lmTextures[surf->lmTexNum];
		surf->lmTexNumActive = surf->lmTexNum;
	}
	else {
		lightMap = &r_lmTextures[0];
		surf->lmTexNumActive = 0;
	}

	GL_BindTexture (lightMap);
	qglTexSubImage2D (GL_TEXTURE_2D, 0,
					surf->lmCoords[0], surf->lmCoords[1],
					surf->lmWidth, surf->lmHeight,
					GL_LIGHTMAP_FORMAT,
					GL_UNSIGNED_BYTE,
					temp);
}


/*
==================
LM_BeginBuildingLightmaps
==================
*/
void LM_BeginBuildingLightmaps (void)
{
	int		i;

	memset (&lmState, 0, sizeof (lmState));
	memset (lmState.buffer, 255, sizeof (lmState.buffer));

	// Setup the base light styles
	for (i=0 ; i<MAX_CS_LIGHTSTYLES ; i++) {
		VectorSet (r_lightStyles[i].rgb, 1, 1, 1);
		r_lightStyles[i].white = 3;
	}

	// Fill in lightmap image structs
	for (i=0 ; i<MAX_LIGHTMAPS ; i++) {
		r_lmTextures[i].texNum = TEX_LIGHTMAPS + i;
		r_lmTextures[0].flags = 0;
		r_lmTextures[i].name[0] = r_lmTextures[i].bareName[0] = 0;		
		r_lmTextures[i].width = r_lmTextures[i].upWidth = LIGHTMAP_SIZE;
		r_lmTextures[i].height = r_lmTextures[i].upHeight = LIGHTMAP_SIZE;
		r_lmTextures[i].touchFrame = 0;
		r_lmTextures[i].target = GL_TEXTURE_2D;
		r_lmTextures[i].format = glState.texRGBFormat;
	}

	// Initialize the base dynamic lightmap texture
	LM_UploadBlock ();
}


/*
========================
LM_CreateSurfaceLightmap
========================
*/
void LM_CreateSurfaceLightmap (mBspSurface_t *surf)
{
	byte	*base;

	if (!LM_AllocBlock (surf->lmWidth, surf->lmHeight, &surf->lmCoords[0], &surf->lmCoords[1])) {
		LM_UploadBlock ();
		memset (lmState.allocated, 0, sizeof (lmState.allocated));

		if (!LM_AllocBlock (surf->lmWidth, surf->lmHeight, &surf->lmCoords[0], &surf->lmCoords[1]))
			Com_Error (ERR_FATAL, "Consecutive calls to LM_AllocBlock (%d, %d) failed\n", surf->lmWidth, surf->lmHeight);
	}

	surf->lmTexNum = lmState.currTexNum;
	surf->lmTexNumActive = -1;

	base = lmState.buffer;
	base += (surf->lmCoords[1] * LIGHTMAP_SIZE + surf->lmCoords[0]) * LIGHTMAP_BYTES;

	LM_SetCacheState (surf);
	LM_BuildLightMap (surf, base, LIGHTMAP_SIZE*LIGHTMAP_BYTES);
}


/*
=======================
LM_EndBuildingLightmaps
=======================
*/
void LM_EndBuildingLightmaps (void)
{
	LM_UploadBlock ();
}
