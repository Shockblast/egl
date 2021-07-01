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
// snd_dma.c
// Main control for any streaming sound output device
//

#include "snd_loc.h"

channel_t   sndChannels[MAX_CHANNELS];

qBool		sndInitialized = qFalse;
qBool		sndActive = qFalse;
qBool		sndRestart;

dma_t		sndDMA;

int			sndSoundTime;	// sample PAIRS
int			sndPaintedTime;	// sample PAIRS

// sound registration
sfx_t		sfxKnown[MAX_SFX];
int			sfxNumKnown;

static uInt	sndRegistrationFrame;

// play sounds
playSound_t	snd_PlaySounds[MAX_PLAYSOUNDS];
playSound_t	snd_FreePlays;
playSound_t	sndPendingPlays;

// raw
int				sndRawEnd;
sfxSamplePair_t	sndRawSamples[MAX_RAW_SAMPLES];

const GUID		DSPROPSETID_EAX20_ListenerProperties = {0x306a6a8, 0xb224, 0x11d2, {0x99, 0xe5, 0x0, 0x0, 0xe8, 0xd8, 0xc7, 0x22}};
const GUID		DSPROPSETID_EAX20_BufferProperties = {0x306a6a7, 0xb224, 0x11d2, {0x99, 0xe5, 0x0, 0x0, 0xe8, 0xd8, 0xc7, 0x22}};

// orientation
vec3_t	sndViewOrigin;
vec3_t	sndViewForward;
vec3_t	sndViewRight;
vec3_t	sndViewUp;

cVar_t	*s_volume;
cVar_t	*s_testsound;
cVar_t	*s_loadas8bit;
cVar_t	*s_khz;
cVar_t	*s_show;
cVar_t	*s_mixahead;
cVar_t	*s_primary;
cVar_t	*s_initsound;

cVar_t	*al_dopplerfactor;
cVar_t	*al_dopplervelocity;
cVar_t	*al_driver;
cVar_t	*al_extensions;
cVar_t	*al_ext_eax;

/*
===============================================================================

	WAV LOADING AND RESAMPLING

===============================================================================
*/

static byte		*sndDataPtr;
static byte		*sndIffEnd;
static byte		*sndLastIffChunk;
static byte		*sndIffData;
static int		sndIffChunkLength;

/*
============
GetLittleShort
============
*/
static short GetLittleShort (void) {
	short val = 0;

	val = *sndDataPtr;
	val = val + (*(sndDataPtr+1)<<8);

	sndDataPtr += 2;

	return val;
}


/*
============
GetLittleLong
============
*/
static int GetLittleLong (void) {
	int val = 0;

	val = *sndDataPtr;
	val = val + (*(sndDataPtr+1)<<8);
	val = val + (*(sndDataPtr+2)<<16);
	val = val + (*(sndDataPtr+3)<<24);

	sndDataPtr += 4;

	return val;
}


/*
============
FindNextChunk
============
*/
static void FindNextChunk (char *name) {
	for ( ; ; ) {
		sndDataPtr = sndLastIffChunk;

		if (sndDataPtr >= sndIffEnd) {
			// didn't find the chunk
			sndDataPtr = NULL;
			return;
		}
		
		sndDataPtr += 4;
		sndIffChunkLength = GetLittleLong();
		if (sndIffChunkLength < 0) {
			sndDataPtr = NULL;
			return;
		}

		sndDataPtr -= 8;
		sndLastIffChunk = sndDataPtr + 8 + ((sndIffChunkLength + 1) & ~1);
		if (!strncmp (sndDataPtr, name, 4))
			return;
	}
}


/*
============
FindChunk
============
*/
static void FindChunk (char *name) {
	sndLastIffChunk = sndIffData;
	FindNextChunk (name);
}


/*
============
Snd_GetWavinfo
============
*/
static wavInfo_t Snd_GetWavinfo (char *name, byte *wav, int wavLength) {
	wavInfo_t	info;
	int			i;
	int			format;
	int			samples;

	memset (&info, 0, sizeof (info));

	if (!wav)
		return info;
		
	sndIffData = wav;
	sndIffEnd = wav + wavLength;

	// find "RIFF" chunk
	FindChunk ("RIFF");
	if (!(sndDataPtr && !strncmp (sndDataPtr+8, "WAVE", 4))) {
		Com_Printf (0, "Missing RIFF/WAVE chunks\n");
		return info;
	}

	// get "fmt " chunk
	sndIffData = sndDataPtr + 12;

	FindChunk ("fmt ");
	if (!sndDataPtr) {
		Com_Printf (0, "Missing fmt chunk\n");
		return info;
	}

	sndDataPtr += 8;
	format = GetLittleShort ();
	if (format != 1) {
		Com_Printf (0, "Microsoft PCM format only\n");
		return info;
	}

	info.channels = GetLittleShort();
	info.rate = GetLittleLong ();
	sndDataPtr += 4+2;
	info.width = GetLittleShort () / 8;

	// get cue chunk
	FindChunk ("cue ");
	if (sndDataPtr) {
		sndDataPtr += 32;
		info.loopStart = GetLittleLong ();

		// if the next chunk is a LIST chunk, look for a cue length marker
		FindNextChunk ("LIST");
		if (sndDataPtr) {
			if (!strncmp (sndDataPtr + 28, "mark", 4)) {
				// this is not a proper parse, but it works with cooledit
				sndDataPtr += 24;
				i = GetLittleLong ();	// samples in loop
				info.samples = info.loopStart + i;
			}
		}
	} else
		info.loopStart = -1;

	// find data chunk
	FindChunk ("data");
	if (!sndDataPtr) {
		Com_Printf (0, "Missing data chunk\n");
		return info;
	}

	sndDataPtr += 4;
	samples = GetLittleLong () / info.width;

	if (info.samples) {
		if (samples < info.samples)
			Com_Error (ERR_DROP, "Sound %s has a bad loop length", name);
	} else
		info.samples = samples;

	info.dataOfs = sndDataPtr - wav;
	
	return info;
}


/*
================
Snd_ResampleSfx
================
*/
static void Snd_ResampleSfx (sfx_t *sfx, int inRate, int inWidth, byte *data) {
	int		outcount;
	int		srcsample;
	float	stepscale;
	int		i;
	int		sample, samplefrac, fracstep;
	sfxCache_t	*sc;
	
	sc = sfx->cache;
	if (!sc)
		return;

	stepscale = (float)inRate / sndDMA.speed;	// this is usually 0.5, 1, or 2

	outcount = sc->length / stepscale;
	sc->length = outcount;
	if (sc->loopStart != -1)
		sc->loopStart = sc->loopStart / stepscale;

	sc->speed = sndDMA.speed;
	if (s_loadas8bit->value)
		sc->width = 1;
	else
		sc->width = inWidth;
	sc->stereo = 0;

	// resample / decimate to the current source rate
	if ((stepscale == 1) && (inWidth == 1) && (sc->width == 1)) {
		// fast special case
		for (i=0 ; i<outcount ; i++)
			((signed char *)sc->data)[i] = (int)((byte)(data[i]) - 128);
	} else {
		// general case
		samplefrac = 0;
		fracstep = stepscale*256;
		for (i=0 ; i<outcount ; i++) {
			srcsample = samplefrac >> 8;
			samplefrac += fracstep;
			if (inWidth == 2)
				sample = LittleShort (((short *)data)[srcsample]);
			else
				sample = (int)((byte)(data[srcsample]) - 128) << 8;
			if (sc->width == 2)
				((short *)sc->data)[i] = sample;
			else
				((signed char *)sc->data)[i] = sample >> 8;
		}
	}
}


/*
==============
Snd_LoadSound
==============
*/
sfxCache_t *Snd_LoadSound (sfx_t *s) {
    char		namebuffer[MAX_QPATH];
	byte		*data;
	wavInfo_t	info;
	int			len;
	float		stepscale;
	sfxCache_t	*sc;
	int			size;
	char		*name;

	s->sRegistrationFrame = sndRegistrationFrame;
	if (s->name[0] == '*')
		return NULL;

	// see if still in memory
	sc = s->cache;
	if (sc)
		return sc;

	// load it in
	if (s->trueName)
		name = s->trueName;
	else
		name = s->name;

	if (name[0] == '#')
		strcpy (namebuffer, &name[1]);
	else
		Q_snprintfz (namebuffer, sizeof (namebuffer), "sound/%s", name);

	size = FS_LoadFile (namebuffer, (void **)&data);

	if (!data) {
		Com_Printf (PRNT_DEV, "Snd_LoadSound: Couldn't load %s\n", namebuffer);
		return NULL;
	}

	if (!sndALActive) {
		info = Snd_GetWavinfo (s->name, data, size);
		if (info.channels != 1) {
			Com_Printf (0, "Snd_LoadSound: %s is a stereo sample\n", s->name);
			FS_FreeFile (data);
			return NULL;
		}

		stepscale = (float)info.rate / sndDMA.speed;	
		len = (info.samples / stepscale) * info.width * info.channels;
	} else
		len = 0;

	sc = s->cache = Mem_PoolAlloc (len + sizeof (sfxCache_t), MEMPOOL_SOUNDSYS, 0);

	if (!sc) {
		FS_FreeFile (data);
		return NULL;
	}

	if (sndALActive) {
		ALint		buffer;
		int			format;
		ALvoid		*wavData;
		ALboolean	loop;

		buffer = OpenAL_GetFreeBuffer ();
		if (buffer == -1)
			Com_Error (ERR_DROP, "Snd_LoadSound: Out of OpenAL buffers");

		alutLoadWAVMemory (data, &format, &wavData, &sc->length, &sc->speed, &loop);
		qalBufferData (sndALBuffers[buffer].buffer, format, wavData, sc->length, sc->speed);
		alutUnloadWAV (format, wavData, sc->length, sc->speed);

		OpenAL_CheckForError ();

		Com_Printf (PRNT_DEV, "Snd_LoadSound: OpenAL: Loaded %s into buffer %d\n", namebuffer, buffer);

		sc->bufferNum = buffer;
	} else {
		sc->length = info.samples;
		sc->loopStart = info.loopStart;
		sc->speed = info.rate;
		sc->width = info.width;
		sc->stereo = info.channels;

		Snd_ResampleSfx (s, sc->speed, sc->width, data + info.dataOfs);
	}

	FS_FreeFile (data);

	return sc;
}

/*
===============================================================================

	CONSOLE FUNCTIONS

===============================================================================
*/

/*
============
Snd_Play_f
============
*/
static void Snd_Play_f (void) {
	int		i;
	char	name[256];
	sfx_t	*sfx;
	
	i = 1;
	while (i < Cmd_Argc ()) {
		if (!strrchr (Cmd_Argv (i), '.')) {
			strcpy (name, Cmd_Argv (i));
			strcat (name, ".wav");
		} else
			strcpy (name, Cmd_Argv (i));
		sfx = Snd_RegisterSound (name);
		Snd_StartSound (NULL, cl.playerNum+1, 0, sfx, 1.0, 1.0, 0);
		i++;
	}
}


/*
================
Snd_Restart_f

For queueing a sound restart
================
*/
static void Snd_Restart_f (void) {
	sndRestart = qTrue;
}


/*
============
Snd_SoundList_f
============
*/
static void Snd_SoundList_f (void) {
	int			i;
	sfx_t		*sfx;
	sfxCache_t	*sc;

	if (sndALActive) {
		for (sfx=sfxKnown, i=0 ; i<sfxNumKnown ; i++, sfx++) {
			if (!sfx->sRegistrationFrame)
				continue;

			sc = sfx->cache;
			if (sc) {
				Com_Printf (0, "%3i          %s\n", sc->bufferNum, sfx->name);
			} else {
				if (sfx->name[0] == '*')
					Com_Printf (0, "(placehold): %s\n", sfx->name);
				else
					Com_Printf (0, "(notloaded): %s\n", sfx->name);
			}
		}
	} else {
		int		total;
		int		size;

		for (sfx=sfxKnown, total=0, i=0 ; i<sfxNumKnown ; i++, sfx++) {
			if (!sfx->sRegistrationFrame)
				continue;

			sc = sfx->cache;
			if (sc) {
				size = sc->length * sc->width * (sc->stereo + 1);
				total += size;
				if (sc->loopStart >= 0)
					Com_Printf (0, "L");
				else
					Com_Printf (0, " ");
				Com_Printf (0, "(%2db) %6i : %s\n", sc->width*8, size, sfx->name);
			} else {
				if (sfx->name[0] == '*')
					Com_Printf (0, "  placeholder : %s\n", sfx->name);
				else
					Com_Printf (0, "  not loaded  : %s\n", sfx->name);
			}
		}

		Com_Printf (0, "Total resident: %i\n", total);
	}
}


/*
=================
Snd_SoundInfo_f
=================
*/
static void Snd_SoundInfo_f (void) {
	if (!sndInitialized) {
		Com_Printf (0, S_COLOR_YELLOW "Sound system not started\n");
		return;
	}

	if (sndALActive) {
		Com_Printf (0, "OpenAL %d.%d on %s\n", alConfig.verMajor, alConfig.verMinor, alConfig.device);

		Com_Printf (0, "AL_RENDERER: %s\n", alConfig.rendString);
		Com_Printf (0, "AL_VENDOR: %s\n", alConfig.vendString);
		Com_Printf (0, "AL_VERSION: %s\n", alConfig.versString);
		Com_Printf (0, "AL_EXTENSIONS: %s\n", alConfig.extsString);

		Com_Printf (0, "Extensions:\n");
		Com_Printf (0, "EAX: %s\n", alConfig.extEAX ? "On" : "Off");
	} else {
		Com_Printf (0, "%5d stereo\n", sndDMA.channels - 1);
		Com_Printf (0, "%5d samples\n", sndDMA.samples);
		Com_Printf (0, "%5d samplepos\n", sndDMA.samplePos);
		Com_Printf (0, "%5d samplebits\n", sndDMA.sampleBits);
		Com_Printf (0, "%5d submission_chunk\n", sndDMA.submissionChunk);
		Com_Printf (0, "%5d speed\n", sndDMA.speed);
		Com_Printf (0, "0x%x dma buffer\n", sndDMA.buffer);
	}
}

/*
==============================================================================

	SOUND REGISTRATION
 
==============================================================================
*/

/*
==================
Snd_FreeSound
==================
*/
static void Snd_FreeSound (sfx_t *sfx) {
	if (!sfx)
		return;

	if (sfx->cache)				// it is possible to have a leftover
		Mem_Free (sfx->cache);	// from a server that didn't finish loading
	memset (sfx, 0, sizeof (*sfx));
}


/*
==================
Snd_FindSpot
==================
*/
static sfx_t *Snd_FindSpot (void) {
	int		i;

	for (i=0 ; i<sfxNumKnown ; i++)
		if (!sfxKnown[i].name[0])
			break;

	if (i == sfxNumKnown) {
		if (sfxNumKnown == MAX_SFX)
			Com_Error (ERR_FATAL, "Snd_FindSpot: MAX_SFX");
		sfxNumKnown++;
	}

	return &sfxKnown[i];
}


/*
==================
Snd_FindName
==================
*/
static sfx_t *Snd_FindName (char *name, qBool create) {
	int		i;
	sfx_t	*sfx;
	char	outName[MAX_QPATH];
//orly	qBool lol;

	if (!name)
		Com_Error (ERR_FATAL, "Snd_FindName: NULL name\n");
	if (!name[0])
		Com_Error (ERR_FATAL, "Snd_FindName: empty name\n");

	if (strlen (name) >= MAX_QPATH)
		Com_Error (ERR_FATAL, "Sound name too long: %s", name);

	// orly test
	Q_snprintfz (outName, sizeof (outName), "%s", name);/*
	lol=qFalse; i=0;
	while (outName[i]) {
		if (outName[i] == '\\') {
			outName[i] = '/'; lol=qTrue;
		}
		i++;
	}
	if (lol)
		Com_Printf (0, "lol %s -> %s\n", name, outName);*/
	// orly test

	// see if already loaded
	for (i=0 ; i<sfxNumKnown ; i++) {
		if (!strcmp (sfxKnown[i].name, outName))
			return &sfxKnown[i];
	}

	if (!create)
		return NULL;

	// find a free sfx
	sfx = Snd_FindSpot ();

	memset (sfx, 0, sizeof (*sfx));
	strcpy (sfx->name, outName);

	// bump
	sfx->sRegistrationFrame = sndRegistrationFrame;
	
	return sfx;
}


/*
==================
Snd_AliasName
==================
*/
sfx_t *Snd_AliasName (char *aliasName, char *trueName) {
	sfx_t	*sfx;
	char	*s;

	s = Mem_PoolAlloc (MAX_QPATH, MEMPOOL_SOUNDSYS, 0);
	strcpy (s, trueName);

	// find a free sfx
	sfx = Snd_FindSpot ();

	memset (sfx, 0, sizeof (*sfx));
	strcpy (sfx->name, aliasName);
	sfx->trueName = s;

	// bump
	sfx->sRegistrationFrame = sndRegistrationFrame;

	return sfx;
}


/*
=====================
Snd_BeginRegistration
=====================
*/
void Snd_BeginRegistration (void) {
	sndRegistrationFrame++;
}


/*
==================
Snd_RegisterSound
==================
*/
sfx_t *Snd_RegisterSound (char *name) {
	sfx_t	*sfx;

	if (!sndInitialized)
		return NULL;

	sfx = Snd_FindName (name, qTrue);
	Snd_LoadSound (sfx);

	return sfx;
}


/*
===============
Snd_RegisterSexedSound
===============
*/
static struct sfx_s *Snd_RegisterSexedSound (char *base, int entNum) {
	int				n;
	char			*p;
	struct sfx_s	*sfx;
	FILE			*f;
	char			model[MAX_QPATH];
	char			sexedFilename[MAX_QPATH];
	char			maleFilename[MAX_QPATH];

	// determine what model the client is using
	model[0] = 0;
	n = CS_PLAYERSKINS + entNum - 1;
	if (cl.configStrings[n][0]) {
		p = strchr (cl.configStrings[n], '\\');
		if (p) {
			p += 1;
			strcpy(model, p);
			p = strchr(model, '/');
			if (p)
				*p = 0;
		}
	}

	// if we can't figure it out, they're male
	if (!model[0])
		strcpy (model, "male");

	// see if we already know of the model specific sound
	Q_snprintfz (sexedFilename, sizeof (sexedFilename), "#players/%s/%s", model, base+1);
	sfx = Snd_FindName (sexedFilename, qFalse);

	if (!sfx) {
		// no, so see if it exists
		FS_FOpenFile (&sexedFilename[1], &f);
		if (f) {
			// yes, close the file and register it
			FS_FCloseFile (f);
			sfx = Snd_RegisterSound (sexedFilename);
		} else {
			// no, revert to the male sound in the pak0.pak
			Q_snprintfz (maleFilename, sizeof (maleFilename), "player/male/%s", base+1);
			sfx = Snd_AliasName (sexedFilename, maleFilename);
		}
	}

	return sfx;
}


/*
=====================
Snd_EndRegistration
=====================
*/
void Snd_EndRegistration (void) {
	int		i;
	sfx_t	*sfx;
	int		size;

	// free untouched sounds and make sure it is paged in
	for (i=0, sfx=sfxKnown ; i<sfxNumKnown ; i++, sfx++) {
		if (!sfx->name[0])
			continue;

		if (sfx->sRegistrationFrame != sndRegistrationFrame) {
			Snd_FreeSound (sfx);
		} else if (sfx->cache) {
			size = sfx->cache->length*sfx->cache->width;
			Com_PageInMemory ((byte *)sfx->cache, size);
		}
	}
}

/*
===============================================================================

	CHANNELS

===============================================================================
*/

/*
=================
Snd_PickChannel
=================
*/
channel_t *Snd_PickChannel (int entNum, int entChannel) {
	int			chanIndex;
	int			firstToDie;
	int			lifeLeft;
	channel_t	*ch;

	if (entChannel < 0)
		Com_Error (ERR_DROP, "Snd_PickChannel: entChannel < 0");

	// Check for replacement sound, or find the best one to replace
	firstToDie = -1;
	lifeLeft = 0x7fffffff;
	for (chanIndex=0 ; chanIndex<MAX_CHANNELS ; chanIndex++) {
		if ((entChannel != 0)	// channel 0 never overrides
			&& (sndChannels[chanIndex].entNum == entNum)
			&& (sndChannels[chanIndex].entChannel == entChannel)) {
			// always override sound from same entity
			firstToDie = chanIndex;
			break;
		}

		// don't let monster sounds override player sounds
		if ((sndChannels[chanIndex].entNum == cl.playerNum+1) && (entNum != cl.playerNum+1) && sndChannels[chanIndex].sfx)
			continue;

		if (sndChannels[chanIndex].end - sndPaintedTime < lifeLeft) {
			lifeLeft = sndChannels[chanIndex].end - sndPaintedTime;
			firstToDie = chanIndex;
		}
   }

	if (firstToDie == -1)
		return NULL;

	ch = &sndChannels[firstToDie];
	memset (ch, 0, sizeof (*ch));

	return ch;
}

/*
==============================================================================

	INIT / SHUTDOWN
 
==============================================================================
*/

/*
================
Snd_Restart

Restart the sound subsystem so it can pick up new parameters and flush all sounds
================
*/
void Snd_Restart (void) {
	Snd_Shutdown ();
	Snd_Init ();

	Snd_BeginRegistration ();
	CL_InitSoundMedia ();
	Snd_EndRegistration ();
}


/*
================
Snd_CheckChanges

Forces a sound restart now
================
*/
void Snd_CheckChanges (void) {
	if (sndRestart) {
		sndRestart = qFalse;
		Snd_Restart ();
	}
}


/*
================
Snd_Init

If forceOn is qTrue, force a restart
================
*/
void Snd_Init (void) {
	if (sndInitialized)
		Snd_Shutdown ();

	Com_Printf (0, "\n--------- Sound Initialization ---------\n");

	Cvar_GetLatchedVars (CVAR_LATCH_AUDIO);

	sndALActive = qFalse;
	sndInitialized = qFalse;
	sndRestart = qFalse;
	sndRegistrationFrame = 1;

	s_initsound = Cvar_Get ("s_initsound",		"1",	CVAR_ARCHIVE|CVAR_LATCH_AUDIO);

	Cmd_AddCommand ("snd_restart",	Snd_Restart_f,		"Restarts the audio subsystem");

	if (!s_initsound->value)
		Com_Printf (0, "...not initializing\n");
	else {
		int		sndInitTime;

		sndInitTime = Sys_Milliseconds ();

		s_volume		= Cvar_Get ("s_volume",			"0.7",	CVAR_ARCHIVE);

		al_dopplerfactor	= Cvar_Get ("al_dopplerfactor",		"1.0",			CVAR_ARCHIVE);
		al_dopplervelocity	= Cvar_Get ("al_dopplervelocity",	"400.0",		CVAR_ARCHIVE);
		al_driver			= Cvar_Get ("al_driver",			AL_DRIVERNAME,	CVAR_ARCHIVE|CVAR_LATCH_AUDIO);
		al_extensions		= Cvar_Get ("al_extensions",		"1",			CVAR_ARCHIVE|CVAR_LATCH_AUDIO);
		al_ext_eax			= Cvar_Get ("al_ext_eax",			"1",			CVAR_ARCHIVE|CVAR_LATCH_AUDIO);

		if (s_initsound->integer == 2) {
			if (OpenAL_Init ())
				sndInitialized = qTrue;
			else {
				Com_Printf (0, "----------------------------------------\n");
				Com_Printf (0, S_COLOR_YELLOW "OpenAL failed to initialize; attempting to fall back\n");
				Com_Printf (0, "----------------------------------------\n");
			}
		}

		if (!sndInitialized) {
			s_khz			= Cvar_Get ("s_khz",		"11",	CVAR_ARCHIVE|CVAR_LATCH_AUDIO);
			s_loadas8bit	= Cvar_Get ("s_loadas8bit",	"1",	CVAR_ARCHIVE|CVAR_LATCH_AUDIO);
			s_mixahead		= Cvar_Get ("s_mixahead",	"0.2",	CVAR_ARCHIVE);
			s_show			= Cvar_Get ("s_show",		"0",	0);
			s_testsound		= Cvar_Get ("s_testsound",	"0",	0);
			s_primary		= Cvar_Get ("s_primary",	"0",	CVAR_ARCHIVE|CVAR_LATCH_AUDIO);	// win32 specific

			if (!SndImp_Init ())
				return;

			Snd_ScaleTableInit ();

			sndInitialized = qTrue;

			sndSoundTime = 0;
			sndPaintedTime = 0;
		}

		Cmd_AddCommand ("play",			Snd_Play_f,				"Plays a sound");
		Cmd_AddCommand ("stopsound",	Snd_StopAllSounds,		"Stops all currently playing sounds");
		Cmd_AddCommand ("soundlist",	Snd_SoundList_f,		"Prints out a list of loaded sound files");
		Cmd_AddCommand ("soundinfo",	Snd_SoundInfo_f,		"Prints out information on sound subsystem");

		sfxNumKnown = 0;

		Snd_StopAllSounds ();

		Com_Printf (0, "----------------------------------------\n");
		Com_Printf (0, "init time: %.2fs\n", (Sys_Milliseconds () - sndInitTime)*0.001f);
	}

	// check memory integrity
	Mem_CheckPoolIntegrity (MEMPOOL_SOUNDSYS);

	Com_Printf (0, "----------------------------------------\n");
}


/*
================
Snd_Shutdown
================
*/
void Snd_Shutdown (void) {
	Cmd_RemoveCommand ("snd_restart");

	if (!sndInitialized)
		return;

	Cmd_RemoveCommand ("play");
	Cmd_RemoveCommand ("stopsound");
	Cmd_RemoveCommand ("soundlist");
	Cmd_RemoveCommand ("soundinfo");

	sndInitialized = qFalse;

	// free all sounds
	Snd_FreeSounds ();

	Mem_FreePool (MEMPOOL_SOUNDSYS);

	sndSoundTime = 0;
	sndPaintedTime = 0;

	sfxNumKnown = 0;

	if (sndALActive)
		OpenAL_Shutdown ();
	else
		SndImp_Shutdown ();
}


/*
================
Snd_FreeSounds
================
*/
void Snd_FreeSounds (void) {
	int		i;
	sfx_t	*sfx;

	for (i=0, sfx=sfxKnown ; i<sfxNumKnown ; i++, sfx++) {
		if (!sfx->name[0])
			continue;
		Snd_FreeSound (sfx);
	}
}

/*
===============================================================================

	PLAYSOUNDS

===============================================================================
*/

/*
=================
Snd_AllocPlaysound
=================
*/
playSound_t *Snd_AllocPlaysound (void) {
	playSound_t	*ps;

	ps = snd_FreePlays.next;
	if (ps == &snd_FreePlays)
		return NULL;	// no free playsounds

	// unlink from freelist
	ps->prev->next = ps->next;
	ps->next->prev = ps->prev;
	
	return ps;
}


/*
=================
Snd_FreePlaysound
=================
*/
void Snd_FreePlaysound (playSound_t *ps) {
	// unlink from channel
	ps->prev->next = ps->next;
	ps->next->prev = ps->prev;

	// add to free list
	ps->next = snd_FreePlays.next;
	snd_FreePlays.next->prev = ps;
	ps->prev = &snd_FreePlays;
	snd_FreePlays.next = ps;
}


/*
===============
Snd_IssuePlaysound

Take the next playsound and begin it on the channel. This is never
called directly by Snd_Play*, but only by the update loop.
===============
*/
void Snd_IssuePlaysound (playSound_t *ps) {
	channel_t	*ch;
	sfxCache_t	*sc;

	if (s_show->value)
		Com_Printf (0, "Issue %i\n", ps->begin);

	// pick a channel to play on
	ch = Snd_PickChannel (ps->entNum, ps->entChannel);
	if (!ch) {
		Snd_FreePlaysound (ps);
		return;
	}

	// spatialize
	if (ps->attenuation == ATTN_STATIC)
		ch->distMult = ps->attenuation * 0.001;
	else
		ch->distMult = ps->attenuation * 0.0005;

	ch->masterVol = ps->volume;
	ch->entNum = ps->entNum;
	ch->entChannel = ps->entChannel;
	ch->sfx = ps->sfx;
	VectorCopy (ps->origin, ch->origin);
	ch->fixedOrigin = ps->fixedOrigin;

	Snd_Spatialize (ch);

	ch->pos = 0;
	sc = Snd_LoadSound (ch->sfx);
	ch->end = sndPaintedTime + sc->length;

	// free the playsound
	Snd_FreePlaysound (ps);
}

/*
===============================================================================

	SOUND PLAYING

===============================================================================
*/

/*
====================
Snd_StartSound

Validates the parms and ques the sound up
if pos is NULL, the sound will be dynamically sourced from the entity
Entchannel 0 will never override a playing sound
====================
*/
void Snd_StartSound (vec3_t origin, int entNum, int entChannel, struct sfx_s *sfx, float volume, float attenuation, float timeOffset) {
	sfxCache_t	*sc;

	if (!sndInitialized)
		return;

	if (!sfx)
		return;

	if (sfx->name[0] == '*')
		sfx = Snd_RegisterSexedSound (sfx->name, entNum);

	// make sure the sound is loaded
	sc = Snd_LoadSound (sfx);
	if (!sc)
		return;		// couldn't load the sound's data

	if (sndALActive) {
		int		index;
		ALint	sourceNum;

		index = OpenAL_GetFreeALIndex ();
		if (index == -1) {
			Com_Printf (0, S_COLOR_YELLOW "Snd_StartSound [OpenAL]: WARNING: Out of indexes!\n");
			return;
		}

		sourceNum = OpenAL_GetFreeSource ();
		if (sourceNum == -1)
			Com_Printf (PRNT_DEV, S_COLOR_YELLOW "Snd_StartSound [OpenAL]: WARNING: Warning: Out of sources!\n");
		else {
			vec3_t	torigin;

			sndALIndexes[index].inUse = qTrue;

			if (origin) {
				sndALIndexes[index].fixedOrigin = qTrue;
				VectorCopy (origin, sndALIndexes[index].origin);
			} else
				sndALIndexes[index].fixedOrigin = qFalse;

			if (!attenuation) {
				Com_Printf (PRNT_DEV, "no attn, ent = %d, volume = %f\n", entNum, volume);

				qalSourcefv(sndALSources[sourceNum], AL_POSITION, vec3Origin);
				qalSourcef (sndALSources[sourceNum], AL_GAIN, 1.0f);
				qalSourcef (sndALSources[sourceNum], AL_ROLLOFF_FACTOR, 0.0f);
				qalSourcei (sndALSources[sourceNum], AL_SOURCE_RELATIVE, AL_TRUE);
				qalSourcef (sndALSources[sourceNum], AL_REFERENCE_DISTANCE, 128.0f);

				sndALIndexes[index].fixedOrigin = qTrue;
				VectorCopy (vec3Origin, sndALIndexes[index].origin);

				entNum = 0;
			} else {
				if (!origin) {	
					CL_CGModule_GetEntitySoundOrigin (entNum, torigin);
					origin = torigin;
				}

				qalSourcefv(sndALSources[sourceNum], AL_POSITION, origin);
				qalSourcef (sndALSources[sourceNum], AL_GAIN, clamp (volume, 0, 1));
				qalSourcef (sndALSources[sourceNum], AL_ROLLOFF_FACTOR, clamp (0.33f * attenuation, 0, 1));
				qalSourcef (sndALSources[sourceNum], AL_REFERENCE_DISTANCE, 256.0f * clamp (volume, 0, 1));
				qalSourcei (sndALSources[sourceNum], AL_SOURCE_RELATIVE, AL_FALSE);
			}

			sndALIndexes[index].sourceIndex = sourceNum;
			sndALIndexes[index].entNum = entNum;

			qalSourcei (sndALSources[sourceNum], AL_BUFFER, sndALBuffers[sfx->cache->bufferNum].buffer);
			qalSourcePlay (sndALSources[sourceNum]);

			OpenAL_CheckForError ();
		}
	} else {
		int			vol;
		playSound_t	*ps, *sort;
		int			start;
		static int	beginOfs = 0;

		vol = volume * 255;

		// make the playSound_t
		ps = Snd_AllocPlaysound ();
		if (!ps)
			return;

		if (origin) {
			VectorCopy (origin, ps->origin);
			ps->fixedOrigin = qTrue;
		} else
			ps->fixedOrigin = qFalse;

		ps->entNum = entNum;
		ps->entChannel = entChannel;
		ps->attenuation = attenuation;
		ps->volume = vol;
		ps->sfx = sfx;

		// drift beginOfs
		start = cl.frame.serverTime * 0.001 * sndDMA.speed + beginOfs;
		if (start < sndPaintedTime) {
			start = sndPaintedTime;
			beginOfs = start - (cl.frame.serverTime * 0.001 * sndDMA.speed);
		} else if (start > sndPaintedTime + 0.3 * sndDMA.speed) {
			start = sndPaintedTime + 0.1 * sndDMA.speed;
			beginOfs = start - (cl.frame.serverTime * 0.001 * sndDMA.speed);
		} else
			beginOfs -= 10;

		if (!timeOffset)
			ps->begin = sndPaintedTime;
		else
			ps->begin = start + timeOffset * sndDMA.speed;

		// sort into the pending sound list
		for (sort=sndPendingPlays.next ; (sort != &sndPendingPlays) && (sort->begin < ps->begin) ; sort=sort->next) ;

		ps->next = sort;
		ps->prev = sort->prev;

		ps->next->prev = ps;
		ps->prev->next = ps;
	}
}


/*
==================
Snd_StartLocalSound
==================
*/
void Snd_StartLocalSound (struct sfx_s *sfx) {
	if (!sndInitialized)
		return;

	Snd_StartSound (NULL, cl.playerNum+1, 0, sfx, 1, 0, 0);
}


/*
==================
Snd_AddLoopSounds

Entities with a ->sound field will generated looped sounds that are automatically
started, stopped, and merged together as the entities are sent to the client
==================
*/
static void Snd_AddLoopSounds (void) {
	if (cl_paused->integer || !cl.soundPrepped || !cl.refreshPrepped || (cls.connState != CA_ACTIVE))
		return;

	if (sndALActive) {
		// FIXME for OpenAL
	} else {
		int				i, j;
		int				sounds[MAX_CS_EDICTS];
		int				left, right;
		int				leftTotal, rightTotal;
		channel_t		*ch;
		sfx_t			*sfx;
		sfxCache_t		*sc;
		int				num;
		entityState_t	*ent;

		for (i=0 ; i<cl.frame.numEntities ; i++) {
			num = (cl.frame.parseEntities + i)&(MAX_PARSEENTITIES_MASK);
			ent = &clParseEntities[num];
			sounds[i] = ent->sound;
		}

		for (i=0 ; i<cl.frame.numEntities ; i++) {
			if (!sounds[i])
				continue;

			if (cl.configStrings[CS_SOUNDS+sounds[i]][0])
				sfx = Snd_RegisterSound (cl.configStrings[CS_SOUNDS+sounds[i]]);
			else
				continue;

			if (!sfx)
				continue;	// bad sound effect
			sc = sfx->cache;
			if (!sc)
				continue;

			num = (cl.frame.parseEntities + i)&(MAX_PARSEENTITIES_MASK);
			ent = &clParseEntities[num];

			// find the total contribution of all sounds of this type
			Snd_SpatializeOrigin (ent->origin, 255.0, SOUND_LOOPATTENUATE, &leftTotal, &rightTotal);
			for (j=i+1 ; j<cl.frame.numEntities ; j++) {
				if (sounds[j] != sounds[i])
					continue;
				sounds[j] = 0;	// don't check this again later

				num = (cl.frame.parseEntities + j)&(MAX_PARSEENTITIES_MASK);
				ent = &clParseEntities[num];

				Snd_SpatializeOrigin (ent->origin, 255.0, SOUND_LOOPATTENUATE, &left, &right);
				leftTotal += left;
				rightTotal += right;
			}

			if ((leftTotal == 0) && (rightTotal == 0))
				continue;	// not audible

			// allocate a channel
			ch = Snd_PickChannel (0, 0);
			if (!ch)
				return;

			if (leftTotal > 255)
				leftTotal = 255;
			if (rightTotal > 255)
				rightTotal = 255;

			ch->leftVol = leftTotal;
			ch->rightVol = rightTotal;
			ch->autoSound = qTrue;	// remove next frame
			ch->sfx = sfx;
			ch->pos = sndPaintedTime % sc->length;
			ch->end = sndPaintedTime + sc->length - ch->pos;
		}
	}
}


/*
============
Snd_RawSamples

Cinematic streaming and voice over network
============
*/
void Snd_RawSamples (int samples, int rate, int width, int channels, byte *data) {
	if (!sndInitialized)
		return;

	if (sndALActive) {
		// FIXME for OpenAL
	} else {
		int		i;
		int		src, dst;
		float	scale;

		if (sndRawEnd < sndPaintedTime)
			sndRawEnd = sndPaintedTime;
		scale = (float)rate / sndDMA.speed;

		if ((channels == 2) && (width == 2)) {
			if (scale == 1.0) {
				// optimized case
				for (i=0 ; i<samples ; i++) {
					dst = sndRawEnd & (MAX_RAW_SAMPLES-1);
					sndRawEnd++;
					sndRawSamples[dst].left = LittleShort(((short *)data)[i*2]) << 8;
					sndRawSamples[dst].right = LittleShort(((short *)data)[i*2+1]) << 8;
				}
			} else {
				for (i=0 ; ; i++) {
					src = i*scale;
					if (src >= samples)
						break;

					dst = sndRawEnd & (MAX_RAW_SAMPLES-1);
					sndRawEnd++;
					sndRawSamples[dst].left = LittleShort(((short *)data)[src*2]) << 8;
					sndRawSamples[dst].right = LittleShort(((short *)data)[src*2+1]) << 8;
				}
			}
		} else if ((channels == 1) && (width == 2)) {
			for (i=0 ; ; i++) {
				src = i*scale;
				if (src >= samples)
					break;
				dst = sndRawEnd & (MAX_RAW_SAMPLES-1);
				sndRawEnd++;
				sndRawSamples[dst].left = LittleShort(((short *)data)[src]) << 8;
				sndRawSamples[dst].right = LittleShort(((short *)data)[src]) << 8;
			}
		} else if ((channels == 2) && (width == 1)) {
			for (i=0 ; ; i++) {
				src = i*scale;
				if (src >= samples)
					break;
				dst = sndRawEnd & (MAX_RAW_SAMPLES-1);
				sndRawEnd++;
				sndRawSamples[dst].left = ((char *)data)[src*2] << 16;
				sndRawSamples[dst].right = ((char *)data)[src*2+1] << 16;
			}
		} else if ((channels == 1) && (width == 1)) {
			for (i=0 ; ; i++) {
				src = i*scale;
				if (src >= samples)
					break;
				dst = sndRawEnd & (MAX_RAW_SAMPLES-1);
				sndRawEnd++;
				sndRawSamples[dst].left = (((byte *)data)[src]-128) << 16;
				sndRawSamples[dst].right = (((byte *)data)[src]-128) << 16;
			}
		}
	}
}

/*
===============================================================================

	SOUND STOPPING

===============================================================================
*/

/*
==================
Snd_ClearBuffer
==================
*/
void Snd_ClearBuffer (void) {	
	if (!sndInitialized)
		return;

	if (sndALActive) {
		OpenAL_DestroyBuffers ();
	} else {
		int		clear;

		sndRawEnd = 0;

		if (sndDMA.sampleBits == 8)
			clear = 0x80;
		else
			clear = 0;

		SndImp_BeginPainting ();
		if (sndDMA.buffer)
			memset (sndDMA.buffer, clear, sndDMA.samples * sndDMA.sampleBits/8);
		SndImp_Submit ();
	}
}


/*
==================
Snd_StopAllSounds
==================
*/
void Snd_StopAllSounds (void) {
	int		i;

	if (!sndInitialized)
		return;

	// clear all the playsounds
	memset (snd_PlaySounds, 0, sizeof (snd_PlaySounds));
	snd_FreePlays.next = snd_FreePlays.prev = &snd_FreePlays;
	sndPendingPlays.next = sndPendingPlays.prev = &sndPendingPlays;

	for (i=0 ; i<MAX_PLAYSOUNDS ; i++) {
		snd_PlaySounds[i].prev = &snd_FreePlays;
		snd_PlaySounds[i].next = snd_FreePlays.next;
		snd_PlaySounds[i].prev->next = &snd_PlaySounds[i];
		snd_PlaySounds[i].next->prev = &snd_PlaySounds[i];
	}

	// clear all the channels
	memset (sndChannels, 0, sizeof (sndChannels));

	Snd_ClearBuffer ();
}

/*
===============================================================================

	SPATIALIZATION

===============================================================================
*/

/*
=================
Snd_SpatializeOrigin

Used for spatializing channels and autosounds
=================
*/
void Snd_SpatializeOrigin (vec3_t origin, float masterVol, float distMult, int *leftVol, int *rightVol) {
	float		dot, dist;
	float		leftScale, rightScale, scale;
	vec3_t		sourceVec;

	if (cls.connState != CA_ACTIVE) {
		*leftVol = *rightVol = 255;
		return;
	}

	// calculate stereo seperation and distance attenuation
	VectorSubtract (origin, sndViewOrigin, sourceVec);

	dist = VectorNormalize (sourceVec, sourceVec) - SOUND_FULLVOLUME;
	if (dist < 0)
		dist = 0;			// close enough to be at full volume
	dist *= distMult;		// different attenuation levels
	
	dot = DotProduct (sndViewRight, sourceVec);

	if ((sndDMA.channels == 1) || !distMult) {
		// no attenuation = no spatialization
		rightScale = 1.0;
		leftScale = 1.0;
	} else {
		rightScale = 0.5 * (1.0 + dot);
		leftScale = 0.5 * (1.0 - dot);
	}

	// add in distance effect
	scale = (1.0 - dist) * rightScale;
	*rightVol = (int) (masterVol * scale);
	if (*rightVol < 0)
		*rightVol = 0;

	scale = (1.0 - dist) * leftScale;
	*leftVol = (int) (masterVol * scale);
	if (*leftVol < 0)
		*leftVol = 0;
}


/*
=================
Snd_Spatialize
=================
*/
void Snd_Spatialize (channel_t *ch) {
	vec3_t		origin;

	// anything coming from the view entity will always be full volume
	if (ch->entNum == cl.playerNum+1) {
		ch->leftVol = ch->masterVol;
		ch->rightVol = ch->masterVol;
		return;
	}

	if (ch->fixedOrigin)
		VectorCopy (ch->origin, origin);
	else
		CL_CGModule_GetEntitySoundOrigin (ch->entNum, origin);

	Snd_SpatializeOrigin (origin, ch->masterVol, ch->distMult, &ch->leftVol, &ch->rightVol);
}


/*
============
Snd_Update

Called once each time through the main loop
============
*/
void Snd_Update (vec3_t viewOrigin, vec3_t forward, vec3_t right, vec3_t up, qBool underWater) {
	CDAudio_Update ();

	VectorCopy (viewOrigin, sndViewOrigin);
	VectorCopy (forward, sndViewForward);
	VectorCopy (right, sndViewRight);
	VectorCopy (up, sndViewUp);

	if (!sndInitialized)
		return;

	/*
	** if the laoding plaque is up, clear everything out to make sure we aren't
	** looping a dirty dma buffer while loading
	*/
	if (cls.disableScreen) {
		Snd_ClearBuffer ();
		return;
	}

	if (sndALActive) {
		ALfloat	orientation[6];
		int		i;

		// update listener information
		orientation[0] = (ALfloat)sndViewForward[0];
		orientation[1] = (ALfloat)sndViewForward[1];
		orientation[2] = (ALfloat)sndViewForward[2];
		orientation[3] = (ALfloat)sndViewUp[0];
		orientation[4] = (ALfloat)sndViewUp[1];
		orientation[5] = (ALfloat)sndViewUp[2];
		qalListenerfv (AL_ORIENTATION, orientation);
		qalListenerfv (AL_POSITION, sndViewOrigin);
		qalListenerf (AL_GAIN, sndActive ? s_volume->value : 0.0);

		qalDistanceModel (AL_INVERSE_DISTANCE_CLAMPED);

		qalDopplerFactor (al_dopplerfactor->value);
		qalDopplerVelocity (al_dopplervelocity->value);

		// update AL origins
		for (i=0 ; i<MAX_CS_SOUNDS ; i++) {
			if (!sndALIndexes[i].inUse)
				continue;

			if (sndALIndexes[i].fixedOrigin)
				qalSourcefv (sndALSources[sndALIndexes[i].sourceIndex], AL_POSITION, sndALIndexes[i].origin);
			else {
				vec3_t entOrigin;
				CL_CGModule_GetEntitySoundOrigin (sndALIndexes[i].entNum, entOrigin);
				qalSourcefv (sndALSources[sndALIndexes[i].sourceIndex], AL_POSITION, entOrigin);
			}
		}

		// If EAX is enabled, apply listener environmental effects
		if (alConfig.extEAX) {
			uInt	eaxEnv;

			if ((cls.connState != CA_ACTIVE) || cl.attractLoop)
				eaxEnv = EAX_ENVIRONMENT_PLAIN;
			else {
				if (underWater)
					eaxEnv = EAX_ENVIRONMENT_UNDERWATER;
				else
					eaxEnv = EAX_ENVIRONMENT_PLAIN;
			}

			qalEAXSet (&DSPROPSETID_EAX20_ListenerProperties, DSPROPERTY_EAXLISTENER_ENVIRONMENT | DSPROPERTY_EAXLISTENER_DEFERRED, 0, &eaxEnv, sizeof(eaxEnv));
		}

		// check for an error
		OpenAL_CheckForError ();
	} else {
		int			i;
		int			total;
		uInt		endtime;
		int			samps;
		channel_t	*ch;
		channel_t	*combine;

		int			samplepos;
		static int	buffers;
		static int	oldsamplepos;
		int			fullsamples;

		// rebuild scale tables if volume is modified
		if (s_volume->modified)
			Snd_ScaleTableInit ();

		combine = NULL;

		// update spatialization for dynamic sounds
		ch = sndChannels;
		for (i=0 ; i<MAX_CHANNELS; i++, ch++) {
			if (!ch->sfx)
				continue;

			if (ch->autoSound) {
				// autosounds are regenerated fresh each frame
				memset (ch, 0, sizeof (*ch));
				continue;
			}

			Snd_Spatialize (ch);		// respatialize channel
			if (!ch->leftVol && !ch->rightVol) {
				memset (ch, 0, sizeof (*ch));
				continue;
			}
		}

		// add loopsounds
		Snd_AddLoopSounds ();

		// debugging output
		if (s_show->value) {
			total = 0;
			ch = sndChannels;
			for (i=0 ; i<MAX_CHANNELS; i++, ch++) {
				if (ch->sfx && (ch->leftVol || ch->rightVol)) {
					Com_Printf (0, "%3i %3i %s\n", ch->leftVol, ch->rightVol, ch->sfx->name);
					total++;
				}
			}
			
			Com_Printf (0, "----(%i)---- painted: %i\n", total, sndPaintedTime);
		}

		// mix some sound
		SndImp_BeginPainting ();

		if (!sndDMA.buffer)
			return;

		// update DMA time
		fullsamples = sndDMA.samples / sndDMA.channels;

		/*
		** it is possible to miscount buffers if it has wrapped twice between
		** calls to Snd_Update. Oh well
		*/
		samplepos = SndImp_GetDMAPos ();

		if (samplepos < oldsamplepos) {
			buffers++;	// buffer wrapped
			
			if (sndPaintedTime > 0x40000000) {
				// time to chop things off to avoid 32 bit limits
				buffers = 0;
				sndPaintedTime = fullsamples;
				Snd_StopAllSounds ();
			}
		}

		oldsamplepos = samplepos;
		sndSoundTime = buffers*fullsamples + samplepos/sndDMA.channels;

		// check to make sure that we haven't overshot
		if (sndPaintedTime < sndSoundTime) {
			Com_Printf (PRNT_DEV, S_COLOR_YELLOW "Snd_Update: overflow\n");
			sndPaintedTime = sndSoundTime;
		}

		// mix ahead of current position
		endtime = sndSoundTime + s_mixahead->value * sndDMA.speed;

		// mix to an even submission block size
		endtime = (endtime + sndDMA.submissionChunk-1) & ~(sndDMA.submissionChunk-1);
		samps = sndDMA.samples >> (sndDMA.channels-1);
		if (endtime - sndSoundTime > samps)
			endtime = sndSoundTime + samps;

		Snd_PaintChannels (endtime);

		SndImp_Submit ();
	}
}
