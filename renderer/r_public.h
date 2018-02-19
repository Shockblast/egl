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
// r_public.h
// Client-refresh global header
//

#include "../common/common.h"
#include "../cgame/cg_shared.h"

#ifndef __REFRESH_H__
#define __REFRESH_H__

/*
=============================================================================

	CINEMATIC PLAYBACK

=============================================================================
*/

#define RoQ_HEADER1			4228
#define RoQ_HEADER2			-1
#define RoQ_HEADER3			30

#define RoQ_FRAMERATE		30

#define RoQ_INFO			0x1001
#define RoQ_QUAD_CODEBOOK	0x1002
#define RoQ_QUAD_VQ			0x1011
#define RoQ_SOUND_MONO		0x1020
#define RoQ_SOUND_STEREO	0x1021

#define RoQ_ID_MOT			0x00
#define RoQ_ID_FCC			0x01
#define RoQ_ID_SLD			0x02
#define RoQ_ID_CCC			0x03

typedef struct roqChunk_s {
	uint16				id;
	uint32				size;
	uint16				arg;
} roqChunk_t;

typedef struct roqCell_s {
	byte				y[4], u, v;
} roqCell_t;

typedef struct roqQCell_s {
	byte				idx[4];
} roqQCell_t;

typedef struct cinematic_s {
	int					time;
	int					frameNum;

	fileHandle_t		fileNum;		// File handle to the open cinematic

	byte				*frames[2];

	int					width;			// Width of the cinematic
	int					height;			// Height of the cinematic
	uint32				*vidBuffer;		// Written to for rendering each frame

	// Audio settings
	qBool				sndRestart;

	byte				*sndBuffer;
	int					sndRate;
	int					sndWidth;
	int					sndChannels;

	struct channel_s	*sndRawChannel;
	qBool				sndAL;

	// RoQ information
	roqChunk_t			roqChunk;
	roqCell_t			roqCells[256];
	roqQCell_t			roqQCells[256];

	byte				*roqBuffer;

	// Cinematic information
	uint32				hPalette[256];

	byte				*hBuffer;		// Buffer for decompression
	int					*hNodes;
	int					hNumNodes[256];

	int					hUsed[512];
	int					hCount[512];
} cinematic_t;

/*
=============================================================================

	FUNCTION PROTOTYPES

=============================================================================
*/

//
// rf_2d.c
//

void		R_DrawPic (struct material_s *mat, float matTime, float x, float y, int w, int h, float s1, float t1, float s2, float t2, vec4_t color);
void		R_DrawRectangle (struct material_s *mat, float matTime, vec2_t tl, vec2_t tr, vec2_t br, vec2_t bl, float s1, float t1, float s2, float t2, vec4_t color);

//
// rf_cull.c
//

qBool		R_CullBox (vec3_t mins, vec3_t maxs, int clipFlags);
qBool		R_CullSphere (const vec3_t origin, const float radius, int clipFlags);

//
// rf_decal.c
//

qBool		R_CreateDecal (refDecal_t *d, vec3_t origin, vec3_t direction, float angle, float size);
qBool		R_FreeDecal (refDecal_t *d);

//
// rf_font.c
//

struct font_s *R_RegisterFont (char *name);
void		R_GetFontDimensions (struct font_s *font, float xScale, float yScale, uint32 flags, vec2_t dest);

void		R_DrawChar (struct font_s *font, float x, float y, float xScale, float yScale, uint32 flags, int num, vec4_t color);
size_t		R_DrawString (struct font_s *font, float x, float y, float xScale, float yScale, uint32 flags, char *string, vec4_t color);
size_t		R_DrawStringLen (struct font_s *font, float x, float y, float xScale, float yScale, uint32 flags, char *string, size_t len, vec4_t color);

//
// rf_image.c
//

qBool		R_UpdateTexture (char *name, byte *data, int width, int height);
void		R_GetImageSize (struct material_s *mat, int *width, int *height);

//
// rf_init.c
//

void		R_MediaInit (void);

typedef enum {
	R_INIT_QGL_FAIL,
	R_INIT_OS_FAIL,
	R_INIT_MODE_FAIL,
	R_INIT_SUCCESS
} rInit_t;
rInit_t		R_Init (void);
void		R_Shutdown (qBool full);

//
// rf_light.c
//

void		R_LightPoint (vec3_t point, vec3_t light);

//
// rf_main.c
//

void		R_RenderScene (refDef_t *rd);
void		R_BeginFrame (float cameraSeparation);
void		R_EndFrame (void);

void		R_ClearScene (void);

void		R_AddDecal (refDecal_t *decal, bvec4_t color, struct material_s *material, float materialTime);
void		R_AddEntity (refEntity_t *ent);
void		R_AddPoly (refPoly_t *poly);
void		R_AddLight (vec3_t org, float intensity, float r, float g, float b);
void		R_AddLightStyle (int style, float r, float g, float b);

void		R_GetRefConfig (refConfig_t *outConfig);

void		R_TransformVectorToScreen (refDef_t *rd, vec3_t in, vec2_t out);

void		R_BeginRegistration (void);
void		R_EndRegistration (void);

//
// rf_model.c
//

void		R_RegisterMap (char *mapName);
struct refModel_s *R_RegisterModel (char *name);
void		R_ModelBounds (struct refModel_s *model, vec3_t mins, vec3_t maxs);

//
// rf_material.c
//

struct material_s *R_RegisterPic (char *name);
struct material_s *R_RegisterPoly (char *name);
struct material_s *R_RegisterSkin (char *name);

//
// rf_sky.c
//

void		R_SetSky (char *name, float rotate, vec3_t axis);

#endif // __REFRESH_H__
