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

netAdr_t	sv_masterAddresses[MAX_MASTERS];	// address of group servers

svClient_t	*sv_currentClient;			// current client

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
void SV_SetState (ssState_t state)
{
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
void SV_DropClient (svClient_t *drop)
{
	// Add the disconnect
	MSG_WriteByte (&drop->netChan.message, SVC_DISCONNECT);

	if (drop->state == SVCS_SPAWNED) {
		// Call the prog function for removing a client
		// This will remove the body, among other things
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
static char *SV_StatusString (void)
{
	char		player[1024];
	static char	status[MAX_SV_MSGLEN - 16];
	int			i;
	svClient_t	*cl;
	int			statusLength;
	int			playerLength;

	Q_snprintfz (status, sizeof (status), "%s\n", Cvar_BitInfo (CVAR_SERVERINFO));
	statusLength = strlen (status);

	for (i=0 ; i<maxclients->integer ; i++) {
		cl = &svs.clients[i];
		if (cl->state == SVCS_CONNECTED || cl->state == SVCS_SPAWNED) {
			Q_snprintfz (player, sizeof (player), "%i %i \"%s\"\n", 
				cl->edict->client->playerState.stats[STAT_FRAGS], cl->ping, cl->name);

			playerLength = strlen (player);
			if (statusLength + playerLength >= sizeof (status))
				break;		// Can't hold any more

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
static void SVC_Status (void)
{
	Netchan_OutOfBandPrint (NS_SERVER, sv_netFrom, "print\n%s", SV_StatusString());
}


/*
================
SVC_Ack
================
*/
static void SVC_Ack (void)
{
	int		i;

	// R1: could be used to flood server console - only show acks from masters.
	for (i=0 ; i<MAX_MASTERS ; i++) {
		if (!sv_masterAddresses[i].port)
			continue;
		if (!NET_CompareBaseAdr (sv_masterAddresses[i], sv_netFrom))
			continue;

		Com_Printf (0, "Ping acknowledge from %s\n", NET_AdrToString (sv_netFrom));
	}
}


/*
================
SVC_Info

Responds with short info for broadcast scans
The second parameter should be the current protocol version number.
================
*/
static void SVC_Info (void)
{
	char	string[64];
	int		i, count;
	int		version;

	if (maxclients->integer == 1)
		return;		// Ignore in single player

	version = atoi (Cmd_Argv (1));

	if (version != ORIGINAL_PROTOCOL_VERSION) {
		// No need to respond, and we don't need to ship off the info packet either
		return;
	}
	else {
		count = 0;
		for (i=0 ; i<maxclients->integer ; i++) {
			if (svs.clients[i].state >= SVCS_CONNECTED)
				count++;
		}

		Q_snprintfz (string, sizeof (string), "%16s %8s %2i/%2i\n", hostname->string, sv.name, count, maxclients->integer);
	}

	Netchan_OutOfBandPrint (NS_SERVER, sv_netFrom, "info\n%s", string);
}


/*
================
SVC_Ping

Just responds with an acknowledgement
================
*/
static void SVC_Ping (void)
{
	Netchan_OutOfBandPrint (NS_SERVER, sv_netFrom, "ack");
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
static void SVC_GetChallenge (void)
{
	int		i;
	int		oldest;
	int		oldestTime;

	oldest = 0;
	oldestTime = 0x7fffffff;

	// See if we already have a challenge for this ip
	for (i=0 ; i<MAX_CHALLENGES ; i++) {
		if (NET_CompareBaseAdr (sv_netFrom, svs.challenges[i].adr))
			break;
		if (svs.challenges[i].time < oldestTime) {
			oldestTime = svs.challenges[i].time;
			oldest = i;
		}
	}

	if (i == MAX_CHALLENGES) {
		// Overwrite the oldest
		svs.challenges[oldest].challenge = rand() & 0x7fff;
		svs.challenges[oldest].adr = sv_netFrom;
		svs.challenges[oldest].time = Sys_Milliseconds ();
		i = oldest;
	}

	// Send it back
	Netchan_OutOfBandPrint (NS_SERVER, sv_netFrom, "challenge %i", svs.challenges[i].challenge);
}


/*
==================
SVC_DirectConnect

A connection request that did not come from the master
==================
*/
static void SVC_DirectConnect (void)
{
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

	adr = sv_netFrom;

	Com_DevPrintf (0, "SVC_DirectConnect ()\n");

	version = atoi (Cmd_Argv (1));
	if (version != ORIGINAL_PROTOCOL_VERSION) {
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nServer is not using same protocol (%i != %i).\n",
								version, ORIGINAL_PROTOCOL_VERSION);
		Com_DevPrintf (0, "    rejected connect from version %i\n", version);
		return;
	}

	qPort = atoi (Cmd_Argv (2));

	challenge = atoi (Cmd_Argv (3));

	Q_strncpyz (userInfo, Cmd_Argv (4), sizeof (userInfo));

	// Force the IP key/value pair so the game can filter based on ip
	Info_SetValueForKey (userInfo, "ip", NET_AdrToString (sv_netFrom));

	// Attractloop servers are ONLY for local clients
	if (sv.attractLoop) {
		if (!NET_IsLocalAddress (adr)) {
			Com_Printf (PRNT_WARNING, "Remote connect in attract loop. Ignored.\n");
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nConnection refused.\n");
			return;
		}
	}

	// See if the challenge is valid
	if (!NET_IsLocalAddress (adr)) {
		for (i=0 ; i<MAX_CHALLENGES ; i++) {
			if (NET_CompareBaseAdr (sv_netFrom, svs.challenges[i].adr)) {
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

	// If there is already a slot for this ip, reuse it
	for (i=0, cl=svs.clients ; i<maxclients->integer ; i++, cl++) {
		if (cl->state == SVCS_FREE)
			continue;
		if (NET_CompareBaseAdr (adr, cl->netChan.remoteAddress)
		&& (cl->netChan.qPort == qPort || adr.port == cl->netChan.remoteAddress.port)) {
			if (!NET_IsLocalAddress (adr) && (svs.realTime - cl->lastConnect) < ((int)sv_reconnect_limit->value * 1000)) {
				Com_DevPrintf (0, "%s:reconnect rejected : too soon\n", NET_AdrToString (adr));
				return;
			}

			Com_Printf (0, "%s:reconnect\n", NET_AdrToString (adr));

			newcl = cl;
			goto gotNewClient;
		}
	}

	// Find a client slot
	newcl = NULL;
	for (i=0, cl=svs.clients ; i<maxclients->integer ; i++, cl++) {
		if (cl->state == SVCS_FREE) {
			newcl = cl;
			break;
		}
	}

	if (!newcl) {
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nServer is full.\n");
		Com_DevPrintf (0, "Rejected a connection.\n");
		return;
	}

gotNewClient:
	/*
	** Build a new connection and accept the new client
	** This is the only place a svClient_t is ever initialized
	*/
	*newcl = temp;
	sv_currentClient = newcl;
	edictnum = (newcl-svs.clients)+1;
	ent = EDICT_NUM(edictnum);
	newcl->edict = ent;
	newcl->challenge = challenge; // Save challenge for checksumming

	// Get the game a chance to reject this connection or modify the userInfo
	if (!(ge->ClientConnect (ent, userInfo))) {
		if (*Info_ValueForKey (userInfo, "rejmsg")) 
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\n%s\nConnection refused.\n",  
				Info_ValueForKey (userInfo, "rejmsg"));
		else
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nConnection refused.\n");
		Com_DevPrintf (0, "Game rejected a connection.\n");
		return;
	}

	// Parse some info from the info strings
	Q_strncpyz (newcl->userInfo, userInfo, sizeof (newcl->userInfo));
	SV_UserinfoChanged (newcl);

	// Send the connect packet to the client
	Netchan_OutOfBandPrint (NS_SERVER, adr, "client_connect");

	Netchan_Setup (NS_SERVER, &newcl->netChan, adr, version, qPort, 0);

	newcl->protocol = version;
	newcl->state = SVCS_CONNECTED;

	MSG_Init (&newcl->datagram, newcl->datagramBuff, sizeof (newcl->datagramBuff));
	newcl->datagram.allowOverflow = qTrue;

	newcl->lastMessage = svs.realTime - ((timeout->value - 5) * 1000);	// don't timeout
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
qBool Rcon_Validate (void)
{
	if (!strlen (rcon_password->string))
		return qFalse;

	if (strcmp (Cmd_Argv (1), rcon_password->string))
		return qFalse;

	return qTrue;
}

void SVC_RemoteCommand (void)
{
	char	remaining[1024];
	qBool	valid;
	int		i;

	valid = Rcon_Validate ();

	if (valid)
		Com_Printf (0, "Rcon from %s:\n%s\n", NET_AdrToString (sv_netFrom), sv_netMessage.data+4);
	else
		Com_Printf (PRNT_ERROR, "Bad rcon from %s:\n%s\n", NET_AdrToString (sv_netFrom), sv_netMessage.data+4);

	Com_BeginRedirect (RD_PACKET, sv_outputBuffer, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);

	if (!valid) {
		Com_Printf (0, "Bad rcon_password.\n");
	}
	else {
		remaining[0] = 0;

		for (i=2 ; i<Cmd_Argc () ; i++) {
			Q_strcatz (remaining, Cmd_Argv (i), sizeof (remaining));
			Q_strcatz (remaining, " ", sizeof (remaining));
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
void SV_ConnectionlessPacket (void)
{
	char	*s;
	char	*c;

	// Should NEVER EVER happen
	if (sv_netMessage.curSize > 1024) {
		Com_Printf (PRNT_WARNING, "Illegitimate packet size (%i) received, ignored.\n", sv_netMessage.curSize);
		return;
	}

	MSG_BeginReading (&sv_netMessage);
	MSG_ReadLong (&sv_netMessage);		// skip the -1 marker

	s = MSG_ReadStringLine (&sv_netMessage);

	Cmd_TokenizeString (s, qFalse);

	c = Cmd_Argv (0);
	Com_DevPrintf (0, "Packet %s : %s\n", NET_AdrToString (sv_netFrom), c);

	if (!strcmp (c, "ping"))
		SVC_Ping ();
	else if (!strcmp (c, "ack"))
		SVC_Ack ();
	else if (!strcmp (c, "status"))
		SVC_Status ();
	else if (!strcmp (c, "info"))
		SVC_Info ();
	else if (!strcmp (c, "getchallenge"))
		SVC_GetChallenge ();
	else if (!strcmp (c, "connect"))
		SVC_DirectConnect ();
	else if (!strcmp (c, "rcon"))
		SVC_RemoteCommand ();
	else
		Com_Printf (PRNT_WARNING, "SV_ConnectionlessPacket: Bad connectionless packet from %s:\n%s\n", NET_AdrToString (sv_netFrom), s);
}

//============================================================================

/*
===================
SV_CalcPings

Updates the cl->ping variables
===================
*/
void SV_CalcPings (void)
{
	int			i, j;
	svClient_t	*cl;
	int			total, count;

	for (i=0 ; i<maxclients->integer ; i++) {
		cl = &svs.clients[i];
		if (!cl->edict || !cl->edict->client || cl->state != SVCS_SPAWNED)
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

		// Let the game dll know about the ping
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
void SV_GiveMsec (void)
{
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
void SV_ReadPackets (void)
{
	int			i;
	svClient_t	*cl;
	int			qPort;

	while (NET_GetPacket (NS_SERVER, &sv_netFrom, &sv_netMessage)) {
		// Check for connectionless packet (0xffffffff) first
		if (*(int *)sv_netMessage.data == -1) {
			SV_ConnectionlessPacket ();
			continue;
		}

		/*
		** Read the qPort out of the message so we can fix up
		** stupid address translating routers
		*/
		MSG_BeginReading (&sv_netMessage);
		MSG_ReadLong (&sv_netMessage);		// Sequence number
		MSG_ReadLong (&sv_netMessage);		// Sequence number
		qPort = MSG_ReadShort (&sv_netMessage) & 0xffff;

		// Check for packets from connected clients
		for (i=0, cl=svs.clients ; i<maxclients->integer ; i++, cl++) {
			if (cl->state == SVCS_FREE)
				continue;
			if (!NET_CompareBaseAdr (sv_netFrom, cl->netChan.remoteAddress))
				continue;
			if (cl->netChan.qPort != qPort)
				continue;
			if (cl->netChan.remoteAddress.port != sv_netFrom.port) {
				Com_Printf (0, "SV_ReadPackets: fixing up a translated port\n");
				cl->netChan.remoteAddress.port = sv_netFrom.port;
			}

			if (Netchan_Process (&cl->netChan, &sv_netMessage)) {
				// This is a valid, sequenced packet, so process it
				if (cl->state != SVCS_FREE) {
					cl->lastMessage = svs.realTime;	// Don't timeout
					SV_ExecuteClientMessage (cl);
				}
			}
			break;
		}
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
void SV_CheckTimeouts (void)
{
	int			i;
	svClient_t	*cl;
	int			droppoint;
	int			zombiepoint;

	droppoint = svs.realTime - 1000*timeout->value;
	zombiepoint = svs.realTime - 1000*zombietime->value;

	for (i=0, cl=svs.clients ; i<maxclients->integer ; i++, cl++) {
		// Message times may be wrong across a changelevel
		if (cl->lastMessage > svs.realTime)
			cl->lastMessage = svs.realTime;

		if ((cl->state == SVCS_FREE) && (cl->lastMessage < zombiepoint)) {
			cl->state = SVCS_FREE;	// Can now be reused
			continue;
		}

		if ((cl->state == SVCS_CONNECTED || cl->state == SVCS_SPAWNED) && cl->lastMessage < droppoint) {
			SV_BroadcastPrintf (PRINT_HIGH, "%s timed out\n", cl->name);
			SV_DropClient (cl); 
			cl->state = SVCS_FREE;	// Don't bother with zombie state
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
void SV_PrepWorldFrame (void)
{
	edict_t	*ent;
	int		i;

	for (i=0 ; i<ge->numEdicts ; i++, ent++) {
		ent = EDICT_NUM(i);
		// Events only last for a single message
		ent->s.event = 0;
	}
}


/*
=================
SV_RunGameFrame
=================
*/
void SV_RunGameFrame (void)
{
	/*
	** We always need to bump framenum, even if we don't run the world, otherwise the delta
	** compression can get confused when a client has the "current" frame
	*/
	sv.frameNum++;
	sv.time = sv.frameNum*100;

	// Don't run if paused
	if (!sv_paused->integer || maxclients->integer > 1) {
		ge->RunFrame ();

		// Never get more than one tic behind
		if (sv.time < (uint32)svs.realTime) {
			if (sv_showclamp->value)
				Com_Printf (0, "sv highclamp\n");
			svs.realTime = sv.time;
		}
	}
}


/*
==================
SV_Frame
==================
*/
void SV_Frame (int msec)
{
	// If server is not active, do nothing
	if (!svs.initialized)
		return;

	svs.realTime += msec;

	// Keep the random time dependent
	rand ();

	// Check timeouts
	SV_CheckTimeouts ();

	// Get packets from clients
	SV_ReadPackets ();

	// Move autonomous things around if enough time has passed
	if (!sv_timedemo->value && (uint32)svs.realTime < sv.time) {
		// Never let the time get too far off
		if (sv.time - svs.realTime > 100) {
			if (sv_showclamp->value)
				Com_Printf (0, "sv lowclamp\n");
			svs.realTime = sv.time - 100;
		}
		NET_Sleep (sv.time - svs.realTime);
		return;
	}

	// Update ping based on the last known frame from all clients
	SV_CalcPings ();

	// Give the clients some timeslices
	SV_GiveMsec ();

	// Let everything in the world think and move
	SV_RunGameFrame ();

	// Send messages back to the clients that had packets read this frame
	SV_SendClientMessages ();

	// Save the entire world state if recording a serverdemo
	SV_RecordDemoMessage ();

	// Send a heartbeat to the master if needed
	SV_MasterHeartbeat ();

	// Clear teleport flags, etc for next frame
	SV_PrepWorldFrame ();

}

//============================================================================

/*
================
SV_MasterHeartbeat

Send a message to the master every few minutes to
let it know we are alive, and log information
================
*/
#define HEARTBEAT_SECONDS	300
void SV_MasterHeartbeat (void)
{
	char		*string;
	int			i;

#ifndef DEDICATED_ONLY
	if (!dedicated->integer)
		return;		// Only dedicated servers send heartbeats
#endif

	if (!public_server->value)
		return;		// A private dedicated game

	// Check for time wraparound
	if (svs.lastHeartBeat > svs.realTime)
		svs.lastHeartBeat = svs.realTime;

	if (svs.realTime - svs.lastHeartBeat < HEARTBEAT_SECONDS*1000)
		return;		// not time to send yet

	svs.lastHeartBeat = svs.realTime;

	// Send the same string that we would give for a status OOB command
	string = SV_StatusString ();

	// Send to group master
	for (i=0 ; i<MAX_MASTERS ; i++) {
		if (!sv_masterAddresses[i].port)
			continue;

		Com_Printf (0, "Sending heartbeat to %s\n", NET_AdrToString (sv_masterAddresses[i]));
		Netchan_OutOfBandPrint (NS_SERVER, sv_masterAddresses[i], "heartbeat\n%s", string);
	}
}


/*
=================
SV_MasterShutdown

Informs all masters that this server is going down
=================
*/
static void SV_MasterShutdown (void)
{
	int		i;

#ifndef DEDICATED_ONLY
	if (!dedicated->integer)
		return;		// Only dedicated servers send heartbeats
#endif

	if (!public_server->value)
		return;		// A private dedicated game

	// Send to group master
	for (i=0 ; i<MAX_MASTERS ; i++) {
		if (!sv_masterAddresses[i].port)
			continue;

		if (i > 0)
			Com_Printf (0, "Sending heartbeat to %s\n", NET_AdrToString (sv_masterAddresses[i]));
		Netchan_OutOfBandPrint (NS_SERVER, sv_masterAddresses[i], "shutdown");
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
void SV_UserinfoChanged (svClient_t *cl)
{
	char	*val;
	int		i;

	// Call prog code to allow overrides
	ge->ClientUserinfoChanged (cl->edict, cl->userInfo);
	
	// Name for C code
	strncpy (cl->name, Info_ValueForKey (cl->userInfo, "name"), sizeof (cl->name)-1);
	// Mask off high bit
	for (i=0 ; i<sizeof (cl->name) ; i++)
		cl->name[i] &= 127;

	// Rate command
	val = Info_ValueForKey (cl->userInfo, "rate");
	if (strlen (val)) {
		i = atoi(val);
		cl->rate = clamp (i, 100, 15000);
	}
	else
		cl->rate = 5000;

	// MSG command
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
void SV_FinalMessage (char *message, qBool reconnect)
{
	int			i;
	svClient_t	*cl;
	
	MSG_Clear (&sv_netMessage);
	MSG_WriteByte (&sv_netMessage, SVC_PRINT);
	MSG_WriteByte (&sv_netMessage, PRINT_HIGH);
	MSG_WriteString (&sv_netMessage, message);

	if (reconnect)
		MSG_WriteByte (&sv_netMessage, SVC_RECONNECT);
	else
		MSG_WriteByte (&sv_netMessage, SVC_DISCONNECT);

	// Send it twice
	// stagger the packets to crutch operating system limited buffers
	for (i=0, cl=svs.clients ; i<maxclients->integer ; i++, cl++) {
		if (cl->state >= SVCS_CONNECTED)
			Netchan_Transmit (&cl->netChan, sv_netMessage.curSize, sv_netMessage.data);
	}

	for (i=0, cl=svs.clients ; i<maxclients->integer ; i++, cl++) {
		if (cl->state >= SVCS_CONNECTED)
			Netchan_Transmit (&cl->netChan, sv_netMessage.curSize, sv_netMessage.data);
	}
}


/*
===============
SV_Init

Only called at egl.exe startup, not for each game
===============
*/
void SV_Init (void)
{
	SV_InitOperatorCommands	();
	
	Cvar_Register ("skill",			"1",											0);
	Cvar_Register ("deathmatch",	"0",											CVAR_SERVERINFO|CVAR_LATCH_SERVER);
	Cvar_Register ("coop",			"0",											CVAR_SERVERINFO|CVAR_LATCH_SERVER);
	Cvar_Register ("dmflags",		Q_VarArgs ("%i", DF_INSTANT_ITEMS),				CVAR_SERVERINFO);
	Cvar_Register ("fraglimit",		"0",											CVAR_SERVERINFO);
	Cvar_Register ("timelimit",		"0",											CVAR_SERVERINFO);
	Cvar_Register ("cheats",		"0",											CVAR_SERVERINFO|CVAR_LATCH_SERVER);
	Cvar_Register ("protocol",		Q_VarArgs ("%i", ORIGINAL_PROTOCOL_VERSION),	CVAR_SERVERINFO|CVAR_READONLY);
	Cvar_Register ("mapname",		"",												CVAR_SERVERINFO|CVAR_READONLY);

	rcon_password			= Cvar_Register ("rcon_password",			"",			0);

	maxclients				= Cvar_Register ("maxclients",				"1",		CVAR_SERVERINFO|CVAR_LATCH_SERVER);
	hostname				= Cvar_Register ("hostname",				"noname",	CVAR_SERVERINFO|CVAR_ARCHIVE);
	timeout					= Cvar_Register ("timeout",					"125",		0);
	zombietime				= Cvar_Register ("zombietime",				"2",		0);

	sv_showclamp			= Cvar_Register ("showclamp",				"0",		0);
	sv_paused				= Cvar_Register ("paused",					"0",		CVAR_CHEAT);
	sv_timedemo				= Cvar_Register ("timedemo",				"0",		CVAR_CHEAT);

	sv_enforcetime			= Cvar_Register ("sv_enforcetime",			"0",		0);
	sv_reconnect_limit		= Cvar_Register ("sv_reconnect_limit",		"3",		CVAR_ARCHIVE);
	sv_noreload				= Cvar_Register ("sv_noreload",				"0",		0);
	sv_airaccelerate		= Cvar_Register ("sv_airaccelerate",		"0",		CVAR_LATCH_SERVER);

	allow_download			= Cvar_Register ("allow_download",			"1",		CVAR_ARCHIVE);
	allow_download_players	= Cvar_Register ("allow_download_players",	"0",		CVAR_ARCHIVE);
	allow_download_models	= Cvar_Register ("allow_download_models",	"1",		CVAR_ARCHIVE);
	allow_download_sounds	= Cvar_Register ("allow_download_sounds",	"1",		CVAR_ARCHIVE);
	allow_download_maps		= Cvar_Register ("allow_download_maps",		"1",		CVAR_ARCHIVE);

	public_server			= Cvar_Register ("public",					"0",		0);

	MSG_Init (&sv_netMessage, sv_netBuffer, sizeof (sv_netBuffer));
}


/*
================
SV_Shutdown

Called when each game quits, before Sys_Quit or Sys_Error
================
*/
void SV_Shutdown (char *finalmsg, qBool reconnect, qBool crashing)
{
	if (svs.clients)
		SV_FinalMessage (finalmsg, reconnect);

	SV_MasterShutdown ();

	if (!crashing) {
		SV_GameAPI_Shutdown ();

		// Get latched vars
		Cvar_GetLatchedVars (CVAR_LATCH_SERVER);
	}

	// Free current level
	if (sv.demoFile) {
		FS_CloseFile (sv.demoFile);
		sv.demoFile = 0;
	}
	memset (&sv, 0, sizeof (serverState_t));
	Com_SetServerState (SS_DEAD);

	// Free server static data
	if (svs.clients)
		Mem_Free (svs.clients);
	if (svs.clientEntities)
		Mem_Free (svs.clientEntities);
	if (svs.demoFile)
		FS_CloseFile (svs.demoFile);
	memset (&svs, 0, sizeof (svs));

	CM_UnloadMap ();
}
