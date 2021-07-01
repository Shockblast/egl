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

cVar_t	*adr0;
cVar_t	*adr1;
cVar_t	*adr2;
cVar_t	*adr3;
cVar_t	*adr4;
cVar_t	*adr5;
cVar_t	*adr6;
cVar_t	*adr7;

cVar_t	*autosensitivity;

cVar_t	*cl_lightlevel;
cVar_t	*cl_maxfps;
cVar_t	*cl_paused;

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

cVar_t	*scr_demomsg;
cVar_t	*scr_graphalpha;

clientState_t	cl;
clientStatic_t	cls;
clMedia_t		clMedia;

entityState_t	clBaseLines[MAX_CS_EDICTS];
entityState_t	clParseEntities[MAX_PARSE_ENTITIES];

netAdr_t	rconLastTo;

extern	cVar_t *allow_download;
extern	cVar_t *allow_download_players;
extern	cVar_t *allow_download_models;
extern	cVar_t *allow_download_sounds;
extern	cVar_t *allow_download_maps;

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
		Com_Printf (0, "Unknown command \"%s" S_STYLE_RETURN "\"\n", cmd);
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
	if ((cls.connState == CA_UNINITIALIZED) || (cls.connState == CA_DISCONNECTED))
		return;

	CL_Disconnect ();

	// drop loading plaque unless this is the initial game start
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
		Com_Printf (0, S_COLOR_YELLOW "Bad server address\n");
		cls.connectTime = 0;
		return;
	}

	if (adr.port == 0)
		adr.port = BigShort (PORT_SERVER);

	port = Cvar_VariableInteger ("qport");
	cvarUserInfoModified = qFalse;

	Netchan_OutOfBandPrint (NS_CLIENT, adr, "connect %i %i %i \"%s\"\n", PROTOCOL_VERSION, port, cls.challenge, Cvar_Userinfo());
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
	cls.connState = state;
	Com_SetClientState (state);
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

	if (cls.connState == CA_DISCONNECTED)
		return;

	if (cl_timedemo && cl_timedemo->integer) {
		int	time;
		
		time = Sys_Milliseconds () - cl.timeDemoStart;
		if (time > 0)
			Com_Printf (0, "%i frames, %3.1f seconds: %3.1f fps\n", cl.timeDemoFrames,
				time/1000.0, cl.timeDemoFrames*1000.0 / time);
	}

	R_SetPalette (NULL);

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

	CM_UnloadMap ();
	CL_RestartMedia ();
	CL_FreeLoc ();

	// stop download
	if (cls.downloadFile) {
		fclose (cls.downloadFile);
		cls.downloadFile = NULL;
	}

	CL_ClearState ();
	CL_SetState (CA_DISCONNECTED);
}


/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
static void CL_ConnectionlessPacket (void) {
	char		*s, *c;
	netAdr_t	remote;

	NET_StringToAdr (cls.serverName, &remote);
	MSG_BeginReading (&netMessage);
	MSG_ReadLong (&netMessage);	// skip the -1

	s = MSG_ReadStringLine (&netMessage);

	Cmd_TokenizeString (s, qFalse);

	c = Cmd_Argv (0);

	Com_Printf (0, "%s: %s\n", NET_AdrToString (netFrom), c);

	// server connection
	if (!strcmp (c, "client_connect")) {
		if (cls.connState == CA_CONNECTED) {
			Com_Printf (0, S_COLOR_YELLOW "Dup connect received. Ignored.\n");
			return;
		}

		Netchan_Setup (NS_CLIENT, &cls.netChan, netFrom, cls.quakePort);
		MSG_WriteChar (&cls.netChan.message, CLC_STRINGCMD);
		MSG_WriteString (&cls.netChan.message, "new");	
		CL_SetState (CA_CONNECTED);
		return;
	}

	// server responding to a status broadcast
	if (!strcmp (c, "info")) {
		CL_UIModule_AddToServerList (NET_AdrToString (netFrom), MSG_ReadString (&netMessage));
		return;
	}

	/*
	** R1CH: security check. (only allow from current connected server and last
	** destination client sent an rcon to)
	*/
	if (!NET_CompareBaseAdr (netFrom, remote) && !NET_CompareBaseAdr (netFrom, rconLastTo)) {
		Com_Printf (0, S_COLOR_YELLOW "Illegal %s from %s. Ignored.\n", c, NET_AdrToString (netFrom));
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
	// print command from somewhere
	if (!strcmp (c, "print")) {
		s = MSG_ReadString (&netMessage);
		Com_Printf (0, "%s", s);
		return;
	}

	// ping from somewhere
	if (!strcmp (c, "ping")) {
		Netchan_OutOfBandPrint (NS_CLIENT, netFrom, "ack");
		return;
	}

	// challenge from the server we are connecting to
	if (!strcmp (c, "challenge")) {
		cls.challenge = atoi(Cmd_Argv (1));
		CL_SendConnectPacket ();
		return;
	}

	// echo request from server
	if (!strcmp (c, "echo")) {
		Netchan_OutOfBandPrint (NS_CLIENT, netFrom, "%s", Cmd_Argv (1));
		return;
	}

	Com_Printf (0, "Unknown command.\n");
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
	clMedia.consoleImage	= Img_RegisterPic ("conback");
	clMedia.hudLoadingImage	= Img_RegisterPic ("loading");
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
	if (cls.connState == CA_UNINITIALIZED)
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
	CL_CGameAPI_Shutdown ();

	// shutdown ui
	CL_UIAPI_Shutdown ();

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

/*
==============
CL_RequestNextDownload
==============
*/
static int	clPrecacheCheck; // for autodownload of precache items
static int	clPrecacheSpawnCount;
static int	clPrecacheTexNum;

static byte	*clPrecacheModel; // used for skin checking in alias models
static int	clPrecacheModelSkin;

#define PLAYER_MULT 5

// ENV_CNT is map load, ENV_CNT+1 is first env map
#define ENV_CNT (CS_PLAYERSKINS + MAX_CS_CLIENTS * PLAYER_MULT)
#define TEXTURE_CNT (ENV_CNT+13)

extern char *sky_NameSuffix[6];
void CL_RequestNextDownload (void) {
	uInt			mapCheckSum;		// for detecting cheater maps
	char			fileName[MAX_OSPATH];
	dMd2Header_t	*md2Header;

	if (cls.connState != CA_CONNECTED)
		return;

	//
	// if downloading is off by option, skip to loading the map
	//
	if (!allow_download->value && (clPrecacheCheck < ENV_CNT))
		clPrecacheCheck = ENV_CNT;

	if (clPrecacheCheck == CS_MODELS) {
		// confirm map
		clPrecacheCheck = CS_MODELS+2; // 0 isn't used
		if (allow_download_maps->value)
			if (!CL_CheckOrDownloadFile (cl.configStrings[CS_MODELS+1]))
				return; // started a download
	}

	//
	// models
	//
	if ((clPrecacheCheck >= CS_MODELS) && (clPrecacheCheck < CS_MODELS+MAX_CS_MODELS)) {
		if (allow_download_models->value) {
			while ((clPrecacheCheck < CS_MODELS+MAX_CS_MODELS) && cl.configStrings[clPrecacheCheck][0]) {
				if ((cl.configStrings[clPrecacheCheck][0] == '*') || (cl.configStrings[clPrecacheCheck][0] == '#')) {
					clPrecacheCheck++;
					continue;
				}
				if (clPrecacheModelSkin == 0) {
					if (!CL_CheckOrDownloadFile (cl.configStrings[clPrecacheCheck])) {
						clPrecacheModelSkin = 1;
						return; // started a download
					}
					clPrecacheModelSkin = 1;
				}

				// checking for skins in the model
				if (!clPrecacheModel) {
					FS_LoadFile (cl.configStrings[clPrecacheCheck], (void **)&clPrecacheModel);
					if (!clPrecacheModel) {
						clPrecacheModelSkin = 0;
						clPrecacheCheck++;
						continue; // couldn't load it
					}
					if (LittleLong (*(uInt *)clPrecacheModel) != MD2_HEADER) {
						// not an alias model
						FS_FreeFile (clPrecacheModel);
						clPrecacheModel = 0;
						clPrecacheModelSkin = 0;
						clPrecacheCheck++;
						continue;
					}
					md2Header = (dMd2Header_t *)clPrecacheModel;
					if (LittleLong (md2Header->version) != MD2_ALIAS_VERSION) {
						clPrecacheCheck++;
						clPrecacheModelSkin = 0;
						continue; // couldn't load it
					}
				}

				md2Header = (dMd2Header_t *)clPrecacheModel;
				while (clPrecacheModelSkin - 1 < LittleLong(md2Header->numSkins)) {
					if (!CL_CheckOrDownloadFile((char *)clPrecacheModel +
						LittleLong (md2Header->ofsSkins) + 
						(clPrecacheModelSkin - 1)*MD2_MAX_SKINNAME)) {
						clPrecacheModelSkin++;
						return; // started a download
					}
					clPrecacheModelSkin++;
				}

				if (clPrecacheModel) { 
					FS_FreeFile (clPrecacheModel);
					clPrecacheModel = 0;
				}

				clPrecacheModelSkin = 0;
				clPrecacheCheck++;
			}
		}
		clPrecacheCheck = CS_SOUNDS;
	}

	//
	// sound
	//
	if ((clPrecacheCheck >= CS_SOUNDS) && (clPrecacheCheck < CS_SOUNDS+MAX_CS_SOUNDS)) { 
		if (allow_download_sounds->value) {
			if (clPrecacheCheck == CS_SOUNDS)
				clPrecacheCheck++; // zero is blank

			while ((clPrecacheCheck < CS_SOUNDS+MAX_CS_SOUNDS) && (cl.configStrings[clPrecacheCheck][0])) {
				if (cl.configStrings[clPrecacheCheck][0] == '*') {
					clPrecacheCheck++;
					continue;
				}

				Q_snprintfz (fileName, sizeof (fileName), "sound/%s", cl.configStrings[clPrecacheCheck++]);
				if (!CL_CheckOrDownloadFile (fileName))
					return; // started a download
			}
		}
		clPrecacheCheck = CS_IMAGES;
	}

	//
	// images
	//
	if ((clPrecacheCheck >= CS_IMAGES) && (clPrecacheCheck < CS_IMAGES+MAX_CS_IMAGES)) {
		if (clPrecacheCheck == CS_IMAGES)
			clPrecacheCheck++; // zero is blank

		while ((clPrecacheCheck < CS_IMAGES+MAX_CS_IMAGES) && (cl.configStrings[clPrecacheCheck][0])) {
			Q_snprintfz (fileName, sizeof (fileName), "pics/%s.pcx", cl.configStrings[clPrecacheCheck++]);
			if (!CL_CheckOrDownloadFile (fileName))
				return; // started a download
		}
		clPrecacheCheck = CS_PLAYERSKINS;
	}

	//
	// skins are special, since a player has three things to download:
	// - model
	// - weapon model
	// - skin
	// so clPrecacheCheck is now * 3
	//
	if ((clPrecacheCheck >= CS_PLAYERSKINS) && (clPrecacheCheck < (CS_PLAYERSKINS + MAX_CS_CLIENTS * PLAYER_MULT))) {
		if (allow_download_players->value) {
			while (clPrecacheCheck < (CS_PLAYERSKINS + MAX_CS_CLIENTS * PLAYER_MULT)) {
				int		i, n;
				char	model[MAX_QPATH];
				char	skin[MAX_QPATH];
				char	*p;

				i = (clPrecacheCheck - CS_PLAYERSKINS)/PLAYER_MULT;
				n = (clPrecacheCheck - CS_PLAYERSKINS)%PLAYER_MULT;

				if (!cl.configStrings[CS_PLAYERSKINS+i][0]) {
					clPrecacheCheck = CS_PLAYERSKINS + (i + 1) * PLAYER_MULT;
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
					Q_snprintfz (fileName, sizeof (fileName), "players/%s/tris.md2", model);
					if (!CL_CheckOrDownloadFile (fileName)) {
						clPrecacheCheck = CS_PLAYERSKINS + i * PLAYER_MULT + 1;
						return; // started a download
					}
					n++;
					// FALL THROUGH

				case 1:
					// weapon model
					Q_snprintfz (fileName, sizeof (fileName), "players/%s/weapon.md2", model);
					if (!CL_CheckOrDownloadFile (fileName)) {
						clPrecacheCheck = CS_PLAYERSKINS + i * PLAYER_MULT + 2;
						return; // started a download
					}
					n++;
					// FALL THROUGH

				case 2:
					// weapon skin
					Q_snprintfz (fileName, sizeof (fileName), "players/%s/weapon.pcx", model);
					if (!CL_CheckOrDownloadFile (fileName)) {
						clPrecacheCheck = CS_PLAYERSKINS + i * PLAYER_MULT + 3;
						return; // started a download
					}
					n++;
					// FALL THROUGH

				case 3:
					// skin
					Q_snprintfz (fileName, sizeof (fileName), "players/%s/%s.pcx", model, skin);
					if (!CL_CheckOrDownloadFile (fileName)) {
						clPrecacheCheck = CS_PLAYERSKINS + i * PLAYER_MULT + 4;
						return; // started a download
					}
					n++;
					// FALL THROUGH

				case 4:
					// skin_i
					Q_snprintfz (fileName, sizeof (fileName), "players/%s/%s_i.pcx", model, skin);
					if (!CL_CheckOrDownloadFile (fileName)) {
						clPrecacheCheck = CS_PLAYERSKINS + i * PLAYER_MULT + 5;
						return; // started a download
					}

					// move on to next model
					clPrecacheCheck = CS_PLAYERSKINS + (i + 1) * PLAYER_MULT;
				}
			}
		}
		// precache phase completed
		clPrecacheCheck = ENV_CNT;
	}

	//
	// map
	//
	if (clPrecacheCheck == ENV_CNT) {
		clPrecacheCheck = ENV_CNT + 1;

		CM_LoadMap (cl.configStrings[CS_MODELS+1], qTrue, &mapCheckSum);

		if (mapCheckSum != atoi(cl.configStrings[CS_MAPCHECKSUM])) {
			Com_Error (ERR_DROP, "Local map version differs from server: %i != '%s'\n",
				mapCheckSum, cl.configStrings[CS_MAPCHECKSUM]);
			return;
		}

		CL_LoadLoc (cl.configStrings[CS_MODELS+1]);
	}

	//
	// sky box env
	//
	if ((clPrecacheCheck > ENV_CNT) && (clPrecacheCheck < TEXTURE_CNT)) {
		if (allow_download->value && allow_download_maps->value) {
			while (clPrecacheCheck < TEXTURE_CNT) {
				int n = clPrecacheCheck++ - ENV_CNT - 1;

				if (n & 1)
					Q_snprintfz (fileName, sizeof (fileName), "env/%s%s.pcx", cl.configStrings[CS_SKY], sky_NameSuffix[n/2]);
				else
					Q_snprintfz (fileName, sizeof (fileName), "env/%s%s.tga", cl.configStrings[CS_SKY], sky_NameSuffix[n/2]);

				if (!CL_CheckOrDownloadFile (fileName))
					return; // started a download
			}
		}
		clPrecacheCheck = TEXTURE_CNT;
	}

	if (clPrecacheCheck == TEXTURE_CNT) {
		clPrecacheCheck = TEXTURE_CNT+1;
		clPrecacheTexNum = 0;
	}

	//
	// confirm existance of textures, download any that don't exist
	//
	if (clPrecacheCheck == TEXTURE_CNT+1) {
		int			numTexInfo;

		numTexInfo = CM_NumTexInfo ();

		if (allow_download->value && allow_download_maps->value) {
			while (clPrecacheTexNum < numTexInfo) {

				Q_snprintfz (fileName, sizeof (fileName), "textures/%s.wal", CM_SurfRName (clPrecacheTexNum++));
				if (!CL_CheckOrDownloadFile (fileName))
					return; // started a download
			}
		}
		clPrecacheCheck = TEXTURE_CNT+999;
	}

	//ZOID
	CL_CGameAPI_Init ();

	MSG_WriteByte (&cls.netChan.message, CLC_STRINGCMD);
	MSG_WriteString (&cls.netChan.message, Q_VarArgs ("begin %i\n", clPrecacheSpawnCount));
}

/*
=======================================================================

	FRAME HANDLING

=======================================================================
*/

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
static void CL_CheckForResend (void) {
	netAdr_t	adr;

	// if the local server is running and we aren't then connect
	if ((cls.connState == CA_DISCONNECTED) && Com_ServerState ()) {
		CL_SetState (CA_CONNECTING);
		strncpy (cls.serverName, "localhost", sizeof (cls.serverName)-1);
		// we don't need a challenge on the localhost
		CL_SendConnectPacket ();
		return;
	}

	// resend if we haven't gotten a reply yet
	if (cls.connState != CA_CONNECTING)
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

		if ((cls.connState == CA_DISCONNECTED) || (cls.connState == CA_CONNECTING))
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
	if ((cls.connState >= CA_CONNECTED) && (cls.realTime - cls.netChan.lastReceived) > (cl_timeout->value*1000)) {
		if (++cl.timeOutCount > 5) {
			// timeoutcount saves debugger
			Com_Printf (0, "\n" S_COLOR_YELLOW "Server connection timed out.\n");
			CL_Disconnect ();
			return;
		}
	} else
		cl.timeOutCount = 0;
}


/*
==================
CL_SendCommand
==================
*/
static void CL_SendCommand (void) {
	// get new key events
	Sys_SendKeyEvents ();

	// allow mice or other external controllers to add commands
	IN_Commands ();

	// process console commands
	Cbuf_Execute ();

	// send intentions now
	CL_SendCmd ();

	// resend a connection request if necessary
	CL_CheckForResend ();
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
CL_Frame
==================
*/
#define FRAMETIME_MAX 0.2
void FASTCALL CL_Frame (int msec) {
	static int	extraTime = 0;

	if (dedicated->integer)
		return;

	extraTime += msec;

	if (!cl_timedemo->integer) {
		if ((cls.connState == CA_CONNECTED) && (extraTime < 100))
			return;			// don't flood packets out while connecting
		if (extraTime < (1000.0/cl_maxfps->value))
			return;			// framerate is too high
	}

	CL_UpdateCvars ();

	// decide the simulation time
	cls.frameTime = extraTime * 0.001;
	cls.trueFrameTime = cls.frameTime;
	cl.time += extraTime;
	cls.realTime = sysTime;

	if (cls.frameTime > FRAMETIME_MAX)
		cls.frameTime = FRAMETIME_MAX;

	// reset
	extraTime = 0;

	// if in the debugger last frame, don't timeout
	if (msec > 5000)
		cls.netChan.lastReceived = Sys_Milliseconds ();

	// let the mouse activate or deactivate
	IN_Frame ();

	// fetch results from server
	CL_ReadPackets ();

	// send a new command message to the server
	CL_SendCommand ();

	// queued video restart
	VID_CheckChanges ();

	// queued audio restart
	Snd_CheckChanges ();

	// queued ui restart
	CL_UI_CheckChanges ();

	// update the screen
	if (host_speeds->value)
		timeBeforeRefresh = Sys_Milliseconds ();
	SCR_UpdateScreen ();
	if (host_speeds->value)
		timeAfterRefresh = Sys_Milliseconds ();

	// update audio orientation
	if ((cls.connState != CA_ACTIVE) || (cl.cinematicTime > 0))
		Snd_Update (vec3Origin, vec3Origin, vec3Origin, vec3Origin, qFalse);

	// advance local effects for next frame
	CIN_RunCinematic ();
	Con_RunConsole ();

	cls.frameCount++;

	// stat logging
	if (log_stats->value && (cls.connState == CA_ACTIVE)) {
		static int  lastTimeCalled = 0;

		if (!lastTimeCalled) {
			lastTimeCalled = Sys_Milliseconds ();

			if (logStatsFP)
				fprintf (logStatsFP, "0\n");
		} else {
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
void CL_Changing_f (void) {
	// if we are downloading, we don't change!  This so we don't suddenly stop downloading a map
	if (cls.downloadFile)
		return;

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
void CL_Connect_f (void) {
	char	*server;

	if (Cmd_Argc () != 2) {
		Com_Printf (0, "usage: connect <server>\n");
		return;	
	}
	
	if (Com_ServerState ()) {
		// if running a local server, kill it and reissue
		SV_Shutdown ("Server quit\n", qFalse);
	} else {
		CL_Disconnect ();
	}

	server = Cmd_Argv (1);

	NET_Config (qTrue);	// allow remote

	CL_Disconnect ();

	CL_SetState (CA_CONNECTING);
	strncpy (cls.serverName, server, sizeof (cls.serverName)-1);
	cls.connectTime = -99999;	// CL_CheckForResend () will fire immediately
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
==================
CL_ForwardToServer_f
==================
*/
void CL_ForwardToServer_f (void) {
	if ((cls.connState != CA_CONNECTED) && (cls.connState != CA_ACTIVE)) {
		Com_Printf (0, "Can't \"%s\", not connected\n", Cmd_Argv (0));
		return;
	}
	
	// don't forward the first argument
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
	// never pause in multiplayer
	if ((Cvar_VariableValue ("maxclients") > 1) || !Com_ServerState ()) {
		Cvar_VariableSetValue (cl_paused, 0);
		return;
	}

	Cvar_VariableSetValue (cl_paused, !cl_paused->integer);
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
	char		*adrString;
	cVar_t		*noudp;
	cVar_t		*noipx;

	NET_Config (qTrue);	// allow remote

	// send a broadcast packet
	Com_Printf (0, "pinging broadcast...\n");

	noudp = Cvar_Get ("noudp", "0", CVAR_NOSET);
	if (!noudp->value) {
		adr.naType = NA_BROADCAST;
		adr.port = BigShort(PORT_SERVER);
		Netchan_OutOfBandPrint (NS_CLIENT, adr, Q_VarArgs ("info %i", PROTOCOL_VERSION));
	}

	noipx = Cvar_Get ("noipx", "0", CVAR_NOSET);
	if (!noipx->value) {
		adr.naType = NA_BROADCAST_IPX;
		adr.port = BigShort(PORT_SERVER);
		Netchan_OutOfBandPrint (NS_CLIENT, adr, Q_VarArgs ("info %i", PROTOCOL_VERSION));
	}

	// send a packet to each address book entry
	for (i=0 ; i<8 ; i++) {
		Q_snprintfz (name, sizeof (name), "adr%i", i);
		adrString = Cvar_VariableString (name);

		if (!adrString || !adrString[0])
			continue;

		Com_Printf (0, "pinging %s...\n", adrString);

		if (!NET_StringToAdr (adrString, &adr)) {
			Com_Printf (0, S_COLOR_YELLOW "Bad address: %s\n", adrString);
			continue;
		}
		if (!adr.port)
			adr.port = BigShort (PORT_SERVER);

		Netchan_OutOfBandPrint (NS_CLIENT, adr, Q_VarArgs ("info %i", PROTOCOL_VERSION));
	}
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

	clPrecacheCheck = CS_MODELS;
	clPrecacheSpawnCount = atoi (Cmd_Argv (1));
	clPrecacheModel = 0;
	clPrecacheModelSkin = 0;

	CL_RequestNextDownload ();
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
		Com_Printf (0, "You must set 'rcon_password' before\n"
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
	
	NET_SendPacket (NS_CLIENT, strlen (message) + 1, message, to);
}


/*
=================
CL_Reconnect_f

The server is changing levels
=================
*/
void CL_Reconnect_f (void) {
	// if we are downloading, we don't change! This so we don't suddenly stop downloading a map
	if (cls.downloadFile)
		return;

	Snd_StopAllSounds ();
	if (cls.connState == CA_CONNECTED) {
		Com_Printf (0, "reconnecting...\n");
		CL_SetState (CA_CONNECTED);
		MSG_WriteChar (&cls.netChan.message, CLC_STRINGCMD);
		MSG_WriteString (&cls.netChan.message, "new");		
		return;
	}

	if (*cls.serverName) {
		if (cls.connState >= CA_CONNECTED) {
			CL_Disconnect ();
			cls.connectTime = cls.realTime - (NET_RETRYDELAY*0.5);
		} else
			cls.connectTime = -99999; // CL_CheckForResend () will fire immediately

		CL_SetState (CA_CONNECTING);
		Com_Printf (0, "reconnecting...\n");
	}
}


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
			Com_Printf (0, "\"%s\" is: \"%s\"\n", Cmd_Argv (1), env);
		else
			Com_Printf (0, "\"%s\" is undefined\n", Cmd_Argv (1), env);
	}
}


/*
==============
CL_Userinfo_f
==============
*/
static void CL_Userinfo_f (void) {
	Com_Printf (0, "User info settings:\n");
	Info_Print (Cvar_Userinfo ());
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

	Cvar_GetLatchedVars (CVAR_LATCH_AUDIO|CVAR_LATCH_SERVER|CVAR_LATCH_VIDEO);

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
	adr0				= Cvar_Get ("adr0",				"",			CVAR_ARCHIVE);
	adr1				= Cvar_Get ("adr1",				"",			CVAR_ARCHIVE);
	adr2				= Cvar_Get ("adr2",				"",			CVAR_ARCHIVE);
	adr3				= Cvar_Get ("adr3",				"",			CVAR_ARCHIVE);
	adr4				= Cvar_Get ("adr4",				"",			CVAR_ARCHIVE);
	adr5				= Cvar_Get ("adr5",				"",			CVAR_ARCHIVE);
	adr6				= Cvar_Get ("adr6",				"",			CVAR_ARCHIVE);
	adr7				= Cvar_Get ("adr7",				"",			CVAR_ARCHIVE);

	autosensitivity		= Cvar_Get ("autosensitivity",	"0",		CVAR_ARCHIVE);

	cl_lightlevel		= Cvar_Get ("r_lightlevel",		"0",		0);
	cl_maxfps			= Cvar_Get ("cl_maxfps",		"90",		0);
	cl_paused			= Cvar_Get ("paused",			"0",		0);

	cl_shownet			= Cvar_Get ("cl_shownet",		"0",		0);

	cl_stereo_separation= Cvar_Get ("cl_stereo_separation",	"0.4",	CVAR_ARCHIVE);
	cl_stereo			= Cvar_Get ("cl_stereo",			"0",	0);

	cl_timedemo			= Cvar_Get ("timedemo",			"0",		0);
	cl_timeout			= Cvar_Get ("cl_timeout",		"120",		0);
	cl_timestamp		= Cvar_Get ("cl_timestamp",		"0",		CVAR_ARCHIVE);

	con_alpha			= Cvar_Get ("con_alpha",		"0.6",		CVAR_ARCHIVE);
	con_clock			= Cvar_Get ("con_clock",		"1",		CVAR_ARCHIVE);
	con_drop			= Cvar_Get ("con_drop",			"0.5",		CVAR_ARCHIVE);
	con_scroll			= Cvar_Get ("con_scroll",		"2",		CVAR_ARCHIVE);

	freelook			= Cvar_Get ("freelook",			"0",		CVAR_ARCHIVE);

	lookspring			= Cvar_Get ("lookspring",		"0",		CVAR_ARCHIVE);
	lookstrafe			= Cvar_Get ("lookstrafe",		"0",		CVAR_ARCHIVE);

	m_forward			= Cvar_Get ("m_forward",		"1",		0);
	m_pitch				= Cvar_Get ("m_pitch",			"0.022",	CVAR_ARCHIVE);
	m_side				= Cvar_Get ("m_side",			"1",		0);
	m_yaw				= Cvar_Get ("m_yaw",			"0.022",	0);

	rcon_clpassword		= Cvar_Get ("rcon_password",	"",			0);
	rcon_address		= Cvar_Get ("rcon_address",		"",			0);

	r_fontscale			= Cvar_Get ("r_fontscale",		"1",		CVAR_ARCHIVE);

	sensitivity			= Cvar_Get ("sensitivity",		"3",		CVAR_ARCHIVE);

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

	scr_demomsg			= Cvar_Get ("scr_demomsg",		"1",		CVAR_ARCHIVE);
	scr_graphalpha		= Cvar_Get ("scr_graphalpha",	"0.6",		CVAR_ARCHIVE);

	Cmd_AddCommand ("loading",		SCR_BeginLoadingPlaque,	"Shows the loading plaque");

	// register our commands
	Cmd_AddCommand ("changing",		CL_Changing_f,			"");
	Cmd_AddCommand ("connect",		CL_Connect_f,			"Connects to a server");
	Cmd_AddCommand ("cmd",			CL_ForwardToServer_f,	"Forwards a command to the server");
	Cmd_AddCommand ("disconnect",	CL_Disconnect_f,		"Disconnects from the current server");
	Cmd_AddCommand ("download",		CL_Download_f,			"Manually download a file from the server");
	Cmd_AddCommand ("exit",			CL_Quit_f,				"Exits");
	Cmd_AddCommand ("pause",		CL_Pause_f,				"Pauses the game");
	Cmd_AddCommand ("pingservers",	CL_PingServers_f,		"Queries the servers in your address book");
	Cmd_AddCommand ("precache",		CL_Precache_f,			"");
	Cmd_AddCommand ("quit",			CL_Quit_f,				"Exits");
	Cmd_AddCommand ("rcon",			CL_Rcon_f,				"");
	Cmd_AddCommand ("reconnect",	CL_Reconnect_f,			"Reconnect to the current server");
	Cmd_AddCommand ("record",		CL_Record_f,			"Start recording a demo");
	Cmd_AddCommand ("savecfg",		CL_WriteConfig,			"Manually save configuration");
	Cmd_AddCommand ("say",			CL_Say_Preprocessor,	"");
	Cmd_AddCommand ("say_team",		CL_Say_Preprocessor,	"");
	Cmd_AddCommand ("setenv",		CL_Setenv_f,			"");
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
	cls.disableScreen = qFalse;	// don't draw yet

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

	CL_UIAPI_Shutdown ();
	CL_CGameAPI_Shutdown ();

	CDAudio_Shutdown ();
	Snd_Shutdown ();

	CL_Loc_Shutdown ();

	IN_Shutdown ();
	VID_Shutdown ();
}
