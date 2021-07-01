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
// snd_pub.h
//

struct sfx_s;

void	Snd_CheckChanges (void);
void	Snd_Init (void);
void	Snd_Shutdown (qBool full);

void	Snd_FreeSounds (void);

// if origin is NULL, the sound will be dynamically sourced from the entity
void	Snd_StartSound (vec3_t origin, int entNum, int entChannel, struct sfx_s *sfx, float volume, float attenuation, float timeOfs);
void	Snd_StartLocalSound (struct sfx_s *sfx);

void	Snd_RawSamples (int samples, int rate, int width, int channels, byte *data);

void	Snd_StopAllSounds (void);
void	Snd_Update (vec3_t viewOrigin, vec3_t forward, vec3_t right, vec3_t up, qBool underWater);

void	Snd_BeginRegistration (void);
struct	sfx_s *Snd_RegisterSound (char *sample);
void	Snd_EndRegistration (void);

/*
=============================================================================

	PUBLIC SYSTEM SPECIFIC FUNCTIONS

=============================================================================
*/

void	SndImp_Activate (qBool active);
