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

#define USE_BYTESWAP
#include "cl_local.h"

cVar_t *allow_download;
cVar_t *allow_download_players;
cVar_t *allow_download_models;
cVar_t *allow_download_sounds;
cVar_t *allow_download_maps;

cVar_t	*autosensitivity;

cVar_t	*cl_lightlevel;
cVar_t	*cl_maxfps;
cVar_t	*r_maxfps;
cVar_t	*cl_paused;
cVar_t	*cl_protocol;

cVar_t	*cl_shownet;

cVar_t	*cl_stereo_separation;
cVar_t	*cl_stereo;

cVar_t	*cl_timedemo;
cVar_t	*cl_timeout;
cVar_t	*cl_timestamp;

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

cVar_t	*sensitivity;

cVar_t	*scr_conspeed;
cVar_t	*scr_printspeed;

cVar_t	*scr_netgraph;
cVar_t	*scr_timegraph;
cVar_t	*scr_debuggraph;
cVar_t	*scr_graphheight;
cVar_t	*scr_graphscale;
cVar_t	*scr_graphshift;

cVar_t	*scr_graphalpha;

clientState_t	cl;
clientStatic_t	cls;
clMedia_t		clMedia;

entityState_t	clBaseLines[MAX_CS_EDICTS];
entityState_t	clParseEntities[MAX_PARSE_ENTITIES];

netAdr_t	rconLastTo;

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
	if ((cls.connectState <= CA_CONNECTED) || (*cmd == '-') || (*cmd == '+')) {
		Com_Printf (0, "Unknown command \"%s" S_STYLE_RETURN "\"\n", cmd);
		return;
	}

	MSG_WriteByte (&cls.netChan.message, CLC_STRINGCMD);
	MSG_Print (&cls.netChan.message, cmd);
	if (Cmd_Argc () > 1) {
		MSG_Print (&cls.netChan.message, " ");
		MSG_Print (&cls.netChan.message, Cmd_Args());
	}

	cls.forcePacket = qTrue;
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
		Com_Printf (0, S_COLOR_YELLOW "Bad server address\n");
		cls.connectCount = 0;
		cls.connectTime = 0;
		return;
	}

	if (adr.port == 0)
		adr.port = BigShort (PORT_SERVER);

	port = Cvar_VariableInteger ("qport");
	cvarUserInfoModified = qFalse;

	if (!cls.serverProtocol)
		cls.serverProtocol = cl_protocol->integer;
	if (!cls.serverProtocol)
		cls.serverProtocol = ENHANCED_PROTOCOL_VERSION;

	Com_Printf (PRNT_DEV, "CL_SendConnectPacket: protocol=%d, port=%d, challenge=%u\n", cls.serverProtocol, port, cls.challenge);
	Netchan_OutOfBandPrint (NS_CLIENT, adr, "connect %i %i %i \"%s\"\n",
		cls.serverProtocol, port, cls.challenge, Cvar_BitInfo (CVAR_USERINFO));
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
CL_SetState
=====================
*/
void CL_SetState (int state) {
	cls.connectState = state;
	Com_SetClientState (state);

	switch (state) {
		case CA_DISCONNECTED:
			Con_Close ();
			CL_UIModule_MainMenu ();
			Key_SetKeyDest (KD_MENU);
			break;

		case CA_CONNECTING:
			Con_Close ();
			Key_SetKeyDest (KD_GAME);
			break;
	}
}


/*
=====================
CL_ClearState
=====================
*/
void CL_ClearState (void) {
	Snd_StopAllSounds ();

	// wipe the entire cl structure
	memset (&cl, 0, sizeof (cl));
	memset (&clBaseLines, 0, sizeof (clBaseLines));
	memset (&clParseEntities, 0, sizeof (clParseEntities));

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

	if (cls.connectState == CA_UNINITIALIZED)
		return;
	if (cls.connectState == CA_DISCONNECTED)
		goto done;

	if (cl_timedemo && cl_timedemo->integer) {
		int	time;
		
		time = Sys_Milliseconds () - cl.timeDemoStart;
		if (time > 0)
			Com_Printf (0, "%i frames, %3.1f seconds: %3.1f fps\n", cl.timeDemoFrames,
				time/1000.0, cl.timeDemoFrames*1000.0 / time);
	}

	cls.connectTime = 0;
	cls.connectCount = 0;

	CIN_StopCinematic ();

	if (cls.demoRecording)
		CL_Stop_f ();

	// send a disconnect message to the server
	final[0] = CLC_STRINGCMD;
	strcpy ((char *)final+1, "disconnect");
	Netchan_Transmit (&cls.netChan, Q_strlen (final), final);
	Netchan_Transmit (&cls.netChan, Q_strlen (final), final);
	Netchan_Transmit (&cls.netChan, Q_strlen (final), final);

	CM_UnloadMap ();
	CL_RestartMedia ();
	CL_FreeLoc ();

	// stop download
	if (cls.download.file) {
		fclose (cls.download.file);
		cls.download.file = NULL;
	}

	CL_ClearState ();
	CL_SetState (CA_DISCONNECTED);

done:
	memset (&cls.serverName, 0, sizeof (cls.serverName));
	cls.serverProtocol = 0;

	Cvar_ForceSet ("$mapname", "");
	Cvar_ForceSet ("$game", "");

	//r1: swap games if needed
	Cvar_GetLatchedVars (CVAR_LATCH_SERVER);

	// drop loading plaque unless this is the initial game start
	SCR_EndLoadingPlaque ();	// get rid of loading plaque
}


/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
static void CL_ConnectionlessPacket (void) {
	char		*s, *c, *printStr;
	netAdr_t	remote;
	qBool		isPrint = qFalse;

	MSG_BeginReading (&netMessage);
	MSG_ReadLong (&netMessage);	// skip the -1

	s = MSG_ReadStringLine (&netMessage);

	Cmd_TokenizeString (s, qFalse);

	c = Cmd_Argv (0);

	// server connection
	if (!strcmp (c, "client_connect")) {
		Com_Printf (0, "%s: %s\n", NET_AdrToString (netFrom), c);

		switch (cls.connectState) {
		case CA_CONNECTED:
			Com_Printf (0, S_COLOR_YELLOW "Dup connect received. Ignored.\n");
			return;

		case CA_DISCONNECTED:
			Com_Printf (PRNT_DEV, "Received connect when disconnected. Ignored.\n");
			return;

		case CA_ACTIVE:
			Com_Printf (PRNT_DEV, "Illegal connect when already connected! (q2msgs?). Ignored.\n");
			return;
		}

		Com_Printf (PRNT_DEV, "client_connect: new\n");

		Netchan_Setup (NS_CLIENT, &cls.netChan, netFrom, cls.serverProtocol, cls.quakePort);
		MSG_WriteChar (&cls.netChan.message, CLC_STRINGCMD);
		MSG_WriteString (&cls.netChan.message, "new");	
		CL_SetState (CA_CONNECTED);

		cls.forcePacket = qTrue;
		return;
	}

	// server responding to a status broadcast
	if (!strcmp (c, "info")) {
		if (CL_UIModule_ParseServerInfo (NET_AdrToString (netFrom), MSG_ReadString (&netMessage)))
			return;
	}

	// print command from somewhere
	if (!strcmp (c, "print")) {
		isPrint = qTrue;
		printStr = MSG_ReadString (&netMessage);
		if (CL_UIModule_ParseServerStatus (NET_AdrToString (netFrom), printStr))
			return;
	}

	/*
	** R1CH: security check. (only allow from current connected server and last
	** destination client sent an rcon to)
	*/
	NET_StringToAdr (cls.serverName, &remote);
	if (!NET_CompareBaseAdr (netFrom, remote) && !NET_CompareBaseAdr (netFrom, rconLastTo)) {
		Com_Printf (0, S_COLOR_YELLOW "Illegal %s from %s. Ignored.\n", c, NET_AdrToString (netFrom));
		return;
	}

	Com_Printf (0, "%s: %s\n", NET_AdrToString (netFrom), c);

	// print command from somewhere
	if (isPrint) {
		//BIG HACK to allow new client on old server!
		Com_Printf (0, "%s", printStr);
		if (!strstr (printStr, "full") &&
			!strstr (printStr, "locked") &&
			!strncmp (printStr, "Server is ", 10) &&
			cls.serverProtocol != ORIGINAL_PROTOCOL_VERSION)
		{
			Com_Printf (0, "Retrying with protocol %d.\n", ORIGINAL_PROTOCOL_VERSION);
			cls.serverProtocol = ORIGINAL_PROTOCOL_VERSION;
			//force immediate retry
			cls.connectTime = -99999;

		}
		return;
	}

	// remote command from gui front end
	if (!strcmp (c, "cmd")) {
		if (!NET_IsLocalAddress (netFrom)) {
			Com_Printf (0, S_COLOR_YELLOW "Command packet from remote host. Ignored.\n");
			return;
		}
		Sys_AppActivate ();
		s = MSG_ReadString (&netMessage);
		Cbuf_AddText (s);
		Cbuf_AddText ("\n");
		return;
	}

	// ping from somewhere
	if (!strcmp (c, "ping")) {
		// R1: stop users from exploiting your connecting, this can bog down even cable users!
		//Netchan_OutOfBandPrint (NS_CLIENT, netFrom, "ack");
		return;
	}

	// challenge from the server we are connecting to
	if (!strcmp (c, "challenge")) {
		cls.challenge = atoi (Cmd_Argv (1));

		cls.connectTime = cls.realTime; // R1: reset timer to stop duplicates
		CL_SendConnectPacket ();
		return;
	}

	// echo request from server
	if (!strcmp (c, "echo")) {
		Netchan_OutOfBandPrint (NS_CLIENT, netFrom, "%s", Cmd_Argv (1));
		return;
	}

	Com_Printf (0, "CL_ConnectionlessPacket: Unknown command.\n");
}

/*
=======================================================================

	MEDIA

=======================================================================
*/

/*
=================
CL_InitImageMedia
=================
*/
void CL_InitImageMedia (void) {
	clMedia.consoleImage	= Img_RegisterImage ("pics/conback.tga", IF_NOMIPMAP);
}


/*
=================
CL_InitSoundMedia
=================
*/
void CL_InitSoundMedia (void) {
	int		i;

	clMedia.talkSfx			= Snd_RegisterSound ("misc/talk.wav");

	for (i=1 ; i<MAX_CS_SOUNDS ; i++) {
		if (!cl.configStrings[CS_SOUNDS+i][0])
			break;

		Snd_RegisterSound (cl.configStrings[CS_SOUNDS+i]);
		Sys_SendKeyEvents ();	// pump message loop
	}

	CL_CGModule_RegisterSounds ();
	CL_UIModule_RegisterSounds ();
}


/*
=================
CL_InitMedia
=================
*/
void CL_InitMedia (void) {
	if (clMedia.initialized)
		return;
	if (cls.connectState == CA_UNINITIALIZED)
		return;

	clMedia.initialized = qTrue;

	// free all sounds
	Snd_FreeSounds ();

	// init ui api
	CL_UIAPI_Init ();

	// register our media
	CL_InitImageMedia ();
	CL_InitSoundMedia ();

	// check memory integrity
	Mem_CheckGlobalIntegrity ();
}


/*
=================
CL_ShutdownMedia
=================
*/
void CL_ShutdownMedia (void) {
	if (!clMedia.initialized)
		return;

	clMedia.initialized = qFalse;

	// shutdown cgame
	CL_CGameAPI_Shutdown (qFalse);

	// shutdown ui
	CL_UIAPI_Shutdown (qFalse);

	// stop and free all sounds
	Snd_StopAllSounds ();
	Snd_FreeSounds ();

	// check memory integrity
	Mem_CheckGlobalIntegrity ();
}


/*
=================
CL_RestartMedia
=================
*/
void CL_RestartMedia (void) {
	CL_ShutdownMedia ();
	CL_InitMedia ();
}

/*
=======================================================================

	DOWNLOAD

=======================================================================
*/

static int	clDownloadCheck; // for autodownload of precache items
static int	clDownloadSpawnCount;
static int	clDownloadTexNum;

static byte	*clDownloadModel; // used for skin checking in alias models
static int	clDownloadModelSkin;

#define PLAYER_MULT 5

// ENV_CNT is map load, ENV_CNT+1 is first env map
#define ENV_CNT (CS_PLAYERSKINS + MAX_CS_CLIENTS * PLAYER_MULT)
#define TEXTURE_CNT (ENV_CNT+13)

extern char *sky_NameSuffix[6];

/*
==============
CL_ResetDownload
==============
*/
static void CL_ResetDownload (void) {
	clDownloadCheck = CS_MODELS;
	clDownloadSpawnCount = atoi (Cmd_Argv (1));
	clDownloadModel = 0;
	clDownloadModelSkin = 0;
}

/*
==============
CL_RequestNextDownload
==============
*/
void CL_RequestNextDownload (void) {
	uInt			mapCheckSum;		// for detecting cheater maps
	char			fileName[MAX_OSPATH];
	dMd2Header_t	*md2Header;

	if (cls.connectState != CA_CONNECTED)
		return;

	//
	// if downloading is off by option, skip to loading the map
	//
	if (!allow_download->value && (clDownloadCheck < ENV_CNT))
		clDownloadCheck = ENV_CNT;

	if (clDownloadCheck == CS_MODELS) {
		// confirm map
		clDownloadCheck = CS_MODELS+2; // 0 isn't used
		if (allow_download_maps->value) {
			if (!CL_CheckOrDownloadFile (cl.configStrings[CS_MODELS+1]))
				return; // started a download
		}
	}

	//
	// models
	//
	if ((clDownloadCheck >= CS_MODELS) && (clDownloadCheck < CS_MODELS+MAX_CS_MODELS)) {
		if (allow_download_models->value) {
			while ((clDownloadCheck < CS_MODELS+MAX_CS_MODELS) && cl.configStrings[clDownloadCheck][0]) {
				if ((cl.configStrings[clDownloadCheck][0] == '*') || (cl.configStrings[clDownloadCheck][0] == '#')) {
					clDownloadCheck++;
					continue;
				}
				if (clDownloadModelSkin == 0) {
					if (!CL_CheckOrDownloadFile (cl.configStrings[clDownloadCheck])) {
						clDownloadModelSkin = 1;
						return; // started a download
					}
					clDownloadModelSkin = 1;
				}

				// checking for skins in the model
				if (!clDownloadModel) {
					FS_LoadFile (cl.configStrings[clDownloadCheck], (void **)&clDownloadModel);
					if (!clDownloadModel) {
						clDownloadModelSkin = 0;
						clDownloadCheck++;
						clDownloadModel = NULL;
						continue; // couldn't load it
					}
					// hacks! yay!
					if (LittleLong (*(uInt *)clDownloadModel) != MD2_HEADER) {
						clDownloadModelSkin = 0;
						clDownloadCheck++;

						FS_FreeFile (clDownloadModel);
						clDownloadModel = NULL;
						continue; // not an alias model
					}
					md2Header = (dMd2Header_t *)clDownloadModel;
					if (LittleLong (md2Header->version) != MD2_MODEL_VERSION) {
						clDownloadCheck++;
						clDownloadModelSkin = 0;

						FS_FreeFile (clDownloadModel);
						clDownloadModel = NULL;
						continue; // couldn't load it
					}
				}

				md2Header = (dMd2Header_t *)clDownloadModel;
				while (clDownloadModelSkin - 1 < LittleLong (md2Header->numSkins)) {
					if (!CL_CheckOrDownloadFile ((char *)clDownloadModel +
						LittleLong (md2Header->ofsSkins) +
						(clDownloadModelSkin - 1)*MD2_MAX_SKINNAME)) {
						clDownloadModelSkin++;

						FS_FreeFile (clDownloadModel);
						clDownloadModel = NULL;
						return; // started a download
					}
					clDownloadModelSkin++;
				}

				if (clDownloadModel) { 
					FS_FreeFile (clDownloadModel);
					clDownloadModel = NULL;
				}

				clDownloadModelSkin = 0;
				clDownloadCheck++;
			}
		}
		clDownloadCheck = CS_SOUNDS;
	}

	//
	// sound
	//
	if ((clDownloadCheck >= CS_SOUNDS) && (clDownloadCheck < CS_SOUNDS+MAX_CS_SOUNDS)) { 
		if (allow_download_sounds->value) {
			if (clDownloadCheck == CS_SOUNDS)
				clDownloadCheck++; // zero is blank

			while ((clDownloadCheck < CS_SOUNDS+MAX_CS_SOUNDS) && (cl.configStrings[clDownloadCheck][0])) {
				if (cl.configStrings[clDownloadCheck][0] == '*') {
					clDownloadCheck++;
					continue;
				}

				Q_snprintfz (fileName, sizeof (fileName), "sound/%s", cl.configStrings[clDownloadCheck++]);
				if (!CL_CheckOrDownloadFile (fileName))
					return; // started a download
			}
		}
		clDownloadCheck = CS_IMAGES;
	}

	//
	// images
	//
	if ((clDownloadCheck >= CS_IMAGES) && (clDownloadCheck < CS_IMAGES+MAX_CS_IMAGES)) {
		if (clDownloadCheck == CS_IMAGES)
			clDownloadCheck++; // zero is blank

		while ((clDownloadCheck < CS_IMAGES+MAX_CS_IMAGES) && (cl.configStrings[clDownloadCheck][0])) {
			Q_snprintfz (fileName, sizeof (fileName), "pics/%s.pcx", cl.configStrings[clDownloadCheck++]);
			if (!CL_CheckOrDownloadFile (fileName))
				return; // started a download
		}
		clDownloadCheck = CS_PLAYERSKINS;
	}

	//
	// skins are special, since a player has three things to download:
	// - model
	// - weapon model
	// - skin
	// so clDownloadCheck is now * 3
	//
	if ((clDownloadCheck >= CS_PLAYERSKINS) && (clDownloadCheck < (CS_PLAYERSKINS + MAX_CS_CLIENTS * PLAYER_MULT))) {
		if (allow_download_players->value) {
			while (clDownloadCheck < (CS_PLAYERSKINS + MAX_CS_CLIENTS * PLAYER_MULT)) {
				int		i, n;
				char	model[MAX_QPATH];
				char	skin[MAX_QPATH];
				char	*p;

				i = (clDownloadCheck - CS_PLAYERSKINS)/PLAYER_MULT;
				n = (clDownloadCheck - CS_PLAYERSKINS)%PLAYER_MULT;

				if (!cl.configStrings[CS_PLAYERSKINS+i][0]) {
					clDownloadCheck = CS_PLAYERSKINS + (i + 1) * PLAYER_MULT;
					continue;
				}

				if ((p = strchr(cl.configStrings[CS_PLAYERSKINS+i], '\\')) != NULL)
					p++;
				else
					p = cl.configStrings[CS_PLAYERSKINS+i];

				Q_strncpyz (model, p, sizeof (model));
				p = strchr (model, '/');
				if (!p)
					p = strchr (model, '\\');
				if (p) {
					*p++ = 0;
					Q_strncpyz (skin, p, sizeof (skin));
				}
				else
					*skin = 0;

				switch (n) {
				case 0:
					// model
					Q_snprintfz (fileName, sizeof (fileName), "players/%s/tris.md2", model);
					if (!CL_CheckOrDownloadFile (fileName)) {
						clDownloadCheck = CS_PLAYERSKINS + i * PLAYER_MULT + 1;
						return; // started a download
					}
					n++;
					// FALL THROUGH

				case 1:
					// weapon model
					Q_snprintfz (fileName, sizeof (fileName), "players/%s/weapon.md2", model);
					if (!CL_CheckOrDownloadFile (fileName)) {
						clDownloadCheck = CS_PLAYERSKINS + i * PLAYER_MULT + 2;
						return; // started a download
					}
					n++;
					// FALL THROUGH

				case 2:
					// weapon skin
					Q_snprintfz (fileName, sizeof (fileName), "players/%s/weapon.pcx", model);
					if (!CL_CheckOrDownloadFile (fileName)) {
						clDownloadCheck = CS_PLAYERSKINS + i * PLAYER_MULT + 3;
						return; // started a download
					}
					n++;
					// FALL THROUGH

				case 3:
					// skin
					Q_snprintfz (fileName, sizeof (fileName), "players/%s/%s.pcx", model, skin);
					if (!CL_CheckOrDownloadFile (fileName)) {
						clDownloadCheck = CS_PLAYERSKINS + i * PLAYER_MULT + 4;
						return; // started a download
					}
					n++;
					// FALL THROUGH

				case 4:
					// skin_i
					Q_snprintfz (fileName, sizeof (fileName), "players/%s/%s_i.pcx", model, skin);
					if (!CL_CheckOrDownloadFile (fileName)) {
						clDownloadCheck = CS_PLAYERSKINS + i * PLAYER_MULT + 5;
						return; // started a download
					}

					// move on to next model
					clDownloadCheck = CS_PLAYERSKINS + (i + 1) * PLAYER_MULT;
				}
			}
		}
		// precache phase completed
		clDownloadCheck = ENV_CNT;
	}

	//
	// map
	//
	if (clDownloadCheck == ENV_CNT) {
		clDownloadCheck = ENV_CNT + 1;

		CM_LoadMap (cl.configStrings[CS_MODELS+1], qTrue, &mapCheckSum);

		if (mapCheckSum != atoi(cl.configStrings[CS_MAPCHECKSUM])) {
			Com_Error (ERR_DROP, "Local map version differs from server: %i != '%s'",
				mapCheckSum, cl.configStrings[CS_MAPCHECKSUM]);
			return;
		}

		CL_LoadLoc (cl.configStrings[CS_MODELS+1]);
	}

	//
	// sky box env
	//
	if ((clDownloadCheck > ENV_CNT) && (clDownloadCheck < TEXTURE_CNT)) {
		if (allow_download->value && allow_download_maps->value) {
			while (clDownloadCheck < TEXTURE_CNT) {
				int n = clDownloadCheck++ - ENV_CNT - 1;

				if (n & 1)
					Q_snprintfz (fileName, sizeof (fileName), "env/%s%s.pcx", cl.configStrings[CS_SKY], sky_NameSuffix[n/2]);
				else
					Q_snprintfz (fileName, sizeof (fileName), "env/%s%s.tga", cl.configStrings[CS_SKY], sky_NameSuffix[n/2]);

				if (!CL_CheckOrDownloadFile (fileName))
					return; // started a download
			}
		}
		clDownloadCheck = TEXTURE_CNT;
	}

	if (clDownloadCheck == TEXTURE_CNT) {
		clDownloadCheck = TEXTURE_CNT+1;
		clDownloadTexNum = 0;
	}

	//
	// confirm existance of textures, download any that don't exist
	//
	if (clDownloadCheck == TEXTURE_CNT+1) {
		int			numTexInfo;

		numTexInfo = CM_NumTexInfo ();

		if (allow_download->value && allow_download_maps->value) {
			while (clDownloadTexNum < numTexInfo) {

				Q_snprintfz (fileName, sizeof (fileName), "textures/%s.wal", CM_SurfRName (clDownloadTexNum++));
				if (!CL_CheckOrDownloadFile (fileName))
					return; // started a download
			}
		}
		clDownloadCheck = TEXTURE_CNT+999;
	}

	CL_CGameAPI_Init ();

	MSG_WriteByte (&cls.netChan.message, CLC_STRINGCMD);
	MSG_WriteString (&cls.netChan.message, Q_VarArgs ("begin %i\n", clDownloadSpawnCount));
	cls.forcePacket = qTrue;
}

/*
=======================================================================

	FRAME HANDLING

=======================================================================
*/

/*
==================
CL_ForcePacket
==================
*/
void CL_ForcePacket (void) {
	cls.forcePacket = qTrue;
}


/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
static void CL_CheckForResend (void) {
	netAdr_t	adr;

	// if the local server is running and we aren't then connect
	if ((cls.connectState == CA_DISCONNECTED) && Com_ServerState ()) {
		CL_SetState (CA_CONNECTING);
		Q_strncpyz (cls.serverName, "localhost", sizeof (cls.serverName));
		// we don't need a challenge on the localhost
		CL_SendConnectPacket ();
		return;
	}

	// resend if we haven't gotten a reply yet
	if (cls.connectState != CA_CONNECTING)
		return;

	// only resend every NET_RETRYDELAY
	if ((cls.realTime - cls.connectTime) < NET_RETRYDELAY)
		return;

	// check if it's a bad server address
	if (!NET_StringToAdr (cls.serverName, &adr)) {
		Com_Printf (0, S_COLOR_YELLOW "Bad server address\n");
		CL_SetState (CA_DISCONNECTED);
		return;
	}

	if (adr.port == 0)
		adr.port = BigShort (PORT_SERVER);

	cls.connectCount++;
	cls.connectTime = cls.realTime;	// for retransmit requests

	Com_Printf (0, "Connecting to %s...\n", cls.serverName);

	Netchan_OutOfBandPrint (NS_CLIENT, adr, "getchallenge\n");
}


/*
=================
CL_ReadPackets
=================
*/
static void CL_ReadPackets (void) {
	while (NET_GetPacket (NS_CLIENT, &netFrom, &netMessage)) {
		// remote command packet
		if (*(int *)netMessage.data == -1) {
			CL_ConnectionlessPacket ();
			continue;
		}

		if ((cls.connectState == CA_DISCONNECTED) || (cls.connectState == CA_CONNECTING))
			continue;		// dump it if not connected

		if (netMessage.curSize < 8) {
			Com_Printf (0, S_COLOR_YELLOW "%s: Runt packet\n", NET_AdrToString(netFrom));
			continue;
		}

		// packet from server
		if (!NET_CompareAdr (netFrom, cls.netChan.remoteAddress)) {
			Com_Printf (PRNT_DEV, S_COLOR_YELLOW "%s:sequenced packet without connection\n", NET_AdrToString (netFrom));
			continue;
		}
		
		if (!Netchan_Process (&cls.netChan, &netMessage))
			continue;	// shunned for some reason

		CL_ParseServerMessage ();
	}

	// check timeout
	if ((cls.connectState >= CA_CONNECTED) && (cls.realTime - cls.netChan.lastReceived) > (cl_timeout->value*1000)) {
		if (++cl.timeOutCount > 5) {
			// timeoutcount saves debugger
			Com_Printf (0, "\n" S_COLOR_YELLOW "Server connection timed out.\n");
			CL_Disconnect ();
			return;
		}
	}
	else
		cl.timeOutCount = 0;
}


/*
==================
CL_UpdateCvars
==================
*/
static void CL_UpdateCvars (void) {
	// check for sanity
	if (r_fontscale->modified) {
		r_fontscale->modified = qFalse;
		if (r_fontscale->value <= 0)
			Cvar_VariableForceSetValue (r_fontscale, 1);
	}
}


/*
==================
CL_SendCommand
==================
*/
static void CL_SendCommand (void) {
	// send client packet to server
	CL_SendCmd ();

	// resend a connection request if necessary
	CL_CheckForResend ();
}


/*
==================
CL_RefreshInputs
==================
*/
static void CL_RefreshInputs (void) {
	// process new key events
	Sys_SendKeyEvents ();

	// process mice & joystick events
	IN_Commands ();

	// process console commands
	Cbuf_Execute ();

	// process packets from server
	CL_ReadPackets ();

	// update usercmd state
	CL_RefreshCmd ();
}


/*
==================
CL_Frame
==================
*/
#define FRAMETIME_MAX 0.5
void __fastcall CL_Frame (int msec) {
	static int	packetDelta = 0;
	static int	refreshDelta = 0;
	static int	miscDelta = 1000;
	qBool		packetFrame = qTrue;
	qBool		refreshFrame = qTrue;
	qBool		miscFrame = qTrue;

	if (dedicated->integer)
		return;

	// internal counters
	packetDelta += msec;
	refreshDelta += msec;
	miscDelta += msec;

	// frame counters
	cls.netFrameTime = packetDelta * 0.001;
	cls.trueNetFrameTime = cls.netFrameTime;
	cls.refreshFrameTime = refreshDelta * 0.001;
	cls.trueRefreshFrameTime = cls.refreshFrameTime;
	cls.realTime = sysTime;

	// if in the debugger last frame, don't timeout
	if (msec > 5000)
		cls.netChan.lastReceived = Sys_Milliseconds ();

	// don't extrapolate too far ahead
	if (cls.netFrameTime > FRAMETIME_MAX)
		cls.netFrameTime = FRAMETIME_MAX;
	if (cls.refreshFrameTime > FRAMETIME_MAX)
		cls.refreshFrameTime = FRAMETIME_MAX;

	if (!cl_timedemo->integer) {
		// don't flood packets out while connecting
		if ((cls.connectState == CA_CONNECTED) && (packetDelta < 100))
			packetFrame = qFalse;

		// packet transmission rate is too high
		if (packetDelta < 1000.0/cl_maxfps->value)
			packetFrame = qFalse;
		// this happens when your video framerate plumits to near-below net 'framerate'
		else if (cls.netFrameTime == cls.refreshFrameTime)
			packetFrame = qFalse;
		// video framerate is too high
		else if (refreshDelta < 1000.0/r_maxfps->value)
			refreshFrame = qFalse;
	
		// don't need to do this stuff much
		if (miscDelta < 250)
			miscFrame = qFalse;
	}

	if (cls.forcePacket || cvarUserInfoModified) {
		packetFrame = qTrue;
		cls.forcePacket = qFalse;
	}

	// update the inputs (keyboard, mouse, server, etc)
	CL_RefreshInputs ();

	// clamp cvar values before changes become visible to server/client
	CL_UpdateCvars ();

	// send commands to the server
	if (packetFrame) {
		packetDelta = 0;
		CL_SendCommand ();
	}

	// render the display
	if (refreshFrame) {
		refreshDelta = 0;

		// stuff that does not need to happen a lot
		if (miscFrame) {
			miscDelta = 0;

			// let the mouse activate or deactivate
			IN_Frame ();

			// queued video restart
			VID_CheckChanges ();

			// queued audio restart
			Snd_CheckChanges ();
		}

		// packet loss catcher
		if (!cls.refreshPrepped && cls.connectState == CA_ACTIVE)
			CL_CGameAPI_Init ();

		// update the screen
		if (host_speeds->value)
			timeBeforeRefresh = Sys_Milliseconds ();
		SCR_UpdateScreen ();
		if (host_speeds->value)
			timeAfterRefresh = Sys_Milliseconds ();

		// update audio orientation
		if ((cls.connectState != CA_ACTIVE) || (cl.cin.time > 0))
			Snd_Update (vec3Origin, vec3Origin, vec3Origin, vec3Origin, qFalse);

		if (miscFrame)
			CDAudio_Update ();

		// advance local effects for next frame
		CIN_RunCinematic ();
	}

	// stat logging
	if (log_stats->value && (cls.connectState == CA_ACTIVE)) {
		static int  lastTimeCalled = 0;

		if (!lastTimeCalled) {
			lastTimeCalled = Sys_Milliseconds ();

			if (logStatsFP)
				fprintf (logStatsFP, "0\n");
		}
		else {
			int now = Sys_Milliseconds ();

			if (logStatsFP)
				fprintf (logStatsFP, "%d\n", now - lastTimeCalled);

			lastTimeCalled = now;
		}
	}
}

/*
=======================================================================

	CONSOLE FUNCTIONS

=======================================================================
*/

/*
=================
CL_Changing_f

Just sent as a hint to the client that they should
drop to full console
=================
*/
static void CL_Changing_f (void) {
	// if we are downloading, we don't change!  This so we don't suddenly stop downloading a map
	if (cls.download.file)
		return;

	cls.connectCount = 0;

	CL_FreeLoc ();
	SCR_BeginLoadingPlaque ();
	CL_SetState (CA_CONNECTED);	// not active anymore, but not disconnected
	Com_Printf (0, "\nChanging map...\n");
}


/*
================
CL_Connect_f
================
*/
static void CL_Connect_f (void) {
	char	*server;

	if (Cmd_Argc () != 2) {
		Com_Printf (0, "usage: connect <server>\n");
		return;	
	}
	
	if (Com_ServerState ()) {
		// if running a local server, kill it and reissue
		SV_Shutdown ("Server quit\n", qFalse, qFalse);
	}
	else {
		CL_Disconnect ();
	}

	server = Cmd_Argv (1);

	NET_Config (NET_CLIENT);

	CL_Disconnect ();

	CL_SetState (CA_CONNECTING);
	Q_strncpyz (cls.serverName, server, sizeof (cls.serverName));
	Q_strncpyz (cls.serverNameLast, cls.serverName, sizeof (cls.serverNameLast));

	cls.connectTime = -99999;	// CL_CheckForResend () will fire immediately
	cls.connectCount = 0;
}


/*
=================
CL_Disconnect_f
=================
*/
static void CL_Disconnect_f (void) {
	Com_Error (ERR_DROP, "Disconnected from server");
}


/*
==================
CL_ForwardToServer_f
==================
*/
static void CL_ForwardToServer_f (void) {
	if ((cls.connectState != CA_CONNECTED) && (cls.connectState != CA_ACTIVE)) {
		Com_Printf (0, "Can't \"%s\", not connected\n", Cmd_Argv (0));
		return;
	}
	
	// don't forward the first argument
	if (Cmd_Argc () > 1) {
		MSG_WriteByte (&cls.netChan.message, CLC_STRINGCMD);
		MSG_Print (&cls.netChan.message, Cmd_Args ());
		cls.forcePacket = qTrue;
	}
}


/*
==================
CL_Pause_f
==================
*/
static void CL_Pause_f (void) {
	// never pause in multiplayer
	if ((Cvar_VariableValue ("maxclients") > 1) || !Com_ServerState ()) {
		Cvar_SetValue ("paused", 0);
		return;
	}

	Cvar_SetValue ("paused", !cl_paused->integer);
}


/*
=================
CL_PingLocal_f
=================
*/
static void CL_PingLocal_f (void) {
	netAdr_t	adr;

	NET_Config (NET_CLIENT);

	// send a broadcast packet
	Com_Printf (0, "info pinging broadcast...\n");

	adr.naType = NA_BROADCAST;
	adr.port = BigShort(PORT_SERVER);
	Netchan_OutOfBandPrint (NS_CLIENT, adr, Q_VarArgs ("info %i", ORIGINAL_PROTOCOL_VERSION));
}


/*
=================
CL_PingServer_f
=================
*/
static void CL_PingServer_f (void) {
	netAdr_t	adr;
	char		*adrString;

	NET_Config (NET_CLIENT);

	// send a packet to each address book entry
	adrString = Cmd_Argv (1);
	if (!adrString || !adrString[0])
		return;

	Com_Printf (0, "pinging %s...\n", adrString);

	if (!NET_StringToAdr (adrString, &adr)) {
		Com_Printf (0, S_COLOR_YELLOW "Bad address: %s\n", adrString);
		return;
	}

	if (!adr.port)
		adr.port = BigShort (PORT_SERVER);

	Netchan_OutOfBandPrint (NS_CLIENT, adr, Q_VarArgs ("info %i", ORIGINAL_PROTOCOL_VERSION));
}


/*
=================
CL_Precache_f

The server will send this command right
before allowing the client into the server
=================
*/
static void CL_Precache_f (void) {
	// Yet another hack to let old demos work the old precache sequence
	if (Cmd_Argc () < 2) {
		uInt	mapCheckSum;		// for detecting cheater maps

		CM_LoadMap (cl.configStrings[CS_MODELS+1], qTrue, &mapCheckSum);
		CL_LoadLoc (cl.configStrings[CS_MODELS+1]);
		CL_CGameAPI_Init ();
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
static void CL_Quit_f (void) {
	CL_Disconnect ();
	Com_Quit ();
}


/*
=====================
CL_Rcon_f

Send the rest of the command line over as an unconnected command.
=====================
*/
static void CL_Rcon_f (void) {
	char		message[1024];
	int			i;
	netAdr_t	to;

	if (!rcon_clpassword->string) {
		Com_Printf (0, "You must set 'rcon_password' before\n"
								"issuing an rcon command.\n");
		return;
	}

	// R1: check buffer size
	if ((Q_strlen (Cmd_Args ()) + Q_strlen (rcon_clpassword->string) + 16) >= sizeof (message)) {
		Com_Printf (0, "Length of password + command exceeds maximum allowed length.\n");
		return;
	}

	message[0] = (char)255;
	message[1] = (char)255;
	message[2] = (char)255;
	message[3] = (char)255;
	message[4] = 0;

	NET_Config (NET_CLIENT);

	Q_strcat (message, "rcon ", sizeof (message));

	Q_strcat (message, rcon_clpassword->string, sizeof (message));
	Q_strcat (message, " ", sizeof (message));

	for (i=1 ; i<Cmd_Argc () ; i++) {
		Q_strcat (message, Cmd_Argv (i), sizeof (message));
		Q_strcat (message, " ", sizeof (message));
	}

	if (cls.connectState >= CA_CONNECTED)
		to = cls.netChan.remoteAddress;
	else {
		if (!Q_strlen (rcon_address->string)) {
			Com_Printf (0,	"You must either be connected,\n"
									"or set the 'rcon_address' cvar\n"
									"to issue rcon commands\n");

			return;
		}

		NET_StringToAdr (rcon_address->string, &to);
		NET_StringToAdr (rcon_address->string, &rconLastTo);

		if (to.port == 0)
			to.port = BigShort (PORT_SERVER);
	}
	
	NET_SendPacket (NS_CLIENT, Q_strlen (message) + 1, message, to);
}


/*
=================
CL_Reconnect_f

The server is changing levels
=================
*/
static void CL_Reconnect_f (void) {
	// if we are downloading, we don't change! This so we don't suddenly stop downloading a map
	if (cls.download.file)
		return;

	cls.connectCount = 0;

	Snd_StopAllSounds ();
	if (cls.connectState == CA_CONNECTED) {
		Com_Printf (0, "reconnecting (soft)...\n");
		CL_SetState (CA_CONNECTED);
		MSG_WriteChar (&cls.netChan.message, CLC_STRINGCMD);
		MSG_WriteString (&cls.netChan.message, "new");	
		cls.forcePacket = qTrue;
		return;
	}

	if (*cls.serverName) {
		if (cls.connectState >= CA_CONNECTED) {
			Com_Printf (0, "Disconnecting...\n");
			Q_strncpyz (cls.serverName, cls.serverNameLast, sizeof (cls.serverName));
			CL_Disconnect ();
			cls.connectTime = cls.realTime - (NET_RETRYDELAY*0.5);
		}
		else {
			cls.connectTime = -99999; // CL_CheckForResend () will fire immediately
		}

		CL_SetState (CA_CONNECTING);
	}

	if (*cls.serverNameLast) {
		CL_SetState (CA_CONNECTING);
		cls.connectTime = -99999; // CL_CheckForResend () will fire immediately
		Com_Printf (0, "reconnecting (hard)...\n");
		Q_strncpyz (cls.serverName, cls.serverNameLast, sizeof (cls.serverName));
	}
	else {
		Com_Printf (0, "No server to reconnect to...\n");
	}
}


/*
===================
CL_Setenv_f
===================
*/
static void CL_Setenv_f (void) {
	int argc = Cmd_Argc ();

	if (argc > 2) {
		char buffer[1000];
		int i;

		Q_strncpyz (buffer, Cmd_Argv (1), sizeof (buffer)-1);
		Q_strcat (buffer, "=", sizeof (buffer));

		for (i=2 ; i<argc ; i++) {
			Q_strcat (buffer, Cmd_Argv (i), sizeof (buffer));
			Q_strcat (buffer, " ", sizeof (buffer));
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
CL_StatusLocal_f
===================
*/
static void CL_StatusLocal_f (void) {
	char		message[16];
	netAdr_t	adr;

	memset (&message, 0, sizeof (message));
	message[0] = (char)255;
	message[1] = (char)255;
	message[2] = (char)255;
	message[3] = (char)255;
	message[4] = (char)0;
	Q_strcat (message, "status", sizeof (message));

	NET_Config (NET_CLIENT);

	// send a broadcast packet
	Com_Printf (0, "status pinging broadcast...\n");

	adr.naType = NA_BROADCAST;
	adr.port = BigShort (PORT_SERVER);
	Netchan_OutOfBand (NS_CLIENT, adr, Q_strlen (message) + 1, message);
}


/*
===================
CL_ServerStatus_f
===================
*/
static void CL_ServerStatus_f (void) {
	char		message[16];
	netAdr_t	adr;

	memset (&message, 0, sizeof (message));
	message[0] = (char)255;
	message[1] = (char)255;
	message[2] = (char)255;
	message[3] = (char)255;
	message[4] = (char)0;
	Q_strcat (message, "status", sizeof (message));

	NET_Config (NET_CLIENT);

	NET_StringToAdr (Cmd_Argv (1), &adr);

	if (adr.port == 0)
		adr.port = BigShort (PORT_SERVER);

	NET_SendPacket (NS_CLIENT, Q_strlen (message) + 1, message, adr);
}


/*
==============
CL_Userinfo_f
==============
*/
static void CL_Userinfo_f (void) {
	Com_Printf (0, "User info settings:\n");
	Info_Print (Cvar_BitInfo (CVAR_USERINFO));
}

/*
=======================================================================

	INIT / SHUTDOWN

=======================================================================
*/

/*
==================
CL_WriteConfig
==================
*/
void CL_WriteConfig (void) {
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
		Com_Printf (0, S_COLOR_RED "ERROR: Couldn't write %s\n", path);
		return;
	}

	fprintf (f, "//\n// configuration file generated by EGL, modify at risk\n//\n");

	fprintf (f, "\n// key bindings\n");
	Key_WriteBindings (f);

	fprintf (f, "\n// console variables\n");
	Cvar_WriteVariables (f);

	fprintf (f, "\n// alias commands\n");
	Alias_WriteAliases (f);

	fprintf (f, "\n\0");
	fclose (f);
	Com_Printf (0, "Saved to %s\n", path);
}


/*
=================
CL_Register
=================
*/
void CL_Register (void) {
	// register our variables
	allow_download			= Cvar_Get ("allow_download",			"1",	CVAR_ARCHIVE);
	allow_download_players	= Cvar_Get ("allow_download_players",	"0",	CVAR_ARCHIVE);
	allow_download_models	= Cvar_Get ("allow_download_models",	"1",	CVAR_ARCHIVE);
	allow_download_sounds	= Cvar_Get ("allow_download_sounds",	"1",	CVAR_ARCHIVE);
	allow_download_maps		= Cvar_Get ("allow_download_maps",		"1",	CVAR_ARCHIVE);

	autosensitivity			= Cvar_Get ("autosensitivity",		"0",		CVAR_ARCHIVE);

	cl_lightlevel			= Cvar_Get ("r_lightlevel",			"0",		0);
	cl_maxfps				= Cvar_Get ("cl_maxfps",			"30",		0);
	r_maxfps				= Cvar_Get ("r_maxfps",				"500",		0);
	cl_paused				= Cvar_Get ("paused",				"0",		0);
	cl_protocol				= Cvar_Get ("cl_protocol",			"0",		0);

	cl_shownet				= Cvar_Get ("cl_shownet",			"0",		0);

	cl_stereo_separation	= Cvar_Get ("cl_stereo_separation",	"0.4",	CVAR_ARCHIVE);
	cl_stereo				= Cvar_Get ("cl_stereo",			"0",	0);

	cl_timedemo				= Cvar_Get ("timedemo",				"0",		0);
	cl_timeout				= Cvar_Get ("cl_timeout",			"120",		0);
	cl_timestamp			= Cvar_Get ("cl_timestamp",			"0",		CVAR_ARCHIVE);

	con_alpha				= Cvar_Get ("con_alpha",			"0.6",		CVAR_ARCHIVE);
	con_clock				= Cvar_Get ("con_clock",			"1",		CVAR_ARCHIVE);
	con_drop				= Cvar_Get ("con_drop",				"0.5",		CVAR_ARCHIVE);
	con_scroll				= Cvar_Get ("con_scroll",			"2",		CVAR_ARCHIVE);

	freelook				= Cvar_Get ("freelook",				"0",		CVAR_ARCHIVE);

	lookspring				= Cvar_Get ("lookspring",			"0",		CVAR_ARCHIVE);
	lookstrafe				= Cvar_Get ("lookstrafe",			"0",		CVAR_ARCHIVE);

	m_forward				= Cvar_Get ("m_forward",			"1",		0);
	m_pitch					= Cvar_Get ("m_pitch",				"0.022",	CVAR_ARCHIVE);
	m_side					= Cvar_Get ("m_side",				"1",		0);
	m_yaw					= Cvar_Get ("m_yaw",				"0.022",	0);

	rcon_clpassword			= Cvar_Get ("rcon_password",		"",			0);
	rcon_address			= Cvar_Get ("rcon_address",			"",			0);

	r_fontscale				= Cvar_Get ("r_fontscale",			"1",		CVAR_ARCHIVE);

	sensitivity				= Cvar_Get ("sensitivity",			"3",		CVAR_ARCHIVE);

	// userinfo cvars
	Cvar_Get ("fov",			"90",			CVAR_USERINFO|CVAR_ARCHIVE);
	Cvar_Get ("password",		"",				CVAR_USERINFO);
	Cvar_Get ("spectator",		"0",			CVAR_USERINFO);
	Cvar_Get ("msg",			"0",			CVAR_USERINFO|CVAR_ARCHIVE);
	Cvar_Get ("name",			"unnamed",		CVAR_USERINFO|CVAR_ARCHIVE);
	Cvar_Get ("rate",			"8000",			CVAR_USERINFO|CVAR_ARCHIVE);
	Cvar_Get ("gender",			"male",			CVAR_USERINFO|CVAR_ARCHIVE);
	Cvar_Get ("hand",			"0",			CVAR_USERINFO|CVAR_ARCHIVE);
	Cvar_Get ("skin",			"",				CVAR_USERINFO|CVAR_ARCHIVE);

	scr_conspeed		= Cvar_Get ("scr_conspeed",		"3",		0);
	scr_printspeed		= Cvar_Get ("scr_printspeed",	"8",		0);
	scr_netgraph		= Cvar_Get ("netgraph",			"0",		0);
	scr_timegraph		= Cvar_Get ("timegraph",		"0",		0);
	scr_debuggraph		= Cvar_Get ("debuggraph",		"0",		0);
	scr_graphheight		= Cvar_Get ("graphheight",		"32",		0);
	scr_graphscale		= Cvar_Get ("graphscale",		"1",		0);
	scr_graphshift		= Cvar_Get ("graphshift",		"0",		0);

	scr_graphalpha		= Cvar_Get ("scr_graphalpha",	"0.6",		CVAR_ARCHIVE);

	// register our commands
	Cmd_AddCommand ("changing",		CL_Changing_f,			"");
	Cmd_AddCommand ("connect",		CL_Connect_f,			"Connects to a server");
	Cmd_AddCommand ("cmd",			CL_ForwardToServer_f,	"Forwards a command to the server");
	Cmd_AddCommand ("disconnect",	CL_Disconnect_f,		"Disconnects from the current server");
	Cmd_AddCommand ("download",		CL_Download_f,			"Manually download a file from the server");
	Cmd_AddCommand ("exit",			CL_Quit_f,				"Exits");
	Cmd_AddCommand ("pause",		CL_Pause_f,				"Pauses the game");
	Cmd_AddCommand ("pingserver",	CL_PingServer_f,		"Sends an info request packet to a server");
	Cmd_AddCommand ("pinglocal",	CL_PingLocal_f,			"Queries broadcast for an info string");
	Cmd_AddCommand ("precache",		CL_Precache_f,			"");
	Cmd_AddCommand ("quit",			CL_Quit_f,				"Exits");
	Cmd_AddCommand ("rcon",			CL_Rcon_f,				"");
	Cmd_AddCommand ("reconnect",	CL_Reconnect_f,			"Reconnect to the current server");
	Cmd_AddCommand ("record",		CL_Record_f,			"Start recording a demo");
	Cmd_AddCommand ("savecfg",		CL_WriteConfig,			"Manually save configuration");
	Cmd_AddCommand ("say",			CL_Say_Preprocessor,	"");
	Cmd_AddCommand ("say_team",		CL_Say_Preprocessor,	"");
	Cmd_AddCommand ("setenv",		CL_Setenv_f,			"");
	Cmd_AddCommand ("statuslocal",	CL_StatusLocal_f,		"Queries broadcast for a status string");
	Cmd_AddCommand ("sstatus",		CL_ServerStatus_f,		"Sends a status request packet to a server");
	Cmd_AddCommand ("stop",			CL_Stop_f,				"Stop recording a demo");
	Cmd_AddCommand ("userinfo",		CL_Userinfo_f,			"");

	/*
	** Forward to server commands...
	** The only thing this does is allow command completion to work -- all unknown
	** commands are automatically forwarded to the server
	*/
	Cmd_AddCommand ("wave",			NULL,		"");
	Cmd_AddCommand ("inven",		NULL,		"");
	Cmd_AddCommand ("kill",			NULL,		"");
	Cmd_AddCommand ("use",			NULL,		"");
	Cmd_AddCommand ("drop",			NULL,		"");
	Cmd_AddCommand ("info",			NULL,		"");
	Cmd_AddCommand ("prog",			NULL,		"");
	Cmd_AddCommand ("give",			NULL,		"");
	Cmd_AddCommand ("god",			NULL,		"");
	Cmd_AddCommand ("notarget",		NULL,		"");
	Cmd_AddCommand ("noclip",		NULL,		"");
	Cmd_AddCommand ("invuse",		NULL,		"");
	Cmd_AddCommand ("invprev",		NULL,		"");
	Cmd_AddCommand ("invnext",		NULL,		"");
	Cmd_AddCommand ("invdrop",		NULL,		"");
	Cmd_AddCommand ("weapnext",		NULL,		"");
	Cmd_AddCommand ("weapprev",		NULL,		"");
}


/*
====================
CL_Init
====================
*/
void CL_Init (void) {
	if (dedicated->integer)
		return;	// nothing running on the client

	CL_SetState (CA_DISCONNECTED);

	cls.realTime = Sys_Milliseconds ();

	cls.disableScreen = qTrue;
	cls.soundPrepped = qFalse;

	CL_Register ();

	// all archived variables will now be loaded
#ifdef VID_INITFIRST
	VID_Init ();
#else
	Snd_Init ();
	VID_Init ();
#endif	
	netMessage.data = netMessageBuffer;
	netMessage.maxSize = sizeof (netMessageBuffer);

	CDAudio_Init ();
	CL_InitInput ();
	IN_Init ();
	SCR_Init ();

	CL_InitMedia ();

	// initialize location scripting
	CL_Loc_Init ();

	// check cvar values
	CL_UpdateCvars ();

	// check memory integrity
	Mem_CheckGlobalIntegrity ();

	cls.disableScreen = qFalse;
	cls.soundPrepped = qTrue;
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
	if (isDown)
		return;
	isDown = qTrue;

	CL_WriteConfig ();

	CL_UIAPI_Shutdown (qTrue);
	CL_CGameAPI_Shutdown (qTrue);

	CDAudio_Shutdown (qTrue);
	Snd_Shutdown (qTrue);

	CL_Loc_Shutdown (qTrue);

	IN_Shutdown (qTrue);
	VID_Shutdown (qTrue);
}
