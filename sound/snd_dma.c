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

// snd_dma.c
// - main control for any streaming sound output device

#include "snd_loc.h"

// only begin attenuating sound volumes when outside the FULLVOLUME range
#define		SOUND_FULLVOLUME	80
#define		SOUND_LOOPATTENUATE	0.003

channel_t   snd_Channels[MAX_CHANNELS];

qBool		snd_Initialized = qFalse;
qBool		snd_IsRegistering = qFalse;
int			snd_RegistrationFrame;

dma_t		dma;

int			snd_SoundTime;		// sample PAIRS
int			snd_PaintedTime;	// sample PAIRS

// during registration it is possible to have more sounds than could
// actually be referenced during gameplay, because we don't want to
// free anything until we are sure we won't need it
#define		MAX_SFX		(MAX_SOUNDS*2)
sfx_t		sfx_Known[MAX_SFX];
int			sfx_NumKnown;

#define		MAX_PLAYSOUNDS	128
playSound_t	snd_PlaySounds[MAX_PLAYSOUNDS];
playSound_t	snd_FreePlays;
playSound_t	snd_PendingPlays;

int						snd_RawEnd;
portable_samplepair_t	snd_RawSamples[MAX_RAW_SAMPLES];

qBool		snd_Restart;

cvar_t		*s_volume;
cvar_t		*s_testsound;
cvar_t		*s_loadas8bit;
cvar_t		*s_khz;
cvar_t		*s_show;
cvar_t		*s_mixahead;
cvar_t		*s_primary;
cvar_t		*s_initsound;

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
void Snd_Play_f (void) {
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
============
Snd_SoundList_f
============
*/
void Snd_SoundList_f (void) {
	int			i;
	sfx_t		*sfx;
	sfxCache_t	*sc;

	if (snd_ALActive) {
		for (sfx=sfx_Known, i=0 ; i<sfx_NumKnown ; i++, sfx++) {
			if (!sfx->sRegistrationFrame)
				continue;

			sc = sfx->cache;
			if (sc) {
				Com_Printf (PRINT_ALL, "%3i          %s\n", sc->bufferNum, sfx->name);
			} else {
				if (sfx->name[0] == '*')
					Com_Printf (PRINT_ALL, "(placehold): %s\n", sfx->name);
				else
					Com_Printf (PRINT_ALL, "(notloaded): %s\n", sfx->name);
			}
		}
	} else {
		int		total;
		int		size;

		for (sfx=sfx_Known, total=0, i=0 ; i<sfx_NumKnown ; i++, sfx++) {
			if (!sfx->sRegistrationFrame)
				continue;
			sc = sfx->cache;
			if (sc) {
				size = sc->length * sc->width * (sc->stereo + 1);
				total += size;
				if (sc->loopStart >= 0)
					Com_Printf (PRINT_ALL, "L");
				else
					Com_Printf (PRINT_ALL, " ");
				Com_Printf (PRINT_ALL, "(%2db) %6i : %s\n", sc->width*8, size, sfx->name);
			} else {
				if (sfx->name[0] == '*')
					Com_Printf (PRINT_ALL, "  placeholder : %s\n", sfx->name);
				else
					Com_Printf (PRINT_ALL, "  not loaded  : %s\n", sfx->name);
			}
		}

		Com_Printf (PRINT_ALL, "Total resident: %i\n", total);
	}
}


/*
=================
Snd_SoundInfo_f
=================
*/
void Snd_SoundInfo_f (void) {
	if (!snd_Initialized) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Sound system not started\n");
		return;
	}

	if (snd_ALActive) {
		int		minor, major;

		alcGetIntegerv (snd_ALDevice, ALC_MAJOR_VERSION, sizeof (major), &major);
		alcGetIntegerv (snd_ALDevice, ALC_MINOR_VERSION, sizeof (minor), &minor);
		Com_Printf (PRINT_ALL, "OpenAL %d.%d on %s\n", major, minor, alcGetString (snd_ALDevice, ALC_DEFAULT_DEVICE_SPECIFIER));

		Com_Printf (PRINT_ALL, "AL_VENDOR: %s\n", alGetString (AL_VENDOR));
		Com_Printf (PRINT_ALL, "AL_VERSION: %s\n", alGetString (AL_VERSION));
		Com_Printf (PRINT_ALL, "AL_EXTENSIONS: %s\n", alGetString (AL_EXTENSIONS));
	} else {
		Com_Printf (PRINT_ALL, "%5d stereo\n", dma.channels - 1);
		Com_Printf (PRINT_ALL, "%5d samples\n", dma.samples);
		Com_Printf (PRINT_ALL, "%5d samplepos\n", dma.samplePos);
		Com_Printf (PRINT_ALL, "%5d samplebits\n", dma.sampleBits);
		Com_Printf (PRINT_ALL, "%5d submission_chunk\n", dma.submissionChunk);
		Com_Printf (PRINT_ALL, "%5d speed\n", dma.speed);
		Com_Printf (PRINT_ALL, "0x%x dma buffer\n", dma.buffer);
	}
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
	Snd_RegisterSounds ();
}


/*
================
Snd_Restart_f

For queueing a sound restart
================
*/
void Snd_Restart_f (void) {
	snd_Restart = qTrue;
}


/*
================
Snd_CheckChanges

Forces a sound restart now
================
*/
void Snd_CheckChanges (void) {
	if (snd_Restart) {
		snd_Restart = qFalse;
		Snd_Restart ();
	}
}


/*
================
Snd_Init
================
*/
void Snd_Init (void) {
	Com_Printf (PRINT_ALL, "\n--------- Sound Initialization ---------\n");

	snd_ALActive = qFalse;
	snd_Initialized = qFalse;
	snd_Restart = qFalse;
	snd_RegistrationFrame = 1;

	s_initsound = Cvar_Get ("s_initsound",		"1",	CVAR_ARCHIVE);

	Cmd_AddCommand ("snd_restart",	Snd_Restart_f);

	if (!s_initsound->value)
		Com_Printf (PRINT_ALL, "...not initializing\n");
	else {
		int		sndInitTime;

		sndInitTime = Sys_Milliseconds ();

		s_volume		= Cvar_Get ("s_volume",		"0.7",	CVAR_ARCHIVE);

		Cmd_AddCommand ("play",			Snd_Play_f);
		Cmd_AddCommand ("stopsound",	Snd_StopAllSounds);
		Cmd_AddCommand ("soundlist",	Snd_SoundList_f);
		Cmd_AddCommand ("soundinfo",	Snd_SoundInfo_f);

		if (s_initsound->integer == 2) {
			if (OpenAL_Init ())
				snd_Initialized = qTrue;
			else
				Com_Printf (PRINT_ALL, S_COLOR_YELLOW "OpenAL failed to initialize; attempting to fall back\n");
		}

		if (!snd_Initialized) {
			s_khz			= Cvar_Get ("s_khz",		"11",	CVAR_ARCHIVE);
			s_loadas8bit	= Cvar_Get ("s_loadas8bit",	"1",	CVAR_ARCHIVE);
			s_mixahead		= Cvar_Get ("s_mixahead",	"0.2",	CVAR_ARCHIVE);
			s_show			= Cvar_Get ("s_show",		"0",	0);
			s_testsound		= Cvar_Get ("s_testsound",	"0",	0);
			s_primary		= Cvar_Get ("s_primary",	"0",	CVAR_ARCHIVE);	// win32 specific

			if (!SNDDMA_Init ())
				return;

			Snd_InitScaleTable ();

			snd_Initialized = qTrue;

			snd_SoundTime = 0;
			snd_PaintedTime = 0;
		}

		sfx_NumKnown = 0;

		Snd_StopAllSounds ();

		Com_Printf (PRINT_ALL, "----------------------------------------\n");
		Com_Printf (PRINT_ALL, "init time: %dms\n", Sys_Milliseconds () - sndInitTime);
	}

	Com_Printf (PRINT_ALL, "----------------------------------------\n");
}


/*
================
Snd_Shutdown
================
*/
void Snd_Shutdown (void) {
	int		i;
	sfx_t	*sfx;

	Cmd_RemoveCommand ("snd_restart");

	if (!snd_Initialized)
		return;

	snd_Initialized = qFalse;

	/* free all sounds */
	for (i=0, sfx=sfx_Known ; i<sfx_NumKnown ; i++, sfx++) {
		if (!sfx->name[0])
			continue;
		if (sfx->cache)
			Z_Free (sfx->cache);
		memset (sfx, 0, sizeof (*sfx));
	}

	Z_FreeTags (ZTAG_SOUNDSYS);
	sfx_NumKnown = 0;

	if (snd_ALActive)
		OpenAL_Shutdown ();
	else
		SNDDMA_Shutdown ();

	Cmd_RemoveCommand ("play");
	Cmd_RemoveCommand ("stopsound");
	Cmd_RemoveCommand ("soundlist");
	Cmd_RemoveCommand ("soundinfo");

	snd_Initialized = qFalse;
}

/*
==============================================================================

	SOUND REGISTRATION
 
==============================================================================
*/

/*
==================
Snd_FindName
==================
*/
sfx_t *Snd_FindName (char *name, qBool create) {
	int		i;
	sfx_t	*sfx;

	if (!name)
		Com_Error (ERR_FATAL, "Snd_FindName: NULL\n");
	if (!name[0])
		Com_Error (ERR_FATAL, "Snd_FindName: empty name\n");

	if (strlen(name) >= MAX_QPATH)
		Com_Error (ERR_FATAL, "Sound name too long: %s", name);

	/* see if already loaded */
	for (i=0 ; i<sfx_NumKnown ; i++) {
		if (!strcmp(sfx_Known[i].name, name))
			return &sfx_Known[i];
	}

	if (!create)
		return NULL;

	/* find a free sfx */
	for (i=0 ; i<sfx_NumKnown ; i++)
		if (!sfx_Known[i].name[0])
			break;

	if (i == sfx_NumKnown) {
		if (sfx_NumKnown == MAX_SFX)
			Com_Error (ERR_FATAL, "Snd_FindName: out of sfx_t");
		sfx_NumKnown++;
	}
	
	sfx = &sfx_Known[i];
	memset (sfx, 0, sizeof (*sfx));
	strcpy (sfx->name, name);
	sfx->sRegistrationFrame = snd_RegistrationFrame;
	
	return sfx;
}


/*
==================
Snd_AliasName
==================
*/
sfx_t *Snd_AliasName (char *aliasname, char *trueName) {
	sfx_t	*sfx;
	char	*s;
	int		i;

	s = Z_TagMalloc (MAX_QPATH, ZTAG_SOUNDSYS);
	strcpy (s, trueName);

	/* find a free sfx */
	for (i=0 ; i<sfx_NumKnown ; i++)
		if (!sfx_Known[i].name[0])
			break;

	if (i == sfx_NumKnown) {
		if (sfx_NumKnown == MAX_SFX)
			Com_Error (ERR_FATAL, "Snd_FindName: out of sfx_t");
		sfx_NumKnown++;
	}
	
	sfx = &sfx_Known[i];
	memset (sfx, 0, sizeof (*sfx));
	strcpy (sfx->name, aliasname);
	sfx->sRegistrationFrame = snd_RegistrationFrame;
	sfx->trueName = s;

	return sfx;
}


/*
=====================
Snd_BeginRegistration
=====================
*/
void Snd_BeginRegistration (void) {
	snd_RegistrationFrame++;
	snd_IsRegistering = qTrue;
}


/*
==================
Snd_RegisterSound
==================
*/
sfx_t *Snd_RegisterSound (char *name) {
	sfx_t	*sfx;

	if (!snd_Initialized)
		return NULL;

	sfx = Snd_FindName (name, qTrue);
	sfx->sRegistrationFrame = snd_RegistrationFrame;

	if (!snd_IsRegistering)
		Snd_LoadSound (sfx);

	return sfx;
}


/*
===============
Snd_RegisterSexedSound
===============
*/
struct sfx_s *Snd_RegisterSexedSound (char *base, int entNum) {
	int				n;
	char			*p;
	struct sfx_s	*sfx;
	FILE			*f;
	char			model[MAX_QPATH];
	char			sexedFilename[MAX_QPATH];
	char			maleFilename[MAX_QPATH];

	/* determine what model the client is using */
	model[0] = 0;
	n = CS_PLAYERSKINS + entNum - 1;
	if (cl.configStrings[n][0]) {
		p = strchr(cl.configStrings[n], '\\');
		if (p) {
			p += 1;
			strcpy(model, p);
			p = strchr(model, '/');
			if (p)
				*p = 0;
		}
	}

	/* if we can't figure it out, they're male */
	if (!model[0])
		strcpy(model, "male");

	/* see if we already know of the model specific sound */
	Com_sprintf (sexedFilename, sizeof(sexedFilename), "#players/%s/%s", model, base+1);
	sfx = Snd_FindName (sexedFilename, qFalse);

	if (!sfx) {
		/* no, so see if it exists */
		FS_FOpenFile (&sexedFilename[1], &f);
		if (f) {
			/* yes, close the file and register it */
			FS_FCloseFile (f);
			sfx = Snd_RegisterSound (sexedFilename);
		} else {
			/* no, revert to the male sound in the pak0.pak */
			Com_sprintf (maleFilename, sizeof(maleFilename), "player/%s/%s", "male", base+1);
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

	/* free any sounds not from this registration sequence */
	for (i=0, sfx=sfx_Known ; i<sfx_NumKnown ; i++, sfx++) {
		if (!sfx->name[0])
			continue;
		if (sfx->sRegistrationFrame != snd_RegistrationFrame) {
			/* don't need this sound */
			if (sfx->cache)	// it is possible to have a leftover
				Z_Free (sfx->cache);	// from a server that didn't finish loading
			memset (sfx, 0, sizeof (*sfx));
		} else {
			/* make sure it is paged in */
			if (sfx->cache) {
				size = sfx->cache->length*sfx->cache->width;
				Com_PageInMemory ((byte *)sfx->cache, size);
			}
		}

	}

	/* load everything in */
	for (i=0, sfx=sfx_Known ; i<sfx_NumKnown ; i++, sfx++) {
		if (!sfx->name[0])
			continue;

		Snd_LoadSound (sfx);
	}

	snd_IsRegistering = qFalse;
}


/*
======================
Snd_RegisterSounds
======================
*/
void Snd_RegisterSounds (void) {
	int		i;

	Snd_BeginRegistration ();
	CL_RegisterClientSounds ();

	for (i=1 ; i<MAX_SOUNDS ; i++) {
		if (!cl.configStrings[CS_SOUNDS+i][0])
			break;

		cl.soundPrecache[i] = Snd_RegisterSound (cl.configStrings[CS_SOUNDS+i]);
		Sys_SendKeyEvents ();	// pump message loop
	}

	Snd_EndRegistration ();
}

//=============================================================================

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

	/* Check for replacement sound, or find the best one to replace */
	firstToDie = -1;
	lifeLeft = 0x7fffffff;
	for (chanIndex=0 ; chanIndex<MAX_CHANNELS ; chanIndex++) {
		if ((entChannel != 0)	// channel 0 never overrides
			&& (snd_Channels[chanIndex].entNum == entNum)
			&& (snd_Channels[chanIndex].entChannel == entChannel)) {
			/* always override sound from same entity */
			firstToDie = chanIndex;
			break;
		}

		/* don't let monster sounds override player sounds */
		if ((snd_Channels[chanIndex].entNum == cl.playerNum+1) && (entNum != cl.playerNum+1) && snd_Channels[chanIndex].sfx)
			continue;

		if (snd_Channels[chanIndex].end - snd_PaintedTime < lifeLeft) {
			lifeLeft = snd_Channels[chanIndex].end - snd_PaintedTime;
			firstToDie = chanIndex;
		}
   }

	if (firstToDie == -1)
		return NULL;

	ch = &snd_Channels[firstToDie];
	memset (ch, 0, sizeof (*ch));

	return ch;
}


/*
=================
Snd_SpatializeOrigin

Used for spatializing channels and autosounds
=================
*/
void Snd_SpatializeOrigin (vec3_t origin, float master_vol, float dist_mult, int *left_vol, int *right_vol) {
	vec_t		dot;
	vec_t		dist;
	vec_t		lscale, rscale, scale;
	vec3_t		source_vec;

	if (!CL_ConnectionState (CA_ACTIVE)) {
		*left_vol = *right_vol = 255;
		return;
	}

	/* calculate stereo seperation and distance attenuation */
	VectorSubtract (origin, cl.refDef.viewOrigin, source_vec);

	dist = VectorNormalize (source_vec, source_vec);
	dist -= SOUND_FULLVOLUME;
	if (dist < 0)
		dist = 0;			// close enough to be at full volume
	dist *= dist_mult;		// different attenuation levels
	
	dot = DotProduct (cl.v_Right, source_vec);

	if ((dma.channels == 1) || !dist_mult) {
		/* no attenuation = no spatialization */
		rscale = 1.0;
		lscale = 1.0;
	} else {
		rscale = 0.5 * (1.0 + dot);
		lscale = 0.5 * (1.0 - dot);
	}

	/* add in distance effect */
	scale = (1.0 - dist) * rscale;
	*right_vol = (int) (master_vol * scale);
	if (*right_vol < 0)
		*right_vol = 0;

	scale = (1.0 - dist) * lscale;
	*left_vol = (int) (master_vol * scale);
	if (*left_vol < 0)
		*left_vol = 0;
}


/*
=================
Snd_Spatialize
=================
*/
void Snd_Spatialize (channel_t *ch) {
	vec3_t		origin;

	/* anything coming from the view entity will always be full volume */
	if (ch->entNum == cl.playerNum+1) {
		ch->leftVol = ch->masterVol;
		ch->rightVol = ch->masterVol;
		return;
	}

	if (ch->fixedOrigin)
		VectorCopy (ch->origin, origin);
	else
		CL_GetEntitySoundOrigin (ch->entNum, origin);

	Snd_SpatializeOrigin (origin, ch->masterVol, ch->distMult, &ch->leftVol, &ch->rightVol);
}


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

	/* unlink from freelist */
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
	/* unlink from channel */
	ps->prev->next = ps->next;
	ps->next->prev = ps->prev;

	/* add to free list */
	ps->next = snd_FreePlays.next;
	snd_FreePlays.next->prev = ps;
	ps->prev = &snd_FreePlays;
	snd_FreePlays.next = ps;
}


/*
===============
Snd_IssuePlaysound

Take the next playsound and begin it on the channel
This is never called directly by Snd_Play*, but only
by the update loop.
===============
*/
void Snd_IssuePlaysound (playSound_t *ps) {
	channel_t	*ch;
	sfxCache_t	*sc;

	if (s_show->value)
		Com_Printf (PRINT_ALL, "Issue %i\n", ps->begin);

	/* pick a channel to play on */
	ch = Snd_PickChannel (ps->entNum, ps->entChannel);
	if (!ch) {
		Snd_FreePlaysound (ps);
		return;
	}

	/* spatialize */
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
	ch->end = snd_PaintedTime + sc->length;

	/* free the playsound */
	Snd_FreePlaysound (ps);
}


/*
====================
Snd_StartSound

Validates the parms and ques the sound up
if pos is NULL, the sound will be dynamically sourced from the entity
Entchannel 0 will never override a playing sound
====================
*/
void Snd_StartSound (vec3_t origin, int entNum, int entChannel, struct sfx_s *sfx, float fVol, float attenuation, float timeOfs) {
	sfxCache_t	*sc;

	if (!snd_Initialized)
		return;

	if (!sfx)
		return;

	if (sfx->name[0] == '*')
		sfx = Snd_RegisterSexedSound (sfx->name, entNum);

	/* make sure the sound is loaded */
	sc = Snd_LoadSound (sfx);
	if (!sc)
		return;		// couldn't load the sound's data

	if (snd_ALActive) {
		int		i;
		ALint	sourceNum;

		i = OpenAL_GetFreeALIndex ();
		if (i == -1) {
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Snd_StartSound [OpenAL]: WARNING: Out of indexes!\n");
			return;
		}

		sourceNum = OpenAL_GetFreeSource ();
		if (sourceNum == -1)
			Com_Printf (PRINT_DEV, S_COLOR_YELLOW "Snd_StartSound [OpenAL]: WARNING: Warning: Out of sources!\n");
		else {
			vec3_t	torigin;

			snd_ALIndexes[i].inUse = qTrue;

			if (origin) {
				snd_ALIndexes[i].fixedOrigin = qTrue;
				VectorCopy (origin, snd_ALIndexes[i].origin);
			} else
				snd_ALIndexes[i].fixedOrigin = qFalse;

			if (!attenuation) {
				Com_Printf (PRINT_DEV, "no attn, ent = %d, fVol = %f\n", entNum, fVol);

				alSourcefv (snd_ALSources[sourceNum], AL_POSITION, vec3Identity);
				OpenAL_CheckForError ();

				alSourcef (snd_ALSources[sourceNum], AL_GAIN, 1.0f);
				OpenAL_CheckForError ();

				alSourcef (snd_ALSources[sourceNum], AL_ROLLOFF_FACTOR, 0.0f);
				OpenAL_CheckForError ();

				alSourcei (snd_ALSources[sourceNum], AL_SOURCE_RELATIVE, AL_TRUE);
				OpenAL_CheckForError ();

				alSourcef (snd_ALSources[sourceNum], AL_REFERENCE_DISTANCE, 128.0f);
				OpenAL_CheckForError ();

				snd_ALIndexes[i].fixedOrigin = qTrue;
				VectorCopy (vec3Identity, snd_ALIndexes[i].origin);

				entNum = 0;
			} else {
				if (!origin) {	
					CL_GetEntitySoundOrigin (entNum, torigin);
					origin = torigin;
				}

				alSourcefv (snd_ALSources[sourceNum], AL_POSITION, origin);
				OpenAL_CheckForError ();

				alSourcef (snd_ALSources[sourceNum], AL_GAIN, clamp (fVol, 0, 1));
				OpenAL_CheckForError ();

				alSourcef (snd_ALSources[sourceNum], AL_ROLLOFF_FACTOR, clamp (0.33f * attenuation, 0, 1));
				OpenAL_CheckForError ();

				alSourcef (snd_ALSources[sourceNum], AL_REFERENCE_DISTANCE, 256.0f * clamp (fVol, 0, 1));
				OpenAL_CheckForError ();

				alSourcei (snd_ALSources[sourceNum], AL_SOURCE_RELATIVE, AL_FALSE);
				OpenAL_CheckForError ();
			}

			alSourcei (snd_ALSources[sourceNum], AL_BUFFER, snd_ALBuffers[sfx->cache->bufferNum].buffer);
			OpenAL_CheckForError ();

			snd_ALIndexes[i].sourceIndex = sourceNum;
			snd_ALIndexes[i].entNum = entNum;

			alSourcePlay (snd_ALSources[sourceNum]);
			OpenAL_CheckForError ();
		}
	} else {
		int			vol;
		playSound_t	*ps, *sort;
		int			start;
		static int	beginOfs = 0;

		vol = fVol * 255;

		/* make the playSound_t */
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

		/* drift beginOfs */
		start = cl.frame.serverTime * 0.001 * dma.speed + beginOfs;
		if (start < snd_PaintedTime) {
			start = snd_PaintedTime;
			beginOfs = start - (cl.frame.serverTime * 0.001 * dma.speed);
		} else if (start > snd_PaintedTime + 0.3 * dma.speed) {
			start = snd_PaintedTime + 0.1 * dma.speed;
			beginOfs = start - (cl.frame.serverTime * 0.001 * dma.speed);
		} else {
			beginOfs -= 10;
		}

		if (!timeOfs)
			ps->begin = snd_PaintedTime;
		else
			ps->begin = start + timeOfs * dma.speed;

		/* sort into the pending sound list */
		for (sort=snd_PendingPlays.next ; sort != &snd_PendingPlays && sort->begin < ps->begin ; sort = sort->next) ;

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
void Snd_StartLocalSound (char *sound) {
	sfx_t	*sfx;

	if (!snd_Initialized)
		return;
		
	sfx = Snd_RegisterSound (sound);
	if (!sfx) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Snd_StartLocalSound: can't cache %s\n", sound);
		return;
	}

	Snd_StartSound (NULL, cl.playerNum+1, 0, sfx, 1, 0, 0);
}


/*
==================
Snd_ClearBuffer
==================
*/
void Snd_ClearBuffer (void) {	
	if (!snd_Initialized)
		return;

	if (snd_ALActive)
		OpenAL_DestroyBuffers ();
	else {
		int		clear;

		snd_RawEnd = 0;

		if (dma.sampleBits == 8)
			clear = 0x80;
		else
			clear = 0;

		SNDDMA_BeginPainting ();
		if (dma.buffer)
			memset(dma.buffer, clear, dma.samples * dma.sampleBits/8);
		SNDDMA_Submit ();
	}
}


/*
==================
Snd_StopAllSounds
==================
*/
void Snd_StopAllSounds (void) {
	int		i;

	if (!snd_Initialized)
		return;

	/* clear all the playsounds */
	memset (snd_PlaySounds, 0, sizeof (snd_PlaySounds));
	snd_FreePlays.next = snd_FreePlays.prev = &snd_FreePlays;
	snd_PendingPlays.next = snd_PendingPlays.prev = &snd_PendingPlays;

	for (i=0 ; i<MAX_PLAYSOUNDS ; i++) {
		snd_PlaySounds[i].prev = &snd_FreePlays;
		snd_PlaySounds[i].next = snd_FreePlays.next;
		snd_PlaySounds[i].prev->next = &snd_PlaySounds[i];
		snd_PlaySounds[i].next->prev = &snd_PlaySounds[i];
	}

	/* clear all the channels */
	memset (snd_Channels, 0, sizeof (snd_Channels));

	Snd_ClearBuffer ();
}


/*
==================
Snd_AddLoopSounds

Entities with a ->sound field will generated looped sounds
that are automatically started, stopped, and merged together
as the entities are sent to the client
==================
*/
void Snd_AddLoopSounds (void) {
	int				i, j;
	int				sounds[MAX_EDICTS];
	int				left, right;
	int				leftTotal, rightTotal;
	channel_t		*ch;
	sfx_t			*sfx;
	sfxCache_t		*sc;
	int				num;
	entityState_t	*ent;

	if (cl_paused->value)
		return;

	if (!CL_ConnectionState (CA_ACTIVE))
		return;

	if (!cl.soundPrepped)
		return;

	for (i=0 ; i<cl.frame.numEntities ; i++) {
		num = (cl.frame.parseEntities + i)&(MAX_PARSE_ENTITIES-1);
		ent = &cl_ParseEntities[num];
		sounds[i] = ent->sound;
	}

	for (i=0 ; i<cl.frame.numEntities ; i++) {
		if (!sounds[i])
			continue;

		sfx = cl.soundPrecache[sounds[i]];
		if (!sfx)
			continue;	// bad sound effect
		sc = sfx->cache;
		if (!sc)
			continue;

		num = (cl.frame.parseEntities + i)&(MAX_PARSE_ENTITIES-1);
		ent = &cl_ParseEntities[num];

		/* find the total contribution of all sounds of this type */
		Snd_SpatializeOrigin (ent->origin, 255.0, SOUND_LOOPATTENUATE, &leftTotal, &rightTotal);
		for (j=i+1 ; j<cl.frame.numEntities ; j++) {
			if (sounds[j] != sounds[i])
				continue;
			sounds[j] = 0;	// don't check this again later

			num = (cl.frame.parseEntities + j)&(MAX_PARSE_ENTITIES-1);
			ent = &cl_ParseEntities[num];

			Snd_SpatializeOrigin (ent->origin, 255.0, SOUND_LOOPATTENUATE, &left, &right);
			leftTotal += left;
			rightTotal += right;
		}

		if ((leftTotal == 0) && (rightTotal == 0))
			continue;	// not audible

		/* allocate a channel */
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
		ch->pos = snd_PaintedTime % sc->length;
		ch->end = snd_PaintedTime + sc->length - ch->pos;
	}
}

//=============================================================================

/*
============
Snd_RawSamples

Cinematic streaming and voice over network
============
*/
void Snd_RawSamples (int samples, int rate, int width, int channels, byte *data) {
	if (!snd_Initialized)
		return;

	if (snd_ALActive) {
	} else {
		int		i;
		int		src, dst;
		float	scale;

		if (snd_RawEnd < snd_PaintedTime)
			snd_RawEnd = snd_PaintedTime;
		scale = (float)rate / dma.speed;

		if ((channels == 2) && (width == 2)) {
			if (scale == 1.0) {
				/* optimized case */
				for (i=0 ; i<samples ; i++) {
					dst = snd_RawEnd & (MAX_RAW_SAMPLES-1);
					snd_RawEnd++;
					snd_RawSamples[dst].left = LittleShort(((short *)data)[i*2]) << 8;
					snd_RawSamples[dst].right = LittleShort(((short *)data)[i*2+1]) << 8;
				}
			} else {
				for (i=0 ; ; i++) {
					src = i*scale;
					if (src >= samples)
						break;

					dst = snd_RawEnd & (MAX_RAW_SAMPLES-1);
					snd_RawEnd++;
					snd_RawSamples[dst].left = LittleShort(((short *)data)[src*2]) << 8;
					snd_RawSamples[dst].right = LittleShort(((short *)data)[src*2+1]) << 8;
				}
			}
		} else if ((channels == 1) && (width == 2)) {
			for (i=0 ; ; i++) {
				src = i*scale;
				if (src >= samples)
					break;
				dst = snd_RawEnd & (MAX_RAW_SAMPLES-1);
				snd_RawEnd++;
				snd_RawSamples[dst].left = LittleShort(((short *)data)[src]) << 8;
				snd_RawSamples[dst].right = LittleShort(((short *)data)[src]) << 8;
			}
		} else if ((channels == 2) && (width == 1)) {
			for (i=0 ; ; i++) {
				src = i*scale;
				if (src >= samples)
					break;
				dst = snd_RawEnd & (MAX_RAW_SAMPLES-1);
				snd_RawEnd++;
				snd_RawSamples[dst].left = ((char *)data)[src*2] << 16;
				snd_RawSamples[dst].right = ((char *)data)[src*2+1] << 16;
			}
		} else if ((channels == 1) && (width == 1)) {
			for (i=0 ; ; i++) {
				src = i*scale;
				if (src >= samples)
					break;
				dst = snd_RawEnd & (MAX_RAW_SAMPLES-1);
				snd_RawEnd++;
				snd_RawSamples[dst].left = (((byte *)data)[src]-128) << 16;
				snd_RawSamples[dst].right = (((byte *)data)[src]-128) << 16;
			}
		}
	}
}

//=============================================================================

/*
============
Snd_Update

Called once each time through the main loop
============
*/
void Snd_Update (void) {
	if (!snd_Initialized)
		return;

	/*
	** if the laoding plaque is up, clear everything out to make sure we
	** aren't looping a dirty dma buffer while loading
	*/
	if (cls.disableScreen) {
		Snd_ClearBuffer ();
		return;
	}

	if (snd_ALActive) {
		ALfloat	orientation[6];
		int		i;

		alListenerfv (AL_POSITION, cl.refDef.viewOrigin);

		orientation[0] = cl.v_Forward[1];
		orientation[1] = -cl.v_Forward[2];
		orientation[2] = -cl.v_Forward[0];
		orientation[3] = cl.v_Up[1];
		orientation[4] = -cl.v_Up[2];
		orientation[5] = -cl.v_Up[0];
		alListenerfv (AL_ORIENTATION, orientation);

		alListenerf (AL_GAIN, s_volume->value);

		OpenAL_CheckForError ();

		for (i=0 ; i<MAX_SOUNDS ; i++) {
			if (snd_ALIndexes[i].inUse) {
				if (snd_ALIndexes[i].fixedOrigin)
					alSourcefv (snd_ALSources[snd_ALIndexes[i].sourceIndex], AL_POSITION, snd_ALIndexes[i].origin);
				else {
					vec3_t entOrigin;
					CL_GetEntitySoundOrigin (snd_ALIndexes[i].entNum, entOrigin);
					alSourcefv (snd_ALSources[snd_ALIndexes[i].sourceIndex], AL_POSITION, entOrigin);
				}
			}
		}
	} else {
		int			i;
		int			total;
		unsigned	endtime;
		int			samps;
		channel_t	*ch;
		channel_t	*combine;

		int			samplepos;
		static int	buffers;
		static int	oldsamplepos;
		int			fullsamples;

		/* rebuild scale tables if volume is modified */
		if (s_volume->modified)
			Snd_InitScaleTable ();

		combine = NULL;

		/* update spatialization for dynamic sounds */
		ch = snd_Channels;
		for (i=0 ; i<MAX_CHANNELS; i++, ch++) {
			if (!ch->sfx)
				continue;

			if (ch->autoSound) {
				/* autosounds are regenerated fresh each frame */
				memset (ch, 0, sizeof (*ch));
				continue;
			}

			Snd_Spatialize (ch);		// respatialize channel
			if (!ch->leftVol && !ch->rightVol) {
				memset (ch, 0, sizeof (*ch));
				continue;
			}
		}

		/* add loopsounds */
		Snd_AddLoopSounds ();

		/* debugging output */
		if (s_show->value) {
			total = 0;
			ch = snd_Channels;
			for (i=0 ; i<MAX_CHANNELS; i++, ch++) {
				if (ch->sfx && (ch->leftVol || ch->rightVol)) {
					Com_Printf (PRINT_ALL, "%3i %3i %s\n", ch->leftVol, ch->rightVol, ch->sfx->name);
					total++;
				}
			}
			
			Com_Printf (PRINT_ALL, "----(%i)---- painted: %i\n", total, snd_PaintedTime);
		}

		/* mix some sound */
		SNDDMA_BeginPainting ();

		if (!dma.buffer)
			return;

		/* update DMA time */
		fullsamples = dma.samples / dma.channels;

		/*
		** it is possible to miscount buffers if it has wrapped twice between
		** calls to Snd_Update. Oh well
		*/
		samplepos = SNDDMA_GetDMAPos ();

		if (samplepos < oldsamplepos) {
			buffers++;	// buffer wrapped
			
			if (snd_PaintedTime > 0x40000000) {
				/* time to chop things off to avoid 32 bit limits */
				buffers = 0;
				snd_PaintedTime = fullsamples;
				Snd_StopAllSounds ();
			}
		}

		oldsamplepos = samplepos;
		snd_SoundTime = buffers*fullsamples + samplepos/dma.channels;

		/* check to make sure that we haven't overshot */
		if (snd_PaintedTime < snd_SoundTime) {
			Com_Printf (PRINT_DEV, S_COLOR_YELLOW "Snd_Update: overflow\n");
			snd_PaintedTime = snd_SoundTime;
		}

		/* mix ahead of current position */
		endtime = snd_SoundTime + s_mixahead->value * dma.speed;

		/* mix to an even submission block size */
		endtime = (endtime + dma.submissionChunk-1) & ~(dma.submissionChunk-1);
		samps = dma.samples >> (dma.channels-1);
		if (endtime - snd_SoundTime > samps)
			endtime = snd_SoundTime + samps;

		Snd_PaintChannels (endtime);

		SNDDMA_Submit ();
	}
}
