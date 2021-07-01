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
// r_cin.c
// Refresh cinematic stuff
//

#include "r_local.h"

static uInt		r_CinematcPalette[256];
extern uInt		imgPaletteTable[256];

/*
=============================================================================

	CIN RENDERING

=============================================================================
*/

/*
=============
R_CinematicPalette
=============
*/
void R_CinematicPalette (const byte *palette) {
	int		i;

	byte *rp = (byte *) r_CinematcPalette;

	if (palette) {
		for (i=0 ; i<256 ; i++) {
			rp[i*4+0] = palette[i*3+0];
			rp[i*4+1] = palette[i*3+1];
			rp[i*4+2] = palette[i*3+2];
			rp[i*4+3] = 0xff;
		}
	}
	else {
		for (i=0 ; i<256 ; i++) {
			rp[i*4+0] = imgPaletteTable[i] & 0xff;
			rp[i*4+1] = (imgPaletteTable[i] >> 8) & 0xff;
			rp[i*4+2] = (imgPaletteTable[i] >> 16) & 0xff;
			rp[i*4+3] = 0xff;
		}
	}

	qglClearColor (0, 0, 0, 0);
	qglClear (GL_COLOR_BUFFER_BIT);
	qglClearColor (1, 0, 0.5 , 0.5);
}


/*
=============
R_DrawCinematic
=============
*/
void R_DrawCinematic (int x, int y, int w, int h, int cols, int rows, byte *data) {
	uInt		image[256*256], *dest;
	int			i, j, trows, row;
	int			frac, fracstep;
	float		hscale, t;
	byte		*source;

	memset (image, 0, sizeof (image));

	GL_Bind (r_CinTexture);

	if (rows <= 256) {
		hscale = 1;
		trows = rows;
	}
	else {
		hscale = rows/256.0;
		trows = 256;
	}

	t = rows*hscale/256;
	for (i=0 ; i<trows ; i++) {
		row = (int)(i*hscale);
		if (row > rows)
			break;

		source = data + cols*row;
		dest = &image[i*256];
		fracstep = cols*0x10000/256;
		frac = fracstep >> 1;

		for (j=0 ; j<256 ; j++) {
			dest[j] = r_CinematcPalette[source[frac>>16]];
			frac += fracstep;
		}
	}

	qglColor4f (1, 1, 1, 1);

	qglTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, image);

	qglBegin (GL_QUADS);

	qglTexCoord2f (0, 0);
	qglVertex2f (x, y);

	qglTexCoord2f (1, 0);
	qglVertex2f (x+w, y);

	qglTexCoord2f (1, t);
	qglVertex2f (x+w, y+h);

	qglTexCoord2f (0, t);
	qglVertex2f (x, y+h);

	qglEnd ();
}
