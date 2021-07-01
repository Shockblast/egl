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
// cg_localents.c
// Local entities
// To do:
// - Everything
//

#include "cg_local.h"

enum {
	LE_BULLETCASING,
	LE_RAILCASING,
	LE_SHOTGUNCASING
};

/*
=============================================================================

	CLIENT-SIDE ENTITIES MANAGEMENT

=============================================================================
*/


typedef struct clent_t {
	struct clent_t	*next;

	float			time;

	struct model_s	*model;				// opaque type outside refresh
	float			angles[3];

	// most recent data
	float			origin[3];
	int				frame;

	float			oldOrigin[3];
	int				oldframe;

	// misc
	float			backlerp;			// 0.0 = current, 1.0 = old
	int				skinnum;			// also used as RF_BEAM's palette index

	int				lightstyle;			// for flashing entities
	float			alpha;				// ignore if RF_TRANSLUCENT isn't set
	float			alphavel;

	struct shader_s	*skin;				// NULL for inline skin
	int				flags;

	int				refflags;			// my stuff

	vec3_t			vel;
	vec3_t			avel;
	vec3_t			accel;

	void			(*think)(struct clent_t *c, vec3_t org, vec3_t angles, float *alpha, int *type, float *time);
} clent_t;

#define INSTANT_CLENT	-10000.0

#define CLF_BOUNCE		1
#define CLF_GRAVITY		2

clent_t		*active_clents, *free_clents;
clent_t		clents[MAX_CLENTITIES];
int			cgNumLEnts = MAX_CLENTITIES;

/*
===============
makeCLEnt
===============
*/
clent_t	*makeLEnt	(float org0,					float org1,						float org2,
					 float angle0,					float angle1,					float angle2,
					 float vel0,					float vel1,						float vel2,
					 float avel0,					float avel1,					float avel2,
					 float accel0,					float accel1,					float accel2,
					 float alpha,					float alphavel,
					 int refflags,					int flags,
					 int lightstyle,
					 struct model_s *model,
					 int frame,
					 struct shader_s	*skin,
					 int skinnum,
					 void (*think)(struct clent_t *c, vec3_t org, vec3_t angle, float *alpha, int *type, float *time)) {
	clent_t		*c;

	if (!free_clents)
		return NULL;

	c = free_clents;
	free_clents = c->next;
	c->next = active_clents;
	active_clents = c;

	c->time = cgs.realTime;

	// model stuff
	c->model = model;

	c->frame = frame;
	c->oldframe = frame;
	c->backlerp = 1.0 - cg.lerpFrac;

	// skin stuff
	c->skin = skin;
	c->skinnum = skinnum;

	c->lightstyle = lightstyle;

	// misc
	c->origin[0] = org0;
	c->origin[1] = org1;
	c->origin[2] = org2;
	
	VectorCopy (c->origin, c->oldOrigin);

	c->angles[0] = angle0;
	c->angles[1] = angle1;
	c->angles[2] = angle2;

	c->vel[0] = vel0;
	c->vel[1] = vel1;
	c->vel[2] = vel2;

	c->avel[0] = avel0;
	c->avel[1] = avel1;
	c->avel[2] = avel2;
	
	c->accel[0] = accel0;
	c->accel[1] = accel1;
	c->accel[2] = accel2;

	c->alpha = alpha;
	c->alphavel = alphavel;

	c->refflags = refflags;
	c->flags = flags;

	if (think)	c->think = think;
	else		c->think = NULL;

	return c;
}


/*
===============
CG_ClearLocalEnts
===============
*/
void CG_ClearLocalEnts (void) {
	int		i;
	
	free_clents = &clents[0];
	active_clents = NULL;

	for (i=0 ;i<cgNumLEnts ; i++)
		clents[i].next = &clents[i+1];
	clents[cgNumLEnts-1].next = NULL;
}


/*
===============
CG_AddLocalEnts
===============
*/
void CG_AddLocalEnts (void) {
#if 0
	clent_t		*c, *next;
	clent_t		*active, *tail;
	trace_t		trace;
	entity_t	ent;
	vec3_t		org, move;
	float		alpha;
	float		time, time2;
	int			i;
	vec3_t		angles;

	active = NULL;
	tail = NULL;

	for (c=active_clents ; c ; c=next) {
		next = c->next;

		// PMM - added INSTANT_CLENT handling for heat beam
		time = 1;
		if (c->alphavel != INSTANT_CLENT) {
			time = (cgs.realTime - c->time)*0.001;
			alpha = c->alpha + time*c->alphavel;
			if (alpha <= 0) {
				// faded out
				c->next = free_clents;
				free_clents = c;
				continue;
			}
		}
		else {
			alpha = c->alpha;
		}

		c->next = NULL;
		if (!tail)
			active = tail = c;
		else {
			tail->next = c;
			tail = c;
		}

		if (alpha > 1.0) alpha = 1;

		time2 = time*time;

		for (i=0 ; i<3 ; i++)
			org[i] = c->origin[i] + c->vel[i]*time + c->accel[i]*time2;

		if (c->think)
			c->think(c, org, c->angles, &alpha, &c->refflags, &time);

		if (c->flags&CLF_GRAVITY) {
			c->vel[2] -= 10;
		}
#define FRAMETIME 0.1
		VectorMA (c->angles, FRAMETIME, c->avel, c->angles);

		if (c->flags&CLF_BOUNCE) {
			VectorScale (c->vel, FRAMETIME, move);
			trace = CG_Trace (org, move, 5, 1);

			if (trace.fraction < 1) {
				if (trace.plane.normal[2] > 0.7) {
					if (c->vel[2] < 60) {
						VectorClear (c->vel);
						VectorClear (c->vel);
					}
				}
			}
		}

		// model stuff
		ent.model = c->model;

		ent.frame = c->frame;
		ent.oldframe = c->oldframe;
		ent.backlerp = 1.0 - cg.lerpFrac;

		// skin stuff
		ent.skin = c->skin;
		ent.skinnum = c->skinnum;

		ent.lightstyle = c->lightstyle;

		// misc
		VectorCopy (org, ent.origin);
		VectorCopy (c->oldOrigin, ent.oldOrigin);
		VectorCopy (org, c->oldOrigin);

		VectorCopy (c->angles, angles);
		if (angles[0] || angles[1] || angles[2])
			AnglesToAxis (angles, ent.axis);
		else
			Matrix_Identity (ent.axis);

		Vector4Set (ent.color, 255, 255, 255, 255);
		ent.scale = 1;
		ent.flags = c->refflags;

		R_AddEntity (&ent);

		if (c->alphavel == PART_INSTANT) {
			c->alphavel = 0.0;
			c->alpha = 0.0;
		}
	}

	active_clents = active;
#endif
}
