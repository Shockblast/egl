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

// pfx_effects.c

#include "../cl_local.h"

/*
==============================================================

	PARTICLE EFFECTS

==============================================================
*/

/*
===============
CL_BlasterBlueParticles
===============
*/
void CL_BlasterBlueParticles (vec3_t org, vec3_t dir)
{
	int			i, count;
	int			rnum, rnum2;
	float		d;

	/* glow marks */
	for (i=0 ; i<2 ; i++)
	{
		rnum = (rand () % 5);
		rnum2 = (rand () % 5);
		makeDecal (
			org[0],								org[1],								org[2],
			dir[0],								dir[1],								dir[2],
			palRed (0x70 + rnum),				palGreen (0x70 + rnum),				palBlue (0x70 + rnum),
			palRed (0x70 + rnum2),				palGreen (0x70 + rnum2),			palBlue (0x70 + rnum2),
			1,									0,
			7 + (frand() * 0.5),				0,
			PT_BLUE,							DF_NOTIMESCALE|DF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			0,									qFalse,
			DECAL_GLOWTIME,						frand () * 360);
	}

	/* burn mark */
	rnum = (rand () % 5);
	makeDecal (
		org[0],								org[1],								org[2],
		dir[0],								dir[1],								dir[2],
		(255-palRed(0x70+rnum))*0.5+128,	(255-palGreen(0x70+rnum))*0.5+128,	(255-palBlue(0x70+rnum))*0.5+128,
		0,									0,									0,
		0.5 + (crand() * 0.1),				0,
		5 + (frand() * 0.5),				0,
		dRandBurnMark (),					DF_ALPHACOLOR,
		GL_ZERO,							GL_ONE_MINUS_SRC_COLOR,
		0,									qFalse,
		0,									frand () * 360);

	/* smoke */
	count = pIncDegree (org, 6 + (cl_smokelinger->value * 0.25), 0.5, qTrue);
	for (i=0 ; i<count ; i++)
	{
		d = 3 + (frand () * 6);
		rnum = (rand () % 5);

		makePart (
			org[0] + (d*dir[0]) + (crand()*2),	org[1] + (d*dir[1]) + (crand()*2),	org[2] + (d*dir[2]) + (crand()*2),
			0,									0,									0,
			0,									0,									0,
			0,									0,									5 + (frand () * 25),
			palRed (0x70 + rnum),				palGreen (0x70 + rnum),				palBlue (0x70 + rnum),
			palRed (0x70 + rnum),				palGreen (0x70 + rnum),				palBlue (0x70 + rnum),
			0.9 + (frand() * 0.1),				-1 / (0.8 + (cl_smokelinger->value * 0.01) + (frand() * 0.1)),
			2 + crand (),						16 + (crand () * 8),
			pRandSmoke (),						PF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			pSmokeTrailThink,					qTrue,
			frand () * 360);
	}

	/* dots */
	count = pIncDegree (org, 60, 0.5, qTrue);
	for (i=0 ; i<count ; i++)
	{
		d = 6 + (frand () * 12);
		rnum = (rand () % 5);

		makePart (
			org[0] + (d*dir[0]) + (crand()*4),	org[1] + (d*dir[1]) + (crand()*4),	org[2] + (d*dir[2]) + (crand()*4),
			0,									0,									0,
			(dir[0] * 25) + (crand () * 35),	(dir[1] * 25) + (crand () * 35),	(dir[2] * 25) + (crand () * 35),
			0,									0,									-(frand () * 10),
			palRed (0x70 + rnum),				palGreen (0x70 + rnum),				palBlue (0x70 + rnum),
			palRed (0x70 + rnum),				palGreen (0x70 + rnum),				palBlue (0x70 + rnum),
			0.9 + (frand () * 0.1),				-1 / (1 + frand () * 0.3),
			11 + (frand () * -10.75),			0.1 + (frand () * 0.5),
			PT_BLUE,							PF_SCALED|PF_GRAVITY|PF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			pBounceThink,						qTrue,
			0);
	}
}


/*
===============
CL_BlasterGoldParticles
===============
*/
void CL_BlasterGoldParticles (vec3_t org, vec3_t dir)
{
	int			i, count;
	int			rnum, rnum2;
	float		d;

	/* glow marks */
	for (i=0 ; i<2 ; i++)
	{
		rnum = (rand () % 5);
		rnum2 = (rand () % 5);
		makeDecal (
			org[0],								org[1],								org[2],
			dir[0],								dir[1],								dir[2],
			palRed (0xe0 + rnum),				palGreen (0xe0 + rnum),				palBlue (0xe0 + rnum),
			palRed (0xe0 + rnum2),				palGreen (0xe0 + rnum2),			palBlue (0xe0 + rnum2),
			1,									0,
			7 + (frand() * 0.5),				0,
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
		0.5 + (crand() * 0.1),				0,
		5 + (frand() * 0.5),				0,
		dRandBurnMark (),					DF_ALPHACOLOR,
		GL_ZERO,							GL_ONE_MINUS_SRC_COLOR,
		0,									qFalse,
		0,									frand () * 360);

	/* smoke */
	count = pIncDegree (org, 6 + (cl_smokelinger->value * 0.25), 0.5, qTrue);
	for (i=0 ; i<count ; i++)
	{
		d = 3 + (frand () * 6);
		rnum = (rand () % 5);

		makePart (
			org[0] + (d*dir[0]) + (crand()*2),	org[1] + (d*dir[1]) + (crand()*2),	org[2] + (d*dir[2]) + (crand()*2),
			0,									0,									0,
			0,									0,									0,
			0,									0,									5 + (frand () * 25),
			palRed (0xe0 + rnum),				palGreen (0xe0 + rnum),				palBlue (0xe0 + rnum),
			palRed (0xe0 + rnum2),				palGreen (0xe0 + rnum2),			palBlue (0xe0 + rnum2),
			0.9 + (frand() * 0.1),				-1 / (0.8 + (cl_smokelinger->value * 0.01) + (frand() * 0.1)),
			2 + crand (),						16 + (crand () * 8),
			pRandSmoke (),						PF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			pSmokeTrailThink,					qTrue,
			frand () * 360);
	}

	/* dots */
	count = pIncDegree (org, 60, 0.5, qTrue);
	for (i=0 ; i<count ; i++)
	{
		d = 6 + (frand () * 12);
		rnum = (rand () % 5);

		makePart (
			org[0] + (d*dir[0]) + (crand()*4),	org[1] + (d*dir[1]) + (crand()*4),	org[2] + (d*dir[2]) + (crand()*4),
			0,									0,									0,
			(dir[0] * 25) + (crand () * 35),	(dir[1] * 25) + (crand () * 35),	(dir[2] * 25) + (crand () * 35),
			0,									0,									-(frand () * 10),
			palRed (0xe0 + rnum),				palGreen (0xe0 + rnum),				palBlue (0xe0 + rnum),
			palRed (0xe0 + rnum),				palGreen (0xe0 + rnum),				palBlue (0xe0 + rnum),
			0.9 + (frand () * 0.1),				-1 / (1 + frand () * 0.3),
			11 + (frand () * -10.75),			0.1 + (frand () * 0.5),
			PT_RED,								PF_SCALED|PF_GRAVITY|PF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			pBounceThink,						qTrue,
			0);
	}
}


/*
===============
CL_BlasterGreenParticles
===============
*/
void CL_BlasterGreenParticles (vec3_t org, vec3_t dir)
{
	int			i, count, rnum;
	float		d, randwhite;

	for (i=0 ; i<2 ; i++)
	{
		/* glow marks */
		makeDecal (
			org[0],								org[1],								org[2],
			dir[0],								dir[1],								dir[2],
			(rand()%41+135),					180 + (rand()%76),					(rand()%41+135),
			(rand()%41+135),					180 + (rand()%76),					(rand()%41+135),
			1,									0,
			5 + (frand() * 0.5),				0,
			PT_GREEN,							DF_NOTIMESCALE|DF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			0,									qFalse,
			DECAL_GLOWTIME,						frand () * 360);
	}

	/* burn mark */
	rnum = (rand()%5);
	makeDecal (
		org[0],								org[1],								org[2],
		dir[0],								dir[1],								dir[2],
		(255-palRed(0xd0+rnum))*0.5+128,	(255-palGreen(0xd0+rnum))*0.5+128,	(255-palBlue(0xd0+rnum))*0.5+128,
		0,									0,									0,
		0.5 + (crand() * 0.1),				0,
		4 + (frand() * 0.5),				0,
		dRandBurnMark (),					DF_ALPHACOLOR,
		GL_ZERO,							GL_ONE_MINUS_SRC_COLOR,
		0,									qFalse,
		0,									frand () * 360);

	/* smoke */
	count = pIncDegree (org, 6 + (cl_smokelinger->value * 0.25), 0.5, qTrue);
	for (i=0 ; i<count ; i++)
	{
		d = 3 + (frand () * 6);
		randwhite = (rand()%2)?150 + (rand()%26) : 0;

		makePart (
			org[0] + (d*dir[0]) + (crand()*2),	org[1] + (d*dir[1]) + (crand()*2),	org[2] + (d*dir[2]) + (crand()*2),
			0,									0,									0,
			0,									0,									0,
			0,									0,									5 + (frand () * 25),
			randwhite,							65 + (rand()%150) + randwhite,		(rand()%50) + randwhite,
			randwhite,							65 + (rand()%150) + randwhite,		(rand()%50) + randwhite,
			0.9 + (frand() * 0.1),				-1 / (0.8 + (cl_smokelinger->value * 0.01) + (frand() * 0.1)),
			2 + crand (),						16 + (crand () * 8),
			pRandSmoke (),						PF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			pSmokeTrailThink,					qTrue,
			frand () * 360);
	}

	/* dots */
	count = pIncDegree (org, 60, 0.5, qTrue);
	for (i=0 ; i<count ; i++)
	{
		d = 6 + (frand () * 12);
		randwhite = (rand()%2)?150 + (rand()%26) : 0;

		makePart (
			org[0] + (d*dir[0]) + (crand()*4),	org[1] + (d*dir[1]) + (crand()*4),	org[2] + (d*dir[2]) + (crand()*4),
			0,									0,									0,
			(dir[0] * 25) + (crand () * 35),	(dir[1] * 25) + (crand () * 35),	(dir[2] * 25) + (crand () * 35),
			0,									0,									-(frand () * 10),
			randwhite,							65 + (rand()%150) + randwhite,		(rand()%50) + randwhite,
			randwhite,							65 + (rand()%150) + randwhite,		(rand()%50) + randwhite,
			0.9 + (frand() * 0.1),				-0.4 / (0.3 + (frand () * 0.3)),
			10 + (frand() * -9.75),				0.1 + (frand() * 0.5),
			PT_GREEN,							PF_SCALED|PF_GRAVITY|PF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			pBounceThink,						qTrue,
			0);
	}
}


/*
===============
CL_BlasterGreyParticles
===============
*/
void CL_BlasterGreyParticles (vec3_t org, vec3_t dir)
{
	int		i, count;
	float	d;

	/* smoke */
	count = pIncDegree (org, 6 + (cl_smokelinger->value * 0.25), 0.5, qTrue);
	for (i=0 ; i<count ; i++)
	{
		d = rand()%13 + 3;

		makePart (
			org[0] + d*dir[0],					org[1] + d*dir[1],					org[2] + d*dir[2],
			0,									0,									0,
			0,									0,									0,
			0,									0,									10 + (frand () * 20),
			130 + (rand()%6),					162 + (rand()%6),					178 + (rand()%6),
			130 + (rand()%6),					162 + (rand()%6),					178 + (rand()%6),
			0.9 + (frand() * 0.1),				-1 / (0.8 + (cl_smokelinger->value * 0.01) + (frand() * 0.1)),
			2 + crand (),						15 + (crand () * 8),
			pRandSmoke (),						PF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			pSmokeTrailThink,					qTrue,
			frand () * 360);
	}

	/* dots */
	count = pIncDegree (org, 50, 0.4, qTrue);
	for (i=0 ; i<count ; i++)
	{
		d = 1.5 + (rand()%13 + 3);

		makePart (
			org[0] + crand () * 4 + d*dir[0],	org[1] + crand () * 4 + d*dir[1],	org[2] + crand () * 4 + d*dir[2],
			0,									0,									0,
			dir[0] * 25 + crand () * 35,		dir[1] * 25 + crand () * 35,		dir[2] * 25 + crand () * 35,
			0,									0,									-10 + (frand () * 10),
			130 + (rand()%6),					162 + (rand()%6),					178 + (rand()%6),
			130 + (rand()%6),					162 + (rand()%6),					178 + (rand()%6),
			0.9 + (frand() * 0.1),				-1 / (0.8 + (frand () * 0.3)),
			10 + (frand() * -9.75),				0.1 + (frand() * 0.5),
			PT_FLARE,							PF_SCALED|PF_GRAVITY,
			GL_SRC_ALPHA,						GL_ONE_MINUS_SRC_ALPHA,
			pBounceThink,						qTrue,
			0);
	}
}


/*
===============
CL_BleedGreenEffect
===============
*/
void CL_BleedGreenEffect (vec3_t org, vec3_t dir, int count)
{
	int			i, j, flags;
	float		d, gore, amnt, fly;
	vec3_t		orgvec, dirvec;

	gore = clamp ((cl_gore->value + 1), 1, 11);
	amnt = pIncDegree (org, (count + gore) * 0.5, 0.25, qTrue);
	fly = ((gore - 1) * 0.1) * 30;

	for (i=0 ; i<amnt ; i++)
	{
		/* drip drip drip */
		d = 1 + (frand () * 6);

		VectorCopy (org, orgvec);
		VectorCopy (dir, dirvec);
		VectorScale (dirvec, d, dirvec);
		for (j=0 ; j<3 ; j++)
		{
			orgvec[j] += crand () * 3;
			dirvec[j] += crand () * (100 + fly);
		}

		flags = PF_SCALED|PF_GRAVITY|PF_DIRECTION;
		if (rand () % (int)(clamp (amnt + 1, gore + 1, amnt + 1) - gore))
			flags |= PF_NODECAL;

		makePart (
			orgvec[0] + (dir[0] * d),			orgvec[1] + (dir[1] * d),			orgvec[2] + (dir[2] * d),
			0,									0,									0,
			dirvec[0],							dirvec[1],							dirvec[2],
			0,									0,									-220,
			20,									50 + (rand()%91),					20,
			10,									50 + (rand()%91),					10,
			1.0,								-0.5 / (0.4 + (frand () * 0.3)),
			1.25 + (frand () * 0.2),			1.35 + (frand () * 0.2),
			PT_BLDDRIP,							flags,
			GL_SRC_ALPHA,						GL_ONE,
			pBloodDripThink,					qTrue,
			PMAXBLDRIPLEN);
	}
}


/*
===============
CL_BleedEffect
===============
*/
void CL_BleedEffect (vec3_t org, vec3_t dir, int count)
{
	int			i, j, flags;
	float		d, gore, amnt, fly;
	vec3_t		orgvec, dirvec;

	gore = clamp ((cl_gore->value + 1), 1, 11);
	amnt = pIncDegree (org, (count + gore) * 0.5, 0.25, qTrue);
	fly = ((gore - 1) * 0.1) * 30;

	for (i=0 ; i<amnt ; i++)
	{
		/* drip drip drip */
		d = 1 + (frand () * 6);

		VectorCopy (org, orgvec);
		VectorCopy (dir, dirvec);
		VectorScale (dirvec, d, dirvec);
		for (j=0 ; j<3 ; j++)
		{
			orgvec[j] += crand () * 3;
			dirvec[j] += crand () * (100 + fly);
		}

		flags = PF_ALPHACOLOR|PF_SCALED|PF_GRAVITY|PF_DIRECTION;
		if (rand () % (int)(clamp (amnt + 1, gore + 1, amnt + 1) - gore))
			flags |= PF_NODECAL;

		makePart (
			orgvec[0] + (dir[0] * d),			orgvec[1] + (dir[1] * d),			orgvec[2] + (dir[2] * d),
			0,									0,									0,
			dirvec[0],							dirvec[1],							dirvec[2],
			0,									0,									-220,
			230 + (frand () * 5),				245 + (frand () * 10),				245 + (frand () * 10),
			0,									0,									0,
			1.0,								-0.5 / (0.4 + (frand () * 0.3)),
			1.25 + (frand () * 0.2),			1.35 + (frand () * 0.2),
			PT_BLDDRIP,							flags,
			GL_ZERO,							GL_ONE_MINUS_SRC_COLOR,
			pBloodDripThink,					qTrue,
			PMAXBLDRIPLEN);
	}
}


/*
===============
CL_BubbleEffect
===============
*/
void CL_BubbleEffect (vec3_t origin)
{
	double	rnum, rnum2;

	/* why bother spawn a particle that's just going to die anyways? */
	if (!(CM_PointContents (origin, 0) & MASK_WATER))
		return;

	rnum = 230 + (frand () * 25);
	rnum2 = 230 + (frand () * 25);
	makePart (
		origin[0] + crand (),			origin[1] + crand (),			origin[2] + crand (),
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


/*
===============
CL_ExplosionBFGEffect
===============
*/
void CL_ExplosionBFGEffect (vec3_t org)
{
	int			i;
	float		randwhite;

	/* normally drawn at the feet of opponents when a bfg explodes */
	for (i=0 ; i<pIncDegree (org, 256, 0.5, qTrue) ; i++)
	{
		randwhite = (rand()%2)?150 + (rand()%26):0;
		makePart (
			org[0] + (crand () * 20),		org[1] + (crand () * 20),			org[2] + (crand () * 20),
			0,								0,									0,
			crand () * 50,					crand () * 50,						crand () * 50,
			0,								0,									-40,
			randwhite,						75 + (rand()%150) + randwhite,		(rand()%50) + randwhite,
			randwhite,						75 + (rand()%150) + randwhite,		(rand()%50) + randwhite,
			1.0,							-0.8 / (0.8 + (frand () * 0.3)),
			11 + (crand () * 10.5),			0.6 + (crand () * 0.5),
			PT_GREEN,						PF_SCALED|PF_GRAVITY|PF_ALPHACOLOR,
			GL_SRC_ALPHA,					GL_ONE,
			pBounceThink,					qTrue,
			0);
	}
}


/*
===============
CL_FlareEffect
===============
*/
void FASTCALL CL_FlareEffect (vec3_t origin, int type, double orient, double size, double sizevel, int color, int colorvel, double alpha, double alphavel)
{
	makePart (
		origin[0],						origin[1],						origin[2],
		0,								0,								0,
		0,								0,								0,
		0,								0,								0,
		palRed (color),					palGreen (color),				palBlue (color),
		palRed (colorvel),				palGreen (colorvel),			palBlue (colorvel),
		alpha,							alphavel,
		size,							sizevel,
		type,							PF_SCALED|PF_ALPHACOLOR,
		GL_SRC_ALPHA,					GL_ONE,
		pFlareThink,					qTrue,
		orient);
}


/*
===============
CL_GenericParticleEffect
===============
*/
void FASTCALL CL_GenericParticleEffect (vec3_t org, vec3_t dir, int color, int count, int numcolors, int dirspread, double alphavel)
{
	int			i, rnum;
	float		d;

	for (i=0 ; i<pIncDegree (org, count, 0.5, qTrue) ; i++)
	{
		d = (rand() & dirspread);
		rnum = (numcolors > 1) ? (rand()&numcolors) : 0;

		makePart (
			org[0] + (crand () * 5) + d*dir[0],	org[1] + (crand () * 5) + d*dir[1],	org[2] + (crand () * 5) + d*dir[2],
			0,									0,									0,
			crand () * 20,						crand () * 20,						crand () * 20,
			0,									0,									-40,
			palRed (color + rnum),				palGreen (color + rnum),			palBlue (color + rnum),
			palRed (color + rnum),				palGreen (color + rnum),			palBlue (color + rnum),
			1,									-1.0 / (0.5 + (frand () * alphavel)),
			1,									1,
			PT_WHITE,							PF_SCALED,
			GL_SRC_ALPHA,						GL_ONE_MINUS_SRC_ALPHA,
			0,									qFalse,
			0);
	}
}


/*
===============
CL_ItemRespawnEffect
===============
*/
void CL_ItemRespawnEffect (vec3_t org)
{
	int			i;

	for (i=0 ; i<pIncDegree (org, 64, 0.5, qTrue) ; i++)
	{
		makePart (
			org[0] + (crand () * 9),		org[1] + (crand () * 9),		org[2] + (crand () * 9),
			0,								0,								0,
			crand () * 10,					crand () * 10,					crand () * 10,
			crand () * 10,					crand () * 10,					20 + (crand () * 10),
			135 + (frand () * 40),			180 + (frand () * 75),			135 + (frand () * 40),
			135 + (frand () * 40),			180 + (frand () * 75),			135 + (frand () * 40),
			1.0,							-1.0 / (1.0 + (frand () * 0.3)),
			4,								2,
			PT_GREEN,						PF_SCALED|PF_ALPHACOLOR,
			GL_SRC_ALPHA,					GL_ONE,
			0,								qFalse,
			0);
	}
}


/*
===============
CL_LogoutEffect
===============
*/
void CL_LogoutEffect (vec3_t org, int type)
{
	int			i, rnum, rnum2;
	float		count = 300;

	switch (type)
	{
		/* green */
		case MZ_LOGIN:
			for (i=0 ; i<pIncDegree (org, count, 0.5, qTrue) ; i++)
			{
				rnum = (rand() % 5);
				rnum2 = (rand() % 5);
				makePart (
					org[0] - 16 + (frand () * 32),		org[1] - 16 + (frand () * 32),		org[2] - 24 + (frand () * 56),
					0,									0,									0,
					crand () * 20,						crand () * 20,						crand () * 20,
					0,									0,									-40,
					palRed (0xd0 + rnum),				palGreen (0xd0 + rnum),				palBlue (0xd0 + rnum),
					palRed (0xd0 + rnum2),				palGreen (0xd0 + rnum2),			palBlue (0xd0 + rnum2),
					1.0,								-1.0 / (1.0 + (frand () * 0.3)),
					3,									1,
					PT_WHITE,							PF_SCALED|PF_ALPHACOLOR,
					GL_SRC_ALPHA,						GL_ONE,
					0,									qFalse,
					0);
			}
			break;

		/* red */
		case MZ_LOGOUT:
			for (i=0 ; i<pIncDegree (org, count, 0.5, qTrue) ; i++)
			{
				rnum = (rand() % 5);
				rnum2 = (rand() % 5);
				makePart (
					org[0] - 16 + (frand () * 32),		org[1] - 16 + (frand () * 32),		org[2] - 24 + (frand () * 56),
					0,									0,									0,
					crand () * 20,						crand () * 20,						crand () * 20,
					0,									0,									-40,
					palRed (0x40 + rnum),				palGreen (0x40 + rnum),				palBlue (0x40 + rnum),
					palRed (0x40 + rnum2),				palGreen (0x40 + rnum2),			palBlue (0x40 + rnum2),
					1.0,								-1.0 / (1.0 + (frand () * 0.3)),
					3,									1,
					PT_WHITE,							PF_SCALED|PF_ALPHACOLOR,
					GL_SRC_ALPHA,						GL_ONE,
					0,									qFalse,
					0);
			}
			break;

		/* golden */
		default:
			for (i=0 ; i<pIncDegree (org, count, 0.5, qTrue) ; i++)
			{
				rnum = (rand() % 5);
				rnum2 = (rand() % 5);
				makePart (
					org[0] - 16 + (frand () * 32),		org[1] - 16 + (frand () * 32),		org[2] - 24 + (frand () * 56),
					0,									0,									0,
					crand () * 20,						crand () * 20,						crand () * 20,
					0,									0,									-40,
					palRed (0xe0 + rnum),				palGreen (0xe0 + rnum),				palBlue (0xe0 + rnum),
					palRed (0xe0 + rnum2),				palGreen (0xe0 + rnum2),			palBlue (0xe0 + rnum2),
					1.0,								-1.0 / (1.0 + (frand () * 0.3)),
					3,									1,
					PT_WHITE,							PF_SCALED|PF_ALPHACOLOR,
					GL_SRC_ALPHA,						GL_ONE,
					0,									qFalse,
					0);
			}
			break;
	}
}


/*
===============
CL_ParticleEffect

Wall impact puffs
===============
*/
void FASTCALL CL_ParticleEffect (vec3_t org, vec3_t dir, int color, int count)
{
	int			i, rnum, rnum2;
	float		d;

	if (FS_CurrGame ("gloom"))
	{
		switch (color)
		{
			/* drone spit */
			case 0xd0:
				CL_GloomDroneEffect (org, dir);
				return;

			/* slash */
			case 0xe0:
				d = 5 + ((rand()%31*0.1) - 1);
				makeDecal (
					org[0],								org[1],								org[2],
					dir[0],								dir[1],								dir[2],
					255,								255,								255,
					0,									0,									0,
					0.7 + (crand () * 0.1),				0,
					d,									0,
					dRandSlashMark (),					DF_ALPHACOLOR,
					GL_ZERO,							GL_ONE_MINUS_SRC_COLOR,
					0,									qFalse,
					0,									frand () * 360);

				for (i=0 ; i<pIncDegree (org, count, 0.5, qTrue) ; i++)
				{
					d = rand()%17;
					rnum = (rand()%5);
					rnum2 = (rand()%5);

					makePart (
						org[0] + (crand () * 4) + d*dir[0],	org[1] + (crand () * 4) + d*dir[1],	org[2] + (crand () * 4) + d*dir[2],
						0,									0,									0,
						dir[0] * crand () * 3,				dir[1] * crand () * 3,				dir[2] * crand () * 3,
						(dir[0] * (crand() * 8)) + (crand () * 4),
						(dir[1] * (crand() * 8)) + (crand () * 4),
						(dir[2] * (crand() * 8)) + (crand () * 4) - (frand() * 60),
						palRed (color + rnum),				palGreen (color + rnum),			palBlue (color + rnum),
						palRed (color + rnum2),				palGreen (color + rnum2),			palBlue (color + rnum2),
						1.0,								-1.0 / (0.5 + (frand () * 0.3)),
						0.5f,								0.6f,
						PT_WHITE,							PF_SCALED|PF_ALPHACOLOR,
						GL_SRC_ALPHA,						GL_ONE,
						0,									qFalse,
						0);
				}

				CL_SparkEffect (org, dir, color, color, count, 1);
				return;

			/* exterm */
			case 0x75:
				CL_BlasterBlueParticles (org, dir);
				return;
		}
	}

	switch (color)
	{
		/* blood */
		case 0xe8:
			CL_BleedEffect (org, dir, count);
			return;

		/* bullet mark */
		case 0x0:
			CL_RicochetEffect (org, dir, count);
			return;

		/* default */
		default:
			for (i=0 ; i<pIncDegree (org, count, 0.5, qTrue) ; i++)
			{
				d = rand()%31;
				rnum = (rand()%5);
				rnum2 = (rand()%5);

				makePart (
					org[0] + ((rand()%7)-4) + d*dir[0],	org[1] + ((rand()%7)-4) + d*dir[1],	org[2] + ((rand()%7)-4) + d*dir[2],
					0,									0,									0,
					crand () * 20,						crand () * 20,						crand () * 20,
					0,									0,									-40,
					palRed (color + rnum),				palGreen (color + rnum),			palBlue (color + rnum),
					palRed (color + rnum2),				palGreen (color + rnum2),			palBlue (color + rnum2),
					1,									-1.0 / (0.5 + (frand () * 0.3)),
					1,									1,
					PT_WHITE,							PF_SCALED,
					GL_SRC_ALPHA,						GL_ONE_MINUS_SRC_ALPHA,
					0,									qFalse,
					0);
			}
			return;
	}
}


/*
===============
CL_ParticleEffect2
===============
*/
void FASTCALL CL_ParticleEffect2 (vec3_t org, vec3_t dir, int color, int count)
{
	if ((color == 0xe2) && FS_CurrGame ("gloom"))
	{
		CL_GloomRepairEffect (org, dir, count);
	}
	else
	{
		int			i, rnum, rnum2;
		float		d;

		for (i=0 ; i<pIncDegree (org, count, 0.5, qTrue) ; i++)
		{
			d = frand () * 7;
			rnum = (rand () % 5);
			rnum2 = (rand () % 5);

			makePart (
				org[0] + (crand () * 4) + d*dir[0],	org[1] + (crand () * 4) + d*dir[1],	org[2] + (crand () * 4) + d*dir[2],
				0,									0,									0,
				crand () * 20,						crand () * 20,						crand () * 20,
				0,									0,									-40,
				palRed (color + rnum),				palGreen (color + rnum),			palBlue (color + rnum),
				palRed (color + rnum2),				palGreen (color + rnum2),			palBlue (color + rnum2),
				1,									-1.0 / (0.5 + (frand () * 0.3)),
				1,									1,
				PT_WHITE,							PF_SCALED,
				GL_SRC_ALPHA,						GL_ONE_MINUS_SRC_ALPHA,
				0,									qFalse,
				0);
		}
	}
}


/*
===============
CL_ParticleEffect3
===============
*/
void FASTCALL CL_ParticleEffect3 (vec3_t org, vec3_t dir, int color, int count)
{
	int			i, rnum, rnum2;
	float		d;

	for (i=0 ; i<pIncDegree (org, count, 0.5, qTrue) ; i++)
	{
		d = frand () * 7;
		rnum = (rand () % 5);
		rnum2 = (rand () % 5);

		makePart (
			org[0] + (crand () * 4) + d*dir[0],	org[1] + (crand () * 4) + d*dir[1],	org[2] + (crand () * 4) + d*dir[2],
			0,									0,									0,
			crand () * 20,						crand () * 20,						crand () * 20,
			0,									0,									40,
			palRed (color + rnum),				palGreen (color + rnum),			palBlue (color + rnum),
			palRed (color + rnum2),				palGreen (color + rnum2),			palBlue (color + rnum2),
			1.0,								-1.0 / (0.5 + (frand () * 0.3)),
			1,									1,
			PT_WHITE,							PF_SCALED,
			GL_SRC_ALPHA,						GL_ONE_MINUS_SRC_ALPHA,
			0,									qFalse,
			0);
	}
}


/*
===============
CL_ParticleSteamEffect

Puffs with velocity along direction, with some randomness thrown in
===============
*/
void FASTCALL CL_ParticleSteamEffect (vec3_t org, vec3_t dir, int color, int count, int magnitude)
{
	int			i, rnum, rnum2;
	float		d;
	vec3_t		r, u, pvel;

	MakeNormalVectors (dir, r, u);

	for (i=0 ; i<pIncDegree (org, count, 0.5, qTrue) ; i++)
	{
		VectorScale (dir, magnitude, pvel);
		d = crand () * magnitude/3;
		VectorMA (pvel, d, r, pvel);
		d = crand () * magnitude/3;
		VectorMA (pvel, d, u, pvel);

		rnum = (rand () % 5);
		rnum2 = (rand () % 5);
		makePart (
			org[0] + magnitude*0.1*crand(),	org[1] + magnitude*0.1*crand(),	org[2] + magnitude*0.1*crand(),
			0,								0,								0,
			pvel[0],						pvel[1],						pvel[2],
			0,								0,								-40/2,
			palRed (color + rnum),			palGreen (color + rnum),		palBlue (color + rnum),
			palRed (color + rnum2),			palGreen (color + rnum2),		palBlue (color + rnum2),
			0.9 + (crand () * 0.1),			-1.0 / (0.5 + (frand () * 0.3)),
			3 + (frand () * 3),				8 + (frand () * 4),
			pRandSmoke (),					PF_SHADE,
			GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
			0,								qFalse,
			0);
	}
}


/*
===============
CL_ParticleSmokeEffect

like the steam effect, but unaffected by gravity
===============
*/
void FASTCALL CL_ParticleSmokeEffect (vec3_t org, vec3_t dir, int color, int count, int magnitude)
{
	int			i, rnum, rnum2;
	float		d;
	vec3_t		r, u, pvel;

	MakeNormalVectors (dir, r, u);

	for (i=0 ; i<pIncDegree (org, count, 0.5, qTrue) ; i++)
	{
		VectorScale (dir, magnitude, pvel);
		d = crand() * magnitude / 3;
		VectorMA (pvel, d, r, pvel);
		d = crand() * magnitude/ 3;
		VectorMA (pvel, d, u, pvel);

		rnum = (rand () % 5);
		rnum2 = (rand () % 5);
		makePart (
			org[0] + magnitude*0.1*crand(),	org[1] + magnitude*0.1*crand(),	org[2] + magnitude*0.1*crand(),
			0,								0,								0,
			pvel[0],						pvel[1],						pvel[2],
			0,								0,								0,
			palRed (color + rnum),			palGreen (color + rnum),		palBlue (color + rnum),
			palRed (color + rnum2),			palGreen (color + rnum2),		palBlue (color + rnum2),
			0.9 + (crand () * 0.1),			-1.0 / (0.5 + (cl_smokelinger->value * 0.5) + (frand () * 0.3)),
			5 + (frand () * 4),				10 + (frand () * 4),
			pRandSmoke (),					PF_SHADE,
			GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
			pSmokeThink,					qTrue,
			frand () * 360);
	}
}


/*
===============
CL_RicochetEffect
===============
*/
void FASTCALL CL_RicochetEffect (vec3_t org, vec3_t dir, int count)
{
	float	d;
	int		i, rnum, rnum2;

	/* bullet mark */
	d = 4 + crand ();
	makeDecal (
		org[0],								org[1],								org[2],
		dir[0],								dir[1],								dir[2],
		255,								255,								255,
		0,									0,									0,
		1,									0,
		d,									0,
		DT_BULLET,							DF_ALPHACOLOR,
		GL_ZERO,							GL_ONE_MINUS_SRC_COLOR,
		0,									qFalse,
		0,									frand () * 360);

	/* dots */
	for (i=0 ; i<pIncDegree (org, count, 0.25, qTrue) ; i++)
	{
		d = rand()%17;
		rnum = (rand()%3) + 2;
		rnum2 = (rand()%5);

		makePart (
			org[0] + ((rand()%7)-3) + d*dir[0],	org[1] + ((rand()%7)-3) + d*dir[1],	org[2] + ((rand()%7)-3) + d*dir[2],
			0,									0,									0,
			dir[0] * crand () * 3,				dir[1] * crand () * 3,				dir[2] * crand () * 3,
			(dir[0] * (crand() * 8)) + ((rand()%7)-3),
			(dir[1] * (crand() * 8)) + ((rand()%7)-3),
			(dir[2] * (crand() * 8)) + ((rand()%7)-3) - (40 * frand() * 1.5),
			palRed (rnum),						palGreen (rnum),					palBlue (rnum),
			palRed (rnum2),						palGreen (rnum2),					palBlue (rnum2),
			1.0,								-1.0 / (0.5 + (frand() * 0.2)),
			0.5f,								0.6f,
			PT_WHITE,							PF_SCALED,
			GL_SRC_ALPHA,						GL_ONE_MINUS_SRC_ALPHA,
			0,									qFalse,
			0);
	}

	CL_SparkEffect (org, dir, 10, 10, count/2, 1);
}


/*
===============
CL_RocketFireParticles
===============
*/
void CL_RocketFireParticles (vec3_t org, vec3_t dir)
{
	int		i;
	double	rnum, rnum2;

	// kthx need to use dir to tell smoke where to fly to
	for (i=0 ; i<pIncDegree (org, 1, 0.25, qTrue) ; i++)
	{
		rnum = (frand () * 80);
		rnum2 = (frand () * 80);

		makePart (
			org[0] + (crand () * 4),		org[1] + (crand () * 4),		org[2] + (crand () * 4) + 16,
			0,								0,								0,
			crand () * 2,					crand () * 2,					crand () * 2,
			crand () * 2,					crand () * 2,					crand () + (frand () * 4),
			50 + rnum,						50 + rnum,						50 + rnum,
			75 + rnum2,						75 + rnum2,						75 + rnum2,
			0.4,							-1.0 / (1.75 + (crand () * 0.25)),
			65 + (crand () * 10),			450 + (crand () * 10),
			pRandSmoke (),					PF_SHADE,
			GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
			pSmokeThink,					qTrue,
			frand () * 360);
	}
}


/*
===============
CL_SparkEffect
===============
*/
void FASTCALL CL_SparkEffect (vec3_t org, vec3_t dir, int color, int colorvel, int count, float lifescale)
{
	int			i;
	float		d, d2;
	float		rnum, rnum2;

	/* sparks */
	for (i=0 ; i<pIncDegree (org, count, 0.5, qTrue) ; i++)
	{
		d = 135 + (crand () * 40) * lifescale;
		d2 = 1 + crand ();
		rnum = (rand () % 5);
		rnum2 = (rand () % 5);

		makePart (
			org[0] + (dir[0] * d2) + crand (),	org[1] + (dir[1] * d2) + crand (),	org[2] + (dir[2] * d2) + crand (),
			0,									0,									0,
			(dir[0] * d) + (crand () * 24),		(dir[1] * d) + (crand () * 24),		(dir[2] * d) + (crand () * 24),
			0,									0,									0,
			palRed (color + rnum),				palGreen (color + rnum),			palBlue (color + rnum),
			palRed (colorvel + rnum2),			palGreen (colorvel + rnum2),		palBlue (colorvel + rnum2),
			1,									-1 / (0.175 + (frand() * 0.05)),
			0.3,								0.33,
			PT_SPARK,							PF_DIRECTION|PF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			pRicochetSparkThink,				qTrue,
			13 + (crand () * 3));
	}

	/* mist */
	for (i=0 ; i<pIncDegree (org, 2, 0.5, qTrue) ; i++)
	{
		d = 1 + (frand () * 3);
		rnum = (rand () % 5);
		rnum2 = (rand () % 5);

		makePart (
			org[0] + (dir[0] * d),				org[1] + (dir[1] * d),				org[2] + (dir[2] * d),
			0,									0,									0,
			d * dir[0],							d * dir[1],							d * dir[2],
			0,									0,									100,
			palRed (color + rnum),				palGreen (color + rnum),			palBlue (color + rnum),
			palRed (colorvel + rnum2),			palGreen (colorvel + rnum2),		palBlue (colorvel + rnum2),
			0.85 + (crand () * 0.1),			-1.0 / (0.3 + (crand () * 0.1)),
			2 + crand(),						13 + (frand () * 5),
			PT_MIST,							PF_GRAVITY|PF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			0,									qFalse,
			frand () * 360);
	}

	/* smoke */
	for (i=1 ; i<pIncDegree (org, 3, 0.4, qTrue) ; i++)
	{
		rnum = (frand () * 50);
		rnum2 = (frand () * 50);
		makePart (
			org[0] + (i*dir[0]*2.5) + crand (),	org[1] + (i*dir[1]*2.5) + crand (),	org[2] + (i*dir[2]*2.5) + crand (),
			0,									0,									0,
			0,									0,									0,
			0,									0,									i*3.5,
			40 + rnum,							40 + rnum,							40 + rnum,
			50 + rnum2,							50 + rnum2,							50 + rnum2,
			0.8 + (crand () * 0.1),				-1 / (1.5 + (cl_smokelinger->value * 0.05) + (crand() * 0.2)),
			1 + (frand () * 4),					11 + (crand () * 3),
			pRandSmoke (),						PF_SHADE,
			GL_SRC_ALPHA,						GL_ONE_MINUS_SRC_ALPHA,
			pFastSmokeThink,					qTrue,
			frand () * 360);
	}

	/* burst smoke */
	for (i=1 ; i<pIncDegree (org, 6, 0.2, qTrue) ; i++)
	{
		rnum = (frand () * 50);
		rnum2 = (frand () * 50);
		makePart (
			org[0]+(i*dir[0]*2.5)+crand()*2,	org[1]+(i*dir[1]*2.5)+crand()*2,	org[2]+(i*dir[2]*2.5)+crand()*2,
			0,									0,									0,
			0,									0,									0,
			0,									0,									5,
			40 + rnum,							40 + rnum,							40 + rnum,
			50 + rnum2,							50 + rnum2,							50 + rnum2,
			0.7 + (crand () * 0.1),				-1 / (1.25 + (cl_smokelinger->value * 0.05) + (crand() * 0.2)),
			1 + (frand () * 4),					11 + (crand () * 3),
			pRandSmoke (),						PF_SHADE,
			GL_SRC_ALPHA,						GL_ONE_MINUS_SRC_ALPHA,
			pFastSmokeThink,					qTrue,
			frand () * 360);
	}
}


/*
===============
CL_SplashEffect
===============
*/
void FASTCALL CL_SplashParticles (vec3_t org, vec3_t dir, int color, int count, qBool glow)
{
	int		i, rnum, rnum2;
	float	d, d2;
	vec3_t	dirvec;
	int		blendDst;

	blendDst = glow ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA;

	/* mist */
	for (i=0 ; i<pIncDegree (org, 1.5, 0.5, qTrue) ; i++)
	{
		d = 1 + (frand () * 3);
		rnum = (rand () % 5);
		rnum2 = (rand () % 5);

		makePart (
			org[0] + (dir[0] * d),				org[1] + (dir[1] * d),				org[2] + (dir[2] * d),
			0,									0,									0,
			d * dir[0],							d * dir[1],							d * dir[2],
			0,									0,									100,
			palRed (color + rnum),				palGreen (color + rnum),			palBlue (color + rnum),
			palRed (color + rnum2),				palGreen (color + rnum2),			palBlue (color + rnum2),
			0.6 + (crand () * 0.1),				-1.0 / (0.4 + (frand () * 0.1)),
			5 + (crand () * 5),					25 + (crand () * 5),
			PT_MIST,							PF_SCALED|PF_GRAVITY,
			GL_SRC_ALPHA,						blendDst,
			0,									qFalse,
			frand () * 360);
	}

	/* point-away plume */
	for (i=0 ; i<pIncDegree (org, 5, 0.5, qTrue) ; i++)
	{
		d = 50 + (crand () * 10);
		rnum = (rand () % 5);
		rnum2 = (rand () % 5);

		makePart (
			org[0] + (dir[0] * 0.5),			org[1] + (dir[1] * 0.5),			org[2] + (dir[2] * 0.5),
			0,									0,									0,
			(dir[0] + (crand () * 0.5)) * d,	(dir[1] + (crand () * 0.5)) * d,	(dir[2] + (crand () * 0.5)) * d,
			0,									0,									0,
			palRed (color + rnum),				palGreen (color + rnum),			palBlue (color + rnum),
			palRed (color + rnum2),				palGreen (color + rnum2),			palBlue (color + rnum2),
			0.9,								-1 / (0.2 + (frand() * 0.05)),
			3,									4,
			PT_SPLASH,							PF_DIRECTION,
			GL_SRC_ALPHA,						blendDst,
			pSplashPlumeThink,					qTrue,
			2 + (frand () * 2));
	}

	/* randomly fly drips */
	for (i=0 ; i<pIncDegree (org, count, 0.5, qTrue) ; i++)
	{
		d = 7 + (frand () * 20);
		d2 = frand () * 4;
		rnum = (rand () % 5);
		rnum2 = (rand () % 5);

		VectorCopy (dir, dirvec);
		dirvec[0] += crand () * 2;
		dirvec[1] += crand () * 2;
		dirvec[2] += crand () * 2;

		makePart (
			org[0] + (dirvec[0] * d2),			org[1] + (dirvec[1] * d2),			org[2] + (dirvec[2] * d2),
			0,									0,									0,
			d * dirvec[0],						d * dirvec[1],						d * dirvec[2],
			0,									0,									-50,
			palRed (color + rnum),				palGreen (color + rnum),			palBlue (color + rnum),
			palRed (color + rnum2),				palGreen (color + rnum2),			palBlue (color + rnum2),
			0.9 + (crand () * 0.1),				-1.0 / (0.5 + (frand () * 0.3)),
			2,									1,
			PT_DRIP,							PF_DIRECTION|PF_GRAVITY,
			GL_SRC_ALPHA,						blendDst,
			pSplashThink,						qTrue,
			PMAXSPLASHLEN);
	}
}

byte		clrtbl[] = {0x00, 0xe0, 0xb0, 0x50, 0xd0, 0xe0, 0xe8};
void FASTCALL CL_SplashEffect (vec3_t org, vec3_t dir, int color, int count)
{
	/* this function merely decides what route to take */
	switch (color)
	{
		case SPLASH_UNKNOWN:
			CL_RicochetEffect (org, dir, count);
			break;
		case SPLASH_SPARKS:
			CL_SparkEffect (org, dir, 12, 12, count, 4);
			break;
		case SPLASH_BLUE_WATER:
			CL_SplashParticles (org, dir, 0x09, count, qFalse);
			break;
		case SPLASH_BROWN_WATER:
			CL_ParticleEffect (org, dir, clrtbl[color], count);
			break;
		case SPLASH_SLIME: /* also gloom drone spit */
			if (FS_CurrGame ("gloom"))
				CL_GloomDroneEffect (org, dir);
			else
				CL_SplashParticles (org, dir, clrtbl[color], count, qTrue);
			break;
		case SPLASH_LAVA:
			CL_SplashParticles (org, dir, clrtbl[color], count, qTrue);
			break;
		case SPLASH_BLOOD:
			CL_BleedEffect (org, dir, count);
			break;
	}
}
