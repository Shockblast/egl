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
// r_lightq3.c
//

#include "r_local.h"

/*
=============================================================================

	DYNAMIC LIGHTS

=============================================================================
*/

#define DLIGHT_SCALE	0.5f

/*
=============
R_Q3BSP_SurfMarkLight
=============
*/
static void R_Q3BSP_SurfMarkLight (uint32 bit, mBspSurface_t *surf)
{
	mQ3BspShaderRef_t	*shaderref;
	shader_t			*shader;

	shaderref = surf->q3_shaderRef;
	if (!shaderref || (shaderref->flags & (SHREF_SKY|SHREF_NODLIGHT)))
		return;

	shader = shaderref->shader;
	if ((shader->flags & (SHADER_SKY|SHADER_FLARE)) || !shader->numPasses)
		return;

	if (surf->dLightFrame != r_refScene.frameCount) {
		surf->dLightBits = 0;
		surf->dLightFrame = r_refScene.frameCount;
	}
	surf->dLightBits |= bit;
}


/*
=================
R_MarkLightWorldNode
=================
*/
static void R_MarkLightWorldNode (dLight_t *light, uint32 bit, mBspNode_t *node)
{
	mBspLeaf_t		*leaf;
	mBspSurface_t	*surf, **mark;
	float			dist, intensity = light->intensity * DLIGHT_SCALE;

	for ( ; ; ) {
		if (node->visFrame != r_refScene.visFrameCount)
			return;
		if (node->plane == NULL)
			break;

		dist = PlaneDiff( light->origin, node->plane);
		if (dist > intensity) {
			node = node->children[0];
			continue;
		}

		if (dist >= -intensity)
			R_MarkLightWorldNode (light, bit, node->children[0]);
		node = node->children[1];
	}

	leaf = (mBspLeaf_t *)node;

	// Check for door connected areas
	if (r_refDef.areaBits) {
		if (!(r_refDef.areaBits[leaf->area>>3] & (1<<(leaf->area&7))))
			return;		// Not visible
	}
	if (!leaf->q3_firstLitSurface)
		return;
	if (!BoundsAndSphereIntersect (leaf->mins, leaf->maxs, light->origin, intensity))
		return;

	mark = leaf->q3_firstLitSurface;
	do {
		surf = *mark++;
		if (!BoundsAndSphereIntersect (surf->mins, surf->maxs, light->origin, intensity))
			continue;

		R_Q3BSP_SurfMarkLight (bit, surf);
	} while (*mark);
}


/*
=================
R_Q3BSP_MarkLights
=================
*/
void R_Q3BSP_MarkLights (void)
{
	int			k;
	dLight_t	*lt;

	if (gl_flashblend->integer || !gl_dynamic->integer || !r_refScene.numDLights || r_vertexLighting->integer || r_fullbright->value)
		return;

	lt = r_refScene.dLightList;
	for (k=0 ; k<r_refScene.numDLights ; k++, lt++)
		R_MarkLightWorldNode (lt, 1<<k, r_refScene.worldModel->bspModel.nodes);
}


/*
=================
R_Q3BSP_MarkLightsBModel
=================
*/
void R_Q3BSP_MarkLightsBModel (entity_t *ent, vec3_t mins, vec3_t maxs)
{
	int				k, c, bit;
	dLight_t		*lt;
	model_t			*model = ent->model;
	mBspSurface_t	*psurf;

	if (!gl_dynamic->integer || !r_refScene.numDLights || r_fullbright->integer)
		return;

	for (k=0, bit=1, lt=r_refScene.dLightList ; k<r_refScene.numDLights ; k++, bit<<=1, lt++) {
		if (!BoundsIntersect (mins, maxs, lt->mins, lt->maxs))
			continue;

		for (c=0, psurf=model->bspModel.firstModelSurface ; c<model->bspModel.numModelSurfaces ; c++, psurf++) {
			if (R_Q3BSP_SurfPotentiallyLit (psurf)) {
				if (psurf->dLightFrame != r_refScene.frameCount) {
					psurf->dLightBits = 0;
					psurf->dLightFrame = r_refScene.frameCount;
				}
				psurf->dLightBits |= bit;
			}
		}
	}
}


/*
=================
R_Q3BSP_AddDynamicLights
=================
*/
void R_Q3BSP_AddDynamicLights (uint32 dLightBits, entity_t *ent, GLenum depthFunc, GLenum sFactor, GLenum dFactor)
{
	int			num, j;
	dLight_t	*light;
	byte		*outColor;
	float		dist;
	vec3_t		tempVec, lightOrigin;
	float		scale;
	GLfloat		s[4], t[4], r[4];

	if (!r_refScene.numDLights)
		return;

	// OpenGL state
	qglDepthFunc (depthFunc);
	GL_BlendFunc (sFactor, dFactor);
	qglEnable (GL_BLEND);

	// Reset
	GL_LoadIdentityTexMatrix ();
	qglMatrixMode (GL_MODELVIEW);

	// Texture state
	GL_SelectTexture (0);
	if (glConfig.extTex3D) {
		qglDisable (GL_TEXTURE_2D);
		qglEnable (GL_TEXTURE_3D);
		GL_BindTexture (r_dLightTexture);
		GL_TextureEnv (GL_MODULATE);

		qglEnable (GL_TEXTURE_GEN_S);
		qglTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		s[1] = s[2] = 0;

		qglEnable (GL_TEXTURE_GEN_T);
		qglTexGeni (GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		t[0] = t[2] = 0;

		qglEnable (GL_TEXTURE_GEN_R);
		qglTexGeni (GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		r[0] = r[1] = 0;
	}
	else {
		qglEnable (GL_TEXTURE_2D);
		GL_BindTexture (r_dLightTexture);
		GL_TextureEnv (GL_MODULATE);

		qglEnable (GL_TEXTURE_GEN_S);
		qglTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		s[1] = s[2] = 0;

		qglEnable (GL_TEXTURE_GEN_T);
		qglTexGeni (GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		t[0] = t[2] = 0;

		qglEnableClientState (GL_COLOR_ARRAY);
		qglColorPointer (4, GL_UNSIGNED_BYTE, 0, rb_colorArray);
	}

	for (num=0, light=r_refScene.dLightList ; num<r_refScene.numDLights ; num++, light++) {
		if (!(dLightBits & (1<<num)))
			continue;	// Not lit by this light

		// Transform
		VectorSubtract (light->origin, ent->origin, lightOrigin);
		if (!Matrix3_Compare (ent->axis, axisIdentity)) {
			VectorCopy (lightOrigin, tempVec);
			Matrix3_TransformVector (ent->axis, tempVec, lightOrigin);
		}

		scale = 1.0f / light->intensity;

		// Calculate coordinates
		s[0] = scale;
		s[3] = (-lightOrigin[0] * scale) + 0.5f;
		qglTexGenfv (GL_S, GL_OBJECT_PLANE, s);

		t[1] = scale;
		t[3] = (-lightOrigin[1] * scale) + 0.5f;
		qglTexGenfv (GL_T, GL_OBJECT_PLANE, t);

		if (glConfig.extTex3D) {
			// Color
			qglColor3fv (light->color);

			// Depth coordinate
			r[2] = scale;
			r[3] = (-lightOrigin[2] * scale) + 0.5f;
			qglTexGenfv (GL_R, GL_OBJECT_PLANE, r);
		}
		else {
			// Color
			outColor = rb_colorArray[0];
			for (j=0 ; j<rb_numVerts ; j++) {
				dist = (rb_inVertexArray[j][2] - lightOrigin[2]) * 2;
				if (dist < 0)
					dist = -dist;

				if (dist < light->intensity) {
					outColor[0] = R_FloatToByte (light->color[0]);
					outColor[1] = R_FloatToByte (light->color[1]);
					outColor[2] = R_FloatToByte (light->color[2]);
				}
				else {
					dist = Q_RSqrt (dist * dist - light->intensity * light->intensity);
					dist = clamp (dist, 0, 1);

					outColor[0] = R_FloatToByte (dist * light->color[0]);
					outColor[1] = R_FloatToByte (dist * light->color[1]);
					outColor[2] = R_FloatToByte (dist * light->color[2]);
				}
				outColor[3] = 255;

				// Next
				outColor += 4;
			}
		}

		// Render
		RB_DrawElements (MBT_Q3BSP);
	}

	// OpenGL state
	qglDisable (GL_BLEND);
	qglDepthFunc (GL_LEQUAL);

	// Texture state
	if (glConfig.extTex3D) {
		qglDisable (GL_TEXTURE_GEN_S);
		qglDisable (GL_TEXTURE_GEN_T);
		qglDisable (GL_TEXTURE_GEN_R);
		qglDisable (GL_TEXTURE_3D);

		qglEnable (GL_TEXTURE_2D);
	}
	else {
		qglDisable (GL_TEXTURE_GEN_S);
		qglDisable (GL_TEXTURE_GEN_T);

		qglDisableClientState (GL_COLOR_ARRAY);
	}
}

/*
=============================================================================

	LIGHTING

=============================================================================
*/

/*
===============
R_Q3BSP_LightForEntity
===============
*/
void R_Q3BSP_LightForEntity (entity_t *ent, int numVerts, byte *bArray)
{
	static vec3_t	tempColorsArray[VERTARRAY_MAX_VERTS];
	vec3_t			vf, vf2;
	float			*cArray;
	float			t[8], direction_uv[2], dot;
	int				r, g, b, vi[3], i, j, index[4];
	vec3_t			dlorigin, ambient, diffuse, dir, direction;

	if (r_fullbright->value || ent->flags & RF_FULLBRIGHT) {
		for (i=0 ; i<numVerts ; i++) {
			tempColorsArray[i][0] = 1;
			tempColorsArray[i][1] = 1;
			tempColorsArray[i][2] = 1;
		}
	}
	else {
		// Probably a weird shader, see mpteam4 for example
		if (!ent->model || ent->model->type == MODEL_Q3BSP) {
			memset (bArray, 0, sizeof (bvec4_t)*numVerts);
			return;
		}

		VectorSet (ambient, 0, 0, 0);
		VectorSet (diffuse, 0, 0, 0);
		VectorSet (direction, 1, 1, 1);

		if (!r_refScene.worldModel->q3BspModel.lightGrid || !r_refScene.worldModel->q3BspModel.numLightGridElems)
			goto dynamic;

		for (i=0 ; i<3 ; i++) {
			vf[i] = (ent->origin[i] - r_refScene.worldModel->q3BspModel.gridMins[i]) / r_refScene.worldModel->q3BspModel.gridSize[i];
			vi[i] = (int)vf[i];
			vf[i] = vf[i] - floor(vf[i]);
			vf2[i] = 1.0f - vf[i];
		}

		index[0] = vi[2]*r_refScene.worldModel->q3BspModel.gridBounds[3] + vi[1]*r_refScene.worldModel->q3BspModel.gridBounds[0] + vi[0];
		index[1] = index[0] + r_refScene.worldModel->q3BspModel.gridBounds[0];
		index[2] = index[0] + r_refScene.worldModel->q3BspModel.gridBounds[3];
		index[3] = index[2] + r_refScene.worldModel->q3BspModel.gridBounds[0];

		for (i=0 ; i<4 ; i++) {
			if (index[i] < 0 || index[i] >= r_refScene.worldModel->q3BspModel.numLightGridElems-1)
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
				ambient[j] += t[i*2] * r_refScene.worldModel->q3BspModel.lightGrid[index[i]].ambient[j];
				ambient[j] += t[i*2+1] * r_refScene.worldModel->q3BspModel.lightGrid[index[i]+1].ambient[j];

				diffuse[j] += t[i*2] * r_refScene.worldModel->q3BspModel.lightGrid[index[i]].diffuse[j];
				diffuse[j] += t[i*2+1] * r_refScene.worldModel->q3BspModel.lightGrid[index[i]+1].diffuse[j];
			}
		}

		for (j=0 ; j<2 ; j++) {
			direction_uv[j] = 0;

			for (i=0 ; i<4 ; i++) {
				direction_uv[j] += t[i*2] * r_refScene.worldModel->q3BspModel.lightGrid[index[i]].direction[j];
				direction_uv[j] += t[i*2+1] * r_refScene.worldModel->q3BspModel.lightGrid[index[i]+1].direction[j];
			}

			direction_uv[j] = AngleModf (direction_uv[j]);
		}

		dot = bound(0.0f, /*r_ambientscale->value*/ 1.0f, 1.0f) * glState.pow2MapOvrbr;
		VectorScale (ambient, dot, ambient);

		dot = bound(0.0f, /*r_directedscale->value*/ 1.0f, 1.0f) * glState.pow2MapOvrbr;
		VectorScale (diffuse, dot, diffuse);

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
		t[0] = R_FastSin (dot + 0.25f);
		t[1] = R_FastSin (dot);

		dot = direction_uv[1] * (1.0 / 255.0);
		t[2] = R_FastSin (dot + 0.25f);
		t[3] = R_FastSin (dot);

		VectorSet (dir, t[2] * t[1], t[3] * t[1], t[0]);

		// rotate direction
		Matrix3_TransformVector (ent->axis, dir, direction);

		cArray = tempColorsArray[0];
		for (i=0 ; i<numVerts ; i++, cArray+=3) {
			dot = DotProduct (rb_normalsArray[i], direction);
			
			if (dot <= 0)
				VectorCopy (ambient, cArray);
			else
				VectorMA (ambient, dot, diffuse, cArray);
		}

dynamic:
		//
		// add dynamic lights
		//
		if (gl_dynamic->value && r_refScene.numDLights) {
			int lnum;
			dLight_t *dl;
			float dist, add, intensity8;

			dl = r_refScene.dLightList;
			for (lnum=0 ; lnum<r_refScene.numDLights ; lnum++, dl++) {
				// translate
				VectorSubtract ( dl->origin, ent->origin, dir );
				dist = VectorLength ( dir );

				if (dist > dl->intensity + ent->model->radius * ent->scale)
					continue;

				// rotate
				Matrix3_TransformVector ( ent->axis, dir, dlorigin );

				intensity8 = dl->intensity * 8;

				cArray = tempColorsArray[0];
				for (i=0 ; i<numVerts ; i++, cArray+=3) {
					VectorSubtract (dlorigin, rb_vertexArray[i], dir);
					add = DotProduct (rb_normalsArray[i], dir);

					if (add > 0) {
						dot = DotProduct (dir, dir);
						add *= (intensity8 / dot) * Q_RSqrt (dot);
						VectorMA (cArray, add, dl->color, cArray);
					}
				}
			}
		}
	}

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
=============================================================================

	QUAKE3 LIGHTMAP

=============================================================================
*/

static byte		*r_q3lmBuffer;
static int		r_q3lmBufferSize;
static int		r_q3lmNumUploaded;
static int		r_q3lmMaxBlockSize;

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
	float	rgb[3], scaled[3];

	if (!data || r_fullbright->integer) {
		for (y=0 ; y<h ; y++, dest)
			memset (dest + y * blockWidth, 255, w * 4);
		return;
	}

	scale = glState.pow2MapOvrbr;
	for (y=0 ; y<h ; y++) {
		for (x=0, rgba=dest+y*blockWidth ; x<w ; x++, rgba+=4) {
			scaled[0] = data[(y*w+x) * Q3LIGHTMAP_BYTES+0] * scale;
			scaled[1] = data[(y*w+x) * Q3LIGHTMAP_BYTES+1] * scale;
			scaled[2] = data[(y*w+x) * Q3LIGHTMAP_BYTES+2] * scale;

			R_ColorNormalize (scaled, rgb);

			rgba[0] = (byte)(rgb[0]*255);
			rgba[1] = (byte)(rgb[1]*255);
			rgba[2] = (byte)(rgb[2]*255);
		}
	}
}


/*
=======================
R_PackLightmaps
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

	maxX = r_q3lmMaxBlockSize / w;
	maxY = r_q3lmMaxBlockSize / h;
	max = maxY;
	if (maxY > maxX)
		max = maxX;

	if (r_q3lmNumUploaded >= R_MAX_LIGHTMAPS-1)
		Com_Error (ERR_DROP, "R_Q3BSP_PackLightmaps: - R_MAX_LIGHTMAPS exceeded\n");

	Com_DevPrintf (0, "Packing %i lightmap(s) -> ", num);

	if (!max || num == 1 || !r_lmPacking->integer) {
		// Process as it is
		R_Q3BSP_BuildLightmap (w, h, data, r_q3lmBuffer, w * 4);

		image = R_Load2DImage (Q_VarArgs ("*lm%i", r_q3lmNumUploaded), (byte **)(&r_q3lmBuffer),
			w, h, IF_CLAMP|IF_NOPICMIP|IF_NOMIPMAP_LINEAR|IF_NOGAMMA|IF_NOINTENS|IF_NOCOMPRESS|IT_LIGHTMAP, Q3LIGHTMAP_BYTES);

		r_lmTextures[r_q3lmNumUploaded] = image;
		rects[0].texNum = r_q3lmNumUploaded;

		rects[0].w = 1; rects[0].x = 0;
		rects[0].h = 1; rects[0].y = 0;

		Com_DevPrintf (0, "%ix%i\n", 1, 1);

		r_q3lmNumUploaded++;
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
	if (rectSize > r_q3lmBufferSize) {
		if (r_q3lmBuffer)
			Mem_Free (r_q3lmBuffer);
		r_q3lmBuffer = Mem_AllocExt (rectSize, qFalse);
		memset (r_q3lmBuffer, 255, rectSize);
		r_q3lmBufferSize = rectSize;
	}

	block = r_q3lmBuffer;
	for (y=0, ty=0.0f, num=0 ; y<rectY ; y++, ty+=th, block+=rectX*xStride*h) {
		for (x=0, tx=0.0f ; x<rectX ; x++, tx+=tw, num++) {
			R_Q3BSP_BuildLightmap (w, h, data + num * size, block + x * xStride, rectX * xStride);

			rects[num].w = tw; rects[num].x = tx;
			rects[num].h = th; rects[num].y = ty;
		}
	}

	image = R_Load2DImage (Q_VarArgs ("*lm%i", r_q3lmNumUploaded), (byte **)(&r_q3lmBuffer),
		rectX * w, rectY * h, IF_CLAMP|IF_NOPICMIP|IF_NOMIPMAP_LINEAR|IF_NOGAMMA|IF_NOINTENS|IF_NOCOMPRESS|IT_LIGHTMAP, Q3LIGHTMAP_BYTES);

	r_lmTextures[r_q3lmNumUploaded] = image;
	for (i=0 ; i<num ; i++)
		rects[i].texNum = r_q3lmNumUploaded;

	Com_DevPrintf (0, "%ix%i\n", rectX, rectY);

	r_q3lmNumUploaded++;
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
	for (size=1 ; size<r_lmMaxBlockSize->integer && size<glConfig.maxTexSize ; size<<=1);

	r_q3lmNumUploaded = 0;
	r_q3lmMaxBlockSize = size;
	size = w * h * Q3LIGHTMAP_BYTES;
	r_q3lmBufferSize = w * h * 4;
	r_q3lmBuffer = Mem_AllocExt (r_q3lmBufferSize, qFalse);

	for (i=0 ; i<numLightmaps ; )
		i += R_Q3BSP_PackLightmaps (numLightmaps - i, w, h, size, data + i * size, &rects[i]);

	if (r_q3lmBuffer)
		Mem_Free (r_q3lmBuffer);

	Com_DevPrintf (0, "Packed %i lightmaps into %i texture(s)\n", numLightmaps, r_q3lmNumUploaded);
}


/*
=======================
R_Q3BSP_TouchLightmaps
=======================	
*/
void R_Q3BSP_TouchLightmaps (void)
{
	int		i;

	for (i=0 ; i<r_q3lmNumUploaded ; i++)
		R_TouchImage (r_lmTextures[i]);
}
