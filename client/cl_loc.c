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

static clLocation_t	*cl_locationList;
static char			cl_locFileName[MAX_QPATH];

/*
=============================================================================

	LOADING

=============================================================================
*/

/*
====================
CL_LoadLoc

Called on map change or location addition to reload the file
====================
*/
void CL_LoadLoc (char *mapName)
{
	clLocation_t	*loc;
	vec3_t			location;
	qBool			finished;
	char			*fileBuffer, *buf;
	char			*token;
	int				fileLen;

	CL_FreeLoc ();

	if (!mapName || !mapName[0])
		return;

	Com_StripExtension (cl_locFileName, sizeof (cl_locFileName), mapName);
	Q_snprintfz (cl_locFileName, sizeof (cl_locFileName), "%s.loc", cl_locFileName);

	fileLen = FS_LoadFile (cl_locFileName, (void **)&fileBuffer);
	if (!fileBuffer || (fileLen <= 0)) {
		Com_Printf (PRNT_DEV, "CL_LoadLoc: %s not found\n", cl_locFileName);
		return;
	}

	buf = (char *)Mem_PoolAlloc (fileLen+1, MEMPOOL_LOCSYS, 0);
	memcpy (buf, fileBuffer, fileLen);

	FS_FreeFile (fileBuffer);

	token = strtok (buf, "\t ");
	while (token) {
		finished = qFalse;

		// X coordinate
		location[0] = (float)atoi (token)*0.125f;

		// Y coordinate
		token = strtok (NULL, "\t ");
		if (!token)
			break;
		location[1] = (float)atoi (token)*0.125f;

		// Z coordinate
		token = strtok (NULL, "\t ");
		if (!token)
			break;
		location[2] = (float)atoi (token)*0.125f;

		// Location
		token = strtok (NULL, "\n\r");
		if (!token)
			break;

		// Allocate
		loc = Mem_PoolAlloc (sizeof (clLocation_t), MEMPOOL_LOCSYS, 0);
		loc->name = Mem_StrDup (token);
		VectorCopy (location, loc->location);

		// Link it in
		loc->next = cl_locationList;
		cl_locationList = loc;

		Com_Printf (PRNT_DEV, "CL_AddLoc: adding location '%s'\n", loc->name);

		// Go to the next x coord
		token = strtok (NULL, "\n\r\t ");
		if (!token)
			finished = qTrue;
	}

	if (!finished) {
		Com_Printf (0, S_COLOR_RED "CL_LoadLoc: Bad loc file '%s'\n", cl_locFileName);
		CL_FreeLoc ();
	}

	Mem_Free (buf);
	Mem_CheckPoolIntegrity (MEMPOOL_LOCSYS);
}


/*
====================
CL_FreeLoc

Called on map change, full shutdown, and to release a bad loc file
====================
*/
void CL_FreeLoc (void)
{
	if (cl_locationList) {
		cl_locationList->next = NULL;
		cl_locationList = NULL;
	}

	Mem_FreePool (MEMPOOL_LOCSYS);
}

/*
=============================================================================

	MESSAGE PREPROCESSING

=============================================================================
*/

/*
====================
CL_Loc_Get

Grabs the location for the message preprocessing
====================
*/
static char *CL_Loc_Get (void)
{
	clLocation_t	*loc, *best;
	uInt			length, bestLength = 0xFFFFFFFF;

	best = NULL;
	for (loc=cl_locationList ; loc ; loc=loc->next) {
		length = VectorDist (loc->location, cl.refDef.viewOrigin);
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
void CL_Say_Preprocessor (void)
{
	char	*locName, *p;
	char	*sayText;
	int		locLen, cmdLen;

	if (!cl_locationList) {
		Cmd_ForwardToServer ();
		return;
	}

	sayText = p = Cmd_Args();

	while (*sayText && *(sayText + 1)) {
		if ((*sayText == '@') && (toupper(*(sayText + 1)) == 'L')) {
			locName = CL_Loc_Get ();
			if (!locName)
				break;

			cmdLen = strlen (Cmd_Args ());
			locLen = strlen (locName);

			if (cmdLen + locLen >= MAX_STRING_CHARS) {
				Com_Printf (PRNT_DEV, "CL_Say_Preprocessor: location expansion aborted, not enough space\n");
				break;
			}

			memmove (sayText + locLen, sayText + 2, cmdLen - (sayText - p) - 1);
			memcpy (sayText, locName, locLen);
			sayText += locLen - 1;
		}
		sayText++;
	}

	Cmd_ForwardToServer ();
}

/*
=============================================================================

	CONSOLE COMMANDS

=============================================================================
*/

/*
====================
CL_AddLoc_f
====================
*/
void CL_AddLoc_f (void)
{
	FILE	*f;
	char	path[MAX_QPATH];

	if (Cmd_Argc () < 2) {
		Com_Printf (0, "syntax: addloc <message>\n");
		return;
	}

	if (!cl_locFileName) {
		Com_Printf (0, "CL_AddLoc_f: No map loaded!\n");
		return;
	}

	Q_snprintfz (path, sizeof (path), "%s/%s", FS_Gamedir (), cl_locFileName);

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

	// Tell them
	Com_Printf (0, "Saved location (x%i y%i z%i): \"%s\"\n",
		(int)cl.refDef.viewOrigin[0],
		(int)cl.refDef.viewOrigin[1],
		(int)cl.refDef.viewOrigin[2],
		Cmd_Args ());

	CL_LoadLoc (cl_locFileName);
}

/*
=============================================================================

	INIT / SHUTDOWN

=============================================================================
*/

static cmdFunc_t	*cmd_addLoc;

/*
====================
CL_Loc_Init
====================
*/
void CL_Loc_Init (void)
{
	// Clear locations
	memset (&cl_locationList, 0, sizeof (cl_locationList));

	// Add console commands
	cmd_addLoc	= Cmd_AddCommand ("addloc",		CL_AddLoc_f,		"");

	// Clear the loc filename
	cl_locFileName[0] = '\0';
}


/*
====================
CL_Loc_Shutdown
====================
*/
void CL_Loc_Shutdown (void)
{
	// Free locations
	CL_FreeLoc ();

	// Remove console commands
	Cmd_RemoveCommand ("addloc", cmd_addLoc);
}
