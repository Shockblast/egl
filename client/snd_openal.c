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

#include "snd_local.h"

#define AL_NO_PROTOTYPES
#include "../include/openal/al.h"

#define ALC_NO_PROTOTYPES
#include "../include/openal/alc.h"

#if defined(WIN32)
# define WIN32_LEAN_AND_MEAN 1
# include <windows.h>

static HINSTANCE			snd_alLibrary;

# define AL_LOADLIB(a)		LoadLibrary (a)
# define AL_GPA(a)			GetProcAddress (snd_alLibrary, a)
# define AL_FREELIB(a)		FreeLibrary (a)

#elif defined(__unix__)

# include <dlfcn.h>
# include <unistd.h>
# include <sys/types.h>

static void					*snd_alLibrary;

# define AL_LOADLIB(a)		dlopen (a, RTLD_LAZY|RTLD_GLOBAL)
# define AL_GPA(a)			dlsym (snd_alLibrary, a)
# define AL_FREELIB(a)		dlclose (a)
#endif

static ALCdevice			*al_hDevice;
static ALCcontext			*al_hALC;

audioAL_t					snd_audioAL;
static channel_t			snd_alOutChannels[MAX_CHANNELS];

static LPALENABLE				qalEnable;
static LPALDISABLE				qalDisable;
static LPALISENABLED			qalIsEnabled;

static LPALGETSTRING			qalGetString;
static LPALGETBOOLEANV			qalGetBooleanv;
static LPALGETINTEGERV			qalGetIntegerv;
static LPALGETFLOATV			qalGetFloatv;
static LPALGETDOUBLEV			qalGetDoublev;
static LPALGETBOOLEAN			qalGetBoolean;
static LPALGETINTEGER			qalGetInteger;
static LPALGETFLOAT				qalGetFloat;
static LPALGETDOUBLE			qalGetDouble;
static LPALGETERROR				qalGetError;

static LPALISEXTENSIONPRESENT	qalIsExtensionPresent;
static LPALGETPROCADDRESS		qalGetProcAddress;
static LPALGETENUMVALUE			qalGetEnumValue;

static LPALLISTENERF			qalListenerf;
static LPALLISTENER3F			qalListener3f;
static LPALLISTENERFV			qalListenerfv;
static LPALLISTENERI			qalListeneri;
static LPALLISTENER3I			qalListener3i;
static LPALLISTENERIV			qalListeneriv;
static LPALGETLISTENERF			qalGetListenerf;
static LPALGETLISTENER3F		qalGetListener3f;
static LPALGETLISTENERFV		qalGetListenerfv;
static LPALGETLISTENERI			qalGetListeneri;
static LPALGETLISTENER3I		qalGetListener3i;
static LPALGETLISTENERIV		qalGetListeneriv;

static LPALGENSOURCES			qalGenSources;
static LPALDELETESOURCES		qalDeleteSources;
static LPALISSOURCE				qalIsSource;
static LPALSOURCEF				qalSourcef;
static LPALSOURCE3F				qalSource3f;
static LPALSOURCEFV				qalSourcefv;
static LPALSOURCEI				qalSourcei;
static LPALSOURCE3I				qalSource3i;
static LPALSOURCEIV				qalSourceiv;
static LPALGETSOURCEF			qalGetSourcef;
static LPALGETSOURCE3F			qalGetSource3f;
static LPALGETSOURCEFV			qalGetSourcefv;
static LPALGETSOURCEI			qalGetSourcei;
static LPALGETSOURCE3I			qalGetSource3i;
static LPALGETSOURCEIV			qalGetSourceiv;
static LPALSOURCEPLAYV			qalSourcePlayv;
static LPALSOURCESTOPV			qalSourceStopv;
static LPALSOURCEREWINDV		qalSourceRewindv;
static LPALSOURCEPAUSEV			qalSourcePausev;
static LPALSOURCEPLAY			qalSourcePlay;
static LPALSOURCESTOP			qalSourceStop;
static LPALSOURCEREWIND			qalSourceRewind;
static LPALSOURCEPAUSE			qalSourcePause;
static LPALSOURCEQUEUEBUFFERS	qalSourceQueueBuffers;
static LPALSOURCEUNQUEUEBUFFERS	qalSourceUnqueueBuffers;

static LPALGENBUFFERS			qalGenBuffers;
static LPALDELETEBUFFERS		qalDeleteBuffers;
static LPALISBUFFER				qalIsBuffer;
static LPALBUFFERDATA			qalBufferData;
static LPALBUFFERF				qalBufferf;
static LPALBUFFER3F				qalBuffer3f;
static LPALBUFFERFV				qalBufferfv;
static LPALBUFFERI				qalBufferi;
static LPALBUFFER3I				qalBuffer3i;
static LPALBUFFERIV				qalBufferiv;
static LPALGETBUFFERF			qalGetBufferf;
static LPALGETBUFFER3F			qalGetBuffer3f;
static LPALGETBUFFERFV			qalGetBufferfv;
static LPALGETBUFFERI			qalGetBufferi;
static LPALGETBUFFER3I			qalGetBuffer3i;
static LPALGETBUFFERIV			qalGetBufferiv;

static LPALDOPPLERFACTOR		qalDopplerFactor;
static LPALDOPPLERVELOCITY		qalDopplerVelocity;
static LPALSPEEDOFSOUND			qalSpeedOfSound;
static LPALDISTANCEMODEL		qalDistanceModel;

static LPALCCREATECONTEXT		qalcCreateContext;
static LPALCMAKECONTEXTCURRENT	qalcMakeContextCurrent;
static LPALCPROCESSCONTEXT		qalcProcessContext;
static LPALCSUSPENDCONTEXT		qalcSuspendContext;
static LPALCDESTROYCONTEXT		qalcDestroyContext;
static LPALCGETCURRENTCONTEXT	qalcGetCurrentContext;
static LPALCGETCONTEXTSDEVICE	qalcGetContextsDevice;

static LPALCOPENDEVICE			qalcOpenDevice;
static LPALCCLOSEDEVICE			qalcCloseDevice;

static LPALCGETERROR			qalcGetError;

static LPALCISEXTENSIONPRESENT	qalcIsExtensionPresent;
static LPALCGETPROCADDRESS		qalcGetProcAddress;

static LPALCGETENUMVALUE		qalcGetEnumValue;
static LPALCGETSTRING			qalcGetString;
static LPALCGETINTEGERV			qalcGetIntegerv;

static LPALCCAPTUREOPENDEVICE	qalcCaptureOpenDevice;
static LPALCCAPTURECLOSEDEVICE	qalcCaptureCloseDevice;
static LPALCCAPTURESTART		qalcCaptureStart;
static LPALCCAPTURESTOP			qalcCaptureStop;
static LPALCCAPTURESAMPLES		qalcCaptureSamples;

/*
===========
ALSnd_CheckForError

Return qTrue if there was an error.
===========
*/
static inline const char *GetALErrorString (ALenum error)
{
	switch (error) {
	case AL_NO_ERROR:			return "AL_NO_ERROR";
	case AL_INVALID_NAME:		return "AL_INVALID_NAME";
	case AL_INVALID_ENUM:		return "AL_INVALID_ENUM";
	case AL_INVALID_VALUE:		return "AL_INVALID_VALUE";
	case AL_INVALID_OPERATION:	return "AL_INVALID_OPERATION";
	case AL_OUT_OF_MEMORY:		return "AL_OUT_OF_MEMORY";
	}

	return "unknown";
}
qBool ALSnd_CheckForError (char *where)
{
	ALenum	error;

	error = qalGetError ();
	if (error != AL_NO_ERROR) {
		Com_Printf (PRNT_ERROR, "AL_ERROR: alGetError (): '%s' (0x%x)", GetALErrorString (error), error);
		if (where)
			Com_Printf (0, " %s\n", where);
		else
			Com_Printf (0, "\n");
		return qFalse;
	}

	return qTrue;
}


/*
==================
ALSnd_Activate
==================
*/
void ALSnd_Activate (qBool active)
{
	if (active)
		qalListenerf (AL_GAIN, s_volume->floatVal * al_gain->floatVal);
	else
		qalListenerf (AL_GAIN, 0.0f);
}

/*
===============================================================================

	BUFFER MANAGEMENT

===============================================================================
*/

/*
==================
ALSnd_BufferFormat
==================
*/
static int ALSnd_BufferFormat (int width, int channels)
{
	switch (width) {
	case 1:
		if (channels == 1)
			return AL_FORMAT_MONO8;
		else if (channels == 2)
			return AL_FORMAT_STEREO8;
		break;

	case 2:
		if (channels == 1)
			return AL_FORMAT_MONO16;
		else if (channels == 2)
			return AL_FORMAT_STEREO16;
		break;
	}

	return AL_FORMAT_MONO16;
}


/*
==================
ALSnd_CreateBuffer
==================
*/
void ALSnd_CreateBuffer (sfxCache_t *sc, int width, int channels, byte *data, int size, int frequency)
{
	if (!sc)
		return;

	// Find the format
	sc->alFormat = ALSnd_BufferFormat (width, channels);

	// Upload
	qalGenBuffers (1, &sc->alBufferNum);
	qalBufferData (sc->alBufferNum, sc->alFormat, data, size, frequency);

	// Check
	if (!qalIsBuffer (sc->alBufferNum))
		Com_Error (ERR_DROP, "ALSnd_CreateBuffer: created buffer is not valid!");
}


/*
==================
ALSnd_DeleteBuffer
==================
*/
void ALSnd_DeleteBuffer (sfxCache_t *sc)
{
	if (sc && sc->alBufferNum)
		qalDeleteBuffers (1, &sc->alBufferNum);
}

/*
===============================================================================

	SOUND PLAYING

===============================================================================
*/

/*
==================
ALSnd_StartChannel
==================
*/
static void ALSnd_StartChannel (channel_t *ch, sfx_t *sfx)
{
	ch->sfx = sfx;

	qalSourcei (ch->alSourceNum, AL_BUFFER, sfx->cache->alBufferNum);
	qalSourcei (ch->alSourceNum, AL_LOOPING, ch->alLooping);
	switch (ch->psType) {
	case PSND_ENTITY:
	case PSND_FIXED:
		qalSourcei (ch->alSourceNum, AL_SOURCE_RELATIVE, AL_FALSE);
		break;

	case PSND_LOCAL:
		qalSourcei (ch->alSourceNum, AL_SOURCE_RELATIVE, AL_TRUE);
		qalSourcefv (ch->alSourceNum, AL_POSITION, vec3Origin);
		qalSourcefv (ch->alSourceNum, AL_VELOCITY, vec3Origin);
		qalSourcef (ch->alSourceNum, AL_REFERENCE_DISTANCE, 0);
		qalSourcef (ch->alSourceNum, AL_MAX_DISTANCE, 0);
		qalSourcef (ch->alSourceNum, AL_ROLLOFF_FACTOR, 0);
		break;
	}

	qalSourcePlay (ch->alSourceNum);
}


/*
==================
ALSnd_StopSound
==================
*/
static void ALSnd_StopSound (channel_t *ch)
{
	ch->sfx = NULL;

	qalSourceStop (ch->alSourceNum);
	qalSourcei (ch->alSourceNum, AL_BUFFER, 0);
}


/*
==================
ALSnd_StopAllSounds
==================
*/
void ALSnd_StopAllSounds (void)
{
	int		i;

	// Stop all sources
	for (i=0 ; i<snd_audioAL.numChannels ; i++) {
		if (!snd_alOutChannels[i].sfx)
			continue;

		ALSnd_StopSound (&snd_alOutChannels[i]);
	}

	// Stop raw streaming
	ALSnd_RawShutdown ();

	// Reset frame count
	snd_audioAL.frameCount = 0;
}

/*
===============================================================================

	SPATIALIZATION

===============================================================================
*/

/*
==================
ALSnd_SpatializeChannel

Updates volume, distance, rolloff, origin, and velocity for a channel
If it's a local sound only the volume is updated
==================
*/
static void ALSnd_SpatializeChannel (channel_t *ch)
{
	vec3_t	position, velocity;

	// Channel volume
	qalSourcef (ch->alSourceNum, AL_GAIN, ch->alVolume);

	// Local sound
	if (ch->psType == PSND_LOCAL)
		return;

	// Distance, rolloff
	qalSourcef (ch->alSourceNum, AL_REFERENCE_DISTANCE, al_maxDistance->floatVal);
	qalSourcef (ch->alSourceNum, AL_MAX_DISTANCE, al_maxDistance->floatVal);
	qalSourcef (ch->alSourceNum, AL_ROLLOFF_FACTOR, al_rollOffFactor->floatVal);

	// Fixed origin
	if (ch->psType == PSND_FIXED) {
		qalSource3f (ch->alSourceNum, AL_POSITION, ch->origin[1], ch->origin[2], -ch->origin[0]);
		qalSource3f (ch->alSourceNum, AL_VELOCITY, 0, 0, 0);
		return;
	}

	// Entity origin
	if (ch->alLooping)
		CL_CGModule_GetEntitySoundOrigin (ch->alLoopEntNum, position, velocity);
	else
		CL_CGModule_GetEntitySoundOrigin (ch->entNum, position, velocity);

	qalSource3f (ch->alSourceNum, AL_POSITION, position[1], position[2], -position[0]);
	qalSource3f (ch->alSourceNum, AL_VELOCITY, velocity[1], velocity[2], -velocity[0]);
}

/*
===============================================================================

	CHANNELS

===============================================================================
*/

/*
=================
ALSnd_PickChannel
=================
*/
static channel_t *ALSnd_PickChannel (int entNum, entChannel_t entChannel)
{
	int			i;
	int			firstToDie;
	int			oldest;
	channel_t	*ch;
	uint32		sourceNum;

	firstToDie = -1;
	oldest = cls.realTime;

	// Check for replacement sound, or find the best one to replace;
	for (i=0, ch=snd_alOutChannels ; i<snd_audioAL.numChannels ; ch++, i++) {
		// Never take over raw stream channels
		if (ch->alRawStream)
			continue;

		// Free channel
		if (!ch->sfx) {
			firstToDie = i;
			break;
		}

		// Channel 0 never overrides
		if (entChannel && ch->entNum == entNum && ch->entChannel == entChannel) {
			// Always override sound from same entity
			firstToDie = i;
			break;
		}

		// Don't let monster sounds override player sounds
		if (ch->entNum == cl.playerNum+1 && entNum != cl.playerNum+1 && ch->sfx)
			continue;

		// Replace the oldest sound
		if (ch->alStartTime < oldest) {
			oldest = ch->alStartTime;
			firstToDie = i;
		}
   }

	if (firstToDie == -1)
		return NULL;

	ch = &snd_alOutChannels[firstToDie];
	sourceNum = ch->alSourceNum;
	memset (ch, 0, sizeof (channel_t));

	ch->entNum = entNum;
	ch->entChannel = entChannel;
	ch->alStartTime = cls.realTime;
	ch->alSourceNum = sourceNum;

	// Stop the source
	ALSnd_StopSound (ch);

	return ch;
}

/*
===============================================================================

	PLAYSOUNDS

===============================================================================
*/

/*
===============
ALSnd_IssuePlaysounds
===============
*/
static void ALSnd_IssuePlaysounds (void)
{
	playSound_t	*ps;
	channel_t	*ch;

	for ( ; ; ) {
		ps = snd_pendingPlays.next;
		if (ps == &snd_pendingPlays)
			break;	// No more pending sounds
		if (ps->beginTime > cls.realTime)
			break;	// No more pending sounds this frame

		// Pick a channel to play on
		ch = ALSnd_PickChannel (ps->entNum, ps->entChannel);
		if (!ch) {
			Snd_FreePlaysound (ps);
			return;
		}

		if (s_show->intVal)
			Com_Printf (0, "Issue %i\n", ps->beginTime);

		// Spatialize
		ch->alLooping = qFalse;
		ch->alRawStream = qFalse;
		ch->alVolume = ps->volume * (1.0f/255.0f);
		ch->entNum = ps->entNum;
		ch->entChannel = ps->entChannel;
		ch->sfx = ps->sfx;
		Vec3Copy (ps->origin, ch->origin);

		// Convert to a local sound if it's not supposed to attenuation
		if (ps->attenuation == ATTN_NONE)
			ch->psType = PSND_LOCAL;
		else
			ch->psType = ps->type;

		ALSnd_SpatializeChannel (ch);
		ALSnd_StartChannel (ch, ps->sfx);

		// Free the playsound
		Snd_FreePlaysound (ps);
	}
}

/*
===============================================================================

	RAW SAMPLING

	Cinematic streaming and voice over network

===============================================================================
*/

/*
===========
ALSnd_RawStart
===========
*/
channel_t *ALSnd_RawStart (void)
{
	channel_t	*ch;

	ch = ALSnd_PickChannel (0, 0);
	if (!ch)
		Com_Error (ERR_FATAL, "ALSnd_RawStart: failed to allocate a source!");

	// Fill source values
	ch->psType = PSND_LOCAL;
	ch->alLooping = qFalse;
	ch->alRawStream = qTrue;
	ch->alVolume = 1;
	ch->sfx = NULL;

	// Spatialize
	qalSourcef (ch->alSourceNum, AL_GAIN, 1);
	qalSourcei (ch->alSourceNum, AL_BUFFER, 0);
	qalSourcei (ch->alSourceNum, AL_LOOPING, AL_FALSE);

	// Local sound
	qalSourcei (ch->alSourceNum, AL_SOURCE_RELATIVE, AL_TRUE);
	qalSourcefv (ch->alSourceNum, AL_POSITION, vec3Origin);
	qalSourcefv (ch->alSourceNum, AL_VELOCITY, vec3Origin);
	qalSourcef (ch->alSourceNum, AL_ROLLOFF_FACTOR, 0);

	return ch;
}


/*
============
ALSnd_RawSamples

Cinematic streaming and voice over network
============
*/
void ALSnd_RawSamples (struct channel_s *rawChannel, int samples, int rate, int width, int channels, byte *data)
{
	ALuint	buffer;
	ALuint	format;

	if (!rawChannel || !rawChannel->alRawStream)
		return;

	// Find the format
	format = ALSnd_BufferFormat (width, channels);

	// Generate a buffer
	qalGenBuffers (1, &buffer);
	qalBufferData (buffer, format, data, (samples * width * channels), rate);

	// Place in queue
	qalSourceQueueBuffers (rawChannel->alSourceNum, 1, &buffer);
}


/*
===========
ALSnd_RawStop
===========
*/
void ALSnd_RawStop (channel_t *rawChannel)
{
	if (!rawChannel || !rawChannel->alRawStream)
		return;

	qalSourceStop (rawChannel->alSourceNum);
	rawChannel->alRawPlaying = qFalse;
	rawChannel->alRawStream = qFalse;
}


/*
===========
ALSnd_RawShutdown
===========
*/
void ALSnd_RawShutdown (void)
{
	channel_t	*ch;
	int			i;

	// Stop all raw streaming channels
	for (i=0, ch=snd_alOutChannels ; i<snd_audioAL.numChannels ; ch++, i++) {
		if (!ch->alRawStream)
			continue;

		ALSnd_RawStop (ch);
	}
}


/*
===========
ALSnd_RawUpdate
===========
*/
static void ALSnd_RawUpdate (channel_t *rawChannel)
{
	int		processed;
	ALuint	buffer;
	ALint	state;

	if (!rawChannel || !rawChannel->alRawStream)
		return;

	// Delete processed buffers
	qalGetSourcei (rawChannel->alSourceNum, AL_BUFFERS_PROCESSED, &processed);
	while (processed--) {
		qalSourceUnqueueBuffers (rawChannel->alSourceNum, 1, &buffer);
		qalDeleteBuffers (1, &buffer);
	}

	// Start the queued buffers
	qalGetSourcei (rawChannel->alSourceNum, AL_BUFFERS_QUEUED, &processed);
	qalGetSourcei (rawChannel->alSourceNum, AL_SOURCE_STATE, &state);
	if (state == AL_STOPPED) {
		if (processed) {
			qalSourcePlay (rawChannel->alSourceNum);
			rawChannel->alRawPlaying = qTrue;
		}
		else if (!rawChannel->alRawPlaying)
			ALSnd_RawStop (rawChannel);
	}
}

/*
===============================================================================

	CHANNEL MIXING

===============================================================================
*/

/*
===========
ALSnd_AddLoopSounds
===========
*/
static void ALSnd_AddLoopSounds (void)
{
	int				i, j;
	channel_t		*ch;
	sfx_t			*sfx;
	entityState_t	*ent;

	if (cl_paused->intVal || Com_ClientState () != CA_ACTIVE || !cls.soundPrepped)
		return;

	// Add looping entity sounds
	for (i=0 ; i<cl.frame.numEntities ; i++) {
		ent = &cl_parseEntities[((cl.frame.parseEntities + i) & MAX_PARSEENTITIES_MASK)];
		if (!ent->sound)
			continue;

		if (!cl.soundCfgStrings[ent->sound] && cl.configStrings[CS_SOUNDS+ent->sound][0])
			cl.soundCfgStrings[ent->sound] = Snd_RegisterSound (cl.configStrings[CS_SOUNDS+ent->sound]);

		sfx = cl.soundCfgStrings[ent->sound];
		if (!sfx || !sfx->cache)
			continue;	// Bad sound effect

		// Update if already active
		for (j=0, ch=snd_alOutChannels ; j<snd_audioAL.numChannels ; ch++, j++) {
			if (ch->sfx != sfx)
				continue;

			if (ch->alRawStream)
				continue;
			if (!ch->alLooping)
				continue;
			if (ch->alLoopEntNum != ent->number)
				continue;
			if (ch->alLoopFrame + 1 != snd_audioAL.frameCount)
				continue;

			ch->alLoopFrame = snd_audioAL.frameCount;
			break;
		}

		// Already active, and simply updated
		if (j != snd_audioAL.numChannels)
			continue;

		// Pick a channel to start the effect
		ch = ALSnd_PickChannel (0, 0);
		if (!ch)
			return;

		ch->alLooping = qTrue;
		ch->alLoopEntNum = ent->number;
		ch->alLoopFrame = snd_audioAL.frameCount;
		ch->alRawStream = qFalse;
		ch->alVolume = 1;
		ch->psType = PSND_ENTITY;

		ALSnd_SpatializeChannel (ch);
		ALSnd_StartChannel (ch, sfx);
	}
}


/*
===========
ALSnd_Update
===========
*/
void ALSnd_Update (refDef_t *rd)
{
	channel_t	*ch;
	int			state;
	int			total;
	int			i;
	ALfloat		origin[3];
	ALfloat		velocity[3];
	ALfloat		orient[6];

	snd_audioAL.frameCount++;

	// Update our position, velocity, and orientation
	if (rd) {
		origin[0] = (ALfloat)rd->viewOrigin[1];
		origin[1] = (ALfloat)rd->viewOrigin[2];
		origin[2] = (ALfloat)rd->viewOrigin[0] * -1;

		velocity[0] = (ALfloat)rd->velocity[1];
		velocity[1] = (ALfloat)rd->velocity[2];
		velocity[2] = (ALfloat)rd->velocity[0] * -1;

		orient[0] = (ALfloat)rd->viewAxis[0][1];
		orient[1] = (ALfloat)rd->viewAxis[0][2] * -1;
		orient[2] = (ALfloat)rd->viewAxis[0][0] * -1;
		orient[3] = (ALfloat)rd->viewAxis[2][1];
		orient[4] = (ALfloat)rd->viewAxis[2][2] * -1;
		orient[5] = (ALfloat)rd->viewAxis[2][0] * -1;
	}
	else {
		origin[0] = 0.0f;
		origin[1] = 0.0f;
		origin[2] = 0.0f;

		velocity[0] = 0.0f;
		velocity[1] = 0.0f;
		velocity[2] = 0.0f;

		orient[0] = axisIdentity[0][1];
		orient[1] = axisIdentity[0][2] * -1;
		orient[2] = axisIdentity[0][0] * -1;
		orient[3] = axisIdentity[2][1];
		orient[4] = axisIdentity[2][2] * -1;
		orient[5] = axisIdentity[2][0] * -1;
	}

	qalListenerfv (AL_POSITION, origin);
	qalListenerfv (AL_VELOCITY, velocity);
	qalListenerfv (AL_ORIENTATION, orient);

	// Update doppler
	if (al_dopplerFactor->modified) {
		al_dopplerFactor->modified = qFalse;
		qalDopplerFactor (al_dopplerFactor->floatVal);
	}
	if (al_dopplerVelocity->modified) {
		al_dopplerVelocity->modified = qFalse;
		qalDopplerVelocity (al_dopplerVelocity->floatVal);
	}

	// Update listener volume
	qalListenerf (AL_GAIN, snd_isActive ? s_volume->floatVal * al_gain->floatVal : 0);

	// Distance model
	qalDistanceModel (AL_INVERSE_DISTANCE_CLAMPED);

	// Add loop sounds
	ALSnd_AddLoopSounds ();

	// Add play sounds
	ALSnd_IssuePlaysounds ();

	// Update channel spatialization
	total = 0;
	for (i=0, ch=snd_alOutChannels ; i<snd_audioAL.numChannels ; ch++, i++) {
		// Update streaming channels
		if (ch->alRawStream) {
			ALSnd_RawUpdate (ch);
		}
		else if (!ch->sfx)
			continue;

		// Stop inactive channels
		if (ch->alLooping) {
			if (ch->alLoopFrame != snd_audioAL.frameCount) {
				ALSnd_StopSound (ch);
				continue;
			}
			else if (!snd_isActive) {
				ch->alLoopFrame = snd_audioAL.frameCount - 1;
				ALSnd_StopSound (ch);
				continue;
			}
		}
		else if (!ch->alRawStream) {
			qalGetSourcei (ch->alSourceNum, AL_SOURCE_STATE, &state);
			if (state == AL_STOPPED) {
				ALSnd_StopSound (ch);
				continue;
			}
		}

		// Debug output
		if (s_show->intVal && ch->alVolume) {
			Com_Printf (0, "%3i %s\n", i+1, ch->sfx->name);
			total++;
		}

		// Spatialize
		if (!ch->alRawStream)
			ALSnd_SpatializeChannel (ch);
	}

	// Debug output
	if (s_show->intVal)
		Com_Printf (0, "----(%i)----\n", total);

	// Check for errors
	if (al_errorCheck->intVal)
		ALSnd_CheckForError ("ALSnd_Update");
}

/*
==============================================================================

	INIT / SHUTDOWN
 
==============================================================================
*/

/*
===========
ALSnd_Init
===========
*/
qBool ALSnd_Init (void)
{
	char	*libName;
	char	*device;
	int		i;

	Com_Printf (0, "Initializing OpenAL\n");

	// Load our OpenAL library
	libName = al_driver->string[0] ? al_driver->string : AL_DRIVERNAME;
	Com_Printf (0, "...LoadLibrary (\"%s\")\n", libName);
	if (!(snd_alLibrary = AL_LOADLIB (libName))) {
		Com_Printf (PRNT_ERROR, "failed!\n");
		return qFalse;
	}

	// Create the QAL bindings
	// FIXME: sanity check each one of these?
	Com_Printf (0, "...creating QAL bindings\n");
	qalEnable					= (LPALENABLE)AL_GPA ("alEnable");
	qalDisable					= (LPALDISABLE)AL_GPA ("alDisable");
	qalIsEnabled				= (LPALISENABLED)AL_GPA ("alIsEnabled");

	qalGetString				= (LPALGETSTRING)AL_GPA ("alGetString");
	qalGetBooleanv				= (LPALGETBOOLEANV)AL_GPA ("alGetBooleanv");
	qalGetIntegerv				= (LPALGETINTEGERV)AL_GPA ("alGetIntegerv");
	qalGetFloatv				= (LPALGETFLOATV)AL_GPA ("alGetFloatv");
	qalGetDoublev				= (LPALGETDOUBLEV)AL_GPA ("alGetDoublev");
	qalGetBoolean				= (LPALGETBOOLEAN)AL_GPA ("alGetBoolean");
	qalGetInteger				= (LPALGETINTEGER)AL_GPA ("alGetInteger");
	qalGetFloat					= (LPALGETFLOAT)AL_GPA ("alGetFloat");
	qalGetDouble				= (LPALGETDOUBLE)AL_GPA ("alGetDouble");
	qalGetError					= (LPALGETERROR)AL_GPA ("alGetError");

	qalIsExtensionPresent		= (LPALISEXTENSIONPRESENT)AL_GPA ("alIsExtensionPresent");
	qalGetProcAddress			= (LPALGETPROCADDRESS)AL_GPA ("alGetProcAddress");
	qalGetEnumValue				= (LPALGETENUMVALUE)AL_GPA ("alGetEnumValue");

	qalListenerf				= (LPALLISTENERF)AL_GPA ("alListenerf");
	qalListener3f				= (LPALLISTENER3F)AL_GPA ("alListener3f");
	qalListenerfv				= (LPALLISTENERFV)AL_GPA ("alListenerfv");
	qalListeneri				= (LPALLISTENERI)AL_GPA ("alListeneri");
	qalListener3i				= (LPALLISTENER3I)AL_GPA ("alListener3i");
	qalListeneriv				= (LPALLISTENERIV)AL_GPA ("alListeneriv");
	qalGetListenerf				= (LPALGETLISTENERF)AL_GPA ("alGetListenerf");
	qalGetListener3f			= (LPALGETLISTENER3F)AL_GPA ("alGetListener3f");
	qalGetListenerfv			= (LPALGETLISTENERFV)AL_GPA ("alGetListenerfv");
	qalGetListeneri				= (LPALGETLISTENERI)AL_GPA ("alGetListeneri");
	qalGetListener3i			= (LPALGETLISTENER3I)AL_GPA ("alGetListener3i");
	qalGetListeneriv			= (LPALGETLISTENERIV)AL_GPA ("alGetListeneriv");

	qalGenSources				= (LPALGENSOURCES)AL_GPA ("alGenSources");
	qalDeleteSources			= (LPALDELETESOURCES)AL_GPA ("alDeleteSources");
	qalIsSource					= (LPALISSOURCE)AL_GPA ("alIsSource");
	qalSourcef					= (LPALSOURCEF)AL_GPA ("alSourcef");
	qalSource3f					= (LPALSOURCE3F)AL_GPA ("alSource3f");
	qalSourcefv					= (LPALSOURCEFV)AL_GPA ("alSourcefv");
	qalSourcei					= (LPALSOURCEI)AL_GPA ("alSourcei");
	qalSource3i					= (LPALSOURCE3I)AL_GPA ("alSource3i");
	qalSourceiv					= (LPALSOURCEIV)AL_GPA ("alSourceiv");
	qalGetSourcef				= (LPALGETSOURCEF)AL_GPA ("alGetSourcef");
	qalGetSource3f				= (LPALGETSOURCE3F)AL_GPA ("alGetSource3f");
	qalGetSourcefv				= (LPALGETSOURCEFV)AL_GPA ("alGetSourcefv");
	qalGetSourcei				= (LPALGETSOURCEI)AL_GPA ("alGetSourcei");
	qalGetSource3i				= (LPALGETSOURCE3I)AL_GPA ("alGetSource3i");
	qalGetSourceiv				= (LPALGETSOURCEIV)AL_GPA ("alGetSourceiv");
	qalSourcePlayv				= (LPALSOURCEPLAYV)AL_GPA ("alSourcePlayv");
	qalSourceStopv				= (LPALSOURCESTOPV)AL_GPA ("alSourceStopv");
	qalSourceRewindv			= (LPALSOURCEREWINDV)AL_GPA ("alSourceRewindv");
	qalSourcePausev				= (LPALSOURCEPAUSEV)AL_GPA ("alSourcePausev");
	qalSourcePlay				= (LPALSOURCEPLAY)AL_GPA ("alSourcePlay");
	qalSourceStop				= (LPALSOURCESTOP)AL_GPA ("alSourceStop");
	qalSourceRewind				= (LPALSOURCEREWIND)AL_GPA ("alSourceRewind");
	qalSourcePause				= (LPALSOURCEPAUSE)AL_GPA ("alSourcePause");
	qalSourceQueueBuffers		= (LPALSOURCEQUEUEBUFFERS)AL_GPA ("alSourceQueueBuffers");
	qalSourceUnqueueBuffers		= (LPALSOURCEUNQUEUEBUFFERS)AL_GPA ("alSourceUnqueueBuffers");

	qalGenBuffers				= (LPALGENBUFFERS)AL_GPA ("alGenBuffers");
	qalDeleteBuffers			= (LPALDELETEBUFFERS)AL_GPA ("alDeleteBuffers");
	qalIsBuffer					= (LPALISBUFFER)AL_GPA ("alIsBuffer");
	qalBufferData				= (LPALBUFFERDATA)AL_GPA ("alBufferData");
	qalBufferf					= (LPALBUFFERF)AL_GPA ("alBufferf");
	qalBuffer3f					= (LPALBUFFER3F)AL_GPA ("alBuffer3f");
	qalBufferfv					= (LPALBUFFERFV)AL_GPA ("alBufferfv");
	qalBufferi					= (LPALBUFFERI)AL_GPA ("alBufferi");
	qalBuffer3i					= (LPALBUFFER3I)AL_GPA ("alBuffer3i");
	qalBufferiv					= (LPALBUFFERIV)AL_GPA ("alBufferiv");
	qalGetBufferf				= (LPALGETBUFFERF)AL_GPA ("alGetBufferf");
	qalGetBuffer3f				= (LPALGETBUFFER3F)AL_GPA ("alGetBuffer3f");
	qalGetBufferfv				= (LPALGETBUFFERFV)AL_GPA ("alGetBufferfv");
	qalGetBufferi				= (LPALGETBUFFERI)AL_GPA ("alGetBufferi");
	qalGetBuffer3i				= (LPALGETBUFFER3I)AL_GPA ("alGetBuffer3i");
	qalGetBufferiv				= (LPALGETBUFFERIV)AL_GPA ("alGetBufferiv");

	qalDopplerFactor			= (LPALDOPPLERFACTOR)AL_GPA ("alDopplerFactor");
	qalDopplerVelocity			= (LPALDOPPLERVELOCITY)AL_GPA ("alDopplerVelocity");
	qalSpeedOfSound				= (LPALSPEEDOFSOUND)AL_GPA ("alSpeedOfSound");
	qalDistanceModel			= (LPALDISTANCEMODEL)AL_GPA ("alDistanceModel");

	qalcCreateContext			= (LPALCCREATECONTEXT)AL_GPA ("alcCreateContext");
	qalcMakeContextCurrent		= (LPALCMAKECONTEXTCURRENT)AL_GPA ("alcMakeContextCurrent");
	qalcProcessContext			= (LPALCPROCESSCONTEXT)AL_GPA ("alcProcessContext");
	qalcSuspendContext			= (LPALCSUSPENDCONTEXT)AL_GPA ("alcSuspendContext");
	qalcDestroyContext			= (LPALCDESTROYCONTEXT)AL_GPA ("alcDestroyContext");
	qalcGetCurrentContext		= (LPALCGETCURRENTCONTEXT)AL_GPA ("alcGetCurrentContext");
	qalcGetContextsDevice		= (LPALCGETCONTEXTSDEVICE)AL_GPA ("alcGetContextsDevice");

	qalcOpenDevice				= (LPALCOPENDEVICE)AL_GPA ("alcOpenDevice");
	qalcCloseDevice				= (LPALCCLOSEDEVICE)AL_GPA ("alcCloseDevice");

	qalcGetError				= (LPALCGETERROR)AL_GPA ("alcGetError");

	qalcIsExtensionPresent		= (LPALCISEXTENSIONPRESENT)AL_GPA ("alcIsExtensionPresent");
	qalcGetProcAddress			= (LPALCGETPROCADDRESS)AL_GPA ("alcGetProcAddress");

	qalcGetEnumValue			= (LPALCGETENUMVALUE)AL_GPA ("alcGetEnumValue");
	qalcGetString				= (LPALCGETSTRING)AL_GPA ("alcGetString");
	qalcGetIntegerv				= (LPALCGETINTEGERV)AL_GPA ("alcGetIntegerv");

	qalcCaptureOpenDevice		= (LPALCCAPTUREOPENDEVICE)AL_GPA ("alcCaptureOpenDevice");
	qalcCaptureCloseDevice		= (LPALCCAPTURECLOSEDEVICE)AL_GPA ("alcCaptureCloseDevice");
	qalcCaptureStart			= (LPALCCAPTURESTART)AL_GPA ("alcCaptureStart");
	qalcCaptureStop				= (LPALCCAPTURESTOP)AL_GPA ("alcCaptureStop");
	qalcCaptureSamples			= (LPALCCAPTURESAMPLES)AL_GPA ("alcCaptureSamples");

	// Open the AL device
	device = al_device->string[0] ? al_device->string : NULL;
	al_hDevice = qalcOpenDevice (device);
	Com_Printf (0, "...opening device\n");
	if (!al_hDevice) {
		Com_Printf (PRNT_ERROR, "failed!\n");
		return qFalse;
	}

	// Create the context and make it current
	al_hALC = qalcCreateContext (al_hDevice, NULL);
	Com_Printf (0, "...creating context\n");
	if (!al_hALC) {
		Com_Printf (PRNT_ERROR, "failed!\n");
		ALSnd_Shutdown ();
		return qFalse;
	}

	Com_Printf (0, "...making current\n");
	if (!qalcMakeContextCurrent (al_hALC)) {
		Com_Printf (PRNT_ERROR, "failed!\n");
		ALSnd_Shutdown ();
		return qFalse;
	}

	// Generate sources
	Com_Printf (0, "...generating sources\n");
	snd_audioAL.numChannels = 0;
	for (i=0 ; i<MAX_CHANNELS ; i++) {
		qalGenSources (1, &snd_alOutChannels[i].alSourceNum);
		if (qalGetError () != AL_NO_ERROR)
			break;
		snd_audioAL.numChannels++;
	}
	if (!snd_audioAL.numChannels) {
		Com_Printf (PRNT_ERROR, "failed!\n");
		ALSnd_Shutdown ();
		return qFalse;
	}
	Com_Printf (0, "...generated %i sources\n", snd_audioAL.numChannels);

	// Doppler
	Com_Printf (0, "...setting doppler\n");
	qalDopplerFactor (al_dopplerFactor->floatVal);
	qalDopplerVelocity (al_dopplerVelocity->floatVal);

	al_dopplerFactor->modified = qFalse;
	al_dopplerVelocity->modified = qFalse;

	// Query some info
	snd_audioAL.extensionString = qalGetString (AL_EXTENSIONS);
	snd_audioAL.rendererString = qalGetString (AL_RENDERER);
	snd_audioAL.vendorString = qalGetString (AL_VENDOR);
	snd_audioAL.versionString = qalGetString (AL_VERSION);
	snd_audioAL.deviceName = qalcGetString (al_hDevice, ALC_DEVICE_SPECIFIER);

	Com_Printf (0, "Initialization successful\n");
	Com_Printf (0, "AL_VENDOR: %s\n", snd_audioAL.vendorString);
	Com_Printf (0, "AL_RENDERER: %s\n", snd_audioAL.rendererString);
	Com_Printf (0, "AL_VERSION: %s\n", snd_audioAL.versionString);
	Com_Printf (0, "AL_EXTENSIONS: %s\n", snd_audioAL.extensionString);
	Com_Printf (0, "ALC_DEVICE_SPECIFIER: %s\n", snd_audioAL.deviceName);
	return qTrue;
}


/*
===========
ALSnd_Shutdown
===========
*/
void ALSnd_Shutdown (void)
{
	int		i;

	Com_Printf (0, "Shutting down OpenAL\n");

	// Make sure RAW is shutdown
	ALSnd_RawShutdown ();

	// Free sources
	Com_Printf (0, "...releasing sources\n");
	for (i=0 ; i<snd_audioAL.numChannels ; i++) {
		qalSourceStop (snd_alOutChannels[i].alSourceNum);
		qalDeleteSources (1, &snd_alOutChannels[i].alSourceNum);
	}
	snd_audioAL.numChannels = 0;

	// Release the context
	if (al_hALC) {
		if (qalcMakeContextCurrent) {
			Com_Printf (0, "...releasing the context\n");
			qalcMakeContextCurrent (NULL);
		}
		if (qalcDestroyContext) {
			Com_Printf (0, "...destroying the context\n");
			qalcDestroyContext (al_hALC);
		}

		al_hALC = NULL;
	}

	// Close the device
	if (al_hDevice) {
		if (qalcCloseDevice) {
			Com_Printf (0, "...closing the device\n");
			qalcCloseDevice (al_hDevice);
		}

		al_hDevice = NULL;
	}

	// Release the library
	if (snd_alLibrary) {
		Com_Printf (0, "...releasing the OpenAL library\n");
		AL_FREELIB (snd_alLibrary);
		snd_alLibrary = NULL;
	}

	// Reset QAL bindings
	qalEnable					= NULL;
	qalDisable					= NULL;
	qalIsEnabled				= NULL;

	qalGetString				= NULL;
	qalGetBooleanv				= NULL;
	qalGetIntegerv				= NULL;
	qalGetFloatv				= NULL;
	qalGetDoublev				= NULL;
	qalGetBoolean				= NULL;
	qalGetInteger				= NULL;
	qalGetFloat					= NULL;
	qalGetDouble				= NULL;
	qalGetError					= NULL;

	qalIsExtensionPresent		= NULL;
	qalGetProcAddress			= NULL;
	qalGetEnumValue				= NULL;

	qalListenerf				= NULL;
	qalListener3f				= NULL;
	qalListenerfv				= NULL;
	qalListeneri				= NULL;
	qalListener3i				= NULL;
	qalListeneriv				= NULL;
	qalGetListenerf				= NULL;
	qalGetListener3f			= NULL;
	qalGetListenerfv			= NULL;
	qalGetListeneri				= NULL;
	qalGetListener3i			= NULL;
	qalGetListeneriv			= NULL;

	qalGenSources				= NULL;
	qalDeleteSources			= NULL;
	qalIsSource					= NULL;
	qalSourcef					= NULL;
	qalSource3f					= NULL;
	qalSourcefv					= NULL;
	qalSourcei					= NULL;
	qalSource3i					= NULL;
	qalSourceiv					= NULL;
	qalGetSourcef				= NULL;
	qalGetSource3f				= NULL;
	qalGetSourcefv				= NULL;
	qalGetSourcei				= NULL;
	qalGetSource3i				= NULL;
	qalGetSourceiv				= NULL;
	qalSourcePlayv				= NULL;
	qalSourceStopv				= NULL;
	qalSourceRewindv			= NULL;
	qalSourcePausev				= NULL;
	qalSourcePlay				= NULL;
	qalSourceStop				= NULL;
	qalSourceRewind				= NULL;
	qalSourcePause				= NULL;
	qalSourceQueueBuffers		= NULL;
	qalSourceUnqueueBuffers		= NULL;

	qalGenBuffers				= NULL;
	qalDeleteBuffers			= NULL;
	qalIsBuffer					= NULL;
	qalBufferData				= NULL;
	qalBufferf					= NULL;
	qalBuffer3f					= NULL;
	qalBufferfv					= NULL;
	qalBufferi					= NULL;
	qalBuffer3i					= NULL;
	qalBufferiv					= NULL;
	qalGetBufferf				= NULL;
	qalGetBuffer3f				= NULL;
	qalGetBufferfv				= NULL;
	qalGetBufferi				= NULL;
	qalGetBuffer3i				= NULL;
	qalGetBufferiv				= NULL;

	qalDopplerFactor			= NULL;
	qalDopplerVelocity			= NULL;
	qalSpeedOfSound				= NULL;
	qalDistanceModel			= NULL;

	qalcCreateContext			= NULL;
	qalcMakeContextCurrent		= NULL;
	qalcProcessContext			= NULL;
	qalcSuspendContext			= NULL;
	qalcDestroyContext			= NULL;
	qalcGetCurrentContext		= NULL;
	qalcGetContextsDevice		= NULL;

	qalcOpenDevice				= NULL;
	qalcCloseDevice				= NULL;

	qalcGetError				= NULL;

	qalcIsExtensionPresent		= NULL;
	qalcGetProcAddress			= NULL;

	qalcGetEnumValue			= NULL;
	qalcGetString				= NULL;
	qalcGetIntegerv				= NULL;

	qalcCaptureOpenDevice		= NULL;
	qalcCaptureCloseDevice		= NULL;
	qalcCaptureStart			= NULL;
	qalcCaptureStop				= NULL;
	qalcCaptureSamples			= NULL;
}
