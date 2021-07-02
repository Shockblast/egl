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
// win_main.c
//

#include "../common/common.h"
#include "win_local.h"
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

winInfo_t	sys_winInfo;

#define MAX_NUM_ARGVS	128
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

	// Make sure the timer is high precision, otherwise NT gets 18ms resolution
	timeBeginPeriod (1);

	// Check operating system info
	vinfo.dwOSVersionInfoSize = sizeof (vinfo);

	if (!GetVersionEx (&vinfo))
		Sys_Error ("Couldn't get OS info");

	if (vinfo.dwMajorVersion < 4)
		Sys_Error ("EGL requires windows version 4 or greater");
	if (vinfo.dwPlatformId == VER_PLATFORM_WIN32s)
		Sys_Error ("EGL doesn't run on Win32s");
	else if (vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		sys_winInfo.isWin32 = qTrue;
	else
		sys_winInfo.isWin32 = qFalse;
}


/*
================
Sys_Quit
================
*/
void Sys_Quit (qBool error)
{
	Sys_DestroyConsole ();

#ifndef DEDICATED_ONLY
	if (!dedicated->intVal)
		CL_ClientShutdown (error);
#endif
	Com_Shutdown ();

	timeEndPeriod (1);

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
	MSG			msg;

	va_start (argptr, error);
	vsnprintf (text, sizeof (text), error, argptr);
	va_end (argptr);

	Sys_SetConsoleTitle ("EGL ERROR!");
	Sys_SetErrorText (text);
	Sys_ShowConsole (1, qTrue);

#ifndef DEDICATED_ONLY
	if (!dedicated->intVal)
		CL_ClientShutdown (qTrue);
#endif
	Com_Shutdown ();

	timeEndPeriod (1);

	// Wait for the user to quit
	for ( ; ; ) {
		if (!GetMessage (&msg, NULL, 0, 0))
			Com_Quit (qTrue);

		TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}

	Sys_DestroyConsole ();

	exit (1);
}


/*
================
Sys_Print
================
*/
void Sys_Print (const char *message)
{
	Conbuf_AppendText (message);
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

	return timeGetTime () - base;
}


/*
================
Sys_UMilliseconds
================
*/
uint32 Sys_UMilliseconds (void)
{
	static uint32	base;
	static qBool	initialized = qFalse;

	if (!initialized) {
		// Let base retain 16 bits of effectively random data
		base = timeGetTime() & 0xffff0000;
		initialized = qTrue;
	}

	return timeGetTime () - base;
}


/*
=================
Sys_AppActivate
=================
*/
void Sys_AppActivate (void)
{
#ifndef DEDICATED_ONLY
	if (dedicated->intVal)
		return;

	SetForegroundWindow (sys_winInfo.hWnd);
	ShowWindow (sys_winInfo.hWnd, SW_SHOW);
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

	// Dispatch window messages
	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE)) {
		if (!GetMessage (&msg, NULL, 0, 0))
			Com_Quit (qTrue);

		sys_winInfo.msgTime = msg.time;
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}

#ifndef DEDICATED_ONLY
	// Update the client
	if (!dedicated->intVal)
		CL_UpdateFrameTime (timeGetTime ());
#endif
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

static qBool Sys_CompareFileAttributes (int found, uint32 mustHave, uint32 cantHave)
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
char *Sys_FindFirst (char *path, uint32 mustHave, uint32 cantHave)
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
char *Sys_FindNext (uint32 mustHave, uint32 cantHave)
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

#if defined(_M_IX86)
# define LIBARCH		"x86"
# ifdef _DEBUG
#  define LIBDEBUGDIR	"debug"
# else
#  define LIBDEBUGDIR	"release"
# endif

#elif defined(_M_ALPHA)
# define LIBARCH		"axp"
# ifdef _DEBUG
#  define LIBDEBUGDIR	"debugaxp"
# else
#  define LIBDEBUGDIR	"releaseaxp"
# endif
#endif

typedef struct libList_s {
	const char		*title;
	HINSTANCE		hInst;
	const char		*fileName;
	LPCSTR			apiFuncName;
} libList_t;

static libList_t sys_libList[LIB_MAX] = {
	{ "LIB_CGAME",	NULL,	"eglcgame" LIBARCH ".dll",	"GetCGameAPI"	},
	{ "LIB_GAME",	NULL,	"game" LIBARCH ".dll",		"GetGameAPI"	},
};

/*
=================
Sys_UnloadLibrary
=================
*/
void Sys_UnloadLibrary (libType_t libType)
{
	HINSTANCE	*lib;

	// Sanity check
	if (libType < 0 || libType > LIB_MAX)
		Com_Error (ERR_FATAL, "Sys_UnloadLibrary: Bad libType (%i)", libType);

	// Find the library
	lib = &sys_libList[libType].hInst;

	// Free it
	Com_DevPrintf (0, "FreeLibrary (%s)\n", sys_libList[libType].title);
	if (!FreeLibrary (*lib))
		Com_Error (ERR_FATAL, "FreeLibrary (%s) failed", sys_libList[libType].title);

	*lib = NULL;
}


/*
=================
Sys_LoadLibrary

Loads a module
=================
*/
void *Sys_LoadLibrary (libType_t libType, void *parms)
{
	HINSTANCE	*lib;
	const char	*libName;
	void		*(*APIfunc) (void *);
	char		name[MAX_OSPATH];
	char		cwd[MAX_OSPATH];
	char		*path;

	// Sanity check
	if (libType < 0 || libType > LIB_MAX)
		Com_Error (ERR_FATAL, "Sys_UnloadLibrary: Bad libType (%i)", libType);

	// Find the library
	lib = &sys_libList[libType].hInst;
	libName = sys_libList[libType].fileName;

	// Make sure it's not already loaded first
	if (*lib)
		Com_Error (ERR_FATAL, "Sys_LoadLibrary (%s) without Sys_UnloadLibrary", sys_libList[libType].title);

	// Check the current debug directory first for development purposes
	_getcwd (cwd, sizeof (cwd));
	Q_snprintfz (name, sizeof(name), "%s/%s/%s", cwd, LIBDEBUGDIR, libName);
	*lib = LoadLibrary (name);
	if (*lib) {
		Com_DevPrintf (0, "Sys_LoadLibrary (%s)\n", name);
	}
	else {
#ifdef _DEBUG
		// Check the current directory for other development purposes
		Q_snprintfz (name, sizeof (name), "%s/%s", cwd, libName);
		*lib = LoadLibrary (name);
		if (*lib) {
			Com_DevPrintf (0, "Sys_LoadLibrary (%s)\n", libName);
		}
		else {
#endif
			// Now run through the search paths
			path = NULL;
			for ( ; ; ) {
				path = FS_NextPath (path);
				if (!path)
					return NULL;	// Couldn't find one anywhere

				Q_snprintfz (name, sizeof(name), "%s/%s", path, libName);
				*lib = LoadLibrary (name);
				if (*lib) {
					Com_DevPrintf (0, "Sys_LoadLibrary (%s)\n",name);
					break;
				}
			}
#ifdef _DEBUG
		}
#endif
	}

	// Find the API function
	APIfunc = (void *)GetProcAddress (*lib, sys_libList[libType].apiFuncName);
	if (!APIfunc) {
		Sys_UnloadLibrary (libType);
		return NULL;
	}

	return APIfunc (parms);
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

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int		time, oldTime, newTime;

	// Previous instances do not exist in Win32
	if (hPrevInstance)
		return 0;

	sys_winInfo.hInstance = hInstance;

	// Create the console
	Sys_CreateConsole ();

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

	// No abort/retry/fail errors
	SetErrorMode (SEM_FAILCRITICALERRORS);

	// Common intialization
	Com_Init (sys_argCnt, sys_argVars);
	oldTime = Sys_Milliseconds ();

#ifndef DEDICATED_ONLY
	// Show the console
	if (!dedicated->intVal)
		Sys_ShowConsole (0, qFalse);
#endif

	// Pump message loop
	Sys_SendKeyEvents ();

	// Initial loop
	for ( ; ; ) {
		// If at a full screen console, don't update unless needed
#ifndef DEDICATED_ONLY
		if (sys_winInfo.appMinimized || dedicated->intVal)
#endif
			Sleep (5);

		do {
			newTime = Sys_Milliseconds ();
			time = newTime - oldTime;
		} while (time < 1);

		_controlfp (_PC_24, _MCW_PC);
		Com_Frame (time);

		oldTime = newTime;
	}

	// Never gets here
	return 0;
}
