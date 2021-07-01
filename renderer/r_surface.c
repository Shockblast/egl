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

// r_surface.c
// - surface-related refresh code

#include "r_local.h"

mBspSurface_t	*r_AlphaSurfaces;

model_t			*r_WorldModel;
entity_t		r_WorldEntity;

int			r_FrameCount;		// used for dlight push checking
int			r_VisFrameCount;	// bumped when going to a new PVS

int			r_OldViewCluster;
int			r_OldViewCluster2;
int			r_ViewCluster;
int			r_ViewCluster2;

#define	MAX_LIGHTMAPS		128

#define	LIGHTMAP_WIDTH		128
#define	LIGHTMAP_HEIGHT		128
#define LIGHTMAP_BYTES		4

#define GL_LIGHTMAP_FORMAT	GL_RGBA

typedef struct {
	int				currTexNum;

	mBspSurface_t	*lmSurfaces[MAX_LIGHTMAPS];
	image_t			lmTextures[MAX_LIGHTMAPS];

	byte			buffer[LIGHTMAP_WIDTH*LIGHTMAP_HEIGHT*LIGHTMAP_BYTES];

	// the lightmap texture data needs to be kept in
	// main memory so texsubimage can update properly
	int				allocated[LIGHTMAP_WIDTH];
} lightMapState_t;

static lightMapState_t	lms;

static void		LM_InitBlock (void);
static void		LM_UploadBlock (qBool dynamic);
static qBool	LM_AllocBlock (int w, int h, int *x, int *y);

/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
image_t *R_TextureAnimation (mBspTexInfo_t *tex) {
	int		c;

	if (!tex->next)
		return tex->image;

	c = ((int) (r_RefDef.time * 2)) % tex->numFrames;
	while (c) {
		tex = tex->next;
		c--;
	}

	return tex->image;
}

/*
=============================================================

	MULTITEXTURE

=============================================================
*/

void GL_SetMultitextureTexEnv (void) {
	if (!qglMTexCoord2f || (!glConfig.extSGISMultiTexture && !glConfig.extArbMultitexture))
		return;

	if (!r_overbrightbits->integer || !glConfig.extMTexCombine) {
		// TU0
		GL_SelectTexture (TU0);
		GL_TexEnv (GL_MODULATE);

		// TU1
		GL_SelectTexture (TU1);
		if (gl_lightmap->integer)
			GL_TexEnv (GL_REPLACE);
		else 
			GL_TexEnv (GL_MODULATE);
	} else {
		// TU0
		GL_SelectTexture (TU0);
		GL_TexEnv (GL_COMBINE_ARB);
		qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
		qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
		qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
		qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);

		// TU1
		GL_SelectTexture (TU1);
		GL_TexEnv (GL_COMBINE_ARB);
		
		if (gl_lightmap->integer) {
			qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
			qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
		} else {
			qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
			qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PREVIOUS_ARB);
		}

		if (r_overbrightbits->integer)
			qglTexEnvi (GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, r_overbrightbits->integer);
		else
			qglTexEnvi (GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1);
	}
}

/*
=============================================================

	RENDERING

=============================================================
*/

/*
================
R_DrawPoly
================
*/
void R_DrawPoly (image_t *texture, mBspSurface_t *surf, qBool useMTex) {
	mBspPoly_t	*p, *bp;
	float		*v;
	int			i;
	float		s, t;
	float		scroll;

	glSpeeds.worldPolys++;

	if (!useMTex && (texture != NULL))
		GL_Bind (glState.texUnit, texture);

	if (surf->flags & SURF_DRAWTURB) {
		//
		// TURBULANT
		//
		float		dstScroll;
		qBool		useDst;

		useDst = (glConfig.extArbMultitexture && glConfig.extNVTexShader && glConfig.extMTexCombine) ? qTrue : qFalse;

		if (useDst) {
			float		args[4] = {0.05, 0, 0, 0.04};

			GL_Bind (0, r_DSTTexture);
			GL_TexEnv (GL_COMBINE_ARB);
			qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
			qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
			qglTexEnvi (GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_2D);
			
			GL_Bind (1, texture);
			qglEnable (GL_TEXTURE_2D);
			GL_TexEnv (GL_COMBINE_ARB);
			qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
			qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
			qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PREVIOUS_ARB);
			if (r_overbrightbits->integer)
				qglTexEnvi (GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, r_overbrightbits->integer);
			else
				qglTexEnvi (GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1);

			qglTexEnvi (GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_2D);
			qglTexEnvi (GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_OFFSET_TEXTURE_2D_NV);
			qglTexEnvi (GL_TEXTURE_SHADER_NV, GL_PREVIOUS_TEXTURE_INPUT_NV, GL_TEXTURE0_ARB);
			qglTexEnvfv (GL_TEXTURE_SHADER_NV, GL_OFFSET_TEXTURE_MATRIX_NV, &args[0]);
			qglTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

			qglEnable (GL_TEXTURE_SHADER_NV);

			dstScroll = -64 * ((r_RefDef.time * 0.15) - (int)(r_RefDef.time * 0.15));
		}

		if (surf->texInfo->flags & SURF_FLOWING)
			scroll = -64 * ((r_RefDef.time * 0.5) - (int)(r_RefDef.time * 0.5));
		else
			scroll = 0;

		for (bp=surf->polys ; bp ; bp=bp->next) {
			p = bp;

			qglBegin (GL_TRIANGLE_FAN);
			for (i=0, v=p->verts[0] ; i<p->numverts ; i++, v+=VERTEXSIZE) {
				s = v[3] + r_WarpSinTable[((int) ((v[4] * ONEDIV8 + r_RefDef.time) * TURBSCALE)) & 255];
				t = v[4] + r_WarpSinTable[((int) ((v[3] * ONEDIV8 + r_RefDef.time) * TURBSCALE)) & 255];

				s += scroll;
				s *= (1.0 / 64.0);
				t *= (1.0 / 64.0);

				if (useDst) {
					qglMTexCoord2f (GL_TEXTURE0_ARB, (v[3]+dstScroll) * 0.015625, v[4] * 0.015625);
					qglMTexCoord2f (GL_TEXTURE1_ARB, s, t);
				} else
					qglTexCoord2f (s, t);
				qglNormal3fv (surf->plane->normal);
				qglVertex3fv (v);
			}
			qglEnd ();
		}

		if (useDst) {
			qglDisable (GL_TEXTURE_2D);
			qglTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			GL_SelectTexture (TU0);
			qglDisable (GL_TEXTURE_SHADER_NV);
			qglBindTexture (GL_TEXTURE_2D, surf->texInfo->image->texNum);
		}
	} else {
		//
		// REGULAR
		//
		if (surf->texInfo->flags & SURF_FLOWING) {
			scroll = -64 * ((r_RefDef.time/40.0) - (int)(r_RefDef.time/40.0));
			if (scroll == 0.0)
				scroll = -64.0;
		} else
			scroll = 0;

		if (useMTex) {
			// MULTITEXTURE
			for (p=surf->polys ; p ; p=p->chain) {
				qglBegin (GL_POLYGON);
				for (v=p->verts[0], i=0 ; i<surf->polys->numverts ; i++, v+=VERTEXSIZE) {
					GL_MTexCoord2f (TU0, v[3] + scroll, v[4]);
					GL_MTexCoord2f (TU1, v[5], v[6]);
					qglNormal3fv (surf->plane->normal);
					qglVertex3fv (v);
				}
				qglEnd ();
			}
		} else {
			// VERTEX
			qglBegin (GL_POLYGON);
			for (v=surf->polys->verts[0], i=0 ; i<surf->polys->numverts ; i++, v+= VERTEXSIZE) {
				qglTexCoord2f (v[3] + scroll, v[4]);
				qglNormal3fv (surf->plane->normal);
				qglVertex3fv (v);
			}
			qglEnd ();
		}
	}
}


/*
================
R_DrawTriangleOutlines
================
*/
void R_DrawTriangleOutlines (mBspSurface_t *surf) {
	int			i;
	mBspPoly_t	*p;

	if (!gl_showtris->integer)
		return;

	qglDisable (GL_DEPTH_TEST);

	if (!surf) {
		int j;

		qglDisable(GL_TEXTURE_2D);

		for (i=0 ; i<MAX_LIGHTMAPS ; i++) {
			for (surf=lms.lmSurfaces[i] ; surf != 0 ; surf=surf->lightMapChain) {
				for (p=surf->polys ; p ; p=p->chain) {
					for (j=2 ; j<p->numverts ; j++) {
						qglBegin (GL_LINE_STRIP);
						qglVertex3fv (p->verts[0]);
						qglVertex3fv (p->verts[j - 1]);
						qglVertex3fv (p->verts[j]);
						qglVertex3fv (p->verts[0]);
						qglEnd ();
					}
				}
			}
		}

		qglEnable(GL_TEXTURE_2D);
	} else if (qglMTexCoord2f) {
		// let's play toss the TU!
		float	TU0_STATE, TU1_STATE;

		GL_SelectTexture (TU1);
		qglGetTexEnvfv (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &TU1_STATE);
		qglDisable (GL_TEXTURE_2D);

		GL_SelectTexture (TU0);
		qglGetTexEnvfv (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &TU0_STATE);
		qglDisable (GL_TEXTURE_2D);

		for (p=surf->polys; p ; p=p->chain) {
			for (i=2 ; i<p->numverts ; i++) {
				qglBegin (GL_LINE_STRIP);
				qglVertex3fv (p->verts[0]);
				qglVertex3fv (p->verts[i - 1]);
				qglVertex3fv (p->verts[i]);
				qglVertex3fv (p->verts[0]);
				qglEnd ();
			}
		}

		qglEnable (GL_TEXTURE_2D);
		GL_TexEnv (TU0_STATE);

		GL_SelectTexture (TU1);
		GL_TexEnv (TU1_STATE);
		qglEnable (GL_TEXTURE_2D);
	}

	qglEnable (GL_DEPTH_TEST);
}


/*
================
DrawGLPolyChain
================
*/
void DrawGLPolyChain (mBspSurface_t *surf, mBspPoly_t *p, float sOffset, float tOffset) {
	float	*v;
	int		j;

	if ((sOffset == 0) && (tOffset == 0)) {
		for ( ; p!=0 ; p=p->chain) {
			qglBegin (GL_POLYGON);
			v = p->verts[0];
			for (j=0 ; j<p->numverts ; j++, v+= VERTEXSIZE) {
				qglTexCoord2f (v[5], v[6]);
				qglNormal3fv (surf->plane->normal);
				qglVertex3fv (v);
			}
			qglEnd ();
		}
	} else {
		for ( ; p!=0 ; p=p->chain) {
			qglBegin (GL_POLYGON);
			v = p->verts[0];
			for (j=0 ; j<p->numverts ; j++, v+= VERTEXSIZE) {
				qglTexCoord2f (v[5] - sOffset, v[6] - tOffset);
				qglNormal3fv (surf->plane->normal);
				qglVertex3fv (v);
			}
			qglEnd ();
		}
	}
}


/*
================
R_BlendLightMaps

This routine takes all the given light mapped surfaces in the world and blends them
into the framebuffer.
================
*/
void R_BlendLightmaps (entity_t *ent) {
	int				i;
	mBspSurface_t	*surf, *newdrawsurf = 0;

	// don't bother if we're set to fullbright
	if (r_fullbright->integer)
		return;

	if (!r_WorldModel->lightData)
		return;

	// don't bother writing Z
	qglDepthMask (GL_FALSE);

	// set the appropriate blending mode unless we're only looking at the lightmaps
	if (!gl_lightmap->value) {
		qglEnable (GL_BLEND);

		if (gl_saturatelighting->value)
			GL_BlendFunc (GL_ONE, GL_ONE);
		else
			GL_BlendFunc (GL_ZERO, GL_SRC_COLOR);
	}

	if (ent->model == r_WorldModel)
		glSpeeds.visibleLightmaps = 0;

	// render static lightmaps first
	for (i=1 ; i<MAX_LIGHTMAPS ; i++) {
		if (lms.lmSurfaces[i]) {
			if (ent->model == r_WorldModel)
				glSpeeds.visibleLightmaps++;

			GL_Bind (glState.texUnit, &lms.lmTextures[i%MAX_LIGHTMAPS]);
			for (surf=lms.lmSurfaces[i] ; surf != 0 ; surf = surf->lightMapChain) {
				if (surf->polys)
					DrawGLPolyChain (surf, surf->polys, 0, 0);
			}
		}
	}

	// render dynamic lightmaps
	if (gl_dynamic->integer) {
		LM_InitBlock ();

		GL_Bind (glState.texUnit, &lms.lmTextures[0]);
		if (ent->model == r_WorldModel)
			glSpeeds.visibleLightmaps++;

		newdrawsurf = lms.lmSurfaces[0];

		for (surf=lms.lmSurfaces[0] ; surf != 0 ; surf = surf->lightMapChain) {
			byte	*base;

			if (LM_AllocBlock (surf->lmWidth, surf->lmHeight, &surf->dLightCoords[0], &surf->dLightCoords[1])) {
				base = lms.buffer;
				base += (surf->dLightCoords[1] * LIGHTMAP_WIDTH + surf->dLightCoords[0]) * LIGHTMAP_BYTES;

				R_BuildLightMap (surf, base, LIGHTMAP_WIDTH*LIGHTMAP_BYTES);
			} else {
				mBspSurface_t *drawsurf;

				// upload what we have so far
				LM_UploadBlock (qTrue);

				// draw all surfaces that use this lightmap
				for (drawsurf = newdrawsurf ; drawsurf != surf ; drawsurf = drawsurf->lightMapChain) {
					if (drawsurf->polys)
						DrawGLPolyChain (drawsurf, drawsurf->polys, 
										(drawsurf->lmCoords[0] - drawsurf->dLightCoords[0]) * ONEDIV128, 
										(drawsurf->lmCoords[1] - drawsurf->dLightCoords[1]) * ONEDIV128);
				}

				newdrawsurf = drawsurf;

				// clear the block
				LM_InitBlock ();

				// try uploading the block now
				if (!LM_AllocBlock (surf->lmWidth, surf->lmHeight, &surf->dLightCoords[0], &surf->dLightCoords[1]))
					Com_Error (ERR_FATAL, "Consecutive calls to LM_AllocBlock (%d, %d) failed (dynamic)\n", surf->lmWidth, surf->lmHeight);

				base = lms.buffer;
				base += (surf->dLightCoords[1] * LIGHTMAP_WIDTH + surf->dLightCoords[0]) * LIGHTMAP_BYTES;

				R_BuildLightMap (surf, base, LIGHTMAP_WIDTH*LIGHTMAP_BYTES);
			}
		}

		// draw remainder of dynamic lightmaps that haven't been uploaded yet
		if (newdrawsurf)
			LM_UploadBlock (qTrue);

		for (surf=newdrawsurf ; surf != 0 ; surf=surf->lightMapChain) {
			if (surf->polys)
				DrawGLPolyChain (surf, surf->polys,
								(surf->lmCoords[0] - surf->dLightCoords[0]) * ONEDIV128,
								(surf->lmCoords[1] - surf->dLightCoords[1]) * ONEDIV128);
		}
	}

	// restore state
	qglDisable (GL_BLEND);
	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDepthMask (GL_TRUE);
}


/*
================
R_RenderBrushPoly
================
*/
void R_RenderBrushPoly (mBspSurface_t *surf) {
	int			maps;
	image_t		*image;
	qBool		isDynamic = qFalse;

	image = R_TextureAnimation (surf->texInfo);

	if (r_shaders->integer && surf->texInfo->shader)
		RS_DrawPoly (surf);
	else {
		qglColor4f (glState.invIntens, glState.invIntens, glState.invIntens, 1);

		GL_ToggleOverbrights (qTrue);
		R_DrawPoly (image, surf, qFalse);
		GL_ToggleOverbrights (qFalse);

		qglColor4f (1, 1, 1, 1);
	}

	RS_SpecialSurface (surf, qFalse, 0);

	if (surf->flags & SURF_DRAWTURB)
		return;

	// check for lightmap modification
	for (maps=0 ; maps<surf->numStyles ; maps++) {
		if (r_LightStyles[surf->styles[maps]].white != surf->cachedLight[maps])
			goto dynamic;
	}

	// dynamic this frame or dynamic previously
	if (surf->dLightFrame == r_FrameCount) {
dynamic:
		if (gl_dynamic->integer) {
			if (!(surf->texInfo->flags & (SURF_SKY|SURF_TRANS33|SURF_TRANS66|SURF_WARP)))
				isDynamic = qTrue;
		}
	}

	if (isDynamic) {
		if (((surf->styles[maps] >= 32) || (surf->styles[maps] == 0)) && (surf->dLightFrame != r_FrameCount)) {
			unsigned int	temp[LIGHTMAP_WIDTH*LIGHTMAP_HEIGHT];

			R_BuildLightMap (surf, (void *)temp, surf->lmWidth*4);
			R_SetCacheState (surf);

			GL_Bind (glState.texUnit, &lms.lmTextures[surf->lmTexNum%MAX_LIGHTMAPS]);
			qglTexSubImage2D (GL_TEXTURE_2D, 0,
							surf->lmCoords[0], surf->lmCoords[1],
							surf->lmWidth, surf->lmHeight,
							GL_LIGHTMAP_FORMAT,
							GL_UNSIGNED_BYTE,
							temp);

			surf->lightMapChain = lms.lmSurfaces[surf->lmTexNum];
			lms.lmSurfaces[surf->lmTexNum] = surf;
		} else {
			surf->lightMapChain = lms.lmSurfaces[0];
			lms.lmSurfaces[0] = surf;
		}
	} else {
		surf->lightMapChain = lms.lmSurfaces[surf->lmTexNum];
		lms.lmSurfaces[surf->lmTexNum] = surf;
	}
}


/*
================
R_DrawAlphaSurfaces

Draw water surfaces and windows
================
*/
void R_DrawAlphaSurfaces (void) {
	mBspSurface_t	*surf;
	float			alpha;

	qglEnable (GL_BLEND);
	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_ToggleOverbrights (qTrue);

	// the textures are prescaled up for a better lighting range,
	// so scale it back down
	for (surf=r_AlphaSurfaces ; surf ; surf=surf->textureChain) {
		// moving trans brushes - spaz
		// FIXME: BROKEN on garena-b when the fields pull back up; the map or what?
		if (surf->entity) {
			r_CurrEntity = surf->entity;
			R_RotateForEntity (surf->entity);
		}

		if (r_shaders->integer && surf->texInfo->shader)
			RS_DrawPoly (surf);
		else {
			qBool trans33 = (surf->texInfo->flags & SURF_TRANS33) ? qTrue : qFalse;
			qBool trans66 = (surf->texInfo->flags & SURF_TRANS66) ? qTrue : qFalse;

			if (trans33)		alpha = 0.34f;
			else if (trans66)	alpha = 0.67f;
			else				alpha = 1.0f;

			qglColor4f (glState.invIntens, glState.invIntens, glState.invIntens, alpha);
			R_DrawPoly (surf->texInfo->image, surf, qFalse);
		}

		RS_SpecialSurface (surf, qFalse, 0);

		if (surf->entity)
			R_LoadModelIdentity ();
	}

	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_ToggleOverbrights (qFalse);
	qglDisable (GL_BLEND);

	r_AlphaSurfaces = NULL;
}


/*
================
R_DrawTextureChains
================
*/
void R_DrawTextureChains (void) {
	int				i;
	mBspSurface_t	*s;
	image_t			*image;

	if (!glConfig.extSGISMultiTexture && !glConfig.extArbMultitexture) {
		for (i=0, image=img_Textures ; i<img_NumTextures ; i++, image++) {
			if (!image->iRegistrationFrame)
				continue;
			if (!image->textureChain)
				continue;

			glSpeeds.visibleTextures++;

			for (s=image->textureChain ; s ; s=s->textureChain)
				R_RenderBrushPoly (s);

			image->textureChain = NULL;
		}
	} else {
		for (i=0, image=img_Textures ; i<img_NumTextures ; i++, image++) {
			if (!image->iRegistrationFrame)
				continue;
			if (!image->textureChain)
				continue;

			glSpeeds.visibleTextures++;

			for (s=image->textureChain ; s ; s=s->textureChain) {
				if (!(s->flags & SURF_DRAWTURB))
					R_RenderBrushPoly (s);
			}
		}

		GL_ToggleMultitexture (qFalse);
		for (i=0, image=img_Textures ; i<img_NumTextures ; i++, image++) {
			if (!image->iRegistrationFrame)
				continue;
			if (!image->textureChain)
				continue;

			for (s=image->textureChain ; s ; s=s->textureChain) {
				if (s->flags & SURF_DRAWTURB)
					R_RenderBrushPoly (s);
			}

			image->textureChain = NULL;
		}
	}
}


/*
================
R_RenderLightmappedPoly
================
*/
void R_RenderLightmappedPoly (mBspSurface_t *surf) {
	int				map;
	image_t			*image = R_TextureAnimation (surf->texInfo);
	qBool			isDynamic = qFalse;
	int				lmTexNum;

	for (map=0 ; map<surf->numStyles ; map++) {
		if (r_LightStyles[surf->styles[map]].white != surf->cachedLight[map])
			goto dynamic;
	}

	// dynamic this frame or dynamic previously
	if (surf->dLightFrame == r_FrameCount) {
dynamic:
		if (gl_dynamic->integer) {
			if (!(surf->texInfo->flags & (SURF_SKY|SURF_TRANS33|SURF_TRANS66|SURF_WARP)))
				isDynamic = qTrue;
		}
	}

	if (isDynamic) {
		unsigned int	temp[LIGHTMAP_WIDTH*LIGHTMAP_HEIGHT];

		R_BuildLightMap (surf, (void *)temp, surf->lmWidth*4);

		if (((surf->styles[map] >= 32) || (surf->styles[map] == 0)) && (surf->dLightFrame != r_FrameCount)) {
			R_SetCacheState (surf);

			lmTexNum = surf->lmTexNum;
		} else
			lmTexNum = 0;

		GL_Bind (glState.texUnit, &lms.lmTextures[lmTexNum%MAX_LIGHTMAPS]);
		qglTexSubImage2D (GL_TEXTURE_2D, 0,
						  surf->lmCoords[0], surf->lmCoords[1],
						  surf->lmWidth, surf->lmHeight,
						  GL_LIGHTMAP_FORMAT,
						  GL_UNSIGNED_BYTE,
						  temp);
	} else
		lmTexNum = surf->lmTexNum;

	GL_Bind (0, image);
	GL_Bind (1, &lms.lmTextures[lmTexNum%MAX_LIGHTMAPS]);

	if (surf->texInfo->shader && r_shaders->integer)
		RS_DrawLightMapPoly (surf, TEXNUM_LIGHTMAPS + lmTexNum);
	else
		R_DrawPoly (NULL, surf, qTrue);

	RS_SpecialSurface (surf, qTrue, TEXNUM_LIGHTMAPS + lmTexNum);
}


/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel (entity_t *ent) {
	vec3_t			mins, maxs;
	vec3_t			origin;
	cBspPlane_t		*plane;
	mBspSurface_t	*surf;
	dLight_t		*lt;
	qBool			rotated;
	float			dot;
	int				i;

	if (ent->model->numModelSurfaces == 0)
		return;

	if (!Matrix_Compare (ent->axis, axisIdentity)) {
		rotated = qTrue;
		for (i=0 ; i<3 ; i++) {
			mins[i] = ent->origin[i] - ent->model->radius * ent->scale;
			maxs[i] = ent->origin[i] + ent->model->radius * ent->scale;
		}

		if (R_CullSphere (ent->origin, ent->model->radius, 15))
			return;
	} else {
		rotated = qFalse;
		VectorMA (ent->origin, ent->scale, ent->model->mins, mins);
		VectorMA (ent->origin, ent->scale, ent->model->maxs, maxs);

		if (R_CullBox (mins, maxs))
			return;
	}

	glSpeeds.numEntities++;

	memset (lms.lmSurfaces, 0, sizeof (lms.lmSurfaces));

	VectorSubtract (r_RefDef.viewOrigin, ent->origin, origin);
	if (rotated) {
		vec3_t		temp;
		VectorCopy (origin, temp);

		Matrix_TransformVector (ent->axis, temp, origin);
	}

	// calculate dynamic lighting for bmodel
	if ((gl_flashblend->integer == 2) || !gl_flashblend->integer) {
		lt = r_DLights;
		for (i=0 ; i<r_NumDLights ; i++, lt++)
			R_MarkLights (lt, 1<<i, ent->model->nodes + ent->model->firstNode);
	}

	qglColor4f (1, 1, 1, (ent->flags & RF_TRANSLUCENT) ? 0.25 : 1);

	// draw texture
	surf = &ent->model->surfaces[ent->model->firstModelSurface];
	for (i=0 ; i<ent->model->numModelSurfaces ; i++, surf++) {
		// find which side of the node we are on
		plane = surf->plane;
		if (plane->type < 3)
			dot = origin[plane->type] - plane->dist;
		else
			dot = DotProduct (origin, plane->normal) - plane->dist;

		// draw the polygon
		if (((surf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(surf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON))) {
			if (surf->texInfo->flags & (SURF_TRANS33|SURF_TRANS66)) {
				// add to the translucent chain
				surf->textureChain = r_AlphaSurfaces;
				r_AlphaSurfaces = surf;
				surf->entity = ent;
			} else {
				if (qglMTexCoord2f && !(surf->flags & SURF_DRAWTURB)) {
					GL_ToggleMultitexture (qTrue);
					GL_SetMultitextureTexEnv ();
					R_RenderLightmappedPoly (surf);
					GL_ToggleMultitexture (qFalse);
				} else {
					R_RenderBrushPoly (surf);
				}
			}
		}
	}

	if (!(ent->flags & RF_TRANSLUCENT) && !qglMTexCoord2f)
		R_BlendLightmaps (ent);
}

/*
=============================================================

	WORLD MODEL

=============================================================
*/

/*
================
R_RecursiveWorldNode
================
*/
void R_RecursiveWorldNode (mBspNode_t *node)
{
	int				c, side, sidebit;
	cBspPlane_t		*plane;
	mBspSurface_t	*surf, **mark;
	mBspLeaf_t		*leaf;
	float			dot;
	image_t			*image;

	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	if (node->visFrame != r_VisFrameCount)
		return;

	if (R_CullBox (node->mins, node->maxs))
		return;

	// if a leaf node, draw stuff
	if (node->contents != -1) {
		leaf = (mBspLeaf_t *)node;

		// check for door connected areas
		if (r_RefDef.areaBits) {
			if (!(r_RefDef.areaBits[leaf->area>>3] & (1<<(leaf->area&7))))
				return;		// not visible
		}

		mark = leaf->firstMarkSurface;
		c = leaf->numMarkSurfaces;

		if (c) {
			do {
				(*mark)->visFrame = r_FrameCount;
				mark++;
			} while (--c);
		}

		glSpeeds.worldLeafs++;
		return;
	}

	// node is just a decision point, so go down the apropriate sides
	plane = node->plane;

	// find which side of the node we are on
	if (plane->type < 3)
		dot = r_RefDef.viewOrigin[plane->type] - plane->dist;
	else
		dot = DotProduct (r_RefDef.viewOrigin, plane->normal) - plane->dist;

	if (dot >= 0) {
		side = 0;
		sidebit = 0;
	} else {
		side = 1;
		sidebit = SURF_PLANEBACK;
	}

	// recurse down the children, front side first
	R_RecursiveWorldNode (node->children[side]);

	// draw stuff
	for (c=node->numSurfaces, surf=r_WorldModel->surfaces + node->firstSurface; c ; c--, surf++) {
		if (surf->visFrame != r_FrameCount)
			continue;

		if ((surf->flags & SURF_PLANEBACK) != sidebit)
			continue;		// wrong side
		
		if (r_Sky.loaded && (surf->texInfo->flags & SURF_SKY)) {
			// just adds to visible sky bounds
			R_AddSkySurface (surf);
		} else if (surf->texInfo->flags & (SURF_TRANS33|SURF_TRANS66)) {
			// add to the translucent chain
			surf->textureChain = r_AlphaSurfaces;
			r_AlphaSurfaces = surf;
			surf->entity = NULL;
		} else {
			if (qglMTexCoord2f && !(surf->flags & SURF_DRAWTURB))
				R_RenderLightmappedPoly (surf);
			else {
				// the polygon is visible, so add it to the texture sorted chain
				image = R_TextureAnimation (surf->texInfo);
				surf->textureChain = image->textureChain;
				image->textureChain = surf;
			}

			if (qglMTexCoord2f)
				R_DrawTriangleOutlines (surf);
		}
	}

	// recurse down the back side
	R_RecursiveWorldNode (node->children[!side]);
}


/*
=============
R_DrawWorld
=============
*/
void R_DrawWorld (void) {
	if (!r_drawworld->integer)
		return;
	if (!r_WorldModel)
		return;
	if (r_RefDef.rdFlags & RDF_NOWORLDMODEL)
		return;

	// auto cycle the world frame for texture animation
	memset (&r_WorldEntity, 0, sizeof (r_WorldEntity));
	r_WorldEntity.model = r_WorldModel;
	r_CurrModel = r_WorldModel;
	r_CurrEntity = &r_WorldEntity;

	VectorSet (glState.texNums, -1, -1, -1);

	memset (lms.lmSurfaces, 0, sizeof (lms.lmSurfaces));

	R_ClearSky ();

	if (qglMTexCoord2f) {
		GL_ToggleMultitexture (qTrue);

		GL_SetMultitextureTexEnv ();
		R_RecursiveWorldNode (r_WorldModel->nodes);

		GL_ToggleMultitexture (qFalse);
	} else
		R_RecursiveWorldNode (r_WorldModel->nodes);

	R_DrawTextureChains ();
	R_BlendLightmaps (&r_WorldEntity);
	R_DrawSky ();

	if (!qglMTexCoord2f)
		R_DrawTriangleOutlines (NULL);
}


/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current cluster
===============
*/
void R_MarkLeaves (void) {
	byte		*vis, fatvis[BSP_MAX_VIS];
	int			i, c, cluster;
	mBspNode_t	*node;
	mBspLeaf_t	*leaf;

	// current viewcluster
	if (!(r_RefDef.rdFlags & RDF_NOWORLDMODEL)) {
		vec3_t	temp;
		VectorCopy (r_RefDef.viewOrigin, temp);

		r_OldViewCluster = r_ViewCluster;
		r_OldViewCluster2 = r_ViewCluster2;

		leaf = Mod_PointInLeaf (r_RefDef.viewOrigin, r_WorldModel);
		r_ViewCluster = r_ViewCluster2 = leaf->cluster;

		// check above and below so crossing solid water doesn't draw wrong
		if (!leaf->contents) {
			// look down a bit
			temp[2] -= 16;
			leaf = Mod_PointInLeaf (temp, r_WorldModel);
			if (!(leaf->contents & CONTENTS_SOLID) && (leaf->cluster != r_ViewCluster2))
				r_ViewCluster2 = leaf->cluster;
		} else {
			// look up a bit
			temp[2] += 16;
			leaf = Mod_PointInLeaf (temp, r_WorldModel);
			if (!(leaf->contents & CONTENTS_SOLID) && (leaf->cluster != r_ViewCluster2))
				r_ViewCluster2 = leaf->cluster;
		}
	}

	if ((r_OldViewCluster == r_ViewCluster) &&
		(r_OldViewCluster2 == r_ViewCluster2) &&
		(!(r_novis->integer)) && (r_ViewCluster != -1))
		return;

	// development aid to let you run around and see exactly where the pvs ends
	if (gl_lockpvs->integer)
		return;

	r_VisFrameCount++;
	r_OldViewCluster = r_ViewCluster;
	r_OldViewCluster2 = r_ViewCluster2;

	if (r_novis->integer || (r_ViewCluster == -1) || (!(r_WorldModel->vis))) {
		// mark everything
		for (i=0 ; i<r_WorldModel->numLeafs ; i++)
			r_WorldModel->leafs[i].visFrame = r_VisFrameCount;
		for (i=0 ; i<r_WorldModel->numNodes ; i++)
			r_WorldModel->nodes[i].visFrame = r_VisFrameCount;
		return;
	}

	vis = Mod_ClusterPVS (r_ViewCluster, r_WorldModel);
	// may have to combine two clusters because of solid water boundaries
	if (r_ViewCluster2 != r_ViewCluster) {
		memcpy (fatvis, vis, (r_WorldModel->numLeafs+7)/8);
		vis = Mod_ClusterPVS (r_ViewCluster2, r_WorldModel);
		c = (r_WorldModel->numLeafs+31)/32;
		for (i=0 ; i<c ; i++)
			((int *)fatvis)[i] |= ((int *)vis)[i];
		vis = fatvis;
	}

	for (i=0, leaf=r_WorldModel->leafs ; i<r_WorldModel->numLeafs ; i++, leaf++) {
		cluster = leaf->cluster;
		if (cluster == -1)
			continue;

		if (vis[cluster>>3] & (1<<(cluster&7))) {
			node = (mBspNode_t *)leaf;
			do {
				if (node->visFrame == r_VisFrameCount)
					break;

				node->visFrame = r_VisFrameCount;
				node = node->parent;
			} while (node);
		}
	}
}

/*
=============================================================================

	LIGHTMAP ALLOCATION

=============================================================================
*/

/*
================
LM_InitBlock
================
*/
static void LM_InitBlock (void) {
	memset (lms.allocated, 0, sizeof (lms.allocated));
}


/*
================
LM_UploadBlock
================
*/
static void LM_UploadBlock (qBool dynamic) {
	int		lmTexNum;

	if (!dynamic)
		lmTexNum = lms.currTexNum;
	else
		lmTexNum = 0;

	GL_Bind (glState.texUnit, &lms.lmTextures[lmTexNum%MAX_LIGHTMAPS]);

	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	if (dynamic) {
		int		i, height;

		for (height=0, i=0 ; i<LIGHTMAP_WIDTH ; i++) {
			if (lms.allocated[i] > height)
				height = lms.allocated[i];
		}

		qglTexSubImage2D (GL_TEXTURE_2D,
							0,
							0, 0,
							LIGHTMAP_WIDTH, height,
							GL_LIGHTMAP_FORMAT,
							GL_UNSIGNED_BYTE,
							lms.buffer);
	} else {
		qglTexImage2D (GL_TEXTURE_2D,
						0,
						glState.texRGBFormat,
						LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT,
						0,
						GL_LIGHTMAP_FORMAT,
						GL_UNSIGNED_BYTE,
						lms.buffer);

		if (++lms.currTexNum == MAX_LIGHTMAPS)
			Com_Error (ERR_DROP, "LM_UploadBlock: - MAX_LIGHTMAPS exceeded\n");
	}
}


/*
================
LM_AllocBlock

Returns a texture number and the position inside it
================
*/
static qBool LM_AllocBlock (int w, int h, int *x, int *y) {
	int		i, j;
	int		best, best2;

	best = LIGHTMAP_HEIGHT;

	for (i=0 ; i<LIGHTMAP_WIDTH-w ; i++) {
		best2 = 0;

		for (j=0 ; j<w ; j++) {
			if (lms.allocated[i+j] >= best)
				break;

			if (lms.allocated[i+j] > best2)
				best2 = lms.allocated[i+j];
		}

		if (j == w) {
			// this is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if (best + h > LIGHTMAP_HEIGHT)
		return qFalse;

	for (i=0 ; i<w ; i++)
		lms.allocated[*x + i] = best + h;

	return qTrue;
}


/*
================
R_BuildPolygonFromSurface
================
*/
void R_BuildPolygonFromSurface (model_t *model, mBspSurface_t *surf) {
	int			i, lindex, lnumverts, vertpage;
	mBspEdge_t	*pedges, *r_pedge;
	float		*vec, s, t;
	mBspPoly_t	*poly;
	vec3_t		total;

	// reconstruct the polygon
	pedges = model->edges;
	lnumverts = surf->numEdges;
	vertpage = 0;

	VectorIdentity (total);

	// draw texture
	poly = Hunk_Alloc (sizeof (mBspPoly_t) + (lnumverts-4) * VERTEXSIZE * sizeof (float));
	poly->next = surf->polys;
	poly->flags = surf->flags;
	surf->polys = poly;
	poly->numverts = lnumverts;

	for (i=0 ; i<lnumverts ; i++) {
		lindex = model->surfEdges[surf->firstEdge + i];

		if (lindex > 0) {
			r_pedge = &pedges[lindex];
			vec = model->vertexes[r_pedge->v[0]].position;
		} else {
			r_pedge = &pedges[-lindex];
			vec = model->vertexes[r_pedge->v[1]].position;
		}
		s = DotProduct (vec, surf->texInfo->vecs[0]) + surf->texInfo->vecs[0][3];
		s /= surf->texInfo->image->width;

		t = DotProduct (vec, surf->texInfo->vecs[1]) + surf->texInfo->vecs[1][3];
		t /= surf->texInfo->image->height;

		VectorAdd (total, vec, total);
		VectorCopy (vec, poly->verts[i]);
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;

		// lightmap texture coordinates
		s = DotProduct (vec, surf->texInfo->vecs[0]) + surf->texInfo->vecs[0][3];
		s -= surf->textureMins[0];
		s += surf->lmCoords[0] * 16;
		s += 8;
		s /= LIGHTMAP_WIDTH * 16;

		t = DotProduct (vec, surf->texInfo->vecs[1]) + surf->texInfo->vecs[1][3];
		t -= surf->textureMins[1];
		t += surf->lmCoords[1] * 16;
		t += 8;
		t /= LIGHTMAP_HEIGHT * 16;

		poly->verts[i][5] = s;
		poly->verts[i][6] = t;
	}

	poly->numverts = lnumverts;

	VectorScale (total, 1.0f/(float)lnumverts, total);

	surf->c_s = (DotProduct (total, surf->texInfo->vecs[0]) + surf->texInfo->vecs[0][3]) / surf->texInfo->image->width;
	surf->c_t = (DotProduct (total, surf->texInfo->vecs[1]) + surf->texInfo->vecs[1][3]) / surf->texInfo->image->height;
}


/*
========================
LM_CreateSurfaceLightmap
========================
*/
void LM_CreateSurfaceLightmap (mBspSurface_t *surf) {
	byte	*base;

	if (surf->flags & (SURF_DRAWSKY|SURF_DRAWTURB))
		return;

	if (!LM_AllocBlock (surf->lmWidth, surf->lmHeight, &surf->lmCoords[0], &surf->lmCoords[1])) {
		LM_UploadBlock (qFalse);
		LM_InitBlock ();

		if (!LM_AllocBlock (surf->lmWidth, surf->lmHeight, &surf->lmCoords[0], &surf->lmCoords[1]))
			Com_Error (ERR_FATAL, "Consecutive calls to LM_AllocBlock (%d, %d) failed\n", surf->lmWidth, surf->lmHeight);
	}

	surf->lmTexNum = lms.currTexNum;

	base = lms.buffer;
	base += (surf->lmCoords[1] * LIGHTMAP_WIDTH + surf->lmCoords[0]) * LIGHTMAP_BYTES;

	R_SetCacheState (surf);
	R_BuildLightMap (surf, base, LIGHTMAP_WIDTH*LIGHTMAP_BYTES);
}


/*
==================
LM_BeginBuildingLightmaps
==================
*/
void LM_BeginBuildingLightmaps (void) {
	int					i;
	unsigned int		dummy[LIGHTMAP_WIDTH*LIGHTMAP_HEIGHT];

	memset (dummy, 255, sizeof (dummy));
	memset (lms.allocated, 0, sizeof (lms.allocated));

	lms.currTexNum = 1;

	GL_ToggleMultitexture (qTrue);
	GL_SelectTexture (TU1);

	/*
	** setup the base lightstyles so the lightmaps won't have to be regenerated
	** the first time they're seen
	*/

	for (i=0 ; i<MAX_LIGHTSTYLES ; i++) {
		VectorSet (r_LightStyles[i].rgb, 1, 1, 1);
		r_LightStyles[i].white = 3;
	}

	// fill in lightmap image structs
	for (i=0 ; i<MAX_LIGHTMAPS ; i++) {
		lms.lmTextures[i].texNum = TEXNUM_LIGHTMAPS + i;
		lms.lmTextures[i].upFlags = lms.lmTextures[0].flags = IT_LIGHTMAP;
		lms.lmTextures[i].name[0] = lms.lmTextures[i].bareName[0] = 0;		
		lms.lmTextures[i].width = lms.lmTextures[i].upWidth = LIGHTMAP_WIDTH;
		lms.lmTextures[i].height = lms.lmTextures[i].upHeight = LIGHTMAP_HEIGHT;
		lms.lmTextures[i].iRegistrationFrame = 0;
		lms.lmTextures[i].textureChain = NULL;
		lms.lmTextures[i].format = glState.texRGBFormat;
	}

	// initialize the dynamic lightmap texture
	GL_Bind (glState.texUnit, &lms.lmTextures[0]);

	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	qglTexImage2D (GL_TEXTURE_2D, 
					0, 
					glState.texRGBFormat,
					LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT, 
					0, 
					GL_LIGHTMAP_FORMAT, 
					GL_UNSIGNED_BYTE, 
					dummy);
}


/*
=======================
LM_EndBuildingLightmaps
=======================
*/
void LM_EndBuildingLightmaps (void) {
	LM_UploadBlock (qFalse);
	GL_ToggleMultitexture (qFalse);
}
