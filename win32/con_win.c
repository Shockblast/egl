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
// TODO:
// - Color code compensation
// - Status bar for errors
// - Better buffer handling
// - up/down arrow in input scrolls through previous commands
//

#include "../common/common.h"
#include "winquake.h"
#include "resource.h"

static HWND		conhWnd;
static HDC		conhDC;

static HWND		hWndConOutput;
static HWND		hWndConInput;
static HWND		hWndConEnterBtn;
static HWND		hWndConExitBtn;

static HFONT	conWindowFont;

static WNDPROC	cmdLineProc;

#define			OUTPUT_BGCOLOR		0xEEEEEE
#define			OUTPUT_TEXTCOLOR	0x000000

#define CON_CLASSNAME	WINDOW_CLASS_NAME"DED"
#define CON_APPNAME		WINDOW_APP_NAME" - Dedicated console"

/*
==============================================================================

	MESSAGE HANDLING

==============================================================================
*/

/*
=================
ConWin_DoCommand
=================
*/
static void ConWin_DoCommand (void) {
	char	buf[1024];

	GetWindowText (hWndConInput, buf, sizeof(buf));

	// print the command in the console

	if (!Q_strlen (buf)) 
		return;

	Com_Printf (0, "]%s\n", buf);

	// add the text
	Cbuf_AddText (buf);
	Cbuf_AddText ("\n");
	
	// execute the command
	Cbuf_Execute ();

	// empty our textfield
	SetWindowText (hWndConInput, "");
}


/*
=================
ConWin_Print
=================
*/
static char *AddNewLinesToString (const char *string) {
	int			i;
	char		*output;
	static char	buffer[4096]; // increase this one ?
	
	// change the \n into \r\n so the newline thing works in our output field
	// TODO: strip color codes? make them work?
	output = buffer;
	for (i=0 ; i<Q_strlen(string) ; i++) {
		if (string[i] == '\n') {
			*output++ = '\r';
			*output++ = '\n';
		}
		else
			*output++ = string[i];
	}
	*output++ = 0;

	return buffer;
}
void ConWin_Print (const char *message) {
	// set textcolor
	SendMessage (hWndConOutput, EM_LINESCROLL, 0, OUTPUT_TEXTCOLOR);

	// set scrollposition
	SendMessage (hWndConOutput, EM_SCROLLCARET, 0, 0);

	// send the message
	// FIXME use an internal buffer, this will break over time otherwise
	SendMessage (hWndConOutput, EM_REPLACESEL, FALSE, (LPARAM)AddNewLinesToString (message));
}


/*
=================
ConsoleWndProc
=================
*/
static LRESULT CALLBACK ConsoleWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static HBRUSH	outputBGBrush;

	switch (uMsg) {
	case WM_CREATE:
		outputBGBrush = CreateSolidBrush (OUTPUT_BGCOLOR);
		break;

	case WM_ACTIVATE:
		if (LOWORD (wParam) != WA_INACTIVE) {
			// focus the input field
			SetFocus (hWndConInput);
		}
		break;

	case WM_CLOSE:
		Cbuf_AddText ("quit");
		return 0;

	case WM_COMMAND:
		switch (LOWORD (wParam)) {
			case IDC_ENTER:
				ConWin_DoCommand ();
				break;
			case IDC_EXIT:
				Cbuf_AddText ("quit");
				return 0;
		}
		break;

	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == hWndConOutput) {
			SetTextColor ((HDC)wParam, OUTPUT_TEXTCOLOR);	// set the textcolor
			SetBkColor ((HDC)wParam, OUTPUT_BGCOLOR);		// set the backgroundcolor
			return ((long) outputBGBrush);
		}
		break;
	}

	return DefWindowProc (hWnd, uMsg, wParam, lParam);
}


/*
=================
CmdLineWndProc
=================
*/
static LRESULT CALLBACK CmdLineWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_KILLFOCUS:
		if ((HWND)wParam == conhWnd) {
			SetFocus (hWnd);
			return 0;
		}
		break;

	case WM_CHAR:
		// we pressed enter ?
		if (wParam == '\r') {
			ConWin_DoCommand ();
			return 0;
		}
		break;
	}

	return CallWindowProc (cmdLineProc, hWnd, uMsg, wParam, lParam);
}

/*
==============================================================================

	ITEMS

==============================================================================
*/

/*
=================
ConWin_CreateItems
=================
*/
static qBool ConWin_CreateItems (int windowWidth, int windowHeight) {
	// create the main window
	conhDC = GetDC (conhWnd);

	// create the font
	conWindowFont = CreateFont (-MulDiv (8, GetDeviceCaps (conhDC, LOGPIXELSY), 72), 0, 0, 0, FW_LIGHT, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH|FF_MODERN, "Fixedsys");
	ReleaseDC (conhWnd, conhDC);

	// create the output field and set the font
	hWndConOutput = CreateWindowEx (
		WS_EX_CLIENTEDGE|WS_EX_TOPMOST,
		"edit", NULL,
		WS_CHILD|WS_VISIBLE|WS_BORDER|WS_VSCROLL|ES_READONLY|ES_AUTOVSCROLL|ES_MULTILINE|ES_LEFT,
		4, 4,
		windowWidth-14, windowHeight-56,
		conhWnd, NULL, globalhInst, NULL);
	if (!hWndConOutput)
		return qFalse;
	SendMessage (hWndConOutput, WM_SETFONT, (WPARAM)conWindowFont, FALSE);

	// create the input field and set the font
	hWndConInput = CreateWindowEx (
		WS_EX_CLIENTEDGE|WS_EX_TOPMOST,
		"edit", NULL,
		WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL|ES_LEFT|ES_WANTRETURN,
		4, windowHeight-50,
		windowWidth-(60+5)*2-12, 24,
		conhWnd, NULL, globalhInst, NULL);
	if (!hWndConInput)
		return qFalse;
	SendMessage (hWndConInput, WM_SETFONT, (WPARAM)conWindowFont, FALSE);

	// create the submit button
	hWndConEnterBtn = CreateWindowEx (
		WS_EX_TOPMOST,
		"button", NULL,
		WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
		windowWidth-(60+5)*2-4, windowHeight-(24)*2,
		60, 20,
		conhWnd, (HMENU)IDC_ENTER, globalhInst, NULL);
	if (!hWndConEnterBtn)
		return qFalse;
	SendMessage (hWndConEnterBtn, WM_SETTEXT, 0, (LPARAM)"Submit");

	// create the exit button
	hWndConExitBtn = CreateWindowEx (
		WS_EX_TOPMOST,
		"button", NULL,
		WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
		windowWidth-(60+5)-4, windowHeight-(24)*2,
		60, 20,
		conhWnd, (HMENU)IDC_EXIT, globalhInst, NULL);
	if (!hWndConExitBtn)
		return qFalse;
	SendMessage (hWndConExitBtn, WM_SETTEXT, 0, (LPARAM)"Exit");

	cmdLineProc = (WNDPROC)SetWindowLong (hWndConInput, GWL_WNDPROC, (long)CmdLineWndProc);

	// show the console
	ShowWindow (conhWnd, SW_SHOWNORMAL);
	UpdateWindow (conhWnd);
	SetForegroundWindow (conhWnd);

	// set focus
	SetFocus (hWndConInput);

	return qTrue;
}

/*
==============================================================================

	INIT / SHUTDOWN

==============================================================================
*/

/*
=================
ConWin_Init

Returns qFalse when it fails to initialize
=================
*/
qBool ConWin_Init (void) {
	WNDCLASS	wClass;
	HDC			dthDC;
	RECT		rect;
	DWORD		dwStyle;
	int			x, y;
	int			w, h;
	int			screenWidth;
	int			screenHeight;

	memset (&wClass, 0, sizeof (wClass));

	// register the frame class
	wClass.style			= CS_HREDRAW|CS_VREDRAW;
	wClass.lpfnWndProc		= (WNDPROC)ConsoleWndProc;
	wClass.cbClsExtra		= 0;
	wClass.cbWndExtra		= 0;
	wClass.hInstance		= globalhInst;
	wClass.hIcon			= LoadIcon (globalhInst, MAKEINTRESOURCE (IDI_ICON1));
	wClass.hCursor			= LoadCursor (NULL, IDC_ARROW);
	wClass.hbrBackground	= (HBRUSH)COLOR_WINDOW;
	wClass.lpszMenuName		= 0;
	wClass.lpszClassName	= CON_CLASSNAME;

	if (!RegisterClass (&wClass))
		return qFalse;

	// get desktop dimensions
	dthDC = GetDC (GetDesktopWindow ());
	screenWidth = GetDeviceCaps (dthDC, HORZRES);
	screenHeight = GetDeviceCaps (dthDC, VERTRES);
	ReleaseDC (GetDesktopWindow (), dthDC);

	// set the style
	dwStyle = WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_GROUP;

	// set the dimensions
	rect.left = rect.top = 0;
	rect.right = (700 > screenWidth) ? screenWidth - 50 : 700;
	rect.bottom = (500 > screenHeight) ? screenHeight - 100 : 500;

	AdjustWindowRect (&rect, dwStyle, FALSE);

	w = rect.right - rect.left;
	h = rect.bottom - rect.top;
    x = (screenWidth - w) / 2;
	y = (screenHeight - h) / 2;

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

	return ConWin_CreateItems (w, h);
}


/*
=================
ConWin_Shutdown
=================
*/
void ConWin_Shutdown (void) {
	if (conhWnd) {
		ShowWindow (conhWnd, SW_HIDE);
		CloseWindow (conhWnd);
		DestroyWindow (conhWnd);
		conhWnd = NULL;
	}

	UnregisterClass (CON_CLASSNAME, globalhInst);
}
