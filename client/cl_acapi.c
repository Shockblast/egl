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
// cl_acapi.c
// Anti-cheat API
//

#include "cl_local.h"

#ifdef CL_ANTICHEAT

#if defined(WIN32)

# define WIN32_LEAN_AND_MEAN 1
# include <windows.h>

static HMODULE				cl_acLibrary;

# define AC_LIBNAME			"anticheat.so"
# define AC_LOADLIB(a)		LoadLibrary (a)
# define AC_FREELIB(a)		FreeLibrary (a)
# define AC_GPA(a)			GetProcAddress (cl_acLibrary, a)

#elif defined __unix__

# include <dlfcn.h>
# include <unistd.h>
# include <sys/types.h>

static void					*cl_acLibrary;

# define AC_LIBNAME			"anticheat.dll"
# define AC_LOADLIB(a)		dlopen (a, RTLD_LAZY|RTLD_GLOBAL)
# define AC_FREELIB(a)		dlclose (a)
# define AC_GPA(a)			dlsym (cl_acLibrary, a)

#endif

typedef struct acExport_s {
	void	(*Check) (void);
} acExport_t;

static acExport_t *acex;

typedef void *(*FNINIT) (void);

/*
===============
CL_ACAPI_Init
===============
*/
qBool CL_ACAPI_Init (void)
{
	qBool				updated = qFalse;
	static FNINIT		init;

	// Already loaded, just re-initialize
	if (acex) {
		acex = (acExport_t *)init ();
		return (acex) ? qTrue : qFalse;
	}

reInit:
	cl_acLibrary = AC_LOADLIB ("anticheat");
	if (!cl_acLibrary)
		return qFalse;

	init = (FNINIT)AC_GPA ("Initialize");
	acex = (acExport_t *)init ();
	if (!updated && !acex) {
		updated = qTrue;
		AC_FREELIB (cl_acLibrary);
		cl_acLibrary = NULL;
		goto reInit;
	}

	return (acex) ? qTrue : qFalse;
}

#endif // CL_ANTICHEAT
