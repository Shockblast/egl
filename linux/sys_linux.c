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
#include <dlfcn.h>
#include <dirent.h>

#include "../common/common.h"

#if defined(__FreeBSD__)
#include <machine/param.h>
#endif
cVar_t *nostdout;

uInt	sys_frameTime;
int	sys_currTime;

uid_t	saved_euid;
qBool	stdin_active = qTrue;

/* Like glob_match, but match PATTERN against any final segment of TEXT.  */
static int glob_match_after_star (char *pattern, char *text)
{
	register char *p = pattern, *t = text;
	register char c, c1;

	while ((c = *p++) == '?' || c == '*')
		if (c == '?' && *t++ == '\0')
			return 0;

	if (c == '\0')
		return 1;

	if (c == '\\')
		c1 = *p;
	else
		c1 = c;

	while (1) {
		if ((c == '[' || *t == c1) && glob_match(p - 1, t))
			return 1;
		if (*t++ == '\0')
			return 0;
	}
}

#if 0 /* Not used */
/* Return nonzero if PATTERN has any special globbing chars in it.  */
static int glob_pattern_p (char *pattern)
{
	register char *p = pattern;
	register char c;
	int open = 0;

	while ((c = *p++) != '\0')
		switch (c) {
		case '?':
		case '*':
			return 1;

		case '[':		/* Only accept an open brace if there is a close */
			open++;		/* brace to match it.  Bracket expressions must be */
			continue;	/* complete, according to Posix.2 */
		case ']':
			if (open)
				return 1;
			continue;

		case '\\':
			if (*p++ == '\0')
				return 0;
		}

	return 0;
}
#endif

/* Match the pattern PATTERN against the string TEXT;
   return 1 if it matches, 0 otherwise.

   A match means the entire string TEXT is used up in matching.

   In the pattern string, `*' matches any sequence of characters,
   `?' matches any character, [SET] matches any character in the specified set,
   [!SET] matches any character not in the specified set.

   A set is composed of characters or ranges; a range looks like
   character hyphen character (as in 0-9 or A-Z).
   [0-9a-zA-Z_] is the set of characters allowed in C identifiers.
   Any other character in the pattern must be matched exactly.

   To suppress the special syntactic significance of any of `[]*?!-\',
   and match the character exactly, precede it with a `\'.
*/

int glob_match (char *pattern, char *text)
{
	register char *p = pattern, *t = text;
	register char c;

	while ((c = *p++) != '\0')
		switch (c) {
		case '?':
			if (*t == '\0')
				return 0;
			else
				++t;
			break;

		case '\\':
			if (*p++ != *t++)
				return 0;
			break;

		case '*':
			return glob_match_after_star(p, t);

		case '[':
			{
				register char c1 = *t++;
				int invert;

				if (!c1)
					return (0);

				invert = ((*p == '!') || (*p == '^'));
				if (invert)
					p++;

				c = *p++;
				while (1) {
					register char cstart = c, cend = c;

					if (c == '\\') {
						cstart = *p++;
						cend = cstart;
					}
					if (c == '\0')
						return 0;

					c = *p++;
					if (c == '-' && *p != ']') {
						cend = *p++;
						if (cend == '\\')
							cend = *p++;
						if (cend == '\0')
							return 0;
						c = *p++;
					}
					if (c1 >= cstart && c1 <= cend)
						goto match;
					if (c == ']')
						break;
				}
				if (!invert)
					return 0;
				break;

			  match:
				/* Skip the rest of the [...] construct that already matched.  */
				while (c != ']') {
					if (c == '\0')
						return 0;
					c = *p++;
					if (c == '\0')
						return 0;
					else if (c == '\\')
						++p;
				}
				if (invert)
					return 0;
				break;
			}

		default:
			if (c != *t++)
				return 0;
		}

	return *t == '\0';
}


// ========================================


/*
================
Sys_Printf
================
*/
void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];
	unsigned char		*p;

	va_start (argptr, fmt);
	vsnprintf (text, sizeof (text), fmt, argptr);
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
void Sys_Init (void)
{
}


/*
================
Sys_Quit
================
*/
void Sys_Quit (void)
{
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
void Sys_Error (char *error, ...)
{
    va_list     argptr;
    char        string[1024];

	// change stdin to non blocking
    fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);

	CL_Shutdown ();
	Com_Shutdown ();
    
    va_start (argptr, error);
	vsnprintf (string, sizeof (string), error, argptr);
    va_end (argptr);
	fprintf (stderr, "Error: %s\n", string);

	_exit (1);
}


/*
================
Sys_Warn
================
*/
void Sys_Warn (char *warning, ...)
{
    va_list     argptr;
    char        string[1024];
    
    va_start (argptr, warning);
	vsnprintf (string, sizeof (string), warning, argptr);
    va_end (argptr);
	fprintf (stderr, "Warning: %s", string);
} 


/*
============
Sys_FileTime

returns -1 if not present
============
*/
int Sys_FileTime (char *path)
 {
	struct	stat	buf;
	
	if (stat (path,&buf) == -1)
		return -1;
	
	return buf.st_mtime;
}

void floating_point_exception_handler(int whatever)
 {
	signal(SIGFPE, floating_point_exception_handler);
}


/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput (void)
 {
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
		// eof
		stdin_active = qFalse;
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
void Sys_ConsoleOutput (char *string)
 {
	if (nostdout && nostdout->value)
		return;

	fputs(string, stdout);
}


/*
================
Sys_Milliseconds
================
*/
int Sys_Milliseconds (void)
 {
	struct timeval tp;
	struct timezone tzp;
	static int		secbase;

	gettimeofday(&tp, &tzp);
	
	if (!secbase) {
		secbase = tp.tv_sec;
		return tp.tv_usec/1000;
	}

	sys_currTime = (tp.tv_sec - secbase)*1000 + tp.tv_usec/1000;
	
	return sys_currTime;
}


/*
================
Sys_AppActivate
================
*/
void Sys_AppActivate (void)
 {
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

	// grab frame time
	sys_frameTime = Sys_Milliseconds();
}


/*
================
Sys_GetClipboardData
================
*/
char *Sys_GetClipboardData (void) {
	return NULL;
}


// =====================================================================

static	char	findbase[MAX_OSPATH];
static	char	findpath[MAX_OSPATH];
static	char	findpattern[MAX_OSPATH];
static	DIR		*fdir;

static qBool CompareAttributes(char *path, char *name, unsigned musthave, unsigned canthave) {
	struct stat st;
	char fn[MAX_OSPATH];

	// '.' and '..' never match
	if ((strcmp (name, ".") == 0) || (strcmp (name, "..") == 0))
		return qFalse;

	return qTrue;

	if (stat(fn, &st) == -1)
		return qFalse; // shouldn't happen

	if ((st.st_mode & S_IFDIR) && (canthave & SFF_SUBDIR))
		return qFalse;

	if ((musthave & SFF_SUBDIR) && !(st.st_mode & S_IFDIR))
		return qFalse;

	return qTrue;
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

	d = readdir(fdir);
	while (d) {
		if (!*findpattern || glob_match (findpattern, d->d_name)) {
			if (CompareAttributes (findbase, d->d_name, musthave, canhave)) {
				sprintf (findpath, "%s/%s", findbase, d->d_name);
				return findpath;
			}
		}

		d = readdir(fdir);
	}
	return NULL;
}

/*
========================================================================

	LIBRARY MANAGEMENT

========================================================================
*/

static void *gameLib = NULL;
static void *cGameLib = NULL;
static void *uiLib = NULL;

/*
=================
Sys_UnloadLibrary
=================
*/
void Sys_UnloadLibrary (int libType)
{
	void **lib;

	switch (libType) {
	case LIB_CGAME:	lib = &cGameLib;	break;
	case LIB_GAME:	lib = &gameLib;		break;
	case LIB_UI:	lib = &uiLib;		break;
	default:
		Com_Printf (0, S_COLOR_RED "Sys_UnloadLibrary: Bad libType\n");
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
void *Sys_LoadLibrary (int libType, void *parms)
{
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
		lib = &cGameLib;
		libname = "eglcgame" ARCH ".so";
		apifuncname = "GetCGameAPI";
		break;

	case LIB_GAME:
		lib = &gameLib;
		libname = "game_" ARCH ".so";
		apifuncname = "GetGameAPI";
		break;

	case LIB_UI:
		lib = &uiLib;
		libname = "ui_" ARCH ".so";
		apifuncname = "GetUIAPI";
		break;

	default:
		Com_Printf (0, S_COLOR_RED "Sys_LoadLibrary: Bad libType\n");
		return NULL;
	}

	// check the current debug directory first for development purposes
	getcwd (cwd, sizeof(cwd));
	Q_snprintfz (name, sizeof(name), "%s/%s/%s", cwd, debugdir, libname);

	*lib = dlopen (name, RTLD_NOW);

	if (*lib) {
		Com_Printf (PRNT_DEV, "LoadLibrary (%s)\n", name);
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
				Com_Printf (PRNT_DEV, "LoadLibrary (%s)\n", name);
				break;
			}
		}
	}

	APIfunc = (void *)dlsym (*lib, apifuncname);

	if (!APIfunc) {
		Sys_UnloadLibrary (libType);
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

	// go back to real user for config loads
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
		// find time spent rendering last frame
		do {
			newtime = Sys_Milliseconds ();
			time = newtime - oldtime;
		} while (time < 1);

		Com_Frame (time);
		oldtime = newtime;
	}
}
