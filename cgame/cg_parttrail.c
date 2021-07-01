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
// cg_parttrail.c
//

#include "cg_local.h"

/*
=============================================================================

	PARTICLE TRAILS

=============================================================================
*/

/*
===============
CG_BeamTrail
===============
*/
void __fastcall CG_BeamTrail (vec3_t start, vec3_t end, int color, double size, double alpha, double alphaVel) {
	vec3_t		dest, move, vec;
	float		len, dec;
	double		run;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 35 - (rand () % 27), 0.2, qFalse);
	VectorScale (vec, dec, vec);

	for (; len>0 ; VectorAdd (move, vec, move)) {
		len -= dec;

		if ((crand () > 0) || (rand () % 14))
			continue;

		VectorCopy (move, dest);

		dest[0] += ((rand () % ((int)(size*32) + 1) - (int)(size*16))/32);
		dest[1] += ((rand () % ((int)(size*32) + 1) - (int)(size*16))/32);
		dest[2] += ((rand () % ((int)(size*32) + 1) - (int)(size*16))/32);

		run = (frand () * 50);
		makePart (
			dest[0],							dest[1],							dest[2],
			0,									0,									0,
			0,									0,									0,
			0,									0,									0,
			palRed (color) + run,				palGreen (color) + run,				palBlue (color) + run,
			palRed (color) + run,				palGreen (color) + run,				palBlue (color) + run,
			alpha,								alphaVel,
			size / 3.0,							0.1 + (frand () * 0.1),
			PT_FLARE,							0,
			GL_SRC_ALPHA,						GL_ONE,
			0,									qFalse,
			0);
	}
}


/*
===============
CG_BfgTrail
===============
*/
void CG_BfgTrail (entity_t *ent) {
	int			i;
	float		angle, sr, sp, sy, cr, cp, cy;
	float		ltime, size, dist = 64;
	vec3_t		forward, org;

	// outer
	makePart (
		ent->origin[0],					ent->origin[1],					ent->origin[2],
		0,								0,								0,
		0,								0,								0,
		0,								0,								0,
		0,								200,							0,
		0,								200,							0,
		0.66,							PART_INSTANT,
		60,								60,
		PT_GREEN,						PF_SCALED,
		GL_SRC_ALPHA,					GL_ONE,
		0,								qFalse,
		0);

	// core
	CG_FlareEffect (ent->origin, PT_FLARE, 0 + (frand () * 32), 45, 45, 0xd0, 0xd0, 0.66, PART_INSTANT);
	CG_FlareEffect (ent->origin, PT_FLARE, 180 + (frand () * 32), 55, 55, 0xd0, 0xd0, 0.66, PART_INSTANT);

	CG_FlareEffect (ent->origin, PT_FLARE, 0 + (frand () * 32), 35, 35, 0xd7, 0xd7, 0.66, PART_INSTANT);
	CG_FlareEffect (ent->origin, PT_FLARE, 180 + (frand () * 32), 45, 45, 0xd7, 0xd7, 0.66, PART_INSTANT);

	ltime = (float) cgs.realTime / 1000.0;
	for (i=0 ; i<(pIncDegree (ent->origin, NUMVERTEXNORMALS/2.0, 0.5, qTrue)) ; i++) {
		angle = ltime * avelocities[i][0];
		sy = sin (angle);
		cy = cos (angle);
		angle = ltime * avelocities[i][1];
		sp = sin (angle);
		cp = cos (angle);
		angle = ltime * avelocities[i][2];
		sr = sin (angle);
		cr = cos (angle);
	
		VectorSet (forward, cp*cy, cp*sy, -sp);

		org[0] = ent->origin[0] + (byteDirs[i][0] * sin (ltime + i) * 64) + (forward[0] * BEAMLENGTH);
		org[1] = ent->origin[1] + (byteDirs[i][1] * sin (ltime + i) * 64) + (forward[0] * BEAMLENGTH);
		org[2] = ent->origin[2] + (byteDirs[i][2] * sin (ltime + i) * 64) + (forward[0] * BEAMLENGTH);

		dist = VectorDistanceFast (org, ent->origin) / 90.0;

		size = (2 - (dist * 2 + 0.1)) * 12;
		makePart (
			org[0],								org[1],								org[2],
			0,									0,									0,
			0,									0,									0,
			0,									0,									0,
			115 + ((1 - (dist * 3)) * 40),		180 + ((1 - (dist * 3)) * 75),		115 + ((1 - (dist * 3)) * 40),
			115 + ((1 - (dist * 3)) * 40),		180 + ((1 - (dist * 3)) * 75),		115 + ((1 - (dist * 3)) * 40),
			0.95 - dist/2.0,					-100,
			size,								size,
			PT_GREEN,							PF_SCALED,
			GL_SRC_ALPHA,						GL_ONE,
			0,									qFalse,
			0);

		// sparks
		if ((!(rand () % 7)) && (((frand () * 20 + 1) / 10.0) <= part_degree->value)) {
			makePart (
				org[0] + (crand () * 4),			org[1] + (crand () * 4),			org[2] + (crand () * 4),
				0,									0,									0,
				crand () * 16,						crand () * 16,						crand () * 16,
				0,									0,									-20,
				115 + ((1 - (dist * 3)) * 40),		180 + ((1 - (dist * 3)) * 75),		115 + ((1 - (dist * 3)) * 40),
				115 + ((1 - (dist * 3)) * 40),		180 + ((1 - (dist * 3)) * 75),		115 + ((1 - (dist * 3)) * 40),
				1.0,								-0.9 / (0.4 + (frand () * 0.3)),
				0.5 + (frand () * 0.25),			0.4 + (frand () * 0.25),
				PT_GREEN,							PF_SCALED|PF_DIRECTION,
				GL_SRC_ALPHA,						GL_ONE,
				pSplashThink,						qTrue,
				PMAXSPLASHLEN);
		}
	}
}


/*
===============
CG_BlasterGoldTrail
===============
*/
void CG_BlasterGoldTrail (vec3_t start, vec3_t end) {
	vec3_t		move, vec;
	float		len, orgscale, velscale;
	int			dec, rnum, rnum2;

	// bubbles
	if (rand () % 2)
		CG_BubbleEffect (start);

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 5, 0.3, qTrue);
	VectorScale (vec, dec, vec);

	orgscale = (cgi.FS_CurrGame ("gloom")) ? 0.5 : 1;
	velscale = (cgi.FS_CurrGame ("gloom")) ? 3 : 5;

	for (; len>0 ; VectorAdd (move, vec, move)) {
		len -= dec;

		rnum = (rand () % 5);
		rnum2 = (rand () % 5);
		makePart (
			move[0] + (crand () * orgscale),	move[1] + (crand () * orgscale),	move[2] + (crand () * orgscale),
			0,									0,									0,
			crand () * velscale,				crand () * velscale,				crand () * velscale,
			0,									0,									0,
			palRed (0xe0 + rnum),				palGreen (0xe0 + rnum),				palBlue (0xe0 + rnum),
			palRed (0xe0 + rnum2),				palGreen (0xe0 + rnum2),			palBlue (0xe0 + rnum2),
			1.0,								-1.0 / (0.25 + (crand () * 0.05)),
			7 + crand (),						frand () * 5,
			PT_RED,								0,
			GL_SRC_ALPHA,						GL_ONE,
			0,									qFalse,
			0);
	}
}


/*
===============
CG_BlasterGreenTrail
===============
*/
void CG_BlasterGreenTrail (vec3_t start, vec3_t end) {
	vec3_t		move, vec;
	float		len, orgscale, velscale;
	int			dec, rnum, rnum2;

	// bubbles
	if (rand () % 2)
		CG_BubbleEffect (start);

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 5, 0.3, qTrue);
	VectorScale (vec, dec, vec);

	if (cgi.FS_CurrGame ("gloom")) {
		orgscale = 0.5;
		velscale = 3;
	}
	else {
		orgscale = 1;
		velscale = 5;
	}

	for (; len>0 ; VectorAdd (move, vec, move)) {
		len -= dec;

		rnum = (rand () % 5);
		rnum2 = (rand () % 5);

		makePart (
			move[0] + (crand () * orgscale),	move[1] + (crand () * orgscale),	move[2] + (crand ()* orgscale),
			0,									0,									0,
			crand () * velscale,				crand () * velscale,				crand () * velscale,
			0,									0,									0,
			palRed (0xd0 + rnum),				palGreen (0xd0 + rnum),				palBlue (0xd0 + rnum),
			palRed (0xd0 + rnum2),				palGreen (0xd0 + rnum2),			palBlue (0xd0 + rnum2),
			1.0,								-1.0 / (0.25 + (crand () * 0.05)),
			5,									1,
			PT_GREEN,							0,
			GL_SRC_ALPHA,						GL_ONE,
			0,									qFalse,
			0);
	}
}


/*
===============
CG_BubbleTrail
===============
*/
void CG_BubbleTrail (vec3_t start, vec3_t end) {
	vec3_t		move, vec;
	float		len, dec;
	double		rnum, rnum2;
	int			i;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 32, 0.5, qFalse);
	VectorScale (vec, dec, vec);

	for (i=0 ; i<len ; i+=dec, VectorAdd (move, vec, move)) {
		rnum = 230 + (frand () * 25);
		rnum2 = 230 + (frand () * 25);

		makePart (
			move[0] + crand (),				move[1] + crand (),				move[2] + crand (),
			0,								0,								0,
			crand () * 4,					crand () * 4,					10 + (crand () * 4),
			0,								0,								0,
			rnum,							rnum,							rnum,
			rnum2,							rnum2,							rnum2,
			0.9 + (crand () * 0.1),			-1.0 / (3 + (frand () * 0.2)),
			0.1 + frand (),					0.1 + frand (),
			PT_BUBBLE,						PF_SHADE|PF_LAVAONLY|PF_SLIMEONLY|PF_WATERONLY,
			GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
			0,								qFalse,
			0);
	}
}


/*
===============
CG_BubbleTrail2

lets you control the number of bubbles by setting the distance between the spawns
===============
*/
void CG_BubbleTrail2 (vec3_t start, vec3_t end, int dist) {
	vec3_t		move, vec;
	float		len, dec;
	double		rnum, rnum2;
	int			i;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, dist, 0.5, qFalse);
	VectorScale (vec, dec, vec);

	for (i=0 ; i<len ; i+=dec, VectorAdd (move, vec, move)) {
		rnum = 230 + (frand () * 25);
		rnum2 = 230 + (frand () * 25);

		makePart (
			move[0] + crand (),				move[1] + crand (),				move[2] + crand (),
			0,								0,								0,
			crand () * 4,					crand () * 4,					10 + (crand () * 4),
			0,								0,								0,
			rnum,							rnum,							rnum,
			rnum2,							rnum2,							rnum2,
			0.9 + (crand () * 0.1),			-1.0 / (3 + (frand () * 0.1)),
			0.1 + frand (),					0.1 + frand (),
			PT_BUBBLE,						PF_SHADE|PF_LAVAONLY|PF_SLIMEONLY|PF_WATERONLY,
			GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
			0,								qFalse,
			0);
	}
}


/*
===============
CG_DebugTrail
===============
*/
void CG_DebugTrail (vec3_t start, vec3_t end) {
	vec3_t		move, vec, right, up;
	float		len, dec, rnum, rnum2;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec, vec);

	MakeNormalVectors (vec, right, up);

	dec = pDecDegree (start, 3, 0.5, qFalse);
	VectorScale (vec, dec, vec);
	VectorCopy (start, move);

	for (; len>0 ; VectorAdd (move, vec, move)) {
		len -= dec;

		rnum = frand () * 40;
		rnum2 = frand () * 40;
		makePart (
			move[0],						move[1],						move[2],
			0,								0,								0,
			0,								0,								0,
			0,								0,								0,
			0,								rnum,							215 + rnum,
			0,								rnum2,							215 + rnum2,
			1.0,							-0.1,
			3.0f,							1.0f,
			PT_BLUE,						PF_SCALED,
			GL_SRC_ALPHA,					GL_ONE,
			0,								qFalse,
			0);
	}
}


/*
===============
CG_FlagTrail
===============
*/
void CG_FlagTrail (vec3_t start, vec3_t end, int flags) {
	vec3_t		move, vec;
	float		len, clrmod, dec;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 4, 0.5, qTrue);
	VectorScale (vec, dec, vec);

	// red
	if (flags & EF_FLAG1) {
		for (; len>0 ; VectorAdd (move, vec, move)) {
			len -= dec;

			clrmod = (rand () % 2) ? (rand () % 170) : 0;
			makePart (
				move[0] + (crand () * 6),		move[1] + (crand () * 6),		move[2] + (crand () * 6),
				0,								0,								0,
				(crand () * 8),					(crand () * 8),					(crand () * 8),
				0,								0,								0,
				140 + (frand () * 50) + clrmod,	clrmod,							clrmod,
				140 + (frand () * 50) + clrmod,	clrmod,							clrmod,
				1.0,							-1.0 / (0.8 + (frand () * 0.2)),
				5,								2,
				PT_FLARE,						PF_SCALED,
				GL_SRC_ALPHA,					GL_ONE,
				0,								qFalse,
				0);
		}
	}

	// blue
	if (flags & EF_FLAG2) {
		for (; len>0 ; VectorAdd (move, vec, move)) {
			len -= dec;

			clrmod = (rand () % 2) ? (rand () % 170) : 0;
			makePart (
				move[0] + (crand () * 6),		move[1] + (crand () * 6),		move[2] + (crand () * 6),
				0,								0,								0,
				crand () * 8,					crand () * 8,					crand () * 8,
				0,								0,								0,
				clrmod,							clrmod + (frand () * 70),		clrmod + 230 + (frand () * 50),
				clrmod,							clrmod + (frand () * 70),		clrmod + 230 + (frand () * 50),
				1.0,							-1.0 / (0.8 + (frand () * 0.2)),
				5,								2,
				PT_FLARE,						PF_SCALED,
				GL_SRC_ALPHA,					GL_ONE,
				0,								qFalse,
				0);
		}
	}
}


/*
===============
CG_GibTrail
===============
*/
void CG_GibTrail (vec3_t start, vec3_t end, int flags) {
	vec3_t		move, vec;
	float		len, dec;
	int			i, partFlags;
	qBool		isGreen = (flags & EF_GREENGIB);

	// bubbles
	if (!(rand () % 10))
		CG_BubbleEffect (start);

	for (i=0 ; i<(clamp (cl_gore->value + 1, 1, 11) / 5.0) ; i++) {
		if (!(rand () % 26)) {
			// drips on ground
			makePart (
				start[0] + crand (),				start[1] + crand (),				start[2] + crand () - (frand () * 4),
				0,									0,									0,
				crand () * 20,						crand () * 20,						crand () * 20 + 20,
				crand () * 10,						crand () * 10,						-50,
				isGreen ? 30 : 235+(frand ()*5),	isGreen ? 70+(rand ()%91) : 245+(frand()*10),	isGreen ? 30 : 245 + (frand ()*10),
				0,									0,									0,
				1.0,								-0.5 / (0.6 + (frand () * 0.3)),
				1.5 + frand (),						2 + frand (),
				PT_BLDDRIP,							PF_GRAVITY|PF_DIRECTION|PF_ALPHACOLOR|PF_NOSFX,
				isGreen ? GL_SRC_ALPHA : GL_ZERO,	isGreen ? GL_ONE : GL_ONE_MINUS_SRC_COLOR,
				pBloodDripThink,					qTrue,
				PMAXBLDDRIPLEN);
		}
	}

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 3.25, 0.5, qTrue);
	VectorScale (vec, dec, vec);

	for (; len > 0 ; VectorAdd (move, vec, move)) {
		len -= dec;
		if (!isGreen) {
			//
			// EF_GIB
			//
			// floating blotches
			if (!(rand () % 10)) {
				partFlags = PF_SCALED|PF_ALPHACOLOR;
				if (rand () % 6)
					partFlags |= PF_NODECAL;

				makePart (
					move[0] + crand (),				move[1] + crand (),				move[2] + crand (),
					0,								0,								0,
					crand (),						crand (),						crand () - 40,
					0,								0,								0,
					115 + (frand () * 5),			125 + (frand () * 10),			120 + (frand () * 10),
					0,								0,								0,
					1.0,							-0.5 / (0.4 + (frand () * 0.3)),
					7.5 + (crand () * 2),			9 + (crand () * 2),
					dRandBloodMark (),				partFlags,
					GL_ZERO,						GL_ONE_MINUS_SRC_COLOR,
					pBloodThink,					qTrue,
					frand () * 360);
			}

			// drip
			for (i=0 ; i<(clamp (cl_gore->value + 1, 1, 11) / 5.0) ; i++)
				if (!(rand () % 26)) {
					partFlags = PF_ALPHACOLOR|PF_GRAVITY|PF_DIRECTION;
					if (rand () % 9)
						partFlags |= PF_NODECAL;

					makePart (
						move[0] + crand (),				move[1] + crand (),				move[2] + crand () - (frand () * 4),
						0,								0,								0,
						crand () * 50,					crand () * 50,					crand () * 50 + 20,
						crand () * 10,					crand () * 10,					-50,
						230 + (frand () * 5),			245 + (frand () * 10),			245 + (frand () * 10),
						0,								0,								0,
						1.0,							-0.5 / (0.6 + (frand ()* 0.3)),
						1.25 + (frand () * 0.2),		1.35 + (frand () * 0.2),
						PT_BLDDRIP,						partFlags,
						GL_ZERO,						GL_ONE_MINUS_SRC_COLOR,
						pBloodDripThink,				qTrue,
						PMAXBLDDRIPLEN*0.5 + (frand () * PMAXBLDDRIPLEN));
				}
		}
		else {
			//
			// EF_GREENGIB
			//
			// floating blotches
			if (!(rand () % 8)) {
				partFlags = PF_SCALED|PF_ALPHACOLOR;
				if (rand () % 6)
					partFlags |= PF_NODECAL;

				makePart (
					move[0] + crand (),				move[1] + crand (),				move[2] + crand (),
					0,								0,								0,
					crand (),						crand (),						crand () - 40,
					0,								0,								0,
					20,								50 + (frand () * 90),			20,
					0,								0 + (frand () * 90),			0,
					0.8 + (frand () * 0.2),			-1.0 / (0.5 + (frand () * 0.3)),
					4 + (crand () * 2),				6 + (crand () * 2),
					dRandBloodMark (),				partFlags,
					GL_SRC_ALPHA,					GL_ONE,
					pBloodThink,					qTrue,
					frand () * 360);
			}

			// drip
			for (i=0 ; i<(clamp (cl_gore->value + 1, 1, 11) / 5.0) ; i++)
				if (!(rand () % 26)) {
					partFlags = PF_ALPHACOLOR|PF_GRAVITY|PF_DIRECTION;
					if (rand () % 9)
						partFlags |= PF_NODECAL;

					makePart (
						move[0] + crand (),				move[1] + crand (),				move[2] + crand () - (frand () * 4),
						0,								0,								0,
						crand () * 50,					crand () * 50,					crand () * 50 + 20,
						crand () * 10,					crand () * 10,					-50,
						30,								70 + (frand () * 90),			30,
						0,								0,								0,
						1.0,							-0.5 / (0.6 + (frand () * 0.3)),
						1.25 + (frand () * 0.2),		1.35 + (frand () * 0.2),
						PT_BLDDRIP,						partFlags,
						GL_SRC_ALPHA,					GL_ONE,
						pBloodDripThink,				qTrue,
						PMAXBLDDRIPLEN*0.5 + (frand () * PMAXBLDDRIPLEN));
				}
		}
	}
}


/*
===============
CG_GrenadeTrail
===============
*/
void CG_GrenadeTrail (vec3_t start, vec3_t end) {
	vec3_t		move, vec;
	float		len, dec;
	double		rnum, rnum2;

	// bubbles
	CG_BubbleEffect (start);

	// flare glow
	makePart (
		end[0],							end[1],							end[2],
		0,								0,								0,
		0,								0,								0,
		0,								0,								0,
		255,							170,							80,
		255,							170,							80,
		0.66 + (crand () * 0.3),		PART_INSTANT,
		19 + (crand () * 2),			19,
		PT_FLARE,						0,
		GL_SRC_ALPHA,					GL_ONE,
		0,								qFalse,
		0);

	// smoke
	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 15, 0.25, qTrue);
	VectorScale (vec, dec, vec);

	len++, dec++;
	for (; len>0 ; VectorAdd (move, vec, move)) {
		len -= dec;

		rnum = (frand () * 80);
		rnum2 = (frand () * 80);
		makePart (
			move[0] + (crand () * 2),		move[1] + (crand () * 2),		move[2] + (crand () * 2),
			0,								0,								0,
			crand () * 3,					crand () * 3,					crand () * 3,
			0,								0,								5,
			50 + rnum,						50 + rnum,						50 + rnum,
			75 + rnum2,						75 + rnum2,						75 + rnum2,
			0.8 + (crand () * 0.1),			-2 / (1 + (cl_smokelinger->value * 0.1) + (crand () * 0.2)),
			6 + (crand () * 3),				30 + (crand () * 5),
			pRandSmoke (),					PF_SHADE,
			GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
			pSmokeThink,					qTrue,
			frand () * 360);
	}

	// sparks
	for (len=0 ; len<(2 + clamp (part_degree->value, 0, 2)) ; len++) {
		makePart (
			end[0] + (crand () * 2),			end[1] + (crand () * 2),			end[2] + (crand () * 2),
			0,									0,									0,
			crand () * 164,						crand () * 164,						crand () * 164,
			0,									0,									0,
			235 + (frand () * 20),				225 + (frand () * 20),				205,
			235 + (frand () * 20),				225 + (frand () * 20),				205,
			0.9,								-2.5 / (0.5 + (crand () * 0.15)),
			0.3,								0.4,
			PT_SPARK,							PF_DIRECTION,
			GL_SRC_ALPHA,						GL_ONE,
			pSparkGrowThink,					qTrue,
			13 + (crand () * 2));
	}
}


/*
===============
CG_Heatbeam
===============
*/
void CG_Heatbeam (vec3_t start, vec3_t forward) {
	vec3_t		move, vec, right, up, dir, end;
	int			i, rnum, rnum2;
	float		len, c, s, ltime, step = 32.0, rstep;
	float		start_pt, rot, variance;

	VectorMA (start, 4096, forward, end);

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec, vec);

	VectorCopy (cg.v_Right, right);
	VectorCopy (cg.v_Up, up);
	VectorMA (move, -0.5, right, move);
	VectorMA (move, -0.5, up, move);

	ltime = (float) cgs.realTime / 1000.0;
	start_pt = fmod (ltime * 96.0, step);
	VectorMA (move, start_pt, vec, move);

	VectorScale (vec, step, vec);

	rstep = M_PI / 10.0;
	for (i=start_pt ; i<len ; i+=step) {
		if (i==0)
			i=0.01;
		if (i > step * 5)
			break;

		variance = 0.5;
		for (rot=0 ; rot<M_PI_TIMES2 ; rot += rstep) {
			c = cos (rot) * variance;
			s = sin (rot) * variance;
			
			// trim it so it looks like it's starting at the origin
			if (i < 10) {
				VectorScale (right, c*(i/10.0), dir);
				VectorMA (dir, s*(i/10.0), up, dir);
			}
			else {
				VectorScale (right, c, dir);
				VectorMA (dir, s, up, dir);
			}

			if (pDegree (move, qFalse))
				continue;

			rnum = (rand () % 5);
			rnum2 = (rand () % 5);
			makePart (
				move[0] + dir[0]*3,				move[1] + dir[1]*3,				move[2] + dir[2]*3,
				0,								0,								0,
				0,								0,								0,
				0,								0,								0,
				palRed (223 - rnum),			palGreen (223 - rnum),			palBlue (223 - rnum),
				palRed (223 - rnum2),			palGreen (223 - rnum2),			palBlue (223 - rnum2),
				0.5,							-1000.0,
				1.0f,							1.0f,
				PT_WHITE,						PF_SCALED,
				GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
				0,								qFalse,
				0);
		}

		VectorAdd (move, vec, move);
	}
}


/*
===============
CG_IonripperTrail
===============
*/
void CG_IonripperTrail (vec3_t start, vec3_t end) {
	vec3_t	move, vec;
	vec3_t	tmpstart, tmpend;
	float	dec, len;
	int		left = 0, rnum, rnum2;

	// gloom optional spiral
	VectorCopy (start, tmpstart);
	VectorCopy (end, tmpend);
	if (cgi.FS_CurrGame ("gloom")) {
		float		c, s, i;
		vec3_t		dir, right, up;

		// I AM LAME ASS -- HEAR MY ROAR!
		tmpstart[2] += 12;
		tmpend[2] += 12;
		// I AM LAME ASS -- HEAR MY ROAR!

		if (glm_blobtype->integer) {
			VectorCopy (tmpstart, move);
			VectorSubtract (tmpend, tmpstart, vec);
			len = VectorNormalize (vec, vec);

			MakeNormalVectors (vec, right, up);

			for (i=0 ; i<len ; i++) {
				if (pDegree (move, qFalse))
					continue;

				c = cos (i);
				s = sin (i);

				VectorScale (right, c, dir);
				VectorMA (dir, s, up, dir);

				rnum = (rand () % 2) ? 110 + (frand () * 125) : 0;
				makePart (
					move[0] + (dir[0] * 1.15),		move[1] + (dir[1] * 1.15),		move[2] + (dir[2] * 1.15),
					0,								0,								0,
					dir[0] * 40,					dir[1] * 40,					dir[2] * 40,
					0,								0,								0,
					rnum,							rnum + 60 + (frand () * 130),	rnum + (frand () * 30),
					rnum,							rnum + 60 + (frand () * 130),	rnum + (frand () * 30),
					0.9,							-1.0 / (0.3 + (frand () * 0.2)),
					3.5,							1.8,
					PT_GREEN,						PF_SCALED,
					GL_SRC_ALPHA,					GL_ONE,
					0,								qFalse,
					0);

				VectorAdd (move, vec, move);
			}
		}
	}

	// bubbles
	CG_BubbleEffect (tmpstart);

	// trail
	VectorCopy (tmpstart, move);
	VectorSubtract (tmpend, tmpstart, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 5, 0.5, qTrue);
	VectorScale (vec, dec, vec);

	for (; len>0 ; VectorAdd (move, vec, move)) {
		len -= dec;

		rnum = rand () % 5;
		rnum2 = rand () % 5;
		makePart (
			move[0],							move[1],							move[2],
			0,									0,									0,
			left ? 15 : -15,					0,									0,
			0,									0,									0,
			palRed (0xe4 + rnum),				palGreen (0xe4 + rnum),				palBlue (0xe4 + rnum),
			palRed (0xe4 + rnum2),				palGreen (0xe4 + rnum2),			palBlue (0xe4 + rnum2),
			0.85,								-1.0 / (0.33 + (frand () * 0.2)),
			8,									3,
			PT_RED,								PF_SCALED,
			GL_SRC_ALPHA,						GL_ONE,
			0,									qFalse,
			0);

		left = left ? 0 : 1;
	}
}


/*
===============
CG_QuadTrail
===============
*/
void CG_QuadTrail (vec3_t start, vec3_t end) {
	vec3_t		move, vec;
	float		dec, len, rnum;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 4, 0.5, qTrue);
	VectorScale (vec, dec, vec);

	for (; len>0 ; VectorAdd (move, vec, move)) {
		len -= dec;

		rnum = (rand () % 2) ? (frand () * 200) : 0;
		makePart (
			move[0] + (crand () * 6),		move[1] + (crand () * 6),		move[2] + (crand () * 6),
			0,								0,								0,
			crand () * 8,					crand () * 8,					crand () * 8,
			0,								0,								0,
			rnum,							rnum + (frand () * 55),			rnum + 200 + (frand () * 50),
			rnum,							rnum + (frand () * 55),			rnum + 200 + (frand () * 50),
			1.0,							-1.0 / (0.8 + (frand () * 0.2)),
			5,								2,
			PT_FLARE,						PF_SCALED,
			GL_SRC_ALPHA,					GL_ONE,
			0,								qFalse,
			0);
	}
}


/*
===============
CG_RailTrail
===============
*/
void CG_RailTrail (vec3_t start, vec3_t end) {
	int			i;
	float		len;
	trace_t		tr;
	vec3_t		vec, move;

	// bubbles
	CG_BubbleTrail (start, end);

	if (cl_railtrail->integer >= 1) {
		// q3 school
		vec3_t		length;

		VectorSubtract(end, start, length);

		for (i=0 ; i<2 ; i++) {
			makePart (
				start[0],						start[1],						start[2],
				length[0],						length[1],						length[2],
				0,								0,								0,
				0,								0,								0,
				cl_railred->value * 255,		cl_railgreen->value * 255,		cl_railblue->value * 255,
				cl_railred->value * 255,		cl_railgreen->value * 255,		cl_railblue->value * 255,
				1.0f,							-1.3,
				0.8 + (i * 2),					1 + (i * 2),
				PT_BEAM,						PF_BEAM,
				GL_SRC_ALPHA,					(i)?GL_ONE:GL_ONE_MINUS_SRC_ALPHA,
				0,								qFalse,
				0);
		}

		if (cl_railtrail->integer >= 2) {
			// new school
			makePart (
				start[0],						start[1],						start[2],
				length[0],						length[1],						length[2],
				0,								0,								0,
				0,								0,								0,
				cl_spiralred->value * 255,		cl_spiralgreen->value * 255,	cl_spiralblue->value * 255,
				cl_spiralred->value * 255,		cl_spiralgreen->value * 255,	cl_spiralblue->value * 255,
				1.0f,							-2.4,
				1.25,							5.75,
				PT_WHITE,						PF_SPIRAL,
				GL_SRC_ALPHA,					GL_ONE,
				pRailSpiralThink,				qTrue,
				0);
		}
	}
	else {
		// old school
		float	d, c, s;
		float	dec;
		vec3_t	right, up, dir;

		VectorCopy (start, move);
		VectorSubtract (end, start, vec);
		len = VectorNormalize (vec, vec);

		MakeNormalVectors (vec, right, up);

		for (i=0 ; i<len ; i++, VectorAdd (move, vec, move)) {
			if (pDegree (move, qTrue))
				continue;

			d = i * 0.1;
			c = cos (d);
			s = sin (d);

			VectorScale (right, c, dir);
			VectorMA (dir, s, up, dir);

			makePart (
				move[0] + (dir[0] * 3),			move[1] + (dir[1] * 3),			move[2] + (dir[2] * 3),
				0,								0,								0,
				0,								0,								0,
				0,								0,								0,
				cl_spiralred->value * 255,		cl_spiralgreen->value * 255,	cl_spiralblue->value * 255,
				cl_spiralred->value * 255,		cl_spiralgreen->value * 255,	cl_spiralblue->value * 255,
				1.0,							-1.0 / (1 + (frand () * 0.2)),
				1.25f,							1.5f,
				PT_WHITE,						PF_SCALED,
				GL_SRC_ALPHA,					GL_ONE,
				0,								qFalse,
				0);
		}

		dec = pDecDegree (start, 3, 0.4, qFalse);
		VectorScale (vec, dec, vec);
		VectorCopy (start, move);

		for (; len>0 ; VectorAdd (move, vec, move)) {
			len -= dec;

			if (pDegree (move, qTrue))
				continue;

			makePart (
				move[0],						move[1],						move[2],
				0,								0,								0,
				crand () * 0.5,					crand () * 0.5,					crand () * 0.5,
				0,								0,								0,
				cl_railred->value * 255,		cl_railgreen->value * 255,		cl_railblue->value * 255,
				cl_railred->value * 255,		cl_railgreen->value * 255,		cl_railblue->value * 255,
				1.0,							-1.0 / (0.6 + (frand () * 0.2)),
				2,								4,
				pRandSmoke (),					PF_SHADE,
				GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
				pSmokeThink,					qTrue,
				frand () * 360);
		}
	}

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	VectorNormalize (vec, vec);

	VectorScale (vec, 2, vec);
	VectorAdd (move, vec, move);

	// kthx particle-based fast expanding ring?
	// make a decal
	tr = CGI_CM_Trace (move, end, 1, 1);
	if (tr.fraction < 1) {
		CG_SparkEffect (tr.endPos, tr.plane.normal, 10, 10, 20, 2, 3);

		// fast-fading mark
		makeDecal (
			tr.endPos[0],						tr.endPos[1],						tr.endPos[2],
			tr.plane.normal[0],					tr.plane.normal[1],					tr.plane.normal[2],
			cl_railred->value*255,				cl_railgreen->value*255,			cl_railblue->value*255,
			cl_railred->value*255,				cl_railgreen->value*255,			cl_railblue->value*255,
			1.0f,								0,
			10 + crand (),						0,
			PT_WHITE,							DF_NOTIMESCALE,
			GL_SRC_ALPHA,						GL_ONE,
			0,									qFalse,
			0.25 + (frand () * 0.25),			frand () * 360);

		// burn mark
		makeDecal (
			tr.endPos[0],						tr.endPos[1],						tr.endPos[2],
			tr.plane.normal[0],					tr.plane.normal[1],					tr.plane.normal[2],
			(cl_railred->value*128)+128,		(cl_railgreen->value*128)+128,		(cl_railblue->value*128)+128,
			0,									0,									0,
			0.7 + (crand () * 0.1),				0,
			10 + crand (),						0,
			DT_BURNMARK,						DF_ALPHACOLOR,
			GL_ZERO,							GL_ONE_MINUS_SRC_COLOR,
			0,									qFalse,
			0,									frand () * 360);

		// "flashing" glow marks
		makeDecal (
			tr.endPos[0],					tr.endPos[1],					tr.endPos[2],
			tr.plane.normal[0],				tr.plane.normal[1],				tr.plane.normal[2],
			cl_railred->value*255,			cl_railgreen->value*255,		cl_railblue->value*255,
			cl_railred->value*255,			cl_railgreen->value*255,		cl_railblue->value*255,
			1.0,							0,
			30,								0,
			PT_FLARE,						DF_NOTIMESCALE,
			GL_SRC_ALPHA,					GL_ONE,
			0,								qFalse,
			0.25 + (frand () * 0.25),		frand () * 360);

		if (cl_railtrail->integer != 1) {
			makeDecal (
				tr.endPos[0],					tr.endPos[1],					tr.endPos[2],
				tr.plane.normal[0],				tr.plane.normal[1],				tr.plane.normal[2],
				cl_spiralred->value*255,		cl_spiralgreen->value*255,		cl_spiralblue->value*255,
				cl_spiralred->value*255,		cl_spiralgreen->value*255,		cl_spiralblue->value*255,
				1.0,							0,
				12,								0,
				PT_FLARE,						DF_NOTIMESCALE,
				GL_SRC_ALPHA,					GL_ONE,
				0,								qFalse,
				0.25 + (frand () * 0.25),		frand () * 360);
		}
	}
}


/*
===============
CG_RocketTrail
===============
*/
void CG_RocketTrail (vec3_t start, vec3_t end) {
	int			fire;
	vec3_t		move, vec;
	float		len, dec;
	double		rnum, rnum2;
	qBool		inWater = qFalse;

	if (CGI_CM_PointContents (start, 0) & MASK_WATER)
		inWater = qTrue;

	// flare glow
	makePart (
		start[0],						start[1],						start[2],
		0,								0,								0,
		0,								0,								0,
		0,								0,								0,
		(inWater) ? 35 : 255,			(inWater) ? 30 : 170,			(inWater) ? 230 : 100,
		(inWater) ? 35 : 255,			(inWater) ? 30 : 170,			(inWater) ? 230 : 100,
		0.66 + (crand () * 0.3),		PART_INSTANT,
		14 + crand (),					15,
		PT_FLARE,						0,
		GL_SRC_ALPHA,					GL_ONE,
		0,								qFalse,
		0);

	// white flare core
	makePart (
		start[0],						start[1],						start[2],
		0,								0,								10,
		0,								0,								0,
		0,								0,								0,
		255,							255,							255,
		255,							255,							255,
		0.66 + (crand () * 0.3),		PART_INSTANT,
		12 + crand (),					15,
		PT_FLARE,						0,
		GL_SRC_ALPHA,					GL_ONE,
		0,								qFalse,
		0);

	// bubbles
	if (inWater)
		CG_BubbleTrail (start, end);

	VectorCopy (start, move);
	VectorSubtract (start, end, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 2.5 + (inWater ? 1 : 0), 0.33, qTrue);
	VectorScale (vec, dec, vec);

	fire = inWater ? PT_BLUEFIRE : pRandFire ();
	for (; len>0 ; VectorAdd (move, vec, move)) {
		len -= dec;

		if (rand () & 1) {
			// fire
			makePart (
				move[0] + crand (),					move[1] + crand (),					move[2] + crand (),
				0,									0,									0,
				crand () * 20,						crand () * 20,						crand () * 20,
				0,									0,									0,
				235 + (frand () * 20),				235 + (frand () * 20),				235 + (frand () * 20),
				235 + (frand () * 20),				235 + (frand () * 20),				235 + (frand () * 20),
				0.9 + (crand () * 0.1),				-1.0 / (0.2 + (crand () * 0.05)),
				3 + (frand () * 4),					10 + (frand () * 5),
				fire,								0,
				GL_SRC_ALPHA,						GL_ONE,
				pFireThink,							qTrue,
				frand () * 360);

			// embers
			if (!inWater && !(rand () % 5))
				makePart (
					move[0],							move[1],							move[2],
					0,									0,									0,
					crand (),							crand (),							crand (),
					0,									0,									0,
					235 + (frand () * 20),				235 + (frand () * 20),				235 + (frand () * 20),
					235 + (frand () * 20),				235 + (frand () * 20),				235 + (frand () * 20),
					0.8 + (crand () * 0.1),				-1.0 / (0.15 + (crand () * 0.05)),
					5 + (crand () * 4.5),				2 + (frand () * 3),
					pRandEmbers (),						0,
					GL_SRC_ALPHA,						GL_ONE,
					pFireThink,							qTrue,
					frand () * 360);
		}
		else {
			// fire
			makePart (
				move[0] + crand (),					move[1] + crand (),					move[2] + crand (),
				0,									0,									0,
				0,									0,									0,
				crand (),							crand (),							crand (),
				235 + (frand () * 20),				235 + (frand () * 20),				235 + (frand () * 20),
				235 + (frand () * 20),				235 + (frand () * 20),				235 + (frand () * 20),
				0.9 + (crand () * 0.1),				-1.0 / (0.25 + (crand () * 0.05)),
				5 + (frand () * 4),					4 + (frand () * 5),
				fire,								0,
				GL_SRC_ALPHA,						GL_ONE,
				pFireThink,							qTrue,
				frand () * 360);

			// dots
			makePart (
				move[0] + crand ()*2,				move[1] + crand ()*2,				move[2] + crand ()*2,
				0,									0,									0,
				0,									0,									0,
				crand () * 128,						crand () * 128,						crand () * 128,
				225 + (frand () * 30),				225 + (frand () * 30),				35 + (frand () * 100),
				225 + (frand () * 30),				225 + (frand () * 30),				35 + (frand () * 100),
				0.9 + (crand () * 0.1),				-1.0 / (0.25 + (crand () * 0.05)),
				1 + (crand () * 0.5),				0.5 + (crand () * 0.25),
				PT_WHITE,							0,
				GL_SRC_ALPHA,						GL_ONE,
				pFireThink,							qTrue,
				frand () * 360);
		}
	}

	// smoke
	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 16, 0.25, qTrue);
	VectorScale (vec, dec, vec);

	for (; len>0 ; VectorAdd (move, vec, move)) {
		len -= dec;

		rnum = (frand () * 60);
		rnum2 = (frand () * 60);
		makePart (
			move[0] + crand (),					move[1] + crand (),					move[2] + crand (),
			0,									0,									0,
			crand (),							crand (),							crand (),
			0,									0,									0.5 + (frand () * 0.5),
			50 + rnum,							50 + rnum,							50 + rnum,
			60 + rnum2,							60 + rnum2,							60 + rnum2,
			0.8 + (crand () * 0.1),				-1.0 / (1.5 + (cl_smokelinger->value * 0.5) + (crand () * 0.2)),
			10 + (crand () * 2),				13 + (crand () * 3),
			pRandSmoke (),						PF_SHADE,
			GL_SRC_ALPHA,						GL_ONE_MINUS_SRC_ALPHA,
			pSmokeTrailThink,					qTrue,
			frand () * 360);
	}
}


/*
===============
CG_TagTrail
===============
*/
void CG_TagTrail (vec3_t start, vec3_t end) {
	vec3_t		move, vec;
	float		len, dec;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 5, 0.5, qFalse);
	VectorScale (vec, dec, vec);

	while (len >= 0) {
		len -= dec;

		makePart (
			move[0] + (crand () * 16),		move[1] + (crand () * 16),		move[2] + (crand () * 16),
			0,								0,								0,
			crand () * 5,					crand () * 5,					crand () * 5,
			0,								0,								0,
			palRed (220),					palGreen (220),					palBlue (220),
			palRed (220),					palGreen (220),					palBlue (220),
			1.0,							-1.0 / (0.8 + (frand () * 0.2)),
			1.0,							1.0,
			PT_WHITE,						PF_SCALED,
			GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
			0,								qFalse,
			0);

		VectorAdd (move, vec, move);
	}
}


/*
===============
CG_TrackerTrail
===============
*/
void CG_TrackerTrail (vec3_t start, vec3_t end) {
	vec3_t		move, vec, porg;
	vec3_t		forward, right, up, angle_dir;
	float		len, dec, dist;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec, vec);

	VectorCopy (vec, forward);
	VecToAngles (forward, angle_dir);
	AngleVectors (angle_dir, forward, right, up);

	dec = pDecDegree (start, 3, 0.5, qTrue);
	VectorScale (vec, dec, vec);

	for (; len>0 ; VectorAdd (move, vec, move)) {
		len -= dec;

		dist = DotProduct (move, forward);
		VectorMA (move, 8 * cos (dist), up, porg);

		makePart (
			porg[0],						porg[1],						porg[2],
			0,								0,								0,
			0,								0,								5,
			0,								0,								0,
			0,								0,								0,
			0,								0,								0,
			1.0,							-2.0,
			1.0f,							1.0f,
			PT_WHITE,						PF_SCALED,
			GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
			0,								qFalse,
			0);
	}
}
