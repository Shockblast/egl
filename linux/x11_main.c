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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

//
// x11_main.c
// Main windowed and fullscreen graphics interface module
//

#include "x11_linux.h"
#include "glimp_linux.h"

// Console variables that we need to access from this module
cVar_t		*vid_xpos;			// X coordinate of window position
cVar_t		*vid_ypos;			// Y coordinate of window position
cVar_t		*vid_fullscreen;

#define DISPLAY_MASK (VisibilityChangeMask | StructureNotifyMask | ExposureMask )
#define INIT_MASK (KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | DISPLAY_MASK |  EnterWindowMask | LeaveWindowMask)

#define KEY_MASK (KeyPressMask | KeyReleaseMask)
#define MOUSE_MASK (ButtonPressMask | ButtonReleaseMask |  PointerMotionMask | ButtonMotionMask )
#define X_MASK (KEY_MASK | MOUSE_MASK | VisibilityChangeMask | StructureNotifyMask )

x11display_t x11display = {.dpy=NULL};

int mx, my;
int p_mouse_x, p_mouse_y;
int old_mouse_x, old_mouse_y;

static qBool mouse_grabbed = qFalse;
static qBool keybd_grabbed = qFalse;
static qBool dgamouse = qFalse;
static int   grab_screenmode = -1;

cVar_t *sensitivity;
cVar_t *m_yaw;
cVar_t *m_pitch;
static cVar_t *in_dgamouse;

cVar_t *m_filter;
cVar_t *in_mouse;
cVar_t *freelook;

/*
==========================================================================

  WINDOW STUFF

==========================================================================
*/

static void updategrabstatus()
{
	static qBool  mg      = qFalse;
	static qBool  mv      = qFalse;
	static qBool  kg      = qFalse;
	static Cursor cr      = None;

	if ((x11display.focused && mouse_grabbed && keybd_grabbed) != mg) {
		// actual grab is made only if keyboard too, to allow windowmanagers to grab to their own task switching window
		mg = !mg;
		if (mg) {
			mx = my = 0;
		      
			// Make sure pointer is inside the window even when using dga
			XWarpPointer (x11display.dpy, None, x11display.grab_win, 0, 0, 0, 0, p_mouse_x = x11display.win_width/2, p_mouse_y = x11display.win_height/2);
			if (!XGrabPointer (x11display.dpy, x11display.grab_win, True, 0, GrabModeAsync, GrabModeAsync, x11display.grab_win, None, CurrentTime)) {
        			if (in_dgamouse->integer) {
#ifdef XF86DGA
					int MajorVersion, MinorVersion;

					if (XF86DGAQueryVersion (x11display.dpy, &MajorVersion, &MinorVersion)) {
						XF86DGADirectVideo (x11display.dpy, x11display.scr, XF86DGADirectMouse);
						DiscardEvents(MotionNotify);
						dgamouse = qTrue;
					}
					else {
						// Unable to query, probalby not supported
						Com_Printf (0, "Failed to detect XF86DGA Mouse\n");
						Cvar_Set ("in_dgamouse", "0", qTrue);
						dgamouse = qFalse;
					}
#else
					Com_Printf (0, "DGA Mouse support not available\n" );
					dgamouse = qFalse;
#endif
				}

				mx = 0;
				my = 0;
				old_mouse_x = 0;
				old_mouse_y = 0;
			}
			else {
				// Failed to grab the mouse, probably the window was activated by clicking on the title bar, try again later
				mg = qFalse;
			}
		}
		else if (x11display.dpy && x11display.grab_win) {
			if (dgamouse) {
				dgamouse = qFalse;
#ifdef XF86DGA
				XF86DGADirectVideo (x11display.dpy, x11display.scr, 0);
#endif
			}
			XUngrabPointer (x11display.dpy, CurrentTime);
		}
	}

	if ((x11display.focused && mouse_grabbed) != mv) {
		// Mouse visibility (and wrapping) is always as requested
		mv = !mv;
		if (mv) {
			if (!cr)
				cr = CreateNullCursor();
			XDefineCursor (x11display.dpy, x11display.grab_win, cr);
		}
		else if (x11display.grab_win) {
			XUndefineCursor (x11display.dpy, x11display.grab_win);
		}
	}

	if ((x11display.focused && keybd_grabbed) != kg) {
		kg = !kg;
		if (kg) {
			if (XGrabKeyboard (x11display.dpy, x11display.win, False, GrabModeAsync, GrabModeAsync, CurrentTime)) /* x11display.win always handles focus and keyboard */
				kg = qFalse; /* if keyboard was not available try again later */
		}
		else
			XUngrabKeyboard (x11display.dpy, CurrentTime);
	}
}

static void X11_SetKMGrab (qBool kg, qBool mg)
{
	keybd_grabbed = kg;
	mouse_grabbed = mg;

	if (x11display.dpy)
		updategrabstatus();
}

qBool X11_SetVideoMode(int width, int height, int fullscreen)
{
  int                  screen_width, screen_height, screen_mode;
  float                ratio;
  XSetWindowAttributes wa;
  unsigned long        mask;

  ScanUnmappedKeys();

  screen_width = width;
  screen_height = height;

  if (!x11display.win) {
    wa.background_pixel = 0;
    wa.border_pixel = 0;
    wa.event_mask = INIT_MASK | FocusChangeMask;

    mask = CWBackPixel | CWBorderPixel | CWEventMask;
    x11display.win = XCreateWindow (x11display.dpy, x11display.root, 0, 0, screen_width, screen_height,
                                    0, CopyFromParent, InputOutput, CopyFromParent, mask, &wa);
    XSetStandardProperties (x11display.dpy, x11display.win, APPLICATION, None, None, NULL, 0, NULL);
    x11display.wmDeleteWindow = XInternAtom(x11display.dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols (x11display.dpy, x11display.win, &x11display.wmDeleteWindow, 1);
  }

  if (fullscreen) {
    SCR_VidmodesFindBest (&screen_mode, &screen_width, &screen_height);

    if (screen_mode < 0) return qFalse;

    if (screen_width < width || screen_height < height) {
      if (width > height) {
        ratio = width / height;
        height = height *ratio;
        width = screen_width;
      } else {
        ratio = height / width;
        width = width *ratio;
        height = screen_height;
      }
    }

    /* Create fulscreen window */
    wa.background_pixel = 0;
    wa.border_pixel = 0;
    wa.event_mask = INIT_MASK;
    wa.override_redirect = True;
    mask = CWBackPixel | CWBorderPixel | CWEventMask | CWOverrideRedirect;

    if (!x11display.fs_win)
      x11display.fs_win = XCreateWindow(x11display.dpy, x11display.root, 0, 0, screen_width, screen_height,
                                        0, CopyFromParent, InputOutput, CopyFromParent, mask, &wa);

    XResizeWindow (x11display.dpy, x11display.gl_win, width, height);
    XReparentWindow (x11display.dpy, x11display.gl_win, x11display.fs_win, (screen_width /2) -(width /2), (screen_height /2) - (height /2));

    XMapWindow (x11display.dpy, x11display.gl_win);
    XMapRaised (x11display.dpy, x11display.win);

    X11_SetNoResize (x11display.fs_win, screen_width, screen_height);

    x11display.grab_win = x11display.fs_win;

    x11display.fullscreenmode = screen_mode+1;
  } else {

    x11display.fullscreenmode = 0;

    SCR_VidmodesSwitchBack();

    SetWindowInitialState(x11display.dpy, x11display.win, NormalState);
    XResizeWindow (x11display.dpy, x11display.gl_win, width, height);
    XReparentWindow (x11display.dpy, x11display.gl_win, x11display.win, 0, 0);
    XMapWindow (x11display.dpy, x11display.gl_win);
    XMapWindow (x11display.dpy, x11display.win);
	
    X11_SetNoResize(x11display.win, width, height);

    x11display.grab_win = x11display.win;
  }

  // save the parent window size for mouse use.
  // this is not the gl context window
  x11display.win_width = width;
  x11display.win_height = height;

  XFlush (x11display.dpy);

  return qTrue;
}

static void X11_FocusEvent(qBool focused)
{
	if (focused == x11display.focused)
		return;

	if (focused) {
		x11display.focused = qTrue;
		if (x11display.fullscreenmode) {
			SCR_VidmodesSwitch (x11display.fullscreenmode-1);
			XMapRaised (x11display.dpy, x11display.win);
			XMapRaised (x11display.dpy, x11display.fs_win);
		}

		updategrabstatus ();

		// Ignore all remaining events to avoid some WM messed up focus events
		XSync (x11display.dpy, 1);

		// After syncing, raise the window again
		// This fixes managed windows over fullscreen windows in some WMs
		if (x11display.fullscreenmode)
			XRaiseWindow (x11display.dpy, x11display.fs_win);
	}
	else {
		x11display.focused = qFalse;
		updategrabstatus ();

		if (x11display.fullscreenmode) {
			XIconifyWindow(x11display.dpy, x11display.win, DefaultScreen(x11display.dpy));
			XUnmapWindow(x11display.dpy, x11display.fs_win);
			SCR_VidmodesSwitchBack();
		}

		// Ignore all remaining events to avoid some WM messed up focus events
		XSync (x11display.dpy, 1);
	}
}


void HandleEvents(void)
{
	XEvent	event;
	qBool	dowarp = qFalse;
	int	mwx = x11display.win_width / 2;
	int	mwy = x11display.win_height / 2;

	if (!x11display.dpy)
		return;
	while (XPending(x11display.dpy)) {
		XNextEvent (x11display.dpy, &event);

		switch (event.type) {
		case KeyPress:
		case KeyRelease:
			// Ignore shift and other modifiers for now, client/keys.c will do that
			event.xkey.state = 0;
			Key_Event (XLateKey(&event.xkey), event.type == KeyPress, Sys_Milliseconds());
			break;

		case MotionNotify:
			if (!mouse_grabbed) {
				mx = event.xmotion.x - p_mouse_x;
				my = event.xmotion.y - p_mouse_y;
				p_mouse_x = event.xmotion.x;
				p_mouse_y = event.xmotion.y;
			}
			else if (dgamouse) {
				mx += event.xmotion.x_root * 2;
				my += event.xmotion.y_root * 2;
			}
			else {
				if (!event.xmotion.send_event) {
					mx += event.xmotion.x - p_mouse_x;
					my += event.xmotion.y - p_mouse_y;

					if(abs (mwx - event.xmotion.x) > mwx/2 || abs (mwy - event.xmotion.y) > mwy/2)
						dowarp = qTrue;
				}

				p_mouse_x = event.xmotion.x;
				p_mouse_y = event.xmotion.y;
			}
			break;

		case ButtonPress:
			switch (event.xbutton.button) {
			case 1:	Key_Event (K_MOUSE1, qTrue, Sys_Milliseconds ());	break;
			case 2:	Key_Event (K_MOUSE3, qTrue, Sys_Milliseconds ());	break;
			case 3:	Key_Event (K_MOUSE2, qTrue, Sys_Milliseconds ());	break;
			case 4:	Key_Event (K_MWHEELUP, qTrue, Sys_Milliseconds ());	break;
			case 5:	Key_Event (K_MWHEELDOWN, qTrue, Sys_Milliseconds ());	break;
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
				Key_Event (K_MOUSE4+event.xbutton.button-6, qTrue, Sys_Milliseconds ());
				break;
			}
			break;

		case ButtonRelease:
			switch (event.xbutton.button) {
			case 1:	Key_Event (K_MOUSE1, qFalse, Sys_Milliseconds ());	break;
			case 2:	Key_Event (K_MOUSE3, qFalse, Sys_Milliseconds ());	break;
			case 3:	Key_Event (K_MOUSE2, qFalse, Sys_Milliseconds ());	break;
			case 4:	Key_Event (K_MWHEELUP, qFalse, Sys_Milliseconds ());	break;
			case 5:	Key_Event (K_MWHEELDOWN, qFalse, Sys_Milliseconds ());	break;
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
				Key_Event (K_MOUSE4+event.xbutton.button-6, qFalse, Sys_Milliseconds ());
				break;
			}
			break;

		case FocusIn:
			X11_FocusEvent(qTrue);
			break;

		case FocusOut:  // let the UglyFocus code check if this is correct
		case EnterNotify:
		case LeaveNotify: {
			Window w;
			int    r;

			// Check focus since WM that lets the X server change the focus dinamically or no WM doesn't send FocusIn or FocusOut events
			XGetInputFocus (x11display.dpy, &w, &r);
			if (w == 1 && (r == RevertToPointerRoot || r == RevertToNone)
			|| w == x11display.fs_win || w == x11display.win) {
				X11_FocusEvent(qTrue);
			}
			else if (x11display.focused) {
				X11_FocusEvent (qFalse);
				Key_ClearStates (); // Don't allow keys to remain pressed if some other program steals focus
			}
			break;
		}
		case ClientMessage:
			if (event.xclient.data.l[0] == x11display.wmDeleteWindow) 
				Cmd_ExecuteString ("quit");
			break;
		}
	}

	if (dowarp && x11display.grab_win && x11display.focused) {
		// Move the mouse to the window center again
		p_mouse_x = mwx;
		p_mouse_y = mwy;
		XWarpPointer (x11display.dpy, None, x11display.grab_win, 0, 0, 0, 0, mwx, mwy);
	}
}

/*
=============================================================================

	GL CONTEXT AND RENDER WINDOW

=============================================================================
*/

static void X11_DisplayNeeded();

qBool X11_CreateGLContext()
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

	struct {
		char *name;
		int  *attributes;
	} attributes_list[] = {
		{"rbga, double-buffered, 24bits depth and 8bit stencil", attributes_0},
		{"rbga, double-buffered and 24bits depth", attributes_1},
		{0, 0}
	};

	XSetWindowAttributes	attr;
	unsigned long		mask;

	Com_Printf (0, "Display initialization\n");

	X11_DisplayNeeded ();

	if (x11display.visinfo) {
		XFree (x11display.visinfo);
		x11display.visinfo = NULL;
	}

	for (i =0; attributes_list[i].name; i++){
		x11display.visinfo = qglXChooseVisual (x11display.dpy, x11display.scr, attributes_list[i].attributes);
		if (!x11display.visinfo){
			Com_Printf (0, "..Failed to get %s\n", attributes_list[i].name);
		}
		else {
			Com_Printf (0, "..Get %s\n", attributes_list[i].name);
			break;
		}
	}

	if (!x11display.visinfo)
		return qFalse;

	x11display.ctx = qglXCreateContext(x11display.dpy, x11display.visinfo, NULL, True);
	x11display.cmap = XCreateColormap(x11display.dpy, x11display.root, x11display.visinfo->visual, AllocNone);
	if (!x11display.cmap) {
		Com_Printf (0, "..Failed to create colormap\n");
		return qFalse;
	}

	attr.colormap = x11display.cmap;
	attr.border_pixel = 0;
	attr.event_mask = DISPLAY_MASK;
	attr.override_redirect = False;
	mask = CWBorderPixel | CWColormap;

	x11display.gl_win = XCreateWindow(x11display.dpy, x11display.root, 0, 0, 1, 1, 0, x11display.visinfo->depth, InputOutput, x11display.visinfo->visual, mask, &attr);
	if (!x11display.gl_win) {
		Com_Printf (0, "..Failed to create render window\n");
		return qFalse;
	}

	return qglXMakeCurrent(x11display.dpy, x11display.gl_win, x11display.ctx);
}

int X11_GetGLAttribute(int attr)
{
	// FIXME
	int (*qglXGetConfig)(Display*, XVisualInfo*, int, int*) = dlsym(NULL, "glXGetConfig");

	if (!qglXGetConfig || !x11display.dpy || !x11display.visinfo)
		return -1;
	if (qglXGetConfig (x11display.dpy, x11display.visinfo, attr, &attr))
		return -1;

	return attr;
}

void X11_DestroyGLContext(void)
{
	X11_SetKMGrab (qFalse, qFalse);
	dgamouse = qFalse;

	if (x11display.dpy) {
		SCR_VidmodesSwitchBack ();

		if (x11display.gl_win) {
			XDestroyWindow (x11display.dpy, x11display.gl_win);
			x11display.gl_win = 0;
		}

		if (x11display.ctx) {
			qglXMakeCurrent(x11display.dpy, None, NULL);
			qglXDestroyContext (x11display.dpy, x11display.ctx);
			x11display.ctx = NULL;
		}

		if (x11display.cmap) {
			XFreeColormap(x11display.dpy, x11display.cmap);
			x11display.cmap = 0;
		}

		if (x11display.visinfo) {
			XFree(x11display.visinfo);
			x11display.visinfo = NULL;
		}


		if (x11display.fs_win) {
			XUnmapWindow(x11display.dpy, x11display.fs_win);
			x11display.focused = qFalse;
		}

		// Managed window is kept to avoid losing window manager information like stay on top, desktop, position on vid_restart.
		XFlush(x11display.dpy);
	}
}

void X11_SwapBuffers (void)
{
	qglXSwapBuffers (x11display.dpy, x11display.gl_win);
}

/*
=============================================================================

	DISPLAY

=============================================================================
*/

void X11_Shutdown (void)
{
	if (x11display.dpy) {
		SCR_VidmodesSwitchBack ();
		SCR_VidmodesFree ();

		X11_DestroyGLContext ();

		if (x11display.win) {
			XDestroyWindow (x11display.dpy, x11display.win);
			x11display.win = 0;
		}
		if (x11display.fs_win) {
			XDestroyWindow (x11display.dpy, x11display.fs_win);
			x11display.fs_win = 0;
		}

		XFlush (x11display.dpy);
		XCloseDisplay (x11display.dpy);

		x11display.dpy = NULL;
	}
}


static void X11_DisplayNeeded()
{
	if (!x11display.dpy) {
		memset (&x11display, 0, sizeof (x11display)); /* clear any older or uninitialized values */

		x11display.dpy = XOpenDisplay (NULL);
		if (!x11display.dpy) {
			Com_Printf (0, "..Error couldn't open the X display\n");
			exit(1);
		}

		SCR_VidmodesInit();
	}

	x11display.scr = DefaultScreen (x11display.dpy);
	x11display.root = RootWindow (x11display.dpy, x11display.scr);
}

/*
=============================================================================

	INPUT

=============================================================================
*/

void Force_CenterView_f (void)
{
	cl.viewAngles[PITCH] = 0;
}

void IN_Init(void)
{
	// compatibility variables
	in_mouse = Cvar_Register ("in_mouse", "1", CVAR_ARCHIVE);
		   Cvar_Register ("m_forward", "1", 0);
		   Cvar_Register ("m_side", "0.8", 0);
	freelook = Cvar_Register ("cl_freelook", "0", 0);

	// mouse variables
	m_filter = Cvar_Register ("m_filter", "0", 0);
	in_dgamouse = Cvar_Register ("in_dgamouse", "1", CVAR_ARCHIVE);
	lookstrafe = Cvar_Register ("lookstrafe", "0", 0);
	sensitivity = Cvar_Register ("sensitivity", "3", 0);
	m_pitch = Cvar_Register ("m_pitch", "0.022", 0);
	m_yaw = Cvar_Register ("m_yaw", "0.022", 0);

	Cmd_AddCommand ("force_centerview", Force_CenterView_f, "Force the screen to a center view");

	mx = my = 0.0;
}

void IN_Shutdown (void)
{
	Cmd_RemoveCommand ("force_centerview", NULL);
}


void IN_Commands (void)
{
	HandleEvents ();
}

void IN_Move (userCmd_t *cmd)
{
	int kd = Key_GetDest ();

	switch (kd) {
	case KD_MENU:
	case KD_CONSOLE:
		if (keybd_grabbed && x11display.gl_win)
			X11_SetKMGrab (qFalse, qTrue);

		if (kd == KD_MENU)
			CL_CGModule_MoveMouse (mx, my);
		mx = my = 0;
		return;
	}

	if (!keybd_grabbed && x11display.gl_win)
		X11_SetKMGrab (qTrue, qTrue);

	if (m_filter->integer) {
		mx = (mx + old_mouse_x) * 0.5f;
		my = (my + old_mouse_y) * 0.5f;
	}
  
	old_mouse_x = mx;
	old_mouse_y = my;
  
	mx *= sensitivity->value;
	my *= sensitivity->value;
  
	cl.viewAngles[YAW] -= m_yaw->value * mx;
	cl.viewAngles[PITCH] += m_pitch->value * my;
    
	mx = my = 0;
}

void IN_Frame (void)
{
	if (!x11display.dpy)
		return;

	// Reset the time remaining to avoid the screensaver while playing
	XResetScreenSaver (x11display.dpy);
	HandleEvents ();
}
