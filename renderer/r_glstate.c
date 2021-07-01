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

// r_glstate.c

#include "r_local.h"

/*
===============
GL_BlendFunc
===============
*/
void GL_BlendFunc (GLenum sFactor, GLenum dFactor) {
	if ((sFactor != glState.blendSFactor) || (dFactor != glState.blendDFactor)) {
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
void GL_SetDefaultState (void) {
	qglFinish ();

	qglColor4f (1, 1, 1, 1);
	qglClearColor (1, 0, 0.5 , 0.5);

	qglDisable (GL_CULL_FACE);
	qglCullFace (GL_FRONT);

	qglDisable (GL_ALPHA_TEST);
	qglAlphaFunc (GL_GREATER, 0.666);

	qglDisable (GL_DEPTH_TEST);
	qglDepthFunc (GL_LEQUAL);
	qglDepthRange (0, 1);

	qglDisable (GL_FOG);

	qglDisable (GL_STENCIL_TEST);
	qglEnable (GL_SCISSOR_TEST);

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglPolygonOffset (-1, -2);

	glState.shadeModel = GL_SMOOTH;
	qglShadeModel (glState.shadeModel);

	qglHint (GL_FOG_HINT,						GL_NICEST);
	qglHint (GL_LINE_SMOOTH_HINT,				GL_NICEST);
	qglHint (GL_PERSPECTIVE_CORRECTION_HINT,	GL_NICEST);
	qglHint (GL_POLYGON_SMOOTH_HINT,			GL_NICEST);

	qglDisable (GL_BLEND);
	glState.blendSFactor = GL_SRC_ALPHA;
	glState.blendDFactor = GL_ONE_MINUS_SRC_ALPHA;
	qglBlendFunc (glState.blendSFactor, glState.blendDFactor);
}


/*
==================
GL_SetupGL2D
==================
*/
void GL_SetupGL2D (void) {
	glState.in2D = qTrue;

	// set 2D virtual screen size
	qglViewport (0, 0, vidDef.width, vidDef.height);
	qglScissor (0, 0, vidDef.width, vidDef.height);

	qglMatrixMode (GL_PROJECTION);
	qglLoadIdentity ();
	qglOrtho (0, vidDef.width, vidDef.height, 0, -99999, 99999);

	qglMatrixMode (GL_MODELVIEW);
	qglLoadIdentity ();

	qglDisable (GL_CULL_FACE);
	qglDisable (GL_DEPTH_TEST);
	GL_ToggleMultisample (qFalse);
}


/*
=============
GL_SetupGL3D
=============
*/
void GL_SetupGL3D (void) {
	GLdouble	xMin, xMax, yMin, yMax, zNear, zFar;
	GLdouble	vAspect = (float)r_RefDef.width/(float)r_RefDef.height;
	int			clearBits;

	// set up viewport
	qglScissor (r_RefDef.x, vidDef.height - r_RefDef.height - r_RefDef.y, r_RefDef.width, r_RefDef.height);
	qglViewport (r_RefDef.x, vidDef.height - r_RefDef.height - r_RefDef.y, r_RefDef.width, r_RefDef.height);

	// clear screen if desired
	clearBits = GL_DEPTH_BUFFER_BIT;
	if (gl_clear->integer)
		clearBits |= GL_COLOR_BUFFER_BIT;

	if (glConfig.useStencil && (gl_shadows->integer > 1)) {
		qglClearStencil (1);
		clearBits |= GL_STENCIL_BUFFER_BIT;
	}

	qglClear (clearBits);

	// set up projection matrix
	zNear = 4;
	zFar = 65536;

	yMax = zNear * tan ((r_RefDef.fovY * M_PI) / 360.0);
	yMin = -yMax;

	xMin = yMin * vAspect;
	xMax = yMax * vAspect;

	xMin += -(2 * glState.cameraSeparation) / zNear;
	xMax += -(2 * glState.cameraSeparation) / zNear;

	r_ProjectionMatrix[0] = (2.0 * zNear) / (xMax - xMin);
	r_ProjectionMatrix[1] = 0.0f;
	r_ProjectionMatrix[2] = 0.0f;
	r_ProjectionMatrix[3] = 0.0f;
	r_ProjectionMatrix[4] = 0.0f;
	r_ProjectionMatrix[5] = (2.0 * zNear) / (yMax - yMin);
	r_ProjectionMatrix[6] = 0.0f;
	r_ProjectionMatrix[7] = 0.0f;
	r_ProjectionMatrix[8] = (xMax + xMin) / (xMax - xMin);
	r_ProjectionMatrix[9] = (yMax + yMin) / (yMax - yMin);
	r_ProjectionMatrix[10] = -(zFar + zNear) / (zFar - zNear);
	r_ProjectionMatrix[11] = -1.0f;
	r_ProjectionMatrix[12] = 0.0f;
	r_ProjectionMatrix[13] = 0.0f;
	r_ProjectionMatrix[14] = -(2.0 * zFar * zNear) / (zFar - zNear);
	r_ProjectionMatrix[15] = 0.0f;

	qglMatrixMode (GL_PROJECTION);
	qglLoadMatrixf (r_ProjectionMatrix);

	// set up the world view matrix
	qglMatrixMode (GL_MODELVIEW);
	Matrix4_Identity (r_WorldViewMatrix);
	Matrix4_Rotate (r_WorldViewMatrix, -90, 1, 0, 0);
	Matrix4_Rotate (r_WorldViewMatrix,  90, 0, 0, 1);
	Matrix4_Rotate (r_WorldViewMatrix, -r_RefDef.viewAngles[2], 1, 0, 0);
	Matrix4_Rotate (r_WorldViewMatrix, -r_RefDef.viewAngles[0], 0, 1, 0);
	Matrix4_Rotate (r_WorldViewMatrix, -r_RefDef.viewAngles[1], 0, 0, 1);
	Matrix4_Translate (r_WorldViewMatrix, -r_RefDef.viewOrigin[0], -r_RefDef.viewOrigin[1], -r_RefDef.viewOrigin[2]);
	qglLoadMatrixf (r_WorldViewMatrix);

	// set drawing parms
	if (gl_cull->integer)
		qglEnable (GL_CULL_FACE);
	else
		qglDisable (GL_CULL_FACE);

	qglEnable (GL_DEPTH_TEST);
	qglDepthMask (GL_TRUE);
	GL_ToggleMultisample (qTrue);
}


/*
===============
GL_ShadeModel
===============
*/
void GL_ShadeModel (GLenum mode) {
	// GL_SMOOTH is GL default
	if (glState.shadeModel == mode)
		return;

	qglShadeModel (mode);
	glState.shadeModel = mode;
}


/*
===============
GL_ToggleMultisample
===============
*/
void GL_ToggleMultisample (qBool enable) {
	// Disabled is the GL default
	if (glState.multiSampleOn && !glConfig.extMultiSample) {
		glState.multiSampleOn = qFalse;
		return;
	}

	if (glState.multiSampleOn == enable)
		return;

	glState.multiSampleOn = enable;
	if (glState.multiSampleOn)
		qglEnable (GL_MULTISAMPLE_ARB);
	else
		qglDisable (GL_MULTISAMPLE_ARB);
}
