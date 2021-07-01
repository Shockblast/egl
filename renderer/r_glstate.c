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
// r_glstate.c
// OpenGL state handling and setup
//

#include "r_local.h"

glState_t	glState;

/*
===============
GL_BlendFunc
===============
*/
void GL_BlendFunc (GLenum sFactor, GLenum dFactor)
{
	if (glState.blendSFactor != sFactor || glState.blendDFactor != dFactor) {
		qglBlendFunc (sFactor, dFactor);
		glState.blendSFactor = sFactor;
		glState.blendDFactor = dFactor;
	}

#ifdef _DEBUG
	switch (sFactor) {
    case GL_ZERO:
	case GL_ONE:
	case GL_DST_COLOR:
	case GL_ONE_MINUS_DST_COLOR:
	case GL_SRC_ALPHA:
	case GL_ONE_MINUS_SRC_ALPHA:
	case GL_DST_ALPHA:
	case GL_ONE_MINUS_DST_ALPHA:
	case GL_SRC_ALPHA_SATURATE:
		break;

	default:
		assert (0);
		break;
	}

	switch (dFactor) {
	case GL_ZERO:
	case GL_ONE:
	case GL_SRC_COLOR:
	case GL_ONE_MINUS_SRC_COLOR:
	case GL_SRC_ALPHA:
	case GL_ONE_MINUS_SRC_ALPHA:
	case GL_DST_ALPHA:
	case GL_ONE_MINUS_DST_ALPHA:
		break;

	default:
		assert (0);
		break;
	}
#endif // _DEBUG
}


/*
==================
GL_SetDefaultState

Sets our default OpenGL state
==================
*/
void GL_SetDefaultState (void)
{
	texUnit_t	i;

	qglFinish ();

	qglColor4f (1, 1, 1, 1);
	qglClearColor (0.5f, 0.5f, 0.5f, 1.0f);

	qglEnable (GL_SCISSOR_TEST);

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglPolygonOffset (0, 0);

	// Texture-unit specific
	for (i=MAX_TEXUNITS-1 ; i>=0 ; i--) {
		glState.tex1DBound[i] = 0;
		glState.tex2DBound[i] = 0;
		glState.tex3DBound[i] = 0;
		glState.texCMBound[i] = 0;

		glState.texEnvModes[i] = 0;
		if (i >= glConfig.maxTexUnits)
			continue;

		// Texture
		GL_SelectTexture (i);
		qglDisable (GL_TEXTURE_1D);
		if (glConfig.extTex3D)
			qglDisable (GL_TEXTURE_3D);

		if (i == 0)
			qglEnable (GL_TEXTURE_2D);
		else
			qglDisable (GL_TEXTURE_2D);

		// Cube mapping
		if (glConfig.extTexCubeMap)
			qglDisable (GL_TEXTURE_CUBE_MAP_ARB);

		// Texgen
		qglDisable (GL_TEXTURE_GEN_S);
		qglDisable (GL_TEXTURE_GEN_T);
		qglDisable (GL_TEXTURE_GEN_R);
		qglDisable (GL_TEXTURE_GEN_Q);
	}

	// Fragment programs
	if (glConfig.extFragmentProgram)
		qglDisable (GL_FRAGMENT_PROGRAM_ARB);

	// Vertex programs
	if (glConfig.extVertexProgram)
		qglDisable (GL_VERTEX_PROGRAM_ARB);

	// Stencil testing
	if (glStatic.useStencil)
		qglDisable (GL_STENCIL_TEST);

	// Polygon offset testing
	qglDisable (GL_POLYGON_OFFSET_FILL);

	// Depth testing
	qglDisable (GL_DEPTH_TEST);
	qglDepthFunc (GL_LEQUAL);
	qglDepthRange (0, 1);

	// Face culling
	qglDisable (GL_CULL_FACE);
	qglCullFace (GL_FRONT);

	// Alpha testing
	qglDisable (GL_ALPHA_TEST);
	qglAlphaFunc (GL_GREATER, 0.666f);

	// Blending
	qglDisable (GL_BLEND);
	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glState.blendSFactor = GL_SRC_ALPHA;
	glState.blendDFactor = GL_ONE_MINUS_SRC_ALPHA;

	// Model shading
	qglShadeModel (GL_SMOOTH);

	// Check for errors
	GL_CheckForError ("GL_SetDefaultState");
}
