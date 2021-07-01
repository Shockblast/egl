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
// glimp_win.c
// This file contains ALL Win32 specific stuff having to do with the OpenGL refresh
//

#include "../renderer/r_local.h"
#include "win_local.h"
#include "glimp_win.h"
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
		if (gl_bitdepth->integer > 0 && !glwState.bppChangeAllowed) {
			glConfig.vidBitDepth = glwState.desktopDM.dmBitsPerPel;

			Cvar_VariableSetValue (gl_bitdepth, 0, qTrue);
			Com_Printf (PRNT_WARNING, "gl_bitdepth requires Win95 OSR2.x or WinNT 4.x -- forcing to 0\n");
		}

		gl_bitdepth->modified = qFalse;
	}

	if (glState.cameraSeparation < 0)
		qglDrawBuffer (GL_BACK_LEFT);
	else if (glState.cameraSeparation > 0)
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
	if (glwState.active) {
		if (gl_errorcheck->integer)
			GL_CheckForError ("GLimp_EndFrame");
	}
	else if (glConfig.vidFullScreen) {
		Sleep (0);
		return;
	}

	if (stricmp (gl_drawbuffer->string, "GL_BACK") == 0) {
		if (glwState.miniDriver) {
			if (!qwglSwapBuffers (glwState.hDC))
				Com_Error (ERR_FATAL, "GLimp_EndFrame: - qwglSwapBuffers failed!\n");
		}
		else {
			SwapBuffers (glwState.hDC);
		}
	}

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
		SetForegroundWindow (glwState.hWnd);
		ShowWindow (glwState.hWnd, SW_SHOW);

		if (glConfig.vidFullScreen && glStatic.allowCDS)
			ChangeDisplaySettings (&glwState.windowDM, CDS_FULLSCREEN);
	}
	else {
		if (glConfig.vidFullScreen) {
			ShowWindow (glwState.hWnd, SW_MINIMIZE);

			if (glStatic.allowCDS)
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
	if (qwglGetDeviceGammaRamp3DFX)
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

	Com_Printf (0, "Attempting PFD(c%d a%d z%d s%d):\n",
		colorBits, alphaBits, depthBits, stencilBits);

	// Set PFD_STEREO if necessary
	if (cl_stereo->integer) {
		Com_Printf (0, "...attempting to use stereo pfd\n");
		pfd->dwFlags |= PFD_STEREO;
		glConfig.stereoEnabled = qTrue;
	}
	else {
		Com_Printf (0, "...not attempting to use stereo pfd\n");
		glConfig.stereoEnabled = qFalse;
	}

	// Choose a pixel format
	if (glwState.miniDriver) {
		iPixelFormat = qwglChoosePixelFormat (glwState.hDC, pfd);
		if (!iPixelFormat) {
			Com_Printf (PRNT_ERROR, "...qwglChoosePixelFormat failed\n");
			return 0;
		}
		else {
			Com_Printf (0, "...qwglChoosePixelFormat succeeded\n");
		}

		if (qwglSetPixelFormat (glwState.hDC, iPixelFormat, pfd) == qFalse) {
			Com_Printf (PRNT_ERROR, "...qwglSetPixelFormat failed\n");
			return 0;
		}
		else {
			Com_Printf (0, "...qwglSetPixelFormat succeeded\n");
		}

		qwglDescribePixelFormat (glwState.hDC, iPixelFormat, sizeof (PIXELFORMATDESCRIPTOR), pfd);
	}
	else {
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
	}

	// Check for hardware acceleration
	if (pfd->dwFlags & PFD_GENERIC_FORMAT) {
		if (!gl_allow_software->integer) {
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

	// Re-check console wordwrap size after video size setup
	CL_ConsoleCheckResize ();

	// Get a DC for the specified window
	if (glwState.hDC != NULL) {
		Com_Printf (PRNT_ERROR, "...non-NULL DC exists\n");
	}

	if ((glwState.hDC = GetDC (glwState.hWnd)) == NULL) {
		Com_Printf (PRNT_ERROR, "...GetDC failed\n");
		return qFalse;
	}
	else {
		Com_Printf (0, "...GetDC succeeded\n");
	}

	// Determine if we're using a minidriver
	glwState.miniDriver = !(strstr (gl_driver->string, "opengl32") != 0);

	// Alpha bits
	alphaBits = r_alphabits->integer;

	// Color bits
	colorBits = r_colorbits->integer;
	if (colorBits == 0)
		colorBits = glwState.desktopDM.dmBitsPerPel;

	// Depth bits
	if (r_depthbits->integer == 0) {
		if (colorBits > 16)
			depthBits = 24;
		else
			depthBits = 16;
	}
	else
		depthBits = r_depthbits->integer;

	// Stencil bits
	stencilBits = r_stencilbits->integer;
	if (!gl_stencilbuffer->integer)
		stencilBits = 0;
	else if (depthBits < 24)
		stencilBits = 0;

	// Setup the PFD
	iPixelFormat = GLimp_SetupPFD (&iPFD, colorBits, depthBits, alphaBits, stencilBits);
	if (!iPixelFormat) {
		// Don't needlessly try again
		if (colorBits = glwState.desktopDM.dmBitsPerPel && alphaBits == 0 && stencilBits == 0) {
			Com_Printf (PRNT_ERROR, "...failed to find a decent pixel format\n");
			return qFalse;
		}

		// Attempt two, no alpha and no stencil bits
		Com_Printf (PRNT_ERROR, "...first attempt failed, trying again\n");

		colorBits = glwState.desktopDM.dmBitsPerPel;
		if (r_depthbits->integer == 0) {
			if (colorBits > 16)
				depthBits = 24;
			else
				depthBits = 16;
		}
		else
			depthBits = r_depthbits->integer;
		alphaBits = 0;
		stencilBits = 0;

		iPixelFormat = GLimp_SetupPFD (&iPFD, colorBits, depthBits, alphaBits, stencilBits);
		if (!iPixelFormat) {
			Com_Printf (PRNT_ERROR, "...failed to find a decent pixel format\n");
			return qFalse;
		}
	}

	// Report if stereo is desired but unavailable
	if (cl_stereo->integer) {
		if (!(iPFD.dwFlags & PFD_STEREO)) {
			Com_Printf (PRNT_ERROR, "...stereo pfd failed\n");
			Cvar_VariableSetValue (cl_stereo, 0, qTrue);
			glConfig.stereoEnabled = qFalse;
		}
		else {
			Com_Printf (0, "...stereo pfd succeeded\n");
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

	glStatic.cColorBits = iPFD.cColorBits;
	glStatic.cAlphaBits = iPFD.cAlphaBits;
	glStatic.cDepthBits = iPFD.cDepthBits;
	glStatic.cStencilBits = iPFD.cStencilBits;

	// Print out PFD specifics
	Com_Printf (0, "----------------------------------------\n");
	Com_Printf (0, "GL_PFD #%i: c(%d-bits) a(%d-bits) z(%d-bit) s(%d-bit)\n",
				iPixelFormat,
				glStatic.cColorBits, glStatic.cAlphaBits, glStatic.cDepthBits, glStatic.cStencilBits);

	return qTrue;
}


/*
=================
GLimp_CreateWindow
=================
*/
static qBool GLimp_CreateWindow (qBool fullScreen, int width, int height)
{
	WNDCLASS		wClass;
	RECT			rect;
	DWORD			dwStyle;
	int				x, y;
	int				w, h;

	memset (&wClass, 0, sizeof (wClass));

	// Register the frame class
	wClass.style			= CS_HREDRAW|CS_VREDRAW;
	wClass.lpfnWndProc		= (WNDPROC)glwState.WndProc;
	wClass.cbClsExtra		= 0;
	wClass.cbWndExtra		= 0;
	wClass.hInstance		= glwState.hInstance;
	wClass.hIcon			= LoadIcon (glwState.hInstance, MAKEINTRESOURCE (IDI_ICON1));
	wClass.hCursor			= LoadCursor (NULL, IDC_ARROW);
	wClass.hbrBackground	= (HBRUSH)GetStockObject(WHITE_BRUSH);
	wClass.lpszMenuName		= 0;
	wClass.lpszClassName	= WINDOW_CLASS_NAME;

	if (!RegisterClass (&wClass)) {
		Com_Error (ERR_FATAL, "Couldn't register window class");
		return qFalse;
	}
	Com_Printf (0, "...RegisterClass succeeded\n");

	if (fullScreen)
		dwStyle = WS_POPUP|WS_SYSMENU;
	else
		dwStyle = WS_TILEDWINDOW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS;

	// Adjust the window
	rect.left = rect.top = 0;
	rect.right = width;
	rect.bottom = height;

	AdjustWindowRect (&rect, dwStyle, FALSE);

	w = rect.right - rect.left;
	h = rect.bottom - rect.top;

	if (fullScreen) {
		x = 0;
		y = 0;
	}
	else {
		x = vid_xpos->integer;
		y = vid_ypos->integer;
	}

	// Create the window
	glwState.hWnd = CreateWindow (
		 WINDOW_CLASS_NAME,		// class name
		 WINDOW_APP_NAME,		// app name
		 dwStyle | WS_VISIBLE,	// windows style
		 x, y,					// x y pos
		 w, h,					// width height
		 NULL,					// handle to parent
		 NULL,					// handle to menu
		 glwState.hInstance,	// app instance
		 NULL);					// no extra params

	if (!glwState.hWnd) {
		char *buf = NULL;

		UnregisterClass (WINDOW_CLASS_NAME, glwState.hInstance);
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
	ShowWindow (glwState.hWnd, SW_SHOWNORMAL);
	UpdateWindow (glwState.hWnd);

	// Init all the OpenGL stuff for the window
	if (!GLimp_GLInit ()) {
		if (glwState.hGLRC) {
			qwglDeleteContext (glwState.hGLRC);
			glwState.hGLRC = NULL;
		}

		if (glwState.hDC) {
			ReleaseDC (glwState.hWnd, glwState.hDC);
			glwState.hDC = NULL;
		}

		Com_Printf (PRNT_ERROR, "OpenGL initialization failed!\n");
		return qFalse;
	}

	Sleep (5);

	SetForegroundWindow (glwState.hWnd);
	SetFocus (glwState.hWnd);

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
void GLimp_Shutdown (void)
{
	if (glwState.hGLRC) {
		if (qwglMakeCurrent) {
			if (!qwglMakeCurrent (NULL, NULL))
				Com_Printf (PRNT_ERROR, "qwglMakeCurrent failed\n");
		}

		if (qwglDeleteContext) {
			if (!qwglDeleteContext (glwState.hGLRC))
				Com_Printf (PRNT_ERROR, "qwglDeleteContext failed\n");
		}

		glwState.hGLRC = NULL;
	}

	if (glwState.hDC) {
		if (!ReleaseDC (glwState.hWnd, glwState.hDC))
			Com_Printf (PRNT_ERROR, "ReleaseDC failed\n");

		glwState.hDC = NULL;
	}

	if (glwState.hWnd) {
		ShowWindow (glwState.hWnd, SW_HIDE);
		DestroyWindow (glwState.hWnd);
		glwState.hWnd = NULL;
	}

	if (glwState.oglLogFP) {
		fclose (glwState.oglLogFP);
		glwState.oglLogFP = 0;
	}

	if (glConfig.vidFullScreen && glStatic.allowCDS)
		ChangeDisplaySettings (NULL, 0);

	UnregisterClass (WINDOW_CLASS_NAME, glwState.hInstance);
}


/*
=================
GLimp_Init

This routine is responsible for initializing the OS specific portions of Opengl under Win32
this means dealing with the pixelformats and doing the wgl interface stuff.
=================
*/
#define OSR2_BUILD_NUMBER 1111
qBool GLimp_Init (void *hInstance, void *WndProc)
{
	OSVERSIONINFO	vinfo;

	vinfo.dwOSVersionInfoSize = sizeof (vinfo);
	glwState.bppChangeAllowed = qFalse;

	if (GetVersionEx (&vinfo)) {
		if (vinfo.dwMajorVersion > 4)
			glwState.bppChangeAllowed = qTrue;
		else if (vinfo.dwMajorVersion == 4) {
			if (vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
				glwState.bppChangeAllowed = qTrue;
			else if (vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
				if (LOWORD (vinfo.dwBuildNumber) >= OSR2_BUILD_NUMBER)
					glwState.bppChangeAllowed = qTrue;
		}
	}
	else {
		Com_Printf (PRNT_ERROR, "GLimp_Init: - GetVersionEx failed\n");
		return qFalse;
	}

	glwState.hInstance = (HINSTANCE) hInstance;
	glwState.WndProc = WndProc;
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
	memset (&glwState.desktopDM, 0, sizeof (glwState.desktopDM));
	EnumDisplaySettings (NULL, ENUM_CURRENT_SETTINGS, &glwState.desktopDM);

	Com_Printf (0, "Mode: %d x %d %s\n", width, height, fullScreen ? "(fullscreen)" : "(windowed)");

	// If a window already exists, destroy it
	if (glwState.hWnd)
		GLimp_Shutdown ();

	// Attempt fullscreen
	if (fullScreen && glStatic.allowCDS) {
		Com_Printf (0, "...attempting fullscreen mode\n");

		memset (&glwState.windowDM, 0, sizeof (glwState.windowDM));
		glwState.windowDM.dmSize		= sizeof (glwState.windowDM);
		glwState.windowDM.dmPelsWidth	= width;
		glwState.windowDM.dmPelsHeight	= height;
		glwState.windowDM.dmFields		= DM_PELSWIDTH | DM_PELSHEIGHT;

		// Set display frequency
		if (r_displayFreq->integer > 0) {
			glwState.windowDM.dmFields |= DM_DISPLAYFREQUENCY;
			glwState.windowDM.dmDisplayFrequency = r_displayFreq->integer;

			Com_Printf (0, "...using r_displayFreq of %d\n", r_displayFreq->integer);
			glConfig.vidFrequency = r_displayFreq->integer;
		}
		else {
			glConfig.vidFrequency = glwState.desktopDM.dmDisplayFrequency;
			Com_Printf (0, "...using desktop frequency: %d\n", glConfig.vidFrequency);
		}

		// Set bit depth
		if (gl_bitdepth->integer > 0) {
			glwState.windowDM.dmBitsPerPel = gl_bitdepth->integer;
			glwState.windowDM.dmFields |= DM_BITSPERPEL;

			Com_Printf (0, "...using gl_bitdepth of %d\n", gl_bitdepth->integer);
			glConfig.vidBitDepth = gl_bitdepth->integer;
		}
		else {
			glConfig.vidBitDepth = glwState.desktopDM.dmBitsPerPel;
			Com_Printf (0, "...using desktop depth: %d\n", glConfig.vidBitDepth);
		}

		// ChangeDisplaySettings
		Com_Printf (0, "...calling to ChangeDisplaySettings\n");
		if (ChangeDisplaySettings (&glwState.windowDM, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL) {
			if (!GLimp_CreateWindow (fullScreen, width, height))
				return qFalse;

			glConfig.vidFullScreen = fullScreen;
			glConfig.vidWidth = width;
			glConfig.vidHeight = height;
			return qTrue;
		}
		else {
			// Try again, assuming dual monitors
			Com_Printf (PRNT_ERROR, "...fullscreen failed\n");
			Com_Printf (0, "...assuming dual monitor fullscreen\n");

			glwState.windowDM.dmPelsWidth = width * 2;

			// Our first CDS failed, so maybe we're running on some weird dual monitor system
			Com_Printf (0, "...calling to ChangeDisplaySettings\n");
			if (ChangeDisplaySettings (&glwState.windowDM, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL) {
				if (!GLimp_CreateWindow (fullScreen, width, height))
					return qFalse;

				glConfig.vidFullScreen = fullScreen;
				glConfig.vidWidth = width * 2;
				glConfig.vidHeight = height;
				return qTrue;
			}
			else {
				// Failed!
				Com_Printf (PRNT_ERROR, "...dual monitor fullscreen failed\n");
				Com_Printf (0, "...attempting windowed mode\n");
			}
		}
	}

	// Create a window (not fullscreen)
	if (glStatic.allowCDS)
		ChangeDisplaySettings (NULL, 0);
	if (!GLimp_CreateWindow (qFalse, width, height))
		return qFalse;

	glConfig.vidBitDepth = glwState.desktopDM.dmBitsPerPel;
	glConfig.vidFrequency = glwState.desktopDM.dmDisplayFrequency;
	glConfig.vidFullScreen = qFalse;
	glConfig.vidWidth = width;
	glConfig.vidHeight = height;
	return qTrue;
}
