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

// the lightmap texture data needs to be kept in
// main memory so texsubimage can update properly
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
#define DEBUGLIGHT_SIZE	8
void R_AddDLightsToList (void)
{
	int			i, k;
	dLight_t	*light;
	float		a, rad;
	vec3_t		v;

	if (r_debugLighting->integer) {
		GL_SelectTexture (0);
		qglDepthMask (GL_FALSE);
		qglDisable (GL_TEXTURE_2D);
		qglEnable (GL_BLEND);
		GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		for (light=r_refScene.dLightList, i=0 ; i<r_refScene.numDLights ; i++, light++) {
			//
			// Radius
			//
			qglColor4f (light->color[0], light->color[1], light->color[2], 0.2f);

			qglBegin (GL_TRIANGLE_FAN);
			qglVertex3f (light->origin[0] - (r_refDef.forwardVec[0] * light->intensity),
				light->origin[1] - (r_refDef.forwardVec[1] * light->intensity),
				light->origin[2] - (r_refDef.forwardVec[2] * light->intensity));

			for (k=32 ; k>=0 ; k--) {
				a = (k / 32.0f) * (M_PI * 2.0f);

				v[0] = light->origin[0] + (r_refDef.rightVec[0] * (float)cos (a) * light->intensity) + (r_refDef.upVec[0] * (float)sin (a) * light->intensity);
				v[1] = light->origin[1] + (r_refDef.rightVec[1] * (float)cos (a) * light->intensity) + (r_refDef.upVec[1] * (float)sin (a) * light->intensity);
				v[2] = light->origin[2] + (r_refDef.rightVec[2] * (float)cos (a) * light->intensity) + (r_refDef.upVec[2] * (float)sin (a) * light->intensity);
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

		qglDisable (GL_BLEND);
		qglEnable (GL_TEXTURE_2D);
		qglDepthMask (GL_TRUE);
	}

	if (!gl_flashblend->integer)
		return;

	qglDepthMask (GL_FALSE);
	qglDisable (GL_TEXTURE_2D);
	qglEnable (GL_BLEND);
	GL_BlendFunc (GL_ONE, GL_ONE);

	for (light=r_refScene.dLightList, i=0 ; i<r_refScene.numDLights ; i++, light++) {
		rad = light->intensity * 0.7f;
		qglBegin (GL_TRIANGLE_FAN);

		qglColor3f (light->color[0] * 0.2f, light->color[1] * 0.2f, light->color[2] * 0.2f);

		v[0] = light->origin[0] - (r_refDef.forwardVec[0] * rad);
		v[1] = light->origin[1] - (r_refDef.forwardVec[1] * rad);
		v[2] = light->origin[2] - (r_refDef.forwardVec[2] * rad);
		qglVertex3fv (v);

		qglColor3f (0, 0, 0);
		for (k=32 ; k>=0 ; k--) {
			a = (k / 32.0f) * (M_PI * 2.0f);

			v[0] = light->origin[0] + (r_refDef.rightVec[0] * (float)cos (a) * rad) + (r_refDef.upVec[0] * (float)sin (a) * rad);
			v[1] = light->origin[1] + (r_refDef.rightVec[1] * (float)cos (a) * rad) + (r_refDef.upVec[1] * (float)sin (a) * rad);
			v[2] = light->origin[2] + (r_refDef.rightVec[2] * (float)cos (a) * rad) + (r_refDef.upVec[2] * (float)sin (a) * rad);
			qglVertex3fv (v);
		}

		qglEnd ();
	}

	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDisable (GL_BLEND);
	qglEnable (GL_TEXTURE_2D);
	qglDepthMask (GL_TRUE);
}


/*
=============
R_Q2BSP_MarkLights
=============
*/
void R_Q2BSP_MarkLights (model_t *model, mBspNode_t *node, dLight_t *light, uint32 bit, qBool visCull)
{
	cBspPlane_t		*plane;
	mBspSurface_t	*surf;
	float			dist;
	int				j;
	
	if (node->q2_contents != -1)
		return;
	if (visCull && node->visFrame != r_refScene.visFrameCount)
		return;

	plane = node->plane;
	if (plane->type < 3)
		dist = light->origin[plane->type] - plane->dist;
	else
		dist = DotProduct (light->origin, plane->normal) - plane->dist;

	if (dist > light->intensity) {
		R_Q2BSP_MarkLights (model, node->children[0], light, bit, visCull);
		return;
	}
	if (dist < -light->intensity) {
		R_Q2BSP_MarkLights (model, node->children[1], light, bit, visCull);
		return;
	}

	// Mark the polygons
	surf = model->bspModel.surfaces + node->q2_firstSurface;
	for (j=0 ; j<node->q2_numSurfaces ; j++, surf++) {
		if (surf->dLightFrame != r_refScene.frameCount) {
			surf->dLightFrame = r_refScene.frameCount;
			surf->dLightBits = 0;
		}
		surf->dLightBits |= bit;
	}

	R_Q2BSP_MarkLights (model, node->children[0], light, bit, visCull);
	R_Q2BSP_MarkLights (model, node->children[1], light, bit, visCull);
}


/*
=============
R_Q2BSP_PushDLights
=============
*/
void R_Q2BSP_PushDLights (void)
{
	int			i;
	dLight_t	*lt;

	if (gl_flashblend->integer)
		return;

	for (lt=r_refScene.dLightList, i=0 ; i<r_refScene.numDLights ; i++, lt++)
		R_Q2BSP_MarkLights (r_refScene.worldModel, r_refScene.worldModel->bspModel.nodes, lt, 1<<i, qTrue);
}


/*
===============
R_Q2BSP_AddDynamicLights
===============
*/
static void R_Q2BSP_AddDynamicLights (mBspSurface_t *surf)
{
	int			lightNum, sd, td, s, t;
	float		fDist, fRad;
	float		scale, sl, st;
	float		fsacc, ftacc;
	float		*bl;
	vec3_t		impact;
	dLight_t	*dl;

	for (lightNum=0, dl=r_refScene.dLightList ; lightNum<r_refScene.numDLights ; lightNum++, dl++) {
		if (!(surf->dLightBits & (1<<lightNum)))
			continue;	// Not lit by this light

		if (surf->q2_plane->type < 3)
			fDist = dl->origin[surf->q2_plane->type] - surf->q2_plane->dist;
		else
			fDist = DotProduct (dl->origin, surf->q2_plane->normal) - surf->q2_plane->dist;
		fRad = dl->intensity - (float)fabs (fDist); // fRad is now the highest intensity on the plane
		if (fRad < 0)
			continue;

		impact[0] = dl->origin[0] - (surf->q2_plane->normal[0] * fDist);
		impact[1] = dl->origin[1] - (surf->q2_plane->normal[1] * fDist);
		impact[2] = dl->origin[2] - (surf->q2_plane->normal[2] * fDist);

		sl = DotProduct (impact, surf->q2_texInfo->vecs[0]) + surf->q2_texInfo->vecs[0][3] - surf->q2_textureMins[0];
		st = DotProduct (impact, surf->q2_texInfo->vecs[1]) + surf->q2_texInfo->vecs[1][3] - surf->q2_textureMins[1];

		bl = r_blockLights;
		for (t=0, ftacc=0 ; t<surf->q2_lmHeight ; t++) {
			td = Q_ftol (st - ftacc);
			if (td < 0)
				td = -td;

			for (s=0, fsacc=0 ; s<surf->q2_lmWidth ; s++) {
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
	mBspSurface_t	*surf;
	mQ2BspTexInfo_t	*tex;
	byte			*lightmap;

	// Didn't hit anything
	if (node->q2_contents != -1)
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
	VectorCopy (mid, r_lightSpot);
	r_lightPlane = plane;

	surf = r_refScene.worldModel->bspModel.surfaces + node->q2_firstSurface;
	for (i=0 ; i<node->q2_numSurfaces ; i++, surf++) {
		if (surf->q2_texInfo->flags & (SURF_TEXINFO_SKY|SURF_TEXINFO_WARP)) 
			continue;	// No lightmaps

		tex = surf->q2_texInfo;

		s = Q_ftol (DotProduct (mid, tex->vecs[0]) + tex->vecs[0][3]);
		t = Q_ftol (DotProduct (mid, tex->vecs[1]) + tex->vecs[1][3]);
		if (s < surf->q2_textureMins[0] || t < surf->q2_textureMins[1])
			continue;

		ds = s - surf->q2_textureMins[0];
		dt = t - surf->q2_textureMins[1];
		if (ds > surf->q2_extents[0] || dt > surf->q2_extents[1])
			continue;

		if (!surf->q2_lmSamples)
			return 0;	// No lightmaps

		ds >>= 4;
		dt >>= 4;

		lightmap = surf->q2_lmSamples;
		VectorClear (r_pointColor);
		if (lightmap) {
			vec3_t scale;

			lightmap += 3 * (dt*surf->q2_lmWidth + ds);

			for (map=0 ; map<surf->q2_numStyles ; map++) {
				VectorScale (r_refScene.lightStyles[surf->q2_styles[map]].rgb, gl_modulate->value, scale);
				for (i=0 ; i<3 ; i++)
					r_pointColor[i] += lightmap[i] * scale[i] * (1.0f/255.0f);

				lightmap += 3*surf->q2_lmWidth*surf->q2_lmWidth;
			}
		}
		
		return 1;
	}

	// Go down back side
	return Q2BSP_RecursiveLightPoint (node->children[!side], mid, end);
}
static qBool R_Q2BSP_RecursiveLightPoint (vec3_t point, vec3_t end, vec3_t color)
{
	int	r;

	if (r_refDef.rdFlags & RDF_NOWORLDMODEL || !r_refScene.worldModel->q2BspModel.lightData) {
		VectorSet (color, 1, 1, 1);
		return qFalse;
	}

	r = Q2BSP_RecursiveLightPoint (r_refScene.worldModel->bspModel.nodes, point, end);
	
	if (r == -1) {
		VectorClear (color);
		return qFalse;
	}

	VectorCopy (r_pointColor, color);
	return qTrue;
}


/*
===============
R_Q2BSP_ShadowForEntity
===============
*/
static qBool R_Q2BSP_ShadowForEntity (entity_t *ent, vec3_t shadowSpot)
{
	vec3_t		end;
	vec3_t		lightColor;

	VectorSet (end, ent->origin[0], ent->origin[1], ent->origin[2] - 2048);

	if (R_Q2BSP_RecursiveLightPoint (ent->origin, end, lightColor)) {
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
R_Q2BSP_LightForEntity
===============
*/
static void R_Q2BSP_LightForEntity (entity_t *ent, int numVerts, byte *bArray)
{
	static vec3_t	tempColorsArray[VERTARRAY_MAX_VERTS];
	float			*cArray;
	vec3_t			end;
	vec3_t			lightColor;
	vec3_t			ambientLight;
	vec3_t			directedLight;
	int				r, g, b, i;
	vec3_t			dir, direction;
	float			dot;

	//
	// Get the lighting from below
	//
	VectorSet (end, ent->origin[0], ent->origin[1], ent->origin[2] - 2048);
	if (R_Q2BSP_RecursiveLightPoint (ent->origin, end, lightColor)) {
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
				Cvar_VariableSetValue (r_lightlevel, 150 * lightColor[0], qTrue);
			else
				Cvar_VariableSetValue (r_lightlevel, 150 * lightColor[2], qTrue);
		}
		else {
			if (lightColor[1] > lightColor[2])
				Cvar_VariableSetValue (r_lightlevel, 150 * lightColor[1], qTrue);
			else
				Cvar_VariableSetValue (r_lightlevel, 150 * lightColor[2], qTrue);
		}

	}

	if (r_fullbright->integer || ent->flags & RF_FULLBRIGHT) {
		for (i=0 ; i<numVerts ; i++) {
			tempColorsArray[i][0] = 1;
			tempColorsArray[i][1] = 1;
			tempColorsArray[i][2] = 1;
		}
	}
	else {
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
		Matrix3_TransformVector (ent->axis, dir, direction);

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
		if (gl_dynamic->integer && r_refScene.numDLights) {
			int			lightNum;
			dLight_t	*dl;
			float		dist, add, intensity8, intensity;
			vec3_t		dlOrigin;

			for (dl=r_refScene.dLightList, lightNum=0 ; lightNum<r_refScene.numDLights ; lightNum++, dl++) {
				if (!BoundsAndSphereIntersect (dl->mins, dl->maxs, ent->origin, ent->model->radius * ent->scale))
					continue;

				// Translate
				VectorSubtract (dl->origin, ent->origin, dir);
				dist = VectorLengthFast (dir);

				if (!dist || dist > dl->intensity + ent->model->radius * ent->scale)
					continue;

				// Rotate
				Matrix3_TransformVector (ent->axis, dir, dlOrigin);

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
	cArray = tempColorsArray[0];
	for (i=0 ; i<numVerts ; i++, bArray+=4, cArray+=3) {
		r = Q_ftol (cArray[0] * ent->color[0]);
		g = Q_ftol (cArray[1] * ent->color[1]);
		b = Q_ftol (cArray[2] * ent->color[2]);

		bArray[0] = clamp (r, 0, 255);
		bArray[1] = clamp (g, 0, 255);
		bArray[2] = clamp (b, 0, 255);
		bArray[3] = 255;
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
	dLight_t	*dl;
	float		add;
	int			lightNum;

	VectorSet (end, point[0], point[1], point[2] - 2048);
	R_Q2BSP_RecursiveLightPoint (point, end, light);

	//
	// Add dynamic lights
	//
	for (dl=r_refScene.dLightList, lightNum=0 ; lightNum<r_refScene.numDLights ; lightNum++, dl++) {
		VectorSubtract (point, dl->origin, dist);
		add = (dl->intensity - VectorLength (dist)) * (1.0f/256.0f);

		if (add > 0)
			VectorMA (light, add, dl->color, light);
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

	R_Q2BSP_LightPoint (r_refDef.viewOrigin, shadelight);

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

	QUAKE2 LIGHTMAP

=============================================================================
*/

static byte		*r_q2lmBuffer;
static int		r_q2lmNumUploaded;
static int		*r_q2lmAllocated;
int				r_q2lmSize;

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
	if (size > sizeof (r_blockLights) >> 4)
		Com_Error (ERR_DROP, "Bad r_blockLights size");

	// Set to full bright if no light data
	if (!surf->q2_lmSamples || r_fullbright->integer) {
		for (i=0 ; i<size*3 ; i++)
			r_blockLights[i] = 255.0f;
	}
	else {
		lightMap = surf->q2_lmSamples;

		// Add all the lightmaps
		if (surf->q2_numStyles == 1) {
			bl = r_blockLights;

			// Optimal case
			VectorScale (r_refScene.lightStyles[surf->q2_styles[0]].rgb, gl_modulate->value, scale);
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
			VectorScale (r_refScene.lightStyles[surf->q2_styles[map]].rgb, gl_modulate->value, scale);
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
				bl = r_blockLights;

				VectorScale (r_refScene.lightStyles[surf->q2_styles[map]].rgb, gl_modulate->value, scale);
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
		if (surf->dLightFrame == r_refScene.frameCount)
			R_Q2BSP_AddDynamicLights (surf);
	}

	// Put into texture format
	stride -= (surf->q2_lmWidth << 2);
	bl = r_blockLights;

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
================
R_Q2BSP_UploadLMBlock
================
*/
static void R_Q2BSP_UploadLMBlock (void)
{
	if (r_q2lmNumUploaded+1 >= R_MAX_LIGHTMAPS)
		Com_Error (ERR_DROP, "R_Q2BSP_UploadLMBlock: - R_MAX_LIGHTMAPS exceeded\n");

	r_lmTextures[r_q2lmNumUploaded++] = R_Load2DImage (Q_VarArgs ("*lm%i", r_q2lmNumUploaded), (byte **)(&r_q2lmBuffer),
		r_q2lmSize, r_q2lmSize, IF_NOPICMIP|IF_NOMIPMAP_LINEAR|IF_NOGAMMA|IF_NOINTENS|IF_NOCOMPRESS|IT_LIGHTMAP, 3);
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

	best = r_q2lmSize;
	for (i=0 ; i<r_q2lmSize-w ; i++) {
		best2 = 0;

		for (j=0 ; j<w ; j++) {
			if (r_q2lmAllocated[i+j] >= best)
				break;

			if (r_q2lmAllocated[i+j] > best2)
				best2 = r_q2lmAllocated[i+j];
		}

		if (j == w) {
			// This is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if (best + h > r_q2lmSize)
		return qFalse;

	for (i=0 ; i<w ; i++)
		r_q2lmAllocated[*x + i] = best + h;

	return qTrue;
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
		surf->q2_cachedLight[map] = r_refScene.lightStyles[surf->q2_styles[map]].white;
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

	// Is this surface allowed to have a lightmap?
	if (surf->q2_texInfo->flags & (SURF_TEXINFO_SKY|SURF_TEXINFO_WARP)) {
		surf->q2_lmTexNumActive = -1;
		return;
	}

	// Dynamic this frame or dynamic previously
	if (gl_dynamic->integer) {
		for (map=0 ; map<surf->q2_numStyles ; map++) {
			if (r_refScene.lightStyles[surf->q2_styles[map]].white != surf->q2_cachedLight[map])
				goto dynamic;
		}

		if (surf->dLightFrame == r_refScene.frameCount)
			goto dynamic;
	}

	// No need to update
	surf->q2_lmTexNumActive = surf->lmTexNum;
	return;

dynamic:
	// Update texture
	R_Q2BSP_BuildLightMap (surf, (void *)temp, surf->q2_lmWidth*4);
	if ((surf->q2_styles[map] >= 32 || surf->q2_styles[map] == 0) && surf->dLightFrame != r_refScene.frameCount) {
		R_Q2BSP_SetLMCacheState (surf);

		GL_BindTexture (r_lmTextures[surf->lmTexNum]);
		surf->q2_lmTexNumActive = surf->lmTexNum;
	}
	else {
		GL_BindTexture (r_lmTextures[0]);
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
==================
R_Q2BSP_BeginBuildingLightmaps
==================
*/
void R_Q2BSP_BeginBuildingLightmaps (void)
{
	int		size, i;

	// Should be no lightmaps at this point
	r_q2lmNumUploaded = 0;

	// Find the maximum size
	for (size=1 ; size<Q2LIGHTMAP_WIDTH && size<glConfig.maxTexSize ; size<<=1);
	r_q2lmSize = size;

	// Allocate buffers and clear values
	r_q2lmAllocated = Mem_AllocExt (sizeof (int) * r_q2lmSize, qTrue);
	r_q2lmBuffer = Mem_AllocExt (r_q2lmSize*r_q2lmSize*4, qFalse);
	memset (r_q2lmBuffer, 255, r_q2lmSize*r_q2lmSize*4);

	// Setup the base light styles
	for (i=0 ; i<MAX_CS_LIGHTSTYLES ; i++) {
		VectorSet (r_refScene.lightStyles[i].rgb, 1, 1, 1);
		r_refScene.lightStyles[i].white = 3;
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
		memset (r_q2lmAllocated, 0, sizeof (int) * r_q2lmSize);

		if (!R_Q2BSP_AllocLMBlock (surf->q2_lmWidth, surf->q2_lmHeight, &surf->q2_lmCoords[0], &surf->q2_lmCoords[1]))
			Com_Error (ERR_FATAL, "Consecutive calls to R_Q2BSP_AllocLMBlock (%d, %d) failed\n", surf->q2_lmWidth, surf->q2_lmHeight);
	}

	surf->lmTexNum = r_q2lmNumUploaded;
	surf->q2_lmTexNumActive = -1;

	base = r_q2lmBuffer;
	base += ((surf->q2_lmCoords[1] * r_q2lmSize + surf->q2_lmCoords[0]) * 4);

	R_Q2BSP_SetLMCacheState (surf);
	R_Q2BSP_BuildLightMap (surf, base, r_q2lmSize*4);
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
	Mem_Free (r_q2lmAllocated);
	Mem_Free (r_q2lmBuffer);
}


/*
=======================
R_Q2BSP_TouchLightmaps
=======================
*/
static void R_Q2BSP_TouchLightmaps (void)
{
	int		i;

	for (i=0 ; i<r_q2lmNumUploaded ; i++)
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
	VectorSet (mins, origin[0] - intensity, origin[1] - intensity, origin[2] - intensity);
	VectorSet (maxs, origin[0] + intensity, origin[1] + intensity, origin[2] + intensity);
}


/*
=============
R_LightPoint
=============
*/
void R_LightPoint (vec3_t point, vec3_t light)
{
	if (r_refDef.rdFlags & RDF_NOWORLDMODEL)
		return;

	if (r_refScene.worldModel->type == MODEL_Q2BSP) {
		R_Q2BSP_LightPoint (point, light);
		return;
	}

	VectorSet (light, 1, 1, 1);
}


/*
=============
R_SetLightLevel
=============
*/
void R_SetLightLevel (void)
{
	if (r_refDef.rdFlags & RDF_NOWORLDMODEL)
		return;

	if (r_refScene.worldModel->type == MODEL_Q2BSP) {
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
	if (r_refDef.rdFlags & RDF_NOWORLDMODEL)
		return;

	if (r_refScene.worldModel->type == MODEL_Q2BSP) {
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
qBool R_ShadowForEntity (entity_t *ent, vec3_t shadowSpot)
{
	if (r_refDef.rdFlags & RDF_NOWORLDMODEL)
		return qFalse;

	if (r_refScene.worldModel->type == MODEL_Q2BSP)
		return R_Q2BSP_ShadowForEntity (ent, shadowSpot);

	return qFalse;
}


/*
=============
R_LightForEntity
=============
*/
void R_LightForEntity (entity_t *ent, int numVerts, byte *bArray)
{
	if (r_refDef.rdFlags & RDF_NOWORLDMODEL || r_refScene.worldModel->type == MODEL_Q2BSP) {
		R_Q2BSP_LightForEntity (ent, numVerts, bArray);
		return;
	}

	R_Q3BSP_LightForEntity (ent, numVerts, bArray);
}
