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
// snd_loc.h
// Private sound functions
//

#include "../client/cl_local.h"
#include "qal.h"

#define		MAX_SFX			(MAX_CS_SOUNDS*2)
#define		MAX_PLAYSOUNDS	128

// only begin attenuating sound volumes when outside the FULLVOLUME range
#define		SOUND_FULLVOLUME	80
#define		SOUND_LOOPATTENUATE	0.003

// !!! if this is changed, the asm code must change !!!
typedef struct sfxSamplePair_s {
	int				left;
	int				right;
} sfxSamplePair_t;

typedef struct sfxCache_s {
	int				length;
	int				loopStart;
	int				speed;			// not needed, because converted on load?
	int				width;
	int				stereo;

	ALint			bufferNum;		//r1: OpenAL Buffer associated with this sound

	byte			data[1];		// variable sized
} sfxCache_t;

typedef struct sfx_s {
	char			name[MAX_QPATH];
	uLong			sRegistrationFrame;		// 0 = free
	sfxCache_t		*cache;
	char			*trueName;
} sfx_t;

// a playSound_t will be generated by each call to Snd_StartSound, when the mixer
// reaches playsound->begin, the playsound will be assigned to a channel
typedef struct playsound_s {
	struct			playsound_s *prev, *next;
	sfx_t			*sfx;
	float			volume;
	float			attenuation;
	int				entNum;
	int				entChannel;
	qBool			fixedOrigin;	// use origin field instead of entnum's origin
	vec3_t			origin;
	uInt			begin;			// begin on this sample
} playSound_t;

typedef struct dma_s {
	int				channels;
	int				samples;				// mono samples in buffer
	int				submissionChunk;		// don't mix less than this #
	int				samplePos;				// in mono samples
	int				sampleBits;
	int				speed;
	byte			*buffer;
} dma_t;

// !!! if this is changed, the asm code must change !!!
typedef struct channel_s {
	sfx_t			*sfx;			// sfx number
	int				leftVol;		// 0-255 volume
	int				rightVol;		// 0-255 volume
	int				end;			// end time in global paintsamples
	int				pos;			// sample position in sfx
	int				looping;		// where to loop, -1 = no looping OBSOLETE?
	int				entNum;			// to allow overriding a specific sound
	int				entChannel;		//
	vec3_t			origin;			// only use if fixed_origin is set
	vec_t			distMult;		// distance multiplier (attenuation/clipK)
	int				masterVol;		// 0-255 master volume
	qBool			fixedOrigin;	// use origin instead of fetching entnum's origin
	qBool			autoSound;		// from an entity->sound, cleared each frame
} channel_t;

typedef struct wavInfo_s {
	int				rate;
	int				width;
	int				channels;
	int				loopStart;
	int				samples;
	int				dataOfs;		// chunk starts this many bytes from file start
} wavInfo_t;

// ==========================================================================

extern dma_t	sndDMA;
extern int		sndPaintedTime;

extern cVar_t	*s_volume;
extern cVar_t	*s_nosound;
extern cVar_t	*s_loadas8bit;
extern cVar_t	*s_khz;
extern cVar_t	*s_show;
extern cVar_t	*s_mixahead;
extern cVar_t	*s_testsound;
extern cVar_t	*s_primary;

extern cVar_t	*al_dopplerfactor;
extern cVar_t	*al_dopplervelocity;
extern cVar_t	*al_driver;
extern cVar_t	*al_extensions;
extern cVar_t	*al_ext_eax;

//
// snd_dma.c
//

#define	MAX_CHANNELS	32
extern channel_t		sndChannels[MAX_CHANNELS];

extern playSound_t		sndPendingPlays;

#define	MAX_RAW_SAMPLES	8192
extern int				sndRawEnd;
extern sfxSamplePair_t	sndRawSamples[MAX_RAW_SAMPLES];

void	Snd_IssuePlaysound (playSound_t *ps);

// picks a channel based on priorities, empty slots, number of channels
channel_t *Snd_PickChannel (int entNum, int entChannel);

void	Snd_Spatialize (channel_t *ch);	// spatializes a channel
void	Snd_SpatializeOrigin (vec3_t origin, float masterVol, float distMult, int *leftVol, int *rightVol);

extern vec3_t	sndViewOrigin;
extern vec3_t	sndViewForward;
extern vec3_t	sndViewRight;
extern vec3_t	sndViewUp;

//
// snd_mix.c
//

void	Snd_ScaleTableInit (void);
sfxCache_t *Snd_LoadSound (sfx_t *s);
void	Snd_PaintChannels (int endtime);

/*
=============================================================================

	OPENAL

=============================================================================
*/

typedef struct alConfig_s {
	byte		*rendString;
	byte		*vendString;
	byte		*versString;
	byte		*extsString;

	byte		*device;

	int			verMinor;
	int			verMajor;

	// extensions
	qBool		extEAX;
} alConfig_t;

extern alConfig_t alConfig;

#define	OPENAL_MAX_BUFFERS	1024
#define OPENAL_MAX_SOURCES	128

typedef struct OpenALBuffer_s {
	ALuint		buffer;
	ALboolean	inUse;
} OpenALBuffer_t;

typedef struct OpenALIndex_s {
	qBool		inUse;
	qBool		fixedOrigin;
	vec3_t		origin;
	int			entNum;
	int			sourceIndex;
} OpenALIndex_t;

extern qBool			sndALActive;

extern ALuint			sndALSources[OPENAL_MAX_SOURCES];
extern OpenALBuffer_t	sndALBuffers[OPENAL_MAX_BUFFERS];
extern OpenALIndex_t	sndALIndexes[MAX_CS_SOUNDS];

qBool	OpenAL_Init (void);
void	OpenAL_Shutdown (void);

void	OpenAL_DestroyBuffers (void);
ALint	OpenAL_GetFreeBuffer (void);
ALint	OpenAL_GetFreeSource (void);

void	OpenAL_FreeALIndexes (int index);
int		OpenAL_GetFreeALIndex (void);

qBool	OpenAL_CheckForError (void);

/*
=============================================================================

	SYSTEM SPECIFIC FUNCTIONS

=============================================================================
*/

qBool	ALimp_Init (void);
void	ALimp_Shutdown (void);

qBool	SndImp_Init (void);			// initializes cycling through a DMA buffer and returns information on it
int		SndImp_GetDMAPos (void);	// gets the current DMA position
void	SndImp_BeginPainting (void);
void	SndImp_Submit (void);
void	SndImp_Shutdown (void);
