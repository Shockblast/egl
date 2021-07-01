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
// sv_ccmds.c
//

#include "sv_local.h"

/*
===============================================================================

	OPERATOR CONSOLE ONLY COMMANDS

	These cmds can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

/*
====================
SV_SetMaster_f

Specify a list of master servers
====================
*/
static void SV_SetMaster_f (void) {
	int		i, slot;

	// only dedicated servers send heartbeats
	if (!dedicated->integer) {
		Com_Printf (0, S_COLOR_YELLOW "Only dedicated servers use masters.\n");
		return;
	}

	// make sure the server is listed public
	Cvar_Set ("public", "1");

	for (i=1 ; i<MAX_MASTERS ; i++)
		memset (&svMasterAddr[i], 0, sizeof (svMasterAddr[i]));

	slot = 1;		// slot 0 will always contain the id master
	for (i=1 ; i<Cmd_Argc () ; i++) {
		if (slot == MAX_MASTERS)
			break;

		if (!NET_StringToAdr (Cmd_Argv (i), &svMasterAddr[i])) {
			Com_Printf (0, S_COLOR_YELLOW "Bad address: %s\n", Cmd_Argv (i));
			continue;
		}
		if (svMasterAddr[slot].port == 0)
			svMasterAddr[slot].port = BigShort (PORT_MASTER);

		Com_Printf (0, "Master server at %s\n", NET_AdrToString (svMasterAddr[slot]));
		Com_Printf (0, "Sending a ping.\n");

		Netchan_OutOfBandPrint (NS_SERVER, svMasterAddr[slot], "ping");

		slot++;
	}

	svs.lastHeartBeat = -9999999;
}


/*
==================
SV_SetPlayer

Sets svCurrClient and svCurrPlayer to the player with idnum Cmd_Argv (1)
==================
*/
static qBool SV_SetPlayer (void) {
	svClient_t	*cl;
	int			i;
	int			idnum;
	char		*s;

	if (Cmd_Argc () < 2)
		return qFalse;

	s = Cmd_Argv (1);

	// numeric values are just slot numbers
	if ((s[0] >= '0') && (s[0] <= '9')) {
		idnum = atoi (Cmd_Argv (1));
		if ((idnum < 0) || (idnum >= maxclients->integer)) {
			Com_Printf (0, S_COLOR_YELLOW "Bad client slot: %i\n", idnum);
			return qFalse;
		}

		svCurrClient = &svs.clients[idnum];
		svCurrPlayer = svCurrClient->edict;
		if (!svCurrClient->state) {
			Com_Printf (0, "Client %i is not active\n", idnum);
			return qFalse;
		}
		return qTrue;
	}

	// check for a name match
	for (i=0, cl=svs.clients ; i<maxclients->integer ; i++, cl++) {
		if (!cl->state)
			continue;

		if (!Q_stricmp (cl->name, s)) {
			svCurrClient = cl;
			svCurrPlayer = svCurrClient->edict;
			return qTrue;
		}
	}

	Com_Printf (0, S_COLOR_YELLOW "Userid %s is not on the server\n", s);
	return qFalse;
}

/*
===============================================================================

	SAVEGAME FILES

===============================================================================
*/

/*
=====================
SV_WipeSavegame

Delete save/<XXX>/
=====================
*/
static void SV_WipeSavegame (char *saveName) {
	char	name[MAX_OSPATH];
	char	*s;

	Com_Printf (PRNT_DEV, "SV_WipeSaveGame (%s)\n", saveName);

	Q_snprintfz (name, sizeof (name), "%s/save/%s/server.ssv", FS_Gamedir (), saveName);
	remove (name);
	Q_snprintfz (name, sizeof (name), "%s/save/%s/game.ssv", FS_Gamedir (), saveName);
	remove (name);

	Q_snprintfz (name, sizeof (name), "%s/save/%s/*.sav", FS_Gamedir (), saveName);
	s = Sys_FindFirst (name, 0, 0);
	while (s) {
		remove (s);
		s = Sys_FindNext (0, 0);
	}
	Sys_FindClose ();

	Q_snprintfz (name, sizeof (name), "%s/save/%s/*.sv2", FS_Gamedir (), saveName);
	s = Sys_FindFirst (name, 0, 0);
	while (s) {
		remove (s);
		s = Sys_FindNext (0, 0);
	}
	Sys_FindClose ();
}


/*
================
SV_CopySaveGame
================
*/
static void SV_CopySaveGame (char *src, char *dst) {
	char	name[MAX_OSPATH], name2[MAX_OSPATH];
	int		l, len;
	char	*found;

	Com_Printf (PRNT_DEV, "SV_CopySaveGame (%s, %s)\n", src, dst);

	SV_WipeSavegame (dst);

	// copy the savegame over
	Q_snprintfz (name, sizeof (name), "%s/save/%s/server.ssv", FS_Gamedir(), src);
	Q_snprintfz (name2, sizeof (name2), "%s/save/%s/server.ssv", FS_Gamedir(), dst);
	FS_CreatePath (name2);
	FS_CopyFile (name, name2);

	Q_snprintfz (name, sizeof (name), "%s/save/%s/game.ssv", FS_Gamedir(), src);
	Q_snprintfz (name2, sizeof (name2), "%s/save/%s/game.ssv", FS_Gamedir(), dst);
	FS_CopyFile (name, name2);

	Q_snprintfz (name, sizeof (name), "%s/save/%s/", FS_Gamedir(), src);
	len = strlen(name);
	Q_snprintfz (name, sizeof (name), "%s/save/%s/*.sav", FS_Gamedir(), src);
	found = Sys_FindFirst(name, 0, 0);
	while (found) {
		strcpy (name+len, found+len);

		Q_snprintfz (name2, sizeof (name2), "%s/save/%s/%s", FS_Gamedir(), dst, found+len);
		FS_CopyFile (name, name2);

		// change sav to sv2
		l = strlen(name);
		strcpy (name+l-3, "sv2");
		l = strlen(name2);
		strcpy (name2+l-3, "sv2");
		FS_CopyFile (name, name2);

		found = Sys_FindNext (0, 0);
	}
	Sys_FindClose ();
}


/*
==============
SV_WriteLevelFile
==============
*/
static void SV_WriteLevelFile (void) {
	char	name[MAX_OSPATH];
	FILE	*f;

	Com_Printf (PRNT_DEV, "SV_WriteLevelFile ()\n");

	Q_snprintfz (name, sizeof (name), "%s/save/current/%s.sv2", FS_Gamedir(), sv.name);
	f = fopen(name, "wb");
	if (!f) {
		Com_Printf (0, S_COLOR_YELLOW "Failed to open %s\n", name);
		return;
	}
	fwrite (sv.configStrings, sizeof (sv.configStrings), 1, f);
	CM_WritePortalState (f);
	fclose (f);

	Q_snprintfz (name, sizeof (name), "%s/save/current/%s.sav", FS_Gamedir(), sv.name);
	ge->WriteLevel (name);
}


/*
==============
SV_ReadLevelFile
==============
*/
void SV_ReadLevelFile (void) {
	char	name[MAX_OSPATH];
	FILE	*f;

	Com_Printf (PRNT_DEV, "SV_ReadLevelFile ()\n");

	Q_snprintfz (name, sizeof (name), "%s/save/current/%s.sv2", FS_Gamedir(), sv.name);
	f = fopen(name, "rb");
	if (!f) {
		Com_Printf (0, S_COLOR_YELLOW "Failed to open %s\n", name);
		return;
	}
	FS_Read (sv.configStrings, sizeof (sv.configStrings), f);
	CM_ReadPortalState (f);
	fclose (f);

	Q_snprintfz (name, sizeof (name), "%s/save/current/%s.sav", FS_Gamedir(), sv.name);
	ge->ReadLevel (name);
}


/*
==============
SV_WriteServerFile
==============
*/
static void SV_WriteServerFile (qBool autoSave) {
	FILE	*f;
	cVar_t	*var;
	char	name[MAX_OSPATH], string[128];
	char	comment[32];
	time_t	aclock;
	struct tm	*newtime;

	Com_Printf (PRNT_DEV, "SV_WriteServerFile (%s)\n", autoSave ? "true" : "false");

	Q_snprintfz (name, sizeof (name), "%s/save/current/server.ssv", FS_Gamedir());
	f = fopen (name, "wb");
	if (!f) {
		Com_Printf (0, S_COLOR_YELLOW "Couldn't write %s\n", name);
		return;
	}

	// write the comment field
	memset (comment, 0, sizeof (comment));

	if (!autoSave) {
		time (&aclock);
		newtime = localtime (&aclock);
		Q_snprintfz (comment, sizeof (comment), "%2i:%i%i %2i/%2i  ",
					newtime->tm_hour, newtime->tm_min/10, newtime->tm_min%10, newtime->tm_mon+1, newtime->tm_mday);
		strncat (comment, sv.configStrings[CS_NAME], sizeof (comment)-1-strlen(comment));
	} else {
		// autosaved
		Q_snprintfz (comment, sizeof (comment), "ENTERING %s", sv.configStrings[CS_NAME]);
	}

	fwrite (comment, 1, sizeof (comment), f);

	// write the mapcmd
	fwrite (svs.mapCmd, 1, sizeof (svs.mapCmd), f);

	// write all CVAR_LATCH_SERVER cvars
	for (var=cvarVariables ; var ; var=var->next) {
		if (!(var->flags & CVAR_LATCH_SERVER))
			continue;

		if ((strlen(var->name) >= sizeof (name)-1) || (strlen(var->string) >= sizeof (string)-1)) {
			Com_Printf (0, S_COLOR_YELLOW "Cvar too long: %s = %s\n", var->name, var->string);
			continue;
		}
		memset (name, 0, sizeof (name));
		memset (string, 0, sizeof (string));
		strcpy (name, var->name);
		strcpy (string, var->string);
		fwrite (name, 1, sizeof (name), f);
		fwrite (string, 1, sizeof (string), f);
	}

	fclose (f);

	// write game state
	Q_snprintfz (name, sizeof (name), "%s/save/current/game.ssv", FS_Gamedir());
	ge->WriteGame (name, autoSave);
}


/*
==============
SV_ReadServerFile
==============
*/
static void SV_ReadServerFile (void) {
	FILE	*f;
	char	name[MAX_OSPATH], string[128];
	char	comment[32];
	char	mapcmd[MAX_TOKEN_CHARS];

	Com_Printf (PRNT_DEV, "SV_ReadServerFile()\n");

	Q_snprintfz (name, sizeof (name), "%s/save/current/server.ssv", FS_Gamedir());
	f = fopen (name, "rb");
	if (!f) {
		Com_Printf (0, S_COLOR_YELLOW "Couldn't read %s\n", name);
		return;
	}

	// read the comment field
	FS_Read (comment, sizeof (comment), f);

	// read the mapcmd
	FS_Read (mapcmd, sizeof (mapcmd), f);

	// read all CVAR_LATCH_SERVER cvars
	for ( ; ; ) {
		if (!fread (name, 1, sizeof (name), f))
			break;
		FS_Read (string, sizeof (string), f);
		Com_Printf (PRNT_DEV, "Set %s = %s\n", name, string);
		Cvar_ForceSet (name, string);
	}

	fclose (f);

	// start a new game fresh with new cvars
	SV_InitGame ();

	strcpy (svs.mapCmd, mapcmd);

	// read game state
	Q_snprintfz (name, sizeof (name), "%s/save/current/game.ssv", FS_Gamedir());
	ge->ReadGame (name);
}


//=========================================================

/*
==================
SV_DemoMap_f

Puts the server in demo mode on a specific map/cinematic
==================
*/
static void SV_DemoMap_f (void) {
	SV_Map (qTrue, Cmd_Argv (1), qFalse);
}


/*
==================
SV_GameMap_f

Saves the state of the map just being exited and goes to a new map.

If the initial character of the map string is '*', the next map is
in a new unit, so the current savegame directory is cleared of
map files.

Example:

*inter.cin+jail

Clears the archived maps, plays the inter.cin cinematic, then
goes to map jail.bsp.
==================
*/
static void SV_GameMap_f (void) {
	char		*map;
	int			i;
	svClient_t	*cl;
	qBool		*savedInuse;

	if (Cmd_Argc () != 2) {
		Com_Printf (0, "USAGE: gamemap <map>\n");
		return;
	}

	Com_Printf (PRNT_DEV, "SV_GameMap(%s)\n", Cmd_Argv (1));

	FS_CreatePath (Q_VarArgs ("%s/save/current/", FS_Gamedir()));

	// check for clearing the current savegame
	map = Cmd_Argv (1);
	if (map[0] == '*') {
		// wipe all the *.sav files
		SV_WipeSavegame ("current");
	} else {
		// save the map just exited
		if (sv.state == SS_GAME) {
			/*
			** clear all the client inUse flags before saving so that
			** when the level is re-entered, the clients will spawn
			** at spawn points instead of occupying body shells
			*/
			savedInuse = Mem_Alloc (maxclients->integer * sizeof (qBool));
			for (i=0, cl=svs.clients ; i<maxclients->integer ; i++, cl++) {
				savedInuse[i] = cl->edict->inUse;
				cl->edict->inUse = qFalse;
			}

			SV_WriteLevelFile ();

			// we must restore these for clients to transfer over correctly
			for (i=0, cl=svs.clients ; i<maxclients->integer ; i++, cl++)
				cl->edict->inUse = savedInuse[i];
			Mem_Free (savedInuse);
		}
	}

	// start up the next map
	SV_Map (qFalse, Cmd_Argv (1), qFalse);

	// archive server state
	strncpy (svs.mapCmd, Cmd_Argv (1), sizeof (svs.mapCmd)-1);

	// copy off the level to the autosave slot
	if (!dedicated->integer) {
		SV_WriteServerFile (qTrue);
		SV_CopySaveGame ("current", "save0");
	}
}


/*
==================
SV_Map_f

Goes directly to a given map without any savegame archiving.
For development work
==================
*/
static void SV_Map_f (void) {
	char	*map;
	char	expanded[MAX_QPATH];

	// if not a pcx, demo, or cinematic, check to make sure the level exists
	map = Cmd_Argv (1);
	if (!strstr (map, ".")) {
		Q_snprintfz (expanded, sizeof (expanded), "maps/%s.bsp", map);
		if (FS_LoadFile (expanded, NULL) == -1) {
			Com_Printf (0, "Can't find %s\n", expanded);
			return;
		}
	}

	sv.state = SS_DEAD;	// don't save current level when changing
	SV_WipeSavegame ("current");
	SV_GameMap_f ();
}

/*
=====================================================================

  SAVEGAMES

=====================================================================
*/

/*
==============
SV_Loadgame_f
==============
*/
static void SV_Loadgame_f (void) {
	char	name[MAX_OSPATH];
	FILE	*f;
	char	*dir;

	if (Cmd_Argc () != 2) {
		Com_Printf (0, "USAGE: loadgame <directory>\n");
		return;
	}

	Com_Printf (0, "Loading game...\n");

	dir = Cmd_Argv (1);
	if (strstr (dir, "..") || strstr (dir, "/") || strstr (dir, "\\"))
		Com_Printf (0, S_COLOR_YELLOW "Bad savedir.\n");

	// make sure the server.ssv file exists
	Q_snprintfz (name, sizeof (name), "%s/save/%s/server.ssv", FS_Gamedir(), Cmd_Argv (1));
	f = fopen (name, "rb");
	if (!f) {
		Com_Printf (0, "No such savegame: %s\n", name);
		return;
	}
	fclose (f);

	SV_CopySaveGame (Cmd_Argv (1), "current");

	SV_ReadServerFile ();

	// go to the map
	sv.state = SS_DEAD;		// don't save current level when changing
	SV_Map (qFalse, svs.mapCmd, qTrue);
}


/*
==============
SV_Savegame_f
==============
*/
static void SV_Savegame_f (void) {
	char	*dir;

	if (sv.state != SS_GAME) {
		Com_Printf (0, "You must be in a game to save.\n");
		return;
	}

	if (Cmd_Argc () != 2) {
		Com_Printf (0, "USAGE: savegame <directory>\n");
		return;
	}

	if (Cvar_VariableValue("deathmatch")) {
		Com_Printf (0, "Can't savegame in a deathmatch\n");
		return;
	}

	if (!strcmp (Cmd_Argv (1), "current")) {
		Com_Printf (0, "Can't save to 'current'\n");
		return;
	}

	if ((maxclients->integer == 1) && (svs.clients[0].edict->client->ps.stats[STAT_HEALTH] <= 0)) {
		Com_Printf (0, "\nCan't savegame while dead!\n");
		return;
	}

	dir = Cmd_Argv (1);
	if (strstr (dir, "..") || strstr (dir, "/") || strstr (dir, "\\"))
		Com_Printf (0, S_COLOR_YELLOW "Bad savedir.\n");

	Com_Printf (0, "Saving game...\n");

	/*
	** archive current level, including all client edicts. when the level is reloaded,
	** they will be shells awaiting a connecting client
	*/
	SV_WriteLevelFile ();

	// save server state
	SV_WriteServerFile (qFalse);

	// copy it off
	SV_CopySaveGame ("current", dir);

	Com_Printf (0, "Done.\n");
}

//===============================================================

/*
==================
SV_Kick_f

Kick a user off of the server
==================
*/
static void SV_Kick_f (void) {
	if (!svs.initialized) {
		Com_Printf (0, "No server running.\n");
		return;
	}

	if (Cmd_Argc () != 2) {
		Com_Printf (0, "Usage: kick <userid>\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	SV_BroadcastPrintf (PRINT_HIGH, "%s was kicked\n", svCurrClient->name);
	// print directly, because the dropped client won't get the SV_BroadcastPrintf message
	SV_ClientPrintf (svCurrClient, PRINT_HIGH, "You were kicked from the game\n");
	SV_DropClient (svCurrClient);
	svCurrClient->lastMessage = svs.realTime;	// min case there is a funny zombie
}


/*
================
SV_Status_f
================
*/
static void SV_Status_f (void) {
	int			i, j, l;
	svClient_t	*cl;
	char		*s;
	int			ping;
	if (!svs.clients) {
		Com_Printf (0, "No server running.\n");
		return;
	}
	Com_Printf (0, "map              : %s\n", sv.name);

	Com_Printf (0, "num score ping name            lastmsg address               qPort \n");
	Com_Printf (0, "--- ----- ---- --------------- ------- --------------------- ------\n");
	for (i=0, cl=svs.clients ; i<maxclients->integer ; i++, cl++) {
		if (!cl->state)
			continue;
		Com_Printf (0, "%3i ", i);
		Com_Printf (0, "%5i ", cl->edict->client->ps.stats[STAT_FRAGS]);

		if (cl->state == SVCS_CONNECTED)
			Com_Printf (0, "CNCT ");
		else if (cl->state == SVCS_FREE)
			Com_Printf (0, "ZMBI ");
		else {
			ping = cl->ping < 9999 ? cl->ping : 9999;
			Com_Printf (0, "%4i ", ping);
		}

		Com_Printf (0, "%s", cl->name);
		l = 16 - strlen(cl->name);
		for (j=0 ; j<l ; j++)
			Com_Printf (0, " ");

		Com_Printf (0, "%7i ", svs.realTime - cl->lastMessage);

		s = NET_AdrToString  (cl->netChan.remoteAddress);
		Com_Printf (0, "%s", s);
		l = 22 - strlen(s);
		for (j=0 ; j<l ; j++)
			Com_Printf (0, " ");
		
		Com_Printf (0, "%5i", cl->netChan.qPort);

		Com_Printf (0, "\n");
	}
	Com_Printf (0, "\n");
}


/*
==================
SV_ConSay_f
==================
*/
static void SV_ConSay_f (void) {
	svClient_t *client;
	int			j;
	char		*p, text[1024];

	if (Cmd_Argc () < 2)
		return;

	strcpy (text, "console: ");
	p = Cmd_Args();

	if (*p == '"') {
		p++;
		p[strlen(p)-1] = 0;
	}

	strcat (text, p);

	for (j=0, client=svs.clients; j<maxclients->integer ; j++, client++) {
		if (client->state != SVCS_SPAWNED)
			continue;
		SV_ClientPrintf (client, PRINT_CHAT, "%s\n", text);
	}
}


/*
==================
SV_Heartbeat_f
==================
*/
static void SV_Heartbeat_f (void) {
	svs.lastHeartBeat = -9999999;
}


/*
===========
SV_Serverinfo_f

Examine or change the serverinfo string
===========
*/
static void SV_Serverinfo_f (void) {
	Com_Printf (0, "Server info settings:\n");
	Info_Print (Cvar_Serverinfo ());
}


/*
===========
SV_DumpUser_f

Examine all a users info strings
===========
*/
static void SV_DumpUser_f (void) {
	if (Cmd_Argc () != 2) {
		Com_Printf (0, "Usage: dumpuser <userid>\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	Com_Printf (0, "userinfo\n");
	Com_Printf (0, "--------\n");
	Info_Print (svCurrClient->userInfo);

}


/*
==============
SV_ServerRecord_f

Begins server demo recording.  Every entity and every message will be
recorded, but no playerinfo will be stored.  Primarily for demo merging.
==============
*/
static void SV_ServerRecord_f (void) {
	char	name[MAX_OSPATH];
	char	buf_data[32768];
	netMsg_t	buf;
	int		len;
	int		i;

	if (Cmd_Argc () != 2) {
		Com_Printf (0, "serverrecord <demoname>\n");
		return;
	}

	if (svs.demoFile) {
		Com_Printf (0, "Already recording.\n");
		return;
	}

	if (sv.state != SS_GAME) {
		Com_Printf (0, "You must be in a level to record.\n");
		return;
	}

	// open the demo file
	Q_snprintfz (name, sizeof (name), "%s/demos/%s.dm2", FS_Gamedir(), Cmd_Argv (1));

	Com_Printf (0, "recording to %s.\n", name);
	FS_CreatePath (name);
	svs.demoFile = fopen (name, "wb");
	if (!svs.demoFile) {
		Com_Printf (0, S_COLOR_RED "ERROR: couldn't open.\n");
		return;
	}

	// setup a buffer to catch all multicasts
	MSG_Init (&svs.demoMultiCast, svs.demoMultiCastBuf, sizeof (svs.demoMultiCastBuf));

	// write a single giant fake message with all the startup info
	MSG_Init (&buf, buf_data, sizeof (buf_data));

	/*
	** serverdata needs to go over for all types of servers
	** to make sure the protocol is right, and to set the gamedir
	*/

	// send the serverdata
	MSG_WriteByte (&buf, SVC_SERVERDATA);
	MSG_WriteLong (&buf, PROTOCOL_VERSION);
	MSG_WriteLong (&buf, svs.spawnCount);

	// 2 means server demo
	MSG_WriteByte (&buf, 2);	// demos are always attract loops
	MSG_WriteString (&buf, Cvar_VariableString ("gamedir"));
	MSG_WriteShort (&buf, -1);

	// send full levelname
	MSG_WriteString (&buf, sv.configStrings[CS_NAME]);

	for (i=0 ; i<MAX_CONFIGSTRINGS ; i++) {
		if (sv.configStrings[i][0]) {
			MSG_WriteByte (&buf, SVC_CONFIGSTRING);
			MSG_WriteShort (&buf, i);
			MSG_WriteString (&buf, sv.configStrings[i]);
		}
	}

	// write it to the demo file
	Com_Printf (PRNT_DEV, "signon message length: %i\n", buf.curSize);
	len = LittleLong (buf.curSize);
	fwrite (&len, 4, 1, svs.demoFile);
	fwrite (buf.data, buf.curSize, 1, svs.demoFile);

	// the rest of the demo file will be individual frames
}


/*
==============
SV_ServerStop_f

Ends server demo recording
==============
*/
static void SV_ServerStop_f (void) {
	if (!svs.demoFile) {
		Com_Printf (0, "Not doing a serverrecord.\n");
		return;
	}
	fclose (svs.demoFile);
	svs.demoFile = NULL;
	Com_Printf (0, "Recording completed.\n");
}


/*
===============
SV_KillServer_f

Kick everyone off, possibly in preparation for a new game
===============
*/
static void SV_KillServer_f (void) {
	if (!svs.initialized)
		return;

	SV_Shutdown ("Server was killed.\n", qFalse);
	NET_Config (qFalse);	// close network sockets
}


/*
===============
SV_ServerCommand_f

Let the game dll handle a command
===============
*/
static void SV_ServerCommand_f (void) {
	if (!ge) {
		Com_Printf (0, "No game loaded.\n");
		return;
	}

	ge->ServerCommand();
}


/*
==================
SV_InitOperatorCommands
==================
*/
void SV_InitOperatorCommands (void) {
	Cmd_AddCommand ("heartbeat",	SV_Heartbeat_f,		"");
	Cmd_AddCommand ("kick",			SV_Kick_f,			"");
	Cmd_AddCommand ("status",		SV_Status_f,		"");
	Cmd_AddCommand ("serverinfo",	SV_Serverinfo_f,	"");
	Cmd_AddCommand ("dumpuser",		SV_DumpUser_f,		"");

	Cmd_AddCommand ("map",			SV_Map_f,			"");
	Cmd_AddCommand ("demomap",		SV_DemoMap_f,		"");
	Cmd_AddCommand ("gamemap",		SV_GameMap_f,		"");
	Cmd_AddCommand ("setmaster",	SV_SetMaster_f,		"");

	if (dedicated->integer)
		Cmd_AddCommand ("say",		SV_ConSay_f,		"");

	Cmd_AddCommand ("serverrecord",	SV_ServerRecord_f,	"");
	Cmd_AddCommand ("serverstop",	SV_ServerStop_f,	"");

	Cmd_AddCommand ("save",			SV_Savegame_f,		"");
	Cmd_AddCommand ("load",			SV_Loadgame_f,		"");

	Cmd_AddCommand ("killserver",	SV_KillServer_f,	"");

	Cmd_AddCommand ("sv",			SV_ServerCommand_f,	"");
}
