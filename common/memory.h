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
// memory.h
//

/*
==============================================================================

	MEMORY ALLOCATION

==============================================================================
*/

#define MEM_SENTINEL1	0xFEBDFAED
#define MEM_SENTINEL2	0xDFBA

typedef struct memChunk_s {
	struct memChunk_s	*next;

	uint32				sentinel1;		// For memory integrity checking

	int					poolNum;		// Each pool has it's own tags
	int					tagNum;			// For group free
	size_t				size;			// Size of allocation including this header

	const char			*allocFile;		// File the memory was allocated in
	int					allocLine;		// Line the memory was allocated at

	void				*memPointer;	// pointer to allocated memory
	size_t				memSize;		// Size minus the header

	uint32				sentinel2;		// For memory integrity checking
} memChunk_t;

// these are private to the client
enum {
	MEMPOOL_GENERIC,

	MEMPOOL_FILESYS,
	MEMPOOL_GUISYS,
	MEMPOOL_IMAGESYS,
	MEMPOOL_LOCSYS,
	MEMPOOL_MODELSYS,
	MEMPOOL_SHADERSYS,
	MEMPOOL_SOUNDSYS,

	MEMPOOL_GAMESYS,
	MEMPOOL_CGAMESYS,

	MEMPOOL_MAX
};

// constants
#define Mem_Free(ptr)									_Mem_Free((ptr),__FILE__,__LINE__)
#define Mem_FreeTag(poolNum,tagNum)						_Mem_FreeTag((poolNum),(tagNum),__FILE__,__LINE__)
#define Mem_FreePool(poolNum)							_Mem_FreePool((poolNum),__FILE__,__LINE__)
#define Mem_Alloc(size)									_Mem_Alloc((size),qTrue,MEMPOOL_GENERIC,0,__FILE__,__LINE__)
#define Mem_AllocExt(size,zeroFill)						_Mem_Alloc((size),(zeroFill),MEMPOOL_GENERIC,0,__FILE__,__LINE__)
#define Mem_PoolAlloc(size,poolNum,tagNum)				_Mem_Alloc((size),qTrue,(poolNum),(tagNum),__FILE__,__LINE__)
#define Mem_PoolAllocExt(size,zeroFill,poolNum,tagNum)	_Mem_Alloc((size),(zeroFill),(poolNum),(tagNum),__FILE__,__LINE__)

#define Mem_StrDup(in)									_Mem_PoolStrDup((in),MEMPOOL_GENERIC,0,__FILE__,__LINE__)
#define Mem_PoolStrDup(in,poolNum,tagNum)				_Mem_PoolStrDup((in),(poolNum),(tagNum),__FILE__,__LINE__)
#define Mem_PoolSize(poolNum)							_Mem_PoolSize((poolNum))
#define Mem_TagSize(poolNum,tagNum)						_Mem_TagSize((poolNum),(tagNum))
#define Mem_ChangeTag(poolNum,tagFrom,tagTo)			_Mem_ChangeTag((poolNum),(tagFrom),(tagTo))

#define Mem_CheckPoolIntegrity(poolNum)					_Mem_CheckPoolIntegrity((poolNum),__FILE__,__LINE__)
#define Mem_CheckGlobalIntegrity()						_Mem_CheckGlobalIntegrity(__FILE__,__LINE__)

#define Mem_TouchPoolMemory(poolNum)					_Mem_TouchPoolMemory((poolNum),__FILE__,__LINE__)
#define Mem_TouchGlobalMemory()							_Mem_TouchGlobalMemory(__FILE__,__LINE__)

// functions
void	_Mem_Free (const void *ptr, const char *fileName, const int fileLine);
void	_Mem_FreeTag (const int poolNum, const int tagNum, const char *fileName, const int fileLine);
void	_Mem_FreePool (const int poolNum, const char *fileName, const int fileLine);
void	*_Mem_Alloc (size_t size, qBool zeroFill, const int poolNum, const int tagNum, const char *fileName, const int fileLine);

char	*_Mem_PoolStrDup (const char *in, const int poolNum, const int tagNum, const char *fileName, const int fileLine);
int		_Mem_PoolSize (const int poolNum);
int		_Mem_TagSize (const int poolNum, const int tagNum);
int		_Mem_ChangeTag (const int poolNum, const int tagFrom, const int tagTo);

void	_Mem_CheckPoolIntegrity (const int poolNum, const char *fileName, const int fileLine);
void	_Mem_CheckGlobalIntegrity (const char *fileName, const int fileLine);

void	_Mem_TouchPoolMemory (const int poolNum, const char *fileName, const int fileLine);
void	_Mem_TouchGlobalMemory (const char *fileName, const int fileLine);

void	Mem_Register (void);
void	Mem_Init (void);
