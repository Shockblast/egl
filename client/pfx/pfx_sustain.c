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

// pfx_sustain.c

#include "../cl_local.h"

/*
==============================================================

	SUSTAINED PARTICLE EFFECTS

==============================================================
*/

csustain_t	cl_sustains[MAX_SUSTAINS];

/*
=================
CL_ClearSustains
=================
*/
void CL_ClearSustains (void)
{
	memset (cl_sustains, 0, sizeof (cl_sustains));
}

/*
=================
CL_ProcessSustains
=================
*/
void CL_ProcessSustains (void)
{
	csustain_t	*s;
	int				i;

	for (i=0, s=cl_sustains ; i<MAX_SUSTAINS ; i++, s++)
	{
		if (s->id)
		{
			if ((s->endtime >= cls.realTime) && (cls.realTime >= s->nextthink))
			{
				s->think (s);
			}
			else if (s->endtime < cls.realTime)
				s->id = 0;
		}
	}
}


/*
===============
CL_Nukeblast
===============
*/
void CL_Nukeblast (csustain_t *self)
{
	vec3_t			dir, porg;
	int				i;
	float			ratio;

	if (FS_CurrGame ("gloom"))
	{
		ratio = 1.0 - (((float)self->endtime - (float)cls.realTime) / 1000.0);

		R_AddLight (self->org, 600 * ratio * (0.9 + (frand () * 0.1)), 1, 1, 1);

		if ((int)(ratio*10) & 1)
			CL_ExplosionParticles (self->org, 3.5 + (crand () * 0.5), qTrue, qFalse);

		for (i=0 ; i<pIncDegree (self->org, 100, 0.5, qTrue) ; i++)
		{
			VectorSet (dir, crand (), crand (), crand ());
			VectorNormalize (dir, dir);
			VectorMA (self->org, (125.0 * ratio), dir, porg);

			makePart (
				porg[0],						porg[1],						porg[2],
				0,								0,								0,
				0,								0,								0,
				0,								0,								0,
				250,							250,							250,
				255,							255,							255,
				0.5,							-8,
				(2-(ratio*2)) + 5,				(2-(ratio*2)) + 5,
				PT_FLARE,						PF_SCALED,
				GL_SRC_ALPHA,					GL_ONE,
				0,								qFalse,
				0);
		}
	}
	else
	{
		int		clrtable[4] = {110, 112, 114, 116};
		int		rnum, rnum2;

		ratio = 1.0 - (((float)self->endtime - (float)cls.realTime)/1000.0);
		for (i=0 ; i<pIncDegree (self->org, 600, 0.1, qTrue) ; i++)
		{
			VectorSet (dir, crand (), crand (), crand ());
			VectorNormalize (dir, dir);
			VectorMA(self->org, (200.0 * ratio), dir, porg);

			rnum = (rand () % 4);
			rnum2 = (rand () % 4);
			makePart (
				porg[0],						porg[1],						porg[2],
				0,								0,								0,
				0,								0,								0,
				0,								0,								0,
				palRed (clrtable[rnum]),		palGreen (clrtable[rnum]),		palBlue (clrtable[rnum]),
				palRed (clrtable[rnum2]),		palGreen (clrtable[rnum2]),		palBlue (clrtable[rnum2]),
				1.0,							PART_INSTANT,
				1.0f,							1.0f,
				PT_WHITE,						PF_SCALED,
				GL_SRC_ALPHA,					GL_ONE,
				0,								qFalse,
				0);
		}
	}
}


/*
=================
CL_ParseNuke
=================
*/
void CL_ParseNuke (void)
{
	int			i;
	csustain_t	*s, *free_sustain;

	/* find a free sustain */
	free_sustain = NULL;
	for (i=0, s=cl_sustains ; i<MAX_SUSTAINS ; i++, s++)
	{
		if (s->id == 0)
		{
			free_sustain = s;
			break;
		}
	}

	if (free_sustain)
	{
		/* found one */
		s->id = 21000;
		MSG_ReadPos (&net_Message, s->org);
		s->endtime = cls.realTime + 1000;
		s->think = CL_Nukeblast;
		s->thinkinterval = 1;
		s->nextthink = cls.realTime;

		if (FS_CurrGame ("gloom"))
			CL_ExplosionParticles (s->org, 3.5, qFalse, qFalse);
	}
	else
	{
		/* no free sustains */
		vec3_t	pos;
		MSG_ReadPos (&net_Message, pos);
	}
}


/*
===============
CL_ParticleSteamEffect2
===============
*/
void CL_ParticleSteamEffect2 (csustain_t *self)
{
	int			i, rnum, rnum2;
	float		d;
	vec3_t		r, u, dir, pvel;

	VectorCopy (self->dir, dir);
	MakeNormalVectors (dir, r, u);

	for (i=0 ; i<pIncDegree (self->org, self->count, 0.5, qTrue) ; i++)
	{
		VectorScale (dir, self->magnitude, pvel);
		d = crand () * self->magnitude/3;
		VectorMA (pvel, d, r, pvel);
		d = crand () * self->magnitude/3;
		VectorMA (pvel, d, u, pvel);

		rnum = (rand () % 5);
		rnum2 = (rand () % 5);
		makePart (
			self->org[0] + self->magnitude * 0.1 * crand (),
			self->org[1] + self->magnitude * 0.1 * crand (),
			self->org[2] + self->magnitude * 0.1 * crand (),
			0,								0,								0,
			pvel[0],						pvel[1],						pvel[2],
			0,								0,								-20,
			palRed (self->color + rnum),	palGreen (self->color + rnum),	palBlue (self->color + rnum),
			palRed (self->color + rnum2),	palGreen (self->color + rnum2),	palBlue (self->color + rnum2),
			0.9 + (crand () * 0.1),			-1.0 / (0.5 + (frand () * 0.3)),
			3 + (frand () * 3),				8 + (frand () * 4),
			pRandSmoke (),					PF_SHADE,
			GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
			0,								qFalse,
			0);
	}

	self->nextthink += self->thinkinterval;
}


/*
=================
CL_ParseSteam
=================
*/
void CL_ParseSteam (void)
{
	int		id, i, r, cnt;
	int		color, magnitude;
	csustain_t	*s, *free_sustain;

	id = MSG_ReadShort (&net_Message);		// an id of -1 is an instant effect
	if (id != -1)
	{
		/* sustained */
		free_sustain = NULL;
		for (i=0, s=cl_sustains; i<MAX_SUSTAINS; i++, s++)
		{
			if (s->id == 0)
			{
				free_sustain = s;
				break;
			}
		}
		if (free_sustain)
		{
			s->id = id;
			s->count = MSG_ReadByte (&net_Message);
			MSG_ReadPos (&net_Message, s->org);
			MSG_ReadDir (&net_Message, s->dir);
			r = MSG_ReadByte (&net_Message);
			s->color = r & 0xff;
			s->magnitude = MSG_ReadShort (&net_Message);
			s->endtime = cls.realTime + MSG_ReadLong (&net_Message);
			s->think = CL_ParticleSteamEffect2;
			s->thinkinterval = 100;
			s->nextthink = cls.realTime;
		}
		else
		{
			vec3_t	pos, dir;
			cnt = MSG_ReadByte (&net_Message);
			MSG_ReadPos (&net_Message, pos);
			MSG_ReadDir (&net_Message, dir);
			r = MSG_ReadByte (&net_Message);
			magnitude = MSG_ReadShort (&net_Message);
			magnitude = MSG_ReadLong (&net_Message); // really interval
		}
	}
	else
	{
		/* instant */
		vec3_t	pos, dir;
		cnt = MSG_ReadByte (&net_Message);
		MSG_ReadPos (&net_Message, pos);
		MSG_ReadDir (&net_Message, dir);
		r = MSG_ReadByte (&net_Message);
		magnitude = MSG_ReadShort (&net_Message);
		color = r & 0xff;
		CL_ParticleSteamEffect (pos, dir, color, cnt, magnitude);
	}
}


/*
===============
CL_Widowbeamout
===============
*/
void CL_Widowbeamout (csustain_t *self)
{
	vec3_t			dir, porg;
	int				i, rnum, rnum2;
	static int		clrtable[4] = {2*8, 13*8, 21*8, 18*8};
	float			ratio;

	ratio = 1.0 - (((float)self->endtime - (float)cls.realTime)/ 2100.0);

	for(i=0;i<pIncDegree (self->org, 300, 0.5, qTrue) ;i++)
	{
		VectorSet (dir, crand (), crand (), crand ());
		VectorNormalize (dir, dir);
	
		VectorMA (self->org, (45.0 * ratio), dir, porg);

		rnum = (rand () % 4);
		rnum2 = (rand () % 4);
		makePart (
			porg[0],						porg[1],						porg[2],
			0,								0,								0,
			0,								0,								0,
			0,								0,								0,
			palRed (clrtable[rnum]),		palGreen (clrtable[rnum]),		palBlue (clrtable[rnum]),
			palRed (clrtable[rnum2]),		palGreen (clrtable[rnum2]),		palBlue (clrtable[rnum2]),
			1.0,							PART_INSTANT,
			1.0f,							1.0f,
			PT_WHITE,						PF_SCALED,
			GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
			0,								qFalse,
			0);
	}
}


/*
=================
CL_ParseWidow
=================
*/
void CL_ParseWidow (void)
{
	int		id, i;
	csustain_t	*s, *free_sustain;

	id = MSG_ReadShort (&net_Message);

	free_sustain = NULL;
	for (i=0, s=cl_sustains; i<MAX_SUSTAINS; i++, s++)
	{
		if (s->id == 0)
		{
			free_sustain = s;
			break;
		}
	}
	if (free_sustain)
	{
		/* found one */
		s->id = id;
		MSG_ReadPos (&net_Message, s->org);
		s->endtime = cls.realTime + 2100;
		s->think = CL_Widowbeamout;
		s->thinkinterval = 1;
		s->nextthink = cls.realTime;
	}
	else
	{
		/* no free sustains */
		vec3_t	pos;
		MSG_ReadPos (&net_Message, pos);
	}
}
