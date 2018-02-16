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
// rb_state.c
// FIXME TODO:
// - Statebit pushing, which will require that all state changes are local to the backend
//

#include "rb_local.h"

rb_glState_t	rb_glState;

/*
==============================================================================

	STATEBIT MANAGEMENT

==============================================================================
*/

/*
===============
RB_StateForBits
===============
*/
void RB_StateForBits (uint32 bits1)
{
	uint32	diff;

	// Process bit group one
	diff = bits1 ^ rb_glState.stateBits1;
	if (diff) {
		// Alpha testing
		if (diff & SB1_ATEST_BITS) {
			switch (bits1 & SB1_ATEST_BITS) {
			case 0:
				qglDisable (GL_ALPHA_TEST);
				break;
			case SB1_ATEST_GT0:
				qglEnable (GL_ALPHA_TEST);
				qglAlphaFunc (GL_GREATER, 0);
				break;
			case SB1_ATEST_LT128:
				qglEnable (GL_ALPHA_TEST);
				qglAlphaFunc (GL_LESS, 0.5f);
				break;
			case SB1_ATEST_GE128:
				qglEnable (GL_ALPHA_TEST);
				qglAlphaFunc (GL_GEQUAL, 0.5f);
				break;
			default:
				assert (0);
				break;
			}

			ri.pc.stateChanges++;
		}

		// Blending
		if (diff & SB1_BLEND_ON) {
			if (bits1 & SB1_BLEND_ON)
				qglEnable (GL_BLEND);
			else
				qglDisable (GL_BLEND);
			ri.pc.stateChanges++;
		}

		if (diff & (SB1_BLENDSRC_BITS|SB1_BLENDDST_BITS)
		&& bits1 & (SB1_BLENDSRC_BITS|SB1_BLENDDST_BITS)) {
			GLenum	sFactor, dFactor;

			switch (bits1 & SB1_BLENDSRC_BITS) {
			case SB1_BLENDSRC_ZERO:					sFactor = GL_ZERO;					break;
			case SB1_BLENDSRC_ONE:					sFactor = GL_ONE;					break;
			case SB1_BLENDSRC_DST_COLOR:			sFactor = GL_DST_COLOR;				break;
			case SB1_BLENDSRC_ONE_MINUS_DST_COLOR:	sFactor = GL_ONE_MINUS_DST_COLOR;	break;
			case SB1_BLENDSRC_SRC_ALPHA:			sFactor = GL_SRC_ALPHA;				break;
			case SB1_BLENDSRC_ONE_MINUS_SRC_ALPHA:	sFactor = GL_ONE_MINUS_SRC_ALPHA;	break;
			case SB1_BLENDSRC_DST_ALPHA:			sFactor = GL_DST_ALPHA;				break;
			case SB1_BLENDSRC_ONE_MINUS_DST_ALPHA:	sFactor = GL_ONE_MINUS_DST_ALPHA;	break;
			case SB1_BLENDSRC_SRC_ALPHA_SATURATE:	sFactor = GL_SRC_ALPHA_SATURATE;	break;
			default:								assert (0);							break;
			}

			switch (bits1 & SB1_BLENDDST_BITS) {
			case SB1_BLENDDST_ZERO:					dFactor = GL_ZERO;					break;
			case SB1_BLENDDST_ONE:					dFactor = GL_ONE;					break;
			case SB1_BLENDDST_SRC_COLOR:			dFactor = GL_SRC_COLOR;				break;
			case SB1_BLENDDST_ONE_MINUS_SRC_COLOR:	dFactor = GL_ONE_MINUS_SRC_COLOR;	break;
			case SB1_BLENDDST_SRC_ALPHA:			dFactor = GL_SRC_ALPHA;				break;
			case SB1_BLENDDST_ONE_MINUS_SRC_ALPHA:	dFactor = GL_ONE_MINUS_SRC_ALPHA;	break;
			case SB1_BLENDDST_DST_ALPHA:			dFactor = GL_DST_ALPHA;				break;
			case SB1_BLENDDST_ONE_MINUS_DST_ALPHA:	dFactor = GL_ONE_MINUS_DST_ALPHA;	break;
			default:								assert (0);							break;
			}

			qglBlendFunc (sFactor, dFactor);
			ri.pc.stateChanges++;
		}

		// Culling
		if (diff & SB1_CULL_BITS) {
			switch (bits1 & SB1_CULL_BITS) {
			case 0:
				qglDisable (GL_CULL_FACE);
				break;
			case SB1_CULL_FRONT:
				qglCullFace (GL_FRONT);
				qglEnable (GL_CULL_FACE);
				break;
			case SB1_CULL_BACK:
				qglCullFace (GL_BACK);
				qglEnable (GL_CULL_FACE);
				break;
			default:
				assert (0);
				break;
			}
			ri.pc.stateChanges++;
		}

		// Depth masking
		if (diff & SB1_DEPTHMASK_ON) {
			if (bits1 & SB1_DEPTHMASK_ON)
				qglDepthMask (GL_TRUE);
			else
				qglDepthMask (GL_FALSE);
			ri.pc.stateChanges++;
		}

		// Depth testing
		if (diff & SB1_DEPTHTEST_ON) {
			if (bits1 & SB1_DEPTHTEST_ON)
				qglEnable (GL_DEPTH_TEST);
			else
				qglDisable (GL_DEPTH_TEST);
			ri.pc.stateChanges++;
		}

		// Polygon offset
		if (diff & SB1_POLYOFFSET_ON) {
			if (bits1 & SB1_POLYOFFSET_ON) {
				qglEnable (GL_POLYGON_OFFSET_FILL);
				qglPolygonOffset (r_offsetFactor->intVal, r_offsetUnits->intVal);
			}
			else
				qglDisable (GL_POLYGON_OFFSET_FILL);
			ri.pc.stateChanges++;
		}

		// Save for the next diff
		rb_glState.stateBits1 = bits1;
	}
}

/*
==============================================================================

	TEXTURE STATE

==============================================================================
*/

/*
===============
RB_BindTexture
===============
*/
void RB_BindTexture (image_t *image)
{
	// Performance evaluation option
	if (gl_nobind->intVal || !image)
		image = ri.noTexture;

	// Determine if it's already bound
	if (rb_glState.texBound[rb_glState.texUnit] == image)
		return;
	rb_glState.texBound[rb_glState.texUnit] = image;

	// Nope, bind it
	qglBindTexture (image->target, image->texNum);

	// Performance counters
	if (r_speeds->intVal) {
		ri.pc.textureBinds++;
		if (image->visFrame != ri.frameCount) {
			image->visFrame = ri.frameCount;
			ri.pc.texturesInUse++;
			ri.pc.texelsInUse += image->upWidth * image->upHeight;
		}
	}
}


/*
===============
RB_SelectTexture
===============
*/
void RB_SelectTexture (texUnit_t texUnit)
{
	if (texUnit == rb_glState.texUnit)
		return;
	if (texUnit > ri.config.maxTexUnits) {
		Com_Error (ERR_DROP, "Attempted selection of an out of bounds (%d) texture unit!", texUnit);
		return;
	}

	// Select the unit
	rb_glState.texUnit = texUnit;
	if (ri.config.extArbMultitexture) {
		qglActiveTextureARB (texUnit + GL_TEXTURE0_ARB);
		qglClientActiveTextureARB (texUnit + GL_TEXTURE0_ARB);
	}
	else if (ri.config.extSGISMultiTexture) {
		qglSelectTextureSGIS (texUnit + GL_TEXTURE0_SGIS);
	}
	else {
		return;
	}

	// Performance counter
	ri.pc.textureUnitChanges++;
}


/*
===============
RB_TextureEnv
===============
*/
void RB_TextureEnv (GLfloat mode)
{
	if (mode == GL_ADD && !ri.config.extTexEnvAdd)
		mode = GL_MODULATE;

	if (mode != rb_glState.texEnvModes[rb_glState.texUnit]) {
		qglTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode);
		rb_glState.texEnvModes[rb_glState.texUnit] = mode;

		// Performance counter
		ri.pc.textureEnvChanges++;
	}
}


/*
===============
RB_TextureTarget

Supplements qglEnable/qglDisable on GL_TEXTURE_1D/2D/3D/CUBE_MAP_ARB.
===============
*/
void RB_TextureTarget (GLenum target)
{
	if (target == rb_glState.texTarget[rb_glState.texUnit])
		return;

	if (rb_glState.texTarget[rb_glState.texUnit])
		qglDisable (rb_glState.texTarget[rb_glState.texUnit]);

	rb_glState.texTarget[rb_glState.texUnit] = target;

	if (target)
		qglEnable (target);
}


/*
===============
RB_LoadTexMatrix
===============
*/
void RB_LoadTexMatrix (mat4x4_t m)
{
	qglLoadMatrixf (m);
	rb_glState.texMatIdentity[rb_glState.texUnit] = qFalse;
}


/*
===============
RB_LoadIdentityTexMatrix
===============
*/
void RB_LoadIdentityTexMatrix (void)
{
	if (!rb_glState.texMatIdentity[rb_glState.texUnit]) {
		qglLoadIdentity ();
		rb_glState.texMatIdentity[rb_glState.texUnit] = qTrue;
	}
}

/*
==============================================================================

	PROGRAM STATE

==============================================================================
*/

/*
===============
RB_BindProgram
===============
*/
void RB_BindProgram (program_t *program)
{
	switch (program->target) {
	case GL_FRAGMENT_PROGRAM_ARB:
		if (rb_glState.boundFragProgram == program->progNum)
			return;
		rb_glState.boundFragProgram = program->progNum;

		qglBindProgramARB (GL_FRAGMENT_PROGRAM_ARB, program->progNum);
		break;

	case GL_VERTEX_PROGRAM_ARB:
		if (rb_glState.boundVertProgram == program->progNum)
			return;
		rb_glState.boundVertProgram = program->progNum;

		qglBindProgramARB (GL_VERTEX_PROGRAM_ARB, program->progNum);
		break;

#ifdef _DEBUG
	default:
		assert (0);
		break;
#endif // _DEBUG
	}
}

/*
==============================================================================

	GENERIC STATE MANAGEMENT

==============================================================================
*/

/*
==================
RB_SetupGL2D
==================
*/
void RB_SetupGL2D (void)
{
	// State
	rb_glState.in2D = qTrue;
	rb_glState.stateBits1 &= ~SB1_DEPTHMASK_ON;
	qglDepthMask (GL_FALSE);

	// Set 2D virtual screen size
	qglViewport (0, 0, ri.config.vidWidth, ri.config.vidHeight);
	qglScissor (0, 0, ri.config.vidWidth, ri.config.vidHeight);

	qglMatrixMode (GL_PROJECTION);
	qglLoadIdentity ();
	qglOrtho (0, ri.config.vidWidth, ri.config.vidHeight, 0, -99999, 99999);

	qglMatrixMode (GL_MODELVIEW);
	qglLoadIdentity ();
}


/*
=============
RB_SetupGL3D
=============
*/
void RB_SetupGL3D (void)
{
	// State
	rb_glState.in2D = qFalse;
	rb_glState.stateBits1 |= SB1_DEPTHMASK_ON;
	qglDepthMask (GL_TRUE);

	// Set up viewport
	if (!ri.scn.mirrorView && !ri.scn.portalView) {
		qglScissor (ri.def.x, ri.config.vidHeight - ri.def.height - ri.def.y, ri.def.width, ri.def.height);
		qglViewport (ri.def.x, ri.config.vidHeight - ri.def.height - ri.def.y, ri.def.width, ri.def.height);
		qglClear (GL_DEPTH_BUFFER_BIT);
	}

	// Set up projection matrix
	R_SetupProjectionMatrix (&ri.def, ri.scn.projectionMatrix);
	if (ri.scn.mirrorView)
		ri.scn.projectionMatrix[0] = -ri.scn.projectionMatrix[0];

	qglMatrixMode (GL_PROJECTION);
	qglLoadMatrixf (ri.scn.projectionMatrix);

	// Set up the world view matrix
	R_SetupModelviewMatrix (&ri.def, ri.scn.worldViewMatrix);

	qglMatrixMode (GL_MODELVIEW);
	qglLoadMatrixf (ri.scn.worldViewMatrix);

	// Handle portal/mirror rendering
	if (ri.scn.mirrorView || ri.scn.portalView) {
		GLdouble	clip[4];

		clip[0] = ri.scn.clipPlane.normal[0];
		clip[1] = ri.scn.clipPlane.normal[1];
		clip[2] = ri.scn.clipPlane.normal[2];
		clip[3] = -ri.scn.clipPlane.dist;

		qglClipPlane (GL_CLIP_PLANE0, clip);
		qglEnable (GL_CLIP_PLANE0);
	}
}


/*
================
RB_ClearBuffers
================
*/
void RB_ClearBuffers (void)
{
	int			clearBits;

	clearBits = GL_DEPTH_BUFFER_BIT;
	if (gl_clear->intVal) {
		qglClearColor (0.5f, 0.5f, 0.5f, 1.0f);
		clearBits |= GL_COLOR_BUFFER_BIT;
	}

	if (ri.useStencil && gl_shadows->intVal) {
		qglClearStencil (128);
		clearBits |= GL_STENCIL_BUFFER_BIT;
	}

	qglClear (clearBits);

	qglDepthRange (0, 1);
}


/*
==================
RB_SetDefaultState

Sets our default OpenGL state
==================
*/
void RB_SetDefaultState (void)
{
	texUnit_t	i;

	rb_glState.stateBits1 = 0;

	qglFinish ();

	qglColor4f (1, 1, 1, 1);
	qglClearColor (0.5f, 0.5f, 0.5f, 1.0f);

	qglEnable (GL_SCISSOR_TEST);

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglPolygonOffset (0, 0);

	// Texture-unit specific
	for (i=MAX_TEXUNITS-1 ; i>=0 ; i--) {
		rb_glState.texBound[i] = NULL;
		rb_glState.texEnvModes[i] = 0;
		rb_glState.texMatIdentity[i] = qTrue;
		if (i >= ri.config.maxTexUnits)
			continue;

		// Texture
		RB_SelectTexture (i);

		qglDisable (GL_TEXTURE_1D);
		qglBindTexture (GL_TEXTURE_1D, 0);

		if (ri.config.extTex3D) {
			qglDisable (GL_TEXTURE_3D);
			qglBindTexture (GL_TEXTURE_3D, 0);
		}
		if (ri.config.extTexCubeMap) {
			qglDisable (GL_TEXTURE_CUBE_MAP_ARB);
			qglBindTexture (GL_TEXTURE_CUBE_MAP_ARB, 0);
		}

		if (i == 0) {
			qglEnable (GL_TEXTURE_2D);
			rb_glState.texTarget[i] = GL_TEXTURE_2D;
		}
		else {
			qglDisable (GL_TEXTURE_2D);
			rb_glState.texTarget[i] = 0;
		}
		qglBindTexture (GL_TEXTURE_2D, 0);

		// Texgen
		qglDisable (GL_TEXTURE_GEN_S);
		qglDisable (GL_TEXTURE_GEN_T);
		qglDisable (GL_TEXTURE_GEN_R);
		qglDisable (GL_TEXTURE_GEN_Q);
	}

	// Fragment programs
	if (ri.config.extFragmentProgram)
		qglDisable (GL_FRAGMENT_PROGRAM_ARB);

	// Vertex programs
	if (ri.config.extVertexProgram)
		qglDisable (GL_VERTEX_PROGRAM_ARB);

	// Stencil testing
	if (ri.useStencil)
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
	rb_glState.stateBits1 |= SB1_CULL_FRONT;

	// Alpha testing
	qglDisable (GL_ALPHA_TEST);
	qglAlphaFunc (GL_GREATER, 0);

	// Blending
	qglDisable (GL_BLEND);
	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	rb_glState.stateBits1 |= SB1_BLENDSRC_SRC_ALPHA|SB1_BLENDDST_ONE_MINUS_SRC_ALPHA;

	// Model shading
	qglShadeModel (GL_SMOOTH);

	// Check for errors
	GL_CheckForError ("RB_SetDefaultState");
}
