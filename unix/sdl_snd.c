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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

//
// unix_snd_sdl.c
//

#include "SDL.h"

#include "../client/snd_local.h"
#include "../unix/unix_local.h"

static	qBool	sndInitialized = qFalse;

static void Paint_Audio (void *unused, Uint8 * stream, int len)
{
	snd_audioDMA.buffer = stream;
	snd_audioDMA.samplePos += len / (snd_audioDMA.sampleBits / 4);

	/** Check for samplepos overflow? */
	DMASnd_PaintChannels (snd_audioDMA.samplePos, 1.0);
}


/**
==============
SDL_SND_Init
===============
*/
qBool SndImp_Init (void)
{
	SDL_AudioSpec desired, obtained;
	int desired_bits, freq;
	
	if (SDL_WasInit(SDL_INIT_EVERYTHING) == 0) {
		if (SDL_Init(SDL_INIT_AUDIO) < 0) {
			Com_Printf (0, "Couldn't init SDL audio: %s\n", SDL_GetError ());
			return qFalse;
		}
	} 
	else if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
		if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
			Com_Printf (0, "Couldn't init SDL audio: %s\n", SDL_GetError ());
			return qFalse;
		}
	}
	
	sndInitialized = qFalse;
	
	desired_bits = Cvar_Register ("sndbits", "16", CVAR_ARCHIVE)->intVal;
	freq = Cvar_Register("s_khz", "0", CVAR_ARCHIVE)->intVal;
	desired.channels = Cvar_Register("sndchannels", "2", CVAR_ARCHIVE)->intVal;
	
	if (freq == 48)      desired.freq = 48000;
	else if (freq == 44) desired.freq = 44100;
	else if (freq == 32) desired.freq = 32000;
	else if (freq == 22) desired.freq = 22050;
	else if (freq == 16) desired.freq = 16000;
	else desired.freq = 11025;
	
	switch (desired_bits) {
		case 8:
			desired.format = AUDIO_U8;
			break;
		case 16:
			if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
				desired.format = AUDIO_S16MSB;
			else
				desired.format = AUDIO_S16LSB;
			break;
		default:
			Com_Printf (0, "Unknown number of audio bits: %d\n", desired_bits);
			return qFalse;
	}
	
	if (desired.freq == 48000)      desired.samples = 4096;
	else if (desired.freq == 44100) desired.samples = 4096;
	else if (desired.freq == 32000) desired.samples = 2048;
	else if (desired.freq == 22050) desired.samples = 1024;
	else if (desired.freq == 16000) desired.samples = 1024;
	else desired.samples = 512;
	
	desired.callback = Paint_Audio;
	
	/** Open the audio device */
	if (SDL_OpenAudio (&desired, &obtained) < 0) {
		Com_Printf (0, "Couldn't open SDL audio: %s\n", SDL_GetError ());
		return qFalse;
	}

	/* Make sure we can support the audio format */
	switch (obtained.format) {
		case AUDIO_U8:
			/** Supported */
			break;
		case AUDIO_S16LSB:
		case AUDIO_S16MSB:
			if (((obtained.format == AUDIO_S16LSB) &&
				 (SDL_BYTEORDER == SDL_LIL_ENDIAN)) ||
				((obtained.format == AUDIO_S16MSB) &&
				 (SDL_BYTEORDER == SDL_BIG_ENDIAN))) {
				/** Supported */
				break;
			}
			/** Unsupported, fall through */
		default:
			/** Not supported -- force SDL to do our bidding */
			SDL_CloseAudio ();
			if (SDL_OpenAudio (&desired, NULL) < 0) {
				Com_Printf (0, "Couldn't open SDL audio: %s\n", SDL_GetError ());
				return qFalse;
			}
			memcpy (&obtained, &desired, sizeof (desired));
			break;
	}
	SDL_PauseAudio (0);

	/** Fill the audio DMA information block */
	snd_audioDMA.sampleBits = (obtained.format & 0xFF);
	snd_audioDMA.speed = obtained.freq;
	snd_audioDMA.channels = obtained.channels;
	snd_audioDMA.samples = obtained.samples * snd_audioDMA.channels;
	snd_audioDMA.samplePos = 0;
	snd_audioDMA.submissionChunk = 1;
	snd_audioDMA.buffer = NULL;
	
	Com_Printf(0, "Initializing SDL Sound System\n");
	
	Com_Printf(0,"Stereo: %d\n", snd_audioDMA.channels - 1);
	Com_Printf(0,"Samples: %d\n", snd_audioDMA.samples);
	Com_Printf(0,"Samplepos: %d\n", snd_audioDMA.samplePos);
	Com_Printf(0,"Samplebits: %d\n", snd_audioDMA.sampleBits);
	Com_Printf(0,"Submission_chunk: %d\n", snd_audioDMA.submissionChunk);
	Com_Printf(0,"Speed: %d\n", snd_audioDMA.speed);

	sndInitialized = qTrue;
	return qTrue;
}


/**
==============
SDL_Shutdown
===============
*/
void SndImp_Shutdown (void)
{
	if (sndInitialized) {
		SDL_CloseAudio ();
		sndInitialized = qFalse;
	}

	if (SDL_WasInit(SDL_INIT_EVERYTHING) == SDL_INIT_AUDIO)
		SDL_Quit();
	else
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

/**
==============
SDL_GetDMAPos
===============
*/
int SndImp_GetDMAPos (void)
{
	return snd_audioDMA.samplePos;
}

/**
==============
SDL_BeginPainting
===============
*/
void SndImp_BeginPainting (void)
{    
}

/**
==============
SDL_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void SndImp_Submit (void)
{
}

