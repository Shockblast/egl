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

// mem_linux.c

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "../linux/glob.h"

#include "../common/common.h"

//===============================================================================

byte	*hunk_MemBase;
int		hunk_MaxSize;
int		hunk_CurSize;

/*
================
Hunk_Begin
================
*/
void *Hunk_Begin (int maxSize) {
	if (maxSize <= 0) {
		Com_Printf (PRINT_DEV, S_COLOR_RED "Hunk_Begin: Attempted allocation of <= 0 memory\n");
		return NULL;
	}

	// reserve a huge chunk of memory, but don't commit any yet
	hunk_MaxSize = maxSize + sizeof(int);
	hunk_CurSize = 0;
	hunk_MemBase = mmap (0, hunk_MaxSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

	if ((hunk_MemBase == NULL) || (hunk_MemBase == (byte *)-1))
		Sys_Error ("unable to virtual allocate %d bytes", maxSize);

	*((int *)hunk_MemBase) = hunk_CurSize;

	return hunk_MemBase + sizeof(int);
}


/*
================
Hunk_End
================
*/
int Hunk_End (void) {
	byte *n;

	n = mremap(hunk_MemBase, hunk_MaxSize, hunk_CurSize + sizeof(int), 0);
	if (n != hunk_MemBase)
		Sys_Error("Hunk_End:  Could not remap virtual block (%d)", errno);
	*((int *)hunk_MemBase) = hunk_CurSize + sizeof(int);
	
	return hunk_CurSize;
}


/*
================
Hunk_Alloc
================
*/
void *Hunk_Alloc (int size) {
	if (size > 0) {
		byte *buf;

		// round to cacheline
		size = (size+31)&~31;
		if (hunk_CurSize + size > hunk_MaxSize)
			Sys_Error ("Hunk_Alloc overflow");

		buf = hunk_MemBase + sizeof(int) + hunk_CurSize;
		hunk_CurSize += size;
		return buf;
	}

	Com_Printf (PRINT_DEV, S_COLOR_RED "Hunk_Alloc: Attempted allocation of <= 0 memory\n");
	return NULL;
}


/*
================
Hunk_Free
================
*/
void Hunk_Free (void *base) {
	if (base) {
		byte	*m;
		m = ((byte *)base) - sizeof(int);
		if (munmap (m, *((int *)m)))
			Sys_Error ("Hunk_Free: munmap failed (%d)", errno);
	} else
		Com_Printf (PRINT_DEV, S_COLOR_RED "Hunk_Free: NULL base\n");
}
