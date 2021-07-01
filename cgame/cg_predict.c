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
void CG_CheckPredictionError (void)
{
	int		frame;
	int		delta[3];
	int		len;
	int		incAck;

	if (!cl_predict->value || (cg.frame.playerState.pMove.pmFlags & PMF_NO_PREDICTION))
		return;

	cgi.NET_GetSequenceState (NULL, &incAck);

	// Calculate the last userCmd_t we sent that the server has processed
	frame = incAck & CMD_MASK;

	// Compare what the server returned with what we had predicted it to be
	VectorSubtract (cg.frame.playerState.pMove.origin, cg.predicted.origins[frame], delta);

	// Save the prediction error for interpolation
	len = abs (delta[0]) + abs (delta[1]) + abs (delta[2]);
	if (len > 640) {
		// 80 world units, a teleport or something
		VectorClear (cg.predicted.error);
	}
	else {
		if (cl_showmiss->value && (delta[0] || delta[1] || delta[2]))
			Com_Printf (0, S_COLOR_YELLOW "CG_CheckPredictionError: prediction miss on frame %i: %i\n",
				cg.frame.serverFrame, delta[0]+delta[1]+delta[2]);

		VectorCopy (cg.frame.playerState.pMove.origin, cg.predicted.origins[frame]);
		VectorScale (delta, (1.0f/8.0f), cg.predicted.error);
	}
}


/*
====================
CG_ClipMoveToEntities
====================
*/
void CG_ClipMoveToEntities (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, trace_t *tr)
{
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
		ent = &cg_parseEntities[num];

		if (!ent->solid)
			continue;

		// Skip yourself
		if (ent->number == cg.playerNum+1)
			continue;

		if (ent->solid == 31) {
			// Special value for bmodel
			cmodel = cg.modelClip[ent->modelIndex];
			if (!cmodel)
				continue;
			headnode = cmodel->headNode;
			angles = ent->angles;
		}
		else {
			// Encoded bbox
			x = 8 * (ent->solid & 31);
			zd = 8 * ((ent->solid >> 5) & 31);
			zu = 8 * ((ent->solid >> 10) & 63) - 32;

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			headnode = CGI_CM_HeadnodeForBox (bmins, bmaxs);
			angles = vec3Origin;	// Boxes don't rotate
		}

		if (tr->allSolid)
			return;

		trace = CGI_CM_TransformedBoxTrace (start, end,
			mins, maxs, headnode, MASK_PLAYERSOLID,
			ent->origin, angles);

		if (trace.allSolid || trace.startSolid || (trace.fraction < tr->fraction)) {
			trace.ent = (struct edict_s *)ent;
			if (tr->startSolid) {
				*tr = trace;
				tr->startSolid = qTrue;
			}
			else
				*tr = trace;
		}
		else if (trace.startSolid)
			tr->startSolid = qTrue;
	}
}


/*
================
CG_PMTrace
================
*/
trace_t CG_PMTrace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, qBool entities)
{
	trace_t		tr;

	if (!mins)
		mins = vec3Origin;

	if (!maxs)
		maxs = vec3Origin;

	// Check against world
	tr = CGI_CM_BoxTrace (start, end, mins, maxs, 0, MASK_PLAYERSOLID);
	if (tr.fraction < 1.0)
		tr.ent = (struct edict_s *)1;

	// Check all other solid models
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
static trace_t CG_PMLTrace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	trace_t	t;

	// Check against world
	t = CGI_CM_BoxTrace (start, end, mins, maxs, 0, MASK_PLAYERSOLID);
	if (t.fraction < 1.0)
		t.ent = (struct edict_s *)1;

	// Check all other solid models
	CG_ClipMoveToEntities (start, mins, maxs, end, &t);

	return t;
}


/*
================
CG_PMPointContents
================
*/
int CG_PMPointContents (vec3_t point)
{
	entityState_t	*ent;
	int				i, num;
	cBspModel_t		*cmodel;
	int				contents;

	contents = CGI_CM_PointContents (point, 0);

	for (i=0 ; i<cg.frame.numEntities ; i++) {
		num = (cg.frame.parseEntities + i)&(MAX_PARSEENTITIES_MASK);
		ent = &cg_parseEntities[num];

		if (ent->solid != 31) // Special value for bmodel
			continue;

		cmodel = cg.modelClip[ent->modelIndex];
		if (!cmodel)
			continue;

		contents |= CGI_CM_TransformedPointContents (point, cmodel->headNode, ent->origin, ent->angles);
	}

	return contents;
}


/*
=================
CG_PredictMovement

Sets cg.predicted.origin and cg.predicted.angles
=================
*/
void CG_PredictMovement (void)
{
	static int	stepFrameLast = 0;
	int			ack, current;
	int			frame, oldFrame;
	int			step;
	pMoveNew_t	pm;

	if (cgi.Cvar_VariableInteger ("paused"))
		return;

	if (!cl_predict->value || cg.frame.playerState.pMove.pmFlags & PMF_NO_PREDICTION) {
		userCmd_t	cmd;

		current = cgi.NET_GetCurrentUserCmdNum ();
		cgi.NET_GetUserCmd (current, &cmd);

		VectorScale (cg.frame.playerState.pMove.velocity, (1.0f/8.0f), cg.predicted.velocity);
		VectorScale (cg.frame.playerState.pMove.origin, (1.0f/8.0f), cg.predicted.origin);

		cg.predicted.angles[0] = SHORT2ANGLE(cmd.angles[0]) + SHORT2ANGLE(cg.frame.playerState.pMove.deltaAngles[0]);
		cg.predicted.angles[1] = SHORT2ANGLE(cmd.angles[1]) + SHORT2ANGLE(cg.frame.playerState.pMove.deltaAngles[1]);
		cg.predicted.angles[2] = SHORT2ANGLE(cmd.angles[2]) + SHORT2ANGLE(cg.frame.playerState.pMove.deltaAngles[2]);

		return;
	}

	cgi.NET_GetSequenceState (&current, &ack);

	// If we are too far out of date, just freeze
	if (current - ack >= CMD_BACKUP) {
		if (cl_showmiss->value)
			Com_Printf (0, S_COLOR_YELLOW "CG_PredictMovement: exceeded CMD_BACKUP\n");
		return;	
	}

	// Copy current state to pmove
	memset (&pm, 0, sizeof (pm));
	pm.trace = CG_PMLTrace;
	pm.pointContents = CG_PMPointContents;
	pm.state = cg.frame.playerState.pMove;

	if (cg.attractLoop)
		pm.state.pmType = PMT_FREEZE;		// Demo playback

	// Run frames
	frame = 0;
	while (++ack <= current) {		// Changed '<' to '<=' cause current is our pending cmd
		frame = ack & CMD_MASK;
		cgi.NET_GetUserCmd (frame, &pm.cmd);

		if (pm.cmd.msec <= 0)
			continue;	// Ignore 'null' usercmd entries.

		if (cgs.serverProtocol == ENHANCED_PROTOCOL_VERSION) {
			VectorCopy (cg.frame.playerState.mins, pm.mins);
			VectorCopy (cg.frame.playerState.maxs, pm.maxs);
		}
		else {
			VectorSet (pm.mins, -16, -16, -24);
			VectorSet (pm.maxs,  16,  16, 32);
		}

		if (pm.state.pmType == PMT_SPECTATOR && cgs.serverProtocol == ENHANCED_PROTOCOL_VERSION)
			pm.multiplier = 2;
		else
			pm.multiplier = 1;

		Pmove (&pm, atof (cg.configStrings[CS_AIRACCEL]));

		// Save for debug checking
		VectorCopy (pm.state.origin, cg.predicted.origins[frame]);
	}

	// Don't forget to reset ack so stair prediction works
	ack--;

	oldFrame = (ack-2) & CMD_MASK;
	step = pm.state.origin[2] - cg.predicted.origins[oldFrame][2];
	if ((step > 63) && (step < 160) && (pm.state.pmFlags & PMF_ON_GROUND) && (current != stepFrameLast)) {
		stepFrameLast = current;
		cg.predicted.step = step * (1.0f/8.0f);
		cg.predicted.stepTime = cgs.realTime - cgs.netFrameTime * 500;
	}

	VectorScale (pm.state.velocity, (1.0f/8.0f), cg.predicted.velocity);
	VectorScale (pm.state.origin, (1.0f/8.0f), cg.predicted.origin);
	VectorCopy (pm.viewAngles, cg.predicted.angles);
}
