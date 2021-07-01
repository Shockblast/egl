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
// sv_user.c
// Server code for moving users
//

#include "sv_local.h"

edict_t	*svCurrPlayer;

#define	MAX_STRINGCMDS	8

/*
=============================================================================

	USER STRINGCMD EXECUTION

	svCurrClient and svCurrPlayer will be valid.
=============================================================================
*/

/*
==================
SV_BeginDemoServer
==================
*/
static void SV_BeginDemoserver (void) {
	char	name[MAX_OSPATH];

	Q_snprintfz (name, sizeof (name), "demos/%s", sv.name);
	FS_FOpenFile (name, &sv.demoFile);

	if (!sv.demoFile)
		Com_Error (ERR_DROP, "Couldn't open %s", name);
}


/*
================
SV_New_f

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each server load.
================
*/
static void SV_New_f (void) {
	char		*gamedir;
	int			playernum;
	edict_t		*ent;

	Com_Printf (PRNT_DEV, "New() from %s\n", svCurrClient->name);

	if (svCurrClient->state != SVCS_CONNECTED) {
		Com_Printf (0, S_COLOR_YELLOW "New not valid -- already spawned\n");
		return;
	}

	// demo servers just dump the file message
	if (sv.state == SS_DEMO) {
		SV_BeginDemoserver ();
		return;
	}

	/*
	** serverdata needs to go over for all types of servers
	** to make sure the protocol is right, and to set the gamedir
	*/
	gamedir = Cvar_VariableString ("gamedir");

	// send the serverdata
	MSG_WriteByte (&svCurrClient->netChan.message, SVC_SERVERDATA);

	// report back the same protocol they used in their connection
	MSG_WriteLong (&svCurrClient->netChan.message,
#ifdef ENHANCED_SERVER
		svCurrClient->protocol);
#else
		ORIGINAL_PROTOCOL_VERSION);
#endif
	MSG_WriteLong (&svCurrClient->netChan.message, svs.spawnCount);
	MSG_WriteByte (&svCurrClient->netChan.message, sv.attractLoop);
	MSG_WriteString (&svCurrClient->netChan.message, gamedir);

	if (sv.state == SS_CINEMATIC || sv.state == SS_PIC)
		playernum = -1;
	else
		playernum = svCurrClient - svs.clients;
	MSG_WriteShort (&svCurrClient->netChan.message, playernum);

	// send full levelname
	MSG_WriteString (&svCurrClient->netChan.message, sv.configStrings[CS_NAME]);

	// game server
	if (sv.state == SS_GAME) {
		// set up the entity for the client
		ent = EDICT_NUM(playernum+1);
		ent->s.number = playernum+1;
		svCurrClient->edict = ent;
		memset (&svCurrClient->lastCmd, 0, sizeof (svCurrClient->lastCmd));

		// begin fetching configstrings
		MSG_WriteByte (&svCurrClient->netChan.message, SVC_STUFFTEXT);
		MSG_WriteString (&svCurrClient->netChan.message, Q_VarArgs ("cmd configstrings %i 0\n",svs.spawnCount));
	}
}


/*
==================
SV_ConfigStrings_f
==================
*/
static void SV_ConfigStrings_f (void) {
	int			start;

	Com_Printf (PRNT_DEV, "Configstrings() from %s\n", svCurrClient->name);

	if (svCurrClient->state != SVCS_CONNECTED) {
		Com_Printf (0, "configstrings not valid -- already spawned\n");
		return;
	}

	// handle the case of a level changing while a client was connecting
	if (atoi (Cmd_Argv (1)) != svs.spawnCount) {
		Com_Printf (0, "SV_ConfigStrings_f from different level\n");
		SV_New_f ();
		return;
	}
	
	start = atoi (Cmd_Argv (2));

	// security check
	if (start < 0) {
		Com_Printf (0, "ERROR: Illegal configstring from %s[%s], client dropped\n",
			svCurrClient->name, NET_AdrToString (svCurrClient->netChan.remoteAddress));
		SV_DropClient (svCurrClient);
		return;
	}

	// write a packet full of data
	while ((svCurrClient->netChan.message.curSize < MAX_MSGLEN/2) && (start < MAX_CONFIGSTRINGS)) {
		if (sv.configStrings[start][0]) {
			MSG_WriteByte (&svCurrClient->netChan.message, SVC_CONFIGSTRING);
			MSG_WriteShort (&svCurrClient->netChan.message, start);
			MSG_WriteString (&svCurrClient->netChan.message, sv.configStrings[start]);
		}

		start++;
	}

	// send next command
	if (start == MAX_CONFIGSTRINGS) {
		MSG_WriteByte (&svCurrClient->netChan.message, SVC_STUFFTEXT);
		MSG_WriteString (&svCurrClient->netChan.message, Q_VarArgs ("cmd baselines %i 0\n", svs.spawnCount));
	}
	else {
		MSG_WriteByte (&svCurrClient->netChan.message, SVC_STUFFTEXT);
		MSG_WriteString (&svCurrClient->netChan.message, Q_VarArgs ("cmd configstrings %i %i\n", svs.spawnCount, start));
	}
}


/*
==================
SV_Baselines_f
==================
*/
static void SV_Baselines_f (void) {
	int					start;
	entityStateOld_t	nullstate;
	entityStateOld_t	*base;

	Com_Printf (PRNT_DEV, "Baselines() from %s\n", svCurrClient->name);

	if (svCurrClient->state != SVCS_CONNECTED) {
		Com_Printf (0, "baselines not valid -- already spawned\n");
		return;
	}
	
	// handle the case of a level changing while a client was connecting
	if (atoi(Cmd_Argv (1)) != svs.spawnCount) {
		Com_Printf (0, "SV_Baselines_f from different level\n");
		SV_New_f ();
		return;
	}
	
	start = atoi(Cmd_Argv (2));
	if (start < 0)
		start = 0;

	memset (&nullstate, 0, sizeof (nullstate));

	// write a packet full of data
	while ((svCurrClient->netChan.message.curSize <  MAX_MSGLEN/2) && (start < MAX_CS_EDICTS)) {
		base = &sv.baseLines[start];
		if (base->modelIndex || base->sound || base->effects) {
			MSG_WriteByte (&svCurrClient->netChan.message, SVC_SPAWNBASELINE);
			MSG_WriteDeltaEntity (&nullstate, base, &svCurrClient->netChan.message, qTrue, qTrue, 0,
#ifdef ENHANCED_SERVER
				 svCurrClient->protocol);
#else
				ORIGINAL_PROTOCOL_VERSION);
#endif
		}

		start++;
	}

	// send next command
	if (start == MAX_CS_EDICTS) {
		MSG_WriteByte (&svCurrClient->netChan.message, SVC_STUFFTEXT);
		MSG_WriteString (&svCurrClient->netChan.message, Q_VarArgs ("precache %i\n", svs.spawnCount));
	}
	else {
		MSG_WriteByte (&svCurrClient->netChan.message, SVC_STUFFTEXT);
		MSG_WriteString (&svCurrClient->netChan.message, Q_VarArgs ("cmd baselines %i %i\n", svs.spawnCount, start));
	}
}


/*
==================
SV_Begin_f
==================
*/
static void SV_Begin_f (void) {
	Com_Printf (PRNT_DEV, "Begin() from %s\n", svCurrClient->name);

	// handle the case of a level changing while a client was connecting
	if (atoi(Cmd_Argv (1)) != svs.spawnCount) {
		Com_Printf (0, "SV_Begin_f from different level\n");
		SV_New_f ();
		return;
	}

	svCurrClient->state = SVCS_SPAWNED;
	
	// call the game begin function
	if (ge)
		ge->ClientBegin (svCurrPlayer);

	Cbuf_InsertFromDefer ();
}

// ==========================================================================

/*
==================
SV_NextDownload_f
==================
*/
static void SV_NextDownload_f (void) {
	int		r;
	int		percent;
	int		size;

	if (!svCurrClient->download)
		return;

	r = svCurrClient->downloadSize - svCurrClient->downloadCount;
	if (r > 1024)
		r = 1024;

	MSG_WriteByte (&svCurrClient->netChan.message, SVC_DOWNLOAD);
	MSG_WriteShort (&svCurrClient->netChan.message, r);

	svCurrClient->downloadCount += r;
	size = svCurrClient->downloadSize;
	if (!size)
		size = 1;
	percent = svCurrClient->downloadCount*100/size;
	MSG_WriteByte (&svCurrClient->netChan.message, percent);
	MSG_Write (&svCurrClient->netChan.message,
		svCurrClient->download + svCurrClient->downloadCount - r, r);

	if (svCurrClient->downloadCount != svCurrClient->downloadSize)
		return;

	FS_FreeFile (svCurrClient->download);
	svCurrClient->download = NULL;
}


/*
==================
SV_BeginDownload_f
==================
*/
static void SV_BeginDownload_f (void) {
	extern cVar_t	*allow_download;
	extern cVar_t	*allow_download_players;
	extern cVar_t	*allow_download_models;
	extern cVar_t	*allow_download_sounds;
	extern cVar_t	*allow_download_maps;
	extern qBool	fileFromPak; // ZOID did file come from pak?

	char	*name;
	int		offset = 0;

	name = Cmd_Argv (1);

	if (Cmd_Argc () > 2)
		offset = atoi (Cmd_Argv (2)); // downloaded offset

	// hacked by zoid to allow more conrol over download
	// no longer able to download from root (win32 issue)
	if (strstr (name, "..") || strstr (name, "\\") || !allow_download->value		// first off, no .. or global allow check
		|| strstr (name, "..")		// don't allow anything with .. path
		|| (*name == '.')			// leading dot is no good
		|| (*name == '/')			// leading slash bad as well, must be in subdir
		|| (strncmp (name, "players/", 8) == 0 && !allow_download_players->value)	// next up, skin check
		|| (strncmp (name, "models/", 7) == 0 && !allow_download_models->value)		// now models
		|| (strncmp (name, "sound/", 6) == 0 && !allow_download_sounds->value)		// now sounds
		|| (strncmp (name, "maps/", 5) == 0 && !allow_download_maps->value)			// now maps (note special case for maps, must not be in pak)
		|| !strstr (name, "/")) {		// MUST be in a subdirectory
		MSG_WriteByte (&svCurrClient->netChan.message, SVC_DOWNLOAD);
		MSG_WriteShort (&svCurrClient->netChan.message, -1);
		MSG_WriteByte (&svCurrClient->netChan.message, 0);
		return;
	}

	if (svCurrClient->download)
		FS_FreeFile (svCurrClient->download);

	svCurrClient->downloadSize = FS_LoadFile (name, (void **)&svCurrClient->download);
	svCurrClient->downloadCount = offset;

	if (offset > svCurrClient->downloadSize)
		svCurrClient->downloadCount = svCurrClient->downloadSize;

	if (!svCurrClient->download
		// special check for maps, if it came from a pak file, don't allow download  ZOID
		|| (strncmp(name, "maps/", 5) == 0 && fileFromPak)) {
		Com_Printf (PRNT_DEV, "Couldn't download %s to %s\n", name, svCurrClient->name);
		if (svCurrClient->download) {
			FS_FreeFile (svCurrClient->download);
			svCurrClient->download = NULL;
		}

		MSG_WriteByte (&svCurrClient->netChan.message, SVC_DOWNLOAD);
		MSG_WriteShort (&svCurrClient->netChan.message, -1);
		MSG_WriteByte (&svCurrClient->netChan.message, 0);
		return;
	}

	SV_NextDownload_f ();
	Com_Printf (PRNT_DEV, "Downloading %s to %s\n", name, svCurrClient->name);
}

//============================================================================

/*
=================
SV_Disconnect_f

The client is going to disconnect, so remove the connection immediately
=================
*/
static void SV_Disconnect_f (void) {
	SV_DropClient (svCurrClient);	
}


/*
==================
SV_ShowServerinfo_f

Dumps the serverinfo info string
==================
*/
static void SV_ShowServerinfo_f (void) {
	Info_Print (Cvar_BitInfo (CVAR_SERVERINFO));
}

void SV_Nextserver (void) {
	char	*v;

	//ZOID, SS_PIC can be nextserver'd in coop mode
	if (sv.state == SS_GAME || (sv.state == SS_PIC && !Cvar_VariableValue("coop")))
		return;		// can't nextserver while playing a normal game

	svs.spawnCount++;	// make sure another doesn't sneak in
	v = Cvar_VariableString ("nextserver");
	if (!v[0])
		Cbuf_AddText ("killserver\n");
	else {
		Cbuf_AddText (v);
		Cbuf_AddText ("\n");
	}

	Cvar_Set ("nextserver","");
}


/*
==================
SV_Nextserver_f

A cinematic has completed or been aborted by a client, so move
to the next server,
==================
*/
static void SV_Nextserver_f (void) {
	if (atoi(Cmd_Argv (1)) != svs.spawnCount) {
		Com_Printf (PRNT_DEV, "Nextserver() from wrong level, from %s\n", svCurrClient->name);
		return;		// leftover from last server
	}

	Com_Printf (PRNT_DEV, "Nextserver() from %s\n", svCurrClient->name);

	SV_Nextserver ();
}


/*
==================
SV_ExecuteUserCommand
==================
*/
typedef struct svUsrCmd_s {
	char	*name;
	void	(*func) (void);
} svUsrCmd_t;

static svUsrCmd_t svUserCmds[] = {
	// auto issued
	{"new",				SV_New_f},
	{"configstrings",	SV_ConfigStrings_f},
	{"baselines",		SV_Baselines_f},
	{"begin",			SV_Begin_f},

	{"nextserver",		SV_Nextserver_f},

	{"disconnect",		SV_Disconnect_f},

	// issued by hand at client consoles
	{"info",			SV_ShowServerinfo_f},

	{"download",		SV_BeginDownload_f},
	{"nextdl",			SV_NextDownload_f},

	// null terminate
	{NULL,				NULL}
};
void SV_ExecuteUserCommand (char *s) {
	svUsrCmd_t	*u;
	char		*testString;

	// catch attempted server exploits
	testString = Cmd_MacroExpandString (s);
	if (!testString)
		return;

	if (strcmp (testString, s)) {
		if ((svCurrClient->state == SVCS_SPAWNED) && *svCurrClient->name)
			SV_BroadcastPrintf (PRINT_HIGH, "%s was dropped: attempted server exploit\n", svCurrClient->name);
		SV_DropClient (svCurrClient);
		return;
	}

	Cmd_TokenizeString (s, qFalse);
	svCurrPlayer = svCurrClient->edict;

	for (u=svUserCmds ; u->name ; u++) {
		if (!strcmp (Cmd_Argv (0), u->name)) {
			u->func ();
			return;
		}
	}

	if ((sv.state == SS_GAME) && ge)
		ge->ClientCommand (svCurrPlayer);
}

/*
===========================================================================

	USER CMD EXECUTION

===========================================================================
*/

/*
===================
SV_ClientThink
===================
*/
static void SV_ClientThink (svClient_t *cl, userCmd_t *cmd) {
	cl->commandMsec -= cmd->msec;

	if ((cl->commandMsec < 0) && sv_enforcetime->value) {
		Com_Printf (PRNT_DEV, S_COLOR_YELLOW "commandMsec underflow from %s\n", cl->name);
		return;
	}

	if (ge)
		ge->ClientThink (cl->edict, cmd);
}


/*
===================
SV_ExecuteClientMessage

The current netMessage is parsed for the given client
===================
*/
void SV_ExecuteClientMessage (svClient_t *cl) {
	userCmd_t	nullcmd;
	userCmd_t	oldest, oldcmd, newcmd;
	int			net_drop;
	int			stringCmdCount;
	int			checksum, calculatedChecksum;
	int			checksumIndex;
	qBool		move_issued;
	int			lastFrame;
	int			c;
	char		*s;

	svCurrClient = cl;
	svCurrPlayer = svCurrClient->edict;

	// only allow one move command
	move_issued = qFalse;
	stringCmdCount = 0;

	for ( ; ; ) {
		if (netMessage.readCount > netMessage.curSize) {
			Com_Printf (0, "SV_ReadClientMessage: badread (%i > %i)\n", netMessage.readCount, netMessage.curSize);
			SV_DropClient (cl);
			return;
		}

		c = MSG_ReadByte (&netMessage);
		if (c == -1)
			break;
				
		switch (c) {
		default:
			Com_Printf (0, "SV_ReadClientMessage: unknown command char\n");
			SV_DropClient (cl);
			return;
						
		case CLC_NOP:
			break;

		case CLC_USERINFO:
			Q_strncpyz (cl->userInfo, MSG_ReadString (&netMessage), sizeof (cl->userInfo));
			SV_UserinfoChanged (cl);
			break;

		case CLC_MOVE:
			if (move_issued)
				return;		// someone is trying to cheat...

			move_issued = qTrue;
			checksumIndex = netMessage.readCount;
			checksum = MSG_ReadByte (&netMessage);
			lastFrame = MSG_ReadLong (&netMessage);
			if (lastFrame != cl->lastFrame) {
				cl->lastFrame = lastFrame;
				if (cl->lastFrame > 0) {
					cl->frameLatency[cl->lastFrame&(LATENCY_COUNTS-1)] = 
						svs.realTime - cl->frames[cl->lastFrame & UPDATE_MASK].sentTime;
				}
			}

			memset (&nullcmd, 0, sizeof (nullcmd));
			MSG_ReadDeltaUsercmd (&netMessage, &nullcmd, &oldest);
			MSG_ReadDeltaUsercmd (&netMessage, &oldest, &oldcmd);
			MSG_ReadDeltaUsercmd (&netMessage, &oldcmd, &newcmd);

			if (cl->state != SVCS_SPAWNED) {
				cl->lastFrame = -1;
				break;
			}

			// if the checksum fails, ignore the rest of the packet
			calculatedChecksum = Com_BlockSequenceCRCByte (
				netMessage.data + checksumIndex + 1,
				netMessage.readCount - checksumIndex - 1,
				cl->netChan.incomingSequence);

			if (calculatedChecksum != checksum) {
				Com_Printf (PRNT_DEV, "Failed command checksum for %s (%d != %d)/%d\n", 
					cl->name, calculatedChecksum, checksum, 
					cl->netChan.incomingSequence);
				return;
			}

			if (!sv_paused->integer) {
				net_drop = cl->netChan.dropped;
				if (net_drop < 20) {
					while (net_drop > 2) {
						SV_ClientThink (cl, &cl->lastCmd);

						net_drop--;
					}
					if (net_drop > 1)
						SV_ClientThink (cl, &oldest);

					if (net_drop > 0)
						SV_ClientThink (cl, &oldcmd);

				}
				SV_ClientThink (cl, &newcmd);
			}

			cl->lastCmd = newcmd;
			break;

		case CLC_STRINGCMD:	
			s = MSG_ReadString (&netMessage);

			// r1:Another security check, client caps at 256+1, but a hacked client could
			//    send huge strings, if they are then used in a mod which sends a %s cprintf
			//    to the exe, this could result in a buffer overflow for example.
			if (Q_strlen (s) > 257) {
				Com_Printf (0, "%s: excessive stringcmd discarded.\n", cl->name);
				break;
			}

			// malicious users may try using too many string commands
			if (++stringCmdCount < MAX_STRINGCMDS)
				SV_ExecuteUserCommand (s);

			if (cl->state == SVCS_FREE)
				return;	// disconnect command
			break;
		}
	}
}
