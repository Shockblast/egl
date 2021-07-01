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

static char *svcStrings[256] = {
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
static void SHOWNET (const char *name)
{
	if (cl_shownet->value >= 2)
		Com_Printf (0, "%3i:%s\n", cls.netMessage.readCount-1, name);
}


/*
=================
CL_ParseEntityBits

Returns the entity number and the header bits
=================
*/
static int CL_ParseEntityBits (uint32 *bits)
{
	uint32		b, total;
	int			number;

	total = MSG_ReadByte (&cls.netMessage);
	if (total & U_MOREBITS1) {
		b = MSG_ReadByte (&cls.netMessage);
		total |= b<<8;
	}
	if (total & U_MOREBITS2) {
		b = MSG_ReadByte (&cls.netMessage);
		total |= b<<16;
	}
	if (total & U_MOREBITS3) {
		b = MSG_ReadByte (&cls.netMessage);
		total |= b<<24;
	}

	if (total & U_NUMBER16)
		number = MSG_ReadShort (&cls.netMessage) % MAX_ENTITIES;
	else
		number = MSG_ReadByte (&cls.netMessage);

	*bits = total;

	return number;
}


/*
==================
CL_ParseDelta

Can go from either a baseline or a previous packet_entity
==================
*/
static void CL_ParseDelta (entityState_t *from, entityState_t *to, int number, int bits)
{
	// set everything to the state we are delta'ing from
	*to = *from;

	if (cls.serverProtocol != ENHANCED_PROTOCOL_VERSION)
		VectorCopy (from->origin, to->oldOrigin);
	else if (!(bits & U_OLDORIGIN) && !(from->renderFx & RF_BEAM))
		VectorCopy (from->origin, to->oldOrigin);

	to->number = number;

	if (bits & U_MODEL)
		to->modelIndex = MSG_ReadByte (&cls.netMessage);
	if (bits & U_MODEL2)
		to->modelIndex2 = MSG_ReadByte (&cls.netMessage);
	if (bits & U_MODEL3)
		to->modelIndex3 = MSG_ReadByte (&cls.netMessage);
	if (bits & U_MODEL4)
		to->modelIndex4 = MSG_ReadByte (&cls.netMessage);
		
	if (bits & U_FRAME8)
		to->frame = MSG_ReadByte (&cls.netMessage);
	if (bits & U_FRAME16)
		to->frame = MSG_ReadShort (&cls.netMessage);

	if ((bits & U_SKIN8) && (bits & U_SKIN16))
		to->skinNum = MSG_ReadLong (&cls.netMessage); // used for laser colors
	else if (bits & U_SKIN8)
		to->skinNum = MSG_ReadByte (&cls.netMessage);
	else if (bits & U_SKIN16)
		to->skinNum = MSG_ReadShort (&cls.netMessage);

	if ((bits & (U_EFFECTS8|U_EFFECTS16)) == (U_EFFECTS8|U_EFFECTS16))
		to->effects = MSG_ReadLong (&cls.netMessage);
	else if (bits & U_EFFECTS8)
		to->effects = MSG_ReadByte (&cls.netMessage);
	else if (bits & U_EFFECTS16)
		to->effects = MSG_ReadShort (&cls.netMessage);

	if ((bits & (U_RENDERFX8|U_RENDERFX16)) == (U_RENDERFX8|U_RENDERFX16))
		to->renderFx = MSG_ReadLong (&cls.netMessage);
	else if (bits & U_RENDERFX8)
		to->renderFx = MSG_ReadByte (&cls.netMessage);
	else if (bits & U_RENDERFX16)
		to->renderFx = MSG_ReadShort (&cls.netMessage);

	if (bits & U_ORIGIN1)
		to->origin[0] = MSG_ReadCoord (&cls.netMessage);
	if (bits & U_ORIGIN2)
		to->origin[1] = MSG_ReadCoord (&cls.netMessage);
	if (bits & U_ORIGIN3)
		to->origin[2] = MSG_ReadCoord (&cls.netMessage);
		
	if (bits & U_ANGLE1)
		to->angles[0] = MSG_ReadAngle (&cls.netMessage);
	if (bits & U_ANGLE2)
		to->angles[1] = MSG_ReadAngle (&cls.netMessage);
	if (bits & U_ANGLE3)
		to->angles[2] = MSG_ReadAngle (&cls.netMessage);

	if (bits & U_OLDORIGIN)
		MSG_ReadPos (&cls.netMessage, to->oldOrigin);

	if (bits & U_SOUND)
		to->sound = MSG_ReadByte (&cls.netMessage);

	if (bits & U_EVENT)
		to->event = MSG_ReadByte (&cls.netMessage);
	else
		to->event = 0;

	if (bits & U_SOLID)
		to->solid = MSG_ReadShort (&cls.netMessage);

	if (bits & U_VELOCITY && cls.serverProtocol == ENHANCED_PROTOCOL_VERSION)
		MSG_ReadPos (&cls.netMessage, to->velocity);
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
static void CL_ParsePlayerstate (const frame_t *oldFrame, frame_t *newFrame, int extraFlags)
{
	playerStateNew_t	*state;
	int					i, statBits;
	int					flags;
	qBool				enhanced;

	enhanced = (cls.serverProtocol == ENHANCED_PROTOCOL_VERSION);

	state = &newFrame->playerState;

	// clear to old value before delta parsing
	if (oldFrame)
		*state = oldFrame->playerState;
	else
		memset (state, 0, sizeof (*state));

	flags = MSG_ReadShort (&cls.netMessage);

	//
	// parse the pMoveState_t
	//
	if (flags & PS_M_TYPE)
		state->pMove.pmType = MSG_ReadByte (&cls.netMessage);

	// protocol changes
	if (flags & PS_M_ORIGIN) {
		if (!enhanced)
			extraFlags |= EPS_PMOVE_ORIGIN2;
		state->pMove.origin[0] = MSG_ReadShort (&cls.netMessage);
		state->pMove.origin[1] = MSG_ReadShort (&cls.netMessage);
	}
	if (extraFlags & EPS_PMOVE_ORIGIN2)
		state->pMove.origin[2] = MSG_ReadShort (&cls.netMessage);

	if (flags & PS_M_VELOCITY) {
		if (!enhanced)
			extraFlags |= EPS_PMOVE_VELOCITY2;
		state->pMove.velocity[0] = MSG_ReadShort (&cls.netMessage);
		state->pMove.velocity[1] = MSG_ReadShort (&cls.netMessage);
	}
	if (extraFlags & EPS_PMOVE_VELOCITY2)
		state->pMove.velocity[2] = MSG_ReadShort (&cls.netMessage);
	// protocol changes

	if (flags & PS_M_TIME)
		state->pMove.pmTime = MSG_ReadByte (&cls.netMessage);

	if (flags & PS_M_FLAGS)
		state->pMove.pmFlags = MSG_ReadByte (&cls.netMessage);

	if (flags & PS_M_GRAVITY)
		state->pMove.gravity = MSG_ReadShort (&cls.netMessage);

	if (flags & PS_M_DELTA_ANGLES) {
		state->pMove.deltaAngles[0] = MSG_ReadShort (&cls.netMessage);
		state->pMove.deltaAngles[1] = MSG_ReadShort (&cls.netMessage);
		state->pMove.deltaAngles[2] = MSG_ReadShort (&cls.netMessage);
	}

	//
	// parse the rest of the playerState_t
	//
	if (flags & PS_VIEWOFFSET) {
		state->viewOffset[0] = MSG_ReadChar (&cls.netMessage) * 0.25;
		state->viewOffset[1] = MSG_ReadChar (&cls.netMessage) * 0.25;
		state->viewOffset[2] = MSG_ReadChar (&cls.netMessage) * 0.25;
	}

	// protocol changes
	if (flags & PS_VIEWANGLES) {
		if (!enhanced)
			extraFlags |= EPS_VIEWANGLE2;
		state->viewAngles[0] = MSG_ReadAngle16 (&cls.netMessage);
		state->viewAngles[1] = MSG_ReadAngle16 (&cls.netMessage);
	}
	if (extraFlags & EPS_VIEWANGLE2)
		state->viewAngles[2] = MSG_ReadAngle16 (&cls.netMessage);
	// protocol changes

	if (flags & PS_KICKANGLES) {
		state->kickAngles[0] = MSG_ReadChar (&cls.netMessage) * 0.25;
		state->kickAngles[1] = MSG_ReadChar (&cls.netMessage) * 0.25;
		state->kickAngles[2] = MSG_ReadChar (&cls.netMessage) * 0.25;
	}

	if (flags & PS_WEAPONINDEX)
		state->gunIndex = MSG_ReadByte (&cls.netMessage);

	// protocol changes
	if (flags & PS_WEAPONFRAME) {
		if (!enhanced)
			extraFlags |= EPS_GUNOFFSET|EPS_GUNANGLES;
		state->gunFrame = MSG_ReadByte (&cls.netMessage);
	}
	if (extraFlags & EPS_GUNOFFSET) {
		state->gunOffset[0] = MSG_ReadChar (&cls.netMessage)*0.25f;
		state->gunOffset[1] = MSG_ReadChar (&cls.netMessage)*0.25f;
		state->gunOffset[2] = MSG_ReadChar (&cls.netMessage)*0.25f;
	}
	if (extraFlags & EPS_GUNANGLES) {
		state->gunAngles[0] = MSG_ReadChar (&cls.netMessage)*0.25f;
		state->gunAngles[1] = MSG_ReadChar (&cls.netMessage)*0.25f;
		state->gunAngles[2] = MSG_ReadChar (&cls.netMessage)*0.25f;
	}
	// protocol changes

	if (flags & PS_BLEND) {
		state->viewBlend[0] = MSG_ReadByte (&cls.netMessage) * (1.0f/255.0f);
		state->viewBlend[1] = MSG_ReadByte (&cls.netMessage) * (1.0f/255.0f);
		state->viewBlend[2] = MSG_ReadByte (&cls.netMessage) * (1.0f/255.0f);
		state->viewBlend[3] = MSG_ReadByte (&cls.netMessage) * (1.0f/255.0f);
	}

	if (flags & PS_FOV)
		state->fov = MSG_ReadByte (&cls.netMessage);

	if (flags & PS_RDFLAGS)
		state->rdFlags = MSG_ReadByte (&cls.netMessage);

	if (flags & PS_BBOX) {
		int x, zd, zu;
		int solid;

		solid = MSG_ReadShort (&cls.netMessage);

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
	if (!enhanced)
		extraFlags |= EPS_STATS;
	if (extraFlags & EPS_STATS) {
		statBits = MSG_ReadLong (&cls.netMessage);
		if (statBits) {
			for (i=0 ; i<MAX_STATS ; i++)
				if (statBits & (1<<i))
					state->stats[i] = MSG_ReadShort (&cls.netMessage);
		}
	}
}


/*
==================
CL_ParsePacketEntities

An SVC_PACKETENTITIES has just been parsed, deal with the rest of the data stream.
==================
*/
static void CL_DeltaEntity (frame_t *newFrame, int newNum, entityState_t *old, int bits)
{
	// Parses deltas from the given base, and adds the resulting entity to the current frame
	entityState_t	*state;

	state = &cl_parseEntities[cl.parseEntities & (MAX_PARSEENTITIES_MASK)];
	cl.parseEntities++;
	newFrame->numEntities++;

	CL_ParseDelta (old, state, newNum, bits);
	CL_CGModule_NewPacketEntityState (newNum, *state);
}
static void CL_ParsePacketEntities (const frame_t *oldFrame, frame_t *newFrame)
{
	int				newNum;
	uint32			bits;
	entityState_t	*oldState = NULL;
	int				oldIndex, oldNum = 99999;

	newFrame->parseEntities = cl.parseEntities;
	newFrame->numEntities = 0;

	CL_CGModule_BeginFrameSequence ();

	// Delta from the entities present in oldFrame
	oldIndex = 0;
	if (oldFrame) {
		if (oldIndex < oldFrame->numEntities) {
			oldState = &cl_parseEntities[(oldFrame->parseEntities+oldIndex) & (MAX_PARSEENTITIES_MASK)];
			oldNum = oldState->number;
		}
	}

	for ( ; ; ) {
		newNum = CL_ParseEntityBits (&bits);
		if (newNum >= MAX_CS_EDICTS)
			Com_Error (ERR_DROP, "CL_ParsePacketEntities: bad number:%i", newNum);

		if (cls.netMessage.readCount > cls.netMessage.curSize)
			Com_Error (ERR_DROP, "CL_ParsePacketEntities: end of message");

		if (!newNum)
			break;

		while (oldNum < newNum) {
			// One or more entities from the old packet are unchanged
			if (cl_shownet->value == 3)
				Com_Printf (0, "   unchanged: %i\n", oldNum);
			CL_DeltaEntity (newFrame, oldNum, oldState, 0);
			
			oldIndex++;

			if (oldIndex >= oldFrame->numEntities)
				oldNum = 99999;
			else {
				oldState = &cl_parseEntities[(oldFrame->parseEntities+oldIndex) & (MAX_PARSEENTITIES_MASK)];
				oldNum = oldState->number;
			}
		}

		if (bits & U_REMOVE) {
			// The entity present in oldFrame is not in the current frame
			if (cl_shownet->value == 3)
				Com_Printf (0, "   remove: %i\n", newNum);
			if (oldNum != newNum)
				Com_DevPrintf (PRNT_WARNING, "U_REMOVE: oldNum != newNum\n");

			oldIndex++;

			if (oldIndex >= oldFrame->numEntities)
				oldNum = 99999;
			else {
				oldState = &cl_parseEntities[(oldFrame->parseEntities+oldIndex) & (MAX_PARSEENTITIES_MASK)];
				oldNum = oldState->number;
			}
			continue;
		}

		if (oldNum == newNum) {
			// Delta from previous state
			if (cl_shownet->value == 3)
				Com_Printf (0, "   delta: %i\n", newNum);
			CL_DeltaEntity (newFrame, newNum, oldState, bits);

			oldIndex++;

			if (oldIndex >= oldFrame->numEntities)
				oldNum = 99999;
			else {
				oldState = &cl_parseEntities[(oldFrame->parseEntities+oldIndex) & (MAX_PARSEENTITIES_MASK)];
				oldNum = oldState->number;
			}
			continue;
		}

		if (oldNum > newNum) {
			// Delta from baseline
			if (cl_shownet->value == 3)
				Com_Printf (0, "   baseline: %i\n", newNum);
			CL_DeltaEntity (newFrame, newNum, &cl_baseLines[newNum], bits);
			continue;
		}

	}

	// Any remaining entities in the old frame are copied over
	while (oldNum != 99999) {
		// One or more entities from the old packet are unchanged
		if (cl_shownet->value == 3)
			Com_Printf (0, "   unchanged: %i\n", oldNum);
		CL_DeltaEntity (newFrame, oldNum, oldState, 0);
		
		oldIndex++;

		if (oldIndex >= oldFrame->numEntities) {
			oldNum = 99999;
		}
		else {
			oldState = &cl_parseEntities[(oldFrame->parseEntities+oldIndex) & (MAX_PARSEENTITIES_MASK)];
			oldNum = oldState->number;
		}
	}

	CL_CGModule_EndFrameSequence ();
}


/*
================
CL_ParseFrame
================
*/
static void CL_ParseFrame (int extraBits)
{
	int		cmd, len;
	uint32	serverFrame;
	int		extraFlags;
	frame_t	*oldFrame;

	memset (&cl.frame, 0, sizeof (cl.frame));

	// Protocol updates
	serverFrame = MSG_ReadLong (&cls.netMessage);
	if (cls.serverProtocol != ENHANCED_PROTOCOL_VERSION) {
		cl.frame.serverFrame = serverFrame;
		cl.frame.deltaFrame = MSG_ReadLong (&cls.netMessage);
	}
	else {
		uint32	offset;

		offset = serverFrame & 0xF8000000;
		offset >>= 27;

		serverFrame &= 0x07FFFFFF;
		cl.frame.serverFrame = serverFrame;

		if (offset == 31)
			cl.frame.deltaFrame = -1;
		else
			cl.frame.deltaFrame = serverFrame - offset;
	}

	if (Com_ClientState () != CA_ACTIVE)
		cl.frame.serverTime = 0;
	else
		cl.frame.serverTime = cl.frame.serverFrame * 100;

	// HACK UGLY SHIT
	// moving the extrabits from cmd over so that the 4 that come from extraflags (surpressCount) don't conflict
	extraFlags = extraBits >> 1;

	// BIG HACK to let old demos continue to work
	if (cls.serverProtocol != 26) {
		byte		data;

		data = MSG_ReadByte (&cls.netMessage);

		//r1: HACK to get extra 4 bits of otherwise unused data
		if (cls.serverProtocol == ENHANCED_PROTOCOL_VERSION) {
			cl.surpressCount = (data & 0x0F);
			extraFlags |= (data & 0xF0) >> 4;
		}
		else {
			cl.surpressCount = data;
		}
	}

	if (cl_shownet->value == 3)
		Com_Printf (0, "   frame:%i  delta:%i\n", cl.frame.serverFrame, cl.frame.deltaFrame);

	// If the frame is delta compressed from data that we no longer have available, we must
	// suck up the rest of the frame, but not use it, then ask for a non-compressed message
	if (cl.frame.deltaFrame <= 0) {
		cl.frame.valid = qTrue;		// uncompressed frame
		cls.demoWaiting = qFalse;	// we can start recording now
		oldFrame = NULL;
	}
	else {
		oldFrame = &cl.frames[cl.frame.deltaFrame & UPDATE_MASK];
		if (!oldFrame->valid) {
			// Should never happen
			Com_Printf (PRNT_ERROR, "Delta from invalid frame (not supposed to happen!).\n");
		}
		if (oldFrame->serverFrame != cl.frame.deltaFrame) {
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly
			Com_DevPrintf (PRNT_WARNING, "Delta frame too old.\n");
		}
		else if (cl.parseEntities - oldFrame->parseEntities > MAX_PARSE_ENTITIES-128) {
			Com_DevPrintf (PRNT_WARNING, "Delta parseEntities too old.\n");
		}
		else
			cl.frame.valid = qTrue;	// Valid delta parse
	}

	// Read areaBits
	len = MSG_ReadByte (&cls.netMessage);
	MSG_ReadData (&cls.netMessage, &cl.frame.areaBits, len);

	// Read playerinfo
	if (cls.serverProtocol != ENHANCED_PROTOCOL_VERSION) {
		cmd = MSG_ReadByte (&cls.netMessage);
		SHOWNET (svcStrings[(cmd > SVC_MAX) ? 0 : cmd]);
		if (cmd != SVC_PLAYERINFO)
			Com_Error (ERR_DROP, "CL_ParseFrame: not playerinfo");
	}
	CL_ParsePlayerstate (oldFrame, &cl.frame, extraFlags);

	// Read packet entities
	if (cls.serverProtocol != ENHANCED_PROTOCOL_VERSION) {
		cmd = MSG_ReadByte (&cls.netMessage);
		SHOWNET (svcStrings[(cmd > SVC_MAX) ? 0 : cmd]);
		if (cmd != SVC_PACKETENTITIES)
			Com_Error (ERR_DROP, "CL_ParseFrame: not packetentities");
	}
	CL_ParsePacketEntities (oldFrame, &cl.frame);

	// Translate for demos
	if (cls.demoRecording && cls.serverProtocol != ORIGINAL_PROTOCOL_VERSION) {
		netMsg_t	fakeMsg;
		byte		fakeFrame[1300];

		// Start
		MSG_Init (&fakeMsg, fakeFrame, sizeof (fakeFrame));
		fakeMsg.allowOverflow = qTrue;

		// SVC_FRAME header
		MSG_WriteByte (&fakeMsg, SVC_FRAME);
		MSG_WriteLong (&fakeMsg, cl.frame.serverFrame);
		MSG_WriteLong (&fakeMsg, cl.frame.deltaFrame);
		MSG_WriteByte (&fakeMsg, cl.surpressCount);

		// areaBits
		MSG_WriteByte (&fakeMsg, len);
		MSG_Write (&fakeMsg, &cl.frame.areaBits, len);

		// Delta playerState
		CL_WriteDemoPlayerstate (oldFrame, &cl.frame, &fakeMsg);

		// Delta packet entities
		CL_WriteDemoPacketEntities (oldFrame, &cl.frame, &fakeMsg);

		if (!fakeMsg.overFlowed) {
			if (fakeMsg.curSize + cl.demoBuffer.curSize > cl.demoBuffer.maxSize)
				Com_DevPrintf (0, "Discarded a demo frame of %d bytes.\n", fakeMsg.curSize);
			else
				MSG_Write (&cl.demoBuffer, fakeFrame, fakeMsg.curSize);
		}
	}

	// Save the frame off in the backup array for later delta comparisons
	cl.frames[cl.frame.serverFrame & UPDATE_MASK] = cl.frame;

	if (cl.frame.valid) {
		// Getting a valid frame message ends the connection process
		if (Com_ClientState () != CA_ACTIVE)
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
static void CL_DownloadFileName (char *dest, int destLen, char *fileName)
{
	Q_snprintfz (dest, destLen, "%s/%s", FS_Gamedir(), fileName);
}


/*
===============
CL_CheckOrDownloadFile

Returns qTrue if the file exists, otherwise it attempts
to start a download from the server.
===============
*/
qBool CL_CheckOrDownloadFile (char *fileName)
{
	FILE	*fp;
	char	name[MAX_OSPATH];

	// Don't download if there's ".." in the path
	if (strstr (fileName, "..")) {
		Com_Printf (PRNT_WARNING, "Refusing to download a path with '..'\n");
		return qTrue;
	}

	// No need to redownload a file that already exists
	if (FS_LoadFile (fileName, NULL) != -1)
		return qTrue;

	// Copy a normalized version of the filename
	Com_NormalizePath (cls.download.name, sizeof (cls.download.name), fileName);

	/*
	** Download to a temp name, and only rename to the real name when done, so if interrupted
	** a runt file wont be left
	** copy, normalize the path, strip the extension, and replace with ".tmp"
	*/
	Com_StripExtension (cls.download.tempName, sizeof (cls.download.tempName), cls.download.name);
	Q_strcatz (cls.download.tempName, ".tmp", sizeof (cls.download.tempName));

	/*
	** ZOID
	** Check to see if we already have a tmp for this file, if so, try to resume
	** open the file if not opened yet
	*/
	CL_DownloadFileName (name, sizeof (name), cls.download.tempName);

	fp = fopen (name, "r+b");
	if (fp) {
		// It exists
		int		len;

		fseek (fp, 0, SEEK_END);
		len = ftell (fp);

		cls.download.file = fp;

		// Give the server an offset to start the download
		Com_Printf (0, "Resuming %s\n", cls.download.name);
		MSG_WriteByte (&cls.netChan.message, CLC_STRINGCMD);
		if (cls.serverProtocol == ENHANCED_PROTOCOL_VERSION)
			MSG_WriteString (&cls.netChan.message, Q_VarArgs ("download \"%s\" %i udp-zlib", cls.download.name, len));
		else
			MSG_WriteString (&cls.netChan.message, Q_VarArgs ("download \"%s\" %i", cls.download.name, len));
	}
	else {
		Com_Printf (0, "Downloading %s\n", cls.download.name);
		MSG_WriteByte (&cls.netChan.message, CLC_STRINGCMD);
		if (cls.serverProtocol == ENHANCED_PROTOCOL_VERSION)
			MSG_WriteString (&cls.netChan.message, Q_VarArgs ("download \"%s\" 0 udp-zlib", cls.download.name));
		else
			MSG_WriteString (&cls.netChan.message, Q_VarArgs ("download \"%s\"", cls.download.name));
	}

	cls.forcePacket = qTrue;

	return qFalse;
}


/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
static void CL_ParseDownload (qBool compressed)
{
	int		size, percent;
	char	name[MAX_OSPATH];

	// Read the data
	size = MSG_ReadShort (&cls.netMessage);
	percent = MSG_ReadByte (&cls.netMessage);
	if (size < 0) {
		if (size == -1)
			Com_Printf (PRNT_WARNING, "Server does not have this file.\n");
		else
			Com_Printf (PRNT_ERROR, "Bad download data from server.\n");

		if (cls.download.file) {
			// If here, we tried to resume a file but the server said no
			fclose (cls.download.file);
			cls.download.file = NULL;
		}
		CL_RequestNextDownload ();
		return;
	}

	// Open the file if not opened yet
	if (!cls.download.file) {
		CL_DownloadFileName (name, sizeof (name), cls.download.tempName);

		FS_CreatePath (name);

		cls.download.file = fopen (name, "wb");
		if (!cls.download.file) {
			cls.netMessage.readCount += size;
			Com_Printf (PRNT_WARNING, "Failed to open %s\n", cls.download.tempName);
			CL_RequestNextDownload ();
			return;
		}
	}

	// Insert block
 	if (compressed) {
 		uint16	uncompressedLen;
 		byte	uncompressed[0xFFFF];

 		uncompressedLen = MSG_ReadShort (&cls.netMessage);
 		if (!uncompressedLen)
			Com_Error (ERR_DROP, "CL_ParseDownload: uncompressedLen == 0");

 		FS_ZLibDecompress (cls.netMessage.data + cls.netMessage.readCount, size, uncompressed, uncompressedLen, -15);
 		fwrite (uncompressed, 1, uncompressedLen, cls.download.file);
 		Com_DevPrintf (0, "SVC_ZDOWNLOAD(%s): %d -> %d\n", cls.download.name, size, uncompressedLen);
 	}
 	else {
		fwrite (cls.netMessage.data + cls.netMessage.readCount, 1, size, cls.download.file);
	}

	cls.netMessage.readCount += size;

	if (percent != 100) {
		// Request next block
		cls.download.percent = percent;

		MSG_WriteByte (&cls.netChan.message, CLC_STRINGCMD);
		MSG_Print (&cls.netChan.message, "nextdl");
		cls.forcePacket = qTrue;
	}
	else {
		char	oldName[MAX_OSPATH];
		char	newName[MAX_OSPATH];
		int		rn;

		fclose (cls.download.file);

		// Rename the temp file to it's final name
		CL_DownloadFileName (oldName, sizeof (oldName), cls.download.tempName);
		CL_DownloadFileName (newName, sizeof (newName), cls.download.name);
		rn = rename (oldName, newName);
		if (rn)
			Com_Printf (PRNT_ERROR, "Failed to rename!\n");
		else
			Com_Printf (0, "Download of %s completed\n", newName);

		cls.download.file = NULL;
		cls.download.percent = 0;

		// Get another file if needed
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
void CL_Reconnect_f (void); // fixme orly
static qBool CL_ParseServerData (void)
{
	extern	cVar_t *fs_game;
	int		i, newVersion;
	char	*str;

	Com_DevPrintf (0, "Serverdata packet received.\n");

	// Wipe the clientState_t struct
	CL_ClearState ();
	CL_SetState (CA_CONNECTED);

	// Parse protocol version number
	i = MSG_ReadLong (&cls.netMessage);
	cls.serverProtocol = i;

	// BIG HACK to let demos from release work with the 3.0x patch!!!
	if (i != ORIGINAL_PROTOCOL_VERSION && i != ENHANCED_PROTOCOL_VERSION && i != 26 && !cl.attractLoop)
		Com_Error (ERR_DROP, "You are running protocol version %i, server is running %i. These are incompatible, please update your client to protocol version %d", ENHANCED_PROTOCOL_VERSION, i, i);

	cl.serverCount = MSG_ReadLong (&cls.netMessage);
	cl.attractLoop = MSG_ReadByte (&cls.netMessage);

	// Game directory
	str = MSG_ReadString (&cls.netMessage);
	Q_strncpyz (cl.gameDir, str, sizeof (cl.gameDir));

	// Set gamedir
	if ((*str && (!fs_game->string || !*fs_game->string || strcmp (fs_game->string, str))) || (!*str && (fs_game->string || *fs_game->string)))
		Cvar_Set ("game", str, qFalse);

	// Parse player entity number
	cl.playerNum = MSG_ReadShort (&cls.netMessage);

	// Get the full level name
	str = MSG_ReadString (&cls.netMessage);

	// Check the protocol version
	if (cls.serverProtocol == ENHANCED_PROTOCOL_VERSION) {
		cl.enhancedServer = MSG_ReadByte (&cls.netMessage);
		newVersion = MSG_ReadShort (&cls.netMessage);
		if (newVersion != ENHANCED_COMPATIBILITY_NUMBER) {
			Com_Printf (0, "Protocol 35 version mismatch, falling back to 34.\n");
			CL_Disconnect (qFalse);

			cls.serverProtocol = ORIGINAL_PROTOCOL_VERSION;
			CL_Reconnect_f ();
			return qFalse;
		}

		if (newVersion >= 1903) {
			/*cl.advancedDeltas = */MSG_ReadByte (&cls.netMessage);
			cl.strafeHack = MSG_ReadByte (&cls.netMessage);
		}
		else
			cl.strafeHack = qFalse;
	}
	else {
		cl.enhancedServer = 0;
		cl.strafeHack = qFalse;
	}

	Com_DevPrintf (0, "Serverdata packet received. protocol=%d, serverCount=%d, attractLoop=%d, playerNum=%d, game=%s, map=%s, enhanced=%d\n",
		cls.serverProtocol, cl.serverCount, cl.attractLoop, cl.playerNum, cl.gameDir, str, cl.enhancedServer);

	CL_MediaRestart ();

	if (cl.playerNum == -1) {
		// Playing a cinematic or showing a pic, not a level
		CIN_PlayCinematic (str);
	}
	else {
		// Seperate the printfs so the server message can have a color
		Com_Printf (0, "\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
		Com_Printf (0, "%c%s\n", 2, str);

		// Need to prep refresh at next oportunity
		cls.refreshPrepped = qFalse;
	}

	return qTrue;
}


/*
==================
CL_ParseBaseline
==================
*/
static void CL_ParseBaseline (void)
{
	entityState_t	*es;
	entityState_t	nullState;
	uint32			bits;
	int				newNum;

	memset (&nullState, 0, sizeof (nullState));

	newNum = CL_ParseEntityBits (&bits);
	es = &cl_baseLines[newNum];
	CL_ParseDelta (&nullState, es, newNum, bits);
}


/*
================
CL_ParseConfigString
================
*/
static void CL_ParseConfigString (void)
{
	int		num;
	char	*str;

	num = MSG_ReadShort (&cls.netMessage);
	if (num < 0 || num >= MAX_CFGSTRINGS)
		Com_Error (ERR_DROP, "CL_ParseConfigString: bad num");
	str = MSG_ReadString (&cls.netMessage);

	strcpy (cl.configStrings[num], str);

	// We do need to know some of these here...
	switch (num) {
	case CS_CDTRACK:
		CDAudio_Play (atoi (cl.configStrings[CS_CDTRACK]), qTrue);
		break;

	case CS_MAXCLIENTS:
		cl.maxClients = atoi (cl.configStrings[CS_MAXCLIENTS]);
		break;

	default:
		if (num >= CS_SOUNDS && num < CS_SOUNDS+MAX_CS_SOUNDS)
			cl.soundCfgStrings[num-CS_SOUNDS] = Snd_RegisterSound (cl.configStrings[num]);
		break;
	}

	// Let CGame know
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
static void CL_ParseStartSoundPacket (void)
{
	vec3_t			pos_vec;
	int				entNum, soundNum, flags;
	float			*pos, volume, attenuation, timeOffset;
	entChannel_t	entChannel;

	flags = MSG_ReadByte (&cls.netMessage);
	soundNum = MSG_ReadByte (&cls.netMessage);

	// Volume
	if (flags & SND_VOLUME)
		volume = MSG_ReadByte (&cls.netMessage) * (1.0f/255.0f);
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME;

	// Attenuation
	if (flags & SND_ATTENUATION)
		attenuation = MSG_ReadByte (&cls.netMessage) * (1.0f/64.0f);
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;

	// Offset
	if (flags & SND_OFFSET)
		timeOffset = MSG_ReadByte (&cls.netMessage) * (1.0f/1000.0f);
	else
		timeOffset = 0;

	// Entity sound
	if (flags & SND_ENT) {
		// Entity reletive
		entChannel = MSG_ReadShort (&cls.netMessage); 
		entNum = entChannel >> 3;
		if (entNum > MAX_CS_EDICTS)
			Com_Error (ERR_DROP, "CL_ParseStartSoundPacket: entNum = %i", entNum);

		entChannel &= 7;
	}
	else {
		entNum = 0;
		entChannel = 0;
	}

	// Specified position
	if (flags & SND_POS) {
		// Positioned in space
		MSG_ReadPos (&cls.netMessage, pos_vec);
 
		pos = pos_vec;
	}
	else {
		// Use entity number
		pos = NULL;
	}

	// Play the sound
	if (!cl.soundCfgStrings[soundNum] && cl.configStrings[CS_SOUNDS+soundNum][0])
		cl.soundCfgStrings[soundNum] = Snd_RegisterSound (cl.configStrings[CS_SOUNDS+soundNum]);

	CL_CGModule_StartSound (pos, entNum, entChannel, soundNum, volume, attenuation, timeOffset);
}


/*
=====================
CL_ParseZPacket
=====================
*/
void CL_ParseZPacket (void)
{
	byte		*buff_in;
	byte		*buff_out;
	netMsg_t	sb, old;
	int16		compressedLen;
	int16		uncompressedLen;

	compressedLen = MSG_ReadShort (&cls.netMessage);
	uncompressedLen = MSG_ReadShort (&cls.netMessage);

	if (cls.serverProtocol != ENHANCED_PROTOCOL_VERSION)
		Com_Error (ERR_DROP, "CL_ParseZPacket: SVC_ZPACKET -requires- ENHANCED_PROTOCOL_VERSION");

	if (uncompressedLen <= 0)
		Com_Error (ERR_DROP, "CL_ParseZPacket: uncompressedLen <= 0");
	if (compressedLen <= 0)
		Com_Error (ERR_DROP, "CL_ParseZPacket: compressedLen <= 0");

	buff_in = Mem_Alloc (compressedLen);
	buff_out = Mem_Alloc (uncompressedLen);

	MSG_ReadData (&cls.netMessage, buff_in, compressedLen);

	MSG_Init (&sb, buff_out, uncompressedLen);
	sb.curSize = FS_ZLibDecompress (buff_in, compressedLen, buff_out, uncompressedLen, -15);

	old = cls.netMessage;
	cls.netMessage = sb;

	CL_ParseServerMessage ();

	cls.netMessage = old;

	Mem_Free (buff_in);
	Mem_Free (buff_out);

	Com_DevPrintf (0, "Got a ZPacket, %d->%d\n", uncompressedLen + 4, compressedLen);
}


/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage (void)
{
	int			cmd, i;
	char		*s, timestamp[10];
	time_t		ctime;
	struct tm	*ltime;
	static int	queryLastTime = 0;
	static int	lastCmd = -2;
	int			extraBits;
	int			oldReadCount;

	if (cl_timestamp->integer) {
		ctime = time (NULL);
		ltime = localtime (&ctime);
		strftime (timestamp, sizeof (timestamp), "%I:%M %p", ltime);

		if (timestamp[0] == '0') {
			for (i=0 ; i<(int)(strlen(timestamp) + 1) ; i++)
				timestamp[i] = timestamp[i+1];
			timestamp[i+1] = '\0';
		}
	}

	// If recording demos, copy the message out
	if (cl_shownet->value == 1)
		Com_Printf (0, "%i ", cls.netMessage.curSize);
	else if (cl_shownet->value >= 2)
		Com_Printf (0, "------------------\n");

	CL_CGModule_StartServerMessage ();

	// Parse the message
	for ( ; ; ) {
		if (cls.netMessage.readCount > cls.netMessage.curSize) {
			Com_Error (ERR_DROP, "CL_ParseServerMessage: Bad server message");
			break;
		}

		oldReadCount = cls.netMessage.readCount;
		cmd = MSG_ReadByte (&cls.netMessage);
		if (cmd == -1) {
			SHOWNET ("END OF MESSAGE");
			break;
		}

		//r1: more hacky bit stealing in the name of bandwidth
		extraBits = cmd & 0xE0;
		cmd &= 0x1F;

		if (cl_shownet->value >= 2) {
			if (!svcStrings[(cmd > SVC_MAX) ? 0 : cmd])
				Com_Printf (PRNT_WARNING, "%3i:BAD CMD %i\n", cls.netMessage.readCount-1, cmd);
			else
				SHOWNET (svcStrings[(cmd > SVC_MAX) ? 0 : cmd]);
		}

		//
		// These are private to the client and server
		//
		switch (cmd) {
		case SVC_NOP:
			break;

		case SVC_DISCONNECT:
			cls.connectCount = 0;
			CL_WriteDemoMessageChunk (cls.netMessage.data + oldReadCount, cls.netMessage.readCount - oldReadCount, qFalse);
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
			CL_WriteDemoMessageChunk (cls.netMessage.data + oldReadCount, cls.netMessage.readCount - oldReadCount, qFalse);
			break;

		case SVC_PRINT:
			i = MSG_ReadByte (&cls.netMessage);
			s = MSG_ReadString (&cls.netMessage);
			CL_WriteDemoMessageChunk (cls.netMessage.data + oldReadCount, cls.netMessage.readCount - oldReadCount, qFalse);

			switch (i) {
			case PRINT_CHAT:
				Snd_StartLocalSound (clMedia.talkSfx, 1);

				if (cl_timestamp->integer)
					Com_Printf (PRNT_CHATHUD, "[%s] %s", timestamp, s);
				else
					Com_Printf (PRNT_CHATHUD, "%s", s);

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
			s = MSG_ReadString (&cls.netMessage);
			Com_DevPrintf (0, "stufftext: %s\n", s);
			if (!cl.attractLoop || !strcmp (s, "precache\n"))
				Cbuf_AddText (s);
			else
				Com_DevPrintf (PRNT_WARNING, "WARNING: Demo triedto execute command '%s', ignored.\n", s);
			break;

		case SVC_SERVERDATA:
			Cbuf_Execute ();	// Make sure any stuffed commands are done
			if (!CL_ParseServerData ())
				return;
			CL_WriteDemoMessageChunk (cls.netMessage.data + oldReadCount, cls.netMessage.readCount - oldReadCount, qFalse);
			break;

		case SVC_CONFIGSTRING:
			CL_ParseConfigString ();
			CL_WriteDemoMessageChunk (cls.netMessage.data + oldReadCount, cls.netMessage.readCount - oldReadCount, qFalse);
			break;

		case SVC_SPAWNBASELINE:
			CL_ParseBaseline ();
			CL_WriteDemoMessageChunk (cls.netMessage.data + oldReadCount, cls.netMessage.readCount - oldReadCount, qFalse);
			break;

		case SVC_DOWNLOAD:
			CL_ParseDownload (qFalse);
			break;

		case SVC_PLAYERINFO:
		case SVC_PACKETENTITIES:
		case SVC_DELTAPACKETENTITIES:
			assert (0);
			Com_Error (ERR_DROP, "CL_ParseServerMessage: Out of place frame data");
			break;

		case SVC_FRAME:
			CL_ParseFrame (extraBits);
			break;

		case SVC_ZPACKET:
			CL_ParseZPacket ();
			break;

		case SVC_ZDOWNLOAD:
			CL_ParseDownload (qTrue);
			break;

		default:
			if (CL_CGModule_ParseServerMessage (cmd))
				break;

			// Unknown to the client and CGame failed to parse it so...
			assert (0);
			Com_Error (ERR_DROP, "CL_ParseServerMessage: Illegible server message\n"
				"Message #%i: %s\n"
				"Last #%i: %s\n"
				"Try 'set cl_protocol 34' before connecting, a protocol change could be causing problems",
				cmd, svcStrings[(cmd > SVC_MAX) ? 0 : cmd],
				lastCmd, (lastCmd == -2) ? "None" : svcStrings[(lastCmd > SVC_MAX) ? 0 : lastCmd]);
			break;
		}

		lastCmd = cmd;
	}

	// Flush this frame
	CL_WriteDemoMessageChunk (NULL, 0, qTrue);

	// Let CGame know
	CL_CGModule_EndServerMessage ();
}
