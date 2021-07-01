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

#ifndef __FONT_H__
#define __FONT_H__

enum {
	// ---------- Com_Printf ----------
	PRINT_ALL		= 1 << 0,
	PRINT_DEV		= 1 << 1,
	PRINT_CONSOLE	= 1 << 2,

	// ------------ STYLES ------------
	FS_SECONDARY	= 1 << 3,
	FS_SHADOW		= 1 << 4,

	// ------- IMAGE SELECTIONS -------
	FS_UIFONT		= 1 << 5
};

// font scaling stuff
#define FT_SCALE		(r_fontscale->value)
#define FT_SIZE			(8.0 * FT_SCALE)

#define FT_SHAOFFSET	2

#define HUD_SCALE		(r_hudscale->value)
#define HUD_FTSIZE		(8.0 * HUD_SCALE)

#endif // __FONT_H__
