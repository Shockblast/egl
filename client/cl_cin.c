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

// cl_cin.c

#include "cl_local.h"

typedef struct {
	byte	*data;
	int		count;
} huffBlock_t;

typedef struct {
	qBool		restartSound;
	int			snd_Rate;
	int			snd_Width;
	int			snd_Channels;

	int			width;
	int			height;
	byte		*pic;
	byte		*picPending;

	// order 1 huffman stuff
	int			*huff_Nodes1;
	int			huff_NumNodes1[256];

	int			huff_Used[512];
	int			huff_Count[512];
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
		Z_Free (cin.pic);
		cin.pic = NULL;
	}

	if (cin.picPending) {
		Z_Free (cin.picPending);
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

	if (cin.huff_Nodes1) {
		Z_Free (cin.huff_Nodes1);
		cin.huff_Nodes1 = NULL;
	}

	// switch back down to 11 khz sound if necessary
	if (cin.restartSound) {
		cin.restartSound = qFalse;
		Snd_Restart_f ();
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
	MSG_Print (&cls.netChan.message, va ("nextserver %i\n", cl.serverCount));
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
		if (cin.huff_Used[i])
			continue;
		if (!cin.huff_Count[i])
			continue;
		if (cin.huff_Count[i] < best) {
			best = cin.huff_Count[i];
			bestnode = i;
		}
	}

	if (bestnode == -1)
		return -1;

	cin.huff_Used[bestnode] = qTrue;
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

	cin.huff_Nodes1 = Z_TagMalloc (256*256*2*4, ZTAG_IMAGESYS);
	memset (cin.huff_Nodes1, 0, 256*256*2*4);

	for (prev=0 ; prev<256 ; prev++) {
		memset (cin.huff_Count, 0, sizeof (cin.huff_Count));
		memset (cin.huff_Used, 0, sizeof (cin.huff_Used));

		// read a row of counts
		FS_Read (counts, sizeof (counts), cl.cinematicFile);
		for (j=0 ; j<256 ; j++)
			cin.huff_Count[j] = counts[j];

		// build the nodes
		numhnodes = 256;
		nodebase = cin.huff_Nodes1 + prev*256*2;

		while (numhnodes != 511) {
			node = nodebase + (numhnodes-256)*2;

			// pick two lowest counts
			node[0] = SmallestNode1 (numhnodes);
			if (node[0] == -1)
				break;	// no more

			node[1] = SmallestNode1 (numhnodes);
			if (node[1] == -1)
				break;

			cin.huff_Count[numhnodes] = cin.huff_Count[node[0]] + cin.huff_Count[node[1]];
			numhnodes++;
		}

		cin.huff_NumNodes1[prev] = numhnodes-1;
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
				nodenum = cin.huff_NumNodes1[nodenum]; \
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
	out_p = out.data = Z_TagMalloc (count, ZTAG_IMAGESYS);

	// read bits
	hnodesbase = cin.huff_Nodes1 - 256*2;	// nodes 0-255 aren't stored

	hnodes = hnodesbase;
	nodenum = cin.huff_NumNodes1[0];
	while (count) {
		inbyte = *input++;
		DO; DO; DO; DO; DO; DO; DO; DO;
	}

	if ((input - in.data != in.count) && (input - in.data != in.count+1))
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Decompression overread by %i", (input - in.data) - in.count);

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
	start = cl.cinematicFrame*cin.snd_Rate/14;
	end = (cl.cinematicFrame+1)*cin.snd_Rate/14;
	count = end - start;

	FS_Read (samples, count*cin.snd_Width*cin.snd_Channels, cl.cinematicFile);

	Snd_RawSamples (count, cin.snd_Rate, cin.snd_Width, cin.snd_Channels, samples);

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
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Dropped frame: %i > %i\n", frame, cl.cinematicFrame+1);
		cl.cinematicTime = cls.realTime - cl.cinematicFrame*1000/14;
	}

	if (cin.pic)
		Z_Free (cin.pic);

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
int Img_LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height);
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
		Com_sprintf (name, sizeof (name), "pics/%s", arg);
		Img_LoadPCX (name, &cin.pic, &palette, &cin.width, &cin.height);

		cl.cinematicFrame = -1;
		cl.cinematicTime = 1;
		SCR_EndLoadingPlaque ();
		CL_SetConnectState (CA_ACTIVE);

		if (!cin.pic) {
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "%s not found.\n", name);
			cl.cinematicTime = 0;
		} else {
			memcpy (cl.cinematicPalette, palette, sizeof (cl.cinematicPalette));
			Z_Free (palette);
		}
		return;
	}

	Com_sprintf (name, sizeof (name), "video/%s", arg);
	FS_FOpenFile (name, &cl.cinematicFile);
	if (!cl.cinematicFile) {
		CIN_FinishCinematic ();
		cl.cinematicTime = 0;	// done
		return;
	}

	SCR_EndLoadingPlaque ();

	CL_SetConnectState (CA_ACTIVE);

	FS_Read (&width, 4, cl.cinematicFile);
	FS_Read (&height, 4, cl.cinematicFile);
	cin.width = LittleLong (width);
	cin.height = LittleLong (height);

	FS_Read (&cin.snd_Rate, 4, cl.cinematicFile);
	cin.snd_Rate = LittleLong (cin.snd_Rate);
	FS_Read (&cin.snd_Width, 4, cl.cinematicFile);
	cin.snd_Width = LittleLong (cin.snd_Width);
	FS_Read (&cin.snd_Channels, 4, cl.cinematicFile);
	cin.snd_Channels = LittleLong (cin.snd_Channels);

	Huff1TableInit ();

	// switch up to 22 khz sound if necessary
	old_khz = Cvar_VariableInteger ("s_khz");
	if (old_khz != cin.snd_Rate/1000) {
		cin.restartSound = qTrue;
		Cvar_SetValue ("s_khz", cin.snd_Rate/1000);
		Snd_Restart_f ();
		Cvar_SetValue ("s_khz", old_khz);
	}

	cl.cinematicFrame = 0;
	cin.pic = CIN_ReadNextFrame ();
	cl.cinematicTime = Sys_Milliseconds ();
}
