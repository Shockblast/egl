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

/*
===============
GL_Enable
===============
*/
void GL_Enable (GLenum cap)
{
	switch (cap) {
	case GL_ALPHA_TEST:
		if (glState.alphaTestOn)
			return;
		glState.alphaTestOn = qTrue;
		break;

	case GL_BLEND:
		if (glState.blendingOn)
			return;
		glState.blendingOn = qTrue;
		break;

	case GL_CULL_FACE:
		if (glState.cullingOn)
			return;
		glState.cullingOn = qTrue;
		break;

	case GL_DEPTH_TEST:
		if (glState.depthTestOn)
			return;
		glState.depthTestOn = qTrue;
		break;

	case GL_POLYGON_OFFSET_FILL:
		if (glState.polyOffsetOn)
			return;
		glState.polyOffsetOn = qTrue;
		break;

	case GL_STENCIL_TEST:
		if (glState.stencilTestOn)
			return;
		if (!glConfig.useStencil)
			return;
		glState.stencilTestOn = qTrue;
		break;

	case GL_TEXTURE_1D:
		if (glState.tex1DOn[glState.texUnit])
			return;
		glState.tex1DOn[glState.texUnit] = qTrue;
		break;

	case GL_TEXTURE_2D:
		if (glState.tex2DOn[glState.texUnit])
			return;
		glState.tex2DOn[glState.texUnit] = qTrue;
		break;

	case GL_TEXTURE_3D:
		if (glState.tex3DOn[glState.texUnit])
			return;
		if (!glConfig.extTex3D)
			return;
		glState.tex3DOn[glState.texUnit] = qTrue;
		break;

	case GL_TEXTURE_CUBE_MAP_ARB:
		if (glState.texCubeMapOn[glState.texUnit])
			return;
		if (!glConfig.extArbTexCubeMap)
			return;
		glState.texCubeMapOn[glState.texUnit] = qTrue;
		break;

	case GL_TEXTURE_GEN_S:
		if (glState.texGenSOn[glState.texUnit])
			return;
		glState.texGenSOn[glState.texUnit] = qTrue;
		break;

	case GL_TEXTURE_GEN_T:
		if (glState.texGenTOn[glState.texUnit])
			return;
		glState.texGenTOn[glState.texUnit] = qTrue;
		break;

	case GL_TEXTURE_GEN_R:
		if (glState.texGenROn[glState.texUnit])
			return;
		glState.texGenROn[glState.texUnit] = qTrue;
		break;

	case GL_TEXTURE_GEN_Q:
		if (glState.texGenQOn[glState.texUnit])
			return;
		glState.texGenQOn[glState.texUnit] = qTrue;
		break;

	case GL_FRAGMENT_PROGRAM_ARB:
		if (glState.fragProgOn)
			return;
		if (!glConfig.extFragmentProgram)
			return;
		glState.fragProgOn = qTrue;
		break;

	case GL_VERTEX_PROGRAM_ARB:
		if (glState.vertProgOn)
			return;
		if (!glConfig.extVertexProgram)
			return;
		glState.vertProgOn = qTrue;
		break;
	}

	qglEnable (cap);
}


/*
===============
GL_Disable
===============
*/
void GL_Disable (GLenum cap)
{
	switch (cap) {
	case GL_ALPHA_TEST:
		if (!glState.alphaTestOn)
			return;
		glState.alphaTestOn = qFalse;
		break;

	case GL_BLEND:
		if (!glState.blendingOn)
			return;
		glState.blendingOn = qFalse;
		break;

	case GL_CULL_FACE:
		if (!glState.cullingOn)
			return;
		glState.cullingOn = qFalse;
		break;

	case GL_DEPTH_TEST:
		if (!glState.depthTestOn)
			return;
		glState.depthTestOn = qFalse;
		break;

	case GL_POLYGON_OFFSET_FILL:
		if (!glState.polyOffsetOn)
			return;
		glState.polyOffsetOn = qFalse;
		break;

	case GL_STENCIL_TEST:
		if (!glState.stencilTestOn)
			return;
		if (!glConfig.useStencil)
			return;
		glState.stencilTestOn = qFalse;
		break;

	case GL_TEXTURE_1D:
		if (!glState.tex1DOn[glState.texUnit])
			return;
		glState.tex1DOn[glState.texUnit] = qFalse;
		break;

	case GL_TEXTURE_2D:
		if (!glState.tex2DOn[glState.texUnit])
			return;
		glState.tex2DOn[glState.texUnit] = qFalse;
		break;

	case GL_TEXTURE_3D:
		if (!glState.tex3DOn[glState.texUnit])
			return;
		if (!glConfig.extTex3D)
			return;
		glState.tex3DOn[glState.texUnit] = qFalse;
		break;

	case GL_TEXTURE_CUBE_MAP_ARB:
		if (!glState.texCubeMapOn[glState.texUnit])
			return;
		if (!glConfig.extArbTexCubeMap)
			return;
		glState.texCubeMapOn[glState.texUnit] = qFalse;
		break;

	case GL_TEXTURE_GEN_S:
		if (!glState.texGenSOn[glState.texUnit])
			return;
		glState.texGenSOn[glState.texUnit] = qFalse;
		break;

	case GL_TEXTURE_GEN_T:
		if (!glState.texGenTOn[glState.texUnit])
			return;
		glState.texGenTOn[glState.texUnit] = qFalse;
		break;

	case GL_TEXTURE_GEN_R:
		if (!glState.texGenROn[glState.texUnit])
			return;
		glState.texGenROn[glState.texUnit] = qFalse;
		break;

	case GL_TEXTURE_GEN_Q:
		if (!glState.texGenQOn[glState.texUnit])
			return;
		glState.texGenQOn[glState.texUnit] = qFalse;
		break;

	case GL_FRAGMENT_PROGRAM_ARB:
		if (!glState.fragProgOn)
			return;
		if (!glConfig.extFragmentProgram)
			return;
		glState.fragProgOn = qFalse;
		break;

	case GL_VERTEX_PROGRAM_ARB:
		if (!glState.vertProgOn)
			return;
		if (!glConfig.extVertexProgram)
			return;
		glState.vertProgOn = qFalse;
		break;
	}

	qglDisable (cap);
}


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
}


/*
===============
GL_CullFace
===============
*/
void GL_CullFace (GLenum mode)
{
	if (glState.cullFace != mode) {
		qglCullFace (mode);
		glState.cullFace = mode;
	}
}


/*
===============
GL_DepthFunc
===============
*/
void GL_DepthFunc (GLenum func)
{
	if (glState.depthFunc != func) {
		qglDepthFunc (func);
		glState.depthFunc = func;
	}
}


/*
==================
GL_SetDefaultState

Sets our default OpenGL state
==================
*/
void GL_SetDefaultState (void)
{
	int		i;

	qglFinish ();

	qglColor4f (1, 1, 1, 1);
	qglClearColor (0.5f, 0.5f, 0.5f, 1.0f);

	qglEnable (GL_SCISSOR_TEST);

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglPolygonOffset (0, 0);

	qglShadeModel (GL_SMOOTH);

	// Texture-unit specific
	for (i=MAX_TEXUNITS-1 ; i>=0 ; i--) {
		glState.tex1DOn[i] = qFalse;
		glState.tex3DOn[i] = qFalse;
		glState.texCubeMapOn[i] = qFalse;
		glState.texEnvModes[i] = 0;
		glState.texGenSOn[i] = qFalse;
		glState.texGenTOn[i] = qFalse;
		glState.texGenROn[i] = qFalse;
		glState.texGenQOn[i] = qFalse;
		glState.texNumsBound[i] = 0;
		if (i >= glConfig.maxTexUnits) {
			glState.tex2DOn[i] = qFalse;
			continue;
		}

		// Texture
		GL_SelectTexture (i);
		qglDisable (GL_TEXTURE_1D);
		if (glConfig.extTex3D)
			qglDisable (GL_TEXTURE_3D);

		if (i == 0) {
			qglEnable (GL_TEXTURE_2D);
			glState.tex2DOn[i] = qTrue;
		}
		else {
			qglDisable (GL_TEXTURE_2D);
			glState.tex2DOn[i] = qFalse;
		}

		// Cube mapping
		if (glConfig.extArbTexCubeMap)
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
	glState.fragProgOn = qFalse;

	// Vertex programs
	if (glConfig.extVertexProgram)
		qglDisable (GL_VERTEX_PROGRAM_ARB);
	glState.vertProgOn = qFalse;

	// Stencil testing
	qglDisable (GL_STENCIL_TEST);
	glState.stencilTestOn = qFalse;

	// Polygon offset testing
	qglDisable (GL_POLYGON_OFFSET_FILL);
	glState.polyOffsetOn = qFalse;

	// Depth testing
	qglDisable (GL_DEPTH_TEST);
	qglDepthFunc (GL_LEQUAL);
	qglDepthRange (0, 1);
	glState.depthTestOn = qFalse;
	glState.depthFunc = GL_LEQUAL;

	// Face culling
	qglDisable (GL_CULL_FACE);
	qglCullFace (GL_FRONT);
	glState.cullingOn = qFalse;
	glState.cullFace = GL_FRONT;

	// Alpha testing
	qglDisable (GL_ALPHA_TEST);
	qglAlphaFunc (GL_GREATER, 0.666f);
	glState.alphaTestOn = qFalse;

	// Blending
	qglDisable (GL_BLEND);
	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glState.blendingOn = qFalse;
	glState.blendSFactor = GL_SRC_ALPHA;
	glState.blendDFactor = GL_ONE_MINUS_SRC_ALPHA;
}


/*
==================
GL_Setup2D
==================
*/
void GL_Setup2D (void)
{
	glState.in2D = qTrue;

	// Set 2D virtual screen size
	qglViewport (0, 0, vidDef.width, vidDef.height);
	qglScissor (0, 0, vidDef.width, vidDef.height);

	qglMatrixMode (GL_PROJECTION);
	qglLoadIdentity ();
	qglOrtho (0, vidDef.width, vidDef.height, 0, -99999, 99999);

	qglMatrixMode (GL_MODELVIEW);
	qglLoadIdentity ();

	GL_Disable (GL_CULL_FACE);
	GL_Disable (GL_DEPTH_TEST);
}


/*
=============
GL_Setup3D
=============
*/
void GL_Setup3D (void)
{
	GLfloat		xMin, xMax, yMin, yMax, zNear, zFar;
	GLfloat		vAspect = (float)r_refDef.width/(float)r_refDef.height;
	int			clearBits;

	glState.in2D = qFalse;

	// Set up viewport
	qglScissor (r_refDef.x, vidDef.height - r_refDef.height - r_refDef.y, r_refDef.width, r_refDef.height);
	qglViewport (r_refDef.x, vidDef.height - r_refDef.height - r_refDef.y, r_refDef.width, r_refDef.height);

	// Clear screen if desired
	clearBits = GL_DEPTH_BUFFER_BIT;
	if (gl_clear->integer) {
		qglClearColor (0.5f, 0.5f, 0.5f, 1.0f);
		clearBits |= GL_COLOR_BUFFER_BIT;
	}

	if (glConfig.useStencil && gl_shadows->integer) {
		qglClearStencil (128);
		clearBits |= GL_STENCIL_BUFFER_BIT;
	}

	qglClear (clearBits);

	// Set up projection matrix
	zNear = 4;
	zFar = 65536;

	yMax = zNear * (float)tan ((r_refDef.fovY * M_PI) / 360.0f);
	yMin = -yMax;

	xMin = yMin * vAspect;
	xMax = yMax * vAspect;

	xMin += -(2 * glState.cameraSeparation) / zNear;
	xMax += -(2 * glState.cameraSeparation) / zNear;

	r_projectionMatrix[0] = (2.0f * zNear) / (xMax - xMin);
	r_projectionMatrix[1] = 0.0f;
	r_projectionMatrix[2] = 0.0f;
	r_projectionMatrix[3] = 0.0f;
	r_projectionMatrix[4] = 0.0f;
	r_projectionMatrix[5] = (2.0f * zNear) / (yMax - yMin);
	r_projectionMatrix[6] = 0.0f;
	r_projectionMatrix[7] = 0.0f;
	r_projectionMatrix[8] = (xMax + xMin) / (xMax - xMin);
	r_projectionMatrix[9] = (yMax + yMin) / (yMax - yMin);
	r_projectionMatrix[10] = -(zFar + zNear) / (zFar - zNear);
	r_projectionMatrix[11] = -1.0f;
	r_projectionMatrix[12] = 0.0f;
	r_projectionMatrix[13] = 0.0f;
	r_projectionMatrix[14] = -(2.0f * zFar * zNear) / (zFar - zNear);
	r_projectionMatrix[15] = 0.0f;

	qglMatrixMode (GL_PROJECTION);
	qglLoadMatrixf (r_projectionMatrix);

	// Set up the world view matrix
	qglMatrixMode (GL_MODELVIEW);
	Matrix4_Identity (r_worldViewMatrix);
	Matrix4_Rotate (r_worldViewMatrix, -90, 1, 0, 0);
	Matrix4_Rotate (r_worldViewMatrix,  90, 0, 0, 1);
	Matrix4_Rotate (r_worldViewMatrix, -r_refDef.viewAngles[2], 1, 0, 0);
	Matrix4_Rotate (r_worldViewMatrix, -r_refDef.viewAngles[0], 0, 1, 0);
	Matrix4_Rotate (r_worldViewMatrix, -r_refDef.viewAngles[1], 0, 0, 1);
	Matrix4_Translate (r_worldViewMatrix, -r_refDef.viewOrigin[0], -r_refDef.viewOrigin[1], -r_refDef.viewOrigin[2]);
	qglLoadMatrixf (r_worldViewMatrix);

	// Set drawing parms
	GL_Enable (GL_DEPTH_TEST);
	qglDepthMask (GL_TRUE);
}
