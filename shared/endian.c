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

// ENDIAN.C

#include "shared.h"

/*
============================================================================

	BYTE ORDER FUNCTIONS

	Updated thanks to Berserk
============================================================================
*/

/*
===============
FloatSwap
===============
*/
static float FloatSwap (float f) {
	union {
		byte	b[4];
		float	f;
	} in, out;
	
	in.f = f;

	out.b[0] = in.b[3];
	out.b[1] = in.b[2];
	out.b[2] = in.b[1];
	out.b[3] = in.b[0];
	
	return out.f;
}


/*
===============
LongSwap
===============
*/
static int LongSwap (int l) {
	union {
		byte	b[4];
		int		l;
	} in, out;

	in.l = l;

	out.b[0] = in.b[3];
	out.b[1] = in.b[2];
	out.b[2] = in.b[1];
	out.b[3] = in.b[0];

	return out.l;
}


/*
===============
ShortSwap
===============
*/
static short ShortSwap (short s) {
	union {
		byte	b[2];
		short	s;
	} in, out;

	in.s = s;

	out.b[0] = in.b[1];
	out.b[1] = in.b[0];

	return out.s;
}

// =========================================================================

#if (defined _WIN32 || defined __linux__)

inline float	BigFloat	(float f)	{ return FloatSwap(f);	}
inline int		BigLong		(int l)		{ return LongSwap(l);	}
inline short	BigShort	(short s)	{ return ShortSwap(s);	}

#elif (defined __MACOS__ || defined MACOS_X)

inline float	LittleFloat	(float f)	{ return FloatSwap(f);	}
inline int		LittleLong	(int l)		{ return LongSwap(l);	}
inline short	LittleShort	(short s)	{ return ShortSwap(s);	}

#endif
