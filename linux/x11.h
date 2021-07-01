#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include <X11/extensions/xf86dga.h>
#include <X11/extensions/xf86vmode.h>

#include <GL/glx.h>

typedef struct x11display_s
{
	Display		*dpy;
	int		scr;
	Window		root, win, gl_win, fs_win, grab_win;
	Colormap	cmap;
	GLXContext	ctx;
	XVisualInfo	*visinfo;
	Atom		wmDeleteWindow;

	uint32		win_width, win_height;
	char		focused;
} x11display_t;
