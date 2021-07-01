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

// pfx_misc.c

#include "../cl_local.h"

/*
==============================================================

	MISCELLANEOUS PARTICLE EFFECTS

==============================================================
*/

/*
===============
CL_BigTeleportParticles
===============
*/
void CL_BigTeleportParticles (vec3_t org)
{
	int			i;
	float		angle, dist;

	for (i=0 ; i<pIncDegree (org, 4096, 0.5, qTrue) ; i++)
	{
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
CL_BlasterTip
===============
*/
void CL_BlasterTip (vec3_t start, vec3_t end)
{
	int		rnum, rnum2;
	vec3_t	move, vec;
	float	len, dec;

	/* bubbles */
	CL_BubbleEffect (start);

	VectorCopy (start, move);
	VectorSubtract (start, end, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 2.5, 0.5, qTrue);
	VectorScale (vec, dec, vec);

	for (; len>0 ; VectorAdd (move, vec, move))
	{
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
CL_ExplosionParticles
===============
*/
void CL_ExplosionParticles (vec3_t org, double scale, qBool exploonly, qBool inwater)
{
	int			i;
	double		rnum, rnum2;
	vec3_t		target, top;
	trace_t		tr;

	CL_ExplosionShards (org, 10);
	CL_ExploRattle (org, scale);

	/* sparks */
	for (i=0 ; i<pIncDegree (org, 125, 0.5, qTrue) ; i++)
	{
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

	/* explosion anim */
	makePart (
		org[0],								org[1],								org[2],
		0,									0,									0,
		0,									0,									0,
		0,									0,									0,
		inwater ? 100 : 255,				inwater ? 100 : 255,				255,
		inwater ? 100 : 255,				inwater ? 100 : 255,				255,
		1.0,								-3 + (crand () * 0.1) + (scale * 0.33),
		(40 + (crand () * 5)) * scale,		(130 + (crand () * 10)) * scale,
		PT_EXPLO1,							0,
		GL_SRC_ALPHA,						GL_ONE,
		pExploAnimThink,					qTrue,
		crand () * 12);

	/* explosion embers */
	for (i=0 ; i<2 ; i++)
	{
		makePart (
			org[0],								org[1],								org[2],
			0,									0,									0,
			0,									0,									0,
			0,									0,									0,
			inwater ? 100 : 255,				inwater ? 100 : 255,				255,
			inwater ? 100 : 255,				inwater ? 100 : 255,				255,
			1.0,								-3.25 + (crand () * 0.1) + (scale * 0.33),
			(2 + crand ()) * scale,				(145 + (crand () * 10)) * scale,
			i ? PT_EMBERS2 : PT_EMBERS,			0,
			GL_SRC_ALPHA,						GL_ONE,
			0,									qFalse,
			crand () * 360);
	}

	if (exploonly)
		return;

	/* explosion flash */
	makePart (
		org[0],								org[1],								org[2],
		0,									0,									0,
		crand () * 20,						crand () * 20,						crand () * 20,
		0,									0,									0,
		inwater ? 100 : 255,				inwater ? 100 : 255,				255,
		inwater ? 100 : 255,				inwater ? 100 : 255,				255,
		1.0,								-4.5 + (crand () * 0.1) + (scale * 0.33),
		(2 + crand ()) * scale,				(130 + (crand () * 10)) * scale,
		PT_EXPLOFLASH,						0,
		GL_SRC_ALPHA,						GL_ONE,
		0,									qFalse,
		crand () * 360);

	/* smoke */
	for (i=0 ; i<pIncDegree (org, 1.25 + (cl_smokelinger->value * 0.05), 0.4, qTrue) ; i++)
	{
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

	/* bubbles */
	if (inwater)
	{
		for (i=0 ; i<pIncDegree (org, (50 * scale), 0.5, qTrue) ; i++)
		{
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

	/* explosion mark */
	for (i=0 ; i<6 ; i++)
	{
		VectorCopy (org, target);
		VectorCopy (org, top);

		if (i&1)
		{
			if (i == 1)		{ target[0] += 35; top[0] -= 35; }
			else if (i == 3){ target[1] += 35; top[1] -= 35; }
			else			{ target[2] += 35; top[2] -= 35; }
		}
		else
		{
			if (i == 0)		{ target[0] -= 35; top[0] += 35; }
			else if (i == 2){ target[1] -= 35; top[1] += 35; }
			else			{ target[2] -= 35; top[2] += 35; }
		}

		tr = CL_Trace (top, target, 1, 1);
		if (tr.fraction < 1)
		{
			/* burn mark */
			makeDecal (
				tr.endPos[0],						tr.endPos[1],						tr.endPos[2],
				tr.plane.normal[0],					tr.plane.normal[1],					tr.plane.normal[2],
				255,								255,								255,
				0,									0,									0,
				0.5 + (crand () * 0.1),				0,
				(35 + (frand () * 5)) * scale,		0,
				dRandBurnMark (),					DF_ALPHACOLOR,
				GL_ZERO,							GL_ONE_MINUS_SRC_COLOR,
				0,									qFalse,
				0,									frand () * 360);
		}
	}
}


/*
===============
CL_ExplosionBFGParticles
===============
*/
void CL_ExplosionBFGParticles (vec3_t org)
{
	int			i, j;
	cdecal_t	*d = NULL;
	vec3_t		target, top;
	trace_t		tr;
	float		mult;
	double		rnum, rnum2;

	mult = (FS_CurrGame ("gloom")) ? 0.18 : 0.2;

	/* dots */
	for (i=0 ; i<pIncDegree (org, 256, 0.5, qTrue) ; i++)
	{
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

	/* decal */
	for (i=0 ; i<6 ; i++)
	{
		VectorCopy (org, target);
		VectorCopy (org, top);

		if (i&1)
		{
			if (i == 1)		{ target[0] += 40; top[0] -= 40; }
			else if (i == 3){ target[1] += 40; top[1] -= 40; }
			else			{ target[2] += 40; top[2] -= 40; }
		}
		else
		{
			if (i == 0)		{ target[0] -= 40; top[0] += 40; }
			else if (i == 2){ target[1] -= 40; top[1] += 40; }
			else			{ target[2] -= 40; top[2] += 40; }
		}

		tr = CL_Trace (top, target, 1, 1);
		if (tr.fraction < 1)
		{
			for (j=0 ; j<2 ; j++)
			{
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
CL_ExplosionColorParticles
===============
*/
void CL_ExplosionColorParticles (vec3_t org)
{
	int			i;

	for (i=0 ; i<pIncDegree (org, 128, 0.5, qTrue) ; i++)
	{
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
CL_FlyParticles
===============
*/
void CL_FlyParticles (vec3_t origin, int count)
{
	int			i;
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	vec3_t		forward;
	float		dist = 64;
	float		ltime;

	if (count > NUMVERTEXNORMALS)
		count = NUMVERTEXNORMALS;

	if (!avelocities[0][0])
	{
		for (i=0 ; i<(NUMVERTEXNORMALS * 3) ; i++)
			avelocities[0][i] = (frand () * 255) * 0.01;
	}

	ltime = (float)cls.realTime / 1000.0;
	for (i=0 ; i<count ; i+=2)
	{
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

void CL_FlyEffect (centity_t *ent, vec3_t origin)
{
	int		n;
	int		count;
	int		starttime;

	if (ent->flyStopTime < cls.realTime)
	{
		starttime = cls.realTime;
		ent->flyStopTime = cls.realTime + 60000;
	}
	else
	{
		starttime = ent->flyStopTime - 60000;
	}

	n = cls.realTime - starttime;
	if (n < 20000)
		count = n * 162 / 20000.0;
	else
	{
		n = ent->flyStopTime - cls.realTime;
		if (n < 20000)
			count = n * 162 / 20000.0;
		else
			count = 162;
	}

	CL_FlyParticles (origin, pIncDegree (origin, count*2, 0.5, qTrue));
}


/*
===============
CL_ForceWall
===============
*/
void CL_ForceWall (vec3_t start, vec3_t end, int color)
{
	vec3_t		move, vec;
	float		len, dec;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 4, 0.5, qFalse);
	VectorScale (vec, dec, vec);

	for (; len>0 ; VectorAdd (move, vec, move))
	{
		len -= dec;
		
		if (frand () > 0.3)
		{
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
CL_MonsterPlasma_Shell
===============
*/
void CL_MonsterPlasma_Shell (vec3_t origin)
{
	vec3_t			dir, porg;
	int				i, rnum, rnum2;

	for(i=0 ; i<pIncDegree (origin, 40, 0.5, qTrue) ; i++)
	{
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
CL_PhalanxTip
===============
*/
void CL_PhalanxTip (vec3_t start, vec3_t end)
{
	int		i, j, k;
	int		rnum, rnum2;
	vec3_t	move, vec, dir;
	float	len, dec, vel;

	/* bubbles */
	CL_BubbleEffect (start);

	VectorCopy (start, move);
	VectorSubtract (start, end, vec);
	len = VectorNormalize (vec, vec);

	dec = pDecDegree (start, 2.5, 0.5, qTrue);
	VectorScale (vec, dec, vec);

	for (; len>0 ; VectorAdd (move, vec, move))
	{
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

	for (i=-2 ; i<=2 ; i+=4)
	{
		for (j=-2 ; j<=2 ; j+=4)
		{
			for (k=-2 ; k<=4 ; k+=4)
			{
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
CL_TeleportParticles
===============
*/
void CL_TeleportParticles (vec3_t org)
{
	int		i;

	for (i=0 ; i<pIncDegree(org, 300, 0.6, qTrue) ; i++)
	{
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
CL_TeleporterParticles
===============
*/
void CL_TeleporterParticles (entityState_t *ent)
{
	int			i;

	for (i=0 ; i<pIncDegree (ent->origin, 10, 0.5, qTrue) ; i++)
	{
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
CL_TrackerExplode
===============
*/
void CL_TrackerExplode (vec3_t origin)
{
	vec3_t			dir, backdir, porg, pvel;
	int				i;
	for(i=0 ; i<pIncDegree (origin, 300, 0.5, qTrue) ; i++)
	{
		VectorSet (dir, crand (), crand (), crand ());
		VectorNormalize (dir, dir);
		VectorScale (dir, -1, backdir);

		VectorMA (origin, 64, dir, porg);
		VectorScale (backdir, 64, pvel);

		makePart (
			porg[0],						porg[1],						porg[2],
			0,								0,								0,
			pvel[0],						pvel[1],						pvel[2],
			0,								0,								0,
			0,								0,								0,
			0,								0,								0,
			1.0,							-1.0,
			1.0f,							1.0f,
			PT_WHITE,						PF_SCALED,
			GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
			0,								qFalse,
			0);
	}
	
}


/*
===============
CL_TrackerShell
===============
*/
void CL_TrackerShell (vec3_t origin)
{
	vec3_t			dir, porg;
	int				i;

	for(i=0 ; i<pIncDegree (origin, 300, 0.5, qTrue) ; i++)
	{
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
CL_TrapParticles
===============
*/
void CL_TrapParticles (entity_t *ent)
{
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

	for (; len>0 ; VectorAdd (move, vec, move))
	{
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
CL_WidowSplash
===============
*/
void CL_WidowSplash (vec3_t org)
{
	int			clrtable[4] = {2*8, 13*8, 21*8, 18*8};
	int			i, rnum, rnum2;
	vec3_t		dir, porg, pvel;

	for (i=0 ; i<pIncDegree (org, 256, 0.5, qTrue) ; i++)
	{
		VectorSet (dir, crand (), crand (), crand ());
		VectorNormalize (dir, dir);
		VectorMA (org, 45.0, dir, porg);
		VectorMA (vec3Identity, 40.0, dir, pvel);

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
