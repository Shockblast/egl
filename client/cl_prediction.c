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

// cl_prediction.c
// - client-side prediction

#include "cl_local.h"

/*
===================
CL_CheckPredictionError
===================
*/
void CL_CheckPredictionError (void) {
	int		frame;
	int		delta[3];
	int		i;
	int		len;

	if (!cl_predict->value || (cl.frame.playerState.pMove.pmFlags & PMF_NO_PREDICTION))
		return;

	/* calculate the last userCmd_t we sent that the server has processed */
	frame = cls.netChan.incomingAcknowledged;
	frame &= (CMD_BACKUP-1);

	/* compare what the server returned with what we had predicted it to be */
	VectorSubtract (cl.frame.playerState.pMove.origin, cl.predictedOrigins[frame], delta);

	/* save the prediction error for interpolation */
	len = abs(delta[0]) + abs(delta[1]) + abs(delta[2]);
	if (len > 640) { // 80 world units
		/* a teleport or something */
		VectorIdentity (cl.predictionError);
	} else {
		if (cl_showmiss->value && (delta[0] || delta[1] || delta[2]))
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "prediction miss on %i: %i\n", cl.frame.serverFrame, 
			delta[0] + delta[1] + delta[2]);

		VectorCopy (cl.frame.playerState.pMove.origin, cl.predictedOrigins[frame]);

		/* save for error itnerpolation */
		for (i=0 ; i<3 ; i++)
			cl.predictionError[i] = delta[i]*ONEDIV8;
	}
}


/*
====================
CL_ClipMoveToEntities
====================
*/
void CL_ClipMoveToEntities (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, trace_t *tr) {
	int				i, x, zd, zu;
	trace_t			trace;
	int				headnode;
	float			*angles;
	entityState_t	*ent;
	int				num;
	cBspModel_t		*cmodel;
	vec3_t			bmins, bmaxs;

	for (i=0 ; i<cl.frame.numEntities ; i++) {
		num = (cl.frame.parseEntities + i) & (MAX_PARSE_ENTITIES - 1);
		ent = &cl_ParseEntities[num];

		if (!ent->solid)
			continue;

		if (ent->number == cl.playerNum+1)
			continue;

		if (ent->solid == 31) {
			/* special value for bmodel */
			cmodel = cl.modelClip[ent->modelIndex];
			if (!cmodel)
				continue;
			headnode = cmodel->headNode;
			angles = ent->angles;
		} else {
			/* encoded bbox */
			x = 8 * (ent->solid & 31);
			zd = 8 * ((ent->solid >> 5) & 31);
			zu = 8 * ((ent->solid >> 10) & 63) - 32;

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			headnode = CM_HeadnodeForBox (bmins, bmaxs);
			angles = vec3Identity;	// boxes don't rotate
		}

		if (tr->allSolid)
			return;

		trace = CM_TransformedBoxTrace (start, end,
			mins, maxs, headnode, MASK_PLAYERSOLID,
			ent->origin, angles);

		if (trace.allSolid || trace.startSolid || (trace.fraction < tr->fraction)) {
			trace.ent = (struct edict_s *)ent;
			if (tr->startSolid) {
				*tr = trace;
				tr->startSolid = qTrue;
			} else
				*tr = trace;
		} else if (trace.startSolid)
			tr->startSolid = qTrue;
	}
}


/*
================
CL_PMTrace
================
*/
trace_t CL_PMTrace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end) {
	trace_t	t;

	/* check against world */
	t = CM_BoxTrace (start, end, mins, maxs, 0, MASK_PLAYERSOLID);
	if (t.fraction < 1.0)
		t.ent = (struct edict_s *)1;

	/* check all other solid models */
	CL_ClipMoveToEntities (start, mins, maxs, end, &t);

	return t;
}


/*
================
CL_PMPointContents
================
*/
int CL_PMPointContents (vec3_t point) {
	int				i;
	entityState_t	*ent;
	int				num;
	cBspModel_t		*cmodel;
	int				contents;

	contents = CM_PointContents (point, 0);

	for (i=0 ; i<cl.frame.numEntities ; i++) {
		num = (cl.frame.parseEntities + i)&(MAX_PARSE_ENTITIES-1);
		ent = &cl_ParseEntities[num];

		if (ent->solid != 31) // special value for bmodel
			continue;

		cmodel = cl.modelClip[ent->modelIndex];
		if (!cmodel)
			continue;

		contents |= CM_TransformedPointContents (point, cmodel->headNode, ent->origin, ent->angles);
	}

	return contents;
}


/*
=================
CL_PredictMovement

Sets cl.predicted_origin and cl.predictedAngles
=================
*/
void CL_PredictMovement (void) {
	int			ack, current;
	int			frame;
	int			oldframe;
	userCmd_t	*cmd;
	pMove_t		pm;
	int			i;
	int			step;
	int			oldz;

	if (!CL_ConnectionState (CA_ACTIVE))
		return;

	if (cl_paused->value)
		return;

	if (!cl_predict->value || (cl.frame.playerState.pMove.pmFlags & PMF_NO_PREDICTION)) {
		/* just set angles */
		for (i=0 ; i<3 ; i++)
			cl.predictedAngles[i] = cl.viewAngles[i] + SHORT2ANGLE(cl.frame.playerState.pMove.deltaAngles[i]);

		return;
	}

	ack = cls.netChan.incomingAcknowledged;
	current = cls.netChan.outgoingSequence;

	/* if we are too far out of date, just freeze */
	if (current - ack >= CMD_BACKUP) {
		if (cl_showmiss->value)
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "exceeded CMD_BACKUP\n");
		return;	
	}

	/* copy current state to pmove */
	memset (&pm, 0, sizeof (pm));
	pm.trace = CL_PMTrace;
	pm.pointcontents = CL_PMPointContents;

	pm_airaccelerate = atof (cl.configStrings[CS_AIRACCEL]);

	pm.s = cl.frame.playerState.pMove;

	frame = 0;

	/* run frames */
	while (++ack < current) {
		frame = ack & (CMD_BACKUP-1);
		cmd = &cl.cmds[frame];

		pm.cmd = *cmd;
		Pmove (&pm);

		/* save for debug checking */
		VectorCopy (pm.s.origin, cl.predictedOrigins[frame]);
	}

	oldframe = (ack-2) & (CMD_BACKUP-1);
	oldz = cl.predictedOrigins[oldframe][2];
	step = pm.s.origin[2] - oldz;
	if ((step > 63) && (step < 160) && (pm.s.pmFlags & PMF_ON_GROUND)) {
		cl.predictedStep = step * ONEDIV8;
		cl.predictedStepTime = cls.realTime - cls.frameTime * 500;
	}

	/* copy results out for rendering */
	cl.predictedOrigin[0] = pm.s.origin[0]*ONEDIV8;
	cl.predictedOrigin[1] = pm.s.origin[1]*ONEDIV8;
	cl.predictedOrigin[2] = pm.s.origin[2]*ONEDIV8;

	VectorCopy (pm.viewAngles, cl.predictedAngles);
}
