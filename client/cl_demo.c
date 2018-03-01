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
// cl_demo.c
//

#include "cl_local.h"

/*
================
CL_WriteDemoPlayerstate

Delta compresses between 'from' and 'to' then writes to 'msg' in standard
ORIGINAL_PROTOCOL_VERSION form. This is basically translating any other protocols
to the target buffer, for things like demos.
================
*/
void CL_WriteDemoPlayerstate (frame_t *from, frame_t *to, netMsg_t *msg)
{
	playerStateNew_t	*ps, *ops;
	playerStateNew_t	dummy;
	int					i, psFlags;
	int					statBits;

	ps = &to->playerState;
	if (!from)
		ops = &dummy;
	else
		ops = &from->playerState;

	ps = &to->playerState;
	if (!from) {
		memset (&dummy, 0, sizeof (dummy));
		ops = &dummy;
	}
	else
		ops = &from->playerState;

	// Determine what needs to be sent
	psFlags = 0;
	if (ps->pMove.pmType != ops->pMove.pmType)
		psFlags |= PS_M_TYPE;

	if (ps->pMove.origin[0] != ops->pMove.origin[0]
	|| ps->pMove.origin[1] != ops->pMove.origin[1]
	|| ps->pMove.origin[2] != ops->pMove.origin[2])
		psFlags |= PS_M_ORIGIN;

	if (ps->pMove.velocity[0] != ops->pMove.velocity[0]
	|| ps->pMove.velocity[1] != ops->pMove.velocity[1]
	|| ps->pMove.velocity[2] != ops->pMove.velocity[2])
		psFlags |= PS_M_VELOCITY;

	if (ps->pMove.pmTime != ops->pMove.pmTime)
		psFlags |= PS_M_TIME;

	if (ps->pMove.pmFlags != ops->pMove.pmFlags)
		psFlags |= PS_M_FLAGS;

	if (ps->pMove.gravity != ops->pMove.gravity)
		psFlags |= PS_M_GRAVITY;

	if (ps->pMove.deltaAngles[0] != ops->pMove.deltaAngles[0]
	|| ps->pMove.deltaAngles[1] != ops->pMove.deltaAngles[1]
	|| ps->pMove.deltaAngles[2] != ops->pMove.deltaAngles[2])
		psFlags |= PS_M_DELTA_ANGLES;


	if (ps->viewOffset[0] != ops->viewOffset[0]
	|| ps->viewOffset[1] != ops->viewOffset[1]
	|| ps->viewOffset[2] != ops->viewOffset[2])
		psFlags |= PS_VIEWOFFSET;

	if (ps->viewAngles[0] != ops->viewAngles[0]
	|| ps->viewAngles[1] != ops->viewAngles[1]
	|| ps->viewAngles[2] != ops->viewAngles[2])
		psFlags |= PS_VIEWANGLES;

	if (ps->kickAngles[0] != ops->kickAngles[0]
	|| ps->kickAngles[1] != ops->kickAngles[1]
	|| ps->kickAngles[2] != ops->kickAngles[2])
		psFlags |= PS_KICKANGLES;

	if (ps->viewBlend[0] != ops->viewBlend[0]
	|| ps->viewBlend[1] != ops->viewBlend[1]
	|| ps->viewBlend[2] != ops->viewBlend[2]
	|| ps->viewBlend[3] != ops->viewBlend[3])
		psFlags |= PS_BLEND;

	if (ps->fov != ops->fov)
		psFlags |= PS_FOV;

	if (ps->rdFlags != ops->rdFlags)
		psFlags |= PS_RDFLAGS;

	if (ps->gunFrame != ops->gunFrame)
		psFlags |= PS_WEAPONFRAME;

	psFlags |= PS_WEAPONINDEX;

	// Write it
	MSG_WriteByte (msg, SVC_PLAYERINFO);
	MSG_WriteShort (msg, psFlags);

	// Write the pMoveState_t
	if (psFlags & PS_M_TYPE)
		MSG_WriteByte (msg, ps->pMove.pmType);

	if (psFlags & PS_M_ORIGIN) {
		MSG_WriteShort (msg, ps->pMove.origin[0]);
		MSG_WriteShort (msg, ps->pMove.origin[1]);
		MSG_WriteShort (msg, ps->pMove.origin[2]);
	}

	if (psFlags & PS_M_VELOCITY) {
		MSG_WriteShort (msg, ps->pMove.velocity[0]);
		MSG_WriteShort (msg, ps->pMove.velocity[1]);
		MSG_WriteShort (msg, ps->pMove.velocity[2]);
	}

	if (psFlags & PS_M_TIME)
		MSG_WriteByte (msg, ps->pMove.pmTime);

	if (psFlags & PS_M_FLAGS)
		MSG_WriteByte (msg, ps->pMove.pmFlags);

	if (psFlags & PS_M_GRAVITY)
		MSG_WriteShort (msg, ps->pMove.gravity);

	if (psFlags & PS_M_DELTA_ANGLES) {
		MSG_WriteShort (msg, ps->pMove.deltaAngles[0]);
		MSG_WriteShort (msg, ps->pMove.deltaAngles[1]);
		MSG_WriteShort (msg, ps->pMove.deltaAngles[2]);
	}

	// Write the rest of the playerState_t
	if (psFlags & PS_VIEWOFFSET) {
		MSG_WriteChar (msg, ps->viewOffset[0]*4);
		MSG_WriteChar (msg, ps->viewOffset[1]*4);
		MSG_WriteChar (msg, ps->viewOffset[2]*4);
	}

	if (psFlags & PS_VIEWANGLES) {
		MSG_WriteAngle16 (msg, ps->viewAngles[0]);
		MSG_WriteAngle16 (msg, ps->viewAngles[1]);
		MSG_WriteAngle16 (msg, ps->viewAngles[2]);
	}

	if (psFlags & PS_KICKANGLES) {
		MSG_WriteChar (msg, ps->kickAngles[0]*4);
		MSG_WriteChar (msg, ps->kickAngles[1]*4);
		MSG_WriteChar (msg, ps->kickAngles[2]*4);
	}

	if (psFlags & PS_WEAPONINDEX) {
		MSG_WriteByte (msg, ps->gunIndex);
	}

	if (psFlags & PS_WEAPONFRAME) {
		MSG_WriteByte (msg, ps->gunFrame);
		MSG_WriteChar (msg, ps->gunOffset[0]*4);
		MSG_WriteChar (msg, ps->gunOffset[1]*4);
		MSG_WriteChar (msg, ps->gunOffset[2]*4);
		MSG_WriteChar (msg, ps->gunAngles[0]*4);
		MSG_WriteChar (msg, ps->gunAngles[1]*4);
		MSG_WriteChar (msg, ps->gunAngles[2]*4);
	}

	if (psFlags & PS_BLEND) {
		// R1: clamp the color
		if (ps->viewBlend[1] > 1)
			ps->viewBlend[1] = 1;
		if (ps->viewBlend[2] > 1)
			ps->viewBlend[2] = 1;
		if (ps->viewBlend[3] > 1)
			ps->viewBlend[3] = 1;

		MSG_WriteByte (msg, ps->viewBlend[0]*255);
		MSG_WriteByte (msg, ps->viewBlend[1]*255);
		MSG_WriteByte (msg, ps->viewBlend[2]*255);
		MSG_WriteByte (msg, ps->viewBlend[3]*255);
	}

	if (psFlags & PS_FOV)
		MSG_WriteByte (msg, ps->fov);

	if (psFlags & PS_RDFLAGS)
		MSG_WriteByte (msg, ps->rdFlags);

	// Send stats
	statBits = 0;
	for (i=0 ; i<MAX_STATS ; i++)
		if (ps->stats[i] != ops->stats[i])
			statBits |= 1<<i;
	MSG_WriteLong (msg, statBits);
	for (i=0 ; i<MAX_STATS ; i++)
		if (statBits & (1<<i))
			MSG_WriteShort (msg, ps->stats[i]);
}


/*
================
CL_WriteDemoPacketEntities

Basically for demo translation, just like CL_WriteDemoPlayerstate above.
================
*/
void CL_WriteDemoPacketEntities (const frame_t *from, frame_t *to, netMsg_t *msg)
{
	entityStateOld_t	*oldEnt, *newEnt;
	int		oldIndex, newIndex;
	int		oldNum, newNum;
	int		from_numEntities;

	MSG_WriteByte (msg, SVC_PACKETENTITIES);

	if (!from)
		from_numEntities = 0;
	else
		from_numEntities = from->numEntities;

	newIndex = 0;
	newEnt = NULL;
	oldIndex = 0;
	oldEnt = NULL;
	while (newIndex < to->numEntities || oldIndex < from_numEntities) {
		if (newIndex >= to->numEntities) {
			newNum = 9999;
		}
		else {
			newEnt =(entityStateOld_t *)&cl_parseEntities[(to->parseEntities+newIndex)&(MAX_PARSE_ENTITIES-1)];
			newNum = newEnt->number;
		}

		if (oldIndex >= from_numEntities) {
			oldNum = 9999;
		}
		else {
			oldEnt = (entityStateOld_t *)&cl_parseEntities[(from->parseEntities+oldIndex)&(MAX_PARSE_ENTITIES-1)];
			oldNum = oldEnt->number;
		}

		if (newNum == oldNum) {
			/*
			** delta update from old position
			** because the force parm is qFalse, this will not result
			** in any bytes being emited if the entity has not changed at all
			** note that players are always 'newentities', this updates their oldorigin always
			** and prevents warping
			*/
			MSG_WriteDeltaEntity (msg, oldEnt, newEnt, qFalse, newEnt->number <= cl.maxClients);
			oldIndex++;
			newIndex++;
			continue;
		}

		if (newNum < oldNum) {
			// This is a new entity, send it from the baseline
			MSG_WriteDeltaEntity (msg, (entityStateOld_t *)&cl_baseLines[newNum], newEnt, qTrue, qTrue);
			newIndex++;
			continue;
		}

		if (newNum > oldNum) {
			// This old entity isn't present in the new message
			MSG_WriteDeltaEntity (msg, oldEnt, NULL, qTrue, qFalse);
			oldIndex++;
			continue;
		}
	}

	// End of packetentities
	MSG_WriteShort (msg, 0);
}


/*
====================
CL_WriteDemoMessageChunk

This is only used for ENHANCED_PROTOCOL_VERSION
====================
*/
void CL_WriteDemoMessageChunk (byte *buffer, size_t length, qBool forceFlush)
{
	if (!cls.demoRecording || cls.serverProtocol == ORIGINAL_PROTOCOL_VERSION)
		return;

	if (forceFlush) {
		if (!cls.demoWaiting) {
			int	swLen;

			if (cl.demoBuffer.overFlowed) {
				Com_DevPrintf (0, "Dropped demo frame, maximum message size exceeded: %i > %i\n", cl.demoBuffer.curSize, cl.demoBuffer.maxSize);

				// Write a message regardless, to keep sync
				MSG_Clear (&cl.demoBuffer);
				MSG_WriteByte (&cl.demoBuffer, SVC_NOP);
			}

			swLen = LittleLong ((int) cl.demoBuffer.curSize);
			FS_Write (&swLen, sizeof (swLen), cls.demoFile);
			FS_Write (cl.demoFrame, cl.demoBuffer.curSize, cls.demoFile);
		}
		MSG_Clear (&cl.demoBuffer);
	}

	if (length)
		MSG_WriteRaw (&cl.demoBuffer, buffer, length);
}


/*
====================
CL_WriteDemoMessageFull

Dumps the current net message, prefixed by the length
This is only used for ORIGINAL_PROTOCOL_VERSION
====================
*/
void CL_WriteDemoMessageFull (void)
{
	int		len, swLen;

	// The first eight bytes are just packet sequencing stuff
	len = (int) cls.netMessage.curSize - 8;
	swLen = LittleLong (len);
	if (swLen) {
		FS_Write (&swLen, sizeof (swLen), cls.demoFile);
		FS_Write (cls.netMessage.data+8, len, cls.demoFile);
	}
}


/*
====================
CL_StartDemoRecording
====================
*/
qBool CL_StartDemoRecording (char *name)
{
	byte				buf_data[MAX_SV_USABLEMSG];
	netMsg_t			buf;
	int					i, len;
	entityStateOld_t	*ent, temp;
	entityStateOld_t	nullstate;

	// Open the demo file
	FS_CreatePath (name);
	FS_OpenFile (name, &cls.demoFile, FS_MODE_WRITE_BINARY);
	if (!cls.demoFile) {
		return qFalse;
	}

	cls.demoRecording = qTrue;

	if (cls.serverProtocol == ENHANCED_PROTOCOL_VERSION) {
		MSG_WriteByte (&cls.netChan.message, CLC_SETTING);
		MSG_WriteShort (&cls.netChan.message, CLSET_RECORDING);
		MSG_WriteShort (&cls.netChan.message, 1);
	}

	// Don't start saving messages until a non-delta compressed message is received
	cls.demoWaiting = qTrue;

	// Write out messages to hold the startup information
	MSG_Init (&buf, buf_data, sizeof (buf_data));

	// Send the serverdata
	MSG_WriteByte (&buf, SVC_SERVERDATA);
	MSG_WriteLong (&buf, ORIGINAL_PROTOCOL_VERSION);
	MSG_WriteLong (&buf, 0x10000 + cl.serverCount);
	MSG_WriteByte (&buf, 1);	// demos are always attract loops
	MSG_WriteString (&buf, cl.gameDir);
	MSG_WriteShort (&buf, cl.playerNum);
	MSG_WriteString (&buf, cl.configStrings[CS_NAME]);

	// Configstrings
	for (i=0 ; i<MAX_CFGSTRINGS ; i++) {
		if (cl.configStrings[i][0]) {
			if (buf.curSize + (int)strlen (cl.configStrings[i]) + 32 > buf.maxSize) {
				// write it out
				len = LittleLong ((int) buf.curSize);
				FS_Write (&len, sizeof (len), cls.demoFile);
				FS_Write (buf.data, buf.curSize, cls.demoFile);
				buf.curSize = 0;
			}

			MSG_WriteByte (&buf, SVC_CONFIGSTRING);
			MSG_WriteShort (&buf, i);
			MSG_WriteString (&buf, cl.configStrings[i]);
		}

	}

	// Baselines
	memset (&nullstate, 0, sizeof (nullstate));
	for (i=0; i<MAX_CS_EDICTS ; i++) {
		memcpy (&temp, &cl_baseLines[i], sizeof (entityStateOld_t));
		ent = &temp;
		if (!ent->modelIndex)
			continue;

		if (buf.curSize + 64 > buf.maxSize) {
			// Write it out
			len = LittleLong ((int) buf.curSize);
			FS_Write (&len, sizeof (len), cls.demoFile);
			FS_Write (buf.data, buf.curSize, cls.demoFile);
			buf.curSize = 0;
		}

		MSG_WriteByte (&buf, SVC_SPAWNBASELINE);		
		MSG_WriteDeltaEntity (&buf, &nullstate, ent, qTrue, qTrue);
	}

	MSG_WriteByte (&buf, SVC_STUFFTEXT);
	MSG_WriteString (&buf, "precache\n");

	// Write it to the demo file
	len = LittleLong ((int) buf.curSize);
	FS_Write (&len, sizeof (len), cls.demoFile);
	FS_Write (buf.data, buf.curSize, cls.demoFile);

	// The rest of the demo file will be individual frames
	return qTrue;
}


/*
====================
CL_StopDemoRecording
====================
*/
void CL_StopDemoRecording (void)
{
	int		len;

	// Write to file
	len = -1;
	FS_Write (&len, sizeof (len), cls.demoFile);
	FS_CloseFile (cls.demoFile);

	if (cls.serverProtocol == ENHANCED_PROTOCOL_VERSION) {
		MSG_WriteByte (&cls.netChan.message, CLC_SETTING);
		MSG_WriteShort (&cls.netChan.message, CLSET_RECORDING);
		MSG_WriteShort (&cls.netChan.message, 0);
	}

	// Finish up
	cls.demoFile = 0;
	cls.demoRecording = qFalse;
}
