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

// cd_win.c

#include <windows.h>
#include "../client/cl_local.h"
#include "winquake.h"

static qBool	cd_IsValid = qFalse;
static qBool	cd_Playing = qFalse;
static qBool	cd_WasPlaying = qFalse;
static qBool	cd_Initialized = qFalse;
static qBool	cd_Enabled = qFalse;
static qBool	cd_PlayLooping = qFalse;

static byte		cd_TrackRemap[100];
static byte		cd_PlayTrack;
static byte		cd_MaxTracks;

cvar_t	*cd_nocd;
cvar_t	*cd_loopcount;
cvar_t	*cd_looptrack;

UINT	wDeviceID;
int		cd_LoopCounter;

/*
===========
CDAudio_Eject
===========
*/
static void CDAudio_Eject (void) {
	DWORD	dwReturn;

	if (dwReturn = mciSendCommand (wDeviceID, MCI_SET, MCI_SET_DOOR_OPEN, (DWORD)NULL))
		Com_Printf (PRINT_DEV, S_COLOR_YELLOW "MCI_SET_DOOR_OPEN failed (%i)\n", dwReturn);
}


/*
===========
CDAudio_CloseDoor
===========
*/
static void CDAudio_CloseDoor (void) {
	DWORD	dwReturn;

	if (dwReturn = mciSendCommand (wDeviceID, MCI_SET, MCI_SET_DOOR_CLOSED, (DWORD)NULL))
		Com_Printf (PRINT_DEV, S_COLOR_YELLOW "MCI_SET_DOOR_CLOSED failed (%i)\n", dwReturn);
}


/*
===========
CDAudio_GetAudioDiskInfo
===========
*/
static int CDAudio_GetAudioDiskInfo (void) {
	DWORD				dwReturn;
	MCI_STATUS_PARMS	mciStatusParms;

	cd_IsValid = qFalse;

	mciStatusParms.dwItem = MCI_STATUS_READY;
	dwReturn = mciSendCommand (wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD) (LPVOID) &mciStatusParms);
	if (dwReturn) {
		Com_Printf (PRINT_DEV, S_COLOR_YELLOW "CDAudio: drive ready test - get status failed\n");
		return -1;
	}

	if (!mciStatusParms.dwReturn) {
		Com_Printf (PRINT_DEV, S_COLOR_YELLOW "CDAudio: drive not ready\n");
		return -1;
	}

	mciStatusParms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
	dwReturn = mciSendCommand (wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD) (LPVOID) &mciStatusParms);
	if (dwReturn) {
		Com_Printf (PRINT_DEV, S_COLOR_YELLOW "CDAudio: get tracks - status failed\n");
		return -1;
	}

	if (mciStatusParms.dwReturn < 1) {
		Com_Printf (PRINT_DEV, S_COLOR_YELLOW "CDAudio: no music tracks\n");
		return -1;
	}

	cd_IsValid = qTrue;
	cd_MaxTracks = mciStatusParms.dwReturn;

	return 0;
}


/*
===========
CDAudio_Pause
===========
*/
void CDAudio_Pause (void) {
	DWORD				dwReturn;
	MCI_GENERIC_PARMS	mciGenericParms;

	if (!cd_Enabled)
		return;

	if (!cd_Playing)
		return;

	mciGenericParms.dwCallback = (DWORD) g_hWnd;
	if (dwReturn = mciSendCommand (wDeviceID, MCI_PAUSE, 0, (DWORD)(LPVOID) &mciGenericParms))
		Com_Printf (PRINT_DEV, S_COLOR_YELLOW "MCI_PAUSE failed (%i)", dwReturn);

	cd_WasPlaying = cd_Playing;
	cd_Playing = qFalse;
}


/*
===========
CDAudio_Play2
===========
*/
void CDAudio_Play2 (int track, qBool looping) {
	DWORD				dwReturn;
	MCI_PLAY_PARMS		mciPlayParms;
	MCI_STATUS_PARMS	mciStatusParms;

	if (!cd_Enabled)
		return;
	
	if (!cd_IsValid) {
		CDAudio_GetAudioDiskInfo();
		if (!cd_IsValid)
			return;
	}

	track = cd_TrackRemap[track];

	if ((track < 1) || (track > cd_MaxTracks)) {
		CDAudio_Stop ();
		return;
	}

	/* don't try to play a non-audio track */
	mciStatusParms.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
	mciStatusParms.dwTrack = track;
	dwReturn = mciSendCommand (wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD) (LPVOID) &mciStatusParms);
	if (dwReturn) {
		Com_Printf (PRINT_DEV, S_COLOR_YELLOW "MCI_STATUS failed (%i)\n", dwReturn);
		return;
	}

	if (mciStatusParms.dwReturn != MCI_CDA_TRACK_AUDIO) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "CDAudio: track %i is not audio\n", track);
		return;
	}

	/* get the length of the track to be played */
	mciStatusParms.dwItem = MCI_STATUS_LENGTH;
	mciStatusParms.dwTrack = track;
	dwReturn = mciSendCommand (wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD) (LPVOID) &mciStatusParms);
	if (dwReturn) {
		Com_Printf (PRINT_DEV, S_COLOR_YELLOW "MCI_STATUS failed (%i)\n", dwReturn);
		return;
	}

	if (cd_Playing) {
		if (cd_PlayTrack == track)
			return;
		CDAudio_Stop ();
	}

	mciPlayParms.dwFrom = MCI_MAKE_TMSF(track, 0, 0, 0);
	mciPlayParms.dwTo = (mciStatusParms.dwReturn << 8) | track;
	mciPlayParms.dwCallback = (DWORD) g_hWnd;
	dwReturn = mciSendCommand (wDeviceID, MCI_PLAY, MCI_NOTIFY | MCI_FROM | MCI_TO, (DWORD)(LPVOID) &mciPlayParms);
	if (dwReturn) {
		Com_Printf (PRINT_DEV, S_COLOR_YELLOW "CDAudio: MCI_PLAY failed (%i)\n", dwReturn);
		return;
	}

	cd_PlayLooping = looping;
	cd_PlayTrack = track;
	cd_Playing = qTrue;

	if (Cvar_VariableValue ("cd_nocd"))
		CDAudio_Pause ();
}


/*
===========
CDAudio_Play
===========
*/
void CDAudio_Play (int track, qBool looping) {
	/* set a loop counter so that this track will change to the looptrack later */
	cd_LoopCounter = 0;
	CDAudio_Play2 (track, looping);
}


/*
===========
CDAudio_Stop
===========
*/
void CDAudio_Stop (void) {
	DWORD	dwReturn;

	if (!cd_Enabled)
		return;
	
	if (!cd_Playing)
		return;

	if (dwReturn = mciSendCommand (wDeviceID, MCI_STOP, 0, (DWORD)NULL))
		Com_Printf (PRINT_DEV, S_COLOR_YELLOW "MCI_STOP failed (%i)", dwReturn);

	cd_WasPlaying = qFalse;
	cd_Playing = qFalse;
}


/*
===========
CDAudio_Resume
===========
*/
void CDAudio_Resume (void) {
	DWORD			dwReturn;
	MCI_PLAY_PARMS	mciPlayParms;

	if (!cd_Enabled)
		return;
	
	if (!cd_IsValid)
		return;

	if (!cd_WasPlaying)
		return;
	
	mciPlayParms.dwFrom = MCI_MAKE_TMSF (cd_PlayTrack, 0, 0, 0);
	mciPlayParms.dwTo = MCI_MAKE_TMSF (cd_PlayTrack + 1, 0, 0, 0);
	mciPlayParms.dwCallback = (DWORD) g_hWnd;
	dwReturn = mciSendCommand (wDeviceID, MCI_PLAY, MCI_TO | MCI_NOTIFY, (DWORD)(LPVOID) &mciPlayParms);
	if (dwReturn) {
		Com_Printf (PRINT_DEV, S_COLOR_YELLOW "CDAudio: MCI_PLAY failed (%i)\n", dwReturn);
		return;
	}

	cd_Playing = qTrue;
}


/*
===========
CD_f
===========
*/
static void CD_f (void) {
	char	*command;
	int		ret;
	int		n;

	if (Cmd_Argc () < 2)
		return;

	command = Cmd_Argv (1);

	if (!Q_stricmp (command, "on")) {
		cd_Enabled = qTrue;
		return;
	}

	if (!Q_stricmp (command, "off")) {
		if (cd_Playing)
			CDAudio_Stop ();
		cd_Enabled = qFalse;
		return;
	}

	if (!Q_stricmp (command, "reset")) {
		cd_Enabled = qTrue;
		if (cd_Playing)
			CDAudio_Stop ();

		for (n=0; n<100 ; n++)
			cd_TrackRemap[n] = n;

		CDAudio_GetAudioDiskInfo ();
		return;
	}

	if (!Q_stricmp (command, "remap")) {
		ret = Cmd_Argc () - 2;
		if (ret <= 0) {
			for (n=1 ; n<100 ; n++) {
				if (cd_TrackRemap[n] != n)
					Com_Printf (PRINT_ALL, "  %u -> %u\n", n, cd_TrackRemap[n]);
			}

			return;
		}
		for (n=1 ; n<=ret ; n++)
			cd_TrackRemap[n] = atoi (Cmd_Argv (n+1));
		return;
	}

	if (!Q_stricmp (command, "close")) {
		CDAudio_CloseDoor ();
		return;
	}

	if (!cd_IsValid) {
		CDAudio_GetAudioDiskInfo ();
		if (!cd_IsValid) {
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "No CD in player.\n");
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
		if (cd_Playing)
			CDAudio_Stop ();
		CDAudio_Eject();
		cd_IsValid = qFalse;
		return;
	}

	if (!Q_stricmp (command, "info")) {
		Com_Printf (PRINT_ALL, "%u tracks\n", cd_MaxTracks);
		if (cd_Playing)
			Com_Printf (PRINT_ALL, "Currently %s track %u\n", cd_PlayLooping ? "looping" : "playing", cd_PlayTrack);
		else if (cd_WasPlaying)
			Com_Printf (PRINT_ALL, "Paused %s track %u\n", cd_PlayLooping ? "looping" : "playing", cd_PlayTrack);
		return;
	}
}


/*
===========
CDAudio_MessageHandler
===========
*/
LRESULT CALLBACK CDAudio_MessageHandler (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (lParam != wDeviceID)
		return 1;

	switch (wParam) {
	case MCI_NOTIFY_SUCCESSFUL:
		if (cd_Playing) {
			cd_Playing = qFalse;
			if (cd_PlayLooping) {
				/* if the track has played the given number of times, go to the ambient track */
				if (++cd_LoopCounter >= cd_loopcount->value)
					CDAudio_Play2 (cd_looptrack->value, qTrue);
				else
					CDAudio_Play2 (cd_PlayTrack, qTrue);
			}
		}
		break;

	case MCI_NOTIFY_ABORTED:
	case MCI_NOTIFY_SUPERSEDED:
		break;

	case MCI_NOTIFY_FAILURE:
		Com_Printf (PRINT_DEV, S_COLOR_YELLOW "MCI_NOTIFY_FAILURE\n");
		CDAudio_Stop ();
		cd_IsValid = qFalse;
		break;

	default:
		Com_Printf (PRINT_DEV, S_COLOR_YELLOW "Unexpected MM_MCINOTIFY type (%i)\n", wParam);
		return 1;
	}

	return 0;
}


/*
===========
CDAudio_Update
===========
*/
void CDAudio_Update (void) {
	if (cd_nocd->value != !cd_Enabled) {
		if (cd_nocd->value) {
			CDAudio_Stop ();
			cd_Enabled = qFalse;
		} else {
			cd_Enabled = qTrue;
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
void CDAudio_Activate (qBool active) {
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
qBool CDAudio_Init (void) {
	DWORD			dwReturn;
	MCI_OPEN_PARMS	mciOpenParms;
	MCI_SET_PARMS	mciSetParms;
	int				n;

	cd_nocd			= Cvar_Get ("cd_nocd",		"0",	CVAR_ARCHIVE);
	cd_loopcount	= Cvar_Get ("cd_loopcount",	"4",	0);
	cd_looptrack	= Cvar_Get ("cd_looptrack",	"11",	0);

	if (cd_nocd->value)
		return qFalse;

	mciOpenParms.lpstrDeviceType = "cdaudio";
	if (dwReturn = mciSendCommand (0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_SHAREABLE, (DWORD) (LPVOID) &mciOpenParms)) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "CDAudio_Init: MCI_OPEN failed (%i)\n", dwReturn);
		return qFalse;
	}
	wDeviceID = mciOpenParms.wDeviceID;

	/* set the time format to track/minute/second/frame (TMSF) */
	mciSetParms.dwTimeFormat = MCI_FORMAT_TMSF;
	if (dwReturn = mciSendCommand (wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)(LPVOID) &mciSetParms)) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "MCI_SET_TIME_FORMAT failed (%i)\n", dwReturn);
		mciSendCommand (wDeviceID, MCI_CLOSE, 0, (DWORD)NULL);
		return qFalse;
	}

	for (n=0 ; n<100; n++)
		cd_TrackRemap[n] = n;
	cd_Initialized = qTrue;
	cd_Enabled = qTrue;

	if (CDAudio_GetAudioDiskInfo ()) {
		Com_Printf (PRINT_ALL, "CDAudio_Init: No CD in player.\n");
		cd_IsValid = qFalse;
		cd_Enabled = qFalse;
	}

	Com_Printf (PRINT_ALL, "CD Audio Initialized\n");

	Cmd_AddCommand ("cd", CD_f);

	return qTrue;
}


/*
===========
CDAudio_Shutdown
===========
*/
void CDAudio_Shutdown (void) {
	if (!cd_Initialized)
		return;

	CDAudio_Stop ();
	if (mciSendCommand (wDeviceID, MCI_CLOSE, MCI_WAIT, (DWORD)NULL))
		Com_Printf (PRINT_DEV, S_COLOR_YELLOW "CDAudio_Shutdown: MCI_CLOSE failed\n");

	Cmd_RemoveCommand ("cd");
}
