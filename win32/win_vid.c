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
// win_vid.c
//

#include "../client/cl_local.h"
#include "../renderer/r_local.h"
#include "win_local.h"
#include "win_glimp.h"
#include "resource.h"

// console variables that we need to access from this module
cVar_t		*vid_xpos;			// x coordinate of window position
cVar_t		*vid_ypos;			// y coordinate of window position
cVar_t		*vid_fullscreen;

cVar_t		*win_noalttab;

static qBool	sys_altTabDisabled;

static qBool	vid_isActive;
static qBool	vid_queueRestart;

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
static void VID_Restart_f (void)
{
	vid_queueRestart = qTrue;
}


/*
============
VID_Front_f
============
*/
static void VID_Front_f (void)
{
	SetWindowLongPtr (sys_winInfo.hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);
	SetForegroundWindow (sys_winInfo.hWnd);
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
static void WIN_ToggleAltTab (qBool enable)
{
	if (enable) {
		if (!sys_altTabDisabled)
			return;

		if (sys_winInfo.isWin32) {
			BOOL	old;
			SystemParametersInfo (SPI_SCREENSAVERRUNNING, 0, &old, 0);
		}
		else {
			UnregisterHotKey (0, 0);
			UnregisterHotKey (0, 1);
		}

		sys_altTabDisabled = qFalse;
	}
	else {
		if (sys_altTabDisabled)
			return;

		if (sys_winInfo.isWin32) {
			BOOL	old;
			SystemParametersInfo (SPI_SCREENSAVERRUNNING, 1, &old, 0);
		}
		else {
			RegisterHotKey (0, 0, MOD_ALT, VK_TAB);
			RegisterHotKey (0, 1, MOD_ALT, VK_RETURN);
		}

		sys_altTabDisabled = qTrue;
	}
}


/*
====================
VID_AppActivate
====================
*/
static void VID_AppActivate (qBool fActive, qBool minimize)
{
	sys_winInfo.appMinimized = minimize;

	Key_ClearStates ();

	// we don't want to act like we're active if we're minimized
	sys_winInfo.appActive = (fActive && !minimize) ? qTrue : qFalse;

	// minimize/restore mouse-capture on demand
	IN_Activate (sys_winInfo.appActive);
	CDAudio_Activate (sys_winInfo.appActive);
	SndImp_Activate (sys_winInfo.appActive);

	if (win_noalttab->intVal)
		WIN_ToggleAltTab (!sys_winInfo.appActive);

	GLimp_AppActivate (sys_winInfo.appActive); // FIXME was fActive, should it be this instead?
}


/*
============
VID_UpdateWindowPosAndSize
============
*/
static void VID_UpdateWindowPosAndSize (void)
{
	if (!vid_xpos->modified && !vid_ypos->modified)
		return;

	if (!ri.config.vidFullScreen) {
		RECT	rect;
		int		style;
		int		w, h;

		rect.left	= 0;
		rect.top	= 0;
		rect.right	= ri.config.vidWidth;
		rect.bottom	= ri.config.vidHeight;

		style = GetWindowLongPtr (sys_winInfo.hWnd, GWL_STYLE);
		AdjustWindowRect (&rect, style, FALSE);

		w = rect.right - rect.left;
		h = rect.bottom - rect.top;

		MoveWindow (sys_winInfo.hWnd, vid_xpos->intVal, vid_ypos->intVal, w, h, qTrue);

		ri.def.width = w;
		ri.def.height = h;
	}

	vid_xpos->modified = qFalse;
	vid_ypos->modified = qFalse;
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
LRESULT CALLBACK MainWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static UINT MSH_MOUSEWHEEL;
	int		state;
	int		width;
	int		height;

	if (uMsg == MSH_MOUSEWHEEL) {
		// for Win95
		if (((int) wParam) > 0) {
			Key_Event (K_MWHEELUP, qTrue, sys_winInfo.msgTime);
			Key_Event (K_MWHEELUP, qFalse, sys_winInfo.msgTime);
		}
		else {
			Key_Event (K_MWHEELDOWN, qTrue, sys_winInfo.msgTime);
			Key_Event (K_MWHEELDOWN, qFalse, sys_winInfo.msgTime);
		}
		goto end;
	}

	switch (uMsg) {
	case WM_MOUSEWHEEL:
		// this chunk of code theoretically only works under NT4 and Win98
		// since this message doesn't exist under Win95
		if ((int16)HIWORD (wParam) > 0) {
			Key_Event (K_MWHEELUP, qTrue, sys_winInfo.msgTime);
			Key_Event (K_MWHEELUP, qFalse, sys_winInfo.msgTime);
		}
		else {
			Key_Event (K_MWHEELDOWN, qTrue, sys_winInfo.msgTime);
			Key_Event (K_MWHEELDOWN, qFalse, sys_winInfo.msgTime);
		}
		goto end;

	case WM_MOUSEHWHEEL:
		if ((int16)HIWORD (wParam) > 0) {
			Key_Event (K_MWHEELRIGHT, qTrue, sys_winInfo.msgTime);
			Key_Event (K_MWHEELRIGHT, qFalse, sys_winInfo.msgTime);
		}
		else {
			Key_Event (K_MWHEELLEFT, qTrue, sys_winInfo.msgTime);
			Key_Event (K_MWHEELLEFT, qFalse, sys_winInfo.msgTime);
		}
		Com_Printf (0, "WM_MOUSE H WHEEL\n");
		goto end;

	case WM_HOTKEY:
		return 0;

	case WM_CREATE:
		MSH_MOUSEWHEEL = RegisterWindowMessage ("MSWHEEL_ROLLMSG");
		goto end;

	case WM_PAINT:
		// force entire screen to update next frame
		SCR_UpdateScreen ();
		goto end;

	case WM_DESTROY:
		// let sound and input know about this?
		sys_winInfo.hWnd = NULL;
		goto end;

	case WM_ACTIVATE:
		// KJB: Watch this for problems in fullscreen modes with Alt-tabbing.
		VID_AppActivate ((qBool)(LOWORD(wParam) != WA_INACTIVE), (qBool)HIWORD (wParam));
		goto end;

	case WM_MOVE:
		if (!ri.config.vidFullScreen) {
			int		xPos, yPos;
			RECT	r;
			int		style;

			xPos = (int16) LOWORD (lParam);		// horizontal position
			yPos = (int16) HIWORD (lParam);		// vertical position

			r.left		= 0;
			r.top		= 0;
			r.right		= 1;
			r.bottom	= 1;

			style = GetWindowLongPtr (hWnd, GWL_STYLE);
			AdjustWindowRect (&r, style, FALSE);

			Cvar_VariableSetValue (vid_xpos, xPos + r.left, qTrue);
			Cvar_VariableSetValue (vid_ypos, yPos + r.top, qTrue);
			vid_xpos->modified = qFalse;
			vid_ypos->modified = qFalse;

			if (sys_winInfo.appActive)
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
			Cbuf_AddText ("quit\n");
			return 0;
		}
		goto end;

	case WM_SYSKEYDOWN:
		if (wParam == 13) {
			Cvar_VariableSetValue (vid_fullscreen, !vid_fullscreen->intVal, qTrue);
			VID_Restart_f ();
			return 0;
		}
	// fall through
	case WM_KEYDOWN:
		Key_Event (In_MapKey (wParam, lParam), qTrue, sys_winInfo.msgTime);
		break;

	case WM_SYSKEYUP:
	case WM_KEYUP:
		Key_Event (In_MapKey (wParam, lParam), qFalse, sys_winInfo.msgTime);
		break;

	case WM_CLOSE:
		Cbuf_AddText ("quit\n");
		return 0;

	case WM_SIZE:
		width = LOWORD (lParam);
		height = HIWORD (lParam);

		if (width > 0 && height > 0) {
			// Force a window size update, so that the window dimensions cannot be float.
			ri.config.vidWidth  = width;
			ri.config.vidHeight = height;

			vid_xpos->modified = qTrue;
			vid_ypos->modified = qTrue;

			VID_UpdateWindowPosAndSize ();

			// Update the subsystems
			CL_SetRefConfig ();
		}
		goto end;

	case MM_MCINOTIFY:
		CDAudio_MessageHandler (hWnd, uMsg, wParam, lParam);
		goto end;
	}

	// FIXME: wtf?
	// - has something to do with window style and the minimize/maximize/exit
	// buttons I think, the key check is just fucking nasty though.

	// pass all unhandled messages to DefWindowProc
	// return 0 if handled message, 1 if not
	state = In_MapKey (wParam, lParam);
	if (state == K_ALT || state == K_F10)
		return 0;

end:
	return DefWindowProc (hWnd, uMsg, wParam, lParam);
}

/*
==============================================================================

	WINDOW SETUP

==============================================================================
*/

/*
============
VID_CheckChanges

This function gets called once just before drawing each frame, and it's sole purpose in life
is to check to see if any of the video mode parameters have changed, and if they have to 
update the video modes to match.
============
*/
void VID_CheckChanges (refConfig_t *outConfig)
{
	static HWND		oldhWnd = 0;
	int				errNum;

	if (win_noalttab->modified) {
		if (win_noalttab->intVal)
			WIN_ToggleAltTab (qFalse);
		else
			WIN_ToggleAltTab (qTrue);

		win_noalttab->modified = qFalse;
	}

	while (vid_queueRestart) {
		qBool cgWasActive = cls.mapLoaded;

		CL_MediaShutdown ();

		// Refresh has changed
		vid_queueRestart = qFalse;
		cls.refreshPrepped = qFalse;
		cls.disableScreen = qTrue;

		// Kill if already active
		if (vid_isActive) {
			R_Shutdown (qFalse);
			vid_isActive = qFalse;
		}

		// Initialize renderer
		if ((errNum = R_Init ()) != R_INIT_SUCCESS) {
			R_Shutdown (qTrue);
			vid_isActive = qFalse;

			switch (errNum) {
			case R_INIT_QGL_FAIL:
				Com_Error (ERR_FATAL, "Couldn't initialize OpenGL!\n" "QGL library failure!");
				break;

			case R_INIT_OS_FAIL:
				Com_Error (ERR_FATAL, "Couldn't initialize OpenGL!\n" "Incorrect operating system!");
				break;

			case R_INIT_MODE_FAIL:
				Com_Error (ERR_FATAL, "Couldn't initialize OpenGL!\n" "Couldn't set video mode!");
				break;
			}
		}

		CL_SetRefConfig ();
		R_GetRefConfig (outConfig);

		Snd_Init ();
		CL_MediaInit ();

		// R1: Restart our input devices as the window handle most likely changed
		if (oldhWnd && sys_winInfo.hWnd != oldhWnd)
			IN_Restart_f ();
		oldhWnd = sys_winInfo.hWnd;

		cls.disableScreen = qFalse;

		CL_ConsoleClose ();

		// This is to stop cgame from initializing on first load
		// and so it will load after a vid_restart while connected somewhere
		if (cgWasActive) {
			CL_CGModule_LoadMap ();
			Key_SetDest (KD_GAME);
		}
		else if (Com_ClientState() < CA_CONNECTED) {
			CL_CGModule_MainMenu ();
		}

		vid_isActive = qTrue;
	}

	// Update our window position
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
void VID_Init (refConfig_t *outConfig)
{
	// Create the video variables so we know how to start the graphics drivers
	vid_xpos		= Cvar_Register ("vid_xpos",			"3",	CVAR_ARCHIVE);
	vid_ypos		= Cvar_Register ("vid_ypos",			"22",	CVAR_ARCHIVE);
	vid_fullscreen	= Cvar_Register ("vid_fullscreen",		"0",	CVAR_ARCHIVE|CVAR_LATCH_VIDEO);

	win_noalttab	= Cvar_Register ("win_noalttab",		"0",	CVAR_ARCHIVE);

	// Add some console commands that we want to handle
	Cmd_AddCommand ("vid_restart",	VID_Restart_f,		"Restarts refresh and media");
	Cmd_AddCommand ("vid_front",	VID_Front_f,		"");

	// Disable the 3Dfx splash screen
	putenv ("FX_GLIDE_NO_SPLASH=0");

	// Start the graphics mode
	vid_queueRestart = qTrue;
	vid_isActive = qFalse;
	VID_CheckChanges (outConfig);
}


/*
============
VID_Shutdown
============
*/
void VID_Shutdown (void)
{
	if (vid_isActive) {
		R_Shutdown (qTrue);
		vid_isActive = qFalse;
	}
}
