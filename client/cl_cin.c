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

#include "cl_local.h"

typedef struct huffBlock_s {
	byte	*data;
	int		count;
} huffBlock_t;

typedef struct cinState_s {
	qBool		restartSound;
	int			sndRate;
	int			sndWidth;
	int			sndChannels;

	int			width;
	int			height;
	byte		*pic;
	byte		*picPending;

	// order 1 huffman stuff
	int			*huffNodes1;
	int			huffNumNodes1[256];

	int			huffUsed[512];
	int			huffCount[512];
} cinState_t;

static cinState_t	cin;

/*
==================
CIN_StopCinematic
==================
*/
void CIN_StopCinematic (void) {
	cl.cinematicTime = 0;	// done
	if (cin.pic) {
		Mem_Free (cin.pic);
		cin.pic = NULL;
	}

	if (cin.picPending) {
		Mem_Free (cin.picPending);
		cin.picPending = NULL;
	}

	if (cl.cinematicPaletteActive) {
		R_SetPalette (NULL);
		cl.cinematicPaletteActive = qFalse;
	}

	if (cl.cinematicFile) {
		fclose (cl.cinematicFile);
		cl.cinematicFile = NULL;
	}

	if (cin.huffNodes1) {
		Mem_Free (cin.huffNodes1);
		cin.huffNodes1 = NULL;
	}

	// switch back down to 11 khz sound if necessary
	if (cin.restartSound) {
		cin.restartSound = qFalse;
		Cbuf_AddText ("snd_restart\n");
	}
}


/*
====================
CIN_FinishCinematic

Called when either the cinematic completes, or it is aborted
====================
*/
void CIN_FinishCinematic (void) {
	// tell the server to advance to the next map / cinematic
	MSG_WriteByte (&cls.netChan.message, CLC_STRINGCMD);
	MSG_Print (&cls.netChan.message, Q_VarArgs ("nextserver %i\n", cl.serverCount));
}

//==========================================================================

/*
==================
SmallestNode1
==================
*/
int	SmallestNode1 (int numhnodes) {
	int		i;
	int		best, bestnode;

	best = 99999999;
	bestnode = -1;
	for (i=0 ; i<numhnodes ; i++) {
		if (cin.huffUsed[i])
			continue;
		if (!cin.huffCount[i])
			continue;
		if (cin.huffCount[i] < best) {
			best = cin.huffCount[i];
			bestnode = i;
		}
	}

	if (bestnode == -1)
		return -1;

	cin.huffUsed[bestnode] = qTrue;
	return bestnode;
}


/*
==================
Huff1TableInit

Reads the 64k counts table and initializes the node trees
==================
*/
void Huff1TableInit (void) {
	int		prev;
	int		j;
	int		*node, *nodebase;
	byte	counts[256];
	int		numhnodes;

	cin.huffNodes1 = Mem_Alloc (256*256*2*4);

	for (prev=0 ; prev<256 ; prev++) {
		memset (cin.huffCount, 0, sizeof (cin.huffCount));
		memset (cin.huffUsed, 0, sizeof (cin.huffUsed));

		// read a row of counts
		FS_Read (counts, sizeof (counts), cl.cinematicFile);
		for (j=0 ; j<256 ; j++)
			cin.huffCount[j] = counts[j];

		// build the nodes
		numhnodes = 256;
		nodebase = cin.huffNodes1 + prev*256*2;

		while (numhnodes != 511) {
			node = nodebase + (numhnodes-256)*2;

			// pick two lowest counts
			node[0] = SmallestNode1 (numhnodes);
			if (node[0] == -1)
				break;	// no more

			node[1] = SmallestNode1 (numhnodes);
			if (node[1] == -1)
				break;

			cin.huffCount[numhnodes] = cin.huffCount[node[0]] + cin.huffCount[node[1]];
			numhnodes++;
		}

		cin.huffNumNodes1[prev] = numhnodes-1;
	}
}


/*
==================
Huff1Decompress
==================
*/
#define DO 	if (nodenum < 256) { \
				hnodes = hnodesbase + (nodenum<<9); \
				*out_p++ = nodenum; \
				if (!--count) \
					break; \
				nodenum = cin.huffNumNodes1[nodenum]; \
			} \
			nodenum = hnodes[nodenum*2 + (inbyte&1)]; \
			inbyte >>=1; \

huffBlock_t Huff1Decompress (huffBlock_t in) {
	byte		*input, *out_p;
	huffBlock_t	out;
	int			nodenum, count;
	int			inbyte;
	int			*hnodes, *hnodesbase;

	// get decompressed count
	count = in.data[0] + (in.data[1]<<8) + (in.data[2]<<16) + (in.data[3]<<24);
	input = in.data + 4;
	out_p = out.data = Mem_Alloc (count);

	// read bits
	hnodesbase = cin.huffNodes1 - 256*2;	// nodes 0-255 aren't stored

	hnodes = hnodesbase;
	nodenum = cin.huffNumNodes1[0];
	while (count) {
		inbyte = *input++;
		DO; DO; DO; DO; DO; DO; DO; DO;
	}

	if ((input - in.data != in.count) && (input - in.data != in.count+1))
		Com_Printf (0, S_COLOR_YELLOW "Decompression overread by %i", (input - in.data) - in.count);

	out.count = out_p - out.data;

	return out;
}
#undef DO

/*
==================
CIN_ReadNextFrame
==================
*/
byte *CIN_ReadNextFrame (void) {
	int			r;
	int			command;
	byte		samples[22050/14*4];
	byte		compressed[0x20000];
	int			size;
	byte		*pic;
	huffBlock_t	in, huf1;
	int			start, end, count;

	// read the next frame
	r = fread (&command, 4, 1, cl.cinematicFile);
	if (r == 0)		// we'll give it one more chance
		r = fread (&command, 4, 1, cl.cinematicFile);

	if (r != 1)
		return NULL;
	command = LittleLong (command);
	if (command == 2)
		return NULL;	// last frame marker

	if (command == 1) {
		// read palette
		FS_Read (cl.cinematicPalette, sizeof (cl.cinematicPalette), cl.cinematicFile);
		cl.cinematicPaletteActive = 0;	// dubious....  exposes an edge case
	}

	// decompress the next frame
	FS_Read (&size, 4, cl.cinematicFile);
	size = LittleLong(size);
	if (size > sizeof (compressed) || size < 1)
		Com_Error (ERR_DROP, "Bad compressed frame size");
	FS_Read (compressed, size, cl.cinematicFile);

	// read sound
	start = cl.cinematicFrame*cin.sndRate/14;
	end = (cl.cinematicFrame+1)*cin.sndRate/14;
	count = end - start;

	FS_Read (samples, count*cin.sndWidth*cin.sndChannels, cl.cinematicFile);

	Snd_RawSamples (count, cin.sndRate, cin.sndWidth, cin.sndChannels, samples);

	in.data = compressed;
	in.count = size;

	huf1 = Huff1Decompress (in);

	pic = huf1.data;

	cl.cinematicFrame++;

	return pic;
}


/*
==================
CIN_RunCinematic
==================
*/
void CIN_RunCinematic (void) {
	int		frame;

	if (cl.cinematicTime <= 0) {
		CIN_StopCinematic ();
		return;
	}

	if (cl.cinematicFrame == -1)
		return;		// static image

	if (!Key_KeyDest (KD_GAME)) {
		// pause if menu or console is up
		cl.cinematicTime = cls.realTime - cl.cinematicFrame*1000/14;
		return;
	}

	frame = (cls.realTime - cl.cinematicTime)*14.0/1000;
	if (frame <= cl.cinematicFrame)
		return;

	if (frame > cl.cinematicFrame+1) {
		Com_Printf (0, S_COLOR_YELLOW "Dropped frame: %i > %i\n", frame, cl.cinematicFrame+1);
		cl.cinematicTime = cls.realTime - cl.cinematicFrame*1000/14;
	}

	if (cin.pic)
		Mem_Free (cin.pic);

	cin.pic = cin.picPending;
	cin.picPending = NULL;
	cin.picPending = CIN_ReadNextFrame ();

	if (!cin.picPending) {
		CIN_StopCinematic ();
		CIN_FinishCinematic ();
		cl.cinematicTime = 1;	// hack to get the black screen behind loading
		SCR_BeginLoadingPlaque ();
		cl.cinematicTime = 0;
		return;
	}
}


/*
==================
CIN_DrawCinematic

Returns qTrue if a cinematic is active, meaning the view rendering
should be skipped
==================
*/
qBool CIN_DrawCinematic (void) {
	if (cl.cinematicTime <= 0)
		return qFalse;

	if (Key_KeyDest (KD_MENU)) {
		// blank screen and pause if menu is up
		R_SetPalette (NULL);
		cl.cinematicPaletteActive = qFalse;
		return qTrue;
	}

	if (!cl.cinematicPaletteActive) {
		R_SetPalette (cl.cinematicPalette);
		cl.cinematicPaletteActive = qTrue;
	}

	if (!cin.pic)
		return qTrue;

	R_StretchRaw (0, 0, vidDef.width, vidDef.height, cin.width, cin.height, cin.pic);

	return qTrue;
}


/*
==================
CIN_PlayCinematic
==================
*/
void Img_LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height);
void CIN_PlayCinematic (char *arg) {
	int		width, height;
	byte	*palette;
	char	name[MAX_OSPATH], *dot;
	int		old_khz;

	// make sure CD isn't playing music
	CDAudio_Stop ();

	cl.cinematicFrame = 0;
	dot = strstr (arg, ".");
	if (dot && !strcmp (dot, ".pcx")) {
		// static pcx image
		Q_snprintfz (name, sizeof (name), "pics/%s", arg);
		Img_LoadPCX (name, &cin.pic, &palette, &cin.width, &cin.height);

		cl.cinematicFrame = -1;
		cl.cinematicTime = 1;
		SCR_EndLoadingPlaque ();
		CL_SetState (CA_ACTIVE);

		if (!cin.pic) {
			Com_Printf (0, S_COLOR_YELLOW "%s not found.\n", name);
			cl.cinematicTime = 0;
		} else {
			memcpy (cl.cinematicPalette, palette, sizeof (cl.cinematicPalette));
			Mem_Free (palette);
		}
		return;
	}

	Q_snprintfz (name, sizeof (name), "video/%s", arg);
	FS_FOpenFile (name, &cl.cinematicFile);
	if (!cl.cinematicFile) {
		CIN_FinishCinematic ();
		cl.cinematicTime = 0;	// done
		return;
	}

	SCR_EndLoadingPlaque ();

	CL_SetState (CA_ACTIVE);

	FS_Read (&width, 4, cl.cinematicFile);
	FS_Read (&height, 4, cl.cinematicFile);
	cin.width = LittleLong (width);
	cin.height = LittleLong (height);

	FS_Read (&cin.sndRate, 4, cl.cinematicFile);
	cin.sndRate = LittleLong (cin.sndRate);
	FS_Read (&cin.sndWidth, 4, cl.cinematicFile);
	cin.sndWidth = LittleLong (cin.sndWidth);
	FS_Read (&cin.sndChannels, 4, cl.cinematicFile);
	cin.sndChannels = LittleLong (cin.sndChannels);

	Huff1TableInit ();

	// switch up to 22 khz sound if necessary
	old_khz = Cvar_VariableInteger ("s_khz");
	if (old_khz != cin.sndRate/1000) {
		cin.restartSound = qTrue;
		Cvar_VariableForceSetValue (s_khz, cin.sndRate/1000);
		Cbuf_AddText ("snd_restart\n");
		Cvar_VariableForceSetValue (s_khz, old_khz);
	}

	cl.cinematicFrame = 0;
	cin.pic = CIN_ReadNextFrame ();
	cl.cinematicTime = Sys_Milliseconds ();
}
