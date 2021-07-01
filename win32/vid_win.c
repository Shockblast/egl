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
// vid_win.c
//

#include "../client/cl_local.h"
#include "../renderer/r_local.h"
#include "winquake.h"
#include "resource.h"
#include "glimp_win.h"

// console variables that we need to access from this module
cVar_t		*vid_xpos;			// x coordinate of window position
cVar_t		*vid_ypos;			// y coordinate of window position
cVar_t		*vid_fullscreen;

cVar_t		*win_noalttab;

HWND		globalhWnd;				// main window handle for life of program

static qBool	sysAltTabDisabled;

static qBool	vidActive;
static qBool	vidRestart;

/*
==============================================================================

	CONSOLE FUNCTIONS

==============================================================================
*/

/*
============
VID_Restart_f
============
*/
void VID_Restart_f (void) {
	vidRestart = qTrue;
}


/*
============
VID_Front_f
============
*/
static void VID_Front_f (void) {
	SetWindowLong (globalhWnd, GWL_EXSTYLE, WS_EX_TOPMOST);
	SetForegroundWindow (globalhWnd);
}

/*
==============================================================================

	MESSAGE HANDLING

==============================================================================
*/

/*
=================
WIN_ToggleAltTab
=================
*/
static void WIN_ToggleAltTab (qBool enable) {
	if (enable) {
		if (!sysAltTabDisabled)
			return;

		if (sysWindows) {
			BOOL	old;
			SystemParametersInfo (SPI_SCREENSAVERRUNNING, 0, &old, 0);
		}
		else {
			UnregisterHotKey (0, 0);
			UnregisterHotKey (0, 1);
		}

		sysAltTabDisabled = qFalse;
	}
	else {
		if (sysAltTabDisabled)
			return;

		if (sysWindows) {
			BOOL	old;
			SystemParametersInfo (SPI_SCREENSAVERRUNNING, 1, &old, 0);
		}
		else {
			RegisterHotKey (0, 0, MOD_ALT, VK_TAB);
			RegisterHotKey (0, 1, MOD_ALT, VK_RETURN);
		}

		sysAltTabDisabled = qTrue;
	}
}


/*
====================
VID_AppActivate
====================
*/
static void VID_AppActivate (qBool fActive, qBool minimize) {
	sysMinimized = minimize;

	Key_ClearStates ();

	// we don't want to act like we're active if we're minimized
	sysActiveApp = (fActive && !minimize) ? qTrue : qFalse;

	// minimize/restore mouse-capture on demand
	IN_Activate (sysActiveApp);
	CDAudio_Activate (sysActiveApp);
	SndImp_Activate (sysActiveApp);

	if (win_noalttab->value)
		WIN_ToggleAltTab (!sysActiveApp);

	GLimp_AppActivate (sysActiveApp); // FIXME was fActive, should it be this instead?
}


/*
====================
MainWndProc

Main window procedure
====================
*/
#ifndef WM_MOUSEWHEEL
# define WM_MOUSEWHEEL		(WM_MOUSELAST+1)	// message that will be supported by the OS
#endif // WM_MOUSEWHEEL

#ifndef WM_MOUSEHWHEEL
# define WM_MOUSEHWHEEL		0x020E
#endif // WM_MOUSEHWHEEL

#ifndef WM_XBUTTONDOWN
# define WM_XBUTTONDOWN		0x020B
# define WM_XBUTTONUP		0x020C
#endif // WM_XBUTTONDOWN

#ifndef MK_XBUTTON1
# define MK_XBUTTON1		0x0020
# define MK_XBUTTON2		0x0040
#endif // MK_XBUTTON1

LRESULT CALLBACK CDAudio_MessageHandler (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MainWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static UINT MSH_MOUSEWHEEL;
	int		state;
	int		width;
	int		height;

	if (uMsg == MSH_MOUSEWHEEL) {
		// for Win95
		if (((int) wParam) > 0) {
			Key_Event (K_MWHEELUP, qTrue, sysMsgTime);
			Key_Event (K_MWHEELUP, qFalse, sysMsgTime);
		}
		else {
			Key_Event (K_MWHEELDOWN, qTrue, sysMsgTime);
			Key_Event (K_MWHEELDOWN, qFalse, sysMsgTime);
		}
		goto end;
	}
	else switch (uMsg) {
		case WM_MOUSEWHEEL:
			// this chunk of code theoretically only works under NT4 and Win98
			// since this message doesn't exist under Win95
			if ((short)HIWORD (wParam) > 0) {
				Key_Event (K_MWHEELUP, qTrue, sysMsgTime);
				Key_Event (K_MWHEELUP, qFalse, sysMsgTime);
			}
			else {
				Key_Event (K_MWHEELDOWN, qTrue, sysMsgTime);
				Key_Event (K_MWHEELDOWN, qFalse, sysMsgTime);
			}
			goto end;

		case WM_MOUSEHWHEEL:
			// this chunk too kthxkthxkthx
			if ((short)HIWORD (wParam) > 0) {
				Key_Event (K_MWHEELRIGHT, qTrue, sysMsgTime);
				Key_Event (K_MWHEELRIGHT, qFalse, sysMsgTime);
			}
			else {
				Key_Event (K_MWHEELLEFT, qTrue, sysMsgTime);
				Key_Event (K_MWHEELLEFT, qFalse, sysMsgTime);
			}
			Com_Printf (0, "WM_MOUSE H WHEEL\n");
			goto end;

		case WM_HOTKEY:
			return 0;

		case WM_CREATE:
			globalhWnd = hWnd;
			MSH_MOUSEWHEEL = RegisterWindowMessage ("MSWHEEL_ROLLMSG");
			goto end;

		case WM_PAINT:
			// force entire screen to update next frame
			SCR_UpdateScreen ();
			goto end;

		case WM_DESTROY:
			// let sound and input know about this?
			globalhWnd = NULL;
			goto end;

		case WM_ACTIVATE:
			// KJB: Watch this for problems in fullscreen modes with Alt-tabbing.
			VID_AppActivate ((qBool)(LOWORD(wParam) != WA_INACTIVE), (qBool)HIWORD (wParam));
			goto end;

		case WM_MOVE:
			if (!glConfig.vidFullscreen) {
				int		xPos, yPos;
				RECT	r;
				int		style;

				xPos = (short) LOWORD (lParam);		// horizontal position
				yPos = (short) HIWORD (lParam);		// vertical position

				r.left		= 0;
				r.top		= 0;
				r.right		= 1;
				r.bottom	= 1;

				style = GetWindowLong (hWnd, GWL_STYLE);
				AdjustWindowRect (&r, style, FALSE);

				Cvar_VariableForceSetValue (vid_xpos, xPos + r.left);
				Cvar_VariableForceSetValue (vid_ypos, yPos + r.top);
				vid_xpos->modified = qFalse;
				vid_ypos->modified = qFalse;

				if (sysActiveApp)
					IN_Activate (qTrue);
			}
			goto end;

		// this is complicated because Win32 seems to pack multiple mouse events into
		// one update sometimes, so we always check all states and look for events
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MOUSEMOVE:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
			state = 0;

			if (wParam & MK_LBUTTON)	state |= 1;
			if (wParam & MK_RBUTTON)	state |= 2;
			if (wParam & MK_MBUTTON)	state |= 4;
			if (wParam & MK_XBUTTON1)	state |= 8;
			if (wParam & MK_XBUTTON2)	state |= 16;

			IN_MouseEvent (state);

			goto end;

		case WM_SYSCOMMAND:
			switch (wParam) {
			case SC_MONITORPOWER:
			case SC_SCREENSAVE:
				return 0;

			case SC_CLOSE:
				Cbuf_AddText ("quit");
				return 0;
			}
			goto end;

		case WM_SYSKEYDOWN:
			if (wParam == 13) {
				Cvar_VariableForceSetValue (vid_fullscreen, !vid_fullscreen->integer);
				VID_Restart_f ();
				return 0;
			}
		// fall through
		case WM_KEYDOWN:
			Key_Event (In_MapKey (wParam, lParam), qTrue, sysMsgTime);
			break;

		case WM_SYSKEYUP:
		case WM_KEYUP:
			Key_Event (In_MapKey (wParam, lParam), qFalse, sysMsgTime);
			break;

		case WM_CLOSE:
			Cbuf_AddText ("quit");
			return 0;

		case WM_SIZE:
			width = LOWORD (lParam);
			height = HIWORD (lParam);

			if ((width > 0) && (height > 0)) {
				vidDef.width  = width;
				vidDef.height = height;

				r_RefDef.width = width;
				r_RefDef.height = height;
			}
			goto end;

		case MM_MCINOTIFY:
			CDAudio_MessageHandler (hWnd, uMsg, wParam, lParam);
			goto end;
	}

	// pass all unhandled messages to DefWindowProc
	// return 0 if handled message, 1 if not
	if ((In_MapKey (wParam, lParam) != K_ALT) && (In_MapKey (wParam, lParam) != K_F10))
end:
		return DefWindowProc (hWnd, uMsg, wParam, lParam);

	return 0;
}

/*
==============================================================================

	WINDOW SETUP

==============================================================================
*/

/*
============
VID_UpdateWindowPosAndSize
============
*/
static void VID_UpdateWindowPosAndSize (void) {
	if (!vid_xpos->modified && !vid_ypos->modified)
		return;

	if (!glConfig.vidFullscreen) {
		RECT	r;
		int		style;
		int		w, h;

		r.left		= 0;
		r.top		= 0;
		r.right		= vidDef.width;
		r.bottom	= vidDef.height;

		style = GetWindowLong (globalhWnd, GWL_STYLE);
		AdjustWindowRect (&r, style, FALSE);

		w = r.right - r.left;
		h = r.bottom - r.top;

		MoveWindow (globalhWnd, vid_xpos->value, vid_ypos->value, w, h, qTrue);
	}

	vid_xpos->modified = qFalse;
	vid_ypos->modified = qFalse;
}


/*
============
VID_CheckChanges

This function gets called once just before drawing each frame, and it's sole purpose in life
is to check to see if any of the video mode parameters have changed, and if they have to 
update the video modes to match.
============
*/
void VID_CheckChanges (void) {
	static HWND		oldhWnd = 0;
	HICON			hIcon;

	if (win_noalttab->modified) {
		if (win_noalttab->integer)
			WIN_ToggleAltTab (qFalse);
		else
			WIN_ToggleAltTab (qTrue);

		win_noalttab->modified = qFalse;
	}

	while (vidRestart) {
		qBool cgWasActive = cls.moduleCGameActive;

		CL_ShutdownMedia ();

		// refresh has changed
		vidRestart = qFalse;
		cls.refreshPrepped = qFalse;
		cls.soundPrepped = qFalse;
		cls.pauseScreen = qTrue;

		// kill if already active
		VID_Shutdown (qFalse);

		if (!R_Init (globalhInst, MainWndProc)) {
			// refresh init failed!
			R_Shutdown (qTrue);
			vidActive = qFalse;

			Com_Error (ERR_FATAL, "Couldn't initialize OpenGL!");
		}

		Snd_Init ();
		CL_InitMedia ();

		// R1: Restart our input devices as the window handle most likely changed
		if (oldhWnd && globalhWnd != oldhWnd)
			IN_Restart_f ();

		// This works around alt-tab not tabbing back correctly
		hIcon = LoadIcon (globalhInst, (MAKEINTRESOURCE (IDI_ICON1)));
		SendMessage (globalhWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		SendMessage (globalhWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

		oldhWnd = globalhWnd;

		Con_Close ();

		// this is to stop cgame from initializing on first load
		if (cgWasActive) {
			CL_CGameAPI_Init ();
			Key_SetKeyDest (KD_GAME);
		}
		else {
			CL_UIModule_MainMenu ();
			Key_SetKeyDest (KD_MENU);
		}

		cls.soundPrepped = qTrue;
		vidActive = qTrue;
	}

	// update our window position
	VID_UpdateWindowPosAndSize ();
}

/*
==============================================================================

	INIT / SHUTDOWN

==============================================================================
*/

/*
============
VID_Init
============
*/
void VID_Init (void) {
	// create the video variables so we know how to start the graphics drivers
	vid_xpos		= Cvar_Get ("vid_xpos",			"3",	CVAR_ARCHIVE);
	vid_ypos		= Cvar_Get ("vid_ypos",			"22",	CVAR_ARCHIVE);
	vid_fullscreen	= Cvar_Get ("vid_fullscreen",	"0",	CVAR_ARCHIVE|CVAR_LATCH_VIDEO);

	win_noalttab	= Cvar_Get ("win_noalttab",		"0",	CVAR_ARCHIVE);

	// add some console commands that we want to handle
	Cmd_AddCommand ("vid_restart",	VID_Restart_f,		"Restarts refresh and media");
	Cmd_AddCommand ("vid_front",	VID_Front_f,		"");

	// disable the 3Dfx splash screen
	putenv ("FX_GLIDE_NO_SPLASH=0");

	// start the graphics mode
	vidRestart = qTrue;
	vidActive = qFalse;
	vid_fullscreen->modified = qTrue;
	VID_CheckChanges ();
}


/*
============
VID_Shutdown
============
*/
void VID_Shutdown (qBool full) {
	if (vidActive) {
		R_Shutdown (full);
		vidActive = qFalse;
	}
}
