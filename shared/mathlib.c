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

// MATHLIB.C

#include "shared.h"

vec3_t		vec3Origin = {
	0, 0, 0
};

vec4_t		vec4Identity = {
	0, 0, 0, 0
};

mat4x4_t	mat4x4Identity = { 
	1, 0, 0, 0, 
	0, 1, 0, 0, 
	0, 0, 1, 0, 
	0, 0, 0, 1 
};

vec3_t		axisIdentity[3] =  { 
	1, 0, 0, 
	0, 1, 0, 
	0, 0, 1 
};

quat_t		quatIdentity = {
	0, 0, 0, 1
};

/*
==============================================================

	MATHLIB

==============================================================
*/

/*
=================
crand

-1 to 1
=================
*/
float crand (void) {
	return (rand () & RANDSIZE) * TWODIVRS - 1.0f;
}


/*
=================
frand

0 to 1
=================
*/
float frand (void) {
	return (rand () & RANDSIZE) * ONEDIVRS;
}

// ===========================================================

vec3_t byteDirs[NUMVERTEXNORMALS] = {
	{-0.525731,	0.000000,	0.850651},	{-0.442863,	0.238856,	0.864188},	{-0.295242,	0.000000,	0.955423},
	{-0.309017,	0.500000,	0.809017},	{-0.162460,	0.262866,	0.951056},	{0.000000,	0.000000,	1.000000},
	{0.000000,	0.850651,	0.525731},	{-0.147621,	0.716567,	0.681718},	{0.147621,	0.716567,	0.681718},
	{0.000000,	0.525731,	0.850651},	{0.309017,	0.500000,	0.809017},	{0.525731,	0.000000,	0.850651},
	{0.295242,	0.000000,	0.955423},	{0.442863,	0.238856,	0.864188},	{0.162460,	0.262866,	0.951056},
	{-0.681718,	0.147621,	0.716567},	{-0.809017,	0.309017,	0.500000},	{-0.587785,	0.425325,	0.688191},
	{-0.850651,	0.525731,	0.000000},	{-0.864188,	0.442863,	0.238856},	{-0.716567,	0.681718,	0.147621},
	{-0.688191,	0.587785,	0.425325},	{-0.500000,	0.809017,	0.309017},	{-0.238856,	0.864188,	0.442863},
	{-0.425325,	0.688191,	0.587785},	{-0.716567,	0.681718,	-0.147621},	{-0.500000,	0.809017,	-0.309017},
	{-0.525731,	0.850651,	0.000000},	{0.000000,	0.850651,	-0.525731},	{-0.238856,	0.864188,	-0.442863},
	{0.000000,	0.955423,	-0.295242},	{-0.262866,	0.951056,	-0.162460},	{0.000000,	1.000000,	0.000000},
	{0.000000,	0.955423,	0.295242},	{-0.262866,	0.951056,	0.162460},	{0.238856,	0.864188,	0.442863},
	{0.262866,	0.951056,	0.162460},	{0.500000,	0.809017,	0.309017},	{0.238856,	0.864188,	-0.442863},
	{0.262866,	0.951056,	-0.162460},	{0.500000,	0.809017,	-0.309017},	{0.850651,	0.525731,	0.000000},
	{0.716567,	0.681718,	0.147621},	{0.716567,	0.681718,	-0.147621},	{0.525731,	0.850651,	0.000000},
	{0.425325,	0.688191,	0.587785},	{0.864188,	0.442863,	0.238856},	{0.688191,	0.587785,	0.425325},
	{0.809017,	0.309017,	0.500000},	{0.681718,	0.147621,	0.716567},	{0.587785,	0.425325,	0.688191},
	{0.955423,	0.295242,	0.000000},	{1.000000,	0.000000,	0.000000},	{0.951056,	0.162460,	0.262866},
	{0.850651,	-0.525731,	0.000000},	{0.955423,	-0.295242,	0.000000},	{0.864188,	-0.442863,	0.238856},
	{0.951056,	-0.162460,	0.262866},	{0.809017,	-0.309017,	0.500000},	{0.681718,	-0.147621,	0.716567},
	{0.850651,	0.000000,	0.525731},	{0.864188,	0.442863,	-0.238856},	{0.809017,	0.309017,	-0.500000},
	{0.951056,	0.162460,	-0.262866},	{0.525731,	0.000000,	-0.850651},	{0.681718,	0.147621,	-0.716567},
	{0.681718,	-0.147621,	-0.716567},	{0.850651,	0.000000,	-0.525731},	{0.809017,	-0.309017,	-0.500000},
	{0.864188,	-0.442863,	-0.238856},	{0.951056,	-0.162460,	-0.262866},	{0.147621,	0.716567,	-0.681718},
	{0.309017,	0.500000,	-0.809017},	{0.425325,	0.688191,	-0.587785},	{0.442863,	0.238856,	-0.864188},
	{0.587785,	0.425325,	-0.688191},	{0.688191,	0.587785,	-0.425325},	{-0.147621,	0.716567,	-0.681718},
	{-0.309017,	0.500000,	-0.809017},	{0.000000,	0.525731,	-0.850651},	{-0.525731,	0.000000,	-0.850651},
	{-0.442863,	0.238856,	-0.864188},	{-0.295242,	0.000000,	-0.955423},	{-0.162460,	0.262866,	-0.951056},
	{0.000000,	0.000000,	-1.000000},	{0.295242,	0.000000,	-0.955423},	{0.162460,	0.262866,	-0.951056},
	{-0.442863,	-0.238856,	-0.864188},	{-0.309017,	-0.500000,	-0.809017},	{-0.162460,	-0.262866,	-0.951056},
	{0.000000,	-0.850651,	-0.525731},	{-0.147621,	-0.716567,	-0.681718},	{0.147621,	-0.716567,	-0.681718},
	{0.000000,	-0.525731,	-0.850651},	{0.309017,	-0.500000,	-0.809017},	{0.442863,	-0.238856,	-0.864188},
	{0.162460,	-0.262866,	-0.951056},	{0.238856,	-0.864188,	-0.442863},	{0.500000,	-0.809017,	-0.309017},
	{0.425325,	-0.688191,	-0.587785},	{0.716567,	-0.681718,	-0.147621},	{0.688191,	-0.587785,	-0.425325},
	{0.587785,	-0.425325,	-0.688191},	{0.000000,	-0.955423,	-0.295242},	{0.000000,	-1.000000,	0.000000},
	{0.262866,	-0.951056,	-0.162460},	{0.000000,	-0.850651,	0.525731},	{0.000000,	-0.955423,	0.295242},
	{0.238856,	-0.864188,	0.442863},	{0.262866,	-0.951056,	0.162460},	{0.500000,	-0.809017,	0.309017},
	{0.716567,	-0.681718,	0.147621},	{0.525731,	-0.850651,	0.000000},	{-0.238856,	-0.864188,	-0.442863},
	{-0.500000,	-0.809017,	-0.309017},	{-0.262866,	-0.951056,	-0.162460},	{-0.850651,	-0.525731,	0.000000},
	{-0.716567,	-0.681718,	-0.147621},	{-0.716567,	-0.681718,	0.147621},	{-0.525731,	-0.850651,	0.000000},
	{-0.500000,	-0.809017,	0.309017},	{-0.238856,	-0.864188,	0.442863},	{-0.262866,	-0.951056,	0.162460},
	{-0.864188,	-0.442863,	0.238856},	{-0.809017,	-0.309017,	0.500000},	{-0.688191,	-0.587785,	0.425325},
	{-0.681718,	-0.147621,	0.716567},	{-0.442863,	-0.238856,	0.864188},	{-0.587785,	-0.425325,	0.688191},
	{-0.309017,	-0.500000,	0.809017},	{-0.147621,	-0.716567,	0.681718},	{-0.425325,	-0.688191,	0.587785},
	{-0.162460,	-0.262866,	0.951056},	{0.442863,	-0.238856,	0.864188},	{0.162460,	-0.262866,	0.951056},
	{0.309017,	-0.500000,	0.809017},	{0.147621,	-0.716567,	0.681718},	{0.000000,	-0.525731,	0.850651},
	{0.425325,	-0.688191,	0.587785},	{0.587785,	-0.425325,	0.688191},	{0.688191,	-0.587785,	0.425325},
	{-0.955423,	0.295242,	0.000000},	{-0.951056,	0.162460,	0.262866},	{-1.000000,	0.000000,	0.000000},
	{-0.850651,	0.000000,	0.525731},	{-0.955423,	-0.295242,	0.000000},	{-0.951056,	-0.162460,	0.262866},
	{-0.864188,	0.442863,	-0.238856},	{-0.951056,	0.162460,	-0.262866},	{-0.809017,	0.309017,	-0.500000},
	{-0.864188,	-0.442863,	-0.238856},	{-0.951056,	-0.162460,	-0.262866},	{-0.809017,	-0.309017,	-0.500000},
	{-0.681718,	0.147621,	-0.716567},	{-0.681718,	-0.147621,	-0.716567},	{-0.850651,	0.000000,	-0.525731},
	{-0.688191,	0.587785,	-0.425325},	{-0.587785,	0.425325,	-0.688191},	{-0.425325,	0.688191,	-0.587785},
	{-0.425325,	-0.688191,	-0.587785},	{-0.587785,	-0.425325,	-0.688191},	{-0.688191,	-0.587785,	-0.425325}
};

/*
=================
DirToByte

This isn't a real cheap function to call!
=================
*/
int DirToByte (const vec3_t dir) {
	int		i, best = 0;
	float	d, bestd = 0;

	if (!dir)
		return 0;

	for (i=0 ; i<NUMVERTEXNORMALS ; i++) {
		d = DotProduct (dir, byteDirs[i]);
		if (d > bestd) {
			bestd = d;
			best = i;
		}
	}

	return best;
}


/*
=================
ByteToDir
=================
*/
void ByteToDir (int b, vec3_t dir) {
	if ((b < 0) || (b >= NUMVERTEXNORMALS)) {
		VectorClear (dir);
		return;
	}

	VectorCopy (byteDirs[b], dir);
}

// ===========================================================

/*
===============
_CrossProduct
===============
*/
void _CrossProduct (const vec3_t v1, const vec3_t v2, vec3_t cross) {
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}


/*
===============
_DotProduct
===============
*/
vec_t _DotProduct (const vec3_t v1, const vec3_t v2) {
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

// ===========================================================

/*
===============
_VectorAdd
===============
*/
void _VectorAdd (const vec3_t veca, const vec3_t vecb, vec3_t out) {
	out[0] = veca[0] + vecb[0];
	out[1] = veca[1] + vecb[1];
	out[2] = veca[2] + vecb[2];
}


/*
===============
_VectorAverage
===============
*/
float _VectorAverage (const vec3_t vec) {
	return (vec[0] + vec[1] + vec[2]) / 3.0;
}


/*
===============
_VectorCompare
===============
*/
qBool _VectorCompare (const vec3_t v1, const vec3_t v2) {
	if ((v1[0] != v2[0]) || (v1[1] != v2[1]) || (v1[2] != v2[2]))
			return qFalse;
			
	return qTrue;
}


/*
===============
_VectorCopy
===============
*/
void _VectorCopy (const vec3_t in, vec3_t out) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}


/*
===============
_VectorDistance
===============
*/
vec_t _VectorDistance (const vec3_t vec1, const vec3_t vec2) {
	vec3_t	v;

	VectorSubtract (vec1, vec2, v);

	return sqrt (DotProduct(v, v));
}

/*
===============
_VectorClear
===============
*/
void _VectorClear (vec3_t vec) {
	vec[0] = 0;
	vec[1] = 0;
	vec[2] = 0;
}


/*
===============
_VectorLength
===============
*/
vec_t _VectorLength (vec3_t v) {
	return sqrt (DotProduct(v, v));
}


/*
===============
_VectorMA
===============
*/
void _VectorMA (const vec3_t veca, const vec_t scale, const vec3_t vecb, vec3_t out) {
	out[0] = veca[0] + (scale * vecb[0]);
	out[1] = veca[1] + (scale * vecb[1]);
	out[2] = veca[2] + (scale * vecb[2]);
}


/*
===============
_VectorNegate
===============
*/
void _VectorNegate (const vec3_t in, vec3_t out) {
	out[0] = -in[0];
	out[1] = -in[1];
	out[2] = -in[2];
}


/*
===============
_VectorScale
===============
*/
void _VectorScale (const vec3_t in, const vec_t scale, vec3_t out) {
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
}


/*
===============
_VectorSet
===============
*/
void _VectorSet (vec3_t out, const vec_t vec0, const vec_t vec1, const vec_t vec2) {
	out[0] = vec0;
	out[1] = vec1;
	out[2] = vec2;
}


/*
===============
_VectorSubtract
===============
*/
void _VectorSubtract (const vec3_t veca, const vec3_t vecb, vec3_t out) {
	out[0] = veca[0] - vecb[0];
	out[1] = veca[1] - vecb[1];
	out[2] = veca[2] - vecb[2];
}


// ===========================================================

/*
===============
_VectorDistanceFast
===============
*/
vec_t _VectorDistanceFast (const vec3_t vec1, const vec3_t vec2) {
	vec3_t	v;

	VectorSubtract (vec1, vec2, v);

	return Q_FastSqrt (DotProduct(v, v));
}


/*
===============
_VectorLengthFast
===============
*/
vec_t _VectorLengthFast (vec3_t v) {
	return Q_FastSqrt (DotProduct(v, v));
}

// ===========================================================

/*
===============
_Vector4Add
===============
*/
void _Vector4Add (const vec4_t veca, const vec4_t vecb, vec4_t out) {
	out[0] = veca[0] + vecb[0];
	out[1] = veca[1] + vecb[1];
	out[2] = veca[2] + vecb[2];
	out[3] = veca[3] + vecb[3];
}


/*
===============
_Vector4Average
===============
*/
float _Vector4Average (const vec4_t vec) {
	return (vec[0] + vec[1] + vec[2] + vec[3]) / 4.0;
}


/*
===============
_Vector4Copy
===============
*/
void _Vector4Copy (const vec4_t in, vec4_t out) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
	out[3] = in[3];
}


/*
===============
_Vector4Identity
===============
*/
void _Vector4Identity (vec4_t vec) {
	vec[0] = 0;
	vec[1] = 0;
	vec[2] = 0;
	vec[3] = 0;
}


/*
===============
_Vector4MA
===============
*/
void _Vector4MA (const vec4_t veca, const vec_t scale, const vec4_t vecb, vec4_t out) {
	out[0] = veca[0] + ( scale * vecb[0] );
	out[1] = veca[1] + ( scale * vecb[1] );
	out[2] = veca[2] + ( scale * vecb[2] );
	out[3] = veca[3] + ( scale * vecb[4] );
}


/*
===============
_Vector4Negate
===============
*/
void _Vector4Negate (const vec4_t in, vec4_t out) {
	out[0] = -in[0];
	out[1] = -in[1];
	out[2] = -in[2];
	out[3] = -in[3];
}


/*
===============
_Vector4Scale
===============
*/
void _Vector4Scale (const vec4_t in, const vec_t scale, vec4_t out) {
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
	out[3] = in[3]*scale;
}


/*
===============
_Vector4Set
===============
*/
void _Vector4Set (vec4_t out, const vec_t vec0, const vec_t vec1, const vec_t vec2, const vec_t vec3) {
	out[0] = vec0;
	out[1] = vec1;
	out[2] = vec2;
	out[3] = vec3;
}


/*
===============
_Vector4Subtract
===============
*/
void _Vector4Subtract (const vec4_t veca, const vec4_t vecb, vec4_t out) {
	out[0] = veca[0] - vecb[0];
	out[1] = veca[1] - vecb[1];
	out[2] = veca[2] - vecb[2];
	out[3] = veca[3] - vecb[3];
}

// ===========================================================

/*
===============
Q_ftol
===============
*/
#ifdef id386
__declspec(naked) long Q_ftol (float f) {
	static int	tmp;
	_asm {
		fld dword ptr [esp+4]
		fistp tmp
		mov eax, tmp
		ret
	}
}
#endif // id386

/*
===============
Q_FastSqrt

5% margin of error
===============
*/
#ifdef id386
float Q_FastSqrt (float value) {
	float result;
	_asm {
		mov eax, value
		sub eax, 0x3f800000
		sar eax, 1
		add eax, 0x3f800000
		mov result, eax
	}
	return result;
}
#endif // id386


/*
===============
Q_RSqrt

LordHavoc's creation
1/sqrt faster but not as precise
===============
*/
float Q_RSqrt (float number) {
	float	y;

	if (number == 0.0f)
		return 0.0f;
	*((int *)&y) = 0x5f3759df - ((* (int *) &number) >> 1);
	return y * (1.5f - (number * 0.5f * y * y));
}


/*
===============
Q_log2
===============
*/
int Q_log2 (int val) {
	int answer = 0;
	while (val >>= 1)
		answer++;
	return answer;
}

/*
===============
Q_NearestPow
===============
*/
void Q_NearestPow (int *var, const qBool oneLess) {
	uInt	i;

	for (i=1 ; i<*var ; i<<=1) ;

	if (oneLess) {
		// find the nearest power of two below input value
		if (i > *var)
			i >>= 1;
	}

	*var = i;
}

// ===========================================================

/*
===============
NormToLatLong
===============
*/
void NormToLatLong (const vec3_t normal, byte latlong[2]) {
	if ((normal[0] == 0) && (normal[1] == 0)) {
		if (normal[2] > 0) {
			latlong[0] = 0;
			latlong[1] = 0;
		} else {
			latlong[0] = 128;
			latlong[1] = 0;
		}
	} else {
		int		angle;

		angle = (int)(acos (normal[2]) * 255.0 / M_PI_TIMES2) & 0xff;
		latlong[0] = angle;
		angle = (int)(atan2 (normal[1], normal[0]) * 255.0 / M_PI_TIMES2) & 0xff;
		latlong[1] = angle;
	}
}


/*
===============
MakeNormalVectors
===============
*/
void MakeNormalVectors (const vec3_t forward, vec3_t right, vec3_t up) {
	float		d;

	// this rotate and negate guarantees a vector not colinear with the original
	VectorSet (right, forward[2], -forward[0], forward[1]);

	d = DotProduct (right, forward);
	VectorMA (right, -d, forward, right);
	VectorNormalize (right, right);
	CrossProduct (right, forward, up);
}


/*
===============
PerpendicularVector

assumes "src" is normalized
===============
*/
void PerpendicularVector (const vec3_t src, vec3_t dst) {
	int		pos, i;
	float	minelem = 1.0F;
	vec3_t	tempvec;

	// find the smallest magnitude axially aligned vector
	for (pos=0, i=0 ; i<3 ; i++) {
		if (fabs (src[i]) < minelem) {
			pos = i;
			minelem = fabs (src[i]);
		}
	}

	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	// project the point onto the plane defined by src
	ProjectPointOnPlane (dst, tempvec, src);

	// normalize the result
	VectorNormalize (dst, dst);
}


/*
===============
VectorNormalize
===============
*/
vec_t VectorNormalize (const vec3_t in, vec3_t out) {
	vec_t	length, invLength;

	length = VectorLength (in);

	if (length) {
		invLength = 1.0/length;
		VectorScale (in, invLength, out);
	}
		
	return length;
}


/*
===============
RotatePointAroundVector
===============
*/
void RotatePointAroundVector (vec3_t dst, const vec3_t dir, const vec3_t point, float degrees) {
	float		t0, t1;
	float		c, s;
	vec3_t		vr, vu, vf;

	s = DEG2RAD (degrees);
	c = cos (s);
	s = sin (s);

	VectorCopy (dir, vf);
	MakeNormalVectors (vf, vr, vu);

	t0 = vr[0] * c + vu[0] * -s;
	t1 = vr[0] * s + vu[0] *  c;
	dst[0] = (t0 * vr[0] + t1 * vu[0] + vf[0] * vf[0]) * point[0]
			+ (t0 * vr[1] + t1 * vu[1] + vf[0] * vf[1]) * point[1]
			+ (t0 * vr[2] + t1 * vu[2] + vf[0] * vf[2]) * point[2];

	t0 = vr[1] * c + vu[1] * -s;
	t1 = vr[1] * s + vu[1] *  c;
	dst[1] = (t0 * vr[0] + t1 * vu[0] + vf[1] * vf[0]) * point[0]
			+ (t0 * vr[1] + t1 * vu[1] + vf[1] * vf[1]) * point[1]
			+ (t0 * vr[2] + t1 * vu[2] + vf[1] * vf[2]) * point[2];

	t0 = vr[2] * c + vu[2] * -s;
	t1 = vr[2] * s + vu[2] *  c;
	dst[2] = (t0 * vr[0] + t1 * vu[0] + vf[2] * vf[0]) * point[0]
			+ (t0 * vr[1] + t1 * vu[1] + vf[2] * vf[1]) * point[1]
			+ (t0 * vr[2] + t1 * vu[2] + vf[2] * vf[2]) * point[2];
}

// ===========================================================

/*
================
R_ConcatRotations
================
*/
void R_ConcatRotations (const vec3_t in1[3], const vec3_t in2[3], vec3_t out[3]) {
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];

	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];

	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
}


/*
================
R_ConcatTransforms
================
*/
void R_ConcatTransforms (const float in1[3][4], const float in2[3][4], float out[3][4]) {
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
	out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] + in1[0][2] * in2[2][3] + in1[0][3];

	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
	out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] + in1[1][2] * in2[2][3] + in1[1][3];

	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
	out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] + in1[2][2] * in2[2][3] + in1[2][3];
}

// ===========================================================

/*
===============
AngleMod
===============
*/
float AngleMod (const float a) {
	return (360.0/65536) * ((int)(a*(65536/360.0)) & 65535);
}


/*
===============
AnglesToAxis
===============
*/
void AnglesToAxis (const vec3_t angles, vec3_t axis[3]) {
	AngleVectors (angles, axis[0], axis[1], axis[2]);
	VectorNegate (axis[1], axis[1]);
}


/*
===============
AngleVectors
===============
*/
void AngleVectors (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up) {
	float			angle;
	static float	sr, sp, sy, cr, cp, cy;
	// static to help MS compiler fp bugs

	angle = DEG2RAD (angles[YAW]);
	sy = sin (angle);
	cy = cos (angle);
	angle = DEG2RAD (angles[PITCH]);
	sp = sin (angle);
	cp = cos (angle);
	angle = DEG2RAD (angles[ROLL]);
	sr = sin (angle);
	cr = cos (angle);

	if (forward) {
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
	}
	if (right) {
		right[0] = (-1 * sr * sp * cy + -1 * cr * -sy);
		right[1] = (-1 * sr * sp * sy + -1 * cr * cy);
		right[2] = -1 * sr * cp;
	}
	if (up) {
		up[0] = (cr * sp * cy + -sr * -sy);
		up[1] = (cr * sp * sy + -sr * cy);
		up[2] = cr * cp;
	}
}


/*
===============
InterpolateAngle
===============
*/
float InterpolateAngle (const float a2, float a1, const float frac) {
	if (a1 - a2 > 180)
		a1 -= 360;
	if (a1 - a2 < -180)
		a1 += 360;

	return a2 + frac * (a1 - a2);
}


/*
===============
VecToAngles
===============
*/
void VecToAngles (const vec3_t vec, vec3_t angles) {
	float	forward;
	float	yaw, pitch;
	
	if ((vec[1] == 0) && (vec[0] == 0)) {
		yaw = 0;
		if (vec[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	} else {
		// PMM - fixed to correct for pitch of 0
		if (vec[0])
			yaw = (atan2 (vec[1], vec[0]) * M_180DIV_PI);
		else if (vec[1] > 0)
			yaw = 90;
		else
			yaw = 270;

		if (yaw < 0)
			yaw += 360;

		forward = sqrt (vec[0] * vec[0] + vec[1] * vec[1]);
		pitch = (atan2 (vec[2], forward) * M_180DIV_PI);
		if (pitch < 0)
			pitch += 360;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

// ===========================================================

/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
int BoxOnPlaneSide (const vec3_t mins, const vec3_t maxs, const cBspPlane_t *p) {
	float	dist1, dist2;
	int		sides;

	// fast axial cases
	if (p->type < 3) {
		if (p->dist <= mins[p->type])
			return 1;
		if (p->dist >= maxs[p->type])
			return 2;
		return 3;
	}
	
	// general case
	switch (p->signBits) {
	case 0:
		dist1 = p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2];
		dist2 = p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2];
		break;

	case 1:
		dist1 = p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2];
		dist2 = p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2];
		break;

	case 2:
		dist1 = p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2];
		dist2 = p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2];
		break;

	case 3:
		dist1 = p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2];
		dist2 = p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2];
		break;

	case 4:
		dist1 = p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2];
		dist2 = p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2];
		break;

	case 5:
		dist1 = p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2];
		dist2 = p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2];
		break;

	case 6:
		dist1 = p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2];
		dist2 = p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2];
		break;

	case 7:
		dist1 = p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2];
		dist2 = p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2];
		break;

	default:
		dist1 = 0;	// shut up compiler
		dist2 = 0;	// shut up compiler
		break;
	}

	sides = 0;
	if (dist1 >= p->dist)
		sides = 1;
	if (dist2 < p->dist)
		sides |= 2;

	return sides;
}

/*
=================
PlaneTypeForNormal
=================
*/
int	PlaneTypeForNormal (const vec3_t normal) {
	vec_t	ax, ay, az;
	
	// NOTE: should these have an epsilon around 1.0?		
	if (normal[0] >= 1.0)
		return PLANE_X;
	if (normal[1] >= 1.0)
		return PLANE_Y;
	if (normal[2] >= 1.0)
		return PLANE_Z;
		
	ax = fabs (normal[0]);
	ay = fabs (normal[1]);
	az = fabs (normal[2]);

	if ((ax >= ay) && (ax >= az))
		return PLANE_ANYX;
	if ((ay >= ax) && (ay >= az))
		return PLANE_ANYY;
	return PLANE_ANYZ;
}


/*
===============
ProjectPointOnPlane
===============
*/
void ProjectPointOnPlane (vec3_t dst, const vec3_t p, const vec3_t normal) {
	float	d;
	vec3_t	n;
	float	inv_denom;

	inv_denom = 1.0F / DotProduct (normal, normal);

	d = DotProduct (normal, p) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}


/*
===============
SignbitsForPlane
===============
*/
int SignbitsForPlane (cBspPlane_t *out) {
	int	bits, j;

	// for fast box on planeside test
	bits = 0;
	for (j=0 ; j<3 ; j++) {
		if (out->normal[j] < 0)
			bits |= 1<<j;
	}
	return bits;
}

// ===========================================================

/*
===============
AddPointToBounds
===============
*/
void AddPointToBounds (const vec3_t v, vec3_t mins, vec3_t maxs) {
	int		i;

	for (i=0 ; i<3 ; i++) {
		if (v[i] < mins[i])
			mins[i] = v[i];
		if (v[i] > maxs[i])
			maxs[i] = v[i];
	}
}


/*
===============
ClearBounds
===============
*/
void ClearBounds (vec3_t mins, vec3_t maxs) {
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}


/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds (const vec3_t mins, const vec3_t maxs) {
	int		i;
	vec3_t	corner;

	for (i=0 ; i<3 ; i++)
		corner[i] = fabs (mins[i]) > fabs (maxs[i]) ? fabs (mins[i]) : fabs (maxs[i]);

	return VectorLength (corner);
}

// ===========================================================

/*
====================
CalcFov
====================
*/
float CalcFov (const float fov_x, const float width, const float height) {
	if ((fov_x < 1) || (fov_x > 179))
		Com_Printf (0, S_COLOR_RED "Bad fov: %f\n", fov_x);

	return (atan (height / (width / tan (fov_x / 360.0 * M_PI)))) * (M_180DIV_PI * 2);
}

/*
==============================================================================

	MATRIX FUNCTIONS

	Thanks to Vic
==============================================================================
*/

/*
===============
Matrix_Compare
===============
*/
qBool Matrix_Compare (const vec3_t a[3], const vec3_t b[3]) {
	return (VectorCompare (a[0], b[0]) && VectorCompare (a[1], b[1]) && VectorCompare (a[2], b[2]));
}


/*
===============
Matrix_Copy
===============
*/
void Matrix_Copy (const vec3_t in[3], vec3_t out[3]) {
	VectorCopy (in[0], out[0]);
	VectorCopy (in[1], out[1]);
	VectorCopy (in[2], out[2]);
}


/*
===============
Matrix_EulerAngles
===============
*/
void Matrix_EulerAngles (vec3_t mat[3], vec3_t angles) {
	float	c;
	float	pitch, yaw, roll;

	pitch = -asin (mat[0][2]);
	c = cos (pitch);
	pitch = RAD2DEG (pitch);

	if (fabs (c) > 0.005) {			// Gimball lock?
		c = 1.0f / c;
		yaw = RAD2DEG (atan2 ((-1)*-mat[0][1] * c, mat[0][0] * c));
		roll = RAD2DEG (atan2 (-mat[1][2] * c, mat[2][2] * c));
	} else {
		if (mat[0][2] > 0)
			pitch = -90;
		else
			pitch = 90;
		yaw = RAD2DEG (atan2 (mat[1][0], (-1)*mat[1][1]));
		roll = 0;
	}

	angles[PITCH] = AngleMod (pitch);
	angles[YAW] = AngleMod (yaw);
	angles[ROLL] = AngleMod (roll);
}


/*
===============
Matrix_FromPoints
===============
*/
void Matrix_FromPoints (const vec3_t v1, const vec3_t v2, const vec3_t v3, vec3_t m[3]) {
	float		d;

	m[2][0] = (v1[1] - v2[1]) * (v3[2] - v2[2]) - (v1[2] - v2[2]) * (v3[1] - v2[1]);
	m[2][1] = (v1[2] - v2[2]) * (v3[0] - v2[0]) - (v1[0] - v2[0]) * (v3[2] - v2[2]);
	m[2][2] = (v1[0] - v2[0]) * (v3[1] - v2[1]) - (v1[1] - v2[1]) * (v3[0] - v2[0]);
	VectorNormalize (m[2], m[2]);

	// this rotate and negate guarantees a vector not colinear with the original
	VectorSet (m[1], m[2][2], -m[2][0], m[2][1]);
	d = -DotProduct (m[1], m[2]);
	VectorMA (m[1], d, m[2], m[1]);
	VectorNormalize (m[1], m[1]);
	CrossProduct (m[1], m[2], m[0]);
}


/*
===============
Matrix_Identity
===============
*/
void Matrix_Identity (vec3_t mat[3]) {
	int i, j;

	for (i=0 ; i<3 ; i++) {
		for (j=0 ; j<3 ; j++) {
			if (i == j)
				mat[i][j] = 1.0;
			else
				mat[i][j] = 0.0;
		}
	}
}


/*
===============
Matrix_Multiply
===============
*/
void Matrix_Multiply (const vec3_t in1[3], const vec3_t in2[3], vec3_t out[3]) {
	out[0][0] = in1[0][0]*in2[0][0] + in1[0][1]*in2[1][0] + in1[0][2]*in2[2][0];
	out[0][1] = in1[0][0]*in2[0][1] + in1[0][1]*in2[1][1] + in1[0][2]*in2[2][1];
	out[0][2] = in1[0][0]*in2[0][2] + in1[0][1]*in2[1][2] + in1[0][2]*in2[2][2];
	out[1][0] = in1[1][0]*in2[0][0] + in1[1][1]*in2[1][0] +	in1[1][2]*in2[2][0];
	out[1][1] = in1[1][0]*in2[0][1] + in1[1][1]*in2[1][1] + in1[1][2]*in2[2][1];
	out[1][2] = in1[1][0]*in2[0][2] + in1[1][1]*in2[1][2] +	in1[1][2]*in2[2][2];
	out[2][0] = in1[2][0]*in2[0][0] + in1[2][1]*in2[1][0] +	in1[2][2]*in2[2][0];
	out[2][1] = in1[2][0]*in2[0][1] + in1[2][1]*in2[1][1] +	in1[2][2]*in2[2][1];
	out[2][2] = in1[2][0]*in2[0][2] + in1[2][1]*in2[1][2] +	in1[2][2]*in2[2][2];
}


/*
===============
Matrix_Rotate
===============
*/
void Matrix_Rotate (vec3_t a[3], float angle, float x, float y, float z) {
	vec3_t	m[3], b[3];
	float	c = cos (DEG2RAD (angle));
	float	s = sin (DEG2RAD (angle));
	float	mc = 1 - c, t1, t2;
	
	m[0][0] = (x * x * mc) + c;
	m[1][1] = (y * y * mc) + c;
	m[2][2] = (z * z * mc) + c;

	t1 = y * x * mc;
	t2 = z * s;
	m[0][1] = t1 + t2;
	m[1][0] = t1 - t2;

	t1 = x * z * mc;
	t2 = y * s;
	m[0][2] = t1 - t2;
	m[2][0] = t1 + t2;

	t1 = y * z * mc;
	t2 = x * s;
	m[1][2] = t1 + t2;
	m[2][1] = t1 - t2;

	Matrix_Copy (a, b);
	Matrix_Multiply (b, m, a);
}


/*
===============
Matrix_TransformVector
===============
*/
void Matrix_TransformVector (const vec3_t m[3], const vec3_t v, vec3_t out) {
	out[0] = m[0][0]*v[0] + m[0][1]*v[1] + m[0][2]*v[2];
	out[1] = m[1][0]*v[0] + m[1][1]*v[1] + m[1][2]*v[2];
	out[2] = m[2][0]*v[0] + m[2][1]*v[1] + m[2][2]*v[2];
}


/*
===============
Matrix_Transpose
===============
*/
void Matrix_Transpose (const vec3_t in[3], vec3_t out[3]) {
	out[0][0] = in[0][0];
	out[1][1] = in[1][1];
	out[2][2] = in[2][2];

	out[0][1] = in[1][0];
	out[0][2] = in[2][0];
	out[1][0] = in[0][1];
	out[1][2] = in[2][1];
	out[2][0] = in[0][2];
	out[2][1] = in[1][2];
}

// ===========================================================

/*
===============
Matrix4_Compare
===============
*/
qBool Matrix4_Compare (const mat4x4_t a, const mat4x4_t b) {
	int		i;

	for (i=0 ; i<16 ; i++) {
		if (a[i] != b[i])
			return qFalse;
	}

	return qTrue;
}


/*
===============
Matrix4_Copy
===============
*/
void Matrix4_Copy (const mat4x4_t a, mat4x4_t b) {
	int		i;

	for (i=0 ; i<16 ; i++)
		b[i] = a[i];
}


/*
===============
Matrix4_Identity
===============
*/
void Matrix4_Identity (mat4x4_t mat) {
	Matrix4_Copy (mat4x4Identity, mat);
}


/*
===============
Matrix4_Matrix3
===============
*/
void Matrix4_Matrix3 (const mat4x4_t in, vec3_t out[3]) {
	out[0][0] = in[0 ];
	out[0][1] = in[4 ];
	out[0][2] = in[8 ];

	out[1][0] = in[1 ];
	out[1][1] = in[5 ];
	out[1][2] = in[9 ];

	out[2][0] = in[2 ];
	out[2][1] = in[6 ];
	out[2][2] = in[10];
}


/*
===============
Matrix4_Multiply
===============
*/
void Matrix4_Multiply (const mat4x4_t a, const mat4x4_t b, mat4x4_t product) {
	product[0 ] = a[0]*b[0 ] + a[4]*b[1 ] + a[8 ]*b[2 ] + a[12]*b[3 ];
	product[1 ] = a[1]*b[0 ] + a[5]*b[1 ] + a[9 ]*b[2 ] + a[13]*b[3 ];
	product[2 ] = a[2]*b[0 ] + a[6]*b[1 ] + a[10]*b[2 ] + a[14]*b[3 ];
	product[3 ] = a[3]*b[0 ] + a[7]*b[1 ] + a[11]*b[2 ] + a[15]*b[3 ];
	product[4 ] = a[0]*b[4 ] + a[4]*b[5 ] + a[8 ]*b[6 ] + a[12]*b[7 ];
	product[5 ] = a[1]*b[4 ] + a[5]*b[5 ] + a[9 ]*b[6 ] + a[13]*b[7 ];
	product[6 ] = a[2]*b[4 ] + a[6]*b[5 ] + a[10]*b[6 ] + a[14]*b[7 ];
	product[7 ] = a[3]*b[4 ] + a[7]*b[5 ] + a[11]*b[6 ] + a[15]*b[7 ];
	product[8 ] = a[0]*b[8 ] + a[4]*b[9 ] + a[8 ]*b[10] + a[12]*b[11];
	product[9 ] = a[1]*b[8 ] + a[5]*b[9 ] + a[9 ]*b[10] + a[13]*b[11];
	product[10] = a[2]*b[8 ] + a[6]*b[9 ] + a[10]*b[10] + a[14]*b[11];
	product[11] = a[3]*b[8 ] + a[7]*b[9 ] + a[11]*b[10] + a[15]*b[11];
	product[12] = a[0]*b[12] + a[4]*b[13] + a[8 ]*b[14] + a[12]*b[15];
	product[13] = a[1]*b[12] + a[5]*b[13] + a[9 ]*b[14] + a[13]*b[15];
	product[14] = a[2]*b[12] + a[6]*b[13] + a[10]*b[14] + a[14]*b[15];
	product[15] = a[3]*b[12] + a[7]*b[13] + a[11]*b[14] + a[15]*b[15];
}


/*
===============
Matrix4_Multiply_Vec3
===============
*/
void Matrix4_Multiply_Vec3 (const mat4x4_t m, const vec3_t v, vec3_t out) {
	out[0] = m[0]*v[0] + m[1]*v[1] + m[2 ]*v[2];
	out[1] = m[4]*v[0] + m[5]*v[1] + m[6 ]*v[2];
	out[2] = m[8]*v[0] + m[9]*v[1] + m[10]*v[2];
}


/*
===============
Matrix4_Multiply_Vec4
===============
*/
void Matrix4_Multiply_Vec4 (const mat4x4_t m, const vec4_t v, vec4_t out) {
	out[0] = m[0]*v[0] + m[4]*v[1] + m[8 ]*v[2] + m[12]*v[3];
	out[1] = m[1]*v[0] + m[5]*v[1] + m[9 ]*v[2] + m[13]*v[3];
	out[2] = m[2]*v[0] + m[6]*v[1] + m[10]*v[2] + m[14]*v[3];
	out[3] = m[3]*v[0] + m[7]*v[1] + m[11]*v[2] + m[15]*v[3];
}

/*
===============
Matrix4_MultiplyFast
===============
*/
void Matrix4_MultiplyFast (mat4x4_t a, mat4x4_t b, mat4x4_t product) {
	product[0]  = a[0] * b[0 ] + a[4] * b[1 ] + a[8 ] * b[2];
	product[1]  = a[1] * b[0 ] + a[5] * b[1 ] + a[9 ] * b[2];
	product[2]  = a[2] * b[0 ] + a[6] * b[1 ] + a[10] * b[2];
	product[3]  = 0.0f;
	product[4]  = a[0] * b[4 ] + a[4] * b[5 ] + a[8 ] * b[6];
	product[5]  = a[1] * b[4 ] + a[5] * b[5 ] + a[9 ] * b[6];
	product[6]  = a[2] * b[4 ] + a[6] * b[5 ] + a[10] * b[6];
	product[7]  = 0.0f;
	product[8]  = a[0] * b[8 ] + a[4] * b[9 ] + a[8 ] * b[10];
	product[9]  = a[1] * b[8 ] + a[5] * b[9 ] + a[9 ] * b[10];
	product[10] = a[2] * b[8 ] + a[6] * b[9 ] + a[10] * b[10];
	product[11] = 0.0f;
	product[12] = a[0] * b[12] + a[4] * b[13] + a[8 ] * b[14] + a[12];
	product[13] = a[1] * b[12] + a[5] * b[13] + a[9 ] * b[14] + a[13];
	product[14] = a[2] * b[12] + a[6] * b[13] + a[10] * b[14] + a[14];
	product[15] = 1.0f;
}


/*
===============
Matrix4_Rotate
===============
*/
void Matrix4_Rotate (mat4x4_t a, float angle, float x, float y, float z) {
	mat4x4_t	m, b;
	float	c = cos (DEG2RAD (angle));
	float	s = sin (DEG2RAD (angle));
	float	mc = 1 - c, t1, t2;
	
	m[0]  = (x * x * mc) + c;
	m[5]  = (y * y * mc) + c;
	m[10] = (z * z * mc) + c;

	t1 = y * x * mc;
	t2 = z * s;
	m[1] = t1 + t2;
	m[4] = t1 - t2;

	t1 = x * z * mc;
	t2 = y * s;
	m[2] = t1 - t2;
	m[8] = t1 + t2;

	t1 = y * z * mc;
	t2 = x * s;
	m[6] = t1 + t2;
	m[9] = t1 - t2;

	m[3] = m[7] = m[11] = m[12] = m[13] = m[14] = 0;
	m[15] = 1;

	Matrix4_Copy (a, b);
	Matrix4_MultiplyFast (b, m, a);
}


/*
===============
Matrix4_Scale
===============
*/
void Matrix4_Scale (mat4x4_t m, float x, float y, float z) {
	m[0] *= x;		m[4] *= y;		m[8 ] *= z;
	m[1] *= x;		m[5] *= y;		m[9 ] *= z;
	m[2] *= x;		m[6] *= y;		m[10] *= z;
	m[3] *= x;		m[7] *= y;		m[11] *= z;
}


/*
===============
Matrix4_Translate
===============
*/
void Matrix4_Translate (mat4x4_t m, float x, float y, float z) {
	m[12] = m[0] * x + m[4] * y + m[8 ] * z + m[12];
	m[13] = m[1] * x + m[5] * y + m[9 ] * z + m[13];
	m[14] = m[2] * x + m[6] * y + m[10] * z + m[14];
	m[15] = m[3] * x + m[7] * y + m[11] * z + m[15];
}

/*
===============
Matrix4_Transpose
===============
*/
void Matrix4_Transpose (mat4x4_t m, mat4x4_t ret) {
	ret[0 ] = m[0]; ret[1 ] = m[4]; ret[2 ] = m[8 ]; ret[3 ] = m[12];
	ret[4 ] = m[1]; ret[5 ] = m[5]; ret[6 ] = m[9 ]; ret[7 ] = m[13];
	ret[8 ] = m[2]; ret[9 ] = m[6]; ret[10] = m[10]; ret[11] = m[14];
	ret[12] = m[3]; ret[13] = m[7]; ret[14] = m[11]; ret[15] = m[15];
}

//============================================================================

/*
===============
Matrix_Quat
===============
*/
void Matrix_Quat (vec3_t m[3], quat_t q) {
	vec_t tr, s;

	tr = m[0][0] + m[1][1] + m[2][2];
	if (tr > 0.00001) {
		s = sqrt (tr + 1.0);
		q[3] = s * 0.5; s = 0.5 / s;
		q[0] = (m[2][1] - m[1][2]) * s;
		q[1] = (m[0][2] - m[2][0]) * s;
		q[2] = (m[1][0] - m[0][1]) * s;
	} else {
		int i, j, k;

		i = 0;
		if (m[1][1] > m[0][0]) i = 1;
		if (m[2][2] > m[i][i]) i = 2;
		j = (i + 1) % 3;
		k = (i + 2) % 3;

		s = sqrt (m[i][i] - (m[j][j] + m[k][k]) + 1.0);

		q[i] = s * 0.5;
		if (s != 0.0)
			s = 0.5 / s;
		q[j] = (m[j][i] + m[i][j]) * s;
		q[k] = (m[k][i] + m[i][k]) * s;
		q[3] = (m[k][j] - m[j][k]) * s;
	}

	Quat_Normalize (q);
}


/*
===============
Quat_ConcatTransforms
===============
*/
void Quat_ConcatTransforms (const quat_t q1, const vec3_t v1, const quat_t q2, const vec3_t v2, quat_t q, vec3_t v) {
	Quat_Multiply (q1, q2, q);
	Quat_TransformVector (q1, v2, v);
	v[0] += v1[0]; v[1] += v1[1]; v[2] += v1[2];
}


/*
===============
Quat_Copy
===============
*/
void Quat_Copy (const quat_t q1, quat_t q2) {
	q2[0] = q1[0];
	q2[1] = q1[1];
	q2[2] = q1[2];
	q2[3] = q1[3];
}


/*
===============
Quat_Conjugate
===============
*/
void Quat_Conjugate (const quat_t q1, quat_t q2) {
	q2[0] = -q1[0];
	q2[1] = -q1[1];
	q2[2] = -q1[2];
	q2[3] = q1[3];
}


/*
===============
Quat_Identity
===============
*/
void Quat_Identity (quat_t q) {
	Quat_Copy (quatIdentity, q);
}


/*
===============
Quat_Inverse
===============
*/
vec_t Quat_Inverse (const quat_t q1, quat_t q2) {
	Quat_Conjugate (q1, q2);

	return Quat_Normalize (q2);
}


/*
===============
Quat_Normalize
===============
*/
vec_t Quat_Normalize (quat_t q) {
	vec_t length;

	length = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
	if (length != 0) {
		vec_t ilength = 1.0 / sqrt (length);
		q[0] *= ilength;
		q[1] *= ilength;
		q[2] *= ilength;
		q[3] *= ilength;
	}

	return length;
}


/*
===============
Quat_Lerp
===============
*/
void Quat_Lerp (const quat_t q1, const quat_t q2, vec_t t, quat_t out) {
	quat_t	p1;
	vec_t	omega, cosom, sinom, scale0, scale1;

	cosom = q1[0] * q2[0] + q1[1] * q2[1] + q1[2] * q2[2] + q1[3] * q2[3];
	if (cosom < 0.0) { 
		cosom = -cosom;
		p1[0] = -q1[0]; p1[1] = -q1[1];
		p1[2] = -q1[2]; p1[3] = -q1[3];
	} else {
		p1[0] = q1[0]; p1[1] = q1[1];
		p1[2] = q1[2]; p1[3] = q1[3];
	}

	if (cosom < 1.0 - 0.001) {
		omega = acos (cosom);
		sinom = 1.0 / sin (omega);
		scale0 = sin ((1.0 - t) * omega) * sinom;
		scale1 = sin (t * omega) * sinom;
	} else { 
		scale0 = 1.0 - t;
		scale1 = t;
	}

	out[0] = scale0 * p1[0] + scale1 * q2[0];
	out[1] = scale0 * p1[1] + scale1 * q2[1];
	out[2] = scale0 * p1[2] + scale1 * q2[2];
	out[3] = scale0 * p1[3] + scale1 * q2[3];
}


/*
===============
Quat_Matrix
===============
*/
void Quat_Matrix (const quat_t q, vec3_t m[3]) {
	vec_t wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

	x2 = q[0] + q[0]; y2 = q[1] + q[1]; z2 = q[2] + q[2];
	xx = q[0] * x2; xy = q[0] * y2; xz = q[0] * z2;
	yy = q[1] * y2; yz = q[1] * z2; zz = q[2] * z2;
	wx = q[3] * x2; wy = q[3] * y2; wz = q[3] * z2;

	m[0][0] = 1.0f - yy - zz; m[0][1] = xy - wz; m[0][2] = xz + wy;
	m[1][0] = xy + wz; m[1][1] = 1.0f - xx - zz; m[1][2] = yz - wx;
	m[2][0] = xz - wy; m[2][1] = yz + wx; m[2][2] = 1.0f - xx - yy;
}


/*
===============
Quat_Multiply
===============
*/
void Quat_Multiply (const quat_t q1, const quat_t q2, quat_t out) {
	out[0] = q1[3] * q2[0] + q1[0] * q2[3] + q1[1] * q2[2] - q1[2] * q2[1];
	out[1] = q1[3] * q2[1] + q1[1] * q2[3] + q1[2] * q2[0] - q1[0] * q2[2];
	out[2] = q1[3] * q2[2] + q1[2] * q2[3] + q1[0] * q2[1] - q1[1] * q2[0];
	out[3] = q1[3] * q2[3] - q1[0] * q2[0] - q1[1] * q2[1] - q1[2] * q2[2];
}


/*
===============
Quat_TransformVector
===============
*/
void Quat_TransformVector (const quat_t q, const vec3_t v, vec3_t out) {
	vec_t wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

	x2 = q[0] + q[0]; y2 = q[1] + q[1]; z2 = q[2] + q[2];
	xx = q[0] * x2; xy = q[0] * y2; xz = q[0] * z2;
	yy = q[1] * y2; yz = q[1] * z2; zz = q[2] * z2;
	wx = q[3] * x2; wy = q[3] * y2; wz = q[3] * z2;

	out[0] = (1.0f - yy - zz) * v[0] + (xy - wz) * v[1] + (xz + wy) * v[2];
	out[1] = (xy + wz) * v[0] + (1.0f - xx - zz) * v[1] + (yz - wx) * v[2];
	out[2] = (xz - wy) * v[0] + (yz + wx) * v[1] + (1.0f - xx - yy) * v[2];
}
