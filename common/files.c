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

#include "common.h"

/*
=============================================================================

	EGL FILESYSTEM

=============================================================================
*/

typedef struct {
	char	name[MAX_QPATH];

	int		filePos;
	int		fileLen;
} packfile_t;

typedef struct pack_s {
	char		fileName[MAX_OSPATH];
	FILE		*handle;
	int			numFiles;
	packfile_t	*files;
} pack_t;

char	fs_gamedir[MAX_OSPATH];
cvar_t	*fs_basedir;
cvar_t	*fs_cddir;
cvar_t	*fs_gamedirvar;

typedef struct fileLink_s {
	struct		fileLink_s	*next;
	char		*from;
	int			fromLength;
	char		*to;
} fileLink_t;

fileLink_t	*fs_links;

typedef struct searchPath_s {
	char		fileName[MAX_OSPATH];
	pack_t		*pack;		// only one of filename / pack will be used
	struct		searchPath_s *next;
} searchPath_t;

static searchPath_t	*fs_searchpaths;
static searchPath_t	*fs_base_searchpaths;	// without gamedirs

cvar_t	*sv_defaultpaks;

// ==========================================================================

/*
================
FS_CopyString
================
*/
static char *FS_CopyString (char *in) {
	char	*out;
	
	out = Z_TagMalloc (strlen(in)+1, ZTAG_FILESYS);
	strcpy (out, in);
	return out;
}

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

	Com_Printf (PRINT_DEV, "CopyFile (%s, %s)\n", src, dst);

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
int file_from_pak = 0;
int FS_FOpenFile (char *filename, FILE **file) {
	searchPath_t	*search;
	char			netpath[MAX_OSPATH];
	pack_t			*pak;
	int				i;
	fileLink_t		*link;

	file_from_pak = 0;

	// check for links first
	for (link=fs_links ; link ; link=link->next) {
		if (!strncmp (filename, link->from, link->fromLength)) {
			Com_sprintf (netpath, sizeof (netpath), "%s%s",link->to, filename+link->fromLength);
			*file = fopen (netpath, "rb");
			if (*file) {		
				Com_Printf (PRINT_DEV, "link file: %s\n",netpath);
				return FS_FileLength (*file);
			}
			return -1;
		}
	}

	// search through the path, one element at a time
	for (search=fs_searchpaths ; search ; search=search->next) {
		// is the element a pak file?
		if (search->pack) {
			// look through all the pak file elements
			pak = search->pack;
			for (i=0 ; i<pak->numFiles ; i++) {
				if (!Q_stricmp (pak->files[i].name, filename)) {
					// found it!
					file_from_pak = 1;
					Com_Printf (PRINT_DEV, "PackFile: %s : %s\n",pak->fileName, filename);

					// open a new file on the pakfile
					*file = fopen (pak->fileName, "rb");
					if (!*file)
						Com_Error (ERR_FATAL, "Couldn't reopen %s", pak->fileName);	
					fseek (*file, pak->files[i].filePos, SEEK_SET);
					return pak->files[i].fileLen;
				}
			}
		} else {		
			// check a file in the directory tree
			Com_sprintf (netpath, sizeof (netpath), "%s/%s", search->fileName, filename);
			
			*file = fopen (netpath, "rb");
			if (!*file)
				continue;
			
			Com_Printf (PRINT_DEV, "FindFile: %s\n",netpath);

			return FS_FileLength (*file);
		}
		
	}
	
	Com_Printf (PRINT_DEV, "FindFile: can't find %s\n", filename);
	
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
			} else
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
	if (!h) {
		if (buffer)
			*buffer = NULL;
		return -1;
	}
	
	if (!buffer) {
		fclose (h);
		return len;
	}

	buf = Z_TagMalloc (len, ZTAG_FILESYS);
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
	Z_Free (buffer);
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
			free (list[i]);
			list[i] = 0;
		}
	}

	free (list);
}


/*
=================
FS_LoadPackFile

Takes an explicit (not game tree related) path to a pak file.

Loads the header and directory, adding the files at the beginning
of the list so they override previous pack files.
=================
*/
pack_t *FS_LoadPackFile (char *packfile) {
	dPackHeader_t	header;
	int				i;
	packfile_t		*newfiles;
	int				numPackFiles;
	pack_t			*pack;
	FILE			*packhandle;
	dPackFile_t		info[PAK_MAXFILES];
	unsigned		checksum;

	packhandle = fopen(packfile, "rb");
	if (!packhandle)
		return NULL;

	fread (&header, 1, sizeof (header), packhandle);
	if (LittleLong (header.ident) != PAK_HEADER)
		Com_Error (ERR_FATAL, "%s is not a packfile", packfile);
	header.dirOfs = LittleLong (header.dirOfs);
	header.dirLen = LittleLong (header.dirLen);

	numPackFiles = header.dirLen / sizeof (dPackFile_t);

	if (numPackFiles > PAK_MAXFILES)
		Com_Error (ERR_FATAL, "%s has too many files, %i > %i", packfile, numPackFiles, PAK_MAXFILES);

	newfiles = Z_TagMalloc (numPackFiles * sizeof (packfile_t), ZTAG_FILESYS);

	fseek (packhandle, header.dirOfs, SEEK_SET);
	fread (info, 1, header.dirLen, packhandle);

	// crc the directory to check for modifications
	checksum = Com_BlockChecksum ((void *)info, header.dirLen);

	// parse the directory
	for (i=0 ; i<numPackFiles ; i++) {
		strcpy (newfiles[i].name, info[i].name);
		newfiles[i].filePos = LittleLong (info[i].filePos);
		newfiles[i].fileLen = LittleLong (info[i].fileLen);
	}

	pack = Z_TagMalloc (sizeof (pack_t), ZTAG_FILESYS);
	strcpy (pack->fileName, packfile);
	pack->handle = packhandle;
	pack->numFiles = numPackFiles;
	pack->files = newfiles;
	
	Com_Printf (PRINT_DEV, " %s", packfile);

	if (strlen (packfile) < 25)
		for (i=0 ; i<(25 - strlen (packfile)) ; i++)
			Com_Printf (PRINT_DEV, " ");

	Com_Printf (PRINT_DEV, " ");

	Com_Printf (PRINT_DEV, "(%4i files)\n", numPackFiles);
	return pack;
}


/*
================
FS_AddGameDirectory

Sets fs_gamedir, adds the directory to the head of the path,
then loads and adds pak1.pak pak2.pak ... 
================
*/
void FS_AddGameDirectory (char *dir) {
	int				i, numfiles;
	searchPath_t	*search;
	pack_t			*pak;
	char			pakfile[MAX_OSPATH];
	char			**searchnames;
	int				fsInitTime;

	fsInitTime = Sys_Milliseconds ();
	Com_Printf (PRINT_DEV, "\n------------- Adding Files -------------\n");
	Com_Printf (PRINT_DEV, "%s:\n", dir);

	strcpy (fs_gamedir, dir);

	/* add the directory to the search path */
	search = Z_TagMalloc (sizeof (searchPath_t), ZTAG_FILESYS);
	strcpy (search->fileName, dir);
	search->next = fs_searchpaths;
	fs_searchpaths = search;

	/* !HACK! make sure egl.pak is loaded !HACK! */
	if (sv_defaultpaks->value) {
		Com_Printf (PRINT_DEV, "...sv_defaultpaks on, forcing load of egl.pak\n");

		Com_sprintf (pakfile, sizeof (pakfile), "%s/egl.pak", BASE_MODDIRNAME);
		pak = FS_LoadPackFile (pakfile);
		if (!pak) {
			Com_Printf (PRINT_DEV, "...forced load of egl.pak FAILED\n");
			goto enddefpaks;
		}

		search = Z_TagMalloc (sizeof (searchPath_t), ZTAG_FILESYS);
		search->pack = pak;
		search->next = fs_searchpaths;
		fs_searchpaths = search;
	}
enddefpaks:

	/* add any pak files in the format pak0.pak pak1.pak, ... */
	for (i=0 ; i<10 ; i++) {
		Com_sprintf (pakfile, sizeof (pakfile), "%s/pak%i.pak", dir, i);
		pak = FS_LoadPackFile (pakfile);
		if (!pak) continue;
		search = Z_TagMalloc (sizeof (searchPath_t), ZTAG_FILESYS);
		search->pack = pak;
		search->next = fs_searchpaths;
		fs_searchpaths = search;
	}

	if (sv_defaultpaks->value)
		return;

	/* add the rest of the *.pak files */
	Com_sprintf (pakfile, sizeof (pakfile), "%s/*.pak", dir);
	searchnames = FS_ListFiles (pakfile, &numfiles, 0, 0);

	if (searchnames) {
		for (i=0 ; i<(numfiles - 1) ; i++) {
			if (strrchr (searchnames[i], '/')) {
				if (searchnames[i][strlen(searchnames[i])-4] != '.')
					continue; // stops .pakanything from loading

				if (strstr (searchnames[i], "/pak0.pak") || strstr (searchnames[i], "/pak1.pak") ||
					strstr (searchnames[i], "/pak2.pak") || strstr (searchnames[i], "/pak3.pak") ||
					strstr (searchnames[i], "/pak4.pak") || strstr (searchnames[i], "/pak5.pak") ||
					strstr (searchnames[i], "/pak6.pak") || strstr (searchnames[i], "/pak7.pak") ||
					strstr (searchnames[i], "/pak8.pak") || strstr (searchnames[i], "/pak9.pak"))
					continue; // FIXME :|

				pak = FS_LoadPackFile (searchnames[i]);
				if (!pak) continue;
				search = Z_TagMalloc (sizeof (searchPath_t), ZTAG_FILESYS);
				search->pack = pak;
				search->next = fs_searchpaths;
				fs_searchpaths = search;
			}
		}

		FS_FreeFileList (searchnames, i);
	}

	Com_Printf (PRINT_DEV, "----------------------------------------\n");
	Com_Printf (PRINT_DEV, "init time: %dms\n", Sys_Milliseconds () - fsInitTime);
	Com_Printf (PRINT_DEV, "----------------------------------------\n");
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

	for (search=fs_searchpaths ; search ; search=search->next) {
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
	if (*fs_gamedir)
		return fs_gamedir;
	else
		return BASE_MODDIRNAME;
}


/*
================
FS_SetGamedir

Sets the gamedir and path to a different directory.
================
*/
void FS_SetGamedir (char *dir) {
	searchPath_t	*next;

	if (strstr (dir, "..") || strstr (dir, "/") || strstr (dir, "\\") || strstr (dir, ":")) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Gamedir should be a single directory name, not a path\n");
		return;
	}

	// free up any current game dir info
	while (fs_searchpaths != fs_base_searchpaths) {
		if (fs_searchpaths->pack) {
			fclose (fs_searchpaths->pack->handle);
			Z_Free (fs_searchpaths->pack->files);
			Z_Free (fs_searchpaths->pack);
		}
		next = fs_searchpaths->next;
		Z_Free (fs_searchpaths);
		fs_searchpaths = next;
	}

	// flush all data, so it will be forced to reload
	if (dedicated && !dedicated->integer) {
		ui_Restart = qTrue;
		vid_Restart = qTrue;
		snd_Restart = qTrue;
	}

	Com_sprintf (fs_gamedir, sizeof (fs_gamedir), "%s/%s", fs_basedir->string, dir);

	if (!strcmp (dir, BASE_MODDIRNAME) || (*dir == 0)) {
		Cvar_FullSet ("gamedir", "", CVAR_SERVERINFO|CVAR_NOSET);
		Cvar_FullSet ("game", "", CVAR_LATCH|CVAR_SERVERINFO);
	} else {
		Cvar_FullSet ("gamedir", dir, CVAR_SERVERINFO|CVAR_NOSET);
		if (fs_cddir->string[0])
			FS_AddGameDirectory (va ("%s/%s", fs_cddir->string, dir));

		FS_AddGameDirectory (va ("%s/%s", fs_basedir->string, dir));
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
		Com_sprintf (name, sizeof (name), "%s/%s/autoexec.cfg", fs_basedir->string, dir);
	else
		Com_sprintf (name, sizeof (name), "%s/%s/autoexec.cfg", fs_basedir->string, BASE_MODDIRNAME);

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
FS_ListFiles
================
*/
char **FS_ListFiles (char *findName, int *numFiles, unsigned mustHave, unsigned cantHave) {
	char	*s;
	int		nfiles = 0;
	char	**list = 0;

	s = Sys_FindFirst (findName, mustHave, cantHave);
	while (s) {
		if (s[strlen(s)-1] != '.')
			nfiles++;
		s = Sys_FindNext (mustHave, cantHave);
	}
	Sys_FindClose ();

	if (!nfiles)
		return NULL;

	nfiles++; // add space for a guard
	*numFiles = nfiles;

	list = malloc (sizeof (char *) * nfiles);
	memset (list, 0, sizeof (char *) * nfiles);

	s = Sys_FindFirst (findName, mustHave, cantHave);
	nfiles = 0;
	while (s) {
		if (s[strlen(s)-1] != '.') {
			list[nfiles] = strdup (s);
			Q_strlwr (list[nfiles]);
			nfiles++;
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
		return fs_gamedir;

	prev = fs_gamedir;
	for (s=fs_searchpaths ; s ; s=s->next) {
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
searchPath_t	*curr_search = NULL;
int				curr_pak_index = 0;
char *FS_FindFirst (char *find) {
	searchPath_t	*search;
	pack_t			*pak;
	int				i;

	for (search=fs_searchpaths ; search ; search=search->next) {
		if (!search->pack)
			continue;

		pak = search->pack;

		for (i=0 ; i<pak->numFiles ; i++) {
			if (strstr (pak->files[i].name, find)) {
				curr_search = search;
				curr_pak_index = i+1;
				return pak->files[i].name;
			}
		}
	}

	curr_search = NULL;
	curr_pak_index = 0;

	return NULL;
}


/*
================
FS_FindNext
================
*/
char *FS_FindNext (char *find) {
	searchPath_t	*search;
	pack_t			*pak;
	int i;

	if (!curr_search)
		return NULL;

	for (search=curr_search ; search ; search=search->next) {
		if (!search->pack)
			continue;
		pak = search->pack;

		for (i=curr_pak_index ; i<pak->numFiles ; i++) {
			if (strstr (pak->files[i].name, find)) {
				curr_search = search;
				curr_pak_index = i+1;
				return pak->files[i].name;
			}
		}

		curr_pak_index = 0;
	}

	curr_search = NULL;
	curr_pak_index = 0;

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
		strcpy (wildcard, Cmd_Argv (1));

	while ((path = FS_NextPath (path)) != NULL) {
		char *tmp = findName;

		Com_sprintf (findName, sizeof (findName), "%s/%s", path, wildcard);

		while (*tmp != 0) {
			if (*tmp == '\\') 
				*tmp = '/';
			tmp++;
		}

		Com_Printf (PRINT_ALL, "Directory of %s\n", findName);
		Com_Printf (PRINT_ALL, "----\n");

		if ((dirnames = FS_ListFiles (findName, &ndirs, 0, 0)) != 0) {
			for (i=0 ; i<ndirs-1 ; i++) {
				if (strrchr (dirnames[i], '/'))
					Com_Printf (PRINT_ALL, "%s\n", strrchr (dirnames[i], '/') + 1);
				else
					Com_Printf (PRINT_ALL, "%s\n", dirnames[i]);

				free (dirnames[i]);
			}
			free (dirnames);
		}
		Com_Printf (PRINT_ALL, "\n");
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
		Com_Printf (PRINT_ALL, "USAGE: link <from> <to>\n");
		return;
	}

	// see if the link already exists
	prev = &fs_links;
	for (l=fs_links ; l ; l=l->next) {
		if (!strcmp (l->from, Cmd_Argv (1))) {
			Z_Free (l->to);
			if (!strlen (Cmd_Argv (2))) {
				// delete it
				*prev = l->next;
				Z_Free (l->from);
				Z_Free (l);
				return;
			}
			l->to = FS_CopyString (Cmd_Argv (2));
			return;
		}
		prev = &l->next;
	}

	// create a new link
	l = Z_TagMalloc (sizeof (*l), ZTAG_FILESYS);
	l->next = fs_links;
	fs_links = l;
	l->from = FS_CopyString (Cmd_Argv (1));
	l->fromLength = strlen (l->from);
	l->to = FS_CopyString (Cmd_Argv (2));
}


/*
============
FS_Path_f
============
*/
void FS_Path_f (void) {
	searchPath_t	*s;
	fileLink_t		*l;

	Com_Printf (PRINT_ALL, "Current search path:\n");
	for (s=fs_searchpaths ; s ; s=s->next) {
		if (s == fs_base_searchpaths)
			Com_Printf (PRINT_ALL, "----------\n");
		if (s->pack)
			Com_Printf (PRINT_ALL, "%s (%i files)\n", s->pack->fileName, s->pack->numFiles);
		else
			Com_Printf (PRINT_ALL, "%s\n", s->fileName);
	}

	Com_Printf (PRINT_ALL, "\nLinks:\n");
	for (l=fs_links ; l ; l=l->next)
		Com_Printf (PRINT_ALL, "%s : %s\n", l->from, l->to);
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
	Cmd_AddCommand ("dir",	FS_Dir_f);
	Cmd_AddCommand ("link",	FS_Link_f);
	Cmd_AddCommand ("path",	FS_Path_f);

	sv_defaultpaks	= Cvar_Get ("sv_defaultpaks",	"0",	CVAR_ARCHIVE);

	fs_basedir		= Cvar_Get ("basedir",			".",	CVAR_NOSET);
	fs_cddir		= Cvar_Get ("cddir",			"",		CVAR_NOSET);
	fs_gamedirvar	= Cvar_Get ("game",				"",		CVAR_LATCH|CVAR_SERVERINFO);

	// load pak files
	if (fs_cddir->string[0])
		FS_AddGameDirectory (va ("%s/"BASE_MODDIRNAME, fs_cddir->string));

	FS_AddGameDirectory (va ("%s/"BASE_MODDIRNAME, fs_basedir->string));

	// any set gamedirs will be freed up to here
	fs_base_searchpaths = fs_searchpaths;

	if (fs_gamedirvar->string[0])
		FS_SetGamedir (fs_gamedirvar->string);
}

/*
================
FS_Shutdown
================
*/
void FS_Shutdown (void) {
	Cmd_RemoveCommand ("dir");
	Cmd_RemoveCommand ("link");
	Cmd_RemoveCommand ("path");
}
