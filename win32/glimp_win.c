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

#define WGLPFD_APP_NAME		WINDOW_APP_NAME" WGLPFD Dummy"
#define WGLPFD_CLASS_NAME	WINDOW_CLASS_NAME" WGLPFD"

glwState_t	glwState;

cVar_t	*gl_nowglpfd;
cVar_t	*r_alphabits;
cVar_t	*r_colorbits;
cVar_t	*r_depthbits;
cVar_t	*r_stencilbits;
cVar_t	*cl_stereo;
cVar_t	*gl_allow_software;
cVar_t	*gl_stencilbuffer;

cVar_t	*glimp_extensions;
cVar_t	*gl_ext_multisample;
cVar_t	*gl_ext_samples;

/*
=============================================================

	FRAME SETUP

=============================================================
*/

/*
=================
GLimp_BeginFrame
=================
*/
void GLimp_BeginFrame (void) {
	if (gl_bitdepth->modified) {
		if ((gl_bitdepth->integer > 0) && !glwState.bppChangeAllowed) {
			HDC hdc = GetDC (NULL);
			glConfig.vidBitDepth = GetDeviceCaps (hdc, BITSPIXEL);
			ReleaseDC (0, hdc);

			Cvar_VariableForceSetValue (gl_bitdepth, 0);
			Com_Printf (0, S_COLOR_YELLOW "gl_bitdepth requires Win95 OSR2.x or WinNT 4.x -- forcing to 0\n");
		}

		gl_bitdepth->modified = qFalse;
	}

	if ((glState.cameraSeparation != 0) && glState.stereoEnabled) {
		if (glState.cameraSeparation < 0)
			qglDrawBuffer (GL_BACK_LEFT);
		else if (glState.cameraSeparation > 0)
			qglDrawBuffer (GL_BACK_RIGHT);
	} else
		qglDrawBuffer (GL_BACK);
}


/*
=================
GLimp_EndFrame

Responsible for doing a swapbuffers and possibly for other stuff as yet to be determined.
Probably better not to make this a GLimp function and instead do a call to GLimp_SwapBuffers.
=================
*/
void GLimp_EndFrame (void) {
#ifdef _DEBUG
	GL_CheckForError ();
#endif // _DEBUG

	if (stricmp (gl_drawbuffer->string, "GL_BACK") == 0) {
		if (!qwglSwapBuffers (glwState.hDC))
			Com_Error (ERR_FATAL, "GLimp_EndFrame: - SwapBuffers failed!\n");
	}

	Sleep (0);
}

/*
=============================================================

	MISC

=============================================================
*/

/*
=================
GLimp_AppActivate
=================
*/
void GLimp_AppActivate (qBool active) {
	if (active) {
		SetForegroundWindow (glwState.hWnd);
		ShowWindow (glwState.hWnd, SW_SHOW);
	} else {
		if (glConfig.vidFullscreen)
			ShowWindow (glwState.hWnd, SW_MINIMIZE);
	}
}

/*
=============================================================

	INIT / SHUTDOWN

=============================================================
*/

/*
=================
VerifyDriver
=================
*/
static qBool VerifyDriver (void) {
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
GLimp_InitExtensions
=================
*/
void GLimp_InitExtensions (HDC hDC) {
	if (!glimp_extensions->integer) {
		Com_Printf (0, "...ignoring GL implementation extensions\n");
		return;
	}

	// get our WGL extension string
	qwglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC) qwglGetProcAddress ("wglGetExtensionsStringARB");
	if (!qwglGetExtensionsStringARB) qwglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC) qwglGetProcAddress ("wglGetExtensionsStringEXT");

	if (qwglGetExtensionsStringARB)
		glwState.wglExtsString = qwglGetExtensionsStringARB (hDC);
	else if (qwglGetExtensionsStringEXT)
		glwState.wglExtsString = qwglGetExtensionsStringEXT ();
	else
		glwState.wglExtsString = NULL;

	if (glwState.wglExtsString == NULL) {
		Com_Printf (0, S_COLOR_YELLOW "WGL extension lookup FAILED!\n");
		return;
	}

	/*
	** WGL_ARB_multisample
	*/
	if (!gl_ext_multisample->integer || !gl_ext_samples->integer)
		Com_Printf (0, "...ignoring WGL_ARB_multisample\n");
	else if (R_ExtensionAvailable (glwState.wglExtsString, "WGL_ARB_multisample")) {
		Com_Printf (0, "...enabling WGL_ARB_multisample\n");
		glwState.wglExtMultisample = qTrue;
	} else
		Com_Printf (0, "...WGL_ARB_multisample not found\n");

	/*
	** WGL_ARB_pixel_format
	*/
	if (R_ExtensionAvailable (glwState.wglExtsString, "WGL_ARB_pixel_format")) {
		qwglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC) qwglGetProcAddress ("wglChoosePixelFormatARB");
		if (qwglChoosePixelFormatARB) qwglGetPixelFormatAttribivARB = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC) qwglGetProcAddress ("wglGetPixelFormatAttribivARB");
		if (qwglGetPixelFormatAttribivARB) qwglGetPixelFormatAttribfvARB = (PFNWGLGETPIXELFORMATATTRIBFVARBPROC) qwglGetProcAddress ("wglGetPixelFormatAttribfvARB");

		if (!qwglGetPixelFormatAttribfvARB)
			Com_Printf (0, "..." S_COLOR_RED "WGL_ARB_pixel_format not properly supported!\n");
		else {
			Com_Printf (0, "...enabling WGL_ARB_pixel_format\n");
			glwState.wglExtPixFormat = qTrue;
		}
	} else
		Com_Printf (0, "...WGL_ARB_pixel_format not found\n");

	/*
	** WGL_EXT_swap_control
	** GL Extension lookup will also load this, but it's afterward so don't check if it's already loaded
	*/
	if (gl_ext_swapinterval->integer) {
		if (R_ExtensionAvailable (glwState.wglExtsString, "WGL_EXT_swap_control")) {
			qwglSwapIntervalEXT = (BOOL (WINAPI *)(int)) qwglGetProcAddress ("wglSwapIntervalEXT");

			if (!qwglSwapIntervalEXT)
				Com_Printf (0, "..." S_COLOR_RED "WGL_EXT_swap_control not properly supported!\n");
			else {
				Com_Printf (0, "...enabling WGL_EXT_swap_control\n");
				if (!glConfig.extWinSwapInterval)
					glConfig.extWinSwapInterval = qTrue;
			}
		} else
			Com_Printf (0, "...WGL_EXT_swap_control not found\n");
	} else
		Com_Printf (0, "...ignoring WGL_EXT_swap_control\n");
}


/*
=================
GLimp_InitGLPFD
=================
*/
qBool GLimp_InitGLPFD (PIXELFORMATDESCRIPTOR *iPFD, int *iPixelFormat) {
	// set PFD_STEREO if necessary
	if (cl_stereo->integer) {
		Com_Printf (0, "...attempting to use stereo\n");
		iPFD->dwFlags |= PFD_STEREO;
		glState.stereoEnabled = qTrue;
	} else
		glState.stereoEnabled = qFalse;

	// Get a DC for the specified window
	if (glwState.hDC != NULL)
		Com_Printf (0, "..." S_COLOR_RED "non-NULL DC exists\n");

	if ((glwState.hDC = GetDC (glwState.hWnd)) == NULL) {
		Com_Printf (0, "..." S_COLOR_RED "getdc failed\n");
		return qFalse;
	}

	// choose pixel format
	if (strstr (gl_driver->string, "opengl32") != 0) {
		glwState.miniDriver = qFalse;

		*iPixelFormat = ChoosePixelFormat (glwState.hDC, &*iPFD);
		if (!iPixelFormat) {
			Com_Printf (0, "..." S_COLOR_RED "ChoosePixelFormat failed\n");
			return qFalse;
		}

		if (SetPixelFormat (glwState.hDC, *iPixelFormat, &*iPFD) == qFalse) {
			Com_Printf (0, "..." S_COLOR_RED "SetPixelFormat failed\n");
			return qFalse;
		}

		DescribePixelFormat (glwState.hDC, *iPixelFormat, sizeof (iPFD), &*iPFD);

		if (iPFD->dwFlags & PFD_GENERIC_ACCELERATED)
			glwState.MCDAccelerated = qTrue;
		else {
			if (gl_allow_software->integer)
				glwState.MCDAccelerated = qTrue;
			else
				glwState.MCDAccelerated = qFalse;
		}
	} else {
		glwState.miniDriver = qTrue;

		*iPixelFormat = qwglChoosePixelFormat (glwState.hDC, &*iPFD);
		if (!iPixelFormat) {
			Com_Printf (0, "..." S_COLOR_RED "qwglChoosePixelFormat failed\n");
			return qFalse;
		}

		if (qwglSetPixelFormat (glwState.hDC, *iPixelFormat, &*iPFD) == qFalse) {
			Com_Printf (0, "..." S_COLOR_RED "qwglSetPixelFormat failed\n");
			return qFalse;
		}

		qwglDescribePixelFormat (glwState.hDC, *iPixelFormat, sizeof (iPFD), &*iPFD);
	}

	// report if stereo is desired but unavailable
	if (cl_stereo->integer && !(iPFD->dwFlags & PFD_STEREO)) {
		Com_Printf (0, S_COLOR_RED "..." S_COLOR_RED "stereo pixel format failed\n");
		Cvar_VariableForceSetValue (cl_stereo, 0);
		glState.stereoEnabled = qFalse;
	}

	// startup the OpenGL subsystem by creating a context
	if ((glwState.hGLRC = qwglCreateContext (glwState.hDC)) == 0) {
		Com_Printf (0, "..." S_COLOR_RED "create context failed\n");
		return qFalse;
	}

	// make the new context current
	if (!qwglMakeCurrent (glwState.hDC, glwState.hGLRC)) {
		Com_Printf (0, "..." S_COLOR_RED "make current failed\n");
		return qFalse;
	}

	if (!VerifyDriver ()) {
		Com_Printf (0, "..." S_COLOR_RED "no hardware acceleration detected\n");
		return qFalse;
	}

	glConfig.cColorBits = (int)iPFD->cColorBits;
	glConfig.cAlphaBits = (int)iPFD->cAlphaBits;
	glConfig.cDepthBits = (int)iPFD->cDepthBits;
	glConfig.cStencilBits = (int)iPFD->cStencilBits;

	return qTrue;
}


/*
=================
GLimp_InitWGLPFD
=================
*/
LRESULT CALLBACK WndProc_temp (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	return DefWindowProc (hWnd, uMsg, wParam, lParam);
}

qBool GLimp_InitWGLPFD (PIXELFORMATDESCRIPTOR *iPFD, int *iPixelFormat,
						int alphaBits, int colorBits, int depthBits, int stencilBits) {
	PIXELFORMATDESCRIPTOR	tempPFD;
	qBool		useMultiSample;
	int			validPFD;
	UINT		numPFDS;
	int			iAttribs[30];
	float		fAttribs[] = {0,0};
	int			iResults[30];

	WNDCLASS	wc_temp;
	HGLRC		hGLRC_temp;
	HWND		hWnd_temp;
	HDC			hDC_temp;

	// register the dumby pfd-seaking child
	if (GetClassInfo (glwState.hInstance, WGLPFD_CLASS_NAME, &wc_temp)) {
		Com_Printf (0, "..." S_COLOR_RED "GLW temp class already exists!\n");
		goto fail;
	}

	memset (&wc_temp, 0, sizeof (wc_temp));

	wc_temp.style			= CS_OWNDC;
	wc_temp.lpfnWndProc		= (WNDPROC)WndProc_temp;
	wc_temp.cbWndExtra		= 0;
	wc_temp.hInstance		= glwState.hInstance;
	wc_temp.hCursor			= LoadCursor (NULL, IDC_ARROW);
	wc_temp.hbrBackground	= NULL;
	wc_temp.lpszClassName	= WGLPFD_CLASS_NAME;

	if (!RegisterClass (&wc_temp)) {
		Com_Printf (0, "..." S_COLOR_RED "WGL PFD temp class registration failed!\n");
		goto fail;
	}

	hWnd_temp = CreateWindow (
		WGLPFD_CLASS_NAME,		// class name
		WGLPFD_APP_NAME,		// app name
		WS_POPUP |				// windows style
		WS_CLIPCHILDREN |
		WS_CLIPSIBLINGS,
		0, 0,
		1, 1,
		glwState.hWnd,				// handle to parent
		0,						// handle to menu
		glwState.hInstance,			// app instance
		NULL);					// no extra params

	if (!glwState.hWnd) {
		UnregisterClass (WGLPFD_CLASS_NAME, glwState.hInstance);
		Com_Printf (0, "..." S_COLOR_RED "CreateWindow failed\n");
		goto fail;
	}

	// Get a DC for the specified window
	if ((hDC_temp = GetDC (hWnd_temp)) == NULL) {
		Com_Printf (0, "..." S_COLOR_RED "getdc failed\n");
		goto fail;
	}

	// find out if we're running on a minidriver or not
	if (strstr (gl_driver->string, "opengl32") != 0)
		glwState.miniDriver = qFalse;
	else
		glwState.miniDriver = qTrue;

	// choose pixel format
	tempPFD = *iPFD;
	*iPixelFormat = ChoosePixelFormat (hDC_temp, &tempPFD);
	if (!iPixelFormat) {
		Com_Printf (0, "..." S_COLOR_RED "ChoosePixelFormat failed\n");
		goto fail;
	}

	if (SetPixelFormat (hDC_temp, *iPixelFormat, &tempPFD) == qFalse) {
		Com_Printf (0, "..." S_COLOR_RED "SetPixelFormat failed\n");
		goto fail;
	}

	// startup the OpenGL subsystem by creating a context
	if ((hGLRC_temp = qwglCreateContext (hDC_temp)) == 0) {
		Com_Printf (0, "..." S_COLOR_RED "create context failed\n");
		goto fail;
	}

	// make the new context current
	if (!qwglMakeCurrent (hDC_temp, hGLRC_temp)) {
		Com_Printf (0, "..." S_COLOR_RED "make context current failed\n");
		goto fail;
	}

	if (!VerifyDriver ()) {
		Com_Printf (0, "..." S_COLOR_RED "no hardware acceleration detected\n");
		goto fail;
	}

	// grab extensions
	GLimp_InitExtensions (hDC_temp);

	// safe-guards
	Com_Printf (0, "----------------------------------------\n");
	if (!glwState.wglExtPixFormat || !qwglGetPixelFormatAttribivARB || !qwglChoosePixelFormatARB) {
		Com_Printf (0, "..." S_COLOR_YELLOW "WGL ARB pixel formatting not supported\n");
		goto fail;
	}

	// check if multisampling is desired and available
	if (gl_ext_multisample->integer && gl_ext_samples->integer && glwState.wglExtMultisample)
		useMultiSample = qTrue;
	else
		useMultiSample = qFalse;

	// make no rendering context current
	qwglMakeCurrent (NULL, NULL);

	// destroy the rendering context
	qwglDeleteContext (hGLRC_temp);
	hGLRC_temp = NULL;

	// get the number of pixel format available
	iAttribs[0] = WGL_NUMBER_PIXEL_FORMATS_ARB;
	iAttribs[1] = 0;
	if (qwglGetPixelFormatAttribivARB (hDC_temp, 0, 0, 1, iAttribs, iResults) == GL_FALSE) {
		Com_Printf (0, "..." S_COLOR_RED "qwglGetPixelFormatAttribivARB failed\n");
		goto fail;
	}

	// choose a pfd with multisampling support
	iAttribs[0 ] = WGL_DOUBLE_BUFFER_ARB;	iAttribs[1 ] = qTrue;
	iAttribs[2 ] = WGL_COLOR_BITS_ARB;		iAttribs[3 ] = colorBits;
	iAttribs[4 ] = WGL_DEPTH_BITS_ARB;		iAttribs[5 ] = depthBits;
	iAttribs[6 ] = WGL_ALPHA_BITS_ARB;		iAttribs[7 ] = alphaBits;
	iAttribs[8 ] = WGL_STENCIL_BITS_ARB;	iAttribs[9 ] = stencilBits;

	if (useMultiSample) {
		iAttribs[10] = WGL_SAMPLE_BUFFERS_ARB;	iAttribs[11] = qTrue;
		iAttribs[12] = WGL_SAMPLES_ARB;			iAttribs[13] = gl_ext_samples->integer;
	} else {
		iAttribs[10] = 0;						iAttribs[11] = 0;
		iAttribs[12] = 0;						iAttribs[13] = 0;
	}

	iAttribs[14] = 0;					iAttribs[15] = 0;

	// first attempt
	validPFD = qwglChoosePixelFormatARB (hDC_temp, iAttribs, fAttribs, 1, &*iPixelFormat, &numPFDS);
	// failure happens not only when the function fails, but also when no matching pixel format has been found
	if ((validPFD == qFalse) || !numPFDS) {
		Com_Printf (0, "..." S_COLOR_RED "qwglChoosePixelFormatARB failed\n");
		goto fail;
	} else {
		// fill the list of attributes we are interested in
		iAttribs[0 ] = WGL_PIXEL_TYPE_ARB;
		iAttribs[1 ] = WGL_COLOR_BITS_ARB;
		iAttribs[2 ] = WGL_RED_BITS_ARB;
		iAttribs[3 ] = WGL_GREEN_BITS_ARB;
		iAttribs[4 ] = WGL_BLUE_BITS_ARB;
		iAttribs[5 ] = WGL_ALPHA_BITS_ARB;
		iAttribs[6 ] = WGL_DEPTH_BITS_ARB;
		iAttribs[7 ] = WGL_STENCIL_BITS_ARB;

		/*
		** Since WGL_ARB_multisample and WGL_pbuffer are extensions, we must check if
		** those extensions are supported before passing the corresponding enums
		** to the driver. This could cause an error if they are not supported.
		*/
		iAttribs[8 ] = useMultiSample ? WGL_SAMPLE_BUFFERS_ARB	: WGL_PIXEL_TYPE_ARB;
		iAttribs[9 ] = useMultiSample ? WGL_SAMPLES_ARB			: WGL_PIXEL_TYPE_ARB;
		iAttribs[12] = WGL_PIXEL_TYPE_ARB;
		iAttribs[10] = WGL_DRAW_TO_WINDOW_ARB;
		iAttribs[11] = WGL_DRAW_TO_BITMAP_ARB;
		iAttribs[13] = WGL_DOUBLE_BUFFER_ARB;
		iAttribs[14] = WGL_STEREO_ARB;
		iAttribs[15] = WGL_ACCELERATION_ARB;
		iAttribs[16] = WGL_NEED_PALETTE_ARB;
		iAttribs[17] = WGL_NEED_SYSTEM_PALETTE_ARB;
		iAttribs[18] = WGL_SWAP_LAYER_BUFFERS_ARB;
		iAttribs[19] = WGL_SWAP_METHOD_ARB;
		iAttribs[20] = WGL_NUMBER_OVERLAYS_ARB;
		iAttribs[21] = WGL_NUMBER_UNDERLAYS_ARB;
		iAttribs[22] = WGL_TRANSPARENT_ARB;
		iAttribs[23] = WGL_SUPPORT_GDI_ARB;
		iAttribs[24] = WGL_SUPPORT_OPENGL_ARB;

		if (qwglGetPixelFormatAttribivARB (hDC_temp, *iPixelFormat, 0, 25, iAttribs, iResults) == GL_FALSE) {
			Com_Printf (0, "..." S_COLOR_RED "qwglGetPixelFormatAttribivARB failed\n");
			goto fail;
		}

		*iPFD = tempPFD;
		glConfig.cColorBits = iResults[1];
		glConfig.cAlphaBits = iResults[5];
		glConfig.cDepthBits = iResults[6];
		glConfig.cStencilBits = iResults[7];

		if (glConfig.extMultiSample)
			glConfig.extMultiSample = qFalse;
		if (useMultiSample) {
			// if this is qTrue multisampling is on
			if (iResults[8]) {
				glConfig.extMultiSample = qTrue;
				glState.multiSampleOn = qFalse;	// induces turning it on in GL_ToggleMultisample
				GL_ToggleMultisample (qTrue);
				if (glConfig.extNVMulSampleHint) {
					if (!Q_stricmp (gl_nv_multisample_filter_hint->string, "nicest"))
						qglHint (GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
					else if (!Q_stricmp (gl_nv_multisample_filter_hint->string, "fastest"))
						qglHint (GL_MULTISAMPLE_FILTER_HINT_NV, GL_FASTEST);
					else
						qglHint (GL_MULTISAMPLE_FILTER_HINT_NV, GL_DONT_CARE);
				}
			} else
				Com_Printf (0, S_COLOR_YELLOW "...multisampling failed\n");
		}
	}

	if (iResults[15] != WGL_FULL_ACCELERATION_ARB)
		Com_Printf (0, S_COLOR_YELLOW "* WARNING * iPixelFormat %d is NOT hardware accelerated! * WARNING *\n", *iPixelFormat);

	ReleaseDC (hWnd_temp, hDC_temp);
	DestroyWindow (hWnd_temp);
	hWnd_temp = NULL;

	// Get a DC for the specified window
	if ((glwState.hDC = GetDC (glwState.hWnd)) == NULL) {
		Com_Printf (0, "..." S_COLOR_RED "getdc failed\n");
		goto fail;
	}

	SetPixelFormat (glwState.hDC, *iPixelFormat, &tempPFD);
	// startup the OpenGL subsystem by creating a context
	if ((glwState.hGLRC = qwglCreateContext (glwState.hDC)) == 0) {
		Com_Printf (0, "..." S_COLOR_RED "create context failed\n");
		goto fail;
	}

	// make the new context current
	if (!qwglMakeCurrent (glwState.hDC, glwState.hGLRC)) {
		Com_Printf (0, "..." S_COLOR_RED "make current failed\n");
		goto fail;
	}

	glConfig.wglPFD = qTrue;
	return qTrue;

fail:
	Com_Printf (0, "..." S_COLOR_YELLOW "falling back to standard PFD\n");
	Com_Printf (0, "----------------------------------------\n");

	ReleaseDC (hWnd_temp, hDC_temp);
	DestroyWindow (hWnd_temp);
	hWnd_temp = NULL;

	if (glwState.hGLRC) {
		qwglDeleteContext (glwState.hGLRC);
		glwState.hGLRC = NULL;
	}

	if (glwState.hDC) {
		ReleaseDC (glwState.hWnd, glwState.hDC);
		glwState.hDC = NULL;
	}

	return qFalse;
}


/*
=================
GLimp_InitGL
=================
*/
qBool GLimp_InitGL (void) {
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

	// set defaults
	glwState.wglExtMultisample = qFalse;
	glwState.wglExtPixFormat = qFalse;

	qwglGetExtensionsStringARB	= NULL;
	qwglGetExtensionsStringEXT	= NULL;

	qwglChoosePixelFormatARB		= NULL;
	qwglGetPixelFormatAttribivARB	= NULL;
	qwglGetPixelFormatAttribfvARB	= NULL;

	qwglSwapIntervalEXT			= NULL;

	// re-check console wordwrap size after video size setup
	Con_CheckResize ();

	if (!gl_nowglpfd->integer && glimp_extensions->integer) {
		Com_Printf (0, "----------------------------------------\n");
		Com_Printf (0, "GL Implementation Initialization:\n");
	}

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

	// choose a PFD
	glConfig.wglPFD = qFalse;
	if (gl_nowglpfd->integer || !glimp_extensions->integer || !GLimp_InitWGLPFD (&iPFD, &iPixelFormat, alphaBits, colorBits, depthBits, stencilBits)) {
		if (!GLimp_InitGLPFD (&iPFD, &iPixelFormat))
			return qFalse;
	}

	// print out PFD specifics
	Com_Printf (0, "%s_PFD: c(%d-bits) a(%d-bits) z(%d-bit) s(%d-bit)\n",
				(glConfig.wglPFD) ? "WGL" : "GL",
				(int)iPFD.cColorBits, (int)iPFD.cAlphaBits, (int)iPFD.cDepthBits, (int)iPFD.cStencilBits);

	return qTrue;
}


/*
=================
GLimp_CreateWindow
=================
*/
qBool GLimp_CreateWindow (void) {
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

	if (!RegisterClass (&wClass))
		Com_Error (ERR_FATAL, "Couldn't register window class");

	if (glConfig.vidFullscreen)
		dwStyle = WS_POPUP|WS_SYSMENU;
	else {
		dwStyle = WS_TILEDWINDOW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS;
	}

	rect.left = rect.top = 0;
	rect.right = vidDef.width;
	rect.bottom = vidDef.height;

	AdjustWindowRect (&rect, dwStyle, FALSE);

	w = rect.right - rect.left;
	h = rect.bottom - rect.top;

	if (glConfig.vidFullscreen) {
		x = 0;
		y = 0;
	} else {
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
	}

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
void GLimp_Register (void) {
	gl_nowglpfd			= Cvar_Get ("gl_nowglpfd",			"0",		CVAR_LATCH_VIDEO);
	r_alphabits			= Cvar_Get ("r_alphabits",			"0",		CVAR_LATCH_VIDEO);
	r_colorbits			= Cvar_Get ("r_colorbits",			"32",		CVAR_LATCH_VIDEO);
	r_depthbits			= Cvar_Get ("r_depthbits",			"24",		CVAR_LATCH_VIDEO);
	r_stencilbits		= Cvar_Get ("r_stencilbits",		"8",		CVAR_LATCH_VIDEO);
	cl_stereo			= Cvar_Get ("cl_stereo",			"0",		CVAR_LATCH_VIDEO);
	gl_allow_software	= Cvar_Get ("gl_allow_software",	"0",		CVAR_LATCH_VIDEO);
	gl_stencilbuffer	= Cvar_Get ("gl_stencilbuffer",		"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);

	glimp_extensions	= Cvar_Get ("glimp_extensions",		"1",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_ext_multisample	= Cvar_Get ("gl_ext_multisample",	"0",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_ext_samples		= Cvar_Get ("gl_ext_samples",		"2",		CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
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
void GLimp_Shutdown (void) {
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

	if (glConfig.vidFullscreen)
		ChangeDisplaySettings (NULL, 0);

	UnregisterClass (WINDOW_CLASS_NAME, glwState.hInstance);
	UnregisterClass (WGLPFD_CLASS_NAME, glwState.hInstance);
}


/*
=================
GLimp_Init

This routine is responsible for initializing the OS specific portions of Opengl under Win32
this means dealing with the pixelformats and doing the wgl interface stuff.
=================
*/
#define OSR2_BUILD_NUMBER 1111
qBool GLimp_Init (void *hInstance, void *WndProc) {
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
	} else {
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
=================
*/
int GLimp_AttemptMode (int mode) {
	static const char	*winFS[] = { "(windowed)", "(fullscreen)" };
	int		width;
	int		height;

	Com_Printf (0, "Initializing OpenGL display\n");
	Com_Printf (0, "Mode %d: ", mode);

	if (!R_GetInfoForMode (mode, &width, &height)) {
		Com_Printf (0, S_COLOR_RED "BAD MODE!\n");
		return modeErr_InvalidMode;
	}

	// print mode information
	Com_Printf (0, "%d x %d %s\n", width, height, winFS[glConfig.vidFullscreen]);

	// if a window already exists, destroy it
	if (glwState.hWnd)
		GLimp_Shutdown ();

	// attempt fullscreen
	if (glConfig.vidFullscreen) {
		DEVMODE		dm;

		Com_Printf (0, "...attempting fullscreen mode\n");

		memset (&dm, 0, sizeof (dm));
		dm.dmSize		= sizeof (dm);
		dm.dmPelsWidth	= width;
		dm.dmPelsHeight	= height;
		dm.dmFields		= DM_PELSWIDTH | DM_PELSHEIGHT;

		// set display frequency
		Com_Printf (0, "...using ");
		if (r_displayfreq->integer > 0) {
			dm.dmFields |= DM_DISPLAYFREQUENCY;
			dm.dmDisplayFrequency	= r_displayfreq->integer;

			Com_Printf (0, "r_displayfreq of %d\n", r_displayfreq->integer);
			glConfig.vidFrequency = r_displayfreq->integer;
		} else {
			HDC hdc = GetDC (NULL);
			int displayFreq = GetDeviceCaps (hdc, VREFRESH);

			Com_Printf (0, "desktop frequency: %d\n", displayFreq);
			glConfig.vidFrequency = displayFreq;

			ReleaseDC (0, hdc);
		}

		// set bit depth
		Com_Printf (0, "...using ");
		if (gl_bitdepth->integer > 0) {
			dm.dmBitsPerPel = gl_bitdepth->integer;
			dm.dmFields |= DM_BITSPERPEL;

			Com_Printf (0, "gl_bitdepth of %d\n", gl_bitdepth->integer);
			glConfig.vidBitDepth = gl_bitdepth->integer;
		} else {
			HDC hdc = GetDC (NULL);
			int bitspixel = GetDeviceCaps (hdc, BITSPIXEL);

			Com_Printf (0, "desktop depth: %d\n", bitspixel);
			glConfig.vidBitDepth = bitspixel;

			ReleaseDC (0, hdc);
		}

		// ChangeDisplaySettings
		Com_Printf (0, "...calling to ChangeDisplaySettings\n");
		if (ChangeDisplaySettings (&dm, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL) {
			// success!
			vidDef.width = width;
			vidDef.height = height;

			if (!GLimp_CreateWindow ())
				return modeErr_InvalidMode;

			return modeErr_None;
		} else {
			// try again, assuming dual monitors
			Com_Printf (0, S_COLOR_RED "failed\n");
			Com_Printf (0, "...trying CDS assuming dual monitors: ");

			dm.dmPelsWidth = width * 2;
			dm.dmPelsHeight = height;
			dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

			// set display frequency
			Com_Printf (0, "...using ");
			if (r_displayfreq->integer > 0) {
				dm.dmFields |= DM_DISPLAYFREQUENCY;
				dm.dmDisplayFrequency	= r_displayfreq->integer;

				Com_Printf (0, "r_displayfreq of %d\n", r_displayfreq->integer);
				glConfig.vidFrequency = r_displayfreq->integer;
			} else {
				HDC hdc = GetDC (NULL);
				int displayFreq = GetDeviceCaps (hdc, VREFRESH);

				Com_Printf (0, "desktop frequency: %d\n", displayFreq);
				glConfig.vidFrequency = displayFreq;

				ReleaseDC (0, hdc);
			}

			// set bit depth
			if (gl_bitdepth->integer > 0) {
				dm.dmBitsPerPel = gl_bitdepth->integer;
				dm.dmFields |= DM_BITSPERPEL;

				Com_Printf (0, "gl_bitdepth of %d\n", gl_bitdepth->integer);
				glConfig.vidBitDepth = gl_bitdepth->integer;
			} else {
				HDC hdc = GetDC (NULL);
				int bitspixel = GetDeviceCaps (hdc, BITSPIXEL);

				Com_Printf (0, "desktop depth: %d\n", bitspixel);
				glConfig.vidBitDepth = bitspixel;

				ReleaseDC (0, hdc);
			}

			// our first CDS failed, so maybe we're running on some weird dual monitor system
			if (ChangeDisplaySettings (&dm, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL) {
				// success!
				vidDef.width = width * 2;
				vidDef.height = height;

				if (!GLimp_CreateWindow ())
					return modeErr_InvalidMode;

				return modeErr_None;
			} else {
				// failed!
				Com_Printf (0, S_COLOR_RED "failed\n");
				Com_Printf (0, "...attempting windowed mode\n");

				ChangeDisplaySettings (NULL, 0);

				vidDef.width = width;
				vidDef.height = height;

				Cvar_VariableForceSetValue (vid_fullscreen, 0);
				vid_fullscreen->modified = qFalse;
				glConfig.vidFullscreen = qFalse;

				if (!GLimp_CreateWindow ())
					return modeErr_InvalidMode;

				return modeErr_InvalidFullscreen;
			}
		}
	} else {
		// create a window (not fullscreen)
		HDC hdc = GetDC (NULL);
		glConfig.vidBitDepth = GetDeviceCaps (hdc, BITSPIXEL);
		ReleaseDC (0, hdc);

		ChangeDisplaySettings (NULL, 0);

		vidDef.width = width;
		vidDef.height = height;

		if (!GLimp_CreateWindow ())
			return modeErr_InvalidMode;

		return modeErr_None;
	}
}
