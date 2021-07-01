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

// cl_tent.c
// - client side temporary entities

#include "cl_local.h"

/*
==============================================================

	EXPLOSION SCREEN RATTLES

==============================================================
*/

#define MAX_EXPLORATTLES		32
float	cl_ExploRattles[MAX_EXPLORATTLES];

/*
=================
CL_ExploRattle
=================
*/
void CL_ExploRattle (vec3_t org, float scale) {
	int		i;
	float	dist, max;
	vec3_t	temp;

	if (!cl_explorattle->integer)
		return;

	for (i=0 ; i<MAX_EXPLORATTLES ; i++) {
		if (cl_ExploRattles[i] > 0)
			continue;

		// calculate distance
		dist = VectorDistance (cl.refDef.viewOrigin, org) * 0.1;
		max = (20 * scale, 20, 50);

		// lessen the effect when it's behind the view
		VectorSubtract (org, cl.refDef.viewOrigin, temp);
		VectorNormalize (temp, temp);

		if (DotProduct (temp, cl.refDef.viewAxis[0]) < 0)
			dist *= 1.25;

		// clamp
		if ((dist > 0) && (dist < max))
			cl_ExploRattles[i] = max - dist;

		break;
	}
}


/*
=================
CL_AddExploRattles
=================
*/
void CL_AddExploRattles (void) {
	int		i;
	float	scale;

	if (!cl_explorattle->integer)
		return;

	scale = clamp (cl_explorattle_scale->value, 0, 0.999);
	for (i=0 ; i<MAX_EXPLORATTLES ; i++) {
		if (cl_ExploRattles[i] <= 0)
			continue;

		cl_ExploRattles[i] *= scale;

		cl.refDef.viewAngles[0] += cl_ExploRattles[i] * crand ();
		cl.refDef.viewAngles[1] += cl_ExploRattles[i] * crand ();
		cl.refDef.viewAngles[2] += cl_ExploRattles[i] * crand ();

		if (cl_ExploRattles[i] < 0.001)
			cl_ExploRattles[i] = -1;
	}
}


/*
=================
CL_ClearExploRattles
=================
*/
void CL_ClearExploRattles (void) {
	int		i;

	for (i=0 ; i<MAX_EXPLORATTLES ; i++)
		cl_ExploRattles[i] = -1;
}

/*
==============================================================

	BEAM MANAGEMENT

==============================================================
*/

#define	MAX_BEAMS	32
typedef struct {
	int		entity;
	int		dest_entity;
	struct model_s	*model;
	int		endtime;
	vec3_t	offset;
	vec3_t	start, end;
} beam_t;
beam_t		cl_beams[MAX_BEAMS];
//PMM - added this for player-linked beams.  Currently only used by the plasma beam
beam_t		cl_playerbeams[MAX_BEAMS];

/*
=================
CL_ParseBeam
=================
*/
int CL_ParseBeam (struct model_s *model) {
	int		ent;
	vec3_t	start, end;
	beam_t	*b;
	int		i;
	
	ent = MSG_ReadShort (&net_Message);
	
	MSG_ReadPos (&net_Message, start);
	MSG_ReadPos (&net_Message, end);

	// override any beam with the same entity
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++) {
		if (b->entity == ent) {
			b->entity = ent;
			b->model = model;
			b->endtime = cls.realTime + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorIdentity (b->offset);
			return ent;
		}
	}

	// find a free beam
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++) {
		if (!b->model || (b->endtime < cls.realTime)) {
			b->entity = ent;
			b->model = model;
			b->endtime = cls.realTime + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorIdentity (b->offset);
			return ent;
		}
	}

	Com_Printf (PRINT_ALL, S_COLOR_YELLOW "beam list overflow!\n");
	return ent;
}


/*
=================
CL_ParseBeam2
=================
*/
int CL_ParseBeam2 (struct model_s *model) {
	int		ent;
	vec3_t	start, end, offset;
	beam_t	*b;
	int		i;
	
	ent = MSG_ReadShort (&net_Message);
	
	MSG_ReadPos (&net_Message, start);
	MSG_ReadPos (&net_Message, end);
	MSG_ReadPos (&net_Message, offset);

	// override any beam with the same entity
	for (i=0, b=cl_beams ; i<MAX_BEAMS ; i++, b++) {
		if (b->entity == ent) {
			b->entity = ent;
			b->model = model;
			b->endtime = cls.realTime + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorCopy (offset, b->offset);
			return ent;
		}
	}

	// find a free beam
	for (i=0, b=cl_beams ; i<MAX_BEAMS ; i++, b++) {
		if (!b->model || (b->endtime < cls.realTime)) {
			b->entity = ent;
			b->model = model;
			b->endtime = cls.realTime + 200;	
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorCopy (offset, b->offset);
			return ent;
		}
	}

	Com_Printf (PRINT_ALL, S_COLOR_YELLOW "beam list overflow!\n");	
	return ent;
}


/*
=================
CL_ParsePlayerBeam

Adds to the cl_playerbeam array instead of the cl_beams array
=================
*/
int CL_ParsePlayerBeam (struct model_s *model) {
	int		ent;
	vec3_t	start, end, offset;
	beam_t	*b;
	int		i;
	
	ent = MSG_ReadShort (&net_Message);
	
	MSG_ReadPos (&net_Message, start);
	MSG_ReadPos (&net_Message, end);

	if (model == clMedia.heatBeamMOD)
		VectorSet (offset, 2, 7, -3);
	else if (model == clMedia.monsterHeatBeamMOD) {
		model = clMedia.heatBeamMOD;
		VectorSet (offset, 0, 0, 0);
	} else
		MSG_ReadPos (&net_Message, offset);

	// override any beam with the same entity
	// PMM - For player beams, we only want one per player (entity) so..
	for (i=0, b=cl_playerbeams ; i< MAX_BEAMS ; i++, b++) {
		if (b->entity == ent) {
			b->entity = ent;
			b->model = model;
			b->endtime = cls.realTime + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorCopy (offset, b->offset);
			return ent;
		}
	}

	// find a free beam
	for (i=0, b=cl_playerbeams ; i< MAX_BEAMS ; i++, b++) {
		if (!b->model || (b->endtime < cls.realTime)) {
			b->entity = ent;
			b->model = model;
			b->endtime = cls.realTime + 100;		// PMM - this needs to be 100 to prevent multiple heatbeams
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorCopy (offset, b->offset);
			return ent;
		}
	}

	Com_Printf (PRINT_ALL, S_COLOR_YELLOW "beam list overflow!\n");	
	return ent;
}


/*
=================
CL_AddBeams
=================
*/
void CL_AddBeams (void) {
	int			i, j;
	float		d, yaw, pitch, forward, len, steps;
	float		model_length;
	beam_t		*b;
	vec3_t		dist, org;
	entity_t	ent;
	vec3_t		angles;
	
	// update beams
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++) {
		if (!b->model || (b->endtime < cls.realTime))
			continue;

		// if coming from the player, update the start position
		// entity 0 is the world
		if (b->entity == cl.playerNum+1) {
			VectorCopy (cl.refDef.viewOrigin, b->start);
			b->start[2] -= 22;	// adjust for view height
		}
		VectorAdd (b->start, b->offset, org);

		// calculate pitch and yaw
		VectorSubtract (b->end, org, dist);

		if (dist[1] == 0 && dist[0] == 0) {
			yaw = 0;
			if (dist[2] > 0)
				pitch = 90;
			else
				pitch = 270;
		} else {
			// PMM - fixed to correct for pitch of 0
			if (dist[0])
				yaw = (atan2 (dist[1], dist[0]) * M_180DIV_PI);
			else if (dist[1] > 0)
				yaw = 90;
			else
				yaw = 270;
			if (yaw < 0)
				yaw += 360;
	
			forward = sqrt (dist[0]*dist[0] + dist[1]*dist[1]);
			pitch = (atan2 (dist[2], forward) * -M_180DIV_PI);
			if (pitch < 0)
				pitch += 360.0;
		}

		// add new entities for the beams
		d = VectorNormalize (dist, dist);

		memset (&ent, 0, sizeof (ent));
		if (b->model == clMedia.lightningMOD) {
			model_length = 35.0;
			d-= 20.0;  // correction so it doesn't end in middle of tesla
		} else {
			model_length = 30.0;
		}
		steps = ceil(d/model_length);
		len = (d-model_length)/(steps-1);

		// PMM - special case for lightning model .. if the real length is shorter than the model,
		// flip it around & draw it from the end to the start.  This prevents the model from going
		// through the tesla mine (instead it goes through the target)
		if ((b->model == clMedia.lightningMOD) && (d <= model_length)) {
			VectorCopy (b->end, ent.origin);
			// offset to push beam outside of tesla model (negative because dist is from end to start

			ent.model = b->model;
			ent.flags = RF_FULLBRIGHT;
			ent.scale = 1;
			VectorSet (angles, pitch, yaw, frand () * 360);

			if (angles[0] || angles[1] || angles[2])
				AnglesToAxis (angles, ent.axis);
			else
				Matrix_Identity (ent.axis);
			Vector4Set (ent.color, 1, 1, 1, 1);
			R_AddEntity (&ent);
			return;
		}

		while (d > 0) {
			VectorCopy (org, ent.origin);
			ent.model = b->model;
			if (b->model == clMedia.lightningMOD) {
				ent.flags = RF_FULLBRIGHT;
				VectorSet (angles, -pitch, yaw + 180.0, frand () * 360);
			} else
				VectorSet (angles, pitch, yaw, frand () * 360);

			ent.scale = 1;
			if (angles[0] || angles[1] || angles[2])
				AnglesToAxis (angles, ent.axis);
			else
				Matrix_Identity (ent.axis);
			Vector4Set (ent.color, 1, 1, 1, 1);
			R_AddEntity (&ent);

			for (j=0 ; j<3 ; j++)
				org[j] += dist[j]*len;
			d -= model_length;
		}
	}
}


/*
=================
CL_AddPlayerBeams
=================
*/
void CL_AddPlayerBeams (void) {
	beam_t		*b;
	vec3_t		dist, org, angles;
	int			i, j, framenum;
	float		d, yaw, pitch, forward, len, steps;
	entity_t	ent;
	
	float			model_length, hand_multiplier;
	frame_t			*oldframe;
	playerState_t	*ps, *ops;

	//PMM
	if (hand) {
		if (hand->integer == 2)
			hand_multiplier = 0;
		else if (hand->integer == 1)
			hand_multiplier = -1;
		else
			hand_multiplier = 1;
	} else
		hand_multiplier = 1;
	//PMM

	// update beams
	for (i=0, b=cl_playerbeams ; i< MAX_BEAMS ; i++, b++) {
		vec3_t		fwd, right, up;
		if (!b->model || (b->endtime < cls.realTime))
			continue;

		if (clMedia.heatBeamMOD && (b->model == clMedia.heatBeamMOD)) {
			// if coming from the player, update the start position
			// entity 0 is the world
			if (b->entity == cl.playerNum+1) {	
				// set up gun position
				// code straight out of CL_AddViewWeapon
				ps = &cl.frame.playerState;
				j = (cl.frame.serverFrame - 1) & UPDATE_MASK;
				oldframe = &cl.frames[j];
				if (oldframe->serverFrame != cl.frame.serverFrame-1 || !oldframe->valid)
					oldframe = &cl.frame;		// previous frame was dropped or involid
				ops = &oldframe->playerState;
				for (j=0 ; j<3 ; j++) {
					b->start[j] = cl.refDef.viewOrigin[j] + ops->gunOffset[j]
						+ cl.lerpFrac * (ps->gunOffset[j] - ops->gunOffset[j]);
				}
				VectorMA (b->start,	(hand_multiplier * b->offset[0]),	cl.v_Right,		org);
				VectorMA (org,		b->offset[1],						cl.v_Forward,	org);
				VectorMA (org,		b->offset[2],						cl.v_Up,		org);

				if (hand->integer == 2)
					VectorMA (org, -1, cl.v_Up, org);

				// FIXME - take these out when final
				VectorCopy (cl.v_Right, right);
				VectorCopy (cl.v_Forward, fwd);
				VectorCopy (cl.v_Up, up);

			} else
				VectorCopy (b->start, org);
		} else {
			// if coming from the player, update the start position
			// entity 0 is the world
			if (b->entity == cl.playerNum+1) {
				VectorCopy (cl.refDef.viewOrigin, b->start);
				b->start[2] -= 22;	// adjust for view height
			}
			VectorAdd (b->start, b->offset, org);
		}

		// calculate pitch and yaw
		VectorSubtract (b->end, org, dist);

		//PMM
		if (clMedia.heatBeamMOD && (b->model == clMedia.heatBeamMOD) && (b->entity == cl.playerNum+1)) {
			vec_t len;

			len = VectorLength (dist);
			VectorScale (fwd, len, dist);
			VectorMA (dist, (hand_multiplier * b->offset[0]), right, dist);
			VectorMA (dist, b->offset[1], fwd, dist);
			VectorMA (dist, b->offset[2], up, dist);

			if (hand && (hand->integer == 2))
				VectorMA (org, -1, cl.v_Up, org);
		}
		//PMM

		if ((dist[1] == 0) && (dist[0] == 0)) {
			yaw = 0;
			if (dist[2] > 0)
				pitch = 90;
			else
				pitch = 270;
		} else {
			// PMM - fixed to correct for pitch of 0
			if (dist[0])
				yaw = (atan2 (dist[1], dist[0]) * M_180DIV_PI);
			else if (dist[1] > 0)
				yaw = 90;
			else
				yaw = 270;
			if (yaw < 0)
				yaw += 360;
	
			forward = sqrt (dist[0]*dist[0] + dist[1]*dist[1]);
			pitch = (atan2 (dist[2], forward) * -M_180DIV_PI);
			if (pitch < 0)
				pitch += 360.0;
		}

		framenum = 1;
		if (clMedia.heatBeamMOD && (b->model == clMedia.heatBeamMOD)) {
			if (b->entity != cl.playerNum+1) {
				framenum = 2;

				VectorSet (angles, -pitch, yaw + 180.0, frand () * 360);
				AngleVectors (angles, fwd, right, up);
					
				// if it's a non-origin offset, it's a player, so use the hardcoded player offset
				if (!VectorCompare (b->offset, vec3Identity)) {
					VectorMA (org, -(b->offset[0])+1, right, org);
					VectorMA (org, -(b->offset[1]), fwd, org);
					VectorMA (org, -(b->offset[2])-10, up, org);
				} else {
					// if it's a monster, do the particle effect
					CL_MonsterPlasma_Shell (b->start);
				}
			} else {
				framenum = 1;
			}
		}

		// if it's the heatBeamMod, draw the particle effect
		if ((clMedia.heatBeamMOD && (b->model == clMedia.heatBeamMOD) && (b->entity == cl.playerNum+1)))
			CL_Heatbeam (org, dist);

		// add new entities for the beams
		d = VectorNormalize (dist, dist);

		memset (&ent, 0, sizeof (ent));
		if (b->model == clMedia.heatBeamMOD)
			model_length = 32.0;
		else if (b->model == clMedia.lightningMOD) {
			model_length = 35.0;
			d-= 20.0;  // correction so it doesn't end in middle of tesla
		} else
			model_length = 30.0;

		steps = ceil(d/model_length);
		len = (d-model_length)/(steps-1);

		// PMM - special case for lightning model .. if the real length is shorter than the model,
		// flip it around & draw it from the end to the start.  This prevents the model from going
		// through the tesla mine (instead it goes through the target)
		if ((b->model == clMedia.lightningMOD) && (d <= model_length)) {
			VectorCopy (b->end, ent.origin);
			// offset to push beam outside of tesla model (negative because dist is from end to start
			// for this beam)
			ent.model = b->model;
			ent.flags = RF_FULLBRIGHT;
			ent.scale = 1;
			VectorSet (angles, pitch, yaw, frand () * 360);
			if (angles[0] || angles[1] || angles[2])
				AnglesToAxis (angles, ent.axis);
			else
				Matrix_Identity (ent.axis);
			Vector4Set (ent.color, 1, 1, 1, 1);
			R_AddEntity (&ent);			
			return;
		}

		while (d > 0) {
			VectorCopy (org, ent.origin);
			ent.model = b->model;
			if (clMedia.heatBeamMOD && (b->model == clMedia.heatBeamMOD)) {
				ent.flags = RF_FULLBRIGHT;
				VectorSet (angles, -pitch, yaw + 180.0, frand () * 360);
				ent.frame = framenum;
			} else if (b->model == clMedia.lightningMOD) {
				ent.flags = RF_FULLBRIGHT;
				VectorSet (angles, -pitch, yaw + 180.0, frand () * 360);
			} else
				VectorSet (angles, pitch, yaw, frand () * 360);

			ent.scale = 1;
			if (angles[0] || angles[1] || angles[2])
				AnglesToAxis (angles, ent.axis);
			else
				Matrix_Identity (ent.axis);
			Vector4Set (ent.color, 1, 1, 1, 1);
			R_AddEntity (&ent);

			for (j=0 ; j<3 ; j++)
				org[j] += dist[j]*len;
			d -= model_length;
		}
	}
}


/*
=================
CL_ParseLightning
=================
*/
int CL_ParseLightning (struct model_s *model) {
	int		srcEnt, destEnt;
	vec3_t	start, end;
	beam_t	*b;
	int		i;
	
	srcEnt = MSG_ReadShort (&net_Message);
	destEnt = MSG_ReadShort (&net_Message);

	MSG_ReadPos (&net_Message, start);
	MSG_ReadPos (&net_Message, end);

	// override any beam with the same source AND destination entities
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++) {
		if (b->entity == srcEnt && b->dest_entity == destEnt) {
			b->entity = srcEnt;
			b->dest_entity = destEnt;
			b->model = model;
			b->endtime = cls.realTime + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorIdentity (b->offset);
			return srcEnt;
		}
	}

	// find a free beam
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++) {
		if (!b->model || (b->endtime < cls.realTime)) {
			b->entity = srcEnt;
			b->dest_entity = destEnt;
			b->model = model;
			b->endtime = cls.realTime + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorIdentity (b->offset);
			return srcEnt;
		}
	}

	Com_Printf (PRINT_ALL, S_COLOR_YELLOW "beam list overflow!\n");	
	return srcEnt;
}

/*
==============================================================

	LASER MANAGEMENT

==============================================================
*/

#define	MAX_LASERS	32
typedef struct {
	entity_t	ent;
	int			endtime;
} laser_t;
laser_t		cl_lasers[MAX_LASERS];

/*
=================
CL_AddLasers
=================
*/
void CL_AddLasers (void) {
	laser_t		*l;
	int			i, j, clr;
	vec3_t		length;

	for (i=0, l=cl_lasers ; i< MAX_LASERS ; i++, l++) {
		if (l->endtime >= cls.realTime) {
			VectorSubtract(l->ent.oldOrigin, l->ent.origin, length);

			clr = ((l->ent.skinNum >> ((rand() % 4)*8)) & 0xff);

			for (j=0 ; j<3 ; j++)
				CL_BeamTrail (l->ent.origin, l->ent.oldOrigin,
					clr, l->ent.frame, 0.33 + (rand()%2 * 0.1), -2);

			// outer
			makePart (
				l->ent.origin[0],				l->ent.origin[1],				l->ent.origin[2],
				length[0],						length[1],						length[2],
				0,								0,								0,
				0,								0,								0,
				palRed (clr),					palGreen (clr),					palBlue (clr),
				palRed (clr),					palGreen (clr),					palBlue (clr),
				0.30,							PART_INSTANT,
				l->ent.frame*0.9 + ((l->ent.frame*0.05)*(rand()%2)),
				l->ent.frame*0.9 + ((l->ent.frame*0.05)*(rand()%2)),
				PT_BEAM,						PF_BEAM,
				GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
				0,								qFalse,
				0);

			// core
			makePart (
				l->ent.origin[0],				l->ent.origin[1],				l->ent.origin[2],
				length[0],						length[1],						length[2],
				0,								0,								0,
				0,								0,								0,
				palRed (clr),					palGreen (clr),					palBlue (clr),
				palRed (clr),					palGreen (clr),					palBlue (clr),
				0.30,							PART_INSTANT,
				l->ent.frame*0.3 + ((l->ent.frame*0.05)*(rand()%2)),
				l->ent.frame*0.3 + ((l->ent.frame*0.05)*(rand()%2)),
				PT_BEAM,						PF_BEAM,
				GL_SRC_ALPHA,					GL_ONE,
				0,								qFalse,
				0);
		}
	}
}


/*
=================
CL_ParseLaser
=================
*/
void CL_ParseLaser (int colors) {
	vec3_t	start, end;
	laser_t	*l;
	int		i, j, clr;
	vec3_t	length;

	MSG_ReadPos (&net_Message, start);
	MSG_ReadPos (&net_Message, end);

	for (i=0, l=cl_lasers ; i<MAX_LASERS ; i++, l++) {
		if (l->endtime < cls.realTime) {
			VectorSubtract(end, start, length);

			clr = ((colors >> ((rand() % 4)*8)) & 0xff);

			for (j=0 ; j<3 ; j++)
				CL_BeamTrail (start, end, clr, 2 + (rand()%2), 0.30, -2);

			// outer
			makePart (
				start[0],							start[1],							start[2],
				length[0],							length[1],							length[2],
				0,									0,									0,
				0,									0,									0,
				palRed (clr),						palGreen (clr),						palBlue (clr),
				palRed (clr),						palGreen (clr),						palBlue (clr),
				0.30,								-2.1,
				(4*0.9) + ((4*0.05)*(rand()%2)),
				(4*0.9) + ((4*0.05)*(rand()%2)),
				PT_BEAM,							PF_BEAM,
				GL_SRC_ALPHA,						GL_ONE_MINUS_SRC_ALPHA,
				0,									qFalse,
				0);

			// core
			makePart (
				start[0],							start[1],							start[2],
				length[0],							length[1],							length[2],
				0,									0,									0,
				0,									0,									0,
				palRed (clr),						palGreen (clr),						palBlue (clr),
				palRed (clr),						palGreen (clr),						palBlue (clr),
				0.30,								-2.1,
				(4*0.9)/3 + ((4*0.05)*(rand()%2)),	(4*0.9)/3 + ((4*0.05)*(rand()%2)),
				PT_BEAM,							PF_BEAM,
				GL_SRC_ALPHA,						GL_ONE,
				0,									qFalse,
				0);
			return;
		}
	}
}	

/*
==============================================================

	TENT MANAGEMENT

==============================================================
*/

/*
=================
CL_AddTEnts
=================
*/
void CL_AddTEnts (void) {
	CL_AddBeams ();
	CL_AddPlayerBeams ();	// PMM - draw plasma beams
	CL_AddLasers ();

	CL_AddExploRattles ();
}


/*
=================
CL_ClearTEnts
=================
*/
void CL_ClearTEnts (void) {
	memset (cl_beams, 0, sizeof (cl_beams));
	memset (cl_lasers, 0, sizeof (cl_lasers));
	memset (cl_playerbeams, 0, sizeof (cl_playerbeams));

	CL_ClearExploRattles ();
}


/*
=================
CL_ParseTEnt
=================
*/
void CL_ParseTEnt (void) {
	int		type, cnt, color, r, ent, magnitude;
	vec3_t	pos, pos2, dir;
	cDLight_t	*dl;

	type = MSG_ReadByte (&net_Message);

	switch (type) {
	case TE_BLOOD:			// bullet hitting flesh
		MSG_ReadPos (&net_Message, pos);
		MSG_ReadDir (&net_Message, dir);
		CL_BleedEffect (pos, dir, 10);
		break;

	case TE_GUNSHOT:			// bullet hitting wall
	case TE_SPARKS:
	case TE_BULLET_SPARKS:
		MSG_ReadPos (&net_Message, pos);
		MSG_ReadDir (&net_Message, dir);

		if (type == TE_GUNSHOT)
			CL_RicochetEffect (pos, dir, 20);
		else
			CL_ParticleEffect (pos, dir, 0xe0, 9);

		if (type != TE_SPARKS) {
			// impact sound
			cnt = rand()&15;
			if (cnt == 1)
				Snd_StartSound (pos, 0, 0, clMedia.ricochet1SFX, 1, ATTN_NORM, 0);
			else if (cnt == 2)
				Snd_StartSound (pos, 0, 0, clMedia.ricochet2SFX, 1, ATTN_NORM, 0);
			else if (cnt == 3)
				Snd_StartSound (pos, 0, 0, clMedia.ricochet3SFX, 1, ATTN_NORM, 0);
		}

		break;
		
	case TE_SCREEN_SPARKS:
	case TE_SHIELD_SPARKS:
		MSG_ReadPos (&net_Message, pos);
		MSG_ReadDir (&net_Message, dir);
		if (type == TE_SCREEN_SPARKS)
			CL_ParticleEffect (pos, dir, 0xd0, 40);
		else
			CL_ParticleEffect (pos, dir, 0xb0, 40);

		//FIXME : replace or remove this sound
		Snd_StartSound (pos, 0, 0, clMedia.laserHitSFX, 1, ATTN_NORM, 0);
		break;
		
	case TE_SHOTGUN:			// bullet hitting wall
		MSG_ReadPos (&net_Message, pos);
		MSG_ReadDir (&net_Message, dir);
		CL_RicochetEffect (pos, dir, 20);
		break;

	case TE_SPLASH:				// bullet hitting water
		cnt = MSG_ReadByte (&net_Message);
		MSG_ReadPos (&net_Message, pos);
		MSG_ReadDir (&net_Message, dir);
		r = MSG_ReadByte (&net_Message);
		if (r > 6)
			color = 0;
		else
			color = r;

		CL_SplashEffect (pos, dir, color, cnt);

		if (r == SPLASH_SPARKS) {
			r = (rand()%3);
			if (r == 0)
				Snd_StartSound (pos, 0, 0, clMedia.spark5SFX, 1, ATTN_STATIC, 0);
			else if (r == 1)
				Snd_StartSound (pos, 0, 0, clMedia.spark6SFX, 1, ATTN_STATIC, 0);
			else
				Snd_StartSound (pos, 0, 0, clMedia.spark7SFX, 1, ATTN_STATIC, 0);
		}
		break;

	case TE_LASER_SPARKS:
		cnt = MSG_ReadByte (&net_Message);
		MSG_ReadPos (&net_Message, pos);
		MSG_ReadDir (&net_Message, dir);
		color = MSG_ReadByte (&net_Message);
		CL_ParticleEffect2 (pos, dir, color, cnt);
		break;

	// RAFAEL
	case TE_BLUEHYPERBLASTER:
		MSG_ReadPos (&net_Message, pos);
		MSG_ReadPos (&net_Message, dir);
		CL_BlasterBlueParticles (pos, dir);
		break;

	case TE_BLASTER:			// blaster hitting wall
		MSG_ReadPos (&net_Message, pos);
		MSG_ReadDir (&net_Message, dir);
		CL_BlasterGoldParticles (pos, dir);
		Snd_StartSound (pos,  0, 0, clMedia.laserHitSFX, 1, ATTN_NORM, 0);
		break;
		
	case TE_RAILTRAIL:			// railgun effect
		MSG_ReadPos (&net_Message, pos);
		MSG_ReadPos (&net_Message, pos2);

		CL_RailTrail (pos, pos2);
		Snd_StartSound (pos, 0, 0, clMedia.railgunFireSFX, 1, ATTN_NORM, 0);
		Snd_StartSound (pos2, 0, 0, clMedia.railgunFireSFX, 1, ATTN_NORM, 0);
		break;

	case TE_EXPLOSION2:
	case TE_GRENADE_EXPLOSION:
	case TE_GRENADE_EXPLOSION_WATER:
		MSG_ReadPos (&net_Message, pos);

		if (type == TE_GRENADE_EXPLOSION_WATER) {
			Snd_StartSound (pos, 0, 0, clMedia.waterExploSFX, 1, ATTN_NORM, 0);
			CL_ExplosionParticles (pos, 1, qFalse, qTrue);
		} else {
			Snd_StartSound (pos, 0, 0, clMedia.grenadeExploSFX, 1, ATTN_NORM, 0);
			CL_ExplosionParticles (pos, 1, qFalse, qFalse);
		}
		break;

	// RAFAEL
	case TE_PLASMA_EXPLOSION:
		MSG_ReadPos (&net_Message, pos);
		CL_ExplosionParticles (pos, 1, qFalse, qFalse);
		Snd_StartSound (pos, 0, 0, clMedia.rocketExploSFX, 1, ATTN_NORM, 0);
		break;
	
	case TE_EXPLOSION1:
	case TE_EXPLOSION1_BIG:						// PMM
	case TE_ROCKET_EXPLOSION:
	case TE_ROCKET_EXPLOSION_WATER:
	case TE_EXPLOSION1_NP:						// PMM
		MSG_ReadPos (&net_Message, pos);

		if ((type != TE_EXPLOSION1_BIG) && (type != TE_EXPLOSION1_NP))		// PMM
			CL_ExplosionParticles (pos, 1, qFalse, !(type != TE_ROCKET_EXPLOSION_WATER));

		if (type == TE_ROCKET_EXPLOSION_WATER)
			Snd_StartSound (pos, 0, 0, clMedia.waterExploSFX, 1, ATTN_NORM, 0);
		else
			Snd_StartSound (pos, 0, 0, clMedia.rocketExploSFX, 1, ATTN_NORM, 0);
		break;

	case TE_BFG_EXPLOSION:
		MSG_ReadPos (&net_Message, pos);
		CL_ExplosionBFGEffect (pos);
		break;

	case TE_BFG_BIGEXPLOSION:
		MSG_ReadPos (&net_Message, pos);
		CL_ExplosionBFGParticles (pos);
		break;

	case TE_BFG_LASER:
		CL_ParseLaser (0xd0d1d2d3);
		break;

	case TE_BUBBLETRAIL:
		MSG_ReadPos (&net_Message, pos);
		MSG_ReadPos (&net_Message, pos2);
		CL_BubbleTrail (pos, pos2);
		break;

	case TE_PARASITE_ATTACK:
	case TE_MEDIC_CABLE_ATTACK:
		ent = CL_ParseBeam (clMedia.parasiteSegmentMOD);
		break;

	case TE_BOSSTPORT:			// boss teleporting to station
		MSG_ReadPos (&net_Message, pos);
		CL_BigTeleportParticles (pos);
		Snd_StartSound (pos, 0, 0, Snd_RegisterSound ("misc/bigtele.wav"), 1, ATTN_NONE, 0);
		break;

	case TE_GRAPPLE_CABLE:
		ent = CL_ParseBeam2 (clMedia.grappleCableMOD);
		break;

	// RAFAEL
	case TE_WELDING_SPARKS:
		cnt = MSG_ReadByte (&net_Message);
		MSG_ReadPos (&net_Message, pos);
		MSG_ReadDir (&net_Message, dir);
		color = MSG_ReadByte (&net_Message);
		CL_ParticleEffect2 (pos, dir, color, cnt);

		dl = CL_AllocDLight ((int)(VectorAverage (pos)));

		VectorCopy (pos, dl->origin);
		VectorSet (dl->color, 1, 1, 0.3);
		dl->decay = 10;
		dl->die = cls.realTime + 100;
		dl->minlight = 100;
		dl->radius = 175;
		break;

	case TE_GREENBLOOD:
		MSG_ReadPos (&net_Message, pos);
		MSG_ReadDir (&net_Message, dir);
		CL_BleedGreenEffect (pos, dir, 10);
		break;

	// RAFAEL
	case TE_TUNNEL_SPARKS:
		cnt = MSG_ReadByte (&net_Message);
		MSG_ReadPos (&net_Message, pos);
		MSG_ReadDir (&net_Message, dir);
		color = MSG_ReadByte (&net_Message);
		CL_ParticleEffect3 (pos, dir, color, cnt);
		break;

	//=============
	//PGM
		// PMM -following code integrated for flechette (different color)
	case TE_BLASTER2:			// green blaster hitting wall
	case TE_FLECHETTE:			// flechette
		MSG_ReadPos (&net_Message, pos);
		MSG_ReadDir (&net_Message, dir);
		
		// PMM
		if (type == TE_BLASTER2)
			CL_BlasterGreenParticles (pos, dir);
		else
			CL_BlasterGreyParticles (pos, dir);

		Snd_StartSound (pos,  0, 0, clMedia.laserHitSFX, 1, ATTN_NORM, 0);
		break;


	case TE_LIGHTNING:
		ent = CL_ParseLightning (clMedia.lightningMOD);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, clMedia.lightningSFX, 1, ATTN_NORM, 0);
		break;

	case TE_DEBUGTRAIL:
		MSG_ReadPos (&net_Message, pos);
		MSG_ReadPos (&net_Message, pos2);
		CL_DebugTrail (pos, pos2);
		break;

	case TE_PLAIN_EXPLOSION:
		MSG_ReadPos (&net_Message, pos);

		if (type == TE_ROCKET_EXPLOSION_WATER) {
			Snd_StartSound (pos, 0, 0, clMedia.waterExploSFX, 1, ATTN_NORM, 0);
			CL_ExplosionParticles (pos, 1, qFalse, qTrue);
		} else {
			Snd_StartSound (pos, 0, 0, clMedia.rocketExploSFX, 1, ATTN_NORM, 0);
			CL_ExplosionParticles (pos, 1, qFalse, qFalse);
		}
		break;

	case TE_FLASHLIGHT:
		MSG_ReadPos (&net_Message, pos);
		ent = MSG_ReadShort (&net_Message);
		CL_Flashlight (ent, pos);
		break;

	case TE_FORCEWALL:
		MSG_ReadPos(&net_Message, pos);
		MSG_ReadPos(&net_Message, pos2);
		color = MSG_ReadByte (&net_Message);
		CL_ForceWall(pos, pos2, color);
		break;

	case TE_HEATBEAM:
		ent = CL_ParsePlayerBeam (clMedia.heatBeamMOD);
		break;

	case TE_MONSTER_HEATBEAM:
		ent = CL_ParsePlayerBeam (clMedia.monsterHeatBeamMOD);
		break;

	case TE_HEATBEAM_SPARKS:
		cnt = 50;
		MSG_ReadPos (&net_Message, pos);
		MSG_ReadDir (&net_Message, dir);
		r = 8;
		magnitude = 60;
		color = r & 0xff;
		CL_ParticleSteamEffect (pos, dir, color, cnt, magnitude);
		Snd_StartSound (pos,  0, 0, clMedia.laserHitSFX, 1, ATTN_NORM, 0);
		break;
	
	case TE_HEATBEAM_STEAM:
		cnt = 20;
		MSG_ReadPos (&net_Message, pos);
		MSG_ReadDir (&net_Message, dir);
		color = 0xe0;
		magnitude = 60;
		CL_ParticleSteamEffect (pos, dir, color, cnt, magnitude);
		Snd_StartSound (pos,  0, 0, clMedia.laserHitSFX, 1, ATTN_NORM, 0);
		break;

	case TE_STEAM:
		CL_ParseSteam();
		break;

	case TE_BUBBLETRAIL2:
		cnt = 8;
		MSG_ReadPos (&net_Message, pos);
		MSG_ReadPos (&net_Message, pos2);
		CL_BubbleTrail2 (pos, pos2, cnt);
		Snd_StartSound (pos,  0, 0, clMedia.laserHitSFX, 1, ATTN_NORM, 0);
		break;

	case TE_MOREBLOOD:
		MSG_ReadPos (&net_Message, pos);
		MSG_ReadDir (&net_Message, dir);
		CL_BleedEffect (pos, dir, 50);
		break;

	case TE_CHAINFIST_SMOKE:
		dir[0] = 0; dir[1] = 0; dir[2] = 1;
		MSG_ReadPos (&net_Message, pos);
		CL_ParticleSmokeEffect (pos, dir, 0, 20, 20);
		break;

	case TE_ELECTRIC_SPARKS:
		MSG_ReadPos (&net_Message, pos);
		MSG_ReadDir (&net_Message, dir);
		CL_ParticleEffect (pos, dir, 0x75, 40);
		//FIXME : replace or remove this sound
		Snd_StartSound (pos, 0, 0, clMedia.laserHitSFX, 1, ATTN_NORM, 0);
		break;

	case TE_TRACKER_EXPLOSION:
		MSG_ReadPos (&net_Message, pos);
		CL_ColorFlash (pos, 0, 150, -1, -1, -1);
		CL_ExplosionColorParticles (pos);
		Snd_StartSound (pos, 0, 0, clMedia.disruptExploSFX, 1, ATTN_NORM, 0);
		break;

	case TE_TELEPORT_EFFECT:
	case TE_DBALL_GOAL:
		MSG_ReadPos (&net_Message, pos);
		CL_TeleportParticles (pos);
		break;

	case TE_WIDOWBEAMOUT:
		CL_ParseWidow ();
		break;

	case TE_NUKEBLAST:
		CL_ParseNuke ();
		break;

	case TE_WIDOWSPLASH:
		MSG_ReadPos (&net_Message, pos);
		CL_WidowSplash (pos);
		break;
	//PGM
	//==============

	default:
		Com_Error (ERR_DROP, "CL_ParseTEnt: bad type");
	}
}
