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
// cl_parse.c
// Parses messages received from the server
//

#include "cl_local.h"

static const char *svcStrings[256] = {
	"SVC_BAD",

	"SVC_MUZZLEFLASH",
	"SVC_MUZZLEFLASH2",
	"SVC_TEMP_ENTITY",
	"SVC_LAYOUT",
	"SVC_INVENTORY",

	"SVC_NOP",
	"SVC_DISCONNECT",
	"SVC_RECONNECT",
	"SVC_SOUND",
	"SVC_PRINT",
	"SVC_STUFFTEXT",
	"SVC_SERVERDATA",
	"SVC_CONFIGSTRING",
	"SVC_SPAWNBASELINE",	
	"SVC_CENTERPRINT",
	"SVC_DOWNLOAD",
	"SVC_PLAYERINFO",
	"SVC_PACKETENTITIES",
	"SVC_DELTAPACKETENTITIES",
	"SVC_FRAME",

	"SVC_ZPACKET",			// new for ENHANCED_PROTOCOL_VERSION
	"SVC_ZDOWNLOAD"			// new for ENHANCED_PROTOCOL_VERSION
};

/*
=================
SHOWNET
=================
*/
static void SHOWNET (const char *name) {
	if (cl_shownet->value >= 2)
		Com_Printf (0, "%3i:%s\n", netMessage.readCount-1, name);
}


/*
=================
CL_ParseEntityBits

Returns the entity number and the header bits
=================
*/
static int CL_ParseEntityBits (uInt *bits) {
	uInt		b, total;
	int			number;

	total = MSG_ReadByte (&netMessage);
	if (total & U_MOREBITS1) {
		b = MSG_ReadByte (&netMessage);
		total |= b<<8;
	}
	if (total & U_MOREBITS2) {
		b = MSG_ReadByte (&netMessage);
		total |= b<<16;
	}
	if (total & U_MOREBITS3) {
		b = MSG_ReadByte (&netMessage);
		total |= b<<24;
	}

	if (total & U_NUMBER16)
		number = MSG_ReadShort (&netMessage);
	else
		number = MSG_ReadByte (&netMessage);

	*bits = total;

	return number;
}


/*
==================
CL_ParseDelta

Can go from either a baseline or a previous packet_entity
==================
*/
static void CL_ParseDelta (entityState_t *from, entityState_t *to, int number, int bits) {
	// set everything to the state we are delta'ing from
	*to = *from;

	if (cls.serverProtocol != ENHANCED_PROTOCOL_VERSION)
		VectorCopy (from->origin, to->oldOrigin);
	else if (!(bits & U_OLDORIGIN) && !(from->renderFx & RF_BEAM) && cls.serverProtocol == ENHANCED_PROTOCOL_VERSION)
		VectorCopy (from->origin, to->oldOrigin);

	to->number = number;

	if (bits & U_MODEL)
		to->modelIndex = MSG_ReadByte (&netMessage);
	if (bits & U_MODEL2)
		to->modelIndex2 = MSG_ReadByte (&netMessage);
	if (bits & U_MODEL3)
		to->modelIndex3 = MSG_ReadByte (&netMessage);
	if (bits & U_MODEL4)
		to->modelIndex4 = MSG_ReadByte (&netMessage);
		
	if (bits & U_FRAME8)
		to->frame = MSG_ReadByte (&netMessage);
	if (bits & U_FRAME16)
		to->frame = MSG_ReadShort (&netMessage);

	if ((bits & U_SKIN8) && (bits & U_SKIN16))
		to->skinNum = MSG_ReadLong (&netMessage); // used for laser colors
	else if (bits & U_SKIN8)
		to->skinNum = MSG_ReadByte (&netMessage);
	else if (bits & U_SKIN16)
		to->skinNum = MSG_ReadShort (&netMessage);

	if ((bits & (U_EFFECTS8|U_EFFECTS16)) == (U_EFFECTS8|U_EFFECTS16))
		to->effects = MSG_ReadLong (&netMessage);
	else if (bits & U_EFFECTS8)
		to->effects = MSG_ReadByte (&netMessage);
	else if (bits & U_EFFECTS16)
		to->effects = MSG_ReadShort (&netMessage);

	if ((bits & (U_RENDERFX8|U_RENDERFX16)) == (U_RENDERFX8|U_RENDERFX16))
		to->renderFx = MSG_ReadLong (&netMessage);
	else if (bits & U_RENDERFX8)
		to->renderFx = MSG_ReadByte (&netMessage);
	else if (bits & U_RENDERFX16)
		to->renderFx = MSG_ReadShort (&netMessage);

	if (bits & U_ORIGIN1)
		to->origin[0] = MSG_ReadCoord (&netMessage);
	if (bits & U_ORIGIN2)
		to->origin[1] = MSG_ReadCoord (&netMessage);
	if (bits & U_ORIGIN3)
		to->origin[2] = MSG_ReadCoord (&netMessage);
		
	if (bits & U_ANGLE1)
		to->angles[0] = MSG_ReadAngle (&netMessage);
	if (bits & U_ANGLE2)
		to->angles[1] = MSG_ReadAngle (&netMessage);
	if (bits & U_ANGLE3)
		to->angles[2] = MSG_ReadAngle (&netMessage);

	if (bits & U_OLDORIGIN)
		MSG_ReadPos (&netMessage, to->oldOrigin);

	if (bits & U_SOUND)
		to->sound = MSG_ReadByte (&netMessage);

	if (bits & U_EVENT)
		to->event = MSG_ReadByte (&netMessage);
	else
		to->event = 0;

	if (bits & U_SOLID)
		to->solid = MSG_ReadShort (&netMessage);

	if ((bits & U_VELOCITY) && (cls.serverProtocol == ENHANCED_PROTOCOL_VERSION))
		MSG_ReadPos (&netMessage, to->velocity);
}

/*
=========================================================================

	FRAME PARSING

=========================================================================
*/

/*
===================
CL_ParsePlayerstate
===================
*/
static void CL_ParsePlayerstate (void) {
	playerStateNew_t	*state;
	int					i, statbits;
	int					flags;

	state = &cl.frame.playerState;

	// clear to old value before delta parsing
	if (cl.oldFrame)
		*state = cl.oldFrame->playerState;
	else
		memset (state, 0, sizeof (*state));

	flags = MSG_ReadShort (&netMessage);

	//
	// parse the pMoveState_t
	//
	if (flags & PS_M_TYPE)
		state->pMove.pmType = MSG_ReadByte (&netMessage);

	if (flags & PS_M_ORIGIN) {
		state->pMove.origin[0] = MSG_ReadShort (&netMessage);
		state->pMove.origin[1] = MSG_ReadShort (&netMessage);
		state->pMove.origin[2] = MSG_ReadShort (&netMessage);
	}

	if (flags & PS_M_VELOCITY) {
		state->pMove.velocity[0] = MSG_ReadShort (&netMessage);
		state->pMove.velocity[1] = MSG_ReadShort (&netMessage);
		state->pMove.velocity[2] = MSG_ReadShort (&netMessage);
	}

	if (flags & PS_M_TIME)
		state->pMove.pmTime = MSG_ReadByte (&netMessage);

	if (flags & PS_M_FLAGS)
		state->pMove.pmFlags = MSG_ReadByte (&netMessage);

	if (flags & PS_M_GRAVITY)
		state->pMove.gravity = MSG_ReadShort (&netMessage);

	if (flags & PS_M_DELTA_ANGLES) {
		state->pMove.deltaAngles[0] = MSG_ReadShort (&netMessage);
		state->pMove.deltaAngles[1] = MSG_ReadShort (&netMessage);
		state->pMove.deltaAngles[2] = MSG_ReadShort (&netMessage);
	}

	if (cl.attractLoop)
		state->pMove.pmType = PMT_FREEZE;		// demo playback

	//
	// parse the rest of the playerState_t
	//
	if (flags & PS_VIEWOFFSET) {
		state->viewOffset[0] = MSG_ReadChar (&netMessage) * 0.25;
		state->viewOffset[1] = MSG_ReadChar (&netMessage) * 0.25;
		state->viewOffset[2] = MSG_ReadChar (&netMessage) * 0.25;
	}

	if (flags & PS_VIEWANGLES) {
		state->viewAngles[0] = MSG_ReadAngle16 (&netMessage);
		state->viewAngles[1] = MSG_ReadAngle16 (&netMessage);
		state->viewAngles[2] = MSG_ReadAngle16 (&netMessage);
	}

	if (flags & PS_KICKANGLES) {
		state->kickAngles[0] = MSG_ReadChar (&netMessage) * 0.25;
		state->kickAngles[1] = MSG_ReadChar (&netMessage) * 0.25;
		state->kickAngles[2] = MSG_ReadChar (&netMessage) * 0.25;
	}

	if (flags & PS_WEAPONINDEX)
		state->gunIndex = MSG_ReadByte (&netMessage);

	if (flags & PS_WEAPONFRAME) {
		state->gunFrame = MSG_ReadByte (&netMessage);
		state->gunOffset[0] = MSG_ReadChar (&netMessage) * 0.25;
		state->gunOffset[1] = MSG_ReadChar (&netMessage) * 0.25;
		state->gunOffset[2] = MSG_ReadChar (&netMessage) * 0.25;
		state->gunAngles[0] = MSG_ReadChar (&netMessage) * 0.25;
		state->gunAngles[1] = MSG_ReadChar (&netMessage) * 0.25;
		state->gunAngles[2] = MSG_ReadChar (&netMessage) * 0.25;
	}

	if (flags & PS_BLEND) {
		state->vBlend[0] = MSG_ReadByte (&netMessage) * ONEDIV255;
		state->vBlend[1] = MSG_ReadByte (&netMessage) * ONEDIV255;
		state->vBlend[2] = MSG_ReadByte (&netMessage) * ONEDIV255;
		state->vBlend[3] = MSG_ReadByte (&netMessage) * ONEDIV255;
	}

	if (flags & PS_FOV)
		state->fov = MSG_ReadByte (&netMessage);

	if (flags & PS_RDFLAGS)
		state->rdFlags = MSG_ReadByte (&netMessage);

	if (flags & PS_BBOX) {
		int x, zd, zu;
		int solid;

		solid = MSG_ReadShort (&netMessage);

		x = 8*(solid & 31);
		zd = 8*((solid>>5) & 31);
		zu = 8*((solid>>10) & 63) - 32;

		state->mins[0] = state->mins[1] = -x;
		state->maxs[0] = state->maxs[1] = x;
		state->mins[2] = -zd;
		state->maxs[2] = zu;
		Com_Printf (0, "received bbox from server: (%f, %f, %f), (%f, %f, %f)\n",
			state->mins[0], state->mins[1], state->mins[2],
			state->maxs[0], state->maxs[1], state->maxs[2]);
	}

	// parse stats
	statbits = MSG_ReadLong (&netMessage);
	for (i=0 ; i<MAX_STATS ; i++) {
		if (statbits & (1<<i))
			state->stats[i] = MSG_ReadShort (&netMessage);
	}
}


/*
==================
CL_ParsePacketEntities

An SVC_PACKETENTITIES has just been parsed, deal with the rest of the data stream.
==================
*/
static void CL_DeltaEntity (int newnum, entityState_t *old, int bits) {
	// Parses deltas from the given base, and adds the resulting entity to the current frame
	entityState_t	*state;

	state = &clParseEntities[cl.parseEntities & (MAX_PARSEENTITIES_MASK)];
	cl.parseEntities++;
	cl.frame.numEntities++;

	CL_ParseDelta (old, state, newnum, bits);
	CL_CGModule_NewPacketEntityState (newnum, *state);
}
static void CL_ParsePacketEntities (void) {
	int				newnum;
	uInt			bits;
	entityState_t	*oldstate;
	int				oldindex, oldnum = 99999;

	cl.frame.parseEntities = cl.parseEntities;
	cl.frame.numEntities = 0;

	CL_CGModule_BeginFrameSequence ();

	// delta from the entities present in oldFrame
	oldindex = 0;
	if (cl.oldFrame) {
		if (oldindex < cl.oldFrame->numEntities) {
			oldstate = &clParseEntities[(cl.oldFrame->parseEntities+oldindex) & (MAX_PARSEENTITIES_MASK)];
			oldnum = oldstate->number;
		}
	}

	for ( ; ; ) {
		newnum = CL_ParseEntityBits (&bits);
		if (newnum >= MAX_CS_EDICTS)
			Com_Error (ERR_DROP, "CL_ParsePacketEntities: bad number:%i", newnum);

		if (netMessage.readCount > netMessage.curSize)
			Com_Error (ERR_DROP, "CL_ParsePacketEntities: end of message");

		if (!newnum)
			break;

		while (oldnum < newnum) {
			// one or more entities from the old packet are unchanged
			if (cl_shownet->value == 3)
				Com_Printf (0, "   unchanged: %i\n", oldnum);
			CL_DeltaEntity (oldnum, oldstate, 0);
			
			oldindex++;

			if (oldindex >= cl.oldFrame->numEntities)
				oldnum = 99999;
			else {
				oldstate = &clParseEntities[(cl.oldFrame->parseEntities+oldindex) & (MAX_PARSEENTITIES_MASK)];
				oldnum = oldstate->number;
			}
		}

		if (bits & U_REMOVE) {
			// the entity present in oldFrame is not in the current frame
			if (cl_shownet->value == 3)
				Com_Printf (0, "   remove: %i\n", newnum);
			if (oldnum != newnum)
				Com_Printf (PRNT_DEV, S_COLOR_YELLOW "U_REMOVE: oldnum != newnum\n");

			oldindex++;

			if (oldindex >= cl.oldFrame->numEntities)
				oldnum = 99999;
			else {
				oldstate = &clParseEntities[(cl.oldFrame->parseEntities+oldindex) & (MAX_PARSEENTITIES_MASK)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if (oldnum == newnum) {
			// delta from previous state
			if (cl_shownet->value == 3)
				Com_Printf (0, "   delta: %i\n", newnum);
			CL_DeltaEntity (newnum, oldstate, bits);

			oldindex++;

			if (oldindex >= cl.oldFrame->numEntities)
				oldnum = 99999;
			else {
				oldstate = &clParseEntities[(cl.oldFrame->parseEntities+oldindex) & (MAX_PARSEENTITIES_MASK)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if (oldnum > newnum) {
			// delta from baseline
			if (cl_shownet->value == 3)
				Com_Printf (0, "   baseline: %i\n", newnum);
			CL_DeltaEntity (newnum, &clBaseLines[newnum], bits);
			continue;
		}

	}

	// any remaining entities in the old frame are copied over
	while (oldnum != 99999) {
		// one or more entities from the old packet are unchanged
		if (cl_shownet->value == 3)
			Com_Printf (0, "   unchanged: %i\n", oldnum);
		CL_DeltaEntity (oldnum, oldstate, 0);
		
		oldindex++;

		if (oldindex >= cl.oldFrame->numEntities)
			oldnum = 99999;
		else {
			oldstate = &clParseEntities[(cl.oldFrame->parseEntities+oldindex) & (MAX_PARSEENTITIES_MASK)];
			oldnum = oldstate->number;
		}
	}

	CL_CGModule_EndFrameSequence ();
}


/*
================
CL_ParseFrame
================
*/
static void CL_ParseFrame (void) {
	int			cmd;
	int			len;

	memset (&cl.frame, 0, sizeof (cl.frame));

	cl.frame.serverFrame = MSG_ReadLong (&netMessage);
	cl.frame.deltaFrame = MSG_ReadLong (&netMessage);
	cl.frame.serverTime = cl.frame.serverFrame*100;

	// BIG HACK to let old demos continue to work
	if (cls.serverProtocol != 26)
		cl.surpressCount = MSG_ReadByte (&netMessage);

	if (cl_shownet->value == 3)
		Com_Printf (0, "   frame:%i  delta:%i\n", cl.frame.serverFrame, cl.frame.deltaFrame);

	// If the frame is delta compressed from data that we no longer have available, we must
	// suck up the rest of the frame, but not use it, then ask for a non-compressed message
	if (cl.frame.deltaFrame <= 0) {
		cl.frame.valid = qTrue;		// uncompressed frame
		cl.oldFrame = NULL;
		cls.demoWaiting = qFalse;	// we can start recording now
	}
	else {
		cl.oldFrame = &cl.frames[cl.frame.deltaFrame & UPDATE_MASK];
		if (!cl.oldFrame->valid) {
			// should never happen
			Com_Printf (0, S_COLOR_RED "Delta from invalid frame (not supposed to happen!).\n");
		}
		if (cl.oldFrame->serverFrame != cl.frame.deltaFrame) {
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly
			Com_Printf (PRNT_DEV, S_COLOR_YELLOW "Delta frame too old.\n");
		}
		else if (cl.parseEntities - cl.oldFrame->parseEntities > MAX_PARSE_ENTITIES-128) {
			Com_Printf (PRNT_DEV, S_COLOR_YELLOW "Delta parseEntities too old.\n");
		}
		else
			cl.frame.valid = qTrue;	// valid delta parse
	}

	// read areaBits
	len = MSG_ReadByte (&netMessage);
	MSG_ReadData (&netMessage, &cl.frame.areaBits, len);

	// read playerinfo
	cmd = MSG_ReadByte (&netMessage);
	SHOWNET (svcStrings[cmd]);
	if (cmd != SVC_PLAYERINFO)
		Com_Error (ERR_DROP, "CL_ParseFrame: not playerinfo");
	CL_ParsePlayerstate ();

	// read packet entities
	cmd = MSG_ReadByte (&netMessage);
	SHOWNET (svcStrings[cmd]);
	if (cmd != SVC_PACKETENTITIES)
		Com_Error (ERR_DROP, "CL_ParseFrame: not packetentities");
	CL_ParsePacketEntities ();

	// save the frame off in the backup array for later delta comparisons
	cl.frames[cl.frame.serverFrame & UPDATE_MASK] = cl.frame;

	if (cl.frame.valid) {
		// getting a valid frame message ends the connection process
		if (cls.connectState != CA_ACTIVE)
			CL_SetState (CA_ACTIVE);
		cls.soundPrepped = qTrue;	// can start mixing ambient sounds
	}
}

/*
=====================================================================

	DOWNLOAD PARSING AND HANDLING

=====================================================================
*/

/*
===============
CL_DownloadFileName
===============
*/
static void CL_DownloadFileName (char *dest, int destLen, char *fileName) {
	if (!strncmp (fileName, "players", 7))
		Q_snprintfz (dest, destLen, "%s/%s", BASE_MODDIRNAME, fileName);
	else
		Q_snprintfz (dest, destLen, "%s/%s", FS_Gamedir(), fileName);
}


/*
===============
CL_CheckOrDownloadFile

Returns qTrue if the file exists, otherwise it attempts
to start a download from the server.
===============
*/
qBool CL_CheckOrDownloadFile (char *fileName) {
	FILE	*fp;
	char	name[MAX_OSPATH], *p;

	while ((p=strstr (cls.download.name, "\\")))
		*p = '/';

	if (strstr (fileName, "..")) {
		Com_Printf (0, S_COLOR_YELLOW "Refusing to download a path with '..'\n");
		return qTrue;
	}

	if (FS_LoadFile (fileName, NULL) != -1) {
		// it exists, no need to download
		return qTrue;
	}

	Q_strncpyz (cls.download.name, fileName, sizeof (cls.download.name));

	/*
	** download to a temp name, and only rename
	** to the real name when done, so if interrupted
	** a runt file wont be left
	*/
	Com_StripExtension (cls.download.tempName, sizeof (cls.download.tempName), cls.download.name);
	Q_strcat (cls.download.tempName, ".tmp", sizeof (cls.download.tempName));

	/*
	** ZOID
	** check to see if we already have a tmp for this file, if so, try to resume
	** open the file if not opened yet
	*/
	CL_DownloadFileName (name, sizeof (name), cls.download.tempName);

	fp = fopen (name, "r+b");
	if (fp) {
		// it exists
		int		len;
		fseek (fp, 0, SEEK_END);
		len = ftell (fp);

		cls.download.file = fp;

		// give the server an offset to start the download
		Com_Printf (0, "Resuming %s\n", cls.download.name);
		MSG_WriteByte (&cls.netChan.message, CLC_STRINGCMD);
		if (cls.serverProtocol == ENHANCED_PROTOCOL_VERSION) {
			MSG_WriteString (&cls.netChan.message, Q_VarArgs ("download \"%s\" %i udp-zlib", cls.download.name, len));
		}
		else {
			MSG_WriteString (&cls.netChan.message, Q_VarArgs ("download \"%s\" %i", cls.download.name, len));
		}
	}
	else {
		Com_Printf (0, "Downloading %s\n", cls.download.name);
		MSG_WriteByte (&cls.netChan.message, CLC_STRINGCMD);
		if (cls.serverProtocol == ENHANCED_PROTOCOL_VERSION) {
			MSG_WriteString (&cls.netChan.message, Q_VarArgs ("download \"%s\" 0 udp-zlib", cls.download.name));
		}
		else {
			MSG_WriteString (&cls.netChan.message, Q_VarArgs ("download \"%s\"", cls.download.name));
		}
	}

	cls.forcePacket = qTrue;

	return qFalse;
}


/*
===============
CL_Download_f

Request a download from the server
===============
*/
void CL_Download_f (void) {
	char	fileName[MAX_OSPATH];

	if (Cmd_Argc () != 2) {
		Com_Printf (0, "Usage: download <filename>\n");
		return;
	}

	Q_snprintfz (fileName, sizeof (fileName), "%s", Cmd_Argv (1));

	if (strstr (fileName, "..")) {
		Com_Printf (0, S_COLOR_YELLOW "Refusing to download a path with ..\n");
		return;
	}

	if (FS_LoadFile (fileName, NULL) != -1) {
		// it exists, no need to download
		Com_Printf (0, S_COLOR_YELLOW "File already exists.\n");
		return;
	}

	Q_strncpyz (cls.download.name, fileName, sizeof (cls.download.name));
	Com_Printf (0, "Downloading %s\n", cls.download.name);

	/*
	** download to a temp name, and only rename to the real name when done,
	** so if interrupted a runt file wont be left
	*/
	Com_StripExtension (cls.download.tempName, sizeof (cls.download.tempName), cls.download.name);
	Q_strcat (cls.download.tempName, ".tmp", sizeof (cls.download.tempName));

	MSG_WriteByte (&cls.netChan.message, CLC_STRINGCMD);
	if (cls.serverProtocol == ENHANCED_PROTOCOL_VERSION) {
		MSG_WriteString (&cls.netChan.message, Q_VarArgs ("download \"%s\" 0 udp-zlib", cls.download.name));
	}
	else {
		MSG_WriteString (&cls.netChan.message, Q_VarArgs ("download \"%s\"", cls.download.name));
	}

	cls.forcePacket = qTrue;
}


/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
#include "../renderer/png/zlib.h" // orly put this into files.c for server usage
int ZLibDecompress (byte *in, int inlen, byte *out, int outlen, int wbits) {
	z_stream	zs;
	int			result;

	memset (&zs, 0, sizeof (zs));

	zs.next_in = in;
	zs.avail_in = 0;

	zs.next_out = out;
	zs.avail_out = outlen;

	result = inflateInit2 (&zs, wbits);
	if (result != Z_OK) {
		Sys_Error ("Error on inflateInit %d\nMessage: %s\n", result, zs.msg);
		return 0;
	}

	zs.avail_in = inlen;

	result = inflate (&zs, Z_FINISH);
	if (result != Z_STREAM_END) {
		Sys_Error("Error on inflate %d\nMessage: %s\n", result, zs.msg);
		zs.total_out = 0;
	}

	result = inflateEnd (&zs);
	if (result != Z_OK) {
		Sys_Error("Error on inflateEnd %d\nMessage: %s\n", result, zs.msg);
		return 0;
	}

	return zs.total_out;
}

int ZLibCompressChunk (byte *in, int len_in, byte *out, int len_out, int method, int wbits) {
	z_stream	zs;
	int			result;

	zs.next_in = in;
	zs.avail_in = len_in;
	zs.total_in = 0;

	zs.next_out = out;
	zs.avail_out = len_out;
	zs.total_out = 0;

	zs.msg = NULL;
	zs.state = NULL;
	zs.zalloc = Z_NULL;
	zs.zfree = Z_NULL;
	zs.opaque = NULL;

	zs.data_type = Z_BINARY;
	zs.adler = 0;
	zs.reserved = 0;

	result = deflateInit2 (&zs, method, Z_DEFLATED, wbits, 9, Z_DEFAULT_STRATEGY);
	if (result != Z_OK) {
		return 0;
	}

	result = deflate(&zs, Z_FINISH);
	if (result != Z_STREAM_END) {
		return 0;
	}

	result = deflateEnd(&zs);
	if (result != Z_OK) {
		return 0;
	}

	return zs.total_out;
}
static void CL_ParseDownload (qBool compressed) {
	int		size, percent;
	char	name[MAX_OSPATH];

	// read the data
	size = MSG_ReadShort (&netMessage);
	percent = MSG_ReadByte (&netMessage);
	if (size == -1) {
		Com_Printf (0, S_COLOR_YELLOW "Server does not have this file.\n");
		if (cls.download.file) {
			// if here, we tried to resume a file but the server said no
			fclose (cls.download.file);
			cls.download.file = NULL;
		}
		CL_RequestNextDownload ();
		return;
	}

	// open the file if not opened yet
	if (!cls.download.file) {
		CL_DownloadFileName (name, sizeof (name), cls.download.tempName);

		FS_CreatePath (name);

		cls.download.file = fopen (name, "wb");
		if (!cls.download.file) {
			netMessage.readCount += size;
			Com_Printf (0, S_COLOR_YELLOW "Failed to open %s\n", cls.download.tempName);
			CL_RequestNextDownload ();
			return;
		}
	}

	// insert block
 	if (compressed) {
 		unsigned short	uncompressedLen;
 		byte			uncompressed[0xFFFF];

 		uncompressedLen = MSG_ReadShort (&netMessage);
 		if (!uncompressedLen)
			Com_Error (ERR_DROP, "CL_ParseDownload: uncompressedLen == 0");

 		ZLibDecompress (netMessage.data + netMessage.readCount, size, uncompressed, uncompressedLen, -15);
 		fwrite (uncompressed, 1, uncompressedLen, cls.download.file);
 		Com_Printf (PRNT_DEV, "SVC_ZDOWNLOAD(%s): %d -> %d\n", cls.download.name, size, uncompressedLen);
 	}
 	else {
		fwrite (netMessage.data + netMessage.readCount, 1, size, cls.download.file);
	}

	netMessage.readCount += size;

	if (percent != 100) {
		// request next block
		cls.download.percent = percent;

		if (!cl.enhancedServer) {
			MSG_WriteByte (&cls.netChan.message, CLC_STRINGCMD);
			MSG_Print (&cls.netChan.message, "nextdl");
			cls.forcePacket = qTrue;
		}
	}
	else {
		char	oldName[MAX_OSPATH];
		char	newName[MAX_OSPATH];
		int		rn;

		fclose (cls.download.file);

		// rename the temp file to it's final name
		CL_DownloadFileName (oldName, sizeof (oldName), cls.download.tempName);
		CL_DownloadFileName (newName, sizeof (newName), cls.download.name);
		rn = rename (oldName, newName);
		if (rn)
			Com_Printf (0, S_COLOR_RED "Failed to rename!\n");

		cls.download.file = NULL;
		cls.download.percent = 0;

		// get another file if needed
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
static void CL_ParseServerData (void) {
	extern	cVar_t *fs_gamedirvar;
	char	*str;
	int		i;

	Com_Printf (PRNT_DEV, "Serverdata packet received.\n");

	// wipe the clientState_t struct
	CL_ClearState ();
	CL_SetState (CA_CONNECTED);

	// parse protocol version number
	i = MSG_ReadLong (&netMessage);
	cls.serverProtocol = i;

	// BIG HACK to let demos from release work with the 3.0x patch!!!
	if ((i != ORIGINAL_PROTOCOL_VERSION) && (i != ENHANCED_PROTOCOL_VERSION) && (i != 26))
		Com_Error (ERR_DROP, "You are running protocol version %i, server is running %i. These are incompatible, please update your client to protocol version %d", ENHANCED_PROTOCOL_VERSION, i, i);

	cl.serverCount = MSG_ReadLong (&netMessage);
	cl.attractLoop = MSG_ReadByte (&netMessage);

	// game directory
	str = MSG_ReadString (&netMessage);
	Q_strncpyz (cl.gameDir, str, sizeof (cl.gameDir));

	// set gamedir
	if ((*str && (!fs_gamedirvar->string || !*fs_gamedirvar->string || strcmp (fs_gamedirvar->string, str))) || (!*str && (fs_gamedirvar->string || *fs_gamedirvar->string)))
		Cvar_Set ("game", str);

	// parse player entity number
	cl.playerNum = MSG_ReadShort (&netMessage);

	// get the full level name
	str = MSG_ReadString (&netMessage);

	// read in download server port (if any)
	if ((cls.serverProtocol == ENHANCED_PROTOCOL_VERSION) && !cl.attractLoop)
		cl.enhancedServer = MSG_ReadByte (&netMessage);
	else
		cl.enhancedServer = 0;

	Com_Printf (PRNT_DEV, "Serverdata packet received. protocol=%d, serverCount=%d, attractLoop=%d, playerNum=%d, game=%s, map=%s\n", cls.serverProtocol, cl.serverCount, cl.attractLoop, cl.playerNum, cl.gameDir, str);

	CL_RestartMedia ();

	if (cl.playerNum == -1) {
		// playing a cinematic or showing a pic, not a level
		CIN_PlayCinematic (str);
	}
	else {
		// seperate the printfs so the server message can have a color
		Com_Printf (0, "\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
		Com_Printf (0, "%c%s\n", 2, str);

		// need to prep refresh at next oportunity
		cls.refreshPrepped = qFalse;
	}
}


/*
==================
CL_ParseBaseline
==================
*/
static void CL_ParseBaseline (void) {
	entityState_t	*es;
	entityState_t	nullState;
	int				bits;
	int				newNum;

	memset (&nullState, 0, sizeof (nullState));

	newNum = CL_ParseEntityBits (&bits);
	es = &clBaseLines[newNum];
	CL_ParseDelta (&nullState, es, newNum, bits);
}


/*
================
CL_ParseConfigString
================
*/
static void CL_ParseConfigString (void) {
	int		num;
	char	*str;

	num = MSG_ReadShort (&netMessage);
	if ((num < 0) || (num >= MAX_CONFIGSTRINGS))
		Com_Error (ERR_DROP, "CL_ParseConfigString: bad num");
	str = MSG_ReadString (&netMessage);

	strcpy (cl.configStrings[num], str);

	if (num == CS_CDTRACK) {
		// cd track
		CDAudio_Play (atoi (cl.configStrings[CS_CDTRACK]), qTrue);
	}
	else
		CL_CGModule_ParseConfigString (num, str);
}

/*
=====================================================================

	SERVER MESSAGES

=====================================================================
*/

/*
==================
CL_ParseStartSoundPacket
==================
*/
static void CL_ParseStartSoundPacket (void) {
	vec3_t	pos_vec;
	int		channel, ent, soundNum, flags;
	float	*pos, volume, attenuation, ofs;

	flags = MSG_ReadByte (&netMessage);
	soundNum = MSG_ReadByte (&netMessage);

	volume = (flags & SND_VOLUME) ? MSG_ReadByte (&netMessage) * ONEDIV255 : DEFAULT_SOUND_PACKET_VOLUME;
	attenuation = (flags & SND_ATTENUATION) ? MSG_ReadByte (&netMessage) / 64.0 : DEFAULT_SOUND_PACKET_ATTENUATION;
	ofs = (flags & SND_OFFSET) ? MSG_ReadByte (&netMessage) / 1000.0 : 0;

	if (flags & SND_ENT) {
		// entity reletive
		channel = MSG_ReadShort (&netMessage); 
		ent = channel >> 3;
		if (ent > MAX_CS_EDICTS)
			Com_Error (ERR_DROP, "CL_ParseStartSoundPacket: ent = %i", ent);

		channel &= 7;
	}
	else {
		ent = 0;
		channel = 0;
	}

	if (flags & SND_POS) {
		// positioned in space
		MSG_ReadPos (&netMessage, pos_vec);
 
		pos = pos_vec;
	}
	else
		// use entity number
		pos = NULL;

	if (cl.configStrings[CS_SOUNDS+soundNum][0])
		Snd_StartSound (pos, ent, channel, Snd_RegisterSound (cl.configStrings[CS_SOUNDS+soundNum]), volume, attenuation, ofs);
}


/*
=====================
CL_ParseZPacket
=====================
*/
void CL_ParseZPacket (void) {
	byte		*buff_in;
	byte		*buff_out;
	netMsg_t	sb, old;
	short		compressedLen;
	short		uncompressedLen;

	compressedLen = MSG_ReadShort (&netMessage);
	uncompressedLen = MSG_ReadShort (&netMessage);

	if (cls.serverProtocol != ENHANCED_PROTOCOL_VERSION)
		Com_Error (ERR_DROP, "CL_ParseZPacket: SVC_ZPACKET -requires- ENHANCED_PROTOCOL_VERSION");

	if (uncompressedLen <= 0)
		Com_Error (ERR_DROP, "CL_ParseZPacket: uncompressedLen <= 0");
	if (compressedLen <= 0)
		Com_Error (ERR_DROP, "CL_ParseZPacket: compressedLen <= 0");

	buff_in = Mem_Alloc (compressedLen);
	buff_out = Mem_Alloc (uncompressedLen);

	MSG_ReadData (&netMessage, buff_in, compressedLen);

	MSG_Init (&sb, buff_out, uncompressedLen);
	sb.curSize = ZLibDecompress (buff_in, compressedLen, buff_out, uncompressedLen, -15);

	old = netMessage;
	netMessage = sb;

	CL_ParseServerMessage ();

	netMessage = old;

	Mem_Free (buff_in);
	Mem_Free (buff_out);

	Com_Printf (PRNT_DEV, "Got a ZPacket, %d->%d\n", uncompressedLen + 4, compressedLen);
}


/*
=====================
CL_ParseServerMessage
=====================
*/
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
			for (i=0 ; i<(Q_strlen(timestamp) + 1) ; i++)
				timestamp[i] = timestamp[i+1];
			timestamp[i+1] = '\0';
		}
	}

	// if recording demos, copy the message out
	if (cl_shownet->value == 1)
		Com_Printf (0, "%i ", netMessage.curSize);
	else if (cl_shownet->value >= 2)
		Com_Printf (0, "------------------\n");

	CL_CGModule_StartServerMessage ();

	// parse the message
	for ( ; ; ) {
		if (netMessage.readCount > netMessage.curSize) {
			Com_Error (ERR_DROP, "CL_ParseServerMessage: Bad server message");
			break;
		}

		cmd = MSG_ReadByte (&netMessage);

		if (cmd == -1) {
			SHOWNET ("END OF MESSAGE");
			break;
		}

		if (cl_shownet->value >= 2) {
			if (!svcStrings[cmd])
				Com_Printf (0, S_COLOR_YELLOW "%3i:BAD CMD %i\n", netMessage.readCount-1, cmd);
			else
				SHOWNET (svcStrings[cmd]);
		}
	
		//
		// these are private to the client and server
		//
		switch (cmd) {
		case SVC_NOP:
			break;

		case SVC_DISCONNECT:
			cls.connectCount = 0;
			Com_Error (ERR_DISCONNECT, "Server disconnected\n");
			break;

		case SVC_RECONNECT:
			Com_Printf (0, "Server disconnected, reconnecting\n");
			if (cls.download.file) {
				// ZOID, close download
				fclose (cls.download.file);
				cls.download.file = NULL;
			}
			CL_SetState (CA_CONNECTING);
			cls.connectTime = -99999;	// CL_CheckForResend () will fire immediately
			cls.connectCount = 0;
			break;

		case SVC_SOUND:
			CL_ParseStartSoundPacket ();
			break;

		case SVC_PRINT:
			i = MSG_ReadByte (&netMessage);
			s = MSG_ReadString (&netMessage);

			switch (i) {
			case PRINT_CHAT:
				Snd_StartLocalSound (clMedia.talkSfx);

				con.orMask = 128;
				if (cl_timestamp->integer)
					Com_Printf (0, "[%s] %s", timestamp, s);
				else
					Com_Printf (0, "%s", s);
				con.orMask = 0;

				if (!queryLastTime || (queryLastTime < cls.realTime - (300 * 1000))) {
					// lets wait 300 seconds
					if (strstr (s, "!version")) {
						queryLastTime = cls.realTime;
						GL_VersionMsg_f ();
					}
					else if (strstr (s, "!renderer")) {
						queryLastTime = cls.realTime;
						GL_RendererMsg_f ();
					}
				}
				break;

			default:
				if (cl_timestamp->integer > 1)
					Com_Printf (0, "[%s] %s", timestamp, s);
				else
					Com_Printf (0, "%s", s);
				break;
			}
			break;

		case SVC_STUFFTEXT:
			s = MSG_ReadString (&netMessage);
			Com_Printf (PRNT_DEV, "stufftext: %s\n", s);
			Cbuf_AddText (s);
			break;

		case SVC_SERVERDATA:
			Cbuf_Execute ();	// make sure any stuffed commands are done
			CL_ParseServerData ();
			break;

		case SVC_CONFIGSTRING:
			CL_ParseConfigString ();
			break;

		case SVC_SPAWNBASELINE:
			CL_ParseBaseline ();
			break;

		case SVC_CENTERPRINT:
			CL_CGModule_ParseCenterPrint ();
			break;

		case SVC_DOWNLOAD:
			CL_ParseDownload (qFalse);
			break;

		case SVC_PLAYERINFO:
			Com_Error (ERR_DROP, "CL_ParseServerMessage: Out of place frame data 'SVC_PLAYERINFO'");
			break;
		case SVC_PACKETENTITIES:
			Com_Error (ERR_DROP, "CL_ParseServerMessage: Out of place frame data 'SVC_PACKETENTITIES'");
			break;
		case SVC_DELTAPACKETENTITIES:
			Com_Error (ERR_DROP, "CL_ParseServerMessage: Out of place frame data 'SVC_DELTAPACKETENTITIES'");
			break;

		case SVC_FRAME:
			CL_ParseFrame ();
			break;

		case SVC_ZPACKET:
			CL_ParseZPacket ();
			break;

		case SVC_ZDOWNLOAD:
			CL_ParseDownload (qTrue);
			break;

		default:
			if (!CL_CGModule_ParseServerMessage (cmd)) {
				// orly this is just temporary
				switch (cmd) {
				case SVC_INVENTORY:
					Com_Printf (0, S_COLOR_RED "Missed packet: SVC_INVENTORY\n");
					break;

				case SVC_LAYOUT:
					Com_Printf (0, S_COLOR_RED "Missed packet: SVC_LAYOUT\n");
					break;

				case SVC_MUZZLEFLASH:
					Com_Printf (0, S_COLOR_RED "Missed packet: SVC_MUZZLEFLASH\n");
					break;

				case SVC_MUZZLEFLASH2:
					Com_Printf (0, S_COLOR_RED "Missed packet: SVC_MUZZLEFLASH2\n");
					break;

				case SVC_TEMP_ENTITY:
					Com_Printf (0, S_COLOR_RED "Missed packet: SVC_TEMP_ENTITY\n");
					break;
				// orly this is just temporary
				}
				Com_Error (ERR_DROP, "CL_ParseServerMessage: Illegible server message #%i\n(Usually caused by packet loss or a faulty connection)", cmd);
			}
			break;
		}
	}

	CL_CGModule_EndServerMessage ();
	CL_AddNetgraph ();

	// we don't know if it is ok to save a demo message until after we have parsed the frame
	if (cls.demoRecording && !cls.demoWaiting)
		CL_WriteDemoMessage ();
}
