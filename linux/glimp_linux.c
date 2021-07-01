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

#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <dlfcn.h>

#include "x11.h"
#include "../renderer/r_local.h"
#include "glimp_linux.h"
#include "../ui/keycodes.h"

glxState_t	glxState;

cVar_t	*r_alphabits;
cVar_t	*r_colorbits;
cVar_t	*r_depthbits;
cVar_t	*r_stencilbits;
cVar_t	*cl_stereo;
cVar_t	*gl_allow_software;
cVar_t	*gl_stencilbuffer;

cVar_t	*vid_width;
cVar_t	*vid_height;

#define DISPLAY_MASK (VisibilityChangeMask | StructureNotifyMask | ExposureMask )
#define INIT_MASK (KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | DISPLAY_MASK )

#define KEY_MASK (KeyPressMask | KeyReleaseMask)
#define MOUSE_MASK (ButtonPressMask | ButtonReleaseMask |  PointerMotionMask | ButtonMotionMask )
#define X_MASK (KEY_MASK | MOUSE_MASK | VisibilityChangeMask | StructureNotifyMask )

x11display_t x11display;

glxState_t glxState;

qBool mouse_avail;
int mouse_buttonstate;
int mouse_oldbuttonstate;
int mx, my;
int p_mouse_x, p_mouse_y;
int old_mouse_x, old_mouse_y;

qBool	mlooking;

qBool mouse_active = qFalse;
static qBool dgamouse = qFalse;
static qBool vidmode_ext = qFalse;

cVar_t	*m_filter;
cVar_t	*in_mouse;
cVar_t	*in_dgamouse;

static cVar_t *sensitivity;
static cVar_t *lookstrafe;
static cVar_t *m_side;
static cVar_t *m_yaw;
static cVar_t *m_pitch;
static cVar_t *m_forward;
cVar_t *freelook;

static Time myxtime;

#ifdef XF86

static int _xf86_vidmodes_suported = 0;
static XF86VidModeModeInfo **_xf86_vidmodes;
static int _xf86_vidmodes_num;
static int _xf86_vidmodes_active = 0;


static void _xf86_VidmodesInit(void){
	int MajorVersion, MinorVersion;
	int i;

	// Get video mode list
	MajorVersion = MinorVersion = 0;

	if (XF86VidModeQueryVersion (x11display.dpy, &MajorVersion, &MinorVersion))
	{
		Com_Printf (0, "..XFree86-VidMode Extension Version %d.%d\n", MajorVersion, MinorVersion);
		XF86VidModeGetAllModeLines (x11display.dpy, x11display.scr, &_xf86_vidmodes_num, &_xf86_vidmodes);
		_xf86_vidmodes_suported = 1;
	}
	else
	{
		Com_Printf (0, "..XFree86-VidMode Extension not available\n");
		_xf86_vidmodes_suported = 0;
	}
}

static void _xf86_VidmodesFree (void)
{
	if (_xf86_vidmodes_suported) {
		XFree(_xf86_vidmodes);
	}

	_xf86_vidmodes_suported = 0;
}

static void _xf86_VidmodesSwitch (int mode)
{
	if (_xf86_vidmodes_suported) {
		XF86VidModeSwitchToMode (x11display.dpy, x11display.scr, _xf86_vidmodes[mode]);
		XF86VidModeSetViewPort (x11display.dpy, x11display.scr, 0, 0);
	}

	_xf86_vidmodes_active = 1;
}

static void _xf86_VidmodesSwitchBack (void)
{
	if (_xf86_vidmodes_suported) {
		if (_xf86_vidmodes_active) 
			_xf86_VidmodesSwitch (0);
	}

	_xf86_vidmodes_active = 0;
}

static void _xf86_VidmodesFindBest(int *mode, int *pwidth, int *pheight){
	int i, best_fit, best_dist, dist, x, y;

	best_fit = -1;
	best_dist = 999999999;

	if (_xf86_vidmodes_suported)
	{
		for (i =0; i <_xf86_vidmodes_num; i++)
		{
			if (_xf86_vidmodes[i]->hdisplay <*pwidth || _xf86_vidmodes[i]->vdisplay <*pheight) 
				continue;

			x = _xf86_vidmodes[i]->hdisplay -*pwidth;
			y = _xf86_vidmodes[i]->vdisplay -*pheight;

			if (x > y)
				dist = y;
			else
				dist = x;

			if (dist < 0) 
				dist = -dist; // Only positive number please
		
			if( dist < best_dist) {
				best_dist = dist;
				best_fit = i;
			}

			Com_Printf (0, "%ix%i -> %ix%i: %i\n", *pwidth, *pheight, _xf86_vidmodes[i]->hdisplay, _xf86_vidmodes[i]->vdisplay, dist);
		}

		if (best_fit >= 0)
		{
			Com_Printf (0, "%ix%i selected\n", _xf86_vidmodes[best_fit]->hdisplay, _xf86_vidmodes[best_fit]->vdisplay);

			*pwidth =_xf86_vidmodes[best_fit]->hdisplay;
			*pheight =_xf86_vidmodes[best_fit]->vdisplay;
		}
	}

	*mode = best_fit;

}
#endif

static void _x11_WaitForMaped (Window w)
{
	XEvent event;

	if (x11display.dpy) {
		do 
		{
			XMaskEvent(x11display.dpy, StructureNotifyMask, &event);
		} while((event.type != MapNotify) || (event.xmap.event != w));
	}
}


static void _x11_WaitForUnmaped (Window w)
{
	XEvent event;

	if (x11display.dpy)
	{
		do
		{
			XMaskEvent(x11display.dpy, StructureNotifyMask, &event);
		} while((event.type != UnmapNotify) || (event.xunmap.event != w));
	}
}

static void _x11_SetNoResize (Window w, int width, int height)
{
	XSizeHints *hints;

	if (x11display.dpy)
	{
		hints = XAllocSizeHints();

		if (hints)
		{
			hints->min_width = hints->max_width = width;
			hints->min_height = hints->max_height = height;
				
			hints->flags = PMaxSize | PMinSize;

			XSetWMNormalHints (x11display.dpy, w, hints);
			XFree (hints);
		}
	}
}


static Cursor CreateNullCursor (Display *display, Window root)
{
    Pixmap cursormask; 
    XGCValues xgc;
    GC gc;
    XColor dummycolour;
    Cursor cursor;

    cursormask = XCreatePixmap (display, root, 1, 1, 1/*depth*/);
    xgc.function = GXclear;
    gc = XCreateGC (display, cursormask, GCFunction, &xgc);
    XFillRectangle (display, cursormask, gc, 0, 0, 1, 1);

    dummycolour.pixel = 0;
    dummycolour.red = 0;
    dummycolour.flags = 04;

    cursor = XCreatePixmapCursor (display, cursormask, cursormask, &dummycolour, &dummycolour, 0, 0);
    XFreePixmap (display, cursormask);
    XFreeGC (display, gc);

    return cursor;
}

void install_grabs (void)
{
	XDefineCursor (x11display.dpy, x11display.win, CreateNullCursor(x11display.dpy, x11display.win));
	XGrabPointer (x11display.dpy, x11display.win, True, 0, GrabModeAsync, GrabModeAsync, x11display.win, None, CurrentTime);

	if (in_dgamouse->integer)
	{
		#ifdef XF86
		int MajorVersion, MinorVersion;

		if (XF86DGAQueryVersion (x11display.dpy, &MajorVersion, &MinorVersion)) 
		{ 
			XF86DGADirectVideo (x11display.dpy, x11display.scr, XF86DGADirectMouse);
			XWarpPointer (x11display.dpy, None, x11display.win, 0, 0, 0, 0, 0, 0);
			dgamouse = qTrue;

		} else
		#endif
		{
			// unable to query, probalby not supported
			Com_Printf(0, "Failed to detect XF86DGA Mouse\n" );
			Cvar_Set( "in_dgamouse", "0" );
			dgamouse = qFalse;
		}
	} else {
		p_mouse_x = x11display.win_width / 2;
		p_mouse_y = x11display.win_height / 2;
		XWarpPointer (x11display.dpy, None, x11display.win, 0, 0, 0, 0, p_mouse_x, p_mouse_y);
	}

	XGrabKeyboard (x11display.dpy, x11display.win, False, GrabModeAsync, GrabModeAsync, CurrentTime);

	mx = 0;
	my = 0;
	old_mouse_x = 0;
	old_mouse_y = 0;
	mouse_active = qTrue;
}

void uninstall_grabs (void)
{
	if (!x11display.dpy || !x11display.win)
		return;

	if (dgamouse)
	{
		dgamouse = qFalse;
		#ifdef XF86
		XF86DGADirectVideo (x11display.dpy, x11display.scr, 0);
		#endif
	}

	XUngrabPointer (x11display.dpy, CurrentTime);
	XUngrabKeyboard (x11display.dpy, CurrentTime);

	// inviso cursor
	XUndefineCursor (x11display.dpy, x11display.win);

	mouse_active = qFalse;
}



static int XLateKey(XKeyEvent *ev)
{
	int key;
	char buf[64];
	KeySym keysym;

	key = 0;

	XLookupString (ev, buf, sizeof buf, &keysym, 0);

	switch (keysym)
	{
		case XK_KP_Page_Up:	 key = K_KP_PGUP; break;
		case XK_Page_Up:	 key = K_PGUP; break;

		case XK_KP_Page_Down: key = K_KP_PGDN; break;
		case XK_Page_Down:	 key = K_PGDN; break;

		case XK_KP_Home: key = K_KP_HOME; break;
		case XK_Home:	 key = K_HOME; break;

		case XK_KP_End:  key = K_KP_END; break;
		case XK_End:	 key = K_END; break;

		case XK_KP_Left: key = K_KP_LEFTARROW; break;
		case XK_Left:	 key = K_LEFTARROW; break;

		case XK_KP_Right: key = K_KP_RIGHTARROW; break;
		case XK_Right:	key = K_RIGHTARROW;		break;

		case XK_KP_Down: key = K_KP_DOWNARROW; break;
		case XK_Down:	 key = K_DOWNARROW; break;

		case XK_KP_Up:   key = K_KP_UPARROW; break;
		case XK_Up:		 key = K_UPARROW;	 break;

		case XK_Escape: key = K_ESCAPE;		break;

		case XK_KP_Enter: key = K_KP_ENTER;	break;
		case XK_Return: key = K_ENTER;		 break;

		case XK_Tab:		key = K_TAB;			 break;

		case XK_F1:		 key = K_F1;				break;

		case XK_F2:		 key = K_F2;				break;

		case XK_F3:		 key = K_F3;				break;

		case XK_F4:		 key = K_F4;				break;

		case XK_F5:		 key = K_F5;				break;

		case XK_F6:		 key = K_F6;				break;

		case XK_F7:		 key = K_F7;				break;

		case XK_F8:		 key = K_F8;				break;

		case XK_F9:		 key = K_F9;				break;

		case XK_F10:		key = K_F10;			 break;

		case XK_F11:		key = K_F11;			 break;

		case XK_F12:		key = K_F12;			 break;

		case XK_BackSpace: key = K_BACKSPACE; break;

		case XK_KP_Delete: key = K_KP_DEL; break;
		case XK_Delete: key = K_DEL; break;

		case XK_Pause:	key = K_PAUSE;		 break;

		case XK_Shift_L:
		case XK_Shift_R:	key = K_SHIFT;		break;

		case XK_Execute: 
		case XK_Control_L: 
		case XK_Control_R:	key = K_CTRL;		 break;

		case XK_Alt_L:	
		case XK_Meta_L: 
		case XK_Alt_R:	
		case XK_Meta_R: key = K_ALT;			break;

		case XK_KP_Begin: key = K_KP_FIVE;	break;

		case XK_Insert:key = K_INS; break;
		case XK_KP_Insert: key = K_KP_INS; break;

		case XK_KP_Multiply: key = '*'; break;
		case XK_KP_Add:  key = K_KP_PLUS; break;
		case XK_KP_Subtract: key = K_KP_MINUS; break;
		case XK_KP_Divide: key = K_KP_SLASH; break;

#if 0
		case 0x021: key = '1';break;/* [!] */
		case 0x040: key = '2';break;/* [@] */
		case 0x023: key = '3';break;/* [#] */
		case 0x024: key = '4';break;/* [$] */
		case 0x025: key = '5';break;/* [%] */
		case 0x05e: key = '6';break;/* [^] */
		case 0x026: key = '7';break;/* [&] */
		case 0x02a: key = '8';break;/* [*] */
		case 0x028: key = '9';;break;/* [(] */
		case 0x029: key = '0';break;/* [)] */
		case 0x05f: key = '-';break;/* [_] */
		case 0x02b: key = '=';break;/* [+] */
		case 0x07c: key = '\'';break;/* [|] */
		case 0x07d: key = '[';break;/* [}] */
		case 0x07b: key = ']';break;/* [{] */
		case 0x022: key = '\'';break;/* ["] */
		case 0x03a: key = ';';break;/* [:] */
		case 0x03f: key = '/';break;/* [?] */
		case 0x03e: key = '.';break;/* [>] */
		case 0x03c: key = ',';break;/* [<] */
#endif

		default:
			key = *(unsigned char*)buf;
			if (key >= 'A' && key <= 'Z')
				key = key - 'A' + 'a';
			if (key >= 1 && key <= 26) /* ctrl+alpha */
				key = key + 'a' - 1;
			break;
	} 

	return key;
}


void HandleEvents(void)
{
	XEvent event;
	qBool dowarp = qFalse;
	int mwx = x11display.win_width / 2;
	int mwy = x11display.win_height / 2;

	if (!x11display.dpy || !x11display.win) 
		return;

	while (XPending(x11display.dpy))
	{
		XNextEvent (x11display.dpy, &event);

		switch (event.type) 
		{
			case KeyPress:
				myxtime = event.xkey.time;
			case KeyRelease:
				Key_Event (XLateKey(&event.xkey), event.type == KeyPress, Sys_Milliseconds());
				break;

			case MotionNotify:
				if (mouse_active)
				{
					if (dgamouse)
					{
						mx += event.xmotion.x_root * 2;
						my += event.xmotion.y_root * 2;
					}
					else
					{
						if( !event.xmotion.send_event ) {
    						    mx += event.xmotion.x - p_mouse_x;
						    my += event.xmotion.y - p_mouse_y;

						    if( abs( mwx - event.xmotion.x ) > mwx / 2 || abs( mwy - event.xmotion.y ) > mwy / 2 )
							dowarp = qTrue;
						}
						p_mouse_x = event.xmotion.x;
						p_mouse_y = event.xmotion.y;
					}
				}
				break;

			case ButtonPress:
				myxtime = event.xbutton.time;
				if (event.xbutton.button == 1) Key_Event(K_MOUSE1, 1, Sys_Milliseconds());
				else if (event.xbutton.button == 2) Key_Event(K_MOUSE3, 1, Sys_Milliseconds());
				else if (event.xbutton.button == 3) Key_Event(K_MOUSE2, 1, Sys_Milliseconds());
				else if (event.xbutton.button == 4) Key_Event(K_MWHEELUP, 1, Sys_Milliseconds());
				else if (event.xbutton.button == 5) Key_Event(K_MWHEELDOWN, 1, Sys_Milliseconds());
				else if (event.xbutton.button >= 6 && event.xbutton.button <= 10) Key_Event(K_MOUSE4+event.xbutton.button-6, 1, Sys_Milliseconds());
				break;

			case ButtonRelease:
				if (event.xbutton.button == 1) Key_Event(K_MOUSE1, 0, Sys_Milliseconds());
				else if (event.xbutton.button == 2) Key_Event(K_MOUSE3, 0, Sys_Milliseconds());
				else if (event.xbutton.button == 3) Key_Event(K_MOUSE2, 0, Sys_Milliseconds());
				else if (event.xbutton.button == 4) Key_Event(K_MWHEELUP, 0, Sys_Milliseconds());
				else if (event.xbutton.button == 5) Key_Event(K_MWHEELDOWN, 0, Sys_Milliseconds());
				else if (event.xbutton.button >= 6 && event.xbutton.button <= 10) Key_Event(K_MOUSE4+event.xbutton.button-6, 0, Sys_Milliseconds());
				break;

			case ClientMessage:
				if (event.xclient.data.l[0] == x11display.wmDeleteWindow) 
					Cmd_ExecuteString ("quit");
				break;
		}
	}

	if (dowarp) 
	{
		/* move the mouse to the window center again */
		p_mouse_x = mwx;
		p_mouse_y = mwy;
		XWarpPointer (x11display.dpy, None, x11display.win, 0, 0, 0, 0, mwx, mwy);
	}
}

void KBD_Update (void)
{
}

void KBD_Close (void)
{
}


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
	qglXSwapBuffers (x11display.dpy, x11display.gl_win);
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
}

/*
=================
GLimp_GetGammaRamp
=================
*/
qBool GLimp_GetGammaRamp (uShort *ramp)
{
	size_t	stride = 256;

	if (XF86VidModeGetGammaRamp (x11display.dpy, x11display.scr, stride, ramp, ramp + stride, ramp + (stride << 1) ) != 0 )
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
	size_t	stride = 256;

	XF86VidModeSetGammaRamp (x11display.dpy, x11display.scr, stride, ramp, ramp + stride, ramp + (stride << 1));
}


/*
=============================================================================

	INIT / SHUTDOWN

=============================================================================
*/


static void signal_handler (int sig)
{
	printf("Received signal %d, exiting...\n", sig);
	GLimp_Shutdown();
	_exit(0);
}

static void InitSig (void) 
{
	signal(SIGHUP, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGILL, signal_handler);
	signal(SIGTRAP, signal_handler);
	signal(SIGIOT, signal_handler);
	signal(SIGBUS, signal_handler);
	signal(SIGFPE, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGTERM, signal_handler);
}


/*
=================
GLimp_Shutdown
=================
*/
void GLimp_Shutdown (void)
{
	uninstall_grabs ();
	mouse_active = qFalse;
	dgamouse = qFalse;

	if (x11display.dpy)
	{
//		IN_Activate (qFalse);
#ifdef XF86
		_xf86_VidmodesSwitchBack();
		_xf86_VidmodesFree();
#endif
		if (x11display.ctx) 
			qglXDestroyContext (x11display.dpy, x11display.ctx);

		if (x11display.gl_win) 
			XDestroyWindow (x11display.dpy, x11display.gl_win);

		if (x11display.win) 
			XDestroyWindow(x11display.dpy, x11display.win);

		XCloseDisplay (x11display.dpy);
	}

	x11display.ctx = NULL;
	x11display.gl_win = 0;
	x11display.win = 0;
	x11display.dpy = NULL;
}


/*
=================
GLimp_Init
=================
*/
qBool GLimp_Init (void *hInstance, void *WndProc)
{
	int i;

	int attributes_0[] = {
		GLX_RGBA,
		GLX_DOUBLEBUFFER,
		GLX_RED_SIZE, 4,
		GLX_GREEN_SIZE, 4,
		GLX_BLUE_SIZE, 4,
		GLX_DEPTH_SIZE, 24,
		GLX_STENCIL_SIZE, 8,
		None
	};

	int attributes_1[] = {
		GLX_RGBA,
		GLX_DOUBLEBUFFER,
		GLX_RED_SIZE, 4,
		GLX_GREEN_SIZE, 4,
		GLX_BLUE_SIZE, 4,
		GLX_DEPTH_SIZE, 24,
		None
	};

	struct{
		char *name;
		int *attributes;
	} attributes_list[] = {
		{"rbga, double-buffered, 24bits depth and 8bit stencil", attributes_0},
		{"rbga, double-buffered and 24bits depth", attributes_1},
		{0, 0}
	};

	XSetWindowAttributes attr;
	unsigned long mask;

	if (x11display.dpy) 
		GLimp_Shutdown();

	InitSig ();

	Sys_Printf ("Display initialization\n");

	x11display.dpy = XOpenDisplay (NULL);

	if (!x11display.dpy )
	{
		Sys_Printf ("..Error couldn't open the X display\n");
		return 0;
	}

	x11display.scr = DefaultScreen (x11display.dpy);
	x11display.root = RootWindow (x11display.dpy, x11display.scr);

#ifdef XF86
	_xf86_VidmodesInit ();
#endif

	for (i =0; attributes_list[i].name; i++){
		x11display.visinfo = qglXChooseVisual (x11display.dpy, x11display.scr, attributes_list[i].attributes);
		if (!x11display.visinfo){
			Sys_Printf ("..Failed to get %s\n", attributes_list[i].name);
		} else{
            Sys_Printf ("..Get %s\n", attributes_list[i].name);
			break;
		}
	}

	if (!x11display.visinfo) 
		return 0;

	x11display.ctx = qglXCreateContext(x11display.dpy, x11display.visinfo, NULL, True);
	x11display.cmap = XCreateColormap(x11display.dpy, x11display.root, x11display.visinfo->visual, AllocNone);

	attr.colormap = x11display.cmap;
	attr.border_pixel = 0;
	attr.event_mask = DISPLAY_MASK;
	attr.override_redirect = False;
	mask = CWBorderPixel | CWColormap;

	x11display.gl_win = XCreateWindow(x11display.dpy, x11display.root, 0, 0, 1, 1, 0,
		x11display.visinfo->depth, InputOutput, x11display.visinfo->visual, mask, &attr);
	qglXMakeCurrent(x11display.dpy, x11display.gl_win, x11display.ctx);

	XSync (x11display.dpy, False);

	return(1);
}


/*
=================
GLimp_AttemptMode

Returns qTrue when the a mode change was successful
=================
*/
qBool GLimp_AttemptMode (int mode)
{
	int width, height, screen_width, screen_height, screen_mode;
	float ratio;
	XSetWindowAttributes wa;
	unsigned long mask;

	qBool	fullscreen = qFalse; // FIXME

	if (!VID_GetModeInfo( &width, &height, mode))
	{
		Com_Printf(0, " invalid mode\n" );
		return qFalse;
	}

	screen_width = width;
	screen_height = height;

	if (fullscreen)
	{
	#ifdef XF86

		_xf86_VidmodesFindBest (&screen_mode, &screen_width, &screen_height);

		if (screen_mode < 0) 
			return qFalse;

		if (screen_width < width || screen_height < height)
		{
			if (width > height)
			{
				ratio = width / height;
				height = height *ratio;
				width = screen_width;
			}
			else
			{
				ratio = height / width;
				width = width *ratio;
				height = screen_height;

			}
		}

		Com_Printf (0, "...setting fullscreen mode %d:", mode);

		/* Create fulscreen window */
		x11display.old_win = x11display.win;

		wa.background_pixel = 0;
		wa.border_pixel = 0;
		wa.event_mask = INIT_MASK;
		wa.override_redirect = True;
		mask = CWBackPixel | CWBorderPixel | CWEventMask | CWOverrideRedirect;

		x11display.win = XCreateWindow(x11display.dpy, x11display.root, 0, 0, screen_width, screen_height,
			0, CopyFromParent, InputOutput, CopyFromParent, mask, &wa);

		XResizeWindow (x11display.dpy, x11display.gl_win, width, height);
		XReparentWindow (x11display.dpy, x11display.gl_win, x11display.win, (screen_width /2) -(width /2), (screen_height /2) - (height /2));

		XMapWindow (x11display.dpy, x11display.gl_win);
		XMapWindow (x11display.dpy, x11display.win);
	
		_x11_SetNoResize (x11display.win, screen_width, screen_height);

		_xf86_VidmodesSwitch (screen_mode);

	#else
		Com_Printf (0, "Fullscreen require XFree86. Sorry.\n");
		return qFalse;
	#endif 
	}
	else
	{
		Com_Printf (0, "...setting mode %d:", mode);

		/* Create managed window */
		x11display.old_win = x11display.win;

		wa.background_pixel = 0;
		wa.border_pixel = 0;
		wa.event_mask = INIT_MASK;
		mask = CWBackPixel | CWBorderPixel | CWEventMask;

		x11display.win = XCreateWindow (x11display.dpy, x11display.root, 0, 0, screen_width, screen_height,
			0, CopyFromParent, InputOutput, CopyFromParent, mask, &wa);
		XSetStandardProperties (x11display.dpy, x11display.win, APPLICATION, None, None, NULL, 0, NULL);
		x11display.wmDeleteWindow = XInternAtom(x11display.dpy, "WM_DELETE_WINDOW", False);
		XSetWMProtocols (x11display.dpy, x11display.win, &x11display.wmDeleteWindow, 1);

		XResizeWindow (x11display.dpy, x11display.gl_win, width, height);
		XReparentWindow (x11display.dpy, x11display.gl_win, x11display.win, 0, 0);
		XMapWindow (x11display.dpy, x11display.gl_win);
		XMapWindow (x11display.dpy, x11display.win);
	
		_x11_SetNoResize(x11display.win, width, height);
	}

	// save the parent window size for mouse use.
	// this is not the gl context window
	x11display.win_width = width;
	x11display.win_height = height;

	if (x11display.old_win)
	{
		XDestroyWindow (x11display.dpy, x11display.old_win);
		x11display.old_win = 0;
	}
	
	XFlush (x11display.dpy);

//	glState.width = width;
//	glState.height = height;
//	glState.fullScreen = fullscreen;

	// let the sound and input subsystems know about the new window
	VID_NewWindow (width, height);

	return qTrue;
}
