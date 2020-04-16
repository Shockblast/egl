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
// unix_snd_alsa.c
//
#if defined (__linux__)
#include <alsa/asoundlib.h>

#include "../client/snd_local.h"
#include "../unix/unix_local.h"

/**
 * These are reasonable default values for good latency.  If ALSA
 * playback stutters or plain does not work, try adjusting these.
 * Period must always be a multiple of 2.  Buffer must always be
 * a multiple of period.  See http://alsa-project.org.
 */
snd_pcm_uframes_t period_size = 2048;
snd_pcm_uframes_t buffer_size = 8192;

snd_pcm_t *playback_handle;
snd_pcm_hw_params_t *hw_params;

static	qBool	sndInitialized = qFalse;
static	int sample_bytes;
static	int buffer_bytes;

cVar_t *sndbits;
cVar_t *sndspeed;
cVar_t *sndchannels;
cVar_t *snddevice;

unsigned int tryrates[] = 
{ 
	48000,
	44100, 
	22050, 
	11025, 
	8000 
};

/**
==============
ALSA_Init
===============
*/
qBool ALSA_Init (void)
{
	int	i,
		err,
		format;

	if(sndInitialized) 
		return qTrue; 
  
	sndbits = Cvar_Register ("sndbits", "16", CVAR_ARCHIVE);
	sndspeed = Cvar_Register ("sndspeed", "0", CVAR_ARCHIVE);
	sndchannels = Cvar_Register ("sndchannels", "2", CVAR_ARCHIVE);
	snddevice = Cvar_Register ("snddevice", "default", CVAR_ARCHIVE);

	if(!Q_stricmp(snddevice->string, "/dev/dsp")) snddevice->string = "default";

	err = snd_pcm_open(&playback_handle, snddevice->string, 
	                   SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	if (err < 0) {
		Com_Printf(0,"ALSA snd error, cannot open device %s (%s)\n", 
                           snddevice->string, snd_strerror(err));
		return qFalse;
	}
	Com_Printf (0, "Using Device: %s.\n", snddevice->string);
  
	err = snd_pcm_hw_params_malloc(&hw_params);
	if (err < 0) {
		Com_Printf(0,"ALSA snd error, cannot allocate hw params (%s)\n", snd_strerror(err));
		return qFalse;
	}
  
	err = snd_pcm_hw_params_any (playback_handle, hw_params);
	if (err < 0) {
		Com_Printf(0,"ALSA snd error, cannot init hw params (%s)\n", snd_strerror(err));
		snd_pcm_hw_params_free(hw_params);
		return qFalse;
	}
  
	err = snd_pcm_hw_params_set_access(playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0) {
		Com_Printf(0,"ALSA snd error, cannot set access (%s)\n", snd_strerror(err));
		snd_pcm_hw_params_free(hw_params);
		return qFalse;
	}
  
	snd_audioDMA.sampleBits = (int)sndbits->floatVal;
	if (snd_audioDMA.sampleBits == 16 || snd_audioDMA.sampleBits != 8) {
		err = snd_pcm_hw_params_set_format(playback_handle, hw_params, SND_PCM_FORMAT_S16_LE);
		if (err < 0) {
			Com_Printf(0,"ALSA snd error, 16 bit sound not supported, trying 8\n");
			snd_audioDMA.sampleBits = 8;
		} else {
			format = SND_PCM_FORMAT_S16_LE;
		}
	}
	if (snd_audioDMA.sampleBits == 8) {
		err = snd_pcm_hw_params_set_format(playback_handle, hw_params, SND_PCM_FORMAT_U8);
		if (err < 0) {
			Com_Printf(0,"ALSA snd error, cannot set sample format (%s)\n", 
			           snd_strerror(err));
			snd_pcm_hw_params_free(hw_params);
			return qFalse;
		} else {
			format = SND_PCM_FORMAT_U8;
		}
	}
  
	snd_audioDMA.speed = (int)sndspeed->floatVal;
	if (!snd_audioDMA.speed) {
		for (i=0 ; i<sizeof(tryrates); i++) {
			unsigned int test = tryrates[i];
			
			err = snd_pcm_hw_params_set_rate_near(playback_handle, hw_params, &test, 0);
			if (err < 0) {
				Com_Printf(0,"ALSA snd error, cannot set sample rate %d (%s)\n",
                                           tryrates[i], snd_strerror(err));
			} else {
	      			if (s_khz->intVal == 48)      snd_audioDMA.speed = 48000;
	      			else if (s_khz->intVal == 44) snd_audioDMA.speed = 44100;
	      			else if (s_khz->intVal == 32) snd_audioDMA.speed = 32000;
	      			else if (s_khz->intVal == 22) snd_audioDMA.speed = 22050;
	      			else if (s_khz->intVal == 16) snd_audioDMA.speed = 16000;
	      			else if (s_khz->intVal == 11) snd_audioDMA.speed = 11025;
	      			else snd_audioDMA.speed = test;
				break;
			}
		}
	}
	if (!snd_audioDMA.speed) {
		Com_Printf(0,"ALSA snd error couldn't set rate.\n");
		snd_pcm_hw_params_free(hw_params);
		return qFalse;
	}

	snd_audioDMA.channels = (int)sndchannels->floatVal;
	if (snd_audioDMA.channels < 1 || snd_audioDMA.channels > 2)
		snd_audioDMA.channels = 2;
	
	err = snd_pcm_hw_params_set_channels(playback_handle,hw_params,snd_audioDMA.channels);
	if (err < 0) {
		Com_Printf(0,"ALSA snd error couldn't set channels %d (%s).\n",
		           snd_audioDMA.channels, snd_strerror(err));
		snd_pcm_hw_params_free(hw_params);
		return qFalse;
	}
  
	err = snd_pcm_hw_params_set_period_size_near(playback_handle, hw_params, &period_size, 0);
	if(err < 0) {
		Com_Printf(0,"ALSA: cannot set period size near(%s)\n", snd_strerror(err));
		snd_pcm_hw_params_free(hw_params);
		return qFalse;
	}

	err = snd_pcm_hw_params_set_buffer_size_near(playback_handle, hw_params, &buffer_size);
	if(err < 0) {
		Com_Printf(0,"ALSA: cannot set buffer size near(%s)\n", snd_strerror(err));
		snd_pcm_hw_params_free(hw_params);
		return qFalse;
	}
  
	err = snd_pcm_hw_params(playback_handle, hw_params);
	if (err < 0) {
		Com_Printf(0,"ALSA snd error couldn't set params (%s).\n",snd_strerror(err));
		snd_pcm_hw_params_free(hw_params);
		return qFalse;
	}

	sample_bytes = snd_audioDMA.sampleBits / 8;
	buffer_bytes = buffer_size * snd_audioDMA.channels * sample_bytes;

	snd_audioDMA.buffer = malloc(buffer_bytes);  /** allocate pcm frame buffer */
	memset(snd_audioDMA.buffer, 0, buffer_bytes);
	snd_audioDMA.samples = buffer_size * snd_audioDMA.channels;
	snd_audioDMA.submissionChunk = period_size * snd_audioDMA.channels;
	snd_audioDMA.samplePos = 0;

	snd_pcm_prepare(playback_handle);
	
	sndInitialized = qTrue;
	
	Com_Printf(0, "Initializing ALSA Sound System\n");
	
	Com_Printf(0,"Period size: %d\n", period_size);
	Com_Printf(0,"Buffer size: %d\n", buffer_size);
	Com_Printf(0,"Stereo: %d\n", snd_audioDMA.channels - 1);
	Com_Printf(0,"Samples: %d\n", snd_audioDMA.samples);
	Com_Printf(0,"Samplepos: %d\n", snd_audioDMA.samplePos);
	Com_Printf(0,"Samplebits: %d\n", snd_audioDMA.sampleBits);
	Com_Printf(0,"Submission_chunk: %d\n", snd_audioDMA.submissionChunk);
	Com_Printf(0,"Speed: %d\n", snd_audioDMA.speed);
	
	return qTrue;
}

/**
==============
ALSA_Shutdown
===============
*/
void ALSA_Shutdown(void)
{
	if(sndInitialized) {
		snd_pcm_drop(playback_handle);
		snd_pcm_close(playback_handle);
		sndInitialized = qFalse;
  	}
	
	free(snd_audioDMA.buffer);
	snd_audioDMA.buffer = NULL;
}

/**
==============
ALSA_GetDMAPos
===============
*/
int ALSA_GetDMAPos (void)
{
	if(sndInitialized) 
		return snd_audioDMA.samplePos;
	else
		Com_Printf (0,"Sound not inizialized\n");

	return qFalse;
}

/**
==============
ALSA_BeginPainting
===============
*/
void ALSA_BeginPainting (void)
{    
}

/**
==============
ALSA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void ALSA_Submit (void)
{
	int	s, 
		w, 
		frames;
	void	*start;

	if(!sndInitialized)
		return;

	s = snd_audioDMA.samplePos * sample_bytes;
	start = (void *) &snd_audioDMA.buffer[s];

	frames = snd_audioDMA.submissionChunk / snd_audioDMA.channels;

	w = snd_pcm_writei(playback_handle, start, frames);
	if(w < 0) {						/** write to card */
		snd_pcm_prepare(playback_handle);		/** xrun occured */
		return;
	}

	snd_audioDMA.samplePos += w * snd_audioDMA.channels;	/** mark progress */

	if(snd_audioDMA.samplePos >= snd_audioDMA.samples) 
		snd_audioDMA.samplePos = 0;			/** wrap buffer */
}
#endif

