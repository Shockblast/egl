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

// sys_linux.c

#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <errno.h>
#include <mntent.h>

#include <dlfcn.h>

#include "../common/common.h"

#include "../linux/rw_linux.h"

cvar_t *nostdout;

unsigned int	sys_FrameTime;

uid_t	saved_euid;
qbool	stdin_active = true;

/*
================
Sys_Printf
================
*/
void Sys_Printf (char *fmt, ...) {
	va_list		argptr;
	char		text[1024];
	unsigned char		*p;

	va_start (argptr,fmt);
	vsprintf (text,fmt,argptr);
	va_end (argptr);

	if (strlen(text) > sizeof(text))
		Sys_Error("memory overwrite in Sys_Printf");

    if (nostdout && nostdout->value)
        return;

	for (p = (unsigned char *)text; *p; p++)
	{
		*p &= 0x7f;
		if (((*p > 128) || (*p < 32)) && (*p != 10) && (*p != 13) && (*p != 9))
			printf ("[%02x]", *p);
		else
			putc (*p, stdout);
	}
}


/*
================
Sys_Init
================
*/
void Sys_Init (void) {
}


/*
================
Sys_Quit
================
*/
void Sys_Quit (void) {
	CL_Shutdown ();
	Com_Shutdown ();
    fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
	_exit(0);
}


/*
================
Sys_Error
================
*/
void Sys_Error (char *error, ...) { 
    va_list     argptr;
    char        string[1024];

	/* change stdin to non blocking */
    fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);

	CL_Shutdown ();
	Com_Shutdown ();
    
    va_start (argptr,error);
    vsprintf (string,error,argptr);
    va_end (argptr);
	fprintf (stderr, "Error: %s\n", string);

	_exit (1);
}


/*
================
Sys_Warn
================
*/
void Sys_Warn (char *warning, ...) { 
    va_list     argptr;
    char        string[1024];
    
    va_start (argptr,warning);
    vsprintf (string,warning,argptr);
    va_end (argptr);
	fprintf (stderr, "Warning: %s", string);
} 


/*
============
Sys_FileTime

returns -1 if not present
============
*/
int	Sys_FileTime (char *path) {
	struct	stat	buf;
	
	if (stat (path,&buf) == -1)
		return -1;
	
	return buf.st_mtime;
}

void floating_point_exception_handler(int whatever) {
	signal(SIGFPE, floating_point_exception_handler);
}


/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput (void) {
    static char text[256];
    int     len;
	fd_set	fdset;
    struct timeval timeout;

	if (!dedicated || !dedicated->value)
		return NULL;

	if (!stdin_active)
		return NULL;

	FD_ZERO (&fdset);
	FD_SET (0, &fdset); // stdin
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	if (select (1, &fdset, NULL, NULL, &timeout) == -1 || !FD_ISSET(0, &fdset))
		return NULL;

	len = read (0, text, sizeof (text));
	if (len == 0) {
		/* eof */
		stdin_active = false;
		return NULL;
	}

	if (len < 1)
		return NULL;
	text[len-1] = 0;    // rip off the /n and terminate

	return text;
}


/*
================
Sys_ConsoleOutput
================
*/
void Sys_ConsoleOutput (char *string) {
	if (nostdout && nostdout->value)
		return;

	fputs(string, stdout);
}


/*
================
Sys_Milliseconds
================
*/
int		g_CurTime;
int Sys_Milliseconds (void) {
	struct timeval tp;
	struct timezone tzp;
	static int		secbase;

	gettimeofday(&tp, &tzp);
	
	if (!secbase) {
		secbase = tp.tv_sec;
		return tp.tv_usec/1000;
	}

	g_CurTime = (tp.tv_sec - secbase)*1000 + tp.tv_usec/1000;
	
	return g_CurTime;
}


/*
================
Sys_AppActivate
================
*/
void Sys_AppActivate (void) {
}


/*
================
Sys_Mkdir
================
*/
void Sys_Mkdir (char *path) {
    mkdir (path, 0777);
}


/*
================
Sys_SendKeyEvents
================
*/
void Sys_SendKeyEvents (void) {
#ifndef DEDICATED_ONLY
	KBD_Update ();
#endif

	/* grab frame time */
	sys_FrameTime = Sys_Milliseconds();
}


/*
================
Sys_GetClipboardData
================
*/
char *Sys_GetClipboardData (void) {
	return NULL;
}


/*
================
Sys_CopyProtect
================
*/
void Sys_CopyProtect (void) {
	FILE *mnt;
	struct mntent *ent;
	char path[MAX_OSPATH];
	struct stat st;
	qbool found_cd = false;

	static qbool checked = false;

	if (checked)
		return;

	if ((mnt = setmntent ("/etc/mtab", "r")) == NULL)
		Com_Error(ERR_FATAL, "Can't read mount table to determine mounted cd location.");

	while ((ent = getmntent(mnt)) != NULL) {
		if (strcmp(ent->mnt_type, "iso9660") == 0) {
			/* found a cd file system */
			found_cd = true;
			sprintf(path, "%s/%s", ent->mnt_dir, "install/data/quake2.exe");
			if (stat(path, &st) == 0) {
				/* found it */
				checked = true;
				endmntent(mnt);
				return;
			}
			sprintf(path, "%s/%s", ent->mnt_dir, "Install/Data/quake2.exe");
			if (stat(path, &st) == 0) {
				/* found it */
				checked = true;
				endmntent(mnt);
				return;
			}
			sprintf(path, "%s/%s", ent->mnt_dir, "quake2.exe");
			if (stat(path, &st) == 0) {
				/* found it */
				checked = true;
				endmntent(mnt);
				return;
			}
		}
	}
	endmntent (mnt);

	if (found_cd)
		Com_Error (ERR_FATAL, "Could not find a Quake2 CD in your CD drive.");
	Com_Error (ERR_FATAL, "Unable to find a mounted iso9660 file system.\n"
		"You must mount the Quake2 CD in a cdrom drive in order to play.");
}

// =====================================================================

static	char	findbase[MAX_OSPATH];
static	char	findpath[MAX_OSPATH];
static	char	findpattern[MAX_OSPATH];
static	DIR		*fdir;

static qbool CompareAttributes(char *path, char *name, unsigned musthave, unsigned canthave) {
	struct stat st;
	char fn[MAX_OSPATH];

	/* '.' and '..' never match */
	if ((strcmp (name, ".") == 0) || (strcmp (name, "..") == 0))
		return false;

	return true;

	if (stat(fn, &st) == -1)
		return false; // shouldn't happen

	if ((st.st_mode & S_IFDIR) && (canthave & SFF_SUBDIR))
		return false;

	if ((musthave & SFF_SUBDIR) && !(st.st_mode & S_IFDIR))
		return false;

	return true;
}


/*
==================
Sys_FindClose
==================
*/
void Sys_FindClose (void) {
	if (fdir != NULL)
		closedir (fdir);

	fdir = NULL;
}


/*
==================
Sys_FindFirst
==================
*/
char *Sys_FindFirst (char *path, unsigned musthave, unsigned canhave) {
	struct dirent *d;
	char *p;

	if (fdir)
		Sys_Error ("Sys_BeginFind without close");

	strcpy (findbase, path);

	if ((p = strrchr (findbase, '/')) != NULL) {
		*p = 0;
		strcpy (findpattern, p + 1);
	} else
		strcpy (findpattern, "*");

	if (strcmp (findpattern, "*.*") == 0)
		strcpy (findpattern, "*");
	
	if ((fdir = opendir (findbase)) == NULL)
		return NULL;
	while ((d = readdir (fdir)) != NULL) {
		if (!*findpattern || glob_match (findpattern, d->d_name)) {
			if (CompareAttributes (findbase, d->d_name, musthave, canhave)) {
				sprintf (findpath, "%s/%s", findbase, d->d_name);
				return findpath;
			}
		}
	}
	return NULL;
}


/*
==================
Sys_FindNext
==================
*/
char *Sys_FindNext (unsigned musthave, unsigned canhave) {
	struct dirent *d;

	if (fdir == NULL)
		return NULL;

	while ((d = readdir(fdir)) != NULL) {
		if (!*findpattern || glob_match (findpattern, d->d_name)) {
			if (CompareAttributes (findbase, d->d_name, musthave, canhave)) {
				sprintf (findpath, "%s/%s", findbase, d->d_name);
				return findpath;
			}
		}
	}
	return NULL;
}

/*
========================================================================

	LIBRARY MANAGEMENT

========================================================================
*/

static void *game_library = NULL;
static void *cgame_library = NULL;
static void *ui_library = NULL;

/*
=================
Sys_UnloadLibrary
=================
*/
void Sys_UnloadLibrary (int libType) {
	void **lib;

	switch (libType) {
	case LIB_CGAME:	lib = &cGameLib;	break;
	case LIB_GAME:	lib = &gameLib;		break;
	case LIB_UI:	lib = &uiLib;		break;
	default:
		Com_Printf (PRINT_ALL, S_COLOR_RED "Sys_UnloadLibrary: Bad libType\n");
		return;
	}

	if (dlclose (*lib))
		Com_Error (ERR_FATAL, "dlclose (%s) failed",
								(*lib == cGameLib) ? "LIB_CGAME" :
								(*lib == gameLib) ? "LIB_GAME" :
								(*lib == uiLib) ? "LIB_UI" :
								"UNKNOWN");

	*lib = NULL;
}

/*
=================
Sys_LoadLibrary
=================
*/
void *Sys_LoadLibrary (int libType, void *parms) {
	char name[MAX_OSPATH];
	char cwd[MAX_OSPATH];
	char *path;
	void *(*APIfunc) (void *);
	void **lib;
	char *libname;
	char *apifuncname;

#if defined __i386__
#define ARCH "i386"

#ifdef NDEBUG
	const char *debugdir = "releasei386";
#else
	const char *debugdir = "debugi386";
#endif
#elif defined __alpha__
#define ARCH "axp"
#ifdef NDEBUG
	const char *debugdir = "releaseaxp";
#else
	const char *debugdir = "debugaxp";
#endif

#elif defined __powerpc__
#define ARCH "axp"
#ifdef NDEBUG
	const char *debugdir = "releaseppc";
#else
	const char *debugdir = "debugppc";
#endif
#elif defined __sparc__
#define ARCH "sparc"
#ifdef NDEBUG
	const char *debugdir = "releasepsparc";
#else
	const char *debugdir = "debugpsparc";
#endif
#else
#define ARCH	"UNKNOW"
#ifdef NDEBUG
	const char *debugdir = "release";
#else
	const char *debugdir = "debug";
#endif
#endif

	switch (libType) {
	case LIB_CGAME:
		lib = &cgame_library;
		libname = "eglcgame" ARCH ".so";
		apifuncname = "GetCGameAPI";
		break;

	case LIB_GAME:
		lib = &game_library;
		libname = "game_" ARCH ".so";
		apifuncname = "GetGameAPI";
		break;

	case LIB_UI:
		lib = &ui_library;
		libname = "ui_" ARCH ".so";
		apifuncname = "GetUIAPI";
		break;

	default:
		Com_Printf (PRINT_ALL, S_COLOR_RED "Sys_LoadLibrary: Bad libType\n");
		return NULL;
	}

	// check the current debug directory first for development purposes
	getcwd (cwd, sizeof(cwd));
	Q_snprintfz (name, sizeof(name), "%s/%s/%s", cwd, debugdir, libname);

	*lib = dlopen (name, RTLD_NOW);

	if (*lib) {
		Com_Printf (PRINT_DEV, "LoadLibrary (%s)\n", name);
	} else {
		// now run through the search paths
		path = NULL;

		while (1) {
			path = FS_NextPath (path);

			if (!path) 
				return NULL; // couldn't find one anywhere

			Q_snprintfz (name, sizeof(name), "%s/%s", path, libname);
			*lib = dlopen (name, RTLD_NOW);

			if (*lib) {
				Com_Printf (PRINT_DEV, "LoadLibrary (%s)\n", name);
				break;
			}
		}
	}

	APIfunc = (void *)dlsym (*lib, apifuncname);

	if (!APIfunc) {
		Sys_UnloadGameLibrary (gamelib);
		return NULL;
	}

	return APIfunc (parms);
}

/*
========================================================================

	MAIN WINDOW LOOP

========================================================================
*/

/*
==================
main
==================
*/
int main (int argc, char **argv)
{
	int		time, oldtime, newtime;

	/* go back to real user for config loads */
	saved_euid = geteuid();
	seteuid(getuid());

	Com_Init (argc, argv);

	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);

	nostdout = Cvar_Get ("nostdout", "0", 0);
	if (!nostdout->value)
	{
		fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
	}

	oldtime = Sys_Milliseconds ();
	while (1)
	{
		/* find time spent rendering last frame */
		do {
			newtime = Sys_Milliseconds ();
			time = newtime - oldtime;
		} while (time < 1);

		Com_Frame (time);
		oldtime = newtime;
	}
}