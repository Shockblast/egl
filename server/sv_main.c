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
// sv_main.c.
// Server main program
//

#include "sv_local.h"

netAdr_t	svMasterAddr[MAX_MASTERS];	// address of group servers

svClient_t	*svCurrClient;			// current client

cVar_t	*sv_paused;
cVar_t	*sv_timedemo;

cVar_t	*sv_enforcetime;

cVar_t	*timeout;				// seconds without any message
cVar_t	*zombietime;			// seconds to sink messages after disconnect

cVar_t	*rcon_password;			// password for remote server commands

cVar_t	*allow_download;
cVar_t	*allow_download_players;
cVar_t	*allow_download_models;
cVar_t	*allow_download_sounds;
cVar_t	*allow_download_maps;

cVar_t	*sv_airaccelerate;

cVar_t	*sv_noreload;			// don't reload level state when reentering

cVar_t	*maxclients;			// FIXME: rename sv_maxclients
cVar_t	*sv_showclamp;

cVar_t	*hostname;
cVar_t	*public_server;			// should heartbeats be sent

cVar_t	*sv_reconnect_limit;	// minimum seconds between connect messages

/*
=====================
SV_SetState
=====================
*/
void SV_SetState (int state) {
	sv.state = state;
	Com_SetServerState (state);
}

//============================================================================

/*
=====================
SV_DropClient

Called when the player is totally leaving the server, either willingly
or unwillingly.  This is NOT called if the entire server is quiting
or crashing.
=====================
*/
void SV_DropClient (svClient_t *drop) {
	// add the disconnect
	MSG_WriteByte (&drop->netChan.message, SVC_DISCONNECT);

	if (drop->state == SVCS_SPAWNED) {
		// call the prog function for removing a client
		// this will remove the body, among other things
		ge->ClientDisconnect (drop->edict);
	}

	if (drop->download) {
		FS_FreeFile (drop->download);
		drop->download = NULL;
	}

	drop->state = SVCS_FREE;		// become free in a few seconds
	drop->name[0] = 0;
}

/*
==============================================================================

	CONNECTIONLESS COMMANDS

==============================================================================
*/

/*
===============
SV_StatusString

Builds the string that is sent as heartbeats and status replies
===============
*/
char *SV_StatusString (void) {
	char		player[1024];
	static char	status[MAX_MSGLEN - 16];
	int			i;
	svClient_t	*cl;
	int			statusLength;
	int			playerLength;

	strcpy (status, Cvar_Serverinfo());
	strcat (status, "\n");
	statusLength = strlen(status);

	for (i=0 ; i<maxclients->integer ; i++) {
		cl = &svs.clients[i];
		if (cl->state == SVCS_CONNECTED || cl->state == SVCS_SPAWNED) {
			Q_snprintfz (player, sizeof (player), "%i %i \"%s\"\n", 
				cl->edict->client->ps.stats[STAT_FRAGS], cl->ping, cl->name);

			playerLength = strlen(player);
			if (statusLength + playerLength >= sizeof (status))
				break;		// can't hold any more

			strcpy (status + statusLength, player);
			statusLength += playerLength;
		}
	}

	return status;
}


/*
================
SVC_Status

Responds with all the info that qplug or qspy can see
================
*/
void SVC_Status (void) {
	Netchan_OutOfBandPrint (NS_SERVER, netFrom, "print\n%s", SV_StatusString());
}


/*
================
SVC_Ack
================
*/
void SVC_Ack (void) {
	Com_Printf (0, "Ping acknowledge from %s\n", NET_AdrToString (netFrom));
}


/*
================
SVC_Info

Responds with short info for broadcast scans
The second parameter should be the current protocol version number.
================
*/
void SVC_Info (void) {
	char	string[64];
	int		i, count;
	int		version;

	if (maxclients->integer == 1)
		return;		// ignore in single player

	version = atoi (Cmd_Argv (1));

	if (version != PROTOCOL_VERSION)
		Q_snprintfz (string, sizeof (string), "%s: wrong version\n", hostname->string, sizeof (string));
	else {
		count = 0;
		for (i=0 ; i<maxclients->integer ; i++)
			if (svs.clients[i].state >= SVCS_CONNECTED)
				count++;

		Q_snprintfz (string, sizeof (string), "%16s %8s %2i/%2i\n", hostname->string, sv.name, count, maxclients->integer);
	}

	Netchan_OutOfBandPrint (NS_SERVER, netFrom, "info\n%s", string);
}


/*
================
SVC_Ping

Just responds with an acknowledgement
================
*/
void SVC_Ping (void) {
	Netchan_OutOfBandPrint (NS_SERVER, netFrom, "ack");
}


/*
=================
SVC_GetChallenge

Returns a challenge number that can be used
in a subsequent client_connect command.
We do this to prevent denial of service attacks that
flood the server with invalid connection IPs.  With a
challenge, they must give a valid IP address.
=================
*/
void SVC_GetChallenge (void) {
	int		i;
	int		oldest;
	int		oldestTime;

	oldest = 0;
	oldestTime = 0x7fffffff;

	// see if we already have a challenge for this ip
	for (i = 0 ; i < MAX_CHALLENGES ; i++) {
		if (NET_CompareBaseAdr (netFrom, svs.challenges[i].adr))
			break;
		if (svs.challenges[i].time < oldestTime) {
			oldestTime = svs.challenges[i].time;
			oldest = i;
		}
	}

	if (i == MAX_CHALLENGES) {
		// overwrite the oldest
		svs.challenges[oldest].challenge = rand() & 0x7fff;
		svs.challenges[oldest].adr = netFrom;
		svs.challenges[oldest].time = sysTime;
		i = oldest;
	}

	// send it back
	Netchan_OutOfBandPrint (NS_SERVER, netFrom, "challenge %i", svs.challenges[i].challenge);
}


/*
==================
SVC_DirectConnect

A connection request that did not come from the master
==================
*/
void SVC_DirectConnect (void) {
	char		userInfo[MAX_INFO_STRING];
	netAdr_t	adr;
	int			i;
	svClient_t	*cl, *newcl;
	svClient_t	temp;
	edict_t		*ent;
	int			edictnum;
	int			version;
	int			qPort;
	int			challenge;

	adr = netFrom;

	Com_Printf (PRNT_DEV, "SVC_DirectConnect ()\n");

	version = atoi (Cmd_Argv (1));
	if (version != PROTOCOL_VERSION) {
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nServer is version %4.2f.\n", VERSION);
		Com_Printf (PRNT_DEV, "    rejected connect from version %i\n", version);
		return;
	}

	qPort = atoi (Cmd_Argv (2));

	challenge = atoi (Cmd_Argv (3));

	strncpy (userInfo, Cmd_Argv (4), sizeof (userInfo)-1);
	userInfo[sizeof (userInfo) - 1] = 0;

	// force the IP key/value pair so the game can filter based on ip
	Info_SetValueForKey (userInfo, "ip", NET_AdrToString(netFrom));

	// attractloop servers are ONLY for local clients
	if (sv.attractLoop) {
		if (!NET_IsLocalAddress (adr)) {
			Com_Printf (0, S_COLOR_YELLOW "Remote connect in attract loop. Ignored.\n");
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nConnection refused.\n");
			return;
		}
	}

	// see if the challenge is valid
	if (!NET_IsLocalAddress (adr)) {
		for (i=0 ; i<MAX_CHALLENGES ; i++) {
			if (NET_CompareBaseAdr (netFrom, svs.challenges[i].adr)) {
				if (challenge == svs.challenges[i].challenge)
					break;		// good
				Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nBad challenge.\n");
				return;
			}
		}

		if (i == MAX_CHALLENGES) {
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nNo challenge for address.\n");
			return;
		}
	}

	newcl = &temp;
	memset (newcl, 0, sizeof (svClient_t));

	// if there is already a slot for this ip, reuse it
	for (i=0, cl=svs.clients ; i<maxclients->integer ; i++, cl++) {
		if (cl->state == SVCS_FREE)
			continue;
		if (NET_CompareBaseAdr (adr, cl->netChan.remoteAddress) &&
			((cl->netChan.qPort == qPort) ||
			(adr.port == cl->netChan.remoteAddress.port))) {
			if (!NET_IsLocalAddress (adr) && (svs.realTime - cl->lastConnect) < ((int)sv_reconnect_limit->value * 1000)) {
				Com_Printf (PRNT_DEV, "%s:reconnect rejected : too soon\n", NET_AdrToString (adr));
				return;
			}
			Com_Printf (0, "%s:reconnect\n", NET_AdrToString (adr));
			newcl = cl;
			goto gotnewcl;
		}
	}

	// find a client slot
	newcl = NULL;
	for (i=0, cl=svs.clients ; i<maxclients->integer ; i++, cl++) {
		if (cl->state == SVCS_FREE) {
			newcl = cl;
			break;
		}
	}

	if (!newcl) {
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nServer is full.\n");
		Com_Printf (PRNT_DEV, "Rejected a connection.\n");
		return;
	}

gotnewcl:
	/*
	** build a new connection
	** accept the new client
	** this is the only place a svClient_t is ever initialized
	*/
	*newcl = temp;
	svCurrClient = newcl;
	edictnum = (newcl-svs.clients)+1;
	ent = EDICT_NUM(edictnum);
	newcl->edict = ent;
	newcl->challenge = challenge; // save challenge for checksumming

	// get the game a chance to reject this connection or modify the userInfo
	if (!(ge->ClientConnect (ent, userInfo))) {
		if (*Info_ValueForKey (userInfo, "rejmsg")) 
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\n%s\nConnection refused.\n",  
				Info_ValueForKey (userInfo, "rejmsg"));
		else
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nConnection refused.\n");
		Com_Printf (PRNT_DEV, "Game rejected a connection.\n");
		return;
	}

	// parse some info from the info strings
	strncpy (newcl->userInfo, userInfo, sizeof (newcl->userInfo)-1);
	SV_UserinfoChanged (newcl);

	// send the connect packet to the client
	Netchan_OutOfBandPrint (NS_SERVER, adr, "client_connect");

	Netchan_Setup (NS_SERVER, &newcl->netChan , adr, qPort);

	newcl->state = SVCS_CONNECTED;
	
	MSG_Init (&newcl->datagram, newcl->datagramBuff, sizeof (newcl->datagramBuff));
	newcl->datagram.allowOverflow = qTrue;
	newcl->lastMessage = svs.realTime;	// don't timeout
	newcl->lastConnect = svs.realTime;
}


/*
===============
SVC_RemoteCommand

A client issued an rcon command.
Shift down the remaining args
Redirect all printfs
===============
*/
int Rcon_Validate (void) {
	if (!strlen (rcon_password->string))
		return 0;

	if (strcmp (Cmd_Argv (1), rcon_password->string))
		return 0;

	return 1;
}

void SVC_RemoteCommand (void) {
	int		i;
	char	remaining[1024];

	i = Rcon_Validate ();

	if (i == 0)
		Com_Printf (0, S_COLOR_RED "Bad rcon from %s:\n%s\n", NET_AdrToString (netFrom), netMessage.data+4);
	else
		Com_Printf (0, "Rcon from %s:\n%s\n", NET_AdrToString (netFrom), netMessage.data+4);

	Com_BeginRedirect (RD_PACKET, svOutputBuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);

	if (!Rcon_Validate ()) {
		Com_Printf (0, "Bad rcon_password.\n");
	} else {
		remaining[0] = 0;

		for (i=2 ; i<Cmd_Argc () ; i++) {
			strcat (remaining, Cmd_Argv (i));
			strcat (remaining, " ");
		}

		Cmd_ExecuteString (remaining);
	}

	Com_EndRedirect ();
}


/*
=================
SV_ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/
void SV_ConnectionlessPacket (void) {
	char	*s;
	char	*c;

	if (netMessage.curSize > 1024) {
		Com_Printf (0, S_COLOR_YELLOW "Illegitimate packet size (%i) received, ignored.\n", netMessage.curSize);
		return;
	}

	MSG_BeginReading (&netMessage);
	MSG_ReadLong (&netMessage);		// skip the -1 marker

	s = MSG_ReadStringLine (&netMessage);

	Cmd_TokenizeString (s, qFalse);

	c = Cmd_Argv (0);
	Com_Printf (PRNT_DEV, "Packet %s : %s\n", NET_AdrToString(netFrom), c);

	if (!strcmp (c, "ping"))
		SVC_Ping ();
	else if (!strcmp (c, "ack"))
		SVC_Ack ();
	else if (!strcmp (c,"status"))
		SVC_Status ();
	else if (!strcmp (c,"info"))
		SVC_Info ();
	else if (!strcmp (c,"getchallenge"))
		SVC_GetChallenge ();
	else if (!strcmp (c,"connect"))
		SVC_DirectConnect ();
	else if (!strcmp (c, "rcon"))
		SVC_RemoteCommand ();
	else
		Com_Printf (0, S_COLOR_YELLOW "SV_ConnectionlessPacket: Bad connectionless packet from %s:\n%s\n", NET_AdrToString (netFrom), s);
}

//============================================================================

/*
===================
SV_CalcPings

Updates the cl->ping variables
===================
*/
void SV_CalcPings (void) {
	int			i, j;
	svClient_t	*cl;
	int			total, count;

	for (i=0 ; i<maxclients->integer ; i++) {
		cl = &svs.clients[i];
		if (cl->state != SVCS_SPAWNED)
			continue;

		total = 0;
		count = 0;
		for (j=0 ; j<LATENCY_COUNTS ; j++) {
			if (cl->frameLatency[j] > 0) {
				count++;
				total += cl->frameLatency[j];
			}
		}

		if (!count)
			cl->ping = 0;
		else
			cl->ping = total / count;

		// let the game dll know about the ping
		cl->edict->client->ping = cl->ping;
	}
}


/*
===================
SV_GiveMsec

Every few frames, gives all clients an allotment of milliseconds
for their command moves.  If they exceed it, assume cheating.
===================
*/
void SV_GiveMsec (void) {
	int			i;
	svClient_t	*cl;

	if (sv.frameNum & 15)
		return;

	for (i=0 ; i<maxclients->integer ; i++) {
		cl = &svs.clients[i];
		if (cl->state == SVCS_FREE)
			continue;
		
		cl->commandMsec = 1800;		// 1600 + some slop
	}
}


/*
=================
SV_ReadPackets
=================
*/
void SV_ReadPackets (void) {
	int			i;
	svClient_t	*cl;
	int			qPort;

	while (NET_GetPacket (NS_SERVER, &netFrom, &netMessage)) {
		// check for connectionless packet (0xffffffff) first
		if (*(int *)netMessage.data == -1) {
			SV_ConnectionlessPacket ();
			continue;
		}

		/*
		** read the qPort out of the message so we can fix up
		** stupid address translating routers
		*/
		MSG_BeginReading (&netMessage);
		MSG_ReadLong (&netMessage);		// sequence number
		MSG_ReadLong (&netMessage);		// sequence number
		qPort = MSG_ReadShort (&netMessage) & 0xffff;

		// check for packets from connected clients
		for (i=0, cl=svs.clients ; i<maxclients->integer ; i++, cl++) {
			if (cl->state == SVCS_FREE)
				continue;
			if (!NET_CompareBaseAdr (netFrom, cl->netChan.remoteAddress))
				continue;
			if (cl->netChan.qPort != qPort)
				continue;
			if (cl->netChan.remoteAddress.port != netFrom.port) {
				Com_Printf (0, "SV_ReadPackets: fixing up a translated port\n");
				cl->netChan.remoteAddress.port = netFrom.port;
			}

			if (Netchan_Process (&cl->netChan, &netMessage)) {
				// this is a valid, sequenced packet, so process it
				if (cl->state != SVCS_FREE) {
					cl->lastMessage = svs.realTime;	// don't timeout
					SV_ExecuteClientMessage (cl);
				}
			}
			break;
		}
		
		if (i != maxclients->integer)
			continue;
	}
}


/*
==================
SV_CheckTimeouts

If a packet has not been received from a client for timeout->value
seconds, drop the conneciton.  Server frames are used instead of
realtime to avoid dropping the local client while debugging.

When a client is normally dropped, the svClient_t goes into a zombie state
for a few seconds to make sure any final reliable message gets resent
if necessary
==================
*/
void SV_CheckTimeouts (void) {
	int			i;
	svClient_t	*cl;
	int			droppoint;
	int			zombiepoint;

	droppoint = svs.realTime - 1000*timeout->value;
	zombiepoint = svs.realTime - 1000*zombietime->value;

	for (i=0, cl=svs.clients ; i<maxclients->integer ; i++, cl++) {
		// message times may be wrong across a changelevel
		if (cl->lastMessage > svs.realTime)
			cl->lastMessage = svs.realTime;

		if ((cl->state == SVCS_FREE) && (cl->lastMessage < zombiepoint)) {
			cl->state = SVCS_FREE;	// can now be reused
			continue;
		}

		if (((cl->state == SVCS_CONNECTED) || (cl->state == SVCS_SPAWNED)) && (cl->lastMessage < droppoint)) {
			SV_BroadcastPrintf (PRINT_HIGH, "%s timed out\n", cl->name);
			SV_DropClient (cl); 
			cl->state = SVCS_FREE;	// don't bother with zombie state
		}
	}
}


/*
================
SV_PrepWorldFrame

This has to be done before the world logic, because
player processing happens outside RunWorldFrame
================
*/
void SV_PrepWorldFrame (void) {
	edict_t	*ent;
	int		i;

	for (i=0 ; i<ge->numEdicts ; i++, ent++) {
		ent = EDICT_NUM(i);
		// events only last for a single message
		ent->s.event = 0;
	}
}


/*
=================
SV_RunGameFrame
=================
*/
void SV_RunGameFrame (void) {
	if (host_speeds->value)
		timeBeforeGame = Sys_Milliseconds ();

	/*
	** we always need to bump framenum, even if we don't run the world, otherwise the delta
	** compression can get confused when a client has the "current" frame
	*/
	sv.frameNum++;
	sv.time = sv.frameNum*100;

	// don't run if paused
	if (!sv_paused->integer || (maxclients->integer > 1)) {
		ge->RunFrame ();

		// never get more than one tic behind
		if (sv.time < svs.realTime) {
			if (sv_showclamp->value)
				Com_Printf (0, "sv highclamp\n");
			svs.realTime = sv.time;
		}
	}

	if (host_speeds->value)
		timeAfterGame = Sys_Milliseconds ();
}


/*
==================
SV_Frame
==================
*/
void SV_Frame (int msec) {
	timeBeforeGame = timeAfterGame = 0;

	// if server is not active, do nothing
	if (!svs.initialized)
		return;

	svs.realTime += msec;

	// keep the random time dependent
	rand ();

	// check timeouts
	SV_CheckTimeouts ();

	// get packets from clients
	SV_ReadPackets ();

	// move autonomous things around if enough time has passed
	if (!sv_timedemo->value && svs.realTime < sv.time) {
		// never let the time get too far off
		if (sv.time - svs.realTime > 100) {
			if (sv_showclamp->value)
				Com_Printf (0, "sv lowclamp\n");
			svs.realTime = sv.time - 100;
		}
		NET_Sleep (sv.time - svs.realTime);
		return;
	}

	// update ping based on the last known frame from all clients
	SV_CalcPings ();

	// give the clients some timeslices
	SV_GiveMsec ();

	// let everything in the world think and move
	SV_RunGameFrame ();

	// send messages back to the clients that had packets read this frame
	SV_SendClientMessages ();

	// save the entire world state if recording a serverdemo
	SV_RecordDemoMessage ();

	// send a heartbeat to the master if needed
	Master_Heartbeat ();

	// clear teleport flags, etc for next frame
	SV_PrepWorldFrame ();

}

//============================================================================

/*
================
Master_Heartbeat

Send a message to the master every few minutes to
let it know we are alive, and log information
================
*/
#define	HEARTBEAT_SECONDS	300
void Master_Heartbeat (void) {
	char		*string;
	int			i;

	if (!dedicated || !dedicated->integer)
		return;		// only dedicated servers send heartbeats

	if (!public_server || !public_server->value)
		return;		// a private dedicated game

	// check for time wraparound
	if (svs.lastHeartBeat > svs.realTime)
		svs.lastHeartBeat = svs.realTime;

	if (svs.realTime - svs.lastHeartBeat < HEARTBEAT_SECONDS*1000)
		return;		// not time to send yet

	svs.lastHeartBeat = svs.realTime;

	// send the same string that we would give for a status OOB command
	string = SV_StatusString();

	// send to group master
	for (i=0 ; i<MAX_MASTERS ; i++) {
		if (svMasterAddr[i].port) {
			Com_Printf (0, "Sending heartbeat to %s\n", NET_AdrToString (svMasterAddr[i]));
			Netchan_OutOfBandPrint (NS_SERVER, svMasterAddr[i], "heartbeat\n%s", string);
		}
	}
}


/*
=================
Master_Shutdown

Informs all masters that this server is going down
=================
*/
void Master_Shutdown (void) {
	int			i;

	if (!dedicated || !dedicated->integer)
		return;		// only dedicated servers send heartbeats

	if (!public_server || !public_server->value)
		return;		// a private dedicated game

	// send to group master
	for (i=0 ; i<MAX_MASTERS ; i++) {
		if (svMasterAddr[i].port) {
			if (i > 0)
				Com_Printf (0, "Sending heartbeat to %s\n", NET_AdrToString (svMasterAddr[i]));
			Netchan_OutOfBandPrint (NS_SERVER, svMasterAddr[i], "shutdown");
		}
	}
}

//============================================================================

/*
=================
SV_UserinfoChanged

Pull specific info from a newly changed userinfo string
into a more C freindly form.
=================
*/
void SV_UserinfoChanged (svClient_t *cl) {
	char	*val;
	int		i;

	// call prog code to allow overrides
	ge->ClientUserinfoChanged (cl->edict, cl->userInfo);
	
	// name for C code
	strncpy (cl->name, Info_ValueForKey (cl->userInfo, "name"), sizeof (cl->name)-1);
	// mask off high bit
	for (i=0 ; i<sizeof (cl->name) ; i++)
		cl->name[i] &= 127;

	// rate command
	val = Info_ValueForKey (cl->userInfo, "rate");
	if (strlen (val)) {
		i = atoi(val);
		cl->rate = clamp (i, 100, 15000);
	} else
		cl->rate = 5000;

	// msg command
	val = Info_ValueForKey (cl->userInfo, "msg");
	if (strlen (val))
		cl->messageLevel = atoi (val);
}

//============================================================================

/*
==================
SV_FinalMessage

Used by SV_Shutdown to send a final message to all connected clients before
the server goes down. The messages are sent immediately, not just stuck on
the outgoing message list, because the server is going to totally exit after
returning from this function.
==================
*/
void SV_FinalMessage (char *message, qBool reconnect) {
	int			i;
	svClient_t	*cl;
	
	MSG_Clear (&netMessage);
	MSG_WriteByte (&netMessage, SVC_PRINT);
	MSG_WriteByte (&netMessage, PRINT_HIGH);
	MSG_WriteString (&netMessage, message);

	if (reconnect)
		MSG_WriteByte (&netMessage, SVC_RECONNECT);
	else
		MSG_WriteByte (&netMessage, SVC_DISCONNECT);

	// send it twice
	// stagger the packets to crutch operating system limited buffers
	for (i=0, cl=svs.clients ; i<maxclients->integer ; i++, cl++) {
		if (cl->state >= SVCS_CONNECTED)
			Netchan_Transmit (&cl->netChan, netMessage.curSize, netMessage.data);
	}

	for (i=0, cl=svs.clients ; i<maxclients->integer ; i++, cl++) {
		if (cl->state >= SVCS_CONNECTED)
			Netchan_Transmit (&cl->netChan, netMessage.curSize, netMessage.data);
	}
}


/*
===============
SV_Init

Only called at egl.exe startup, not for each game
===============
*/
void SV_Init (void) {
	SV_InitOperatorCommands	();

	rcon_password	= Cvar_Get ("rcon_password",		"",			0);
	
	Cvar_Get ("skill",			"1",								0);
	Cvar_Get ("deathmatch",		"0",								CVAR_LATCH_SERVER);
	Cvar_Get ("coop",			"0",								CVAR_LATCH_SERVER);
	Cvar_Get ("dmflags",		Q_VarArgs ("%i", DF_INSTANT_ITEMS),	CVAR_SERVERINFO);
	Cvar_Get ("fraglimit",		"0",								CVAR_SERVERINFO);
	Cvar_Get ("timelimit",		"0",								CVAR_SERVERINFO);
	Cvar_Get ("cheats",			"0",								CVAR_SERVERINFO|CVAR_LATCH_SERVER);
	Cvar_Get ("protocol",		Q_VarArgs ("%i", PROTOCOL_VERSION),	CVAR_SERVERINFO|CVAR_NOSET);

	maxclients				= Cvar_Get ("maxclients",			"1",		CVAR_SERVERINFO|CVAR_LATCH_SERVER);
	hostname				= Cvar_Get ("hostname",				"noname",	CVAR_SERVERINFO|CVAR_ARCHIVE);
	timeout					= Cvar_Get ("timeout",				"125",		0);
	zombietime				= Cvar_Get ("zombietime",			"2",		0);

	sv_showclamp			= Cvar_Get ("showclamp",			"0",		0);
	sv_paused				= Cvar_Get ("paused",				"0",		0);
	sv_timedemo				= Cvar_Get ("timedemo",				"0",		0);

	sv_enforcetime			= Cvar_Get ("sv_enforcetime",		"0",		0);
	sv_reconnect_limit		= Cvar_Get ("sv_reconnect_limit",	"3",		CVAR_ARCHIVE);
	sv_noreload				= Cvar_Get ("sv_noreload",			"0",		0);
	sv_airaccelerate		= Cvar_Get ("sv_airaccelerate",		"0",		CVAR_LATCH_SERVER);

	allow_download			= Cvar_Get ("allow_download",			"1",	CVAR_ARCHIVE);
	allow_download_players	= Cvar_Get ("allow_download_players",	"0",	CVAR_ARCHIVE);
	allow_download_models	= Cvar_Get ("allow_download_models",	"1",	CVAR_ARCHIVE);
	allow_download_sounds	= Cvar_Get ("allow_download_sounds",	"1",	CVAR_ARCHIVE);
	allow_download_maps		= Cvar_Get ("allow_download_maps",		"1",	CVAR_ARCHIVE);

	public_server		= Cvar_Get ("public",	"0",	0);

	MSG_Init (&netMessage, netMessageBuffer, sizeof (netMessageBuffer));
}


/*
================
SV_Shutdown

Called when each game quits, before Sys_Quit or Sys_Error
================
*/
void SV_Shutdown (char *finalmsg, qBool reconnect) {
	if (svs.clients)
		SV_FinalMessage (finalmsg, reconnect);

	Master_Shutdown ();
	SV_GameAPI_Shutdown ();

	// free current level
	if (sv.demoFile)
		fclose (sv.demoFile);
	memset (&sv, 0, sizeof (sv));
	SV_SetState (SS_DEAD);

	// free server static data
	if (svs.clients)
		Mem_Free (svs.clients);
	if (svs.clientEntities)
		Mem_Free (svs.clientEntities);
	if (svs.demoFile)
		fclose (svs.demoFile);
	memset (&svs, 0, sizeof (svs));

	CM_UnloadMap ();
}
