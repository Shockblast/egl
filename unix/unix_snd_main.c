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
// unix_snd_main.c
//

#include "../client/snd_local.h"
#include "unix_local.h"

cVar_t	*s_bits;
cVar_t	*s_speed;
cVar_t	*s_channels;
cVar_t	*s_system;

typedef enum sndSystem_s {
	SNDSYS_NONE,
#if defined (__linux__)
	SNDSYS_ALSA,
#endif
	SNDSYS_OSS,
	SNDSYS_SDL,
} sndSystem_t;

char	*s_sysStrings[] = {
	"None",
#if defined (__linux__)
	"ALSA",
#endif
	"OSS",
	"SDL",

	NULL
};

#define S_DEFAULTSYS	SNDSYS_OSS

static sndSystem_t		s_curSystem;

/*
=======================================================================

	UNIX SOUND BACKEND

    This attempts access to ALSA, OSS, and SDL audio systems.
=======================================================================
*/

/*
==============
SndImp_Init
===============
*/
qBool SndImp_Init (void)
{
	sndSystem_t		oldSystem;
	qBool			success;

	// Register variables
	if (!s_bits) {
		s_bits     = Cvar_Register ("s_bits", "16", CVAR_ARCHIVE);
		s_speed    = Cvar_Register ("s_speed", "0", CVAR_ARCHIVE);
		s_channels = Cvar_Register ("s_channels", "2", CVAR_ARCHIVE);
		s_system   = Cvar_Register ("s_system", s_sysStrings[S_DEFAULTSYS], CVAR_ARCHIVE);
	}

	// Find the target system
	oldSystem = s_curSystem;
#if defined (__linux__)
	if (!Q_stricmp (s_system->string, "alsa"))
		s_curSystem = SNDSYS_ALSA;
	else 
#endif
	     if (!Q_stricmp (s_system->string, "oss"))
		s_curSystem = SNDSYS_OSS;
	else if (!Q_stricmp (s_system->string, "sdl"))
		s_curSystem = SNDSYS_SDL;
	else {
		Com_Printf (PRNT_ERROR, "SndImp_Init: Invalid s_system selection, defaulting to '%s'\n", s_sysStrings[S_DEFAULTSYS]);
		s_curSystem = S_DEFAULTSYS;
	}

mark0:
	// Initialize the target system
	success = qFalse;
	switch (s_curSystem) {
#if defined (__linux__)
	case SNDSYS_ALSA:
		success = ALSA_Init ();
		break;
#endif
	case SNDSYS_OSS:
		success = OSS_Init ();
		break;

	case SNDSYS_SDL:
		success = SDL_SND_Init ();
		break;
	}

	// If failed, fall-back
#if defined (__linux__)
	if (!success && s_curSystem > SNDSYS_ALSA) 
#endif
#if defined (__FreeBSD__)
	if (!success && s_curSystem > SNDSYS_OSS)
#endif
	{
		Com_Printf (PRNT_WARNING, "%s failed, attempting next system\n", s_sysStrings[s_curSystem]);
		s_curSystem--;
		goto mark0;
	}

	// Done
	return success;
}


/*
==============
SndImp_Shutdown
===============
*/
void SndImp_Shutdown (void)
{
	switch (s_curSystem) {
#if defined (__linux__)
	case SNDSYS_ALSA:
		ALSA_Shutdown ();
		break;
#endif
	case SNDSYS_OSS:
		OSS_Shutdown ();
		break;

	case SNDSYS_SDL:
		SDL_Shutdown ();
		break;
	}

	// Should never reach here
	return;
}


/*
==============
SndImp_GetDMAPos
===============
*/
int SndImp_GetDMAPos (void)
{
	switch (s_curSystem) {
#if defined (__linux__)
	case SNDSYS_ALSA:
		return ALSA_GetDMAPos ();
#endif
	case SNDSYS_OSS:
		return OSS_GetDMAPos ();

	case SNDSYS_SDL:
		return SDL_GetDMAPos ();
	}

	// Should never reach here
	return 0;
}


/*
==============
SndImp_BeginPainting
===============
*/
void SndImp_BeginPainting (void)
{
	switch (s_curSystem) {
#if defined (__linux__)
	case SNDSYS_ALSA:
		ALSA_BeginPainting ();
		break;
#endif
	case SNDSYS_OSS:
		OSS_BeginPainting ();
		break;

	case SNDSYS_SDL:
		SDL_BeginPainting ();
		break;
	}
}


/*
==============
SndImp_Submit

Send sound to device if buffer isn't really the DMA buffer
===============
*/
void SndImp_Submit (void)
{
	switch (s_curSystem) {
#if defined (__linux__)
	case SNDSYS_ALSA:
		ALSA_Submit ();
		break;
#endif
	case SNDSYS_OSS:
		OSS_Submit ();
		break;

	case SNDSYS_SDL:
		SDL_Submit ();
		break;
	}
}
