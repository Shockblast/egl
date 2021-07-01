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
#include "glimp_win.h"
#include "winquake.h"
#include "resource.h"
#include "wglext.h"

glwState_t	glwState;

cVar_t	*r_alphabits;
cVar_t	*r_colorbits;
cVar_t	*r_depthbits;
cVar_t	*r_stencilbits;
cVar_t	*cl_stereo;
cVar_t	*gl_allow_software;
cVar_t	*gl_stencilbuffer;

cVar_t	*vid_width;
cVar_t	*vid_height;

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

			Cvar_VariableForceSetValue (gl_bitdepth, 0);
			Com_Printf (0, S_COLOR_YELLOW "gl_bitdepth requires Win95 OSR2.x or WinNT 4.x -- forcing to 0\n");
		}

		gl_bitdepth->modified = qFalse;
	}

	if (glState.cameraSeparation != 0 && glState.stereoEnabled) {
		if (glState.cameraSeparation < 0)
			qglDrawBuffer (GL_BACK_LEFT);
		else if (glState.cameraSeparation > 0)
			qglDrawBuffer (GL_BACK_RIGHT);
	}
	else {
		qglDrawBuffer (GL_BACK);
	}
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
			if (!SwapBuffers (glwState.hDC))
				Com_Error (ERR_FATAL, "GLimp_EndFrame: - SwapBuffers failed!\n");
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

		if (glConfig.vidFullScreen && glConfig.allowCDS)
			ChangeDisplaySettings (&glwState.windowDM, CDS_FULLSCREEN);
	}
	else {
		if (glConfig.vidFullScreen) {
			ShowWindow (glwState.hWnd, SW_MINIMIZE);

			if (glConfig.allowCDS)
				ChangeDisplaySettings (NULL, 0);
		}
	}
}


/*
=================
GLimp_GetGammaRamp
=================
*/
qBool GLimp_GetGammaRamp (uShort *ramp)
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
void GLimp_SetGammaRamp (uShort *ramp)
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
GLimp_VerifyDriver
=================
*/
static qBool GLimp_VerifyDriver (void)
{
	char	buffer[1024];

	Q_strncpyz (buffer, qglGetString (GL_RENDERER), sizeof (buffer));
	Q_strlwr (buffer);
	if (!Q_stricmp (buffer, "gdi generic")) {
		if (!glwState.MCDAccelerated)
			return qFalse;
	}

	return qTrue;
}


/*
=================
GLimp_InitGL
=================
*/
static qBool GLimp_InitGL (void)
{
	PIXELFORMATDESCRIPTOR	iPFD;
	int		iPixelFormat;
	int		alphaBits;
	int		colorBits;
	int		depthBits;
	int		stencilBits;

	// desired PFD settings
	alphaBits = r_alphabits->integer;
	colorBits = r_colorbits->integer;
	depthBits = r_depthbits->integer;

	stencilBits = gl_stencilbuffer->integer ? r_stencilbits->integer : 0;

	// re-check console wordwrap size after video size setup
	Con_CheckResize ();

	// fill out a generic PFD
	iPFD.nSize			= sizeof (iPFD);
	iPFD.nVersion		= 1;
	iPFD.dwFlags		= PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	iPFD.iPixelType		= PFD_TYPE_RGBA;

	iPFD.cColorBits		= colorBits;
	iPFD.cRedBits		= iPFD.cRedShift	= 0;
	iPFD.cGreenBits		= iPFD.cGreenShift	= 0;
	iPFD.cBlueBits		= iPFD.cBlueShift	= 0;

	iPFD.cAlphaBits		= alphaBits;
	iPFD.cAlphaShift	= 0;

	iPFD.cAccumBits		= 0;
	iPFD.cAccumRedBits	= 0;
	iPFD.cAccumGreenBits= 0;
	iPFD.cAccumBlueBits	= 0;
	iPFD.cAccumAlphaBits= 0;

	iPFD.cDepthBits		= depthBits;
	iPFD.cStencilBits	= stencilBits;

	iPFD.cAuxBuffers	= 0;
	iPFD.iLayerType		= PFD_MAIN_PLANE;
	iPFD.bReserved		= 0;

	iPFD.dwLayerMask	= 0;
	iPFD.dwVisibleMask	= 0;
	iPFD.dwDamageMask	= 0;

	// set PFD_STEREO if necessary
	if (cl_stereo->integer) {
		Com_Printf (0, "...attempting to use stereo pfd\n");
		iPFD.dwFlags |= PFD_STEREO;
		glState.stereoEnabled = qTrue;
	}
	else {
		Com_Printf (0, "...not attempting to use stereo pfd\n");
		glState.stereoEnabled = qFalse;
	}

	// Get a DC for the specified window
	if (glwState.hDC != NULL) {
		Com_Printf (0, "..." S_COLOR_RED "non-NULL DC exists\n");
	}

	if ((glwState.hDC = GetDC (glwState.hWnd)) == NULL) {
		Com_Printf (0, "..." S_COLOR_RED "GetDC failed\n");
		return qFalse;
	}
	else {
		Com_Printf (0, "...GetDC succeeded\n");
	}

	// choose pixel format
	if (strstr (gl_driver->string, "opengl32") != 0) {
		glwState.miniDriver = qFalse;

		iPixelFormat = ChoosePixelFormat (glwState.hDC, &iPFD);
		if (!iPixelFormat) {
			Com_Printf (0, "..." S_COLOR_RED "ChoosePixelFormat failed\n");
			return qFalse;
		}
		else {
			Com_Printf (0, "...ChoosePixelFormat succeeded\n");
		}

		if (SetPixelFormat (glwState.hDC, iPixelFormat, &iPFD) == qFalse) {
			Com_Printf (0, "..." S_COLOR_RED "SetPixelFormat failed\n");
			return qFalse;
		}
		else {
			Com_Printf (0, "...SetPixelFormat succeeded\n");
		}

		DescribePixelFormat (glwState.hDC, iPixelFormat, sizeof (iPFD), &iPFD);

		if (iPFD.dwFlags & PFD_GENERIC_ACCELERATED)
			glwState.MCDAccelerated = qTrue;
		else {
			if (gl_allow_software->integer)
				glwState.MCDAccelerated = qTrue;
			else
				glwState.MCDAccelerated = qFalse;
		}
	}
	else {
		glwState.miniDriver = qTrue;

		iPixelFormat = qwglChoosePixelFormat (glwState.hDC, &iPFD);
		if (!iPixelFormat) {
			Com_Printf (0, "..." S_COLOR_RED "qwglChoosePixelFormat failed\n");
			return qFalse;
		}
		else {
			Com_Printf (0, "...qwglChoosePixelFormat succeeded\n");
		}

		if (qwglSetPixelFormat (glwState.hDC, iPixelFormat, &iPFD) == qFalse) {
			Com_Printf (0, "..." S_COLOR_RED "qwglSetPixelFormat failed\n");
			return qFalse;
		}
		else {
			Com_Printf (0, "...qwglSetPixelFormat succeeded\n");
		}

		qwglDescribePixelFormat (glwState.hDC, iPixelFormat, sizeof (iPFD), &iPFD);
	}

	// report if stereo is desired but unavailable
	if (cl_stereo->integer && !(iPFD.dwFlags & PFD_STEREO)) {
		Com_Printf (0, S_COLOR_RED "..." S_COLOR_RED "stereo pfd failed\n");
		Cvar_VariableForceSetValue (cl_stereo, 0);
		glState.stereoEnabled = qFalse;
	}
	else {
		Com_Printf (0, "...stereo pfd succeeded\n");
	}

	// startup the OpenGL subsystem by creating a context
	if ((glwState.hGLRC = qwglCreateContext (glwState.hDC)) == 0) {
		Com_Printf (0, "..." S_COLOR_RED "qwglCreateContext failed\n");
		return qFalse;
	}
	else {
		Com_Printf (0, "...qwglCreateContext succeeded\n");
	}

	// make the new context current
	if (!qwglMakeCurrent (glwState.hDC, glwState.hGLRC)) {
		Com_Printf (0, "..." S_COLOR_RED "qwglMakeCurrent failed\n");
		return qFalse;
	}
	else {
		Com_Printf (0, "...qwglMakeCurrent succeeded\n");
	}

	// check for hardware acceleration
	if (!GLimp_VerifyDriver ()) {
		Com_Printf (0, "..." S_COLOR_RED "no hardware acceleration detected\n");
		return qFalse;
	}
	else {
		Com_Printf (0, "...hardware acceleration detected\n");
	}

	glConfig.cColorBits = (int)iPFD.cColorBits;
	glConfig.cAlphaBits = (int)iPFD.cAlphaBits;
	glConfig.cDepthBits = (int)iPFD.cDepthBits;
	glConfig.cStencilBits = (int)iPFD.cStencilBits;

	// print out PFD specifics
	Com_Printf (0, "----------------------------------------\n");
	Com_Printf (0, "GL_PFD #%i: c(%d-bits) a(%d-bits) z(%d-bit) s(%d-bit)\n",
				iPixelFormat,
				(int)iPFD.cColorBits, (int)iPFD.cAlphaBits, (int)iPFD.cDepthBits, (int)iPFD.cStencilBits);


	return qTrue;
}


/*
=================
GLimp_CreateWindow
=================
*/
static qBool GLimp_CreateWindow (void)
{
	WNDCLASS		wClass;
	RECT			rect;
	DWORD			dwStyle;
	int				x, y;
	int				w, h;

	memset (&wClass, 0, sizeof (wClass));

	// register the frame class
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

	if (glConfig.vidFullScreen)
		dwStyle = WS_POPUP|WS_SYSMENU;
	else
		dwStyle = WS_TILEDWINDOW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS;

	rect.left = rect.top = 0;
	rect.right = vidDef.width;
	rect.bottom = vidDef.height;

	AdjustWindowRect (&rect, dwStyle, FALSE);

	w = rect.right - rect.left;
	h = rect.bottom - rect.top;

	if (glConfig.vidFullScreen) {
		x = 0;
		y = 0;
	}
	else {
		x = vid_xpos->integer;
		y = vid_ypos->integer;
	}

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
		UnregisterClass (WINDOW_CLASS_NAME, glwState.hInstance);
		Com_Error (ERR_FATAL, "Couldn't create window");
		return qFalse;
	}
	Com_Printf (0, "...CreateWindow succeeded\n");

	ShowWindow (glwState.hWnd, SW_SHOWNORMAL);
	UpdateWindow (glwState.hWnd);

	// init all the gl stuff for the window
	if (!GLimp_InitGL ()) {
		if (glwState.hGLRC) {
			qwglDeleteContext (glwState.hGLRC);
			glwState.hGLRC = NULL;
		}

		if (glwState.hDC) {
			ReleaseDC (glwState.hWnd, glwState.hDC);
			glwState.hDC = NULL;
		}

		Com_Printf (0, S_COLOR_RED "OpenGL initialization failed!\n");
		return qFalse;
	}

	SetForegroundWindow (glwState.hWnd);
	SetFocus (glwState.hWnd);

	return qTrue;
}


/*
=================
GLimp_Register
=================
*/
void GLimp_Register (void)
{
	r_alphabits			= Cvar_Get ("r_alphabits",			"0",		CVAR_LATCH_VIDEO);
	r_colorbits			= Cvar_Get ("r_colorbits",			"32",		CVAR_LATCH_VIDEO);
	r_depthbits			= Cvar_Get ("r_depthbits",			"24",		CVAR_LATCH_VIDEO);
	r_stencilbits		= Cvar_Get ("r_stencilbits",		"8",		CVAR_LATCH_VIDEO);
	cl_stereo			= Cvar_Get ("cl_stereo",			"0",		CVAR_LATCH_VIDEO);
	gl_allow_software	= Cvar_Get ("gl_allow_software",	"0",		CVAR_LATCH_VIDEO);
	gl_stencilbuffer	= Cvar_Get ("gl_stencilbuffer",		"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);

	vid_width			= Cvar_Get ("vid_width",			"0",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	vid_height			= Cvar_Get ("vid_height",			"0",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
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
				Com_Printf (0, S_COLOR_RED "qwglMakeCurrent failed\n");
		}

		if (qwglDeleteContext) {
			if (!qwglDeleteContext (glwState.hGLRC))
				Com_Printf (0, S_COLOR_RED "qwglDeleteContext failed\n");
		}

		glwState.hGLRC = NULL;
	}

	if (glwState.hDC) {
		if (!ReleaseDC (glwState.hWnd, glwState.hDC))
			Com_Printf (0, S_COLOR_RED "ReleaseDC failed\n");

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

	if (glConfig.vidFullScreen && glConfig.allowCDS)
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
		Com_Printf (0, S_COLOR_RED "GLimp_Init: - GetVersionEx failed\n");
		return qFalse;
	}

	glwState.hInstance = (HINSTANCE) hInstance;
	glwState.WndProc = WndProc;

	GLimp_Register ();

	return qTrue;
}

/*
=================
GLimp_AttemptMode

Returns qTrue when the a mode change was successful
=================
*/
qBool GLimp_AttemptMode (int mode)
{
	int		width;
	int		height;

	Com_Printf (0, "Initializing OpenGL display\n");

	memset (&glwState.desktopDM, 0, sizeof (glwState.desktopDM));
	EnumDisplaySettings (NULL, ENUM_CURRENT_SETTINGS, &glwState.desktopDM);

	// Custom modes support
	if (vid_width->integer && vid_height->integer) {
		Com_Printf (0, "Custom Mode: ");
		width = vid_width->integer;
		height = vid_height->integer;
	}
	else {
		Com_Printf (0, "Mode %d: ", mode);
		if (!R_GetInfoForMode (mode, &width, &height)) {
			Com_Printf (0, S_COLOR_RED "BAD MODE!\n");
			return qFalse;
		}
	}

	// Print mode information
	Com_Printf (0, "%d x %d %s\n", width, height, glConfig.vidFullScreen ? "(fullscreen)" : "(windowed)");

	// If a window already exists, destroy it
	if (glwState.hWnd)
		GLimp_Shutdown ();

	// Attempt fullscreen
	if (glConfig.vidFullScreen && glConfig.allowCDS) {
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
			// Success!
			vidDef.width = width;
			vidDef.height = height;

			if (!GLimp_CreateWindow ())
				return qFalse;

			return qTrue;
		}
		else {
			// Try again, assuming dual monitors
			Com_Printf (0, "..." S_COLOR_RED "fullscreen failed\n");
			Com_Printf (0, "...assuming dual monitor fullscreen\n");

			glwState.windowDM.dmPelsWidth = width * 2;

			// Our first CDS failed, so maybe we're running on some weird dual monitor system
			Com_Printf (0, "...calling to ChangeDisplaySettings\n");
			if (ChangeDisplaySettings (&glwState.windowDM, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL) {
				// Success!
				vidDef.width = width * 2;
				vidDef.height = height;

				if (!GLimp_CreateWindow ())
					return qFalse;

				return qTrue;
			}
			else {
				// Failed!
				Com_Printf (0, "..." S_COLOR_RED "dual monitor fullscreen failed\n");
				Com_Printf (0, "...attempting windowed mode\n");
			}
		}
	}

	// Create a window (not fullscreen)
	glConfig.vidBitDepth = glwState.desktopDM.dmBitsPerPel;

	if (glConfig.allowCDS)
		ChangeDisplaySettings (NULL, 0);

	Cvar_VariableForceSetValue (vid_fullscreen, 0);
	vid_fullscreen->modified = qFalse;
	glConfig.vidFullScreen = qFalse;

	vidDef.width = width;
	vidDef.height = height;

	if (!GLimp_CreateWindow ())
		return qFalse;

	return qTrue;
}
