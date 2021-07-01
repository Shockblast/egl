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
// r_math.c
// Renderer math library
//

#include "r_local.h"

/*
==============
R_FloatToByte
==============
*/
byte R_FloatToByte (float x)
{
	union {
		float			f;
		uint32			i;
	} f2i;

	// Shift float to have 8bit fraction at base of number
	f2i.f = x + 32768.0f;
	f2i.i &= 0x7FFFFF;

	// Then read as integer and kill float bits...
	return (byte)min(f2i.i, 255);
}


/*
===============
R_ColorNormalize
===============
*/
float R_ColorNormalize (const float *in, float *out)
{
	float	f = max (max (in[0], in[1]), in[2]);

	if (f > 1.0f) {
		f = 1.0f / f;
		out[0] = in[0] * f;
		out[1] = in[1] * f;
		out[2] = in[2] * f;
	}
	else {
		out[0] = in[0];
		out[1] = in[1];
		out[2] = in[2];
	}

	return f;
}


/*
===============
Matrix4_Copy2D
===============
*/
void Matrix4_Copy2D (const mat4x4_t m1, mat4x4_t m2)
{
	m2[0] = m1[0];
	m2[1] = m1[1];
	m2[4] = m1[4];
	m2[5] = m1[5];
	m2[12] = m1[12];
	m2[13] = m1[13];
}


/*
===============
Matrix4_Multiply2D
===============
*/
void Matrix4_Multiply2D (const mat4x4_t m1, const mat4x4_t m2, mat4x4_t out)
{
	out[0]  = m1[0] * m2[0] + m1[4] * m2[1];
	out[1]  = m1[1] * m2[0] + m1[5] * m2[1];
	out[4]  = m1[0] * m2[4] + m1[4] * m2[5];
	out[5]  = m1[1] * m2[4] + m1[5] * m2[5];
	out[12] = m1[0] * m2[12] + m1[4] * m2[13] + m1[12];
	out[13] = m1[1] * m2[12] + m1[5] * m2[13] + m1[13];
}


/*
===============
Matrix4_Scale2D
===============
*/
void Matrix4_Scale2D (mat4x4_t m, float x, float y)
{
	m[0] *= x;
	m[1] *= x;
	m[4] *= y;
	m[5] *= y;
}


/*
===============
Matrix4_Stretch2D
===============
*/
void Matrix4_Stretch2D (mat4x4_t m, float s, float t)
{
	m[0] *= s;
	m[1] *= s;
	m[4] *= s;
	m[5] *= s;
	m[12] = s * m[12] + t;
	m[13] = s * m[13] + t;
}


/*
===============
Matrix4_Translate2D
===============
*/
void Matrix4_Translate2D (mat4x4_t m, float x, float y)
{
	m[12] += x;
	m[13] += y;
}


/*
===============
Matrix4_Multiply_Vector
===============
*/
void Matrix4_Multiply_Vector (const mat4x4_t m, const vec4_t v, vec4_t out)
{
	out[0] = m[0] * v[0] + m[4] * v[1] + m[8] * v[2] + m[12] * v[3];
	out[1] = m[1] * v[0] + m[5] * v[1] + m[9] * v[2] + m[13] * v[3];
	out[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14] * v[3];
	out[3] = m[3] * v[0] + m[7] * v[1] + m[11] * v[2] + m[15] * v[3];
}
