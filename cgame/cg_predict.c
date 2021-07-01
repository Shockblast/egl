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
// cg_predict.c
//

#include "cg_local.h"

/*
===================
CG_CheckPredictionError
===================
*/
void CG_CheckPredictionError (void) {
	int		frame;
	int		delta[3];
	int		len;
	int		incAck;

	if (!cl_predict->value || (cg.frame.playerState.pMove.pmFlags & PMF_NO_PREDICTION))
		return;

	cgi.NET_GetSequenceState (NULL, &incAck);

	// calculate the last userCmd_t we sent that the server has processed
	frame = incAck & CMD_MASK;

	// compare what the server returned with what we had predicted it to be
	VectorSubtract (cg.frame.playerState.pMove.origin, cg.predictedOrigins[frame], delta);

	// save the prediction error for interpolation
	len = abs (delta[0]) + abs (delta[1]) + abs (delta[2]);
	if (len > 640) { // 80 world units
		// a teleport or something
		VectorClear (cg.predictionError);
	} else {
		if (cl_showmiss->value && (delta[0] || delta[1] || delta[2]))
			Com_Printf (0, S_COLOR_YELLOW "CG_CheckPredictionError: prediction miss on frame %i: %i\n",
				cg.frame.serverFrame, delta[0]+delta[1]+delta[2]);

		VectorCopy (cg.frame.playerState.pMove.origin, cg.predictedOrigins[frame]);
		VectorScale (delta, ONEDIV8, cg.predictionError);
	}
}


/*
====================
CG_ClipMoveToEntities
====================
*/
void CG_ClipMoveToEntities (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, trace_t *tr) {
	int				i, x, zd, zu;
	trace_t			trace;
	int				headnode;
	float			*angles;
	entityState_t	*ent;
	int				num;
	cBspModel_t		*cmodel;
	vec3_t			bmins, bmaxs;

	for (i=0 ; i<cg.frame.numEntities ; i++) {
		num = (cg.frame.parseEntities + i) & (MAX_PARSEENTITIES_MASK);
		ent = &cgParseEntities[num];

		if (!ent->solid)
			continue;

		if (ent->number == cg.playerNum+1)
			continue;

		if (ent->solid == 31) {
			// special value for bmodel
			cmodel = cg.modelClip[ent->modelIndex];
			if (!cmodel)
				continue;
			headnode = cmodel->headNode;
			angles = ent->angles;
		} else {
			// encoded bbox
			x = 8 * (ent->solid & 31);
			zd = 8 * ((ent->solid >> 5) & 31);
			zu = 8 * ((ent->solid >> 10) & 63) - 32;

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			headnode = cgi.CM_HeadnodeForBox (bmins, bmaxs);
			angles = vec3Origin;	// boxes don't rotate
		}

		if (tr->allSolid)
			return;

		trace = cgi.CM_TransformedBoxTrace (start, end,
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
CG_PMTrace
================
*/
trace_t CG_PMTrace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, qBool entities) {
	trace_t		tr;

	// check against world
	tr = cgi.CM_BoxTrace (start, end, mins, maxs, 0, MASK_PLAYERSOLID);
	if (tr.fraction < 1.0)
		tr.ent = (struct edict_s *)1;

	// check all other solid models
	if (entities)
		CG_ClipMoveToEntities (start, mins, maxs, end, &tr);

	return tr;
}


/*
================
CG_PMLTrace

Local version
================
*/
static trace_t CG_PMLTrace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end) {
	trace_t	t;

	// check against world
	t = cgi.CM_BoxTrace (start, end, mins, maxs, 0, MASK_PLAYERSOLID);
	if (t.fraction < 1.0)
		t.ent = (struct edict_s *)1;

	// check all other solid models
	CG_ClipMoveToEntities (start, mins, maxs, end, &t);

	return t;
}


/*
================
CG_PMPointContents
================
*/
int CG_PMPointContents (vec3_t point) {
	entityState_t	*ent;
	int				i, num;
	cBspModel_t		*cmodel;
	int				contents;

	contents = cgi.CM_PointContents (point, 0);

	for (i=0 ; i<cg.frame.numEntities ; i++) {
		num = (cg.frame.parseEntities + i)&(MAX_PARSEENTITIES_MASK);
		ent = &cgParseEntities[num];

		if (ent->solid != 31) // special value for bmodel
			continue;

		cmodel = cg.modelClip[ent->modelIndex];
		if (!cmodel)
			continue;

		contents |= cgi.CM_TransformedPointContents (point, cmodel->headNode, ent->origin, ent->angles);
	}

	return contents;
}


/*
=================
CG_PredictMovement

Sets cg.predictedOrigin and cg.predictedAngles
=================
*/
void CG_PredictMovement (void) {
	int			ack, current;
	int			frame;
	int			oldframe;
	pMove_t		pm;
	int			step;
	int			oldz;

	if (cgi.Cvar_VariableInteger ("paused"))
		return;

	if (!cl_predict->value || (cg.frame.playerState.pMove.pmFlags & PMF_NO_PREDICTION)) {
		userCmd_t	cmd;

		current = cgi.NET_GetCurrentUserCmdNum ();
		cgi.NET_GetUserCmd (current, &cmd);

		VectorScale (cg.frame.playerState.pMove.velocity, ONEDIV8, cg.predictedVelocity);
		VectorScale (cg.frame.playerState.pMove.origin, ONEDIV8, cg.predictedOrigin);

		cg.predictedAngles[0] = SHORT2ANGLE (cmd.angles[0]) + SHORT2ANGLE(cg.frame.playerState.pMove.deltaAngles[0]);
		cg.predictedAngles[1] = SHORT2ANGLE (cmd.angles[1]) + SHORT2ANGLE(cg.frame.playerState.pMove.deltaAngles[1]);
		cg.predictedAngles[2] = SHORT2ANGLE (cmd.angles[2]) + SHORT2ANGLE(cg.frame.playerState.pMove.deltaAngles[2]);

		return;
	}

	cgi.NET_GetSequenceState (&current, &ack);

	// if we are too far out of date, just freeze
	if (current - ack >= CMD_BACKUP) {
		if (cl_showmiss->value)
			Com_Printf (0, S_COLOR_YELLOW "CG_PredictMovement: exceeded CMD_BACKUP\n");
		return;	
	}

	// copy current state to pmove
	memset (&pm, 0, sizeof (pm));
	pm.trace = CG_PMLTrace;
	pm.pointContents = CG_PMPointContents;
	pm.state = cg.frame.playerState.pMove;

	// run frames
	frame = 0;
	while (++ack < current) {
		frame = ack & CMD_MASK;
		cgi.NET_GetUserCmd (frame, &pm.cmd);

		Pmove (&pm, atof (cg.configStrings[CS_AIRACCEL]));

		// save for debug checking
		VectorCopy (pm.state.origin, cg.predictedOrigins[frame]);
	}

	oldframe = (ack-2) & CMD_MASK;
	oldz = cg.predictedOrigins[oldframe][2];
	step = pm.state.origin[2] - oldz;

	if ((step > 63) && (step < 160) && (pm.state.pmFlags & PMF_ON_GROUND)) {
		cg.predictedStep = step * ONEDIV8;
		cg.predictedStepTime = cgs.realTime - cgs.frameTime * 500;
	}

	VectorScale (pm.state.velocity, ONEDIV8, cg.predictedVelocity);
	VectorScale (pm.state.origin, ONEDIV8, cg.predictedOrigin);
	VectorCopy (pm.viewAngles, cg.predictedAngles);
}
