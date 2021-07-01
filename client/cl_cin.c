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
// cl_cin.c
//

#define USE_BYTESWAP
#include "cl_local.h"

typedef struct huffBlock_s {
	byte	*data;
	int		count;
} huffBlock_t;

typedef struct cinState_s {
	qBool				restartSound;
	int					sndRate;
	int					sndWidth;
	int					sndChannels;
	struct channel_s	*sndRawChannel;
	qBool				sndAL;

	int					width;
	int					height;
	byte				*pic;
	byte				*picPending;

	// Order 1 huffman stuff
	int					*huffNodes1;
	int					huffNumNodes1[256];

	int					huffUsed[512];
	int					huffCount[512];
} cinState_t;

static cinState_t	cl_cinState;

/*
==================
CIN_StopCinematic
==================
*/
void CIN_StopCinematic (void)
{
	cl.cin.time = 0;	// Done

	Snd_RawStop (cl_cinState.sndRawChannel);

	if (cl_cinState.pic) {
		Mem_Free (cl_cinState.pic);
		cl_cinState.pic = NULL;
	}

	if (cl_cinState.picPending) {
		Mem_Free (cl_cinState.picPending);
		cl_cinState.picPending = NULL;
	}

	if (cl.cin.fileNum) {
		FS_CloseFile (cl.cin.fileNum);
		cl.cin.fileNum = 0;
	}

	if (cl_cinState.huffNodes1) {
		Mem_Free (cl_cinState.huffNodes1);
		cl_cinState.huffNodes1 = NULL;
	}

	// Switch back down to 11 khz sound if necessary
	if (cl_cinState.restartSound) {
		cl_cinState.restartSound = qFalse;
		Cbuf_AddText ("snd_restart\n");
	}
}


/*
====================
CIN_FinishCinematic

Called when either the cinematic completes, or it is aborted
====================
*/
void CIN_FinishCinematic (void)
{
	// Tell the server to advance to the next map / cinematic
	MSG_WriteByte (&cls.netChan.message, CLC_STRINGCMD);
	MSG_Print (&cls.netChan.message, Q_VarArgs ("nextserver %i\n", cl.serverCount));

	CL_SetState (CA_CONNECTED);
}

// ==========================================================================

/*
==================
Huff1_SmallestNode
==================
*/
static int Huff1_SmallestNode (int numNodes)
{
	int		i;
	int		best, bestNode;

	best = 99999999;
	bestNode = -1;
	for (i=0 ; i<numNodes ; i++) {
		if (cl_cinState.huffUsed[i])
			continue;
		if (!cl_cinState.huffCount[i])
			continue;
		if (cl_cinState.huffCount[i] < best) {
			best = cl_cinState.huffCount[i];
			bestNode = i;
		}
	}

	if (bestNode == -1)
		return -1;

	cl_cinState.huffUsed[bestNode] = qTrue;
	return bestNode;
}


/*
==================
Huff1_TableInit

Reads the 64k counts table and initializes the node trees
==================
*/
static void Huff1_TableInit (cinState_t *cin)
{
	int		*node, *nodeBase;
	byte	counts[256];
	int		numNodes;
	int		prev, j;

	cin->huffNodes1 = Mem_Alloc (256*256*2*4);
	for (prev=0 ; prev<256 ; prev++) {
		memset (cin->huffCount, 0, sizeof (cin->huffCount));
		memset (cin->huffUsed, 0, sizeof (cin->huffUsed));

		// Read a row of counts
		FS_Read (counts, sizeof (counts), cl.cin.fileNum);
		for (j=0 ; j<256 ; j++)
			cin->huffCount[j] = counts[j];

		// Build the nodes
		numNodes = 256;
		nodeBase = cin->huffNodes1 + prev*256*2;

		while (numNodes != 511) {
			node = nodeBase + (numNodes-256)*2;

			// Pick two lowest counts
			node[0] = Huff1_SmallestNode (numNodes);
			if (node[0] == -1)
				break;	// no more

			node[1] = Huff1_SmallestNode (numNodes);
			if (node[1] == -1)
				break;

			cin->huffCount[numNodes] = cin->huffCount[node[0]] + cin->huffCount[node[1]];
			numNodes++;
		}

		cin->huffNumNodes1[prev] = numNodes-1;
	}
}


/*
==================
Huff1_Decompress
==================
*/
#define DO 	if (nodenum < 256) { \
				hnodes = hnodesbase + (nodenum<<9); \
				*out_p++ = nodenum; \
				if (!--count) \
					break; \
				nodenum = cin->huffNumNodes1[nodenum]; \
			} \
			nodenum = hnodes[nodenum*2 + (inbyte&1)]; \
			inbyte >>=1; \

static huffBlock_t Huff1_Decompress (cinState_t *cin, huffBlock_t in)
{
	byte		*input, *out_p;
	huffBlock_t	out;
	int			nodenum, count;
	int			inbyte;
	int			*hnodes, *hnodesbase;

	// Get decompressed count
	count = in.data[0] + (in.data[1]<<8) + (in.data[2]<<16) + (in.data[3]<<24);
	input = in.data + 4;
	out_p = out.data = Mem_Alloc (count);

	// Read bits
	hnodesbase = cin->huffNodes1 - 256*2;	// nodes 0-255 aren't stored

	hnodes = hnodesbase;
	nodenum = cin->huffNumNodes1[0];
	while (count) {
		inbyte = *input++;
		DO; DO; DO; DO; DO; DO; DO; DO;
	}

	if (input - in.data != in.count && input - in.data != in.count+1)
		Com_Printf (PRNT_WARNING, "Decompression overread by %i", (input - in.data) - in.count);

	out.count = out_p - out.data;

	return out;
}
#undef DO

/*
==================
CIN_ReadNextFrame
==================
*/
static byte *CIN_ReadNextFrame (cinState_t *cin)
{
	int			command;
	byte		data[0x40000];
	byte		compressed[0x20000];
	int			size;
	byte		*pic;
	huffBlock_t	in, huf1;
	int			start, end, samples;

	// Read the next frame
	FS_Read (&command, sizeof (command), cl.cin.fileNum);
	command = LittleLong (command);
	switch (command) {
	case 2:
		// Last frame marker
		return NULL;

	case 1:
		// Read palette
		FS_Read (cl.cin.palette, sizeof (cl.cin.palette), cl.cin.fileNum);
		break;
	}

	// Decompress the next frame
	FS_Read (&size, sizeof (size), cl.cin.fileNum);
	size = LittleLong (size);
	if (size > sizeof (compressed) || size < 1)
		Com_Error (ERR_DROP, "Bad compressed frame size");
	FS_Read (compressed, size, cl.cin.fileNum);

	// Read sound
	start = cl.cin.frameNum * cin->sndRate / 14;
	end = (cl.cin.frameNum+1) * cin->sndRate / 14;
	samples = end - start;

	// FIXME: HACK: disgusting, but necessary for sync with OpenAL
	if (!cl.cin.frameNum && cl_cinState.sndAL) {
		samples += 4096;
		if (cin->sndWidth == 2)
			memset (data, 0x00, 4096*cin->sndWidth*cin->sndChannels);
		else
			memset (data, 0x80, 4096*cin->sndWidth*cin->sndChannels);
		FS_Read (data+(4096*cin->sndWidth*cin->sndChannels), (samples-4096)*cin->sndWidth*cin->sndChannels, cl.cin.fileNum);
	}
	else
		FS_Read (data, samples*cin->sndWidth*cin->sndChannels, cl.cin.fileNum);

	Snd_RawSamples (cin->sndRawChannel, samples, cin->sndRate, cin->sndWidth, cin->sndChannels, data);

	in.data = compressed;
	in.count = size;

	huf1 = Huff1_Decompress (&cl_cinState, in);

	pic = huf1.data;

	cl.cin.frameNum++;
	return pic;
}


/*
==================
CIN_RunCinematic
==================
*/
void CIN_RunCinematic (void)
{
	int		frame;

	if (cl.cin.time <= 0) {
		CIN_StopCinematic ();
		return;
	}

	if (cl.cin.frameNum == -1)
		return;		// Static image

	if (Key_GetDest () != KD_GAME) {
		// Pause if menu or console is up
		cl.cin.time = cls.realTime - cl.cin.frameNum*1000/14;
		return;
	}

	frame = (cls.realTime - cl.cin.time)*14.0f/1000;
	if (frame <= cl.cin.frameNum)
		return;

	if (frame > cl.cin.frameNum+1) {
		Com_Printf (PRNT_WARNING, "Dropped frame: %i > %i\n", frame, cl.cin.frameNum+1);
		cl.cin.time = cls.realTime - cl.cin.frameNum*1000/14;
	}

	if (cl_cinState.pic)
		Mem_Free (cl_cinState.pic);

	cl_cinState.pic = cl_cinState.picPending;
	cl_cinState.picPending = NULL;
	cl_cinState.picPending = CIN_ReadNextFrame (&cl_cinState);

	if (!cl_cinState.picPending) {
		CIN_StopCinematic ();
		CIN_FinishCinematic ();

		cl.cin.time = 1;	// FIXME: Hack to get the black screen behind loading
		SCR_BeginLoadingPlaque ();
		cl.cin.time = 0;
		return;
	}
}


/*
==================
CIN_DrawCinematic
==================
*/
void CIN_DrawCinematic (void)
{
	// Pause if menu is up
	if (Key_GetDest () == KD_MENU)
		return;

	// No cinematic image to render!
	if (!cl_cinState.pic)
		return;

	// Set the palette and render
	R_CinematicPalette ((const byte *)cl.cin.palette);
	R_DrawCinematic (0, 0, cls.glConfig.vidWidth, cls.glConfig.vidHeight, cl_cinState.width, cl_cinState.height, cl_cinState.pic);
}


/*
==================
CIN_PlayCinematic
==================
*/
void R_LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height); // FIXME
void CIN_PlayCinematic (char *arg)
{
	byte	*palette;
	char	name[MAX_OSPATH], *dot;
	int		old_khz;

	// Make sure CD isn't playing music
	CDAudio_Stop ();

	cl.cin.frameNum = 0;

	// Static pcx image
	dot = strstr (arg, ".");
	if (dot && !strcmp (dot, ".pcx")) {
		Q_snprintfz (name, sizeof (name), "pics/%s", arg);
		R_LoadPCX (name, &cl_cinState.pic, &palette, &cl_cinState.width, &cl_cinState.height);

		cl.cin.frameNum = -1;
		cl.cin.time = 1;
		SCR_EndLoadingPlaque ();
		CL_SetState (CA_ACTIVE);

		if (!cl_cinState.pic) {
			Com_Printf (PRNT_WARNING, "%s not found.\n", name);
			cl.cin.time = 0;
		}
		else {
			memcpy (cl.cin.palette, palette, sizeof (cl.cin.palette));
			Mem_Free (palette);
		}
		return;
	}

	// Load the video
	Q_snprintfz (name, sizeof (name), "video/%s", arg);
	FS_OpenFile (name, &cl.cin.fileNum, FS_MODE_READ_BINARY);
	if (!cl.cin.fileNum) {
		CIN_FinishCinematic ();
		cl.cin.time = 0;	// Done
		return;
	}

	// Set the prep state
	SCR_EndLoadingPlaque ();
	CL_SetState (CA_ACTIVE);

	// Read and swap values
	FS_Read (&cl_cinState.width, 4, cl.cin.fileNum);
	FS_Read (&cl_cinState.height, 4, cl.cin.fileNum);
	cl_cinState.width = LittleLong (cl_cinState.width);
	cl_cinState.height = LittleLong (cl_cinState.height);

	FS_Read (&cl_cinState.sndRate, 4, cl.cin.fileNum);
	FS_Read (&cl_cinState.sndWidth, 4, cl.cin.fileNum);
	FS_Read (&cl_cinState.sndChannels, 4, cl.cin.fileNum);
	cl_cinState.sndRate = LittleLong (cl_cinState.sndRate);
	cl_cinState.sndWidth = LittleLong (cl_cinState.sndWidth);
	cl_cinState.sndChannels = LittleLong (cl_cinState.sndChannels);

	// Setup the streaming channel
	cl_cinState.sndRawChannel = Snd_RawStart ();
	cl_cinState.sndAL = (cl_cinState.sndRawChannel) ? qTrue : qFalse;

	// Setup the huff table
	Huff1_TableInit (&cl_cinState);

	// Switch up to 22 khz sound if necessary
	if (!cl_cinState.sndAL) {
		old_khz = Cvar_GetIntegerValue ("s_khz");
		if (old_khz != cl_cinState.sndRate/1000) {
			cl_cinState.restartSound = qTrue;
			Cvar_VariableSetValue (s_khz, cl_cinState.sndRate/1000, qTrue);
			Cbuf_AddText ("snd_restart\n");
			Cvar_VariableSetValue (s_khz, old_khz, qTrue);
		}
	}

	// Start
	cl.cin.frameNum = 0;
	cl_cinState.pic = CIN_ReadNextFrame (&cl_cinState);
	cl.cin.time = Sys_Milliseconds ();
}
