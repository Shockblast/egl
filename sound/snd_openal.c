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

#include "snd_loc.h"

/*
** Sources store locations, directions, and other attributes of
** an object in 3D space and have a buffer associated with them
** for playback. There are normally far more sources defined than
** buffers. When the program wants to play a sound, it controls
** execution through a source object. Sources are processed
** independently from each other.
*/

/*
** Buffers store compressed or un-compressed audio data. It is
** common to initialize a large set of buffers when the program
** first starts (or at non-critical times during execution --
** between levels in a game, for instance). Buffers are referred to
** by Sources. Data (audio sample data) is associated with buffers.
*/

/*
** There is only one listener (per audio context). The listener
** attributes are similar to source attributes, but are used to
** represent where the user is hearing the audio from. The influence
** of all the sources from the perspective of the listener is mixed
** and played for the user.
*/

qBool			snd_ALActive;
void			*snd_ALDevice;

unsigned int	snd_MaxALSources = 0;
unsigned int	snd_MaxALBuffers = 0;

ALuint			snd_ALSources[MAX_OPENAL_SOURCES];
OpenALBuffer_t	snd_ALBuffers[MAX_OPENAL_BUFFERS];
OpenALIndex_t	snd_ALIndexes[MAX_SOUNDS];

/*
==============
OpenAL_CheckForError
==============
*/
static const char *GetALErrorString (ALenum err) {
	switch (err) {
	case AL_NO_ERROR:		    return ("AL_NO_ERROR");				break;
	case AL_INVALID_NAME:		return ("AL_INVALID_NAME");			break;
	case AL_INVALID_ENUM:		return ("AL_INVALID_ENUM");			break;
	case AL_INVALID_VALUE:		return ("AL_INVALID_VALUE");		break;
	case AL_INVALID_OPERATION:	return ("AL_INVALID_OPERATION");	break;
	case AL_OUT_OF_MEMORY:		return ("AL_OUT_OF_MEMORY");		break;
	default:					return ("UNKNOWN");					break;
	}
}
void OpenAL_CheckForError (void) {
	int		error;
	if ((error = alGetError()) != AL_NO_ERROR)
		Com_Printf (PRINT_ALL, S_COLOR_RED "OpenAL: ERROR: %s\n", GetALErrorString (error));
}


/*
==============
DisplayALError

Show and clear OpenAL Library error message, if any.
==============
*/
void DisplayALError (const char *msg, int code) {
	Com_Printf (PRINT_ALL, S_COLOR_RED "OpenAL: Error: %s(%x)\n", msg, code);
}


/*
==============
OpenAL_DestroyBuffers
==============
*/
void OpenAL_DestroyBuffers (void) {
	int		i;

	Com_Printf (PRINT_DEV, "Destroying OpenAL buffers\n");
	for (i=0 ; i<snd_MaxALSources ; i++)
		alSourceStop (snd_ALSources[i]);
}


/*
==============
OpenAL_GetFreeALIndex
==============
*/
int OpenAL_GetFreeALIndex (void) {
	int		i;
	for (i=0 ; i<MAX_SOUNDS ; i++) {
		if (!snd_ALIndexes[i].inUse)
			return i;
	}

	return -1;
}


/*
==============
OpenAL_FreeALIndexes
==============
*/
void OpenAL_FreeALIndexes (int index) {
	int		i;
	for (i=0 ; i<MAX_SOUNDS ; i++) {
		if (snd_ALIndexes[i].sourceIndex == index)
			snd_ALIndexes[i].inUse = qFalse;
	}	
}


/*
==============
OpenAL_GetFreeSource
==============
*/
ALint OpenAL_GetFreeSource (void) {
	ALenum	state;
	int		i;

	for (i=0 ; i<snd_MaxALSources ; i++) {
		alGetSourcei (snd_ALSources[i], AL_SOURCE_STATE, &state);
		OpenAL_CheckForError ();
		if ((state == AL_STOPPED) || (state == AL_INITIAL)) {
			OpenAL_FreeALIndexes (i);
			return i;
		}
	}

	return -1;
}


/*
===============
OpenAL_Shutdown

Shut down the OpenAL interface, if it is running.
===============
*/
void OpenAL_Shutdown (void) {
	int			i;
	int			error;
	ALCcontext	*Context;

	if (!snd_ALActive)
		return;

	for (i=0 ; i<snd_MaxALSources ; i++) {
		alSourceStop (snd_ALSources[i]);
		OpenAL_CheckForError ();

		alSourcei (snd_ALSources[i], AL_BUFFER, 0);
		OpenAL_CheckForError ();
	}

	alDeleteSources (snd_MaxALSources, snd_ALSources);
	OpenAL_CheckForError ();

	for (i=0 ; i<snd_MaxALBuffers ; i++) {
		if (snd_ALBuffers[i].inUse) {
			alDeleteBuffers (1, &snd_ALBuffers[i].buffer);

			if ((error = alGetError()) != AL_NO_ERROR) {
				DisplayALError ("OpenAL_Shutdown: alDeleteBuffers: ", error);
				return;
			}
		}
	}

	snd_MaxALSources = 0;
	snd_MaxALBuffers = 0;

	memset (snd_ALIndexes, 0, sizeof (snd_ALIndexes));

	// get active context
	Context = alcGetCurrentContext ();

	// disable context
	alcMakeContextCurrent (NULL);

	// release context(s)
	alcDestroyContext (Context);

	// close device
	alcCloseDevice (snd_ALDevice);

	snd_ALActive = qFalse;
	Com_Printf (PRINT_DEV, "OpenAL: Shutdown complete\n");
}


/*
============
OpenAL_GetFreeBuffer
============
*/
ALint OpenAL_GetFreeBuffer (void) {
	int		i;
	for (i=0 ; i<snd_MaxALBuffers ; i++) {
		if (!snd_ALBuffers[i].inUse) {
			snd_ALBuffers[i].inUse = qTrue;
			return i;
		}
	}

	return -1;
}


/*
============
OpenAL_Init

Try to initialize the OpenAL library. Return 0 on success or non-zero on failure.
Error message will be automatically printed if initialization fails.
============
*/
qBool OpenAL_Init (void) {
	int			i;
	int			major, minor;
	ALint		error;
	ALCcontext	*Context;

	if (snd_ALActive) {
		Com_Printf (PRINT_ALL, S_COLOR_RED "Error: OpenAL_Init when snd_ALActive = qTrue\n");
		return qFalse;
	}

	snd_ALDevice = alcOpenDevice ((ALubyte*)"DirectSound3D");
	if (snd_ALDevice == NULL) {
		Com_Printf (PRINT_ALL, S_COLOR_RED "OpenAL: DirectSound3D unavailable; cannot continue\n");
		return qFalse;
	}

	// create context(s)
	Context = alcCreateContext (snd_ALDevice, NULL);
	if (Context == NULL) {
		Com_Printf (PRINT_ALL, S_COLOR_RED "OpenAL: unable to create rendering context; cannot continue\n");
		return qFalse;
	}

	// set active context
	alcMakeContextCurrent (Context);
	if (alcGetError (snd_ALDevice) != ALC_NO_ERROR) {
		Com_Printf (PRINT_ALL, S_COLOR_RED "OpenAL: unable to activate rendering context; cannot continue\n");
		return qFalse;
	}

	// clear error Codes
	alGetError ();
	alcGetError (snd_ALDevice);

	/*
	** set the listener attributes
	*/

	// position
	alListenerfv (AL_POSITION, cl.refDef.viewOrigin);
	if ((error = alGetError ()) != AL_NO_ERROR) {
		DisplayALError ("OpenAL_Init: alListenerfv POSITION: ", error);
		return qFalse;
	}

	// orientation
	alListenerfv (AL_ORIENTATION, vec3Identity);
	if ((error = alGetError()) != AL_NO_ERROR) {
		DisplayALError ("OpenAL_Init: alListenerfv ORIENTATION: ", error);
		return qFalse;
	}

	// print out version
	alcGetIntegerv (snd_ALDevice, ALC_MAJOR_VERSION, sizeof (major), &major);
	alcGetIntegerv (snd_ALDevice, ALC_MINOR_VERSION, sizeof (minor), &minor);
	Com_Printf (PRINT_ALL, "OpenAL %d.%d on %s\n", major, minor, alcGetString (snd_ALDevice, ALC_DEFAULT_DEVICE_SPECIFIER));

	// generate buffers
	for (i=0 ; i<MAX_OPENAL_BUFFERS ; i++) {
		alGenBuffers(1, &snd_ALBuffers[i].buffer);
		if ((error = alGetError()) != AL_NO_ERROR) {
			DisplayALError ("OpenAL_Inital: GenBuffers: ", error);
			return qFalse;
		}
	}

	snd_MaxALBuffers = i;
	Com_Printf (PRINT_ALL, "...generated %d buffers\n", snd_MaxALBuffers);

	// generate as many sources as possible
	for (i=0 ; i<MAX_OPENAL_SOURCES ; i++) {
		alGenSources (1, &snd_ALSources[i]);
		if ((error = alGetError()) != AL_NO_ERROR)
			break;
	}

	snd_MaxALSources = i;
	Com_Printf (PRINT_ALL, "...generated %d sources\n", snd_MaxALSources);

	if (snd_MaxALSources <= 32)
		Com_Printf (PRINT_ALL, "WARNING: Only %d sources available. This probably isn't enough, it is recommended you go back to DirectSound\n", snd_MaxALSources);

	snd_ALActive = qTrue;

	return qTrue;
}
