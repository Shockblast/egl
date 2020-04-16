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
// sdl_glimp.c
// This file contains GL context related code and some glue
//

#include <unistd.h>
#include <limits.h>

#include "SDL.h"
#include "SDL_syswm.h"

#include <X11/Xlib.h>

#include "../renderer/r_local.h"
#include "../client/cl_local.h"
#include "../unix/unix_glimp.h"
#include "../unix/unix_local.h"

qBool   vid_queueRestart,
        SDL_active = qFalse,
        mouse_avail = qFalse,
        mouse_active = qFalse,
        mlooking = qFalse,
        have_stencil = qFalse;

cVar_t  *mouse_grabbed,
        *vid_fullscreen,
        *vid_gamma,
        *r_hwGamma;

int     mouse_buttonstate,
        mouse_oldbuttonstate;

float   windowed_mouse;

SDL_Surface *surface;

glxState_t glxState = {.OpenGLLib = NULL};

void Force_CenterView_f (void)
{
	cl.viewAngles[PITCH] = 0;
}

void InstallGrabs(void)
{
	SDL_WM_GrabInput(SDL_GRAB_ON);
	SDL_ShowCursor(SDL_DISABLE);
}

void UnInstallGrabs(void)
{
	SDL_ShowCursor(SDL_ENABLE);
	SDL_WM_GrabInput(SDL_GRAB_OFF);
}

void IN_DeactivateMouse( void ) 
{
	if (!mouse_avail)
		return;

	if (mouse_active) {
		UnInstallGrabs();
		mouse_active = qFalse;
	}
}

void IN_ActivateMouse( void ) 
{
	if (!mouse_avail)
		return;

	if (!mouse_active) {
		InstallGrabs();
		mouse_active = qTrue;
	}
}

void IN_Activate (qBool active)
{
	if (active)
		IN_ActivateMouse ();
	else
		IN_DeactivateMouse ();
}

int XLateKey(unsigned int keysym)
{
	int key;
	
	key = 0;
	switch(keysym) {
		case SDLK_KP9:		key = K_KP_PGUP;       break;
		case SDLK_PAGEUP:	key = K_PGUP;          break;
		case SDLK_KP3:		key = K_KP_PGDN;       break;
		case SDLK_PAGEDOWN:	key = K_PGDN;          break;
		case SDLK_KP7:		key = K_KP_HOME;       break;
		case SDLK_HOME:		key = K_HOME;          break;
		case SDLK_KP1:		key = K_KP_END;        break;
		case SDLK_END:		key = K_END;           break;
		case SDLK_KP4:		key = K_KP_LEFTARROW;  break;
		case SDLK_LEFT:		key = K_LEFTARROW;     break;
		case SDLK_KP6:		key = K_KP_RIGHTARROW; break;
		case SDLK_RIGHT:	key = K_RIGHTARROW;    break;
		case SDLK_KP2:		key = K_KP_DOWNARROW;  break;
		case SDLK_DOWN:		key = K_DOWNARROW;     break;
		case SDLK_KP8:		key = K_KP_UPARROW;    break;
		case SDLK_UP:		key = K_UPARROW;       break;
		case SDLK_ESCAPE:	key = K_ESCAPE;        break;
		case SDLK_KP_ENTER:	key = K_KP_ENTER;      break;
		case SDLK_RETURN:	key = K_ENTER;         break;
		case SDLK_TAB:		key = K_TAB;           break;
		case SDLK_F1:		key = K_F1;            break;
		case SDLK_F2:		key = K_F2;            break;
		case SDLK_F3:		key = K_F3;            break;
		case SDLK_F4:		key = K_F4;            break;
		case SDLK_F5:		key = K_F5;            break;
		case SDLK_F6:		key = K_F6;            break;
		case SDLK_F7:		key = K_F7;            break;
		case SDLK_F8:		key = K_F8;            break;
		case SDLK_F9:		key = K_F9;            break;
		case SDLK_F10:		key = K_F10;           break;
		case SDLK_F11:		key = K_F11;           break;
		case SDLK_F12:		key = K_F12;           break;
		case SDLK_BACKSPACE:	key = K_BACKSPACE;     break;
		case SDLK_KP_PERIOD:	key = K_KP_DEL;        break;
		case SDLK_DELETE:	key = K_DEL;           break;
		case SDLK_PAUSE:	key = K_PAUSE;         break;
		case SDLK_LSHIFT:
		case SDLK_RSHIFT:	key = K_SHIFT;         break;
		case SDLK_LCTRL:
		case SDLK_RCTRL:	key = K_CTRL;          break;
		case SDLK_LMETA:
		case SDLK_RMETA:
		case SDLK_LALT:
		case SDLK_RALT:		key = K_ALT;           break;
		case SDLK_INSERT:	key = K_INS;           break;
		case SDLK_KP0:		key = K_KP_INS;        break;
		case SDLK_KP_MULTIPLY:	key = '*';             break;
		case SDLK_KP_PLUS:	key = K_KP_PLUS;       break;
		case SDLK_KP_MINUS:	key = K_KP_MINUS;      break;
		case SDLK_KP_DIVIDE:	key = K_KP_SLASH;      break;
		case SDLK_WORLD_7:	key = '`';             break;
		
		case 92:		key = '~';	       break; /* Italy */
		case 94:		key = '~';	       break; /* Deutschland */
		case 178:		key = '~'; 	       break; /* France */
		case 186:		key = '~'; 	       break; /* Spain */
		
		default: /* assuming that the other sdl keys are mapped to ascii */ 
			if (keysym < 256)
				key = keysym;
			break;
	}

	return key;
}

void GetEvent(SDL_Event *event)
{
	int key;
	
	switch(event->type) {
		
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			/* convert from SDL keysym to engine enum */
			key = XLateKey(event->key.keysym.sym);
			/* send the event to the engine */
			Key_Event(key, event->key.state, Sys_Milliseconds());
			
			/* special cases */
			if(event->type == SDL_KEYDOWN)
			{
				/* Alt + Return : toggle fullscreen */
				if((event->key.keysym.mod & KMOD_ALT) && event->key.keysym.sym == SDLK_RETURN)
				{
					cVar_t	*fullscreen;
					
					SDL_WM_ToggleFullScreen(surface);
					
					if (surface->flags & SDL_FULLSCREEN) 
						Cvar_SetValue( "vid_fullscreen", 1, qTrue );
					else 
						Cvar_SetValue( "vid_fullscreen", 0, qFalse );
					
					fullscreen = Cvar_Register( "vid_fullscreen", "0", 0);
					fullscreen->modified = qFalse;
				}
				/* Ctrl + g : toggle mouse grab */
				else if((event->key.keysym.mod & KMOD_CTRL) && event->key.keysym.sym == SDLK_g)
				{
					SDL_GrabMode gm = SDL_WM_GrabInput(SDL_GRAB_QUERY);	
					Cvar_SetValue( "mouse_grabbed", (gm == SDL_GRAB_ON) ? 0 : 1, qTrue  );
				}
			}
		break;
		
		case SDL_MOUSEMOTION:
			/*
			 * this if is needed to avoid the move event from SDL_WarpMouse()
			 * see comments from HandleEvents()
			 */
			if(event->motion.x != ri.config.vidWidth / 2 || event->motion.y != ri.config.vidHeight / 2)
			{
				CL_MoveMouse(event->motion.xrel, event->motion.yrel);
			}
			break;
		
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			if(event->button.button <= 3) /* buttons 1,2,3 */
			{
				Key_Event(K_MOUSE1 + event->button.button - 1, event->button.state, Sys_Milliseconds() );
			}
			else if(event->button.button == 4) /* buttons 4 = wheelup */
			{
				Key_Event(K_MWHEELUP, event->button.state, Sys_Milliseconds() );
			}
			else if(event->button.button == 5) /* buttons 5 = wheeldown */
			{
				Key_Event(K_MWHEELDOWN, event->button.state, Sys_Milliseconds() );
			}
			/* other buttons : no K_MOUSEn for them! one could add some */
			break;
		
		case SDL_QUIT:
			Sys_Quit(1);
			break;
	}
}

void HandleEvents(void)
{
	SDL_Event event;
	
	if (SDL_active) {
		while (SDL_PollEvent(&event))
			GetEvent(&event);
		
		if(windowed_mouse)
		{
			/*
			 * center the mouse on the screen to avoid the borders
			 * This shouln't happend with SDL with an hidden and grabbed cursor !?!
			 * (see the SDL_MouseMotionEvent documantation)
			 */
			SDL_WarpMouse(ri.config.vidWidth / 2, ri.config.vidHeight / 2);
		}
		
		/* if mouse grab changed, apply it */
		if (windowed_mouse != mouse_grabbed->intVal) {
			windowed_mouse = mouse_grabbed->intVal;	
			if (!mouse_grabbed->intVal) UnInstallGrabs();
			else InstallGrabs();
		}
	}
}

void IN_Commands (void)
{
	HandleEvents();
}

void IN_Move (userCmd_t *cmd)
{
	if (!mouse_avail)
		return;
}

static void *cmd_force_centerview_f;
void IN_Init (void)
{
	// Mouse variables
	mouse_grabbed	= Cvar_Register ("mouse_grabbed", "1", CVAR_ARCHIVE);

	cmd_force_centerview_f = Cmd_AddCommand ("force_centerview", Force_CenterView_f, "Force the screen to a center view");
    
	if (mouse_avail) mouse_avail = qTrue;
}

void IN_Shutdown (void)
{
	if (mouse_avail) mouse_avail = qFalse;
	Cmd_RemoveCommand ("force_centerview", cmd_force_centerview_f);
}

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
	SDL_GL_SwapBuffers();
}

/*
==============
VID_Restart_f

Console command to re-start the video mode and refresh DLL. We do this
simply by setting the modified flag for the vid_ref variable, which will
cause the entire video mode and refresh DLL to be reset on the next frame.
==============
*/
void VID_Restart_f (void)
{
	vid_queueRestart = qTrue;
}

/*
============
ListRemaps_f
Console command to list all keys mapped to AUX%d
============
*/
static void ListRemaps_f (void)
{
}


/*
============
VID_CheckChanges

This function gets called once just before drawing each frame, and it's sole purpose in life
is to check to see if any of the video mode parameters have changed, and if they have to 
update the rendering DLL and/or video mode to match.
============
*/
void VID_CheckChanges (refConfig_t *outConfig)
{
	int errNum;

	while (vid_queueRestart) {
		qBool cgWasActive = cls.mapLoaded;

		CL_MediaShutdown ();

		// Refresh has changed
		vid_queueRestart = qFalse;
		cls.refreshPrepped = qFalse;
		cls.disableScreen = qTrue;

		// Kill if already active
		if (SDL_active) {
			R_Shutdown (qFalse);
			SDL_active = qFalse;
		}

		// Initialize renderer
		errNum = R_Init ();

		// Refresh init failed!
		if (errNum != R_INIT_SUCCESS) {
			R_Shutdown (qTrue);
			SDL_active = qFalse;

			switch (errNum) {
			case R_INIT_QGL_FAIL:
				Com_Error (ERR_FATAL, "Couldn't initialize OpenGL!\n" "QGL library failure!");
				break;

			case R_INIT_OS_FAIL:
				Com_Error (ERR_FATAL, "Couldn't initialize OpenGL!\n" "Incorrect operating system!");
				break;

			case R_INIT_MODE_FAIL:
				Com_Error (ERR_FATAL, "Couldn't initialize OpenGL!\n" "Couldn't set video mode!");
				break;
			}
		}

		R_GetRefConfig (outConfig);

		Snd_Init ();
		CL_MediaInit ();

		cls.disableScreen = qFalse;

		CL_ConsoleClose ();

		// This is to stop cgame from initializing on first load
		// and so it will load after a vid_restart while connected somewhere
		if (cgWasActive) {
			CL_CGModule_LoadMap ();
			Key_SetDest (KD_GAME);
		}
		else {
			CL_CGModule_MainMenu ();
		}
		
		InstallGrabs();
		
		SDL_active = qTrue;
	}
}

static void *cmd_vid_restart_f;
static void *cmd_listremaps_f;
/*
============
VID_Init
============
*/
void VID_Init (refConfig_t *outConfig)
{
	
	if (SDL_WasInit(SDL_INIT_AUDIO|SDL_INIT_CDROM|SDL_INIT_VIDEO) == 0) {
		if (SDL_Init(SDL_INIT_VIDEO) < 0) {
			Sys_Error("SDL Init failed: %s\n", SDL_GetError());
		}
	} else if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
			Sys_Error("SDL Init failed: %s\n", SDL_GetError());
		}
	}
	
	vid_fullscreen = Cvar_Register ("vid_fullscreen", "1", CVAR_ARCHIVE);
	vid_gamma = Cvar_Register ("vid_gamma", "0.6", CVAR_ARCHIVE);
	r_hwGamma = Cvar_Register ("r_hwGamma", "1", CVAR_ARCHIVE);
	mouse_grabbed = Cvar_Register ("mouse_grabbed", "1", CVAR_ARCHIVE);

	// Add some console commands that we want to handle
	cmd_vid_restart_f = Cmd_AddCommand ("vid_restart", VID_Restart_f, "Restarts refresh and media");
	cmd_listremaps_f = Cmd_AddCommand ("listremaps", ListRemaps_f, "Lists what keys are remapped to AUX* bindings");

	// Start the graphics mode and load refresh DLL
	SDL_active = qFalse;
	vid_fullscreen->modified = qTrue;
	vid_queueRestart = qTrue;

	VID_CheckChanges (outConfig);
}

/*
============
VID_Shutdown
============
*/
void VID_Shutdown (void)
{
	if(SDL_active) {
		R_Shutdown (qTrue);
		SDL_active = qFalse;
	}

	Cmd_RemoveCommand ("vid_restart", cmd_vid_restart_f);
	Cmd_RemoveCommand ("listremaps", cmd_listremaps_f);
}

/*
=================
GLimp_Shutdown
=================
*/
void GLimp_Shutdown (qBool full)
{	
	if (surface)
		SDL_FreeSurface(surface);
	surface = NULL;
	
	if (SDL_WasInit(SDL_INIT_EVERYTHING) == SDL_INIT_VIDEO)
		SDL_Quit();
	else
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		
	SDL_active = qFalse;
}

static void SDL_Version(void)
{
	const SDL_version *version;
	version = SDL_Linked_Version();
        
        Com_Printf(0, "Linked against SDL version %d.%d.%d\n" 
                      "Using SDL library version %d.%d.%d\n",
                      SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL,
                      version->major, version->minor, version->patch);
}

/*
=================
GLimp_Init
=================
*/
qBool GLimp_Init( void )
{
	int	alphaBits,
		colorBits,
		depthBits,
		stencilBits;
	int	r = 0,
		g = 0,
		b = 0;
	qBool	fullscreen;
	
	Com_Printf (0, "SDL-OpenGL Display initialization\n");
	
	SDL_Version();
	
	/* Just toggle fullscreen if that's all that has been changed */
	if (surface && (surface->w == ri.config.vidWidth) && (surface->h == ri.config.vidHeight)) {
		int isfullscreen = (surface->flags & SDL_FULLSCREEN) ? 1 : 0;
		if (fullscreen != isfullscreen)
			SDL_WM_ToggleFullScreen(surface);

		isfullscreen = (surface->flags & SDL_FULLSCREEN) ? 1 : 0;
		if (fullscreen == isfullscreen)
			return qTrue;
	}
	
	srandom(getpid());

	// free resources in use
	if (surface)
		SDL_FreeSurface(surface);
	
	
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE);
	SDL_SetVideoMode( 0, 0, 0, 0 );
	
	// Alpha bits
	alphaBits = r_alphabits->intVal;

	// Color bits
	colorBits = r_colorbits->intVal;
	if (colorBits == 0)
		colorBits = 24;

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
	
	if( colorBits == 24 || colorBits == 32 ) {
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		r = g = b = 8;
	}
	else if( colorBits == 16 ) {
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 4);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 4);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 4);
		r = g = b = 4;
	}
	
	ri.cColorBits = r+g+b;
	ri.cAlphaBits = SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, alphaBits );
	ri.cDepthBits = SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, depthBits );
	ri.cStencilBits = SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, stencilBits );
	
	if( colorBits == 24 || colorBits == 32 )
		Com_Printf( 0, "..Get rbga, double-buffered, %d/%d/%d Color bits, %dbits alpha, %dbits depth, %dbit stencil\n", r, g, b, alphaBits, depthBits, stencilBits);
	else if( colorBits == 16 ) 
		Com_Printf( 0, "..Get %d/%d/%d Color bits, %dbits depth, %dbit stencil\n", r, g, b, depthBits, stencilBits);
		
	SDL_WM_SetCaption("SDL-OpenGL - "WINDOW_APP_NAME, "SDL-OpenGL - "WINDOW_APP_NAME);
	SDL_active=qTrue;
	SDL_EnableKeyRepeat( SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL );
	SDL_EnableUNICODE(SDL_ENABLE);
	return qTrue;
}

qBool GLimp_AttemptMode (qBool fullScreen, int width, int height)
{
	ri.config.vidFullScreen = fullScreen;
	ri.config.vidWidth = width;
	ri.config.vidHeight = height;

	
	if (SDL_SetVideoMode(width, height, r_colorbits->intVal,
			SDL_OPENGL | (fullScreen == qTrue ? SDL_FULLSCREEN : 0)) == NULL) {
		Com_Printf(0," setting the video mode failed: %s", SDL_GetError());
		return qFalse;
	}

	Com_Printf (0, "Mode: %d x %d %s\n", width, height, fullScreen ? "(fullscreen)" : "(windowed)");
	return qTrue;

}

qBool GLimp_GetGammaRamp(uint16 *ramp)
{
	size_t stride = 256;
	return SDL_GetGammaRamp(ramp, ramp + stride, ramp + (stride << 1)) == -1 ? qFalse : qTrue;
}

void GLimp_SetGammaRamp(uint16 *ramp)
{
	size_t stride = 256;
	if (SDL_SetGammaRamp(ramp, ramp + stride, ramp + (stride << 1)) == -1)
		Com_Printf(0,"SDL_GLimp_SetGammaRamp failed: ", SDL_GetError());
}

void IN_Frame (void)
{
	if (!SDL_active) return;
//	HandleEvents ();
}

void X11_Shutdown (void)
{
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

/*
================
Sys_GetClipboardData
================
*/
char *Sys_GetClipboardData(void)
{
	Window sowner;
	Atom type, property;
	int format, result;
	unsigned long len, bytes_left, tmp;
	unsigned char *data;
	char *ret = NULL;
	SDL_SysWMinfo info;

	SDL_VERSION(&info.version);
	
	if (SDL_GetWMInfo(&info)) {
		if (info.subsystem == SDL_SYSWM_X11) {
			Display *SDL_Display = info.info.x11.display;
			Window SDL_Window = info.info.x11.window;

			/* Enable the special window hook events */
			SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

			sowner = XGetSelectionOwner(SDL_Display, XA_PRIMARY);
			if ((sowner == None) || (sowner == SDL_Window)) {
				sowner = DefaultRootWindow(SDL_Display);
				property = XA_CUT_BUFFER0;
			}
			else {
				int selection_response = 0;
				SDL_Event event;

				sowner = SDL_Window;
				property = XInternAtom(SDL_Display, "SDL_SELECTION", False);
				XConvertSelection(SDL_Display, XA_PRIMARY, XA_STRING,
						property, sowner, CurrentTime);
				while (!selection_response) {
					SDL_WaitEvent(&event);
					if (event.type == SDL_SYSWMEVENT) {
						XEvent xevent = event.syswm.msg->event.xevent;

						if ((xevent.type == SelectionNotify) &&
						    (xevent.xselection.requestor == sowner))
							selection_response = 1;
					}
				}
			}
			
			XFlush(SDL_Display);
			
			XGetWindowProperty(SDL_Display, sowner, property,
				           0, 0, False, XA_STRING,
				           &type, &format, &len,
				           &bytes_left, &data);
			
			if (bytes_left > 0) {
				result = XGetWindowProperty(SDL_Display, sowner, property, 
				                            0, INT_MAX/4, False, XA_STRING,
							    &type, &format, &len, 
							    &tmp, &data);
					if (result == Success) {
						ret = strdup((char *)data);
					}
					XFree(data);
			}
			
			SDL_EventState(SDL_SYSWMEVENT, SDL_DISABLE);
		}

	}
	return ret;
}

