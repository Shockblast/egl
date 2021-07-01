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
// Main windowed and fullscreen graphics interface module. This module
// is used for both the software and OpenGL rendering versions of the
// Quake refresh engine.

#include <assert.h>
#include <dlfcn.h> // ELF dl loader
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "../client/cl_local.h"
#include "../renderer/r_local.h"

#include "../linux/rw_linux.h" // kthx glimp_linux.* ?

// console variables that we need to access from this module
cvar_t			*vid_xpos;			// x coordinate of window position
cvar_t			*vid_ypos;			// y coordinate of window position
cvar_t			*vid_fullscreen;

// Global variables used internally by this module
void			*reflib_library;		// Handle to refresh DLL 

static qBool	vid_Active;
static qBool	vidRestart;

/** KEYBOARD **************************************************************/

void Do_Key_Event(int key, qboolean down);

void (*KBD_Update_fp)(void);
void (*KBD_Init_fp)(Key_Event_fp_t fp);
void (*KBD_Close_fp)(void);

/** MOUSE *****************************************************************/

in_state_t in_state;

void (*RW_IN_Init_fp)(in_state_t *in_state_p);
void (*RW_IN_Shutdown_fp)(void);
void (*RW_IN_Activate_fp)(qboolean active);
void (*RW_IN_Commands_fp)(void);
void (*RW_IN_Move_fp)(userCmd_t *cmd);
void (*RW_IN_Frame_fp)(void);

void Real_IN_Init (void);

//==========================================================================

/*
============
VID_Restart_f
============
*/
void VID_Restart_f (void) {
	vidRestart = qTrue;
}


void VID_FreeReflib (void) {
	if (vid_Active) {
		if (KBD_Close_fp)
			KBD_Close_fp ();
		if (RW_IN_Shutdown_fp)
			RW_IN_Shutdown_fp ();
	}

	KBD_Init_fp = NULL;
	KBD_Update_fp = NULL;
	KBD_Close_fp = NULL;
	RW_IN_Init_fp = NULL;
	RW_IN_Shutdown_fp = NULL;
	RW_IN_Activate_fp = NULL;
	RW_IN_Commands_fp = NULL;
	RW_IN_Move_fp = NULL;
	RW_IN_Frame_fp = NULL;

	vid_Active = qFalse;
}


/*
==============
VID_LoadRefresh
==============
*/
qboolean VID_LoadRefresh (char *name) {
	refimport_t	ri;
	GetRefAPI_t	GetRefAPI;
	char	fn[MAX_OSPATH];
	struct stat st;
	extern uid_t saved_euid;
	FILE *fp;
	
	if (vid_Active)
	{
		if (KBD_Close_fp)
			KBD_Close_fp();
		if (RW_IN_Shutdown_fp)
			RW_IN_Shutdown_fp();
		KBD_Close_fp = NULL;
		RW_IN_Shutdown_fp = NULL;
		R_Shutdown ();
		VID_FreeReflib ();
	}

	Com_Printf( "------- Loading %s -------\n", name );

	//regain root
	seteuid(saved_euid);

	if ((fp = fopen(so_file, "r")) == NULL) {
		Com_Printf( "LoadLibrary(\"%s\") failed: can't open %s (required for location of ref libraries)\n", name, so_file);
		return false;
	}
	fgets(fn, sizeof(fn), fp);
	fclose(fp);
	while (*fn && isspace(fn[strlen(fn) - 1]))
		fn[strlen(fn) - 1] = 0;

	strcat(fn, "/");
	strcat(fn, name);

	// permission checking
	if (strstr(fn, "softx") == NULL) { // softx doesn't require root
		if (stat(fn, &st) == -1) {
			Com_Printf( "LoadLibrary(\"%s\") failed: %s\n", name, strerror(errno));
			return false;
		}
#if 0
		if (st.st_uid != 0) {
			Com_Printf( "LoadLibrary(\"%s\") failed: ref is not owned by root\n", name);
			return false;
		}
		if ((st.st_mode & 0777) & ~0700) {
			Com_Printf( "LoadLibrary(\"%s\") failed: invalid permissions, must be 700 for security considerations\n", name);
			return false;
		}
#endif
	} else {
		// softx requires we give up root now
		setreuid(getuid(), getuid());
		setegid(getgid());
	}

	if ( ( reflib_library = dlopen( fn, RTLD_LAZY | RTLD_GLOBAL ) ) == 0 ) {
		Com_Printf( "LoadLibrary(\"%s\") failed: %s\n", name , dlerror());
		return false;
	}

	Com_Printf( "LoadLibrary(\"%s\")\n", fn );

	if ( ( GetRefAPI = (void *) dlsym( reflib_library, "GetRefAPI" ) ) == 0 )
		Com_Error( ERR_FATAL, "dlsym failed on %s", name );

	re = GetRefAPI( ri );

	if (re.api_version != API_VERSION) {
		VID_FreeReflib ();
		Com_Error (ERR_FATAL, "%s has incompatible apiVersion", name);
	}

	// Init IN (Mouse)
	in_state.IN_CenterView_fp = IN_CenterView;
	in_state.Key_Event_fp = Do_Key_Event;
	in_state.viewangles = cl.viewangles;
	in_state.in_strafe_state = &in_strafe.state;

	if ((RW_IN_Init_fp = dlsym(reflib_library, "RW_IN_Init")) == NULL ||
		(RW_IN_Shutdown_fp = dlsym(reflib_library, "RW_IN_Shutdown")) == NULL ||
		(RW_IN_Activate_fp = dlsym(reflib_library, "RW_IN_Activate")) == NULL ||
		(RW_IN_Commands_fp = dlsym(reflib_library, "RW_IN_Commands")) == NULL ||
		(RW_IN_Move_fp = dlsym(reflib_library, "RW_IN_Move")) == NULL ||
		(RW_IN_Frame_fp = dlsym(reflib_library, "RW_IN_Frame")) == NULL)
		Sys_Error("No RW_IN functions in REF.\n");

	Real_IN_Init();

	if (!R_Init (0, 0)) {
		// refresh init failed!
		R_Shutdown ();
		VID_FreeReflib ();
		return false;
	}

	// Init KBD
#if 1
	if ((KBD_Init_fp = dlsym(reflib_library, "KBD_Init")) == NULL ||
		(KBD_Update_fp = dlsym(reflib_library, "KBD_Update")) == NULL ||
		(KBD_Close_fp = dlsym(reflib_library, "KBD_Close")) == NULL)
		Sys_Error("No KBD functions in REF.\n");
#else
	{
		void KBD_Init(void);
		void KBD_Update(void);
		void KBD_Close(void);

		KBD_Init_fp = KBD_Init;
		KBD_Update_fp = KBD_Update;
		KBD_Close_fp = KBD_Close;
	}
#endif
	KBD_Init_fp (Do_Key_Event);

	// give up root now
	setreuid (getuid(), getuid());
	setegid (getgid());

	Com_Printf( "------------------------------------\n");
	vid_Active = qTrue;
	return true;
}

/*
============
VID_CheckChanges

This function gets called once just before drawing each frame, and it's sole purpose in life
is to check to see if any of the video mode parameters have changed, and if they have to 
update the rendering DLL and/or video mode to match.
============
*/
void VID_CheckChanges (void) {
	while (vidRestart) {
		Snd_StopAllSounds ();

		// refresh has changed
		vidRestart = qFalse;
		vid_fullscreen->modified = qTrue;
		cl.refreshPrepped = qFalse;
		cls.disableScreen = qTrue;

		// restart if already active
		VID_Shutdown ();

		if (!R_Init (0, 0)) {
			// refresh init failed!
			R_Shutdown ();
			vid_Active = qFalse;

			Com_Error (ERR_FATAL, "Couldn't initialize OpenGL!");
		}

		vid_Active = qTrue;
		cls.disableScreen = qFalse;
	}
}

/*
============
VID_Init
============
*/
void VID_Init (void) {
	// Create the video variables so we know how to start the graphics drivers
	vid_xpos = Cvar_Get ("vid_xpos", "3", CVAR_ARCHIVE);
	vid_ypos = Cvar_Get ("vid_ypos", "22", CVAR_ARCHIVE);
	vid_fullscreen = Cvar_Get ("vid_fullscreen", "0", CVAR_ARCHIVE);

	// Add some console commands that we want to handle
	Cmd_AddCommand ("vid_restart", VID_Restart_f);

	// Disable the 3Dfx splash screen
	putenv ("FX_GLIDE_NO_SPLASH=0");
		
	// Start the graphics mode and load refresh DLL
	vidRestart = qTrue;
	VID_CheckChanges ();
}

/*
============
VID_Shutdown
============
*/
void VID_Shutdown (void) {
	if (vid_Active) {
		if (KBD_Close_fp)
			KBD_Close_fp();
		if (RW_IN_Shutdown_fp)
			RW_IN_Shutdown_fp();
		KBD_Close_fp = NULL;
		RW_IN_Shutdown_fp = NULL;
		R_Shutdown ();
		VID_FreeReflib ();
	}
}


/*****************************************************************************/
/* INPUT                                                                     */
/*****************************************************************************/

cvar_t	*in_joystick;

// This is fake, it's acutally done by the Refresh load
void IN_Init (void) {
	in_joystick	= Cvar_Get ("in_joystick", "0", CVAR_ARCHIVE);
}

void Real_IN_Init (void) {
	if (RW_IN_Init_fp)
		RW_IN_Init_fp(&in_state);
}

void IN_Shutdown (void) {
	if (RW_IN_Shutdown_fp)
		RW_IN_Shutdown_fp();
}

void IN_Commands (void) {
	if (RW_IN_Commands_fp)
		RW_IN_Commands_fp();
}

void IN_Move (userCmd_t *cmd) {
	if (RW_IN_Move_fp)
		RW_IN_Move_fp(cmd);
}

void IN_Frame (void) {
	if (RW_IN_Activate_fp) {
		if ( !cl.refresh_prepped || cls.key_dest == KD_CONSOLE || cls.key_dest == key_menu)
			RW_IN_Activate_fp(false);
		else
			RW_IN_Activate_fp(true);
	}

	if (RW_IN_Frame_fp)
		RW_IN_Frame_fp();
}

void IN_Activate (qboolean active) {
}

void Do_Key_Event(int key, qboolean down) {
	Key_Event (key, down, Sys_Milliseconds ());
}

