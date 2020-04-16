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

static int				cg_numSolids;
static entityState_t	*cg_solidList[MAX_PARSE_ENTITIES];

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

	cgi.NET_GetSequenceState (NULL, &incAck);

	// Calculate the last userCmd_t we sent that the server has processed
	frame = incAck & CMD_MASK;

	// Compare what the server returned with what we had predicted it to be
	Vec3Subtract (cg.frame.playerState.pMove.origin, cg.predicted.origins[frame], delta);

	// Save the prediction error for interpolation
	len = abs (delta[0]) + abs (delta[1]) + abs (delta[2]);
	if (len > 640) {
		// 80 world units, a teleport or something
		Vec3Clear (cg.predicted.error);
	}
	else {
		if (cl_showmiss->intVal && (delta[0] || delta[1] || delta[2]))
			Com_Printf (PRNT_WARNING, "CG_CheckPredictionError: prediction miss on frame %i: %i\n",
				cg.frame.serverFrame, delta[0]+delta[1]+delta[2]);

		Vec3Copy (cg.frame.playerState.pMove.origin, cg.predicted.origins[frame]);
		Vec3Scale (delta, (1.0f/8.0f), cg.predicted.error);
	}
}


/*
====================
CG_BuildSolidList
====================
*/
void CG_BuildSolidList (void)
{
	entityState_t	*ent;
	int				num, i;

	cg_numSolids = 0;
	for (i=0 ; i<cg.frame.numEntities ; i++) {
		num = (cg.frame.parseEntities + i) & (MAX_PARSEENTITIES_MASK);
		ent = &cg_parseEntities[num];

		if (ent->solid)
			cg_solidList[cg_numSolids++] = ent;
	}
}


/*
====================
CG_ClipMoveToEntities
====================
*/
static void CG_ClipMoveToEntities (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int ignoreNum, qBool entities, qBool bModels, trace_t *out)
{
	int				i, x, zd, zu;
	trace_t			trace;
	int				headnode;
	float			*angles;
	entityState_t	*ent;
	struct cBspModel_s *cmodel;
	vec3_t			bmins, bmaxs;

	for (i=0 ; i<cg_numSolids ; i++) {
		ent = cg_solidList[i];
		if (ent->number == ignoreNum)
			continue;

		if (ent->solid == 31) {
			// Special value for bmodel
			if (!bModels)
				continue;

			cmodel = cg.modelCfgClip[ent->modelIndex];
			if (!cmodel)
				continue;
			headnode = cgi.CM_InlineModelHeadNode (cmodel);
			angles = ent->angles;
		}
		else {
			// Encoded bbox
			if (!entities)
				continue;

			x = 8 * (ent->solid & 31);
			zd = 8 * ((ent->solid >> 5) & 31);
			zu = 8 * ((ent->solid >> 10) & 63) - 32;

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			headnode = cgi.CM_HeadnodeForBox (bmins, bmaxs);
			angles = vec3Origin;	// Boxes don't rotate
		}

		if (out->allSolid)
			return;

		cgi.CM_TransformedBoxTrace (&trace, start, end, mins, maxs, headnode, MASK_PLAYERSOLID, ent->origin, angles);
		if (trace.allSolid || trace.startSolid || trace.fraction < out->fraction) {
			trace.ent = (struct edict_s *)ent;
			if (out->startSolid) {
				*out = trace;
				out->startSolid = qTrue;
			}
			else
				*out = trace;
		}
		else if (trace.startSolid)
			out->startSolid = qTrue;
	}
}


/*
================
CG_PMTrace
================
*/
void CG_PMTrace (trace_t *out, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, qBool entities, qBool bModels)
{
	if (!out)
		return;

	if (!mins)
		mins = vec3Origin;

	if (!maxs)
		maxs = vec3Origin;

	// Check against world
	*out = cgi.CM_BoxTrace (start, end, mins, maxs, 0, MASK_PLAYERSOLID);
	if (out->fraction < 1.0)
		out->ent = (struct edict_s *)1;

	// Check all other solid models
	if (entities || bModels)
		CG_ClipMoveToEntities (start, mins, maxs, end, cg.playerNum+1, entities, bModels, out);
}


/*
================
CG_PMLTrace

Local version
================
*/
static trace_t CG_PMLTrace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	trace_t	tr;

	// Check against world
	tr = cgi.CM_BoxTrace (start, end, mins, maxs, 0, MASK_PLAYERSOLID);
	if (tr.fraction < 1.0)
		tr.ent = (struct edict_s *)1;

	// Check all other solid models
	CG_ClipMoveToEntities (start, mins, maxs, end, cg.playerNum+1, qTrue, qTrue, &tr);

	return tr;
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
	struct cBspModel_s *cmodel;
	int				contents;

	contents = cgi.CM_PointContents (point, 0);

	for (i=0 ; i<cg.frame.numEntities ; i++) {
		num = (cg.frame.parseEntities + i)&(MAX_PARSEENTITIES_MASK);
		ent = &cg_parseEntities[num];
		if (ent->solid != 31) // Special value for bmodel
			continue;

		cmodel = cg.modelCfgClip[ent->modelIndex];
		if (!cmodel)
			continue;

		contents |= cgi.CM_TransformedPointContents (point, cgi.CM_InlineModelHeadNode (cmodel), ent->origin, ent->angles);
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
	int			ack, current;
	int			frame;
	int			step;
	float		oldStep;
	pMoveNew_t	pm;

	if (cgi.Cvar_GetIntegerValue ("paused"))
		return;

	if (!cl_predict->intVal || cg.frame.playerState.pMove.pmFlags & PMF_NO_PREDICTION) {
		userCmd_t	cmd;

		current = cgi.NET_GetCurrentUserCmdNum ();
		cgi.NET_GetUserCmd (current, &cmd);

		Vec3Scale (cg.frame.playerState.pMove.velocity, (1.0f/8.0f), cg.predicted.velocity);
		Vec3Scale (cg.frame.playerState.pMove.origin, (1.0f/8.0f), cg.predicted.origin);

		cg.predicted.angles[0] = SHORT2ANGLE(cmd.angles[0]) + SHORT2ANGLE(cg.frame.playerState.pMove.deltaAngles[0]);
		cg.predicted.angles[1] = SHORT2ANGLE(cmd.angles[1]) + SHORT2ANGLE(cg.frame.playerState.pMove.deltaAngles[1]);
		cg.predicted.angles[2] = SHORT2ANGLE(cmd.angles[2]) + SHORT2ANGLE(cg.frame.playerState.pMove.deltaAngles[2]);

		return;
	}

	cgi.NET_GetSequenceState (&current, &ack);

	// If we are too far out of date, just freeze
	if (current - ack >= CMD_BACKUP) {
		if (cl_showmiss->intVal)
			Com_Printf (PRNT_WARNING, "CG_PredictMovement: exceeded CMD_BACKUP\n");
		return;	
	}

	// Copy current state to pmove
	memset (&pm, 0, sizeof (pm));
	pm.trace = CG_PMLTrace;
	pm.pointContents = CG_PMPointContents;
	pm.state = cg.frame.playerState.pMove;

	if (cg.attractLoop)
		pm.state.pmType = PMT_FREEZE;		// Demo playback

	if (pm.state.pmType == PMT_SPECTATOR && cg.serverProtocol == ENHANCED_PROTOCOL_VERSION)
		pm.multiplier = 2;
	else
		pm.multiplier = 1;

	pm.strafeHack = cg.strafeHack;

	// Run frames
	frame = 0;
	while (++ack <= current) {		// Changed '<' to '<=' cause current is our pending cmd
		frame = ack & CMD_MASK;
		cgi.NET_GetUserCmd (frame, &pm.cmd);

		if (pm.cmd.msec <= 0)
			continue;	// Ignore 'null' usercmd entries.

		// Playerstate transmitted mins/maxs
		if (cg.serverProtocol == ENHANCED_PROTOCOL_VERSION) {
			Vec3Copy (cg.frame.playerState.mins, pm.mins);
			Vec3Copy (cg.frame.playerState.maxs, pm.maxs);
		}
		else {
			Vec3Set (pm.mins, -16, -16, -24);
			Vec3Set (pm.maxs,  16,  16,  32);
		}

		Pmove (&pm, atof (cg.configStrings[CS_AIRACCEL]));

		// Save for debug checking
		Vec3Copy (pm.state.origin, cg.predicted.origins[frame]);
	}

	// Calculate the step adjustment
	step = pm.state.origin[2] - (int)(cg.predicted.origin[2] * 8);
	if (pm.step && step > 0 && step < 320 && pm.state.pmFlags & PMF_ON_GROUND) {
		if (cg.realTime - cg.predicted.stepTime < 150)
			oldStep = cg.predicted.step * (150 - (cg.realTime - cg.predicted.stepTime)) * (1.0f / 150.0f);
		else
			oldStep = 0;

		cg.predicted.step = oldStep + step * (1.0f/8.0f);
		cg.predicted.stepTime = cg.realTime - cg.netFrameTime * 500;
	}

	Vec3Scale (pm.state.velocity, (1.0f/8.0f), cg.predicted.velocity);
	Vec3Scale (pm.state.origin, (1.0f/8.0f), cg.predicted.origin);
	Vec3Copy (pm.viewAngles, cg.predicted.angles);
}
