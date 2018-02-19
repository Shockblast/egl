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
// win_glimp.c
// This file contains ALL Win32 specific stuff having to do with the OpenGL refresh
//

#include "../renderer/r_local.h"
#include "win_local.h"
#include "win_glimp.h"
#include "resource.h"
#include "wglext.h"

glwState_t	glwState;

/*
=============================================================================

	FRAME SETUP

=============================================================================
*/

/*
=================
GLimp_BeginFrame
=================
*/
void GLimp_BeginFrame (void)
{
	if (gl_bitdepth->modified) {
		if (gl_bitdepth->intVal > 0 && !glwState.bppChangeAllowed) {
			ri.config.vidBitDepth = glwState.desktopBPP;

			Cvar_VariableSetValue (gl_bitdepth, 0, qTrue);
			Com_Printf (PRNT_WARNING, "gl_bitdepth requires Win95 OSR2.x or WinNT 4.x -- forcing to 0\n");
		}

		gl_bitdepth->modified = qFalse;
	}

	if (ri.cameraSeparation < 0)
		qglDrawBuffer (GL_BACK_LEFT);
	else if (ri.cameraSeparation > 0)
		qglDrawBuffer (GL_BACK_RIGHT);
	else
		qglDrawBuffer (GL_BACK);
}


/*
=================
GLimp_EndFrame

Responsible for doing a swapbuffers and possibly for other stuff as yet to be determined.
Probably better not to make this a GLimp function and instead do a call to GLimp_SwapBuffers.

Only error check if active, and don't swap if not active an you're in fullscreen
=================
*/
void GLimp_EndFrame (void)
{
	// Update the swap interval
	if (r_swapInterval->modified) {
		r_swapInterval->modified = qFalse;
		if (!ri.config.stereoEnabled && ri.config.extWinSwapInterval)
			qwglSwapIntervalEXT (r_swapInterval->intVal);
	}

	// Check for errors
	if (glwState.active) {
		if (gl_errorcheck->intVal)
			GL_CheckForError ("GLimp_EndFrame");
	}
	else if (ri.config.vidFullScreen) {
		Sleep (0);
		return;
	}

	// Swap buffers
	if (stricmp (gl_drawbuffer->string, "GL_BACK") == 0) {
		SwapBuffers (glwState.hDC);
	}

	// Conserve CPU
	Sleep (0);
}

/*
=============================================================================

	MISC

=============================================================================
*/

/*
=================
GLimp_AppActivate
=================
*/
void GLimp_AppActivate (qBool active)
{
	glwState.active = active;

	if (active) {
		SetForegroundWindow (sys_winInfo.hWnd);
		ShowWindow (sys_winInfo.hWnd, SW_SHOW);

		if (ri.config.vidFullScreen)
			ChangeDisplaySettings (&glwState.windowDM, CDS_FULLSCREEN);
	}
	else {
		if (ri.config.vidFullScreen) {
			ShowWindow (sys_winInfo.hWnd, SW_MINIMIZE);
			ChangeDisplaySettings (NULL, 0);
		}
	}
}


/*
=================
GLimp_GetGammaRamp
=================
*/
qBool GLimp_GetGammaRamp (uint16 *ramp)
{
	if (qwglGetDeviceGammaRamp3DFX) {
		if (qwglGetDeviceGammaRamp3DFX (glwState.hDC, ramp))
			return qTrue;
	}

	if (GetDeviceGammaRamp (glwState.hDC, ramp))
		return qTrue;

	return qFalse;
}


/*
=================
GLimp_SetGammaRamp
=================
*/
void GLimp_SetGammaRamp (uint16 *ramp)
{
	if (qwglSetDeviceGammaRamp3DFX)
		qwglSetDeviceGammaRamp3DFX (glwState.hDC, ramp);
	else
		SetDeviceGammaRamp (glwState.hDC, ramp);
}

/*
=============================================================================

	INIT / SHUTDOWN

=============================================================================
*/

/*
=================
GLimp_SetupPFD
=================
*/
static int GLimp_SetupPFD (PIXELFORMATDESCRIPTOR *pfd, int colorBits, int depthBits, int alphaBits, int stencilBits)
{
	int		iPixelFormat;

	// Fill out the PFD
	pfd->nSize			= sizeof (PIXELFORMATDESCRIPTOR);
	pfd->nVersion		= 1;
	pfd->dwFlags		= PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd->iPixelType		= PFD_TYPE_RGBA;

	pfd->cColorBits		= colorBits;
	pfd->cRedBits		= 0;
	pfd->cRedShift		= 0;
	pfd->cGreenBits		= 0;
	pfd->cGreenShift	= 0;
	pfd->cBlueBits		= 0;
	pfd->cBlueShift		= 0;

	pfd->cAlphaBits		= alphaBits;
	pfd->cAlphaShift	= 0;

	pfd->cAccumBits		= 0;
	pfd->cAccumRedBits	= 0;
	pfd->cAccumGreenBits= 0;
	pfd->cAccumBlueBits	= 0;
	pfd->cAccumAlphaBits= 0;

	pfd->cDepthBits		= depthBits;
	pfd->cStencilBits	= stencilBits;

	pfd->cAuxBuffers	= 0;
	pfd->iLayerType		= PFD_MAIN_PLANE;
	pfd->bReserved		= 0;

	pfd->dwLayerMask	= 0;
	pfd->dwVisibleMask	= 0;
	pfd->dwDamageMask	= 0;

	Com_Printf (0, "...PFD(c%d a%d z%d s%d):\n",
		colorBits, alphaBits, depthBits, stencilBits);

	// Set PFD_STEREO if necessary
	if (cl_stereo->intVal) {
		Com_Printf (0, "...attempting to use stereo pfd\n");
		pfd->dwFlags |= PFD_STEREO;
		ri.config.stereoEnabled = qTrue;
	}
	else {
		Com_Printf (0, "...not attempting to use stereo pfd\n");
		ri.config.stereoEnabled = qFalse;
	}

	// Choose a pixel format
	iPixelFormat = ChoosePixelFormat (glwState.hDC, pfd);
	if (!iPixelFormat) {
		Com_Printf (PRNT_ERROR, "...ChoosePixelFormat failed\n");
		return 0;
	}
	else {
		Com_Printf (0, "...ChoosePixelFormat succeeded\n");
	}

	if (SetPixelFormat (glwState.hDC, iPixelFormat, pfd) == qFalse) {
		Com_Printf (PRNT_ERROR, "...SetPixelFormat failed\n");
		return 0;
	}
	else {
		Com_Printf (0, "...SetPixelFormat succeeded\n");
	}

	DescribePixelFormat (glwState.hDC, iPixelFormat, sizeof (PIXELFORMATDESCRIPTOR), pfd);

	// Check for hardware acceleration
	if (pfd->dwFlags & PFD_GENERIC_FORMAT) {
		if (!gl_allow_software->intVal) {
			Com_Printf (PRNT_ERROR, "...no hardware acceleration detected\n");
			return 0;
		}
		else {
			Com_Printf (0, "...using software emulation\n");
		}
	}
	else if (pfd->dwFlags & PFD_GENERIC_ACCELERATED) {
		Com_Printf (0, "...MCD acceleration found\n");
	}
	else {
		Com_Printf (0, "...hardware acceleration detected\n");
	}

	return iPixelFormat;
}


/*
=================
GLimp_GLInit
=================
*/
static qBool GLimp_GLInit (void)
{
	PIXELFORMATDESCRIPTOR	iPFD;
	int		iPixelFormat;
	int		alphaBits, colorBits, depthBits, stencilBits;

	Com_Printf (0, "OpenGL init\n");

	// This shouldn't happen
	if (glwState.hDC) {
		Com_Printf (PRNT_ERROR, "...non-NULL hDC exists!\n");
		if (ReleaseDC (sys_winInfo.hWnd, glwState.hDC))
			Com_Printf (0, "...hDC release succeeded\n");
		else
			Com_Printf (PRNT_ERROR, "...hDC release failed\n");
	}

	// Get a DC for the specified window
	if ((glwState.hDC = GetDC (sys_winInfo.hWnd)) == NULL) {
		Com_Printf (PRNT_ERROR, "...GetDC failed\n");
		return qFalse;
	}
	else {
		Com_Printf (0, "...GetDC succeeded\n");
	}

	// Alpha bits
	alphaBits = r_alphabits->intVal;

	// Color bits
	colorBits = r_colorbits->intVal;
	if (colorBits == 0)
		colorBits = glwState.desktopBPP;

	// Depth bits
	if (r_depthbits->intVal == 0) {
		if (colorBits > 16)
			depthBits = 24;
		else
			depthBits = 16;
	}
	else
		depthBits = r_depthbits->intVal;

	// Stencil bits
	stencilBits = r_stencilbits->intVal;
	if (!gl_stencilbuffer->intVal)
		stencilBits = 0;
	else if (depthBits < 24)
		stencilBits = 0;

	// Setup the PFD
	iPixelFormat = GLimp_SetupPFD (&iPFD, colorBits, depthBits, alphaBits, stencilBits);
	if (!iPixelFormat) {
		// Don't needlessly try again
		if (colorBits == glwState.desktopBPP && alphaBits == 0 && stencilBits == 0) {
			Com_Printf (PRNT_ERROR, "...failed to find a decent pixel format\n");
			return qFalse;
		}

		// Attempt two, no alpha and no stencil bits
		Com_Printf (PRNT_ERROR, "...first attempt failed, trying again\n");

		colorBits = glwState.desktopBPP;
		if (r_depthbits->intVal == 0) {
			if (colorBits > 16)
				depthBits = 24;
			else
				depthBits = 16;
		}
		else
			depthBits = r_depthbits->intVal;
		alphaBits = 0;
		stencilBits = 0;

		iPixelFormat = GLimp_SetupPFD (&iPFD, colorBits, depthBits, alphaBits, stencilBits);
		if (!iPixelFormat) {
			Com_Printf (PRNT_ERROR, "...failed to find a decent pixel format\n");
			return qFalse;
		}
	}

	// Report if stereo is desired but unavailable
	if (cl_stereo->intVal) {
		if (iPFD.dwFlags & PFD_STEREO) {
			Com_Printf (0, "...stereo pfd succeeded\n");
		}
		else {
			Com_Printf (PRNT_ERROR, "...stereo pfd failed\n");
			Cvar_VariableSetValue (cl_stereo, 0, qTrue);
			ri.config.stereoEnabled = qFalse;
		}
	}

	// Startup the OpenGL subsystem by creating a context
	if ((glwState.hGLRC = qwglCreateContext (glwState.hDC)) == 0) {
		Com_Printf (PRNT_ERROR, "...qwglCreateContext failed\n");
		return qFalse;
	}
	else {
		Com_Printf (0, "...qwglCreateContext succeeded\n");
	}

	// Make the new context current
	if (!qwglMakeCurrent (glwState.hDC, glwState.hGLRC)) {
		Com_Printf (PRNT_ERROR, "...qwglMakeCurrent failed\n");
		return qFalse;
	}
	else {
		Com_Printf (0, "...qwglMakeCurrent succeeded\n");
	}

	ri.cColorBits = iPFD.cColorBits;
	ri.cAlphaBits = iPFD.cAlphaBits;
	ri.cDepthBits = iPFD.cDepthBits;
	ri.cStencilBits = iPFD.cStencilBits;

	// Print out PFD specifics
	Com_Printf (0, "----------------------------------------\n");
	Com_Printf (0, "GL_PFD #%i: c(%d-bits) a(%d-bits) z(%d-bit) s(%d-bit)\n",
				iPixelFormat,
				ri.cColorBits, ri.cAlphaBits, ri.cDepthBits, ri.cStencilBits);

	return qTrue;
}


/*
=================
GLimp_CreateWindow
=================
*/
LRESULT CALLBACK MainWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static qBool GLimp_CreateWindow (qBool fullScreen, int width, int height)
{
	RECT		rect;
	DWORD		exStyle, dwStyle;
	int			w, h;
	int			x, y;

	// Register the window class if needed
	if (!glwState.classRegistered) {
		WNDCLASS		wClass;

		memset (&wClass, 0, sizeof (wClass));

		wClass.style			= CS_HREDRAW|CS_VREDRAW;
		wClass.lpfnWndProc		= (WNDPROC)MainWndProc;
		wClass.cbClsExtra		= 0;
		wClass.cbWndExtra		= 0;
		wClass.hInstance		= sys_winInfo.hInstance;
		wClass.hIcon			= LoadIcon (sys_winInfo.hInstance, MAKEINTRESOURCE (IDI_ICON1));
		wClass.hCursor			= LoadCursor (NULL, IDC_ARROW);
		wClass.hbrBackground	= (HBRUSH)GetStockObject(WHITE_BRUSH);
		wClass.lpszMenuName		= 0;
		wClass.lpszClassName	= WINDOW_CLASS_NAME;

		if (!RegisterClass (&wClass)) {
			Com_Error (ERR_FATAL, "Couldn't register window class");
			return qFalse;
		}
		Com_Printf (0, "...RegisterClass succeeded\n");
		glwState.classRegistered = qTrue;
	}

	// Adjust the window
	rect.left = 0;
	rect.top = 0;
	rect.right = width;
	rect.bottom = height;

	if (fullScreen) {
		exStyle = WS_EX_TOPMOST;
		dwStyle = WS_POPUP|WS_SYSMENU;
	}
	else {
		exStyle = 0;
		dwStyle = WS_TILEDWINDOW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS;

		AdjustWindowRect (&rect, dwStyle, FALSE);
	}

	w = rect.right - rect.left;
	h = rect.bottom - rect.top;

	if (fullScreen) {
		x = 0;
		y = 0;
	}
	else {
		x = vid_xpos->intVal;
		y = vid_ypos->intVal;

		// Clamp
		if (x < 0)
			x = 0;
		if (y < 0)
			y = 0;
		if (w < glwState.desktopWidth && h < glwState.desktopHeight) {
			if (x + w > glwState.desktopWidth)
				x = glwState.desktopWidth - w;
			if (y + h > glwState.desktopHeight)
				y = glwState.desktopHeight - h;
		}
	}

	// Create the window
	sys_winInfo.hWnd = CreateWindowEx (
		exStyle,
		WINDOW_CLASS_NAME,		// class name
		WINDOW_APP_NAME,		// app name
		dwStyle | WS_VISIBLE,	// windows style
		x, y,					// x y pos
		w, h,					// width height
		NULL,					// handle to parent
		NULL,					// handle to menu
		sys_winInfo.hInstance,	// app instance
		NULL);					// no extra params

	if (!sys_winInfo.hWnd) {
		char *buf = NULL;

		UnregisterClass (WINDOW_CLASS_NAME, sys_winInfo.hInstance);
		FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
						NULL,
						GetLastError (),
						MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
						(LPTSTR) &buf,
						0,
						NULL);
		Com_Error (ERR_FATAL, "Couldn't create window\nGetLastError: %s", buf);
		return qFalse;
	}
	Com_Printf (0, "...CreateWindow succeeded\n");

	// Show the window
	ShowWindow (sys_winInfo.hWnd, SW_SHOWNORMAL);
	UpdateWindow (sys_winInfo.hWnd);

	// Init all the OpenGL stuff for the window
	if (!GLimp_GLInit ()) {
		if (glwState.hGLRC) {
			qwglDeleteContext (glwState.hGLRC);
			glwState.hGLRC = NULL;
		}

		if (glwState.hDC) {
			ReleaseDC (sys_winInfo.hWnd, glwState.hDC);
			glwState.hDC = NULL;
		}

		Com_Printf (PRNT_ERROR, "OpenGL initialization failed!\n");
		return qFalse;
	}

	Sleep (5);

	SetForegroundWindow (sys_winInfo.hWnd);
	SetFocus (sys_winInfo.hWnd);
	return qTrue;
}


/*
=================
GLimp_Shutdown

This routine does all OS specific shutdown procedures for the OpenGL
subsystem. Under OpenGL this means NULLing out the current DC and
HGLRC, deleting the rendering context, and releasing the DC acquired
for the window. The state structure is also nulled out.
=================
*/
void GLimp_Shutdown (qBool full)
{
	Com_Printf (0, "OpenGL shutdown:\n");

	// Set the current context to NULL
	if (qwglMakeCurrent) {
		if (qwglMakeCurrent (NULL, NULL))
			Com_Printf (0, "...qwglMakeCurrent(0, 0) succeeded\n");
		else
			Com_Printf (PRNT_ERROR, "...qwglMakeCurrent(0, 0) failed\n");
	}

	// Delete the OpenGL context
	if (glwState.hGLRC) {
		if (qwglDeleteContext (glwState.hGLRC))
			Com_Printf (0, "...context deletion succeeded\n");
		else
			Com_Printf (PRNT_ERROR, "...context deletion failed\n");
		glwState.hGLRC = NULL;
	}

	// Release the hDC
	if (glwState.hDC) {
		if (ReleaseDC (sys_winInfo.hWnd, glwState.hDC))
			Com_Printf (0, "...hDC release succeeded\n");
		else
			Com_Printf (PRNT_ERROR, "...hDC release failed\n");
		glwState.hDC = NULL;
	}

	// Destroy the window
	if (sys_winInfo.hWnd) {
		Com_Printf (0, "...destroying the window\n");
		ShowWindow (sys_winInfo.hWnd, SW_HIDE);
		DestroyWindow (sys_winInfo.hWnd);
		sys_winInfo.hWnd = NULL;
	}

	// Close the gl_log file
	if (glwState.oglLogFP) {
		fclose (glwState.oglLogFP);
		glwState.oglLogFP = NULL;
	}

	// Reset display settings
	if (glwState.cdsFS) {
		Com_Printf (0, "...resetting display settings\n");
		ChangeDisplaySettings (NULL, 0);
		glwState.cdsFS = qFalse;
	}

	// Unregister the window
	if (full) {
		Com_Printf (0, "...unregistering the window\n");
		UnregisterClass (WINDOW_CLASS_NAME, sys_winInfo.hInstance);
		glwState.classRegistered = qFalse;
	}
}


/*
=================
GLimp_Init

This routine is responsible for initializing the OS specific portions of Opengl under Win32
this means dealing with the pixelformats and doing the wgl interface stuff.
=================
*/
#define OSR2_BUILD_NUMBER 1111
qBool GLimp_Init (void)
{
	glwState.bppChangeAllowed = qTrue;
	return qTrue;
}


/*
=================
GLimp_AttemptMode

Returns qTrue when the a mode change was successful
=================
*/
qBool GLimp_AttemptMode (qBool fullScreen, int width, int height)
{
	HDC		hdc;

	// Get desktop properties
	hdc = GetDC (GetDesktopWindow ());
	glwState.desktopBPP = GetDeviceCaps (hdc, BITSPIXEL);
	glwState.desktopWidth = GetDeviceCaps (hdc, HORZRES);
	glwState.desktopHeight = GetDeviceCaps (hdc, VERTRES);
	glwState.desktopHZ = GetDeviceCaps (hdc, VREFRESH);
	ReleaseDC (GetDesktopWindow (), hdc);

	Com_Printf (0, "Mode: %d x %d %s\n", width, height, fullScreen ? "(fullscreen)" : "(windowed)");

	// Set before calling GLimp_CreateWindow because it can trigger WM_MOVE which may
	// act incorrectly when switching between fullscreen and windowed
	ri.config.vidFullScreen = fullScreen;
	ri.config.vidWidth = width;
	ri.config.vidHeight = height;

	// Attempt fullscreen
	if (fullScreen) {
		Com_Printf (0, "...attempting fullscreen mode\n");

		memset (&glwState.windowDM, 0, sizeof (glwState.windowDM));
		glwState.windowDM.dmSize		= sizeof (glwState.windowDM);
		glwState.windowDM.dmPelsWidth	= width;
		glwState.windowDM.dmPelsHeight	= height;
		glwState.windowDM.dmFields		= DM_PELSWIDTH | DM_PELSHEIGHT;

		// Set display frequency
		if (r_displayFreq->intVal > 0) {
			glwState.windowDM.dmFields |= DM_DISPLAYFREQUENCY;
			glwState.windowDM.dmDisplayFrequency = r_displayFreq->intVal;

			Com_Printf (0, "...using r_displayFreq of %d\n", r_displayFreq->intVal);
			ri.config.vidFrequency = r_displayFreq->intVal;
		}
		else {
			ri.config.vidFrequency = glwState.desktopHZ;
			Com_Printf (0, "...using desktop frequency: %d\n", ri.config.vidFrequency);
		}

		// Set bit depth
		if (gl_bitdepth->intVal > 0) {
			glwState.windowDM.dmBitsPerPel = gl_bitdepth->intVal;
			glwState.windowDM.dmFields |= DM_BITSPERPEL;

			Com_Printf (0, "...using gl_bitdepth of %d\n", gl_bitdepth->intVal);
			ri.config.vidBitDepth = gl_bitdepth->intVal;
		}
		else {
			ri.config.vidBitDepth = glwState.desktopBPP;
			Com_Printf (0, "...using desktop depth: %d\n", ri.config.vidBitDepth);
		}

		// ChangeDisplaySettings
		Com_Printf (0, "...calling to ChangeDisplaySettings\n");
		if (ChangeDisplaySettings (&glwState.windowDM, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
			Com_Printf (PRNT_ERROR, "...fullscreen mode failed\n");
			Com_Printf (0, "...resetting display\n");
			ChangeDisplaySettings (NULL, 0);
			glwState.cdsFS = qFalse;
			return qFalse;
		}

		// Create the window
		if (!GLimp_CreateWindow (fullScreen, width, height)) {
			Com_Printf (0, "...resetting display\n");
			ChangeDisplaySettings (NULL, 0);
			glwState.cdsFS = qFalse;
			return qFalse;
		}

		glwState.cdsFS = qTrue;
		return qTrue;
	}

	// Create a window (not fullscreen)
	ri.config.vidBitDepth = glwState.desktopBPP;
	ri.config.vidFrequency = glwState.desktopHZ;

	// Reset the display
	if (glwState.cdsFS) {
		Com_Printf (0, "...resetting display\n");
		ChangeDisplaySettings (NULL, 0);
		glwState.cdsFS = qFalse;
	}

	// Create the window
	if (!GLimp_CreateWindow (qFalse, width, height)) {
		Com_Printf (PRNT_ERROR, "...windowed mode failed\n");
		return qFalse;
	}

	return qTrue;
}
