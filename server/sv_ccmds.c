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

#include "sv_local.h"

/*
===============================================================================

	OPERATOR CONSOLE ONLY COMMANDS
These commands can only be entered from stdin or by a remote operator datagram

===============================================================================
*/

/*
====================
SV_SetMaster_f

Specify a list of master servers
====================
*/
void SV_SetMaster_f (void) {
	int		i, slot;

	/* only dedicated servers send heartbeats */
	if (!dedicated->integer) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Only dedicated servers use masters.\n");
		return;
	}

	/* make sure the server is listed public */
	Cvar_Set ("public", "1");

	for (i=1 ; i<MAX_MASTERS ; i++)
		memset (&master_adr[i], 0, sizeof (master_adr[i]));

	slot = 1;		// slot 0 will always contain the id master
	for (i=1 ; i<Cmd_Argc () ; i++) {
		if (slot == MAX_MASTERS)
			break;

		if (!NET_StringToAdr (Cmd_Argv (i), &master_adr[i])) {
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Bad address: %s\n", Cmd_Argv (i));
			continue;
		}
		if (master_adr[slot].port == 0)
			master_adr[slot].port = BigShort (PORT_MASTER);

		Com_Printf (PRINT_ALL, "Master server at %s\n", NET_AdrToString (master_adr[slot]));
		Com_Printf (PRINT_ALL, "Sending a ping.\n");

		Netchan_OutOfBandPrint (NS_SERVER, master_adr[slot], "ping");

		slot++;
	}

	svs.last_HeartBeat = -9999999;
}


/*
==================
SV_SetPlayer

Sets sv_client and sv_player to the player with idnum Cmd_Argv (1)
==================
*/
qBool SV_SetPlayer (void) {
	client_t	*cl;
	int			i;
	int			idnum;
	char		*s;

	if (Cmd_Argc () < 2)
		return qFalse;

	s = Cmd_Argv (1);

	/* numeric values are just slot numbers */
	if ((s[0] >= '0') && (s[0] <= '9')) {
		idnum = atoi(Cmd_Argv (1));
		if ((idnum < 0) || (idnum >= maxclients->integer)) {
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Bad client slot: %i\n", idnum);
			return qFalse;
		}

		sv_client = &svs.clients[idnum];
		sv_player = sv_client->edict;
		if (!sv_client->state) {
			Com_Printf (PRINT_ALL, "Client %i is not active\n", idnum);
			return qFalse;
		}
		return qTrue;
	}

	/* check for a name match */
	for (i=0, cl=svs.clients ; i<maxclients->integer ; i++, cl++) {
		if (!cl->state)
			continue;

		if (!strcmp(cl->name, s)) {
			sv_client = cl;
			sv_player = sv_client->edict;
			return qTrue;
		}
	}

	Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Userid %s is not on the server\n", s);
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
void SV_WipeSavegame (char *savename) {
	char	name[MAX_OSPATH];
	char	*s;

	Com_Printf (PRINT_DEV, "SV_WipeSaveGame(%s)\n", savename);

	Com_sprintf (name, sizeof (name), "%s/save/%s/server.ssv", FS_Gamedir (), savename);
	remove (name);
	Com_sprintf (name, sizeof (name), "%s/save/%s/game.ssv", FS_Gamedir (), savename);
	remove (name);

	Com_sprintf (name, sizeof (name), "%s/save/%s/*.sav", FS_Gamedir (), savename);
	s = Sys_FindFirst (name, 0, 0);
	while (s) {
		remove (s);
		s = Sys_FindNext (0, 0);
	}
	Sys_FindClose ();

	Com_sprintf (name, sizeof (name), "%s/save/%s/*.sv2", FS_Gamedir (), savename);
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
void SV_CopySaveGame (char *src, char *dst) {
	char	name[MAX_OSPATH], name2[MAX_OSPATH];
	int		l, len;
	char	*found;

	Com_Printf (PRINT_DEV, "SV_CopySaveGame(%s, %s)\n", src, dst);

	SV_WipeSavegame (dst);

	/* copy the savegame over */
	Com_sprintf (name, sizeof (name), "%s/save/%s/server.ssv", FS_Gamedir(), src);
	Com_sprintf (name2, sizeof (name2), "%s/save/%s/server.ssv", FS_Gamedir(), dst);
	FS_CreatePath (name2);
	FS_CopyFile (name, name2);

	Com_sprintf (name, sizeof (name), "%s/save/%s/game.ssv", FS_Gamedir(), src);
	Com_sprintf (name2, sizeof (name2), "%s/save/%s/game.ssv", FS_Gamedir(), dst);
	FS_CopyFile (name, name2);

	Com_sprintf (name, sizeof (name), "%s/save/%s/", FS_Gamedir(), src);
	len = strlen(name);
	Com_sprintf (name, sizeof (name), "%s/save/%s/*.sav", FS_Gamedir(), src);
	found = Sys_FindFirst(name, 0, 0);
	while (found) {
		strcpy (name+len, found+len);

		Com_sprintf (name2, sizeof (name2), "%s/save/%s/%s", FS_Gamedir(), dst, found+len);
		FS_CopyFile (name, name2);

		/* change sav to sv2 */
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
void SV_WriteLevelFile (void) {
	char	name[MAX_OSPATH];
	FILE	*f;

	Com_Printf (PRINT_DEV, "SV_WriteLevelFile()\n");

	Com_sprintf (name, sizeof (name), "%s/save/current/%s.sv2", FS_Gamedir(), sv.name);
	f = fopen(name, "wb");
	if (!f) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Failed to open %s\n", name);
		return;
	}
	fwrite (sv.configStrings, sizeof (sv.configStrings), 1, f);
	CM_WritePortalState (f);
	fclose (f);

	Com_sprintf (name, sizeof (name), "%s/save/current/%s.sav", FS_Gamedir(), sv.name);
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

	Com_Printf (PRINT_DEV, "SV_ReadLevelFile()\n");

	Com_sprintf (name, sizeof (name), "%s/save/current/%s.sv2", FS_Gamedir(), sv.name);
	f = fopen(name, "rb");
	if (!f) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Failed to open %s\n", name);
		return;
	}
	FS_Read (sv.configStrings, sizeof (sv.configStrings), f);
	CM_ReadPortalState (f);
	fclose (f);

	Com_sprintf (name, sizeof (name), "%s/save/current/%s.sav", FS_Gamedir(), sv.name);
	ge->ReadLevel (name);
}


/*
==============
SV_WriteServerFile
==============
*/
void SV_WriteServerFile (qBool autosave) {
	FILE	*f;
	cvar_t	*var;
	char	name[MAX_OSPATH], string[128];
	char	comment[32];
	time_t	aclock;
	struct tm	*newtime;

	Com_Printf (PRINT_DEV, "SV_WriteServerFile(%s)\n", autosave ? "qTrue" : "qFalse");

	Com_sprintf (name, sizeof (name), "%s/save/current/server.ssv", FS_Gamedir());
	f = fopen (name, "wb");
	if (!f) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Couldn't write %s\n", name);
		return;
	}

	/* write the comment field */
	memset (comment, 0, sizeof (comment));

	if (!autosave) {
		time (&aclock);
		newtime = localtime (&aclock);
		Com_sprintf (comment, sizeof (comment), "%2i:%i%i %2i/%2i  ",
					newtime->tm_hour, newtime->tm_min/10, newtime->tm_min%10, newtime->tm_mon+1, newtime->tm_mday);
		strncat (comment, sv.configStrings[CS_NAME], sizeof (comment)-1-strlen(comment));
	} else {
		/* autosaved */
		Com_sprintf (comment, sizeof (comment), "ENTERING %s", sv.configStrings[CS_NAME]);
	}

	fwrite (comment, 1, sizeof (comment), f);

	/* write the mapcmd */
	fwrite (svs.mapCmd, 1, sizeof (svs.mapCmd), f);

	/* write all CVAR_LATCH cvars */
	for (var=cvar_Variables ; var ; var=var->next) {
		if (!(var->flags & CVAR_LATCH))
			continue;

		if ((strlen(var->name) >= sizeof (name)-1) || (strlen(var->string) >= sizeof (string)-1)) {
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Cvar too long: %s = %s\n", var->name, var->string);
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

	/* write game state */
	Com_sprintf (name, sizeof (name), "%s/save/current/game.ssv", FS_Gamedir());
	ge->WriteGame (name, autosave);
}


/*
==============
SV_ReadServerFile
==============
*/
void SV_ReadServerFile (void) {
	FILE	*f;
	char	name[MAX_OSPATH], string[128];
	char	comment[32];
	char	mapcmd[MAX_TOKEN_CHARS];

	Com_Printf (PRINT_DEV, "SV_ReadServerFile()\n");

	Com_sprintf (name, sizeof (name), "%s/save/current/server.ssv", FS_Gamedir());
	f = fopen (name, "rb");
	if (!f) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Couldn't read %s\n", name);
		return;
	}

	/* read the comment field */
	FS_Read (comment, sizeof (comment), f);

	/* read the mapcmd */
	FS_Read (mapcmd, sizeof (mapcmd), f);

	/* read all CVAR_LATCH cvars */
	for ( ; ; ) {
		if (!fread (name, 1, sizeof (name), f))
			break;
		FS_Read (string, sizeof (string), f);
		Com_Printf (PRINT_DEV, "Set %s = %s\n", name, string);
		Cvar_ForceSet (name, string);
	}

	fclose (f);

	/* start a new game fresh with new cvars */
	SV_InitGame ();

	strcpy (svs.mapCmd, mapcmd);

	/* read game state */
	Com_sprintf (name, sizeof (name), "%s/save/current/game.ssv", FS_Gamedir());
	ge->ReadGame (name);
}


//=========================================================

/*
==================
SV_DemoMap_f

Puts the server in demo mode on a specific map/cinematic
==================
*/
void SV_DemoMap_f (void) {
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
void SV_GameMap_f (void) {
	char		*map;
	int			i;
	client_t	*cl;
	qBool		*savedInuse;

	if (Cmd_Argc () != 2) {
		Com_Printf (PRINT_ALL, "USAGE: gamemap <map>\n");
		return;
	}

	Com_Printf (PRINT_DEV, "SV_GameMap(%s)\n", Cmd_Argv (1));

	FS_CreatePath (va("%s/save/current/", FS_Gamedir()));

	/* check for clearing the current savegame */
	map = Cmd_Argv (1);
	if (map[0] == '*') {
		/* wipe all the *.sav files */
		SV_WipeSavegame ("current");
	} else {
		/* save the map just exited */
		if (sv.state == SS_GAME) {
			/*
			** clear all the client inUse flags before saving so that
			** when the level is re-entered, the clients will spawn
			** at spawn points instead of occupying body shells
			*/
			savedInuse = malloc (maxclients->integer * sizeof (qBool));
			for (i=0, cl=svs.clients ; i<maxclients->integer ; i++, cl++) {
				savedInuse[i] = cl->edict->inUse;
				cl->edict->inUse = qFalse;
			}

			SV_WriteLevelFile ();

			/* we must restore these for clients to transfer over correctly */
			for (i=0, cl=svs.clients ; i<maxclients->integer ; i++, cl++)
				cl->edict->inUse = savedInuse[i];
			free (savedInuse);
		}
	}

	/* start up the next map */
	SV_Map (qFalse, Cmd_Argv (1), qFalse);

	/* archive server state */
	strncpy (svs.mapCmd, Cmd_Argv (1), sizeof (svs.mapCmd)-1);

	/* copy off the level to the autosave slot */
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
void SV_Map_f (void) {
	char	*map;
	char	expanded[MAX_QPATH];

	/* if not a pcx, demo, or cinematic, check to make sure the level exists */
	map = Cmd_Argv (1);
	if (!strstr (map, ".")) {
		Com_sprintf (expanded, sizeof (expanded), "maps/%s.bsp", map);
		if (FS_LoadFile (expanded, NULL) == -1) {
			Com_Printf (PRINT_ALL, "Can't find %s\n", expanded);
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
void SV_Loadgame_f (void) {
	char	name[MAX_OSPATH];
	FILE	*f;
	char	*dir;

	if (Cmd_Argc () != 2) {
		Com_Printf (PRINT_ALL, "USAGE: loadgame <directory>\n");
		return;
	}

	Com_Printf (PRINT_ALL, "Loading game...\n");

	dir = Cmd_Argv (1);
	if (strstr (dir, "..") || strstr (dir, "/") || strstr (dir, "\\"))
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Bad savedir.\n");

	/* make sure the server.ssv file exists */
	Com_sprintf (name, sizeof (name), "%s/save/%s/server.ssv", FS_Gamedir(), Cmd_Argv (1));
	f = fopen (name, "rb");
	if (!f) {
		Com_Printf (PRINT_ALL, "No such savegame: %s\n", name);
		return;
	}
	fclose (f);

	SV_CopySaveGame (Cmd_Argv (1), "current");

	SV_ReadServerFile ();

	/* go to the map */
	sv.state = SS_DEAD;		// don't save current level when changing
	SV_Map (qFalse, svs.mapCmd, qTrue);
}


/*
==============
SV_Savegame_f
==============
*/
void SV_Savegame_f (void) {
	char	*dir;

	if (sv.state != SS_GAME) {
		Com_Printf (PRINT_ALL, "You must be in a game to save.\n");
		return;
	}

	if (Cmd_Argc () != 2) {
		Com_Printf (PRINT_ALL, "USAGE: savegame <directory>\n");
		return;
	}

	if (Cvar_VariableValue("deathmatch")) {
		Com_Printf (PRINT_ALL, "Can't savegame in a deathmatch\n");
		return;
	}

	if (!strcmp (Cmd_Argv (1), "current")) {
		Com_Printf (PRINT_ALL, "Can't save to 'current'\n");
		return;
	}

	if ((maxclients->integer == 1) && (svs.clients[0].edict->client->ps.stats[STAT_HEALTH] <= 0)) {
		Com_Printf (PRINT_ALL, "\nCan't savegame while dead!\n");
		return;
	}

	dir = Cmd_Argv (1);
	if (strstr (dir, "..") || strstr (dir, "/") || strstr (dir, "\\"))
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Bad savedir.\n");

	Com_Printf (PRINT_ALL, "Saving game...\n");

	/*
	** archive current level, including all client edicts. when the level is reloaded,
	** they will be shells awaiting a connecting client
	*/
	SV_WriteLevelFile ();

	/* save server state */
	SV_WriteServerFile (qFalse);

	/* copy it off */
	SV_CopySaveGame ("current", dir);

	Com_Printf (PRINT_ALL, "Done.\n");
}

//===============================================================

/*
==================
SV_Kick_f

Kick a user off of the server
==================
*/
void SV_Kick_f (void) {
	if (!svs.initialized) {
		Com_Printf (PRINT_ALL, "No server running.\n");
		return;
	}

	if (Cmd_Argc () != 2) {
		Com_Printf (PRINT_ALL, "Usage: kick <userid>\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	SV_BroadcastPrintf (PRINT_HIGH, "%s was kicked\n", sv_client->name);
	/* print directly, because the dropped client won't get the SV_BroadcastPrintf message */
	SV_ClientPrintf (sv_client, PRINT_HIGH, "You were kicked from the game\n");
	SV_DropClient (sv_client);
	sv_client->lastMessage = svs.realTime;	// min case there is a funny zombie
}


/*
================
SV_Status_f
================
*/
void SV_Status_f (void) {
	int			i, j, l;
	client_t	*cl;
	char		*s;
	int			ping;
	if (!svs.clients) {
		Com_Printf (PRINT_ALL, "No server running.\n");
		return;
	}
	Com_Printf (PRINT_ALL, "map              : %s\n", sv.name);

	Com_Printf (PRINT_ALL, "num score ping name            lastmsg address               qPort \n");
	Com_Printf (PRINT_ALL, "--- ----- ---- --------------- ------- --------------------- ------\n");
	for (i=0, cl=svs.clients ; i<maxclients->integer ; i++, cl++) {
		if (!cl->state)
			continue;
		Com_Printf (PRINT_ALL, "%3i ", i);
		Com_Printf (PRINT_ALL, "%5i ", cl->edict->client->ps.stats[STAT_FRAGS]);

		if (cl->state == CS_CONNECTED)
			Com_Printf (PRINT_ALL, "CNCT ");
		else if (cl->state == CS_FREE)
			Com_Printf (PRINT_ALL, "ZMBI ");
		else {
			ping = cl->ping < 9999 ? cl->ping : 9999;
			Com_Printf (PRINT_ALL, "%4i ", ping);
		}

		Com_Printf (PRINT_ALL, "%s", cl->name);
		l = 16 - strlen(cl->name);
		for (j=0 ; j<l ; j++)
			Com_Printf (PRINT_ALL, " ");

		Com_Printf (PRINT_ALL, "%7i ", svs.realTime - cl->lastMessage);

		s = NET_AdrToString  (cl->netChan.remoteAddress);
		Com_Printf (PRINT_ALL, "%s", s);
		l = 22 - strlen(s);
		for (j=0 ; j<l ; j++)
			Com_Printf (PRINT_ALL, " ");
		
		Com_Printf (PRINT_ALL, "%5i", cl->netChan.qPort);

		Com_Printf (PRINT_ALL, "\n");
	}
	Com_Printf (PRINT_ALL, "\n");
}


/*
==================
SV_ConSay_f
==================
*/
void SV_ConSay_f (void) {
	client_t *client;
	int		j;
	char	*p;
	char	text[1024];

	if (Cmd_Argc () < 2)
		return;

	strcpy (text, "console: ");
	p = Cmd_Args();

	if (*p == '"') {
		p++;
		p[strlen(p)-1] = 0;
	}

	strcat(text, p);

	for (j=0, client=svs.clients; j<maxclients->integer ; j++, client++) {
		if (client->state != CS_SPAWNED)
			continue;
		SV_ClientPrintf(client, PRINT_CHAT, "%s\n", text);
	}
}


/*
==================
SV_Heartbeat_f
==================
*/
void SV_Heartbeat_f (void) {
	svs.last_HeartBeat = -9999999;
}


/*
===========
SV_Serverinfo_f

Examine or change the serverinfo string
===========
*/
void SV_Serverinfo_f (void) {
	Com_Printf (PRINT_ALL, "Server info settings:\n");
	Info_Print (Cvar_Serverinfo ());
}


/*
===========
SV_DumpUser_f

Examine all a users info strings
===========
*/
void SV_DumpUser_f (void) {
	if (Cmd_Argc () != 2) {
		Com_Printf (PRINT_ALL, "Usage: dumpuser <userid>\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	Com_Printf (PRINT_ALL, "userinfo\n");
	Com_Printf (PRINT_ALL, "--------\n");
	Info_Print (sv_client->userInfo);

}


/*
==============
SV_ServerRecord_f

Begins server demo recording.  Every entity and every message will be
recorded, but no playerinfo will be stored.  Primarily for demo merging.
==============
*/
void SV_ServerRecord_f (void) {
	char	name[MAX_OSPATH];
	char	buf_data[32768];
	netMsg_t	buf;
	int		len;
	int		i;

	if (Cmd_Argc () != 2) {
		Com_Printf (PRINT_ALL, "serverrecord <demoname>\n");
		return;
	}

	if (svs.demo_File) {
		Com_Printf (PRINT_ALL, "Already recording.\n");
		return;
	}

	if (sv.state != SS_GAME) {
		Com_Printf (PRINT_ALL, "You must be in a level to record.\n");
		return;
	}

	/* open the demo file */
	Com_sprintf (name, sizeof (name), "%s/demos/%s.dm2", FS_Gamedir(), Cmd_Argv (1));

	Com_Printf (PRINT_ALL, "recording to %s.\n", name);
	FS_CreatePath (name);
	svs.demo_File = fopen (name, "wb");
	if (!svs.demo_File) {
		Com_Printf (PRINT_ALL, S_COLOR_RED "ERROR: couldn't open.\n");
		return;
	}

	/* setup a buffer to catch all multicasts */
	MSG_Init (&svs.demo_MultiCast, svs.demo_MultiCastBuf, sizeof (svs.demo_MultiCastBuf));

	/* write a single giant fake message with all the startup info */
	MSG_Init (&buf, buf_data, sizeof (buf_data));

	/*
	** serverdata needs to go over for all types of servers
	** to make sure the protocol is right, and to set the gamedir
	*/

	/* send the serverdata */
	MSG_WriteByte (&buf, SVC_SERVERDATA);
	MSG_WriteLong (&buf, PROTOCOL_VERSION);
	MSG_WriteLong (&buf, svs.spawnCount);

	/* 2 means server demo */
	MSG_WriteByte (&buf, 2);	// demos are always attract loops
	MSG_WriteString (&buf, Cvar_VariableString ("gamedir"));
	MSG_WriteShort (&buf, -1);

	/* send full levelname */
	MSG_WriteString (&buf, sv.configStrings[CS_NAME]);

	for (i=0 ; i<MAX_CONFIGSTRINGS ; i++) {
		if (sv.configStrings[i][0]) {
			MSG_WriteByte (&buf, SVC_CONFIGSTRING);
			MSG_WriteShort (&buf, i);
			MSG_WriteString (&buf, sv.configStrings[i]);
		}
	}

	/* write it to the demo file */
	Com_Printf (PRINT_DEV, "signon message length: %i\n", buf.curSize);
	len = LittleLong (buf.curSize);
	fwrite (&len, 4, 1, svs.demo_File);
	fwrite (buf.data, buf.curSize, 1, svs.demo_File);

	/* the rest of the demo file will be individual frames */
}


/*
==============
SV_ServerStop_f

Ends server demo recording
==============
*/
void SV_ServerStop_f (void) {
	if (!svs.demo_File) {
		Com_Printf (PRINT_ALL, "Not doing a serverrecord.\n");
		return;
	}
	fclose (svs.demo_File);
	svs.demo_File = NULL;
	Com_Printf (PRINT_ALL, "Recording completed.\n");
}


/*
===============
SV_KillServer_f

Kick everyone off, possibly in preparation for a new game
===============
*/
void SV_KillServer_f (void) {
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
void SV_ServerCommand_f (void) {
	if (!ge) {
		Com_Printf (PRINT_ALL, "No game loaded.\n");
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
	Cmd_AddCommand ("heartbeat",	SV_Heartbeat_f);
	Cmd_AddCommand ("kick",			SV_Kick_f);
	Cmd_AddCommand ("status",		SV_Status_f);
	Cmd_AddCommand ("serverinfo",	SV_Serverinfo_f);
	Cmd_AddCommand ("dumpuser",		SV_DumpUser_f);

	Cmd_AddCommand ("map",			SV_Map_f);
	Cmd_AddCommand ("demomap",		SV_DemoMap_f);
	Cmd_AddCommand ("gamemap",		SV_GameMap_f);
	Cmd_AddCommand ("setmaster",	SV_SetMaster_f);

	if (dedicated->integer)
		Cmd_AddCommand ("say",		SV_ConSay_f);

	Cmd_AddCommand ("serverrecord",	SV_ServerRecord_f);
	Cmd_AddCommand ("serverstop",	SV_ServerStop_f);

	Cmd_AddCommand ("save",			SV_Savegame_f);
	Cmd_AddCommand ("load",			SV_Loadgame_f);

	Cmd_AddCommand ("killserver",	SV_KillServer_f);

	Cmd_AddCommand ("sv",			SV_ServerCommand_f);
}
