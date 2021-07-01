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

static float	r_BlockLights[34*34*3];

vec3_t			r_PointColor;
cBspPlane_t		*r_LightPlane;		// used for the shadow plane
vec3_t			r_LightSpot;

/*
=============================================================================

	DYNAMIC LIGHTS

=============================================================================
*/

/*
=============
R_RenderDLights
=============
*/
void R_RenderDLights (void) {
	int			i, j, k;
	dLight_t	*light;
	float		a, rad;
	vec3_t		v;

	if (!gl_flashblend->integer)
		return;

	qglDepthMask (GL_FALSE);
	qglDisable (GL_TEXTURE_2D);
	qglEnable (GL_BLEND);
	GL_BlendFunc (GL_ONE, GL_ONE);

	for (light=r_DLights, i=0 ; i<r_NumDLights ; i++, light++) {
		rad = light->intensity * 0.7;
		qglBegin (GL_TRIANGLE_FAN);

		if (gl_flashblend->integer == 2)
			qglColor3f (light->color[0] * 0.05, light->color[1] * 0.05, light->color[2] * 0.05);
		else
			qglColor3f (light->color[0] * 0.2, light->color[1] * 0.2, light->color[2] * 0.2);

		for (k=0 ; k<3 ; k++)
			v[k] = light->origin[k] - (v_Forward[k] * rad);
		qglVertex3fv (v);

		qglColor3f (0, 0, 0);
		for (k=32 ; k>=0 ; k--) {
			a = (k / 32.0) * M_PI_TIMES2;
			for (j=0 ; j<3 ; j++)
				v[j] = light->origin[j] + (v_Right[j] * cos (a) * rad) + (v_Up[j] * sin (a) * rad);
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
void R_MarkLights (dLight_t *light, int bit, mBspNode_t *node) {
	cBspPlane_t		*plane;
	mBspSurface_t	*surf;
	float			dist;
	int				j;
	
	if (node->contents != -1)
		return;

	plane = node->plane;
	if (plane->type < 3)
		dist = light->origin[plane->type] - plane->dist;
	else
		dist = DotProduct(light->origin, plane->normal) - plane->dist;

	if (dist > (light->intensity)) {
		R_MarkLights (light, bit, node->children[0]);
		return;
	}
	if (dist < (-light->intensity)) {
		R_MarkLights (light, bit, node->children[1]);
		return;
	}

	// mark the polygons
	surf = r_WorldModel->surfaces + node->firstSurface;
	for (j=0 ; j<node->numSurfaces ; j++, surf++) {
		if (surf->dLightFrame != r_FrameCount) {
			surf->dLightBits = 0;
			surf->dLightFrame = r_FrameCount;
		}
		surf->dLightBits |= bit;
	}

	R_MarkLights (light, bit, node->children[0]);
	R_MarkLights (light, bit, node->children[1]);
}


/*
=============
R_PushDLights
=============
*/
void R_PushDLights (void) {
	int			i;
	dLight_t	*dl;

	if ((gl_flashblend->integer != 2) && gl_flashblend->integer)
		return;

	for (dl=r_DLights, i=0 ; i<r_NumDLights ; i++, dl++)
		R_MarkLights (dl, 1 << i, r_WorldModel->nodes);
}


/*
===============
R_AddDynamicLights
===============
*/
void R_AddDynamicLights (mBspSurface_t *surf) {
	int				lnum, sd, td, s, t, i;
	float			fDist, fRad;
	float			scale, sl, st;
	float			fsacc, ftacc;
	float			*bl;
	vec3_t			impact;
	cBspPlane_t		*plane;
	mBspTexInfo_t	*tex;
	dLight_t		*dl;

	tex = surf->texInfo;

	for (lnum=0, dl=r_DLights ; lnum<r_NumDLights ; lnum++, dl++) {
		if (!(surf->dLightBits & (1<<lnum)))
			continue;	// not lit by this light

		plane = surf->plane;
		if (plane->type < 3)
			fDist = dl->origin[plane->type] - plane->dist;
		else
			fDist = DotProduct (dl->origin, plane->normal) - plane->dist;
		fRad = dl->intensity - fabs (fDist); // fRad is now the highest intensity on the plane
		if (fRad < 0)
			continue;

		for (i=0 ; i<3 ; i++)
			impact[i] = dl->origin[i] - (surf->plane->normal[i] * fDist);

		sl = DotProduct (impact, tex->vecs[0]) + tex->vecs[0][3] - surf->textureMins[0];
		st = DotProduct (impact, tex->vecs[1]) + tex->vecs[1][3] - surf->textureMins[1];

		bl = r_BlockLights;
		for (t=0, ftacc=0 ; t<surf->lmHeight ; t++) {
			td = st - ftacc;
			if (td < 0)
				td = -td;

			for (s=0, fsacc=0 ; s<surf->lmWidth ; s++) {
				sd = Q_ftol (sl - fsacc);

				if (sd < 0)
					sd = -sd;

				if (sd > td)
					fDist = sd + (td>>1);
				else
					fDist = td + (sd>>1);

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
int RecursiveLightPoint (mBspNode_t *node, vec3_t start, vec3_t end) {
	float			front, back, frac;
	int				i, s, t, ds, dt, r;
	int				side, map;
	cBspPlane_t		*plane;
	vec3_t			mid;
	mBspSurface_t	*surf;
	mBspTexInfo_t	*tex;
	byte			*lightmap;

	// didn't hit anything
	if (node->contents != -1)
		return -1;
	
	// calculate mid point
	plane = node->plane;
	if (plane->type < 3) {
		front = start[plane->type] - plane->dist;
		back = end[plane->type] - plane->dist;
	} else {
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
	
	// go down front side
	r = RecursiveLightPoint (node->children[side], start, mid);
	if (r >= 0)
		return r;	// hit something
		
	if ((back < 0) == side)
		return -1;	// didn't hit anything
		
	// check for impact on this node
	VectorCopy (mid, r_LightSpot);
	r_LightPlane = plane;

	surf = r_WorldModel->surfaces + node->firstSurface;
	for (i=0 ; i<node->numSurfaces ; i++, surf++) {
		// no lightmaps
		if (surf->flags & (SURF_DRAWTURB|SURF_DRAWSKY)) 
			continue;

		tex = surf->texInfo;
		
		s = DotProduct (mid, tex->vecs[0]) + tex->vecs[0][3];
		t = DotProduct (mid, tex->vecs[1]) + tex->vecs[1][3];;

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
		VectorClear (r_PointColor);
		if (lightmap) {
			vec3_t scale;

			lightmap += 3 * (dt*surf->lmWidth + ds);

			for (map=0 ; map<surf->numStyles ; map++) {
				VectorScale (r_LightStyles[surf->styles[map]].rgb, gl_modulate->value, scale);
				for (i=0 ; i<3 ; i++)
					r_PointColor[i] += lightmap[i] * scale[i] * ONEDIV255;

				lightmap += 3*surf->lmWidth*surf->lmWidth;
			}
		}
		
		return 1;
	}

	// go down back side
	return RecursiveLightPoint (node->children[!side], mid, end);
}
qBool R_RecursiveLightPoint (vec3_t point, vec3_t end, vec3_t color) {
	float	r;

	if ((r_RefDef.rdFlags & RDF_NOWORLDMODEL) || !r_WorldModel || !r_WorldModel->lightData) {
		VectorSet (color, 1, 1, 1);
		return qFalse;
	}

	r = RecursiveLightPoint (r_WorldModel->nodes, point, end);
	
	if (r == -1) {
		VectorClear (color);
		return qFalse;
	} else {
		VectorCopy (r_PointColor, color);
		return qTrue;
	}

	return qFalse;
}


/*
===============
R_LightForEntity
===============
*/
void R_LightForEntity (entity_t *ent, int numVerts, qBool hasShell, qBool hasShadow, vec3_t shadowspot) {
	int			i;
	vec3_t		end;
	vec3_t		lightColor;
	vec3_t		ambientLight;
	vec3_t		directedLight;

	//
	// get the lighting from below
	//
	VectorSet (end, ent->origin[0], ent->origin[1], ent->origin[2] - 2048);
	if (R_RecursiveLightPoint (ent->origin, end, lightColor)) {
		// found!
		if (hasShadow)
			VectorCopy (r_LightSpot, shadowspot);

		VectorScale (lightColor, 1, directedLight);
		VectorScale (lightColor, 0.6, ambientLight);
	} else {
		// not found!
		if (hasShadow) {
			hasShadow = qFalse;
			VectorClear (shadowspot);
		}

		VectorCopy (lightColor, ambientLight);
		VectorCopy (lightColor, directedLight);
	}

	// save off light value for server to look at (BIG HACK!)
	if (ent->flags & RF_WEAPONMODEL) {
		// pick the greatest component, which should be
		// the same as the mono value returned by software
		if (lightColor[0] > lightColor[1]) {
			if (lightColor[0] > lightColor[2])
				Cvar_VariableForceSetValue (r_lightlevel, 150 * lightColor[0]);
			else
				Cvar_VariableForceSetValue (r_lightlevel, 150 * lightColor[2]);
		} else {
			if (lightColor[1] > lightColor[2])
				Cvar_VariableForceSetValue (r_lightlevel, 150 * lightColor[1]);
			else
				Cvar_VariableForceSetValue (r_lightlevel, 150 * lightColor[2]);
		}

	}

	if (r_fullbright->integer || (ent->flags & RF_FULLBRIGHT)) {
		for (i=0 ; i<numVerts ; i++) {
			colorArray[i][0] = 1;
			colorArray[i][1] = 1;
			colorArray[i][2] = 1;
		}
	} else {
		vec3_t		dir, direction;
		float		dot;

		//
		// reset
		//
		for (i=0 ; i<numVerts ; i++) {
			colorArray[i][0] = 0;
			colorArray[i][1] = 0;
			colorArray[i][2] = 0;
		}

		// only needed to get the location for your own shadow
		if (ent->flags & RF_VIEWERMODEL)
			return;

		//
		// flag effects
		//
		if (hasShell || (ent->flags & RF_MINLIGHT)) {
			for (i=0 ; i<3 ; i++)
				if (ambientLight[i] > 0.1)
					break;

			if (i == 3) {
				ambientLight[0] += 0.1;
				ambientLight[1] += 0.1;
				ambientLight[2] += 0.1;
			}
		}

		if (ent->flags & RF_GLOW) {
			// bonus items will pulse with time
			float	scale;
			float	min;

			scale = 0.1 * sin (r_RefDef.time * 7);
			for (i=0 ; i<3 ; i++) {
				min = ambientLight[i] * 0.8;
				ambientLight[i] += scale;
				if (ambientLight[i] < min)
					ambientLight[i] = min;
			}
		}

		//
		// add ambient lights
		//
		VectorSet (dir, -1, 0, 1);
		Matrix_TransformVector (ent->axis, dir, direction);

		for (i=0 ; i<numVerts; i++) {
			dot = DotProduct (normalsArray[i], direction);
			if (dot <= 0)
				VectorCopy (ambientLight, colorArray[i]);
			else
				VectorMA (ambientLight, dot, directedLight, colorArray[i]);
		}

		//
		// add dynamic lights
		//
		if (gl_dynamic->integer && r_NumDLights) {
			int			lnum;
			dLight_t	*dl;
			float		dist, add, intensity8, intensity;
			vec3_t		dlOrigin;

			for (dl=r_DLights, lnum=0 ; lnum<r_NumDLights ; lnum++, dl++) {
				// translate
				VectorSubtract (dl->origin, ent->origin, dir);
				dist = VectorLength (dir);

				if (dist > dl->intensity + ent->model->radius * ent->scale)
					continue;

				// rotate
				Matrix_TransformVector (ent->axis, dir, dlOrigin);

				intensity = (dl->intensity - dist);
				if (intensity <= 0)
					continue;
				intensity8 = intensity * 8;

				for (i=0 ; i<numVerts ; i++) {
					VectorSubtract (dlOrigin, vertexArray[i], dir);
					add = DotProduct (normalsArray[i], dir);

					// add some ambience
					VectorMA (colorArray[i], intensity * 0.5 * ONEDIV256, dl->color, colorArray[i]);

					// shade the verts
					if (add > 0) {
						dot = DotProduct (dir, dir);
						add *= intensity8 * Q_RSqrt (dot);
						if (add > 255.0)
							add = 255.0 / add;

						VectorMA (colorArray[i], add * ONEDIV256, dl->color, colorArray[i]);
					}
				}
			}
		}
	}

	//
	// clamp
	//
	for (i=0 ; i<numVerts ; i++) {
		colorArray[i][0] = clamp (colorArray[i][0] * ent->color[0], -1.0f, 1.0f);
		colorArray[i][1] = clamp (colorArray[i][1] * ent->color[1], -1.0f, 1.0f);
		colorArray[i][2] = clamp (colorArray[i][2] * ent->color[2], -1.0f, 1.0f);
	}
}


/*
===============
R_LightPoint
===============
*/
void R_LightPoint (vec3_t point, vec3_t light) {
	vec3_t		end;
	vec3_t		dist;
	dLight_t	*dl;
	float		add;
	int			lnum;

	VectorSet (end, point[0], point[1], point[2] - 2048);
	R_RecursiveLightPoint (point, end, light);

	//
	// add dynamic lights
	//
	for (dl=r_DLights, lnum=0 ; lnum<r_NumDLights ; lnum++, dl++) {
		VectorSubtract (point, dl->origin, dist);
		add = (dl->intensity - VectorLength (dist)) * ONEDIV256;

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
void R_SetLightLevel (void) {
	vec3_t		shadelight;

	if (r_RefDef.rdFlags & RDF_NOWORLDMODEL)
		return;

	R_LightPoint (r_RefDef.viewOrigin, shadelight);

	// pick the greatest component, which should be
	// the same as the mono value returned by software
	if (shadelight[0] > shadelight[1]) {
		if (shadelight[0] > shadelight[2])
			Cvar_VariableForceSetValue (r_lightlevel, 150 * shadelight[0]);
		else
			Cvar_VariableForceSetValue (r_lightlevel, 150 * shadelight[2]);
	} else {
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

/*
===============
R_SetCacheState
===============
*/
void R_SetCacheState (mBspSurface_t *surf) {
	int		map;

	for (map=0 ; map<surf->numStyles ; map++)
		surf->cachedLight[map] = r_LightStyles[surf->styles[map]].white;
}


/*
===============
R_BuildLightMap

Combine and scale multiple lightmaps into the floating format in blocklights
===============
*/
void R_BuildLightMap (mBspSurface_t *surf, byte *dest, int stride) {
	int				i, j, size;
	float			*bl, max;
	vec3_t			scale;
	byte			*lightmap;

	if (surf->texInfo->flags & (SURF_SKY|SURF_TRANS33|SURF_TRANS66|SURF_WARP))
		Com_Error (ERR_DROP, "R_BuildLightMap called for non-lit surface");

	size = surf->lmWidth*surf->lmHeight;
	if (size > sizeof (r_BlockLights) >> 4)
		Com_Error (ERR_DROP, "Bad r_BlockLights size");

	// set to full bright if no light data
	if (!surf->lmSamples) {
		for (i=0 ; i<size*3 ; i++)
			r_BlockLights[i] = 255;

		goto store;
	}

	lightmap = surf->lmSamples;

	// add all the lightmaps
	if (surf->numStyles == 1) {
		int		map;

		for (map=0 ; map<surf->numStyles ; map++) {
			bl = r_BlockLights;

			VectorScale (r_LightStyles[surf->styles[map]].rgb, gl_modulate->value, scale);
			if ((scale[0] == 1.0F) && (scale[1] == 1.0F) && (scale[2] == 1.0F)) {
				for (i=0 ; i<size ; i++, bl+=3) {
					bl[0] = lightmap[i*3+0];
					bl[1] = lightmap[i*3+1];
					bl[2] = lightmap[i*3+2];
				}
			} else {
				for (i=0 ; i<size ; i++, bl+=3) {
					bl[0] = lightmap[i*3+0] * scale[0];
					bl[1] = lightmap[i*3+1] * scale[1];
					bl[2] = lightmap[i*3+2] * scale[2];
				}
			}

			// skip to next lightmap
			lightmap += size*3;
		}
	} else {
		int		map;

		memset (r_BlockLights, 0, sizeof (r_BlockLights[0]) * size * 3);
		for (map=0 ; map<surf->numStyles ; map++) {
			bl = r_BlockLights;

			VectorScale (r_LightStyles[surf->styles[map]].rgb, gl_modulate->value, scale);

			if ((scale[0] == 1.0F) && (scale[1] == 1.0F) && (scale[2] == 1.0F)) {
				for (i=0 ; i<size ; i++, bl+=3) {
					bl[0] += lightmap[i*3+0];
					bl[1] += lightmap[i*3+1];
					bl[2] += lightmap[i*3+2];
				}
			} else {
				for (i=0 ; i<size ; i++, bl+=3) {
					bl[0] += lightmap[i*3+0] * scale[0];
					bl[1] += lightmap[i*3+1] * scale[1];
					bl[2] += lightmap[i*3+2] * scale[2];
				}
			}

			// skip to next lightmap
			lightmap += size*3;
		}
	}

	// add all the dynamic lights
	if (surf->dLightFrame == r_FrameCount)
		R_AddDynamicLights (surf);

	// put into texture format
store:
	stride -= (surf->lmWidth<<2);
	bl = r_BlockLights;

	for (i=0 ; i<surf->lmHeight ; i++) {
		for (j=0 ; j<surf->lmWidth ; j++) {
			// catch negative lights
			if (bl[0] < 0)	bl[0] = 0;
			if (bl[1] < 0)	bl[1] = 0;
			if (bl[2] < 0)	bl[2] = 0;

			// determine the brightest of the three color components
			max = bl[0];
			if (bl[1] > max) max = bl[1];
			if (bl[2] > max) max = bl[2];

			/*
			** rescale all the color components if the intensity of the greatest
			** channel exceeds 1.0
			*/
			if (max > 255) {
				max = 255.0F / max;

				dest[0] = bl[0]*max;
				dest[1] = bl[1]*max;
				dest[2] = bl[2]*max;
				dest[3] = 255*max;
			} else {
				dest[0] = bl[0];
				dest[1] = bl[1];
				dest[2] = bl[2];
				dest[3] = 255;
			}

			bl += 3;
			dest += 4;
		}

		dest += stride;
	}
}
