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

// msg.c

#include "common.h"

/*
==============================================================================

	SUPPORTING FUNCTIONS

==============================================================================
*/

/*
================
MSG_Init
================
*/
void MSG_Init (netMsg_t *buf, byte *data, int length) {
	memset (buf, 0, sizeof (*buf));
	buf->data = data;
	buf->maxSize = length;
}


/*
================
MSG_Clear
================
*/
void MSG_Clear (netMsg_t *buf) {
	buf->curSize = 0;
	buf->overFlowed = qFalse;
}


/*
================
MSG_GetSpace
================
*/
void *MSG_GetSpace (netMsg_t *buf, int length) {
	void	*data;
	
	if (buf->curSize + length > buf->maxSize) {
		if (!buf->allowOverflow)
			Com_Error (ERR_FATAL, "MSG_GetSpace: overflow without allowOverflow set");
		
		if (length > buf->maxSize)
			Com_Error (ERR_FATAL, "MSG_GetSpace: %i is > full buffer size", length);
			
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "MSG_GetSpace: overflow\n");
		MSG_Clear (buf); 
		buf->overFlowed = qTrue;
	}

	data = buf->data + buf->curSize;
	buf->curSize += length;
	
	return data;
}


/*
================
MSG_Write
================
*/
void MSG_Write (netMsg_t *buf, void *data, int length) {
	memcpy (MSG_GetSpace (buf,length),data,length);		
}


/*
================
MSG_Print
================
*/
void MSG_Print (netMsg_t *buf, char *data) {
	int		len;
	
	len = strlen(data)+1;

	if (buf->curSize) {
		if (buf->data[buf->curSize-1])
			memcpy ((byte *)MSG_GetSpace (buf, len),data,len); // no trailing 0
		else
			memcpy ((byte *)MSG_GetSpace (buf, len-1)-1,data,len); // write over trailing 0
	} else
		memcpy ((byte *)MSG_GetSpace (buf, len),data,len);
}

/*
==============================================================================

	WRITING FUNCTIONS

==============================================================================
*/

/*
================
MSG_WriteChar
================
*/
void MSG_WriteChar (netMsg_t *sb, int c) {
	byte	*buf;
	
#ifdef PARANOID
	if ((c < -128) || (c > 127))
		Com_Error (ERR_FATAL, "MSG_WriteChar: range error");
#endif

	buf = MSG_GetSpace (sb, 1);
	buf[0] = c;
}


/*
================
MSG_WriteByte
================
*/
void MSG_WriteByte (netMsg_t *sb, int c) {
	byte	*buf;
	
#ifdef PARANOID
	if ((c < 0) || (c > 255))
		Com_Error (ERR_FATAL, "MSG_WriteByte: range error");
#endif

	buf = MSG_GetSpace (sb, 1);
	buf[0] = c;
}


/*
================
MSG_WriteShort
================
*/
void MSG_WriteShort (netMsg_t *sb, int c) {
	byte	*buf;
	
#ifdef PARANOID
	if ((c < ((short)0x8000)) || (c > (short)0x7fff))
		Com_Error (ERR_FATAL, "MSG_WriteShort: range error");
#endif

	buf = MSG_GetSpace (sb, 2);
	buf[0] = c&0xff;
	buf[1] = c>>8;
}


/*
================
MSG_WriteLong
================
*/
void MSG_WriteLong (netMsg_t *sb, int c) {
	byte	*buf;
	
	buf = MSG_GetSpace (sb, 4);
	buf[0] = c&0xff;
	buf[1] = (c>>8)&0xff;
	buf[2] = (c>>16)&0xff;
	buf[3] = c>>24;
}


/*
================
MSG_WriteFloat
================
*/
void MSG_WriteFloat (netMsg_t *sb, float f) {
	union {
		float	f;
		int		l;
	} dat;
	
	
	dat.f = f;
	dat.l = LittleLong (dat.l);
	
	MSG_Write (sb, &dat.l, 4);
}


/*
================
MSG_WriteString
================
*/
void MSG_WriteString (netMsg_t *sb, char *s) {
	if (!s)
		MSG_Write (sb, "", 1);
	else
		MSG_Write (sb, s, strlen(s)+1);
}


/*
================
MSG_WriteCoord
================
*/
void MSG_WriteCoord (netMsg_t *sb, float f) {
	MSG_WriteShort (sb, (int)(f*8));
}


/*
================
MSG_WritePos
================
*/
void MSG_WritePos (netMsg_t *sb, vec3_t pos) {
	MSG_WriteShort (sb, (int)(pos[0]*8));
	MSG_WriteShort (sb, (int)(pos[1]*8));
	MSG_WriteShort (sb, (int)(pos[2]*8));
}


/*
================
MSG_WriteAngle
================
*/
void MSG_WriteAngle (netMsg_t *sb, float f) {
	MSG_WriteByte (sb, (int)(f*256/360) & 255);
}


/*
================
MSG_WriteAngle16
================
*/
void MSG_WriteAngle16 (netMsg_t *sb, float f) {
	MSG_WriteShort (sb, ANGLE2SHORT (f));
}


/*
================
MSG_WriteDeltaUsercmd
================
*/
void MSG_WriteDeltaUsercmd (netMsg_t *buf, userCmd_t *from, userCmd_t *cmd) {
	int		bits;

	//
	// send the movement message
	//
	bits = 0;
	if (cmd->angles[0] != from->angles[0])		bits |= CM_ANGLE1;
	if (cmd->angles[1] != from->angles[1])		bits |= CM_ANGLE2;
	if (cmd->angles[2] != from->angles[2])		bits |= CM_ANGLE3;
	if (cmd->forwardMove != from->forwardMove)	bits |= CM_FORWARD;
	if (cmd->sideMove != from->sideMove)		bits |= CM_SIDE;
	if (cmd->upMove != from->upMove)			bits |= CM_UP;
	if (cmd->buttons != from->buttons)			bits |= CM_BUTTONS;
	if (cmd->impulse != from->impulse)			bits |= CM_IMPULSE;

	MSG_WriteByte (buf, bits);

	if (bits & CM_ANGLE1)	MSG_WriteShort (buf, cmd->angles[0]);
	if (bits & CM_ANGLE2)	MSG_WriteShort (buf, cmd->angles[1]);
	if (bits & CM_ANGLE3)	MSG_WriteShort (buf, cmd->angles[2]);
	
	if (bits & CM_FORWARD)	MSG_WriteShort (buf, cmd->forwardMove);
	if (bits & CM_SIDE)		MSG_WriteShort (buf, cmd->sideMove);
	if (bits & CM_UP)		MSG_WriteShort (buf, cmd->upMove);

	if (bits & CM_BUTTONS)	MSG_WriteByte (buf, cmd->buttons);
	if (bits & CM_IMPULSE)	MSG_WriteByte (buf, cmd->impulse);

	MSG_WriteByte (buf, cmd->msec);
	MSG_WriteByte (buf, cmd->lightLevel);
}


/*
================
MSG_WriteDir
================
*/
void MSG_WriteDir (netMsg_t *sb, vec3_t dir) {
	int		best;

	best = DirToByte(dir);
	MSG_WriteByte (sb, best);
}


/*
================
MSG_ReadDir
================
*/
void MSG_ReadDir (netMsg_t *sb, vec3_t dir) {
	int		b;

	b = MSG_ReadByte (sb);
	ByteToDir (b, dir);
}


/*
==================
MSG_WriteDeltaEntity

Writes part of a packetentities message.
Can delta from either a baseline or a previous packet_entity
==================
*/
void MSG_WriteDeltaEntity (entityState_t *from, entityState_t *to, netMsg_t *msg, qBool force, qBool newentity) {
	int		bits;

	if (!to->number)
		Com_Error (ERR_FATAL, "Unset entity number");
	if (to->number >= MAX_EDICTS)
		Com_Error (ERR_FATAL, "Entity number >= MAX_EDICTS");

	/* send an update */
	bits = 0;

	if (to->number >= 256)
		bits |= U_NUMBER16;		// number8 is implicit otherwise

	if (to->origin[0] != from->origin[0])		bits |= U_ORIGIN1;
	if (to->origin[1] != from->origin[1])		bits |= U_ORIGIN2;
	if (to->origin[2] != from->origin[2])		bits |= U_ORIGIN3;

	if (to->angles[0] != from->angles[0])		bits |= U_ANGLE1;		
	if (to->angles[1] != from->angles[1])		bits |= U_ANGLE2;
	if (to->angles[2] != from->angles[2])		bits |= U_ANGLE3;
		
	if (to->skinNum != from->skinNum) {
		if ((unsigned)to->skinNum < 256)			bits |= U_SKIN8;
		else if ((unsigned)to->skinNum < 0x10000)	bits |= U_SKIN16;
		else										bits |= (U_SKIN8|U_SKIN16);
	}
		
	if (to->frame != from->frame) {
		if (to->frame < 256)	bits |= U_FRAME8;
		else					bits |= U_FRAME16;
	}

	if (to->effects != from->effects) {
		if (to->effects < 256)			bits |= U_EFFECTS8;
		else if (to->effects < 0x8000)	bits |= U_EFFECTS16;
		else							bits |= U_EFFECTS8|U_EFFECTS16;
	}
	
	if (to->renderFx != from->renderFx) {
		if (to->renderFx < 256)			bits |= U_RENDERFX8;
		else if (to->renderFx < 0x8000)	bits |= U_RENDERFX16;
		else							bits |= U_RENDERFX8|U_RENDERFX16;
	}
	
	if (to->solid != from->solid)
		bits |= U_SOLID;

	/* event is not delta compressed, just 0 compressed */
	if (to->event)
		bits |= U_EVENT;
	
	if (to->modelIndex != from->modelIndex)		bits |= U_MODEL;
	if (to->modelIndex2 != from->modelIndex2)	bits |= U_MODEL2;
	if (to->modelIndex3 != from->modelIndex3)	bits |= U_MODEL3;
	if (to->modelIndex4 != from->modelIndex4)	bits |= U_MODEL4;

	if (to->sound != from->sound)
		bits |= U_SOUND;

	if (newentity || (to->renderFx & RF_BEAM))
		bits |= U_OLDORIGIN;

	//
	// write the message
	//
	if (!bits && !force)
		return;		// nothing to send!

	//----------

	if (bits & 0xff000000)		bits |= U_MOREBITS3 | U_MOREBITS2 | U_MOREBITS1;
	else if (bits & 0x00ff0000)	bits |= U_MOREBITS2 | U_MOREBITS1;
	else if (bits & 0x0000ff00)	bits |= U_MOREBITS1;

	MSG_WriteByte (msg,	bits&255);

	if (bits & 0xff000000) {
		MSG_WriteByte (msg,	(bits>>8)&255);
		MSG_WriteByte (msg,	(bits>>16)&255);
		MSG_WriteByte (msg,	(bits>>24)&255);
	} else if (bits & 0x00ff0000) {
		MSG_WriteByte (msg,	(bits>>8)&255);
		MSG_WriteByte (msg,	(bits>>16)&255);
	} else if (bits & 0x0000ff00) {
		MSG_WriteByte (msg,	(bits>>8)&255);
	}

	//----------

	if (bits & U_NUMBER16)	MSG_WriteShort (msg, to->number);
	else					MSG_WriteByte (msg,	to->number);

	if (bits & U_MODEL)		MSG_WriteByte (msg,	to->modelIndex);
	if (bits & U_MODEL2)	MSG_WriteByte (msg,	to->modelIndex2);
	if (bits & U_MODEL3)	MSG_WriteByte (msg,	to->modelIndex3);
	if (bits & U_MODEL4)	MSG_WriteByte (msg,	to->modelIndex4);

	if (bits & U_FRAME8)	MSG_WriteByte (msg, to->frame);
	if (bits & U_FRAME16)	MSG_WriteShort (msg, to->frame);

	// used for laser colors
	if ((bits & U_SKIN8) && (bits & U_SKIN16))	MSG_WriteLong (msg, to->skinNum);
	else if (bits & U_SKIN8)					MSG_WriteByte (msg, to->skinNum);
	else if (bits & U_SKIN16)					MSG_WriteShort (msg, to->skinNum);

	if ((bits & (U_EFFECTS8|U_EFFECTS16)) == (U_EFFECTS8|U_EFFECTS16))		MSG_WriteLong (msg, to->effects);
	else if (bits & U_EFFECTS8)		MSG_WriteByte (msg, to->effects);
	else if (bits & U_EFFECTS16)	MSG_WriteShort (msg, to->effects);

	if ((bits & (U_RENDERFX8|U_RENDERFX16)) == (U_RENDERFX8|U_RENDERFX16))	MSG_WriteLong (msg, to->renderFx);
	else if (bits & U_RENDERFX8)	MSG_WriteByte (msg, to->renderFx);
	else if (bits & U_RENDERFX16)	MSG_WriteShort (msg, to->renderFx);

	if (bits & U_ORIGIN1)	MSG_WriteCoord (msg, to->origin[0]);		
	if (bits & U_ORIGIN2)	MSG_WriteCoord (msg, to->origin[1]);
	if (bits & U_ORIGIN3)	MSG_WriteCoord (msg, to->origin[2]);

	if (bits & U_ANGLE1)	MSG_WriteAngle(msg, to->angles[0]);
	if (bits & U_ANGLE2)	MSG_WriteAngle(msg, to->angles[1]);
	if (bits & U_ANGLE3)	MSG_WriteAngle(msg, to->angles[2]);

	if (bits & U_OLDORIGIN) {
		MSG_WriteCoord (msg, to->oldOrigin[0]);
		MSG_WriteCoord (msg, to->oldOrigin[1]);
		MSG_WriteCoord (msg, to->oldOrigin[2]);
	}

	if (bits & U_SOUND)	MSG_WriteByte (msg, to->sound);
	if (bits & U_EVENT)	MSG_WriteByte (msg, to->event);
	if (bits & U_SOLID)	MSG_WriteShort (msg, to->solid);
}

/*
==============================================================================

	READING FUNCTIONS

==============================================================================
*/

/*
================
MSG_BeginReading
================
*/
void MSG_BeginReading (netMsg_t *msg) {
	msg->readCount = 0;
}


/*
================
MSG_ReadChar

returns -1 if no more characters are available
================
*/
int MSG_ReadChar (netMsg_t *msg_read) {
	int	c;
	
	if (msg_read->readCount+1 > msg_read->curSize)
		c = -1;
	else
		c = (signed char)msg_read->data[msg_read->readCount];
	msg_read->readCount++;
	
	return c;
}


/*
================
MSG_ReadByte
================
*/
int MSG_ReadByte (netMsg_t *msg_read) {
	int	c;
	
	if (msg_read->readCount+1 > msg_read->curSize)
		c = -1;
	else
		c = (byte)msg_read->data[msg_read->readCount];
	msg_read->readCount++;
	
	return c;
}


/*
================
MSG_ReadShort
================
*/
int MSG_ReadShort (netMsg_t *msg_read) {
	int	c;
	
	if (msg_read->readCount+2 > msg_read->curSize)
		c = -1;
	else		
		c = (short)(msg_read->data[msg_read->readCount]
		+ (msg_read->data[msg_read->readCount+1]<<8));
	
	msg_read->readCount += 2;
	
	return c;
}


/*
================
MSG_ReadLong
================
*/
int MSG_ReadLong (netMsg_t *msg_read) {
	int	c;
	
	if (msg_read->readCount+4 > msg_read->curSize)
		c = -1;
	else
		c = msg_read->data[msg_read->readCount]
		+ (msg_read->data[msg_read->readCount+1]<<8)
		+ (msg_read->data[msg_read->readCount+2]<<16)
		+ (msg_read->data[msg_read->readCount+3]<<24);
	
	msg_read->readCount += 4;
	
	return c;
}


/*
================
MSG_ReadFloat
================
*/
float MSG_ReadFloat (netMsg_t *msg_read) {
	union {
		byte	b[4];
		float	f;
		int		l;
	} dat;
	
	if (msg_read->readCount+4 > msg_read->curSize)
		dat.f = -1;
	else {
		dat.b[0] =	msg_read->data[msg_read->readCount];
		dat.b[1] =	msg_read->data[msg_read->readCount+1];
		dat.b[2] =	msg_read->data[msg_read->readCount+2];
		dat.b[3] =	msg_read->data[msg_read->readCount+3];
	}
	msg_read->readCount += 4;
	
	dat.l = LittleLong (dat.l);

	return dat.f;	
}


/*
================
MSG_ReadString
================
*/
char *MSG_ReadString (netMsg_t *msg_read) {
	static char	string[2048];
	int			l, c;
	
	l = 0;
	do {
		c = MSG_ReadByte (msg_read);

		if ((c == -1) || (c == 0))
			break;

		string[l] = c;
		l++;
	} while (l < sizeof (string)-1);
	
	string[l] = 0;
	
	return string;
}


/*
================
MSG_ReadStringLine
================
*/
char *MSG_ReadStringLine (netMsg_t *msg_read) {
	static char	string[2048];
	int			l, c;
	
	l = 0;
	do {
		c = MSG_ReadByte (msg_read);

		if ((c == -1) || (c == 0) || (c == '\n'))
			break;

		string[l] = c;
		l++;
	} while (l < sizeof (string)-1);
	
	string[l] = 0;
	
	return string;
}


/*
================
MSG_ReadCoord
================
*/
float MSG_ReadCoord (netMsg_t *msg_read) {
	return MSG_ReadShort (msg_read) * (1.0/8);
}


/*
================
MSG_ReadPos
================
*/
void MSG_ReadPos (netMsg_t *msg_read, vec3_t pos) {
	pos[0] = MSG_ReadShort (msg_read) * (1.0/8);
	pos[1] = MSG_ReadShort (msg_read) * (1.0/8);
	pos[2] = MSG_ReadShort (msg_read) * (1.0/8);
}


/*
================
MSG_ReadAngle
================
*/
float MSG_ReadAngle (netMsg_t *msg_read) {
	return MSG_ReadChar (msg_read) * (360.0/256);
}


/*
================
MSG_ReadAngle16
================
*/
float MSG_ReadAngle16 (netMsg_t *msg_read) {
	return SHORT2ANGLE (MSG_ReadShort (msg_read));
}


/*
================
MSG_ReadDeltaUsercmd
================
*/
void MSG_ReadDeltaUsercmd (netMsg_t *msg_read, userCmd_t *from, userCmd_t *move) {
	int bits;

	memcpy (move, from, sizeof (*move));

	bits = MSG_ReadByte (msg_read);
		
	/* read current angles */
	if (bits & CM_ANGLE1)	move->angles[0] = MSG_ReadShort (msg_read);
	if (bits & CM_ANGLE2)	move->angles[1] = MSG_ReadShort (msg_read);
	if (bits & CM_ANGLE3)	move->angles[2] = MSG_ReadShort (msg_read);
		
	/* read movement */
	if (bits & CM_FORWARD)	move->forwardMove = MSG_ReadShort (msg_read);
	if (bits & CM_SIDE)		move->sideMove = MSG_ReadShort (msg_read);
	if (bits & CM_UP)		move->upMove = MSG_ReadShort (msg_read);
	
	/* read buttons */
	if (bits & CM_BUTTONS)
		move->buttons = MSG_ReadByte (msg_read);

	if (bits & CM_IMPULSE)
		move->impulse = MSG_ReadByte (msg_read);

	/* read time to run command */
	move->msec = MSG_ReadByte (msg_read);

	/* read the light level */
	move->lightLevel = MSG_ReadByte (msg_read);
}


/*
================
MSG_ReadData
================
*/
void MSG_ReadData (netMsg_t *msg_read, void *data, int len) {
	int		i;

	for (i=0 ; i<len ; i++)
		((byte *)data)[i] = MSG_ReadByte (msg_read);
}
