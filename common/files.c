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
#define USE_HASH
#include "common.h"

cVar_t	*fs_basedir;
cVar_t	*fs_cddir;
cVar_t	*fs_gamedirvar;

cVar_t	*sv_defaultpaks;

/*
=============================================================================

	EGL FILESYSTEM

=============================================================================
*/

#define MAX_FILE_HASH	256

static char			fsGameDir[MAX_OSPATH];

typedef struct mPackFile_s {
	char				fileName[MAX_QPATH];

	int					filePos;
	int					fileLen;

	struct mPackFile_s	*hashNext;
} mPackFile_t;

typedef struct mPack_s {
	char				fileName[MAX_OSPATH];

	FILE				*handle;

	struct mPackFile_s	*fileHashTree[MAX_FILE_HASH];

	int					numFiles;
	mPackFile_t			*files;
} mPack_t;

typedef struct fileLink_s {
	struct fileLink_s	*next;
	char				*from;
	int					fromLength;
	char				*to;
} fileLink_t;

typedef struct searchPath_s {
	char				fileName[MAX_OSPATH];
	mPack_t				*pack;		// only one of filename / pack will be used
	struct searchPath_s	*next;
} searchPath_t;

static fileLink_t	*fsLinks;

static searchPath_t	*fsSearchPaths;
static searchPath_t	*fsBaseSearchPaths;	// without gamedirs

/*
=============================================================================

	FILE HANDLING

=============================================================================
*/

/*
================
FS_FileLength
================
*/
int FS_FileLength (FILE *f) {
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
void FS_CreatePath (char *path) {
	char	*ofs;
	
	for (ofs=path+1 ; *ofs ; ofs++) {
		if (*ofs == '/') {
			// create the directory
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
void FS_CopyFile (char *src, char *dst) {
	FILE	*f1, *f2;
	int		l;
	byte	buffer[65536];

	Com_Printf (PRNT_DEV, "CopyFile (%s, %s)\n", src, dst);

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
==============
FS_FCloseFile

For some reason, other dll's can't just cal fclose()
on files returned by FS_FOpenFile...
==============
*/
void FS_FCloseFile (FILE *f) {
	fclose (f);
}


/*
===========
FS_FOpenFile

Finds the file in the search path.
returns filesize and an open FILE *
Used for streaming data out of either a pak file or
a seperate file.
===========
*/
qBool	fileFromPak = qFalse;
int FS_FOpenFile (char *fileName, FILE **file) {
	searchPath_t	*searchPath;
	char			netpath[MAX_OSPATH];
	mPack_t			*pak;
	mPackFile_t		*searchFile;
	fileLink_t		*link;
	uLong			hashValue;

	fileFromPak = qFalse;

	// check for links first
	for (link=fsLinks ; link ; link=link->next) {
		if (!strncmp (fileName, link->from, link->fromLength)) {
			Q_snprintfz (netpath, sizeof (netpath), "%s%s", link->to, fileName+link->fromLength);
			*file = fopen (netpath, "rb");
			if (*file) {		
				Com_Printf (PRNT_DEV, "link file: %s\n", netpath);
				return FS_FileLength (*file);
			}
			return -1;
		}
	}

	// calculate hash value
	hashValue = CalcHash (fileName, MAX_FILE_HASH);

	// search through the path, one element at a time
	for (searchPath=fsSearchPaths ; searchPath ; searchPath=searchPath->next) {
		// is the element a pak file?
		if (searchPath->pack) {
			// look through all the pak file elements
			pak = searchPath->pack;

			for (searchFile=pak->fileHashTree[hashValue] ; searchFile ; searchFile=searchFile->hashNext) {
				if (!Q_stricmp (searchFile->fileName, fileName)) {
					// found it!
					fileFromPak = qTrue;
					Com_Printf (PRNT_DEV, "PackFile: %s : %s\n", pak->fileName, fileName);

					// open a new file on the pakfile
					*file = fopen (pak->fileName, "rb");
					if (!*file)
						Com_Error (ERR_FATAL, "Couldn't reopen %s", pak->fileName);	

					fseek (*file, searchFile->filePos, SEEK_SET);
					return searchFile->fileLen;
				}
			}
		}
		else {		
			// check a file in the directory tree
			Q_snprintfz (netpath, sizeof (netpath), "%s/%s", searchPath->fileName, fileName);
			
			*file = fopen (netpath, "rb");
			if (!*file)
				continue;
			
			Com_Printf (PRNT_DEV, "FindFile: %s\n", netpath);

			return FS_FileLength (*file);
		}
		
	}
	
	Com_Printf (PRNT_DEV, "FindFile: can't find %s\n", fileName);
	
	*file = NULL;
	return -1;
}


/*
=================
FS_Read

Properly handles partial reads
=================
*/
void CDAudio_Stop (void);
#define	MAX_READ	0x10000		// read in blocks of 64k
void FS_Read (void *buffer, int len, FILE *f) {
	int		block, remaining;
	int		read;
	byte	*buf;
	int		tries;

	buf = (byte *)buffer;

	// read in chunks for progress bar
	remaining = len;
	tries = 0;
	while (remaining) {
		block = remaining;
		if (block > MAX_READ)
			block = MAX_READ;
		read = fread (buf, 1, block, f);
		if (read == 0) {
			// we might have been trying to read from a CD
			if (!tries) {
				tries = 1;
				CDAudio_Stop ();
			}
			else
				Com_Error (ERR_FATAL, "FS_Read: 0 bytes read");
		}

		if (read == -1)
			Com_Error (ERR_FATAL, "FS_Read: -1 bytes read");

		// do some progress bar thing here
		remaining -= read;
		buf += read;
	}
}


/*
============
FS_LoadFile

Filename are reletive to the egl search path a null buffer
will just return the file length without loading
============
*/
int FS_LoadFile (char *path, void **buffer) {
	FILE	*h;
	byte	*buf;
	int		len;

	buf = NULL;	// quiet compiler warning

	// look for it in the filesystem or pack files
	len = FS_FOpenFile (path, &h);
	if (!h || (len <= 0)) {
		if (buffer)
			*buffer = NULL;
		return -1;
	}
	
	if (!buffer) {
		fclose (h);
		return len;
	}

	buf = Mem_PoolAlloc (len, MEMPOOL_FILESYS, 0);
	*buffer = buf;

	FS_Read (buf, len, h);

	fclose (h);

	return len;
}


/*
=============
FS_FreeFile
=============
*/
void FS_FreeFile (void *buffer) {
	if (buffer)
		Mem_Free (buffer);
}


/*
=============
FS_FreeFileList
=============
*/
void FS_FreeFileList (char **list, int n) {
	int i;

	for (i=0 ; i<n ; i++) {
		if (list[i]) {
			Mem_Free (list[i]);
			list[i] = NULL;
		}
	}

	Mem_Free (list);
}


/*
=================
FS_LoadPackFile

Takes an explicit (not game tree related) path to a pak file.

Loads the header and directory, adding the files at the beginning
of the list so they override previous pack files.
=================
*/
mPack_t *FS_LoadPackFile (char *packFile) {
	dPackHeader_t	header;
	mPackFile_t		*packFiles;
	mPack_t			*pack;
	FILE			*packHandle;
	dPackFile_t		info[PAK_MAXFILES];
	int				i, numPackFiles;
	uLong			hashValue;

	packHandle = fopen (packFile, "rb");
	if (!packHandle)
		return NULL;

	fread (&header, 1, sizeof (header), packHandle);
	if (LittleLong (header.ident) != PAK_HEADER)
		Com_Error (ERR_FATAL, "%s is not a packfile", packFile);
	header.dirOfs = LittleLong (header.dirOfs);
	header.dirLen = LittleLong (header.dirLen);

	numPackFiles = header.dirLen / sizeof (dPackFile_t);
	if (numPackFiles > PAK_MAXFILES)
		Com_Error (ERR_FATAL, "%s has too many files, %i > %i", packFile, numPackFiles, PAK_MAXFILES);

	fseek (packHandle, header.dirOfs, SEEK_SET);
	fread (info, 1, header.dirLen, packHandle);

	//
	// create pak
	//
	pack = Mem_PoolAlloc (sizeof (mPack_t), MEMPOOL_FILESYS, 0);
	packFiles = Mem_PoolAlloc (numPackFiles * sizeof (mPackFile_t), MEMPOOL_FILESYS, 0);

	Q_strncpyz (pack->fileName, packFile, sizeof (pack->fileName));

	// parse the directory
	for (i=0 ; i<numPackFiles ; i++) {
		Q_strncpyz (packFiles[i].fileName, info[i].name, sizeof (packFiles[i].fileName));
		packFiles[i].filePos = LittleLong (info[i].filePos);
		packFiles[i].fileLen = LittleLong (info[i].fileLen);

		// link it into the hash tree
		hashValue = CalcHash (packFiles[i].fileName, MAX_FILE_HASH);

		packFiles[i].hashNext = pack->fileHashTree[hashValue];
		pack->fileHashTree[hashValue] = &packFiles[i];
	}

	pack->handle = packHandle;
	pack->numFiles = numPackFiles;
	pack->files = packFiles;

	Com_Printf (PRNT_DEV, " %s", packFile);

	if (Q_strlen (packFile) < 25)
		for (i=0 ; i<(25 - Q_strlen (packFile)) ; i++)
			Com_Printf (PRNT_DEV, " ");
	Com_Printf (PRNT_DEV, " (%4i files)\n", numPackFiles);

	return pack;
}


/*
================
FS_AddGameDirectory

Sets fsGameDir, adds the directory to the head of the path,
then loads and adds pak1.pak pak2.pak ... 
================
*/
void FS_AddGameDirectory (char *dir) {
	int				i, numFiles;
	searchPath_t	*search;
	mPack_t			*pak;
	char			pakfile[MAX_OSPATH];
	char			**searchnames;
	int				fsInitTime;

	fsInitTime = Sys_Milliseconds ();
	Com_Printf (PRNT_DEV, "\n------------- Adding Files -------------\n");
	Com_Printf (PRNT_DEV, "%s:\n", dir);

	Q_strncpyz (fsGameDir, dir, sizeof (fsGameDir));

	// add the directory to the search path
	search = Mem_PoolAlloc (sizeof (searchPath_t), MEMPOOL_FILESYS, 0);
	Q_strncpyz (search->fileName, dir, sizeof (search->fileName));
	search->next = fsSearchPaths;
	fsSearchPaths = search;

	// !HACK! make sure egl.pak is loaded !HACK!
	if (sv_defaultpaks->value) {
		Com_Printf (PRNT_DEV, "...sv_defaultpaks on, forcing load of egl.pak\n");

		Q_snprintfz (pakfile, sizeof (pakfile), "%s/egl.pak", BASE_MODDIRNAME);
		pak = FS_LoadPackFile (pakfile);
		if (!pak) {
			Com_Printf (PRNT_DEV, "...forced load of egl.pak FAILED\n");
		}
		else {
			search = Mem_PoolAlloc (sizeof (searchPath_t), MEMPOOL_FILESYS, 0);
			search->pack = pak;
			search->next = fsSearchPaths;
			fsSearchPaths = search;
		}
	}

	// add any pak files in the format pak0.pak pak1.pak, ...
	for (i=0 ; i<10 ; i++) {
		Q_snprintfz (pakfile, sizeof (pakfile), "%s/pak%i.pak", dir, i);
		pak = FS_LoadPackFile (pakfile);
		if (!pak)
			continue;
		search = Mem_PoolAlloc (sizeof (searchPath_t), MEMPOOL_FILESYS, 0);
		search->pack = pak;
		search->next = fsSearchPaths;
		fsSearchPaths = search;
	}

	if (sv_defaultpaks->value)
		return;

	// add the rest of the *.pak files
	Q_snprintfz (pakfile, sizeof (pakfile), "%s/*.pak", dir);
	searchnames = FS_ListFiles (pakfile, &numFiles, 0, 0);

	if (searchnames) {
		for (i=0 ; i<(numFiles - 1) ; i++) {
			if (strrchr (searchnames[i], '/')) {
				if (searchnames[i][Q_strlen (searchnames[i])-4] != '.')
					continue; // stops .pakanything from loading

				if (strstr (searchnames[i], "/pak0.pak") || strstr (searchnames[i], "/pak1.pak") ||
					strstr (searchnames[i], "/pak2.pak") || strstr (searchnames[i], "/pak3.pak") ||
					strstr (searchnames[i], "/pak4.pak") || strstr (searchnames[i], "/pak5.pak") ||
					strstr (searchnames[i], "/pak6.pak") || strstr (searchnames[i], "/pak7.pak") ||
					strstr (searchnames[i], "/pak8.pak") || strstr (searchnames[i], "/pak9.pak"))
					continue; // FIXME :|

				pak = FS_LoadPackFile (searchnames[i]);
				if (!pak)
					continue;
				search = Mem_PoolAlloc (sizeof (searchPath_t), MEMPOOL_FILESYS, 0);
				search->pack = pak;
				search->next = fsSearchPaths;
				fsSearchPaths = search;
			}
		}

		FS_FreeFileList (searchnames, i);
	}

	Com_Printf (PRNT_DEV, "----------------------------------------\n");
	Com_Printf (PRNT_DEV, "init time: %.2fs\n", (Sys_Milliseconds () - fsInitTime)*0.001f);
	Com_Printf (PRNT_DEV, "----------------------------------------\n");
}

/*
=============================================================================

	GAME PATH

=============================================================================
*/

/*
==============
FS_CurrGame

Checks name against game dir name and returns if it matches
==============
*/
qBool FS_CurrGame (char *name) {
	searchPath_t	*search;

	for (search=fsSearchPaths ; search ; search=search->next) {
		if (strstr (search->fileName, name))
			return qTrue;
	}

	return qFalse;
}


/*
============
FS_Gamedir

Called to find where to write a file (demos, savegames, etc)
============
*/
char *FS_Gamedir (void) {
	if (*fsGameDir)
		return fsGameDir;
	else
		return BASE_MODDIRNAME;
}


/*
================
FS_SetGamedir

Sets the gamedir and path to a different directory.
================
*/
void FS_SetGamedir (char *dir, qBool firstTime) {
	searchPath_t	*next;

	if (strstr (dir, "..") || strstr (dir, "/") || strstr (dir, "\\") || strstr (dir, ":")) {
		Com_Printf (0, S_COLOR_YELLOW "Gamedir should be a single directory name, not a path\n");
		return;
	}

	// free up any current game dir info
	for ( ; fsSearchPaths != fsBaseSearchPaths ; fsSearchPaths=next) {
		next = fsSearchPaths->next;
		if (fsSearchPaths->pack) {
			fclose (fsSearchPaths->pack->handle);
			Mem_Free (fsSearchPaths->pack->files);
			Mem_Free (fsSearchPaths->pack);
		}
		Mem_Free (fsSearchPaths);
	}

	// forced to reload to flush old data
	if (dedicated && !dedicated->integer && Cmd_Exists ("vid_restart"))
		Cmd_ExecuteString ("vid_restart\n");

	Q_snprintfz (fsGameDir, sizeof (fsGameDir), "%s/%s", fs_basedir->string, dir);

	if (!strcmp (dir, BASE_MODDIRNAME) || (*dir == 0)) {
		Cvar_FullSet ("gamedir", "", CVAR_SERVERINFO|CVAR_NOSET);
		Cvar_FullSet ("game", "", CVAR_LATCH_SERVER|CVAR_SERVERINFO|CVAR_RESET_GAMEDIR);
	}
	else {
		Cvar_FullSet ("gamedir", dir, CVAR_SERVERINFO|CVAR_NOSET);
		if (fs_cddir->string[0])
			FS_AddGameDirectory (Q_VarArgs ("%s/%s", fs_cddir->string, dir));

		FS_AddGameDirectory (Q_VarArgs ("%s/%s", fs_basedir->string, dir));
	}

	if (!firstTime) {
		Cbuf_AddText ("exec config.cfg\n");
		Cbuf_AddText ("exec eglcfg.cfg\n");
		FS_ExecAutoexec ();
	}
}


/*
=============
FS_ExecAutoexec
=============
*/
void FS_ExecAutoexec (void) {
	char	*dir;
	char	name[MAX_QPATH];

	dir = Cvar_VariableString ("gamedir");
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

static searchPath_t	*fsCurrSearch = NULL;
static int			fsCurrPakIndex = 0;

/*
================
FS_ListFiles
================
*/
char **FS_ListFiles (char *findName, int *numFiles, uInt mustHave, uInt cantHave) {
	char	*s;
	int		nFiles = 0;
	char	**list = 0;

	if (!findName)
		return NULL;

	s = Sys_FindFirst (findName, mustHave, cantHave);
	while (s) {
		if (s[Q_strlen (s) - 1] != '.')
			nFiles++;
		s = Sys_FindNext (mustHave, cantHave);
	}
	Sys_FindClose ();

	if (!nFiles)
		return NULL;

	nFiles++; // add space for a guard
	if (numFiles)
		*numFiles = nFiles;

	list = Mem_PoolAlloc (sizeof (char *) * nFiles, MEMPOOL_FILESYS, 0);

	s = Sys_FindFirst (findName, mustHave, cantHave);
	nFiles = 0;
	while (s) {
		if (s[Q_strlen (s) - 1] != '.') {
			list[nFiles] = Mem_PoolStrDup (s, MEMPOOL_FILESYS, 0);
#ifdef _WIN32
			Q_strlwr (list[nFiles]);
#endif
			nFiles++;
		}
		s = Sys_FindNext (mustHave, cantHave);
	}
	Sys_FindClose ();

	return list;
}


/*
================
FS_NextPath

Allows enumerating all of the directories in the search path
================
*/
char *FS_NextPath (char *prevpath) {
	searchPath_t	*s;
	char			*prev;

	if (!prevpath)
		return fsGameDir;

	prev = fsGameDir;
	for (s=fsSearchPaths ; s ; s=s->next) {
		if (s->pack)
			continue;
		if (prevpath == prev)
			return s->fileName;
		prev = s->fileName;
	}

	return NULL;
}


/*
================
FS_FindFirst
================
*/
char *FS_FindFirst (char *find) {
	searchPath_t	*search;
	mPack_t			*pak;
	int				i;

	for (search=fsSearchPaths ; search ; search=search->next) {
		if (!search->pack)
			continue;

		pak = search->pack;

		for (i=0 ; i<pak->numFiles ; i++) {
			if (strstr (pak->files[i].fileName, find)) {
				fsCurrSearch = search;
				fsCurrPakIndex = i+1;
				return pak->files[i].fileName;
			}
		}
	}

	fsCurrSearch = NULL;
	fsCurrPakIndex = 0;

	return NULL;
}


/*
================
FS_FindNext
================
*/
char *FS_FindNext (char *find) {
	searchPath_t	*search;
	mPack_t			*pak;
	int i;

	if (!fsCurrSearch)
		return NULL;

	for (search=fsCurrSearch ; search ; search=search->next) {
		if (!search->pack)
			continue;
		pak = search->pack;

		for (i=fsCurrPakIndex ; i<pak->numFiles ; i++) {
			if (strstr (pak->files[i].fileName, find)) {
				fsCurrSearch = search;
				fsCurrPakIndex = i+1;
				return pak->files[i].fileName;
			}
		}

		fsCurrPakIndex = 0;
	}

	fsCurrSearch = NULL;
	fsCurrPakIndex = 0;

	return NULL;
}

/*
=============================================================================

	CONSOLE FUNCTIONS

=============================================================================
*/

/*
================
FS_Dir_f
================
*/
void FS_Dir_f (void) {
	char	*path = NULL;
	char	findName[1024];
	char	wildcard[1024] = "*.*";
	char	**dirnames;
	int		ndirs, i;

	if (Cmd_Argc () != 1)
		Q_strncpyz (wildcard, Cmd_Argv (1), sizeof (wildcard));

	while ((path = FS_NextPath (path)) != NULL) {
		char *tmp = findName;

		Q_snprintfz (findName, sizeof (findName), "%s/%s", path, wildcard);

		while (*tmp != 0) {
			if (*tmp == '\\') 
				*tmp = '/';
			tmp++;
		}

		Com_Printf (0, "Directory of %s\n", findName);
		Com_Printf (0, "----\n");

		if ((dirnames = FS_ListFiles (findName, &ndirs, 0, 0)) != 0) {
			for (i=0 ; i<ndirs-1 ; i++) {
				if (strrchr (dirnames[i], '/'))
					Com_Printf (0, "%s\n", strrchr (dirnames[i], '/') + 1);
				else
					Com_Printf (0, "%s\n", dirnames[i]);

				Mem_Free (dirnames[i]);
			}
			Mem_Free (dirnames);
		}
		Com_Printf (0, "\n");
	}
}


/*
================
FS_Link_f

Creates a fileLink_t
================
*/
void FS_Link_f (void) {
	fileLink_t	*l, **prev;

	if (Cmd_Argc () != 3) {
		Com_Printf (0, "USAGE: link <from> <to>\n");
		return;
	}

	// see if the link already exists
	prev = &fsLinks;
	for (l=fsLinks ; l ; l=l->next) {
		if (!strcmp (l->from, Cmd_Argv (1))) {
			Mem_Free (l->to);
			if (!Q_strlen (Cmd_Argv (2))) {
				// delete it
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

	// create a new link
	l = Mem_PoolAlloc (sizeof (*l), MEMPOOL_FILESYS, 0);
	l->from = Mem_PoolStrDup (Cmd_Argv (1), MEMPOOL_FILESYS, 0);
	l->fromLength = Q_strlen (l->from);
	l->to = Mem_PoolStrDup (Cmd_Argv (2), MEMPOOL_FILESYS, 0);
	l->next = fsLinks;
	fsLinks = l;
}


/*
============
FS_Path_f
============
*/
void FS_Path_f (void) {
	searchPath_t	*s;
	fileLink_t		*l;

	Com_Printf (0, "Current search path:\n");
	for (s=fsSearchPaths ; s ; s=s->next) {
		if (s == fsBaseSearchPaths)
			Com_Printf (0, "----------\n");

		if (s->pack)
			Com_Printf (0, "%s (%i files)\n", s->pack->fileName, s->pack->numFiles);
		else
			Com_Printf (0, "%s\n", s->fileName);
	}

	Com_Printf (0, "\nLinks:\n");
	for (l=fsLinks ; l ; l=l->next) {
		Com_Printf (0, "%s : %s\n", l->from, l->to);
	}
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
void FS_Init (void) {
	// register commands/cvars
	Cmd_AddCommand ("dir",	FS_Dir_f,		"");
	Cmd_AddCommand ("link",	FS_Link_f,		"");
	Cmd_AddCommand ("path",	FS_Path_f,		"");

	sv_defaultpaks	= Cvar_Get ("sv_defaultpaks",	"0",	CVAR_ARCHIVE);

	fs_basedir		= Cvar_Get ("basedir",			".",	CVAR_NOSET);
	fs_cddir		= Cvar_Get ("cddir",			"",		CVAR_NOSET);
	fs_gamedirvar	= Cvar_Get ("game",				"",		CVAR_LATCH_SERVER|CVAR_SERVERINFO|CVAR_RESET_GAMEDIR);

	// load pak files
	if (fs_cddir->string[0])
		FS_AddGameDirectory (Q_VarArgs ("%s/"BASE_MODDIRNAME, fs_cddir->string));

	FS_AddGameDirectory (Q_VarArgs ("%s/"BASE_MODDIRNAME, fs_basedir->string));

	// any set gamedirs will be freed up to here
	fsBaseSearchPaths = fsSearchPaths;

	if (fs_gamedirvar->string[0])
		FS_SetGamedir (fs_gamedirvar->string, qTrue);

	// check memory integrity
	Mem_CheckPoolIntegrity (MEMPOOL_FILESYS);
}

/*
================
FS_Shutdown
================
*/
void FS_Shutdown (void) {
}
