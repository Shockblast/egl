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
// cg_partsustain.c
//

#include "cg_local.h"

#define MAX_SUSTAINS	32
cgSustainPfx_t	cg_pfxSustains[MAX_SUSTAINS];

/*
=============================================================================

	SUSTAIN HANDLING

=============================================================================
*/

/*
=================
CG_ClearSustains
=================
*/
void CG_ClearSustains (void)
{
	memset (cg_pfxSustains, 0, sizeof (cg_pfxSustains));
}


/*
=================
CG_AddSustains
=================
*/
void CG_AddSustains (void)
{
	cgSustainPfx_t		*s;
	int				i;

	for (i=0, s=cg_pfxSustains ; i<MAX_SUSTAINS ; i++, s++) {
		if (s->id) {
			if ((s->endtime >= cg.realTime) && (cg.realTime >= s->nextthink))
				s->think (s);
			else if (s->endtime < cg.realTime)
				s->id = 0;
		}
	}
}


/*
=================
CG_FindSustainSlot
=================
*/
cgSustainPfx_t *CG_FindSustainSlot (void)
{
	int			i;
	cgSustainPfx_t	*s;

	for (i=0, s=cg_pfxSustains ; i<MAX_SUSTAINS ; i++, s++) {
		if (s->id == 0) {
			return s;
			break;
		}
	}

	return NULL;
}

/*
=============================================================================

	SUSTAINED EFFECTS

=============================================================================
*/

/*
===============
CG_Nukeblast
===============
*/
static void CG_Nukeblast (cgSustainPfx_t *self)
{
	vec3_t			dir, porg;
	int				i;
	float			ratio;

	if (cg.currGameMod == GAME_MOD_GLOOM) {
		int		rnum, rnum2;

		ratio = 1.0f - (((float)self->endtime - (float)cg.realTime) / 1000.0f);

		cgi.R_AddLight (self->org, 600 * ratio * (0.9f + (frand () * 0.1f)), 1, 1, 1);

		// Sparks
		for (i=0 ; i<50 ; i++) {
			CG_SpawnParticle (
				self->org[0] + (crand () * 36),		self->org[1] + (crand () * 36),		self->org[2] + (crand () * 36) + (frand () * 100.0f),
				0,									0,									0,
				crand () * 500,						crand () * 500,						crand () * 500,
				0,									0,									0,
				235 + (frand () * 20),				225 + (frand () * 20),				205,
				235 + (frand () * 20),				225 + (frand () * 20),				205,
				0.9f,								-1.5f / (0.6f + (crand () * 0.15f)),
				0.3f,								0.4f,
				PT_SPARK,							0,
				pSparkGrowThink,					qTrue,
				PART_STYLE_DIRECTION,
				(16 + (crand () * 4)) * 3);
		}

		// Smoke
		if ((int)(ratio*10) & 1) {
			rnum = 70 + (frand () * 40);
			rnum2 = 80 + (frand () * 40);
			CG_SpawnParticle (
				self->org[0] + (crand () * 8),	self->org[1] + (crand () * 8),	self->org[2] + (frand () * 150.0f),
				0,								0,								0,
				crand () * 2,					crand () * 2,					crand () * 2,
				0,								0,								5 + (frand () * 6),
				rnum,							rnum,							rnum,
				rnum2,							rnum2,							rnum2,
				0.75f + (crand () * 0.1f),		-2.0f / (1.5f + (cg_particleSmokeLinger->floatVal * 0.4f) + (crand () * 0.2f)),
				(150 + (crand () * 5)),			(150 + (crand () * 20)),
				pRandSmoke (),					PF_SHADE,
				pSmokeThink,					qTrue,
				PART_STYLE_QUAD,
				frand () * 361);
		}

		// Rising explosion anim
		if (rand () & 1) {
			CG_SpawnParticle (
				self->org[0],						self->org[1],						self->org[2] + (crand () * 64) + 32,
				0,									0,									0,
				0,									0,									ratio * 650.0f,
				0,									0,									ratio * -250.0f,
				255,								255,								255,
				255,								255,								255,
				0.75f,								-2.5f + (crand () * 0.1f) + 0.33f,
				(40 + (crand () * 5)),				(260 + (crand () * 20)),
				PT_EXPLO1,							PF_NOCLOSECULL,
				pExploAnimThink,					qTrue,
				PART_STYLE_QUAD,
				crand () * 12);
		}
		else {
			CG_SpawnParticle (
				self->org[0],						self->org[1],						self->org[2] + (crand () * 64) + 32,
				0,									0,									0,
				0,									0,									ratio * 650.0f,
				0,									0,									ratio * -250.0f,
				255,								255,								255,
				255,								255,								255,
				0.75f,								-2.5f + (crand () * 0.1f) + 0.33f,
				(40 + (crand () * 5)),				(260 + (crand () * 20)),
				PT_EXPLO1,							PF_NOCLOSECULL,
				NULL,								qFalse,
				PART_STYLE_QUAD,
				crand () * 12);
		}

		// Bottom explosion anim
		if (rand () & 1) {
			CG_SpawnParticle (
				self->org[0] + (crand () * 32),		self->org[1] + (crand () * 32),		self->org[2],
				0,									0,									0,
				0,									0,									0,
				0,									0,									0,
				255,								255,								255,
				255,								255,								255,
				0.75f,								-3 + (crand () * 0.1f) + 0.33f,
				(60 + (crand () * 20)),				(150 + (crand () * 10)),
				PT_EXPLO1,							PF_NOCLOSECULL,
				pExploAnimThink,					qTrue,
				PART_STYLE_QUAD,
				crand () * 12);
		}
		else {
			CG_SpawnParticle (
				self->org[0] + (crand () * 32),		self->org[1] + (crand () * 32),		self->org[2],
				0,									0,									0,
				0,									0,									0,
				0,									0,									0,
				255,								255,								255,
				255,								255,								255,
				0.75f,								-3 + (crand () * 0.1f) + 0.33f,
				(60 + (crand () * 20)),				(150 + (crand () * 10)),
				PT_EXPLO1,							PF_NOCLOSECULL,
				NULL,								qFalse,
				PART_STYLE_QUAD,
				crand () * 12);
		}

		// Top explosion anim
		if (rand () & 1) {
			CG_SpawnParticle (
				self->org[0] + (crand () * 64),		self->org[1] + (crand () * 64),		self->org[2] + 150.0f,
				0,									0,									0,
				0,									0,									0,
				0,									0,									0,
				255,								255,								255,
				255,								255,								255,
				0.75f,								-3 + (crand () * 0.1f) + 0.33f,
				(80 + (crand () * 5)),				(350 + (crand () * 50)),
				PT_EXPLO1,							PF_NOCLOSECULL,
				pExploAnimThink,					qTrue,
				PART_STYLE_QUAD,
				crand () * 12);
		}
		else {
			CG_SpawnParticle (
				self->org[0] + (crand () * 64),		self->org[1] + (crand () * 64),		self->org[2] + 150.0f,
				0,									0,									0,
				0,									0,									0,
				0,									0,									0,
				255,								255,								255,
				255,								255,								255,
				0.75f,								-3 + (crand () * 0.1f) + 0.33f,
				(80 + (crand () * 5)),				(350 + (crand () * 50)),
				PT_EXPLO1,							PF_NOCLOSECULL,
				NULL,								qFalse,
				PART_STYLE_QUAD,
				crand () * 12);
		}
	}
	else {
		int		clrtable[4] = {110, 112, 114, 116};
		int		rnum, rnum2;

		ratio = 1.0f - (((float)self->endtime - (float)cg.realTime)/1000.0f);
		for (i=0 ; i<600 ; i++) {
			Vec3Set (dir, crand (), crand (), crand ());
			VectorNormalizeFastf (dir);
			Vec3MA (self->org, (200.0f * ratio), dir, porg);

			rnum = (rand () % 4);
			rnum2 = (rand () % 4);
			CG_SpawnParticle (
				porg[0],						porg[1],						porg[2],
				0,								0,								0,
				0,								0,								0,
				0,								0,								0,
				palRed (clrtable[rnum]),		palGreen (clrtable[rnum]),		palBlue (clrtable[rnum]),
				palRed (clrtable[rnum2]),		palGreen (clrtable[rnum2]),		palBlue (clrtable[rnum2]),
				1.0,							PART_INSTANT,
				1.0f,							1.0f,
				PT_GENERIC,						PF_SCALED,
				0,								qFalse,
				PART_STYLE_QUAD,
				0);
		}
	}
}

/*
===============
CG_ParticleSteamEffect

Puffs with velocity along direction, with some randomness thrown in
===============
*/
void CG_ParticleSteamEffect (vec3_t org, vec3_t dir, int color, int count, int magnitude)
{
	int			rnum, rnum2;
	float		i, d;
	vec3_t		r, u, pvel;

	MakeNormalVectorsf (dir, r, u);

	for (i=0 ; i<count ; i++) {
		Vec3Scale (dir, magnitude, pvel);
		d = crand () * magnitude/3;
		Vec3MA (pvel, d, r, pvel);
		d = crand () * magnitude/3;
		Vec3MA (pvel, d, u, pvel);

		rnum = (rand () % 5);
		rnum2 = (rand () % 5);
		CG_SpawnParticle (
			org[0] + magnitude*0.1f*crand(),org[1] + magnitude*0.1f*crand(),org[2] + magnitude*0.1f*crand(),
			0,								0,								0,
			pvel[0],						pvel[1],						pvel[2],
			0,								0,								-40/2,
			palRed (color + rnum),			palGreen (color + rnum),		palBlue (color + rnum),
			palRed (color + rnum2),			palGreen (color + rnum2),		palBlue (color + rnum2),
			0.9f + (crand () * 0.1f),		-1.0f / (0.5f + (frand () * 0.3f)),
			3 + (frand () * 3),				8 + (frand () * 4),
			pRandSmoke (),					PF_SHADE,
			0,								qFalse,
			PART_STYLE_QUAD,
			0);
	}
}


/*
===============
CG_ParticleSteamEffect2
===============
*/
void CG_ParticleSteamEffect2 (cgSustainPfx_t *self)
{
	int			i, rnum, rnum2;
	float		d;
	vec3_t		r, u, dir, pvel;

	Vec3Copy (self->dir, dir);
	MakeNormalVectorsf (dir, r, u);

	for (i=0 ; i<self->count ; i++) {
		Vec3Scale (dir, self->magnitude, pvel);
		d = crand () * self->magnitude/3;
		Vec3MA (pvel, d, r, pvel);
		d = crand () * self->magnitude/3;
		Vec3MA (pvel, d, u, pvel);

		rnum = (rand () % 5);
		rnum2 = (rand () % 5);
		CG_SpawnParticle (
			self->org[0] + self->magnitude * 0.1f * crand (),
			self->org[1] + self->magnitude * 0.1f * crand (),
			self->org[2] + self->magnitude * 0.1f * crand (),
			0,								0,								0,
			pvel[0],						pvel[1],						pvel[2],
			0,								0,								-20,
			palRed (self->color + rnum),	palGreen (self->color + rnum),	palBlue (self->color + rnum),
			palRed (self->color + rnum2),	palGreen (self->color + rnum2),	palBlue (self->color + rnum2),
			0.9f + (crand () * 0.1f),		-1.0f / (0.5f + (frand () * 0.3f)),
			3 + (frand () * 3),				8 + (frand () * 4),
			pRandSmoke (),					PF_SHADE,
			0,								qFalse,
			PART_STYLE_QUAD,
			0);
	}

	self->nextthink += self->thinkinterval;
}


/*
===============
CG_Widowbeamout
===============
*/
void CG_Widowbeamout (cgSustainPfx_t *self)
{
	vec3_t			dir, porg;
	int				i, rnum, rnum2;
	static int		clrtable[4] = {2*8, 13*8, 21*8, 18*8};
	float			ratio;

	ratio = 1.0f - (((float)self->endtime - (float)cg.realTime)/ 2100.0f);

	for (i=0 ; i<300 ; i++) {
		Vec3Set (dir, crand (), crand (), crand ());
		VectorNormalizeFastf (dir);
	
		Vec3MA (self->org, (45.0f * ratio), dir, porg);

		rnum = (rand () % 4);
		rnum2 = (rand () % 4);
		CG_SpawnParticle (
			porg[0],						porg[1],						porg[2],
			0,								0,								0,
			0,								0,								0,
			0,								0,								0,
			palRed (clrtable[rnum]),		palGreen (clrtable[rnum]),		palBlue (clrtable[rnum]),
			palRed (clrtable[rnum2]),		palGreen (clrtable[rnum2]),		palBlue (clrtable[rnum2]),
			1.0,							PART_INSTANT,
			1.0f,							1.0f,
			PT_GENERIC,						PF_SCALED,
			0,								qFalse,
			PART_STYLE_QUAD,
			0);
	}
}

/*
=============================================================================

	SUSTAIN PARSING

=============================================================================
*/

/*
=================
CG_ParseNuke
=================
*/
void CG_ParseNuke (void)
{
	cgSustainPfx_t	*s;

	s = CG_FindSustainSlot (); // find a free sustain

	if (s) {
		// Found one
		s->id = 21000;
		cgi.MSG_ReadPos (s->org);
		s->endtime = cg.realTime + 1000;
		s->think = CG_Nukeblast;
		s->thinkinterval = 1;
		s->nextthink = cg.realTime;

		if (cg.currGameMod == GAME_MOD_GLOOM) {
			vec3_t	target, top;
			trace_t	tr;
			int		i;

			// Explosion mark
			// FIXME: better placement (see BFG explosion code)
			for (i=0 ; i<6 ; i++) {
				Vec3Copy (s->org, target);
				Vec3Copy (s->org, top);

				switch (i) {
				case 0: target[0] -= 35; top[0] += 35; break;
				case 1:	target[0] += 35; top[0] -= 35; break;
				case 2:	target[1] -= 35; top[1] += 35; break;
				case 3:	target[1] += 35; top[1] -= 35; break;
				case 4:	target[2] -= 35; top[2] += 35; break;
				case 5:	target[2] += 35; top[2] -= 35; break;
				}

				tr = cgi.CM_Trace (top, target, 1, 1);
				if (tr.fraction >= 1)
					continue;	// Didn't hit anything

				// Burn mark
				CG_SpawnDecal (
					tr.endPos[0],						tr.endPos[1],						tr.endPos[2],
					tr.plane.normal[0],					tr.plane.normal[1],					tr.plane.normal[2],
					255,								255,								255,
					0,									0,									0,
					0.9f + (crand () * 0.1f),			0.8f,
					(35 + (frand () * 5)) * 3,
					dRandExploMark (),					DF_ALPHACOLOR,
					0,									qFalse,
					0,									frand () * 360);
			}
		}
	}
	else {
		// No free sustains
		vec3_t	pos;
		cgi.MSG_ReadPos (pos);
	}
}


/*
=================
CG_ParseSteam
=================
*/
void CG_ParseSteam (void)
{
	int		id, r, cnt;
	int		color, magnitude;
	cgSustainPfx_t	*s;

	id = cgi.MSG_ReadShort ();		// an id of -1 is an instant effect
	if (id != -1) {
		// Sustained
		s = CG_FindSustainSlot (); // find a free sustain

		if (s) {
			s->id = id;
			s->count = cgi.MSG_ReadByte ();
			cgi.MSG_ReadPos (s->org);
			cgi.MSG_ReadDir (s->dir);
			r = cgi.MSG_ReadByte ();
			s->color = r & 0xff;
			s->magnitude = cgi.MSG_ReadShort ();
			s->endtime = cg.realTime + cgi.MSG_ReadLong ();
			s->think = CG_ParticleSteamEffect2;
			s->thinkinterval = 100;
			s->nextthink = cg.realTime;
		}
		else {
			vec3_t	pos, dir;
			cnt = cgi.MSG_ReadByte ();
			cgi.MSG_ReadPos (pos);
			cgi.MSG_ReadDir (dir);
			r = cgi.MSG_ReadByte ();
			magnitude = cgi.MSG_ReadShort ();
			magnitude = cgi.MSG_ReadLong (); // really interval
		}
	}
	else {
		// Instant
		vec3_t	pos, dir;
		cnt = cgi.MSG_ReadByte ();
		cgi.MSG_ReadPos (pos);
		cgi.MSG_ReadDir (dir);
		r = cgi.MSG_ReadByte ();
		magnitude = cgi.MSG_ReadShort ();
		color = r & 0xff;
		CG_ParticleSteamEffect (pos, dir, color, cnt, magnitude);
	}
}


/*
=================
CG_ParseWidow
=================
*/
void CG_ParseWidow (void)
{
	int			id;
	cgSustainPfx_t	*s;

	id = cgi.MSG_ReadShort ();

	s = CG_FindSustainSlot (); // find a free sustain

	if (s) {
		// Found one
		s->id = id;
		cgi.MSG_ReadPos (s->org);
		s->endtime = cg.realTime + 2100;
		s->think = CG_Widowbeamout;
		s->thinkinterval = 1;
		s->nextthink = cg.realTime;
	}
	else {
		// No free sustains
		vec3_t	pos;
		cgi.MSG_ReadPos (pos);
	}
}
