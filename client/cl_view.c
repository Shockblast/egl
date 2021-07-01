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

// cl_view.c
// - player rendering positioning

#include "cl_local.h"

int				gun_frame;
struct model_s	*gun_model;

cvar_t			*cl_testparticles;
cvar_t			*cl_testentities;
cvar_t			*cl_testlights;
cvar_t			*cl_testblend;

/*
==============
CL_ClearEffects
==============
*/
void CL_ClearEffects (void) {
	CL_ClearCLEnts ();
	CL_ClearDecals ();
	CL_ClearParticles ();
	CL_ClearDLights ();
	CL_ClearLightStyles ();
	CL_ClearTEnts ();
}

// ====================================================================

/*
================
View_TestParticles
================
*/
void View_TestParticles (void) {
	int			i, j;
	float		d, r, u;
	vec3_t		origin;
	dvec4_t		color;

	color[0] = 255;
	color[1] = 255;
	color[2] = 255;
	color[3] = 1;
	for (i=0 ; i<MAX_PARTICLES ; i++) {
		d = i*0.25;
		r = 4*((i&7)-3.5);
		u = 4*(((i>>3)&7)-3.5);

		for (j=0 ; j<3 ; j++)
			origin[j] = cl.refDef.viewOrigin[j] + cl.v_Forward[j]*d + cl.v_Right[j]*r + cl.v_Up[j]*u;

		R_AddParticle (origin, vec3Identity, color, 1, clMedia.mediaTable[i%PDT_PICTOTAL], 0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, (i*2)%361);
	}
}


/*
================
View_TestEntities
================
*/
void View_TestEntities (void) {
	int			i, j;
	float		f, r;
	entity_t	ent;

	for (i=0 ; i<32 ; i++) {
		r = 64 * ((i%4) - 1.5);
		f = 64 * (i/4) + 128;

		for (j=0 ; j<3 ; j++) {
			ent.origin[j] = cl.refDef.viewOrigin[j] + cl.v_Forward[j]*f + cl.v_Right[j]*r;
			ent.oldOrigin[j] = ent.origin[j];
		}

		Matrix_Identity (ent.axis);

		ent.model = cl.baseClientInfo.model;
		ent.skin = cl.baseClientInfo.skin;
		ent.skinNum = 0;

		ent.flags = 0;
		ent.scale = 1;
		Vector4Set (ent.color, 1, 1, 1, 1);

		ent.backLerp = 0;
		ent.frame = 0;
		ent.oldFrame = 0;

		R_AddEntity (&ent);
	}
}


/*
================
View_TestLights
================
*/
void View_TestLights (void) {
	int			i, j;
	float		f, r;
	vec3_t		origin;
	float		red, green, blue;

	for (i=0 ; i<32 ; i++) {
		r = 64 * ((i%4) - 1.5);
		f = 64 * (i/4) + 128;

		for (j=0 ; j<3 ; j++)
			origin[j] = cl.refDef.viewOrigin[j] + cl.v_Forward[j]*f + cl.v_Right[j]*r;
			
		red = ((i%6)+1) & 1;
		green = (((i%6)+1) & 2)>>1;
		blue = (((i%6)+1) & 4)>>2;

		R_AddLight (origin, 200, red, green, blue);
	}
}

// ====================================================================

/*
==================
View_RenderView
==================
*/
void View_RenderView (vRect_t vRect, float stereoSeparation) {
	if (!CL_ConnectionState (CA_ACTIVE))
		return;

	if (!cl.refreshPrepped)
		return;		// still loading

	if (cl_timedemo->value) {
		if (!cl.timeDemoStart)
			cl.timeDemoStart = Sys_Milliseconds ();
		cl.timeDemoFrames++;
	}

	// an invalid frame will just use the exact previous refdef
	// we can't use the old frame if the video mode has changed, though
	if (cl.frame.valid && (cl.forceRefresh || !cl_paused->value)) {
		cl.forceRefresh = qFalse;

		R_ClearFrame ();

		CL_AddEntities ();
		CL_AddCLEnts ();
		CL_AddTEnts ();
		CL_AddParticles ();
		CL_AddDecals ();
		CL_AddDLights ();
		CL_AddLightStyles ();

		if (cl_testparticles->integer)
			View_TestParticles ();
		if (cl_testentities->integer)
			View_TestEntities ();
		if (cl_testlights->integer)
			View_TestLights ();
		if (cl_testblend->integer) {
			cl.refDef.vBlend[0] = 1;
			cl.refDef.vBlend[1] = 0.5;
			cl.refDef.vBlend[2] = 0.25;
			cl.refDef.vBlend[3] = 0.5;
		}

		// offset the viewOrigin if we're using stereo separation
		if (stereoSeparation != 0) {
			vec3_t	tmp;

			VectorScale (cl.v_Right, stereoSeparation, tmp);
			VectorAdd (cl.refDef.viewOrigin, tmp, cl.refDef.viewOrigin);
		}

		// never let it sit exactly on a node line, because a water plane
		// can dissapear when viewed with the eye exactly on it. the server
		// protocol only specifies to 1/8 pixel, so add 1/16 in each axis
		cl.refDef.viewOrigin[0] += ONEDIV16;
		cl.refDef.viewOrigin[1] += ONEDIV16;
		cl.refDef.viewOrigin[2] += ONEDIV16;

		cl.refDef.x			= vRect.x;
		cl.refDef.y			= vRect.y;
		cl.refDef.width		= vRect.width;
		cl.refDef.height	= vRect.height;

		cl.refDef.fovY		= CalcFov (cl.refDef.fovX, cl.refDef.width, cl.refDef.height);

		cl.refDef.time		= cl.time * 0.001;

		cl.refDef.areaBits	= cl.frame.areaBits;

		cl.refDef.rdFlags	= cl.frame.playerState.rdFlags;
	}

	R_RenderFrame (&cl.refDef);
}

/*
=======================================================================

	FUNCTIONS

=======================================================================
*/

/*
=============
View_Viewpos_f
=============
*/
void View_Viewpos_f (void) {
	Com_Printf (PRINT_ALL, "(%i %i %i) : %i\n",
				(int)cl.refDef.viewOrigin[0], (int)cl.refDef.viewOrigin[1], (int)cl.refDef.viewOrigin[2], 
				(int)cl.refDef.viewAngles[YAW]);
}


/*
=============
View_Gun_FrameNext_f
=============
*/
void View_Gun_FrameNext_f (void) {
	Com_Printf (PRINT_ALL, "Gun frame %i\n", ++gun_frame);
}


/*
=============
View_Gun_FramePrev_f
=============
*/
void View_Gun_FramePrev_f (void) {
	if (--gun_frame < 0)
		gun_frame = 0;
	Com_Printf (PRINT_ALL, "Gun frame %i\n", gun_frame);
}


/*
=============
View_Gun_Model_f
=============
*/
void View_Gun_Model_f (void) {
	char	name[MAX_QPATH];

	if (Cmd_Argc () != 2) {
		gun_model = NULL;
		return;
	}

	Com_sprintf (name, sizeof (name), "models/%s/tris.md2", Cmd_Argv (1));
	gun_model = Mod_RegisterModel (name);
}

/*
=======================================================================

	INIT / SHUTDOWN

=======================================================================
*/

/*
=============
View_Init
=============
*/
void View_Init (void) {
	// cvars
	cl_testblend		= Cvar_Get ("cl_testblend",		"0",	0);
	cl_testparticles	= Cvar_Get ("cl_testparticles",	"0",	0);
	cl_testentities		= Cvar_Get ("cl_testentities",	"0",	0);
	cl_testlights		= Cvar_Get ("cl_testlights",	"0",	0);

	// commands
	Cmd_AddCommand ("gun_next",		View_Gun_FrameNext_f);
	Cmd_AddCommand ("gun_prev",		View_Gun_FramePrev_f);
	Cmd_AddCommand ("gun_model",	View_Gun_Model_f);

	Cmd_AddCommand ("viewpos",		View_Viewpos_f);
}


/*
=============
View_Shutdown
=============
*/
void View_Shutdown (void) {
	Cmd_RemoveCommand ("gun_next");
	Cmd_RemoveCommand ("gun_prev");
	Cmd_RemoveCommand ("gun_model");

	Cmd_RemoveCommand ("viewpos");
}
