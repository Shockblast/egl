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

/*
=============================================================================

	LOCAL RENDERING FUNCTIONS

=============================================================================
*/

/*
================
CL_DrawFill
================
*/
void CL_DrawFill (float x, float y, int w, int h, vec4_t color)
{
	R_DrawPic (clMedia.whiteTexture, 0, x, y, w, h, 0, 0, 1, 1, color);
}

/*
=============================================================================

	FRAME HANDLING

=============================================================================
*/

/*
================
SCR_BeginLoadingPlaque
================
*/
void SCR_BeginLoadingPlaque (void)
{
	// Stop audio
	cls.soundPrepped = qFalse;
	cls.refreshPrepped = qFalse;
	Snd_StopAllSounds ();
	CDAudio_Stop ();

	// Update connection info
	CL_CGModule_UpdateConnectInfo ();

	// Update the screen
	SCR_UpdateScreen ();

	// Shutdown media
	CL_MediaShutdown ();
}


/*
================
SCR_EndLoadingPlaque
================
*/
void SCR_EndLoadingPlaque (void)
{
	// Clear the notify lines
	CL_ClearNotifyLines ();

	// Load media
	CL_MediaInit ();
}


/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush text to the screen.
==================
*/
void SCR_UpdateScreen (void)
{
	int		i, numFrames;
	float	separation[2];

	// If the screen is disabled do nothing at all
	if (cls.disableScreen)
		return;

	// Not initialized yet
	if (!clMedia.initialized)
		return;

	// Set separation values
	if (cls.refConfig.stereoEnabled) {
		numFrames = 2;

		// Range check cl_camera_separation so we don't inadvertently fry someone's brain
		Cvar_VariableSetValue (cl_stereo_separation, clamp (cl_stereo_separation->floatVal, 0.0, 1.0), qTrue);

		separation[0] = -cl_stereo_separation->floatVal * 0.5f;
		separation[1] = cl_stereo_separation->floatVal * 0.5f;
	}
	else {
		numFrames = 1;
		separation[0] = separation[1] = 0;
	}

	// Update connection info
	switch (Com_ClientState ()) {
	case CA_CONNECTING:
	case CA_CONNECTED:
		CL_CGModule_UpdateConnectInfo ();
		break;
	}

	// Render frame(s)
	for (i=0 ; i<numFrames ; i++) {
		R_BeginFrame (separation[i]);

		if (cl.cin.time > 0) {
			// Render the cinematic
			CIN_DrawCinematic ();
			GUI_Refresh ();
		}
		else {
			// Time demo update
			if (Com_ClientState() == CA_ACTIVE && cl_timedemo->intVal) {
				if (!cl.timeDemoStart)
					cl.timeDemoStart = Sys_Milliseconds ();
				cl.timeDemoFrames++;
			}

			// Render the scene
			CL_CGModule_RenderView (separation[i]);
			GUI_Refresh ();
		}

		CL_DrawConsole ();
	}

	R_EndFrame ();
}

