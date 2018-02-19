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
// cl_main.c
// Client main loop
//

#include "cl_local.h"

cVar_t *allow_download;
cVar_t *allow_download_players;
cVar_t *allow_download_models;
cVar_t *allow_download_sounds;
cVar_t *allow_download_maps;

cVar_t	*cl_lightlevel;
cVar_t	*cl_downloadToBase;
cVar_t	*cl_maxfps;
cVar_t	*r_maxfps;
cVar_t	*cl_paused;
cVar_t	*cl_protocol;

cVar_t	*cl_shownet;

cVar_t	*cl_stereo_separation;

cVar_t	*cl_timedemo;
cVar_t	*cl_timeout;
cVar_t	*cl_timestamp;

cVar_t	*con_chatHud;
cVar_t	*con_chatHudLines;
cVar_t	*con_chatHudPosX;
cVar_t	*con_chatHudPosY;
cVar_t	*con_chatHudShadow;
cVar_t	*con_notifyfade;
cVar_t	*con_notifylarge;
cVar_t	*con_notifylines;
cVar_t	*con_notifytime;
cVar_t	*con_alpha;
cVar_t	*con_clock;
cVar_t	*con_drop;
cVar_t	*con_scroll;

cVar_t	*freelook;

cVar_t	*lookspring;
cVar_t	*lookstrafe;

cVar_t	*m_pitch;
cVar_t	*m_yaw;
cVar_t	*m_forward;
cVar_t	*m_side;

cVar_t	*rcon_clpassword;
cVar_t	*rcon_address;

cVar_t	*r_fontScale;

cVar_t	*sensitivity;

cVar_t	*scr_conspeed;

clientState_t	cl;
clientStatic_t	cls;
clMedia_t		clMedia;

entityState_t	cl_baseLines[MAX_CS_EDICTS];
entityState_t	cl_parseEntities[MAX_PARSE_ENTITIES];

static netAdr_t	cl_lastRConTo;

struct memPool_s	*cl_cGameSysPool;
struct memPool_s	*cl_cinSysPool;
struct memPool_s	*cl_guiSysPool;
struct memPool_s	*cl_soundSysPool;

//======================================================================

/*
===================
CL_ForwardCmdToServer

Adds the current command line as a CLC_STRINGCMD to the client message.
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
qBool CL_ForwardCmdToServer (void)
{
	char	*cmd;

	cmd = Cmd_Argv (0);
	if (Com_ClientState () <= CA_CONNECTED || *cmd == '-' || *cmd == '+')
		return qFalse;

	MSG_WriteByte (&cls.netChan.message, CLC_STRINGCMD);
	MSG_WriteStringCat (&cls.netChan.message, cmd);
	if (Cmd_Argc () > 1) {
		MSG_WriteStringCat (&cls.netChan.message, " ");
		MSG_WriteStringCat (&cls.netChan.message, Cmd_Args ());
	}

	cls.forcePacket = qTrue;
	return qTrue;
}


/*
=======================
CL_SendConnectPacket

We have gotten a challenge from the server, so try and connect.
======================
*/
static void CL_SendConnectPacket (int useProtocol)
{
	netAdr_t	adr;
	int			port;
	uint32		msgLen;

	if (!NET_StringToAdr (cls.serverName, &adr)) {
		Com_Printf (PRNT_WARNING, "Bad server address\n");
		cls.connectCount = 0;
		cls.connectTime = 0;
		return;
	}

	if (!adr.port)
		adr.port = BigShort (PORT_SERVER);

	port = Cvar_GetIntegerValue ("qport");

	com_userInfoModified = qFalse;

	// Decide on the maximum message length and protocol version to try
	if (Com_ServerState() == SS_DEMO) {
		msgLen = MAX_SV_USABLEMSG;
		cls.serverProtocol = ORIGINAL_PROTOCOL_VERSION;
	}
	else {
		if (!cls.serverProtocol) {
			if (cl_protocol->intVal) {
				// Force the version attempt
				cls.serverProtocol = cl_protocol->intVal;
				Com_Printf (PRNT_WARNING, "WARNING: CL_SendConnectPacket: cl_protocol is for debugging now, if you have trouble connecting set it to 0 to auto-detect (default).\n");
			}
			else if (useProtocol) {
				// Protocol version retrieved from server
				cls.serverProtocol = useProtocol;
			}
			else {
				// Really should never happen
				cls.serverProtocol = ENHANCED_PROTOCOL_VERSION;
			}
		}

		// Enhanced protocol port is a byte
		if (cls.serverProtocol == ENHANCED_PROTOCOL_VERSION) {
			port &= 0xff;

			msgLen = Cvar_GetIntegerValue ("net_maxMsgLen");
			if (msgLen > MAX_CL_USABLEMSG)
				msgLen = MAX_CL_USABLEMSG;
		}
		else {
			msgLen = MAX_SV_USABLEMSG;
		}
	}

	cls.quakePort = port;

	// Send
	Com_DevPrintf (0, "CL_SendConnectPacket: protocol=%d, port=%d, challenge=%u\n", cls.serverProtocol, port, cls.challenge);
	if (cls.serverProtocol == ENHANCED_PROTOCOL_VERSION)
		Netchan_OutOfBandPrint (NS_CLIENT, &adr, "connect %i %i %i \"%s\" %u\n",
			cls.serverProtocol, port, cls.challenge, Cvar_BitInfo (CVAR_USERINFO), msgLen);
	else
		Netchan_OutOfBandPrint (NS_CLIENT, &adr, "connect %i %i %i \"%s\"\n",
			cls.serverProtocol, port, cls.challenge, Cvar_BitInfo (CVAR_USERINFO));
}


/*
=====================
CL_ResetServerCount
=====================
*/
void CL_ResetServerCount (void)
{
	cl.serverCount = -1;
}


/*
=================
CL_SetRefConfig
=================
*/
void CL_SetRefConfig (void)
{
	if (!clMedia.initialized)
		return;

	// Update client
	R_GetRefConfig (&cls.refConfig);

	// Update modules
	CL_CGModule_SetRefConfig ();

	// Re-check console wordwrap size after video size setup
	CL_ConsoleCheckResize ();
}


/*
=====================
CL_SetState
=====================
*/
void CL_SetState (caState_t state)
{
	// Store the state
	Com_SetClientState (state);

	// Apply any state-specific actions
	switch (state) {
	case CA_CONNECTING:
		// FIXME: only call if last state was < CA_CONNECTING?
		CL_CGModule_ForceMenuOff ();
		Snd_StopAllSounds ();
		break;

	case CA_CONNECTED:
		Cvar_FixCheatVars ();
		break;
	}
}


/*
=====================
CL_ClearState
=====================
*/
void CL_ClearState (void)
{
	// Stop sound
	Snd_StopAllSounds ();

	// Wipe the entire cl structure
	memset (&cl, 0, sizeof (cl));
	memset (&cl_baseLines, 0, sizeof (cl_baseLines));
	memset (&cl_parseEntities, 0, sizeof (cl_parseEntities));

	// Set defaults
	cl.maxClients = MAX_CS_CLIENTS;

	// Clear the netChan
	MSG_Clear (&cls.netChan.message);

	// Clear the demo buffer
	MSG_Init (&cl.demoBuffer, cl.demoFrame, sizeof (cl.demoFrame));
	cl.demoBuffer.allowOverflow = qTrue;
}


/*
=====================
CL_Disconnect

Goes from a connected state to full screen console state
Sends a disconnect message to the server
This is also called on Com_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect (qBool openMenu)
{
	byte	final[32];

	if (Com_ClientState () == CA_UNINITIALIZED)
		return;
	if (Com_ClientState () == CA_DISCONNECTED)
		goto done;

	if (cl_timedemo->intVal) {
		int	time;

		Com_Printf (0, "Timedemo complete...\n");
		time = Sys_Milliseconds () - cl.timeDemoStart;
		if (time > 0)
			Com_Printf (0, "%i frames, %3.1f seconds: %3.1f fps\n", cl.timeDemoFrames,
				time/1000.0, cl.timeDemoFrames*1000.0f / time);
	}

	cls.connectCount = 0;
	cls.connectTime = -99999;	// CL_CheckForResend () will fire immediately

	CIN_StopCinematic ();

	if (cls.demoRecording)
		CL_StopDemoRecording ();

	// Send a disconnect message to the server
	final[0] = CLC_STRINGCMD;
	strcpy ((char *)final+1, "disconnect");
	Netchan_Transmit (&cls.netChan, 11, final);
	Netchan_Transmit (&cls.netChan, 11, final);
	Netchan_Transmit (&cls.netChan, 11, final);

	// Stop download
	if (cls.download.file) {
		fclose (cls.download.file);
		cls.download.file = NULL;
	}

done:
	CM_UnloadMap ();
	CL_MediaRestart ();

	CL_ClearState ();
	CL_SetState (CA_DISCONNECTED);

	if (openMenu)
		CL_CGModule_MainMenu ();

#ifdef CL_HTTPDL
	CL_HTTPDL_CancelDownloads (qTrue);
	cls.download.httpReferer[0] = '\0';
#endif

	cls.serverMessage[0] = '\0';
	cls.serverName[0] = '\0';
	cls.serverProtocol = 0;

	Cvar_Set ("$mapname", "", qTrue);
	Cvar_Set ("$game", "", qTrue);

	// Swap games if needed
	Cvar_GetLatchedVars (CVAR_LATCH_SERVER);

	// Drop loading plaque unless this is the initial game start
	SCR_EndLoadingPlaque ();
}

/*
=============================================================================

	CONNECTION-LESS PACKETS

=============================================================================
*/

/*
=================
CL_ClientConnect_CP

Server connection
=================
*/
static void CL_ClientConnect_CP (void)
{
	char		*p;
	char		*buff;
	int			i;

	switch (Com_ClientState()) {
	case CA_CONNECTED:
		Com_DevPrintf (PRNT_WARNING, "Dup connect received. Ignored.\n");
		return;

	case CA_DISCONNECTED:
		Com_DevPrintf (0, "Received connect when disconnected. Ignored.\n");
		return;

	case CA_ACTIVE:
		Com_DevPrintf (0, "Illegal connect when already connected! (q2msgs?). Ignored.\n");
		return;
	}

	Com_DevPrintf (0, "client_connect: new\n");

	Netchan_Setup (NS_CLIENT, &cls.netChan, &cls.netFrom, cls.serverProtocol, cls.quakePort, 0);

	// Parse arguments (R1Q2 enhanced protocol only)
	if (cls.serverProtocol == ENHANCED_PROTOCOL_VERSION) {
		buff = NET_AdrToString (&cls.netChan.remoteAddress);
		for (i=1 ; i<Cmd_Argc() ; i++) {
			p = Cmd_Argv (i);
			if (!strncmp (p, "dlserver=", 9)) {
#ifdef CL_HTTPL
				p += 9;
				Q_snprintfz (cls.download.httpReferer, sizeof (cls.download.httpReferer), "quake2://%s", buff);
				CL_HTTPDL_SetServer (p);
				if (cls.download.httpServer[0])
					Com_DevPrintf (0, "HTTP downloading enabled, using '%s'\n", cls.download.httpServer);
#endif
			}
#ifdef CL_ANTICHEAT
			else if (!strncmp (p, "ac=", 3)) {
				p += 3;
				if (!p[0])
					continue;
				i = atoi (p);
				if (i) {
					MSG_WriteByte (&cls.netChan.message, CLC_NOP);
					Netchan_Transmit (&cls.netChan, 0, NULL);

					// Don't sit and stutter sounds while loading
					Snd_StopAllSounds ();

					// Let the GUI system know
					// FIXME: Use this, durr
					Com_Printf (0, "Loading and initializing the anticheat module, please wait.\n");
					GUI_NamedGlobalEvent ("ACLoading");
					SCR_UpdateScreen ();

					// Load the API
					if (!CL_ACAPI_Init ())
						Com_Printf (PRNT_ERROR, "Anticheat failed to load, trying to connect without it.\n");
				}
			}
#endif
		}
	}

	MSG_WriteChar (&cls.netChan.message, CLC_STRINGCMD);
	MSG_WriteString (&cls.netChan.message, "new");

	Key_SetDest (KD_GAME);
	CL_SetState (CA_CONNECTED);
	cls.forcePacket = qTrue;
}


/*
=================
CL_Info_CP

Server responding to a status broadcast
=================
*/
static void CL_Info_CP (void)
{
	CL_CGModule_ParseServerInfo (NET_AdrToString (&cls.netFrom), MSG_ReadString (&cls.netMessage));
}


/*
=================
CL_Cmd_CP

Remote command from gui front end
=================
*/
static void CL_Cmd_CP (void)
{
	char	*s;

	if (!NET_IsLocalAddress (cls.netFrom)) {
		Com_Printf (PRNT_WARNING, "Command packet from remote host. Ignored.\n");
		return;
	}
	Sys_AppActivate ();
	s = MSG_ReadString (&cls.netMessage);
	Cbuf_AddText (s);
	Cbuf_AddText ("\n");
}


/*
=================
CL_Ping_CP

Ping from server
=================
*/
static void CL_Ping_CP (void)
{
	// R1: stop users from exploiting your connecting, this can bog down even cable users!
	//Netchan_OutOfBandPrint (NS_CLIENT, &cls.netFrom, "ack");
}


/*
=================
CL_Challenge_CP

Challenge from the server we are connecting to
=================
*/
static void CL_Challenge_CP (void)
{
	char	*p;
	int		protocol, i;

	// Local challenge
	cls.challenge = atoi (Cmd_Argv (1));
	Com_DevPrintf (0, "client: received challenge\n");

	// Flags
	protocol = ORIGINAL_PROTOCOL_VERSION;
	for (i=2 ; i<Cmd_Argc() ; i++) {
		p = Cmd_Argv (i);
		if (!strncmp (p, "p=", 2)) {
			p += 2;
			if (!p[0])
				continue;
			for ( ; ; ) {
				i = atoi (p);
				if (i == ENHANCED_PROTOCOL_VERSION)
					protocol = i;
				p = strchr (p, ',');
				if (!p)
					break;
				p++;
				if (!p[0])
					break;
			}
		}
	}

	// Reset timer to avoid duplicates, and send the connect packet
	cls.connectTime = cls.realTime;
	CL_SendConnectPacket (protocol);
}


/*
=================
CL_Echo_CP

Echo request from server
=================
*/
static void CL_Echo_CP (void)
{
	Netchan_OutOfBandPrint (NS_CLIENT, &cls.netFrom, "%s", Cmd_Argv (1));
}


/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
static void CL_ConnectionlessPacket (void)
{
	char		*cmd, *printStr;
	netAdr_t	remote;
	qBool		isPrint;

	MSG_BeginReading (&cls.netMessage);
	assert (MSG_ReadLong (&cls.netMessage) == -1);	// Skip the -1

	cmd = MSG_ReadStringLine (&cls.netMessage);
	Cmd_TokenizeString (cmd, qFalse);

	cmd = Cmd_Argv (0);

	if (!strcmp (cmd, "client_connect")) {
		Com_Printf (0, "%s: %s\n", NET_AdrToString (&cls.netFrom), cmd);
		CL_ClientConnect_CP ();
		return;
	}
	else if (!strcmp (cmd, "info")) {
		CL_Info_CP ();
		return;
	}
	else if (!strcmp (cmd, "print")) {
		// Print command from somewhere
		isPrint = qTrue;
		printStr = MSG_ReadString (&cls.netMessage);
		if (CL_CGModule_ParseServerStatus (NET_AdrToString (&cls.netFrom), printStr))
			return;
		Q_strncpyz (cls.serverMessage, printStr, sizeof (cls.serverMessage));
	}
	else {
		printStr = NULL;
		isPrint = qFalse;
	}

	// Only allow the following from current connected server and last destination client sent an rcon to
	NET_StringToAdr (cls.serverName, &remote);
	if (!NET_CompareBaseAdr (cls.netFrom, remote) && !NET_CompareBaseAdr (cls.netFrom, cl_lastRConTo)) {
		Com_Printf (PRNT_WARNING, "Illegal %s from %s. Ignored.\n", cmd, NET_AdrToString (&cls.netFrom));
		return;
	}

	Com_Printf (0, "%s: %s\n", NET_AdrToString (&cls.netFrom), cmd);

	if (isPrint)
		Com_Printf (0, "%s", printStr);
	else if (!strcmp (cmd, "cmd"))
		CL_Cmd_CP ();
	else if (!strcmp (cmd, "ping"))
		CL_Ping_CP ();
	else if (!strcmp (cmd, "challenge"))
		CL_Challenge_CP ();
	else if (!strcmp (cmd, "echo"))
		CL_Echo_CP ();
	else
		Com_Printf (PRNT_WARNING, "CL_ConnectionlessPacket: Unknown command.\n");
}

/*
=============================================================================

	MEDIA

=============================================================================
*/

/*
=================
CL_ImageMediaInit
=================
*/
void CL_ImageMediaInit (void)
{
	clMedia.cinMaterial			= R_RegisterPic ("***r_cinTexture***");
	clMedia.consoleMaterial		= R_RegisterPic ("pics/conback.tga");
	clMedia.whiteTexture		= R_RegisterPic ("***r_whiteTexture***");
	clMedia.blackTexture		= R_RegisterPic ("***r_blackTexture***");
}


/*
=================
CL_SoundMediaInit
=================
*/
void CL_SoundMediaInit (void)
{
	int		i;

	// Register local sounds
	clMedia.talkSfx = Snd_RegisterSound ("misc/talk.wav");
	for (i=1 ; i<MAX_CS_SOUNDS ; i++) {
		if (!cl.configStrings[CS_SOUNDS+i][0]) {
			cl.soundCfgStrings[i] = NULL;
			break;
		}

		cl.soundCfgStrings[i] = Snd_RegisterSound (cl.configStrings[CS_SOUNDS+i]);
		Sys_SendKeyEvents ();	// pump message loop
	}

	// Update CGame
	CL_CGModule_RegisterSounds ();

	// Register GUI media
	GUI_RegisterSounds ();
}


/*
=================
CL_MediaInit
=================
*/
void CL_MediaInit (void)
{
	if (clMedia.initialized)
		return;
	if (Com_ClientState () == CA_UNINITIALIZED)
		return;

	clMedia.initialized = qTrue;

	// Free all sounds
	Snd_FreeSounds ();

	// Init CGame api
	CL_CGameAPI_Init ();

	// Register our media
	CL_ImageMediaInit ();
	CL_SoundMediaInit ();

	// Check memory integrity
	Mem_CheckGlobalIntegrity ();

	// Touch memory
	Mem_TouchGlobal ();
}


/*
=================
CL_MediaShutdown
=================
*/
void CL_MediaShutdown (void)
{
	if (!clMedia.initialized)
		return;

	clMedia.initialized = qFalse;

	// Shutdown CGame
	CL_CGameAPI_Shutdown ();

	// Stop and free all sounds
	Snd_StopAllSounds ();
	Snd_FreeSounds ();

	// Touch memory
	Mem_TouchGlobal ();
}


/*
=================
CL_MediaRestart
=================
*/
void CL_MediaRestart (void)
{
	CL_MediaShutdown ();
	CL_MediaInit ();
}

/*
=============================================================================

	FRAME HANDLING

=============================================================================
*/

/*
==================
CL_ForcePacket
==================
*/
void CL_ForcePacket (void)
{
	cls.forcePacket = qTrue;
}


/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
static void CL_CheckForResend (void)
{
	netAdr_t	adr;

	// If the local server is running and we aren't then connect
	if (Com_ClientState() == CA_DISCONNECTED && Com_ServerState() >= SS_GAME) {
		CL_SetState (CA_CONNECTING);
		Q_strncpyz (cls.serverName, "localhost", sizeof (cls.serverName));

		// We don't need a challenge on the localhost
		CL_SendConnectPacket (ORIGINAL_PROTOCOL_VERSION);
		return;
	}

	// Resend if we haven't gotten a reply yet
	if (Com_ClientState () != CA_CONNECTING)
		return;

	// Only resend every NET_RETRYDELAY
	if (cls.realTime - cls.connectTime < NET_RETRYDELAY)
		return;

	// Check if it's a bad server address
	if (!NET_StringToAdr (cls.serverName, &adr)) {
		Com_Printf (PRNT_WARNING, "Bad server address\n");
		CL_SetState (CA_DISCONNECTED);
		CL_CGModule_MainMenu ();
		return;
	}

	if (!adr.port)
		adr.port = BigShort (PORT_SERVER);

	cls.connectCount++;
	cls.connectTime = cls.realTime;	// For retransmit requests

	Com_Printf (0, "Connecting to %s...\n", cls.serverName);

	Netchan_OutOfBandPrint (NS_CLIENT, &adr, "getchallenge\n");
}


/*
=================
CL_ReadPackets
=================
*/
static void CL_ReadPackets (void)
{
	while (NET_GetPacket (NS_CLIENT, &cls.netFrom, &cls.netMessage)) {
		// Remote command packet
		if (*(int *)cls.netMessage.data == -1) {
			CL_ConnectionlessPacket ();
			continue;
		}

		if (Com_ClientState () <= CA_CONNECTING)
			continue;		// Dump it if not connected

		if (cls.netMessage.curSize < 8) {
			Com_DevPrintf (PRNT_WARNING, "%s: Runt packet\n", NET_AdrToString (&cls.netFrom));
			continue;
		}

		// Packet from server
		if (!NET_CompareAdr (cls.netFrom, cls.netChan.remoteAddress)) {
			Com_DevPrintf (PRNT_WARNING, "%s: sequenced packet without connection\n", NET_AdrToString (&cls.netFrom));
			continue;
		}

		if (!Netchan_Process (&cls.netChan, &cls.netMessage))
			continue;	// Shunned for some reason

		// Force a packet update if a reliable was recieved
		if (cls.netChan.gotReliable && Com_ClientState() == CA_CONNECTED)
			cls.forcePacket = qTrue;

		CL_ParseServerMessage ();

		// We don't know if it is ok to save a demo message until after we have parsed the frame
		if (cls.demoRecording && !cls.demoWaiting && cls.serverProtocol == ORIGINAL_PROTOCOL_VERSION)
			CL_WriteDemoMessageFull ();
	}

	// Check timeout
	if (Com_ClientState() >= CA_CONNECTED && cls.realTime-cls.netChan.lastReceived > cl_timeout->floatVal*1000) {
		if (++cl.timeOutCount > 5) {
			// Timeoutcount saves debugger
			Com_Printf (PRNT_WARNING, "Server connection timed out.\n");
			CL_Disconnect (qTrue);
			return;
		}
	}
	else
		cl.timeOutCount = 0;
}


/*
==================
CL_SendCommand
==================
*/
static void CL_SendCommand (void)
{
	// Send client packet to server
	CL_SendCmd ();

	// Resend a connection request if necessary
	CL_CheckForResend ();
}


/*
==================
CL_RefreshInputs
==================
*/
static void CL_RefreshInputs (void)
{
	// Process new key events
	Sys_SendKeyEvents ();

	// Process mice & joystick events
	IN_Commands ();

	// Process console commands
	Cbuf_Execute ();

	// Process packets from server
	CL_ReadPackets ();

	// Update usercmd state
	CL_RefreshCmd ();
}


/*
==================
CL_Frame
==================
*/
#define FRAMETIME_MAX 0.5f
void CL_Frame (int msec)
{
	static int	packetDelta = 0;
	static int	refreshDelta = 0;
	static int	miscDelta = 1000;
	qBool		packetFrame = qTrue;
	qBool		refreshFrame = qTrue;
	qBool		miscFrame = qTrue;

	// Internal counters
	packetDelta += msec;
	refreshDelta += msec;
	miscDelta += msec;

	// Frame counters
	cls.netFrameTime = packetDelta * 0.001f;
	cls.trueNetFrameTime = cls.netFrameTime;
	cls.refreshFrameTime = refreshDelta * 0.001f;
	cls.trueRefreshFrameTime = cls.refreshFrameTime;
	cls.realTime = Sys_Milliseconds ();

	// If in the debugger last frame, don't timeout
	if (msec > 5000)
		cls.netChan.lastReceived = Sys_Milliseconds ();

	// Don't extrapolate too far ahead
	if (cls.netFrameTime > FRAMETIME_MAX)
		cls.netFrameTime = FRAMETIME_MAX;
	if (cls.refreshFrameTime > FRAMETIME_MAX)
		cls.refreshFrameTime = FRAMETIME_MAX;

	if (!cl_timedemo->intVal) {
		// Packet transmission rate is too high
		if (packetDelta < 1000.0f/cl_maxfps->floatVal)
			packetFrame = qFalse;
		// This happens when your video framerate plumits to near-below net 'framerate'
		else if (cls.netFrameTime == cls.refreshFrameTime)
			packetFrame = qFalse;

		// Video framerate is too high
		if (refreshDelta < 1000.0f/r_maxfps->floatVal)
			refreshFrame = qFalse;
	
		// Don't need to do this stuff much (10FPS)
		if (miscDelta < 1000/10)
			miscFrame = qFalse;
	}

	// Don't flood packets out while connecting (10FPS)
	if (Com_ClientState () == CA_CONNECTED) {
		if (packetDelta < 1000/10)
			packetFrame = qFalse;

#ifdef CL_HTTPDL
		CL_HTTPDL_RunDownloads ();
#endif
	}

	// Update the inputs (keyboard, mouse, server, etc)
	CL_RefreshInputs ();

	// Send commands to the server
	if (cls.forcePacket || com_userInfoModified) {
		if (Com_ClientState () < CA_CONNECTED)
			com_userInfoModified = qFalse;
		else
			packetFrame = qTrue;
		cls.forcePacket = qFalse;
	}

	if (packetFrame) {
		packetDelta = 0;
		CL_SendCommand ();

#ifdef CL_HTTPDL
		CL_HTTPDL_RunDownloads ();
#endif
	}

	// Render the display
	if (refreshFrame) {
		refreshDelta = 0;

		// Stuff that does not need to happen a lot
		if (miscFrame) {
			miscDelta = 0;

			// Let the mouse activate or deactivate
			IN_Frame ();

			// Queued video restart
			VID_CheckChanges (&cls.refConfig);

			// Queued audio restart
			Snd_CheckChanges ();
		}

		// Update the screen
		SCR_UpdateScreen ();

		// Advance local effects for next frame
		CIN_RunCinematic ();

		// Update audio orientation
		if (Com_ClientState () != CA_ACTIVE || cl.cin.time > 0)
			Snd_Update (NULL);

		if (miscFrame)
			CDAudio_Update ();
	}
}

/*
=============================================================================

	CONSOLE FUNCTIONS

=============================================================================
*/

/*
=================
CL_Changing_f

Just sent as a hint to the client that they should
drop to full console
=================
*/
static void CL_Changing_f (void)
{
	// If we are downloading, we don't change! This so we don't suddenly stop downloading a map
	if (cls.download.file)
		return;

	if (cls.demoRecording)
		CL_StopDemoRecording ();

	// Update the screen
	SCR_UpdateScreen ();

	// Prevent manual issuing crashing game
	if (Com_ClientState() < CA_CONNECTED)
		return;

	cls.connectCount = 0;

	SCR_BeginLoadingPlaque ();
	CL_SetState (CA_CONNECTED);	// Not active anymore, but not disconnected
	Com_Printf (0, "\nChanging map...\n");
}


/*
================
CL_Connect_f
================
*/
static void CL_Connect_f (void)
{
	char		*server;
	netAdr_t	adr;

	if (Cmd_Argc() != 2) {
		Com_Printf (0, "usage: connect <server>\n");
		return;	
	}
	
	if (Com_ServerState()) {
		// If running a local server, kill it and reissue
		SV_ServerShutdown ("Server quit\n", qFalse, qFalse);
	}
	server = Cmd_Argv (1);

	NET_Config (NET_CLIENT);

	// Validate the address
	if (!NET_StringToAdr (server, &adr)) {
		Com_Printf (PRNT_WARNING, "Bad server address: %s\n", server);
		return;
	}

	if (!adr.port)
		adr.port = BigShort (PORT_SERVER);

	// Disconnect if connected
	CL_Disconnect (qFalse);

	// Reset the protocol attempt if we're connecting to a different server
	if (!NET_CompareAdr (adr, cls.netChan.remoteAddress)) {
		Com_DevPrintf (0, "Resetting protocol attempt since %s", NET_AdrToString (&adr));
		Com_DevPrintf (0, " is not %s.\n", NET_AdrToString (&cls.netChan.remoteAddress));
		cls.serverProtocol = 0;
	}

	// Set the client state and prepare to connect
	CL_SetState (CA_CONNECTING);
	cls.serverMessage[0] = '\0';
	Q_strncpyz (cls.serverName, server, sizeof (cls.serverName));
	Q_strncpyz (cls.serverNameLast, server, sizeof (cls.serverNameLast));

	cls.connectTime = -99999;	// CL_CheckForResend () will fire immediately
	cls.connectCount = 0;
}


/*
=================
CL_Disconnect_f
=================
*/
static void CL_Disconnect_f (void)
{
	if (Com_ClientState () == CA_DISCONNECTED)
		return;

	cls.serverProtocol = 0;
	Com_Error (ERR_DROP, "Disconnected from server");
}


/*
===============
CL_Download_f

Request a download from the server
===============
*/
static void CL_Download_f (void)
{
	char	*fileName;

	if (Cmd_Argc () != 2) {
		Com_Printf (0, "Usage: download <filename>\n");
		return;
	}

	if (Com_ClientState() < CA_CONNECTED) {
		Com_Printf (PRNT_WARNING, "Not connected.\n");
		return;
	}

	// It exists, no need to download
	fileName = Cmd_Argv (1);
	if (FS_FileExists (fileName) != -1) {
		Com_Printf (PRNT_WARNING, "File already exists.\n");
		return;
	}

	CL_CheckOrDownloadFile (fileName);
}


/*
==================
CL_ForwardToServer_f
==================
*/
static void CL_ForwardToServer_f (void)
{
	if (Com_ClientState() < CA_CONNECTED) {
		Com_Printf (0, "Can't \"%s\", not connected\n", Cmd_Argv (0));
		return;
	}
	
	// Don't forward the first argument
	if (Cmd_Argc () > 1) {
		MSG_WriteByte (&cls.netChan.message, CLC_STRINGCMD);
		MSG_WriteStringCat (&cls.netChan.message, Cmd_Args ());
		cls.forcePacket = qTrue;
	}
}


/*
==================
CL_Pause_f
==================
*/
static void CL_Pause_f (void)
{
	// Never pause in multiplayer
	if (Cvar_GetFloatValue ("maxclients") > 1 || !Com_ServerState ()) {
		Cvar_SetValue ("paused", 0, qFalse);
		return;
	}

	Cvar_SetValue ("paused", !cl_paused->intVal, qFalse);
}


/*
=================
CL_PingLocal_f
=================
*/
static void CL_PingLocal_f (void)
{
	netAdr_t	adr;

	NET_Config (NET_CLIENT);

	// Send a broadcast packet
	Com_Printf (0, "info pinging broadcast...\n");

	adr.naType = NA_BROADCAST;
	adr.port = BigShort (PORT_SERVER);
	Netchan_OutOfBandPrint (NS_CLIENT, &adr, Q_VarArgs ("info %i", ORIGINAL_PROTOCOL_VERSION));
}


/*
=================
CL_PingServer_f
=================
*/
static void CL_PingServer_f (void)
{
	netAdr_t	adr;
	char		*adrString;

	if (Cmd_Argc() != 2) {
		Com_Printf (0, "usage: pingserver <address>[:port]\n");
		return;
	}

	NET_Config (NET_CLIENT);

	adrString = Cmd_Argv (1);
	if (!adrString || !adrString[0])
		return;

	Com_Printf (0, "pinging %s...\n", adrString);

	if (!NET_StringToAdr (adrString, &adr)) {
		Com_Printf (PRNT_WARNING, "Bad address: %s\n", adrString);
		return;
	}

	if (!adr.port)
		adr.port = BigShort (PORT_SERVER);

	Netchan_OutOfBandPrint (NS_CLIENT, &adr, Q_VarArgs ("info %i", ORIGINAL_PROTOCOL_VERSION));
}


/*
=================
CL_Precache_f

The server will send this command right
before allowing the client into the server
=================
*/
static void CL_Precache_f (void)
{
	// Yet another hack to let old demos work the old precache sequence
	if (Cmd_Argc () < 2) {
		uint32	mapCheckSum;	// For detecting cheater maps

		CM_LoadMap (cl.configStrings[CS_MODELS+1], qTrue, &mapCheckSum);
		CL_CGModule_LoadMap ();
		return;
	}

	CL_ResetDownload ();
	CL_RequestNextDownload ();
}


/*
==================
CL_Quit_f
==================
*/
static void CL_Quit_f (void)
{
	CL_Disconnect (qFalse);
	Com_Quit (qFalse);
}


/*
=====================
CL_Rcon_f

Send the rest of the command line over as an unconnected command.
=====================
*/
static void CL_Rcon_f (void)
{
	char		message[1024];
	int			i;
	netAdr_t	to;

	if (!rcon_clpassword->string) {
		Com_Printf (0, "You must set 'rcon_password' before\n" "issuing an rcon command.\n");
		return;
	}

	// R1: check buffer size
	if ((strlen(Cmd_Args())+strlen(rcon_clpassword->string)+16) >= sizeof(message)) {
		Com_Printf (0, "Length of password + command exceeds maximum allowed length.\n");
		return;
	}

	message[0] = (char)255;
	message[1] = (char)255;
	message[2] = (char)255;
	message[3] = (char)255;
	message[4] = 0;

	NET_Config (NET_CLIENT);

	Q_strcatz (message, "rcon ", sizeof (message));

	Q_strcatz (message, rcon_clpassword->string, sizeof (message));
	Q_strcatz (message, " ", sizeof (message));

	for (i=1 ; i<Cmd_Argc () ; i++) {
		Q_strcatz (message, Cmd_Argv (i), sizeof (message));
		Q_strcatz (message, " ", sizeof (message));
	}

	if (Com_ClientState () >= CA_CONNECTED) {
		to = cls.netChan.remoteAddress;
		cl_lastRConTo = cls.netChan.remoteAddress;
	}
	else {
		if (!strlen (rcon_address->string)) {
			Com_Printf (0, "You must either be connected,\n"
									"or set the 'rcon_address' cvar\n"
									"to issue rcon commands\n");

			return;
		}

		NET_StringToAdr (rcon_address->string, &to);
		NET_StringToAdr (rcon_address->string, &cl_lastRConTo);

		if (to.port == 0) {
			to.port = BigShort (PORT_SERVER);
			cl_lastRConTo.port = to.port;
		}
	}

	NET_SendPacket (NS_CLIENT, strlen(message)+1, message, &to);
}


/*
=================
CL_Reconnect_f

The server is changing levels, or the user input the command.
=================
*/
void CL_Reconnect_f (void)
{
	// If we are downloading, we don't change! This so we don't suddenly stop downloading a map
	if (cls.download.file)
		return;

	cls.connectCount = 0;

	Snd_StopAllSounds ();

	if (Com_ClientState () == CA_CONNECTED) {
		Com_Printf (0, "reconnecting (soft)...\n");
		MSG_WriteChar (&cls.netChan.message, CLC_STRINGCMD);
		MSG_WriteString (&cls.netChan.message, "new");
		cls.forcePacket = qTrue;
		return;
	}

	if (cls.serverName[0]) {
		if (Com_ClientState () >= CA_CONNECTED) {
			Com_Printf (0, "Disconnecting...\n");
			Q_strncpyz (cls.serverName, cls.serverNameLast, sizeof (cls.serverName));
			CL_Disconnect (qFalse);
			cls.connectTime = cls.realTime - (NET_RETRYDELAY*0.5f);
		}
		else {
			cls.connectTime = -99999; // CL_CheckForResend () will fire immediately
		}

		CL_SetState (CA_CONNECTING);
	}

	if (cls.serverNameLast[0]) {
		cls.connectTime = -99999; // CL_CheckForResend () will fire immediately
		CL_SetState (CA_CONNECTING);
		Com_Printf (0, "reconnecting (hard)...\n");
		Q_strncpyz (cls.serverName, cls.serverNameLast, sizeof (cls.serverName));
	}
	else {
		Com_Printf (0, "No server to reconnect to...\n");
	}
}


/*
====================
CL_Record_f

record <demoname>

Begins recording a demo from the current position
====================
*/
static void CL_Record_f (void)
{
	char			name[MAX_OSPATH];

	if (Cmd_Argc () != 2) {
		Com_Printf (0, "record <demoname>\n");
		return;
	}

	if (cls.demoRecording) {
		Com_Printf (0, "Already recording.\n");
		return;
	}

	if (Com_ClientState () != CA_ACTIVE) {
		Com_Printf (0, "You must be in a level to record.\n");
		return;
	}

	if (cl.attractLoop) {
		Com_Printf (0, "Unable to record from a demo stream due to insufficient deltas.\n");
		return;
	}

	if (strstr (Cmd_Argv(1), "..") || strchr (Cmd_Argv(1), '/') || strchr (Cmd_Argv(1), '\\')) {
		Com_Printf (0, "Illegal filename.\n");
		return;
	}

	// Start recording
	Q_snprintfz (name, sizeof (name), "demos/%s.dm2", Cmd_Argv (1));
	if (CL_StartDemoRecording (name)) {
		Com_Printf (0, "recording to %s.\n", name);
	}
	else {
		Com_Printf (PRNT_ERROR, "CL_Record_f: couldn't open.\n");
	}
}


/*
===================
CL_Setenv_f
===================
*/
static void CL_Setenv_f (void)
{
	int argc = Cmd_Argc ();

	if (argc > 2) {
		char buffer[1000];
		int i;

		Q_strncpyz (buffer, Cmd_Argv (1), sizeof (buffer)-1);
		Q_strcatz (buffer, "=", sizeof (buffer));

		for (i=2 ; i<argc ; i++) {
			Q_strcatz (buffer, Cmd_Argv (i), sizeof (buffer));
			Q_strcatz (buffer, " ", sizeof (buffer));
		}

		putenv (buffer);
	}
	else if (argc == 2) {
		char *env = getenv (Cmd_Argv (1));

		if (env)
			Com_Printf (0, "\"%s\" is: \"%s\"\n", Cmd_Argv (1), env);
		else
			Com_Printf (0, "\"%s\" is undefined\n", Cmd_Argv (1), env);
	}
}


/*
===================
CL_ServerStatus_f
===================
*/
static void CL_ServerStatus_f (void)
{
	char		message[16];
	netAdr_t	adr;

	if (Cmd_Argc() != 2) {
		Com_Printf (0, "usage: sstatus <address>[:port]\n");
		return;
	}

	memset (&message, 0, sizeof (message));
	message[0] = (char)255;
	message[1] = (char)255;
	message[2] = (char)255;
	message[3] = (char)255;
	message[4] = (char)0;
	Q_strcatz (message, "status", sizeof (message));

	NET_Config (NET_CLIENT);

	NET_StringToAdr (Cmd_Argv (1), &adr);

	if (!adr.port)
		adr.port = BigShort (PORT_SERVER);

	NET_SendPacket (NS_CLIENT, strlen(message)+1, message, &adr);
}


/*
===================
CL_StatusLocal_f
===================
*/
static void CL_StatusLocal_f (void)
{
	char		message[16];
	int		messageLen;
	netAdr_t	adr;

	memset (&message, 0, sizeof (message));
	message[0] = (char)255;
	message[1] = (char)255;
	message[2] = (char)255;
	message[3] = (char)255;
	message[4] = (char)0;
	Q_strcatz (message, "status", sizeof (message));
	messageLen = (int)strlen (message) + 1;

	NET_Config (NET_CLIENT);

	// Send a broadcast packet
	Com_Printf (0, "status pinging broadcast...\n");

	adr.naType = NA_BROADCAST;
	adr.port = BigShort (PORT_SERVER);
	Netchan_OutOfBand (NS_CLIENT, &adr, messageLen, (byte *)message);
}


/*
====================
CL_Stop_f

Stop recording a demo
====================
*/
static void CL_Stop_f (void)
{
	size_t		len;

	if (!cls.demoRecording) {
		Com_Printf (0, "Not recording a demo.\n");
		return;
	}

	len = FS_FileLength (cls.demoFile);

	CL_StopDemoRecording ();

	Com_Printf (0, "Stopped recording, wrote %d bytes\n", len);
}


/*
==============
CL_Userinfo_f
==============
*/
static void CL_Userinfo_f (void)
{
	Com_Printf (0, "User info settings:\n");
	Info_Print (Cvar_BitInfo (CVAR_USERINFO));
}

/*
=============================================================================

	INIT / SHUTDOWN

=============================================================================
*/

/*
==================
CL_WriteConfig
==================
*/
static void CL_WriteConfig (void)
{
	FILE	*f;
	char	path[MAX_QPATH];

	Cvar_GetLatchedVars (CVAR_LATCH_AUDIO|CVAR_LATCH_SERVER|CVAR_LATCH_VIDEO|CVAR_RESET_GAMEDIR);

	Com_Printf (0, "Saving configuration...\n");

	if (Cmd_Argc () == 2)
		Q_snprintfz (path, sizeof (path), "%s/%s", FS_Gamedir (), Cmd_Argv (1));
	else
		Q_snprintfz (path, sizeof (path), "%s/eglcfg.cfg", FS_Gamedir ());

	f = fopen (path, "wt");
	if (!f) {
		Com_Printf (PRNT_ERROR, "ERROR: Couldn't write %s\n", path);
		return;
	}

	fprintf (f, "//\n// configuration file generated by EGL, modify at risk\n//\n");

	fprintf (f, "\n// key bindings\n");
	Key_WriteBindings (f);

	fprintf (f, "\n// console variables\n");
	Cvar_WriteVariables (f);

	fprintf (f, "\n\0");
	fclose (f);
	Com_Printf (0, "Saved to %s\n", path);
}


/*
=================
CL_Register
=================
*/
static void CL_Register (void)
{
	// Register our variables
	allow_download			= Cvar_Register ("allow_download",			"1",	CVAR_ARCHIVE);
	allow_download_players	= Cvar_Register ("allow_download_players",	"0",	CVAR_ARCHIVE);
	allow_download_models	= Cvar_Register ("allow_download_models",	"1",	CVAR_ARCHIVE);
	allow_download_sounds	= Cvar_Register ("allow_download_sounds",	"1",	CVAR_ARCHIVE);
	allow_download_maps		= Cvar_Register ("allow_download_maps",		"1",	CVAR_ARCHIVE);

	cl_downloadToBase		= Cvar_Register ("cl_downloadToBase",		"0",		CVAR_ARCHIVE);
	cl_lightlevel			= Cvar_Register ("r_lightlevel",			"0",		0);
	cl_maxfps				= Cvar_Register ("cl_maxfps",				"30",		0);
	r_maxfps				= Cvar_Register ("r_maxfps",				"500",		0);
	cl_paused				= Cvar_Register ("paused",					"0",		CVAR_CHEAT);
	cl_protocol				= Cvar_Register ("cl_protocol",				"0",		0);

	cl_shownet				= Cvar_Register ("cl_shownet",				"0",		0);

	cl_stereo_separation	= Cvar_Register ("cl_stereo_separation",	"0.4",		CVAR_ARCHIVE);

	cl_timedemo				= Cvar_Register ("timedemo",				"0",		CVAR_CHEAT);
	cl_timeout				= Cvar_Register ("cl_timeout",				"120",		0);
	cl_timestamp			= Cvar_Register ("cl_timestamp",			"0",		CVAR_ARCHIVE);

	con_chatHud				= Cvar_Register ("con_chatHud",				"0",		CVAR_ARCHIVE);
	con_chatHudLines		= Cvar_Register ("con_chatHudLines",		"4",		CVAR_ARCHIVE);
	con_chatHudPosX			= Cvar_Register ("con_chatHudPosX",			"8",		CVAR_ARCHIVE);
	con_chatHudPosY			= Cvar_Register ("con_chatHudPosY",			"-14",		CVAR_ARCHIVE);
	con_chatHudShadow		= Cvar_Register ("con_chatHudShadow",		"0",		CVAR_ARCHIVE);
	con_notifyfade			= Cvar_Register ("con_notifyfade",			"1",		CVAR_ARCHIVE);
	con_notifylarge			= Cvar_Register ("con_notifylarge",			"0",		CVAR_ARCHIVE);
	con_notifylines			= Cvar_Register ("con_notifylines",			"4",		CVAR_ARCHIVE);
	con_notifytime			= Cvar_Register ("con_notifytime",			"3",		CVAR_ARCHIVE);
	con_alpha				= Cvar_Register ("con_alpha",				"1",		CVAR_ARCHIVE);
	con_clock				= Cvar_Register ("con_clock",				"1",		CVAR_ARCHIVE);
	con_drop				= Cvar_Register ("con_drop",				"0.5",		CVAR_ARCHIVE);
	con_scroll				= Cvar_Register ("con_scroll",				"2",		CVAR_ARCHIVE);

	freelook				= Cvar_Register ("freelook",				"0",		CVAR_ARCHIVE);

	lookspring				= Cvar_Register ("lookspring",				"0",		CVAR_ARCHIVE);
	lookstrafe				= Cvar_Register ("lookstrafe",				"0",		CVAR_ARCHIVE);

	m_forward				= Cvar_Register ("m_forward",				"1",		0);
	m_pitch					= Cvar_Register ("m_pitch",					"0.022",	CVAR_ARCHIVE);
	m_side					= Cvar_Register ("m_side",					"1",		0);
	m_yaw					= Cvar_Register ("m_yaw",					"0.022",	0);

	rcon_clpassword			= Cvar_Register ("rcon_password",			"",			0);
	rcon_address			= Cvar_Register ("rcon_address",			"",			0);

	r_fontScale				= Cvar_Register ("r_fontScale",				"1",		CVAR_ARCHIVE);

	sensitivity				= Cvar_Register ("sensitivity",				"3",		CVAR_ARCHIVE);

	scr_conspeed			= Cvar_Register ("scr_conspeed",			"3",		0);

	Cmd_AddCommand ("changing",			CL_Changing_f,			"");
	Cmd_AddCommand ("connect",			CL_Connect_f,			"Connects to a server");
	Cmd_AddCommand ("cmd",				CL_ForwardToServer_f,	"Forwards a command to the server");
	Cmd_AddCommand ("disconnect",		CL_Disconnect_f,		"Disconnects from the current server");
	Cmd_AddCommand ("download",			CL_Download_f,			"Manually download a file from the server");
	Cmd_AddCommand ("exit",				CL_Quit_f,				"Exits");
	Cmd_AddCommand ("pause",			CL_Pause_f,				"Pauses the game");
	Cmd_AddCommand ("pingserver",		CL_PingServer_f,		"Sends an info request packet to a server");
	Cmd_AddCommand ("pinglocal",		CL_PingLocal_f,			"Queries broadcast for an info string");
	Cmd_AddCommand ("precache",			CL_Precache_f,			"");
	Cmd_AddCommand ("quit",				CL_Quit_f,				"Exits");
	Cmd_AddCommand ("rcon",				CL_Rcon_f,				"");
	Cmd_AddCommand ("reconnect",		CL_Reconnect_f,			"Reconnect to the current server");
	Cmd_AddCommand ("record",			CL_Record_f,			"Start recording a demo");
	Cmd_AddCommand ("savecfg",			CL_WriteConfig,			"Saves current settings to the configuration file");
	Cmd_AddCommand ("setenv",			CL_Setenv_f,			"");
	Cmd_AddCommand ("statuslocal",		CL_StatusLocal_f,		"Queries broadcast for a status string");
	Cmd_AddCommand ("sstatus",			CL_ServerStatus_f,		"Sends a status request packet to a server");
	Cmd_AddCommand ("stop",				CL_Stop_f,				"Stop recording a demo");
	Cmd_AddCommand ("userinfo",			CL_Userinfo_f,			"");

#if defined(CL_ANTICHEAT) && defined(_DEBUG)
	Cmd_AddCommand ("initanticheat",	CL_ACAPI_Init,			"");
#endif // CL_ANTICHEAT && _DEBUG
}


/*
====================
CL_ClientInit
====================
*/
void CL_ClientInit (void)
{
	int		i;

	// Fill with strings so we're not if'ing every time it's used
	for (i=SVC_MAX ; i<256 ; i++)
		cl_svcStrings[i] = cl_svcStrings[0];

	// All archived variables will now be loaded
	CL_Register ();

	// Create memory pools
	cl_cGameSysPool = Mem_CreatePool ("Client: CGame system");
	cl_cinSysPool = Mem_CreatePool ("Client: Cinematic system");
	cl_guiSysPool = Mem_CreatePool ("Client: GUI system");
	cl_soundSysPool = Mem_CreatePool ("Client: Sound system");

	// Initialize video/sound
	VID_Init (&cls.refConfig);

	// Initialize the net buffer
	MSG_Init (&cls.netMessage, cls.netBuffer, sizeof (cls.netBuffer));

	CL_SetState (CA_DISCONNECTED);
	cls.realTime = Sys_Milliseconds ();
	cls.disableScreen = qTrue;

	// Initialize subsystems
	CDAudio_Init ();
	CL_InputInit ();
	IN_Init ();

#ifdef CL_HTTPDL
	CL_HTTPDL_Init ();
#endif

	// Load client media
	CL_MediaInit ();
	GUI_Init ();
	CL_CGModule_MainMenu ();

	// Touch memory
	Mem_TouchGlobal ();

	// Ready!
	cls.disableScreen = qFalse;
}


/*
===============
CL_ClientShutdown

FIXME: this is a callback from Sys_Quit Sys_Error and Com_Error. It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void CL_ClientShutdown (qBool error)
{
	static qBool isDown = qFalse;
	if (isDown)
		return;
	isDown = qTrue;

#ifdef CL_HTTPDL
	CL_HTTPDL_Cleanup (qTrue);
#endif

	if (!error)
		CL_WriteConfig ();

	CL_CGameAPI_Shutdown ();

	CDAudio_Shutdown ();
	Snd_Shutdown ();

	IN_Shutdown ();
	VID_Shutdown ();
}
