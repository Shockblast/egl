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

typedef struct vidDef_s {
	int			width;
	int			height;
} vidDef_t;

extern	vidDef_t	vidDef;

//
// r_cin.c
//

void	R_CinematicPalette (const byte *palette);
void	R_DrawCinematic (int x, int y, int w, int h, int cols, int rows, byte *data);

//
// r_draw.c
//

void	R_DrawPic (struct shader_s *shader, float x, float y, int w, int h, float s1, float t1, float s2, float t2, vec4_t color);
void	R_DrawChar (struct shader_s *charsShader, float x, float y, int flags, float scale, int num, vec4_t color);
int		R_DrawString (struct shader_s *charsShader, float x, float y, int flags, float scale, char *string, vec4_t color);
int		R_DrawStringLen (struct shader_s *charsShader, float x, float y, int flags, float scale, char *string, int len, vec4_t color);
void	R_DrawFill (float x, float y, int w, int h, vec4_t color);

//
// r_fx.c
//

uInt	R_GetClippedFragments (vec3_t origin, float radius, vec3_t axis[3], int maxfverts, vec3_t *fverts, int maxfragments, fragment_t *fragments);

//
// r_image.c
//

void	R_GetImageSize (struct shader_s *shader, int *width, int *height);

//
// r_main.c
//

qBool	R_CullBox (vec3_t mins, vec3_t maxs, int clipFlags);
qBool	R_CullSphere (const vec3_t origin, const float radius, int clipFlags);

qBool	R_CullVisNode (struct mBspNode_s *node);

void	R_RenderScene (refDef_t *rd);
void	R_BeginFrame (float cameraSeparation);
void	R_EndFrame (void);

void	R_ClearScene (void);

void	R_AddEntity (entity_t *ent);
void	R_AddPoly (poly_t *poly);
void	R_AddLight (vec3_t org, float intensity, float r, float g, float b);
void	R_AddLightStyle (int style, float r, float g, float b);

void	GL_RendererMsg_f (void);
void	GL_VersionMsg_f (void);

void	R_LightPoint (vec3_t point, vec3_t light);
void	R_TransformToScreen (vec3_t in, vec2_t out);

void	R_BeginRegistration (void);
void	R_EndRegistration (void);
void	R_MediaInit (void);

qBool	R_Init (void *hInstance, void *hWnd);
void	R_Shutdown (void);

//
// r_model.c
//

void	R_RegisterMap (char *mapName);
struct	model_s *R_RegisterModel (char *name);
void	R_ModelBounds (struct model_s *model, vec3_t mins, vec3_t maxs);

//
// r_shader.c
//

struct shader_s *R_RegisterPic (char *name);
struct shader_s *R_RegisterPoly (char *name);
struct shader_s *R_RegisterSkin (char *name);
struct shader_s *R_RegisterTexture (char *name, int surfParams);

//
// r_sky.c
//

void	R_SetSky (char *name, float rotate, vec3_t axis);

#endif // __REFRESH_H__
