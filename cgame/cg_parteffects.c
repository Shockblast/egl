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
// cg_parteffects.c
//

#include "cg_local.h"

/*
=============================================================================

	PARTICLE EFFECTS

=============================================================================
*/

/*
===============
CG_BlasterBlueParticles
===============
*/
void CG_BlasterBlueParticles (vec3_t org, vec3_t dir) {
	int			i, count;
	int			rnum, rnum2;
	float		d;

	// glow marks
	for (i=0 ; i<2 ; i++) {
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

	// burn mark
	rnum = (rand () % 5);
	makeDecal (
		org[0],								org[1],								org[2],
		dir[0],								dir[1],								dir[2],
		(255-palRed(0x70+rnum))*0.5+128,	(255-palGreen(0x70+rnum))*0.5+128,	(255-palBlue(0x70+rnum))*0.5+128,
		0,									0,									0,
		0.9 + (crand() * 0.1),				0,
		5 + (frand() * 0.5),				0,
		DT_BURNMARK,						DF_ALPHACOLOR,
		GL_ZERO,							GL_ONE_MINUS_SRC_COLOR,
		0,									qFalse,
		0,									frand () * 360);

	// smoke
	count = pIncDegree (org, 6 + (cl_smokelinger->value * 0.25), 0.5, qTrue);
	for (i=0 ; i<count ; i++) {
		d = 3 + (frand () * 6);
		rnum = (rand () % 5);

		makePart (
			org[0] + (d*dir[0]) + (crand()*2),	org[1] + (d*dir[1]) + (crand()*2),	org[2] + (d*dir[2]) + (crand()*2),
			0,									0,									0,
			0,									0,									0,
			0,									0,									5 + (frand () * 25),
			palRed (0x70 + rnum),				palGreen (0x70 + rnum),				palBlue (0x70 + rnum),
			palRed (0x70 + rnum),				palGreen (0x70 + rnum),				palBlue (0x70 + rnum),
			0.9 + (frand() * 0.1),				-1.0 / (0.8 + (cl_smokelinger->value * 0.01) + (frand() * 0.1)),
			2 + crand (),						16 + (crand () * 8),
			pRandSmoke (),						PF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			pSmokeTrailThink,					qTrue,
			frand () * 360);
	}

	// dots
	count = pIncDegree (org, 60, 0.5, qTrue);
	for (i=0 ; i<count ; i++) {
		d = 6 + (frand () * 12);
		rnum = (rand () % 5);

		makePart (
			org[0] + (d*dir[0]) + (crand()*4),	org[1] + (d*dir[1]) + (crand()*4),	org[2] + (d*dir[2]) + (crand()*4),
			0,									0,									0,
			(dir[0] * 25) + (crand () * 35),	(dir[1] * 25) + (crand () * 35),	(dir[2] * 25) + (crand () * 35),
			0,									0,									-(frand () * 10),
			palRed (0x70 + rnum),				palGreen (0x70 + rnum),				palBlue (0x70 + rnum),
			palRed (0x70 + rnum),				palGreen (0x70 + rnum),				palBlue (0x70 + rnum),
			0.9 + (frand () * 0.1),				-1.0 / (1 + frand () * 0.3),
			11 + (frand () * -10.75),			0.1 + (frand () * 0.5),
			PT_BLUE,							PF_SCALED|PF_GRAVITY|PF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			pBounceThink,						qTrue,
			0);
	}
}


/*
===============
CG_BlasterGoldParticles
===============
*/
void CG_BlasterGoldParticles (vec3_t org, vec3_t dir) {
	int			i, count;
	int			rnum, rnum2;
	float		d;

	// glow marks
	for (i=0 ; i<2 ; i++) {
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

	// burn mark
	rnum = (rand () % 5);
	makeDecal (
		org[0],								org[1],								org[2],
		dir[0],								dir[1],								dir[2],
		(255-palRed(0xe0+rnum))*0.5+128,	(255-palGreen(0xe0+rnum))*0.5+128,	(255-palBlue(0xe0+rnum))*0.5+128,
		0,									0,									0,
		0.9 + (crand() * 0.1),				0,
		5 + (frand() * 0.5),				0,
		DT_BURNMARK,						DF_ALPHACOLOR,
		GL_ZERO,							GL_ONE_MINUS_SRC_COLOR,
		0,									qFalse,
		0,									frand () * 360);

	// smoke
	count = pIncDegree (org, 6 + (cl_smokelinger->value * 0.25), 0.5, qTrue);
	for (i=0 ; i<count ; i++) {
		d = 3 + (frand () * 6);
		rnum = (rand () % 5);

		makePart (
			org[0] + (d*dir[0]) + (crand()*2),	org[1] + (d*dir[1]) + (crand()*2),	org[2] + (d*dir[2]) + (crand()*2),
			0,									0,									0,
			0,									0,									0,
			0,									0,									5 + (frand () * 25),
			palRed (0xe0 + rnum),				palGreen (0xe0 + rnum),				palBlue (0xe0 + rnum),
			palRed (0xe0 + rnum2),				palGreen (0xe0 + rnum2),			palBlue (0xe0 + rnum2),
			0.9 + (frand() * 0.1),				-1.0 / (0.8 + (cl_smokelinger->value * 0.01) + (frand() * 0.1)),
			2 + crand (),						16 + (crand () * 8),
			pRandSmoke (),						PF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			pSmokeTrailThink,					qTrue,
			frand () * 360);
	}

	// dots
	count = pIncDegree (org, 60, 0.5, qTrue);
	for (i=0 ; i<count ; i++) {
		d = 6 + (frand () * 12);
		rnum = (rand () % 5);

		makePart (
			org[0] + (d*dir[0]) + (crand()*4),	org[1] + (d*dir[1]) + (crand()*4),	org[2] + (d*dir[2]) + (crand()*4),
			0,									0,									0,
			(dir[0] * 25) + (crand () * 35),	(dir[1] * 25) + (crand () * 35),	(dir[2] * 25) + (crand () * 35),
			0,									0,									-(frand () * 10),
			palRed (0xe0 + rnum),				palGreen (0xe0 + rnum),				palBlue (0xe0 + rnum),
			palRed (0xe0 + rnum),				palGreen (0xe0 + rnum),				palBlue (0xe0 + rnum),
			0.9 + (frand () * 0.1),				-1.0 / (1 + frand () * 0.3),
			11 + (frand () * -10.75),			0.1 + (frand () * 0.5),
			PT_RED,								PF_SCALED|PF_GRAVITY|PF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			pBounceThink,						qTrue,
			0);
	}
}


/*
===============
CG_BlasterGreenParticles
===============
*/
void CG_BlasterGreenParticles (vec3_t org, vec3_t dir) {
	int			i, count, rnum;
	float		d, randwhite;

	for (i=0 ; i<2 ; i++) {
		// glow marks
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

	// burn mark
	rnum = (rand()%5);
	makeDecal (
		org[0],								org[1],								org[2],
		dir[0],								dir[1],								dir[2],
		(255-palRed(0xd0+rnum))*0.5+128,	(255-palGreen(0xd0+rnum))*0.5+128,	(255-palBlue(0xd0+rnum))*0.5+128,
		0,									0,									0,
		0.9 + (crand() * 0.1),				0,
		4 + (frand() * 0.5),				0,
		DT_BURNMARK,						DF_ALPHACOLOR,
		GL_ZERO,							GL_ONE_MINUS_SRC_COLOR,
		0,									qFalse,
		0,									frand () * 360);

	// smoke
	count = pIncDegree (org, 6 + (cl_smokelinger->value * 0.25), 0.5, qTrue);
	for (i=0 ; i<count ; i++) {
		d = 3 + (frand () * 6);
		randwhite = (rand()&1)?150 + (rand()%26) : 0;

		makePart (
			org[0] + (d*dir[0]) + (crand()*2),	org[1] + (d*dir[1]) + (crand()*2),	org[2] + (d*dir[2]) + (crand()*2),
			0,									0,									0,
			0,									0,									0,
			0,									0,									5 + (frand () * 25),
			randwhite,							65 + (rand()%150) + randwhite,		(rand()%50) + randwhite,
			randwhite,							65 + (rand()%150) + randwhite,		(rand()%50) + randwhite,
			0.9 + (frand() * 0.1),				-1.0 / (0.8 + (cl_smokelinger->value * 0.01) + (frand() * 0.1)),
			2 + crand (),						16 + (crand () * 8),
			pRandSmoke (),						PF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			pSmokeTrailThink,					qTrue,
			frand () * 360);
	}

	// dots
	count = pIncDegree (org, 60, 0.5, qTrue);
	for (i=0 ; i<count ; i++) {
		d = 6 + (frand () * 12);
		randwhite = (rand()&1)?150 + (rand()%26) : 0;

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
CG_BlasterGreyParticles
===============
*/
void CG_BlasterGreyParticles (vec3_t org, vec3_t dir) {
	int		i, count;
	float	d;

	// smoke
	count = pIncDegree (org, 6 + (cl_smokelinger->value * 0.25), 0.5, qTrue);
	for (i=0 ; i<count ; i++) {
		d = rand()%13 + 3;

		makePart (
			org[0] + d*dir[0],					org[1] + d*dir[1],					org[2] + d*dir[2],
			0,									0,									0,
			0,									0,									0,
			0,									0,									10 + (frand () * 20),
			130 + (rand()%6),					162 + (rand()%6),					178 + (rand()%6),
			130 + (rand()%6),					162 + (rand()%6),					178 + (rand()%6),
			0.9 + (frand() * 0.1),				-1.0 / (0.8 + (cl_smokelinger->value * 0.01) + (frand() * 0.1)),
			2 + crand (),						15 + (crand () * 8),
			pRandSmoke (),						PF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			pSmokeTrailThink,					qTrue,
			frand () * 360);
	}

	// dots
	count = pIncDegree (org, 50, 0.4, qTrue);
	for (i=0 ; i<count ; i++) {
		d = 1.5 + (rand()%13 + 3);

		makePart (
			org[0] + crand () * 4 + d*dir[0],	org[1] + crand () * 4 + d*dir[1],	org[2] + crand () * 4 + d*dir[2],
			0,									0,									0,
			dir[0] * 25 + crand () * 35,		dir[1] * 25 + crand () * 35,		dir[2] * 25 + crand () * 35,
			0,									0,									-10 + (frand () * 10),
			130 + (rand()%6),					162 + (rand()%6),					178 + (rand()%6),
			130 + (rand()%6),					162 + (rand()%6),					178 + (rand()%6),
			0.9 + (frand() * 0.1),				-1.0 / (0.8 + (frand () * 0.3)),
			10 + (frand() * -9.75),				0.1 + (frand() * 0.5),
			PT_FLARE,							PF_SCALED|PF_GRAVITY,
			GL_SRC_ALPHA,						GL_ONE_MINUS_SRC_ALPHA,
			pBounceThink,						qTrue,
			0);
	}
}


/*
===============
CG_BleedEffect
===============
*/
void CG_BleedEffect (vec3_t org, vec3_t dir, int count) {
	int			i, flags;
	float		d, gore, amnt, fly;
	vec3_t		orgvec, dirvec;

	gore = clamp ((cl_gore->value + 1), 1, 11);
	amnt = pIncDegree (org, (count + gore) * 0.5, 0.25, qTrue);
	fly = ((gore - 1) * 0.1) * 30;

	// splurt
	for (i=0 ; i<clamp (cl_gore->integer + 5, 5, 15)/5 ; i++) {
		makePart (
			org[0] + (crand () * 3),			org[1] + (crand () * 3),			org[2] + (crand () * 3),
			0,									0,									0,
			0,									0,									0,
			0,									0,									0,
			230 + (frand () * 5),				245 + (frand () * 10),				245 + (frand () * 10),
			0,									0,									0,
			1.0,								-0.5 / (0.4 + (frand () * 0.3)),
			7 + (crand () * 2),					12 + (crand () * 3),
			PT_BLDSPURT,						PF_SCALED|PF_ALPHACOLOR,
			GL_ZERO,							GL_ONE_MINUS_SRC_COLOR,
			NULL,								qFalse,
			frand () * 360);
	}

	// drip drip drip
	for (i=0 ; i<amnt ; i++) {
		d = 1 + (frand () * 6);

		VectorCopy (org, orgvec);
		VectorCopy (dir, dirvec);
		VectorScale (dirvec, d, dirvec);

		orgvec[0] += crand () * 3;
		orgvec[1] += crand () * 3;
		orgvec[2] += crand () * 3;

		dirvec[0] += crand () * (200 + fly);
		dirvec[1] += crand () * (200 + fly);
		dirvec[2] += crand () * (200 + fly);

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
			1.0,								-0.5 / (0.4 + (frand () * 0.9)),
			0.25 + (frand () * 0.9),			0.35 + (frand () * 0.5),
			PT_BLDDRIP,							flags,
			GL_ZERO,							GL_ONE_MINUS_SRC_COLOR,
			pBloodDripThink,					qTrue,
			PMAXBLDDRIPLEN*0.5 + (frand () * PMAXBLDDRIPLEN));
	}
}


/*
===============
CG_BleedGreenEffect
===============
*/
void CG_BleedGreenEffect (vec3_t org, vec3_t dir, int count) {
	int			i, flags;
	float		d, gore, amnt, fly;
	vec3_t		orgvec, dirvec;

	gore = clamp ((cl_gore->value + 1), 1, 11);
	amnt = pIncDegree (org, (count + gore) * 0.5, 0.25, qTrue);
	fly = ((gore - 1) * 0.1) * 30;

	// drip drip drip
	for (i=0 ; i<amnt ; i++) {
		d = 1 + (frand () * 6);

		VectorCopy (org, orgvec);
		VectorCopy (dir, dirvec);
		VectorScale (dirvec, d, dirvec);

		orgvec[0] += crand () * 3;
		orgvec[1] += crand () * 3;
		orgvec[2] += crand () * 3;

		dirvec[0] += crand () * (100 + fly);
		dirvec[1] += crand () * (100 + fly);
		dirvec[2] += crand () * (100 + fly);

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
			PMAXBLDDRIPLEN);
	}
}


/*
===============
CG_BubbleEffect
===============
*/
void CG_BubbleEffect (vec3_t origin) {
	double	rnum, rnum2;

	// why bother spawn a particle that's just going to die anyways?
	if (!(CGI_CM_PointContents (origin, 0) & MASK_WATER))
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
CG_ExplosionBFGEffect
===============
*/
void CG_ExplosionBFGEffect (vec3_t org) {
	int			i;
	float		randwhite;

	// normally drawn at the feet of opponents when a bfg explodes
	for (i=0 ; i<pIncDegree (org, 256, 0.5, qTrue) ; i++) {
		randwhite = (rand()&1)?150 + (rand()%26):0;
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
CG_FlareEffect
===============
*/
void __fastcall CG_FlareEffect (vec3_t origin, int type, double orient, double size, double sizevel, int color, int colorvel, double alpha, double alphavel) {
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
CG_ItemRespawnEffect
===============
*/
void CG_ItemRespawnEffect (vec3_t org) {
	int			i;

	for (i=0 ; i<pIncDegree (org, 64, 0.5, qTrue) ; i++) {
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
CG_LogoutEffect
===============
*/
void CG_LogoutEffect (vec3_t org, int type) {
	int			i, rnum, rnum2;
	float		count = 300;

	switch (type) {
	case MZ_LOGIN:
		// green
		for (i=0 ; i<pIncDegree (org, count, 0.5, qTrue) ; i++) {
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

	case MZ_LOGOUT:
		// red
		for (i=0 ; i<pIncDegree (org, count, 0.5, qTrue) ; i++) {
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

	default:
		// golden
		for (i=0 ; i<pIncDegree (org, count, 0.5, qTrue) ; i++) {
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
CG_ParticleEffect

Wall impact puffs
===============
*/
void __fastcall CG_ParticleEffect (vec3_t org, vec3_t dir, int color, int count) {
	int			i, rnum, rnum2;
	float		d;

	if (cgi.FS_CurrGame ("gloom")) {
		switch (color) {
		// drone spit
		case 0xd0:
			CG_GloomDroneEffect (org, dir);
			return;

		// slash
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

			for (i=0 ; i<pIncDegree (org, count, 0.5, qTrue) ; i++) {
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

			CG_SparkEffect (org, dir, color, color, count, 1, 1);
			return;

		// exterm
		case 0x75:
			CG_BlasterBlueParticles (org, dir);
			return;
		}
	}

	switch (color) {
	// blood
	case 0xe8:
		CG_BleedEffect (org, dir, count);
		return;

	// bullet mark
	case 0x0:
		CG_RicochetEffect (org, dir, count);
		return;

	// default
	default:
		for (i=0 ; i<pIncDegree (org, count, 0.5, qTrue) ; i++) {
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
CG_ParticleEffect2
===============
*/
void __fastcall CG_ParticleEffect2 (vec3_t org, vec3_t dir, int color, int count) {
	if ((color == 0xe2) && cgi.FS_CurrGame ("gloom"))
		CG_GloomRepairEffect (org, dir, count);
	else {
		int			i, rnum, rnum2;
		float		d;

		for (i=0 ; i<pIncDegree (org, count, 0.5, qTrue) ; i++) {
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
CG_ParticleEffect3
===============
*/
void __fastcall CG_ParticleEffect3 (vec3_t org, vec3_t dir, int color, int count) {
	int			i, rnum, rnum2;
	float		d;

	for (i=0 ; i<pIncDegree (org, count, 0.5, qTrue) ; i++) {
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
CG_ParticleSmokeEffect

like the steam effect, but unaffected by gravity
===============
*/
void __fastcall CG_ParticleSmokeEffect (vec3_t org, vec3_t dir, int color, int count, int magnitude) {
	int			i, rnum, rnum2;
	float		d;
	vec3_t		r, u, pvel;

	MakeNormalVectors (dir, r, u);

	for (i=0 ; i<pIncDegree (org, count, 0.5, qTrue) ; i++) {
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
CG_RicochetEffect
===============
*/
void __fastcall CG_RicochetEffect (vec3_t org, vec3_t dir, int count) {
	int		i, rnum, rnum2;
	float	d;

	// bullet mark
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

	// dots
	for (i=0 ; i<pIncDegree (org, count, 0.25, qTrue) ; i++) {
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

	CG_SparkEffect (org, dir, 10, 10, count/2, 1, 1);
}


/*
===============
CG_RocketFireParticles
===============
*/
void CG_RocketFireParticles (vec3_t org, vec3_t dir) {
	int		i;
	double	rnum, rnum2;

	// kthx need to use dir to tell smoke where to fly to
	for (i=0 ; i<pIncDegree (org, 1, 0.25, qTrue) ; i++) {
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
CG_SparkEffect
===============
*/
void __fastcall CG_SparkEffect (vec3_t org, vec3_t dir, int color, int colorvel, int count, float smokeScale, float lifeScale) {
	int			i;
	float		d, d2;
	float		rnum, rnum2;

	// sparks
	for (i=0 ; i<pIncDegree (org, count, 0.5, qTrue) ; i++) {
		d = 135 + (crand () * 40) * lifeScale;
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
			1,									-1.0 / (0.175 + (frand() * 0.05)),
			0.3,								0.33,
			PT_SPARK,							PF_DIRECTION|PF_ALPHACOLOR,
			GL_SRC_ALPHA,						GL_ONE,
			pRicochetSparkThink,				qTrue,
			13 + (crand () * 3));
	}

	// mist
	for (i=0 ; i<pIncDegree (org, 2, 0.5, qTrue) ; i++) {
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

	// smoke
	for (i=1 ; i<pIncDegree (org, 3, 0.4, qTrue) ; i++) {
		rnum = (frand () * 50);
		rnum2 = (frand () * 50);
		makePart (
			org[0] + (i*dir[0]*2.5) + crand (),	org[1] + (i*dir[1]*2.5) + crand (),	org[2] + (i*dir[2]*2.5) + crand (),
			0,									0,									0,
			0,									0,									0,
			0,									0,									i*3.5,
			40 + rnum,							40 + rnum,							40 + rnum,
			50 + rnum2,							50 + rnum2,							50 + rnum2,
			0.8 + (crand () * 0.1),				-1.0 / (1.5 + (cl_smokelinger->value * 0.05) + (crand() * 0.2)),
			(1 + (frand () * 4)) * smokeScale,	(11 + (crand () * 3)) * smokeScale,
			pRandSmoke (),						PF_SHADE,
			GL_SRC_ALPHA,						GL_ONE_MINUS_SRC_ALPHA,
			pFastSmokeThink,					qTrue,
			frand () * 360);
	}

	// burst smoke
	for (i=1 ; i<pIncDegree (org, 6, 0.2, qTrue) ; i++) {
		rnum = (frand () * 50);
		rnum2 = (frand () * 50);
		makePart (
			org[0]+(i*dir[0]*2.5)+crand()*2,	org[1]+(i*dir[1]*2.5)+crand()*2,	org[2]+(i*dir[2]*2.5)+crand()*2,
			0,									0,									0,
			0,									0,									0,
			0,									0,									5,
			40 + rnum,							40 + rnum,							40 + rnum,
			50 + rnum2,							50 + rnum2,							50 + rnum2,
			0.7 + (crand () * 0.1),				-1.0 / (1.25 + (cl_smokelinger->value * 0.05) + (crand() * 0.2)),
			(1 + (frand () * 4)) * smokeScale,	(11 + (crand () * 3)) * smokeScale,
			pRandSmoke (),						PF_SHADE,
			GL_SRC_ALPHA,						GL_ONE_MINUS_SRC_ALPHA,
			pFastSmokeThink,					qTrue,
			frand () * 360);
	}
}


/*
===============
CG_SplashEffect
===============
*/
void __fastcall CG_SplashParticles (vec3_t org, vec3_t dir, int color, int count, qBool glow) {
	int		i, rnum, rnum2;
	float	d, d2;
	vec3_t	dirvec;
	int		blendDst;

	blendDst = glow ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA;

	// mist
	for (i=0 ; i<pIncDegree (org, 1.5, 0.5, qTrue) ; i++) {
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

	// point-away plume
	for (i=0 ; i<pIncDegree (org, 5, 0.5, qTrue) ; i++) {
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
			0.9,								-1.0 / (0.2 + (frand() * 0.05)),
			3,									4,
			PT_SPLASH,							PF_DIRECTION,
			GL_SRC_ALPHA,						blendDst,
			pSplashPlumeThink,					qTrue,
			2 + (frand () * 2));
	}

	// randomly fly drips
	for (i=0 ; i<pIncDegree (org, count, 0.5, qTrue) ; i++) {
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
void __fastcall CG_SplashEffect (vec3_t org, vec3_t dir, int color, int count) {
	// this function merely decides what route to take
	switch (color) {
	case SPLASH_UNKNOWN:
		CG_RicochetEffect (org, dir, count);
		break;
	case SPLASH_SPARKS:
		CG_SparkEffect (org, dir, 12, 12, count, 1, 4);
		break;
	case SPLASH_BLUE_WATER:
		CG_SplashParticles (org, dir, 0x09, count, qFalse);
		break;
	case SPLASH_BROWN_WATER:
		CG_ParticleEffect (org, dir, clrtbl[color], count);
		break;
	case SPLASH_SLIME: // also gloom drone spit
		if (cgi.FS_CurrGame ("gloom"))
			CG_GloomDroneEffect (org, dir);
		else
			CG_SplashParticles (org, dir, clrtbl[color], count, qTrue);
		break;
	case SPLASH_LAVA:
		CG_SplashParticles (org, dir, clrtbl[color], count, qTrue);
		break;
	case SPLASH_BLOOD:
		CG_BleedEffect (org, dir, count);
		break;
	}
}

/*
=============================================================================

	MISCELLANEOUS PARTICLE EFFECTS

=============================================================================
*/

/*
===============
CG_BigTeleportParticles
===============
*/
void CG_BigTeleportParticles (vec3_t org) {
	int			i;
	float		angle, dist;

	for (i=0 ; i<pIncDegree (org, 4096, 0.5, qTrue) ; i++) {
		angle = M_PI_TIMES2 * (rand () & 1023) / 1023.0;
		dist = rand () & 31;

		makePart (
			org[0] + cos (angle) * dist,	org[1] + sin (angle) * dist,	org[2] + 8 + (frand () * 90),
			0,								0,								0,
			cos (angle) * (70+(rand()&63)),	sin (angle) * (70+(rand()&63)),	-100 + (rand () & 31),
			-cos (angle) * 100,				-sin (angle) * 100,				160,
			255,							255,							255,
			230,							230,							230,
			1.0,							-0.3 / (0.2 + (frand () * 0.3)),
			10,								3,
			PT_FLARE,						PF_SCALED,
			GL_SRC_ALPHA,					GL_ONE,
			0,								qFalse,
			0);
	}
}


/*
===============
CG_BlasterTip
===============
*/
void CG_BlasterTip (vec3_t start, vec3_t end) {
	int		rnum, rnum2;
	vec3_t	move, vec;
	float	len, dec;

	// bubbles
	CG_BubbleEffect (start);

	VectorCopy (start, move);
	VectorSubtract (start, end, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 2.5, 0.5, qTrue);
	VectorScale (vec, dec, vec);

	for (; len>0 ; VectorAdd (move, vec, move)) {
		len -= dec;

		rnum = (rand () % 5);
		rnum2 = (rand () % 5);

		makePart (
			move[0] + crand (),					move[1] + crand (),					move[2] + crand (),
			0,									0,									0,
			crand () * 2,						crand () * 2,						crand () * 2,
			crand () * 2,						crand () * 2,						crand () * 2,
			palRed (0xe0 + rnum),				palGreen (0xe0 + rnum),				palBlue (0xe0 + rnum),
			palRed (0xe0 + rnum2),				palGreen (0xe0 + rnum2),			palBlue (0xe0 + rnum2),
			1.0,								-15,
			4 + (frand () * 4),					1.5 + (frand () * 2.5),
			PT_RED,								0,
			GL_SRC_ALPHA,						GL_ONE,
			0,									qFalse,
			0);
	}
}


/*
===============
CG_ExplosionParticles
===============
*/
void CG_ExplosionParticles (vec3_t org, double scale, qBool exploOnly, qBool inWater) {
	int			i;
	double		rnum, rnum2;
	vec3_t		target, top;
	trace_t		tr;
	int			waterOffset;

	CG_ExploRattle (org, scale);

	// sparks
	for (i=0 ; i<pIncDegree (org, 125, 0.5, qTrue) ; i++) {
		makePart (
			org[0] + (crand () * 12 * scale),	org[1] + (crand () * 12 * scale),	org[2] + (crand () * 12 * scale),
			0,									0,									0,
			crand () * (200 * scale),			crand () * (200 * scale),			crand () * (200 * scale),
			0,									0,									0,
			235 + (frand () * 20),				225 + (frand () * 20),				205,
			235 + (frand () * 20),				225 + (frand () * 20),				205,
			0.9,								-1.5 / (0.6 + (crand () * 0.15)),
			0.3,								0.4,
			PT_SPARK,							PF_DIRECTION,
			GL_SRC_ALPHA,						GL_ONE,
			pSparkGrowThink,					qTrue,
			(13 + (crand () * 4)) * scale);
	}

	if (inWater)
		waterOffset = 155;
	else
		waterOffset = 0;

	// explosion anim
	makePart (
		org[0],								org[1],								org[2],
		0,									0,									0,
		0,									0,									0,
		0,									0,									0,
		255 - waterOffset,					255 - waterOffset,					255,
		255 - waterOffset,					255 - waterOffset,					255,
		1.0,								-3 + (crand () * 0.1) + (scale * 0.33),
		(40 + (crand () * 5)) * scale,		(130 + (crand () * 10)) * scale,
		PT_EXPLO1,							0,
		GL_SRC_ALPHA,						GL_ONE,
		pExploAnimThink,					qTrue,
		crand () * 12);

	// explosion embers
	for (i=0 ; i<2 ; i++) {
		makePart (
			org[0],								org[1],								org[2],
			0,									0,									0,
			0,									0,									0,
			0,									0,									0,
			255 - waterOffset,					255 - waterOffset,					255,
			255 - waterOffset,					255 - waterOffset,					255,
			1.0,								-3.25 + (crand () * 0.1) + (scale * 0.33),
			(2 + crand ()) * scale,				(145 + (crand () * 10)) * scale,
			i?PT_EXPLOEMBERS1:PT_EXPLOEMBERS2,	0,
			GL_SRC_ALPHA,						GL_ONE,
			0,									qFalse,
			crand () * 360);
	}

	if (exploOnly)
		return;

	// explosion flash
	makePart (
		org[0],								org[1],								org[2],
		0,									0,									0,
		crand () * 20,						crand () * 20,						crand () * 20,
		0,									0,									0,
		255 - waterOffset,					255 - waterOffset,					255,
		255 - waterOffset,					255 - waterOffset,					255,
		1.0,								-4.5 + (crand () * 0.1) + (scale * 0.33),
		(2 + crand ()) * scale,				(130 + (crand () * 10)) * scale,
		PT_EXPLOFLASH,						0,
		GL_SRC_ALPHA,						GL_ONE,
		0,									qFalse,
		crand () * 360);

	// smoke
	for (i=0 ; i<pIncDegree (org, 1.25 + (cl_smokelinger->value * 0.05), 0.4, qTrue) ; i++) {
		rnum = (frand () * 80);
		rnum2 = (frand () * 80);
		makePart (
			org[0] + (crand () * 4),		org[1] + (crand () * 4),		org[2] + (crand () * 4),
			0,								0,								0,
			crand () * 2,					crand () * 2,					crand () * 2,
			crand () * 2,					crand () * 2,					crand () + (frand () * 4),
			50 + rnum,						50 + rnum,						50 + rnum,
			75 + rnum2,						75 + rnum2,						75 + rnum2,
			0.9 + (crand () * 0.1),			-1.0 / (1.75 + (cl_smokelinger->value * 0.5) + (crand () * 0.2)),
			(60 + (crand () * 5)) * scale,	(110 + (crand () * 10)) * scale,
			pRandSmoke (),					PF_SHADE,
			GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
			pSmokeThink,					qTrue,
			frand () * 361);
	}

	// bubbles
	if (inWater) {
		for (i=0 ; i<pIncDegree (org, (50 * scale), 0.5, qTrue) ; i++) {
			rnum = 230 + (frand () * 25);
			rnum2 = 230 + (frand () * 25);
			makePart (
				org[0] + crand (),				org[1] + crand (),				org[2] + crand (),
				0,								0,								0,
				crand () * (164 * scale),		crand () * (164 * scale),		10 + (crand () * (164 * scale)),
				0,								0,								0,
				rnum,							rnum,							rnum,
				rnum2,							rnum2,							rnum2,
				0.9 + (crand () * 0.1),			-1.0 / (1 + (frand () * 0.2)),
				0.1 + frand (),					0.1 + frand (),
				PT_BUBBLE,						PF_SHADE|PF_LAVAONLY|PF_SLIMEONLY|PF_WATERONLY,
				GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
				0,								qFalse,
				0);
		}
	}

	// explosion mark
	for (i=0 ; i<6 ; i++) {
		VectorCopy (org, target);
		VectorCopy (org, top);

		switch (i) {
		case 0: target[0] -= 35; top[0] += 35; break;
		case 1:	target[0] += 35; top[0] -= 35; break;
		case 2:	target[1] -= 35; top[1] += 35; break;
		case 3:	target[1] += 35; top[1] -= 35; break;
		case 4:	target[2] -= 35; top[2] += 35; break;
		case 5:	target[2] += 35; top[2] -= 35; break;
		}

		tr = CGI_CM_Trace (top, target, 1, 1);
		if (tr.fraction < 1) {
			// burn mark
			makeDecal (
				tr.endPos[0],						tr.endPos[1],						tr.endPos[2],
				tr.plane.normal[0],					tr.plane.normal[1],					tr.plane.normal[2],
				255,								255,								255,
				0,									0,									0,
				0.5 + (crand () * 0.1),				0,
				(35 + (frand () * 5)) * scale,		0,
				dRandExploMark (),					DF_ALPHACOLOR,
				GL_ZERO,							GL_ONE_MINUS_SRC_COLOR,
				0,									qFalse,
				0,									frand () * 360);
		}
	}
}


/*
===============
CG_ExplosionBFGParticles
===============
*/
void CG_ExplosionBFGParticles (vec3_t org) {
	int			i, j;
	cgDecal_t	*d = NULL;
	vec3_t		target, top;
	trace_t		tr;
	float		mult;
	double		rnum, rnum2;

	mult = (cgi.FS_CurrGame ("gloom")) ? 0.18 : 0.2;

	// dots
	for (i=0 ; i<pIncDegree (org, 256, 0.5, qTrue) ; i++) {
		rnum = (rand () % 2) ? 150 + (frand () * 25) : 0;
		makePart (
			org[0] + (crand () * 15),		org[1] + (crand () * 15),			org[2] + (crand () * 15),
			0,								0,									0,
			(crand () * 192) * mult,		(crand () * 192) * mult,			(crand () * 192) * mult,
			0,								0,									-40,
			rnum,							rnum + 75 + (frand () * 150),		rnum + (frand () * 50),
			rnum,							rnum + 75 + (frand () * 150),		rnum + (frand () * 50),
			1.0,							-0.8 / (0.8 + (frand () * 0.3)),
			11 + (crand () * 10.5),			0.6 + (crand () * 0.5),
			PT_GREEN,						PF_SCALED|PF_GRAVITY,
			GL_SRC_ALPHA,					GL_ONE,
			pBounceThink,					qTrue,
			0);
	}

	// decal
	for (i=0 ; i<6 ; i++) {
		VectorCopy (org, target);
		VectorCopy (org, top);

		if (i&1) {
			if (i == 1)		{ target[0] += 40; top[0] -= 40; }
			else if (i == 3){ target[1] += 40; top[1] -= 40; }
			else			{ target[2] += 40; top[2] -= 40; }
		}
		else {
			if (i == 0)		{ target[0] -= 40; top[0] += 40; }
			else if (i == 2){ target[1] -= 40; top[1] += 40; }
			else			{ target[2] -= 40; top[2] += 40; }
		}

		tr = CGI_CM_Trace (top, target, 1, 1);
		if (tr.fraction < 1) {
			for (j=0 ; j<2 ; j++) {
				rnum = (frand () * 25);
				rnum2 = (frand () * 25);
				d = makeDecal (
					tr.endPos[0],						tr.endPos[1],						tr.endPos[2],
					tr.plane.normal[0],					tr.plane.normal[1],					tr.plane.normal[2],
					200 * j,							215 + (rnum * (1 - j)),				200 * j,
					200 * j,							215 + (rnum2 * (1 - j)),			200 * j,
					1.0f,								0,
					28 + (crand () * 2) - (8 * j),		0,
					PT_FLARE,							0,
					GL_SRC_ALPHA,						GL_ONE,
					0,									qFalse,
					0,									frand () * 360);
			}

			if (d)
				break;
		}
	}
}


/*
===============
CG_ExplosionColorParticles
===============
*/
void CG_ExplosionColorParticles (vec3_t org) {
	int			i;

	for (i=0 ; i<pIncDegree (org, 128, 0.5, qTrue) ; i++) {
		makePart (
			org[0] + (crand () * 16),		org[1] + (crand () * 16),			org[2] + (crand () * 16),
			0,								0,									0,
			crand () * 128,					crand () * 128,						crand () * 128,
			0,								0,									-40,
			0 + crand (),					0 + crand (),						0 + crand (),
			0 + crand (),					0 + crand (),						0 + crand (),
			1.0,							-0.4 / (0.6 + (frand () * 0.2)),
			1.0,							1.0,
			PT_WHITE,						PF_SCALED,
			GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
			0,								qFalse,
			0);
	}
}


/*
===============
CG_FlyEffect
===============
*/
void CG_FlyParticles (vec3_t origin, int count) {
	int			i;
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	vec3_t		forward;
	float		dist = 64;
	float		ltime;

	if (count > NUMVERTEXNORMALS)
		count = NUMVERTEXNORMALS;

	ltime = (float)cgs.realTime / 1000.0;
	for (i=0 ; i<count ; i+=2) {
		angle = ltime * avelocities[i][0];
		sy = sin (angle);
		cy = cos (angle);
		angle = ltime * avelocities[i][1];
		sp = sin (angle);
		cp = cos (angle);
		angle = ltime * avelocities[i][2];
		sr = sin (angle);
		cr = cos (angle);
	
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;

		dist = sin (ltime + i) * 64;

		makePart (
			origin[0] + (byteDirs[i][0] * dist) + (forward[0] * BEAMLENGTH),
			origin[1] + (byteDirs[i][1] * dist) + (forward[1] * BEAMLENGTH),
			origin[2] + (byteDirs[i][2] * dist) + (forward[2] * BEAMLENGTH),
			0,								0,								0,
			0,								0,								0,
			0,								0,								0,
			0,								0,								0,
			0,								0,								0,
			1,								-100,
			1.5f,							1.5f,
			PT_FLY,							PF_SCALED,
			GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
			0,								qFalse,
			(rand () % 2) ? 0 : 10);
	}
}

void CG_FlyEffect (cgEntity_t *ent, vec3_t origin) {
	int		n;
	int		count;
	int		starttime;

	if (ent->flyStopTime < cgs.realTime) {
		starttime = cgs.realTime;
		ent->flyStopTime = cgs.realTime + 60000;
	}
	else
		starttime = ent->flyStopTime - 60000;

	n = cgs.realTime - starttime;
	if (n < 20000)
		count = n * 162 / 20000.0;
	else {
		n = ent->flyStopTime - cgs.realTime;
		if (n < 20000)
			count = n * 162 / 20000.0;
		else
			count = 162;
	}

	CG_FlyParticles (origin, pIncDegree (origin, count*2, 0.5, qTrue));
}


/*
===============
CG_ForceWall
===============
*/
void CG_ForceWall (vec3_t start, vec3_t end, int color) {
	vec3_t		move, vec;
	float		len, dec;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 4, 0.5, qFalse);
	VectorScale (vec, dec, vec);

	for (; len>0 ; VectorAdd (move, vec, move)) {
		len -= dec;
		
		if (frand () > 0.3) {
			makePart (
				move[0] + (crand () * 3),		move[1] + (crand () * 3),		move[2] + (crand () * 3),
				0,								0,								0,
				0,								0,								-40 - (crand () * 10),
				0,								0,								0,
				palRed (color),					palGreen (color),				palBlue (color),
				palRed (color),					palGreen (color),				palBlue (color),
				1.0,							-1.0 / (3.0 + (frand () * 0.5)),
				1.0f,							1.0f,
				PT_WHITE,						PF_SCALED,
				GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
				0,								qFalse,
				0);
		}
	}
}


/*
===============
CG_MonsterPlasma_Shell
===============
*/
void CG_MonsterPlasma_Shell (vec3_t origin) {
	vec3_t			dir, porg;
	int				i, rnum, rnum2;

	for (i=0 ; i<pIncDegree (origin, 40, 0.5, qTrue) ; i++) {
		VectorSet (dir, crand (), crand (), crand ());
		VectorNormalize (dir, dir);
		VectorMA(origin, 10, dir, porg);

		rnum = (rand () % 5);
		rnum2 = (rand () % 5);
		makePart (
			porg[0],						porg[1],						porg[2],
			0,								0,								0,
			0,								0,								0,
			0,								0,								0,
			palRed (0xe0 + rnum),			palGreen (0xe0 + rnum),			palBlue (0xe0 + rnum),
			palRed (0xe0 + rnum2),			palGreen (0xe0 + rnum2),		palBlue (0xe0 + rnum2),
			1.0,							PART_INSTANT,
			1.0,							1.0,
			PT_WHITE,						PF_SCALED,
			GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
			0,								qFalse,
			0);
	}
}


/*
===============
CG_PhalanxTip
===============
*/
void CG_PhalanxTip (vec3_t start, vec3_t end) {
	int		i, j, k;
	int		rnum, rnum2;
	vec3_t	move, vec, dir;
	float	len, dec, vel;

	// bubbles
	CG_BubbleEffect (start);

	VectorCopy (start, move);
	VectorSubtract (start, end, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 2.5, 0.5, qTrue);
	VectorScale (vec, dec, vec);

	for (; len>0 ; VectorAdd (move, vec, move)) {
		len -= dec;

		rnum = (rand () % 5);
		rnum2 = (rand () % 5);

		makePart (
			move[0] + crand (),					move[1] + crand (),					move[2] + crand (),
			0,									0,									0,
			crand () * 2,						crand () * 2,						crand () * 2,
			crand () * 2,						crand () * 2,						crand () * 2,
			palRed (0xe0 + rnum),				palGreen (0xe0 + rnum),				palBlue (0xe0 + rnum),
			palRed (0xe0 + rnum2),				palGreen (0xe0 + rnum2),			palBlue (0xe0 + rnum2),
			1.0,								-15,
			5 + (frand () * 4),					3 + (frand () * 2.5),
			PT_RED,								0,
			GL_SRC_ALPHA,						GL_ONE,
			0,									qFalse,
			0);
	}

	for (i=-2 ; i<=2 ; i+=4) {
		for (j=-2 ; j<=2 ; j+=4) {
			for (k=-2 ; k<=4 ; k+=4) {
				VectorSet (dir, j * 4, i * 4, k * 4);	
				VectorNormalize (dir, dir);
				vel = 10 + rand () % 11;

				rnum = (rand () % 5);
				rnum2 = (rand () % 5);
				makePart (
					start[0] + i + ((rand () % 6) * crand ()),
					start[1] + j + ((rand () % 6) * crand ()),
					start[2] + k + ((rand () % 6) * crand ()),
					0,									0,									0,
					dir[0] * vel,						dir[1] * vel,						dir[2] * vel,
					0,									0,									-40,
					palRed (0xe0 + rnum),				palGreen (0xe0 + rnum),				palBlue (0xe0 + rnum),
					palRed (0xe0 + rnum2),				palGreen (0xe0 + rnum2),			palBlue (0xe0 + rnum2),
					0.9,								-3.5,
					2 + (frand () * 0.5),				0.5 + (frand () * 0.5),
					PT_WHITE,							0,
					GL_SRC_ALPHA,						GL_ONE,
					0,									qFalse,
					0);
			}
		}
	}
}


/*
===============
CG_TeleportParticles
===============
*/
void CG_TeleportParticles (vec3_t org) {
	int		i;

	for (i=0 ; i<pIncDegree(org, 300, 0.6, qTrue) ; i++) {
		makePart (
			org[0] + (crand () * 32),		org[1] + (crand () * 32),		org[2] + (frand () * 85 - 25),
			0,								0,								0,
			crand () * 50,					crand () * 50,					crand () * 50,
			crand () * 50,					crand () * 50,					50 + (crand () * 20),
			220,							190,							150,
			255,							255,							230,
			0.9 + (frand () * 0.25),		-0.3 / (0.1 + (frand () * 0.1)),
			10 + (frand () * 0.25),			0.5 + (frand () * 0.25),
			PT_FLARE,						PF_SCALED|PF_GRAVITY,
			GL_SRC_ALPHA,					GL_ONE,
			pBounceThink,					qTrue,
			0);
	}
}


/*
===============
CG_TeleporterParticles
===============
*/
void CG_TeleporterParticles (entityState_t *ent) {
	int		i;

	for (i=0 ; i<pIncDegree (ent->origin, 2, 0.5, qTrue) ; i++) {
		makePart (
			ent->origin[0] + (crand () * 16),	ent->origin[1] + (crand () * 16),	ent->origin[2] + (crand () * 8) - 3,
			0,									0,									0,
			crand () * 15,						crand () * 15,						80 + (frand () * 5),
			0,									0,									-40,
			210 + (crand () *  5),				180 + (crand () *  5),				120 + (crand () *  5),
			255 + (crand () *  5),				210 + (crand () *  5),				140 + (crand () *  5),
			1.0,								-0.6 + (crand () * 0.1),
			2 + (frand () * 0.5),				1 + (crand () * 0.5),
			PT_WHITE,							PF_SCALED,
			GL_SRC_ALPHA,						GL_ONE,
			0,									qFalse,
			0);
	}
}


/*
===============
CG_TrackerShell
===============
*/
void CG_TrackerShell (vec3_t origin) {
	vec3_t			dir, porg;
	int				i;

	for (i=0 ; i<pIncDegree (origin, 300, 0.5, qTrue) ; i++) {
		VectorSet (dir, crand (), crand (), crand ());
		VectorNormalize (dir, dir);
		VectorMA (origin, 40, dir, porg);

		makePart (
			porg[0],						porg[1],						porg[2],
			0,								0,								0,
			0,								0,								0,
			0,								0,								0,
			0,								0,								0,
			0,								0,								0,
			1.0,							PART_INSTANT,
			1.0f,							1.0f,
			PT_WHITE,						PF_SCALED,
			GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
			0,								qFalse,
			0);
	}
}


/*
===============
CG_TrapParticles
===============
*/
void CG_TrapParticles (entity_t *ent) {
	vec3_t		move, vec, start, end, dir, org;
	int			i, j, k, rnum, rnum2;
	float		len, vel, dec;

	ent->origin[2] -= 16;
	VectorCopy (ent->origin, start);
	VectorCopy (ent->origin, end);
	end[2] += 10;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (org, 5, 0.5, qTrue);
	VectorScale (vec, dec, vec);

	for (; len>0 ; VectorAdd (move, vec, move)) {
		len -= dec;

		rnum = (rand () % 5);
		rnum2 = (rand () % 5);
		makePart (
			move[0] + (crand () * 2),		move[1] + (crand () * 1.5),		move[2] + (crand () * 1.5),
			0,								0,								0,
			crand () * 20,					crand () * 20,					crand () * 20,
			0,								0,								40,
			palRed (0xe0 + rnum),			palGreen (0xe0 + rnum),			palBlue (0xe0 + rnum),
			palRed (0xe0 + rnum2),			palGreen (0xe0 + rnum2),		palBlue (0xe0 + rnum2),
			1.0,							-1.0 / (0.45 + (frand () * 0.2)),
			5.0,							1.0,
			PT_WHITE,						PF_SCALED,
			GL_SRC_ALPHA,					GL_ONE,
			0,								qFalse,
			0);
	}

	ent->origin[2]+=14;
	VectorCopy (ent->origin, org);

	for (i=-2 ; i<=2 ; i+=4)
		for (j=-2 ; j<=2 ; j+=4)
			for (k=-2 ; k<=4 ; k+=4)
			{
				dir[0] = j * 8;
				dir[1] = i * 8;
				dir[2] = k * 8;
	
				VectorNormalize (dir, dir);
				vel = 50 + rand () & 63;

				rnum = (rand () % 5);
				rnum2 = (rand () % 5);
				makePart (
					org[0] + i + ((rand () & 23) * crand ()),
					org[1] + j + ((rand () & 23) * crand ()),
					org[2] + k + ((rand () & 23) * crand ()),
					0,								0,								0,
					dir[0] * vel,					dir[1] * vel,					dir[2] * vel,
					0,								0,								-40,
					palRed (0xe0 + rnum),			palGreen (0xe0 + rnum),			palBlue (0xe0 + rnum),
					palRed (0xe0 + rnum2),			palGreen (0xe0 + rnum2),		palBlue (0xe0 + rnum2),
					1.0,							-1.0 / (0.3 + (frand () * 0.15)),
					2.0,							1.0,
					PT_WHITE,						PF_SCALED,
					GL_SRC_ALPHA,					GL_ONE,
					0,								qFalse,
					0);
			}
}


/*
===============
CG_WidowSplash
===============
*/
void CG_WidowSplash (vec3_t org) {
	int			clrtable[4] = {2*8, 13*8, 21*8, 18*8};
	int			i, rnum, rnum2;
	vec3_t		dir, porg, pvel;

	for (i=0 ; i<pIncDegree (org, 256, 0.5, qTrue) ; i++) {
		VectorSet (dir, crand (), crand (), crand ());
		VectorNormalize (dir, dir);
		VectorMA (org, 45.0, dir, porg);
		VectorMA (vec3Origin, 40.0, dir, pvel);

		rnum = (rand () % 4);
		rnum2 = (rand () % 4);
		makePart (
			porg[0],						porg[1],						porg[2],
			0,								0,								0,
			pvel[0],						pvel[1],						pvel[2],
			0,								0,								0,
			palRed (clrtable[rnum]),		palGreen (clrtable[rnum]),		palBlue (clrtable[rnum]),
			palRed (clrtable[rnum2]),		palGreen (clrtable[rnum2]),		palBlue (clrtable[rnum2]),
			1.0,							-0.8 / (0.5 + (frand () * 0.3)),
			1.0,							1.0,
			PT_WHITE,						PF_SCALED,
			GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
			0,								qFalse,
			0);
	}
}
