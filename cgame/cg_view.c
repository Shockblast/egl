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
static void V_CalcVrect (void)
{
	int		size;

	// Bound viewsize
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
static void V_SizeUp_f (void)
{
	cgi.Cvar_ForceSetValue ("viewsize", viewsize->integer + 10.0f);
}


/*
=================
V_SizeDown_f
=================
*/
static void V_SizeDown_f (void)
{
	cgi.Cvar_ForceSetValue ("viewsize", viewsize->integer - 10.0f);
}


/*
==============
V_TileClear
==============
*/
static void V_TileRect (int x, int y, int w, int h)
{
	if (w == 0 || h == 0)
		return;	// Prevents div by zero (should never happen)

	if (cgMedia.tileBackShader)
		cgi.R_DrawPic (cgMedia.tileBackShader,
		(float)x, (float)y, (float)w, (float)h,
		x/64.0f, y/64.0f, (x+w)/64.0f, (y+h)/64.0f, colorWhite);
	else
		cgi.R_DrawFill ((float)x, (float)y, (float)w, (float)h, colorBlack);
}
static void V_TileClear (void)
{
	int		top, bottom, left, right;

	if (viewsize->integer >= 100)
		return;		// Full screen rendering

	top = cg.refDef.y;
	bottom = top + cg.refDef.height-1;
	left = cg.refDef.x;
	right = left + cg.refDef.width-1;

	// Clear above view screen
	V_TileRect (0, 0, cg.vidWidth, top);

	// Clear below view screen
	V_TileRect (0, bottom, cg.vidWidth, cg.vidHeight - bottom);

	// Clear left of view screen
	V_TileRect (0, top, left, bottom - top + 1);

	// Clear right of view screen
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
static particle_t v_testParticleList[PT_PICTOTAL];
static void V_TestParticles (void)
{
	int			i;
	float		d, r, u;
	vec3_t		origin;
	poly_t		poly;
	float		scale;
	bvec4_t		outColor;

	cgi.R_ClearScene ();
	Vector4Set (outColor, 255, 255, 255, 255);
	scale = 0.5f;

	for (i=0 ; i<PT_PICTOTAL ; i++) {
		d = i*0.25f;
		r = 4*((i&7)-3.5f);
		u = 4*(((i>>3)&7)-3.5f);

		// Center
		origin[0] = cg.refDef.viewOrigin[0] + cg.forwardVec[0]*d + cg.rightVec[0]*r + cg.upVec[0]*u;
		origin[1] = cg.refDef.viewOrigin[1] + cg.forwardVec[1]*d + cg.rightVec[1]*r + cg.upVec[1]*u;
		origin[2] = cg.refDef.viewOrigin[2] + cg.forwardVec[2]*d + cg.rightVec[2]*r + cg.upVec[2]*u;

		// Top left
		*(int *)v_testParticleList[i].outColor[0] = *(int *)outColor;
		Vector2Set (v_testParticleList[i].outCoords[0], 0, 0);
		VectorSet (v_testParticleList[i].outVertices[0],	origin[0] + cg.upVec[0]*scale - cg.rightVec[0]*scale,
															origin[1] + cg.upVec[1]*scale - cg.rightVec[1]*scale,
															origin[2] + cg.upVec[2]*scale - cg.rightVec[2]*scale);

		// Bottom left
		*(int *)v_testParticleList[i].outColor[0] = *(int *)outColor;
		Vector2Set (v_testParticleList[i].outCoords[1], 0, 1);
		VectorSet (v_testParticleList[i].outVertices[1],	origin[0] - cg.upVec[0]*scale - cg.rightVec[0]*scale,
															origin[1] - cg.upVec[1]*scale - cg.rightVec[1]*scale,
															origin[2] - cg.upVec[2]*scale - cg.rightVec[2]*scale);

		// Bottom right
		*(int *)v_testParticleList[i].outColor[0] = *(int *)outColor;
		Vector2Set (v_testParticleList[i].outCoords[2], 1, 1);
		VectorSet (v_testParticleList[i].outVertices[2],	origin[0] - cg.upVec[0]*scale + cg.rightVec[0]*scale,
															origin[1] - cg.upVec[1]*scale + cg.rightVec[1]*scale,
															origin[2] - cg.upVec[2]*scale + cg.rightVec[2]*scale);

		// Top right
		*(int *)v_testParticleList[i].outColor[0] = *(int *)outColor;
		Vector2Set (v_testParticleList[i].outCoords[3], 1, 0);
		VectorSet (v_testParticleList[i].outVertices[3],	origin[0] + cg.upVec[0]*scale + cg.rightVec[0]*scale,
															origin[1] + cg.upVec[1]*scale + cg.rightVec[1]*scale,
															origin[2] + cg.upVec[2]*scale + cg.rightVec[2]*scale);

		// Render it
		poly.numVerts = 4;
		poly.colors = v_testParticleList[i].outColor;
		poly.texCoords = v_testParticleList[i].outCoords;
		poly.vertices = v_testParticleList[i].outVertices;
		poly.shader = cgMedia.particleTable[i % PT_PICTOTAL];

		cgi.R_AddPoly (&poly);
	}
}


/*
================
V_TestEntities
================
*/
static void V_TestEntities (void)
{
	int			i, j;
	float		f, r;
	entity_t	ent;

	cgi.R_ClearScene ();
	for (i=0 ; i<32 ; i++) {
		r = 64 * ((i%4) - 1.5f);
		f = 64 * (i/40.f) + 128;

		for (j=0 ; j<3 ; j++) {
			ent.origin[j] = cg.refDef.viewOrigin[j] + cg.forwardVec[j]*f + cg.rightVec[j]*r;
			ent.oldOrigin[j] = ent.origin[j];
		}

		Matrix_Identity (ent.axis);

		ent.model = cg.baseClientInfo.model;
		ent.skin = cg.baseClientInfo.skin;
		ent.skinNum = 0;

		ent.flags = 0;
		ent.scale = 1;
		Vector4Set (ent.color, 255, 255, 255, 255);

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
static void V_TestLights (void)
{
	int			i;
	float		f, r;
	vec3_t		origin;
	float		red, green, blue;

	for (i=0 ; i<32 ; i++) {
		r = 64 * ((i%4) - 1.5f);
		f = 64 * (i/40.f) + 128;

		origin[0] = cg.refDef.viewOrigin[0] + cg.forwardVec[0]*f + cg.rightVec[0]*r;
		origin[1] = cg.refDef.viewOrigin[1] + cg.forwardVec[1]*f + cg.rightVec[1]*r;
		origin[2] = cg.refDef.viewOrigin[2] + cg.forwardVec[2]*f + cg.rightVec[2]*r;
			
		red = (float)(((i%6)+1) & 1);
		green = (float)((((i%6)+1) & 2)>>1);
		blue = (float)((((i%6)+1) & 4)>>2);

		cgi.R_AddLight (origin, 200, red, green, blue);
	}
}

// ====================================================================

/*
===============
V_CalcThirdPersonView
===============
*/
static void ClipCam (vec3_t start, vec3_t end, vec3_t newPos)
{
	trace_t	tr;
	vec3_t	mins, maxs;

	VectorSet (mins, -5, -5, -5);
	VectorSet (maxs, 5, 5, 5);

	tr = CG_PMTrace (start, mins, maxs, end, qTrue);

	newPos[0] = tr.endPos[0];
	newPos[1] = tr.endPos[1];
	newPos[2] = tr.endPos[2];
}
static void V_CalcThirdPersonView (void)
{
	vec3_t	end, camPosition;
	vec3_t	dir, newDir;
	float	upDist, backDist, angle;

	// Set the camera angle
	if (cg_thirdPersonAngle->modified) {
		cg_thirdPersonAngle->modified = qFalse;

		if (cg_thirdPersonAngle->value < 0.0f)
			cgi.Cvar_ForceSetValue ("cg_thirdPersonAngle", 0.0f);
	}

	// Set the camera distance
	if (cg_thirdPersonDist->modified) {
		cg_thirdPersonDist->modified = qFalse;

		if (cg_thirdPersonDist->value < 1.0f)
			cgi.Cvar_ForceSetValue ("cg_thirdPersonDist", 1.0f);
	}

	// Trig stuff
	angle = M_PI * (cg_thirdPersonAngle->value / 180.0f);
	upDist = cg_thirdPersonDist->value * sin (angle);
	backDist = cg_thirdPersonDist->value * cos (angle);

	// Move up
	VectorMA (cg.refDef.viewOrigin, -backDist, cg.forwardVec, end);
	VectorMA (end, upDist, cg.upVec, end);

	// Clip
	ClipCam (cg.refDef.viewOrigin, end, camPosition);

	// Adjust player transparency
	cg.cameraTrans = VectorDistFast (cg.refDef.viewOrigin, camPosition);
	if (cg.cameraTrans < cg_thirdPersonDist->value) {
		cg.cameraTrans = (cg.cameraTrans / cg_thirdPersonDist->value) * 255;

		if (cg.cameraTrans < 0)
			return;
		else if (cg.cameraTrans > 245)
			cg.cameraTrans = 245;
	}
	else {
		cg.cameraTrans = 255;
	}

	// Clip and adjust aim
	VectorMA (cg.refDef.viewOrigin, 8192, cg.forwardVec, dir);
	ClipCam (cg.refDef.viewOrigin, dir, newDir);

	VectorSubtract (newDir, camPosition, dir);
	VectorNormalize (dir, dir);
	VecToAngles (dir, newDir);

	// Apply
	AngleVectors (newDir, cg.forwardVec, cg.rightVec, cg.upVec);
	VectorCopy (newDir, cg.refDef.viewAngles);
	AnglesToAxis (cg.refDef.viewAngles, cg.refDef.viewAxis);

	VectorCopy (camPosition, cg.refDef.viewOrigin);
}


/*
===============
V_CalcViewValues

Sets cg.refDef view values
===============
*/
static void V_CalcViewValues (void)
{
	playerStateNew_t	*ps, *ops;
	cgEntity_t			*ent;
	int					i;

	// Set cg.lerpFrac
	if (cgi.Cvar_VariableInteger ("timedemo"))
		cg.lerpFrac = 1.0f;
	else
		cg.lerpFrac = 1.0f - (cg.frame.serverTime - cg.refreshTime)*0.01f;

	// Find the previous frame to interpolate from
	ps = &cg.frame.playerState;
	if (cg.oldFrame.serverFrame != cg.frame.serverFrame-1 || !cg.oldFrame.valid)
		ops = &cg.frame.playerState;	// previous frame was dropped or invalid
	else
		ops = &cg.oldFrame.playerState;

	// See if the player entity was teleported this frame
	if (fabs(ops->pMove.origin[0] - ps->pMove.origin[0]) > 256*8	||
		abs (ops->pMove.origin[1] - ps->pMove.origin[1]) > 256*8	||
		abs (ops->pMove.origin[2] - ps->pMove.origin[2]) > 256*8)
		ops = ps;		// don't interpolate

	ent = &cg_entityList[cg.playerNum+1];

	// Calculate the origin
	if (cl_predict->integer && !(cg.frame.playerState.pMove.pmFlags & PMF_NO_PREDICTION)) {
		// Use predicted values
		uInt		delta;
		float		backLerp;

		backLerp = 1.0f - cg.lerpFrac;
		for (i=0 ; i<3 ; i++) {
			cg.refDef.viewOrigin[i] = cg.predicted.origin[i] + ops->viewOffset[i] 
								+ cg.lerpFrac*(ps->viewOffset[i] - ops->viewOffset[i])
								- backLerp*cg.predicted.error[i];
		}

		// Smooth out stair climbing
		delta = cgs.realTime - cg.predicted.stepTime;
		if (delta < 100)
			cg.refDef.viewOrigin[2] -= cg.predicted.step * (100 - delta) * 0.01f;
	}
	else {
		// Just use interpolated values
		for (i=0 ; i<3 ; i++) {
			cg.refDef.viewOrigin[i] = ops->pMove.origin[i]*(1.0f/8.0f) + ops->viewOffset[i] 
								+ cg.lerpFrac*(ps->pMove.origin[i]*(1.0f/8.0f) + ps->viewOffset[i] 
								- (ops->pMove.origin[i]*(1.0f/8.0f) + ops->viewOffset[i]));
		}
	}

	// If not running a demo or on a locked frame, add the local angle movement
	if ((cg.frame.playerState.pMove.pmType < PMT_DEAD) && !cg.attractLoop) {
		// Use predicted values
		VectorCopy (cg.predicted.angles, cg.refDef.viewAngles);
	}
	else {
		// Just use interpolated values
		if (cg.frame.playerState.pMove.pmType >= PMT_DEAD && ops->pMove.pmType < PMT_DEAD) {
			cg.refDef.viewAngles[0] = InterpolateAngle (cg.predicted.angles[0], ps->viewAngles[0], cg.lerpFrac);
			cg.refDef.viewAngles[1] = InterpolateAngle (cg.predicted.angles[1], ps->viewAngles[1], cg.lerpFrac);
			cg.refDef.viewAngles[2] = InterpolateAngle (cg.predicted.angles[2], ps->viewAngles[2], cg.lerpFrac);
		}
		else {
			cg.refDef.viewAngles[0] = InterpolateAngle (ops->viewAngles[0], ps->viewAngles[0], cg.lerpFrac);
			cg.refDef.viewAngles[1] = InterpolateAngle (ops->viewAngles[1], ps->viewAngles[1], cg.lerpFrac);
			cg.refDef.viewAngles[2] = InterpolateAngle (ops->viewAngles[2], ps->viewAngles[2], cg.lerpFrac);
		}
	}

	cg.refDef.viewAngles[0] += InterpolateAngle (ops->kickAngles[0], ps->kickAngles[0], cg.lerpFrac);
	cg.refDef.viewAngles[1] += InterpolateAngle (ops->kickAngles[1], ps->kickAngles[1], cg.lerpFrac);
	cg.refDef.viewAngles[2] += InterpolateAngle (ops->kickAngles[2], ps->kickAngles[2], cg.lerpFrac);

	AngleVectors (cg.refDef.viewAngles, cg.forwardVec, cg.rightVec, cg.upVec);

	if (cg.refDef.viewAngles[0] || cg.refDef.viewAngles[1] || cg.refDef.viewAngles[2])
		AnglesToAxis (cg.refDef.viewAngles, cg.refDef.viewAxis);
	else
		Matrix_Identity (cg.refDef.viewAxis);

	// Interpolate field of view
	cg.refDef.fovX = ops->fov + cg.lerpFrac * (ps->fov - ops->fov);

	// Don't interpolate blend color
	Vector4Copy (ps->vBlend, cg.refDef.vBlend);

	// Offset if in third person
	if (cg.thirdPerson) {
		V_CalcThirdPersonView ();
	}
}


/*
==================
V_RenderView
==================
*/
#define FRAMETIME_MAX 0.5
void V_RenderView (int vidWidth, int vidHeight, int realTime, float netFrameTime, float refreshFrameTime, float stereoSeparation, qBool forceRefresh)
{
	// Video dimensions
	cg.vidWidth = vidWidth;
	cg.vidHeight = vidHeight;

	if (!cg.frame.valid) {
		SCR_DrawLoading ();
		return;
	}

	// Check cvar sanity
	CG_UpdateCvars ();

	// Calculate screen dimensions and clear the background
	V_CalcVrect ();
	V_TileClear ();

	// Set time
	cgs.realTime = realTime;

	cgs.netFrameTime = netFrameTime;
	cg.netTime += netFrameTime * 1000.0f;
	cgs.refreshFrameTime = refreshFrameTime;
	cg.refreshTime += refreshFrameTime * 1000.0f;

	// Clamp time
	cg.netTime = clamp (cg.netTime, cg.frame.serverTime - 100, cg.frame.serverTime);
	if (cgs.netFrameTime > FRAMETIME_MAX)
		cgs.netFrameTime = FRAMETIME_MAX;

	cg.refreshTime = clamp (cg.refreshTime, cg.frame.serverTime - 100, cg.frame.serverTime);
	if (cgs.refreshFrameTime > FRAMETIME_MAX)
		cgs.refreshFrameTime = FRAMETIME_MAX;

	// Predict all unacknowledged movements
	CG_PredictMovement ();

	// Watch for gender bending if desired
	CG_FixUpGender ();

	// Run light styles
	CG_RunLightStyles ();

	// An invalid frame will just use the exact previous refdef
	// We can't use the old frame if the video mode has changed, though
	if (cg.frame.valid && (forceRefresh || !cgi.Cvar_VariableInteger ("paused"))) {
		cgi.R_ClearScene ();

		// Calculate the view values
		V_CalcViewValues ();

		// Add in entities and effects
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

		// Offset the viewOrigin if we're using stereo separation
		if (stereoSeparation != 0)
			VectorMA (cg.refDef.viewOrigin, stereoSeparation, cg.rightVec, cg.refDef.viewOrigin);

		// Never let it sit exactly on a node line, because a water plane
		// can dissapear when viewed with the eye exactly on it. the server
		// protocol only specifies to 1/8 pixel, so add 1/16 in each axis
		cg.refDef.viewOrigin[0] += (1.0f/16.0f);
		cg.refDef.viewOrigin[1] += (1.0f/16.0f);
		cg.refDef.viewOrigin[2] += (1.0f/16.0f);

		cg.refDef.fovY		= CalcFov (cg.refDef.fovX, (float)cg.refDef.width, (float)cg.refDef.height);

		cg.refDef.time		= cg.refreshTime * 0.001f;
		cg.refDef.areaBits	= cg.frame.areaBits;

		cg.refDef.rdFlags	= cg.frame.playerState.rdFlags;
	}

	// Render the frame
	cgi.R_RenderScene (&cg.refDef);

	// Update orientation for sound subsystem
	cgi.Snd_Update (cg.refDef.viewOrigin, cg.forwardVec, cg.rightVec, cg.upVec, (cg.refDef.rdFlags & RDF_UNDERWATER) ? qTrue : qFalse);

	// Run subsystems
	CG_RunDLights ();

	// Render screen stuff
	SCR_Draw ();

	// Increment frame counter
	cgs.frameCount++;
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
static void V_Viewpos_f (void)
{
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

static cmdFunc_t	*cmd_sizeUp;
static cmdFunc_t	*cmd_sizeDown;
static cmdFunc_t	*cmd_viewPos;

/*
=============
V_Register
=============
*/
void V_Register (void)
{
	cl_testblend		= cgi.Cvar_Get ("cl_testblend",			"0",		0);
	cl_testentities		= cgi.Cvar_Get ("cl_testentities",		"0",		0);
	cl_testlights		= cgi.Cvar_Get ("cl_testlights",		"0",		0);
	cl_testparticles	= cgi.Cvar_Get ("cl_testparticles",		"0",		0);

	viewsize			= cgi.Cvar_Get ("viewsize",				"100",		CVAR_ARCHIVE);

	cmd_sizeUp		= CGI_Cmd_AddCommand ("sizeup",		V_SizeUp_f,		"Increases viewport size");
	cmd_sizeDown	= CGI_Cmd_AddCommand ("sizedown",	V_SizeDown_f,	"Decreases viewport size");

	cmd_viewPos		= CGI_Cmd_AddCommand ("viewpos",	V_Viewpos_f,	"Prints view position and yaw");
}


/*
=============
V_Unregister
=============
*/
void V_Unregister (void)
{
	CGI_Cmd_RemoveCommand ("sizeup", cmd_sizeUp);
	CGI_Cmd_RemoveCommand ("sizedown", cmd_sizeDown);

	CGI_Cmd_RemoveCommand ("viewpos", cmd_viewPos);
}
