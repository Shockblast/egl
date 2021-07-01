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
// con_win.c
//

#include "../common/common.h"
#include "winquake.h"
#include "resource.h"
#include <commctrl.h>

static HWND		conhWnd;

static HWND		conEditBarhWnd;
static HWND		contStatushWnd;

#define CON_CLASSNAME	WINDOW_CLASS_NAME"DED"
#define CON_APPNAME		WINDOW_APP_NAME" Dedicated console"

/*
==============================================================================

	RENDERING

==============================================================================
*/

/*
=================
SysConsole_Update
=================
*/
void SysConsole_Update (void) {
	RECT		rect;

	// get window rect
	GetClientRect (conhWnd, &rect);

	// edit bar
	conEditBarhWnd = CreateWindow (
		"Field", "Edit", WS_CHILD|WS_BORDER|WS_VISIBLE|ES_AUTOVSCROLL|ES_WANTRETURN,
		rect.bottom - 20, rect.left, rect.right, 20,
		conhWnd, (HMENU)NULL, globalhInst, NULL);

	SetWindowLong (conEditBarhWnd, GWL_ID, IDC_CON_EDITLINE);

	// status bar
	contStatushWnd = CreateWindow (
		STATUSCLASSNAME, "", WS_CHILD|WS_VISIBLE,
		0, 0, rect.right-rect.left, 0,
		conhWnd, (HMENU)NULL, globalhInst, NULL);
}

/*
==============================================================================

	MESSAGE HANDLING

==============================================================================
*/

LRESULT CALLBACK ConsoleWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_HOTKEY:
		return 0;

	case WM_SYSCOMMAND:
		switch (wParam) {
		case SC_MONITORPOWER:
		case SC_SCREENSAVE:
			return 0;

		case SC_CLOSE:
			Cbuf_AddText ("quit");
			return 0;
		}
		break;

	case WM_CREATE:
		globalhWnd = hWnd;
		break;

	case WM_PAINT:
		break;

	case WM_DESTROY:
		globalhWnd = NULL;
		break;
	}

	return DefWindowProc (hWnd, uMsg, wParam, lParam);
}

/*
==============================================================================

	INIT / SHUTDOWN

==============================================================================
*/

/*
=================
SysConsole_Init

Returns qFalse when it fails to initialize
=================
*/
qBool SysConsole_Init (void) {
	WNDCLASS	wClass;
	RECT		rect;
	DWORD		dwStyle;
	int			x, y;
	int			w, h;

	return qFalse; // orly

	memset (&wClass, 0, sizeof (wClass));

	// register the frame class
	wClass.style			= CS_HREDRAW|CS_VREDRAW;
	wClass.lpfnWndProc		= (WNDPROC)ConsoleWndProc;
	wClass.cbClsExtra		= 0;
	wClass.cbWndExtra		= 0;
	wClass.hInstance		= globalhInst;
	wClass.hIcon			= LoadIcon (globalhInst, MAKEINTRESOURCE (IDI_ICON1));
	wClass.hCursor			= LoadCursor (NULL, IDC_ARROW);
	wClass.hbrBackground	= (HBRUSH)GetStockObject(GRAY_BRUSH);
	wClass.lpszMenuName		= 0;
	wClass.lpszClassName	= CON_CLASSNAME;

	if (!RegisterClass (&wClass))
		return qFalse;

	dwStyle = WS_GROUP|WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_DLGFRAME;

	rect.left = rect.top = 0;
	rect.right = 600;
	rect.bottom = 450;

	AdjustWindowRect (&rect, dwStyle, FALSE);

	x = 50;
	y = 50;
	w = rect.right - rect.left;
	h = rect.bottom - rect.top;

	conhWnd = CreateWindow (
		 CON_CLASSNAME,			// class name
		 CON_APPNAME,			// app name
		 dwStyle | WS_VISIBLE,	// windows style
		 x, y,					// x y pos
		 w, h,					// width height
		 NULL,					// handle to parent
		 NULL,					// handle to menu
		 globalhInst,			// app instance
		 NULL);					// no extra params

	if (!conhWnd) {
		UnregisterClass (CON_CLASSNAME, globalhInst);
		return qFalse;
	}

	SysConsole_Update ();

	// show and update
	ShowWindow (conhWnd, SW_SHOWNORMAL);
	UpdateWindow (conhWnd);

	SetForegroundWindow (conhWnd);
	SetFocus (conhWnd);

	return qTrue;
}


/*
=================
SysConsole_Shutdown
=================
*/
void SysConsole_Shutdown (void) {
	if (conhWnd) {
		ShowWindow (conhWnd, SW_HIDE);
		DestroyWindow (conhWnd);
		conhWnd = NULL;
	}

	UnregisterClass (CON_CLASSNAME, globalhInst);
}
