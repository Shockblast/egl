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

// mem.c

#include "common.h"

/*
==============================================================================

	ZONE MEMORY ALLOCATION

==============================================================================
*/

#define	Z_MAGIC		0x1d1d

typedef struct zHead_s {
	struct zHead_s	*prev, *next;
	short			magic;
	short			tag;	// for group free
	int				size;
} zHead_t;

zHead_t		z_chain;
int			z_count, z_bytes;

/*
========================
Z_Free
========================
*/
void Z_Free (void *ptr) {
	zHead_t	*z;

	z = ((zHead_t *)ptr) - 1;

	if (z->magic != Z_MAGIC)
		Com_Error (ERR_FATAL, "Z_Free: bad magic");

	z->prev->next = z->next;
	z->next->prev = z->prev;

	z_count--;
	z_bytes -= z->size;
	free (z);
}


/*
========================
Z_FreeTags
========================
*/
void Z_FreeTags (int tag) {
	zHead_t	*z, *next;

	for (z=z_chain.next ; z != &z_chain ; z=next) {
		next = z->next;
		if (z->tag == tag)
			Z_Free ((void *)(z+1));
	}
}


/*
========================
Z_Malloc
========================
*/
void *Z_Malloc (int size) {
	return Z_TagMalloc (size, 0);
}


/*
========================
Z_TagMalloc
========================
*/
void *Z_TagMalloc (int size, int tag) {
	zHead_t	*z;

	if (size <= 0) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Z_TagMalloc: Attempted allocation of '0' memory ignored.\n");
		return NULL;
	}

	size = size + sizeof (zHead_t);
	z = malloc (size);
	if (!z)
		Com_Error (ERR_FATAL, "Z_Malloc: failed on allocation of %i bytes", size);

	memset (z, 0, size);
	z_count++;
	z_bytes += size;
	z->magic = Z_MAGIC;
	z->tag = tag;
	z->size = size;

	z->next = z_chain.next;
	z->prev = &z_chain;
	z_chain.next->prev = z;
	z_chain.next = z;

	return (void *)(z+1);
}


/*
========================
Z_Stats_f
========================
*/
void Z_Stats_f (void) {
	Com_Printf (PRINT_ALL, "%i bytes in %i blocks\n", z_bytes, z_count);
}

/*
==============================================================================

	INIT / SHUTDOWN

==============================================================================
*/

/*
========================
Z_Init
========================
*/
void Z_Init (void)
{
	z_chain.next = z_chain.prev = &z_chain;

	Cmd_AddCommand ("z_stats",	Z_Stats_f);
}


/*
========================
Z_Shutdown
========================
*/
void Z_Shutdown (void)
{
	Cmd_RemoveCommand ("z_stats");

	Z_FreeTags (ZTAG_CMDSYS);
	Z_FreeTags (ZTAG_CVARSYS);
	Z_FreeTags (ZTAG_KEYSYS);
	Z_FreeTags (ZTAG_FILESYS);
}
