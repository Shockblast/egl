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
// cg_partgloom.c
//

#include "cg_local.h"

/*
=============================================================================

	GLOOM SPECIFIC PARTICLE EFFECTS

=============================================================================
*/

/*
===============
CG_GloomBlobTip
===============
*/
void CG_GloomBlobTip (vec3_t start, vec3_t end)
{
	vec3_t	move, vec, tmpstart, tmpend;
	float	len, dec, rnum, rnum2;

	// I AM LAME ASS -- HEAR MY ROAR!
	VectorCopy (start, tmpstart);
	VectorCopy (end, tmpend);
	tmpstart[2] += 12;
	tmpend[2] += 12;
	// I AM LAME ASS -- HEAR MY ROAR!

	// bubbles
	CG_BubbleEffect (tmpstart);

	VectorCopy (tmpstart, move);
	VectorSubtract (tmpstart, tmpend, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (tmpstart, 2, 0.5, qTrue);
	VectorScale (vec, dec, vec);

	for (; len>0 ; VectorAdd (move, vec, move)) {
		len -= dec;

		rnum = (rand () % 2) ? 100 + (frand () * 30) : (frand () * 30);
		rnum2 = (frand () * 40);
		makePart (
			move[0] + crand (),					move[1] + crand (),					move[2] + crand (),
			0,									0,									0,
			0,									0,									0,
			0,									0,									0,
			rnum,								200 + rnum2,						rnum,
			rnum,								200 + rnum2,						rnum,	
			0.9f,								-15,
			3.5f + (frand () * 4),				3.5f + (frand () * 2.5f),
			PT_FLAREGLOW,						PF_SCALED|PF_ALPHACOLOR,
			0,									qFalse,
			0);
	}
}


/*
===============
CG_GloomDroneEffect
===============
*/
void CG_GloomDroneEffect (vec3_t org, vec3_t dir)
{
	float	i, d;
	float	rnum, rnum2;

	// Decal
	for (i=0 ; i<2 ; i++) {
		rnum = 10 + (frand () * 30);
		rnum2 = (frand () * 40);
		makeDecal (
			org[0],								org[1],								org[2],
			dir[0],								dir[1],								dir[2],
			200 * i,							215 + (rnum * (1 - i)),				200 * i,
			200 * i,							215 + (rnum2 * (1 - i)),			200 * i,
			1.0f,								0,
			12 + crand () - (4.5f * i),
			DT_DRONE_SPIT_GLOW,					DF_ALPHACOLOR,
			0,									qFalse,
			0,									frand () * 360);
	}

	// Particles
	for (i=0 ; i<pIncDegree (org, 40, 0.5, qTrue) ; i++) {
		d = 2 + (frand () * 22);
		rnum = 20 + (frand () * 30);
		rnum2 = (frand () * 40) + (rand () % 2 * 30);

		if (rand () % 2) {
			rnum += 90 + (frand () * 10);
			rnum2 += 40 + (frand () * 10);
		}

		makePart (
			org[0] + (crand () * 4) + d*dir[0],	org[1] + (crand () * 4) + d*dir[1],	org[2] + (crand () * 4) + d*dir[2],
			0,									0,									0,
			crand () * 20,						crand () * 20,						crand () * 20,
			0,									0,									-40,
			rnum,								150 + rnum2,						rnum,
			rnum,								150 + rnum2,						rnum,	
			1.0f,								-1.0f / (0.5f + (frand () * 0.3f)),
			7 + crand (),						3 + crand (),
			PT_FLAREGLOW,						PF_SCALED|PF_GRAVITY|PF_ALPHACOLOR|PF_NOCLOSECULL,
			pBounceThink,						qTrue,
			0);
	}
}


/*
===============
CG_GloomEmberTrail
===============
*/
void CG_GloomEmberTrail (vec3_t start, vec3_t end)
{
	float	rnum, rnum2;
	float	dec, len;
	vec3_t	move, vec;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 14, 0.25, qTrue);
	VectorScale (vec, dec, vec);

	for (; len>0 ; VectorAdd (move, vec, move)) {
		len -= dec;

		// Explosion anim
		makePart (
			move[0] + (crand () * 2),			move[1] + (crand () * 2),			move[2] + (crand () * 2),
			0,									0,									0,
			0,									0,									0,
			0,									0,									0,
			245,								245,								255,
			245,								245,								255,
			1.0f,								-3.5f + (crand () * 0.1f),
			10 + (crand () * 5),				70 + (crand () * 10),
			PT_EXPLO1 + (rand () % 6),			PF_ALPHACOLOR,
			0,									qFalse,
			crand () * 12);

		// Smoke
		rnum = 60 + (frand () * 50);
		rnum2 = 70 + (frand () * 50);
		makePart (
			start[0] + (crand () * 4),		start[1] + (crand () * 4),		start[2] + (crand () * 4),
			0,								0,								0,
			crand () * 2,					crand () * 2,					crand () * 2,
			crand () * 2,					crand () * 2,					crand () + (rand () % 4),
			rnum,							rnum,							rnum,
			rnum2,							rnum2,							rnum2,
			0.8f,							-1.0f / (0.125f + (cg_particleSmokeLinger->value * 0.1f) + (crand () * 0.1f)),
			20 + (crand () * 5),			60 + (crand () * 10),
			pRandSmoke (),					PF_SHADE,
			NULL,							0,
			frand () * 361);
	}
}


/*
===============
CG_GloomFlareTrail
===============
*/
void CG_GloomFlareTrail (vec3_t start, vec3_t end)
{
	vec3_t		move, vec;
	float		len, dec;
	float		rnum, rnum2;

	// Tip
	CG_FlareEffect (start, PT_FLAREGLOW, 0, 25, 25, 0x0f, 0x0f, 0.66f + (frand () * 0.1f), PART_INSTANT); // core
	CG_FlareEffect (start, PT_FLAREGLOW, 0, 30, 30, 0xd0, 0xd0, 0.66f + (frand () * 0.1f), PART_INSTANT); // outer

	// Following blur
	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 8, 0.25, qTrue);
	VectorScale (vec, dec, vec);

	for (; len>0 ; VectorAdd (move, vec, move)) {
		len -= dec;

		// Smoke
		if (!(rand () % 2)) {
			rnum = 60 + (frand () * 50);
			rnum2 = 70 + (frand () * 50);
			makePart (
				start[0] + (crand () * 2),		start[1] + (crand () * 2),		start[2] + (crand () * 2),
				0,								0,								0,
				crand () * 3,					crand () * 3,					crand () * 3,
				0,								0,								5,
				rnum,							rnum,							rnum,
				rnum2,							rnum2,							rnum2,
				0.3f + (frand () * 0.1f),		-1.0f / (1.5f + (cg_particleSmokeLinger->value * 0.5f) + (crand () * 0.2f)),
				10 + (crand () * 5),			30 + (crand () * 5),
				pRandSmoke (),					PF_SHADE,
				pSmokeThink,					qTrue,
				frand () * 360);
		}

		// Glowing trail
		CG_FlareEffect (move, PT_FLAREGLOW, 0, 15, 10, 0x0f, 0x0f, 0.5f + (frand () * 0.1f), -2.25); // core
		CG_FlareEffect (move, PT_FLAREGLOW, 0, 20, 15, 0xd0, 0xd0, 0.5f + (frand () * 0.1f), -2.25); // outer
	}
}


/*
===============
CG_GloomGasEffect
===============
*/
void CG_GloomGasEffect (vec3_t origin)
{
	float	rnum, rnum2;

	if (rand () & 3)
		return;

	rnum = (float)(rand () % 61);
	rnum2 = (float)(rand () % 61);
	makePart (
		origin[0] + (crand () * 2),		origin[1] + (crand () * 2),		origin[2] + (crand () * 2),
		0,								0,								0,
		0,								0,								0,
		0,								0,								(frand () * 3),
		70 + rnum,						100 + rnum,						70 + rnum,
		70 + rnum2,						110 + rnum2,					70 + rnum2,
		0.35f,							-1.0f / (5.1f + (frand () * 0.2f)),
		30 + (frand () * 10),			300 + (crand () * 50),
		pRandSmoke (),					PF_SHADE,
		pSmokeThink,					qTrue,
		frand () * 360);
}


/*
===============
CG_GloomRepairEffect
===============
*/
void CG_GloomRepairEffect (vec3_t org, vec3_t dir, int count)
{
	int			i, rnum, rnum2;
	float		d;

	for (i=0 ; i<2 ; i++) {
		// Glow marks
		rnum = (rand () % 5);
		rnum2 = (rand () % 5);
		makeDecal (
			org[0],								org[1],								org[2],
			dir[0],								dir[1],								dir[2],
			palRed (0xe0 + rnum),				palGreen (0xe0 + rnum),				palBlue (0xe0 + rnum),
			palRed (0xe0 + rnum2),				palGreen (0xe0 + rnum2),			palBlue (0xe0 + rnum2),
			1.0f,								0,
			3 + (frand () * 0.5f),
			DT_ENGYREPAIR_GLOWMARK,				DF_NOTIMESCALE|DF_ALPHACOLOR,
			0,									qFalse,
			DECAL_GLOWTIME,						frand () * 360);
	}

	// Burn mark
	rnum = (rand () % 5);
	makeDecal (
		org[0],								org[1],								org[2],
		dir[0],								dir[1],								dir[2],
		(255-palRed(0xe0+rnum))*0.5f+128,	(255-palGreen(0xe0+rnum))*0.5f+128,	(255-palBlue(0xe0+rnum))*0.5f+128,
		0,									0,									0,
		0.9f + (crand () * 0.1f),			0,
		2 + (frand () * 0.5f),
		DT_ENGYREPAIR_BURNMARK,				DF_ALPHACOLOR,
		0,									qFalse,
		0,									frand () * 360);

	// Dots
	for (i=0 ; i<(int)pIncDegree (org, count*2.0f, 0.5f, qTrue) ; i++) {
		d = (frand () * 5) + 2;
		rnum = (rand () % 5);
		makePart (
			org[0] + (crand () * 4) + d*dir[0],	org[1] + (crand () * 4) + d*dir[1],	org[2] + (crand () * 4) + d*dir[2],
			0,									0,									0,
			crand () * 18,						crand () * 18,						crand () * 18,
			0,									0,									40,
			palRed (0xe0 + rnum),				palGreen (0xe0 + rnum),				palBlue (0xe0 + rnum),
			palRed (0xe0 + rnum),				palGreen (0xe0 + rnum),				palBlue (0xe0 + rnum),
			1.0f,								-1.0f / (0.5f + (frand () * 0.3f)),
			6 + (frand () * -5.75f),			0.5f + (crand () * 0.45f),
			PT_ENGYREPAIR_DOT,					PF_SCALED|PF_GRAVITY|PF_ALPHACOLOR|PF_NOCLOSECULL,
			pBounceThink,						qTrue,
			0);
	}
}


/*
===============
CG_GloomStingerFire
===============
*/
void CG_GloomStingerFire (vec3_t start, vec3_t end, float size, qBool light)
{
	vec3_t	move, vec;
	float	len, dec, waterScale;
	qBool	inWater = qFalse;
	int		tipimage, trailimage;

	if (CGI_CM_PointContents (start, 0) & MASK_WATER)
		inWater = qTrue;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 30, 0.4f, qTrue);
	VectorScale (vec, dec, vec);

	if (light) {
		if (inWater || glm_bluestingfire->integer) {
			cgi.R_AddLight (start,	175, 0.1f, 0, 0.9f);
			cgi.R_AddLight (end,	50, 0.1f, 0, 0.9f);
		}
		else {
			cgi.R_AddLight (start,	175, 0.9f, 0.8f, 0);
			cgi.R_AddLight (end,	50, 0.9f, 0.8f, 0);
		}
	}

	if (glm_bluestingfire->integer)
		tipimage = trailimage = PT_BLUEFIRE;
	else {
		tipimage = (inWater) ? PT_BLUEFIRE : pRandFire ();
		trailimage = pRandFire ();
	}

	waterScale = (inWater || glm_bluestingfire->integer) ? 100.0f : 0.0f;
	if (rand () & 1) {
		// Tip
		makePart (
			start[0] + (crand () * 2),				start[1] + (crand () * 2),				start[2] + (crand () * 2),
			0,										0,										0,
			crand () * 2,							crand () * 2,							crand () * 2,
			0,										0,										0,
			235 + (frand () * 20) - waterScale,		230 + (frand () * 20) - waterScale,		220 + (frand () * 20),
			235 + (frand () * 20) - waterScale,		230 + (frand () * 20) - waterScale,		220 + (frand () * 20),
			0.6f + (crand () * 0.1f),				-0.3f / (0.05f + (frand () * 0.1f)),
			size + (crand () * 2),					(size * 0.25f) + (crand () * 3),
			tipimage,								PF_SCALED|PF_ALPHACOLOR,
			pFireThink,								qTrue,
			frand () * 360);
	}
	else {
		// Parts that spiral away
		makePart (
			start[0] + (crand () * 2),				start[1] + (crand () * 2),				start[2] + (crand () * 2),
			0,										0,										0,
			crand () * 80,							crand () * 80,							crand () * 80,
			0,										0,										0,
			235 + (frand () * 20) - waterScale,		230 + (frand () * 20) - waterScale,		220 + (frand () * 20),
			235 + (frand () * 20) - waterScale,		230 + (frand () * 20) - waterScale,		220 + (frand () * 20),
			0.6f + (crand () * 0.1f),				-0.3f / (0.05f + (frand () * 0.1f)),
			(size * 0.6f) + (crand () * 2),			(size * 1.2f) + (crand () * 2),
			tipimage,								PF_SCALED|PF_ALPHACOLOR,
			pFireThink,								qTrue,
			frand () * 360);
	}

	// Fire
	for (; len>0 ; VectorAdd (move, vec, move)) {
		len -= dec;

		makePart (
			move[0] + (crand () * 8),				move[1] + (crand () * 8),				move[2] + (crand () * 8),
			0,										0,										0,
			crand () * 2,							crand () * 2,							crand () * 2,
			0,										0,										0,
			235 + (frand () * 20) - waterScale,		230 + (frand () * 20) - waterScale,		220 + (frand () * 20) - waterScale,
			235 + (frand () * 20) - waterScale,		230 + (frand () * 20) - waterScale,		220 + (frand () * 20) - waterScale,
			0.6f + (frand () * 0.2f),				-0.25f / (0.05f + (frand () * 0.1f)),
			(size * 0.8f) + (crand () * 2),			2 + crand (),
			trailimage,								PF_SCALED|PF_ALPHACOLOR,
			pFireThink,								qTrue,
			frand () * 360);
	}
}
