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
// mathlib.c
//

#include "shared.h"

vec3_t		vec3Origin = {
	0, 0, 0
};

vec4_t		vec4Origin = {
	0, 0, 0, 0
};

/*
=============================================================================

	MATHLIB

=============================================================================
*/

vec3_t byteDirs[NUMVERTEXNORMALS] = {
	{-0.525731f,	0.000000f,	0.850651f},		{-0.442863f,	0.238856f,	0.864188f},		{-0.295242f,	0.000000f,	0.955423f},
	{-0.309017f,	0.500000f,	0.809017f},		{-0.162460f,	0.262866f,	0.951056f},		{0.000000f,		0.000000f,	1.000000f},
	{0.000000f,		0.850651f,	0.525731f},		{-0.147621f,	0.716567f,	0.681718f},		{0.147621f,		0.716567f,	0.681718f},
	{0.000000f,		0.525731f,	0.850651f},		{0.309017f,		0.500000f,	0.809017f},		{0.525731f,		0.000000f,	0.850651f},
	{0.295242f,		0.000000f,	0.955423f},		{0.442863f,		0.238856f,	0.864188f},		{0.162460f,		0.262866f,	0.951056f},
	{-0.681718f,	0.147621f,	0.716567f},		{-0.809017f,	0.309017f,	0.500000f},		{-0.587785f,	0.425325f,	0.688191f},
	{-0.850651f,	0.525731f,	0.000000f},		{-0.864188f,	0.442863f,	0.238856f},		{-0.716567f,	0.681718f,	0.147621f},
	{-0.688191f,	0.587785f,	0.425325f},		{-0.500000f,	0.809017f,	0.309017f},		{-0.238856f,	0.864188f,	0.442863f},
	{-0.425325f,	0.688191f,	0.587785f},		{-0.716567f,	0.681718f,	-0.147621f},	{-0.500000f,	0.809017f,	-0.309017f},
	{-0.525731f,	0.850651f,	0.000000f},		{0.000000f,		0.850651f,	-0.525731f},	{-0.238856f,	0.864188f,	-0.442863f},
	{0.000000f,		0.955423f,	-0.295242f},	{-0.262866f,	0.951056f,	-0.162460f},	{0.000000f,		1.000000f,	0.000000f},
	{0.000000f,		0.955423f,	0.295242f},		{-0.262866f,	0.951056f,	0.162460f},		{0.238856f,		0.864188f,	0.442863f},
	{0.262866f,		0.951056f,	0.162460f},		{0.500000f,		0.809017f,	0.309017f},		{0.238856f,		0.864188f,	-0.442863f},
	{0.262866f,		0.951056f,	-0.162460f},	{0.500000f,		0.809017f,	-0.309017f},	{0.850651f,		0.525731f,	0.000000f},
	{0.716567f,		0.681718f,	0.147621f},		{0.716567f,		0.681718f,	-0.147621f},	{0.525731f,		0.850651f,	0.000000f},
	{0.425325f,		0.688191f,	0.587785f},		{0.864188f,		0.442863f,	0.238856f},		{0.688191f,		0.587785f,	0.425325f},
	{0.809017f,		0.309017f,	0.500000f},		{0.681718f,		0.147621f,	0.716567f},		{0.587785f,		0.425325f,	0.688191f},
	{0.955423f,		0.295242f,	0.000000f},		{1.000000f,		0.000000f,	0.000000f},		{0.951056f,		0.162460f,	0.262866f},
	{0.850651f,		-0.525731f,	0.000000f},		{0.955423f,		-0.295242f,	0.000000f},		{0.864188f,		-0.442863f,	0.238856f},
	{0.951056f,		-0.162460f,	0.262866f},		{0.809017f,		-0.309017f,	0.500000f},		{0.681718f,		-0.147621f,	0.716567f},
	{0.850651f,		0.000000f,	0.525731f},		{0.864188f,		0.442863f,	-0.238856f},	{0.809017f,		0.309017f,	-0.500000f},
	{0.951056f,		0.162460f,	-0.262866f},	{0.525731f,		0.000000f,	-0.850651f},	{0.681718f,		0.147621f,	-0.716567f},
	{0.681718f,		-0.147621f,	-0.716567f},	{0.850651f,		0.000000f,	-0.525731f},	{0.809017f,		-0.309017f,	-0.500000f},
	{0.864188f,		-0.442863f,	-0.238856f},	{0.951056f,		-0.162460f,	-0.262866f},	{0.147621f,		0.716567f,	-0.681718f},
	{0.309017f,		0.500000f,	-0.809017f},	{0.425325f,		0.688191f,	-0.587785f},	{0.442863f,		0.238856f,	-0.864188f},
	{0.587785f,		0.425325f,	-0.688191f},	{0.688191f,		0.587785f,	-0.425325f},	{-0.147621f,	0.716567f,	-0.681718f},
	{-0.309017f,	0.500000f,	-0.809017f},	{0.000000f,		0.525731f,	-0.850651f},	{-0.525731f,	0.000000f,	-0.850651f},
	{-0.442863f,	0.238856f,	-0.864188f},	{-0.295242f,	0.000000f,	-0.955423f},	{-0.162460f,	0.262866f,	-0.951056f},
	{0.000000f,		0.000000f,	-1.000000f},	{0.295242f,		0.000000f,	-0.955423f},	{0.162460f,		0.262866f,	-0.951056f},
	{-0.442863f,	-0.238856f,	-0.864188f},	{-0.309017f,	-0.500000f,	-0.809017f},	{-0.162460f,	-0.262866f,	-0.951056f},
	{0.000000f,		-0.850651f,	-0.525731f},	{-0.147621f,	-0.716567f,	-0.681718f},	{0.147621f,		-0.716567f,	-0.681718f},
	{0.000000f,		-0.525731f,	-0.850651f},	{0.309017f,		-0.500000f,	-0.809017f},	{0.442863f,		-0.238856f,	-0.864188f},
	{0.162460f,		-0.262866f,	-0.951056f},	{0.238856f,		-0.864188f,	-0.442863f},	{0.500000f,		-0.809017f,	-0.309017f},
	{0.425325f,		-0.688191f,	-0.587785f},	{0.716567f,		-0.681718f,	-0.147621f},	{0.688191f,		-0.587785f,	-0.425325f},
	{0.587785f,		-0.425325f,	-0.688191f},	{0.000000f,		-0.955423f,	-0.295242f},	{0.000000f,		-1.000000f,	0.000000f},
	{0.262866f,		-0.951056f,	-0.162460f},	{0.000000f,		-0.850651f,	0.525731f},		{0.000000f,		-0.955423f,	0.295242f},
	{0.238856f,		-0.864188f,	0.442863f},		{0.262866f,		-0.951056f,	0.162460f},		{0.500000f,		-0.809017f,	0.309017f},
	{0.716567f,		-0.681718f,	0.147621f},		{0.525731f,		-0.850651f,	0.000000f},		{-0.238856f,	-0.864188f,	-0.442863f},
	{-0.500000f,	-0.809017f,	-0.309017f},	{-0.262866f,	-0.951056f,	-0.162460f},	{-0.850651f,	-0.525731f,	0.000000f},
	{-0.716567f,	-0.681718f,	-0.147621f},	{-0.716567f,	-0.681718f,	0.147621f},		{-0.525731f,	-0.850651f,	0.000000f},
	{-0.500000f,	-0.809017f,	0.309017f},		{-0.238856f,	-0.864188f,	0.442863f},		{-0.262866f,	-0.951056f,	0.162460f},
	{-0.864188f,	-0.442863f,	0.238856f},		{-0.809017f,	-0.309017f,	0.500000f},		{-0.688191f,	-0.587785f,	0.425325f},
	{-0.681718f,	-0.147621f,	0.716567f},		{-0.442863f,	-0.238856f,	0.864188f},		{-0.587785f,	-0.425325f,	0.688191f},
	{-0.309017f,	-0.500000f,	0.809017f},		{-0.147621f,	-0.716567f,	0.681718f},		{-0.425325f,	-0.688191f,	0.587785f},
	{-0.162460f,	-0.262866f,	0.951056f},		{0.442863f,		-0.238856f,	0.864188f},		{0.162460f,		-0.262866f,	0.951056f},
	{0.309017f,		-0.500000f,	0.809017f},		{0.147621f,		-0.716567f,	0.681718f},		{0.000000f,		-0.525731f,	0.850651f},
	{0.425325f,		-0.688191f,	0.587785f},		{0.587785f,		-0.425325f,	0.688191f},		{0.688191f,		-0.587785f,	0.425325f},
	{-0.955423f,	0.295242f,	0.000000f},		{-0.951056f,	0.162460f,	0.262866f},		{-1.000000f,	0.000000f,	0.000000f},
	{-0.850651f,	0.000000f,	0.525731f},		{-0.955423f,	-0.295242f,	0.000000f},		{-0.951056f,	-0.162460f,	0.262866f},
	{-0.864188f,	0.442863f,	-0.238856f},	{-0.951056f,	0.162460f,	-0.262866f},	{-0.809017f,	0.309017f,	-0.500000f},
	{-0.864188f,	-0.442863f,	-0.238856f},	{-0.951056f,	-0.162460f,	-0.262866f},	{-0.809017f,	-0.309017f,	-0.500000f},
	{-0.681718f,	0.147621f,	-0.716567f},	{-0.681718f,	-0.147621f,	-0.716567f},	{-0.850651f,	0.000000f,	-0.525731f},
	{-0.688191f,	0.587785f,	-0.425325f},	{-0.587785f,	0.425325f,	-0.688191f},	{-0.425325f,	0.688191f,	-0.587785f},
	{-0.425325f,	-0.688191f,	-0.587785f},	{-0.587785f,	-0.425325f,	-0.688191f},	{-0.688191f,	-0.587785f,	-0.425325f}
};

/*
=================
DirToByte

This isn't a real cheap function to call!
=================
*/
byte DirToByte (vec3_t dirVec)
{
	byte	i, best;
	float	d, bestDot;

	if (!dirVec)
		return 0;

	best = 0;
	bestDot = 0;
	for (i=0 ; i<NUMVERTEXNORMALS ; i++) {
		d = DotProduct (dirVec, byteDirs[i]);
		if (d > bestDot) {
			bestDot = d;
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
void ByteToDir (byte dirByte, vec3_t dirVec)
{
	if (dirByte >= NUMVERTEXNORMALS) {
		VectorClear (dirVec);
		return;
	}

	VectorCopy (byteDirs[dirByte], dirVec);
}

// ===========================================================================

/*
===============
Q_ftol
===============
*/
#ifdef id386
__declspec_naked long Q_ftol (float f)
{
	static int	tmp;
	__asm {
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
float Q_FastSqrt (float value)
{
	float result;
	__asm {
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

1/sqrt, faster but not as precise
===============
*/
float Q_RSqrt (float number)
{
	float	y;

	if (number == 0.0f)
		return 0.0f;
	*((int *)&y) = 0x5f3759df - ((* (int *) &number) >> 1);
	return y * (1.5f - (number * 0.5f * y * y));
}


/*
===============
Q_RSqrtd

1/sqrt, faster but not as precise
===============
*/
double Q_RSqrtd (double number)
{
	double	y;

	if (number == 0.0)
		return 0.0;
	*((int *)&y) = 0x5f3759df - ((* (int *) &number) >> 1);
	return y * (1.5f - (number * 0.5 * y * y));
}


/*
===============
Q_log2
===============
*/
int Q_log2 (int val)
{
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
void Q_NearestPow (int *var, qBool oneLess)
{
	int	i;

	for (i=1 ; i<*var ; i<<=1) ;

	if (oneLess) {
		// find the nearest power of two below input value
		if (i > *var)
			i >>= 1;
	}

	*var = i;
}

// ===========================================================================

/*
====================
CalcFovY

Calculates aspect based on fovX and the screen dimensions
====================
*/
float CalcFovY (float fovX, float width, float height)
{
	if (fovX < 1 || fovX > 179)
		Com_Printf (PRNT_ERROR, "Bad fov: %f\n", fovX);

	return (float)(atan (height / (width / tan (fovX / 360.0f * M_PI)))) * ((180.0f / M_PI) * 2);
}


// ===========================================================================

/*
===============
NormToLatLong
===============
*/
void NormToLatLong (vec3_t normal, byte latlong[2])
{
	if ((normal[0] == 0) && (normal[1] == 0)) {
		if (normal[2] > 0) {
			latlong[0] = 0;
			latlong[1] = 0;
		}
		else {
			latlong[0] = 128;
			latlong[1] = 0;
		}
	}
	else {
		int		angle;

		angle = (int)(acos (normal[2]) * 255.0 / (M_PI * 2.0f)) & 0xff;
		latlong[0] = angle;
		angle = (int)(atan2 (normal[1], normal[0]) * 255.0 / (M_PI * 2.0f)) & 0xff;
		latlong[1] = angle;
	}
}


/*
===============
MakeNormalVectorsf
===============
*/
void MakeNormalVectorsf (vec3_t forward, vec3_t right, vec3_t up)
{
	float		d;

	// this rotate and negate guarantees a vector not colinear with the original
	VectorSet (right, forward[2], -forward[0], forward[1]);

	d = DotProduct (right, forward);
	VectorMA (right, -d, forward, right);
	VectorNormalizef (right, right);
	CrossProduct (right, forward, up);
}


/*
===============
MakeNormalVectorsd
===============
*/
void MakeNormalVectorsd (dvec3_t forward, dvec3_t right, dvec3_t up)
{
	double		d;

	// this rotate and negate guarantees a vector not colinear with the original
	VectorSet (right, forward[2], -forward[0], forward[1]);

	d = DotProduct (right, forward);
	VectorMA (right, -d, forward, right);
	VectorNormalized (right, right);
	CrossProduct (right, forward, up);
}


/*
===============
PerpendicularVector

assumes "src" is normalized
===============
*/
void PerpendicularVector (vec3_t src, vec3_t dst)
{
	int		pos = 5;
	float	minelem = 1.0F;
	vec3_t	tempvec;

	// find the smallest magnitude axially aligned vector
	if (fabs (src[0]) < minelem) {
		pos = 0;
		minelem = (float)fabs (src[0]);
	}
	if (fabs (src[1]) < minelem) {
		pos = 1;
		minelem = (float)fabs (src[1]);
	}
	if (fabs (src[2]) < minelem) {
		pos = 2;
		minelem = (float)fabs (src[2]);
	}

#ifdef _DEBUG
	assert (pos != 5);
#endif // _DEBUG

	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	// project the point onto the plane defined by src
	ProjectPointOnPlane (dst, tempvec, src);

	// normalize the result
	VectorNormalizef (dst, dst);
}


/*
===============
RotatePointAroundVector
===============
*/
void RotatePointAroundVector (vec3_t dst, vec3_t dir, vec3_t point, float degrees)
{
	float		t0, t1;
	float		c, s;
	vec3_t		vr, vu, vf;

	s = DEG2RAD (degrees);
	c = (float)cos (s);
	s = (float)sin (s);

	VectorCopy (dir, vf);
	MakeNormalVectorsf (vf, vr, vu);

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


/*
===============
VectorNormalizef
===============
*/
float VectorNormalizef (vec3_t in, vec3_t out)
{
	float	length, invLength;

	length = (float)VectorLength (in);

	if (length) {
		invLength = 1.0f/length;
		VectorScale (in, invLength, out);
	}
	else {
		VectorClear (out);
	}
		
	return length;
}


/*
===============
VectorNormalized
===============
*/
double VectorNormalized (dvec3_t in, dvec3_t out)
{
	double	length, invLength;

	length = VectorLength (in);

	if (length) {
		invLength = 1.0f/length;
		VectorScale (in, invLength, out);
	}
	else {
		VectorClear (out);
	}
		
	return length;
}


/*
===============
VectorNormalizeFastf
===============
*/
void VectorNormalizeFastf (vec3_t v)
{
	float	ilength = Q_RSqrt (DotProduct(v,v));

	v[0] *= ilength;
	v[1] *= ilength;
	v[2] *= ilength;
}


/*
===============
VectorNormalizeFastd
===============
*/
void VectorNormalizeFastd (dvec3_t v)
{
	double	ilength = Q_RSqrtd (DotProduct(v,v));

	v[0] *= ilength;
	v[1] *= ilength;
	v[2] *= ilength;
}
