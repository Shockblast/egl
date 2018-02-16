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
// FIXME TODO: other neat features like maximum size, hunk management?
//

#include "common.h"

#define MEM_HEAD_SENTINEL_TOP	0xFEBDFAED
#define MEM_HEAD_SENTINEL_BOT	0xD0BAF0FF
#define MEM_FOOT_SENTINEL		0xF00DF00D

typedef struct memBlockFoot_s {
	uint32				sentinel;					// For memory integrity checking
} memBlockFoot_t;

typedef struct memBlock_s {
	struct memBlock_s	*next;

	uint32				topSentinel;				// For memory integrity checking

	struct memPool_s	*pool;						// Owner pool
	int					tagNum;						// For group free
	size_t				size;						// Size of allocation including this header

	const char			*allocFile;					// File the memory was allocated in
	int					allocLine;					// Line the memory was allocated at

	void				*memPointer;				// pointer to allocated memory
	size_t				memSize;					// Size minus the header

	memBlockFoot_t		*footer;					// Allocated in the space AFTER the block to check for overflow

	uint32				botSentinel;				// For memory integrity checking
} memBlock_t;

#define MEM_MAX_POOLCOUNT	32
#define MEM_MAX_POOLNAME	64

typedef struct memPool_s {
	char				name[MEM_MAX_POOLNAME];		// Name of pool
	qBool				inUse;						// Slot in use?

	memBlock_t			*blocks;					// Allocated blocks

	uint32				blockCount;					// Total allocated blocks
	uint32				byteCount;					// Total allocated bytes

	const char			*createFile;				// File this pool was created on
	int					createLine;					// Line this pool was created on
} memPool_t;

static memPool_t	m_poolList[MEM_MAX_POOLCOUNT];
static uint32		m_numPools;

memPool_t			*m_genericPool;

/*
==============================================================================

	POOL MANAGEMENT

==============================================================================
*/

/*
========================
Mem_FindPool
========================
*/
static memPool_t *Mem_FindPool (const char *name)
{
	memPool_t	*pool;
	uint32		i;

	for (i=0, pool=&m_poolList[0] ; i<m_numPools ; pool++, i++) {
		if (!pool->inUse)
			continue;
		if (strcmp (name, pool->name))
			continue;

		return pool;
	}

	return NULL;
}


/*
========================
_Mem_CreatePool
========================
*/
memPool_t *_Mem_CreatePool (const char *name, const char *fileName, const int fileLine)
{
	memPool_t	*pool;
	uint32		i;

	// Check name
	if (!name || !name[0])
		Com_Error (ERR_FATAL, "Mem_CreatePool: NULL name %s:#%i", fileName, fileLine);
	if (strlen(name)+1 >= MEM_MAX_POOLNAME)
		Com_Printf (PRNT_WARNING, "Mem_CreatePoole: name '%s' too long, truncating!\n", name);

	// See if it already exists
	pool = Mem_FindPool (name);
	if (pool) {
		return pool;
	}

	// Nope, create a slot
	for (i=0, pool=&m_poolList[0] ; i<m_numPools ; pool++, i++) {
		if (!pool->inUse)
			break;
	}
	if (i == m_numPools) {
		if (m_numPools+1 >= MEM_MAX_POOLCOUNT)
			Com_Error (ERR_FATAL, "Mem_CreatePool: MEM_MAX_POOLCOUNT");
		pool = &m_poolList[m_numPools++];
	}

	// Store values
	pool->blocks = NULL;
	pool->blockCount = 0;
	pool->byteCount = 0;
	pool->createFile = fileName;
	pool->createLine = fileLine;
	pool->inUse = qTrue;
	Q_strncpyz (pool->name, name, sizeof (pool->name));
	return pool;
}


/*
========================
_Mem_DeletePool
========================
*/
uint32 _Mem_DeletePool (struct memPool_s *pool, const char *fileName, const int fileLine)
{
	uint32	size;

	if (!pool)
		return 0;

	// Release all allocated memory
	size = _Mem_FreePool (pool, fileName, fileLine);

	// Simple, yes?
	pool->inUse = qFalse;
	pool->name[0] = '\0';

	return size;
}


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
uint32 _Mem_Free (const void *ptr, const char *fileName, const int fileLine)
{
	memBlock_t	*mem;
	memBlock_t	*search;
	memBlock_t	**prev;
	uint32		size;

	assert (ptr);
	if (!ptr)
		return 0;

	// Check sentinels
	mem = (memBlock_t *)((byte *)ptr - sizeof (memBlock_t));
	if (mem->topSentinel != MEM_HEAD_SENTINEL_TOP) {
		Com_Error (ERR_FATAL,
			"Mem_Free: bad memory header top sentinel [buffer underflow]\n"
			"free: %s:#%i",
			fileName, fileLine);
	}
	else if (mem->botSentinel != MEM_HEAD_SENTINEL_BOT) {
		Com_Error (ERR_FATAL,
			"Mem_Free: bad memory header bottom sentinel [buffer underflow]\n"
			"free: %s:#%i",
			fileName, fileLine);
	}
	else if (!mem->footer) {
		Com_Error (ERR_FATAL,
			"Mem_Free: bad memory footer [buffer overflow]\n"
			"pool: %s\n"
			"alloc: %s:#%i\n"
			"free: %s:#%i",
			mem->pool ? mem->pool->name : "UNKNOWN", mem->allocFile, mem->allocLine, fileName, fileLine);
	}
	else if (mem->footer->sentinel != MEM_FOOT_SENTINEL) {
		Com_Error (ERR_FATAL,
			"Mem_Free: bad memory footer sentinel [buffer overflow]\n"
			"pool: %s\n"
			"alloc: %s:#%i\n"
			"free: %s:#%i",
			mem->pool ? mem->pool->name : "UNKNOWN", mem->allocFile, mem->allocLine, fileName, fileLine);
	}

	// Decrement counters
	mem->pool->blockCount--;
	mem->pool->byteCount -= mem->size;
	size = mem->size;

	// De-link it
	prev = &mem->pool->blocks;
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
	return size;
}


/*
========================
_Mem_FreeTag

Free memory blocks assigned to a specified tag within a pool
========================
*/
uint32 _Mem_FreeTag (struct memPool_s *pool, const int tagNum, const char *fileName, const int fileLine)
{
	memBlock_t	*mem, *next;
	uint32		size;

	if (!pool)
		return 0;

	size = 0;
	for (mem=pool->blocks; mem ; mem=next) {
		next = mem->next;
		if (mem->tagNum == tagNum)
			size += _Mem_Free (mem->memPointer, fileName, fileLine);
	}

	return size;
}


/*
========================
_Mem_FreePool

Free all items within a pool
========================
*/
uint32 _Mem_FreePool (struct memPool_s *pool, const char *fileName, const int fileLine)
{
	memBlock_t	*mem, *next;
	uint32		size;

	if (!pool)
		return 0;

	size = 0;
	for (mem=pool->blocks ; mem ; mem=next) {
		next = mem->next;
		size += _Mem_Free (mem->memPointer, fileName, fileLine);
	}

	assert (pool->blockCount == 0);
	assert (pool->byteCount == 0);
	return size;
}


/*
========================
_Mem_Alloc

Optionally returns 0 filled memory allocated in a pool with a tag
========================
*/
void *_Mem_Alloc (size_t size, qBool zeroFill, struct memPool_s *pool, const int tagNum, const char *fileName, const int fileLine)
{
	memBlock_t	*mem;

	// Check pool
	if (!pool)
		return NULL;

	// Check size
	if (size <= 0) {
		Com_DevPrintf (PRNT_WARNING, "Mem_Alloc: Attempted allocation of '%i' memory ignored\n" "alloc: %s:#%i\n", size, fileName, fileLine);
		return NULL;
	}
	if (size > 0x40000000)
		Com_Error (ERR_FATAL, "Mem_Alloc: Attempted allocation of '%i' bytes!\n" "alloc: %s:#%i\n", size, fileName, fileLine);

	// Add header and round to cacheline
	size = (size + sizeof (memBlock_t) + sizeof (memBlockFoot_t) + 31) & ~31;
#ifdef MEM_USE_CALLOC
	if (zeroFill)
		mem = calloc (1, size);
	else
		mem = malloc (size);
#else
	mem = malloc (size);
#endif
	if (!mem)
		Com_Error (ERR_FATAL, "Mem_Alloc: failed on allocation of %i bytes\n" "alloc: %s:#%i", size, fileName, fileLine);

#ifndef MEM_USE_CALLOC
	// Zero fill
	if (zeroFill)
		memset (mem, 0, size);
#endif

	// For integrity checking and stats
	pool->blockCount++;
	pool->byteCount += size;

	// Fill in the header
	mem->topSentinel = MEM_HEAD_SENTINEL_TOP;
	mem->tagNum = tagNum;
	mem->size = size;
	mem->memPointer = (void *)(mem+1);
	mem->memSize = size - sizeof (memBlock_t) - sizeof (memBlockFoot_t);
	mem->pool = pool;
	mem->allocFile = fileName;
	mem->allocLine = fileLine;
	mem->footer = (memBlockFoot_t *)((byte *)mem->memPointer + mem->memSize);
	mem->botSentinel = MEM_HEAD_SENTINEL_BOT;

	// Fill in the footer
	mem->footer->sentinel = MEM_FOOT_SENTINEL;

	// Link it in to the appropriate pool
	mem->next = pool->blocks;
	pool->blocks = mem;

	return mem->memPointer;
}

/*
==============================================================================

	MISC FUNCTIONS

==============================================================================
*/

/*
================
_Mem_PoolStrDup

No need to null terminate the extra spot because Mem_Alloc returns zero-filled memory
================
*/
char *_Mem_PoolStrDup (const char *in, struct memPool_s *pool, const int tagNum, const char *fileName, const int fileLine)
{
	char	*out;

	out = _Mem_Alloc ((size_t)(strlen (in) + 1), qTrue, pool, tagNum, fileName, fileLine);
	strcpy (out, in);

	return out;
}


/*
================
_Mem_PoolSize
================
*/
uint32 _Mem_PoolSize (struct memPool_s *pool)
{
	if (!pool)
		return 0;

	return pool->byteCount;
}


/*
================
_Mem_TagSize
================
*/
uint32 _Mem_TagSize (struct memPool_s *pool, const int tagNum)
{
	memBlock_t	*mem;
	uint32		size;

	if (!pool)
		return 0;

	size = 0;
	for (mem=pool->blocks ; mem ; mem=mem->next) {
		if (mem->tagNum == tagNum)
			size += mem->size;
	}

	return size;
}


/*
========================
_Mem_ChangeTag
========================
*/
uint32 _Mem_ChangeTag (struct memPool_s *pool, const int tagFrom, const int tagTo)
{
	memBlock_t	*mem;
	uint32		numChanged;

	if (!pool)
		return 0;

	numChanged = 0;
	for (mem=pool->blocks ; mem ; mem=mem->next) {
		if (mem->tagNum == tagFrom) {
			mem->tagNum = tagTo;
			numChanged++;
		}
	}

	return numChanged;
}


/*
========================
_Mem_CheckPoolIntegrity
========================
*/
void _Mem_CheckPoolIntegrity (struct memPool_s *pool, const char *fileName, const int fileLine)
{
	memBlock_t	*mem;
	uint32		blocks;
	uint32		size;

	assert (pool);
	if (!pool)
		return;

	// Check sentinels
	for (mem=pool->blocks, blocks=0, size=0 ; mem ; blocks++, mem=mem->next) {
		size += mem->size;
		if (mem->topSentinel != MEM_HEAD_SENTINEL_TOP) {
			Com_Error (ERR_FATAL,
				"Mem_CheckPoolIntegrity: bad memory head top sentinel [buffer underflow]\n"
				"check: %s:#%i",
				fileName, fileLine);
		}
		else if (mem->botSentinel != MEM_HEAD_SENTINEL_BOT) {
			Com_Error (ERR_FATAL,
				"Mem_CheckPoolIntegrity: bad memory head bottom sentinel [buffer underflow]\n"
				"check: %s:#%i",
				fileName, fileLine);
		}
		else if (!mem->footer) {
			Com_Error (ERR_FATAL,
				"Mem_CheckPoolIntegrity: bad memory footer [buffer overflow]\n"
				"pool: %s\n"
				"alloc: %s:#%i\n"
				"check: %s:#%i",
				mem->pool ? mem->pool->name : "UNKNOWN", mem->allocFile, mem->allocLine, fileName, fileLine);
		}
		else if (mem->footer->sentinel != MEM_FOOT_SENTINEL) {
			Com_Error (ERR_FATAL,
				"Mem_CheckPoolIntegrity: bad memory foot sentinel [buffer overflow]\n"
				"pool: %s\n"
				"alloc: %s:#%i\n"
				"check: %s:#%i",
				mem->pool ? mem->pool->name : "UNKNOWN", mem->allocFile, mem->allocLine, fileName, fileLine);
		}
	}

	// Check block/byte counts
	if (pool->blockCount != blocks)
		Com_Error (ERR_FATAL, "Mem_CheckPoolIntegrity: bad block count\n" "check: %s:#%i", fileName, fileLine);
	if (pool->byteCount != size)
		Com_Error (ERR_FATAL, "Mem_CheckPoolIntegrity: bad pool size\n" "check: %s:#%i", fileName, fileLine);
}


/*
========================
_Mem_CheckGlobalIntegrity
========================
*/
void _Mem_CheckGlobalIntegrity (const char *fileName, const int fileLine)
{
	memPool_t	*pool;
	uint32		startTime;
	uint32		i;

	startTime = Sys_UMilliseconds ();

	for (i=0, pool=&m_poolList[0] ; i<m_numPools ; pool++, i++) {
		if (pool->inUse)
			_Mem_CheckPoolIntegrity (pool, fileName, fileLine);
	}

	Com_DevPrintf (0, "Mem_CheckGlobalIntegrity: %ims\n", Sys_UMilliseconds()-startTime);
}


/*
========================
_Mem_TouchPool
========================
*/
void _Mem_TouchPool (struct memPool_s *pool, const char *fileName, const int fileLine)
{
	memBlock_t	*mem;
	uint32		blocks;
	uint32		i;
	int			sum;
	uint32		startTime;

	assert (pool);
	if (!pool)
		return;

	sum = 0;
	startTime = Sys_UMilliseconds();

	// Cycle through the blocks
	for (mem=pool->blocks, blocks=0 ; mem ; blocks++, mem=mem->next) {
		// Touch each page
		for (i=0 ; i<mem->memSize ; i+=128) {
			sum += ((byte *)mem->memPointer)[i];
		}
	}
}


/*
========================
_Mem_TouchGlobal
========================
*/
void _Mem_TouchGlobal (const char *fileName, const int fileLine)
{
	memPool_t	*pool;
	uint32		startTime;
	uint32		i, num;

	startTime = Sys_UMilliseconds ();

	// Touch every pool
	num = 0;
	for (i=0, pool=&m_poolList[0] ; i<m_numPools ; pool++, i++) {
		if (pool->inUse) {
			_Mem_TouchPool (pool, fileName, fileLine);
			num++;
		}
	}

	Com_DevPrintf (0, "Mem_TouchGlobal: %u pools touched in %ims\n", num, Sys_UMilliseconds()-startTime);
}

/*
==============================================================================

	CONSOLE COMMANDS

==============================================================================
*/

/*
========================
Mem_Check_f
========================
*/
static void Mem_Check_f (void)
{
	Mem_CheckGlobalIntegrity ();
}


/*
========================
Mem_Stats_f
========================
*/
static void Mem_Stats_f (void)
{
	uint32		totalBlocks, totalBytes;
	memPool_t	*pool;
	uint32		poolNum, i;

	if (Cmd_Argc () > 1) {
		memPool_t	*best;
		memBlock_t	*mem;

		best = NULL;
		for (i=0, pool=&m_poolList[0] ; i<m_numPools ; pool++, i++) {
			if (!pool->inUse)
				continue;
			if (strstr (pool->name, Cmd_Args())) {
				if (best) {
					Com_Printf (0, "Too many matches for '%s'...\n", Cmd_Args());
					return;
				}
				best = pool;
			}
		}
		if (!best) {
			Com_Printf (0, "No matches for '%s'...\n", Cmd_Args());
			return;
		}

		Com_Printf (0, "Pool stats for '%s':\n", best->name);
		Com_Printf (0, "block size       line  file\n");
		Com_Printf (0, "----- ---------- ----- -----------------\n");

		totalBytes = 0;
		for (i=0, mem=best->blocks ; mem ; mem=mem->next, i++) {
			if (i & 1)
				Com_Printf (0, S_COLOR_GREY);

			Com_Printf (0, "%5i %9iB %5i %s\n", i+1, mem->size, mem->allocLine, mem->allocFile);

			totalBytes += mem->size;
		}

		Com_Printf (0, "----------------------------------------\n");
		Com_Printf (0, "Total: %i blocks, %i bytes (%6.3fMB)\n", i, totalBytes, totalBytes/1048576.0f);
		return;
	}

	Com_Printf (0, "Memory stats:\n");
	Com_Printf (0, "    blocks size                  name\n");
	Com_Printf (0, "--- ------ ---------- ---------- --------\n");

	totalBlocks = 0;
	totalBytes = 0;
	poolNum = 0;
	for (i=0, pool=&m_poolList[0] ; i<m_numPools ; pool++, i++) {
		if (!pool->inUse)
			continue;

		poolNum++;
		if (poolNum & 1)
			Com_Printf (0, S_COLOR_GREY);

		Com_Printf (0, "#%2i %6i %9iB (%6.3fMB) %s\n", poolNum, pool->blockCount, pool->byteCount, pool->byteCount/1048576.0f, pool->name);

		totalBlocks += pool->blockCount;
		totalBytes += pool->byteCount;
	}

	Com_Printf (0, "----------------------------------------\n");
	Com_Printf (0, "Total: %i pools, %i blocks, %i bytes (%6.3fMB)\n", i, totalBlocks, totalBytes, totalBytes/1048576.0f);
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
	Cmd_AddCommand ("memcheck",		Mem_Check_f,		"Checks global memory integrity");
	Cmd_AddCommand ("memstats",		Mem_Stats_f,		"Prints out current internal memory statistics");
}


/*
========================
Mem_Init
========================
*/
void Mem_Init (void)
{
}
