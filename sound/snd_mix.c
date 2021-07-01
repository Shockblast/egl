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
// snd_mix.c
// Portable code to mix sounds for snd_dma.c
//

#include "snd_loc.h"

#define	PAINTBUFFER_SIZE	2048
sfxSamplePair_t sndPaintBuffer[PAINTBUFFER_SIZE];
int		sndScaleTable[32][256];
int		*sndMixPtr;
int		sndLinearCount;
int		sndVolume;
short	*sndOutput;

/*
================
Snd_WriteLinearBlastStereo16
================
*/
void Snd_WriteLinearBlastStereo16 (void);

#if !(defined __linux__ && defined __i386__)
#ifndef id386

void Snd_WriteLinearBlastStereo16 (void) {
	int		i;
	int		val;

	for (i=0 ; i<sndLinearCount ; i+=2) {
		val = sndMixPtr[i]>>8;
		if (val > 0x7fff)
			sndOutput[i] = 0x7fff;
		else if (val < (short)0x8000)
			sndOutput[i] = (short)0x8000;
		else
			sndOutput[i] = val;

		val = sndMixPtr[i+1]>>8;
		if (val > 0x7fff)
			sndOutput[i+1] = 0x7fff;
		else if (val < (short)0x8000)
			sndOutput[i+1] = (short)0x8000;
		else
			sndOutput[i+1] = val;
	}
}

#else

__declspec (naked) void Snd_WriteLinearBlastStereo16 (void)
{
	__asm {
		push edi
		push ebx
		mov ecx,ds:dword ptr[sndLinearCount]
		mov ebx,ds:dword ptr[sndMixPtr]
		mov edi,ds:dword ptr[sndOutput]
LWLBLoopTop:
		mov eax,ds:dword ptr[-8+ebx+ecx*4]
		sar eax,8
		cmp eax,07FFFh
		jg LClampHigh
		cmp eax,0FFFF8000h
		jnl LClampDone
		mov eax,0FFFF8000h
		jmp LClampDone
LClampHigh:
		mov eax,07FFFh
LClampDone:
		mov edx,ds:dword ptr[-4+ebx+ecx*4]
		sar edx,8
		cmp edx,07FFFh
		jg LClampHigh2
		cmp edx,0FFFF8000h
		jnl LClampDone2
		mov edx,0FFFF8000h
		jmp LClampDone2
LClampHigh2:
		mov edx,07FFFh
LClampDone2:
		shl edx,16
		and eax,0FFFFh
		or edx,eax
		mov ds:dword ptr[-4+edi+ecx*2],edx
		sub ecx,2
		jnz LWLBLoopTop
		pop ebx
		pop edi
		ret
	}
}
#endif
#endif


/*
================
Snd_TransferStereo16
================
*/
void Snd_TransferStereo16 (uLong *pbuf, int endTime) {
	int		lpos;
	int		lpaintedtime;
	
	sndMixPtr = (int *) sndPaintBuffer;
	lpaintedtime = sndPaintedTime;

	while (lpaintedtime < endTime) {
		// handle recirculating buffer issues
		lpos = lpaintedtime & ((sndDMA.samples>>1)-1);

		sndOutput = (short *) pbuf + (lpos<<1);

		sndLinearCount = (sndDMA.samples>>1) - lpos;
		if (lpaintedtime + sndLinearCount > endTime)
			sndLinearCount = endTime - lpaintedtime;

		sndLinearCount <<= 1;

		// write a linear blast of samples
		Snd_WriteLinearBlastStereo16 ();

		sndMixPtr += sndLinearCount;
		lpaintedtime += (sndLinearCount>>1);
	}
}


/*
===================
Snd_TransferPaintBuffer
===================
*/
void Snd_TransferPaintBuffer (int endTime) {
	int		out_idx;
	int		count;
	int		out_mask;
	int		*p;
	int		step;
	int		val;
	uLong	*pbuf;

	pbuf = (uLong *)sndDMA.buffer;

	if (s_testsound->value) {
		int		i;
		int		count;

		// write a fixed sine wave
		count = (endTime - sndPaintedTime);
		for (i=0 ; i<count ; i++)
			sndPaintBuffer[i].left = sndPaintBuffer[i].right = sin ((sndPaintedTime+i)*0.1)*20000*256;
	}

	if ((sndDMA.sampleBits == 16) && (sndDMA.channels == 2)) {
		// optimized case
		Snd_TransferStereo16 (pbuf, endTime);
	} else {
		// general case
		p = (int *) sndPaintBuffer;
		count = (endTime - sndPaintedTime) * sndDMA.channels;
		out_mask = sndDMA.samples - 1; 
		out_idx = sndPaintedTime * sndDMA.channels & out_mask;
		step = 3 - sndDMA.channels;

		if (sndDMA.sampleBits == 16) {
			short *out = (short *) pbuf;
			while (count--) {
				val = *p >> 8;
				p += step;
				if (val > 0x7fff)
					val = 0x7fff;
				else if (val < (short)0x8000)
					val = (short)0x8000;
				out[out_idx] = val;
				out_idx = (out_idx + 1) & out_mask;
			}
		} else if (sndDMA.sampleBits == 8) {
			byte	*out = (byte *) pbuf;
			while (count--) {
				val = *p >> 8;
				p += step;
				if (val > 0x7fff)
					val = 0x7fff;
				else if (val < (short)0x8000)
					val = (short)0x8000;
				out[out_idx] = (val>>8) + 128;
				out_idx = (out_idx + 1) & out_mask;
			}
		}
	}
}

/*
===============================================================================

	CHANNEL MIXING

===============================================================================
*/

/*
================
Snd_PaintChannelFrom8
================
*/
#if !(defined __linux__ && defined __i386__)
#ifndef id386

void Snd_PaintChannelFrom8 (channel_t *ch, sfxCache_t *sc, int count, int offset) {
	int		data;
	int		*lScale, *rScale;
	byte	*sfx;
	int		i;
	sfxSamplePair_t	*samp;

	if (ch->leftVol > 255)
		ch->leftVol = 255;
	if (ch->rightVol > 255)
		ch->rightVol = 255;

	/*
	** ZOID-- >>11 has been changed to >>3, >>11 didn't make much sense
	** as it would always be zero.
	*/
	lScale = sndScaleTable[ch->leftVol >> 3];
	rScale = sndScaleTable[ch->rightVol >> 3];
	sfx = (signed char *)sc->data + ch->pos;

	samp = &sndPaintBuffer[offset];

	for (i=0 ; i<count ; i++, samp++) {
		data = sfx[i];
		samp->left += lScale[data];
		samp->right += rScale[data];
	}
	
	ch->pos += count;
}

#else

__declspec (naked) void Snd_PaintChannelFrom8 (channel_t *ch, sfxCache_t *sc, int count, int offset) {
	__asm {
		push esi
		push edi
		push ebx
		push ebp
		mov ebx,ds:dword ptr[4+16+esp]
		mov esi,ds:dword ptr[8+16+esp]
		mov eax,ds:dword ptr[4+ebx]
		mov edx,ds:dword ptr[8+ebx]
		cmp eax,255
		jna LLeftSet
		mov eax,255
LLeftSet:
		cmp edx,255
		jna LRightSet
		mov edx,255
LRightSet:
		and eax,0F8h
		add esi,20
		and edx,0F8h
		mov edi,ds:dword ptr[16+ebx]
		mov ecx,ds:dword ptr[12+16+esp]
		add esi,edi
		shl eax,7
		add edi,ecx
		shl edx,7
		mov ds:dword ptr[16+ebx],edi
		add eax,offset sndScaleTable
		add edx,offset sndScaleTable
		sub ebx,ebx
		mov bl,ds:byte ptr[-1+esi+ecx*1]
		test ecx,1
		jz LMix8Loop
		mov edi,ds:dword ptr[eax+ebx*4]
		mov ebp,ds:dword ptr[edx+ebx*4]
		add edi,ds:dword ptr[sndPaintBuffer+0-8+ecx*8]
		add ebp,ds:dword ptr[sndPaintBuffer+4-8+ecx*8]
		mov ds:dword ptr[sndPaintBuffer+0-8+ecx*8],edi
		mov ds:dword ptr[sndPaintBuffer+4-8+ecx*8],ebp
		mov bl,ds:byte ptr[-2+esi+ecx*1]
		dec ecx
		jz LDone
LMix8Loop:
		mov edi,ds:dword ptr[eax+ebx*4]
		mov ebp,ds:dword ptr[edx+ebx*4]
		add edi,ds:dword ptr[sndPaintBuffer+0-8+ecx*8]
		add ebp,ds:dword ptr[sndPaintBuffer+4-8+ecx*8]
		mov bl,ds:byte ptr[-2+esi+ecx*1]
		mov ds:dword ptr[sndPaintBuffer+0-8+ecx*8],edi
		mov ds:dword ptr[sndPaintBuffer+4-8+ecx*8],ebp
		mov edi,ds:dword ptr[eax+ebx*4]
		mov ebp,ds:dword ptr[edx+ebx*4]
		mov bl,ds:byte ptr[-3+esi+ecx*1]
		add edi,ds:dword ptr[sndPaintBuffer+0-8*2+ecx*8]
		add ebp,ds:dword ptr[sndPaintBuffer+4-8*2+ecx*8]
		mov ds:dword ptr[sndPaintBuffer+0-8*2+ecx*8],edi
		mov ds:dword ptr[sndPaintBuffer+4-8*2+ecx*8],ebp
		sub ecx,2
		jnz LMix8Loop
LDone:
		pop ebp
		pop ebx
		pop edi
		pop esi
		ret
	}
}

#endif
#endif


/*
================
Snd_PaintChannelFrom16
================
*/
void Snd_PaintChannelFrom16 (channel_t *ch, sfxCache_t *sc, int count, int offset) {
	int		data, i;
	int		left, right;
	int		leftVol, rightVol;
	signed short	*sfx;
	sfxSamplePair_t	*samp;

	leftVol = ch->leftVol * sndVolume;
	rightVol = ch->rightVol * sndVolume;
	sfx = (signed short *)sc->data + ch->pos;

	samp = &sndPaintBuffer[offset];
	for (i=0 ; i<count ; i++, samp++) {
		data = sfx[i];
		left = (data * leftVol)>>8;
		right = (data * rightVol)>>8;
		samp->left += left;
		samp->right += right;
	}

	ch->pos += count;
}


/*
================
Snd_PaintChannels
================
*/
void Snd_PaintChannels (int endTime) {
	int			i;
	int			end;
	channel_t	*ch;
	sfxCache_t	*sc;
	int			lTime, count;
	playSound_t	*ps;

	sndVolume = s_volume->value*256;

	while (sndPaintedTime < endTime) {
		// if sndPaintBuffer is smaller than DMA buffer
		end = endTime;
		if (endTime - sndPaintedTime > PAINTBUFFER_SIZE)
			end = sndPaintedTime + PAINTBUFFER_SIZE;

		// start any playsounds
		for ( ; ; ) {
			ps = sndPendingPlays.next;
			if (ps == &sndPendingPlays)
				break;	// no more pending sounds
			if (ps->begin <= sndPaintedTime) {
				Snd_IssuePlaysound (ps);
				continue;
			}

			if (ps->begin < end)
				end = ps->begin;		// stop here
			break;
		}

		// clear the paint buffer
		if (sndRawEnd < sndPaintedTime)
			memset (sndPaintBuffer, 0, (end - sndPaintedTime) * sizeof (sfxSamplePair_t));
		else {
			// copy from the streaming sound source
			int		s;
			int		stop;

			stop = (end < sndRawEnd) ? end : sndRawEnd;

			for (i=sndPaintedTime ; i<stop ; i++) {
				s = i & (MAX_RAW_SAMPLES-1);
				sndPaintBuffer[i-sndPaintedTime] = sndRawSamples[s];
			}

			for ( ; i<end ; i++) {
				sndPaintBuffer[i-sndPaintedTime].left =
				sndPaintBuffer[i-sndPaintedTime].right = 0;
			}
		}

		// paint in the channels
		ch = sndChannels;
		for (i=0; i<MAX_CHANNELS ; i++, ch++) {
			lTime = sndPaintedTime;
		
			while (lTime < end) {
				if (!ch->sfx || (!ch->leftVol && !ch->rightVol))
					break;

				// max painting is to the end of the buffer
				count = end - lTime;

				// might be stopped by running out of data
				if (ch->end - lTime < count)
					count = ch->end - lTime;
		
				sc = Snd_LoadSound (ch->sfx);
				if (!sc)
					break;

				if ((count > 0) && ch->sfx) {	
					if (sc->width == 1) // FIXME; 8 bit asm is wrong now
						Snd_PaintChannelFrom8 (ch, sc, count, lTime - sndPaintedTime);
					else
						Snd_PaintChannelFrom16 (ch, sc, count, lTime - sndPaintedTime);
	
					lTime += count;
				}

				// if at end of loop, restart
				if (lTime >= ch->end) {
					if (ch->autoSound) {
						// autolooping sounds always go back to start
						ch->pos = 0;
						ch->end = lTime + sc->length;
					} else if (sc->loopStart >= 0) {
						ch->pos = sc->loopStart;
						ch->end = lTime + sc->length - ch->pos;
					} else {
						// channel just stopped
						ch->sfx = NULL;
					}
				}
			}
															  
		}

		// transfer out according to DMA format
		Snd_TransferPaintBuffer (end);
		sndPaintedTime = end;
	}
}


/*
================
Snd_ScaleTableInit
================
*/
void Snd_ScaleTableInit (void) {
	int		i, j;
	int		scale;

	s_volume->modified = qFalse;
	for (i=0 ; i<32 ; i++) {
		scale = i * 8 * 256 * s_volume->value;
		for (j=0 ; j<256 ; j++) {
			sndScaleTable[i][j] = ((signed char)j) * scale;
		}
	}
}
