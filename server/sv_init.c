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
// sv_init.c
//

#include "sv_local.h"

serverStatic_t	svs;				// persistant server info
serverState_t	sv;					// local server

float			svAirAcceleration = 0;

/*
================
SV_CreateBaseline

Entity baselines are used to compress the update messages
to the clients -- only the fields that differ from the
baseline will be transmitted
================
*/
static void SV_CreateBaseline (void) {
	edict_t			*svent;
	int				entnum;	

	for (entnum=1 ; entnum<ge->numEdicts ; entnum++) {
		svent = EDICT_NUM(entnum);
		if (!svent->inUse)
			continue;
		if (!svent->s.modelIndex && !svent->s.sound && !svent->s.effects)
			continue;
		svent->s.number = entnum;

		// take current state as baseline
		VectorCopy (svent->s.origin, svent->s.oldOrigin);
		sv.baseLines[entnum] = svent->s;
	}
}


/*
=================
SV_CheckForSavegame
=================
*/
static void SV_CheckForSavegame (void) {
	char		name[MAX_OSPATH];
	FILE		*f;
	int			i;

	if (sv_noreload->value)
		return;

	if (Cvar_VariableValue ("deathmatch"))
		return;

	Q_snprintfz (name, sizeof (name), "%s/save/current/%s.sav", FS_Gamedir(), sv.name);
	f = fopen (name, "rb");
	if (!f)
		return;		// no savegame

	fclose (f);

	SV_ClearWorld ();

	// get configstrings and areaportals
	SV_ReadLevelFile ();

	if (!sv.loadGame) {
		// coming back to a level after being in a different level, so run it for ten seconds

		/*
		** rlava2 was sending too many lightstyles, and overflowing the
		** reliable data. temporarily changing the server state to loading
		** prevents these from being passed down.
		*/
		int			previousState;		// PGM

		previousState = sv.state;				// PGM
		sv.state = SS_LOADING;					// PGM
		for (i=0 ; i<100 ; i++)
			ge->RunFrame ();

		sv.state = previousState;				// PGM
	}
}


/*
================
SV_SpawnServer

Change the server to a new map, taking all connected
clients along with it.
================
*/
static void SV_SpawnServer (char *server, char *spawnPoint, int serverState, qBool attractloop, qBool loadGame) {
	int		i;
	uInt	checksum;

	if (attractloop)
		Cvar_Set ("paused", "0");

	Com_Printf (0, "--------- Server Initialization --------\n");

	Com_Printf (PRNT_DEV, "SpawnServer: %s\n",server);
	if (sv.demoFile)
		fclose (sv.demoFile);

	svs.spawnCount++;		// any partially connected client will be restarted

	SV_SetState (SS_DEAD);

	// wipe the entire per-level structure
	memset (&sv, 0, sizeof (sv));
	svs.realTime = 0;
	sv.loadGame = loadGame;
	sv.attractLoop = attractloop;

	// save name for levels that don't set message
	strcpy (sv.configStrings[CS_NAME], server);
	if (Cvar_VariableValue ("deathmatch")) {
		Q_snprintfz (sv.configStrings[CS_AIRACCEL], sizeof (sv.configStrings[CS_AIRACCEL]), "%g", sv_airaccelerate->value);
		svAirAcceleration = sv_airaccelerate->value;
	} else {
		strcpy (sv.configStrings[CS_AIRACCEL], "0");
		svAirAcceleration = 0;
	}

	MSG_Init (&sv.multiCast, sv.multiCast_Buff, sizeof (sv.multiCast_Buff));

	strcpy (sv.name, server);

	// leave slots at start for clients only
	for (i=0 ; i<maxclients->integer ; i++) {
		// needs to reconnect
		if (svs.clients[i].state > SVCS_CONNECTED)
			svs.clients[i].state = SVCS_CONNECTED;
		svs.clients[i].lastFrame = -1;
	}

	sv.time = 1000;
	
	strcpy (sv.name, server);
	strcpy (sv.configStrings[CS_NAME], server);

	if (serverState != SS_GAME) {
		sv.models[1] = CM_LoadMap ("", qFalse, &checksum);	// no real map
	} else {
		Q_snprintfz (sv.configStrings[CS_MODELS+1],sizeof (sv.configStrings[CS_MODELS+1]), "maps/%s.bsp", server);
		sv.models[1] = CM_LoadMap (sv.configStrings[CS_MODELS+1], qFalse, &checksum);
	}
	Q_snprintfz (sv.configStrings[CS_MAPCHECKSUM],sizeof (sv.configStrings[CS_MAPCHECKSUM]), "%i", checksum);

	// clear physics interaction links
	SV_ClearWorld ();
	
	for (i=1 ; i<CM_NumInlineModels() ; i++) {
		Q_snprintfz (sv.configStrings[CS_MODELS+1+i], sizeof (sv.configStrings[CS_MODELS+1+i]), "*%i", i);
		sv.models[i+1] = CM_InlineModel (sv.configStrings[CS_MODELS+1+i]);
	}

	// spawn the rest of the entities on the map

	// precache and static commands can be issued during map initialization
	SV_SetState (SS_LOADING);

	// load and spawn all other entities
	ge->SpawnEntities (sv.name, CM_EntityString (), spawnPoint);

	// run two frames to allow everything to settle
	ge->RunFrame ();
	ge->RunFrame ();

	// all precaches are complete
	SV_SetState (serverState);
	
	// create a baseline for more efficient communications
	SV_CreateBaseline ();

	// check for a savegame
	SV_CheckForSavegame ();

	// set serverinfo variable
	Cvar_FullSet ("mapname", sv.name, CVAR_SERVERINFO | CVAR_NOSET);

	Com_Printf (0, "----------------------------------------\n");
}


/*
==============
SV_InitGame

A brand new game has been started
==============
*/
void SV_InitGame (void) {
	int		i;
	edict_t	*ent;
	char	idmaster[32];

	if (svs.initialized) {
		// cause any connected clients to reconnect
		SV_Shutdown ("Server restarted\n", qTrue);
	} else {
		// make sure the client is down
		CL_Drop ();
		SCR_BeginLoadingPlaque ();
	}

	// get any latched variable changes (maxclients, etc)
	Cvar_GetLatchedVars (CVAR_LATCH_SERVER);

	svs.initialized = qTrue;

	if (Cvar_VariableValue ("coop") && Cvar_VariableValue ("deathmatch")) {
		Com_Printf (0, "Deathmatch and Coop both set, disabling Coop\n");
		Cvar_FullSet ("coop", "0",  CVAR_SERVERINFO | CVAR_LATCH_SERVER);
	}

	/*
	** dedicated servers are can't be single player and are usually DM
	** so unless they explicity set coop, force it to deathmatch
	*/
	if (dedicated->integer) {
		if (!Cvar_VariableValue ("coop"))
			Cvar_FullSet ("deathmatch", "1",  CVAR_SERVERINFO | CVAR_LATCH_SERVER);
	}

	// init clients
	if (Cvar_VariableValue ("deathmatch")) {
		if (maxclients->integer <= 1)
			Cvar_FullSet ("maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH_SERVER);
		else if (maxclients->integer > MAX_CS_CLIENTS)
			Cvar_FullSet ("maxclients", Q_VarArgs ("%i", MAX_CS_CLIENTS), CVAR_SERVERINFO | CVAR_LATCH_SERVER);
	} else if (Cvar_VariableValue ("coop")) {
		if (maxclients->integer <= 1 || maxclients->integer > 4)
			Cvar_FullSet ("maxclients", "4", CVAR_SERVERINFO | CVAR_LATCH_SERVER);
	} else {
		// non-deathmatch, non-coop is one player
		Cvar_FullSet ("maxclients", "1", CVAR_SERVERINFO | CVAR_LATCH_SERVER);
	}

	svs.spawnCount = rand ();
	svs.clients = Mem_Alloc (sizeof (svClient_t)*maxclients->integer);
	svs.numClientEntities = maxclients->integer*UPDATE_BACKUP*64;
	svs.clientEntities = Mem_Alloc (sizeof (entityState_t)*svs.numClientEntities);

	// init network stuff
	NET_Config ((maxclients->integer > 1));

	// heartbeats will always be sent to the id master
	svs.lastHeartBeat = -99999;		// send immediately
	Q_snprintfz (idmaster, sizeof (idmaster), "192.246.40.37:%i", PORT_MASTER);
	NET_StringToAdr (idmaster, &svMasterAddr[0]);

	// init game
	SV_GameAPI_Init ();
	for (i=0 ; i<maxclients->integer ; i++) {
		ent = EDICT_NUM(i+1);
		ent->s.number = i+1;
		svs.clients[i].edict = ent;
		memset (&svs.clients[i].lastCmd, 0, sizeof (svs.clients[i].lastCmd));
	}
}


/*
======================
SV_Map

  the full syntax is:

  map [*]<map>$<startspot>+<nextserver>

command from the console or progs.
Map can also be a.cin, .pcx, or .dm2 file
Nextserver is used to allow a cinematic to play, then proceed to
another level:

	map tram.cin+jail_e3
======================
*/
void SV_Map (qBool attractloop, char *levelstring, qBool loadGame) {
	char	level[MAX_QPATH];
	char	*ch;
	int		l;
	char	spawnPoint[MAX_QPATH];

	sv.loadGame = loadGame;
	sv.attractLoop = attractloop;

	if ((sv.state == SS_DEAD) && !sv.loadGame)
		SV_InitGame ();	// the game is just starting

	strcpy (level, levelstring);

	// if there is a + in the map, set nextserver to the remainder
	ch = strstr(level, "+");
	if (ch) {
		*ch = 0;
		Cvar_Set ("nextserver", Q_VarArgs ("gamemap \"%s\"", ch+1));
	} else
		Cvar_Set ("nextserver", "");

	//ZOID special hack for end game screen in coop mode
	if (Cvar_VariableValue ("coop") && !Q_stricmp(level, "victory.pcx"))
		Cvar_Set ("nextserver", "gamemap \"*base1\"");

	// if there is a $, use the remainder as a spawnPoint
	ch = strstr(level, "$");
	if (ch) {
		*ch = 0;
		strcpy (spawnPoint, ch+1);
	} else
		spawnPoint[0] = 0;

	// skip the end-of-unit flag if necessary
	if (level[0] == '*')
		strcpy (level, level+1);

	l = strlen(level);
	if ((l > 4) && !strcmp (level+l-4, ".cin")) {
		SCR_BeginLoadingPlaque ();			// for local system
		SV_BroadcastCommand ("changing\n");
		SV_SpawnServer (level, spawnPoint, SS_CINEMATIC, attractloop, loadGame);
	} else if ((l > 4) && !strcmp (level+l-4, ".dm2")) {
		SCR_BeginLoadingPlaque ();			// for local system
		SV_BroadcastCommand ("changing\n");
		SV_SpawnServer (level, spawnPoint, SS_DEMO, attractloop, loadGame);
	} else if ((l > 4) && !strcmp (level+l-4, ".pcx")) {
		SCR_BeginLoadingPlaque ();			// for local system
		SV_BroadcastCommand ("changing\n");
		SV_SpawnServer (level, spawnPoint, SS_PIC, attractloop, loadGame);
	} else {
		SCR_BeginLoadingPlaque ();			// for local system
		SV_BroadcastCommand ("changing\n");
		SV_SendClientMessages ();
		SV_SpawnServer (level, spawnPoint, SS_GAME, attractloop, loadGame);
		Cbuf_CopyToDefer ();
	}

	SV_BroadcastCommand ("reconnect\n");
}
