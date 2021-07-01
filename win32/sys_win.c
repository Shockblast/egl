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
// sys_win.c
//

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

qBool	sysWindows;
qBool	sysActiveApp;
qBool	sysMinimized = qFalse;

uInt	sysMsgTime;
uInt	sysFrameTime;
int		sysTime;

static HANDLE	hInput, hOutput;
static qBool	sysConWindow;

#define	MAX_NUM_ARGVS	128
static int	sysArgCnt = 0;
static char	*sysArgVars[MAX_NUM_ARGVS];

/*
==============================================================================

	SYSTEM IO

==============================================================================
*/

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
		sysWindows = qTrue;

	if (dedicated->integer) {
		sysConWindow = ConWin_Init ();

		// old qHost
		if (!sysConWindow) {
			if (!AllocConsole ())
				Sys_Error ("Couldn't create dedicated server console");

			hInput = GetStdHandle (STD_INPUT_HANDLE);
			hOutput = GetStdHandle (STD_OUTPUT_HANDLE);
		
			// let QHOST hook in
			ConProcInit (sysArgCnt, sysArgVars);
		}
	}
}


/*
================
Sys_Quit
================
*/
void Sys_Quit (void) {
	qBool	wasDedicated;

	timeEndPeriod (1);

	wasDedicated = dedicated->integer ? qTrue : qFalse;

	CL_Shutdown ();

	if (dedicated && dedicated->integer)
		FreeConsole ();

	Com_Shutdown ();

	// shut down QHOST hooks if necessary
	if (wasDedicated) {
		if (sysConWindow)
			ConWin_Shutdown ();
		else
			ConProcShutdown ();
	}

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
	qBool		wasDedicated;

	va_start (argptr, error);
	vsnprintf (text, sizeof (text), error, argptr);
	va_end (argptr);

	MessageBox (NULL, text, "EGL Fatal Error", MB_OK|MB_ICONERROR);

	wasDedicated = dedicated->integer ? qTrue : qFalse;

	CL_Shutdown ();
	Com_Shutdown ();

	// shut down QHOST hooks if necessary
	if (wasDedicated) {
		if (sysConWindow)
			ConWin_Shutdown ();
		else
			ConProcShutdown ();
	}

	exit (1);
}


/*
================
Sys_Alert
================
*/
void Sys_Alert (char *alert, ...) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, alert);
	vsnprintf (text, sizeof (text), alert, argptr);
	va_end (argptr);

	MessageBox (NULL, text, "EGL Alert", MB_OK|MB_ICONWARNING);
	OutputDebugString (text);
}

// ===========================================================================


/*
================
Sys_ScanForCD
================
*/
char *Sys_ScanForCD (void) {
	static char	cddir[MAX_OSPATH];
	static qBool	done = qFalse;
	char			drive[4];
	FILE			*f;
	char			test[MAX_QPATH];

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
		Q_snprintfz (cddir, sizeof (cddir), "%sinstall\\data", drive);
		Q_snprintfz (test, sizeof (test), "%sinstall\\data\\quake2.exe", drive);
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

// ===========================================================================

/*
================
Sys_Milliseconds
================
*/
int Sys_Milliseconds (void) {
	static int		base;
	static qBool	initialized = qFalse;

	if (!initialized) {
		// let base retain 16 bits of effectively random data
		base = timeGetTime() & 0xffff0000;
		initialized = qTrue;
	}
	sysTime = timeGetTime () - base;

	return sysTime;
}


/*
=================
Sys_AppActivate
=================
*/
void Sys_AppActivate (void) {
#ifndef DEDICATED_ONLY
	SetForegroundWindow (globalhWnd);
	ShowWindow (globalhWnd, SW_SHOW);
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
		sysMsgTime = msg.time;
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}

	// grab frame time
	sysFrameTime = timeGetTime ();
}


/*
================
Sys_GetClipboardData
================
*/
char *Sys_GetClipboardData (void) {
	char	*data = NULL;
	char	*cliptext;

	if (OpenClipboard (NULL) != 0) {
		HANDLE hClipboardData;

		if ((hClipboardData = GetClipboardData (CF_TEXT)) != 0) {
			if ((cliptext = GlobalLock (hClipboardData)) != 0) {
				data = Mem_Alloc (GlobalSize (hClipboardData) + 1);
				strcpy (data, cliptext);
				GlobalUnlock (hClipboardData);
			}
		}

		CloseClipboard ();
	}

	return data;
}

// ===========================================================================

char	sysFindBase[MAX_OSPATH];
char	sysFindPath[MAX_OSPATH];
int		sysFindHandle;

static qBool CompareAttributes (int found, uInt mustHave, uInt cantHave) {
	if ((found & _A_RDONLY) && (cantHave & SFF_RDONLY))
		return qFalse;
	if ((found & _A_HIDDEN) && (cantHave & SFF_HIDDEN))
		return qFalse;
	if ((found & _A_SYSTEM) && (cantHave & SFF_SYSTEM))
		return qFalse;
	if ((found & _A_SUBDIR) && (cantHave & SFF_SUBDIR))
		return qFalse;
	if ((found & _A_ARCH) && (cantHave & SFF_ARCH))
		return qFalse;

	if ((mustHave & SFF_RDONLY) && !(found & _A_RDONLY))
		return qFalse;
	if ((mustHave & SFF_HIDDEN) && !(found & _A_HIDDEN))
		return qFalse;
	if ((mustHave & SFF_SYSTEM) && !(found & _A_SYSTEM))
		return qFalse;
	if ((mustHave & SFF_SUBDIR) && !(found & _A_SUBDIR))
		return qFalse;
	if ((mustHave & SFF_ARCH) && !(found & _A_ARCH))
		return qFalse;

	return qTrue;
}

/*
================
Sys_FindClose
================
*/
void Sys_FindClose (void) {
	if (sysFindHandle != -1)
		_findclose (sysFindHandle);

	sysFindHandle = 0;
}


/*
================
Sys_FindFirst
================
*/
char *Sys_FindFirst (char *path, uInt mustHave, uInt cantHave) {
	struct _finddata_t findinfo;

	if (sysFindHandle)
		Sys_Error ("Sys_BeginFind without close");
	sysFindHandle = 0;

	Com_FilePath (path, sysFindBase);
	sysFindHandle = _findfirst(path, &findinfo);

	while ((sysFindHandle != -1)) {
		if (CompareAttributes (findinfo.attrib, mustHave, cantHave)) {
			Q_snprintfz (sysFindPath, sizeof (sysFindPath), "%s/%s", sysFindBase, findinfo.name);
			return sysFindPath;
		}
		else if (_findnext (sysFindHandle, &findinfo) == -1) {
			_findclose (sysFindHandle);
			sysFindHandle = -1;
		}
	}

	return NULL; 
}


/*
================
Sys_FindNext
================
*/
char *Sys_FindNext (uInt mustHave, uInt cantHave) {
	struct _finddata_t findinfo;

	if (sysFindHandle == -1)
		return NULL;

	while (_findnext (sysFindHandle, &findinfo) != -1) {
		if (CompareAttributes (findinfo.attrib, mustHave, cantHave)) {
			Q_snprintfz (sysFindPath, sizeof (sysFindPath), "%s/%s", sysFindBase, findinfo.name);
			return sysFindPath;
		}
	}

	return NULL;
}

/*
==============================================================================

	LIBRARY MANAGEMENT

==============================================================================
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
		Com_Printf (0, S_COLOR_RED "Sys_UnloadLibrary: Bad libType\n");
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

Loads a module
=================
*/
void *Sys_LoadLibrary (int libType, void *parms) {
	char	name[MAX_OSPATH];
	char	cwd[MAX_OSPATH];
	char	*path;
	void	*(*APIfunc) (void *);
	HINSTANCE	*lib;
	char	*libName;
	char	*apiFuncName;

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
		lib			= &cGameLib;
		libName		= "eglcgame" ARCH ".dll";
		apiFuncName	= "GetCGameAPI";
		break;

	case LIB_GAME:
		lib			= &gameLib;
		libName		= "game" ARCH ".dll";
		apiFuncName	= "GetGameAPI";
		break;

	case LIB_UI:
		lib			= &uiLib;
		libName		= "eglui" ARCH ".dll";
		apiFuncName	= "GetUIAPI";
		break;

	default:
		Com_Printf (0, S_COLOR_RED "Sys_LoadLibrary: Bad libType\n");
		return NULL;
	}

	if (*lib)
		Com_Error (ERR_FATAL, "Sys_LoadLibrary without Sys_UnloadLibrary");

	// check the current debug directory first for development purposes
	_getcwd (cwd, sizeof (cwd));
	Q_snprintfz (name, sizeof(name), "%s/%s/%s", cwd, debugdir, libName);

	*lib = LoadLibrary (name);
	if (*lib)
		Com_Printf (PRNT_DEV, "LoadLibrary (%s)\n", name);
	else {
#ifdef _DEBUG
		// check the current directory for other development purposes
		Q_snprintfz (name, sizeof (name), "%s/%s", cwd, libName);
		*lib = LoadLibrary (name);
		if (*lib)
			Com_Printf (PRNT_DEV, "LoadLibrary (%s)\n", libName);
		else {
#endif
			// now run through the search paths
			path = NULL;
			while (1) {
				path = FS_NextPath (path);
				if (!path)
					return NULL;		// couldn't find one anywhere
				Q_snprintfz (name, sizeof(name), "%s/%s", path, libName);
				*lib = LoadLibrary (name);
				if (*lib) {
					Com_Printf (PRNT_DEV, "LoadLibrary (%s)\n",name);
					break;
				}
			}
#ifdef _DEBUG
		}
#endif
	}

	APIfunc = (void *)GetProcAddress (*lib, apiFuncName);
	if (!APIfunc) {
		Sys_UnloadLibrary (libType);
		return NULL;
	}

	return APIfunc (parms);
}

/*
==============================================================================

	CONSOLE WINDOW INPUT/OUTPUT

==============================================================================
*/

static char	sysConsoleText[256];
static int	sysConsoleTextLen;

/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput (void) {
	if (!dedicated || !dedicated->integer)
		return NULL;

	if (sysConWindow) {
		return NULL;
	}
	else {
		INPUT_RECORD	recs[1024];
		int				dummy, ch;
		int				numRead, numEvents;

		for ( ; ; ) {
			if (!GetNumberOfConsoleInputEvents (hInput, &numEvents))
				Sys_Error ("Error getting # of console events");
			if (numEvents <= 0)
				break;

			if (!ReadConsoleInput (hInput, recs, 1, &numRead))
				Sys_Error ("Error reading console input");

			if (numRead != 1)
				Sys_Error ("Couldn't read console input");

			if (recs[0].EventType == KEY_EVENT) {
				if (!recs[0].Event.KeyEvent.bKeyDown) {
					ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

					switch (ch) {
					case '\r':
						WriteFile (hOutput, "\r\n", 2, &dummy, NULL);	

						if (sysConsoleTextLen) {
							sysConsoleText[sysConsoleTextLen] = 0;
							sysConsoleTextLen = 0;
							return sysConsoleText;
						}
						break;

					case '\b':
						if (sysConsoleTextLen) {
							sysConsoleTextLen--;
							WriteFile (hOutput, "\b \b", 3, &dummy, NULL);	
						}
						break;

					default:
						if (ch >= ' ') {
							if (sysConsoleTextLen < sizeof (sysConsoleText)-2) {
								WriteFile (hOutput, &ch, 1, &dummy, NULL);	
								sysConsoleText[sysConsoleTextLen] = ch;
								sysConsoleTextLen++;
							}
						}

						break;
					}
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
	if (!dedicated || !dedicated->integer)
		return;

	if (sysConWindow) {
		ConWin_Print (string);
	}
	else {
		int		dummy;
		char	text[256];

		if (sysConsoleTextLen) {
			text[0] = '\r';
			memset (&text[1], ' ', sysConsoleTextLen);
			text[sysConsoleTextLen+1] = '\r';
			text[sysConsoleTextLen+2] = 0;
			WriteFile (hOutput, text, sysConsoleTextLen+2, &dummy, NULL);
		}

		WriteFile (hOutput, string, Q_strlen (string), &dummy, NULL);

		if (sysConsoleTextLen)
			WriteFile (hOutput, sysConsoleText, sysConsoleTextLen, &dummy, NULL);
	}
}

/*
==============================================================================

	MAIN WINDOW LOOP

==============================================================================
*/

/*
==================
WinMain
==================
*/
static void ParseCommandLine (LPSTR lpCmdLine) {
	sysArgCnt = 1;
	sysArgVars[0] = "exe";

	while (*lpCmdLine && (sysArgCnt < MAX_NUM_ARGVS)) {
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine) {
			sysArgVars[sysArgCnt] = lpCmdLine;
			sysArgCnt++;

			while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
				lpCmdLine++;

			if (*lpCmdLine) {
				*lpCmdLine = 0;
				lpCmdLine++;
			}
			
		}
	}

}

HINSTANCE	globalhInst;
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	int		time, oldTime, newTime;
	MSG		msg;

	// previous instances do not exist in Win32
	if (hPrevInstance)
		return 0;

	globalhInst = hInstance;

	// parse the command line
	ParseCommandLine (lpCmdLine);

	// if we find the CD, add a +set cddirxxx command line
	if (sysArgCnt < (MAX_NUM_ARGVS - 3)) {
		int		i;

		for (i=0 ; i<sysArgCnt ; i++)
			if (!strcmp (sysArgVars[i], "cddir"))
				break;

		// don't override a cddir on the command line
		if (i == sysArgCnt) {
			char	*cddir = Sys_ScanForCD ();
			if (cddir) {
				sysArgVars[sysArgCnt++] = "+set";
				sysArgVars[sysArgCnt++] = "cddir";
				sysArgVars[sysArgCnt++] = cddir;
			}
		}
	}

	// common intialization
	Com_Init (sysArgCnt, sysArgVars);
	oldTime = Sys_Milliseconds ();

	// initial loop
	for ( ; ; ) {
		// if at a full screen console, don't update unless needed
		if (sysMinimized || (dedicated && dedicated->integer))
			Sleep (1);

		while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE)) {
			if (!GetMessage (&msg, NULL, 0, 0))
				Com_Quit ();
			sysMsgTime = msg.time;
			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}

		// main loop
		for ( ; ; ) {
			if (sysMinimized)
				Sleep (1);

			do {
				newTime = Sys_Milliseconds ();
				time = newTime - oldTime;
			} while (time < 1);

			_controlfp (_PC_24, _MCW_PC);
			Com_Frame (time);

			oldTime = newTime;
		}
	}

	// never gets here
	return 0;
}
