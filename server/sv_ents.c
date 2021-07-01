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

#include "sv_local.h"

/*
=============================================================================

	Encode a client frame onto the network channel

=============================================================================
*/

/*
=============
SV_EmitPacketEntities

Writes a delta update of an entityState_t list to the message.
=============
*/
void SV_EmitPacketEntities (clientFrame_t *from, clientFrame_t *to, netMsg_t *msg) {
	entityState_t	*oldent, *newent;
	int		oldindex, newindex;
	int		oldnum, newnum;
	int		from_numEntities;
	int		bits;

	MSG_WriteByte (msg, SVC_PACKETENTITIES);

	if (!from)
		from_numEntities = 0;
	else
		from_numEntities = from->numEntities;

	newindex = 0;
	oldindex = 0;
	while (newindex < to->numEntities || oldindex < from_numEntities) {
		if (newindex >= to->numEntities)
			newnum = 9999;
		else {
			newent = &svs.client_Entities[(to->firstEntity+newindex)%svs.num_ClientEntities];
			newnum = newent->number;
		}

		if (oldindex >= from_numEntities)
			oldnum = 9999;
		else {
			oldent = &svs.client_Entities[(from->firstEntity+oldindex)%svs.num_ClientEntities];
			oldnum = oldent->number;
		}

		if (newnum == oldnum) {
			/* delta update from old position
			** because the force parm is qFalse, this will not result
			** in any bytes being emited if the entity has not changed at all
			** note that players are always 'newentities', this updates their oldorigin always
			** and prevents warping
			*/
			MSG_WriteDeltaEntity (oldent, newent, msg, qFalse, newent->number <= maxclients->integer);
			oldindex++;
			newindex++;
			continue;
		}

		if (newnum < oldnum) {
			/* this is a new entity, send it from the baseline */
			MSG_WriteDeltaEntity (&sv.baseLines[newnum], newent, msg, qTrue, qTrue);
			newindex++;
			continue;
		}

		if (newnum > oldnum) {
			/* the old entity isn't present in the new message */
			bits = U_REMOVE;
			if (oldnum >= 256)
				bits |= U_NUMBER16 | U_MOREBITS1;

			MSG_WriteByte (msg,	bits&255);
			if (bits & 0x0000ff00)
				MSG_WriteByte (msg,	(bits>>8)&255);

			if (bits & U_NUMBER16)
				MSG_WriteShort (msg, oldnum);
			else
				MSG_WriteByte (msg, oldnum);

			oldindex++;
			continue;
		}
	}

	MSG_WriteShort (msg, 0);	// end of packetentities
}


/*
=============
SV_WritePlayerstateToClient
=============
*/
void SV_WritePlayerstateToClient (clientFrame_t *from, clientFrame_t *to, netMsg_t *msg) {
	int				i;
	int				pflags;
	playerState_t	*ps, *ops;
	playerState_t	dummy;
	int				statBits;

	ps = &to->ps;
	if (!from) {
		memset (&dummy, 0, sizeof (dummy));
		ops = &dummy;
	} else
		ops = &from->ps;

	/* determine what needs to be sent */
	pflags = 0;

	if (ps->pMove.pmType != ops->pMove.pmType)
		pflags |= PS_M_TYPE;

	if (ps->pMove.origin[0] != ops->pMove.origin[0]
		|| ps->pMove.origin[1] != ops->pMove.origin[1]
		|| ps->pMove.origin[2] != ops->pMove.origin[2])
		pflags |= PS_M_ORIGIN;

	if (ps->pMove.velocity[0] != ops->pMove.velocity[0]
		|| ps->pMove.velocity[1] != ops->pMove.velocity[1]
		|| ps->pMove.velocity[2] != ops->pMove.velocity[2])
		pflags |= PS_M_VELOCITY;

	if (ps->pMove.pmTime != ops->pMove.pmTime)
		pflags |= PS_M_TIME;

	if (ps->pMove.pmFlags != ops->pMove.pmFlags)
		pflags |= PS_M_FLAGS;

	if (ps->pMove.gravity != ops->pMove.gravity)
		pflags |= PS_M_GRAVITY;

	if (ps->pMove.deltaAngles[0] != ops->pMove.deltaAngles[0]
		|| ps->pMove.deltaAngles[1] != ops->pMove.deltaAngles[1]
		|| ps->pMove.deltaAngles[2] != ops->pMove.deltaAngles[2])
		pflags |= PS_M_DELTA_ANGLES;


	if (ps->viewOffset[0] != ops->viewOffset[0]
		|| ps->viewOffset[1] != ops->viewOffset[1]
		|| ps->viewOffset[2] != ops->viewOffset[2])
		pflags |= PS_VIEWOFFSET;

	if (ps->viewAngles[0] != ops->viewAngles[0]
		|| ps->viewAngles[1] != ops->viewAngles[1]
		|| ps->viewAngles[2] != ops->viewAngles[2])
		pflags |= PS_VIEWANGLES;

	if (ps->kickAngles[0] != ops->kickAngles[0]
		|| ps->kickAngles[1] != ops->kickAngles[1]
		|| ps->kickAngles[2] != ops->kickAngles[2])
		pflags |= PS_KICKANGLES;

	if (ps->vBlend[0] != ops->vBlend[0]
		|| ps->vBlend[1] != ops->vBlend[1]
		|| ps->vBlend[2] != ops->vBlend[2]
		|| ps->vBlend[3] != ops->vBlend[3])
		pflags |= PS_BLEND;

	if (ps->fov != ops->fov)
		pflags |= PS_FOV;

	if (ps->rdFlags != ops->rdFlags)
		pflags |= PS_RDFLAGS;

	if (ps->gunFrame != ops->gunFrame)
		pflags |= PS_WEAPONFRAME;

	pflags |= PS_WEAPONINDEX;

	// write it
	MSG_WriteByte (msg, SVC_PLAYERINFO);
	MSG_WriteShort (msg, pflags);

	// write the pMoveState_t
	if (pflags & PS_M_TYPE)
		MSG_WriteByte (msg, ps->pMove.pmType);

	if (pflags & PS_M_ORIGIN) {
		MSG_WriteShort (msg, ps->pMove.origin[0]);
		MSG_WriteShort (msg, ps->pMove.origin[1]);
		MSG_WriteShort (msg, ps->pMove.origin[2]);
	}

	if (pflags & PS_M_VELOCITY) {
		MSG_WriteShort (msg, ps->pMove.velocity[0]);
		MSG_WriteShort (msg, ps->pMove.velocity[1]);
		MSG_WriteShort (msg, ps->pMove.velocity[2]);
	}

	if (pflags & PS_M_TIME)
		MSG_WriteByte (msg, ps->pMove.pmTime);

	if (pflags & PS_M_FLAGS)
		MSG_WriteByte (msg, ps->pMove.pmFlags);

	if (pflags & PS_M_GRAVITY)
		MSG_WriteShort (msg, ps->pMove.gravity);

	if (pflags & PS_M_DELTA_ANGLES) {
		MSG_WriteShort (msg, ps->pMove.deltaAngles[0]);
		MSG_WriteShort (msg, ps->pMove.deltaAngles[1]);
		MSG_WriteShort (msg, ps->pMove.deltaAngles[2]);
	}

	/* write the rest of the playerState_t */
	if (pflags & PS_VIEWOFFSET) {
		MSG_WriteChar (msg, ps->viewOffset[0]*4);
		MSG_WriteChar (msg, ps->viewOffset[1]*4);
		MSG_WriteChar (msg, ps->viewOffset[2]*4);
	}

	if (pflags & PS_VIEWANGLES) {
		MSG_WriteAngle16 (msg, ps->viewAngles[0]);
		MSG_WriteAngle16 (msg, ps->viewAngles[1]);
		MSG_WriteAngle16 (msg, ps->viewAngles[2]);
	}

	if (pflags & PS_KICKANGLES) {
		MSG_WriteChar (msg, ps->kickAngles[0]*4);
		MSG_WriteChar (msg, ps->kickAngles[1]*4);
		MSG_WriteChar (msg, ps->kickAngles[2]*4);
	}

	if (pflags & PS_WEAPONINDEX) {
		MSG_WriteByte (msg, ps->gunIndex);
	}

	if (pflags & PS_WEAPONFRAME) {
		MSG_WriteByte (msg, ps->gunFrame);
		MSG_WriteChar (msg, ps->gunOffset[0]*4);
		MSG_WriteChar (msg, ps->gunOffset[1]*4);
		MSG_WriteChar (msg, ps->gunOffset[2]*4);
		MSG_WriteChar (msg, ps->gunAngles[0]*4);
		MSG_WriteChar (msg, ps->gunAngles[1]*4);
		MSG_WriteChar (msg, ps->gunAngles[2]*4);
	}

	if (pflags & PS_BLEND) {
		MSG_WriteByte (msg, ps->vBlend[0]*255);
		MSG_WriteByte (msg, ps->vBlend[1]*255);
		MSG_WriteByte (msg, ps->vBlend[2]*255);
		MSG_WriteByte (msg, ps->vBlend[3]*255);
	}

	if (pflags & PS_FOV)
		MSG_WriteByte (msg, ps->fov);

	if (pflags & PS_RDFLAGS)
		MSG_WriteByte (msg, ps->rdFlags);

	/* send stats */
	statBits = 0;
	for (i=0 ; i<MAX_STATS ; i++)
		if (ps->stats[i] != ops->stats[i])
			statBits |= 1<<i;
	MSG_WriteLong (msg, statBits);
	for (i=0 ; i<MAX_STATS ; i++)
		if (statBits & (1<<i))
			MSG_WriteShort (msg, ps->stats[i]);
}


/*
==================
SV_WriteFrameToClient
==================
*/
void SV_WriteFrameToClient (client_t *client, netMsg_t *msg) {
	clientFrame_t		*frame, *oldframe;
	int					lastFrame;

	/* this is the frame we are creating */
	frame = &client->frames[sv.frameNum & UPDATE_MASK];

	if (client->lastFrame <= 0) {
		/* client is asking for a retransmit */
		oldframe = NULL;
		lastFrame = -1;
	} else if (sv.frameNum - client->lastFrame >= (UPDATE_BACKUP - 3)) {
		/* client hasn't gotten a good message through in a long time */
		oldframe = NULL;
		lastFrame = -1;
	} else {
		/* we have a valid message to delta from */
		oldframe = &client->frames[client->lastFrame & UPDATE_MASK];
		lastFrame = client->lastFrame;
	}

	MSG_WriteByte (msg, SVC_FRAME);
	MSG_WriteLong (msg, sv.frameNum);
	MSG_WriteLong (msg, lastFrame);	// what we are delta'ing from
	MSG_WriteByte (msg, client->surpressCount);	// rate dropped packets
	client->surpressCount = 0;

	/* send over the areaBits */
	MSG_WriteByte (msg, frame->areaBytes);
	MSG_Write (msg, frame->areaBits, frame->areaBytes);

	/* delta encode the playerstate */
	SV_WritePlayerstateToClient (oldframe, frame, msg);

	/* delta encode the entities */
	SV_EmitPacketEntities (oldframe, frame, msg);
}


/*
=============================================================================

Build a client frame structure

=============================================================================
*/

byte		fatpvs[65536/8];	// 32767 is BSP_MAX_LEAFS

/*
============
SV_FatPVS

The client will interpolate the view position,
so we can't use a single PVS point
===========
*/
void SV_FatPVS (vec3_t org) {
	int		leafs[64];
	int		i, j, count;
	int		longs;
	byte	*src;
	vec3_t	mins, maxs;

	for (i=0 ; i<3 ; i++) {
		mins[i] = org[i] - 8;
		maxs[i] = org[i] + 8;
	}

	count = CM_BoxLeafnums (mins, maxs, leafs, 64, NULL);
	if (count < 1)
		Com_Error (ERR_FATAL, "SV_FatPVS: count < 1");
	longs = (CM_NumClusters()+31)>>5;

	// convert leafs to clusters
	for (i=0 ; i<count ; i++)
		leafs[i] = CM_LeafCluster(leafs[i]);

	memcpy (fatpvs, CM_ClusterPVS(leafs[0]), longs<<2);
	// or in all the other leaf bits
	for (i=1 ; i<count ; i++) {
		for (j=0 ; j<i ; j++)
			if (leafs[i] == leafs[j])
				break;
		if (j != i)
			continue;		// already have the cluster we want
		src = CM_ClusterPVS(leafs[i]);
		for (j=0 ; j<longs ; j++)
			((long *)fatpvs)[j] |= ((long *)src)[j];
	}
}


/*
=============
SV_BuildClientFrame

Decides which entities are going to be visible to the client, and
copies off the playerstat and areaBits.
=============
*/
void SV_BuildClientFrame (client_t *client) {
	int		e, i;
	vec3_t	org;
	edict_t	*ent;
	edict_t	*clent;
	clientFrame_t	*frame;
	entityState_t	*state;
	int		l;
	int		clientarea, clientcluster;
	int		leafnum;
	int		c_fullsend;
	byte	*clientphs;
	byte	*bitvector;

	clent = client->edict;
	if (!clent->client)
		return;		// not in game yet

	/* this is the frame we are creating */
	frame = &client->frames[sv.frameNum & UPDATE_MASK];

	frame->sentTime = svs.realTime; // save it for ping calc later

	/* find the client's PVS */
	for (i=0 ; i<3 ; i++)
		org[i] = clent->client->ps.pMove.origin[i]*ONEDIV8 + clent->client->ps.viewOffset[i];

	leafnum = CM_PointLeafnum (org);
	clientarea = CM_LeafArea (leafnum);
	clientcluster = CM_LeafCluster (leafnum);

	/* calculate the visible areas */
	frame->areaBytes = CM_WriteAreaBits (frame->areaBits, clientarea);

	/* grab the current playerState_t */
	frame->ps = clent->client->ps;


	SV_FatPVS (org);
	clientphs = CM_ClusterPHS (clientcluster);

	/* build up the list of visible entities */
	frame->numEntities = 0;
	frame->firstEntity = svs.next_ClientEntities;

	c_fullsend = 0;

	for (e=1 ; e<ge->numEdicts ; e++) {
		ent = EDICT_NUM(e);

		/* ignore ents without visible models */
		if (ent->svFlags & SVF_NOCLIENT)
			continue;

		/* ignore ents without visible models unless they have an effect */
		if (!ent->s.modelIndex && !ent->s.effects && !ent->s.sound && !ent->s.event)
			continue;

		/* ignore if not touching a PV leaf */
		if (ent != clent) {
			/* check area */
			if (!CM_AreasConnected (clientarea, ent->areaNum)) {
				/*
				** doors can legally straddle two areas, so
				** we may need to check another one
				*/
				if (!ent->areaNum2 || !CM_AreasConnected (clientarea, ent->areaNum2))
					continue;		// blocked by a door
			}

			/* beams just check one point for PHS */
			if (ent->s.renderFx & RF_BEAM) {
				l = ent->clusterNums[0];
				if (!(clientphs[l >> 3] & (1 << (l&7))))
					continue;
			} else {
				// FIXME: if an ent has a model and a sound, but isn't
				/* in the PVS, only the PHS, clear the model */
				if (ent->s.sound)
					bitvector = fatpvs;	//clientphs;
				else
					bitvector = fatpvs;

				if (ent->numClusters == -1) {
					/* too many leafs for individual check, go by headnode */
					if (!CM_HeadnodeVisible (ent->headNode, bitvector))
						continue;
					c_fullsend++;
				} else {
					/* check individual leafs */
					for (i=0 ; i < ent->numClusters ; i++) {
						l = ent->clusterNums[i];
						if (bitvector[l >> 3] & (1 << (l&7)))
							break;
					}
					if (i == ent->numClusters)
						continue;		// not visible
				}

				if (!ent->s.modelIndex) {
					/* don't send sounds if they will be attenuated away */
					vec3_t	delta;
					float	len;

					VectorSubtract (org, ent->s.origin, delta);
					len = VectorLength (delta);
					if (len > 400)
						continue;
				}
			}
		}

		/* add it to the circular client_Entities array */
		state = &svs.client_Entities[svs.next_ClientEntities%svs.num_ClientEntities];
		if (ent->s.number != e) {
			Com_Printf (PRINT_DEV, "FIXING ENT->S.NUMBER!!!\n");
			ent->s.number = e;
		}
		*state = ent->s;

		/* don't mark players missiles as solid */
		if (ent->owner == client->edict)
			state->solid = 0;

		svs.next_ClientEntities++;
		frame->numEntities++;
	}
}


/*
==================
SV_RecordDemoMessage

Save everything in the world out without deltas.
Used for recording footage for merged or assembled demos
==================
*/
void SV_RecordDemoMessage (void) {
	int			e;
	edict_t		*ent;
	entityState_t	nostate;
	netMsg_t	buf;
	byte		buf_data[32768];
	int			len;

	if (!svs.demo_File)
		return;

	memset (&nostate, 0, sizeof (nostate));
	MSG_Init (&buf, buf_data, sizeof (buf_data));

	/* write a frame message that doesn't contain a playerState_t */
	MSG_WriteByte (&buf, SVC_FRAME);
	MSG_WriteLong (&buf, sv.frameNum);

	MSG_WriteByte (&buf, SVC_PACKETENTITIES);

	e = 1;
	ent = EDICT_NUM(e);
	while (e < ge->numEdicts) {
		/* ignore ents without visible models unless they have an effect */
		if (ent->inUse && ent->s.number &&
			(ent->s.modelIndex || ent->s.effects || ent->s.sound || ent->s.event) &&
			!(ent->svFlags & SVF_NOCLIENT))
			MSG_WriteDeltaEntity (&nostate, &ent->s, &buf, qFalse, qTrue);

		e++;
		ent = EDICT_NUM(e);
	}

	MSG_WriteShort (&buf, 0);		// end of packetentities

	/* now add the accumulated multicast information */
	MSG_Write (&buf, svs.demo_MultiCast.data, svs.demo_MultiCast.curSize);
	MSG_Clear (&svs.demo_MultiCast);

	/* now write the entire message to the file, prefixed by the length */
	len = LittleLong (buf.curSize);
	fwrite (&len, 4, 1, svs.demo_File);
	fwrite (buf.data, buf.curSize, 1, svs.demo_File);
}
