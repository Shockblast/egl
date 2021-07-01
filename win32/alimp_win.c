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

// ALIMP_WIN.C
// This file contains ALL Win32 specific stuff having to do with OpenAL

#include "../sound/snd_loc.h"
#include "alimp_win.h"
#include "winquake.h"

alwState_t	alwState;

/*
=================
ALimp_Init
=================
*/
qBool ALimp_Init (void) {
	if ((alwState.hDevice = qalcOpenDevice ((ALubyte *)"DirectSound3D")) == NULL) {
		Com_Printf (0, S_COLOR_RED "OpenAL_Init: DirectSound3D unavailable; cannot continue\n");
		return qFalse;
	}

	// create context(s)
	if ((alwState.hALC = qalcCreateContext (alwState.hDevice, NULL)) == NULL) {
		Com_Printf (0, S_COLOR_RED "OpenAL_Init: unable to create rendering context; cannot continue\n");
		return qFalse;
	}

	// set active context
	qalcMakeContextCurrent (alwState.hALC);
	if (qalcGetError (alwState.hDevice) != ALC_NO_ERROR) {
		Com_Printf (0, S_COLOR_RED "OpenAL_Init: unable to activate rendering context; cannot continue\n");
		return qFalse;
	}

	// clear error codes
	qalGetError ();
	qalcGetError (alwState.hDevice);

	// get our strings
	alConfig.rendString = qalGetString (AL_RENDERER);
	alConfig.vendString = qalGetString (AL_VENDOR);
	alConfig.versString = qalGetString (AL_VERSION);
	alConfig.extsString = qalGetString (AL_EXTENSIONS);

	// print out version
	qalcGetIntegerv (alwState.hDevice, ALC_MAJOR_VERSION, sizeof (alConfig.verMajor), &alConfig.verMajor);
	qalcGetIntegerv (alwState.hDevice, ALC_MINOR_VERSION, sizeof (alConfig.verMinor), &alConfig.verMinor);
	alConfig.device = qalcGetString (alwState.hDevice, ALC_DEVICE_SPECIFIER);
	Com_Printf (0, "OpenAL %d.%d on %s\n", alConfig.verMajor, alConfig.verMinor, alConfig.device);

	return qTrue;
}


/*
=================
ALimp_Shutdown
=================
*/
void ALimp_Shutdown (void) {
	// disable context
	qalcMakeContextCurrent (NULL);

	// release context(s)
	qalcDestroyContext (alwState.hALC);
	alwState.hALC = NULL;

	// close device
	qalcCloseDevice (alwState.hDevice);
	alwState.hDevice = NULL;

	memset(&alConfig, 0, sizeof(alConfig_t));
	memset(&alwState, 0, sizeof(alwState_t));
}
