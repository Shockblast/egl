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
// r_glstate.h
//

/*
=============================================================================

	OPENGL STATE MANAGEMENT

=============================================================================
*/

typedef struct glState_s {
	float			realTime;

	float			pow2MapOvrbr;

	// Texture
	float			invIntens;		// inverse intensity

	texUnit_t		texUnit;
	GLuint			tex1DBound[MAX_TEXUNITS];
	GLuint			tex2DBound[MAX_TEXUNITS];
	GLuint			tex3DBound[MAX_TEXUNITS];
	GLuint			texCMBound[MAX_TEXUNITS];

	GLfloat			texEnvModes[MAX_TEXUNITS];
	qBool			texMatIdentity[MAX_TEXUNITS];

	GLint			texMinFilter;
	GLint			texMagFilter;

	// Programs
	GLenum			boundFragProgram;
	GLenum			boundVertProgram;

	// Scene
	qBool			in2D;
	float			cameraSeparation;

	// State management
	GLenum			blendSFactor;
	GLenum			blendDFactor;
} glState_t;

void		GL_BlendFunc (GLenum sFactor, GLenum dFactor);

void		GL_SetDefaultState (void);

extern glState_t	glState;
