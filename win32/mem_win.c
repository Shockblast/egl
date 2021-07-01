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

// mem_win.c

#include "../common/common.h"
#include "winquake.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>

#define HUNK_USEVA
#define HUNK_MARKER		0x01dd021f

typedef struct memHunk_s {
	struct memHunk_s	*prev, *next;
	int					totalSize;
	int					*sizes;
	int					marker;		// must be HUNK_MARKER
} memHunk_t;

int			hunkCount;
int			hunkCurSize;
int			hunkMaxSize;
byte		*hunkMemBase;

/*
================
Hunk_Begin
================
*/
void *Hunk_Begin (int maxSize) {
	if (maxSize <= 0) {
		Com_Printf (PRINT_DEV, S_COLOR_RED "Hunk_Begin: Attempted allocation of '0' memory ignored.\n");
		return NULL;
	}

	// reserve a huge chunk of memory, but don't commit any yet
	hunkCurSize = 0;
	hunkMaxSize = maxSize;
#ifdef HUNK_USEVA
	hunkMemBase = VirtualAlloc (
					NULL,
					maxSize,
					MEM_RESERVE,
					PAGE_NOACCESS);
#else
	hunkMemBase = malloc (maxSize);
	memset (hunkMemBase, 0, maxSize);
#endif
	if (!hunkMemBase)
		Sys_Error ("VirtualAlloc reserve failed");

	return (void *)hunkMemBase;
}


/*
================
Hunk_End
================
*/
int Hunk_End (void) {
	// free the remaining unused virtual memory
	hunkCount++;
	return hunkCurSize;
}


/*
================
Hunk_Alloc
================
*/
void *Hunk_Alloc (int size) {
#ifdef HUNK_USEVA
	void	*buf;
#endif

	if (size <= 0) {
		Com_Printf (PRINT_DEV, S_COLOR_RED "Hunk_Alloc: Attempted allocation of <= 0 memory\n");
		return NULL;
	}

	// round to cacheline
	size = (size+31)&~31;

#ifdef HUNK_USEVA
	// commit pages as needed
	buf = VirtualAlloc (
			hunkMemBase,
			hunkCurSize + size,
			MEM_COMMIT,
			PAGE_READWRITE);

	if (!buf) {
		FormatMessage (
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			GetLastError (),
			MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &buf,
			0,
			NULL);

		Sys_Error ("VirtualAlloc commit failed.\n%s", buf);
	}
#endif
	hunkCurSize += size;
	if (hunkCurSize > hunkMaxSize)
		Sys_Error ("Hunk_Alloc overflow");

	return (void *)(hunkMemBase + hunkCurSize - size);
}


/*
================
Hunk_Free
================
*/
void Hunk_Free (void *base) {
	if (base) {
#ifdef HUNK_USEVA
		VirtualFree (base, 0, MEM_RELEASE);
#else
		free (base);
#endif
	} else
		Com_Printf (PRINT_DEV, S_COLOR_RED "Hunk_Free: NULL base\n");

	hunkCount--;
}
