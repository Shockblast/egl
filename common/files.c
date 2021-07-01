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

#define USE_BYTESWAP
#include "common.h"
#include "../include/minizip/unzip.h"

#define FS_MAX_HASHSIZE		1024
#define FS_MAX_FILEINDICES	1024
#define FS_MAX_SEARCHFILES	65536

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
	int						numFiles;
	mPackFile_t				*files;

	struct mPackFile_s		*fileHashTree[FS_MAX_HASHSIZE];
} mPack_t;

/*
=============================================================================

	FILESYSTEM FUNCTIONALITY

=============================================================================
*/

typedef struct fs_fileLink_s {
	struct fs_fileLink_s	*next;
	char					*from;
	int						fromLength;
	char					*to;
} fs_fileLink_t;

typedef struct fs_searchPath_s {
	char					pathName[MAX_OSPATH];
	char					gamePath[MAX_OSPATH];
	mPack_t					*package;

	struct fs_searchPath_s	*next;
} fs_searchPath_t;

static char				fs_gameDir[MAX_OSPATH];

static fs_fileLink_t	*fs_fileLinks;

static fs_searchPath_t	*fs_searchPaths;
static fs_searchPath_t	*fs_baseSearchPath;	// Without gamedirs

/*
=============================================================================

	FILE INDEXING

=============================================================================
*/

typedef struct fs_handleIndex_s {
	char		name[MAX_QPATH];

	qBool		inUse;

	// One of these is always NULL
	FILE		*file;
	unzFile		*zip;
} fileIndex_t;

static fileIndex_t	fs_fileIndices[FS_MAX_FILEINDICES];

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
int FS_ZLibDecompress (byte *in, int inlen, byte *out, int outlen, int wbits)
{
	z_stream	zs;
	int			result;

	memset (&zs, 0, sizeof (zs));

	zs.next_in = in;
	zs.avail_in = 0;

	zs.next_out = out;
	zs.avail_out = outlen;

	result = inflateInit2 (&zs, wbits);
	if (result != Z_OK) {
		Sys_Error ("Error on inflateInit %d\nMessage: %s\n", result, zs.msg);
		return 0;
	}

	zs.avail_in = inlen;

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
int FS_ZLibCompressChunk (byte *in, int len_in, byte *out, int len_out, int method, int wbits)
{
	z_stream	zs;
	int			result;

	zs.next_in = in;
	zs.avail_in = len_in;
	zs.total_in = 0;

	zs.next_out = out;
	zs.avail_out = len_out;
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
	FILE	*f1, *f2;
	int		l;
	byte	buffer[65536];

	if (fs_developer->integer)
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
static int FS_GetFreeHandle (fileIndex_t **handle)
{
	int		i;

	for (i=0 ; i<FS_MAX_FILEINDICES ; i++) {
		if (fs_fileIndices[i].inUse)
			continue;

		fs_fileIndices[i].inUse = qTrue;
		*handle = &fs_fileIndices[i];
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
static fileIndex_t *FS_GetHandle (fileHandle_t handle)
{
	if (handle < 0 || handle > FS_MAX_FILEINDICES)
		Com_Error (ERR_FATAL, "FS_GetHandle: invalid handle index");

	return &fs_fileIndices[handle-1];
}


/*
============
FS_FileLength
============
*/
int FS_FileLength (fileHandle_t fileNum)
{
	fileIndex_t		*handle;
	int				pos;
	int				end;

	handle = FS_GetHandle (fileNum);
	if (handle->file) {
		pos = ftell (handle->file);
		fseek (handle->file, 0, SEEK_END);
		end = ftell (handle->file);
		fseek (handle->file, pos, SEEK_SET);

		return end;
	}
	else if (handle->zip) {
		// FIXME: self explanatory
		Com_Error (ERR_FATAL, "FS_FileLength: zip file LOL UNSUPPORTED FUCK");
		return -1;
	}

	return -1;
}


/*
=================
FS_Read

Properly handles partial reads
=================
*/
void CDAudio_Stop (void);
int FS_Read (void *buffer, int len, fileHandle_t fileNum)
{
	fileIndex_t	*handle;
	int			remaining;
	int			read;
	byte		*buf;
	qBool		tried;

	handle = FS_GetHandle (fileNum);

	// Read in chunks for progress bar
	remaining = len;
	buf = (byte *)buffer;

	tried = qFalse;
	if (handle->file) {
		// File
		while (remaining) {
			read = fread (buf, 1, remaining, handle->file);
			switch (read) {
			case 0:
				// We might have been trying to read from a CD
				if (!tried) {
					tried = qTrue;
#ifndef DEDICATED_ONLY
					if (!dedicated->integer)
						CDAudio_Stop ();
#endif
				}
				else {
					if (fs_developer->integer)
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
	else if (handle->zip) {
		// Zip file
		while (remaining) {
			read = unzReadCurrentFile (handle->zip, buf, remaining);
			switch (read) {
			case 0:
				// We might have been trying to read from a CD
				if (!tried) {
					tried = qTrue;
#ifndef DEDICATED_ONLY
					if (!dedicated->integer)
						CDAudio_Stop ();
#endif
				}
				else {
					if (fs_developer->integer)
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

	// Unknown...
	return 0;
}


/*
=================
FS_Write
=================
*/
int FS_Write (void *buffer, int size, fileHandle_t fileNum)
{
	fileIndex_t	*handle;
	int			remaining;
	int			write;
	byte		*buf;

	handle = FS_GetHandle (fileNum);

	if (size < 0)
		Com_Error (ERR_FATAL, "FS_Write: size < 0");

	// Write
	remaining = size;
	buf = (byte *)buffer;

	if (handle->file) {
		// File
		while (remaining) {
			write = fwrite (buf, 1, remaining, handle->file);

			switch (write) {
			case 0:
				if (fs_developer->integer)
					Com_Printf (PRNT_ERROR, "FS_Write: 0 bytes written to %s\n", handle->name);
				return size - remaining;

			case -1:
				Com_Error (ERR_FATAL, "FS_Write: -1 bytes written to %s", handle->name);
				break;
			}

			remaining -= write;
			buf += write;
		}

		return size;
	}
	else if (handle->zip) {
		// Zip file
		Com_Error (ERR_FATAL, "FS_Write: can't write to zip file %s", handle->name);
	}

	// Unknown...
	return 0;
}


/*
=================
FS_Seek
=================
*/
void FS_Seek (fileHandle_t fileNum, int offset, int seekOrigin)
{
	fileIndex_t		*handle;
	unz_file_info	info;
	int				remaining, r, len;
	byte			dummy[0x8000];

	handle = FS_GetHandle (fileNum);

	if (handle->file) {
		// Seek through a regular file
		switch (seekOrigin) {
		case FS_SEEK_SET:
			fseek (handle->file, offset, SEEK_SET);
			break;

		case FS_SEEK_CUR:
			fseek (handle->file, offset, SEEK_CUR);
			break;

		case FS_SEEK_END:
			fseek (handle->file, offset, SEEK_END);
			break;

		default:
			Com_Error (ERR_FATAL, "FS_Seek: bad origin (%i)", seekOrigin);
			break;
		}
	}
	else if (handle->zip) {
		// Seek through a zip
		switch (seekOrigin) {
		case FS_SEEK_SET:
			remaining = offset;
			break;

		case FS_SEEK_CUR:
			remaining = offset + unztell (handle->zip);
			break;

		case FS_SEEK_END:
			unzGetCurrentFileInfo (handle->zip, &info, NULL, 0, NULL, 0, NULL, 0);

			remaining = offset + info.uncompressed_size;
			break;

		default:
			Com_Error (ERR_FATAL, "FS_Seek: bad origin (%i)", seekOrigin);
		}

		// Reopen the file
		unzCloseCurrentFile (handle->zip);
		unzOpenCurrentFile (handle->zip);

		// Skip until the desired offset is reached
		while (remaining) {
			len = remaining;
			if (len > sizeof (dummy))
				len = sizeof (dummy);

			r = unzReadCurrentFile (handle->zip, dummy, len);
			if (r <= 0)
				break;

			remaining -= r;
		}
	}
}


/*
==============
FS_OpenFileAppend
==============
*/
static int FS_OpenFileAppend (fileIndex_t *handle, qBool binary)
{
	char	path[MAX_OSPATH];

	Q_snprintfz (path, sizeof (path), "%s/%s", fs_gameDir, handle->name);

	// Create the path if it doesn't exist
	FS_CreatePath (path);

	// Open
	if (binary)
		handle->file = fopen (path, "ab");
	else
		handle->file = fopen (path, "at");

	// Return length
	if (handle->file) {
		if (fs_developer->integer)
			Com_Printf (0, "FS_OpenFileAppend: \"%s\"", path);
		return __FileLen (handle->file);
	}

	// Failed to open
	if (fs_developer->integer)
		Com_Printf (0, "FS_OpenFileAppend: couldn't open \"%s\"", path);
	return -1;
}


/*
==============
FS_OpenFileWrite
==============
*/
static int FS_OpenFileWrite (fileIndex_t *handle, qBool binary)
{
	char	path[MAX_OSPATH];

	Q_snprintfz (path, sizeof (path), "%s/%s", fs_gameDir, handle->name);

	// Create the path if it doesn't exist
	FS_CreatePath (path);

	// Open
	if (binary)
		handle->file = fopen (path, "wb");
	else
		handle->file = fopen (path, "wt");

	// Return length
	if (handle->file) {
		if (fs_developer->integer)
			Com_Printf (0, "FS_OpenFileWrite: \"%s\"", path);
		return 0;
	}

	// Failed to open
	if (fs_developer->integer)
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
static int FS_OpenFileRead (fileIndex_t *handle)
{
	fs_searchPath_t		*searchPath;
	char				netPath[MAX_OSPATH];
	mPack_t				*package;
	mPackFile_t			*searchFile;
	fs_fileLink_t		*link;
	uint32				hashValue;

	fs_fileFromPak = qFalse;
	// Check for links first
	for (link=fs_fileLinks ; link ; link=link->next) {
		if (!strncmp (handle->name, link->from, link->fromLength)) {
			Q_snprintfz (netPath, sizeof (netPath), "%s%s", link->to, handle->name+link->fromLength);

			// Open
			handle->file = fopen (netPath, "rb");

			// Return length
			if (fs_developer->integer)
				Com_Printf (0, "FS_OpenFileRead: link file: %s\n", netPath);
			if (handle->file)
				return __FileLen (handle->file);

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
					if (fs_developer->integer)
						Com_Printf (0, "FS_OpenFileRead: pack file %s : %s\n", package->name, handle->name);

					// Open a new file on the pakfile
					handle->file = fopen (package->name, "rb");
					if (handle->file) {
						fseek (handle->file, searchFile->filePos, SEEK_SET);
						return searchFile->fileLen;
					}
				}
				else if (package->pkz) {
					if (fs_developer->integer)
						Com_Printf (0, "FS_OpenFileRead: pkz file %s : %s\n", package->name, handle->name);

					handle->zip = unzOpen (package->name);
					if (handle->zip) {
						if (unzSetOffset (handle->zip, searchFile->filePos) == UNZ_OK) {
							if (unzOpenCurrentFile (handle->zip) == UNZ_OK)
								return searchFile->fileLen;
						}

						// Failed to locate/open
						unzClose (handle->zip);
					}
				}

				Com_Error (ERR_FATAL, "FS_OpenFileRead: couldn't reopen \"%s\"", handle->name);
			}
		}
		else {
			// Check a file in the directory tree
			Q_snprintfz (netPath, sizeof (netPath), "%s/%s", searchPath->pathName, handle->name);

			handle->file = fopen (netPath, "rb");
			if (handle->file) {
				// Found it!
				if (fs_developer->integer)
					Com_Printf (0, "FS_OpenFileRead: %s\n", netPath);
				return __FileLen (handle->file);
			}
		}
		
	}

	if (fs_developer->integer)
		Com_Printf (0, "FS_OpenFileRead: can't find %s\n", handle->name);

	return -1;
}


/*
===========
FS_OpenFile
===========
*/
int FS_OpenFile (char *fileName, fileHandle_t *fileNum, int mode)
{
	fileIndex_t	*handle;
	int			fileSize;

	*fileNum = FS_GetFreeHandle (&handle);

	Q_strncpyz (handle->name, fileName, sizeof (handle->name));

	// Open under the desired mode
	switch (mode) {
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
		Com_Error (ERR_FATAL, "FS_OpenFile: invalid mode '%i'", mode);
		break;
	}

	// Failed
	if (fileSize == -1) {
		memset (handle, 0, sizeof (fileIndex_t));
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
	fileIndex_t	*handle;

	// Get local handle
	handle = FS_GetHandle (fileNum);
	if (!handle->inUse)
		return;

	// Close file/zip
	if (handle->file)
		fclose (handle->file);
	else if (handle->zip)
		unzClose (handle->zip);

	// Clear handle
	memset (handle, 0, sizeof (fileIndex_t));
}

// ==========================================================================

/*
============
FS_LoadFile

Filename are reletive to the egl search path a null buffer
will just return the file length without loading
============
*/
int FS_LoadFile (char *path, void **buffer)
{
	byte			*buf;
	int				len;
	fileHandle_t	fileNum;

	buf = NULL;	// quiet compiler warning

	// Look for it in the filesystem or pack files
	len = FS_OpenFile (path, &fileNum, FS_MODE_READ_BINARY);
	if (!fileNum || len <= 0) {
		if (buffer)
			*buffer = NULL;
		return -1;
	}

	// Just needed to get the length
	if (!buffer) {
		FS_CloseFile (fileNum);
		return len;
	}

	buf = Mem_PoolAlloc (len, MEMPOOL_FILESYS, 0);
	*buffer = buf;

	FS_Read (buf, len, fileNum);
	FS_CloseFile (fileNum);

	return len;
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
	dPackHeader_t	header;
	mPackFile_t		*outPackFile;
	mPack_t			*outPack;
	FILE			*handle;
	dPackFile_t		info[PAK_MAX_FILES];
	int				i, numFiles;
	uint32			hashValue;

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
	if (numFiles <= 0) {
		fclose (handle);
		Com_Error (ERR_FATAL, "FS_LoadPAK: \"%s\" is empty", fileName);
	}

	// Read past header
	fseek (handle, header.dirOfs, SEEK_SET);
	fread (info, 1, header.dirLen, handle);

	// Create pak
	outPack = Mem_PoolAlloc (sizeof (mPack_t), MEMPOOL_FILESYS, 0);
	outPackFile = Mem_PoolAlloc (sizeof (mPackFile_t) * numFiles, MEMPOOL_FILESYS, 0);

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
	outPkz = Mem_PoolAlloc (sizeof (mPack_t), MEMPOOL_FILESYS, 0);
	outPkzFile = Mem_PoolAlloc (sizeof (mPackFile_t) * numFiles, MEMPOOL_FILESYS, 0);

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
	char			searchName[MAX_OSPATH];
	char			*packFiles[PAK_MAX_PAKS];
	int				numPacks, i;
	fs_searchPath_t	*search;
	mPack_t			*pak;
	mPack_t			*pkz;

	if (fs_developer->integer)
		Com_Printf (0, "FS_AddGameDirectory: adding \"%s\"\n", dir);

	// Set as game directory
	Q_strncpyz (fs_gameDir, dir, sizeof (fs_gameDir));

	// Add the directory to the search path
	search = Mem_PoolAlloc (sizeof (fs_searchPath_t), MEMPOOL_FILESYS, 0);
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
		search = Mem_PoolAlloc (sizeof (fs_searchPath_t), MEMPOOL_FILESYS, 0);
		search->package = pak;
		search->next = fs_searchPaths;
		fs_searchPaths = search;
	}

	// Add the rest of the *.pak files
	if (!fs_defaultPaks->value) {
		numPacks = Sys_FindFiles (dir, "*/*.pak", packFiles, PAK_MAX_PAKS, 0, qFalse, qTrue, qFalse);

		for (i=0 ; i<numPacks ; i++) {
			if (strstr (packFiles[i], "/pak0.pak") || strstr (packFiles[i], "/pak1.pak") ||
				strstr (packFiles[i], "/pak2.pak") || strstr (packFiles[i], "/pak3.pak") ||
				strstr (packFiles[i], "/pak4.pak") || strstr (packFiles[i], "/pak5.pak") ||
				strstr (packFiles[i], "/pak6.pak") || strstr (packFiles[i], "/pak7.pak") ||
				strstr (packFiles[i], "/pak8.pak") || strstr (packFiles[i], "/pak9.pak"))
				continue; // FIXME :|

			pak = FS_LoadPAK (packFiles[i], qTrue);
			if (!pak)
				continue;
			search = Mem_PoolAlloc (sizeof (fs_searchPath_t), MEMPOOL_FILESYS, 0);
			Q_strncpyz (search->pathName, dir, sizeof (search->pathName));
			Q_strncpyz (search->gamePath, gamePath, sizeof (search->gamePath));
			search->package = pak;
			search->next = fs_searchPaths;
			fs_searchPaths = search;
		}

		FS_FreeFileList (packFiles, numPacks);
	}

	// Load *.pkz files
	numPacks = Sys_FindFiles (dir, "*/*.pkz", packFiles, PAK_MAX_PAKS, 0, qFalse, qTrue, qFalse);

	for (i=0 ; i<numPacks ; i++) {
		pkz = FS_LoadPKZ (packFiles[i], qTrue);
		if (!pkz)
			continue;
		search = Mem_PoolAlloc (sizeof (fs_searchPath_t), MEMPOOL_FILESYS, 0);
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
	fs_searchPath_t	*next;
	mPack_t			*package;
	int				initTime;

	// Make sure it's not a path
	if (strstr (dir, "..") || strstr (dir, "/") || strstr (dir, "\\") || strstr (dir, ":")) {
		Com_Printf (PRNT_WARNING, "FS_SetGamedir: Gamedir should be a single directory name, not a path\n");
		return;
	}

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
	initTime = Sys_Milliseconds ();
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

	if (!firstTime) {
		Com_Printf (0, "----------------------------------------\n");
		Com_Printf (0, "init time: %.2fs\n", (Sys_Milliseconds () - initTime)*0.001f);
		Com_Printf (0, "----------------------------------------\n");

#ifndef DEDICATED_ONLY
		// Forced to reload to flush old data
		if (!dedicated->integer) {
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
int FS_FindFiles (char *path, char *filter, char *extension, char **fileList, int maxFiles, qBool addGameDir, qBool recurse)
{
	fs_searchPath_t	*search;
	fs_searchPath_t	**invertedPaths;
	mPackFile_t		*packFile;
	mPack_t			*pack;
	int				fileCount;
	char			*name;
	char			dir[MAX_OSPATH];
	char			ext[MAX_QEXT];
	char			*dirFiles[FS_MAX_SEARCHFILES];
	int				dirCount, i, j, k;
	int				numPaths;

	// Sanity check
	if (maxFiles > FS_MAX_SEARCHFILES) {
		Com_Printf (PRNT_ERROR, "FS_FindFiles: maxFiles(%i) > %i, forcing %i...\n", maxFiles, FS_MAX_SEARCHFILES, FS_MAX_SEARCHFILES);
		maxFiles = FS_MAX_SEARCHFILES;
	}

	// We do this since searchPath is technically "backwards"
	// FIXME: store an inverted list, so this isn't necessary
	for (numPaths=0, search=fs_searchPaths ; search ; search=search->next, numPaths++);
	invertedPaths = Mem_PoolAlloc (sizeof (fs_searchPath_t) * numPaths, MEMPOOL_FILESYS, 0);

	for (i=numPaths-1, search=fs_searchPaths ; i>=0 ; search=search->next, i--) {
		invertedPaths[i] = search;
	}

	// Search through the path, one element at a time
	fileCount = 0;
	for (k=0 ; k<numPaths ; k++) {
		search = invertedPaths[k];

		if (search->package) {
			// Pack file
			pack = search->package;
			for (i=0, packFile=pack->files ; i<pack->numFiles ; i++, packFile++) {
				// Match path
				if (!recurse) {
					Com_FilePath (packFile->fileName, dir, sizeof(dir));
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
							fileList[fileCount++] = Mem_StrDup (Q_VarArgs ("%s/%s", search->gamePath, name));
						else
							fileList[fileCount++] = Mem_StrDup (name);
					}
				}
			}
		}
		else {
			// Directory tree
			Q_snprintfz (dir, sizeof (dir), "%s/%s", search->pathName, path);

			if (extension) {
				Q_snprintfz (ext, sizeof (ext), "*.%s", extension);
				dirCount = Sys_FindFiles (dir, ext, dirFiles, FS_MAX_SEARCHFILES, 0, recurse, qTrue, qFalse);
			}
			else {
				dirCount = Sys_FindFiles (dir, "*", dirFiles, FS_MAX_SEARCHFILES, 0, recurse, qTrue, qTrue);
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
							fileList[fileCount++] = Mem_StrDup (Q_VarArgs ("%s/%s", search->gamePath, name));
						else
							fileList[fileCount++] = Mem_StrDup (name);
					}
				}

				Mem_Free (dirFiles[i]);
			}
		}
	}

	Mem_Free (invertedPaths);
	return fileCount;
}


/*
=============
_FS_FreeFileList
=============
*/
void _FS_FreeFileList (char **list, int num, const char *fileName, const int fileLine)
{
	int		i;

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
	fs_searchPath_t	*s;
	char			*prev;

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

Creates a fs_fileLink_t
================
*/
void FS_Link_f (void)
{
	fs_fileLink_t	*l, **prev;

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
			l->to = Mem_PoolStrDup (Cmd_Argv (2), MEMPOOL_FILESYS, 0);
			return;
		}
		prev = &l->next;
	}

	// Create a new link
	l = Mem_PoolAlloc (sizeof (*l), MEMPOOL_FILESYS, 0);
	l->from = Mem_PoolStrDup (Cmd_Argv (1), MEMPOOL_FILESYS, 0);
	l->fromLength = strlen (l->from);
	l->to = Mem_PoolStrDup (Cmd_Argv (2), MEMPOOL_FILESYS, 0);
	l->next = fs_fileLinks;
	fs_fileLinks = l;
}


/*
============
FS_Path_f
============
*/
void FS_Path_f (void)
{
	fs_searchPath_t	*s;
	fs_fileLink_t	*l;

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
	int		initTime;

	initTime = Sys_Milliseconds ();
	Com_Printf (0, "\n------- Filesystem Initialization ------\n");

	// Register commands/cvars
	Cmd_AddCommand ("link",	FS_Link_f,		"");
	Cmd_AddCommand ("path",	FS_Path_f,		"");

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

	if (fs_game->string[0])
		FS_SetGamedir (fs_game->string, qTrue);

	// Check memory integrity
	Mem_CheckPoolIntegrity (MEMPOOL_FILESYS);

	Com_Printf (0, "----------------------------------------\n");
	Com_Printf (0, "init time: %.2fs\n", (Sys_Milliseconds () - initTime)*0.001f);
	Com_Printf (0, "----------------------------------------\n");
}
