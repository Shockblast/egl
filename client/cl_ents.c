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

// cl_ents.c
// - entity parsing and management

#include "cl_local.h"

/*
=========================================================================

	FRAME PARSING

=========================================================================
*/

/*
=================
CL_ParseEntityBits

Returns the entity number and the header bits
=================
*/
int	bitcounts[32];	/// just for protocol profiling
int CL_ParseEntityBits (unsigned *bits) {
	unsigned	b, total;
	int			i;
	int			number;

	total = MSG_ReadByte (&net_Message);
	if (total & U_MOREBITS1) {
		b = MSG_ReadByte (&net_Message);
		total |= b<<8;
	}
	if (total & U_MOREBITS2) {
		b = MSG_ReadByte (&net_Message);
		total |= b<<16;
	}
	if (total & U_MOREBITS3) {
		b = MSG_ReadByte (&net_Message);
		total |= b<<24;
	}

	// count the bits for net profiling
	for (i=0 ; i<32 ; i++)
		if (total&(1<<i))
			bitcounts[i]++;

	if (total & U_NUMBER16)
		number = MSG_ReadShort (&net_Message);
	else
		number = MSG_ReadByte (&net_Message);

	*bits = total;

	return number;
}


/*
==================
CL_ParseDelta

Can go from either a baseline or a previous packet_entity
==================
*/
void CL_ParseDelta (entityState_t *from, entityState_t *to, int number, int bits) {
	// set everything to the state we are delta'ing from
	*to = *from;

	VectorCopy (from->origin, to->oldOrigin);
	to->number = number;

	if (bits & U_MODEL)		to->modelIndex = MSG_ReadByte (&net_Message);
	if (bits & U_MODEL2)	to->modelIndex2 = MSG_ReadByte (&net_Message);
	if (bits & U_MODEL3)	to->modelIndex3 = MSG_ReadByte (&net_Message);
	if (bits & U_MODEL4)	to->modelIndex4 = MSG_ReadByte (&net_Message);
		
	if (bits & U_FRAME8)	to->frame = MSG_ReadByte (&net_Message);
	if (bits & U_FRAME16)	to->frame = MSG_ReadShort (&net_Message);

	if ((bits & U_SKIN8) && (bits & U_SKIN16))	to->skinNum = MSG_ReadLong (&net_Message); // used for laser colors
	else if (bits & U_SKIN8)	to->skinNum = MSG_ReadByte (&net_Message);
	else if (bits & U_SKIN16)	to->skinNum = MSG_ReadShort (&net_Message);

	if ((bits & (U_EFFECTS8|U_EFFECTS16)) == (U_EFFECTS8|U_EFFECTS16))	to->effects = MSG_ReadLong (&net_Message);
	else if (bits & U_EFFECTS8)		to->effects = MSG_ReadByte (&net_Message);
	else if (bits & U_EFFECTS16)	to->effects = MSG_ReadShort (&net_Message);

	if ((bits & (U_RENDERFX8|U_RENDERFX16)) == (U_RENDERFX8|U_RENDERFX16))	to->renderFx = MSG_ReadLong (&net_Message);
	else if (bits & U_RENDERFX8)	to->renderFx = MSG_ReadByte (&net_Message);
	else if (bits & U_RENDERFX16)	to->renderFx = MSG_ReadShort (&net_Message);

	if (bits & U_ORIGIN1)		to->origin[0] = MSG_ReadCoord (&net_Message);
	if (bits & U_ORIGIN2)		to->origin[1] = MSG_ReadCoord (&net_Message);
	if (bits & U_ORIGIN3)		to->origin[2] = MSG_ReadCoord (&net_Message);
		
	if (bits & U_ANGLE1)		to->angles[0] = MSG_ReadAngle (&net_Message);
	if (bits & U_ANGLE2)		to->angles[1] = MSG_ReadAngle (&net_Message);
	if (bits & U_ANGLE3)		to->angles[2] = MSG_ReadAngle (&net_Message);

	if (bits & U_OLDORIGIN)		MSG_ReadPos (&net_Message, to->oldOrigin);

	if (bits & U_SOUND)	to->sound = MSG_ReadByte (&net_Message);

	if (bits & U_EVENT)	to->event = MSG_ReadByte (&net_Message);
	else				to->event = 0;

	if (bits & U_SOLID)	to->solid = MSG_ReadShort (&net_Message);
}


/*
==================
CL_DeltaEntity

Parses deltas from the given base and adds the resulting entity
to the current frame
==================
*/
void CL_DeltaEntity (frame_t *frame, int newnum, entityState_t *old, int bits) {
	centity_t		*ent;
	entityState_t	*state;

	ent = &cl_Entities[newnum];

	state = &cl_ParseEntities[cl.parseEntities & (MAX_PARSE_ENTITIES-1)];
	cl.parseEntities++;
	frame->numEntities++;

	CL_ParseDelta (old, state, newnum, bits);

	// some data changes will force no lerping
	if ((state->modelIndex != ent->current.modelIndex)			||
		(state->modelIndex2 != ent->current.modelIndex2)		||
		(state->modelIndex3 != ent->current.modelIndex3)		||
		(state->modelIndex4 != ent->current.modelIndex4)		||
		(abs (state->origin[0] - ent->current.origin[0]) > 512)	||
		(abs (state->origin[1] - ent->current.origin[1]) > 512)	||
		(abs (state->origin[2] - ent->current.origin[2]) > 512)	||
		(state->event == EV_PLAYER_TELEPORT)					||
		(state->event == EV_OTHER_TELEPORT))
			ent->serverFrame = -99;

	if (ent->serverFrame != cl.frame.serverFrame - 1) {
		// wasn't in last update, so initialize some things duplicate
		// the current state so lerping doesn't hurt anything
		ent->prev = *state;
		if (state->event == EV_OTHER_TELEPORT) {
			VectorCopy (state->origin, ent->prev.origin);
			VectorCopy (state->origin, ent->lerpOrigin);
		} else {
			VectorCopy (state->oldOrigin, ent->prev.origin);
			VectorCopy (state->oldOrigin, ent->lerpOrigin);
		}
	} else {
		// shuffle the last state to previous
		ent->prev = ent->current;
	}

	ent->serverFrame = cl.frame.serverFrame;
	ent->current = *state;
}


/*
==================
CL_ParsePacketEntities

An SVC_PACKETENTITIES has just been parsed, deal with the
rest of the data stream.
==================
*/
void CL_ParsePacketEntities (frame_t *oldFrame, frame_t *newFrame) {
	int				newnum;
	unsigned int	bits;
	entityState_t	*oldstate;
	int				oldindex, oldnum = 99999;

	newFrame->parseEntities = cl.parseEntities;
	newFrame->numEntities = 0;

	// delta from the entities present in oldFrame
	oldindex = 0;
	if (oldFrame) {
		if (oldindex < oldFrame->numEntities) {
			oldstate = &cl_ParseEntities[(oldFrame->parseEntities+oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}
	}

	for ( ; ; ) {
		newnum = CL_ParseEntityBits (&bits);
		if (newnum >= MAX_EDICTS)
			Com_Error (ERR_DROP,"CL_ParsePacketEntities: bad number:%i", newnum);

		if (net_Message.readCount > net_Message.curSize)
			Com_Error (ERR_DROP,"CL_ParsePacketEntities: end of message");

		if (!newnum)
			break;

		while (oldnum < newnum) {
			// one or more entities from the old packet are unchanged
			if (cl_shownet->value == 3)
				Com_Printf (PRINT_ALL, "   unchanged: %i\n", oldnum);
			CL_DeltaEntity (newFrame, oldnum, oldstate, 0);
			
			oldindex++;

			if (oldindex >= oldFrame->numEntities)
				oldnum = 99999;
			else {
				oldstate = &cl_ParseEntities[(oldFrame->parseEntities+oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
		}

		if (bits & U_REMOVE) {
			// the entity present in oldFrame is not in the current frame
			if (cl_shownet->value == 3)
				Com_Printf (PRINT_ALL, "   remove: %i\n", newnum);
			if (oldnum != newnum)
				Com_Printf (PRINT_DEV, S_COLOR_YELLOW "U_REMOVE: oldnum != newnum\n");

			oldindex++;

			if (oldindex >= oldFrame->numEntities)
				oldnum = 99999;
			else {
				oldstate = &cl_ParseEntities[(oldFrame->parseEntities+oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if (oldnum == newnum) {
			// delta from previous state
			if (cl_shownet->value == 3)
				Com_Printf (PRINT_ALL, "   delta: %i\n", newnum);
			CL_DeltaEntity (newFrame, newnum, oldstate, bits);

			oldindex++;

			if (oldindex >= oldFrame->numEntities)
				oldnum = 99999;
			else {
				oldstate = &cl_ParseEntities[(oldFrame->parseEntities+oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if (oldnum > newnum) {
			// delta from baseline
			if (cl_shownet->value == 3)
				Com_Printf (PRINT_ALL, "   baseline: %i\n", newnum);
			CL_DeltaEntity (newFrame, newnum, &cl_Entities[newnum].baseLine, bits);
			continue;
		}

	}

	// any remaining entities in the old frame are copied over
	while (oldnum != 99999) {
		// one or more entities from the old packet are unchanged
		if (cl_shownet->value == 3)
			Com_Printf (PRINT_ALL, "   unchanged: %i\n", oldnum);
		CL_DeltaEntity (newFrame, oldnum, oldstate, 0);
		
		oldindex++;

		if (oldindex >= oldFrame->numEntities)
			oldnum = 99999;
		else {
			oldstate = &cl_ParseEntities[(oldFrame->parseEntities+oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}
	}
}


/*
===================
CL_ParsePlayerstate
===================
*/
void CL_ParsePlayerstate (frame_t *oldFrame, frame_t *newFrame) {
	int				flags;
	playerState_t	*state;
	int				i, statbits;

	state = &newFrame->playerState;

	// clear to old value before delta parsing
	if (oldFrame)
		*state = oldFrame->playerState;
	else
		memset (state, 0, sizeof (*state));

	flags = MSG_ReadShort (&net_Message);

	//
	// parse the pMoveState_t
	//
	if (flags & PS_M_TYPE)
		state->pMove.pmType = MSG_ReadByte (&net_Message);

	if (flags & PS_M_ORIGIN) {
		state->pMove.origin[0] = MSG_ReadShort (&net_Message);
		state->pMove.origin[1] = MSG_ReadShort (&net_Message);
		state->pMove.origin[2] = MSG_ReadShort (&net_Message);
	}

	if (flags & PS_M_VELOCITY) {
		state->pMove.velocity[0] = MSG_ReadShort (&net_Message);
		state->pMove.velocity[1] = MSG_ReadShort (&net_Message);
		state->pMove.velocity[2] = MSG_ReadShort (&net_Message);
	}

	if (flags & PS_M_TIME)
		state->pMove.pmTime = MSG_ReadByte (&net_Message);

	if (flags & PS_M_FLAGS)
		state->pMove.pmFlags = MSG_ReadByte (&net_Message);

	if (flags & PS_M_GRAVITY)
		state->pMove.gravity = MSG_ReadShort (&net_Message);

	if (flags & PS_M_DELTA_ANGLES) {
		state->pMove.deltaAngles[0] = MSG_ReadShort (&net_Message);
		state->pMove.deltaAngles[1] = MSG_ReadShort (&net_Message);
		state->pMove.deltaAngles[2] = MSG_ReadShort (&net_Message);
	}

	if (cl.attractLoop)
		state->pMove.pmType = PMT_FREEZE;		// demo playback

	//
	// parse the rest of the playerState_t
	//
	if (flags & PS_VIEWOFFSET) {
		state->viewOffset[0] = MSG_ReadChar (&net_Message) * 0.25;
		state->viewOffset[1] = MSG_ReadChar (&net_Message) * 0.25;
		state->viewOffset[2] = MSG_ReadChar (&net_Message) * 0.25;
	}

	if (flags & PS_VIEWANGLES) {
		state->viewAngles[0] = MSG_ReadAngle16 (&net_Message);
		state->viewAngles[1] = MSG_ReadAngle16 (&net_Message);
		state->viewAngles[2] = MSG_ReadAngle16 (&net_Message);
	}

	if (flags & PS_KICKANGLES) {
		state->kickAngles[0] = MSG_ReadChar (&net_Message) * 0.25;
		state->kickAngles[1] = MSG_ReadChar (&net_Message) * 0.25;
		state->kickAngles[2] = MSG_ReadChar (&net_Message) * 0.25;
	}

	if (flags & PS_WEAPONINDEX)
		state->gunIndex = MSG_ReadByte (&net_Message);

	if (flags & PS_WEAPONFRAME) {
		state->gunFrame = MSG_ReadByte (&net_Message);
		state->gunOffset[0] = MSG_ReadChar (&net_Message) * 0.25;
		state->gunOffset[1] = MSG_ReadChar (&net_Message) * 0.25;
		state->gunOffset[2] = MSG_ReadChar (&net_Message) * 0.25;
		state->gunAngles[0] = MSG_ReadChar (&net_Message) * 0.25;
		state->gunAngles[1] = MSG_ReadChar (&net_Message) * 0.25;
		state->gunAngles[2] = MSG_ReadChar (&net_Message) * 0.25;
	}

	if (flags & PS_BLEND) {
		state->vBlend[0] = MSG_ReadByte (&net_Message) / 255.0;
		state->vBlend[1] = MSG_ReadByte (&net_Message) / 255.0;
		state->vBlend[2] = MSG_ReadByte (&net_Message) / 255.0;
		state->vBlend[3] = MSG_ReadByte (&net_Message) / 255.0;
	}

	if (flags & PS_FOV)
		state->fov = MSG_ReadByte (&net_Message);

	if (flags & PS_RDFLAGS)
		state->rdFlags = MSG_ReadByte (&net_Message);

	// parse stats
	statbits = MSG_ReadLong (&net_Message);
	for (i=0 ; i<MAX_STATS ; i++)
		if (statbits & (1<<i))
			state->stats[i] = MSG_ReadShort (&net_Message);
}


/*
==============
CL_EntityEvent

An entity has just been parsed that has an event value
the female events are there for backwards compatability
==============
*/
void CL_EntityEvent (entityState_t *ent) {
	switch (ent->event) {
	case EV_ITEM_RESPAWN:
		Snd_StartSound (NULL, ent->number, CHAN_WEAPON, Snd_RegisterSound ("items/respawn1.wav"), 1, ATTN_IDLE, 0);
		CL_ItemRespawnEffect (ent->origin);
		break;
	case EV_PLAYER_TELEPORT:
		Snd_StartSound (NULL, ent->number, CHAN_WEAPON, Snd_RegisterSound ("misc/tele1.wav"), 1, ATTN_IDLE, 0);
		CL_TeleportParticles (ent->origin);
		break;
	case EV_FOOTSTEP:
		if (cl_footsteps->value)
			Snd_StartSound (NULL, ent->number, CHAN_BODY, clMedia.footstepSFX[rand()%4], 1, ATTN_NORM, 0);
		break;
	case EV_FALLSHORT:
		Snd_StartSound (NULL, ent->number, CHAN_AUTO, Snd_RegisterSound ("player/land1.wav"), 1, ATTN_NORM, 0);
		break;
	case EV_FALL:
		Snd_StartSound (NULL, ent->number, CHAN_AUTO, Snd_RegisterSound ("*fall2.wav"), 1, ATTN_NORM, 0);
		break;
	case EV_FALLFAR:
		Snd_StartSound (NULL, ent->number, CHAN_AUTO, Snd_RegisterSound ("*fall1.wav"), 1, ATTN_NORM, 0);
		break;
	}
}


/*
==================
CL_FireEntityEvents
==================
*/
void CL_FireEntityEvents (frame_t *frame) {
	entityState_t		*s1;
	int					pnum, num;

	for (pnum = 0 ; pnum<frame->numEntities ; pnum++) {
		num = (frame->parseEntities + pnum) & (MAX_PARSE_ENTITIES-1);
		s1 = &cl_ParseEntities[num];
		if (s1->event)
			CL_EntityEvent (s1);

		// EF_TELEPORTER acts like an event, but is not cleared each frame
		if (s1->effects & EF_TELEPORTER)
			CL_TeleporterParticles (s1);
	}
}


/*
================
CL_ParseFrame
================
*/
void CL_ParseFrame (void) {
	int			cmd;
	int			len;
	frame_t		*old;

	memset (&cl.frame, 0, sizeof (cl.frame));

	cl.frame.serverFrame = MSG_ReadLong (&net_Message);
	cl.frame.deltaFrame = MSG_ReadLong (&net_Message);
	cl.frame.serverTime = cl.frame.serverFrame*100;

	// BIG HACK to let old demos continue to work
	if (cls.serverProtocol != 26)
		cl.surpressCount = MSG_ReadByte (&net_Message);

	if (cl_shownet->value == 3)
		Com_Printf (PRINT_ALL, "   frame:%i  delta:%i\n", cl.frame.serverFrame, cl.frame.deltaFrame);

	// If the frame is delta compressed from data that we no longer have available, we must
	// suck up the rest of the frame, but not use it, then ask for a non-compressed message
	if (cl.frame.deltaFrame <= 0) {
		cl.frame.valid = qTrue;		// uncompressed frame
		old = NULL;
		cls.demoWaiting = qFalse;	// we can start recording now
	} else {
		old = &cl.frames[cl.frame.deltaFrame & UPDATE_MASK];
		if (!old->valid) {
			// should never happen
			Com_Printf (PRINT_ALL, S_COLOR_RED "Delta from invalid frame (not supposed to happen!).\n");
		}
		if (old->serverFrame != cl.frame.deltaFrame) {
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly
			Com_Printf (PRINT_DEV, S_COLOR_YELLOW "Delta frame too old.\n");
		} else if (cl.parseEntities - old->parseEntities > MAX_PARSE_ENTITIES-128) {
			Com_Printf (PRINT_DEV, S_COLOR_YELLOW "Delta parse_entities too old.\n");
		} else
			cl.frame.valid = qTrue;	// valid delta parse
	}

	// clamp time
	cl.time = clamp (cl.time, cl.frame.serverTime - 100, cl.frame.serverTime);

	// read areaBits
	len = MSG_ReadByte (&net_Message);
	MSG_ReadData (&net_Message, &cl.frame.areaBits, len);

	// read playerinfo
	cmd = MSG_ReadByte (&net_Message);
	SHOWNET (svc_strings[cmd]);
	if (cmd != SVC_PLAYERINFO)
		Com_Error (ERR_DROP, "CL_ParseFrame: not playerinfo");
	CL_ParsePlayerstate (old, &cl.frame);

	// read packet entities
	cmd = MSG_ReadByte (&net_Message);
	SHOWNET (svc_strings[cmd]);
	if (cmd != SVC_PACKETENTITIES)
		Com_Error (ERR_DROP, "CL_ParseFrame: not packetentities");
	CL_ParsePacketEntities (old, &cl.frame);

	// save the frame off in the backup array for later delta comparisons
	cl.frames[cl.frame.serverFrame & UPDATE_MASK] = cl.frame;

	if (cl.frame.valid) {
		// getting a valid frame message ends the connection process
		if (!CL_ConnectionState (CA_ACTIVE)) {
			CL_SetConnectState (CA_ACTIVE);
			cl.forceRefresh = qTrue;
			cl.predictedOrigin[0] = cl.frame.playerState.pMove.origin[0]*ONEDIV8;
			cl.predictedOrigin[1] = cl.frame.playerState.pMove.origin[1]*ONEDIV8;
			cl.predictedOrigin[2] = cl.frame.playerState.pMove.origin[2]*ONEDIV8;
			VectorCopy (cl.frame.playerState.viewAngles, cl.predictedAngles);

			// get rid of loading plaque
			if ((cls.disableServerCount != cl.serverCount) && cl.refreshPrepped)
				SCR_EndLoadingPlaque ();
		}

		// can start mixing ambient sounds
		cl.soundPrepped = qTrue;
	
		// fire entity events
		CL_FireEntityEvents (&cl.frame);
		CL_CheckPredictionError ();
	}
}

/*
==========================================================================

	INTERPOLATE BETWEEN FRAMES TO GET RENDERING PARMS

==========================================================================
*/

/*
===============
CL_AddPacketEntities
===============
*/
float flightminmaxs[6] = {-4, -4, -4, 4, 4, 4};
void AddClientFlashlight (vec3_t origin) {  
	vec3_t	forward;
	trace_t	tr;  

	VectorScale (cl.v_Forward, 2048, forward);
	VectorAdd (forward, cl.refDef.viewOrigin, forward);
	tr = CM_BoxTrace (cl.refDef.viewOrigin, forward, flightminmaxs, flightminmaxs + 3, 0,
						CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_DEADMONSTER);

	VectorCopy (tr.endPos, origin);
}
void CL_AddPacketEntities (frame_t *frame) {
	entity_t			ent;
	entityState_t		*s1;
	clientInfo_t		*ci;
	centity_t			*cent;
	vec3_t				autoRotate, angles;
	vec3_t				autoRotateAxis[3];
	int					i, pnum, autoanim;
	unsigned int		effects;
	qBool				isSelf, isPred, isDrawn;

	// bonus items rotate at a fixed rate
	VectorSet (autoRotate, 0, AngleMod (cls.realTime * 0.1), 0);
	AnglesToAxis (autoRotate, autoRotateAxis);

	autoanim = cls.realTime * 0.001;								// brush models can auto animate their frames

	memset (&ent, 0, sizeof (ent));

	for (pnum=0 ; pnum<frame->numEntities ; pnum++) {
		s1 = &cl_ParseEntities[(frame->parseEntities+pnum)&(MAX_PARSE_ENTITIES-1)];

		cent = &cl_Entities[s1->number];

		ent.muzzleOn = cent->muzzleOn;
		ent.muzzType = cent->muzzType;
		ent.muzzSilenced = cent->muzzSilenced;
		ent.muzzVWeap = cent->muzzVWeap;

		effects = s1->effects;
		ent.flags = s1->renderFx;

		isSelf = isPred = qFalse;
		isDrawn = qTrue;

		// set frame
		if (effects & EF_ANIM01)
			ent.frame = autoanim & 1;
		else if (effects & EF_ANIM23)
			ent.frame = 2 + (autoanim & 1);
		else if (effects & EF_ANIM_ALL)
			ent.frame = autoanim;
		else if (effects & EF_ANIM_ALLFAST)
			ent.frame = cls.realTime / 100;
		else
			ent.frame = s1->frame;
		ent.oldFrame = cent->prev.frame;

		// check effects
		if (effects & EF_PENT) {
			effects &= ~EF_PENT;
			effects |= EF_COLOR_SHELL;
			if (!(ent.flags & RF_SHELL_RED))
				ent.flags |= RF_SHELL_RED;
		}

		if (effects & EF_POWERSCREEN) {
			if (!(ent.flags & RF_SHELL_GREEN))
				ent.flags |= RF_SHELL_GREEN;
		}

		if (effects & EF_QUAD) {
			effects &= ~EF_QUAD;
			effects |= EF_COLOR_SHELL;
			if (!(ent.flags & RF_SHELL_BLUE))
				ent.flags |= RF_SHELL_BLUE;
		}

		if (effects & EF_DOUBLE) {
			effects &= ~EF_DOUBLE;
			effects |= EF_COLOR_SHELL;
			if (!(ent.flags & RF_SHELL_DOUBLE))
				ent.flags |= RF_SHELL_DOUBLE;
		}

		if (effects & EF_HALF_DAMAGE) {
			effects &= ~EF_HALF_DAMAGE;
			effects |= EF_COLOR_SHELL;
			if (!(ent.flags & RF_SHELL_HALF_DAM))
				ent.flags |= RF_SHELL_HALF_DAM;
		}

		ent.backLerp = 1.0 - cl.lerpFrac;
		ent.scale = 1;
		Vector4Set (ent.color, 1, 1, 1, 1);

		// is it me?
		if (s1->number == cl.playerNum+1) {
			isSelf = qTrue;

			if ((cl_predict->value) && !(cl.frame.playerState.pMove.pmFlags & PMF_NO_PREDICTION)) {
				VectorCopy (cl.predictedOrigin, ent.origin);
				VectorCopy (cl.predictedOrigin, ent.oldOrigin);
				isPred = qTrue;
			}
		}

		if (!isPred) {
			if (ent.flags & (RF_FRAMELERP|RF_BEAM)) {
				// step origin discretely, because the frames do the animation properly
				VectorCopy (cent->current.origin, ent.origin);
				VectorCopy (cent->current.oldOrigin, ent.oldOrigin);
			} else {
				// interpolate origin
				for (i=0 ; i<3 ; i++) {
					ent.origin[i] = ent.oldOrigin[i] = cent->prev.origin[i] + cl.lerpFrac * 
						(cent->current.origin[i] - cent->prev.origin[i]);
				}
			}
		}
	
		// tweak the color of beams
		if (ent.flags & RF_BEAM) {
			// the four beam colors are encoded in 32 bits of skinNum (hack)
			int		clr;
			vec3_t	length;

			clr = ((s1->skinNum >> ((rand () % 4)*8)) & 0xff);

			if (rand () % 2)
				CL_BeamTrail (s1->origin, s1->oldOrigin,
					clr, ent.frame,
					0.33 + (frand () * 0.2), -1 / (5 + (frand () * 0.3)));

			VectorSubtract (s1->oldOrigin, s1->origin, length);

			// outer
			makePart (
				ent.origin[0],					ent.origin[1],					ent.origin[2],
				length[0],						length[1],						length[2],
				0,								0,								0,
				0,								0,								0,
				palRed (clr),					palGreen (clr),					palBlue (clr),
				palRed (clr),					palGreen (clr),					palBlue (clr),
				0.30,							PART_INSTANT,
				(ent.frame * 0.9) + ((ent.frame * 0.05) * (rand () % 2)),
				(ent.frame * 0.9) + ((ent.frame * 0.05) * (rand () % 2)),
				PT_BEAM,						PF_BEAM,
				GL_SRC_ALPHA,					GL_ONE_MINUS_SRC_ALPHA,
				0,								qFalse,
				0);

			// core
			makePart (
				ent.origin[0],					ent.origin[1],					ent.origin[2],
				length[0],						length[1],						length[2],
				0,								0,								0,
				0,								0,								0,
				palRed (clr),					palGreen (clr),					palBlue (clr),
				palRed (clr),					palGreen (clr),					palBlue (clr),
				0.30,							PART_INSTANT,
				(ent.frame * 0.9) * 0.3 + ((ent.frame * 0.05) * (rand () % 2)),
				(ent.frame * 0.9) * 0.3 + ((ent.frame * 0.05) * (rand () % 2)),
				PT_BEAM,						PF_BEAM,
				GL_SRC_ALPHA,					GL_ONE,
				0,								qFalse,
				0);

			goto done;
		} else {
			// set skin
			if (s1->modelIndex == 255) {
				// use custom player skin
				ent.skinNum = 0;
				ci = &cl.clientInfo[s1->skinNum & 0xff];
				ent.skin = ci->skin;
				ent.model = ci->model;
				if (!ent.skin || !ent.model) {
					ent.skin = cl.baseClientInfo.skin;
					ent.model = cl.baseClientInfo.model;
				}

				//PGM
				// kthx globalize so it's not being looked up each frame
				if (ent.flags & RF_USE_DISGUISE) {
					if (!Q_strnicmp ((char *)ent.skin, "players/male", 12)) {
						ent.skin = Img_RegisterSkin ("players/male/disguise.pcx");
						ent.model = Mod_RegisterModel ("players/male/tris.md2");
					} else if (!Q_strnicmp ((char *)ent.skin, "players/female", 14)) {
						ent.skin = Img_RegisterSkin ("players/female/disguise.pcx");
						ent.model = Mod_RegisterModel ("players/female/tris.md2");
					} else if (!Q_strnicmp ((char *)ent.skin, "players/cyborg", 14)) {
						ent.skin = Img_RegisterSkin ("players/cyborg/disguise.pcx");
						ent.model = Mod_RegisterModel ("players/cyborg/tris.md2");
					}
				}
				//PGM
			} else {
				ent.skinNum = s1->skinNum;
				ent.skin = NULL;
				ent.model = cl.modelDraw[s1->modelIndex];
			}
		}

		if (ent.model) {
			// gloom-specific effects
			if (FS_CurrGame ("gloom")) {
				// stinger fire/C4 debris
				if (!Q_stricmp ((char *)ent.model, "sprites/s_firea.sp2") ||
					!Q_stricmp ((char *)ent.model, "sprites/s_fireb.sp2") ||
					!Q_stricmp ((char *)ent.model, "sprites/s_flame.sp2")) {
					if (effects & EF_ROCKET) {
						// C4 debris
						CL_GloomEmberTrail (cent->lerpOrigin, ent.origin);
					} else if (glm_advstingfire->integer) {
						// stinger fire
						CL_GloomStingerFire (cent->lerpOrigin, ent.origin, 35 + (frand () * 10), qTrue);
					}

					// skip the original lighting/trail effects
					if ((effects & EF_ROCKET) || glm_advstingfire->integer)
						goto done;
				}

				// bio flare
				else if (!Q_stricmp ((char *)ent.model, "models/objects/laser/tris.md2")) {
					if ((effects & EF_ROCKET) || !(effects & EF_BLASTER)) {
						CL_GloomFlareTrail (cent->lerpOrigin, ent.origin);

						if (effects & EF_ROCKET) {
							effects &= ~EF_ROCKET;
							R_AddLight (ent.origin, 200, 0, 1, 0);
						}
					}
				}

				// blob model
				else if (!Q_stricmp ((char *)ent.model, "models/objects/tlaser/tris.md2")) {
					CL_GloomBlobTip (cent->lerpOrigin, ent.origin);
					isDrawn = qFalse;
				}

				// st/stinger gas
				else if (!Q_stricmp ((char *)ent.model, "models/objects/smokexp/tris.md2")) {
					if (glm_advgas->integer) {
						CL_GloomGasEffect (ent.origin);
						goto done;
					}
				}

				// c4 explosion sprite
				else if (!Q_stricmp ((char *)ent.model, "models/objects/r_explode/tris.md2")) {	
					R_AddLight (ent.origin, 200 + (150*(ent.frame - 29)/36), 1, 0.8, 0.6);
					goto done;
				}
				else if (!Q_stricmp ((char *)ent.model, "models/objects/r_explode/tris2.md2") ||
						!Q_stricmp ((char *)ent.model, "models/objects/explode/tris.md2")) {
					// just don't draw this crappy looking crap
					goto done;
				}
			}

			// xatrix-specific effects
			else if (FS_CurrGame ("xatrix")) {
				// ugly phalanx tip
				if (!Q_stricmp ((char *)ent.model, "sprites/s_photon.sp2")) {
					CL_PhalanxTip (cent->lerpOrigin, ent.origin);
					isDrawn = qFalse;
				}
			}

			// ugly model-based blaster tip
			if (!Q_stricmp ((char *)ent.model, "models/objects/laser/tris.md2")) {
				CL_BlasterTip (cent->lerpOrigin, ent.origin);
				isDrawn = qFalse;
			}
		}

		// generically translucent
		if (ent.flags & RF_TRANSLUCENT)
			ent.color[3] = 0.70;

		// calculate angles
		if (effects & EF_ROTATE) {
			// some bonus items auto-rotate
			Matrix_Copy (autoRotateAxis, ent.axis);
		// RAFAEL
		} else if (effects & EF_SPINNINGLIGHTS) {
			vec3_t	forward;
			vec3_t	start;

			angles[0] = 0;
			angles[1] = AngleMod (cls.realTime/2.0) + s1->angles[1];
			angles[2] = 180;

			AnglesToAxis (angles, ent.axis);

			AngleVectors (angles, forward, NULL, NULL);
			VectorMA (ent.origin, 64, forward, start);
			R_AddLight (start, 100, 1, 0, 0);
		} else {
			// interpolate angles
			float	a1, a2;

			for (i=0 ; i<3 ; i++) {
				a1 = cent->current.angles[i];
				a2 = cent->prev.angles[i];
				angles[i] = LerpAngle (a2, a1, cl.lerpFrac);
			}

			if (angles[0] || angles[1] || angles[2])
				AnglesToAxis (angles, ent.axis);
			else
				Matrix_Identity (ent.axis);
		}

		// flip your shadow around for lefty
		if (isSelf && (hand->integer == 1)) {
			// kthx shadows still use stupid pop/push matrix so this doesn't even work right now
			if (!(ent.flags & RF_CULLHACK))
				ent.flags |= RF_CULLHACK;
			VectorNegate (ent.axis[1], ent.axis[1]);
		}

		// if set to invisible, skip
		if (!s1->modelIndex)
			goto done;

		if (effects & EF_BFG) {
			if (!(ent.flags & RF_TRANSLUCENT))
				ent.flags |= RF_TRANSLUCENT;
			ent.color[3] = 0.30;
		}

		// RAFAEL
		if (effects & EF_PLASMA) {
			if (!(ent.flags & RF_TRANSLUCENT))
				ent.flags |= RF_TRANSLUCENT;
			ent.color[3] = 0.6;
		}

		if (effects & EF_SPHERETRANS) {
			if (!(ent.flags & RF_TRANSLUCENT))
				ent.flags |= RF_TRANSLUCENT;

			// PMM - *sigh*  yet more EF overloading
			if (effects & EF_TRACKERTRAIL)
				ent.color[3] = 0.6;
			else
				ent.color[3] = 0.3;
		}

		if (effects & (EF_GIB|EF_GREENGIB|EF_GRENADE|EF_ROCKET|EF_BLASTER|EF_HYPERBLASTER|EF_BLUEHYPERBLASTER)) {
			if (!(ent.flags & RF_NOSHADOW))
				ent.flags |= RF_NOSHADOW;
		}

		if (isSelf && !(ent.flags & RF_VIEWERMODEL))
			ent.flags |= RF_VIEWERMODEL;

		// hackish mod handling for shells
		if (effects & EF_COLOR_SHELL) {
			if (ent.flags & RF_SHELL_HALF_DAM) {
				if (FS_CurrGame ("rogue")) {
					if (ent.flags & (RF_SHELL_RED|RF_SHELL_BLUE|RF_SHELL_DOUBLE))
						ent.flags &= ~RF_SHELL_HALF_DAM;
				}
			}

			if (ent.flags & RF_SHELL_DOUBLE) {
				if (FS_CurrGame ("rogue")) {
					if (ent.flags & (RF_SHELL_RED|RF_SHELL_BLUE|RF_SHELL_GREEN))
						ent.flags &= ~RF_SHELL_DOUBLE;
					if (ent.flags & RF_SHELL_RED)
						ent.flags |= RF_SHELL_BLUE;
					else if (ent.flags & RF_SHELL_BLUE)
						if (ent.flags & RF_SHELL_GREEN)
							ent.flags &= ~RF_SHELL_BLUE;
						else
							ent.flags |= RF_SHELL_GREEN;
				}
			}
		}

		// add lights to shells
		if ((cl.refDef.rdFlags & RDF_IRGOGGLES) && (ent.flags & RF_IR_VISIBLE)) {
			if (effects & EF_COLOR_SHELL)
				R_AddLight (ent.origin, 50, 1, 0, 0);

			if (!(ent.flags & RF_FULLBRIGHT))
				ent.flags |= RF_FULLBRIGHT;
			VectorSet (ent.color, 1, 0, 0);
		} else if (cl.refDef.rdFlags & RDF_UVGOGGLES) {
			if (effects & EF_COLOR_SHELL)
				R_AddLight (ent.origin, 50, 0, 1, 0);

			if (!(ent.flags & RF_FULLBRIGHT))
				ent.flags |= RF_FULLBRIGHT;
			VectorSet (ent.color, 0, 1, 0);
		} else {
			if (effects & EF_COLOR_SHELL) {
				if (ent.flags & RF_SHELL_RED)
					R_AddLight (ent.origin, 50, 1, 0, 0);
				if (ent.flags & RF_SHELL_GREEN)
					R_AddLight (ent.origin, 50, 0, 1, 0);
				if (ent.flags & RF_SHELL_BLUE)
					R_AddLight (ent.origin, 50, 0, 0, 1);
				if (ent.flags & RF_SHELL_DOUBLE)
					R_AddLight (ent.origin, 50, 0.9, 0.7, 0);
				if (ent.flags & RF_SHELL_HALF_DAM)
					R_AddLight (ent.origin, 50, 0.56, 0.59, 0.45);
			}
		}

		// add to refresh list
		if (isDrawn)
			R_AddEntity (&ent);

		ent.skin = NULL;	// never use a custom skin on others
		ent.skinNum = 0;

		// duplicate for linked models
		if (s1->modelIndex2) {
			if (s1->modelIndex2 == 255) {
				// custom weapon
				ci = &cl.clientInfo[s1->skinNum & 0xff];
				i = (s1->skinNum >> 8);	// 0 is default weapon model
				if (!cl_vwep->integer || (i > MAX_CLIENTWEAPONMODELS - 1))
					i = 0;

				ent.model = ci->weaponmodel[i];
				if (!ent.model) {
					if (i != 0)
						ent.model = ci->weaponmodel[0];
					if (!ent.model)
						ent.model = cl.baseClientInfo.weaponmodel[0];
				}
			} else
				ent.model = cl.modelDraw[s1->modelIndex2];

			// PMM - check for the defender sphere shell .. make it translucent
			// replaces the previous version which used the high bit on modelIndex2 to determine transparency
			if (!Q_stricmp (cl.configStrings[CS_MODELS+(s1->modelIndex2)], "models/items/shell/tris.md2")) {
				if (!(ent.flags & RF_TRANSLUCENT))
					ent.flags |= RF_TRANSLUCENT;
				ent.color[3] = 0.32;
			}
			// pmm

			if (isDrawn)
				R_AddEntity (&ent);
		}

		if (s1->modelIndex3) {
			ent.model = cl.modelDraw[s1->modelIndex3];

			if (isDrawn)
				R_AddEntity (&ent);
		}

		if (s1->modelIndex4) {
			ent.model = cl.modelDraw[s1->modelIndex4];

			if (isDrawn)
				R_AddEntity (&ent);
		}

		// add automatic particle trails
		if (effects &~ EF_ROTATE) {
			if (effects & EF_ROCKET) {
				CL_RocketTrail (cent->lerpOrigin, ent.origin);
				R_AddLight (ent.origin, 200, 1, 1, 0.6);
			}
			// PGM - Do not reorder EF_BLASTER and EF_HYPERBLASTER
			else if (effects & EF_BLASTER) {
				//PGM
				// EF_BLASTER | EF_TRACKER is a special case for EF_BLASTER2... Cheese!
				if (effects & EF_TRACKER) {
					CL_BlasterGreenTrail (cent->lerpOrigin, ent.origin);
					R_AddLight (ent.origin, 200, 0, 1, 0);		
				} else {
					CL_BlasterGoldTrail (cent->lerpOrigin, ent.origin);
					R_AddLight (ent.origin, 200, 1, 1, 0);
				}
				//PGM
			} else if (effects & EF_HYPERBLASTER) {
				if (effects & EF_TRACKER)						// PGM	overloaded for blaster2.
					R_AddLight (ent.origin, 200, 0, 1, 0);		// PGM
				else {
					if (ent.model && FS_CurrGame ("gloom")) {
						if (!Q_stricmp ((char *)ent.model, "sprites/s_shine.sp2")) {
							static int	lasttime = -1;
							vec3_t		florigin;

							if (glm_flashpred->integer && (lasttime != cls.realTime)) {
								vec3_t			org, forward;
								trace_t			tr;
								playerState_t	*ps;

								ps = &cl.frame.playerState; //calc server side player origin
								org[0] = ps->pMove.origin[0] * ONEDIV8;
								org[1] = ps->pMove.origin[1] * ONEDIV8;
								org[2] = ps->pMove.origin[2] * ONEDIV8;

								// get server side player viewangles
								AngleVectors (ps->viewAngles, forward, NULL, NULL);
								VectorScale (forward, 2048, forward);
								VectorAdd (forward, org, forward);
								tr = CM_BoxTrace (org, forward, flightminmaxs, flightminmaxs+3, 0,
													CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_DEADMONSTER);
								VectorSubtract (tr.endPos, ent.origin, forward); 

								if (VectorLength (forward) > 256)
									VectorCopy (ent.origin, florigin);
								else {
									lasttime = cls.realTime;
									AddClientFlashlight (florigin);
								}
							} else
								VectorCopy (ent.origin, florigin);

							if (glm_flwhite->integer)
								R_AddLight (florigin, 200, 1, 1, 1);
							else
								R_AddLight (florigin, 200, 1, 1, 0);
						} else
							R_AddLight (ent.origin, 200, 1, 1, 0);
					} else
						R_AddLight (ent.origin, 200, 1, 1, 0);
				}
			} else if (effects & EF_GIB)
				CL_GibTrail (cent->lerpOrigin, ent.origin, EF_GIB);
			else if (effects & EF_GRENADE)
				CL_GrenadeTrail (cent->lerpOrigin, ent.origin);
			else if (effects & EF_FLIES)
				CL_FlyEffect (cent, ent.origin);
			else if (effects & EF_BFG) {
				// flying
				if (effects & EF_ANIM_ALLFAST) {
					CL_BfgTrail (&ent);
					i = 200;
				// explosion
				} else {
					static int BFG_BrightRamp[6] = { 300, 400, 600, 300, 150, 75 };
					i = BFG_BrightRamp[s1->frame%6];
				}

				R_AddLight (ent.origin, i, 0, 1, 0);
			// RAFAEL
			} else if (effects & EF_TRAP) {
				ent.origin[2] += 32;
				CL_TrapParticles (&ent);
				i = (frand () * 100) + 100;
				R_AddLight (ent.origin, i, 1, 0.8, 0.1);
			} else if (effects & EF_FLAG1) {
				CL_FlagTrail (cent->lerpOrigin, ent.origin, EF_FLAG1);
				R_AddLight (ent.origin, 225, 1, 0.1, 0.1);
			} else if (effects & EF_FLAG2) {
				CL_FlagTrail (cent->lerpOrigin, ent.origin, EF_FLAG2);
				R_AddLight (ent.origin, 225, 0.1, 0.1, 1);

			//ROGUE
			} else if (effects & EF_TAGTRAIL) {
				CL_TagTrail (cent->lerpOrigin, ent.origin);
				R_AddLight (ent.origin, 225, 1.0, 1.0, 0.0);
			} else if (effects & EF_TRACKERTRAIL) {
				if (effects & EF_TRACKER) {
					float intensity;

					intensity = 50 + (500 * (sin (cls.realTime/500.0) + 1.0));
					R_AddLight (ent.origin, intensity, -1.0, -1.0, -1.0);
				} else {
					CL_TrackerShell (cent->lerpOrigin);
					R_AddLight (ent.origin, 155, -1.0, -1.0, -1.0);
				}
			} else if (effects & EF_TRACKER) {
				CL_TrackerTrail (cent->lerpOrigin, ent.origin);
				R_AddLight (ent.origin, 200, -1, -1, -1);
			//ROGUE

			// RAFAEL
			} else if (effects & EF_GREENGIB)
				CL_GibTrail (cent->lerpOrigin, ent.origin, EF_GREENGIB);
			// RAFAEL

			else if (effects & EF_IONRIPPER) {
				CL_IonripperTrail (cent->lerpOrigin, ent.origin);
				if (FS_CurrGame ("gloom"))
					R_AddLight (ent.origin, 100, 0.3, 1, 0.3);
				else
					R_AddLight (ent.origin, 100, 1, 0.5, 0.5);
			}

			// RAFAEL
			else if (effects & EF_BLUEHYPERBLASTER)
				R_AddLight (ent.origin, 200, 0, 0, 1);
			// RAFAEL

			else if (effects & EF_PLASMA) {
				if (effects & EF_ANIM_ALLFAST)
					CL_BlasterGoldTrail (cent->lerpOrigin, ent.origin);

				R_AddLight (ent.origin, 130, 1, 0.5, 0.5);
			}
		}

done:
		if (cent->muzzleOn) {
			cent->muzzleOn = qFalse;
			cent->muzzType = -1;
			cent->muzzSilenced = qFalse;
			cent->muzzVWeap = qFalse;
		}
		VectorCopy (ent.origin, cent->lerpOrigin);
	}
}


/*
==============
CL_AddViewWeapon
==============
*/
void CL_AddViewWeapon (playerState_t *ps, playerState_t *ops) {
	entity_t		gun;
	entityState_t	*s1;
	centity_t		*cent;
	int				i, pnum;
	vec3_t			angles;

	// allow the gun to be completely removed
	if (!cl_gun->integer || (hand->integer == 2))
		return;

	memset (&gun, 0, sizeof (gun));

	if (gun_model) {
		// development tool
		gun.model = gun_model;
	} else
		gun.model = cl.modelDraw[ps->gunIndex];

	if (!gun.model)
		return;

	// set up gun position
	for (i=0 ; i<3 ; i++) {
		gun.origin[i] = cl.refDef.viewOrigin[i] + ops->gunOffset[i] + cl.lerpFrac * (ps->gunOffset[i] - ops->gunOffset[i]);
		angles[i] = cl.refDef.viewAngles[i] + LerpAngle (ops->gunAngles[i], ps->gunAngles[i], cl.lerpFrac);
	}

	if (angles[0] || angles[1] || angles[2])
		AnglesToAxis (angles, gun.axis);
	else
		Matrix_Identity (gun.axis);

	if (gun_frame) {
		// development tool
		gun.frame = gun_frame;
		gun.oldFrame = gun_frame;
	} else {
		gun.frame = ps->gunFrame;
		// just changed weapons, don't lerp from old
		if (gun.frame == 0)
			gun.oldFrame = 0;
		else
			gun.oldFrame = ops->gunFrame;
	}

	cent = &cl_Entities[cl.playerNum+1];
	gun.flags = RF_MINLIGHT|RF_DEPTHHACK|RF_WEAPONMODEL;
	for (pnum = 0 ; pnum<cl.frame.numEntities ; pnum++) {
		s1 = &cl_ParseEntities[(cl.frame.parseEntities+pnum)&(MAX_PARSE_ENTITIES-1)];
		if (s1->number != cl.playerNum + 1)
			continue;

		gun.flags |= s1->renderFx;
		if (s1->effects & EF_PENT)
			gun.flags |= RF_SHELL_RED;
		if (s1->effects & EF_QUAD)
			gun.flags |= RF_SHELL_BLUE;
		if (s1->effects & EF_DOUBLE)
			gun.flags |= RF_SHELL_DOUBLE;
		if (s1->effects & EF_HALF_DAMAGE)
			gun.flags |= RF_SHELL_HALF_DAM;

		if (s1->renderFx & RF_TRANSLUCENT)
			gun.color[3] = 0.70f;

		if (s1->effects & EF_BFG) {
			if (!(gun.flags & RF_TRANSLUCENT))
				gun.flags |= RF_TRANSLUCENT;
			gun.color[3] = 0.30;
		}

		if (s1->effects & EF_PLASMA) {
			if (!(gun.flags & RF_TRANSLUCENT))
				gun.flags |= RF_TRANSLUCENT;
			gun.color[3] = 0.6;
		}

		if (s1->effects & EF_SPHERETRANS) {
			if (!(gun.flags & RF_TRANSLUCENT))
				gun.flags |= RF_TRANSLUCENT;

			if (s1->effects & EF_TRACKERTRAIL)
				gun.color[3] = 0.6;
			else
				gun.color[3] = 0.3;
		}
	}

	gun.scale = 1;
	gun.backLerp = 1.0 - cl.lerpFrac;
	VectorCopy (gun.origin, gun.oldOrigin);	// don't lerp origins at all
	Vector4Set (gun.color, 1, 1, 1, 1);

	gun.muzzleOn = cent->muzzleOn;
	gun.muzzType = cent->muzzType;
	gun.muzzSilenced = cent->muzzSilenced;
	gun.muzzVWeap = qTrue;

	if (hand->integer == 1) {
		gun.flags |= RF_CULLHACK;
		VectorNegate (gun.axis[1], gun.axis[1]);
	}

	R_AddEntity (&gun);
}


/*
===============
CL_CalcViewValues

Sets cl.refdef view values
===============
*/
void CL_CalcViewValues (void) {
	int				i;
	float			lerp, backLerp;
	centity_t		*ent;
	frame_t			*oldFrame;
	playerState_t	*ps, *ops;

	// find the previous frame to interpolate from
	ps = &cl.frame.playerState;
	i = (cl.frame.serverFrame - 1) & UPDATE_MASK;
	oldFrame = &cl.frames[i];

	// previous frame was dropped or involid
	if (oldFrame->serverFrame != cl.frame.serverFrame-1 || !oldFrame->valid)
		oldFrame = &cl.frame;

	ops = &oldFrame->playerState;

	// see if the player entity was teleported this frame
	if (fabs (ops->pMove.origin[0] - ps->pMove.origin[0]) > 256*8	||
		abs (ops->pMove.origin[1] - ps->pMove.origin[1]) > 256*8	||
		abs (ops->pMove.origin[2] - ps->pMove.origin[2]) > 256*8)
		ops = ps;		// don't interpolate

	ent = &cl_Entities[cl.playerNum+1];
	lerp = cl.lerpFrac;

	// calculate the origin
	if (cl_predict->value && !(cl.frame.playerState.pMove.pmFlags & PMF_NO_PREDICTION)) {
		// use predicted values
		unsigned	delta;

		backLerp = 1.0 - lerp;
		for (i=0 ; i<3 ; i++) {
			cl.refDef.viewOrigin[i] = cl.predictedOrigin[i] + ops->viewOffset[i] 
								+ cl.lerpFrac * (ps->viewOffset[i] - ops->viewOffset[i])
								- backLerp * cl.predictionError[i];
		}

		// smooth out stair climbing
		delta = cls.realTime - cl.predictedStepTime;
		if (delta < 100)
			cl.refDef.viewOrigin[2] -= cl.predictedStep * (100 - delta) * 0.01;
	} else {
		// just use interpolated values
		for (i=0 ; i<3 ; i++) {
			cl.refDef.viewOrigin[i] = ops->pMove.origin[i]*ONEDIV8 + ops->viewOffset[i] 
								+ lerp * (ps->pMove.origin[i]*ONEDIV8 + ps->viewOffset[i] 
								- (ops->pMove.origin[i]*ONEDIV8 + ops->viewOffset[i]));
		}
	}

	// if not running a demo or on a locked frame, add the local angle movement
	if (cl.frame.playerState.pMove.pmType < PMT_DEAD) {
		// use predicted values
		VectorCopy (cl.predictedAngles, cl.refDef.viewAngles);
	} else {
		// just use interpolated values
		for (i=0 ; i<3 ; i++)
			cl.refDef.viewAngles[i] = LerpAngle (ops->viewAngles[i], ps->viewAngles[i], lerp);
	}

	for (i=0 ; i<3 ; i++)
		cl.refDef.viewAngles[i] += LerpAngle (ops->kickAngles[i], ps->kickAngles[i], lerp);

	AngleVectors (cl.refDef.viewAngles, cl.v_Forward, cl.v_Right, cl.v_Up);

	if (cl.refDef.viewAngles[0] || cl.refDef.viewAngles[1] || cl.refDef.viewAngles[2])
		AnglesToAxis (cl.refDef.viewAngles, cl.refDef.viewAxis);
	else
		Matrix_Identity (cl.refDef.viewAxis);

	// interpolate field of view
	cl.refDef.fovX = ops->fov + lerp * (ps->fov - ops->fov);

	// don't interpolate blend color
	Vector4Copy (ps->vBlend, cl.refDef.vBlend);

	// add the weapon
	CL_AddViewWeapon (ps, ops);
}


/*
===============
CL_AddEntities

Emits all entities, particles, and lights to the refresh
===============
*/
void CL_AddEntities (void) {
	if (!CL_ConnectionState (CA_ACTIVE))
		return;

	if (cl.time > cl.frame.serverTime) {
		if (cl_showclamp->value)
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "high clamp %i\n", cl.time - cl.frame.serverTime);

		cl.time = cl.frame.serverTime;
		cl.lerpFrac = 1.0;
	} else if (cl.time < cl.frame.serverTime - 100) {
		if (cl_showclamp->value)
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "low clamp %i\n", cl.frame.serverTime - 100 - cl.time);

		cl.time = cl.frame.serverTime - 100;
		cl.lerpFrac = 0;
	} else
		cl.lerpFrac = 1.0 - (cl.frame.serverTime - cl.time) * 0.01;

	if (cl_timedemo->value)
		cl.lerpFrac = 1.0;
	
	CL_CalcViewValues ();
	CL_AddPacketEntities (&cl.frame);
}


/*
===============
CL_GetEntitySoundOrigin

Called to get the sound spatialization origin
===============
*/
void CL_GetEntitySoundOrigin (int ent, vec3_t org) {
	centity_t	*old;

	if ((ent < 0) || (ent >= MAX_EDICTS))
		Com_Error (ERR_DROP, "CL_GetEntitySoundOrigin: bad ent");

	old = &cl_Entities[ent];
	VectorCopy (old->lerpOrigin, org);

	// FIXME: bmodel issues...
}
