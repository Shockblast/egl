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

// sys_win.h

#include "../common/common.h"
#include "winquake.h"
#include "resource.h"
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>

#define MINIMUM_WIN_MEMORY	0x0a00000
#define MAXIMUM_WIN_MEMORY	0x1000000

qBool		sys_Windows;

qBool		sys_ActiveApp;
qBool		sys_Minimized;

static HANDLE	hInput, hOutput;

unsigned int	sys_MsgTime;
unsigned int	sys_FrameTime;

#define	MAX_NUM_ARGVS	128
int			argc;
char		*argv[MAX_NUM_ARGVS];

/*
===============================================================================

	SYSTEM IO

===============================================================================
*/

/*
================
WinError
================
*/
void WinError (void) {
	LPVOID lpMsgBuf;

	FormatMessage (
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError (),
		MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL
	);

	// display the string
	MessageBox (NULL, lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION);

	// free the buffer
	LocalFree (lpMsgBuf);
}


/*
================
Sys_Init
================
*/
void Sys_Init (void) {
	OSVERSIONINFO	vinfo;

	timeBeginPeriod (1);

	vinfo.dwOSVersionInfoSize = sizeof (vinfo);

	if (!GetVersionEx (&vinfo))
		Sys_Error ("Couldn't get OS info");

	if (vinfo.dwMajorVersion < 4)
		Sys_Error ("EGL requires windows version 4 or greater");
	if (vinfo.dwPlatformId == VER_PLATFORM_WIN32s)
		Sys_Error ("EGL doesn't run on Win32s");
	else if (vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		sys_Windows = qTrue;

	if (dedicated->integer) {
		SetPriorityClass (GetCurrentProcess(), HIGH_PRIORITY_CLASS);

		if (!AllocConsole ())
			Sys_Error ("Couldn't create dedicated server console");

		hInput = GetStdHandle (STD_INPUT_HANDLE);
		hOutput = GetStdHandle (STD_OUTPUT_HANDLE);
	
		// let QHOST hook in
		ConProcInit (argc, argv);
	}
}


/*
================
Sys_Quit
================
*/
void Sys_Quit (void) {
	timeEndPeriod (1);

	CL_Shutdown ();

	if (dedicated && dedicated->integer)
		FreeConsole ();

	Com_Shutdown ();

	// shut down QHOST hooks if necessary
	ConProcShutdown ();

	exit (0);
}


/*
================
Sys_Error
================
*/
void Sys_Error (char *error, ...) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	MessageBox (NULL, text, "EGL Fatal Error", MB_OK|MB_ICONWARNING);

	CL_Shutdown ();
	Com_Shutdown ();

	// shut down QHOST hooks if necessary
	ConProcShutdown ();

	exit (1);
}

//================================================================


/*
================
Sys_ScanForCD
================
*/
char *Sys_ScanForCD (void) {
	static char	cddir[MAX_OSPATH];
	static qBool	done = qFalse;
	char		drive[4];
	FILE		*f;
	char		test[MAX_QPATH];

	if (done)	// don't re-check
		return cddir;

	// no abort/retry/fail errors
	SetErrorMode (SEM_FAILCRITICALERRORS);

	drive[0] = 'c';
	drive[1] = ':';
	drive[2] = '\\';
	drive[3] = 0;

	done = qTrue;

	// scan the drives
	for (drive[0]='c' ; drive[0]<='z' ; drive[0]++) {
		// where activision put the stuff
		sprintf (cddir, "%sinstall\\data", drive);
		sprintf (test, "%sinstall\\data\\quake2.exe", drive);
		f = fopen (test, "r");
		if (f) {
			fclose (f);
			if (GetDriveType (drive) == DRIVE_CDROM)
				return cddir;
		}
	}

	cddir[0] = 0;
	
	return NULL;
}

//================================================================

static char	console_text[256];
static int	console_textlen;

/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput (void) {
	INPUT_RECORD	recs[1024];
	int	dummy;
	int	ch, numread, numevents;

	if (!dedicated || !dedicated->integer)
		return NULL;

	for ( ; ; ) {
		if (!GetNumberOfConsoleInputEvents (hInput, &numevents))
			Sys_Error ("Error getting # of console events");

		if (numevents <= 0)
			break;

		if (!ReadConsoleInput (hInput, recs, 1, &numread))
			Sys_Error ("Error reading console input");

		if (numread != 1)
			Sys_Error ("Couldn't read console input");

		if (recs[0].EventType == KEY_EVENT) {
			if (!recs[0].Event.KeyEvent.bKeyDown) {
				ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

				switch (ch) {
				case '\r':
					WriteFile (hOutput, "\r\n", 2, &dummy, NULL);	

					if (console_textlen) {
						console_text[console_textlen] = 0;
						console_textlen = 0;
						return console_text;
					}
					break;

				case '\b':
					if (console_textlen) {
						console_textlen--;
						WriteFile (hOutput, "\b \b", 3, &dummy, NULL);	
					}
					break;

				default:
					if (ch >= ' ') {
						if (console_textlen < sizeof (console_text)-2) {
							WriteFile (hOutput, &ch, 1, &dummy, NULL);	
							console_text[console_textlen] = ch;
							console_textlen++;
						}
					}

					break;
				}
			}
		}
	}

	return NULL;
}


/*
================
Sys_ConsoleOutput

Print text to the dedicated console
================
*/
void Sys_ConsoleOutput (char *string) {
	int		dummy;
	char	text[256];

	if (!dedicated || !dedicated->integer)
		return;

	if (console_textlen) {
		text[0] = '\r';
		memset (&text[1], ' ', console_textlen);
		text[console_textlen+1] = '\r';
		text[console_textlen+2] = 0;
		WriteFile (hOutput, text, console_textlen+2, &dummy, NULL);
	}

	WriteFile (hOutput, string, strlen (string), &dummy, NULL);

	if (console_textlen)
		WriteFile (hOutput, console_text, console_textlen, &dummy, NULL);
}


/*
================
Sys_Milliseconds
================
*/
int		g_CurTime;
int Sys_Milliseconds (void) {
	static int		base;
	static qBool	initialized = qFalse;

	if (!initialized) {
		// let base retain 16 bits of effectively random data
		base = timeGetTime() & 0xffff0000;
		initialized = qTrue;
	}
	g_CurTime = timeGetTime () - base;

	return g_CurTime;
}


/*
=================
Sys_AppActivate
=================
*/
void Sys_AppActivate (void) {
#ifndef DEDICATED_ONLY
	SetForegroundWindow (g_hWnd);
	ShowWindow (g_hWnd, SW_SHOW);
#endif
}


/*
================
Sys_Mkdir
================
*/
void Sys_Mkdir (char *path) {
	_mkdir (path);
}


/*
================
Sys_SendKeyEvents

Send Key_Event calls
================
*/
void Sys_SendKeyEvents (void) {
	MSG		msg;

	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE)) {
		if (!GetMessage (&msg, NULL, 0, 0))
			Sys_Quit ();
		sys_MsgTime = msg.time;
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}

	// grab frame time
	sys_FrameTime = timeGetTime();
}


/*
================
Sys_GetClipboardData
================
*/
char *Sys_GetClipboardData (void) {
	char *data = NULL;
	char *cliptext;

	if (OpenClipboard (NULL) != 0) {
		HANDLE hClipboardData;

		if ((hClipboardData = GetClipboardData (CF_TEXT)) != 0) {
			if ((cliptext = GlobalLock (hClipboardData)) != 0) {
				data = malloc (GlobalSize (hClipboardData) + 1);
				strcpy (data, cliptext);
				GlobalUnlock (hClipboardData);
			}
		}

		CloseClipboard ();
	}

	return data;
}

// ===========================================================================

char	findbase[MAX_OSPATH];
char	findpath[MAX_OSPATH];
int		findhandle;

static qBool CompareAttributes (unsigned found, unsigned musthave, unsigned canthave) {
	if ((found & _A_RDONLY) && (canthave & SFF_RDONLY))
		return qFalse;
	if ((found & _A_HIDDEN) && (canthave & SFF_HIDDEN))
		return qFalse;
	if ((found & _A_SYSTEM) && (canthave & SFF_SYSTEM))
		return qFalse;
	if ((found & _A_SUBDIR) && (canthave & SFF_SUBDIR))
		return qFalse;
	if ((found & _A_ARCH) && (canthave & SFF_ARCH))
		return qFalse;

	if ((musthave & SFF_RDONLY) && !(found & _A_RDONLY))
		return qFalse;
	if ((musthave & SFF_HIDDEN) && !(found & _A_HIDDEN))
		return qFalse;
	if ((musthave & SFF_SYSTEM) && !(found & _A_SYSTEM))
		return qFalse;
	if ((musthave & SFF_SUBDIR) && !(found & _A_SUBDIR))
		return qFalse;
	if ((musthave & SFF_ARCH) && !(found & _A_ARCH))
		return qFalse;

	return qTrue;
}

/*
================
Sys_FindClose
================
*/
void Sys_FindClose (void) {
	if (findhandle != -1)
		_findclose (findhandle);

	findhandle = 0;
}


/*
================
Sys_FindFirst
================
*/
char *Sys_FindFirst (char *path, unsigned musthave, unsigned canthave) {
	struct _finddata_t findinfo;

	if (findhandle)
		Sys_Error ("Sys_BeginFind without close");
	findhandle = 0;

	Com_FilePath (path, findbase);
	findhandle = _findfirst(path, &findinfo);

	while ((findhandle != -1)) {
		if (CompareAttributes (findinfo.attrib, musthave, canthave)) {
			Com_sprintf (findpath, sizeof (findpath), "%s/%s", findbase, findinfo.name);
			return findpath;
		} else if (_findnext (findhandle, &findinfo) == -1) {
			_findclose (findhandle);
			findhandle = -1;
		}
	}

	return NULL; 
}


/*
================
Sys_FindNext
================
*/
char *Sys_FindNext (unsigned musthave, unsigned canthave) {
	struct _finddata_t findinfo;

	if (findhandle == -1)
		return NULL;

	while (_findnext(findhandle, &findinfo) != -1) {
		if (CompareAttributes (findinfo.attrib, musthave, canthave)) {
			Com_sprintf (findpath, sizeof (findpath), "%s/%s", findbase, findinfo.name);
			return findpath;
		}
	}

	return NULL;
}

/*
========================================================================

	LIBRARY MANAGEMENT

========================================================================
*/

static HINSTANCE	cGameLib;
static HINSTANCE	gameLib;
static HINSTANCE	uiLib;

/*
=================
Sys_UnloadLibrary
=================
*/
void Sys_UnloadLibrary (int libType) {
	HINSTANCE	*lib;

	switch (libType) {
	case LIB_CGAME:	lib = &cGameLib;	break;
	case LIB_GAME:	lib = &gameLib;		break;
	case LIB_UI:	lib = &uiLib;		break;
	default:
		Com_Printf (PRINT_ALL, S_COLOR_RED "Sys_UnloadLibrary: Bad libType\n");
		return;
	}

	if (!FreeLibrary (*lib))
		Com_Error (ERR_FATAL, "FreeLibrary (%s) failed",
								(*lib == cGameLib) ? "LIB_CGAME" :
								(*lib == gameLib) ? "LIB_GAME" :
								(*lib == uiLib) ? "LIB_UI" :
								"UNKNOWN");

	*lib = NULL;
}

/*
=================
Sys_LoadLibrary

Loads the game dll
=================
*/
void *Sys_LoadLibrary (int libType, void *parms) {
	char	name[MAX_OSPATH];
	char	cwd[MAX_OSPATH];
	char	*path;
	void	*(*APIfunc) (void *);
	HINSTANCE	*lib;
	char	*libname;
	char	*apifuncname;

#if defined _M_IX86
#define ARCH	"x86"
#ifdef NDEBUG
	const char *debugdir = "release";
#else
	const char *debugdir = "debug";
#endif // _M_IX86

#elif defined _M_ALPHA
#define ARCH	"axp"
#ifdef NDEBUG
	const char *debugdir = "releaseaxp";
#else
	const char *debugdir = "debugaxp";
#endif
#endif // _M_ALPHA

	switch (libType) {
	case LIB_CGAME:
		lib = &cGameLib;
		libname = "eglcgame" ARCH ".dll";
		apifuncname = "GetCGameAPI";
		break;

	case LIB_GAME:
		lib			= &gameLib;
		libname		= "game" ARCH ".dll";
		apifuncname	= "GetGameAPI";
		break;

	case LIB_UI:
		lib			= &uiLib;
		libname		= "eglui" ARCH ".dll";
		apifuncname	= "GetUIAPI";
		break;

	default:
		Com_Printf (PRINT_ALL, S_COLOR_RED "Sys_LoadLibrary: Bad libType\n");
		return NULL;
	}

	if (*lib)
		Com_Error (ERR_FATAL, "Sys_LoadLibrary without Sys_UnloadLibrary");

	//*check the current debug directory first for development purposes
	_getcwd (cwd, sizeof (cwd));
	Com_sprintf (name, sizeof(name), "%s/%s/%s", cwd, debugdir, libname);

	*lib = LoadLibrary (name);
	if (*lib)
		Com_Printf (PRINT_DEV, "LoadLibrary (%s)\n", name);
	else {
#ifdef DEBUG
		// check the current directory for other development purposes
		Com_sprintf (name, sizeof (name), "%s/%s", cwd, gamename);
		*lib = LoadLibrary (name);
		if (*lib)
			Com_Printf (PRINT_DEV, "LoadLibrary (%s)\n", name);
		else {
#endif
			// now run through the search paths
			path = NULL;
			while (1) {
				path = FS_NextPath (path);
				if (!path)
					return NULL;		// couldn't find one anywhere
				Com_sprintf (name, sizeof(name), "%s/%s", path, libname);
				*lib = LoadLibrary (name);
				if (*lib) {
					Com_Printf (PRINT_DEV, "LoadLibrary (%s)\n",name);
					break;
				}
			}
#ifdef DEBUG
		}
#endif
	}

	APIfunc = (void *)GetProcAddress (*lib, apifuncname);
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
WinMain
==================
*/
static void ParseCommandLine (LPSTR lpCmdLine) {
	argc = 1;
	argv[0] = "exe";

	while (*lpCmdLine && (argc < MAX_NUM_ARGVS)) {
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine) {
			argv[argc] = lpCmdLine;
			argc++;

			while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
				lpCmdLine++;

			if (*lpCmdLine) {
				*lpCmdLine = 0;
				lpCmdLine++;
			}
			
		}
	}

}

HINSTANCE	g_hInst;
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	MSG		msg;
	int		time;
	int		oldtime;
	int		newtime;
	char	*cddir;

	// previous instances do not exist in Win32
	if (hPrevInstance)
		return 0;

	g_hInst = hInstance;

	ParseCommandLine (lpCmdLine);

	// if we find the CD, add a +set cddirxxx command line
	cddir = Sys_ScanForCD ();
	if (cddir && (argc < (MAX_NUM_ARGVS - 3))) {
		int		i;

		// don't override a cddir on the command line
		for (i=0 ; i<argc ; i++)
			if (!strcmp(argv[i], "cddir"))
				break;
		if (i == argc) {
			argv[argc++] = "+set";
			argv[argc++] = "cddir";
			argv[argc++] = cddir;
		}
	}

	Com_Init (argc, argv);
	oldtime = Sys_Milliseconds ();

	// main window message loop
	for ( ; ; ) {
		// if at a full screen console, don't update unless needed
		if (sys_Minimized || (dedicated && dedicated->integer))
			Sleep (1);

		while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE)) {
			if (!GetMessage (&msg, NULL, 0, 0))
				Com_Quit ();
			sys_MsgTime = msg.time;
			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}

		do {
			newtime = Sys_Milliseconds ();
			time = newtime - oldtime;
		} while (time < 1);

		_controlfp (_PC_24, _MCW_PC);
		Com_Frame (time);

		oldtime = newtime;
	}

	// never gets here
	return qTrue;
}
