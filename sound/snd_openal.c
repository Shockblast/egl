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
// snd_openal.c
//

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

alConfig_t		alConfig;

qBool			sndALActive;

uInt			alMaxSources = 0;
uInt			alMaxBuffers = 0;

ALuint			sndALSources[OPENAL_MAX_SOURCES];
OpenALBuffer_t	sndALBuffers[OPENAL_MAX_BUFFERS];
OpenALIndex_t	sndALIndexes[MAX_CS_SOUNDS];

/*
==============
OpenAL_CheckForError

Returns qTrue if there was an error
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
qBool OpenAL_CheckForError (void) {
	int		error;

	if ((error = qalGetError()) != AL_NO_ERROR) {
		Com_Printf (0, S_COLOR_RED "OpenAL: ERROR: %s\n", GetALErrorString (error));
		return qTrue;
	}

	return qFalse;
}


/*
==============
DisplayALError

Show and clear OpenAL Library error message, if any.
==============
*/
void DisplayALError (const char *msg, int code) {
	Com_Printf (0, S_COLOR_RED "OpenAL: Error: %s(%x)\n", msg, code);
}


/*
==============
OpenAL_DestroyBuffers
==============
*/
void OpenAL_DestroyBuffers (void) {
	int		i;

	Com_Printf (PRNT_DEV, "Destroying OpenAL buffers\n");
	for (i=0 ; i<alMaxSources ; i++) {
		qalSourceStop (sndALSources[i]);
		qalSourcei (sndALSources[i], AL_BUFFER, 0);
	}
}


/*
==============
OpenAL_GetFreeALIndex
==============
*/
int OpenAL_GetFreeALIndex (void) {
	int		i;
	for (i=0 ; i<MAX_CS_SOUNDS ; i++) {
		if (!sndALIndexes[i].inUse)
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
	for (i=0 ; i<MAX_CS_SOUNDS ; i++) {
		if (sndALIndexes[i].sourceIndex == index)
			sndALIndexes[i].inUse = qFalse;
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

	for (i=0 ; i<alMaxSources ; i++) {
		qalGetSourcei (sndALSources[i], AL_SOURCE_STATE, &state);
		OpenAL_CheckForError ();
		if ((state == AL_STOPPED) || (state == AL_INITIAL)) {
			OpenAL_FreeALIndexes (i);
			return i;
		}
	}

	return -1;
}


/*
============
OpenAL_GetFreeBuffer
============
*/
ALint OpenAL_GetFreeBuffer (void) {
	int		i;
	for (i=0 ; i<alMaxBuffers ; i++) {
		if (!sndALBuffers[i].inUse) {
			sndALBuffers[i].inUse = qTrue;
			return i;
		}
	}

	return -1;
}

/*
===============================================================================

	INIT / SHUTDOWN

===============================================================================
*/

/*
=================
OpenAL_InitExtensions
=================
*/
static void OpenAL_InitExtensions (void){
	if (!al_extensions->integer){
		Com_Printf (0, "...ignoring OpenAL extensions\n");
		return;
	}

	Com_Printf (0, "Initializing OpenAL extensions\n");

	/*
	** EAX2.0
	*/
	if (al_ext_eax->integer) {
		if (qalIsExtensionPresent ("EAX2.0")) {
			qalEAXSet = (ALEAXSET) qalGetProcAddress ("EAXSet");
			if (qalEAXSet) qalEAXGet = (ALEAXGET) qalGetProcAddress ("EAXGet");

			if (qalEAXGet) {
				alConfig.extEAX = qTrue;
				Com_Printf (0, "...enabling EAX2.0\n");
			}
			else
				Com_Printf (0, "..." S_COLOR_RED "EAX2.0 not properly supported!\n");
		}
		else
			Com_Printf (0, "...EAX2.0 not found\n");
	}
	else
		Com_Printf (0, "...ignoring EAX2.0\n");
}


/*
============
OpenAL_Init

Try to initialize the OpenAL library. Return 0 on success or non-zero on failure.
Error message will be automatically printed if initialization fails.
============
*/
qBool OpenAL_Init (void) {
	ALfloat		orientation[6];
	ALint		error;
	int			i;

	// set defaults
	alConfig.extEAX = qFalse;

	qalEAXSet = NULL;
	qalEAXGet = NULL;

	// make sure we're not trying to initialize twice
	if (sndALActive) {
		Com_Printf (0, S_COLOR_RED "Error: OpenAL_Init when sndALActive = qTrue\n");
		return qFalse;
	}

	// initialize our QAL dynamic bindings
	if (!QAL_Init (al_driver->string)) {
		QAL_Shutdown ();
		Com_Printf (0, S_COLOR_RED "OpenAL_Init: could not load \"%s\"\n", al_driver->string);
		return qFalse;
	}

	// initialize OS-specific parts of OpenAL
	if (!ALimp_Init ()) {
		Com_Printf (0, S_COLOR_RED "OpenAL_Init: unable to init al implementation\n");
		return qFalse;
	}

	// set position
	qalListenerfv (AL_POSITION, sndViewOrigin);
	if ((error = qalGetError ()) != AL_NO_ERROR) {
		DisplayALError ("OpenAL_Init: alListenerfv POSITION: ", error);
		return qFalse;
	}

	// set orientation
	orientation[0] = (ALfloat)sndViewForward[0];
	orientation[1] = (ALfloat)sndViewForward[1];
	orientation[2] = (ALfloat)sndViewForward[2];
	orientation[3] = (ALfloat)sndViewUp[0];
	orientation[4] = (ALfloat)sndViewUp[1];
	orientation[5] = (ALfloat)sndViewUp[2];
	qalListenerfv (AL_ORIENTATION, orientation);
	if ((error = qalGetError()) != AL_NO_ERROR) {
		DisplayALError ("OpenAL_Init: alListenerfv ORIENTATION: ", error);
		return qFalse;
	}

	// generate buffers
	for (i=0 ; i<OPENAL_MAX_BUFFERS ; i++) {
		qalGenBuffers (1, &sndALBuffers[i].buffer);
		if ((error = qalGetError ()) != AL_NO_ERROR) {
			DisplayALError ("OpenAL_Inital: GenBuffers: %i", error);
			return qFalse;
		}
	}

	alMaxBuffers = i;
	Com_Printf (0, "...generated %d buffers\n", alMaxBuffers);

	// generate as many sources as possible
	for (i=0 ; i<OPENAL_MAX_SOURCES ; i++) {
		qalGenSources (1, &sndALSources[i]);
		if ((error = qalGetError()) != AL_NO_ERROR)
			break;
	}

	alMaxSources = i;
	Com_Printf (0, "...generated %d sources\n", alMaxSources);

	if (alMaxSources <= 32) {
		Com_Printf (0, "WARNING: Only %d sources available. This probably isn't enough, it is recommended you go back to DirectSound\n", alMaxSources);
	}

	if (OpenAL_CheckForError ()) {
		return qFalse;
	}

	sndALActive = qTrue;
	OpenAL_InitExtensions ();

	return sndALActive;
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

	if (!sndALActive)
		return;

	for (i=0 ; i<alMaxSources ; i++) {
		qalSourceStop (sndALSources[i]);
		qalSourcei (sndALSources[i], AL_BUFFER, 0);
	}

	qalDeleteSources (alMaxSources, sndALSources);
	OpenAL_CheckForError ();

	for (i=0 ; i<alMaxBuffers ; i++) {
		if (sndALBuffers[i].inUse) {
			qalDeleteBuffers (1, &sndALBuffers[i].buffer);

			if ((error = qalGetError()) != AL_NO_ERROR) {
				DisplayALError ("OpenAL_Shutdown: alDeleteBuffers: ", error);
				return;
			}
		}
	}

	alMaxSources = 0;
	alMaxBuffers = 0;

	memset (sndALIndexes, 0, sizeof (sndALIndexes));

	ALimp_Shutdown ();

	sndALActive = qFalse;
	Com_Printf (PRNT_DEV, "OpenAL_Shutdown: Shutdown complete\n");
}
