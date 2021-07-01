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

#define USE_BYTESWAP
#include "cl_local.h"

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length
====================
*/
void CL_WriteDemoMessage (void) {
	int		len, swlen;

	// the first eight bytes are just packet sequencing stuff
	len = netMessage.curSize-8;
	swlen = LittleLong(len);
	fwrite (&swlen, 4, 1, cls.demoFile);
	fwrite (netMessage.data+8,	len, 1, cls.demoFile);
}


/*
====================
CL_Stop_f

Stop recording a demo
====================
*/
void CL_Stop_f (void) {
	int		len;

	if (!cls.demoRecording) {
		Com_Printf (0, "Not recording a demo.\n");
		return;
	}

	// finish up
	len = -1;
	fwrite (&len, 4, 1, cls.demoFile);
	fclose (cls.demoFile);
	cls.demoFile = NULL;
	cls.demoRecording = qFalse;
	Com_Printf (0, "Stopped demo.\n");
}


/*
====================
CL_Record_f

record <demoname>

Begins recording a demo from the current position
====================
*/
void CL_Record_f (void) {
	char			name[MAX_OSPATH];
	char			buf_data[MAX_MSGLEN];
	netMsg_t		buf;
	int				i, len;
	entityStateOld_t	*ent, temp;
	entityStateOld_t	nullstate;

	if (Cmd_Argc () != 2) {
		Com_Printf (0, "record <demoname>\n");
		return;
	}

	if (cls.demoRecording) {
		Com_Printf (0, "Already recording.\n");
		return;
	}

	if (cls.connectState != CA_ACTIVE) {
		Com_Printf (0, "You must be in a level to record.\n");
		return;
	}

	// open the demo file
	Q_snprintfz (name, sizeof (name), "%s/demos/%s.dm2", FS_Gamedir(), Cmd_Argv (1));

	Com_Printf (0, "recording to %s.\n", name);
	FS_CreatePath (name);
	cls.demoFile = fopen (name, "wb");
	if (!cls.demoFile) {
		Com_Printf (0, S_COLOR_RED "ERROR: couldn't open.\n");
		return;
	}

	if (cls.serverProtocol == ENHANCED_PROTOCOL_VERSION)
		Com_Printf (0, "Warning: demos recorded at cl_protocol %d may not be compatible with non-R1Q2/EGL clients!\n", ENHANCED_PROTOCOL_VERSION);

	cls.demoRecording = qTrue;

	// don't start saving messages until a non-delta compressed message is received
	cls.demoWaiting = qTrue;

	// write out messages to hold the startup information
	MSG_Init (&buf, buf_data, sizeof (buf_data));

	// send the serverdata
	MSG_WriteByte (&buf, SVC_SERVERDATA);
	MSG_WriteLong (&buf, ORIGINAL_PROTOCOL_VERSION);
	MSG_WriteLong (&buf, 0x10000 + cl.serverCount);
	MSG_WriteByte (&buf, 1);	// demos are always attract loops
	MSG_WriteString (&buf, cl.gameDir);
	MSG_WriteShort (&buf, cl.playerNum);

	MSG_WriteString (&buf, cl.configStrings[CS_NAME]);

	// configstrings
	for (i=0 ; i<MAX_CONFIGSTRINGS ; i++) {
		if (cl.configStrings[i][0]) {
			if (buf.curSize + Q_strlen (cl.configStrings[i]) + 32 > buf.maxSize) {
				// write it out
				len = LittleLong (buf.curSize);
				fwrite (&len, 4, 1, cls.demoFile);
				fwrite (buf.data, buf.curSize, 1, cls.demoFile);
				buf.curSize = 0;
			}

			MSG_WriteByte (&buf, SVC_CONFIGSTRING);
			MSG_WriteShort (&buf, i);
			MSG_WriteString (&buf, cl.configStrings[i]);
		}

	}

	// baselines
	memset (&nullstate, 0, sizeof (nullstate));
	for (i=0; i<MAX_CS_EDICTS ; i++) {
		memcpy (&temp, &clBaseLines[i], sizeof (entityStateOld_t));
		ent = &temp;
		if (!ent->modelIndex)
			continue;

		if (buf.curSize + 64 > buf.maxSize) {
			// write it out
			len = LittleLong (buf.curSize);
			fwrite (&len, 4, 1, cls.demoFile);
			fwrite (buf.data, buf.curSize, 1, cls.demoFile);
			buf.curSize = 0;
		}

		MSG_WriteByte (&buf, SVC_SPAWNBASELINE);		
		MSG_WriteDeltaEntity (&nullstate, ent, &buf, qTrue, qTrue, 0, ORIGINAL_PROTOCOL_VERSION);
	}

	MSG_WriteByte (&buf, SVC_STUFFTEXT);
	MSG_WriteString (&buf, "precache\n");

	// write it to the demo file

	len = LittleLong (buf.curSize);
	fwrite (&len, 4, 1, cls.demoFile);
	fwrite (buf.data, buf.curSize, 1, cls.demoFile);

	// the rest of the demo file will be individual frames
}
