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
// memory.c
// Memory handling with sentinel checking and pools with tags for grouped free'ing
//

#include "common.h"

static memChunk_t	*m_allocChains[MEMPOOL_MAX];
static uInt			m_blockCount[MEMPOOL_MAX];
static uInt			m_byteCount[MEMPOOL_MAX];

/*
==============================================================================

	POOL AND TAG MEMORY ALLOCATION

==============================================================================
*/

/*
========================
_Mem_Free
========================
*/
void _Mem_Free (const void *ptr, const char *fileName, const int fileLine)
{
	memChunk_t	*mem;
	memChunk_t	*search;
	memChunk_t	**prev;

	mem = (memChunk_t *)((byte *)ptr - sizeof (memChunk_t));

	// Check sentinels
	if (mem->sentinel1 != MEM_SENTINEL1)
		Com_Error (ERR_FATAL, "Mem_Free: bad memory head sentinel\n" "free: %s:#%i", fileName, fileLine);
	else if (mem->sentinel2 != MEM_SENTINEL2)
		Com_Error (ERR_FATAL, "Mem_Free: bad memory foot sentinel\n" "alloc: %s:#%i\n" "free: %s:#%i", mem->allocFile, mem->allocLine, fileName, fileLine);

	m_blockCount[mem->poolNum]--;
	m_byteCount[mem->poolNum] -= mem->size;

	// De-link it
	prev = &m_allocChains[mem->poolNum];
	for ( ; ; ) {
		search = *prev;
		if (!search)
			break;

		if (search == mem) {
			*prev = search->next;
			break;
		}
		prev = &search->next;
	}

	// Free it
	free (mem);
}


/*
========================
_Mem_FreeTags

Free tags within a pool
========================
*/
void _Mem_FreeTags (const int poolNum, const int tagNum, const char *fileName, const int fileLine)
{
	memChunk_t	*mem, *next;

	for (mem=m_allocChains[poolNum] ; mem ; mem=next) {
		next = mem->next;
		if (mem->tagNum == tagNum)
			_Mem_Free (mem->memPointer, fileName, fileLine);
	}
}


/*
========================
_Mem_FreePool

Free all items within a pool
========================
*/
void _Mem_FreePool (const int poolNum, const char *fileName, const int fileLine)
{
	memChunk_t	*mem, *next;

	for (mem=m_allocChains[poolNum] ; mem ; mem=next) {
		next = mem->next;
		_Mem_Free (mem->memPointer, fileName, fileLine);
	}
}


/*
========================
_Mem_Alloc
========================
*/
void *_Mem_Alloc (const size_t size, const char *fileName, const int fileLine)
{
	return _Mem_PoolAlloc (size, MEMPOOL_GENERIC, 0, fileName, fileLine);
}


/*
========================
_Mem_PoolAlloc

Returns 0 filled memory allocated in a pool with a tag
========================
*/
void *_Mem_PoolAlloc (size_t size, const int poolNum, const int tagNum, const char *fileName, const int fileLine)
{
	memChunk_t	*mem;

	// Check size
	if (size <= 0) {
		Com_Printf (PRNT_DEV, S_COLOR_YELLOW "Mem_PoolAlloc: Attempted allocation of '0' memory ignored\n" "\"%s\" line #%i\n", fileName, fileLine);
		return NULL;
	}

	// Add header and round to cacheline
	size = (size + sizeof (memChunk_t) + 31) & ~31;
	mem = malloc (size);
	if (!mem)
		Com_Error (ERR_FATAL, "Mem_PoolAlloc: failed on allocation of %i bytes\n" "\"%s\" line #%i", size, fileName, fileLine);

	// Zero fill
	memset (mem, 0, size);

	// For integrity checking and stats
	m_blockCount[poolNum]++;
	m_byteCount[poolNum] += size;

	// Fill in the header
	mem->sentinel1 = MEM_SENTINEL1;
	mem->poolNum = poolNum;
	mem->tagNum = tagNum;
	mem->size = size;
	mem->memPointer = (void *)(mem+1);
	mem->memSize = size - sizeof (memChunk_t);
	mem->allocFile = fileName;
	mem->allocLine = fileLine;
	mem->sentinel2 = MEM_SENTINEL2;

	// Link it in to the appropriate pool
	mem->next = m_allocChains[poolNum];
	m_allocChains[poolNum] = mem;

	return mem->memPointer;
}

/*
==============================================================================

	MISC FUNCTIONS

==============================================================================
*/

/*
================
_Mem_StrDup
================
*/
char *_Mem_StrDup (const char *in, const char *fileName, const int fileLine)
{
	return _Mem_PoolStrDup (in, MEMPOOL_GENERIC, 0, fileName, fileLine);
}


/*
================
_Mem_PoolStrDup

No need to null terminate the extra spot because Mem_Alloc returns zero-filled memory
================
*/
char *_Mem_PoolStrDup (const char *in, const int poolNum, const int tagNum, const char *fileName, const int fileLine)
{
	char	*out;

	out = _Mem_PoolAlloc ((size_t)(strlen (in) + 1), poolNum, tagNum, fileName, fileLine);
	strcpy (out, in);

	return out;
}


/*
================
_Mem_PoolSize
================
*/
int _Mem_PoolSize (const int poolNum, const char *fileName, const int fileLine)
{
	memChunk_t	*mem;
	int			size = 0;

	for (mem=m_allocChains[poolNum] ; mem ; mem=mem->next)
		size += mem->size;

	return size;
}


/*
================
_Mem_TagSize
================
*/
int _Mem_TagSize (const int poolNum, const int tagNum, const char *fileName, const int fileLine)
{
	memChunk_t	*mem, *next;
	int			size = 0;

	for (mem=m_allocChains[poolNum] ; mem ; mem=next) {
		next = mem->next;
		if (mem->tagNum == tagNum)
			size += mem->size;
	}

	return size;
}


/*
========================
_Mem_CheckPoolIntegrity
========================
*/
void _Mem_CheckPoolIntegrity (const int poolNum, const char *fileName, const int fileLine)
{
	memChunk_t	*mem;
	uInt		blocks;
	uInt		size;

	// Check sentinels
	for (mem=m_allocChains[poolNum], blocks=0, size=0 ; mem ; blocks++, mem=mem->next) {
		size += mem->size;
		if (mem->sentinel1 != MEM_SENTINEL1) {
			Com_Error (ERR_FATAL, "Mem_CheckPoolIntegrity: bad memory head sentinel\n" "check: %s:#%i", fileName, fileLine);
			break;
		}
		else if (mem->sentinel2 != MEM_SENTINEL2) {
			Com_Error (ERR_FATAL, "Mem_CheckPoolIntegrity: bad memory foot sentinel\n" "alloc: %s:#%i\n" "check: %s:#%i", mem->allocFile, mem->allocLine, fileName, fileLine);
			break;
		}
	}

	// Check block/byte counts
	if (m_blockCount[poolNum] != blocks)
		Com_Error (ERR_FATAL, "Mem_CheckPoolIntegrity: bad block count\n" "check: %s:#%i", fileName, fileLine);
	if (m_byteCount[poolNum] != size)
		Com_Error (ERR_FATAL, "Mem_CheckPoolIntegrity: bad pool size\n" "check: %s:#%i", fileName, fileLine);
}


/*
========================
_Mem_CheckGlobalIntegrity
========================
*/
void _Mem_CheckGlobalIntegrity (const char *fileName, const int fileLine)
{
	int		poolNum;

	for (poolNum=0 ; poolNum<MEMPOOL_MAX ; poolNum++)
		_Mem_CheckPoolIntegrity (poolNum, fileName, fileLine);
}

/*
==============================================================================

	CONSOLE COMMANDS

==============================================================================
*/

/*
========================
Mem_Stats_f
========================
*/
static void Mem_Stats_f (void)
{
	uInt	totalBlocks;
	uInt	totalBytes;
	int		i;

	Com_Printf (0, "Memory stats:\n");

	for (i=0, totalBlocks=0, totalBytes=0 ; i<MEMPOOL_MAX ; i++) {
		Com_Printf (0, "Pool %2i: %5i blocks %8i bytes (%0.2fMB)\n",
			i+1, m_blockCount[i], m_byteCount[i], m_byteCount[i]/1048576.0);

		totalBlocks += m_blockCount[i];
		totalBytes += m_byteCount[i];
	}

	Com_Printf (0, "Total: %i pools, %i blocks, %i bytes (%0.2fMB)\n", i, totalBlocks, totalBytes, totalBytes/1048576.0);
}

/*
==============================================================================

	INIT / SHUTDOWN

==============================================================================
*/

/*
========================
Mem_Register
========================
*/
void Mem_Register (void)
{
	Cmd_AddCommand ("memstats",		Mem_Stats_f,		"Prints out current internal memory statistics");
}


/*
========================
Mem_Init
========================
*/
void Mem_Init (void)
{
	memset (&m_blockCount, 0, sizeof (m_blockCount));
	memset (&m_byteCount, 0, sizeof (m_byteCount));
}
