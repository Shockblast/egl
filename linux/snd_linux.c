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

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <linux/soundcard.h>
#include <stdio.h>

#include "../client/snd_local.h"

int				audio_fd = -1;
static	qBool	sndInitialized = qFalse;

cVar_t *sndbits;
cVar_t *sndspeed;
cVar_t *sndchannels;
cVar_t *snddevice;

static int tryrates[] = { 11025, 22051, 44100, 8000 };

/*
==============
SndImp_Init
===============
*/
qBool SndImp_Init (void) {
	int rc;
    int fmt;
	int tmp;
    int i;
    char *s;
	struct audio_buf_info info;
	int caps;
	extern uid_t saved_euid;

	if (sndInitialized)
		return;

	if (!snddevice) {
		sndbits = Cvar_Register ("sndbits", "16", CVAR_ARCHIVE);
		sndspeed = Cvar_Register ("sndspeed", "0", CVAR_ARCHIVE);
		sndchannels = Cvar_Register ("sndchannels", "2", CVAR_ARCHIVE);
		snddevice = Cvar_Register ("snddevice", "/dev/dsp", CVAR_ARCHIVE);
	}

	// open /dev/dsp, confirm capability to mmap, and get size of dma buffer
	if (audio_fd == -1) {
		seteuid (saved_euid);

		audio_fd = open (snddevice->string, O_RDWR);

		seteuid(getuid ());

		if (audio_fd < 0) {
			perror(snddevice->string);
			Com_Printf (0, "Could not open %s\n", snddevice->string);
			return 0;
		}
	}

    rc = ioctl(audio_fd, SNDCTL_DSP_RESET, 0);
    if (rc < 0) {
		perror(snddevice->string);
		Com_Printf (0, "Could not reset %s\n", snddevice->string);
		close(audio_fd);
		return 0;
	}

	if (ioctl (audio_fd, SNDCTL_DSP_GETCAPS, &caps) == -1) {
		perror(snddevice->string);
        Com_Printf (0, "Sound driver too old\n");
		close(audio_fd);
		return 0;
	}

	if (!(caps & DSP_CAP_TRIGGER) || !(caps & DSP_CAP_MMAP)) {
		Com_Printf (0, "Sorry but your soundcard can't do this\n");
		close(audio_fd);
		return 0;
	}

    if (ioctl (audio_fd, SNDCTL_DSP_GETOSPACE, &info) == -1) {   
        perror ("GETOSPACE");
		Com_Printf (0, "Um, can't do GETOSPACE?\n");
		close (audio_fd);
		return 0;
    }

	// set sample bits & speed
    snd_audioDMA.sampleBits = (int)sndbits->value;
	if (snd_audioDMA.sampleBits != 16 && snd_audioDMA.sampleBits != 8) {
        ioctl (audio_fd, SNDCTL_DSP_GETFMTS, &fmt);
        if (fmt & AFMT_S16_LE) snd_audioDMA.sampleBits = 16;
        else if (fmt & AFMT_U8) snd_audioDMA.sampleBits = 8;
    }

    snd_audioDMA.speed = (int)sndspeed->value;
    if (!snd_audioDMA.speed) {
        for (i=0 ; i<sizeof(tryrates)/4 ; i++)
            if (!ioctl (audio_fd, SNDCTL_DSP_SPEED, &tryrates[i])) break;
        snd_audioDMA.speed = tryrates[i];
    }

	snd_audioDMA.channels = (int)sndchannels->value;
	if ((snd_audioDMA.channels < 1) || (snd_audioDMA.channels > 2))
		snd_audioDMA.channels = 2;
	
	snd_audioDMA.samples = info.fragstotal * info.fragsize / (snd_audioDMA.sampleBits/8);
	snd_audioDMA.submissionChunk = 1;

	// memory map the dma buffer
	if (!snd_audioDMA.buffer)
		snd_audioDMA.buffer = (unsigned char *) mmap(NULL, info.fragstotal
			* info.fragsize, PROT_WRITE, MAP_FILE|MAP_SHARED, audio_fd, 0);

	if (!snd_audioDMA.buffer) {
		perror (snddevice->string);
		Com_Printf (0, "Could not mmap %s\n", snddevice->string);
		close (audio_fd);
		return 0;
	}

	tmp = 0;
	if (snd_audioDMA.channels == 2)
		tmp = 1;
    rc = ioctl (audio_fd, SNDCTL_DSP_STEREO, &tmp);
    if (rc < 0) {
		perror (snddevice->string);
        Com_Printf (0, "Could not set %s to stereo=%d", snddevice->string, snd_audioDMA.channels);
		close (audio_fd);
        return 0;
    }
	if (tmp)
		snd_audioDMA.channels = 2;
	else
		snd_audioDMA.channels = 1;

    if (snd_audioDMA.sampleBits == 16) {
        rc = AFMT_S16_LE;
        rc = ioctl (audio_fd, SNDCTL_DSP_SETFMT, &rc);
        if (rc < 0) {
			perror (snddevice->string);
			Com_Printf (0, "Could not support 16-bit data.  Try 8-bit.\n");
			close (audio_fd);
			return 0;
		}
    } else if (snd_audioDMA.sampleBits == 8) {
        rc = AFMT_U8;
        rc = ioctl (audio_fd, SNDCTL_DSP_SETFMT, &rc);
        if (rc < 0) {
			perror (snddevice->string);
			Com_Printf (0, "Could not support 8-bit data.\n");
			close (audio_fd);
			return 0;
		}
    } else {
		perror (snddevice->string);
		Com_Printf (0, "%d-bit sound not supported.", snd_audioDMA.sampleBits);
		close (audio_fd);
		return 0;
	}

    rc = ioctl (audio_fd, SNDCTL_DSP_SPEED, &snd_audioDMA.speed);
    if (rc < 0) {
		perror (snddevice->string);
        Com_Printf (0, "Could not set %s speed to %d", snddevice->string, snd_audioDMA.speed);
		close (audio_fd);
        return 0;
    }

	// toggle the trigger & start her up
    tmp = 0;
    rc  = ioctl (audio_fd, SNDCTL_DSP_SETTRIGGER, &tmp);
	if (rc < 0) {
		perror (snddevice->string);
		Com_Printf (0, "Could not toggle.\n");
		close (audio_fd);
		return 0;
	}

    tmp = PCM_ENABLE_OUTPUT;
    rc = ioctl (audio_fd, SNDCTL_DSP_SETTRIGGER, &tmp);
	if (rc < 0) {
		perror (snddevice->string);
		Com_Printf (0, "Could not toggle.\n");
		close (audio_fd);
		return 0;
	}

	snd_audioDMA.samplePos = 0;

	sndInitialized = qTrue;
	snd_isActive = qTrue;
	return 1;

}


/*
==============
SndImp_Shutdown
===============
*/
void SndImp_Shutdown(void)
{
//FIXME: some problem when really shutting down here
//  snd_isActive = qFalse;
//  if (audio_fd != -1) close(audio_fd), audio_fd=-1;
}


/*
==============
SndImp_GetDMAPos
===============
*/
int SndImp_GetDMAPos (void) {
	struct count_info count;

	if (!sndInitialized)
		return 0;

	if (ioctl (audio_fd, SNDCTL_DSP_GETOPTR, &count) == -1) {
		perror (snddevice->string);
		Com_Printf (0, "Uh, sound dead.\n");
		close (audio_fd);
		sndInitialized = qFalse;
		return 0;
	}

	snd_audioDMA.samplePos = count.ptr / (snd_audioDMA.sampleBits / 8);

	return snd_audioDMA.samplePos;

}


/*
==============
SndImp_BeginPainting
===============
*/
void SndImp_BeginPainting (void)
{
}


/*
==============
SndImp_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void SndImp_Submit (void)
{
}

