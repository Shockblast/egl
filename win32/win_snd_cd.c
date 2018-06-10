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
// win_snd_cd.c
//

#include <windows.h>
#include "../client/cl_local.h"
#include "win_local.h"

static qBool	cd_isValid;
static qBool	cd_isPlaying;
static qBool	cd_wasPlaying;
static qBool	cd_initialized;
static qBool	cd_enabled;
static qBool	cd_playLooping;

static byte		cd_trackRemap[100];
static byte		cd_playTrack;
static byte		cd_maxTracks;

cVar_t	*cd_nocd;
cVar_t	*cd_loopcount;
cVar_t	*cd_looptrack;

MCIDEVICEID	wDeviceID;
int			cd_LoopCounter;

static void	*cmd_cd;

/*
===========
CDAudio_Eject
===========
*/
static void CDAudio_Eject (void)
{
	DWORD	dwReturn;

	dwReturn = mciSendCommand (wDeviceID, MCI_SET, MCI_SET_DOOR_OPEN, (DWORD_PTR) NULL);
	if (dwReturn)
		Com_DevPrintf (PRNT_WARNING, "MCI_SET_DOOR_OPEN failed (%i)\n", dwReturn);
}


/*
===========
CDAudio_CloseDoor
===========
*/
static void CDAudio_CloseDoor (void)
{
	DWORD	dwReturn;

	dwReturn = mciSendCommand (wDeviceID, MCI_SET, MCI_SET_DOOR_CLOSED, (DWORD_PTR) NULL);
	if (dwReturn)
		Com_DevPrintf (PRNT_WARNING, "MCI_SET_DOOR_CLOSED failed (%i)\n", dwReturn);
}


/*
===========
CDAudio_GetAudioDiskInfo
===========
*/
static int CDAudio_GetAudioDiskInfo (void)
{
	DWORD				dwReturn;
	MCI_STATUS_PARMS	mciStatusParms;

	cd_isValid = qFalse;

	mciStatusParms.dwItem = MCI_STATUS_READY;
	dwReturn = mciSendCommand (wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD_PTR) &mciStatusParms);
	if (dwReturn) {
		Com_DevPrintf (PRNT_WARNING, "CDAudio: drive ready test - get status failed\n");
		return -1;
	}

	if (!mciStatusParms.dwReturn) {
		Com_DevPrintf (PRNT_WARNING, "CDAudio: drive not ready\n");
		return -1;
	}

	mciStatusParms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
	dwReturn = mciSendCommand (wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD_PTR) &mciStatusParms);
	if (dwReturn) {
		Com_DevPrintf (PRNT_WARNING, "CDAudio: get tracks - status failed\n");
		return -1;
	}

	if (mciStatusParms.dwReturn < 1) {
		Com_DevPrintf (PRNT_WARNING, "CDAudio: no music tracks\n");
		return -1;
	}

	cd_isValid = qTrue;
	cd_maxTracks = mciStatusParms.dwReturn;

	return 0;
}


/*
===========
CDAudio_Pause
===========
*/
void CDAudio_Pause (void)
{
	DWORD				dwReturn;
	MCI_GENERIC_PARMS	mciGenericParms;

	if (!cd_enabled)
		return;
	if (!cd_isPlaying)
		return;

	mciGenericParms.dwCallback = (DWORD_PTR) sys_winInfo.hWnd;
	dwReturn = mciSendCommand (wDeviceID, MCI_PAUSE, 0, (DWORD_PTR) &mciGenericParms);
	if (dwReturn)
		Com_DevPrintf (PRNT_WARNING, "MCI_PAUSE failed (%i)", dwReturn);

	cd_wasPlaying = cd_isPlaying;
	cd_isPlaying = qFalse;
}


/*
===========
CDAudio_Play2
===========
*/
void CDAudio_Play2 (int track, qBool looping)
{
	DWORD				dwReturn;
	MCI_PLAY_PARMS		mciPlayParms;
	MCI_STATUS_PARMS	mciStatusParms;

	if (!cd_enabled)
		return;
	
	if (!cd_isValid) {
		CDAudio_GetAudioDiskInfo();
		if (!cd_isValid)
			return;
	}

	track = cd_trackRemap[track];
	if (track < 1 || track > cd_maxTracks) {
		CDAudio_Stop ();
		return;
	}

	// don't try to play a non-audio track
	mciStatusParms.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
	mciStatusParms.dwTrack = track;
	dwReturn = mciSendCommand (wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD_PTR) &mciStatusParms);
	if (dwReturn) {
		Com_DevPrintf (PRNT_WARNING, "MCI_STATUS failed (%i)\n", dwReturn);
		return;
	}

	if (mciStatusParms.dwReturn != MCI_CDA_TRACK_AUDIO) {
		Com_Printf (PRNT_WARNING, "CDAudio: track %i is not audio\n", track);
		return;
	}

	// get the length of the track to be played
	mciStatusParms.dwItem = MCI_STATUS_LENGTH;
	mciStatusParms.dwTrack = track;
	dwReturn = mciSendCommand (wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD_PTR) &mciStatusParms);
	if (dwReturn) {
		Com_DevPrintf (PRNT_WARNING, "MCI_STATUS failed (%i)\n", dwReturn);
		return;
	}

	if (cd_isPlaying) {
		if (cd_playTrack == track)
			return;
		CDAudio_Stop ();
	}

	mciPlayParms.dwFrom = MCI_MAKE_TMSF(track, 0, 0, 0);
	mciPlayParms.dwTo = (mciStatusParms.dwReturn << 8) | track;
	mciPlayParms.dwCallback = (DWORD_PTR) sys_winInfo.hWnd;
	dwReturn = mciSendCommand (wDeviceID, MCI_PLAY, MCI_NOTIFY | MCI_FROM | MCI_TO, (DWORD_PTR) &mciPlayParms);
	if (dwReturn) {
		Com_DevPrintf (PRNT_WARNING, "CDAudio: MCI_PLAY failed (%i)\n", dwReturn);
		return;
	}

	cd_playLooping = looping;
	cd_playTrack = track;
	cd_isPlaying = qTrue;

	if (cd_nocd->intVal)
		CDAudio_Pause ();
}


/*
===========
CDAudio_Play
===========
*/
void CDAudio_Play (int track, qBool looping)
{
	// set a loop counter so that this track will change to the looptrack later
	cd_LoopCounter = 0;
	CDAudio_Play2 (track, looping);
}


/*
===========
CDAudio_Stop
===========
*/
void CDAudio_Stop (void)
{
	DWORD	dwReturn;

	if (!cd_enabled || !cd_isPlaying)
		return;

	dwReturn = mciSendCommand (wDeviceID, MCI_STOP, 0, (DWORD_PTR) NULL);
	if (dwReturn)
		Com_DevPrintf (PRNT_WARNING, "MCI_STOP failed (%i)", dwReturn);

	cd_wasPlaying = qFalse;
	cd_isPlaying = qFalse;
}


/*
===========
CDAudio_Resume
===========
*/
void CDAudio_Resume (void)
{
	DWORD			dwReturn;
	MCI_PLAY_PARMS	mciPlayParms;

	if (!cd_enabled || !cd_isValid || !cd_wasPlaying)
		return;

	mciPlayParms.dwFrom = MCI_MAKE_TMSF (cd_playTrack, 0, 0, 0);
	mciPlayParms.dwTo = MCI_MAKE_TMSF (cd_playTrack + 1, 0, 0, 0);
	mciPlayParms.dwCallback = (DWORD_PTR) sys_winInfo.hWnd;
	dwReturn = mciSendCommand (wDeviceID, MCI_PLAY, MCI_TO | MCI_NOTIFY, (DWORD_PTR) &mciPlayParms);
	if (dwReturn) {
		Com_DevPrintf (PRNT_WARNING, "CDAudio: MCI_PLAY failed (%i)\n", dwReturn);
		return;
	}

	cd_isPlaying = qTrue;
}


/*
===========
CD_f
===========
*/
static void CD_f (void)
{
	char	*command;
	int		ret;
	int		n;

	if (Cmd_Argc () < 2)
		return;

	command = Cmd_Argv (1);

	if (!Q_stricmp (command, "on")) {
		cd_enabled = qTrue;
		return;
	}

	if (!Q_stricmp (command, "off")) {
		if (cd_isPlaying)
			CDAudio_Stop ();
		cd_enabled = qFalse;
		return;
	}

	if (!Q_stricmp (command, "reset")) {
		cd_enabled = qTrue;
		if (cd_isPlaying)
			CDAudio_Stop ();

		for (n=0; n<100 ; n++)
			cd_trackRemap[n] = n;

		CDAudio_GetAudioDiskInfo ();
		return;
	}

	if (!Q_stricmp (command, "remap")) {
		ret = Cmd_Argc () - 2;
		if (ret <= 0) {
			for (n=1 ; n<100 ; n++) {
				if (cd_trackRemap[n] != n)
					Com_Printf (0, "  %u -> %u\n", n, cd_trackRemap[n]);
			}

			return;
		}
		for (n=1 ; n<=ret ; n++)
			cd_trackRemap[n] = atoi (Cmd_Argv (n+1));
		return;
	}

	if (!Q_stricmp (command, "close")) {
		CDAudio_CloseDoor ();
		return;
	}

	if (!cd_isValid) {
		CDAudio_GetAudioDiskInfo ();
		if (!cd_isValid) {
			Com_Printf (PRNT_WARNING, "No CD in player.\n");
			return;
		}
	}

	if (!Q_stricmp (command, "play")) {
		CDAudio_Play (atoi(Cmd_Argv (2)), qFalse);
		return;
	}

	if (!Q_stricmp (command, "loop")) {
		CDAudio_Play (atoi(Cmd_Argv (2)), qTrue);
		return;
	}

	if (!Q_stricmp (command, "stop")) {
		CDAudio_Stop ();
		return;
	}

	if (!Q_stricmp (command, "pause")) {
		CDAudio_Pause ();
		return;
	}

	if (!Q_stricmp (command, "resume")) {
		CDAudio_Resume ();
		return;
	}

	if (!Q_stricmp (command, "eject")) {
		if (cd_isPlaying)
			CDAudio_Stop ();
		CDAudio_Eject();
		cd_isValid = qFalse;
		return;
	}

	if (!Q_stricmp (command, "info")) {
		Com_Printf (0, "%u tracks\n", cd_maxTracks);
		if (cd_isPlaying)
			Com_Printf (0, "Currently %s track %u\n", cd_playLooping ? "looping" : "playing", cd_playTrack);
		else if (cd_wasPlaying)
			Com_Printf (0, "Paused %s track %u\n", cd_playLooping ? "looping" : "playing", cd_playTrack);
		return;
	}
}


/*
===========
CDAudio_MessageHandler
===========
*/
LRESULT CALLBACK CDAudio_MessageHandler (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (lParam != (LPARAM) wDeviceID)
		return 1;

	switch (wParam) {
	case MCI_NOTIFY_SUCCESSFUL:
		if (cd_isPlaying) {
			cd_isPlaying = qFalse;
			if (cd_playLooping) {
				// if the track has played the given number of times, go to the ambient track
				if (++cd_LoopCounter >= cd_loopcount->intVal)
					CDAudio_Play2 (cd_looptrack->intVal, qTrue);
				else
					CDAudio_Play2 (cd_playTrack, qTrue);
			}
		}
		break;

	case MCI_NOTIFY_ABORTED:
	case MCI_NOTIFY_SUPERSEDED:
		break;

	case MCI_NOTIFY_FAILURE:
		Com_DevPrintf (PRNT_WARNING, "MCI_NOTIFY_FAILURE\n");
		CDAudio_Stop ();
		cd_isValid = qFalse;
		break;

	default:
		Com_DevPrintf (PRNT_WARNING, "Unexpected MM_MCINOTIFY type (%i)\n", wParam);
		return 1;
	}

	return 0;
}


/*
===========
CDAudio_Update
===========
*/
void CDAudio_Update (void)
{
	if (cd_nocd->intVal != !cd_enabled) {
		if (cd_nocd->intVal) {
			CDAudio_Stop ();
			cd_enabled = qFalse;
		}
		else {
			cd_enabled = qTrue;
			CDAudio_Resume ();
		}
	}
}


/*
===========
CDAudio_Activate

Called when the main window gains or loses focus. The window have been
destroyed and recreated between a deactivate and an activate.
===========
*/
void CDAudio_Activate (qBool active)
{
	if (active)
		CDAudio_Resume ();
	else
		CDAudio_Pause ();
}


/*
===========
CDAudio_Init
===========
*/
qBool CDAudio_Init (void)
{
	DWORD			dwReturn;
	MCI_OPEN_PARMS	mciOpenParms;
	MCI_SET_PARMS	mciSetParms;
	int				n;

	cd_nocd			= Cvar_Register ("cd_nocd",		"0",	CVAR_ARCHIVE);
	cd_loopcount	= Cvar_Register ("cd_loopcount",	"4",	0);
	cd_looptrack	= Cvar_Register ("cd_looptrack",	"11",	0);

	if (cd_nocd->intVal)
		return qFalse;

	mciOpenParms.lpstrDeviceType = "cdaudio";
	dwReturn = mciSendCommand (0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_SHAREABLE, (DWORD_PTR) &mciOpenParms);
	if (dwReturn) {
		Com_Printf (PRNT_WARNING, "CDAudio_Init: MCI_OPEN failed (%i)\n", dwReturn);
		return qFalse;
	}
	wDeviceID = mciOpenParms.wDeviceID;

	// set the time format to track/minute/second/frame (TMSF)
	mciSetParms.dwTimeFormat = MCI_FORMAT_TMSF;
	dwReturn = mciSendCommand (wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD_PTR) &mciSetParms);
	if (dwReturn) {
		Com_Printf (PRNT_WARNING, "MCI_SET_TIME_FORMAT failed (%i)\n", dwReturn);
		mciSendCommand (wDeviceID, MCI_CLOSE, 0, (DWORD_PTR) NULL);
		return qFalse;
	}

	for (n=0 ; n<100; n++)
		cd_trackRemap[n] = n;
	cd_initialized = qTrue;
	cd_enabled = qTrue;

	if (CDAudio_GetAudioDiskInfo ()) {
		Com_Printf (0, "CDAudio_Init: No CD in player.\n");
		cd_isValid = qFalse;
		cd_enabled = qFalse;
	}

	Com_Printf (0, "CD Audio Initialized\n");

	cmd_cd	= Cmd_AddCommand ("cd",	CD_f,		"Has the CD player perform a specified task");

	return qTrue;
}


/*
===========
CDAudio_Shutdown
===========
*/
void CDAudio_Shutdown (void)
{
	if (!cd_initialized)
		return;

	CDAudio_Stop ();
	if (mciSendCommand (wDeviceID, MCI_CLOSE, MCI_WAIT, (DWORD_PTR) NULL))
		Com_DevPrintf (PRNT_WARNING, "CDAudio_Shutdown: MCI_CLOSE failed\n");

	Cmd_RemoveCommand ("cd", cmd_cd);
}
