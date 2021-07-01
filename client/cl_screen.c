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
// cl_screen.c
// Client master for refresh
//

#include "cl_local.h"

static qBool	scrDrawLoading;

/*
=======================================================================

	GRAPHS

=======================================================================
*/

// need this to negotiate the values the game dll sends to rgb values
static byte	graphPal[] = {
	#include "graphpal.h"
};

float	palRedf		(int color) { return ((graphPal[color*3+0] > 0) ? graphPal[color*3+0] * ONEDIV255 : 0); }
float	palGreenf	(int color) { return ((graphPal[color*3+1] > 0) ? graphPal[color*3+1] * ONEDIV255 : 0); }
float	palBluef	(int color) { return ((graphPal[color*3+2] > 0) ? graphPal[color*3+2] * ONEDIV255 : 0); }

typedef struct graphStamp_s {
	float	value;
	int		color;
} graphStamp_t;

static int			graphCurrent;
static graphStamp_t	graphValues[1024];
#define GRAPHMASK	(1024-1)

/*
==============
CL_AddNetgraph

A new packet was just parsed
==============
*/
void CL_AddNetgraph (void) {
	int		i, in, ping;

	// if using the debuggraph for something else, don't add the net lines
	if (scr_debuggraph->value || scr_timegraph->value)
		return;

	for (i=0 ; i<cls.netChan.dropped ; i++)
		SCR_DebugGraph (30, 0x40);

	for (i=0 ; i<cl.surpressCount ; i++)
		SCR_DebugGraph (30, 0xdf);

	// see what the latency was on this packet
	in = cls.netChan.incomingAcknowledged & CMD_MASK;
	ping = cls.realTime - cl.cmdTime[in];

	ping /= 30;
	if (ping > 30)
		ping = 30;

	SCR_DebugGraph (ping, 0xd0);
}


/*
==============
SCR_DebugGraph
==============
*/
void SCR_DebugGraph (float value, int color) {
	graphValues[graphCurrent & GRAPHMASK].value = value;
	graphValues[graphCurrent & GRAPHMASK].color = color;
	graphCurrent++;
}


/*
==============
SCR_DrawDebugGraph
==============
*/
void SCR_DrawDebugGraph (void) {
	int		a, i, h;
	float	v;
	vec4_t	color;

	if (!scr_debuggraph->value && !scr_timegraph->value && !scr_netgraph->value)
		return;

	// draw the graph
	Vector4Set (color, colorDkGrey[0], colorDkGrey[1], colorDkGrey[2], scr_graphalpha->value);
	Draw_Fill (0, vidDef.height-scr_graphheight->value, vidDef.width, scr_graphheight->value, color);

	for (a=0 ; a<vidDef.width ; a++) {
		i = (graphCurrent-1-a+1024) & GRAPHMASK;
		v = graphValues[i].value;
		v = v*scr_graphscale->value + scr_graphshift->value;
		
		if (v < 0)
			v += scr_graphheight->value * (1+(int)(-v/scr_graphheight->value));

		h = (int)v % scr_graphheight->integer;
		VectorSet (color, palRedf (graphValues[i].color), palGreenf (graphValues[i].color), palBluef (graphValues[i].color));
		Draw_Fill (vidDef.width-1-a, vidDef.height-h, 1, h, color);
	}
}

// ====================================================================

/*
================
SCR_BeginLoadingPlaque
================
*/
void SCR_BeginLoadingPlaque (void) {
	// stop audio
	cls.soundPrepped = qFalse;
	Snd_StopAllSounds ();
	CDAudio_Stop ();

	// get rid of the console
	con.dropHeight = 0;

	// update the screen
	scrDrawLoading = qTrue;
	SCR_UpdateScreen ();
	cls.pauseScreen = qTrue;

	CL_ShutdownMedia ();
}


/*
================
SCR_EndLoadingPlaque
================
*/
void SCR_EndLoadingPlaque (void) {
	cls.soundPrepped = qTrue;

	Con_ClearNotify ();

	CL_InitMedia ();
}

/*
=======================================================================

	FRAMERATE BENCHMARKING

=======================================================================
*/

/*
================
SCR_Benchmark_f
================
*/
static void SCR_Benchmark_f (void) {
	int			i, j, times;
	int			start, stop;
	float		time, timeinc;
	float		result;
	refDef_t	refDef;

	memset (&refDef, 0, sizeof (refDef_t));

	refDef.width = vidDef.width;
	refDef.height = vidDef.height;
	refDef.fovX = 90;
	refDef.fovY = CalcFov (refDef.fovX, refDef.width, refDef.height);
	VectorMA (cl.frame.playerState.viewOffset, ONEDIV8, cl.frame.playerState.pMove.origin, refDef.viewOrigin);

	if (Cmd_Argc () >= 2)
		times = atoi (Cmd_Argv (1));
	else
		times = 10;

	for (j=0, result=0, timeinc=0 ; j<times ; j++) {
		start = Sys_Milliseconds ();

		if (Cmd_Argc () >= 3) {
			// run without page flipping
			R_BeginFrame (0);
			for (i=0 ; i<128 ; i++) {
				refDef.viewAngles[1] = i/128.0*360.0;
				AnglesToAxis (refDef.viewAngles, refDef.viewAxis);

				R_RenderScene (&refDef);
			}
			R_EndFrame ();
		}
		else {
			for (i=0 ; i<128 ; i++) {
				refDef.viewAngles[1] = i/128.0*360.0;
				AnglesToAxis (refDef.viewAngles, refDef.viewAxis);

				R_BeginFrame (0);
				R_RenderScene (&refDef);
				R_EndFrame ();
			}
		}

		stop = Sys_Milliseconds ();
		time = (stop - start) / 1000.0;
		timeinc += time;
		result += 128.0 / time;
	}

	Com_Printf (0, "%f seconds, %f average seconds (%f fps)\n", timeinc, timeinc/times, result/times);
}


/*
================
SCR_TimeRefresh_f
================
*/
static void SCR_TimeRefresh_f (void) {
	int			i, start, stop;
	float		time;
	refDef_t	refDef;

	start = Sys_Milliseconds ();

	memset (&refDef, 0, sizeof (refDef_t));

	refDef.width = vidDef.width;
	refDef.height = vidDef.height;
	refDef.fovX = 90;
	refDef.fovY = CalcFov (refDef.fovX, refDef.width, refDef.height);
	VectorMA (cl.frame.playerState.viewOffset, ONEDIV8, cl.frame.playerState.pMove.origin, refDef.viewOrigin);

	if (Cmd_Argc () == 2) {
		// run without page flipping
		R_BeginFrame (0);
		for (i=0 ; i<128 ; i++) {
			refDef.viewAngles[1] = i/128.0*360.0;
			AnglesToAxis (refDef.viewAngles, refDef.viewAxis);

			R_RenderScene (&refDef);
		}
		R_EndFrame ();
	}
	else {
		for (i=0 ; i<128 ; i++) {
			refDef.viewAngles[1] = i/128.0*360.0;
			AnglesToAxis (refDef.viewAngles, refDef.viewAxis);

			R_BeginFrame (0);
			R_RenderScene (&refDef);
			R_EndFrame ();
		}
	}

	stop = Sys_Milliseconds ();
	time = (stop - start) / 1000.0;
	Com_Printf (0, "%f seconds (%f fps)\n", time, 128/time);
}

/*
=======================================================================

	INITIALIZATION

=======================================================================
*/

/*
==================
SCR_Init
==================
*/
void SCR_Init (void) {
	Cmd_AddCommand ("benchmark",	SCR_Benchmark_f,		"Multiple speed tests for one scene");
	Cmd_AddCommand ("timerefresh",	SCR_TimeRefresh_f,		"Prints framerate of current scene");

	cls.screenInitialized = qTrue;
}

/*
=======================================================================

	DRAW EVERYTHING

=======================================================================
*/

/*
==================
SCR_RenderView
==================
*/
void SCR_RenderView (float stereoSeparation) {
	if (cl_timedemo->integer) {
		if (!cl.timeDemoStart)
			cl.timeDemoStart = Sys_Milliseconds ();
		cl.timeDemoFrames++;
	}

	// only render the frame when map is loaded
	if (CM_ClientLoad ())
		CL_CGModule_RenderView (stereoSeparation);
}


/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush text to the screen.
==================
*/
void SCR_UpdateScreen (void) {
	int		i, numFrames;
	float	separation[2];

	// if the screen is disabled do nothing at all
	if (cls.disableScreen || cls.pauseScreen) {
		cls.pauseScreen = qFalse;
		return;
	}

	if (!clMedia.initialized || !con.initialized || !cls.screenInitialized)
		return;		// not initialized yet

	if (cl_stereo->integer) {
		numFrames = 2;

		// range check cl_camera_separation so we don't inadvertently fry someone's brain
		Cvar_VariableForceSetValue (cl_stereo_separation, clamp (cl_stereo_separation->value, 0.0, 1.0));

		separation[0] = -cl_stereo_separation->value * 0.5;
		separation[1] = cl_stereo_separation->value * 0.5;
	}
	else {
		numFrames = 1;
		separation[0] = separation[1] = 0;
	}

	for (i=0 ; i<numFrames ; i++) {
		R_BeginFrame (separation[i]);

		if (scrDrawLoading) {
			// clear palette and draw loading bar
			scrDrawLoading = qFalse;
			CL_UIModule_DrawConnectScreen (qTrue, qFalse);
		}
		else if (cl.cin.time > 0) {
			CIN_DrawCinematic ();
		}
		else {
			switch (cls.connectState) {
			case CA_DISCONNECTED:
				CL_UIModule_Refresh (qTrue);
				break;

			case CA_CONNECTING:
			case CA_CONNECTED:
				CL_UIModule_DrawConnectScreen (qTrue, qFalse);
				break;

			case CA_LOADING:
				SCR_RenderView (separation[i]);
				CL_UIModule_DrawConnectScreen (qFalse, qTrue);
				break;

			case CA_ACTIVE:
				// do 3D refresh drawing, and then update the screen
				SCR_RenderView (separation[i]);

				CL_UIModule_Refresh (qFalse);

				if (scr_timegraph->value)
					SCR_DebugGraph (cls.netFrameTime*300, 0);
				SCR_DrawDebugGraph ();
				break;
			}
		}

		Con_DrawConsole ();
	}

	R_EndFrame ();
}
