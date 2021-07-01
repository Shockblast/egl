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

// cl_main.c
// - client main loop

#include "cl_local.h"

cvar_t	*freelook;

cvar_t	*adr0;
cvar_t	*adr1;
cvar_t	*adr2;
cvar_t	*adr3;
cvar_t	*adr4;
cvar_t	*adr5;
cvar_t	*adr6;
cvar_t	*adr7;

cvar_t	*autosensitivity;

cvar_t	*ch_alpha;
cvar_t	*ch_pulse;
cvar_t	*ch_scale;
cvar_t	*ch_red;
cvar_t	*ch_green;
cvar_t	*ch_blue;

cvar_t	*cl_add_particles;
cvar_t	*cl_autoskins;

cvar_t	*cl_add_decals;
cvar_t	*cl_decal_burnlife;
cvar_t	*cl_decal_life;
cvar_t	*cl_decal_lod;
cvar_t	*cl_decal_max;

cvar_t	*cl_explorattle;
cvar_t	*cl_explorattle_scale;
cvar_t	*cl_footsteps;
cvar_t	*cl_gore;
cvar_t	*cl_gun;
cvar_t	*cl_lightlevel;
cvar_t	*cl_maxfps;
cvar_t	*cl_noskins;
cvar_t	*cl_predict;
cvar_t	*cl_paused;

cvar_t	*cl_railred;
cvar_t	*cl_railgreen;
cvar_t	*cl_railblue;
cvar_t	*cl_railtrail;

cvar_t	*cl_showclamp;
cvar_t	*cl_showfps;
cvar_t	*cl_showmiss;
cvar_t	*cl_shownet;
cvar_t	*cl_showping;
cvar_t	*cl_showtime;
cvar_t	*cl_smokelinger;

cvar_t	*cl_spiralred;
cvar_t	*cl_spiralgreen;
cvar_t	*cl_spiralblue;

cvar_t	*cl_stereo_separation;
cvar_t	*cl_stereo;

cvar_t	*cl_timedemo;
cvar_t	*cl_timeout;
cvar_t	*cl_timestamp;
cvar_t	*cl_vwep;

cvar_t	*con_alpha;
cvar_t	*con_clock;
cvar_t	*con_drop;
cvar_t	*con_scroll;

cvar_t	*glm_advgas;
cvar_t	*glm_advstingfire;
cvar_t	*glm_blobtype;
cvar_t	*glm_bluestingfire;
cvar_t	*glm_flashpred;
cvar_t	*glm_flwhite;
cvar_t	*glm_jumppred;
cvar_t	*glm_showclass;

cvar_t	*lookspring;
cvar_t	*lookstrafe;

cvar_t	*m_pitch;
cvar_t	*m_yaw;
cvar_t	*m_forward;
cvar_t	*m_side;

cvar_t	*part_cull;
cvar_t	*part_degree;
cvar_t	*part_lod;

cvar_t	*rcon_clpassword;
cvar_t	*rcon_address;

cvar_t	*sensitivity;

cvar_t	*fov;
cvar_t	*gender;
cvar_t	*hand;
cvar_t	*info_password;
cvar_t	*info_spectator;
cvar_t	*msg;
cvar_t	*name;
cvar_t	*rate;
cvar_t	*skin;

cvar_t	*ui_sensitivity;

clMedia_t		clMedia;
clientStatic_t	cls;
clientState_t	cl;

char			cl_WeaponModels[MAX_CLIENTWEAPONMODELS][MAX_QPATH];
int				cl_NumWeaponModels;

centity_t		cl_Entities[MAX_EDICTS];

entityState_t	cl_ParseEntities[MAX_PARSE_ENTITIES];

netAdr_t	last_rcon_to;

extern	cvar_t *allow_download;
extern	cvar_t *allow_download_players;
extern	cvar_t *allow_download_models;
extern	cvar_t *allow_download_sounds;
extern	cvar_t *allow_download_maps;

//======================================================================

/*
===================
Cmd_ForwardToServer

adds the current command line as a CLC_STRINGCMD to the client message.
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void Cmd_ForwardToServer (void) {
	char	*cmd;

	cmd = Cmd_Argv (0);
	if ((cls.connState <= CA_CONNECTED) || (*cmd == '-') || (*cmd == '+')) {
		Com_Printf (PRINT_ALL, "Unknown command \"%s" S_STYLE_RETURN "\"\n", cmd);
		return;
	}

	MSG_WriteByte (&cls.netChan.message, CLC_STRINGCMD);
	MSG_Print (&cls.netChan.message, cmd);
	if (Cmd_Argc () > 1) {
		MSG_Print (&cls.netChan.message, " ");
		MSG_Print (&cls.netChan.message, Cmd_Args());
	}
}


/*
================
CL_Drop

Called after an ERR_DROP was thrown
================
*/
void CL_Drop (void) {
	if (CL_ConnectionState (CA_UNINITIALIZED))
		return;
	if (CL_ConnectionState (CA_DISCONNECTED))
		return;

	CL_Disconnect ();

	/* drop loading plaque unless this is the initial game start */
	if (cls.disableServerCount != -1)
		SCR_EndLoadingPlaque ();	// get rid of loading plaque
}


/*
=======================
CL_SendConnectPacket

We have gotten a challenge from the server, so try and connect.
======================
*/
void CL_SendConnectPacket (void) {
	netAdr_t	adr;
	int			port;

	if (!NET_StringToAdr (cls.serverName, &adr)) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Bad server address\n");
		cls.connectTime = 0;
		return;
	}

	if (adr.port == 0)
		adr.port = BigShort (PORT_SERVER);

	port = Cvar_VariableInteger ("qport");
	cvar_UserInfoModified = qFalse;

	Netchan_OutOfBandPrint (NS_CLIENT, adr, "connect %i %i %i \"%s\"\n",
		PROTOCOL_VERSION, port, cls.challenge, Cvar_Userinfo());
}


/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
void CL_CheckForResend (void) {
	netAdr_t	adr;

	/* if the local server is running and we aren't then connect */
	if (CL_ConnectionState (CA_DISCONNECTED) && Com_ServerState ()) {
		CL_SetConnectState (CA_CONNECTING);
		strncpy (cls.serverName, "localhost", sizeof (cls.serverName)-1);
		/* we don't need a challenge on the localhost */
		CL_SendConnectPacket ();
		return;
	}

	/* resend if we haven't gotten a reply yet */
	if (!CL_ConnectionState (CA_CONNECTING))
		return;

	if ((cls.realTime - cls.connectTime) < 3000)
		return;

	CL_ClearDecals (); // need to free memory on map change

	if (!NET_StringToAdr (cls.serverName, &adr)) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Bad server address\n");
		CL_SetConnectState (CA_DISCONNECTED);
		return;
	}

	if (adr.port == 0)
		adr.port = BigShort (PORT_SERVER);

	cls.connectTime = cls.realTime;	// for retransmit requests

	Com_Printf (PRINT_ALL, "Connecting to %s...\n", cls.serverName);

	Netchan_OutOfBandPrint (NS_CLIENT, adr, "getchallenge\n");
}


/*
=====================
CL_ResetServerCount
=====================
*/
void CL_ResetServerCount (void) {
	cl.serverCount = -1;
}


/*
=====================
CL_SetConnectState
=====================
*/
void CL_SetConnectState (int state) {
	cls.connState = state;
}


/*
=====================
CL_ConnectionState
=====================
*/
qBool CL_ConnectionState (int state) {
	if (cls.connState == state)
		return qTrue;

	return qFalse;
}


/*
=====================
CL_ClearState
=====================
*/
void CL_ClearState (void) {
	Snd_StopAllSounds ();
	CL_ClearEffects ();

	/* wipe the entire cl structure */
	memset (&cl, 0, sizeof (cl));
	memset (&cl_Entities, 0, sizeof (cl_Entities));

	MSG_Clear (&cls.netChan.message);
}


/*
=====================
CL_Disconnect

Goes from a connected state to full screen console state
Sends a disconnect message to the server
This is also called on Com_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect (void) {
	byte	final[32];

	if (CL_ConnectionState (CA_DISCONNECTED))
		return;

	if (cl_timedemo && cl_timedemo->value) {
		int	time;
		
		time = Sys_Milliseconds () - cl.timeDemoStart;
		if (time > 0)
			Com_Printf (PRINT_ALL, "%i frames, %3.1f seconds: %3.1f fps\n", cl.timeDemoFrames,
				time/1000.0, cl.timeDemoFrames*1000.0 / time);
	}

	VectorIdentity (cl.refDef.vBlend);
	R_SetPalette (NULL);

	CL_UIModule_ForceMenuOff ();

	cls.connectTime = 0;

	CIN_StopCinematic ();

	if (cls.demoRecording)
		CL_Stop_f ();

	// send a disconnect message to the server
	final[0] = CLC_STRINGCMD;
	strcpy ((char *)final+1, "disconnect");
	Netchan_Transmit (&cls.netChan, strlen (final), final);
	Netchan_Transmit (&cls.netChan, strlen (final), final);
	Netchan_Transmit (&cls.netChan, strlen (final), final);

	CL_ClearState ();

	CM_UnloadMap ();

	// stop download
	if (cls.downloadFile) {
		fclose (cls.downloadFile);
		cls.downloadFile = NULL;
	}

	CL_SetConnectState (CA_DISCONNECTED);
}


/*
=================
CL_ParseStatusMessage

Handle a reply from a ping
=================
*/
void CL_ParseStatusMessage (void) {
	char	*s;

	s = MSG_ReadString (&net_Message);

	// FIXME KTHX added the following line to stop the extra space in the join server menu .. bad?
	s[strlen(s)-1] = '\0';
	Com_Printf (PRINT_ALL, "%s\n", s);
	CL_UIModule_AddToServerList (net_From, s);
}


/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
void CL_ConnectionlessPacket (void) {
	char		*s, *c;
	netAdr_t	remote;

	NET_StringToAdr (cls.serverName, &remote);
	MSG_BeginReading (&net_Message);
	MSG_ReadLong (&net_Message);	// skip the -1

	s = MSG_ReadStringLine (&net_Message);

	Cmd_TokenizeString (s, qFalse);

	c = Cmd_Argv (0);

	Com_Printf (PRINT_ALL, "%s: %s\n", NET_AdrToString (net_From), c);

	/* server connection */
	if (!strcmp (c, "client_connect")) {
		if (CL_ConnectionState (CA_CONNECTED)) {
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Dup connect received. Ignored.\n");
			return;
		}

		Netchan_Setup (NS_CLIENT, &cls.netChan, net_From, cls.quakePort);
		MSG_WriteChar (&cls.netChan.message, CLC_STRINGCMD);
		MSG_WriteString (&cls.netChan.message, "new");	
		CL_SetConnectState (CA_CONNECTED);
		return;
	}

	/* server responding to a status broadcast */
	if (!strcmp (c, "info")) {
		CL_ParseStatusMessage ();
		return;
	}

	/*
	** R1CH: security check. (only allow from current connected server and last
	** destination client sent an rcon to)
	*/
	if (!NET_CompareBaseAdr (net_From, remote) && !NET_CompareBaseAdr (net_From, last_rcon_to)) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Illegal %s from %s. Ignored.\n", c, NET_AdrToString (net_From));
		return;
	}

	/* remote command from gui front end */
	if (!strcmp (c, "cmd")) {
		if (!NET_IsLocalAddress (net_From)) {
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Command packet from remote host. Ignored.\n");
			return;
		}
		Sys_AppActivate ();
		s = MSG_ReadString (&net_Message);
		Cbuf_AddText (s);
		Cbuf_AddText ("\n");
		return;
	}
	/* print command from somewhere */
	if (!strcmp (c, "print")) {
		s = MSG_ReadString (&net_Message);
		Com_Printf (PRINT_ALL, "%s", s);
		return;
	}

	/* ping from somewhere */
	if (!strcmp (c, "ping")) {
		Netchan_OutOfBandPrint (NS_CLIENT, net_From, "ack");
		return;
	}

	/* challenge from the server we are connecting to */
	if (!strcmp (c, "challenge")) {
		cls.challenge = atoi(Cmd_Argv (1));
		CL_SendConnectPacket ();
		return;
	}

	/* echo request from server */
	if (!strcmp (c, "echo")) {
		Netchan_OutOfBandPrint (NS_CLIENT, net_From, "%s", Cmd_Argv (1));
		return;
	}

	Com_Printf (PRINT_ALL, "Unknown command.\n");
}


/*
=================
CL_ReadPackets
=================
*/
void CL_ReadPackets (void) {
	while (NET_GetPacket (NS_CLIENT, &net_From, &net_Message)) {
		/* remote command packet */
		if (*(int *)net_Message.data == -1) {
			CL_ConnectionlessPacket ();
			continue;
		}

		if (CL_ConnectionState (CA_DISCONNECTED) || CL_ConnectionState (CA_CONNECTING))
			continue;		// dump it if not connected

		if (net_Message.curSize < 8) {
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "%s: Runt packet\n", NET_AdrToString(net_From));
			continue;
		}

		/* packet from server */
		if (!NET_CompareAdr (net_From, cls.netChan.remoteAddress)) {
			Com_Printf (PRINT_DEV, S_COLOR_YELLOW "%s:sequenced packet without connection\n", NET_AdrToString (net_From));
			continue;
		}
		
		if (!Netchan_Process (&cls.netChan, &net_Message))
			continue;	// wasn't accepted for some reason

		CL_ParseServerMessage ();
	}

	/* check timeout */
	if ((cls.connState >= CA_CONNECTED) && (cls.realTime - cls.netChan.lastReceived) > (cl_timeout->value*1000)) {
		if (++cl.timeOutCount > 5) {	// timeoutcount saves debugger
			Com_Printf (PRINT_ALL, "\n" S_COLOR_YELLOW "Server connection timed out.\n");
			CL_Disconnect ();
			return;
		}
	} else
		cl.timeOutCount = 0;
}

/*
=======================================================================

	CONSOLE FUNCTIONS

=======================================================================
*/

/*
===================
CL_Setenv_f
===================
*/
void CL_Setenv_f (void) {
	int argc = Cmd_Argc ();

	if (argc > 2) {
		char buffer[1000];
		int i;

		strcpy (buffer, Cmd_Argv (1));
		strcat (buffer, "=");

		for (i=2 ; i<argc ; i++) {
			strcat (buffer, Cmd_Argv (i));
			strcat (buffer, " ");
		}

		putenv (buffer);
	} else if (argc == 2) {
		char *env = getenv (Cmd_Argv (1));

		if (env)
			Com_Printf (PRINT_ALL, "\"%s\" is: \"%s\"\n", Cmd_Argv (1), env);
		else
			Com_Printf (PRINT_ALL, "\"%s\" is undefined\n", Cmd_Argv (1), env);
	}
}


/*
==================
CL_ForwardToServer_f
==================
*/
void CL_ForwardToServer_f (void) {
	if (!CL_ConnectionState (CA_CONNECTED) && !CL_ConnectionState (CA_ACTIVE)) {
		Com_Printf (PRINT_ALL, "Can't \"%s\", not connected\n", Cmd_Argv (0));
		return;
	}
	
	/* don't forward the first argument */
	if (Cmd_Argc () > 1) {
		MSG_WriteByte (&cls.netChan.message, CLC_STRINGCMD);
		MSG_Print (&cls.netChan.message, Cmd_Args ());
	}
}


/*
==================
CL_Pause_f
==================
*/
void CL_Pause_f (void) {
	/* never pause in multiplayer */
	if ((Cvar_VariableValue ("maxclients") > 1) || !Com_ServerState ()) {
		Cvar_SetValue ("paused", 0);
		return;
	}

	Cvar_SetValue ("paused", !cl_paused->value);
}


/*
==================
CL_Quit_f
==================
*/
void CL_Quit_f (void) {
	CL_Disconnect ();
	Com_Quit ();
}


/*
================
CL_Connect_f
================
*/
void CL_Connect_f (void) {
	char	*server;

	if (Cmd_Argc () != 2) {
		Com_Printf (PRINT_ALL, "usage: connect <server>\n");
		return;	
	}
	
	if (Com_ServerState ()) {
		/* if running a local server, kill it and reissue */
		SV_Shutdown (va ("Server quit\n", msg), qFalse);
	} else {
		CL_Disconnect ();
	}

	server = Cmd_Argv (1);

	NET_Config (qTrue);	// allow remote

	CL_Disconnect ();

	CL_SetConnectState (CA_CONNECTING);
	strncpy (cls.serverName, server, sizeof (cls.serverName)-1);
	cls.connectTime = -99999;	// CL_CheckForResend () will fire immediately
}


/*
=====================
CL_Rcon_f

Send the rest of the command line over as an unconnected command.
=====================
*/
void CL_Rcon_f (void) {
	char		message[1024];
	int			i;
	netAdr_t	to;

	if (!rcon_clpassword->string) {
		Com_Printf (PRINT_ALL, "You must set 'rcon_password' before\n"
								"issuing an rcon command.\n");
		return;
	}

	message[0] = (char)255;
	message[1] = (char)255;
	message[2] = (char)255;
	message[3] = (char)255;
	message[4] = 0;

	NET_Config (qTrue);	// allow remote

	strcat (message, "rcon ");

	strcat (message, rcon_clpassword->string);
	strcat (message, " ");

	for (i=1 ; i<Cmd_Argc () ; i++) {
		strcat (message, Cmd_Argv (i));
		strcat (message, " ");
	}

	if (cls.connState >= CA_CONNECTED)
		to = cls.netChan.remoteAddress;
	else {
		if (!strlen (rcon_address->string)) {
			Com_Printf (PRINT_ALL,	"You must either be connected,\n"
									"or set the 'rcon_address' cvar\n"
									"to issue rcon commands\n");

			return;
		}

		NET_StringToAdr (rcon_address->string, &to);
		NET_StringToAdr (rcon_address->string, &last_rcon_to);

		if (to.port == 0)
			to.port = BigShort (PORT_SERVER);
	}
	
	NET_SendPacket (NS_CLIENT, strlen (message) + 1, message, to);
}


/*
=================
CL_Disconnect_f
=================
*/
void CL_Disconnect_f (void) {
	Com_Error (ERR_DROP, "Disconnected from server");
}


/*
=================
CL_Changing_f

Just sent as a hint to the client that they should
drop to full console
=================
*/
void CL_Changing_f (void) {
	//ZOID
	/* if we are downloading, we don't change!  This so we don't suddenly stop downloading a map */
	if (cls.downloadFile)
		return;

	SCR_BeginLoadingPlaque ();
	CL_SetConnectState (CA_CONNECTED);	// not active anymore, but not disconnected
	Com_Printf (PRINT_ALL, "\nChanging map...\n");
}


/*
=================
CL_Reconnect_f

The server is changing levels
=================
*/
void CL_Reconnect_f (void) {
	//ZOID
	// if we are downloading, we don't change! This so we don't suddenly stop downloading a map
	if (cls.downloadFile)
		return;

	Snd_StopAllSounds ();
	if (CL_ConnectionState (CA_CONNECTED)) {
		Com_Printf (PRINT_ALL, "reconnecting...\n");
		CL_SetConnectState (CA_CONNECTED);
		MSG_WriteChar (&cls.netChan.message, CLC_STRINGCMD);
		MSG_WriteString (&cls.netChan.message, "new");		
		return;
	}

	if (*cls.serverName) {
		if (cls.connState >= CA_CONNECTED) {
			CL_Disconnect();
			cls.connectTime = cls.realTime - 1500;
		} else
			cls.connectTime = -99999; // CL_CheckForResend () will fire immediately

		CL_SetConnectState (CA_CONNECTING);
		Com_Printf (PRINT_ALL, "reconnecting...\n");
	}
}


/*
=================
CL_PingServers_f
=================
*/
void CL_PingServers_f (void) {
	int			i;
	netAdr_t	adr;
	char		name[32];
	char		*adrstring;
	cvar_t		*noudp;
	cvar_t		*noipx;

	NET_Config (qTrue);	// allow remote

	// send a broadcast packet
	Com_Printf (PRINT_ALL, "pinging broadcast...\n");

	noudp = Cvar_Get ("noudp", "0", CVAR_NOSET);
	if (!noudp->value) {
		adr.naType = NA_BROADCAST;
		adr.port = BigShort(PORT_SERVER);
		Netchan_OutOfBandPrint (NS_CLIENT, adr, va("info %i", PROTOCOL_VERSION));
	}

	noipx = Cvar_Get ("noipx", "0", CVAR_NOSET);
	if (!noipx->value) {
		adr.naType = NA_BROADCAST_IPX;
		adr.port = BigShort(PORT_SERVER);
		Netchan_OutOfBandPrint (NS_CLIENT, adr, va("info %i", PROTOCOL_VERSION));
	}

	// send a packet to each address book entry
	for (i=0 ; i<8 ; i++) {
		Com_sprintf (name, sizeof (name), "adr%i", i);
		adrstring = Cvar_VariableString (name);

		if (!adrstring || !adrstring[0])
			continue;

		Com_Printf (PRINT_ALL, "pinging %s...\n", adrstring);

		if (!NET_StringToAdr (adrstring, &adr)) {
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Bad address: %s\n", adrstring);
			continue;
		}
		if (!adr.port)
			adr.port = BigShort (PORT_SERVER);

		Netchan_OutOfBandPrint (NS_CLIENT, adr, va("info %i", PROTOCOL_VERSION));
	}
}


/*
=================
CL_Skins_f

Load or download any custom player skins and models
=================
*/
void CL_Skins_f (void) {
	int		i;

	if (FS_CurrGame ("gloom")) {
		Com_Printf (PRINT_ALL, S_COLOR_CYAN "Command cannot be used in Gloom (considered cheating).\n");
		return;
	}

	for (i=0 ; i<MAX_CLIENTS ; i++) {
		if (!cl.configStrings[CS_PLAYERSKINS+i][0])
			continue;

		Com_Printf (PRINT_ALL, "client %i: %s\n", i, cl.configStrings[CS_PLAYERSKINS+i]); 
		SCR_UpdateScreen ();
		Sys_SendKeyEvents ();	// pump message loop
		CL_ParseClientinfo (i);
	}
}

/*
=======================================================================

	REGISTRATION

=======================================================================
*/

/*
=================
CL_RegisterClientSounds
=================
*/
void CL_RegisterClientSounds (void) {
	int		i;
	char	name[MAX_QPATH];

	clMedia.ricochet1SFX		= Snd_RegisterSound ("world/ric1.wav");
	clMedia.ricochet2SFX		= Snd_RegisterSound ("world/ric2.wav");
	clMedia.ricochet3SFX		= Snd_RegisterSound ("world/ric3.wav");
	clMedia.spark5SFX			= Snd_RegisterSound ("world/spark5.wav");
	clMedia.spark6SFX			= Snd_RegisterSound ("world/spark6.wav");
	clMedia.spark7SFX			= Snd_RegisterSound ("world/spark7.wav");
	clMedia.disruptExploSFX		= Snd_RegisterSound ("weapons/disrupthit.wav");
	clMedia.grenadeExploSFX		= Snd_RegisterSound ("weapons/grenlx1a.wav");
	clMedia.rocketExploSFX		= Snd_RegisterSound ("weapons/rocklx1a.wav");
	clMedia.waterExploSFX		= Snd_RegisterSound ("weapons/xpld_wat.wav");
	clMedia.laserHitSFX			= Snd_RegisterSound ("weapons/lashit.wav");
	clMedia.railgunFireSFX		= Snd_RegisterSound ("weapons/railgf1a.wav");
	clMedia.lightningSFX		= Snd_RegisterSound ("weapons/tesla.wav");

	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof (name), "player/step%i.wav", i+1);
		clMedia.footstepSFX[i] = Snd_RegisterSound (name);
	}

	Snd_RegisterSound ("player/land1.wav");
	Snd_RegisterSound ("player/fall2.wav");
	Snd_RegisterSound ("player/fall1.wav");
	Snd_RegisterSound ("misc/udeath.wav");
}


/*
=================
CL_RegisterGloomMedia
=================
*/
void CL_RegisterGloomMedia (void) { // orly
	if (!FS_CurrGame ("gloom"))
		return;
return; // kthx save for next version
	Mod_RegisterModel ("players/engineer/tris.md2");
	Mod_RegisterModel ("players/engineer/weapon.md2");
	Mod_RegisterModel ("players/male/tris.md2");
	Mod_RegisterModel ("players/male/autogun.md2");
	Mod_RegisterModel ("players/male/shotgun.md2");
	Mod_RegisterModel ("players/male/weapon.md2");
	Mod_RegisterModel ("players/female/tris.md2");
	Mod_RegisterModel ("players/female/weapon.md2");
	Mod_RegisterModel ("players/hsold/tris.md2");
	Mod_RegisterModel ("players/hsold/weapon.md2");
	Mod_RegisterModel ("players/male/smg.md2");
	Mod_RegisterModel ("players/exterm/tris.md2");
	Mod_RegisterModel ("players/exterm/weapon.md2");
	Mod_RegisterModel ("players/mech/tris.md2");
 	Mod_RegisterModel ("players/mech/weapon.md2");

	/*Mod_RegisterModel ("models/objects/dmspot/tris.md2");
	Mod_RegisterModel ("models/turret/base.md2");
	Mod_RegisterModel ("models/turret/gun.md2");
	Mod_RegisterModel ("models/turret/mgun.md2");
	Mod_RegisterModel ("models/objects/detector/tris.md2");
	Mod_RegisterModel ("models/objects/tripwire/tris.md2");
	Mod_RegisterModel ("models/objects/depot/tris.md2");

	Mod_RegisterModel ("models/weapons/v_auto/tris.md2");
	Mod_RegisterModel ("models/weapons/v_shot/tris.md2");
	Mod_RegisterModel ("models/weapons/v_spas/tris.md2");
	Mod_RegisterModel ("models/weapons/v_launch/tris.md2");
	Mod_RegisterModel ("models/weapons/v_pist/tris.md2");
 	Mod_RegisterModel ("models/weapons/v_sub/tris.md2");
	Mod_RegisterModel ("models/weapons/v_mag/tris.md2");
	Mod_RegisterModel ("models/weapons/v_plas/tris.md2");
	Mod_RegisterModel ("models/weapons/v_mech/tris.md2");

	Mod_RegisterModel ("models/objects/c4/tris.md2");
	Mod_RegisterModel ("models/objects/grenade/tris.md2");
	Mod_RegisterModel ("models/objects/debris1/tris.md2");
	Mod_RegisterModel ("models/objects/debris2/tris.md2");*/

	Mod_RegisterModel ("players/breeder/tris.md2");
	Mod_RegisterModel ("players/breeder/weapon.md2");
	Mod_RegisterModel ("players/hatch/tris.md2");
	Mod_RegisterModel ("players/hatch/weapon.md2");
	Mod_RegisterModel ("players/drone/tris.md2");
	Mod_RegisterModel ("players/drone/weapon.md2");
	Mod_RegisterModel ("players/wraith/tris.md2");
	Mod_RegisterModel ("players/wraith/weapon.md2");
	Mod_RegisterModel ("players/stinger/tris.md2");
	Mod_RegisterModel ("players/stinger/weapon.md2");
	Mod_RegisterModel ("players/guardian/tris.md2");
	Mod_RegisterModel ("players/guardian/weapon.md2");
	Mod_RegisterModel ("players/stalker/tris.md2");
	Mod_RegisterModel ("players/stalker/weapon.md2");

	/*Mod_RegisterModel ("models/objects/cocoon/tris.md2");
	Mod_RegisterModel ("models/objects/organ/spiker/tris.md2");
	Mod_RegisterModel ("models/objects/organ/healer/tris.md2");
	Mod_RegisterModel ("models/objects/organ/obstacle/tris.md2");
	Mod_RegisterModel ("models/objects/organ/gas/tris.md2");
	Mod_RegisterModel ("models/objects/spike/tris.md2");
	Mod_RegisterModel ("models/objects/spore/tris.md2");
	Mod_RegisterModel ("models/objects/smokexp/tris.md2");
	Mod_RegisterModel ("models/objects/web/ball.md2");

	Mod_RegisterModel ("models/gibs/hatchling/leg/tris.md2");
	Mod_RegisterModel ("models/gibs/guardian/gib2.md2");
	Mod_RegisterModel ("models/gibs/guardian/gib1.md2");
	Mod_RegisterModel ("models/gibs/stalker/gib1.md2");
	Mod_RegisterModel ("models/gibs/stalker/gib2.md2");
	Mod_RegisterModel ("models/gibs/stalker/gib3.md2");
	Mod_RegisterModel ("models/objects/sspore/tris.md2");*/
}


/*
=================
CL_InitMedia
=================
*/
char	*sb_nums[2][11] = {
	{"num_0",  "num_1",  "num_2",  "num_3",  "num_4",  "num_5",  "num_6",  "num_7",  "num_8",  "num_9",  "num_minus"},
	{"anum_0", "anum_1", "anum_2", "anum_3", "anum_4", "anum_5", "anum_6", "anum_7", "anum_8", "anum_9", "anum_minus"}
};
void CL_InitPDImage (int spot, char *name, int flags) {
	clMedia.mediaTable[spot % PDT_PICTOTAL] = Img_FindImage (name, IF_NOFLUSH|IF_NOGAMMA|IF_NOINTENS|IF_CLAMP|flags);
}
void CL_InitMedia (void) {
	int		i, j;
	char	chPic[MAX_QPATH];

	// pics
	clMedia.consoleImage	= Img_RegisterPic ("conback");
	clMedia.tileBackImage	= Img_RegisterPic ("backtile");

	if (crosshair->integer) {
		crosshair->integer = (crosshair->integer < 0) ? 0 : crosshair->integer;

		Com_sprintf (chPic, sizeof (chPic), "ch%d", crosshair->integer);
		clMedia.crosshairImage = Img_RegisterPic (chPic);
	}

	clMedia.hudFieldImage		= Img_RegisterPic ("field_3");
	clMedia.hudInventoryImage	= Img_RegisterPic ("inventory");
	clMedia.hudLoadingImage		= Img_RegisterPic ("loading");
	clMedia.hudNetImage			= Img_RegisterPic ("net");
	for (i=0 ; i<2 ; i++)
		for (j=0 ; j<11 ; j++)
			clMedia.hudNumImages[i][j] = Img_RegisterPic (va ("%s", sb_nums[i][j]));
	clMedia.hudPausedImage	= Img_RegisterPic ("pause");

	Img_RegisterPic ("w_machinegun");
	Img_RegisterPic ("a_bullets");
	Img_RegisterPic ("i_health");
	Img_RegisterPic ("a_grenades");

	// models
	clMedia.parasiteSegmentMOD	= Mod_RegisterModel ("models/monsters/parasite/segment/tris.md2");
	clMedia.grappleCableMOD		= Mod_RegisterModel ("models/ctf/segment/tris.md2");

	clMedia.lightningMOD		= Mod_RegisterModel ("models/proj/lightning/tris.md2");
	clMedia.heatBeamMOD			= Mod_RegisterModel ("models/proj/beam/tris.md2");
	clMedia.monsterHeatBeamMOD	= Mod_RegisterModel ("models/proj/widowbeam/tris.md2");


	// Particles / Decals
	CL_InitPDImage (PT_BLUE,			"egl/parts/blue.tga",			0);
	CL_InitPDImage (PT_GREEN,			"egl/parts/green.tga",			0);
	CL_InitPDImage (PT_RED,				"egl/parts/red.tga",			0);
	CL_InitPDImage (PT_WHITE,			"egl/parts/white.tga",			0);

	CL_InitPDImage (PT_SMOKE,			"egl/parts/smoke.tga",			0);
	CL_InitPDImage (PT_SMOKE2,			"egl/parts/smoke2.tga",			0);

	CL_InitPDImage (PT_BLUEFIRE,		"egl/parts/bluefire.tga",		0);
	CL_InitPDImage (PT_FIRE,			"egl/parts/fire.tga",			0);

	CL_InitPDImage (PT_BEAM,			"egl/parts/beam.tga",			IF_NOMIPMAP);
	CL_InitPDImage (PT_BLDDRIP,			"egl/parts/blooddrip.tga",		0);
	CL_InitPDImage (PT_BUBBLE,			"egl/parts/bubble.tga",			0);
	CL_InitPDImage (PT_DRIP,			"egl/parts/drip.tga",			0);
	CL_InitPDImage (PT_EMBERS,			"egl/parts/embers.tga",			0);
	CL_InitPDImage (PT_EMBERS2,			"egl/parts/embers2.tga",		0);
	CL_InitPDImage (PT_EXPLOFLASH,		"egl/parts/exploflash.tga",		0);
	CL_InitPDImage (PT_EXPLOWAVE,		"egl/parts/explowave.tga",		0);
	CL_InitPDImage (PT_FLARE,			"egl/parts/flare.tga",			0);
	CL_InitPDImage (PT_FLY,				"egl/parts/fly.tga",			0);
	CL_InitPDImage (PT_MIST,			"egl/parts/mist.tga",			0);
	CL_InitPDImage (PT_SPARK,			"egl/parts/spark.tga",			0);
	CL_InitPDImage (PT_SPLASH,			"egl/parts/splash.tga",			0);

	// Animated explosions
	CL_InitPDImage (PT_EXPLO1,			"egl/parts/explo1.tga",			0);
	CL_InitPDImage (PT_EXPLO2,			"egl/parts/explo2.tga",			0);
	CL_InitPDImage (PT_EXPLO3,			"egl/parts/explo3.tga",			0);
	CL_InitPDImage (PT_EXPLO4,			"egl/parts/explo4.tga",			0);
	CL_InitPDImage (PT_EXPLO5,			"egl/parts/explo5.tga",			0);
	CL_InitPDImage (PT_EXPLO6,			"egl/parts/explo6.tga",			0);
	CL_InitPDImage (PT_EXPLO7,			"egl/parts/explo7.tga",			0);

	// Decal specific
	CL_InitPDImage (DT_BLOOD,			"egl/decals/blood.tga",			0);
	CL_InitPDImage (DT_BLOOD2,			"egl/decals/blood2.tga",		0);
	CL_InitPDImage (DT_BLOOD3,			"egl/decals/blood3.tga",		0);
	CL_InitPDImage (DT_BLOOD4,			"egl/decals/blood4.tga",		0);
	CL_InitPDImage (DT_BLOOD5,			"egl/decals/blood5.tga",		0);
	CL_InitPDImage (DT_BLOOD6,			"egl/decals/blood6.tga",		0);

	CL_InitPDImage (DT_BULLET,			"egl/decals/bullet.tga",		0);

	CL_InitPDImage (DT_BURNMARK,		"egl/decals/burnmrk.tga",		0);
	CL_InitPDImage (DT_BURNMARK2,		"egl/decals/burnmrk2.tga",		0);
	CL_InitPDImage (DT_BURNMARK3,		"egl/decals/burnmrk3.tga",		0);

	CL_InitPDImage (DT_SLASH,			"egl/decals/slash.tga",			0);
	CL_InitPDImage (DT_SLASH2,			"egl/decals/slash2.tga",		0);
	CL_InitPDImage (DT_SLASH3,			"egl/decals/slash3.tga",		0);

	// hack! this will be "cleaner" when the cgame dll's come in, but for now we do this
	CL_RegisterGloomMedia ();

	// sounds
	CL_RegisterClientSounds ();
}


/*
=================
CL_PrepRefresh

Call before entering a new level, or after changing dlls
=================
*/
void KeepAlive (void) {
	Netchan_Transmit (&cls.netChan, 0, NULL);
	Com_Printf (PRINT_ALL, "keep alive packet\r");
	SCR_UpdateScreen ();
}
void CL_PrepRefresh (void) {
	char		mapName[64];
	char		name[MAX_QPATH];
	int			i, prepInitTime;
	float		rotate;
	vec3_t		axis;

	if (!cl.configStrings[CS_MODELS+1][0])
		return;		// no map loaded

	prepInitTime = Sys_Milliseconds ();

	// let the render dll load the map
	strcpy (mapName, cl.configStrings[CS_MODELS+1] + 5);	// skip "maps/"
	mapName[strlen (mapName) - 4] = 0;						// cut off ".bsp"

	// register models, pics, and skins
	Com_Printf (PRINT_ALL, "Map: %s\r", mapName);
	SCR_UpdateScreen ();
	R_BeginRegistration (mapName);

	// register client media
	Com_Printf (PRINT_ALL, "client media\r");
	SCR_UpdateScreen ();
	CL_InitMedia ();

	// load models
	cl_NumWeaponModels = 1;
	strcpy (cl_WeaponModels[0], "weapon.md2");
	for (i=1 ; i<MAX_MODELS && cl.configStrings[CS_MODELS+i][0] ; i++) {
		strcpy (name, cl.configStrings[CS_MODELS+i]);
		name[37] = 0;	// never go beyond one line
		if (name[0] != '*')
			Com_Printf (PRINT_ALL, "%s\r", name);

		SCR_UpdateScreen ();
		Sys_SendKeyEvents ();	// pump message loop
		if (name[0] == '#') {
			// special player weapon model
			if (cl_NumWeaponModels < MAX_CLIENTWEAPONMODELS) {
				strncpy(cl_WeaponModels[cl_NumWeaponModels], cl.configStrings[CS_MODELS+i]+1,
					sizeof (cl_WeaponModels[cl_NumWeaponModels]) - 1);
				cl_NumWeaponModels++;
			}
		} else {
			cl.modelDraw[i] = Mod_RegisterModel (cl.configStrings[CS_MODELS+i]);
			if (name[0] == '*')
				cl.modelClip[i] = CM_InlineModel (cl.configStrings[CS_MODELS+i]);
			else
				cl.modelClip[i] = NULL;
		}
		if (name[0] != '*')
			Com_Printf (PRINT_ALL, "     \r");
	}

	KeepAlive (); // don't time out!

	Com_Printf (PRINT_ALL, "images\r", i);
	SCR_UpdateScreen ();
	for (i=1 ; i<MAX_IMAGES && cl.configStrings[CS_IMAGES+i][0] ; i++) {
		cl.imagePrecache[i] = Img_RegisterPic (cl.configStrings[CS_IMAGES+i]);
		Sys_SendKeyEvents ();	// pump message loop
	}

	for (i=0 ; i<MAX_CLIENTS ; i++) {
		if (!cl.configStrings[CS_PLAYERSKINS+i][0])
			continue;

		Com_Printf (PRINT_ALL, "client %i\r", i);
		SCR_UpdateScreen ();
		Sys_SendKeyEvents ();	// pump message loop
		CL_ParseClientinfo (i);
	}

	CL_LoadClientinfo (&cl.baseClientInfo, "unnamed\\male/grunt");

	// check for and load the sky box images
	if (R_CheckLoadSky ()) {
		// set sky textures and speed
		Com_Printf (PRINT_ALL, "sky\r", i);
		SCR_UpdateScreen ();
		rotate = atof (cl.configStrings[CS_SKYROTATE]);
		sscanf (cl.configStrings[CS_SKYAXIS], "%f %f %f", &axis[0], &axis[1], &axis[2]);
		R_SetSky (cl.configStrings[CS_SKY], rotate, axis);
	} else {
		Com_Printf (PRINT_ALL, "no sky\r");
		SCR_UpdateScreen ();
	}

	// the renderer can now free unneeded stuff
	R_EndRegistration ();

	KeepAlive (); // don't time out!
	Com_Printf (PRINT_ALL, "     \r");
	SCR_UpdateScreen ();

	// clear any lines of console text
	Con_ClearNotify ();

	SCR_UpdateScreen ();
	cl.refreshPrepped	= qTrue;
	cl.forceRefresh		= qTrue;	// make sure we have a valid refdef

	// start the cd track
	CDAudio_Play (atoi (cl.configStrings[CS_CDTRACK]), qTrue);

	Com_Printf (PRINT_ALL, "refresh prepped in: %dms\n", Sys_Milliseconds () - prepInitTime);
}


/*
==============
CL_Userinfo_f
==============
*/
void CL_Userinfo_f (void) {
	Com_Printf (PRINT_ALL, "User info settings:\n");
	Info_Print (Cvar_Userinfo ());
}


/*
==============
CL_RequestNextDownload
==============
*/
int precache_check; // for autodownload of precache items
int precache_spawncount;
int precache_tex;
int precache_model_skin;

byte *precache_model; // used for skin checking in alias models

#define PLAYER_MULT 5

// ENV_CNT is map load, ENV_CNT+1 is first env map
#define ENV_CNT (CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT)
#define TEXTURE_CNT (ENV_CNT+13)

extern char *sky_NameSuffix[6];
void CL_RequestNextDownload (void) {
	unsigned		map_CheckSum;		// for detecting cheater maps
	char			fn[MAX_OSPATH];
	dMd2Header_t	*pheader;

	if (!CL_ConnectionState (CA_CONNECTED))
		return;

	if (!allow_download->value && (precache_check < ENV_CNT))
		precache_check = ENV_CNT;

	//ZOID
	if (precache_check == CS_MODELS) {
		// confirm map
		precache_check = CS_MODELS+2; // 0 isn't used
		if (allow_download_maps->value)
			if (!CL_CheckOrDownloadFile(cl.configStrings[CS_MODELS+1]))
				return; // started a download
	}

	if (precache_check >= CS_MODELS && precache_check < CS_MODELS+MAX_MODELS) {
		if (allow_download_models->value) {
			while ((precache_check < CS_MODELS+MAX_MODELS) && cl.configStrings[precache_check][0]) {
				if ((cl.configStrings[precache_check][0] == '*') || (cl.configStrings[precache_check][0] == '#')) {
					precache_check++;
					continue;
				}
				if (precache_model_skin == 0) {
					if (!CL_CheckOrDownloadFile (cl.configStrings[precache_check])) {
						precache_model_skin = 1;
						return; // started a download
					}
					precache_model_skin = 1;
				}

				// checking for skins in the model
				if (!precache_model) {
					FS_LoadFile (cl.configStrings[precache_check], (void **)&precache_model);
					if (!precache_model) {
						precache_model_skin = 0;
						precache_check++;
						continue; // couldn't load it
					}
					if (LittleLong (*(unsigned *)precache_model) != MD2_HEADER) {
						/* not an alias model */
						FS_FreeFile (precache_model);
						precache_model = 0;
						precache_model_skin = 0;
						precache_check++;
						continue;
					}
					pheader = (dMd2Header_t *)precache_model;
					if (LittleLong (pheader->version) != MD2_ALIAS_VERSION) {
						precache_check++;
						precache_model_skin = 0;
						continue; // couldn't load it
					}
				}

				pheader = (dMd2Header_t *)precache_model;
				while (precache_model_skin - 1 < LittleLong(pheader->numSkins)) {
					if (!CL_CheckOrDownloadFile((char *)precache_model +
						LittleLong(pheader->ofsSkins) + 
						(precache_model_skin - 1)*MD2_MAX_SKINNAME)) {
						precache_model_skin++;
						return; // started a download
					}
					precache_model_skin++;
				}

				if (precache_model) { 
					FS_FreeFile (precache_model);
					precache_model = 0;
				}

				precache_model_skin = 0;
				precache_check++;
			}
		}
		precache_check = CS_SOUNDS;
	}

	if ((precache_check >= CS_SOUNDS) && (precache_check < CS_SOUNDS+MAX_SOUNDS)) { 
		if (allow_download_sounds->value) {
			if (precache_check == CS_SOUNDS)
				precache_check++; // zero is blank

			while ((precache_check < CS_SOUNDS+MAX_SOUNDS) && (cl.configStrings[precache_check][0])) {
				if (cl.configStrings[precache_check][0] == '*') {
					precache_check++;
					continue;
				}

				Com_sprintf(fn, sizeof (fn), "sound/%s", cl.configStrings[precache_check++]);
				if (!CL_CheckOrDownloadFile(fn))
					return; // started a download
			}
		}
		precache_check = CS_IMAGES;
	}

	if ((precache_check >= CS_IMAGES) && (precache_check < CS_IMAGES+MAX_IMAGES)) {
		if (precache_check == CS_IMAGES)
			precache_check++; // zero is blank

		while ((precache_check < CS_IMAGES+MAX_IMAGES) && (cl.configStrings[precache_check][0])) {
			Com_sprintf (fn, sizeof (fn), "pics/%s.pcx", cl.configStrings[precache_check++]);
			if (!CL_CheckOrDownloadFile (fn))
				return; // started a download
		}
		precache_check = CS_PLAYERSKINS;
	}
	/*
	** skins are special, since a player has three things to download:
	** model, weapon model and skin
	** so precache_check is now *3
	*/
	if ((precache_check >= CS_PLAYERSKINS) && (precache_check < (CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT))) {
		if (allow_download_players->value) {
			while (precache_check < (CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT)) {
				int		i, n;
				char	model[MAX_QPATH];
				char	skin[MAX_QPATH];
				char	*p;

				i = (precache_check - CS_PLAYERSKINS)/PLAYER_MULT;
				n = (precache_check - CS_PLAYERSKINS)%PLAYER_MULT;

				if (!cl.configStrings[CS_PLAYERSKINS+i][0]) {
					precache_check = CS_PLAYERSKINS + (i + 1) * PLAYER_MULT;
					continue;
				}

				if ((p = strchr(cl.configStrings[CS_PLAYERSKINS+i], '\\')) != NULL)
					p++;
				else
					p = cl.configStrings[CS_PLAYERSKINS+i];

				strcpy(model, p);
				p = strchr(model, '/');
				if (!p)
					p = strchr(model, '\\');
				if (p) {
					*p++ = 0;
					strcpy(skin, p);
				} else
					*skin = 0;

				switch (n) {
				case 0:
					// model
					Com_sprintf(fn, sizeof (fn), "players/%s/tris.md2", model);
					if (!CL_CheckOrDownloadFile (fn)) {
						precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 1;
						return; // started a download
					}
					n++;
					// FALL THROUGH

				case 1:
					// weapon model
					Com_sprintf(fn, sizeof (fn), "players/%s/weapon.md2", model);
					if (!CL_CheckOrDownloadFile (fn)) {
						precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 2;
						return; // started a download
					}
					n++;
					// FALL THROUGH

				case 2:
					// weapon skin
					Com_sprintf(fn, sizeof (fn), "players/%s/weapon.pcx", model);
					if (!CL_CheckOrDownloadFile (fn)) {
						precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 3;
						return; // started a download
					}
					n++;
					// FALL THROUGH

				case 3:
					// skin
					Com_sprintf(fn, sizeof (fn), "players/%s/%s.pcx", model, skin);
					if (!CL_CheckOrDownloadFile (fn)) {
						precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 4;
						return; // started a download
					}
					n++;
					// FALL THROUGH

				case 4:
					// skin_i
					Com_sprintf(fn, sizeof (fn), "players/%s/%s_i.pcx", model, skin);
					if (!CL_CheckOrDownloadFile (fn)) {
						precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 5;
						return; // started a download
					}

					// move on to next model
					precache_check = CS_PLAYERSKINS + (i + 1) * PLAYER_MULT;
				}
			}
		}
		// precache phase completed
		precache_check = ENV_CNT;
	}

	if (precache_check == ENV_CNT) {
		precache_check = ENV_CNT + 1;

		CM_LoadMap (cl.configStrings[CS_MODELS+1], qTrue, &map_CheckSum);

		if (map_CheckSum != atoi(cl.configStrings[CS_MAPCHECKSUM])) {
			Com_Error (ERR_DROP, "Local map version differs from server: %i != '%s'\n",
				map_CheckSum, cl.configStrings[CS_MAPCHECKSUM]);
			return;
		}
	}

	if ((precache_check > ENV_CNT) && (precache_check < TEXTURE_CNT)) {
		if (allow_download->value && allow_download_maps->value) {
			while (precache_check < TEXTURE_CNT) {
				int n = precache_check++ - ENV_CNT - 1;

				if (n & 1)
					Com_sprintf (fn, sizeof (fn), "env/%s%s.pcx", cl.configStrings[CS_SKY], sky_NameSuffix[n/2]);
				else
					Com_sprintf (fn, sizeof (fn), "env/%s%s.tga", cl.configStrings[CS_SKY], sky_NameSuffix[n/2]);

				if (!CL_CheckOrDownloadFile (fn))
					return; // started a download
			}
		}
		precache_check = TEXTURE_CNT;
	}

	if (precache_check == TEXTURE_CNT) {
		precache_check = TEXTURE_CNT+1;
		precache_tex = 0;
	}

	// confirm existance of textures, download any that don't exist
	if (precache_check == TEXTURE_CNT+1) {
		// from common/cmodel.c
		extern int			cm_NumTexInfo;
		extern mapSurface_t	cm_MapSurfaces[];

		if (allow_download->value && allow_download_maps->value) {
			while (precache_tex < cm_NumTexInfo) {
				char fn[MAX_OSPATH];

				sprintf(fn, "textures/%s.wal", cm_MapSurfaces[precache_tex++].rname);
				if (!CL_CheckOrDownloadFile (fn))
					return; // started a download
			}
		}
		precache_check = TEXTURE_CNT+999;
	}

	//ZOID
	Snd_RegisterSounds ();
	CL_PrepRefresh ();

	MSG_WriteByte (&cls.netChan.message, CLC_STRINGCMD);
	MSG_WriteString (&cls.netChan.message, va("begin %i\n", precache_spawncount));
}


/*
=================
CL_Precache_f

The server will send this command right
before allowing the client into the server
=================
*/
void CL_Precache_f (void) {
	/*
	** Yet another hack to let old demos work
	** the old precache sequence
	*/
	if (Cmd_Argc () < 2) {
		unsigned	map_CheckSum;		// for detecting cheater maps

		CM_LoadMap (cl.configStrings[CS_MODELS+1], qTrue, &map_CheckSum);
		Snd_RegisterSounds ();
		CL_PrepRefresh ();
		return;
	}

	precache_check = CS_MODELS;
	precache_spawncount = atoi (Cmd_Argv (1));
	precache_model = 0;
	precache_model_skin = 0;

	CL_RequestNextDownload ();
}


/*
==================
CL_FixCvarCheats
==================
*/
typedef struct {
	char	*name;
	char	*value;
	cvar_t	*var;
} cheatVar_t;

cheatVar_t	cheatVars[] = {
	{"timescale",			"1"},
	{"timedemo",			"0"},
	{"r_drawworld",			"1"},
	{"cl_testlights",		"0"},
	{"r_fullbright",		"0"},
	{"paused",				"0"},
	{"fixedtime",			"0"},
	{"gl_lightmap",			"0"},
	{"gl_saturatelighting",	"0"},
	{NULL,					NULL}
};

int		numCheatVars;
void CL_FixCvarCheats (void) {
	int			i;
	cheatVar_t	*var;

	if (!strcmp(cl.configStrings[CS_MAXCLIENTS], "1")  || !cl.configStrings[CS_MAXCLIENTS][0])
		return;		// single player can cheat

	/* find all the cvars if we haven't done it yet */
	if (!numCheatVars) {
		while (cheatVars[numCheatVars].name) {
			cheatVars[numCheatVars].var = Cvar_Get (cheatVars[numCheatVars].name, cheatVars[numCheatVars].value, 0);
			numCheatVars++;
		}
	}

	/* make sure they are all set to the proper values */
	for (i=0, var=cheatVars ; i<numCheatVars ; i++, var++) {
		if (strcmp (var->var->string, var->value))
			Cvar_ForceSet (var->name, var->value);
	}
}


/*
==================
CL_FixGloomCvarCheats
==================
*/
cheatVar_t	glmcheatVars[] = {
	{"cl_noskins",			"0"},
	{"cl_testblend",		"0"},
	{"gl_nobind",			"0"},
	{"gl_polyblend",		"1"},
	{"r_fog",				"0"},
	{"r_overbrightbits",	"0"},
	{NULL,					NULL}
};

int		numGlmCheatVars;
void CL_FixGloomCvarCheats (void) {
	int			i;
	cheatVar_t	*var;

	if (!strcmp(cl.configStrings[CS_MAXCLIENTS], "1")  || !cl.configStrings[CS_MAXCLIENTS][0])
		return;		// single player can cheat

	if (!FS_CurrGame ("gloom"))
		return;

	/* find all the cvars if we haven't done it yet */
	if (!numGlmCheatVars) {
		while (glmcheatVars[numGlmCheatVars].name) {
			glmcheatVars[numGlmCheatVars].var = Cvar_Get (glmcheatVars[numGlmCheatVars].name, glmcheatVars[numGlmCheatVars].value, 0);
			numGlmCheatVars++;
		}
	}

	/* make sure they are all set to the proper values */
	for (i=0, var=glmcheatVars ; i<numGlmCheatVars ; i++, var++) {
		if (strcmp (var->var->string, var->value))
			Cvar_ForceSet (var->name, var->value);
	}
}


/*
==================
CL_SendCommand
==================
*/
void CL_SendCommand (void) {
	/* get new key events */
	Sys_SendKeyEvents ();

	/* allow mice or other external controllers to add commands */
	IN_Commands ();

	/* process console commands */
	Cbuf_Execute ();

	/* fix any cheating cvars */
	CL_FixCvarCheats ();
	CL_FixGloomCvarCheats ();

	/* send intentions now */
	CL_SendCmd ();

	/* resend a connection request if necessary */
	CL_CheckForResend ();
}


/*
==================
CL_UpdateCvars
==================
*/
void CL_UpdateCvars (void) {
	/* bound viewsize */
	if (scr_viewsize->integer < 40)
		Cvar_ForceSetValue ("viewsize", 40);
	if (scr_viewsize->integer > 100)
		Cvar_ForceSetValue ("viewsize", 100);

	/* check for sanity */
	if (r_fontscale->value <= 0)
		Cvar_ForceSetValue ("r_fontscale", 1);
	if (r_hudscale->value <= 0)
		Cvar_ForceSetValue ("r_hudscale", 1);
}


/*
==================
CL_Frame
==================
*/
void FASTCALL CL_Frame (int msec) {
	static int  lasttimecalled;
	static int	extratime;

	if (dedicated->integer)
		return;

	extratime += msec;

	if (!cl_timedemo->value) {
		if (CL_ConnectionState (CA_CONNECTED) && (extratime < 100))
			return;			// don't flood packets out while connecting
		if (extratime < (1000.0/cl_maxfps->value))
			return;			// framerate is too high
	}

	CL_UpdateCvars ();

	/* let the mouse activate or deactivate */
	IN_Frame ();

	/* decide the simulation time */
	cls.frameTime = extratime/1000.0;
	cl.time += extratime;
	cls.realTime = g_CurTime;

	extratime = 0;

	if (cls.frameTime > (1.0 / 5.0))
		cls.frameTime = (1.0 / 5.0);

	/* if in the debugger last frame, don't timeout */
	if (msec > 5000)
		cls.netChan.lastReceived = Sys_Milliseconds ();

	/* fetch results from server */
	CL_ReadPackets ();

	/* send a new command message to the server */
	CL_SendCommand ();

	/* predict all unacknowledged movements */
	CL_PredictMovement ();

	/* allow rendering to refresh */
	VID_CheckChanges ();
	if (!cl.refreshPrepped && CL_ConnectionState (CA_ACTIVE))
		CL_PrepRefresh ();

	/* allow the sound system to restart */
	Snd_CheckChanges ();

	/* reload the UI DLL if desired */
	CL_UI_CheckChanges ();

	/* update the screen */
	if (host_speeds->value)
		time_before_ref = Sys_Milliseconds ();

	SCR_UpdateScreen ();

	if (host_speeds->value)
		time_after_ref = Sys_Milliseconds ();

	/* update audio */
	Snd_Update ();
	CDAudio_Update ();

	/* advance local effects for next frame */
	CL_RunDLights ();
	CL_RunLightStyles ();
	CIN_RunCinematic ();
	Con_RunConsole ();

	cls.frameCount++;

	if (log_stats->value && CL_ConnectionState (CA_ACTIVE)) {
		if (!lasttimecalled) {
			lasttimecalled = Sys_Milliseconds ();

			if (logStatsFP)
				fprintf (logStatsFP, "0\n");
		} else {
			int now = Sys_Milliseconds ();

			if (logStatsFP)
				fprintf (logStatsFP, "%d\n", now - lasttimecalled);

			lasttimecalled = now;
		}
	}
}

/*
=======================================================================

	INIT / SHUTDOWN

=======================================================================
*/

/*
=================
CL_Register
=================
*/
void CL_Register (void) {
	/* register our variables */
	adr0		= Cvar_Get ("adr0",			"",			CVAR_ARCHIVE);
	adr1		= Cvar_Get ("adr1",			"",			CVAR_ARCHIVE);
	adr2		= Cvar_Get ("adr2",			"",			CVAR_ARCHIVE);
	adr3		= Cvar_Get ("adr3",			"",			CVAR_ARCHIVE);
	adr4		= Cvar_Get ("adr4",			"",			CVAR_ARCHIVE);
	adr5		= Cvar_Get ("adr5",			"",			CVAR_ARCHIVE);
	adr6		= Cvar_Get ("adr6",			"",			CVAR_ARCHIVE);
	adr7		= Cvar_Get ("adr7",			"",			CVAR_ARCHIVE);

	autosensitivity		= Cvar_Get ("autosensitivity",	"0",	CVAR_ARCHIVE);

	ch_alpha			= Cvar_Get ("ch_alpha",			"1",		CVAR_ARCHIVE);
	ch_pulse			= Cvar_Get ("ch_pulse",			"1",		CVAR_ARCHIVE);
	ch_scale			= Cvar_Get ("ch_scale",			"1",		CVAR_ARCHIVE);
	ch_red				= Cvar_Get ("ch_red",			"1",		CVAR_ARCHIVE);
	ch_green			= Cvar_Get ("ch_green",			"1",		CVAR_ARCHIVE);
	ch_blue				= Cvar_Get ("ch_blue",			"1",		CVAR_ARCHIVE);

	cl_add_particles	= Cvar_Get ("cl_particles",		"1",		0);

	cl_autoskins		= Cvar_Get ("cl_autoskins",		"0",		0);

	cl_add_decals		= Cvar_Get ("cl_decals",		"1",		CVAR_ARCHIVE);
	cl_decal_burnlife	= Cvar_Get ("cl_decal_burnlife","10",		CVAR_ARCHIVE);
	cl_decal_life		= Cvar_Get ("cl_decal_life",	"60",		CVAR_ARCHIVE);
	cl_decal_lod		= Cvar_Get ("cl_decal_lod",		"1",		CVAR_ARCHIVE);
	cl_decal_max		= Cvar_Get ("cl_decal_max",		"4000",		CVAR_ARCHIVE);

	cl_explorattle			= Cvar_Get ("cl_explorattle",		"1",		CVAR_ARCHIVE);
	cl_explorattle_scale	= Cvar_Get ("cl_explorattle_scale",	"0.3",		CVAR_ARCHIVE);

	cl_footsteps		= Cvar_Get ("cl_footsteps",		"1",		0);
	cl_gore				= Cvar_Get ("cl_gore",			"3",		CVAR_ARCHIVE);
	cl_gun				= Cvar_Get ("cl_gun",			"1",		0);
	cl_lightlevel		= Cvar_Get ("r_lightlevel",		"0",		0);
	cl_maxfps			= Cvar_Get ("cl_maxfps",		"90",		0);
	cl_noskins			= Cvar_Get ("cl_noskins",		"0",		0);
	cl_paused			= Cvar_Get ("paused",			"0",		0);
	cl_predict			= Cvar_Get ("cl_predict",		"1",		0);

	cl_railred			= Cvar_Get ("cl_railred",		"1",		CVAR_ARCHIVE);
	cl_railgreen		= Cvar_Get ("cl_railgreen",		"1",		CVAR_ARCHIVE);
	cl_railblue			= Cvar_Get ("cl_railblue",		"1",		CVAR_ARCHIVE);
	cl_railtrail		= Cvar_Get ("cl_railtrail",		"2",		CVAR_ARCHIVE);


	cl_showclamp		= Cvar_Get ("showclamp",		"0",		0);
	cl_showfps			= Cvar_Get ("cl_showfps",		"1",		CVAR_ARCHIVE);
	cl_showmiss			= Cvar_Get ("cl_showmiss",		"0",		0);
	cl_shownet			= Cvar_Get ("cl_shownet",		"0",		0);
	cl_showping			= Cvar_Get ("cl_showping",		"1",		CVAR_ARCHIVE);
	cl_showtime			= Cvar_Get ("cl_showtime",		"1",		CVAR_ARCHIVE);
	cl_smokelinger		= Cvar_Get ("cl_smokelinger",	"0",		CVAR_ARCHIVE);

	cl_spiralred		= Cvar_Get ("cl_spiralred",		"0",		CVAR_ARCHIVE);
	cl_spiralgreen		= Cvar_Get ("cl_spiralgreen",	"0.6",		CVAR_ARCHIVE);
	cl_spiralblue		= Cvar_Get ("cl_spiralblue",	"0.9",		CVAR_ARCHIVE);

	cl_stereo_separation	= Cvar_Get ("cl_stereo_separation",	"0.4",	CVAR_ARCHIVE);
	cl_stereo				= Cvar_Get ("cl_stereo",			"0",	0);

	cl_timedemo			= Cvar_Get ("timedemo",			"0",		0);
	cl_timeout			= Cvar_Get ("cl_timeout",		"120",		0);
	cl_timestamp		= Cvar_Get ("cl_timestamp",		"0",		CVAR_ARCHIVE);
	cl_vwep				= Cvar_Get ("cl_vwep",			"1",		CVAR_ARCHIVE);

	con_alpha			= Cvar_Get ("con_alpha",		"0.6",		CVAR_ARCHIVE);
	con_clock			= Cvar_Get ("con_clock",		"1",		CVAR_ARCHIVE);
	con_drop			= Cvar_Get ("con_drop",			"0.5",		CVAR_ARCHIVE);
	con_scroll			= Cvar_Get ("con_scroll",		"2",		CVAR_ARCHIVE);

	freelook			= Cvar_Get ("freelook",			"0",		CVAR_ARCHIVE);

	glm_advgas			= Cvar_Get ("glm_advgas",		"0",		CVAR_ARCHIVE);
	glm_advstingfire	= Cvar_Get ("glm_advstingfire", "1",		CVAR_ARCHIVE);
	glm_blobtype		= Cvar_Get ("glm_blobtype",		"1",		CVAR_ARCHIVE);
	glm_bluestingfire	= Cvar_Get ("glm_bluestingfire","0",		CVAR_ARCHIVE);
	glm_flashpred		= Cvar_Get ("glm_flashpred",	"0",		CVAR_ARCHIVE);
	glm_flwhite			= Cvar_Get ("glm_flwhite",		"1",		CVAR_ARCHIVE);
	glm_jumppred		= Cvar_Get ("glm_jumppred",		"0",		CVAR_ARCHIVE);
	glm_showclass		= Cvar_Get ("glm_showclass",	"1",		CVAR_ARCHIVE);

	lookspring			= Cvar_Get ("lookspring",		"0",		CVAR_ARCHIVE);
	lookstrafe			= Cvar_Get ("lookstrafe",		"0",		CVAR_ARCHIVE);

	ui_sensitivity		= Cvar_Get ("ui_sensitivity",	"1",		CVAR_ARCHIVE);

	m_forward			= Cvar_Get ("m_forward",		"1",		0);
	m_pitch				= Cvar_Get ("m_pitch",			"0.022",	CVAR_ARCHIVE);
	m_side				= Cvar_Get ("m_side",			"1",		0);
	m_yaw				= Cvar_Get ("m_yaw",			"0.022",	0);

	part_cull			= Cvar_Get ("part_cull",		"1",		CVAR_ARCHIVE);
	part_degree			= Cvar_Get ("part_degree",		"1",		CVAR_ARCHIVE);
	part_lod			= Cvar_Get ("part_lod",			"1",		CVAR_ARCHIVE);

	rcon_clpassword		= Cvar_Get ("rcon_password",	"",		0);
	rcon_address		= Cvar_Get ("rcon_address",		"",		0);

	r_fontscale			= Cvar_Get ("r_fontscale",		"1",		CVAR_ARCHIVE);
	r_hudscale			= Cvar_Get ("r_hudscale",		"1",		CVAR_ARCHIVE);

	sensitivity			= Cvar_Get ("sensitivity",		"3",	CVAR_ARCHIVE);

	/* userinfo cvars */
	fov					= Cvar_Get ("fov",			"90",			(CVAR_USERINFO|CVAR_ARCHIVE));
	gender				= Cvar_Get ("gender",		"male",			(CVAR_USERINFO|CVAR_ARCHIVE));
	hand				= Cvar_Get ("hand",			"0",			(CVAR_USERINFO|CVAR_ARCHIVE));
	info_password		= Cvar_Get ("password",		"",				CVAR_USERINFO);
	info_spectator		= Cvar_Get ("spectator",	"0",			CVAR_USERINFO);
	msg					= Cvar_Get ("msg",			"1",			(CVAR_USERINFO|CVAR_ARCHIVE));
	name				= Cvar_Get ("name",			"unnamed",		(CVAR_USERINFO|CVAR_ARCHIVE));
	rate				= Cvar_Get ("rate",			"8000",			(CVAR_USERINFO|CVAR_ARCHIVE));
	skin				= Cvar_Get ("skin",			"male/grunt",	(CVAR_USERINFO|CVAR_ARCHIVE));

	/* register our commands */
	Cmd_AddCommand ("changing",		CL_Changing_f);
	Cmd_AddCommand ("connect",		CL_Connect_f);
	Cmd_AddCommand ("cmd",			CL_ForwardToServer_f);
	Cmd_AddCommand ("disconnect",	CL_Disconnect_f);
	Cmd_AddCommand ("download",		CL_Download_f);
	Cmd_AddCommand ("exit",			CL_Quit_f);
	Cmd_AddCommand ("pause",		CL_Pause_f);
	Cmd_AddCommand ("pingservers",	CL_PingServers_f);
	Cmd_AddCommand ("precache",		CL_Precache_f);
	Cmd_AddCommand ("quit",			CL_Quit_f);
	Cmd_AddCommand ("rcon",			CL_Rcon_f);
	Cmd_AddCommand ("reconnect",	CL_Reconnect_f);
	Cmd_AddCommand ("record",		CL_Record_f);
	Cmd_AddCommand ("setenv",		CL_Setenv_f);
	Cmd_AddCommand ("skins",		CL_Skins_f);
	Cmd_AddCommand ("stop",			CL_Stop_f);
	Cmd_AddCommand ("userinfo",		CL_Userinfo_f);

	/*
	** Forward to server commands...
	** The only thing this does is allow command completion to work -- all unknown
	** commands are automatically forwarded to the server
	*/
	Cmd_AddCommand ("wave",		NULL);
	Cmd_AddCommand ("inven",	NULL);
	Cmd_AddCommand ("kill",		NULL);
	Cmd_AddCommand ("use",		NULL);
	Cmd_AddCommand ("drop",		NULL);
	Cmd_AddCommand ("say",		NULL);
	Cmd_AddCommand ("say_team",	NULL);
	Cmd_AddCommand ("info",		NULL);
	Cmd_AddCommand ("prog",		NULL);
	Cmd_AddCommand ("give",		NULL);
	Cmd_AddCommand ("god",		NULL);
	Cmd_AddCommand ("notarget",	NULL);
	Cmd_AddCommand ("noclip",	NULL);
	Cmd_AddCommand ("invuse",	NULL);
	Cmd_AddCommand ("invprev",	NULL);
	Cmd_AddCommand ("invnext",	NULL);
	Cmd_AddCommand ("invdrop",	NULL);
	Cmd_AddCommand ("weapnext",	NULL);
	Cmd_AddCommand ("weapprev",	NULL);
}


/*
====================
CL_Init
====================
*/
void CL_Init (void) {
	if (dedicated->integer)
		return;	// nothing running on the client

	CL_SetConnectState (CA_DISCONNECTED);
	cls.realTime = Sys_Milliseconds ();

	CL_Register ();

	SCR_Init ();
	cls.disableScreen = qFalse;	// don't draw yet

	// all archived variables will now be loaded
#ifdef VID_INITFIRST
	VID_Init ();
	Snd_Init ();	// sound must be initialized after window is created
#else
	Snd_Init ();
	VID_Init ();
#endif
	View_Init ();
	CL_UI_Init ();
	
	net_Message.data = net_MessageBuffer;
	net_Message.maxSize = sizeof (net_MessageBuffer);

	CDAudio_Init ();
	CL_InitInput ();
	IN_Init ();
}


/*
===============
CL_Shutdown

FIXME: this is a callback from Sys_Quit Sys_Error and Com_Error. It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void CL_Shutdown (void) {
	static qBool isDown = qFalse;
	
	if (isDown) {
		printf ("recursive shutdown\n");
		return;
	}
	isDown = qTrue;

	Com_WriteConfig ();

	SCR_Shutdown ();

	CDAudio_Shutdown ();
	Snd_Shutdown ();

	IN_Shutdown ();

	VID_Shutdown ();
	View_Shutdown ();
	CL_UI_Shutdown ();
	Con_Shutdown ();
}
