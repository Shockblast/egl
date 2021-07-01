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
// cl_loc.c
// Nearest location messaging
//
// Originally by R1CH -- www.r1ch.net --
// - Added "addloc" command
// - Random safety checks
// - Slaughtered with my coding style
// - Changed link list style
//

#include "cl_local.h"

typedef struct clLocation_s {
	struct		clLocation_s	*next;

	char		*name;
	vec3_t		location;
} clLocation_t;

static clLocation_t	*clLocations;
static char			locFileName[MAX_QPATH];

/*
==============================================================

	LOADING

==============================================================
*/

/*
====================
CL_LoadLoc

Called on map change or location addition to reload the file
====================
*/
void CL_LoadLoc (char *mapName) {
	clLocation_t	*loc = clLocations;
	char	*x, *y, *z, *name, *line;
	char	readLine[0xFF];
	FILE	*fLoc;
	int		fileLen;

	CL_FreeLoc ();

	if (!mapName)
		return;
	if (!mapName[0])
		return;

	Com_StripExtension (mapName, locFileName);
	Q_snprintfz (locFileName, sizeof (locFileName), "%s.loc", locFileName);

	fileLen = FS_FOpenFile (locFileName, (FILE **)&fLoc);
	if (!fLoc || (fileLen <= 0)) {
		Com_Printf (PRNT_DEV, "CL_LoadLoc: %s not found\n", locFileName);
		return;
	}

	while (line = (fgets (readLine, sizeof (readLine), fLoc))) {
		x = line;

		name = NULL;

		while (*line) {
			if (*line == ' ') {
				*line++ = '\0';
				y = line;
				break;
			}
			line++;
		}

		while (*line) {
			if (*line == ' ') {
				*line++ = '\0';
				z = line;
				break;
			}
			line++;
		}

		while (*line) {
			if ((*line == ' ') && !name) {
				*line = '\0';
				name = line + 1;
			} else if ((*line == '\n') || (*line == '\r')) {
				*line = '\0';
				break;
			}
			line++;
		}

		loc = Mem_Alloc (sizeof (clLocation_t));

		loc->name = Mem_Alloc (strlen (name) + 1);
		strcpy (loc->name, name);
		VectorSet (loc->location, (float)atoi(x)*0.125, (float)atoi(y)*0.125, (float)atoi(z)*0.125);

		// link it in
		loc->next = clLocations;
		clLocations = loc;

		Com_Printf (PRNT_DEV, "CL_AddLoc: adding location '%s'\n", name);
	}

	FS_FCloseFile (fLoc);
}


/*
====================
CL_FreeLoc

Called on map change and on full shutdown
====================
*/
void CL_FreeLoc (void) {
	clLocation_t	*loc;
	clLocation_t	*next;

	if (!clLocations)
		return;

	// free locations
	next = NULL;
	for (loc=clLocations ; loc ; loc=next) {
		next = loc->next;

		Mem_Free (loc->name);
		Mem_Free (loc);
		loc = NULL;
	}

	clLocations = NULL;
}

/*
==============================================================

	MESSAGE PREPROCESSING

==============================================================
*/

/*
====================
CL_Loc_Get

Grabs the location for the message preprocessing
====================
*/
static char *CL_Loc_Get (void) {
	clLocation_t	*loc, *best;
	uInt			length, bestLength = 0xFFFFFFFF;

	best = NULL;
	for (loc=clLocations ; loc ; loc=loc->next) {
		length = VectorDistance (loc->location, cl.refDef.viewOrigin);
		if (length < bestLength) {
			best = loc;
			bestLength = length;
		}
	}

	return best->name;
}


/*
====================
CL_Say_Preprocessor
====================
*/
void CL_Say_Preprocessor (void) {
	if (clLocations) {
		char	*locName, *p;
		char	*sayText;

		sayText = p = Cmd_Args();

		while (*sayText && *(sayText + 1)) {
			if ((*sayText == '@') && (*(sayText + 1) == 'l')) {
				int locLen, cmdLen;

				locName = CL_Loc_Get ();
				if (!locName)
					break;

				cmdLen = strlen (Cmd_Args());
				locLen = strlen (locName);

				if (cmdLen + locLen >= 1024) {
					Com_Printf (PRNT_DEV, "CL_Say_Preprocessor: location expansion aborted, not enough space\n");
					break;
				}

				memmove (sayText + locLen, sayText + 2, cmdLen - (sayText - p) - 1);
				memcpy (sayText, locName, locLen);
				sayText += locLen;
			}
			sayText++;
		}
	}

	Cmd_ForwardToServer ();
}

/*
==============================================================

	CONSOLE COMMANDS

==============================================================
*/

/*
====================
CL_AddLoc_f
====================
*/
void CL_AddLoc_f (void) {
	FILE	*f;
	char	path[MAX_QPATH];

	if (Cmd_Argc () < 2) {
		Com_Printf (0, "syntax: addloc <message>\n");
		return;
	}

	if (!locFileName) {
		Com_Printf (0, "CL_AddLoc_f: No map loaded!\n");
		return;
	}

	Q_snprintfz (path, sizeof (path), "%s/%s", FS_Gamedir (), locFileName);

	f = fopen (path, "ab");
	if (!f) {
		Com_Printf (0, S_COLOR_RED "ERROR: Couldn't write %s\n", path);
		return;
	}

	fprintf (f, "%i %i %i %s\n",
		(int)(cl.refDef.viewOrigin[0]*8),
		(int)(cl.refDef.viewOrigin[1]*8),
		(int)(cl.refDef.viewOrigin[2]*8),
		Cmd_Args ());

	fclose (f);

	// tell them
	Com_Printf (0, "Saved location (x%i y%i z%i): \"%s\"\n",
		(int)cl.refDef.viewOrigin[0],
		(int)cl.refDef.viewOrigin[1],
		(int)cl.refDef.viewOrigin[2],
		Cmd_Args ());

	CL_LoadLoc (locFileName);
}

/*
==============================================================

	INIT / SHUTDOWN

==============================================================
*/

/*
====================
CL_Loc_Init
====================
*/
void CL_Loc_Init (void) {
	// clear locations
	memset (&clLocations, 0, sizeof (clLocations));

	// add console commands
	Cmd_AddCommand ("addloc",		CL_AddLoc_f,		"");

	// clear the loc filename
	locFileName[0] = '\0';
}


/*
====================
CL_Loc_Shutdown
====================
*/
void CL_Loc_Shutdown (void) {
	// free locations
	CL_FreeLoc ();

	// remove console commands
	Cmd_RemoveCommand ("addloc");
}
