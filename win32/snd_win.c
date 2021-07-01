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

#include <float.h>
#include "../sound/snd_loc.h"
#include "winquake.h"

#define iDirectSoundCreate(a,b,c)	pDirectSoundCreate(a,b,c)

HRESULT (WINAPI *pDirectSoundCreate)(GUID FAR *lpGUID, LPDIRECTSOUND FAR *lplpDS, IUnknown FAR *pUnkOuter);

// 64K is > 1 second at 16-bit, 22050 Hz
#define	WAV_BUFFERS				64
#define	WAV_MASK				0x3F
#define	WAV_BUFFER_SIZE			0x0400
#define SECONDARY_BUFFER_SIZE	0x10000

typedef enum {
	SIS_SUCCESS,
	SIS_FAILURE,
	SIS_NOTAVAIL
} sndinitstat;

cvar_t	*s_wavonly;

static qBool	DSound_Init;
static qBool	Wav_Init;
static qBool	snd_FirstTime = qTrue;
static qBool	snd_IsDSound;
static qBool	snd_IsWave;
static qBool	primary_format_set;

// starts at 0 for disabled
static int	sample16;
static int	snd_sent, snd_completed;

/*
** Global variables. Must be visible to window-procedure function
** so it can unlock and free the data block after it has been played.
*/

HANDLE		hData;
HPSTR		lpData, lpData2;

HGLOBAL		hWaveHdr;
LPWAVEHDR	lpWaveHdr;

HWAVEOUT	hWaveOut; 

WAVEOUTCAPS	wavecaps;

DWORD		gSndBufSize;

MMTIME		mmstarttime;

LPDIRECTSOUND		pDS;
LPDIRECTSOUNDBUFFER	pDSBuf, pDSPBuf;

HINSTANCE			hInstDS;

void FreeSound (void);

/*
==============
GetDSoundErrorString
===============
*/
static const char *GetDSoundErrorString (int error) {
	switch (error) {
	case DSERR_BUFFERLOST:			return "DSERR_BUFFERLOST";
	case DSERR_INVALIDCALL:			return "DSERR_INVALIDCALLS";
	case DSERR_INVALIDPARAM:		return "DSERR_INVALIDPARAM";
	case DSERR_PRIOLEVELNEEDED:		return "DSERR_PRIOLEVELNEEDED";
	}

	return "unknown";
}


/*
==============
DS_CreateBuffers
===============
*/
static qBool DS_CreateBuffers (void) {
	DSBUFFERDESC	dsbuf;
	DSBCAPS			dsbcaps;
	WAVEFORMATEX	pformat, format;
	DWORD			dwWrite;

	memset (&format, 0, sizeof (format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = dma.channels;
	format.wBitsPerSample = dma.sampleBits;
	format.nSamplesPerSec = dma.speed;
	format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
	format.cbSize = 0;
	format.nAvgBytesPerSec = format.nSamplesPerSec*format.nBlockAlign; 

	Com_Printf ((snd_FirstTime) ? PRINT_ALL : PRINT_DEV, "Creating DS buffers\n");

	Com_Printf (PRINT_DEV, "...setting EXCLUSIVE coop level: ");
	if (DS_OK != pDS->lpVtbl->SetCooperativeLevel (pDS, g_hWnd, DSSCL_EXCLUSIVE)) {
		Com_Printf (PRINT_ALL, S_COLOR_RED "FAILED!\n");
		FreeSound ();
		return qFalse;
	}
	Com_Printf (PRINT_DEV, "ok\n");

	// get access to the primary buffer, if possible, so we can set the sound hardware format
	memset (&dsbuf, 0, sizeof (dsbuf));
	dsbuf.dwSize = sizeof (DSBUFFERDESC);
	dsbuf.dwFlags = DSBCAPS_PRIMARYBUFFER;
	dsbuf.dwBufferBytes = 0;
	dsbuf.lpwfxFormat = NULL;

	memset(&dsbcaps, 0, sizeof (dsbcaps));
	dsbcaps.dwSize = sizeof (dsbcaps);
	primary_format_set = qFalse;

	Com_Printf (PRINT_DEV, "...creating primary buffer: ");
	if (DS_OK == pDS->lpVtbl->CreateSoundBuffer (pDS, &dsbuf, &pDSPBuf, NULL)) {
		pformat = format;
		Com_Printf (PRINT_DEV, "ok\n");

		Com_Printf (PRINT_DEV, "...setting primary sound format: ");
		if (DS_OK != pDSPBuf->lpVtbl->SetFormat (pDSPBuf, &pformat))
			Com_Printf (PRINT_ALL, S_COLOR_RED "FAILED!\n");
		else {
			Com_Printf (PRINT_DEV, "ok\n");

			primary_format_set = qTrue;
		}
	} else
		Com_Printf (PRINT_ALL, S_COLOR_RED "FAILED!\n");

	if (!primary_format_set || !s_primary->value) {
		// create the secondary buffer we'll actually work with
		memset (&dsbuf, 0, sizeof (dsbuf));
		dsbuf.dwSize = sizeof (DSBUFFERDESC);
		dsbuf.dwFlags = DSBCAPS_CTRLFREQUENCY | DSBCAPS_LOCSOFTWARE;
		dsbuf.dwBufferBytes = SECONDARY_BUFFER_SIZE;
		dsbuf.lpwfxFormat = &format;

		memset(&dsbcaps, 0, sizeof (dsbcaps));
		dsbcaps.dwSize = sizeof (dsbcaps);

		Com_Printf (PRINT_DEV, "...creating secondary buffer: ");
		if (DS_OK != pDS->lpVtbl->CreateSoundBuffer (pDS, &dsbuf, &pDSBuf, NULL)) {
			Com_Printf (PRINT_ALL, S_COLOR_RED "FAILED!\n");
			FreeSound ();
			return qFalse;
		}
		Com_Printf (PRINT_DEV, "ok\n");

		dma.channels = format.nChannels;
		dma.sampleBits = format.wBitsPerSample;
		dma.speed = format.nSamplesPerSec;

		if (DS_OK != pDSBuf->lpVtbl->GetCaps (pDSBuf, &dsbcaps)) {
			Com_Printf (PRINT_ALL, S_COLOR_RED "GetCaps FAILED!\n");
			FreeSound ();
			return qFalse;
		}

		Com_Printf ((snd_FirstTime) ? PRINT_ALL : PRINT_DEV, "...using secondary sound buffer\n");
	} else {
		Com_Printf ((snd_FirstTime) ? PRINT_ALL : PRINT_DEV, "...using primary buffer\n");

		if (DS_OK != pDS->lpVtbl->SetCooperativeLevel (pDS, g_hWnd, DSSCL_WRITEPRIMARY)) {
			Com_Printf (PRINT_ALL, "...setting WRITEPRIMARY coop level: " S_COLOR_RED "FAILED!\n");
			FreeSound ();
			return qFalse;
		}
		Com_Printf (PRINT_DEV, "...setting WRITEPRIMARY coop level: ok\n");

		if (DS_OK != pDSPBuf->lpVtbl->GetCaps (pDSPBuf, &dsbcaps)) {
			Com_Printf (PRINT_ALL, S_COLOR_RED "GetCaps FAILED!\n");
			return qFalse;
		}

		pDSBuf = pDSPBuf;
	}

	// Make sure mixer is active
	pDSBuf->lpVtbl->Play (pDSBuf, 0, 0, DSBPLAY_LOOPING);

	Com_Printf ((snd_FirstTime) ? PRINT_ALL : PRINT_DEV,
								"   %d channel(s)\n"
								"   %d bits/sample\n"
								"   %d bytes/sec\n",
								dma.channels, dma.sampleBits, dma.speed);
	
	gSndBufSize = dsbcaps.dwBufferBytes;

	// we don't want anyone to access the buffer directly w/o locking it first
	lpData = NULL; 

	pDSBuf->lpVtbl->Stop (pDSBuf);
	pDSBuf->lpVtbl->GetCurrentPosition (pDSBuf, &mmstarttime.u.sample, &dwWrite);
	pDSBuf->lpVtbl->Play (pDSBuf, 0, 0, DSBPLAY_LOOPING);

	dma.samples = gSndBufSize / (dma.sampleBits / 8);
	dma.samplePos = 0;
	dma.submissionChunk = 1;
	dma.buffer = (byte *) lpData;
	sample16 = (dma.sampleBits/8) - 1;

	return qTrue;
}


/*
==============
DS_DestroyBuffers
===============
*/
static void DS_DestroyBuffers (void) {
	Com_Printf (PRINT_DEV, "Destroying DS buffers\n");
	if (pDS) {
		Com_Printf (PRINT_DEV, "...setting NORMAL coop level\n");
		pDS->lpVtbl->SetCooperativeLevel (pDS, g_hWnd, DSSCL_NORMAL);
	}

	if (pDSBuf) {
		Com_Printf (PRINT_DEV, "...stopping and releasing sound buffer\n");
		pDSBuf->lpVtbl->Stop (pDSBuf);
		pDSBuf->lpVtbl->Release (pDSBuf);
	}

	// only release primary buffer if it's not also the mixing buffer we just released
	if (pDSPBuf && (pDSBuf != pDSPBuf)) {
		Com_Printf (PRINT_DEV, "...releasing primary buffer\n");
		pDSPBuf->lpVtbl->Release(pDSPBuf);
	}
	pDSBuf = NULL;
	pDSPBuf = NULL;

	dma.buffer = NULL;
}


/*
==================
FreeSound
==================
*/
void FreeSound (void) {
	int		i;

	Com_Printf (PRINT_DEV, "Shutting down sound system\n");

	if (pDS)
		DS_DestroyBuffers();

	if (hWaveOut) {
		Com_Printf (PRINT_DEV, "...resetting waveOut\n");
		waveOutReset (hWaveOut);

		if (lpWaveHdr) {
			Com_Printf (PRINT_DEV, "...unpreparing headers\n");
			for (i=0 ; i<WAV_BUFFERS ; i++)
				waveOutUnprepareHeader (hWaveOut, lpWaveHdr+i, sizeof (WAVEHDR));
		}

		Com_Printf (PRINT_DEV, "...closing waveOut\n");
		waveOutClose (hWaveOut);

		if (hWaveHdr) {
			Com_Printf (PRINT_DEV, "...freeing WAV header\n");
			GlobalUnlock(hWaveHdr);
			GlobalFree(hWaveHdr);
		}

		if (hData) {
			Com_Printf (PRINT_DEV, "...freeing WAV buffer\n");
			GlobalUnlock(hData);
			GlobalFree(hData);
		}

	}

	if (pDS) {
		Com_Printf (PRINT_DEV, "...releasing DS object\n");
		pDS->lpVtbl->Release(pDS);
	}

	if (hInstDS) {
		Com_Printf (PRINT_DEV, "...freeing DSOUND.DLL\n");
		FreeLibrary (hInstDS);
		hInstDS = NULL;
	}

	pDS = NULL;
	pDSBuf = NULL;
	pDSPBuf = NULL;
	hWaveOut = 0;
	hData = 0;
	hWaveHdr = 0;
	lpData = NULL;
	lpWaveHdr = NULL;
	DSound_Init = qFalse;
	Wav_Init = qFalse;
}


/*
==================
SNDDMA_InitDirect

Direct-Sound support
==================
*/
sndinitstat SNDDMA_InitDirect (void) {
	DSCAPS			dscaps;
	HRESULT			hresult;

	dma.channels = 2;
	dma.sampleBits = 16;

	if (!s_khz) s_khz = Cvar_Get("s_khz", "11", CVAR_ARCHIVE);
	switch (s_khz->integer) {
	case 48:	dma.speed = 48000;	break;
	case 44:	dma.speed = 44100;	break;
	case 32:	dma.speed = 32000;	break;
	case 22:	dma.speed = 22050;	break;
	case 16:	dma.speed = 16000;	break;
	case 11:	dma.speed = 11025;	break;
	case 8:		dma.speed = 8000;	break;
	default:	dma.speed = 11025;
	}

	Com_Printf (PRINT_ALL, "Initializing DirectSound\n");

	if (!hInstDS) {
		Com_Printf (PRINT_DEV, "...LoadLibrary (\"dsound.dll\"): ");
		hInstDS = LoadLibrary ("dsound.dll");

		if (hInstDS == NULL) {
			Com_Printf (PRINT_ALL, S_COLOR_RED "FAILED!\n");
			return SIS_FAILURE;
		}
		Com_Printf (PRINT_DEV, "ok\n");
		pDirectSoundCreate = (void *)GetProcAddress (hInstDS, "DirectSoundCreate");

		if (!pDirectSoundCreate) {
			Com_Printf (PRINT_ALL, S_COLOR_RED "Couldn't get DSound proc address!\n");
			return SIS_FAILURE;
		}
	}

	Com_Printf (PRINT_DEV, "...creating DS object: ");
	while ((hresult = iDirectSoundCreate (NULL, &pDS, NULL)) != DS_OK) {
		if (hresult != DSERR_ALLOCATED) {
			Com_Printf (PRINT_ALL, S_COLOR_RED "FAILED!\n");
			return SIS_FAILURE;
		}

		if (MessageBox (NULL,
						"The sound hardware is in use by another app.\n\n"
						"Select Retry to try to start sound again or Cancel to run Quake 2 with no sound.",
						"Sound not available",
						MB_RETRYCANCEL | MB_SETFOREGROUND | MB_ICONEXCLAMATION) != IDRETRY) {
			Com_Printf (PRINT_ALL, S_COLOR_RED "FAILED! Hardware already in use!\n");
			return SIS_NOTAVAIL;
		}
	}
	Com_Printf (PRINT_DEV, "ok\n");

	dscaps.dwSize = sizeof (dscaps);

	if (DS_OK != pDS->lpVtbl->GetCaps (pDS, &dscaps)) {
		Com_Printf (PRINT_ALL, S_COLOR_RED "GetCaps FAILED!\n");
	}

	if (dscaps.dwFlags & DSCAPS_EMULDRIVER) {
		Com_Printf (PRINT_ALL, S_COLOR_RED "No DSound driver found!\n");
		FreeSound ();
		return SIS_FAILURE;
	}

	if (!DS_CreateBuffers ())
		return SIS_FAILURE;

	DSound_Init = qTrue;

	Com_Printf (PRINT_DEV, "...completed successfully\n");

	return SIS_SUCCESS;
}


/*
==================
SNDDM_InitWav

Crappy windows multimedia base
==================
*/
qBool SNDDMA_InitWav (void) {
	WAVEFORMATEX	format;
	int				i;
	HRESULT			hr;

	Com_Printf (PRINT_ALL, "Initializing wave sound\n");
	
	snd_sent = 0;
	snd_completed = 0;

	dma.channels = 2;
	dma.sampleBits = 16;

	if (!s_khz) s_khz = Cvar_Get ("s_khz", "11", CVAR_ARCHIVE);
	switch (s_khz->integer) {
	case 48:	dma.speed = 48000;	break;
	case 44:	dma.speed = 44100;	break;
	case 32:	dma.speed = 32000;	break;
	case 22:	dma.speed = 22050;	break;
	case 16:	dma.speed = 16000;	break;
	case 11:	dma.speed = 11025;	break;
	case 8:		dma.speed = 8000;	break;
	default:	dma.speed = 11025;
	}

	memset (&format, 0, sizeof (format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = dma.channels;
	format.wBitsPerSample = dma.sampleBits;
	format.nSamplesPerSec = dma.speed;
	format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
	format.cbSize = 0;
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign; 

	// Open a waveform device for output using window callback.
	Com_Printf (PRINT_DEV, "...opening waveform device: ");
	while ((hr = waveOutOpen((LPHWAVEOUT)&hWaveOut, WAVE_MAPPER, 
					&format, 
					0, 0L, CALLBACK_NULL)) != MMSYSERR_NOERROR) {
		if (hr != MMSYSERR_ALLOCATED) {
			Com_Printf (PRINT_ALL, S_COLOR_RED "FAILED!\n");
			return qFalse;
		}

		if (MessageBox (NULL,
						"The sound hardware is in use by another app.\n\n"
						"Select Retry to try to start sound again or Cancel to run Quake 2 with no sound.",
						"Sound not available",
						MB_RETRYCANCEL | MB_SETFOREGROUND | MB_ICONEXCLAMATION) != IDRETRY) {
			Com_Printf (PRINT_ALL, S_COLOR_RED "FAILED! Hardware in use!\n");
			return qFalse;
		}
	}
	Com_Printf (PRINT_DEV, "ok\n");

	/*
	** Allocate and lock memory for the waveform data. The memory 
	** for waveform data must be globally allocated with 
	** GMEM_MOVEABLE and GMEM_SHARE flags. 
	*/
	Com_Printf (PRINT_DEV, "...allocating waveform buffer: ");
	gSndBufSize = WAV_BUFFERS*WAV_BUFFER_SIZE;
	hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, gSndBufSize); 
	if (!hData) { 
		Com_Printf (PRINT_ALL, S_COLOR_RED "FAILED!\n");
		FreeSound ();
		return qFalse;
	}
	Com_Printf (PRINT_DEV, "ok\n");

	Com_Printf (PRINT_DEV, "...locking waveform buffer: ");
	lpData = GlobalLock(hData);
	if (!lpData) { 
		Com_Printf (PRINT_ALL, S_COLOR_RED "FAILED!\n");
		FreeSound ();
		return qFalse;
	} 
	memset (lpData, 0, gSndBufSize);
	Com_Printf (PRINT_DEV, "ok\n");

	/*
	** Allocate and lock memory for the header. This memory must 
	** also be globally allocated with GMEM_MOVEABLE and 
	** GMEM_SHARE flags. 
	*/
	Com_Printf (PRINT_DEV, "...allocating waveform header: ");
	hWaveHdr = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, 
		(DWORD) sizeof (WAVEHDR) * WAV_BUFFERS); 

	if (hWaveHdr == NULL) {
		Com_Printf (PRINT_ALL, S_COLOR_RED "FAILED\n");
		FreeSound ();
		return qFalse; 
	}
	Com_Printf (PRINT_DEV, "ok\n");

	Com_Printf (PRINT_DEV, "...locking waveform header: ");
	lpWaveHdr = (LPWAVEHDR) GlobalLock(hWaveHdr); 

	if (lpWaveHdr == NULL) {
		Com_Printf (PRINT_ALL, S_COLOR_RED "FAILED!\n");
		FreeSound ();
		return qFalse; 
	}
	memset (lpWaveHdr, 0, sizeof (WAVEHDR) * WAV_BUFFERS);
	Com_Printf (PRINT_DEV, "ok\n");

	// After allocation, set up and prepare headers.
	Com_Printf (PRINT_DEV, "...preparing headers: ");
	for (i=0 ; i<WAV_BUFFERS ; i++) {
		lpWaveHdr[i].dwBufferLength = WAV_BUFFER_SIZE; 
		lpWaveHdr[i].lpData = lpData + i*WAV_BUFFER_SIZE;

		if (waveOutPrepareHeader (hWaveOut, lpWaveHdr+i, sizeof (WAVEHDR)) != MMSYSERR_NOERROR) {
			Com_Printf (PRINT_ALL, S_COLOR_RED "FAILED!\n");
			FreeSound ();
			return qFalse;
		}
	}
	Com_Printf (PRINT_DEV, "ok\n");

	dma.samples = gSndBufSize / (dma.sampleBits / 8);
	dma.samplePos = 0;
	dma.submissionChunk = 512;
	dma.buffer = (byte *) lpData;
	sample16 = (dma.sampleBits/8) - 1;

	Wav_Init = qTrue;

	return qTrue;
}


/*
==================
SNDDMA_Init

Try to find a sound device to mix for.
Returns qFalse if nothing is found.
==================
*/
int SNDDMA_Init (void) {
	sndinitstat	stat;

	memset ((void *)&dma, 0, sizeof (dma));

	s_wavonly = Cvar_Get ("s_wavonly", "0", 0);

	DSound_Init = qFalse;
	Wav_Init = qFalse;

	stat = SIS_FAILURE;	// assume DirectSound won't initialize

	// Init DirectSound
	if (!s_wavonly->integer) {
		if (snd_FirstTime || snd_IsDSound) {
			stat = SNDDMA_InitDirect ();

			if (stat == SIS_SUCCESS) {
				snd_IsDSound = qTrue;

				if (snd_FirstTime)
					Com_Printf (PRINT_DEV, "DSound init succeeded\n");
			} else {
				snd_IsDSound = qFalse;
				Com_Printf (PRINT_ALL, S_COLOR_RED "DSound init FAILED!\n");
			}
		}
	}

	/*
	** if DirectSound didn't succeed in initializing, try to initialize
	** waveOut sound, unless DirectSound failed because the hardware is
	** already allocated (in which case the user has already chosen not
	** to have sound)
	*/
	if (!DSound_Init && (stat != SIS_NOTAVAIL)) {
		if (snd_FirstTime || snd_IsWave) {
			snd_IsWave = SNDDMA_InitWav ();

			if (snd_IsWave) {
				if (snd_FirstTime)
					Com_Printf (PRINT_DEV, "Wave sound init succeeded\n");
			} else {
				Com_Printf (PRINT_ALL, S_COLOR_RED "Wave sound init FAILED!\n");
			}
		}
	}

	snd_FirstTime = qFalse;

	if (!DSound_Init && !Wav_Init) {
		if (snd_FirstTime)
			Com_Printf (PRINT_ALL, S_COLOR_RED "No sound device initialized\n");

		return 0;
	}

	return 1;
}


/*
==============
SNDDMA_GetDMAPos

Return the current sample position (in mono samples read) inside the recirculating
dma buffer, so the mixing code will know how many sample are required to fill it up.
===============
*/
int SNDDMA_GetDMAPos (void) {
	MMTIME	mmtime;
	int		s;
	DWORD	dwWrite;

	if (DSound_Init) {
		mmtime.wType = TIME_SAMPLES;
		pDSBuf->lpVtbl->GetCurrentPosition (pDSBuf, &mmtime.u.sample, &dwWrite);
		s = mmtime.u.sample - mmstarttime.u.sample;
	} else if (Wav_Init) {
		s = snd_sent * WAV_BUFFER_SIZE;
	}

	s >>= sample16;
	s &= (dma.samples-1);

	return s;
}


/*
==============
SNDDMA_BeginPainting

Makes sure dma.buffer is valid
===============
*/
DWORD	locksize;
void SNDDMA_BeginPainting (void) {
	int		reps;
	DWORD	dwSize2;
	DWORD	*pbuf, *pbuf2;
	HRESULT	hresult;
	DWORD	dwStatus;

	if (!pDSBuf)
		return;

	// if the buffer was lost or stopped, restore it and/or restart it
	if (pDSBuf->lpVtbl->GetStatus (pDSBuf, &dwStatus) != DS_OK)
		Com_Printf (PRINT_ALL, S_COLOR_RED "Couldn't get sound buffer status\n");
	
	if (dwStatus & DSBSTATUS_BUFFERLOST)
		pDSBuf->lpVtbl->Restore (pDSBuf);
	
	if (!(dwStatus & DSBSTATUS_PLAYING))
		pDSBuf->lpVtbl->Play(pDSBuf, 0, 0, DSBPLAY_LOOPING);

	// lock the dsound buffer
	reps = 0;
	dma.buffer = NULL;

	while ((hresult = pDSBuf->lpVtbl->Lock(pDSBuf, 0, gSndBufSize, &pbuf, &locksize, &pbuf2, &dwSize2, 0)) != DS_OK) {
		if (hresult != DSERR_BUFFERLOST) {
			Com_Printf (PRINT_ALL, S_COLOR_RED "SNDDMA_BeginPainting: Lock failed with error '%s'\n", GetDSoundErrorString (hresult));
			Snd_Shutdown ();
			return;
		} else {
			pDSBuf->lpVtbl->Restore (pDSBuf);
		}

		if (++reps > 2)
			return;
	}
	dma.buffer = (byte *)pbuf;
}


/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
Also unlocks the dsound buffer
===============
*/
void SNDDMA_Submit (void) {
	LPWAVEHDR	h;
	int			wResult;

	if (!dma.buffer)
		return;

	// unlock the dsound buffer
	if (pDSBuf)
		pDSBuf->lpVtbl->Unlock (pDSBuf, dma.buffer, locksize, NULL, 0);

	if (!Wav_Init)
		return;

	//
	// find which sound blocks have completed
	//
	for ( ; ; ) {
		if (snd_completed == snd_sent) {
			Com_Printf (PRINT_DEV, S_COLOR_YELLOW "Sound overrun\n");
			break;
		}

		if (!(lpWaveHdr[snd_completed & WAV_MASK].dwFlags & WHDR_DONE))
			break;

		snd_completed++;	// this buffer has been played
	}

	//
	// submit a few new sound blocks
	//
	while (((snd_sent - snd_completed) >> sample16) < 8) {
		h = lpWaveHdr + (snd_sent&WAV_MASK);
		if (snd_PaintedTime/256 <= snd_sent)
			break;

		snd_sent++;
		/*
		** Now the data block can be sent to the output device. The 
		** waveOutWrite function returns immediately and waveform 
		** data is sent to the output device in the background. 
		*/
		wResult = waveOutWrite(hWaveOut, h, sizeof (WAVEHDR)); 

		if (wResult != MMSYSERR_NOERROR) { 
			Com_Printf (PRINT_ALL, S_COLOR_RED "Failed to write block to device\n");
			FreeSound ();
			return; 
		} 
	}
}


/*
==============
SNDDMA_Shutdown

Reset the sound device for exiting
===============
*/
void SNDDMA_Shutdown (void) {
	FreeSound ();
}


/*
===========
SNDDMA_Activate

Called when the main window gains or loses focus. The window have been
destroyed and recreated between a deactivate and an activate.
===========
*/
void SNDDMA_Activate (qBool active) {
	if (snd_ALActive) {
		if (!active)
			OpenAL_DestroyBuffers ();
	} else if (pDS && g_hWnd && snd_IsDSound) {
		if (active)
			DS_CreateBuffers ();
		else
			DS_DestroyBuffers ();
	}
}
