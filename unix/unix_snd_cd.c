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
// unix_snd_cd.c
//

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#if defined (__linux__)
#include <linux/cdrom.h>
#endif
#include "../client/cl_local.h"

#if defined (__FreeBSD__)
#include <sys/cdio.h>
#define CDROM_DATA_TRACK 4
#define CDROMEJECT CDIOCEJECT
#define CDROMCLOSETRAY CDIOCCLOSE
#define CDROMREADTOCHDR CDIOREADTOCHEADER
#define CDROMREADTOCENTRY CDIOREADTOCENTRYS
#define CDROMPLAYTRKIND CDIOCPLAYTRACKS
#define CDROMRESUME CDIOCRESUME
#define CDROMSTOP CDIOCSTOP
#define CDROMPAUSE CDIOCPAUSE
#define CDROMSUBCHNL CDIOCREADSUBCHANNEL
#endif

static qBool	cd_IsValid = qFalse;
static qBool	cd_Playing = qFalse;
static qBool	cd_WasPlaying = qFalse;
static qBool	cd_Initialized = qFalse;
static qBool	cd_Enabled = qTrue;
static qBool	cd_PlayLooping = qFalse;

static float	cd_Volume;
static byte	cd_TrackRemap[100];
static byte	cd_PlayTrack;
static byte	cd_MaxTracks;

static int	cd_File = -1;

cVar_t         *cd_volume;
cVar_t         *cd_nocd;
cVar_t         *cd_dev;


/*
===========
CDAudio_Eject
===========
*/
static void CDAudio_Eject (void)
{
	if (cd_File == -1 || !cd_Enabled)
		return;		// no cd init'd
		
	if (ioctl(cd_File, CDROMEJECT) == -1)
		Com_DevPrintf(PRNT_WARNING, "ioctl cdromeject failed\n");
}


/*
===========
CDAudio_CloseDoor
===========
*/
static void CDAudio_CloseDoor (void)
{
	if (cd_File == -1 || !cd_Enabled)
		return;		// no cd init'd

	if (ioctl(cd_File, CDROMCLOSETRAY) == -1)
		Com_DevPrintf(PRNT_WARNING,"ioctl cdromclosetray failed\n");
}


/*
===========
CDAudio_GetAudioDiskInfo
===========
*/
static int CDAudio_GetAudioDiskInfo (void)
{
#if defined (__linux__)
	struct cdrom_tochdr tochdr;
#endif
#if defined (__FreeBSD__)
	struct ioc_toc_header tochdr;
#endif

	cd_IsValid = qFalse;
	
	if (ioctl(cd_File, CDROMREADTOCHDR, &tochdr) == -1) {
		Com_DevPrintf(PRNT_WARNING, "ioctl cdromreadtochdr failed\n");
		return -1;
	}
#if defined (__linux__)
	if (tochdr.cdth_trk0 < 1) 
#endif
#if defined (__FreeBSD__)
	if (tochdr.starting_track < 1)
#endif
	{
		Com_DevPrintf(PRNT_WARNING, "CDAudio: no music tracks\n");
		return -1;
	}
	
	cd_IsValid = qTrue;
#if defined (__linux__)
	cd_MaxTracks = tochdr.cdth_trk1;
#endif
#if defined (__FreeBSD__)
	cd_MaxTracks = tochdr.ending_track;
#endif

	return 0;
}


/*
===========
CDAudio_Pause
===========
*/
void CDAudio_Pause (void)
{
	if (cd_File == -1 || !cd_Enabled)
		return;

	if (!cd_Playing)
		return;
	if (ioctl(cd_File, CDROMPAUSE) == -1)
		Com_DevPrintf(PRNT_WARNING, "ioctl cdrompause failed\n");

	cd_WasPlaying = cd_Playing;
	cd_Playing = qFalse;
}


/*
===========
CDAudio_Play2
===========
*/
void CDAudio_Play2 (void)
{
	int		track, i = 0, free_tracks = 0, remap_track;
	float		f;
	byte           *track_bools;
#if defined (__linux__)
	struct cdrom_tocentry entry;
	struct cdrom_ti	ti;
#endif
#if defined (__FreeBSD__)
	struct ioc_read_toc_entry entry;
	struct cd_toc_entry toc_buffer;
	struct ioc_play_track ti;
#endif

	if (cd_File == -1 || !cd_Enabled)
		return;

	track_bools = (byte *) malloc(cd_MaxTracks * sizeof(byte));

	if (track_bools == 0)
		return;

	// create array of available audio tracknumbers

	for (; i < cd_MaxTracks; i++) {
#if defined (__FreeBSD__)
		bzero((char *)&toc_buffer, sizeof(toc_buffer));
		entry.data_len = sizeof(toc_buffer);
		entry.data = &toc_buffer;

		entry.starting_track = cd_TrackRemap[i];
		entry.address_format = CD_LBA_FORMAT;
#endif
#if defined (__linux__)

		entry.cdte_track = cd_TrackRemap[i];
		entry.cdte_format = CDROM_LBA;
#endif
		if (ioctl(cd_File, CDROMREADTOCENTRY, &entry) == -1) {
			track_bools[i] = 0;
		} else
#if defined (__FreeBSD__)
			track_bools[i] = (entry.data->control != CDROM_DATA_TRACK);
#endif
#if defined (__linux__)
			track_bools[i] = (entry.cdte_ctrl != CDROM_DATA_TRACK);
#endif	
			free_tracks += track_bools[i];
	}

	if (!free_tracks) {
		Com_DevPrintf(PRNT_WARNING, "CDAudio_RandomPlay: Unable to find and play a random audio track, insert an audio cd please");
		goto free_end;
	}
	// choose random audio track
	do {
		do {
			f = ((float)rand()) / ((float)RAND_MAX + 1.0);
			track = (int)(cd_MaxTracks * f);
		}
		while (!track_bools[track]);

		remap_track = cd_TrackRemap[track];

		if (cd_Playing) {
			if (cd_PlayTrack == remap_track) {
				goto free_end;
			}
			CDAudio_Stop();
		}
#if defined (__linux__)
		ti.cdti_trk0 = remap_track;
		ti.cdti_trk1 = remap_track;
		ti.cdti_ind0 = 0;
		ti.cdti_ind1 = 0;
#endif
#if defined (__FreeBSD__)
		ti.start_track = remap_track;
		ti.end_track = remap_track;
		ti.start_index = 0;
		ti.end_index = 0;
#endif

		if (ioctl(cd_File, CDROMPLAYTRKIND, &ti) == -1) {
			track_bools[track] = 0;
			free_tracks--;
		} else {
			cd_PlayLooping = qTrue;
			cd_PlayTrack = remap_track;
			cd_Playing = qTrue;
			break;
		}
	}
	while (free_tracks > 0);

free_end:
	free((void *)track_bools);
}


/*
===========
CDAudio_Play
===========
*/
void CDAudio_Play (int track, qBool looping)
{
#if defined (__linux__)
	struct cdrom_tocentry entry;
	struct cdrom_ti ti;
#endif
#if defined (__FreeBSD__)
	struct ioc_read_toc_entry entry;
	struct cd_toc_entry toc_buffer;
	struct ioc_play_track ti;
#endif

	if (cd_File == -1 || !cd_Enabled)
		return;

	if (!cd_IsValid) {
		CDAudio_GetAudioDiskInfo();
		if (!cd_IsValid)
			return;
	}
	track = cd_TrackRemap[track];

	if (track < 1 || track > cd_MaxTracks) {
		Com_DevPrintf(PRNT_WARNING, "CDAudio: Bad track number %u.\n", track);
		return;
	}
	// don't try to play a non-audio track
#if defined (__linux__)
	entry.cdte_track = track;
	entry.cdte_format = CDROM_MSF;
#endif
#if defined (__FreeBSD__)
	bzero((char *)&toc_buffer, sizeof(toc_buffer));
	entry.data_len = sizeof(toc_buffer);
	entry.data = &toc_buffer;
	// don't try to play a non-audio track
	entry.starting_track = track;
	entry.address_format = CD_MSF_FORMAT;
#endif
	if (ioctl(cd_File, CDROMREADTOCENTRY, &entry) == -1) {
		Com_DevPrintf(PRNT_WARNING, "ioctl cdromreadtocentry failed\n");
		return;
	}
#if defined (__linux__)
	if (entry.cdte_ctrl == CDROM_DATA_TRACK)
#endif
#if defined (__FreeBSD__)
	if (toc_buffer.control == CDROM_DATA_TRACK)
#endif
	{
		Com_Printf(PRNT_WARNING, "CDAudio: track %i is not audio\n", track);
		return;
	}
	if (cd_Playing) {
		if (cd_PlayTrack == track)
			return;
		CDAudio_Stop();
	}
#if defined (__linux__)
	ti.cdti_trk0 = track;
	ti.cdti_trk1 = track;
	ti.cdti_ind0 = 1;
	ti.cdti_ind1 = 99;
#endif
#if defined (__FreeBSD__)
	ti.start_track = track;
	ti.end_track = track;
	ti.start_index = 1;
	ti.end_index = 99;
#endif

	if (ioctl(cd_File, CDROMPLAYTRKIND, &ti) == -1) {
		Com_DevPrintf(PRNT_WARNING, "ioctl cdromplaytrkind failed\n");
		return;
	}
	if (ioctl(cd_File, CDROMRESUME) == -1)
		Com_DevPrintf(PRNT_WARNING, "ioctl cdromresume failed\n");

	cd_PlayLooping = looping;
	cd_PlayTrack = track;
	cd_Playing = qTrue;

	if (cd_volume->floatVal == 0.0)
		CDAudio_Pause();
}


/*
===========
CDAudio_Stop
===========
*/
void CDAudio_Stop (void)
{
	if (cd_File == -1 || !cd_Enabled)
		return;

	if (!cd_Playing)
		return;

	if (ioctl(cd_File, CDROMSTOP) == -1)
		Com_DevPrintf(PRNT_WARNING, "ioctl cdromstop failed (%d)\n", errno);

	cd_WasPlaying = qFalse;
	cd_Playing = qFalse;
}


/*
===========
CDAudio_Resume
===========
*/
void CDAudio_Resume (void)
{
	if (cd_File == -1 || !cd_Enabled)
		return;

	if (!cd_IsValid)
		return;

	if (!cd_WasPlaying)
		return;

	if (ioctl(cd_File, CDROMRESUME) == -1)
		Com_DevPrintf(PRNT_WARNING,"ioctl cdromresume failed\n");
	cd_Playing = qTrue;
}


/*
===========
CD_f
===========
*/
static void CD_f (void)
{
	char           *command;
	int		ret;
	int		n;

	if (Cmd_Argc() < 2)
		return;

	command = Cmd_Argv(1);

	if (Q_stricmp(command, "on") == 0) {
		cd_Enabled = qTrue;
		return;
	}
	if (Q_stricmp(command, "off") == 0) {
		if (cd_Playing)
			CDAudio_Stop();
		cd_Enabled = qFalse;
		return;
	}
	if (Q_stricmp(command, "reset") == 0) {
		cd_Enabled = qTrue;
		if (cd_Playing)
			CDAudio_Stop();
		for (n = 0; n < 100; n++)
			cd_TrackRemap[n] = n;
		CDAudio_GetAudioDiskInfo();
		return;
	}
	if (Q_stricmp(command, "remap") == 0) {
		ret = Cmd_Argc() - 2;
		if (ret <= 0) {
			for (n = 1; n < 100; n++)
				if (cd_TrackRemap[n] != n)
					Com_Printf(0, "  %u -> %u\n", n, cd_TrackRemap[n]);
			return;
		}
		for (n = 1; n <= ret; n++)
			cd_TrackRemap[n] = atoi(Cmd_Argv(n + 1));
		return;
	}
	if (Q_stricmp(command, "close") == 0) {
		CDAudio_CloseDoor();
		return;
	}
	if (!cd_IsValid) {
		CDAudio_GetAudioDiskInfo();
		if (!cd_IsValid) {
			Com_Printf(0, "No CD in player.\n");
			return;
		}
	}
	if (Q_stricmp(command, "play") == 0) {
		CDAudio_Play((byte) atoi(Cmd_Argv(2)), qFalse);
		return;
	}
	if (Q_stricmp(command, "loop") == 0) {
		CDAudio_Play((byte) atoi(Cmd_Argv(2)), qTrue);
		return;
	}
	if (Q_stricmp(command, "stop") == 0) {
		CDAudio_Stop();
		return;
	}
	if (Q_stricmp(command, "pause") == 0) {
		CDAudio_Pause();
		return;
	}
	if (Q_stricmp(command, "resume") == 0) {
		CDAudio_Resume();
		return;
	}
	if (Q_stricmp(command, "eject") == 0) {
		if (cd_Playing)
			CDAudio_Stop();
		CDAudio_Eject();
		cd_IsValid = qFalse;
		return;
	}
	if (!Q_stricmp(command, "random")) {
		CDAudio_Play2();
		return;
	}
	if (Q_stricmp(command, "info") == 0) {
		Com_Printf(0, "%u tracks\n", cd_MaxTracks);
		if (cd_Playing)
			Com_Printf(0, "Currently %s track %u\n", cd_PlayLooping ? "looping" : "playing", cd_PlayTrack);
		else if (cd_WasPlaying)
			Com_Printf(0, "Paused %s track %u\n", cd_PlayLooping ? "looping" : "playing", cd_PlayTrack);
		Com_Printf(0, "Volume is %f\n", cd_Volume);
		return;
	}
}


/*
===========
CDAudio_Update
===========
*/
void CDAudio_Update (void)
{
#if defined (__linux__)
	struct cdrom_subchnl subchnl;
#endif
#if defined (__FreeBSD__)
	struct ioc_read_subchannel subchnl;
	struct cd_sub_channel_info data;
#endif
	static time_t	lastchk;

	if (cd_File == -1 || !cd_Enabled)
		return;

	if (cd_volume && cd_volume->floatVal != cd_Volume) {
		if (cd_Volume) {
			Cvar_SetValue("cd_volume", 0.0, qFalse);
			cd_Volume = cd_volume->floatVal;
			CDAudio_Pause();
		} else {
			Cvar_SetValue("cd_volume", 1.0, qTrue);
			cd_Volume = cd_volume->floatVal;
			CDAudio_Resume();
		}
	}
	if (cd_Playing && lastchk < time(NULL)) {
		lastchk = time(NULL) + 2;	// two seconds between chks
#if defined (__linux__)
		subchnl.cdsc_format = CDROM_MSF;
#endif
#if defined (__FreeBSD__)
		subchnl.address_format = CD_MSF_FORMAT;
		subchnl.data_format = CD_CURRENT_POSITION;
		subchnl.data_len = sizeof(data);
		subchnl.track = cd_PlayTrack;
		subchnl.data = &data;
#endif
		if (ioctl(cd_File, CDROMSUBCHNL, &subchnl) == -1) {
			Com_DevPrintf(PRNT_WARNING,"ioctl cdromsubchnl failed\n");
			cd_Playing = qFalse;
			return;
		}
#if defined (__linux__)
		if (subchnl.cdsc_audiostatus != CDROM_AUDIO_PLAY &&
		    subchnl.cdsc_audiostatus != CDROM_AUDIO_PAUSED) 
#endif
#if defined (__FreeBSD__)
		if (subchnl.data->header.audio_status != CD_AS_PLAY_IN_PROGRESS &&
		    subchnl.data->header.audio_status != CD_AS_PLAY_PAUSED)
#endif
		{
			cd_Playing = qFalse;
			if (cd_PlayLooping)
				CDAudio_Play(cd_PlayTrack, qTrue);
		}
	}
}


/*
===========
CDAudio_Activate
===========
*/
void CDAudio_Activate (qBool active)
{
	if (active)
		CDAudio_Resume();
	else
		CDAudio_Pause();
}


/*
===========
CDAudio_Init
===========
*/
qBool CDAudio_Init (void)
{
	int		i;
	cVar_t         *cv;
	extern uid_t	saved_euid;

	if (cd_Initialized)
		return 0;

	cv = Cvar_Register("nocdaudio", "0", CVAR_LATCH_AUDIO);
	if (cv->floatVal)
		return -1;

	cd_nocd = Cvar_Register("cd_nocd", "0", CVAR_ARCHIVE);
	if (cd_nocd->floatVal)
		return -1;

	cd_volume = Cvar_Register("cd_volume", "1", CVAR_ARCHIVE);
#if defined (__linux__)
	cd_dev = Cvar_Register("cd_dev", "/dev/cdrom", CVAR_ARCHIVE);
#endif
#if defined (__FreeBSD__)
	cd_dev = Cvar_Register("cd_dev", "/dev/acd0", CVAR_ARCHIVE);
#endif	
seteuid(saved_euid);

	cd_File = open(cd_dev->string, O_RDONLY | O_NONBLOCK | O_EXCL);

	seteuid(getuid());

	if (cd_File == -1) {
		Com_Printf(0, "CDAudio_Init: open of \"%s\" failed (%i)\n", cd_dev->string, errno);

		cd_File = -1;
		return -1;
	}
	for (i = 0; i < 100; i++)
		cd_TrackRemap[i] = i;
	cd_Initialized = qTrue;
	cd_Enabled = qTrue;

	if (CDAudio_GetAudioDiskInfo()) {
		Com_Printf(0, "CDAudio_Init: No CD in player.\n");
		cd_IsValid = qFalse;
	}
	Cmd_AddCommand("cd", CD_f, "Has the CD player perform a specified task");

	Com_Printf(0, "CD Audio Initialized\n");

	return 0;
}


/*
===========
CDAudio_Shutdown
===========
*/
void CDAudio_Shutdown (void)
{
	if (!cd_Initialized)
		return;
		
	CDAudio_Stop();
	close(cd_File);
	cd_File = -1;
	cd_Initialized = qFalse;
}

