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
// cg_view.c
//

#include "cg_local.h"

static cVar_t	*cl_testblend;
static cVar_t	*cl_testentities;
static cVar_t	*cl_testlights;
static cVar_t	*cl_testparticles;

static cVar_t	*viewsize;

/*
=======================================================================

	SCREEN SIZE

=======================================================================
*/

/*
=================
V_CalcVrect

Sets the coordinates of the rendered window
=================
*/
static void V_CalcVrect (void) {
	int		size;

	// bound viewsize
	if (viewsize->integer < 40)
		cgi.Cvar_ForceSetValue ("viewsize", 40);
	if (viewsize->integer > 100)
		cgi.Cvar_ForceSetValue ("viewsize", 100);

	size = viewsize->integer;

	cg.refDef.width = cg.vidWidth*size/100;
	cg.refDef.width &= ~7;

	cg.refDef.height = cg.vidHeight*size/100;
	cg.refDef.height &= ~1;

	cg.refDef.x = (cg.vidWidth - cg.refDef.width)/2;
	cg.refDef.y = (cg.vidHeight - cg.refDef.height)/2;
}


/*
=================
V_SizeUp_f
=================
*/
static void V_SizeUp_f (void) {
	cgi.Cvar_ForceSetValue ("viewsize", viewsize->integer + 10);
}


/*
=================
V_SizeDown_f
=================
*/
static void V_SizeDown_f (void) {
	cgi.Cvar_ForceSetValue ("viewsize", viewsize->integer - 10);
}


/*
==============
V_TileClear
==============
*/
static void V_TileRect (int x, int y, int w, int h) {
	if ((w == 0) || (h == 0))
		return;	// prevents div by zero (should never happen)

	if (cgMedia.tileBackImage)
		cgi.Draw_Pic (cgMedia.tileBackImage, x, y, w, h, x/64.0, y/64.0, (x+w)/64.0, (y+h)/64.0, colorWhite);
	else
		cgi.Draw_Fill (x, y, w, h, colorBlack);
}
static void V_TileClear (void) {
	int		top, bottom, left, right;

	if (viewsize->integer >= 100)
		return;		// full screen rendering

	top = cg.refDef.y;
	bottom = top + cg.refDef.height-1;
	left = cg.refDef.x;
	right = left + cg.refDef.width-1;

	// clear above view screen
	V_TileRect (0, 0, cg.vidWidth, top);

	// clear below view screen
	V_TileRect (0, bottom, cg.vidWidth, cg.vidHeight - bottom);

	// clear left of view screen
	V_TileRect (0, top, left, bottom - top + 1);

	// clear right of view screen
	V_TileRect (right, top, cg.vidWidth - right, bottom - top + 1);
}

/*
=======================================================================

	VIEW RENDERING

=======================================================================
*/

/*
================
V_TestParticles
================
*/
static void V_TestParticles (void) {
	int			i, j;
	float		d, r, u;
	vec3_t		origin;
	dvec4_t		color;

	cgi.R_ClearScene ();
	Vector4Set (color, 255, 255, 255, 1);
	for (i=0 ; i<MAX_PARTICLES ; i++) {
		d = i*0.25;
		r = 4*((i&7)-3.5);
		u = 4*(((i>>3)&7)-3.5);

		for (j=0 ; j<3 ; j++)
			origin[j] = cg.refDef.viewOrigin[j] + cg.v_Forward[j]*d + cg.v_Right[j]*r + cg.v_Up[j]*u;

		cgi.R_AddParticle (origin, vec3Origin, color, 1, cgMedia.mediaTable[i%PDT_PICTOTAL], 0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, (i*2)%361);
	}
}


/*
================
V_TestEntities
================
*/
static void V_TestEntities (void) {
	int			i, j;
	float		f, r;
	entity_t	ent;

	cgi.R_ClearScene ();
	for (i=0 ; i<32 ; i++) {
		r = 64 * ((i%4) - 1.5);
		f = 64 * (i/4) + 128;

		for (j=0 ; j<3 ; j++) {
			ent.origin[j] = cg.refDef.viewOrigin[j] + cg.v_Forward[j]*f + cg.v_Right[j]*r;
			ent.oldOrigin[j] = ent.origin[j];
		}

		Matrix_Identity (ent.axis);

		ent.model = cg.baseClientInfo.model;
		ent.skin = cg.baseClientInfo.skin;
		ent.skinNum = 0;

		ent.flags = 0;
		ent.scale = 1;
		Vector4Set (ent.color, 1, 1, 1, 1);

		ent.backLerp = 0;
		ent.frame = 0;
		ent.oldFrame = 0;

		cgi.R_AddEntity (&ent);
	}
}


/*
================
V_TestLights
================
*/
static void V_TestLights (void) {
	int			i, j;
	float		f, r;
	vec3_t		origin;
	float		red, green, blue;

	for (i=0 ; i<32 ; i++) {
		r = 64 * ((i%4) - 1.5);
		f = 64 * (i/4) + 128;

		for (j=0 ; j<3 ; j++)
			origin[j] = cg.refDef.viewOrigin[j] + cg.v_Forward[j]*f + cg.v_Right[j]*r;
			
		red = ((i%6)+1) & 1;
		green = (((i%6)+1) & 2)>>1;
		blue = (((i%6)+1) & 4)>>2;

		cgi.R_AddLight (origin, 200, red, green, blue);
	}
}

// ====================================================================

/*
===============
V_CalcViewValues

Sets cg.refDef view values
===============
*/
static void V_CalcViewValues (void) {
	int				i;
	cgEntity_t		*ent;
	playerState_t	*ps, *ops;

	// set cg.lerpFrac
	if (cgi.Cvar_VariableInteger ("timedemo"))
		cg.lerpFrac = 1.0;
	else
		cg.lerpFrac = 1.0 - (cg.frame.serverTime - cg.time) * 0.01;

	// find the previous frame to interpolate from
	ps = &cg.frame.playerState;
	if (cg.oldFrame.serverFrame != cg.frame.serverFrame-1 || !cg.oldFrame.valid)
		ops = &cg.frame.playerState;	// previous frame was dropped or invalid
	else
		ops = &cg.oldFrame.playerState;

	// see if the player entity was teleported this frame
	if (fabs(ops->pMove.origin[0] - ps->pMove.origin[0]) > 256*8	||
		abs (ops->pMove.origin[1] - ps->pMove.origin[1]) > 256*8	||
		abs (ops->pMove.origin[2] - ps->pMove.origin[2]) > 256*8)
		ops = ps;		// don't interpolate

	ent = &cgEntities[cg.playerNum+1];

	// calculate the origin
	if (cl_predict->integer && !(cg.frame.playerState.pMove.pmFlags & PMF_NO_PREDICTION)) {
		// use predicted values
		uInt		delta;
		float		backLerp;

		backLerp = 1.0 - cg.lerpFrac;
		for (i=0 ; i<3 ; i++) {
			cg.refDef.viewOrigin[i] = cg.predictedOrigin[i] + ops->viewOffset[i] 
								+ cg.lerpFrac*(ps->viewOffset[i] - ops->viewOffset[i])
								- backLerp*cg.predictionError[i];
		}

		// smooth out stair climbing
		delta = cgs.realTime - cg.predictedStepTime;
		if (delta < 100)
			cg.refDef.viewOrigin[2] -= cg.predictedStep * (100 - delta) * 0.01;
	} else {
		// just use interpolated values
		for (i=0 ; i<3 ; i++) {
			cg.refDef.viewOrigin[i] = ops->pMove.origin[i]*ONEDIV8 + ops->viewOffset[i] 
								+ cg.lerpFrac*(ps->pMove.origin[i]*ONEDIV8 + ps->viewOffset[i] 
								- (ops->pMove.origin[i]*ONEDIV8 + ops->viewOffset[i]));
		}
	}

	// if not running a demo or on a locked frame, add the local angle movement
	if ((cg.frame.playerState.pMove.pmType < PMT_DEAD) && !cg.attractLoop) {
		// use predicted values
		VectorCopy (cg.predictedAngles, cg.refDef.viewAngles);
	} else {
		// just use interpolated values
		for (i=0 ; i<3 ; i++)
			cg.refDef.viewAngles[i] = InterpolateAngle (ops->viewAngles[i], ps->viewAngles[i], cg.lerpFrac);
	}

	for (i=0 ; i<3 ; i++)
		cg.refDef.viewAngles[i] += InterpolateAngle (ops->kickAngles[i], ps->kickAngles[i], cg.lerpFrac);

	AngleVectors (cg.refDef.viewAngles, cg.v_Forward, cg.v_Right, cg.v_Up);

	if (cg.refDef.viewAngles[0] || cg.refDef.viewAngles[1] || cg.refDef.viewAngles[2])
		AnglesToAxis (cg.refDef.viewAngles, cg.refDef.viewAxis);
	else
		Matrix_Identity (cg.refDef.viewAxis);

	// interpolate field of view
	cg.refDef.fovX = ops->fov + cg.lerpFrac * (ps->fov - ops->fov);

	// don't interpolate blend color
	Vector4Copy (ps->vBlend, cg.refDef.vBlend);
}


/*
==================
V_RenderView
==================
*/
#define FRAMETIME_MAX 0.2
void V_RenderView (int vidWidth, int vidHeight, int realTime, float frameTime, float stereoSeparation, qBool forceRefresh) {
	// check cvar sanity
	CG_UpdateCvars ();

	// video dimensions
	cg.vidWidth = vidWidth;
	cg.vidHeight = vidHeight;

	// calculate screen dimensions and clear the background
	V_CalcVrect ();
	V_TileClear ();

	// set time
	cgs.realTime = realTime;
	cgs.frameTime = frameTime;
	cgs.frameCount++;
	cg.time += frameTime * 1000;

	// clamp time
	cg.time = clamp (cg.time, cg.frame.serverTime - 100, cg.frame.serverTime);
	if (cgs.frameTime > FRAMETIME_MAX)
		cgs.frameTime = FRAMETIME_MAX;

	// predict all unacknowledged movements
	CG_PredictMovement ();

	// watch for gender bending if desired
	CG_FixUpGender ();

	// run light styles
	CG_RunLightStyles ();

	// an invalid frame will just use the exact previous refdef
	// we can't use the old frame if the video mode has changed, though
	if (cg.frame.valid && (forceRefresh || !cgi.Cvar_VariableInteger ("paused"))) {
		cgi.R_ClearScene ();

		// calculate the view values
		V_CalcViewValues ();

		// add in entities and effects
		CG_AddEntities ();

		if (cl_testblend->integer) {
			cg.refDef.vBlend[0] = 1;
			cg.refDef.vBlend[1] = 0.5;
			cg.refDef.vBlend[2] = 0.25;
			cg.refDef.vBlend[3] = 0.5;
		}
		if (cl_testentities->integer)
			V_TestEntities ();
		if (cl_testlights->integer)
			V_TestLights ();
		if (cl_testparticles->integer)
			V_TestParticles ();

		// offset the viewOrigin if we're using stereo separation
		if (stereoSeparation != 0)
			VectorMA (cg.refDef.viewOrigin, stereoSeparation, cg.v_Right, cg.refDef.viewOrigin);

		// never let it sit exactly on a node line, because a water plane
		// can dissapear when viewed with the eye exactly on it. the server
		// protocol only specifies to 1/8 pixel, so add 1/16 in each axis
		cg.refDef.viewOrigin[0] += ONEDIV16;
		cg.refDef.viewOrigin[1] += ONEDIV16;
		cg.refDef.viewOrigin[2] += ONEDIV16;

		cg.refDef.fovY		= CalcFov (cg.refDef.fovX, cg.refDef.width, cg.refDef.height);

		cg.refDef.time		= cg.time * 0.001;
		cg.refDef.areaBits	= cg.frame.areaBits;

		cg.refDef.rdFlags	= cg.frame.playerState.rdFlags;
	}

	// render the frame
	cgi.R_RenderScene (&cg.refDef);

	// update orientation for sound subsystem
	cgi.Snd_Update (cg.refDef.viewOrigin, cg.v_Forward, cg.v_Right, cg.v_Up, (cg.refDef.rdFlags & RDF_UNDERWATER) ? qTrue : qFalse);

	// run subsystems
	CG_RunDLights ();

	// render screen stuff
	SCR_Draw ();
}

/*
=======================================================================

	CONSOLE FUNCTIONS

=======================================================================
*/

/*
=============
V_Viewpos_f
=============
*/
static void V_Viewpos_f (void) {
	Com_Printf (0, "(x%i y%i z%i) : yaw%i\n",
				(int)cg.refDef.viewOrigin[0],
				(int)cg.refDef.viewOrigin[1],
				(int)cg.refDef.viewOrigin[2], 
				(int)cg.refDef.viewAngles[YAW]);
}

/*
=======================================================================

	INIT / SHUTDOWN

=======================================================================
*/

/*
=============
V_Register
=============
*/
void V_Register (void) {
	cl_testblend		= cgi.Cvar_Get ("cl_testblend",			"0",		0);
	cl_testentities		= cgi.Cvar_Get ("cl_testentities",		"0",		0);
	cl_testlights		= cgi.Cvar_Get ("cl_testlights",		"0",		0);
	cl_testparticles	= cgi.Cvar_Get ("cl_testparticles",		"0",		0);

	viewsize			= cgi.Cvar_Get ("viewsize",				"100",		CVAR_ARCHIVE);

	cgi.Cmd_AddCommand ("sizeup",		V_SizeUp_f,		"Increases viewport size");
	cgi.Cmd_AddCommand ("sizedown",		V_SizeDown_f,	"Decreases viewport size");

	cgi.Cmd_AddCommand ("viewpos",		V_Viewpos_f,	"Prints view position and yaw");
}


/*
=============
V_Unregister
=============
*/
void V_Unregister (void) {
	cgi.Cmd_RemoveCommand ("sizeup");
	cgi.Cmd_RemoveCommand ("sizedown");

	cgi.Cmd_RemoveCommand ("viewpos");
}
