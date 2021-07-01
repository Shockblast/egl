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
// refresh.h
// Client-refresh global header
//

#include "../cgame/cgref.h"

#ifndef __REFRESH_H__
#define __REFRESH_H__

extern	vidDef_t	vidDef;

//
// r_cin.c
//

void	R_SetPalette (const byte *palette);
void	R_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data);

//
// r_draw.c
//

void	Draw_Pic (struct image_s *image, float x, float y, int w, int h, float s1, float t1, float s2, float t2, vec4_t color);
void	Draw_StretchPic (char *pic, float x, float y, int w, int h, vec4_t color);
void	Draw_Char (struct image_s *charsImage, float x, float y, int flags, float scale, int num, vec4_t color);
int		Draw_String (struct image_s *charsImage, float x, float y, int flags, float scale, char *string, vec4_t color);
int		Draw_StringLen (struct image_s *charsImage, float x, float y, int flags, float scale, char *string, int len, vec4_t color);
void	Draw_Fill (float x, float y, int w, int h, vec4_t color);

//
// r_fx.c
//

uInt	R_GetClippedFragments (vec3_t origin, float radius, vec3_t axis[3], int maxfverts, vec3_t *fverts, int maxfragments, fragment_t *fragments);

//
// r_image.c
//

struct	image_s *Img_FindImage (char *name, int flags);

struct	image_s *Img_RegisterPic (char *name);
struct	image_s *Img_RegisterSkin (char *name);

void	Img_GetSize (struct image_s *image, int *width, int *height);

//
// r_main.c
//

qBool	R_CullBox (vec3_t mins, vec3_t maxs);
qBool	R_CullSphere (const vec3_t origin, const float radius);

qBool	R_CullVisNode (struct mBspNode_s *node);

void	R_RenderScene (refDef_t *rd);
void	R_BeginFrame (float cameraSeparation);
void	R_EndFrame (void);

void	R_ClearScene (void);

void	R_AddEntity (entity_t *ent);
void	R_AddParticle (vec3_t org, vec3_t angle, dvec4_t color, double size, struct image_s *image, int flags, int sFactor, int dFactor, double orient);
void	R_AddDecal (vec2_t *coords, vec3_t *verts, int numVerts, struct mBspNode_s *node, dvec4_t color, struct image_s *image, int flags, int sFactor, int dFactor);
void	R_AddLight (vec3_t org, float intensity, float r, float g, float b);
void	R_AddLightStyle (int style, float r, float g, float b);

void	GL_RendererMsg_f (void);
void	GL_VersionMsg_f (void);

void	R_LightPoint (vec3_t point, vec3_t light);
void	R_TransformToScreen (vec3_t in, vec2_t out);

void	R_BeginRegistration (void);
void	R_EndRegistration (void);
void	R_InitMedia (void);

qBool	R_Init (void *hInstance, void *hWnd);
void	R_Shutdown (void);

//
// r_model.c
//

void	Mod_RegisterMap (char *mapName);
struct	model_s *Mod_RegisterModel (char *name);
void	Mod_ModelBounds (struct model_s *model, vec3_t mins, vec3_t maxs);

//
// r_sky.c
//

void	R_SetSky (char *name, float rotate, vec3_t axis);

#endif // __REFRESH_H__
