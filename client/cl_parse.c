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

// cl_parse.c
// - parse a message received from the server

#include "cl_local.h"

char *svc_strings[256] = {
	"svc_bad",

	"svc_muzzleflash",
	"svc_muzzlflash2",
	"svc_temp_entity",
	"svc_layout",
	"svc_inventory",

	"svc_nop",
	"svc_disconnect",
	"svc_reconnect",
	"svc_sound",
	"svc_print",
	"svc_stufftext",
	"svc_serverdata",
	"svc_configstring",
	"svc_spawnbaseline",	
	"svc_centerprint",
	"svc_download",
	"svc_playerinfo",
	"svc_packetentities",
	"svc_deltapacketentities",
	"svc_frame"
};

//=============================================================================

/*
===============
CL_DownloadFileName
===============
*/
void CL_DownloadFileName (char *dest, int destlen, char *fn) {
	if (!strncmp (fn, "players", 7))
		Q_snprintfz (dest, destlen, "%s/%s", BASE_MODDIRNAME, fn);
	else
		Q_snprintfz (dest, destlen, "%s/%s", FS_Gamedir(), fn);
}


/*
===============
CL_CheckOrDownloadFile

Returns qTrue if the file exists, otherwise it attempts
to start a download from the server.
===============
*/
qBool CL_CheckOrDownloadFile (char *filename) {
	FILE	*fp;
	char	name[MAX_OSPATH];

	if (strstr (filename, "..")) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Refusing to download a path with ..\n");
		return qTrue;
	}

	if (FS_LoadFile (filename, NULL) != -1) {
		/* it exists, no need to download */
		return qTrue;
	}

	strcpy (cls.downloadName, filename);

	/*
	** download to a temp name, and only rename
	** to the real name when done, so if interrupted
	** a runt file wont be left
	*/
	Com_StripExtension (cls.downloadName, cls.downloadTempName);
	strcat (cls.downloadTempName, ".tmp");

	/* ZOID
	** check to see if we already have a tmp for this file, if so, try to resume
	** open the file if not opened yet
	*/
	CL_DownloadFileName (name, sizeof (name), cls.downloadTempName);

	fp = fopen (name, "r+b");
	if (fp) {
		/* it exists */
		int len;
		fseek(fp, 0, SEEK_END);
		len = ftell(fp);

		cls.downloadFile = fp;

		/* give the server an offset to start the download */
		Com_Printf (PRINT_ALL, "Resuming %s\n", cls.downloadName);
		MSG_WriteByte (&cls.netChan.message, CLC_STRINGCMD);
		MSG_WriteString (&cls.netChan.message, va("download %s %i", cls.downloadName, len));
	} else {
		Com_Printf (PRINT_ALL, "Downloading %s\n", cls.downloadName);
		MSG_WriteByte (&cls.netChan.message, CLC_STRINGCMD);
		MSG_WriteString (&cls.netChan.message, va("download %s", cls.downloadName));
	}

	cls.downloadNumber++;

	return qFalse;
}


/*
===============
CL_Download_f

Request a download from the server
===============
*/
void CL_Download_f (void) {
	char	filename[MAX_OSPATH];

	if (Cmd_Argc () != 2) {
		Com_Printf (PRINT_ALL, "Usage: download <filename>\n");
		return;
	}

	Q_snprintfz (filename, sizeof (filename), "%s", Cmd_Argv (1));

	if (strstr (filename, "..")) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Refusing to download a path with ..\n");
		return;
	}

	if (FS_LoadFile (filename, NULL) != -1) {
		/* it exists, no need to download */
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "File already exists.\n");
		return;
	}

	strcpy (cls.downloadName, filename);
	Com_Printf (PRINT_ALL, "Downloading %s\n", cls.downloadName);

	/*
	** download to a temp name, and only rename to the real name when done,
	** so if interrupted a runt file wont be left
	*/
	Com_StripExtension (cls.downloadName, cls.downloadTempName);
	strcat (cls.downloadTempName, ".tmp");

	MSG_WriteByte (&cls.netChan.message, CLC_STRINGCMD);
	MSG_WriteString (&cls.netChan.message, va("download %s", cls.downloadName));

	cls.downloadNumber++;
}


/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
void CL_ParseDownload (void) {
	int		size, percent;
	char	name[MAX_OSPATH];
	int		r;

	/* read the data */
	size = MSG_ReadShort (&net_Message);
	percent = MSG_ReadByte (&net_Message);
	if (size == -1) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Server does not have this file.\n");
		if (cls.downloadFile) {
			/* if here, we tried to resume a file but the server said no */
			fclose (cls.downloadFile);
			cls.downloadFile = NULL;
		}
		CL_RequestNextDownload ();
		return;
	}

	/* open the file if not opened yet */
	if (!cls.downloadFile) {
		CL_DownloadFileName (name, sizeof (name), cls.downloadTempName);

		FS_CreatePath (name);

		cls.downloadFile = fopen (name, "wb");
		if (!cls.downloadFile) {
			net_Message.readCount += size;
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Failed to open %s\n", cls.downloadTempName);
			CL_RequestNextDownload ();
			return;
		}
	}

	fwrite (net_Message.data + net_Message.readCount, 1, size, cls.downloadFile);
	net_Message.readCount += size;

	if (percent != 100) {
		/* request next block */
		cls.downloadPercent = percent;

		MSG_WriteByte (&cls.netChan.message, CLC_STRINGCMD);
		MSG_Print (&cls.netChan.message, "nextdl");
	} else {
		char	oldn[MAX_OSPATH];
		char	newn[MAX_OSPATH];

		fclose (cls.downloadFile);

		/* rename the temp file to it's final name */
		CL_DownloadFileName (oldn, sizeof (oldn), cls.downloadTempName);
		CL_DownloadFileName (newn, sizeof (newn), cls.downloadName);
		r = rename (oldn, newn);
		if (r)
			Com_Printf (PRINT_ALL, S_COLOR_RED "Failed to rename!\n");

		cls.downloadFile = NULL;
		cls.downloadPercent = 0;

		/* get another file if needed */
		CL_RequestNextDownload ();
	}
}

/*
=====================================================================

	SERVER CONNECTING MESSAGES

=====================================================================
*/

/*
==================
CL_ParseServerData
==================
*/
void CL_ParseServerData (void) {
	extern cvar_t	*fs_gamedirvar;
	char	*str;
	int		i;
	
	Com_Printf (PRINT_DEV, "Serverdata packet received.\n");

	/* wipe the clientState_t struct */
	CL_ClearState ();
	CL_SetConnectState (CA_CONNECTED);

	/* parse protocol version number */
	i = MSG_ReadLong (&net_Message);
	cls.serverProtocol = i;

	/* BIG HACK to let demos from release work with the 3.0x patch!!! */
	if (Com_ServerState () && (PROTOCOL_VERSION == 34)) {
	} else if (i != PROTOCOL_VERSION)
		Com_Error (ERR_DROP, "Server returned version %i, not %i", i, PROTOCOL_VERSION);

	cl.serverCount = MSG_ReadLong (&net_Message);
	cl.attractLoop = MSG_ReadByte (&net_Message);

	/* game directory */
	str = MSG_ReadString (&net_Message);
	strncpy (cl.gameDir, str, sizeof (cl.gameDir)-1);

	/* set gamedir */
	if ((*str && (!fs_gamedirvar->string || !*fs_gamedirvar->string || strcmp(fs_gamedirvar->string, str))) || (!*str && (fs_gamedirvar->string || *fs_gamedirvar->string)))
		Cvar_Set ("game", str);

	/* parse player entity number */
	cl.playerNum = MSG_ReadShort (&net_Message);

	/* get the full level name */
	str = MSG_ReadString (&net_Message);

	if (cl.playerNum == -1) {
		/* playing a cinematic or showing a pic, not a level */
		CIN_PlayCinematic (str);
	} else {
		/* seperate the printfs so the server message can have a color */
		Com_Printf (PRINT_ALL, "\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
		Com_Printf (PRINT_ALL, "%c%s\n", 2, str);

		/* need to prep refresh at next oportunity */
		cl.refreshPrepped = qFalse;
	}
}


/*
==================
CL_ParseBaseline
==================
*/
void CL_ParseBaseline (void) {
	entityState_t	*es;
	entityState_t	nullState;
	int				bits;
	int				newNum;

	memset (&nullState, 0, sizeof (nullState));

	newNum = CL_ParseEntityBits (&bits);
	es = &cl_Entities[newNum].baseLine;
	CL_ParseDelta (&nullState, es, newNum, bits);
}


/*
================
CL_LoadClientinfo
================
*/
qBool	change_prediction = qFalse;
int		classtype = GLM_DEFAULT;
void CL_LoadClientinfo (clientInfo_t *ci, char *s) {
	int			i;
	char		*t;
	char		model_name[MAX_QPATH];
	char		skin_name[MAX_QPATH];
	char		model_filename[MAX_QPATH];
	char		skin_filename[MAX_QPATH];
	char		weapon_filename[MAX_QPATH];

	strncpy(ci->cinfo, s, sizeof (ci->cinfo));
	ci->cinfo[sizeof (ci->cinfo)-1] = 0;

	/* isolate the player's name */
	strncpy(ci->name, s, sizeof (ci->name));
	ci->name[sizeof (ci->name)-1] = 0;
	t = strstr (s, "\\");

	if (t) {
		ci->name[t-s] = 0;
		s = t+1;
	}

	if (cl_noskins->integer || (*s == 0)) {
		Com_sprintf (model_filename, sizeof (model_filename),	"players/male/tris.md2");
		Com_sprintf (weapon_filename, sizeof (weapon_filename),	"players/male/weapon.md2");
		Com_sprintf (skin_filename, sizeof (skin_filename),		"players/male/grunt.pcx");
		Com_sprintf (ci->iconname, sizeof (ci->iconname),		"players/male/grunt_i.pcx");

		ci->model = Mod_RegisterModel (model_filename);
		memset(ci->weaponmodel, 0, sizeof (ci->weaponmodel));
		ci->weaponmodel[0] = Mod_RegisterModel (weapon_filename);
		ci->skin = Img_RegisterSkin (skin_filename);
		ci->icon = Img_RegisterPic (ci->iconname);
	} else {
		/* isolate the model name */
		strcpy (model_name, s);
		t = strstr(model_name, "/");
		if (!t)
			t = strstr(model_name, "\\");
		if (!t)
			t = model_name;
		*t = 0;

		/* isolate the skin name */
		strcpy (skin_name, s + strlen(model_name) + 1);

		/* jump prediction hack */
		if (FS_CurrGame ("gloom")) {
			if (change_prediction) {
				classtype = GLM_DEFAULT;
				if(!strcmp (model_name,"male")) {
					if (!strcmp (skin_name, "commando"))		classtype = GLM_COMMANDO;
					else if (!strcmp (skin_name, "shotgun"))	classtype = GLM_ST;
					else if (!strcmp (skin_name, "soldier"))	classtype = GLM_GRUNT;
					else if (!strcmp (skin_name, "obs"))		classtype = GLM_OBSERVER;
				} else if(!strcmp (model_name, "female")) {
					if (!strcmp (skin_name, "bio"))	classtype = GLM_BIOTECH;
				} else if (!strcmp (model_name, "hatch")) {
					if(!strcmp (skin_name, "kam"))	classtype = GLM_KAMIKAZE;
					else							classtype = GLM_HATCHLING;
				} else if (!strcmp (model_name, "breeder"))	classtype = GLM_BREEDER;
				else if (!strcmp (model_name, "engineer"))	classtype = GLM_ENGINEER;
				else if (!strcmp (model_name, "drone"))		classtype = GLM_DRONE;
				else if (!strcmp (model_name, "wraith"))	classtype = GLM_WRAITH;
				else if (!strcmp (model_name, "stinger"))	classtype = GLM_STINGER;
				else if (!strcmp (model_name, "guardian"))	classtype = GLM_GUARDIAN;
				else if (!strcmp (model_name, "stalker"))	classtype = GLM_STALKER;
				else if (!strcmp (model_name, "hsold"))		classtype = GLM_HT;
				else if (!strcmp (model_name, "exterm"))	classtype = GLM_EXTERM;
				else if (!strcmp (model_name, "mech"))		classtype = GLM_MECH;
				else										classtype = GLM_DEFAULT;

				change_prediction = qFalse;
			}
		} else
			classtype = GLM_DEFAULT;

		/* model file */
		Com_sprintf (model_filename, sizeof (model_filename), "players/%s/tris.md2", model_name);
		ci->model = Mod_RegisterModel (model_filename);
		if (!ci->model) {
			strcpy (model_name, "male");
			Com_sprintf (model_filename, sizeof (model_filename), "players/male/tris.md2");
			ci->model = Mod_RegisterModel (model_filename);
		}

		/* skin file */
		Com_sprintf (skin_filename, sizeof (skin_filename), "players/%s/%s.pcx", model_name, skin_name);
		ci->skin = Img_RegisterSkin (skin_filename);

		/*
		** if we don't have the skin and the model wasn't male,
		** see if the male has it (this is for CTF's skins)
		*/
		if (!ci->skin && Q_stricmp (model_name, "male")) {
			/* change model to male */
			strcpy (model_name, "male");
			Com_sprintf (model_filename, sizeof (model_filename), "players/male/tris.md2");
			ci->model = Mod_RegisterModel (model_filename);

			/* see if the skin exists for the male model */
			Com_sprintf (skin_filename, sizeof (skin_filename), "players/%s/%s.pcx", model_name, skin_name);
			ci->skin = Img_RegisterSkin (skin_filename);
		}

		/*
		** if we still don't have a skin, it means that the male model didn't have
		** it, so default to grunt
		*/
		if (!ci->skin) {
			/* see if the skin exists for the male model */
			Com_sprintf (skin_filename, sizeof (skin_filename), "players/%s/grunt.pcx", model_name, skin_name);
			ci->skin = Img_RegisterSkin (skin_filename);
		}

		/* weapon file */
		for (i=0 ; i<cl_NumWeaponModels ; i++) {
			Com_sprintf (weapon_filename, sizeof (weapon_filename), "players/%s/%s", model_name, cl_WeaponModels[i]);
			ci->weaponmodel[i] = Mod_RegisterModel(weapon_filename);
			if (!ci->weaponmodel[i] && strcmp (model_name, "cyborg") == 0) {
				/* try male */
				Com_sprintf (weapon_filename, sizeof (weapon_filename), "players/male/%s", cl_WeaponModels[i]);
				ci->weaponmodel[i] = Mod_RegisterModel (weapon_filename);
			}

			if (!cl_vwep->integer)
				break; // only one when vwep is off
		}

		/* icon file */
		Com_sprintf (ci->iconname, sizeof (ci->iconname), "/players/%s/%s_i.pcx", model_name, skin_name);
		ci->icon = Img_RegisterPic (ci->iconname);
	}

	/* must have loaded all data types to be valud */
	if (!ci->skin || !ci->icon || !ci->model || !ci->weaponmodel[0]) {
		ci->skin = NULL;
		ci->icon = NULL;
		ci->model = NULL;
		ci->weaponmodel[0] = NULL;
		return;
	}
}


/*
================
CL_ParseClientinfo

Load the skin, icon, and model for a client
================
*/
void CL_ParseClientinfo (int player) {
	char			*s;
	clientInfo_t	*ci;

	s = cl.configStrings[player+CS_PLAYERSKINS];

	ci = &cl.clientInfo[player];

	change_prediction = (cl.playerNum == player) ? qTrue : qFalse;

	CL_LoadClientinfo (ci, s);
}


/*
================
CL_ParseConfigString
================
*/
void CL_ParseConfigString (void) {
	int		i;
	char	*s;
	char	olds[MAX_QPATH];

	i = MSG_ReadShort (&net_Message);
	if (i < 0 || i >= MAX_CONFIGSTRINGS)
		Com_Error (ERR_DROP, "configstring > MAX_CONFIGSTRINGS");
	s = MSG_ReadString(&net_Message);

	strncpy (olds, cl.configStrings[i], sizeof (olds));
	olds[sizeof (olds) - 1] = 0;

	strcpy (cl.configStrings[i], s);

	/* do something apropriate */
	if (i >= CS_LIGHTS && i < CS_LIGHTS+MAX_LIGHTSTYLES) {
		CL_SetLightstyle (i - CS_LIGHTS);
	} else if (i == CS_CDTRACK) {
		if (cl.refreshPrepped)
			CDAudio_Play (atoi(cl.configStrings[CS_CDTRACK]), qTrue);
	} else if (i >= CS_MODELS && i < CS_MODELS+MAX_MODELS) {
		if (cl.refreshPrepped) {
			cl.modelDraw[i-CS_MODELS] = Mod_RegisterModel (cl.configStrings[i]);
			if (cl.configStrings[i][0] == '*')
				cl.modelClip[i-CS_MODELS] = CM_InlineModel (cl.configStrings[i]);
			else
				cl.modelClip[i-CS_MODELS] = NULL;
		}
	} else if (i >= CS_SOUNDS && i < CS_SOUNDS+MAX_MODELS) {
		if (cl.refreshPrepped)
			cl.soundPrecache[i-CS_SOUNDS] = Snd_RegisterSound (cl.configStrings[i]);
	} else if (i >= CS_IMAGES && i < CS_IMAGES+MAX_MODELS) {
		if (cl.refreshPrepped)
			cl.imagePrecache[i-CS_IMAGES] = Img_RegisterPic (cl.configStrings[i]);
	} else if (i >= CS_PLAYERSKINS && i < CS_PLAYERSKINS+MAX_CLIENTS) {
		if (cl.refreshPrepped && strcmp(olds, s))
			CL_ParseClientinfo (i-CS_PLAYERSKINS);
	}
}

/*
=====================================================================

	ACTION MESSAGES

=====================================================================
*/

/*
==================
CL_ParseStartSoundPacket
==================
*/
void CL_ParseStartSoundPacket (void) {
	vec3_t	pos_vec;
	int		channel, ent, sound_num, flags;
	float	*pos, volume, attenuation, ofs;

	flags = MSG_ReadByte (&net_Message);
	sound_num = MSG_ReadByte (&net_Message);

	if (flags & SND_VOLUME)
		volume = MSG_ReadByte (&net_Message) / 255.0;
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME;
	
	if (flags & SND_ATTENUATION)
		attenuation = MSG_ReadByte (&net_Message) / 64.0;
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;	

	if (flags & SND_OFFSET)
		ofs = MSG_ReadByte (&net_Message) / 1000.0;
	else
		ofs = 0;

	if (flags & SND_ENT) {
		/* entity reletive */
		channel = MSG_ReadShort(&net_Message); 
		ent = channel>>3;
		if (ent > MAX_EDICTS)
			Com_Error (ERR_DROP,"CL_ParseStartSoundPacket: ent = %i", ent);

		channel &= 7;
	} else {
		ent = 0;
		channel = 0;
	}

	if (flags & SND_POS) {
		/* positioned in space */
		MSG_ReadPos (&net_Message, pos_vec);
 
		pos = pos_vec;
	} else
		/* use entity number */
		pos = NULL;

	if (!cl.soundPrecache[sound_num])
		return;

	Snd_StartSound (pos, ent, channel, cl.soundPrecache[sound_num], volume, attenuation, ofs);
}


/*
=====================
CL_ParseServerMessage
=====================
*/
void SHOWNET (char *s) {
	if (cl_shownet->value >= 2)
		Com_Printf (PRINT_ALL, "%3i:%s\n", net_Message.readCount-1, s);
}
int		pvernext = 0;
void CL_ParseServerMessage (void) {
	int			cmd, i;
	char		*s, timestamp[10];
	time_t		ctime;
	struct tm	*ltime;
	static int	queryLastTime = 0;

	if (cl_timestamp->integer) {
		ctime = time (NULL);
		ltime = localtime (&ctime);
		strftime (timestamp, sizeof (timestamp), "%I:%M %p", ltime);

		if (timestamp[0] == '0') {
			for (i=0 ; i<(strlen(timestamp) + 1) ; i++)
				timestamp[i] = timestamp[i+1];
			timestamp[i+1] = '\0';
		}
	}

	/* if recording demos, copy the message out */
	if (cl_shownet->value == 1)
		Com_Printf (PRINT_ALL, "%i ",net_Message.curSize);
	else if (cl_shownet->value >= 2)
		Com_Printf (PRINT_ALL, "------------------\n");

	/* parse the message */
	for ( ; ; ) {
		if (net_Message.readCount > net_Message.curSize) {
			Com_Error (ERR_DROP,"CL_ParseServerMessage: Bad server message");
			break;
		}

		cmd = MSG_ReadByte (&net_Message);

		if (cmd == -1) {
			SHOWNET("END OF MESSAGE");
			break;
		}

		if (cl_shownet->value >= 2) {
			if (!svc_strings[cmd])
				Com_Printf (PRINT_ALL, S_COLOR_YELLOW "%3i:BAD CMD %i\n", net_Message.readCount-1,cmd);
			else
				SHOWNET (svc_strings[cmd]);
		}
	
		/* other commands */
		switch (cmd) {
		case SVC_NOP:
			break;
			
		case SVC_DISCONNECT:
			Com_Error (ERR_DISCONNECT, "Server disconnected\n");
			break;

		case SVC_RECONNECT:
			Com_Printf (PRINT_ALL, "Server disconnected, reconnecting\n");
			if (cls.downloadFile) {
				// ZOID, close download
				fclose (cls.downloadFile);
				cls.downloadFile = NULL;
			}
			CL_SetConnectState (CA_CONNECTING);
			cls.connectTime = -99999;	// CL_CheckForResend () will fire immediately
			break;

		case SVC_PRINT:
			i = MSG_ReadByte (&net_Message);
			s = MSG_ReadString (&net_Message);

			if (i == PRINT_CHAT) {
				Snd_StartLocalSound ("misc/talk.wav");

				con.orMask = 128;
				if (cl_timestamp->integer)
					Com_Printf (PRINT_ALL, "[%s] %s", timestamp, s);
				else
					Com_Printf (PRINT_ALL, "%s", s);
				con.orMask = 0;

				if (!queryLastTime || (queryLastTime < cls.realTime - (30 * 1000))) {
					if (strstr (s, "!p_version")) {
						queryLastTime = cls.realTime;
						GL_VersionMsg_f ();
					} else if (strstr (s, "!p_renderer")) {
						queryLastTime = cls.realTime;
						GL_RendererMsg_f ();
					}
				} else {
					if (strstr (s, "!p_version") || strstr (s, "!p_renderer"))
						Com_Printf (PRINT_ALL, "Ignoring inquery for %d more seconds\n", 30 - ((cls.realTime - queryLastTime) / 1000));
				}
			} else {
				if (cl_timestamp->integer > 1)
					Com_Printf (PRINT_ALL, "[%s] %s", timestamp, s);
				else
					Com_Printf (PRINT_ALL, "%s", s);
			}
			break;
			
		case SVC_CENTERPRINT:
			SCR_CenterPrint (MSG_ReadString (&net_Message));
			break;
			
		case SVC_STUFFTEXT:
			s = MSG_ReadString (&net_Message);
			Com_Printf (PRINT_DEV, "stufftext: %s\n", s);
			Cbuf_AddText (s);
			break;
			
		case SVC_SERVERDATA:
			Cbuf_Execute ();	// make sure any stuffed commands are done
			CL_ParseServerData ();
			break;
			
		case SVC_CONFIGSTRING:	CL_ParseConfigString ();	break;
		case SVC_SOUND:			CL_ParseStartSoundPacket();	break;
		case SVC_SPAWNBASELINE:	CL_ParseBaseline ();		break;
		case SVC_TEMP_ENTITY:	CL_ParseTEnt ();			break;
		case SVC_MUZZLEFLASH:	CL_ParseMuzzleFlash ();		break;
		case SVC_MUZZLEFLASH2:	CL_ParseMuzzleFlash2 ();	break;
		case SVC_DOWNLOAD:		CL_ParseDownload ();		break;
		case SVC_FRAME:			CL_ParseFrame ();			break;
		case SVC_INVENTORY:		HUD_ParseInventory ();		break;

		case SVC_LAYOUT:
			s = MSG_ReadString (&net_Message);
			strncpy (cl.layout, s, sizeof (cl.layout)-1);
			break;

		case SVC_PLAYERINFO:
		case SVC_PACKETENTITIES:
		case SVC_DELTAPACKETENTITIES:
			Com_Error (ERR_DROP, "Out of place frame data '%s'",
				(cmd == SVC_PLAYERINFO) ? "svc_playerinfo" :
				(cmd == SVC_PACKETENTITIES) ? "svc_packetentities" :
				(cmd == SVC_DELTAPACKETENTITIES) ? "svc_deltapacketentities" : "shouldn't happen");
			break;

		default:
			Com_Error (ERR_DROP,"CL_ParseServerMessage: Illegible server message\n");
			break;
		}
	}

	CL_AddNetgraph ();

	/*
	** we don't know if it is ok to save a demo message until
	** after we have parsed the frame
	*/
	if (cls.demoRecording && !cls.demoWaiting)
		CL_WriteDemoMessage ();
}
