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

qBool	sys_isWindows;
qBool	sys_activeApp;
qBool	sys_isMinimized = qFalse;

uInt	sys_msgTime;
uInt	sys_frameTime;
int		sys_currTime;

static HANDLE	hInput, hOutput;
static qBool	sys_consoleWindow;

#define	MAX_NUM_ARGVS	128
static int	sys_argCnt = 0;
static char	*sys_argVars[MAX_NUM_ARGVS];

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
void Sys_Init (void)
{
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
		sys_isWindows = qTrue;

	if (dedicated->integer) {
		sys_consoleWindow = ConWin_Init ();

		// Old qHost
		if (!sys_consoleWindow) {
			if (!AllocConsole ())
				Sys_Error ("Couldn't create dedicated server console");

			hInput = GetStdHandle (STD_INPUT_HANDLE);
			hOutput = GetStdHandle (STD_OUTPUT_HANDLE);
		
			// Let QHOST hook in
			ConProcInit (sys_argCnt, sys_argVars);
		}
	}
}


/*
================
Sys_Quit
================
*/
void Sys_Quit (void)
{
	qBool	wasDedicated;

	timeEndPeriod (1);

	wasDedicated = dedicated->integer ? qTrue : qFalse;

	CL_Shutdown ();

	if (dedicated && dedicated->integer)
		FreeConsole ();

	Com_Shutdown ();

	// Shut down QHOST hooks if necessary
	if (wasDedicated) {
		if (sys_consoleWindow)
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
void Sys_Error (char *error, ...)
{
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

	// Shut down QHOST hooks if necessary
	if (wasDedicated) {
		if (sys_consoleWindow)
			ConWin_Shutdown ();
		else
			ConProcShutdown ();
	}

	exit (1);
}

// ===========================================================================


/*
================
Sys_ScanForCD
================
*/
char *Sys_ScanForCD (void)
{
	static char	cddir[MAX_OSPATH];
	static qBool	done = qFalse;
	char			drive[4];
	FILE			*f;
	char			test[MAX_QPATH];

	if (done)	// Don't re-check
		return cddir;

	// No abort/retry/fail errors
	SetErrorMode (SEM_FAILCRITICALERRORS);

	drive[0] = 'c';
	drive[1] = ':';
	drive[2] = '\\';
	drive[3] = 0;

	done = qTrue;

	// Scan the drives
	for (drive[0]='c' ; drive[0]<='z' ; drive[0]++) {
		// Where activision put the stuff
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
int Sys_Milliseconds (void)
{
	static int		base;
	static qBool	initialized = qFalse;

	if (!initialized) {
		// Let base retain 16 bits of effectively random data
		base = timeGetTime() & 0xffff0000;
		initialized = qTrue;
	}
	sys_currTime = timeGetTime () - base;

	return sys_currTime;
}


/*
=================
Sys_AppActivate
=================
*/
void Sys_AppActivate (void)
{
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
void Sys_Mkdir (char *path)
{
	_mkdir (path);
}


/*
================
Sys_SendKeyEvents

Send Key_Event calls
================
*/
void Sys_SendKeyEvents (void)
{
	MSG		msg;

	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE)) {
		if (!GetMessage (&msg, NULL, 0, 0))
			Sys_Quit ();
		sys_msgTime = msg.time;
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}

	// Grab frame time
	sys_frameTime = timeGetTime ();
}


/*
================
Sys_GetClipboardData
================
*/
char *Sys_GetClipboardData (void)
{
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

char	sys_findBase[MAX_OSPATH];
char	sys_findPath[MAX_OSPATH];
int		sys_findHandle;

static qBool Sys_CompareFileAttributes (int found, uInt mustHave, uInt cantHave)
{
	// read only
	if (found & _A_RDONLY) {
		if (cantHave & SFF_RDONLY)
			return qFalse;
	}
	else if (mustHave & SFF_RDONLY)
		return qFalse;

	// hidden
	if (found & _A_HIDDEN) {
		if (cantHave & SFF_HIDDEN)
			return qFalse;
	}
	else if (mustHave & SFF_HIDDEN)
		return qFalse;

	// system
	if (found & _A_SYSTEM) {
		if (cantHave & SFF_SYSTEM)
			return qFalse;
	}
	else if (mustHave & SFF_SYSTEM)
		return qFalse;

	// subdir
	if (found & _A_SUBDIR) {
		if (cantHave & SFF_SUBDIR)
			return qFalse;
	}
	else if (mustHave & SFF_SUBDIR)
		return qFalse;

	// arch
	if (found & _A_ARCH) {
		if (cantHave & SFF_ARCH)
			return qFalse;
	}
	else if (mustHave & SFF_ARCH)
		return qFalse;

	return qTrue;
}

/*
================
Sys_FindClose
================
*/
void Sys_FindClose (void)
{
	if (sys_findHandle != -1)
		_findclose (sys_findHandle);

	sys_findHandle = 0;
}


/*
================
Sys_FindFirst
================
*/
char *Sys_FindFirst (char *path, uInt mustHave, uInt cantHave)
{
	struct _finddata_t findinfo;

	if (sys_findHandle)
		Sys_Error ("Sys_BeginFind without close");
	sys_findHandle = 0;

	Com_FilePath (path, sys_findBase, sizeof (sys_findBase));
	sys_findHandle = _findfirst(path, &findinfo);

	while (sys_findHandle != -1) {
		if (Sys_CompareFileAttributes (findinfo.attrib, mustHave, cantHave)) {
			Q_snprintfz (sys_findPath, sizeof (sys_findPath), "%s/%s", sys_findBase, findinfo.name);
			return sys_findPath;
		}
		else if (_findnext (sys_findHandle, &findinfo) == -1) {
			_findclose (sys_findHandle);
			sys_findHandle = -1;
		}
	}

	return NULL; 
}


/*
================
Sys_FindNext
================
*/
char *Sys_FindNext (uInt mustHave, uInt cantHave)
{
	struct _finddata_t findinfo;

	if (sys_findHandle == -1)
		return NULL;

	while (_findnext (sys_findHandle, &findinfo) != -1) {
		if (Sys_CompareFileAttributes (findinfo.attrib, mustHave, cantHave)) {
			Q_snprintfz (sys_findPath, sizeof (sys_findPath), "%s/%s", sys_findBase, findinfo.name);
			return sys_findPath;
		}
	}

	return NULL;
}


/*
================
Sys_FindFiles
================
*/
int Sys_FindFiles (char *path, char *pattern, char **fileList, int maxFiles, int fileCount, qBool recurse, qBool addFiles, qBool addDirs)
{
	WIN32_FIND_DATA	findInfo;
	HANDLE			findHandle;
	BOOL			findRes = TRUE;
	char			findPath[MAX_OSPATH], searchPath[MAX_OSPATH];

	Q_snprintfz (searchPath, sizeof (searchPath), "%s/*", path);

	findHandle = FindFirstFile (searchPath, &findInfo);
	if (findHandle == INVALID_HANDLE_VALUE)
		return fileCount;

	while (findRes == TRUE) {
		// Check for invalid file name
		if (findInfo.cFileName[strlen(findInfo.cFileName)-1] == '.') {
			findRes = FindNextFile (findHandle, &findInfo);
			continue;
		}

		Q_snprintfz (findPath, sizeof (findPath), "%s/%s", path, findInfo.cFileName);

		if (findInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			// Add a directory
			if (addDirs && fileCount < maxFiles)
				fileList[fileCount++] = Mem_StrDup (findPath);

			// Recurse down the next directory
			if (recurse)
				fileCount = Sys_FindFiles (findPath, pattern, fileList, maxFiles, fileCount, recurse, addFiles, addDirs);
		}
		else {
			// Match pattern
			if (!Q_WildcardMatch (pattern, findPath, 1)) {
				findRes = FindNextFile (findHandle, &findInfo);
				continue;
			}

			// Add a file
			if (addFiles && fileCount < maxFiles)
				fileList[fileCount++] = Mem_StrDup (findPath);
		}

		findRes = FindNextFile (findHandle, &findInfo);
	}

	FindClose (findHandle);

	return fileCount;
}

/*
==============================================================================

	LIBRARY MANAGEMENT

==============================================================================
*/

static HINSTANCE	sys_cGameLib;
static HINSTANCE	sys_gameLib;
static HINSTANCE	sys_uiLib;

/*
=================
Sys_UnloadLibrary
=================
*/
void Sys_UnloadLibrary (int libType)
{
	HINSTANCE	*lib;

	switch (libType) {
	case LIB_CGAME:	lib = &sys_cGameLib;	break;
	case LIB_GAME:	lib = &sys_gameLib;		break;
	case LIB_UI:	lib = &sys_uiLib;		break;
	default:
		Com_Printf (0, S_COLOR_RED "Sys_UnloadLibrary: Bad libType\n");
		return;
	}

	if (!FreeLibrary (*lib))
		Com_Error (ERR_FATAL, "FreeLibrary (%s) failed",
								(*lib == sys_cGameLib) ? "LIB_CGAME" :
								(*lib == sys_gameLib) ? "LIB_GAME" :
								(*lib == sys_uiLib) ? "LIB_UI" :
								"UNKNOWN");

	*lib = NULL;
}

/*
=================
Sys_LoadLibrary

Loads a module
=================
*/
void *Sys_LoadLibrary (int libType, void *parms)
{
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
		lib			= &sys_cGameLib;
		libName		= "eglcgame" ARCH ".dll";
		apiFuncName	= "GetCGameAPI";
		break;

	case LIB_GAME:
		lib			= &sys_gameLib;
		libName		= "game" ARCH ".dll";
		apiFuncName	= "GetGameAPI";
		break;

	case LIB_UI:
		lib			= &sys_uiLib;
		libName		= "eglui" ARCH ".dll";
		apiFuncName	= "GetUIAPI";
		break;

	default:
		Com_Printf (0, S_COLOR_RED "Sys_LoadLibrary: Bad libType\n");
		return NULL;
	}

	if (*lib)
		Com_Error (ERR_FATAL, "Sys_LoadLibrary without Sys_UnloadLibrary");

	// Check the current debug directory first for development purposes
	_getcwd (cwd, sizeof (cwd));
	Q_snprintfz (name, sizeof(name), "%s/%s/%s", cwd, debugdir, libName);

	*lib = LoadLibrary (name);
	if (*lib)
		Com_Printf (PRNT_DEV, "LoadLibrary (%s)\n", name);
	else {
#ifdef _DEBUG
		// Check the current directory for other development purposes
		Q_snprintfz (name, sizeof (name), "%s/%s", cwd, libName);
		*lib = LoadLibrary (name);
		if (*lib)
			Com_Printf (PRNT_DEV, "LoadLibrary (%s)\n", libName);
		else {
#endif
			// Now run through the search paths
			path = NULL;
			while (1) {
				path = FS_NextPath (path);
				if (!path)
					return NULL;	// Couldn't find one anywhere

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

static char	sys_consoleText[256];
static int	sys_consoleTextLen;

/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput (void)
{
	if (!dedicated || !dedicated->integer)
		return NULL;

	if (sys_consoleWindow) {
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

						if (sys_consoleTextLen) {
							sys_consoleText[sys_consoleTextLen] = 0;
							sys_consoleTextLen = 0;
							return sys_consoleText;
						}
						break;

					case '\b':
						if (sys_consoleTextLen) {
							sys_consoleTextLen--;
							WriteFile (hOutput, "\b \b", 3, &dummy, NULL);	
						}
						break;

					default:
						if (ch >= ' ') {
							if (sys_consoleTextLen < sizeof (sys_consoleText)-2) {
								WriteFile (hOutput, &ch, 1, &dummy, NULL);	
								sys_consoleText[sys_consoleTextLen] = ch;
								sys_consoleTextLen++;
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
void Sys_ConsoleOutput (char *string)
{
	if (!dedicated || !dedicated->integer)
		return;

	if (sys_consoleWindow) {
		ConWin_Print (string);
	}
	else {
		int		dummy;
		char	text[256];

		if (sys_consoleTextLen) {
			text[0] = '\r';
			memset (&text[1], ' ', sys_consoleTextLen);
			text[sys_consoleTextLen+1] = '\r';
			text[sys_consoleTextLen+2] = 0;
			WriteFile (hOutput, text, sys_consoleTextLen+2, &dummy, NULL);
		}

		WriteFile (hOutput, string, strlen (string), &dummy, NULL);

		if (sys_consoleTextLen)
			WriteFile (hOutput, sys_consoleText, sys_consoleTextLen, &dummy, NULL);
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
static void ParseCommandLine (LPSTR lpCmdLine)
{
	sys_argCnt = 1;
	sys_argVars[0] = "exe";

	while (*lpCmdLine && sys_argCnt < MAX_NUM_ARGVS) {
		while (*lpCmdLine && *lpCmdLine <= 32 || *lpCmdLine > 126)
			lpCmdLine++;

		if (*lpCmdLine) {
			sys_argVars[sys_argCnt] = lpCmdLine;
			sys_argCnt++;

			while (*lpCmdLine && *lpCmdLine > 32 && *lpCmdLine <= 126)
				lpCmdLine++;

			if (*lpCmdLine) {
				*lpCmdLine = 0;
				lpCmdLine++;
			}
			
		}
	}

}

HINSTANCE	globalhInst;
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int		time, oldTime, newTime;
	MSG		msg;

	// Previous instances do not exist in Win32
	if (hPrevInstance)
		return 0;

	globalhInst = hInstance;

	// Parse the command line
	ParseCommandLine (lpCmdLine);

	// If we find the CD, add a +set cddirxxx command line
	if (sys_argCnt < MAX_NUM_ARGVS - 3) {
		int		i;

		for (i=0 ; i<sys_argCnt ; i++)
			if (!strcmp (sys_argVars[i], "cddir"))
				break;

		// Don't override a cddir on the command line
		if (i == sys_argCnt) {
			char	*cddir = Sys_ScanForCD ();
			if (cddir) {
				sys_argVars[sys_argCnt++] = "+set";
				sys_argVars[sys_argCnt++] = "cddir";
				sys_argVars[sys_argCnt++] = cddir;
			}
		}
	}

	// Common intialization
	Com_Init (sys_argCnt, sys_argVars);
	oldTime = Sys_Milliseconds ();

	// Initial loop
	for ( ; ; ) {
		// If at a full screen console, don't update unless needed
		if (sys_isMinimized || (dedicated && dedicated->integer))
			Sleep (1);

		while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE)) {
			if (!GetMessage (&msg, NULL, 0, 0))
				Com_Quit ();
			sys_msgTime = msg.time;
			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}

		// Main loop
		for ( ; ; ) {
			if (sys_isMinimized)
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

	// Never gets here
	return 0;
}
