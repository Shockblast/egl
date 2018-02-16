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
// win_local.h
// Win32-specific Quake header file
//

#pragma warning (disable : 4229)  // mgraph gets this

#include <windows.h>

#ifndef DEDICATED_ONLY
# include <mmreg.h>
# include <mmsystem.h>
# include <winuser.h>
# include <dsound.h>
# include <ctype.h>
# include <commctrl.h>
#endif

#define WINDOW_APP_NAME		"EGL v"EGL_VERSTR
#define WINDOW_CLASS_NAME	"EGL"

typedef struct winInfo_s {
	HINSTANCE	hInstance;
	HWND		hWnd;

	qBool		isWin32;

	qBool		appActive;
	qBool		appMinimized;

	uint32		msgTime;
} winInfo_t;

extern winInfo_t	sys_winInfo;

//
// win_console.c
//

void	Conbuf_AppendText (const char *pMsg);

//
// win_input.c
//

void	IN_Activate (qBool active);
void	IN_MouseEvent (int mstate);
