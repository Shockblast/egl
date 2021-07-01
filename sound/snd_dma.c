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

#define USE_BYTESWAP
#include "snd_loc.h"

channel_t			snd_outChannels[MAX_CHANNELS];

static qBool		snd_isInitialized = qFalse;
static qBool		snd_queueRestart;

dma_t				snd_audioDMA;

int					snd_soundTime;		// sample PAIRS
int					snd_paintedTime;	// sample PAIRS

// sound registration
#define MAX_SFX_HASH	64

static sfx_t		snd_sfxList[MAX_SFX];
static sfx_t		*snd_sfxHashList[MAX_SFX_HASH];
static int			snd_numSFX;

static uLong		snd_registrationFrame;

// play sounds
playSound_t			snd_playSounds[MAX_PLAYSOUNDS];
playSound_t			snd_freePlays;
playSound_t			snd_pendingPlays;

// raw
int					snd_rawEnd;
sfxSamplePair_t		snd_rawSamples[MAX_RAW_SAMPLES];

// orientation
vec3_t	snd_viewOrigin;
vec3_t	snd_viewForward;
vec3_t	snd_viewRight;
vec3_t	snd_viewUp;

cVar_t	*s_volume;
cVar_t	*s_testsound;
cVar_t	*s_loadas8bit;
cVar_t	*s_khz;
cVar_t	*s_show;
cVar_t	*s_mixahead;
cVar_t	*s_primary;
cVar_t	*s_initsound;

/*
===============================================================================

	WAV LOADING AND RESAMPLING

===============================================================================
*/

static byte		*snd_dataPtr;
static byte		*snd_iffEnd;
static byte		*snd_lastIffChunk;
static byte		*snd_iffData;
static int		snd_iffChunkLength;

/*
============
GetLittleShort
============
*/
static short GetLittleShort (void)
{
	short val = 0;

	val = *snd_dataPtr;
	val = val + (*(snd_dataPtr+1)<<8);

	snd_dataPtr += 2;

	return val;
}


/*
============
GetLittleLong
============
*/
static int GetLittleLong (void)
{
	int val = 0;

	val = *snd_dataPtr;
	val = val + (*(snd_dataPtr+1)<<8);
	val = val + (*(snd_dataPtr+2)<<16);
	val = val + (*(snd_dataPtr+3)<<24);

	snd_dataPtr += 4;

	return val;
}


/*
============
FindNextChunk
============
*/
static void FindNextChunk (char *name)
{
	for ( ; ; ) {
		snd_dataPtr = snd_lastIffChunk;

		snd_dataPtr += 4;
		if (snd_dataPtr >= snd_iffEnd) {
			// Didn't find the chunk
			snd_dataPtr = NULL;
			return;
		}

		snd_iffChunkLength = GetLittleLong ();
		if (snd_iffChunkLength < 0) {
			snd_dataPtr = NULL;
			return;
		}

		snd_dataPtr -= 8;
		snd_lastIffChunk = snd_dataPtr + 8 + ((snd_iffChunkLength + 1) & ~1);
		if (!strncmp (snd_dataPtr, name, 4))
			return;
	}
}


/*
============
FindChunk
============
*/
static void FindChunk (char *name)
{
	snd_lastIffChunk = snd_iffData;
	FindNextChunk (name);
}


/*
============
Snd_GetWavinfo
============
*/
static wavInfo_t Snd_GetWavinfo (char *name, byte *wav, int wavLength)
{
	wavInfo_t	info;
	int			i;
	int			format;
	int			samples;

	memset (&info, 0, sizeof (info));

	if (!wav)
		return info;
		
	snd_iffData = wav;
	snd_iffEnd = wav + wavLength;

	// Find "RIFF" chunk
	FindChunk ("RIFF");
	if (!(snd_dataPtr && !strncmp (snd_dataPtr+8, "WAVE", 4))) {
		Com_Printf (0, "Missing RIFF/WAVE chunks\n");
		return info;
	}

	// Get "fmt " chunk
	snd_iffData = snd_dataPtr + 12;

	FindChunk ("fmt ");
	if (!snd_dataPtr) {
		Com_Printf (0, "Missing fmt chunk\n");
		return info;
	}

	snd_dataPtr += 8;
	format = GetLittleShort ();
	if (format != 1) {
		Com_Printf (0, "Microsoft PCM format only\n");
		return info;
	}

	info.channels = GetLittleShort();
	info.rate = GetLittleLong ();
	snd_dataPtr += 4+2;
	info.width = GetLittleShort () / 8;

	// Get cue chunk
	FindChunk ("cue ");
	if (snd_dataPtr) {
		snd_dataPtr += 32;
		info.loopStart = GetLittleLong ();

		// If the next chunk is a LIST chunk, look for a cue length marker
		FindNextChunk ("LIST");
		if (snd_dataPtr) {
			if (!strncmp (snd_dataPtr + 28, "mark", 4)) {
				// This is not a proper parse, but it works with cooledit
				snd_dataPtr += 24;
				i = GetLittleLong ();	// Samples in loop
				info.samples = info.loopStart + i;
			}
		}
	}
	else
		info.loopStart = -1;

	// Find data chunk
	FindChunk ("data");
	if (!snd_dataPtr) {
		Com_Printf (0, "Missing data chunk\n");
		return info;
	}

	snd_dataPtr += 4;
	samples = GetLittleLong () / info.width;

	if (info.samples) {
		if (samples < info.samples)
			Com_Error (ERR_DROP, "Sound %s has a bad loop length", name);
	}
	else
		info.samples = samples;

	info.dataOfs = snd_dataPtr - wav;
	
	return info;
}


/*
================
Snd_ResampleSfx
================
*/
static void Snd_ResampleSfx (sfx_t *sfx, int inRate, int inWidth, byte *data)
{
	int		outcount;
	int		srcsample;
	float	stepscale;
	int		i;
	int		sample, samplefrac, fracstep;
	sfxCache_t	*sc;
	
	sc = sfx->cache;
	if (!sc)
		return;

	stepscale = (float)inRate / snd_audioDMA.speed;	// This is usually 0.5, 1, or 2

	outcount = sc->length / stepscale;
	sc->length = outcount;
	if (sc->loopStart != -1)
		sc->loopStart = sc->loopStart / stepscale;

	sc->speed = snd_audioDMA.speed;
	if (s_loadas8bit->value)
		sc->width = 1;
	else
		sc->width = inWidth;
	sc->stereo = 0;

	// Resample / decimate to the current source rate
	if (stepscale == 1 && inWidth == 1 && sc->width == 1) {
		// Fast special case
		for (i=0 ; i<outcount ; i++)
			((signed char *)sc->data)[i] = (int)((byte)(data[i]) - 128);
	}
	else {
		// General case
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
sfxCache_t *Snd_LoadSound (sfx_t *s)
{
    char		namebuffer[MAX_QPATH];
	byte		*data;
	wavInfo_t	info;
	int			len;
	float		stepscale;
	sfxCache_t	*sc;
	int			size;
	char		*name;

	s->touchFrame = snd_registrationFrame;
	if (s->name[0] == '*')
		return NULL;

	// See if still in memory
	sc = s->cache;
	if (sc)
		return sc;

	// Load it in
	if (s->trueName)
		name = s->trueName;
	else
		name = s->name;

	if (name[0] == '#')
		Q_strncpyz (namebuffer, &name[1], sizeof (namebuffer));
	else
		Q_snprintfz (namebuffer, sizeof (namebuffer), "sound/%s", name);

	size = FS_LoadFile (namebuffer, (void **)&data);

	if (!data) {
		Com_Printf (PRNT_DEV, "Snd_LoadSound: Couldn't load %s\n", namebuffer);
		return NULL;
	}

	info = Snd_GetWavinfo (s->name, data, size);
	if (info.channels != 1) {
		Com_Printf (0, "Snd_LoadSound: %s is a stereo sample\n", s->name);
		FS_FreeFile (data);
		return NULL;
	}

	stepscale = (float)info.rate / snd_audioDMA.speed;	
	len = (info.samples / stepscale) * info.width * info.channels;

	sc = s->cache = Mem_PoolAlloc (len + sizeof (sfxCache_t), MEMPOOL_SOUNDSYS, 0);

	if (!sc) {
		FS_FreeFile (data);
		return NULL;
	}

	sc->length = info.samples;
	sc->loopStart = info.loopStart;
	sc->speed = info.rate;
	sc->width = info.width;
	sc->stereo = info.channels;

	Snd_ResampleSfx (s, sc->speed, sc->width, data + info.dataOfs);

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
static void Snd_Play_f (void)
{
	char	name[256];
	sfx_t	*sfx;
	int		i;
	
	i = 1;
	while (i < Cmd_Argc ()) {
		if (!strrchr (Cmd_Argv (i), '.')) {
			Q_snprintfz (name, sizeof (name), "%s.wav", Cmd_Argv (i));
		}
		else
			Q_strncpyz (name, Cmd_Argv (i), sizeof (name));
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
static void Snd_Restart_f (void)
{
	snd_queueRestart = qTrue;
}


/*
============
Snd_SoundList_f
============
*/
static void Snd_SoundList_f (void)
{
	int			i;
	sfx_t		*sfx;
	sfxCache_t	*sc;
	int			total;
	int			size;

	for (sfx=snd_sfxList, total=0, i=0 ; i<snd_numSFX ; i++, sfx++) {
		if (!sfx->touchFrame)
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
		}
		else {
			if (sfx->name[0] == '*')
				Com_Printf (0, "  placeholder : %s\n", sfx->name);
			else
				Com_Printf (0, "  not loaded  : %s\n", sfx->name);
		}
	}

	Com_Printf (0, "Total resident: %i\n", total);
}


/*
=================
Snd_SoundInfo_f
=================
*/
static void Snd_SoundInfo_f (void)
{
	if (!snd_isInitialized) {
		Com_Printf (0, S_COLOR_YELLOW "Sound system not started\n");
		return;
	}

	Com_Printf (0, "%5d stereo\n", snd_audioDMA.channels - 1);
	Com_Printf (0, "%5d samples\n", snd_audioDMA.samples);
	Com_Printf (0, "%5d samplepos\n", snd_audioDMA.samplePos);
	Com_Printf (0, "%5d samplebits\n", snd_audioDMA.sampleBits);
	Com_Printf (0, "%5d submission_chunk\n", snd_audioDMA.submissionChunk);
	Com_Printf (0, "%5d speed\n", snd_audioDMA.speed);
	Com_Printf (0, "0x%x dma buffer\n", snd_audioDMA.buffer);
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
static void Snd_FreeSound (sfx_t *sfx)
{
	sfx_t	*hashSfx;
	sfx_t	**prev;

	if (!sfx)
		return;

	// De-link it from the hash tree
	prev = &snd_sfxHashList[sfx->hashValue];
	for ( ; ; ) {
		hashSfx = *prev;
		if (!hashSfx)
			break;

		if (hashSfx == sfx) {
			*prev = hashSfx->hashNext;
			break;
		}
		prev = &hashSfx->hashNext;
	}

	if (sfx->trueName)
		Mem_Free (sfx->trueName);
	if (sfx->cache)				// It is possible to have a leftover
		Mem_Free (sfx->cache);	// from a server that didn't finish loading
	memset (sfx, 0, sizeof (*sfx));
}


/*
==================
Snd_FindSpot
==================
*/
static sfx_t *Snd_FindSpot (void)
{
	int		i;

	for (i=0 ; i<snd_numSFX ; i++)
		if (!snd_sfxList[i].name[0])
			break;

	if (i == snd_numSFX) {
		if (snd_numSFX == MAX_SFX)
			Com_Error (ERR_FATAL, "Snd_FindSpot: MAX_SFX");
		snd_numSFX++;
	}

	return &snd_sfxList[i];
}


/*
==================
Snd_FindName
==================
*/
static sfx_t *Snd_FindName (char *name, qBool create)
{
	sfx_t	*sfx;
	char	outName[MAX_QPATH];
	uLong	hash;

	if (!name)
		Com_Error (ERR_FATAL, "Snd_FindName: NULL name\n");
	if (!name[0])
		Com_Error (ERR_FATAL, "Snd_FindName: empty name\n");

	if (strlen (name) >= MAX_QPATH)
		Com_Error (ERR_FATAL, "Snd_FindName: Sound name too long: %s", name);

	// Copy, then normalize the path, but don't strip the extension
	Q_strncpyz (outName, name, sizeof (outName));
	Q_FixPathName (outName, sizeof (outName), 3, qFalse);

	// See if already loaded
	hash = CalcHash (name, MAX_SFX_HASH);
	for (sfx=snd_sfxHashList[hash] ; sfx ; sfx=sfx->hashNext) {
		if (!strcmp (sfx->name, outName))
			return sfx;
	}

	if (!create)
		return NULL;

	// Find a free sfx
	sfx = Snd_FindSpot ();

	// Fill
	memset (sfx, 0, sizeof (*sfx));
	Q_strncpyz (sfx->name, outName, sizeof (sfx->name));
	sfx->hashValue = CalcHash (sfx->name, MAX_SFX_HASH);

	// Bump
	sfx->touchFrame = snd_registrationFrame;

	// Link it in
	sfx->hashNext = snd_sfxHashList[sfx->hashValue];
	snd_sfxHashList[sfx->hashValue] = sfx;

	return sfx;
}


/*
==================
Snd_AliasName
==================
*/
static sfx_t *Snd_AliasName (char *aliasName, char *trueName)
{
	sfx_t	*sfx;
	int		len;

	len = strlen (trueName);
	if (len+1 >= MAX_QPATH)
		Com_Error (ERR_FATAL, "Snd_AliasName: Sound name too long: %s", trueName);

	// Find a free sfx
	sfx = Snd_FindSpot ();

	memset (sfx, 0, sizeof (*sfx));

	// Fill
	Q_strncpyz (sfx->name, aliasName, sizeof (sfx->name));
	sfx->trueName = Mem_PoolAlloc (len+1, MEMPOOL_SOUNDSYS, 0);
	strcpy (sfx->trueName, trueName);
	sfx->hashValue = CalcHash (sfx->name, MAX_SFX_HASH);

	// Bump
	sfx->touchFrame = snd_registrationFrame;

	// Link it in
	sfx->hashNext = snd_sfxHashList[sfx->hashValue];
	snd_sfxHashList[sfx->hashValue] = sfx;

	return sfx;
}


/*
=====================
Snd_BeginRegistration
=====================
*/
void Snd_BeginRegistration (void)
{
	snd_registrationFrame++;
}


/*
==================
Snd_RegisterSound
==================
*/
sfx_t *Snd_RegisterSound (char *name)
{
	sfx_t	*sfx;

	if (!snd_isInitialized)
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
static struct sfx_s *Snd_RegisterSexedSound (char *base, int entNum)
{
	char			model[MAX_QPATH];
	char			sexedFilename[MAX_QPATH];
	char			maleFilename[MAX_QPATH];
	int				fileNum, n;
	struct sfx_s	*sfx;
	char			*p;

	// Determine what model the client is using
	model[0] = 0;
	n = CS_PLAYERSKINS + entNum - 1;
	if (cl.configStrings[n][0]) {
		p = strchr (cl.configStrings[n], '\\');
		if (p) {
			p += 1;
			Q_strncpyz (model, p, sizeof (model));
			p = strchr (model, '/');
			if (p)
				*p = 0;
		}
	}

	// If we can't figure it out, they're male
	if (!model[0])
		Q_strncpyz (model, "male", sizeof (model));

	// See if we already know of the model specific sound
	Q_snprintfz (sexedFilename, sizeof (sexedFilename), "#players/%s/%s", model, base+1);
	sfx = Snd_FindName (sexedFilename, qFalse);

	if (!sfx) {
		// No, so see if it exists
		FS_OpenFile (&sexedFilename[1], &fileNum, FS_MODE_READ_BINARY);
		if (fileNum) {
			// Yes, close the file and register it
			FS_CloseFile (fileNum);
			sfx = Snd_RegisterSound (sexedFilename);
		}
		else {
			// No, revert to the male sound in the pak0.pak
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
void Snd_EndRegistration (void)
{
	sfx_t	*sfx;
	int		size;
	int		i;

	// Free untouched sounds and make sure it is paged in
	for (i=0, sfx=snd_sfxList ; i<snd_numSFX ; i++, sfx++) {
		if (!sfx->name[0])
			continue;

		if (sfx->touchFrame != snd_registrationFrame) {
			Snd_FreeSound (sfx);
		}
		else if (sfx->cache) {
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
channel_t *Snd_PickChannel (int entNum, int entChannel)
{
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
		if (entChannel != 0	// channel 0 never overrides
			&& snd_outChannels[chanIndex].entNum == entNum
			&& snd_outChannels[chanIndex].entChannel == entChannel) {
			// always override sound from same entity
			firstToDie = chanIndex;
			break;
		}

		// Don't let monster sounds override player sounds
		if (snd_outChannels[chanIndex].entNum == cl.playerNum+1 && entNum != cl.playerNum+1 && snd_outChannels[chanIndex].sfx)
			continue;

		if (snd_outChannels[chanIndex].end - snd_paintedTime < lifeLeft) {
			lifeLeft = snd_outChannels[chanIndex].end - snd_paintedTime;
			firstToDie = chanIndex;
		}
   }

	if (firstToDie == -1)
		return NULL;

	ch = &snd_outChannels[firstToDie];
	memset (ch, 0, sizeof (*ch));

	return ch;
}

/*
==============================================================================

	INIT / SHUTDOWN
 
==============================================================================
*/

static cmdFunc_t	*cmd_snd_restart;

static cmdFunc_t	*cmd_play;
static cmdFunc_t	*cmd_stopSound;
static cmdFunc_t	*cmd_soundList;
static cmdFunc_t	*cmd_soundInfo;

/*
================
Snd_Restart

Restart the sound subsystem so it can pick up new parameters and flush all sounds
================
*/
void Snd_Restart (void)
{
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
void Snd_CheckChanges (void)
{
	if (snd_queueRestart) {
		snd_queueRestart = qFalse;
		Snd_Restart ();
	}
}


/*
================
Snd_Init

If forceOn is qTrue, force a restart
================
*/
void Snd_Init (void)
{
	if (snd_isInitialized)
		Snd_Shutdown ();

	Com_Printf (0, "\n--------- Sound Initialization ---------\n");

	Cvar_GetLatchedVars (CVAR_LATCH_AUDIO);

	snd_isInitialized = qFalse;
	snd_queueRestart = qFalse;
	snd_registrationFrame = 1;

	s_initsound = Cvar_Get ("s_initsound",		"1",	CVAR_ARCHIVE|CVAR_LATCH_AUDIO);

	cmd_snd_restart = Cmd_AddCommand ("snd_restart",	Snd_Restart_f,		"Restarts the audio subsystem");

	if (!s_initsound->value)
		Com_Printf (0, "...not initializing\n");
	else {
		int		sndInitTime;

		sndInitTime = Sys_Milliseconds ();

		s_volume		= Cvar_Get ("s_volume",			"0.7",	CVAR_ARCHIVE);

		if (!snd_isInitialized) {
			s_khz			= Cvar_Get ("s_khz",		"11",	CVAR_ARCHIVE|CVAR_LATCH_AUDIO);
			s_loadas8bit	= Cvar_Get ("s_loadas8bit",	"1",	CVAR_ARCHIVE|CVAR_LATCH_AUDIO);
			s_mixahead		= Cvar_Get ("s_mixahead",	"0.2",	CVAR_ARCHIVE);
			s_show			= Cvar_Get ("s_show",		"0",	0);
			s_testsound		= Cvar_Get ("s_testsound",	"0",	0);
			s_primary		= Cvar_Get ("s_primary",	"0",	CVAR_ARCHIVE|CVAR_LATCH_AUDIO);	// win32 specific

			if (!SndImp_Init ())
				return;

			Snd_ScaleTableInit ();

			snd_isInitialized = qTrue;

			snd_soundTime = 0;
			snd_paintedTime = 0;
		}

		cmd_play		= Cmd_AddCommand ("play",			Snd_Play_f,				"Plays a sound");
		cmd_stopSound	= Cmd_AddCommand ("stopsound",	Snd_StopAllSounds,		"Stops all currently playing sounds");
		cmd_soundList	= Cmd_AddCommand ("soundlist",	Snd_SoundList_f,		"Prints out a list of loaded sound files");
		cmd_soundInfo	= Cmd_AddCommand ("soundinfo",	Snd_SoundInfo_f,		"Prints out information on sound subsystem");

		snd_numSFX = 0;

		Snd_StopAllSounds ();

		Com_Printf (0, "----------------------------------------\n");
		Com_Printf (0, "init time: %.2fs\n", (Sys_Milliseconds () - sndInitTime)*0.001f);
	}

	// Check memory integrity
	Mem_CheckPoolIntegrity (MEMPOOL_SOUNDSYS);

	Com_Printf (0, "----------------------------------------\n");
}


/*
================
Snd_Shutdown
================
*/
void Snd_Shutdown (void)
{
	Cmd_RemoveCommand ("snd_restart", cmd_snd_restart);

	if (!snd_isInitialized)
		return;

	Cmd_RemoveCommand ("play", cmd_play);
	Cmd_RemoveCommand ("stopsound", cmd_stopSound);
	Cmd_RemoveCommand ("soundlist", cmd_soundList);
	Cmd_RemoveCommand ("soundinfo", cmd_soundInfo);

	snd_isInitialized = qFalse;

	// Free all sounds
	Snd_FreeSounds ();

	Mem_FreePool (MEMPOOL_SOUNDSYS);

	snd_soundTime = 0;
	snd_paintedTime = 0;

	snd_numSFX = 0;

	SndImp_Shutdown ();
}


/*
================
Snd_FreeSounds
================
*/
void Snd_FreeSounds (void)
{
	int		i;
	sfx_t	*sfx;

	for (i=0, sfx=snd_sfxList ; i<snd_numSFX ; i++, sfx++) {
		if (!sfx->name[0])
			continue;
		Snd_FreeSound (sfx);
	}

	memset (snd_sfxList, 0, sizeof (snd_sfxList));
	memset (snd_sfxHashList, 0, sizeof (snd_sfxHashList));
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
static playSound_t *Snd_AllocPlaysound (void)
{
	playSound_t	*ps;

	ps = snd_freePlays.next;
	if (ps == &snd_freePlays)
		return NULL;	// no free playsounds

	// Unlink from freelist
	ps->prev->next = ps->next;
	ps->next->prev = ps->prev;
	
	return ps;
}


/*
=================
Snd_FreePlaysound
=================
*/
static void Snd_FreePlaysound (playSound_t *ps)
{
	// Unlink from channel
	ps->prev->next = ps->next;
	ps->next->prev = ps->prev;

	// Add to free list
	ps->next = snd_freePlays.next;
	snd_freePlays.next->prev = ps;
	ps->prev = &snd_freePlays;
	snd_freePlays.next = ps;
}


/*
===============
Snd_IssuePlaysound

Take the next playsound and begin it on the channel. This is never
called directly by Snd_Play*, but only by the update loop.
===============
*/
void Snd_IssuePlaysound (playSound_t *ps)
{
	channel_t	*ch;
	sfxCache_t	*sc;

	if (s_show->value)
		Com_Printf (0, "Issue %i\n", ps->begin);

	// Pick a channel to play on
	ch = Snd_PickChannel (ps->entNum, ps->entChannel);
	if (!ch) {
		Snd_FreePlaysound (ps);
		return;
	}

	// Spatialize
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
	ch->end = snd_paintedTime + sc->length;

	// Free the playsound
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
void Snd_StartSound (vec3_t origin, int entNum, int entChannel, struct sfx_s *sfx, float volume, float attenuation, float timeOffset)
{
	sfxCache_t	*sc;
	int			vol;
	playSound_t	*ps, *sort;
	int			start;
	static int	beginOfs = 0;

	if (!snd_isInitialized)
		return;

	if (!sfx)
		return;

	if (sfx->name[0] == '*')
		sfx = Snd_RegisterSexedSound (sfx->name, entNum);

	// Make sure the sound is loaded
	sc = Snd_LoadSound (sfx);
	if (!sc)
		return;		// Couldn't load the sound's data

	vol = volume * 255;

	// Make the playSound_t
	ps = Snd_AllocPlaysound ();
	if (!ps)
		return;

	if (origin) {
		VectorCopy (origin, ps->origin);
		ps->fixedOrigin = qTrue;
	}
	else
		ps->fixedOrigin = qFalse;

	ps->entNum = entNum;
	ps->entChannel = entChannel;
	ps->attenuation = attenuation;
	ps->volume = vol;
	ps->sfx = sfx;

	// Drift beginOfs
	start = cl.frame.serverTime * 0.001 * snd_audioDMA.speed + beginOfs;
	if (start < snd_paintedTime) {
		start = snd_paintedTime;
		beginOfs = start - (cl.frame.serverTime * 0.001 * snd_audioDMA.speed);
	}
	else if (start > snd_paintedTime + 0.3 * snd_audioDMA.speed) {
		start = snd_paintedTime + 0.1 * snd_audioDMA.speed;
		beginOfs = start - (cl.frame.serverTime * 0.001 * snd_audioDMA.speed);
	}
	else
		beginOfs -= 10;

	if (!timeOffset)
		ps->begin = snd_paintedTime;
	else
		ps->begin = start + timeOffset * snd_audioDMA.speed;

	// Sort into the pending sound list
	for (sort=snd_pendingPlays.next ; (sort != &snd_pendingPlays) && (sort->begin < ps->begin) ; sort=sort->next) ;

	ps->next = sort;
	ps->prev = sort->prev;

	ps->next->prev = ps;
	ps->prev->next = ps;
}


/*
==================
Snd_StartLocalSound
==================
*/
void Snd_StartLocalSound (struct sfx_s *sfx)
{
	if (!snd_isInitialized)
		return;

	Snd_StartSound (NULL, cl.playerNum+1, CHAN_AUTO, sfx, 1, 0, 0);
}


/*
==================
Snd_AddLoopSounds

Entities with a ->sound field will generated looped sounds that are automatically
started, stopped, and merged together as the entities are sent to the client
==================
*/
static void Snd_AddLoopSounds (void)
{
	int				i, j;
	int				sounds[MAX_CS_EDICTS];
	int				left, right;
	int				leftTotal, rightTotal;
	channel_t		*ch;
	sfx_t			*sfx;
	sfxCache_t		*sc;
	int				num;
	entityState_t	*ent;
	vec3_t			origin;

	if (cl_paused->integer || cls.connectState != CA_ACTIVE || !cls.soundPrepped)
		return;

	for (i=0 ; i<cl.frame.numEntities ; i++) {
		num = (cl.frame.parseEntities + i)&(MAX_PARSEENTITIES_MASK);
		ent = &cl_parseEntities[num];
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
			continue;	// Bad sound effect
		sc = sfx->cache;
		if (!sc)
			continue;

		num = (cl.frame.parseEntities + i) & MAX_PARSEENTITIES_MASK;
		ent = &cl_parseEntities[num];

		// Special case for bmodels
		if (ent->solid == 31) {
			cBspModel_t		*cModel = CM_InlineModel (cl.configStrings[CS_MODELS+ent->modelIndex]);

			if (cModel) {
				origin[0] = ent->origin[0] + 0.5 * (cModel->mins[0]+cModel->maxs[0]);
				origin[1] = ent->origin[1] + 0.5 * (cModel->mins[1]+cModel->maxs[1]);
				origin[2] = ent->origin[2] + 0.5 * (cModel->mins[2]+cModel->maxs[2]);
			}
			else {
				origin[0] = ent->origin[0];
				origin[1] = ent->origin[1];
				origin[2] = ent->origin[2];
			}
		}
		else {
			origin[0] = ent->origin[0];
			origin[1] = ent->origin[1];
			origin[2] = ent->origin[2];
		}

		// Find the total contribution of all sounds of this type
		Snd_SpatializeOrigin (origin, 255.0f, SOUND_LOOPATTENUATE, &leftTotal, &rightTotal);
		for (j=i+1 ; j<cl.frame.numEntities ; j++) {
			if (sounds[j] != sounds[i])
				continue;
			sounds[j] = 0;	// Don't check this again later

			num = (cl.frame.parseEntities + j)&(MAX_PARSEENTITIES_MASK);
			ent = &cl_parseEntities[num];

			Snd_SpatializeOrigin (origin, 255.0f, SOUND_LOOPATTENUATE, &left, &right);
			leftTotal += left;
			rightTotal += right;
		}

		if (leftTotal == 0 && rightTotal == 0)
			continue;	// Not audible

		// Allocate a channel
		ch = Snd_PickChannel (0, 0);
		if (!ch)
			return;

		if (leftTotal > 255)
			leftTotal = 255;
		if (rightTotal > 255)
			rightTotal = 255;

		ch->leftVol = leftTotal;
		ch->rightVol = rightTotal;
		ch->autoSound = qTrue;	// Remove next frame
		ch->sfx = sfx;
		ch->pos = snd_paintedTime % sc->length;
		ch->end = snd_paintedTime + sc->length - ch->pos;
	}
}


/*
============
Snd_RawSamples

Cinematic streaming and voice over network
============
*/
void Snd_RawSamples (int samples, int rate, int width, int channels, byte *data)
{
	int		i;
	int		src, dst;
	float	scale;

	// Why do anything if sound is off?
	if (!snd_isInitialized)
		return;

	if (snd_rawEnd < snd_paintedTime)
		snd_rawEnd = snd_paintedTime;
	scale = (float)rate / snd_audioDMA.speed;

	// Channels
	switch (channels) {
	case 1:
		// Width
		switch (width) {
		case 1:
			// Channels: 1 -- Width: 1
			for (i=0 ; ; i++) {
				src = i*scale;
				if (src >= samples)
					break;
				dst = snd_rawEnd & (MAX_RAW_SAMPLES-1);
				snd_rawEnd++;
				snd_rawSamples[dst].left = (((byte *)data)[src]-128) << 16;
				snd_rawSamples[dst].right = (((byte *)data)[src]-128) << 16;
			}
			break;

		case 2:
			// Channels: 1 -- Width: 2
			for (i=0 ; ; i++) {
				src = i*scale;
				if (src >= samples)
					break;
				dst = snd_rawEnd & (MAX_RAW_SAMPLES-1);
				snd_rawEnd++;
				snd_rawSamples[dst].left = LittleShort(((short *)data)[src]) << 8;
				snd_rawSamples[dst].right = LittleShort(((short *)data)[src]) << 8;
			}
			break;
		}
		break;

	case 2:
		// Width
		switch (width) {
		case 1:
			// Channels: 2 -- Width: 1
			for (i=0 ; ; i++) {
				src = i*scale;
				if (src >= samples)
					break;
				dst = snd_rawEnd & (MAX_RAW_SAMPLES-1);
				snd_rawEnd++;
				snd_rawSamples[dst].left = ((char *)data)[src*2] << 16;
				snd_rawSamples[dst].right = ((char *)data)[src*2+1] << 16;
			}
			break;

		case 2:
			// Channels: 2 -- Width: 2
			if (scale == 1.0) {
				// Optimized case
				for (i=0 ; i<samples ; i++) {
					dst = snd_rawEnd & (MAX_RAW_SAMPLES-1);
					snd_rawEnd++;
					snd_rawSamples[dst].left = LittleShort(((short *)data)[i*2]) << 8;
					snd_rawSamples[dst].right = LittleShort(((short *)data)[i*2+1]) << 8;
				}
			}
			else {
				for (i=0 ; ; i++) {
					src = i*scale;
					if (src >= samples)
						break;

					dst = snd_rawEnd & (MAX_RAW_SAMPLES-1);
					snd_rawEnd++;
					snd_rawSamples[dst].left = LittleShort(((short *)data)[src*2]) << 8;
					snd_rawSamples[dst].right = LittleShort(((short *)data)[src*2+1]) << 8;
				}
			}
			break;
		}
		break;
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
void Snd_ClearBuffer (void)
{	
	int		clear;

	if (!snd_isInitialized)
		return;

	snd_rawEnd = 0;

	if (snd_audioDMA.sampleBits == 8)
		clear = 0x80;
	else
		clear = 0;

	SndImp_BeginPainting ();
	if (snd_audioDMA.buffer)
		memset (snd_audioDMA.buffer, clear, snd_audioDMA.samples * snd_audioDMA.sampleBits/8);
	SndImp_Submit ();
}


/*
==================
Snd_StopAllSounds
==================
*/
void Snd_StopAllSounds (void)
{
	int		i;

	if (!snd_isInitialized)
		return;

	// Clear all the playsounds
	memset (snd_playSounds, 0, sizeof (snd_playSounds));
	snd_freePlays.next = snd_freePlays.prev = &snd_freePlays;
	snd_pendingPlays.next = snd_pendingPlays.prev = &snd_pendingPlays;

	for (i=0 ; i<MAX_PLAYSOUNDS ; i++) {
		snd_playSounds[i].prev = &snd_freePlays;
		snd_playSounds[i].next = snd_freePlays.next;
		snd_playSounds[i].prev->next = &snd_playSounds[i];
		snd_playSounds[i].next->prev = &snd_playSounds[i];
	}

	// Clear all the channels
	memset (snd_outChannels, 0, sizeof (snd_outChannels));

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
void Snd_SpatializeOrigin (vec3_t origin, float masterVol, float distMult, int *leftVol, int *rightVol)
{
	float		dot, dist;
	float		leftScale, rightScale, scale;
	vec3_t		sourceVec;

	if (cls.connectState != CA_ACTIVE) {
		*leftVol = *rightVol = 255;
		return;
	}

	// Calculate stereo seperation and distance attenuation
	VectorSubtract (origin, snd_viewOrigin, sourceVec);

	dist = VectorNormalize (sourceVec, sourceVec) - SOUND_FULLVOLUME;
	if (dist < 0)
		dist = 0;			// close enough to be at full volume
	dist *= distMult;		// different attenuation levels
	
	dot = DotProduct (snd_viewRight, sourceVec);

	if (snd_audioDMA.channels == 1 || !distMult) {
		// No attenuation = no spatialization
		rightScale = 1.0;
		leftScale = 1.0;
	}
	else {
		rightScale = 0.5 * (1.0 + dot);
		leftScale = 0.5 * (1.0 - dot);
	}

	// Add in distance effect
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
void Snd_Spatialize (channel_t *ch)
{
	vec3_t		origin;

	// Anything coming from the view entity will always be full volume
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
void Snd_Update (vec3_t viewOrigin, vec3_t forward, vec3_t right, vec3_t up, qBool underWater)
{
	int			i;
	int			total;
	uInt		endtime;
	uInt			samps;
	channel_t	*ch;
	channel_t	*combine;

	int			samplepos;
	static int	buffers;
	static int	oldsamplepos;
	int			fullsamples;

	VectorCopy (viewOrigin, snd_viewOrigin);
	VectorCopy (forward, snd_viewForward);
	VectorCopy (right, snd_viewRight);
	VectorCopy (up, snd_viewUp);

	if (!snd_isInitialized)
		return;

	// Don't play sounds while the screen is disabled
	if (cls.disableScreen) {
		Snd_ClearBuffer ();
		return;
	}

	// Rebuild scale tables if volume is modified
	if (s_volume->modified)
		Snd_ScaleTableInit ();

	combine = NULL;

	// Update spatialization for dynamic sounds
	ch = snd_outChannels;
	for (i=0 ; i<MAX_CHANNELS; i++, ch++) {
		if (!ch->sfx)
			continue;

		if (ch->autoSound) {
			// Autosounds are regenerated fresh each frame
			memset (ch, 0, sizeof (*ch));
			continue;
		}

		Snd_Spatialize (ch);	// Respatialize channel
		if (!ch->leftVol && !ch->rightVol) {
			memset (ch, 0, sizeof (*ch));
			continue;
		}
	}

	// Add loopsounds
	Snd_AddLoopSounds ();

	// Debugging output
	if (s_show->value) {
		total = 0;
		ch = snd_outChannels;
		for (i=0 ; i<MAX_CHANNELS; i++, ch++) {
			if (ch->sfx && (ch->leftVol || ch->rightVol)) {
				Com_Printf (0, "%3i %3i %s\n", ch->leftVol, ch->rightVol, ch->sfx->name);
				total++;
			}
		}
		
		Com_Printf (0, "----(%i)---- painted: %i\n", total, snd_paintedTime);
	}

	// Mix some sound
	SndImp_BeginPainting ();

	if (!snd_audioDMA.buffer)
		return;

	// Update DMA time
	fullsamples = snd_audioDMA.samples / snd_audioDMA.channels;

	/*
	** It is possible to miscount buffers if it has wrapped twice between
	** calls to Snd_Update. Oh well
	*/
	samplepos = SndImp_GetDMAPos ();

	if (samplepos < oldsamplepos) {
		buffers++;	// Buffer wrapped
		
		if (snd_paintedTime > 0x40000000) {
			// Time to chop things off to avoid 32 bit limits
			buffers = 0;
			snd_paintedTime = fullsamples;
			Snd_StopAllSounds ();
		}
	}

	oldsamplepos = samplepos;
	snd_soundTime = buffers*fullsamples + samplepos/snd_audioDMA.channels;

	// Check to make sure that we haven't overshot
	if (snd_paintedTime < snd_soundTime) {
		Com_Printf (PRNT_DEV, S_COLOR_YELLOW "Snd_Update: overflow\n");
		snd_paintedTime = snd_soundTime;
	}

	// Mix ahead of current position
	endtime = snd_soundTime + s_mixahead->value * snd_audioDMA.speed;

	// Mix to an even submission block size
	endtime = (endtime + snd_audioDMA.submissionChunk-1) & ~(snd_audioDMA.submissionChunk-1);
	samps = snd_audioDMA.samples >> (snd_audioDMA.channels-1);
	if (endtime - snd_soundTime > samps)
		endtime = snd_soundTime + samps;

	Snd_PaintChannels (endtime);

	SndImp_Submit ();
}
