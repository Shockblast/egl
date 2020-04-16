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
// rb_local.h
//

#include "r_local.h"

#define FTABLE_SIZE		2048
#define FTABLE_CLAMP(x)	(((int)((x)*FTABLE_SIZE) & (FTABLE_SIZE-1)))
#define FTABLE_EVALUATE(table,x) (table[FTABLE_CLAMP(x)])

extern float			rb_sinTable[FTABLE_SIZE];
extern float			rb_triangleTable[FTABLE_SIZE];
extern float			rb_squareTable[FTABLE_SIZE];
extern float			rb_sawtoothTable[FTABLE_SIZE];
extern float			rb_inverseSawtoothTable[FTABLE_SIZE];
extern float			rb_noiseTable[FTABLE_SIZE];

/*
===============================================================================

	OPENGL STATE

===============================================================================
*/

// FIXME: make part of rbData once it's made local to the backend
typedef struct rb_glState_s {
	// Texture state
	texUnit_t			texUnit;
	image_t				*texBound[MAX_TEXUNITS];
	GLenum				texTarget[MAX_TEXUNITS];
	GLfloat				texEnvModes[MAX_TEXUNITS];
	qBool				texMatIdentity[MAX_TEXUNITS];

	// Program state
	GLenum				boundFragProgram;
	GLenum				boundVertProgram;

	// Scene
	qBool				in2D;

	// Generic state
	uint32				stateBits1;
} rb_glState_t;

extern rb_glState_t	rb_glState;

/*
===============================================================================

	FUNCTION PROTOTYPES

===============================================================================
*/

//
// rb_math.c
//

float		*RB_TableForFunc (shTableFunc_t func);

void		Matrix4_Copy2D (const mat4x4_t m1, mat4x4_t m2);
void		Matrix4_Multiply2D (const mat4x4_t m1, const mat4x4_t m2, mat4x4_t out);
void		Matrix4_Scale2D (mat4x4_t m, float x, float y);
void		Matrix4_Stretch2D (mat4x4_t m, float s, float t);
void		Matrix4_Translate2D (mat4x4_t m, float x, float y);

//
// rb_state.c
//

void		RB_StateForBits (uint32 bits1);

void		RB_SelectTexture (texUnit_t texUnit);
void		RB_TextureEnv (GLfloat mode);
void		RB_TextureTarget (GLenum target);
void		RB_LoadTexMatrix (mat4x4_t m);
void		RB_LoadIdentityTexMatrix (void);

void		RB_BindProgram (program_t *program);
