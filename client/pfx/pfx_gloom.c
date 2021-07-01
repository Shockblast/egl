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

// pfx_gloom.c

#include "../cl_local.h"

// kthx a shining example of why I need a cgame dll?

/*
==============================================================

	GLOOM SPECIFIC PARTICLE EFFECTS

==============================================================
*/

/*
===============
CL_GloomBlobTip
===============
*/
void CL_GloomBlobTip (vec3_t start, vec3_t end)
{
	vec3_t	move, vec, tmpstart, tmpend;
	float	len, dec, rnum, rnum2;

	// I AM LAME ASS -- HEAR MY ROAR!
	VectorCopy (start, tmpstart);
	VectorCopy (end, tmpend);
	tmpstart[2] += 12;
	tmpend[2] += 12;
	// I AM LAME ASS -- HEAR MY ROAR!

	/* bubbles */
	CL_BubbleEffect (tmpstart);

	VectorCopy (tmpstart, move);
	VectorSubtract (tmpstart, tmpend, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (tmpstart, 2, 0.5, qTrue);
	VectorScale (vec, dec, vec);

	for (; len>0 ; VectorAdd (move, vec, move))
	{
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
			0.9,								-15,
			3.5 + (frand () * 4),				3.5 + (frand () * 2.5),
			PT_FLARE,							PF_SCALED|PF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			0,									qFalse,
			0);
	}
}


/*
===============
CL_GloomDroneEffect
===============
*/
void CL_GloomDroneEffect (vec3_t org, vec3_t dir)
{
	int			i;
	float		d;
	double		rnum, rnum2;

	/* decal */
	for (i=0 ; i<2 ; i++)
	{
		rnum = 10 + (frand () * 30);
		rnum2 = (frand () * 40);
		makeDecal (
			org[0],								org[1],								org[2],
			dir[0],								dir[1],								dir[2],
			200 * i,							215 + (rnum * (1 - i)),				200 * i,
			200 * i,							215 + (rnum2 * (1 - i)),			200 * i,
			1.0f,								0,
			12 + crand () - (4.5 * i),			0,
			PT_FLARE,							DF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			0,									qFalse,
			0,									frand () * 360);
	}

	/* particles */
	for (i=0 ; i<pIncDegree (org, 40, 0.5, qTrue) ; i++)
	{
		d = 2 + (frand () * 22);
		rnum = 20 + (frand () * 30);
		rnum2 = (frand () * 40) + (rand () % 2 * 30);

		if (rand () % 2)
		{
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
			1.0,								-1.0 / (0.5 + (frand () * 0.3)),
			7 + crand (),						3 + crand (),
			PT_FLARE,							PF_SCALED|PF_GRAVITY|PF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			pBounceThink,						qTrue,
			0);
	}
}


/*
===============
CL_GloomEmberTrail
===============
*/
void CL_GloomEmberTrail (vec3_t start, vec3_t end)
{
	double	rnum, rnum2;
	float	dec, len;
	vec3_t	move, vec;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 14, 0.25, qTrue);
	VectorScale (vec, dec, vec);

	for (; len>0 ; VectorAdd (move, vec, move))
	{
		len -= dec;

		/* explosion anim */
		makePart (
			move[0] + (crand () * 2),			move[1] + (crand () * 2),			move[2] + (crand () * 2),
			0,									0,									0,
			0,									0,									0,
			0,									0,									0,
			245,								245,								255,
			245,								245,								255,
			1.0,								-3.5 + (crand () * 0.1),
			10 + (crand () * 5),				70 + (crand () * 10),
			PT_EXPLO1 + (rand () % 6),			PF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			0,									qFalse,
			crand () * 12);

		/* smoke */
		rnum = (frand () * 80);
		rnum2 = (frand () * 80);
		makePart (
			start[0] + (crand () * 4),		start[1] + (crand () * 4),		start[2] + (crand () * 4),
			0,								0,								0,
			crand () * 2,					crand () * 2,					crand () * 2,
			crand () * 2,					crand () * 2,					crand () + (rand () % 4),
			50 + rnum,						50 + rnum,						50 + rnum,
			75 + rnum2,						75 + rnum2,						75 + rnum2,
			0.8,							-1.0 / (0.125 + (cl_smokelinger->value * 0.1) + (crand () * 0.1)),
			20 + (crand () * 5),			60 + (crand () * 10),
			pRandSmoke (),					PF_SHADE,
			GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
			pSmokeTrailThink,				qTrue,
			frand () * 361);
	}
}


/*
===============
CL_GloomFlareTrail
===============
*/
void CL_GloomFlareTrail (vec3_t start, vec3_t end)
{
	vec3_t		move, vec;
	float		len, dec;
	float		rnum, rnum2;

	/* tip */
	CL_FlareEffect (start, PT_FLARE, 0, 25, 25, 0x0f, 0x0f, 0.66 + (frand () * 0.1), PART_INSTANT); // core
	CL_FlareEffect (start, PT_FLARE, 0, 30, 30, 0xd0, 0xd0, 0.66 + (frand () * 0.1), PART_INSTANT); // outer

	/* following blur */
	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 8, 0.25, qTrue);
	VectorScale (vec, dec, vec);

	for (; len>0 ; VectorAdd (move, vec, move))
	{
		len -= dec;

		/* smoke */
		if (!(rand () % 2))
		{
			rnum = (frand () * 80);
			rnum2 = (frand () * 80);
			makePart (
				start[0] + (crand () * 2),		start[1] + (crand () * 2),		start[2] + (crand () * 2),
				0,								0,								0,
				crand () * 3,					crand () * 3,					crand () * 3,
				0,								0,								5,
				50 + rnum,						50 + rnum,						50 + rnum,
				75 + rnum2,						75 + rnum2,						75 + rnum2,
				0.3 + (frand () * 0.1),			-1.0 / (1.5 + (cl_smokelinger->value * 0.5) + (crand () * 0.2)),
				10 + (crand () * 5),			30 + (crand () * 5),
				pRandSmoke (),					PF_SHADE,
				GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
				pSmokeThink,					qTrue,
				frand () * 360);
		}

		/* glowing trail */
		CL_FlareEffect (move, PT_FLARE, 0, 15, 10, 0x0f, 0x0f, 0.5 + (frand () * 0.1), -2.25); // core
		CL_FlareEffect (move, PT_FLARE, 0, 20, 15, 0xd0, 0xd0, 0.5 + (frand () * 0.1), -2.25); // outer
	}
}


/*
===============
CL_GloomGasEffect
===============
*/
void CL_GloomGasEffect (vec3_t origin)
{
	float	rnum, rnum2;
	int		i;

	for (i=0 ; i<pIncDegree (origin, 1.25, 0.25, qTrue) ; i++)
	{
		makePart (
			origin[0],						origin[1],						origin[2],
			0,								0,								0,
			0,								0,								0,
			0,								0,								(frand () * 3),
			60,								90,								60,
			60,								90,								60,
			0.4,							-100,
			100,							100,
			i?PT_SMOKE:PT_SMOKE2,			PF_SCALED,
			GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
			pSmokeThink,					qTrue,
			(cls.realTime / 1000) % 361);
	}

	if (rand () % 5) return;

	rnum = (rand () % 61);
	rnum2 = (rand () % 61);
	makePart (
		origin[0] + (crand () * 2),		origin[1] + (crand () * 2),		origin[2] + (crand () * 2),
		0,								0,								0,
		0,								0,								0,
		0,								0,								(frand () * 3),
		70 + rnum,						100 + rnum,						70 + rnum,
		70 + rnum2,						110 + rnum2,					70 + rnum2,
		0.4,							-1.0 / (5.1 + (frand () * 0.2)),
		65 + (frand () * 15),			150 + (frand () * 15),
		pRandSmoke (),					PF_SHADE,
		GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
		pSmokeThink,					qTrue,
		frand () * 360);
}


/*
===============
CL_GloomRepairEffect
===============
*/
void CL_GloomRepairEffect (vec3_t org, vec3_t dir, int count)
{
	int			i, rnum, rnum2;
	float		d;

	for (i=0 ; i<2 ; i++)
	{
		/* glow marks */
		rnum = (rand () % 5);
		rnum2 = (rand () % 5);
		makeDecal (
			org[0],								org[1],								org[2],
			dir[0],								dir[1],								dir[2],
			palRed (0xe0 + rnum),				palGreen (0xe0 + rnum),				palBlue (0xe0 + rnum),
			palRed (0xe0 + rnum2),				palGreen (0xe0 + rnum2),			palBlue (0xe0 + rnum2),
			1.0f,								0,
			3 + (frand () * 0.5),				0,
			PT_RED,								DF_NOTIMESCALE|DF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			0,									qFalse,
			DECAL_GLOWTIME,						frand () * 360);
	}

	/* burn mark */
	rnum = (rand () % 5);
	makeDecal (
		org[0],								org[1],								org[2],
		dir[0],								dir[1],								dir[2],
		(255-palRed(0xe0+rnum))*0.5+128,	(255-palGreen(0xe0+rnum))*0.5+128,	(255-palBlue(0xe0+rnum))*0.5+128,
		0,									0,									0,
		0.5 + (crand () * 0.1),				0,
		2 + (frand () * 0.5),				0,
		dRandBurnMark (),					DF_ALPHACOLOR,
		GL_ZERO,							GL_ONE_MINUS_SRC_COLOR,
		0,									qFalse,
		0,									frand () * 360);

	/* dots */
	for (i=0 ; i<pIncDegree (org, count*2, 0.5, qTrue) ; i++)
	{
		d = (frand () * 5) + 2;
		rnum = (rand () % 5);
		makePart (
			org[0] + (crand () * 4) + d*dir[0],	org[1] + (crand () * 4) + d*dir[1],	org[2] + (crand () * 4) + d*dir[2],
			0,									0,									0,
			crand () * 18,						crand () * 18,						crand () * 18,
			0,									0,									40,
			palRed (0xe0 + rnum),				palGreen (0xe0 + rnum),				palBlue (0xe0 + rnum),
			palRed (0xe0 + rnum),				palGreen (0xe0 + rnum),				palBlue (0xe0 + rnum),
			1.0,								-1.0 / (0.5 + (frand () * 0.3)),
			6 + (frand () * -5.75),				0.5 + (crand () * 0.45),
			PT_RED,								PF_SCALED|PF_GRAVITY|PF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			pBounceThink,						qTrue,
			0);
	}
}


/*
===============
CL_GloomStingerFire
===============
*/
void CL_GloomStingerFire (vec3_t start, vec3_t end, double size, qBool light)
{
	vec3_t	move, vec;
	float	len, dec, waterscale;
	qBool	inwater = qFalse;
	int		tipimage, trailimage;

	if (CM_PointContents (start, 0) & MASK_WATER)
		inwater = qTrue;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 30, 0.4, qTrue);
	VectorScale (vec, dec, vec);

	if (light)
	{
		if (inwater || glm_bluestingfire->integer)
		{
			R_AddLight (start,	175, 0.1, 0, 0.9);
			R_AddLight (end,	50, 0.1, 0, 0.9);
		}
		else
		{
			R_AddLight (start,	175, 0.9, 0.8, 0);
			R_AddLight (end,	50, 0.9, 0.8, 0);
		}
	}

	if (glm_bluestingfire->integer)
		tipimage = trailimage = PT_BLUEFIRE;
	else
	{
		tipimage = (inwater) ? PT_BLUEFIRE : PT_FIRE;
		trailimage = PT_FIRE;
	}

	/* tip */
	waterscale = (inwater || glm_bluestingfire->integer) ? 100 : 0;
	makePart (
		start[0],								start[1],								start[2],
		0,										0,										0,
		crand () * 2,							crand () * 2,							crand () * 2,
		0,										0,										0,
		235 + (frand () * 20) - waterscale,		230 + (frand () * 20) - waterscale,		220 + (frand () * 20),
		235 + (frand () * 20) - waterscale,		230 + (frand () * 20) - waterscale,		220 + (frand () * 20),
		0.5,									-0.2 / (0.05 + (frand () * 0.1)),
		size,									2,
		tipimage,								PF_SCALED|PF_ALPHACOLOR,
		GL_SRC_ALPHA,							GL_ONE,
		pFireThink,								qTrue,
		frand () * 360);

	/* fire */
	for (; len>0 ; VectorAdd (move, vec, move))
	{
		len -= dec;

		makePart (
			move[0],								move[1],								move[2],
			0,										0,										0,
			crand () * 2,							crand () * 2,							crand () * 2,
			0,										0,										0,
			235 + (frand () * 20) - waterscale,		230 + (frand () * 20) - waterscale,		220 + (frand () * 20) - waterscale,
			235 + (frand () * 20) - waterscale,		230 + (frand () * 20) - waterscale,		220 + (frand () * 20) - waterscale,
			0.5,									-0.2 / (0.05 + (frand () * 0.1)),
			size * 0.75,							1,
			trailimage,								PF_SCALED|PF_ALPHACOLOR,
			GL_SRC_ALPHA,							GL_ONE,
			pFireThink,								qTrue,
			frand () * 360);
	}
}
