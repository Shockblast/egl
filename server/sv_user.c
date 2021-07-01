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

// sv_user.c
// - server code for moving users

#include "sv_local.h"

edict_t	*sv_player;

/*
============================================================

	USER STRINGCMD EXECUTION
sv_client and sv_player will be valid.

============================================================
*/

/*
==================
SV_BeginDemoServer
==================
*/
void SV_BeginDemoserver (void) {
	char		name[MAX_OSPATH];

	Com_sprintf (name, sizeof (name), "demos/%s", sv.name);
	FS_FOpenFile (name, &sv.demo_File);
	if (!sv.demo_File)
		Com_Error (ERR_DROP, "Couldn't open %s\n", name);
}


/*
================
SV_New_f

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each server load.
================
*/
void SV_New_f (void) {
	char		*gamedir;
	int			playernum;
	edict_t		*ent;

	Com_Printf (PRINT_DEV, "New() from %s\n", sv_client->name);

	if (sv_client->state != CS_CONNECTED) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "New not valid -- already spawned\n");
		return;
	}

	/* demo servers just dump the file message */
	if (sv.state == SS_DEMO) {
		SV_BeginDemoserver ();
		return;
	}

	/*
	** serverdata needs to go over for all types of servers
	** to make sure the protocol is right, and to set the gamedir
	*/
	gamedir = Cvar_VariableString ("gamedir");

	/* send the serverdata */
	MSG_WriteByte (&sv_client->netChan.message, SVC_SERVERDATA);
	MSG_WriteLong (&sv_client->netChan.message, PROTOCOL_VERSION);
	MSG_WriteLong (&sv_client->netChan.message, svs.spawnCount);
	MSG_WriteByte (&sv_client->netChan.message, sv.attractLoop);
	MSG_WriteString (&sv_client->netChan.message, gamedir);

	if (sv.state == SS_CINEMATIC || sv.state == SS_PIC)
		playernum = -1;
	else
		playernum = sv_client - svs.clients;
	MSG_WriteShort (&sv_client->netChan.message, playernum);

	/* send full levelname */
	MSG_WriteString (&sv_client->netChan.message, sv.configStrings[CS_NAME]);

	/* game server */
	if (sv.state == SS_GAME) {
		/* set up the entity for the client */
		ent = EDICT_NUM(playernum+1);
		ent->s.number = playernum+1;
		sv_client->edict = ent;
		memset (&sv_client->lastCmd, 0, sizeof (sv_client->lastCmd));

		/* begin fetching configstrings */
		MSG_WriteByte (&sv_client->netChan.message, SVC_STUFFTEXT);
		MSG_WriteString (&sv_client->netChan.message, va("cmd configstrings %i 0\n",svs.spawnCount));
	}
}


/*
==================
SV_Configstrings_f
==================
*/
void SV_Configstrings_f (void) {
	int			start;

	Com_Printf (PRINT_DEV, "Configstrings() from %s\n", sv_client->name);

	if (sv_client->state != CS_CONNECTED) {
		Com_Printf (PRINT_ALL, "configstrings not valid -- already spawned\n");
		return;
	}

	/* handle the case of a level changing while a client was connecting */
	if (atoi (Cmd_Argv (1)) != svs.spawnCount) {
		Com_Printf (PRINT_ALL, "SV_Configstrings_f from different level\n");
		SV_New_f ();
		return;
	}
	
	start = atoi (Cmd_Argv (2));
	if (start < 0)
		start = 0;

	/* write a packet full of data */
	while ((sv_client->netChan.message.curSize < MAX_MSGLEN/2) && (start < MAX_CONFIGSTRINGS)) {
		if (sv.configStrings[start][0]) {
			MSG_WriteByte (&sv_client->netChan.message, SVC_CONFIGSTRING);
			MSG_WriteShort (&sv_client->netChan.message, start);
			MSG_WriteString (&sv_client->netChan.message, sv.configStrings[start]);
		}

		start++;
	}

	/* send next command */
	if (start == MAX_CONFIGSTRINGS) {
		MSG_WriteByte (&sv_client->netChan.message, SVC_STUFFTEXT);
		MSG_WriteString (&sv_client->netChan.message, va("cmd baselines %i 0\n", svs.spawnCount));
	} else {
		MSG_WriteByte (&sv_client->netChan.message, SVC_STUFFTEXT);
		MSG_WriteString (&sv_client->netChan.message, va("cmd configstrings %i %i\n", svs.spawnCount, start));
	}
}


/*
==================
SV_Baselines_f
==================
*/
void SV_Baselines_f (void) {
	int				start;
	entityState_t	nullstate;
	entityState_t	*base;

	Com_Printf (PRINT_DEV, "Baselines() from %s\n", sv_client->name);

	if (sv_client->state != CS_CONNECTED) {
		Com_Printf (PRINT_ALL, "baselines not valid -- already spawned\n");
		return;
	}
	
	/* handle the case of a level changing while a client was connecting */
	if (atoi(Cmd_Argv (1)) != svs.spawnCount) {
		Com_Printf (PRINT_ALL, "SV_Baselines_f from different level\n");
		SV_New_f ();
		return;
	}
	
	start = atoi(Cmd_Argv (2));
	if (start < 0)
		start = 0;

	memset (&nullstate, 0, sizeof (nullstate));

	/* write a packet full of data */
	while ((sv_client->netChan.message.curSize <  MAX_MSGLEN/2) && (start < MAX_EDICTS)) {
		base = &sv.baseLines[start];
		if (base->modelIndex || base->sound || base->effects) {
			MSG_WriteByte (&sv_client->netChan.message, SVC_SPAWNBASELINE);
			MSG_WriteDeltaEntity (&nullstate, base, &sv_client->netChan.message, qTrue, qTrue);
		}

		start++;
	}

	/* send next command */
	if (start == MAX_EDICTS) {
		MSG_WriteByte (&sv_client->netChan.message, SVC_STUFFTEXT);
		MSG_WriteString (&sv_client->netChan.message, va("precache %i\n", svs.spawnCount));
	} else {
		MSG_WriteByte (&sv_client->netChan.message, SVC_STUFFTEXT);
		MSG_WriteString (&sv_client->netChan.message, va("cmd baselines %i %i\n",svs.spawnCount, start));
	}
}


/*
==================
SV_Begin_f
==================
*/
void SV_Begin_f (void) {
	Com_Printf (PRINT_DEV, "Begin() from %s\n", sv_client->name);

	/* handle the case of a level changing while a client was connecting */
	if (atoi(Cmd_Argv (1)) != svs.spawnCount) {
		Com_Printf (PRINT_ALL, "SV_Begin_f from different level\n");
		SV_New_f ();
		return;
	}

	sv_client->state = CS_SPAWNED;
	
	/* call the game begin function */
	ge->ClientBegin (sv_player);

	Cbuf_InsertFromDefer ();
}

//=============================================================================

/*
==================
SV_NextDownload_f
==================
*/
void SV_NextDownload_f (void) {
	int		r;
	int		percent;
	int		size;

	if (!sv_client->download)
		return;

	r = sv_client->downloadSize - sv_client->downloadCount;
	if (r > 1024)
		r = 1024;

	MSG_WriteByte (&sv_client->netChan.message, SVC_DOWNLOAD);
	MSG_WriteShort (&sv_client->netChan.message, r);

	sv_client->downloadCount += r;
	size = sv_client->downloadSize;
	if (!size)
		size = 1;
	percent = sv_client->downloadCount*100/size;
	MSG_WriteByte (&sv_client->netChan.message, percent);
	MSG_Write (&sv_client->netChan.message,
		sv_client->download + sv_client->downloadCount - r, r);

	if (sv_client->downloadCount != sv_client->downloadSize)
		return;

	FS_FreeFile (sv_client->download);
	sv_client->download = NULL;
}


/*
==================
SV_BeginDownload_f
==================
*/
void SV_BeginDownload_f (void) {
	char	*name;
	extern	cvar_t *allow_download;
	extern	cvar_t *allow_download_players;
	extern	cvar_t *allow_download_models;
	extern	cvar_t *allow_download_sounds;
	extern	cvar_t *allow_download_maps;
	extern	int		file_from_pak; // ZOID did file come from pak?
	int offset = 0;

	name = Cmd_Argv (1);

	if (Cmd_Argc () > 2)
		offset = atoi (Cmd_Argv (2)); // downloaded offset

	// hacked by zoid to allow more conrol over download
	if (!allow_download->value		// first off, no .. or global allow check
		|| strstr (name, "..")		// don't allow anything with .. path
		|| (*name == '.')			// leading dot is no good
		|| (*name == '/')			// leading slash bad as well, must be in subdir
		|| (strncmp(name, "players/", 8) == 0 && !allow_download_players->value)	// next up, skin check
		|| (strncmp(name, "models/", 7) == 0 && !allow_download_models->value)		// now models
		|| (strncmp(name, "sound/", 6) == 0 && !allow_download_sounds->value)		// now sounds
		|| (strncmp(name, "maps/", 5) == 0 && !allow_download_maps->value)			// now maps (note special case for maps, must not be in pak)
		|| !strstr (name, "/")) {		// MUST be in a subdirectory
		MSG_WriteByte (&sv_client->netChan.message, SVC_DOWNLOAD);
		MSG_WriteShort (&sv_client->netChan.message, -1);
		MSG_WriteByte (&sv_client->netChan.message, 0);
		return;
	}


	if (sv_client->download)
		FS_FreeFile (sv_client->download);

	sv_client->downloadSize = FS_LoadFile (name, (void **)&sv_client->download);
	sv_client->downloadCount = offset;

	if (offset > sv_client->downloadSize)
		sv_client->downloadCount = sv_client->downloadSize;

	if (!sv_client->download
		/* special check for maps, if it came from a pak file, don't allow download  ZOID */
		|| (strncmp(name, "maps/", 5) == 0 && file_from_pak)) {
		Com_Printf (PRINT_DEV, "Couldn't download %s to %s\n", name, sv_client->name);
		if (sv_client->download) {
			FS_FreeFile (sv_client->download);
			sv_client->download = NULL;
		}

		MSG_WriteByte (&sv_client->netChan.message, SVC_DOWNLOAD);
		MSG_WriteShort (&sv_client->netChan.message, -1);
		MSG_WriteByte (&sv_client->netChan.message, 0);
		return;
	}

	SV_NextDownload_f ();
	Com_Printf (PRINT_DEV, "Downloading %s to %s\n", name, sv_client->name);
}

//============================================================================

/*
=================
SV_Disconnect_f

The client is going to disconnect, so remove the connection immediately
=================
*/
void SV_Disconnect_f (void) {
	SV_DropClient (sv_client);	
}


/*
==================
SV_ShowServerinfo_f

Dumps the serverinfo info string
==================
*/
void SV_ShowServerinfo_f (void) {
	Info_Print (Cvar_Serverinfo());
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
void SV_Nextserver_f (void) {
	if (atoi(Cmd_Argv (1)) != svs.spawnCount) {
		Com_Printf (PRINT_DEV, "Nextserver() from wrong level, from %s\n", sv_client->name);
		return;		// leftover from last server
	}

	Com_Printf (PRINT_DEV, "Nextserver() from %s\n", sv_client->name);

	SV_Nextserver ();
}

typedef struct {
	char	*name;
	void	(*func) (void);
} ucmd_t;

ucmd_t ucmds[] = {
	/* auto issued */
	{"new", SV_New_f},
	{"configstrings", SV_Configstrings_f},
	{"baselines", SV_Baselines_f},
	{"begin", SV_Begin_f},

	{"nextserver", SV_Nextserver_f},

	{"disconnect", SV_Disconnect_f},

	/* issued by hand at client consoles */
	{"info", SV_ShowServerinfo_f},

	{"download", SV_BeginDownload_f},
	{"nextdl", SV_NextDownload_f},

	{NULL, NULL}
};


/*
==================
SV_ExecuteUserCommand
==================
*/
void SV_ExecuteUserCommand (char *s) {
	ucmd_t	*u;
	
	Cmd_TokenizeString (s, qFalse);
	sv_player = sv_client->edict;

	for (u=ucmds ; u->name ; u++) {
		if (!strcmp (Cmd_Argv (0), u->name)) {
			u->func ();
			break;
		}
	}

	if (!u->name && sv.state == SS_GAME)
		ge->ClientCommand (sv_player);
}

/*
===========================================================================

	USER CMD EXECUTION

===========================================================================
*/

void SV_ClientThink (client_t *cl, userCmd_t *cmd) {
	cl->commandMsec -= cmd->msec;

	if ((cl->commandMsec < 0) && sv_enforcetime->value) {
		Com_Printf (PRINT_DEV, S_COLOR_YELLOW "commandMsec underflow from %s\n", cl->name);
		return;
	}

	ge->ClientThink (cl->edict, cmd);
}

/*
===================
SV_ExecuteClientMessage

The current net_Message is parsed for the given client
===================
*/
#define	MAX_STRINGCMDS	8
void SV_ExecuteClientMessage (client_t *cl) {
	int		c;
	char	*s;

	userCmd_t	nullcmd;
	userCmd_t	oldest, oldcmd, newcmd;
	int			net_drop;
	int			stringCmdCount;
	int			checksum, calculatedChecksum;
	int			checksumIndex;
	qBool		move_issued;
	int			lastFrame;

	sv_client = cl;
	sv_player = sv_client->edict;

	/* only allow one move command */
	move_issued = qFalse;
	stringCmdCount = 0;

	for ( ; ; ) {
		if (net_Message.readCount > net_Message.curSize) {
			Com_Printf (PRINT_ALL, "SV_ReadClientMessage: badread\n");
			SV_DropClient (cl);
			return;
		}	

		c = MSG_ReadByte (&net_Message);
		if (c == -1)
			break;
				
		switch (c) {
		default:
			Com_Printf (PRINT_ALL, "SV_ReadClientMessage: unknown command char\n");
			SV_DropClient (cl);
			return;
						
		case CLC_NOP:
			break;

		case CLC_USERINFO:
			strncpy (cl->userInfo, MSG_ReadString (&net_Message), sizeof (cl->userInfo)-1);
			SV_UserinfoChanged (cl);
			break;

		case CLC_MOVE:
			if (move_issued)
				return;		// someone is trying to cheat...

			move_issued = qTrue;
			checksumIndex = net_Message.readCount;
			checksum = MSG_ReadByte (&net_Message);
			lastFrame = MSG_ReadLong (&net_Message);
			if (lastFrame != cl->lastFrame) {
				cl->lastFrame = lastFrame;
				if (cl->lastFrame > 0) {
					cl->frameLatency[cl->lastFrame&(LATENCY_COUNTS-1)] = 
						svs.realTime - cl->frames[cl->lastFrame & UPDATE_MASK].sentTime;
				}
			}

			memset (&nullcmd, 0, sizeof (nullcmd));
			MSG_ReadDeltaUsercmd (&net_Message, &nullcmd, &oldest);
			MSG_ReadDeltaUsercmd (&net_Message, &oldest, &oldcmd);
			MSG_ReadDeltaUsercmd (&net_Message, &oldcmd, &newcmd);

			if (cl->state != CS_SPAWNED) {
				cl->lastFrame = -1;
				break;
			}

			/* if the checksum fails, ignore the rest of the packet */
			calculatedChecksum = Com_BlockSequenceCRCByte (
				net_Message.data + checksumIndex + 1,
				net_Message.readCount - checksumIndex - 1,
				cl->netChan.incomingSequence);

			if (calculatedChecksum != checksum) {
				Com_Printf (PRINT_DEV, "Failed command checksum for %s (%d != %d)/%d\n", 
					cl->name, calculatedChecksum, checksum, 
					cl->netChan.incomingSequence);
				return;
			}

			if (!sv_paused->value) {
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
			s = MSG_ReadString (&net_Message);

			/* malicious users may try using too many string commands */
			if (++stringCmdCount < MAX_STRINGCMDS)
				SV_ExecuteUserCommand (s);

			if (cl->state == CS_FREE)
				return;	// disconnect command
			break;
		}
	}
}
