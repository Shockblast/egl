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
GL_BlendFunc
===============
*/
void GL_BlendFunc (GLenum sFactor, GLenum dFactor)
{
	if (sFactor != glState.blendSFactor || dFactor != glState.blendDFactor) {
		qglBlendFunc (sFactor, dFactor);
		glState.blendSFactor = sFactor;
		glState.blendDFactor = dFactor;
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
	qglFinish ();

	qglColor4f (1, 1, 1, 1);
	qglClearColor (0.5f, 0.5f, 0.5f, 1.0f);

	qglDisable (GL_CULL_FACE);
	qglCullFace (GL_FRONT);

	qglDisable (GL_ALPHA_TEST);
	qglAlphaFunc (GL_GREATER, 0.666f);

	qglDisable (GL_DEPTH_TEST);
	qglDepthFunc (GL_LEQUAL);
	qglDepthRange (0, 1);

	qglDisable (GL_STENCIL_TEST);
	qglEnable (GL_SCISSOR_TEST);

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglPolygonOffset (0, 0);

	glState.shadeModel = GL_SMOOTH;
	qglShadeModel (glState.shadeModel);

	qglHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
	qglHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	qglHint (GL_POLYGON_SMOOTH_HINT, GL_NICEST);

	qglDisable (GL_BLEND);
	glState.blendSFactor = GL_SRC_ALPHA;
	glState.blendDFactor = GL_ONE_MINUS_SRC_ALPHA;
	qglBlendFunc (glState.blendSFactor, glState.blendDFactor);
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

	qglDisable (GL_CULL_FACE);
	qglDisable (GL_DEPTH_TEST);
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
	qglEnable (GL_DEPTH_TEST);
	qglDepthMask (GL_TRUE);
}


/*
===============
GL_ShadeModel
===============
*/
void GL_ShadeModel (GLenum mode)
{
	// GL_SMOOTH is GL default
	if (glState.shadeModel == mode)
		return;

	qglShadeModel (mode);
	glState.shadeModel = mode;
}
