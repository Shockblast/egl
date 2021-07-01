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
cgSustainPfx_t	cgPfxSustains[MAX_SUSTAINS];

/*
==============================================================

	SUSTAIN HANDLING

==============================================================
*/

/*
=================
CG_ClearSustains
=================
*/
void CG_ClearSustains (void) {
	memset (cgPfxSustains, 0, sizeof (cgPfxSustains));
}


/*
=================
CG_AddSustains
=================
*/
void CG_AddSustains (void) {
	cgSustainPfx_t		*s;
	int				i;

	for (i=0, s=cgPfxSustains ; i<MAX_SUSTAINS ; i++, s++) {
		if (s->id) {
			if ((s->endtime >= cgs.realTime) && (cgs.realTime >= s->nextthink))
				s->think (s);
			else if (s->endtime < cgs.realTime)
				s->id = 0;
		}
	}
}


/*
=================
CG_FindSustainSlot
=================
*/
cgSustainPfx_t *CG_FindSustainSlot (void) {
	int			i;
	cgSustainPfx_t	*s;

	for (i=0, s=cgPfxSustains ; i<MAX_SUSTAINS ; i++, s++) {
		if (s->id == 0) {
			return s;
			break;
		}
	}

	return NULL;
}

/*
==============================================================

	SUSTAINED EFFECTS

==============================================================
*/

/*
===============
CG_Nukeblast
===============
*/
void CG_Nukeblast (cgSustainPfx_t *self) {
	vec3_t			dir, porg;
	int				i;
	float			ratio;

	if (cgi.FS_CurrGame ("gloom")) {
		ratio = 1.0 - (((float)self->endtime - (float)cgs.realTime) / 1000.0);

		cgi.R_AddLight (self->org, 600 * ratio * (0.9 + (frand () * 0.1)), 1, 1, 1);

		if ((int)(ratio*10) & 1)
			CG_ExplosionParticles (self->org, 3.5 + (crand () * 0.5), qTrue, qFalse);

		for (i=0 ; i<pIncDegree (self->org, 100, 0.5, qTrue) ; i++) {
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
	} else {
		int		clrtable[4] = {110, 112, 114, 116};
		int		rnum, rnum2;

		ratio = 1.0 - (((float)self->endtime - (float)cgs.realTime)/1000.0);
		for (i=0 ; i<pIncDegree (self->org, 600, 0.1, qTrue) ; i++) {
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
===============
CG_ParticleSteamEffect

Puffs with velocity along direction, with some randomness thrown in
===============
*/
void FASTCALL CG_ParticleSteamEffect (vec3_t org, vec3_t dir, int color, int count, int magnitude) {
	int			i, rnum, rnum2;
	float		d;
	vec3_t		r, u, pvel;

	MakeNormalVectors (dir, r, u);

	for (i=0 ; i<pIncDegree (org, count, 0.5, qTrue) ; i++) {
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
CG_ParticleSteamEffect2
===============
*/
void CG_ParticleSteamEffect2 (cgSustainPfx_t *self) {
	int			i, rnum, rnum2;
	float		d;
	vec3_t		r, u, dir, pvel;

	VectorCopy (self->dir, dir);
	MakeNormalVectors (dir, r, u);

	for (i=0 ; i<pIncDegree (self->org, self->count, 0.5, qTrue) ; i++) {
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
===============
CG_Widowbeamout
===============
*/
void CG_Widowbeamout (cgSustainPfx_t *self) {
	vec3_t			dir, porg;
	int				i, rnum, rnum2;
	static int		clrtable[4] = {2*8, 13*8, 21*8, 18*8};
	float			ratio;

	ratio = 1.0 - (((float)self->endtime - (float)cgs.realTime)/ 2100.0);

	for (i=0 ; i<pIncDegree (self->org, 300, 0.5, qTrue) ; i++) {
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
==============================================================

	SUSTAIN PARSING

==============================================================
*/

/*
=================
CG_ParseNuke
=================
*/
void CG_ParseNuke (void) {
	cgSustainPfx_t	*s;

	s = CG_FindSustainSlot (); // find a free sustain

	if (s) {
		// found one
		s->id = 21000;
		cgi.MSG_ReadPos (s->org);
		s->endtime = cgs.realTime + 1000;
		s->think = CG_Nukeblast;
		s->thinkinterval = 1;
		s->nextthink = cgs.realTime;

		if (cgi.FS_CurrGame ("gloom"))
			CG_ExplosionParticles (s->org, 3.5, qFalse, qFalse);
	} else {
		// no free sustains
		vec3_t	pos;
		cgi.MSG_ReadPos (pos);
	}
}


/*
=================
CG_ParseSteam
=================
*/
void CG_ParseSteam (void) {
	int		id, r, cnt;
	int		color, magnitude;
	cgSustainPfx_t	*s;

	id = cgi.MSG_ReadShort ();		// an id of -1 is an instant effect
	if (id != -1) {
		// sustained
		s = CG_FindSustainSlot (); // find a free sustain

		if (s) {
			s->id = id;
			s->count = cgi.MSG_ReadByte ();
			cgi.MSG_ReadPos (s->org);
			cgi.MSG_ReadDir (s->dir);
			r = cgi.MSG_ReadByte ();
			s->color = r & 0xff;
			s->magnitude = cgi.MSG_ReadShort ();
			s->endtime = cgs.realTime + cgi.MSG_ReadLong ();
			s->think = CG_ParticleSteamEffect2;
			s->thinkinterval = 100;
			s->nextthink = cgs.realTime;
		} else {
			vec3_t	pos, dir;
			cnt = cgi.MSG_ReadByte ();
			cgi.MSG_ReadPos (pos);
			cgi.MSG_ReadDir (dir);
			r = cgi.MSG_ReadByte ();
			magnitude = cgi.MSG_ReadShort ();
			magnitude = cgi.MSG_ReadLong (); // really interval
		}
	} else {
		// instant
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
void CG_ParseWidow (void) {
	int			id;
	cgSustainPfx_t	*s;

	id = cgi.MSG_ReadShort ();

	s = CG_FindSustainSlot (); // find a free sustain

	if (s) {
		// found one
		s->id = id;
		cgi.MSG_ReadPos (s->org);
		s->endtime = cgs.realTime + 2100;
		s->think = CG_Widowbeamout;
		s->thinkinterval = 1;
		s->nextthink = cgs.realTime;
	} else {
		// no free sustains
		vec3_t	pos;
		cgi.MSG_ReadPos (pos);
	}
}
