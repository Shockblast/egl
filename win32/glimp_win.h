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
// glimp_win.h
//

#ifndef WIN32
#  error You should not be including this file on this platform
#endif // WIN32

#ifndef __GLIMP_WIN_H__
#define __GLIMP_WIN_H__

typedef struct glwState_s {
	qBool		active;

	qBool		miniDriver;
	qBool		bppChangeAllowed;

	HINSTANCE	hInstance;
	void		*WndProc;

	HDC			hDC;			// handle to device context
	HWND		hWnd;			// handle to window
	HGLRC		hGLRC;			// handle to GL rendering context

	HINSTANCE	hInstOpenGL;	// HINSTANCE for the OpenGL library

	DEVMODE		windowDM;
	DEVMODE		desktopDM;

	FILE		*oglLogFP;		// for gl_log logging
} glwState_t;

extern glwState_t	glwState;

#endif	// __GLIMP_WIN_H__
