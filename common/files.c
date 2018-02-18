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
// files.c
//

#include "common.h"
#include "../include/minizip/unzip.h"

#define FS_MAX_PAKS			1024
#define FS_MAX_HASHSIZE		1024
#define FS_MAX_FILEINDICES	1024

cVar_t	*fs_basedir;
cVar_t	*fs_cddir;
cVar_t	*fs_game;
cVar_t	*fs_gamedircvar;
cVar_t	*fs_defaultPaks;

/*
=============================================================================

	IN-MEMORY PAK FILES

=============================================================================
*/

typedef struct mPackFile_s {
	char					fileName[MAX_QPATH];

	int						filePos;
	int						fileLen;

	struct mPackFile_s		*hashNext;
} mPackFile_t;

typedef struct mPack_s {
	char					name[MAX_OSPATH];

	// Standard
	FILE					*pak;

	// Compressed
	unzFile					*pkz;

	// Information
	size_t					numFiles;
	mPackFile_t				*files;

	struct mPackFile_s		*fileHashTree[FS_MAX_HASHSIZE];
} mPack_t;

/*
=============================================================================

	FILESYSTEM FUNCTIONALITY

=============================================================================
*/

typedef struct fsLink_s {
	struct fsLink_s			*next;
	char					*from;
	size_t					fromLength;
	char					*to;
} fsLink_t;

typedef struct fsPath_s {
	char					pathName[MAX_OSPATH];
	char					gamePath[MAX_OSPATH];
	mPack_t					*package;

	struct fsPath_s			*next;
} fsPath_t;

static char		fs_gameDir[MAX_OSPATH];

static fsLink_t	*fs_fileLinks;

static fsPath_t	*fs_searchPaths;
static fsPath_t	**fs_invSearchPaths;
static size_t	fs_numInvSearchPaths;
static fsPath_t	*fs_baseSearchPath;		// Without gamedirs

/*
=============================================================================

	FILE INDEXING

=============================================================================
*/

typedef struct fsHandleIndex_s {
	char					name[MAX_QPATH];

	qBool					inUse;
	fsOpenMode_t			openMode;

	// One of these is always NULL
	FILE					*regFile;
	unzFile					*pkzFile;
} fsHandleIndex_t;

static fsHandleIndex_t	fs_fileIndices[FS_MAX_FILEINDICES];

/*
=============================================================================

	ZLIB FUNCTIONS

=============================================================================
*/

/*
================
FS_ZLibDecompress
================
*/
int FS_ZLibDecompress (byte *in, int inLen, byte *out, int outLen, int wbits)
{
	z_stream	zs;
	int			result;

	memset (&zs, 0, sizeof (zs));

	zs.next_in = in;
	zs.avail_in = 0;

	zs.next_out = out;
	zs.avail_out = outLen;

	result = inflateInit2 (&zs, wbits);
	if (result != Z_OK) {
		Sys_Error ("Error on inflateInit %d\nMessage: %s\n", result, zs.msg);
		return 0;
	}

	zs.avail_in = inLen;

	result = inflate (&zs, Z_FINISH);
	if (result != Z_STREAM_END) {
		Sys_Error ("Error on inflate %d\nMessage: %s\n", result, zs.msg);
		zs.total_out = 0;
	}

	result = inflateEnd (&zs);
	if (result != Z_OK) {
		Sys_Error ("Error on inflateEnd %d\nMessage: %s\n", result, zs.msg);
		return 0;
	}

	return zs.total_out;
}


/*
================
FS_ZLibCompressChunk
================
*/
int FS_ZLibCompressChunk (byte *in, int inLen, byte *out, int outLen, int method, int wbits)
{
	z_stream	zs;
	int			result;

	zs.next_in = in;
	zs.avail_in = inLen;
	zs.total_in = 0;

	zs.next_out = out;
	zs.avail_out = outLen;
	zs.total_out = 0;

	zs.msg = NULL;
	zs.state = NULL;
	zs.zalloc = Z_NULL;
	zs.zfree = Z_NULL;
	zs.opaque = NULL;

	zs.data_type = Z_BINARY;
	zs.adler = 0;
	zs.reserved = 0;

	result = deflateInit2 (&zs, method, Z_DEFLATED, wbits, 9, Z_DEFAULT_STRATEGY);
	if (result != Z_OK)
		return 0;

	result = deflate(&zs, Z_FINISH);
	if (result != Z_STREAM_END)
		return 0;

	result = deflateEnd(&zs);
	if (result != Z_OK)
		return 0;

	return zs.total_out;
}

/*
=============================================================================

	HELPER FUNCTIONS

=============================================================================
*/

/*
================
__FileLen
================
*/
static int __FileLen (FILE *f)
{
	int		pos;
	int		end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}


/*
============
FS_CreatePath

Creates any directories needed to store the given filename
============
*/
void FS_CreatePath (char *path)
{
	char	*ofs;

	for (ofs=path+1 ; *ofs ; ofs++) {
		if (*ofs == '/') {
			// Create the directory
			*ofs = 0;
			Sys_Mkdir (path);
			*ofs = '/';
		}
	}
}


/*
================
FS_CopyFile
================
*/
void FS_CopyFile (char *src, char *dst)
{
	FILE		*f1, *f2;
	size_t		l;
	static byte	buffer[65536];

	if (fs_developer->intVal)
		Com_Printf (0, "FS_CopyFile (%s, %s)\n", src, dst);

	f1 = fopen (src, "rb");
	if (!f1)
		return;
	f2 = fopen (dst, "wb");
	if (!f2) {
		fclose (f1);
		return;
	}

	for ( ; ; ) {
		l = fread (buffer, 1, sizeof (buffer), f1);
		if (!l)
			break;
		fwrite (buffer, 1, l, f2);
	}

	fclose (f1);
	fclose (f2);
}

/*
=============================================================================

	FILE HANDLING

=============================================================================
*/

/*
=================
FS_GetFreeHandle
=================
*/
static fileHandle_t FS_GetFreeHandle (fsHandleIndex_t **handle)
{
	fileHandle_t		i;
	fsHandleIndex_t		*hIndex;

	for (i=0, hIndex=fs_fileIndices ; i<FS_MAX_FILEINDICES ; hIndex++, i++) {
		if (hIndex->inUse)
			continue;

		hIndex->inUse = qTrue;
		*handle = hIndex;
		return i+1;
	}

	Com_Error (ERR_FATAL, "FS_GetFreeHandle: no free handles!");
	return 0;
}


/*
=================
FS_GetHandle
=================
*/
static fsHandleIndex_t *FS_GetHandle (fileHandle_t fileNum)
{
	fsHandleIndex_t *hIndex;

	if (fileNum == 0 || fileNum > FS_MAX_FILEINDICES)
		Com_Error (ERR_FATAL, "FS_GetHandle: invalid file number");

	hIndex = &fs_fileIndices[fileNum-1];
	if (!hIndex->inUse)
		Com_Error (ERR_FATAL, "FS_GetHandle: invalid handle index");

	return hIndex;
}


/*
============
FS_FileLength

Returns the TOTAL file size, not the position.
Make sure to move the position back after moving to the beginning of the file for the size lookup.
============
*/
size_t FS_FileLength (fileHandle_t fileNum)
{
	fsHandleIndex_t		*handle;

	handle = FS_GetHandle (fileNum);
	if (handle->regFile) {
		return __FileLen (handle->regFile);
	}
	else if (handle->pkzFile) {
		// FIXME
		return 0;
	}

	// Shouldn't happen...
	assert (0);
	return 0;
}


/*
============
FS_Tell

Returns the current file position.
============
*/
size_t FS_Tell (fileHandle_t fileNum)
{
	fsHandleIndex_t	*handle;

	handle = FS_GetHandle (fileNum);
	if (handle->regFile)
		return ftell (handle->regFile);
	else if (handle->pkzFile)
		return unztell (handle->pkzFile);

	// Shouldn't happen...
	assert (0);
	return 0;
}


/*
=================
FS_Read

Properly handles partial reads
=================
*/
void CDAudio_Stop (void);
size_t FS_Read (void *buffer, size_t len, fileHandle_t fileNum)
{
	fsHandleIndex_t	*handle;
	size_t			remaining;
	byte			*buf;
	qBool			tried;

	handle = FS_GetHandle (fileNum);
	if (handle->openMode != FS_MODE_READ_BINARY)
		Com_Error (ERR_FATAL, "FS_Read: %s: was not opened in read mode", handle->name);

	// Read in chunks for progress bar
	remaining = len;
	buf = (byte *)buffer;

	tried = qFalse;
	if (handle->regFile) {
		// File
		while (remaining) {
			size_t read = fread (buf, 1, remaining, handle->regFile);
			switch (read) {
			case 0:
				// We might have been trying to read from a CD
				if (!tried) {
					tried = qTrue;
#ifndef DEDICATED_ONLY
					if (!dedicated->intVal)
						CDAudio_Stop ();
#endif
				}
				else {
					if (fs_developer->intVal)
						Com_Printf (0, "FS_Read: 0 bytes read from \"%s\"", handle->name);
					return len - remaining;
				}
				break;
			}

			// Do some progress bar thing here
			remaining -= read;
			buf += read;
		}

		return len;
	}
	else if (handle->pkzFile) {
		// Zip file
		while (remaining) {
			int read = unzReadCurrentFile (handle->pkzFile, buf, (uint32) remaining);
			switch (read) {
			case 0:
				// We might have been trying to read from a CD
				if (!tried) {
					tried = qTrue;
#ifndef DEDICATED_ONLY
					if (!dedicated->intVal)
						CDAudio_Stop ();
#endif
				}
				else {
					if (fs_developer->intVal)
						Com_Printf (0, "FS_Read: 0 bytes read from \"%s\"", handle->name);
					return len - remaining;
				}
				break;

			case -1:
				Com_Error (ERR_FATAL, "FS_Read: -1 bytes read from \"%s\"", handle->name);
				break;
			}

			// Do some progress bar thing here
			remaining -= read;
			buf += read;
		}

		return len;
	}

	// Shouldn't happen...
	assert (0);
	return 0;
}


/*
=================
FS_Write
=================
*/
size_t FS_Write (void *buffer, size_t size, fileHandle_t fileNum)
{
	fsHandleIndex_t	*handle;
	size_t			remaining;
	size_t			write;
	byte			*buf;

	handle = FS_GetHandle (fileNum);
	switch (handle->openMode) {
	case FS_MODE_WRITE_BINARY:
	case FS_MODE_APPEND_BINARY:
	case FS_MODE_WRITE_TEXT:
	case FS_MODE_APPEND_TEXT:
		break;
	default:
		Com_Error (ERR_FATAL, "FS_Write: %s: was no opened in append/write mode", handle->name);
		break;
	}
	if (size == 0)
		Com_Error (ERR_FATAL, "FS_Write: size == 0");

	// Write
	remaining = size;
	buf = (byte *)buffer;

	if (handle->regFile) {
		// File
		while (remaining) {
			write = fwrite (buf, 1, remaining, handle->regFile);

			switch (write) {
			case 0:
				if (fs_developer->intVal)
					Com_Printf (PRNT_ERROR, "FS_Write: 0 bytes written to %s\n", handle->name);
				return size - remaining;
			}

			remaining -= write;
			buf += write;
		}

		return size;
	}
	else if (handle->pkzFile) {
		// Zip file
		Com_Error (ERR_FATAL, "FS_Write: can't write to zip file %s", handle->name);
	}

	// Shouldn't happen...
	assert (0);
	return 0;
}


/*
=================
FS_Seek
=================
*/
void FS_Seek (fileHandle_t fileNum, long offset, fsSeekOrigin_t seekOrigin)
{
	fsHandleIndex_t	*handle;
	unz_file_info	info;
	int				remaining = 0, r, len;
	static byte		dummy[0x8000];

	handle = FS_GetHandle (fileNum);
	if (handle->regFile) {
		// Seek through a regular file
		switch (seekOrigin) {
		case FS_SEEK_SET:
			fseek (handle->regFile, offset, SEEK_SET);
			break;

		case FS_SEEK_CUR:
			fseek (handle->regFile, offset, SEEK_CUR);
			break;

		case FS_SEEK_END:
			fseek (handle->regFile, offset, SEEK_END);
			break;

		default:
			Com_Error (ERR_FATAL, "FS_Seek: bad origin (%i)", seekOrigin);
			break;
		}
	}
	else if (handle->pkzFile) {
		// Seek through a zip
		switch (seekOrigin) {
		case FS_SEEK_SET:
			remaining = offset;
			break;

		case FS_SEEK_CUR:
			remaining = offset + unztell (handle->pkzFile);
			break;

		case FS_SEEK_END:
			unzGetCurrentFileInfo (handle->pkzFile, &info, NULL, 0, NULL, 0, NULL, 0);

			remaining = offset + info.uncompressed_size;
			break;

		default:
			Com_Error (ERR_FATAL, "FS_Seek: bad origin (%i)", seekOrigin);
		}

		// Reopen the file
		unzCloseCurrentFile (handle->pkzFile);
		unzOpenCurrentFile (handle->pkzFile);

		// Skip until the desired offset is reached
		while (remaining) {
			len = remaining;
			if (len > sizeof (dummy))
				len = sizeof (dummy);

			r = unzReadCurrentFile (handle->pkzFile, dummy, len);
			if (r <= 0)
				break;

			remaining -= r;
		}
	}
	else
		assert (0);
}


/*
==============
FS_OpenFileAppend
==============
*/
static int FS_OpenFileAppend (fsHandleIndex_t *handle, qBool binary)
{
	char	path[MAX_OSPATH];

	Q_snprintfz (path, sizeof (path), "%s/%s", fs_gameDir, handle->name);

	// Create the path if it doesn't exist
	FS_CreatePath (path);

	// Open
	if (binary)
		handle->regFile = fopen (path, "ab");
	else
		handle->regFile = fopen (path, "at");

	// Return length
	if (handle->regFile) {
		if (fs_developer->intVal)
			Com_Printf (0, "FS_OpenFileAppend: \"%s\"", path);
		return __FileLen (handle->regFile);
	}

	// Failed to open
	if (fs_developer->intVal)
		Com_Printf (0, "FS_OpenFileAppend: couldn't open \"%s\"", path);
	return -1;
}


/*
==============
FS_OpenFileWrite
==============
*/
static int FS_OpenFileWrite (fsHandleIndex_t *handle, qBool binary)
{
	char	path[MAX_OSPATH];

	Q_snprintfz (path, sizeof (path), "%s/%s", fs_gameDir, handle->name);

	// Create the path if it doesn't exist
	FS_CreatePath (path);

	// Open
	if (binary)
		handle->regFile = fopen (path, "wb");
	else
		handle->regFile = fopen (path, "wt");

	// Return length
	if (handle->regFile) {
		if (fs_developer->intVal)
			Com_Printf (0, "FS_OpenFileWrite: \"%s\"", path);
		return 0;
	}

	// Failed to open
	if (fs_developer->intVal)
		Com_Printf (0, "FS_OpenFileWrite: couldn't open \"%s\"", path);
	return -1;
}


/*
===========
FS_OpenFileRead

Finds the file in the search path.
returns filesize and an open FILE *
Used for streaming data out of either a pak file or
a seperate file.
===========
*/
qBool	fs_fileFromPak = qFalse;
static int FS_OpenFileRead (fsHandleIndex_t *handle)
{
	fsPath_t		*searchPath;
	char			netPath[MAX_OSPATH];
	mPack_t			*package;
	mPackFile_t		*searchFile;
	fsLink_t		*link;
	uint32			hashValue;

	fs_fileFromPak = qFalse;
	// Check for links first
	for (link=fs_fileLinks ; link ; link=link->next) {
		if (!strncmp (handle->name, link->from, link->fromLength)) {
			Q_snprintfz (netPath, sizeof (netPath), "%s%s", link->to, handle->name+link->fromLength);

			// Open
			handle->regFile = fopen (netPath, "rb");

			// Return length
			if (fs_developer->intVal)
				Com_Printf (0, "FS_OpenFileRead: link file: %s\n", netPath);
			if (handle->regFile)
				return __FileLen (handle->regFile);

			// Failed to load
			return -1;
		}
	}

	// Calculate hash value
	hashValue = Com_HashFileName (handle->name, FS_MAX_HASHSIZE);

	// Search through the path, one element at a time
	for (searchPath=fs_searchPaths ; searchPath ; searchPath=searchPath->next) {
		// Is the element a pak file?
		if (searchPath->package) {
			// Look through all the pak file elements
			package = searchPath->package;

			for (searchFile=package->fileHashTree[hashValue] ; searchFile ; searchFile=searchFile->hashNext) {
				if (Q_stricmp (searchFile->fileName, handle->name))
					continue;

				// Found it!
				fs_fileFromPak = qTrue;

				if (package->pak) {
					if (fs_developer->intVal)
						Com_Printf (0, "FS_OpenFileRead: pack file %s : %s\n", package->name, handle->name);

					// Open a new file on the pakfile
					handle->regFile = fopen (package->name, "rb");
					if (handle->regFile) {
						fseek (handle->regFile, searchFile->filePos, SEEK_SET);
						return searchFile->fileLen;
					}
				}
				else if (package->pkz) {
					if (fs_developer->intVal)
						Com_Printf (0, "FS_OpenFileRead: pkz file %s : %s\n", package->name, handle->name);

					handle->pkzFile = unzOpen (package->name);
					if (handle->pkzFile) {
						if (unzSetOffset (handle->pkzFile, searchFile->filePos) == UNZ_OK) {
							if (unzOpenCurrentFile (handle->pkzFile) == UNZ_OK)
								return searchFile->fileLen;
						}

						// Failed to locate/open
						unzClose (handle->pkzFile);
					}
				}

				Com_Error (ERR_FATAL, "FS_OpenFileRead: couldn't reopen \"%s\"", handle->name);
			}
		}
		else {
			// Check a file in the directory tree
			Q_snprintfz (netPath, sizeof (netPath), "%s/%s", searchPath->pathName, handle->name);

			handle->regFile = fopen (netPath, "rb");
			if (handle->regFile) {
				// Found it!
				if (fs_developer->intVal)
					Com_Printf (0, "FS_OpenFileRead: %s\n", netPath);
				return __FileLen (handle->regFile);
			}
		}
	}

	if (fs_developer->intVal)
		Com_Printf (0, "FS_OpenFileRead: can't find %s\n", handle->name);

	return -1;
}


/*
===========
FS_OpenFile
===========
*/
int FS_OpenFile (char *fileName, fileHandle_t *fileNum, fsOpenMode_t openMode)
{
	fsHandleIndex_t	*handle;
	int				fileSize = -1;

	*fileNum = FS_GetFreeHandle (&handle);

	Q_strncpyz (handle->name, fileName, sizeof (handle->name));
	handle->openMode = openMode;

	// Open under the desired mode
	switch (openMode) {
	case FS_MODE_APPEND_BINARY:
		fileSize = FS_OpenFileAppend (handle, qTrue);
		break;
	case FS_MODE_APPEND_TEXT:
		fileSize = FS_OpenFileAppend (handle, qFalse);
		break;

	case FS_MODE_READ_BINARY:
		fileSize = FS_OpenFileRead (handle);
		break;

	case FS_MODE_WRITE_BINARY:
		fileSize = FS_OpenFileWrite (handle, qTrue);
		break;
	case FS_MODE_WRITE_TEXT:
		fileSize = FS_OpenFileWrite (handle, qFalse);
		break;

	default:
		Com_Error (ERR_FATAL, "FS_OpenFile: %s: invalid mode '%i'", handle->name, openMode);
		break;
	}

	// Failed
	if (fileSize == -1) {
		memset (handle, 0, sizeof (fsHandleIndex_t));
		*fileNum = 0;
	}

	return fileSize;
}


/*
==============
FS_CloseFile
==============
*/
void FS_CloseFile (fileHandle_t fileNum)
{
	fsHandleIndex_t	*handle;

	// Get local handle
	handle = FS_GetHandle (fileNum);
	if (!handle->inUse)
		return;

	// Close file/zip
	if (handle->regFile)
		fclose (handle->regFile);
	else if (handle->pkzFile) {
		unzCloseCurrentFile (handle->pkzFile);
		unzClose (handle->pkzFile);
	}
	else
		assert (0);

	// Clear handle
	handle->inUse = qFalse;
	handle->name[0] = '\0';
	handle->pkzFile = NULL;
	handle->regFile = NULL;
}

// ==========================================================================

/*
============
FS_LoadFile

Filename are reletive to the egl search path.
A NULL buffer will just return the file length without loading.
-1 is returned if it wasn't found, 0 is returned if it's a blank file. In both cases a buffer is set to NULL.
============
*/
int FS_LoadFile (char *path, void **buffer, char *terminate)
{
	byte			*buf;
	int				fileLen;
	fileHandle_t	fileNum;
	size_t			termLen;

	// Look for it in the filesystem or pack files
	fileLen = FS_OpenFile (path, &fileNum, FS_MODE_READ_BINARY);
	if (!fileNum || fileLen <= 0) {
		if (buffer)
			*buffer = NULL;
		if (fileNum)
			FS_CloseFile (fileNum);
		if (fileLen >= 0)
			return 0;
		return -1;
	}

	// Just needed to get the length
	if (!buffer) {
		FS_CloseFile (fileNum);
		return fileLen;
	}

	// Allocate a local buffer
	// If we're terminating, pad by one byte. Mem_PoolAlloc below will zero-fill...
	if (terminate)
		termLen = strlen (terminate);
	else
		termLen = 0;
	buf = Mem_PoolAlloc (fileLen+termLen, com_fileSysPool, 0);
	*buffer = buf;

	// Copy the file data to a local buffer
	FS_Read (buf, fileLen, fileNum);
	FS_CloseFile (fileNum);

	// Terminate if desired
	if (termLen)
		strncpy ((char *)buf+fileLen, terminate, termLen);
	return (int) ((size_t) fileLen + termLen);
}


/*
=============
_FS_FreeFile
=============
*/
void _FS_FreeFile (void *buffer, const char *fileName, const int fileLine)
{
	if (buffer)
		_Mem_Free (buffer, fileName, fileLine);
}

// ==========================================================================

/*
============
FS_FileExists

Filename are reletive to the egl search path.
Just like calling FS_LoadFile with a NULL buffer.
============
*/
int FS_FileExists (char *path)
{
	int				fileLen;
	fileHandle_t	fileNum;

	// Look for it in the filesystem or pack files
	fileLen = FS_OpenFile (path, &fileNum, FS_MODE_READ_BINARY);
	if (!fileNum || fileLen <= 0)
		return -1;

	// Just needed to get the length
	FS_CloseFile (fileNum);
	return fileLen;
}

/*
=============================================================================

	PACKAGE LOADING

=============================================================================
*/

/*
=================
FS_LoadPAK

Takes an explicit (not game tree related) path to a pak file.

Loads the header and directory, adding the files at the beginning
of the list so they override previous pack files.
=================
*/
mPack_t *FS_LoadPAK (char *fileName, qBool complain)
{
	dPackHeader_t		header;
	mPackFile_t			*outPackFile;
	mPack_t				*outPack;
	FILE				*handle;
	static dPackFile_t	info[PAK_MAX_FILES];
	size_t				i, numFiles;
	uint32				hashValue;

	// Open
	handle = fopen (fileName, "rb");
	if (!handle) {
		if (complain)
			Com_Printf (PRNT_ERROR, "FS_LoadPAK: couldn't open \"%s\"\n", fileName);
		return NULL;
	}

	// Read header
	fread (&header, 1, sizeof (header), handle);
	if (LittleLong (header.ident) != PAK_HEADER) {
		fclose (handle);
		Com_Error (ERR_FATAL, "FS_LoadPAK: \"%s\" is not a packfile", fileName);
	}

	header.dirOfs = LittleLong (header.dirOfs);
	header.dirLen = LittleLong (header.dirLen);

	// Sanity checks
	numFiles = header.dirLen / sizeof (dPackFile_t);
	if (numFiles > PAK_MAX_FILES) {
		fclose (handle);
		Com_Error (ERR_FATAL, "FS_LoadPAK: \"%s\" has too many files (%i > %i)", fileName, numFiles, PAK_MAX_FILES);
	}
	if (numFiles == 0) {
		fclose (handle);
		Com_Error (ERR_FATAL, "FS_LoadPAK: \"%s\" is empty", fileName);
	}

	// Read past header
	fseek (handle, header.dirOfs, SEEK_SET);
	fread (info, 1, header.dirLen, handle);

	// Create pak
	outPack = Mem_PoolAlloc (sizeof (mPack_t), com_fileSysPool, 0);
	outPackFile = Mem_PoolAlloc (sizeof (mPackFile_t) * numFiles, com_fileSysPool, 0);

	Q_strncpyz (outPack->name, fileName, sizeof (outPack->name));
	outPack->pak = handle;
	outPack->numFiles = numFiles;
	outPack->files = outPackFile;

	// Parse the directory
	for (i=0 ; i<numFiles ; i++) {
		Q_strncpyz (outPackFile->fileName, info[i].name, sizeof (outPackFile->fileName));
		outPackFile->filePos = LittleLong (info[i].filePos);
		outPackFile->fileLen = LittleLong (info[i].fileLen);

		// Link it into the hash tree
		hashValue = Com_HashFileName (outPackFile->fileName, FS_MAX_HASHSIZE);

		outPackFile->hashNext = outPack->fileHashTree[hashValue];
		outPack->fileHashTree[hashValue] = outPackFile;

		// Next
		outPackFile++;
	}

	Com_Printf (0, "FS_LoadPAK: loaded \"%s\"\n", fileName);
	return outPack;
}


/*
=================
FS_LoadPKZ
=================
*/
mPack_t *FS_LoadPKZ (char *fileName, qBool complain)
{
	mPackFile_t		*outPkzFile;
	mPack_t			*outPkz;
	unzFile			*handle;
	unz_global_info	global;
	unz_file_info	info;
	char			name[MAX_QPATH];
	int				status;
	int				numFiles;
	uint32			hashValue;

	// Open
	handle = unzOpen (fileName);
	if (!handle) {
		if (complain)
			Com_Printf (PRNT_ERROR, "FS_LoadPKZ: couldn't open \"%s\"\n", fileName);
		return NULL;
	}

	// Get global info
	if (unzGetGlobalInfo (handle, &global) != UNZ_OK) {
		unzClose (handle);
		Com_Error (ERR_FATAL, "FS_LoadPKZ: \"%s\" is not a packfile", fileName);
	}

	// Sanity checks
	numFiles = global.number_entry;
	if (numFiles > PKZ_MAX_FILES) {
		unzClose (handle);
		Com_Error (ERR_FATAL, "FS_LoadPKZ: \"%s\" has too many files (%i > %i)", fileName, numFiles, PKZ_MAX_FILES);
	}
	if (numFiles <= 0) {
		unzClose (handle);
		Com_Error (ERR_FATAL, "FS_LoadPKZ: \"%s\" is empty", fileName);
	}

	// Create pak
	outPkz = Mem_PoolAlloc (sizeof (mPack_t), com_fileSysPool, 0);
	outPkzFile = Mem_PoolAlloc (sizeof (mPackFile_t) * numFiles, com_fileSysPool, 0);

	Q_strncpyz (outPkz->name, fileName, sizeof (outPkz->name));
	outPkz->pkz = handle;
	outPkz->numFiles = numFiles;
	outPkz->files = outPkzFile;

	status = unzGoToFirstFile (handle);

	while (status == UNZ_OK) {
		unzGetCurrentFileInfo (handle, &info, name, MAX_QPATH, NULL, 0, NULL, 0);

		Q_strncpyz (outPkzFile->fileName, name, sizeof (outPkzFile->fileName));
		outPkzFile->filePos = unzGetOffset (handle);
		outPkzFile->fileLen = info.uncompressed_size;

		// Link it into the hash tree
		hashValue = Com_HashFileName (outPkzFile->fileName, FS_MAX_HASHSIZE);

		outPkzFile->hashNext = outPkz->fileHashTree[hashValue];
		outPkz->fileHashTree[hashValue] = outPkzFile;

		// Next
		outPkzFile++;

		status = unzGoToNextFile (handle);
	}

	Com_Printf (0, "FS_LoadPKZ: loaded \"%s\"\n", fileName);
	return outPkz;
}


/*
================
FS_AddGameDirectory

Sets fs_gameDir, adds the directory to the head of the path, and loads *.pak/*.pkz
================
*/
static void FS_AddGameDirectory (char *dir, char *gamePath)
{
	char		searchName[MAX_OSPATH];
	char		*packFiles[FS_MAX_PAKS];
	int			numPacks, i;
	fsPath_t	*search;
	mPack_t		*pak;
	mPack_t		*pkz;

	if (fs_developer->intVal)
		Com_Printf (0, "FS_AddGameDirectory: adding \"%s\"\n", dir);

	// Set as game directory
	Q_strncpyz (fs_gameDir, dir, sizeof (fs_gameDir));

	// Add the directory to the search path
	search = Mem_PoolAlloc (sizeof (fsPath_t), com_fileSysPool, 0);
	Q_strncpyz (search->pathName, dir, sizeof (search->pathName));
	Q_strncpyz (search->gamePath, gamePath, sizeof (search->gamePath));
	search->next = fs_searchPaths;
	fs_searchPaths = search;

	// Add any pak files in the format pak0.pak pak1.pak, ...
	for (i=0 ; i<10 ; i++) {
		Q_snprintfz (searchName, sizeof (searchName), "%s/pak%i.pak", dir, i);
		pak = FS_LoadPAK (searchName, qFalse);
		if (!pak)
			continue;
		search = Mem_PoolAlloc (sizeof (fsPath_t), com_fileSysPool, 0);
		search->package = pak;
		search->next = fs_searchPaths;
		fs_searchPaths = search;
	}

	// Add the rest of the *.pak files
	if (!fs_defaultPaks->intVal) {
		numPacks = Sys_FindFiles (dir, "*/*.pak", packFiles, FS_MAX_PAKS, 0, qFalse, qTrue, qFalse);

		for (i=0 ; i<numPacks ; i++) {
			if (strstr (packFiles[i], "/pak0.pak") || strstr (packFiles[i], "/pak1.pak")
			|| strstr (packFiles[i], "/pak2.pak") || strstr (packFiles[i], "/pak3.pak")
			|| strstr (packFiles[i], "/pak4.pak") || strstr (packFiles[i], "/pak5.pak")
			|| strstr (packFiles[i], "/pak6.pak") || strstr (packFiles[i], "/pak7.pak")
			|| strstr (packFiles[i], "/pak8.pak") || strstr (packFiles[i], "/pak9.pak"))
				continue; // FIXME :|

			pak = FS_LoadPAK (packFiles[i], qTrue);
			if (!pak)
				continue;
			search = Mem_PoolAlloc (sizeof (fsPath_t), com_fileSysPool, 0);
			Q_strncpyz (search->pathName, dir, sizeof (search->pathName));
			Q_strncpyz (search->gamePath, gamePath, sizeof (search->gamePath));
			search->package = pak;
			search->next = fs_searchPaths;
			fs_searchPaths = search;
		}

		FS_FreeFileList (packFiles, numPacks);
	}

	// Load *.pkz files
	numPacks = Sys_FindFiles (dir, "*/*.pkz", packFiles, FS_MAX_PAKS, 0, qFalse, qTrue, qFalse);

	for (i=0 ; i<numPacks ; i++) {
		pkz = FS_LoadPKZ (packFiles[i], qTrue);
		if (!pkz)
			continue;
		search = Mem_PoolAlloc (sizeof (fsPath_t), com_fileSysPool, 0);
		Q_strncpyz (search->pathName, dir, sizeof (search->pathName));
		Q_strncpyz (search->gamePath, gamePath, sizeof (search->gamePath));
		search->package = pkz;
		search->next = fs_searchPaths;
		fs_searchPaths = search;
	}

	FS_FreeFileList (packFiles, numPacks);
}

/*
=============================================================================

	GAME PATH

=============================================================================
*/

/*
============
FS_Gamedir
============
*/
char *FS_Gamedir (void)
{
	if (*fs_gameDir)
		return fs_gameDir;
	else
		return BASE_MODDIRNAME;
}


/*
================
FS_SetGamedir

Sets the gamedir and path to a different directory.
================
*/
void FS_SetGamedir (char *dir, qBool firstTime)
{
	fsPath_t	*next;
	mPack_t		*package;
	uint32		initTime;
	size_t		i;

	// Make sure it's not a path
	if (strstr (dir, "..") || strchr (dir, '/') || strchr (dir, '\\') || strchr (dir, ':')) {
		Com_Printf (PRNT_WARNING, "FS_SetGamedir: Gamedir should be a single directory name, not a path\n");
		return;
	}

	// Free old inverted paths
	if (fs_invSearchPaths)
		Mem_Free (fs_invSearchPaths);

	// Free up any current game dir info
	for ( ; fs_searchPaths != fs_baseSearchPath ; fs_searchPaths=next) {
		next = fs_searchPaths->next;

		if (fs_searchPaths->package) {
			package = fs_searchPaths->package;

			if (package->pak)
				fclose (package->pak);
			else if (package->pkz)
				unzClose (package->pkz);

			Mem_Free (package->files);
			Mem_Free (package);
		}

		Mem_Free (fs_searchPaths);
	}

	// Load packages
	initTime = Sys_UMilliseconds ();
	Com_Printf (0, "\n------------- Changing Game ------------\n");

	Q_snprintfz (fs_gameDir, sizeof (fs_gameDir), "%s/%s", fs_basedir->string, dir);

	if (!strcmp (dir, BASE_MODDIRNAME) || *dir == 0) {
		Cvar_VariableSet (fs_gamedircvar, "", qTrue);
		Cvar_VariableSet (fs_game, "", qTrue);
	}
	else {
		Cvar_VariableSet (fs_gamedircvar, dir, qTrue);
		if (fs_cddir->string[0])
			FS_AddGameDirectory (Q_VarArgs ("%s/%s", fs_cddir->string, dir), dir);

		FS_AddGameDirectory (Q_VarArgs ("%s/%s", fs_basedir->string, dir), dir);
	}

	// Store a copy of the search paths inverted for FS_FindFiles
	for (fs_numInvSearchPaths=0, next=fs_searchPaths ; next ; next=next->next, fs_numInvSearchPaths++);
	fs_invSearchPaths = Mem_PoolAlloc (sizeof (fsPath_t) * fs_numInvSearchPaths, com_fileSysPool, 0);
	for (i=fs_numInvSearchPaths-1, next=fs_searchPaths ; ; next=next->next)
	{
		fs_invSearchPaths[i] = next;

		if (i == 0)
			break;

		i--;
	}

	if (!firstTime) {
		Com_Printf (0, "----------------------------------------\n");
		Com_Printf (0, "init time: %ums\n", Sys_UMilliseconds()-initTime);
		Com_Printf (0, "----------------------------------------\n");

#ifndef DEDICATED_ONLY
		// Forced to reload to flush old data
		if (!dedicated->intVal) {
			Cbuf_AddText ("exec default.cfg\n");
			Cbuf_AddText ("exec config.cfg\n");
			Cbuf_AddText ("exec eglcfg.cfg\n");
			FS_ExecAutoexec ();
			Cbuf_Execute ();
			Cbuf_AddText ("vid_restart\nsnd_restart\n");
			Cbuf_Execute ();
		}
#endif
	}
}


/*
=============
FS_ExecAutoexec
=============
*/
void FS_ExecAutoexec (void)
{
	char	*dir;
	char	name[MAX_QPATH];

	dir = Cvar_GetStringValue ("gamedir");
	if (*dir)
		Q_snprintfz (name, sizeof (name), "%s/%s/autoexec.cfg", fs_basedir->string, dir);
	else
		Q_snprintfz (name, sizeof (name), "%s/%s/autoexec.cfg", fs_basedir->string, BASE_MODDIRNAME);

	if (Sys_FindFirst (name, 0, (SFF_SUBDIR|SFF_HIDDEN|SFF_SYSTEM)))
		Cbuf_AddText ("exec autoexec.cfg\n");

	Sys_FindClose ();
}

/*
=============================================================================

	FILE SEARCHING

=============================================================================
*/

/*
================
FS_FindFiles
================
*/
size_t FS_FindFiles (char *path, char *filter, char *extension, char **fileList, size_t maxFiles, qBool addGameDir, qBool recurse)
{
	fsPath_t	*search;
	mPackFile_t	*packFile;
	mPack_t		*pack;
	size_t		fileCount;
	char		*name;
	char		dir[MAX_OSPATH];
	char		ext[MAX_QEXT];
	static char	*dirFiles[FS_MAX_FINDFILES];
	size_t		dirCount, i, j, k;

	// Sanity check
	if (maxFiles > FS_MAX_FINDFILES) {
		Com_Printf (PRNT_ERROR, "FS_FindFiles: maxFiles(%i) > %i, forcing %i...\n", maxFiles, FS_MAX_FINDFILES, FS_MAX_FINDFILES);
		maxFiles = FS_MAX_FINDFILES;
	}

	// Search through the path, one element at a time
	fileCount = 0;
	for (k=0 ; k<fs_numInvSearchPaths ; k++) {
		search = fs_invSearchPaths[k];

		if (search->package) {
			// Pack file
			pack = search->package;
			for (i=0, packFile=pack->files ; i<pack->numFiles ; i++, packFile++) {
				// Match path
				if (!recurse) {
					Com_FilePath (packFile->fileName, dir, sizeof (dir));
					if (Q_stricmp (path, dir))
						continue;
				}
				// Check path
				else if (!strstr (packFile->fileName, path))
					continue;

				// Match extension
				if (extension) {
					Com_FileExtension (packFile->fileName, ext, sizeof (ext));

					// Filter or compare
					if (strchr (extension, '*')) {
						if (!Q_WildcardMatch (extension, ext, 1))
							continue;
					}
					else {
						if (Q_stricmp (extension, ext))
							continue;
					}
				}

				// Match filter
				if (filter) {
					if (!Q_WildcardMatch (filter, packFile->fileName, 1))
						continue;
				}

				// Found something
				name = packFile->fileName;
				if (fileCount < maxFiles) {
					// Ignore duplicates
					for (j=0 ; j<fileCount ; j++) {
						if (!Q_stricmp (fileList[j], name))
							break;
					}

					if (j == fileCount) {
						if (addGameDir)
							fileList[fileCount++] = Mem_PoolStrDup (Q_VarArgs ("%s/%s", search->gamePath, name), com_fileSysPool, 0);
						else
							fileList[fileCount++] = Mem_PoolStrDup (name, com_fileSysPool, 0);
					}
				}
			}
		}
		else {
			// Directory tree
			Q_snprintfz (dir, sizeof (dir), "%s/%s", search->pathName, path);

			if (extension) {
				Q_snprintfz (ext, sizeof (ext), "*.%s", extension);
				dirCount = Sys_FindFiles (dir, ext, dirFiles, FS_MAX_FINDFILES, 0, recurse, qTrue, qFalse);
			}
			else {
				dirCount = Sys_FindFiles (dir, "*", dirFiles, FS_MAX_FINDFILES, 0, recurse, qTrue, qTrue);
			}

			for (i=0 ; i<dirCount ; i++) {
				// Match filter
				if (filter) {
					if (!Q_WildcardMatch (filter, dirFiles[i]+strlen(search->pathName)+1, 1)) {
						Mem_Free (dirFiles[i]);
						continue;
					}
				}

				// Found something
				name = dirFiles[i] + strlen (search->pathName) + 1;

				if (fileCount < maxFiles) {
					// Ignore duplicates
					for (j=0 ; j<fileCount ; j++) {
						if (!Q_stricmp (fileList[j], name))
							break;
					}

					if (j == fileCount) {
						if (addGameDir)
							fileList[fileCount++] = Mem_PoolStrDup (Q_VarArgs ("%s/%s", search->gamePath, name), com_fileSysPool, 0);
						else
							fileList[fileCount++] = Mem_PoolStrDup (name, com_fileSysPool, 0);
					}
				}

				Mem_Free (dirFiles[i]);
			}
		}
	}

	return fileCount;
}


/*
=============
_FS_FreeFileList
=============
*/
void _FS_FreeFileList (char **list, size_t num, const char *fileName, const int fileLine)
{
	size_t		i;

	for (i=0 ; i<num ; i++) {
		if (!list[i])
			continue;

		_Mem_Free (list[i], fileName, fileLine);
		list[i] = NULL;
	}
}


/*
================
FS_NextPath

Allows enumerating all of the directories in the search path
================
*/
char *FS_NextPath (char *prevPath)
{
	fsPath_t	*s;
	char		*prev;

	if (!prevPath)
		return fs_gameDir;

	prev = fs_gameDir;
	for (s=fs_searchPaths ; s ; s=s->next) {
		if (s->package)
			continue;
		if (prevPath == prev)
			return s->pathName;
		prev = s->pathName;
	}

	return NULL;
}

/*
=============================================================================

	CONSOLE FUNCTIONS

=============================================================================
*/

/*
================
FS_Link_f

Creates a fsLink_t
================
*/
static void FS_Link_f (void)
{
	fsLink_t	*l, **prev;

	if (Cmd_Argc () != 3) {
		Com_Printf (0, "USAGE: link <from> <to>\n");
		return;
	}

	// See if the link already exists
	prev = &fs_fileLinks;
	for (l=fs_fileLinks ; l ; l=l->next) {
		if (!strcmp (l->from, Cmd_Argv (1))) {
			Mem_Free (l->to);
			if (!strlen (Cmd_Argv (2))) {
				// Delete it
				*prev = l->next;
				Mem_Free (l->from);
				Mem_Free (l);
				return;
			}
			l->to = Mem_PoolStrDup (Cmd_Argv (2), com_fileSysPool, 0);
			return;
		}
		prev = &l->next;
	}

	// Create a new link
	l = Mem_PoolAlloc (sizeof (*l), com_fileSysPool, 0);
	l->from = Mem_PoolStrDup (Cmd_Argv (1), com_fileSysPool, 0);
	l->fromLength = strlen (l->from);
	l->to = Mem_PoolStrDup (Cmd_Argv (2), com_fileSysPool, 0);
	l->next = fs_fileLinks;
	fs_fileLinks = l;
}


/*
============
FS_ListHandles_f
============
*/
static void FS_ListHandles_f (void)
{
	fsHandleIndex_t	*index;
	int				i;

	Com_Printf (0, " #  mode name\n");
	Com_Printf (0, "--- ---- ----------------\n");
	for (i=0, index=&fs_fileIndices[0] ; i<FS_MAX_FILEINDICES ; index++, i++) {
		if (!index->inUse)
			continue;

		Com_Printf (0, "%3i ", i+1);
		switch (index->openMode) {
		case FS_MODE_READ_BINARY:	Com_Printf (0, "RB ");	break;
		case FS_MODE_WRITE_BINARY:	Com_Printf (0, "WB ");	break;
		case FS_MODE_APPEND_BINARY:	Com_Printf (0, "AB ");	break;
		case FS_MODE_WRITE_TEXT:	Com_Printf (0, "WT ");	break;
		case FS_MODE_APPEND_TEXT:	Com_Printf (0, "AT ");	break;
		}
		Com_Printf (0, "%s\n", index->name);
	}
}


/*
============
FS_Path_f
============
*/
static void FS_Path_f (void)
{
	fsPath_t	*s;
	fsLink_t	*l;

	Com_Printf (0, "Current search path:\n");
	for (s=fs_searchPaths ; s ; s=s->next) {
		if (s == fs_baseSearchPath)
			Com_Printf (0, "----------\n");

		if (s->package)
			Com_Printf (0, "%s (%i files)\n", s->package->name, s->package->numFiles);
		else
			Com_Printf (0, "%s\n", s->pathName);
	}

	Com_Printf (0, "\nLinks:\n");
	for (l=fs_fileLinks ; l ; l=l->next)
		Com_Printf (0, "%s : %s\n", l->from, l->to);
}

/*
=============================================================================

	INIT / SHUTDOWN

=============================================================================
*/

/*
================
FS_Init
================
*/
void FS_Init (void)
{
	uint32		initTime;
	fsPath_t	*next;
	size_t		i;

	initTime = Sys_UMilliseconds ();
	Com_Printf (0, "\n------- Filesystem Initialization ------\n");

	// Register commands/cvars
	Cmd_AddCommand ("link",			FS_Link_f,			"");
	Cmd_AddCommand ("listHandles",	FS_ListHandles_f,	"Lists active files");
	Cmd_AddCommand ("path",			FS_Path_f,			"");

	fs_basedir		= Cvar_Register ("basedir",			".",	CVAR_READONLY);
	fs_cddir		= Cvar_Register ("cddir",			"",		CVAR_READONLY);
	fs_game			= Cvar_Register ("game",			"",		CVAR_LATCH_SERVER|CVAR_SERVERINFO|CVAR_RESET_GAMEDIR);
	fs_gamedircvar	= Cvar_Register ("gamedir",			"",		CVAR_SERVERINFO|CVAR_READONLY);
	fs_defaultPaks	= Cvar_Register ("fs_defaultPaks",	"1",	CVAR_ARCHIVE);

	// Load pak files
	if (fs_cddir->string[0])
		FS_AddGameDirectory (Q_VarArgs ("%s/"BASE_MODDIRNAME, fs_cddir->string), BASE_MODDIRNAME);

	FS_AddGameDirectory (Q_VarArgs ("%s/"BASE_MODDIRNAME, fs_basedir->string), BASE_MODDIRNAME);

	// Any set gamedirs will be freed up to here
	fs_baseSearchPath = fs_searchPaths;

	// Load the game directory
	if (fs_game->string[0]) {
		FS_SetGamedir (fs_game->string, qTrue);
	}
	else {
		// Store a copy of the search paths inverted for FS_FindFiles
		for (fs_numInvSearchPaths=0, next=fs_searchPaths ; next ; next=next->next, fs_numInvSearchPaths++);
		fs_invSearchPaths = Mem_PoolAlloc (sizeof (fsPath_t) * fs_numInvSearchPaths, com_fileSysPool, 0);
		for (i=fs_numInvSearchPaths-1, next=fs_searchPaths ; ; next=next->next)
		{
			fs_invSearchPaths[i] = next;

			if (i == 0)
				break;

			i--;
		}
	}

	Com_Printf (0, "----------------------------------------\n");

	// Check memory integrity
	Mem_CheckPoolIntegrity (com_fileSysPool);

	Com_Printf (0, "init time: %ums\n", Sys_UMilliseconds()-initTime);
	Com_Printf (0, "----------------------------------------\n");
}
