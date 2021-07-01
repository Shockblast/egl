#ifndef __linux__
#  error You should not be including this file on this platform
#endif // __linux__

#ifndef __X11_LINUX_H__
#define __X11_LINUX_H__

#include <dlfcn.h> // ELF dl loader
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>


#ifdef XF86DGA
#include <X11/extensions/xf86dga.h>
#endif

#ifdef XF86VMODE
#include <X11/extensions/xf86vmode.h>
#endif

#include <GL/glx.h>

#include "../renderer/r_local.h"
#include "../client/cl_local.h"

/**** x11_main ****/

typedef struct x11display_s {
	Display		*dpy;
	int		scr;
	Window		root;
        Window		win;      /* managed window */
        Window		gl_win;   /* render window */
        Window		fs_win;   /* full screen unmanaged window */
        Window		grab_win; /* window that will grab the mouse events (either win or fs_win depending on fullscreen)*/
	Colormap	cmap;
	GLXContext	ctx;
	XVisualInfo	*visinfo;
	Atom		wmDeleteWindow;
	uint32		win_width, win_height;
	qBool		focused;
	int		fullscreenmode; /* > 0 when fullscreen */
} x11display_t;


extern x11display_t  x11display;

/**** x11_misc ****/

void SCR_VidmodesInit (void);
void SCR_VidmodesFree (void);

void SCR_VidmodesSwitch (int mode);
void SCR_VidmodesSwitchBack (void);
void SCR_VidmodesFindBest (int *mode, int *pwidth, int *pheight);

qBool SCR_GetGammaRamp (uint16 *ramp);
qBool SCR_SetGammaRamp (uint16 *ramp);

qBool X11_CreateGLContext (void);
int X11_GetGLAttribute (int attr);

void DiscardEvents (int event_type);
Cursor CreateNullCursor (void);
void X11_SetNoResize (Window w, int width, int height);

void SetWindowInitialState (Display *display, Window w, int state);

int XLateKey (XKeyEvent *ev);

int ScanUnmappedKeys (void);
char* GetAuxKeyRemapName (int index, int* kc);

#endif  // __X11_LINUX_H__
