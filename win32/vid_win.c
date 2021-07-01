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

#include "../client/cl_local.h"
#include "../renderer/r_local.h"
#include "winquake.h"
#include "glimp_win.h"

// console variables that we need to access from this module
cvar_t		*vid_xpos;			// x coordinate of window position
cvar_t		*vid_ypos;			// y coordinate of window position
cvar_t		*vid_maximized;
cvar_t		*vid_fullscreen;

cvar_t		*win_noalttab;

HWND		g_hWnd;				// main window handle for life of program

static	qBool		sys_Active;
static	qBool		sys_AltTabDisabled;

qBool				vid_Restart;

/*
=================
WIN_ToggleAltTab
=================
*/
static void WIN_ToggleAltTab (qBool enable) {
	if (enable) {
		if (!sys_AltTabDisabled)
			return;

		if (sys_Windows) {
			BOOL	old;
			SystemParametersInfo (SPI_SCREENSAVERRUNNING, 0, &old, 0);
		} else {
			UnregisterHotKey (0, 0);
			UnregisterHotKey (0, 1);
		}

		sys_AltTabDisabled = qFalse;
	} else {
		if (sys_AltTabDisabled)
			return;

		if (sys_Windows) {
			BOOL	old;
			SystemParametersInfo (SPI_SCREENSAVERRUNNING, 1, &old, 0);
		} else {
			RegisterHotKey (0, 0, MOD_ALT, VK_TAB);
			RegisterHotKey (0, 1, MOD_ALT, VK_RETURN);
		}

		sys_AltTabDisabled = qTrue;
	}
}


/*
====================
VID_AppActivate
====================
*/
static void VID_AppActivate (qBool fActive, qBool minimize) {
	sys_Minimized = minimize;

	Key_ClearStates ();

	// we don't want to act like we're active if we're minimized
	sys_ActiveApp = (fActive && !minimize) ? qTrue : qFalse;

	// minimize/restore mouse-capture on demand
	IN_Activate (sys_ActiveApp);
	CDAudio_Activate (sys_ActiveApp);
	SNDDMA_Activate (sys_ActiveApp);

	if (win_noalttab->value)
		WIN_ToggleAltTab (!sys_ActiveApp);

	GLimp_AppActivate (fActive);
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

	if (uMsg == MSH_MOUSEWHEEL) {
		// for Win95
		if (((int) wParam) > 0) {
			Key_Event (K_MWHEELUP, qTrue, sys_MsgTime);
			Key_Event (K_MWHEELUP, qFalse, sys_MsgTime);
		} else {
			Key_Event (K_MWHEELDOWN, qTrue, sys_MsgTime);
			Key_Event (K_MWHEELDOWN, qFalse, sys_MsgTime);
		}
		goto end;
	} else switch (uMsg) {
		case WM_MOUSEWHEEL:
			// this chunk of code theoretically only works under NT4 and Win98
			// since this message doesn't exist under Win95
			if ((short)HIWORD (wParam) > 0) {
				Key_Event (K_MWHEELUP, qTrue, sys_MsgTime);
				Key_Event (K_MWHEELUP, qFalse, sys_MsgTime);
			} else {
				Key_Event (K_MWHEELDOWN, qTrue, sys_MsgTime);
				Key_Event (K_MWHEELDOWN, qFalse, sys_MsgTime);
			}
			goto end;

		case WM_MOUSEHWHEEL:
			// this chunk too kthxkthxkthx
			if ((short)HIWORD (wParam) > 0) {
				Key_Event (K_MWHEELRIGHT, qTrue, sys_MsgTime);
				Key_Event (K_MWHEELRIGHT, qFalse, sys_MsgTime);
			} else {
				Key_Event (K_MWHEELLEFT, qTrue, sys_MsgTime);
				Key_Event (K_MWHEELLEFT, qFalse, sys_MsgTime);
			}
			Com_Printf (PRINT_ALL, "WM_MOUSE H WHEEL\n");
			goto end;

		case WM_HOTKEY:
			return 0;

		case WM_CREATE:
			g_hWnd = hWnd;
			MSH_MOUSEWHEEL = RegisterWindowMessage ("MSWHEEL_ROLLMSG");
			goto end;

		case WM_PAINT:
			// force entire screen to update next frame
			SCR_UpdateScreen ();
			goto end;

		case WM_DESTROY:
			// let sound and input know about this?
			g_hWnd = NULL;
			goto end;

		case WM_ACTIVATE:
			// KJB: Watch this for problems in fullscreen modes with Alt-tabbing.
			VID_AppActivate (LOWORD(wParam) != WA_INACTIVE, (BOOL) HIWORD(wParam));
			goto end;

		case WM_MOVE:
			if (sys_Active && hWnd && !glConfig.vidFullscreen) {
				int		xPos, yPos;
				RECT	r;
				int		style;

				xPos = (short) LOWORD (lParam);		// horizontal position
				yPos = (short) HIWORD (lParam);		// vertical position

				r.left = r.top = 0;
				r.right = r.bottom	= 1;

				style = GetWindowLong (hWnd, GWL_STYLE);
				AdjustWindowRect (&r, style, FALSE);

				Cvar_ForceSetValue ("vid_xpos", xPos + r.left);
				Cvar_ForceSetValue ("vid_ypos", yPos + r.top);
				vid_xpos->modified = qFalse;
				vid_ypos->modified = qFalse;

				if (sys_ActiveApp)
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
		case WM_XBUTTONUP: {
				int		state = 0;

				if (wParam & MK_LBUTTON)	state |= 1;
				if (wParam & MK_RBUTTON)	state |= 2;
				if (wParam & MK_MBUTTON)	state |= 4;
				if (wParam & MK_XBUTTON1)	state |= 8;
				if (wParam & MK_XBUTTON2)	state |= 16;

				IN_MouseEvent (state);
			}
			goto end;

		case WM_SYSCOMMAND:
			switch (wParam) {
			case SC_MONITORPOWER:
			case SC_SCREENSAVE:
				return 0;

			case SC_CLOSE:
				Cbuf_ExecuteText (EXEC_NOW, "quit");
				return 0;
			}
			goto end;

		case WM_SYSKEYDOWN:
			if (wParam == 13) {
				Cvar_ForceSetValue ("vid_fullscreen", !vid_fullscreen->integer);
				VID_Restart_f ();
				return 0;
			}
		// fall through
		case WM_KEYDOWN:
			Key_Event (Key_MapKey (lParam), qTrue, sys_MsgTime);
			break;

		case WM_SYSKEYUP:
		case WM_KEYUP:
			Key_Event (Key_MapKey (lParam), qFalse, sys_MsgTime);
			break;

		case WM_CLOSE:
			Cbuf_ExecuteText (EXEC_NOW, "quit");
			return 0;

		case WM_SIZE: {
				int width = LOWORD (lParam);
				int height = HIWORD (lParam);

				if ((width > 0) && (height > 0)) {
					vidDef.width  = width;
					vidDef.height = height;

					cl.refDef.width = width;
					cl.refDef.height = height;

					r_RefDef.width = width;
					r_RefDef.height = height;
				}
			}
			goto end;

		case MM_MCINOTIFY:
			CDAudio_MessageHandler (hWnd, uMsg, wParam, lParam);
			goto end;
	}

	// pass all unhandled messages to DefWindowProc
	// return 0 if handled message, 1 if not
	if ((Key_MapKey (lParam) != K_ALT) && (Key_MapKey (lParam) != K_F10))
end:
		return DefWindowProc (hWnd, uMsg, wParam, lParam);

	return 0;
}


/*
============
VID_NewWindow
============
*/
void VID_NewWindow (void) {
	cl.forceRefresh = qTrue;		// can't use a paused refdef
}


/*
============
VID_Restart_f
============
*/
void VID_Restart_f (void) {
	vid_Restart = qTrue;
}


/*
============
VID_Front_f
============
*/
void VID_Front_f (void) {
	SetWindowLong (g_hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);
	SetForegroundWindow (g_hWnd);
}


/*
============
VID_UpdateWindowPosAndSize
============
*/
void VID_UpdateWindowPosAndSize (void) {
	if (!vid_xpos->modified && !vid_ypos->modified)
		return;

	if (sys_Active && g_hWnd && !glConfig.vidFullscreen) {
		RECT	r;
		int		style;
		int		w, h;

		r.left		= 0;
		r.top		= 0;
		r.right		= vidDef.width;
		r.bottom	= vidDef.height;

		style = GetWindowLong (g_hWnd, GWL_STYLE);
		AdjustWindowRect (&r, style, FALSE);

		w = r.right - r.left;
		h = r.bottom - r.top;

		MoveWindow (g_hWnd, vid_xpos->value, vid_ypos->value, w, h, qTrue);
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
	if (win_noalttab->modified) {
		if (win_noalttab->integer)
			WIN_ToggleAltTab (qFalse);
		else
			WIN_ToggleAltTab (qTrue);

		win_noalttab->modified = qFalse;
	}

	while (vid_Restart) {
		// can't use a paused refdef
		cl.forceRefresh = qTrue;

		Snd_StopAllSounds ();

		// clear decals to stop problems with nodes
		CL_ClearDecals ();

		// restart the ui so the vid dimensions are updated
		ui_Restart = qTrue;

		// refresh has changed
		vid_Restart = qFalse;
		vid_fullscreen->modified = qTrue;
		cl.refreshPrepped = qFalse;
		cls.disableScreen = qTrue;

		// kill if already active
		VID_Shutdown ();

		if (!R_Init (g_hInst, MainWndProc)) {
			// refresh init failed!
			R_Shutdown ();
			sys_Active = qFalse;

			Com_Error (ERR_FATAL, "Couldn't initialize OpenGL!");
		}

		sys_Active = qTrue;
		cls.disableScreen = qFalse;
	}

	// update our window position
	VID_UpdateWindowPosAndSize ();
}


/*
============
VID_Init
============
*/
void VID_Init (void) {
	// create the video variables so we know how to start the graphics drivers
	vid_xpos		= Cvar_Get ("vid_xpos",			"3",	CVAR_ARCHIVE);
	vid_ypos		= Cvar_Get ("vid_ypos",			"22",	CVAR_ARCHIVE);
	vid_maximized	= Cvar_Get ("vid_maximized",	"0",	CVAR_ARCHIVE);
	vid_fullscreen	= Cvar_Get ("vid_fullscreen",	"0",	CVAR_ARCHIVE|CVAR_LATCHVIDEO);

	win_noalttab	= Cvar_Get ("win_noalttab",		"0",	CVAR_ARCHIVE);

	// add some console commands that we want to handle
	Cmd_AddCommand ("vid_restart",	VID_Restart_f);
	Cmd_AddCommand ("vid_front",	VID_Front_f);

	// disable the 3Dfx splash screen
	putenv ("FX_GLIDE_NO_SPLASH=0");
		
	// start the graphics mode
	vid_Restart = qTrue;
	sys_Active = qFalse;
	VID_CheckChanges ();
}


/*
============
VID_Shutdown
============
*/
void VID_Shutdown (void) {
	if (sys_Active) {
		R_Shutdown ();
		sys_Active = qFalse;
	}
}
