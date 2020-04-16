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
// cg_entities.c
//

#include "cg_local.h"

cgEntity_t		cg_entityList[MAX_CS_EDICTS];
entityState_t	cg_parseEntities[MAX_PARSE_ENTITIES];

static qBool	cg_inFrameSequence = qFalse;

/*
==========================================================================

	ENTITY STATE

==========================================================================
*/

/*
==================
CG_BeginFrameSequence
==================
*/
void CG_BeginFrameSequence (frame_t frame)
{
	if (cg_inFrameSequence) {
		Com_Error (ERR_DROP, "CG_BeginFrameSequence: already in sequence");
		return;
	}

	cg.oldFrame = cg.frame;
	cg.frame = frame;

	cg_inFrameSequence = qTrue;
}


/*
==================
CG_NewPacketEntityState
==================
*/
void CG_NewPacketEntityState (int entNum, entityState_t state)
{
	cgEntity_t		*ent;

	if (!cg_inFrameSequence)
		Com_Error (ERR_DROP, "CG_NewPacketEntityState: no sequence");

	ent = &cg_entityList[entNum];
	cg_parseEntities[(cg.frame.parseEntities+cg.frame.numEntities) & (MAX_PARSEENTITIES_MASK)] = state;
	cg.frame.numEntities++;

	// Some data changes will force no lerping
	if (state.modelIndex != ent->current.modelIndex
	|| state.modelIndex2 != ent->current.modelIndex2
	|| state.modelIndex3 != ent->current.modelIndex3
	|| state.modelIndex4 != ent->current.modelIndex4
	|| abs ((int)state.origin[0] - (int)ent->current.origin[0]) > 512
	|| abs ((int)state.origin[1] - (int)ent->current.origin[1]) > 512
	|| abs ((int)state.origin[2] - (int)ent->current.origin[2]) > 512
	|| state.event == EV_PLAYER_TELEPORT
	|| state.event == EV_OTHER_TELEPORT)
		ent->serverFrame = -99;

	if (ent->serverFrame != cg.frame.serverFrame - 1) {
		// Wasn't in last update, so initialize some things duplicate
		// the current state so lerping doesn't hurt anything
		ent->prev = state;
		if (state.event == EV_OTHER_TELEPORT) {
			Vec3Copy (state.origin, ent->prev.origin);
			Vec3Copy (state.origin, ent->lerpOrigin);
		}
		else {
			Vec3Copy (state.oldOrigin, ent->prev.origin);
			Vec3Copy (state.oldOrigin, ent->lerpOrigin);
		}
	}
	else {
		// Shuffle the last state to previous
		ent->prev = ent->current;
	}

	ent->serverFrame = cg.frame.serverFrame;
	ent->current = state;
}


/*
==============
CG_EntityEvent

An entity has just been parsed that has an event value
the female events are there for backwards compatability
==============
*/
// this here is ugly and hacky, will be scripted soon
enum {
	SURF_NORMAL			= 1 << 10,	// 0x400

	SURF_CONCRETE		= 1 << 11,	// 0x800
	SURF_DIRT			= 1 << 12,	// 0x1000
	SURF_DUCT			= 1 << 13,	// 0x2000
	SURF_GRASS			= 1 << 14,	// 0x4000
	SURF_GRAVEL			= 1 << 15,	// 0x8000
	SURF_METAL			= 1 << 16,	// 0x10000
	SURF_METALGRATE		= 1 << 17,	// 0x20000
	SURF_METALLADDER	= 1 << 18,	// 0x40000
	SURF_MUD			= 1 << 19,	// 0x80000
	SURF_SAND			= 1 << 20,	// 0x100000
	SURF_SLOSH			= 1 << 21,	// 0x200000
	SURF_SNOW			= 1 << 22,	// 0x400000
	SURF_TILE			= 1 << 23,	// 0x800000
	SURF_WADE			= 1 << 24,	// 0x1000000
	SURF_WOOD			= 1 << 25,	// 0x2000000
	SURF_WOODPANEL		= 1 << 26	// 0x4000000
	// 0x8000000
	// 0x10000000
	// 0x20000000
	// 0x40000000
	// 0x80000000
};
#define SURF_MAXFLAGS (SURF_NORMAL-1)

enum {
	STEP_NORMAL,

	STEP_CONCRETE,
	STEP_DIRT,
	STEP_DUCT,
	STEP_GRASS,
	STEP_GRAVEL,
	STEP_METAL,
	STEP_METALGRATE,
	STEP_METALLADDER,
	STEP_MUD,
	STEP_SAND,
	STEP_SLOSH,
	STEP_SNOW,
	STEP_TILE,
	STEP_WADE,
	STEP_WOOD,
	STEP_WOODPANEL
};

int CG_StepTypeForTexture (cBspSurface_t *surf)
{
//	int		surfflags;
	char	newName[16];
	int		type;

	// some maps have UPPERCASE TEXTURE NAMES
	strcpy (newName, surf->name);
	Q_strlwr (newName);

	// this will be done after map load
//	surfflags = surf->flags;
//	if (surfflags > SURF_MAXFLAGS)
//		surfflags = SURF_MAXFLAGS;

	if (strstr (newName, "/dirt")) {
		type = STEP_DIRT;
	}
	else if (strstr (newName, "/mud")) {
		type = STEP_MUD;
	}
	else if (strstr (newName, "/cindr5_2")) {
		type = STEP_CONCRETE;
	}
	else if (strstr (newName, "/grass")) {
		type = STEP_GRASS;
	}
	else if (strstr (newName, "/c_met")
	|| strstr (newName, "/florr")
	|| strstr (newName, "/stairs")
	|| strstr (newName, "/rmetal")
	|| strstr (newName, "/blum")
	|| strstr (newName, "/metal")
	|| strstr (newName, "/floor3_1")
	|| strstr (newName, "/floor3_2")
	|| strstr (newName, "/floor3_3")
	|| strstr (newName, "/bflor3_1")
	|| strstr (newName, "/bflor3_2")
	|| strstr (newName, "/grate")
	|| strstr (newName, "/grnx")
	|| strstr (newName, "/grill")) {
		type = STEP_METAL;
	}
	else if (strstr (newName, "/rock")
	|| strstr (newName, "/rrock")) {
		type = STEP_GRAVEL;
	}
	else if (strstr (newName, "/airduc")) {
		type = STEP_DUCT;
	}
	else {
	//	Com_Printf (0, "normal: ");
		type = STEP_NORMAL;
	}
	//Com_Printf (0, "%s\n", newName);

//	surfflags |= type;

	// strip the type out of the flags
//	type = surfflags &~ SURF_MAXFLAGS;

	return type;
}

static void CG_FootStep (entityState_t *ent)
{
	trace_t			tr;
	vec3_t			end;
	int				stepType;
	struct sfx_s	*sound;

	Vec3Set (end, ent->origin[0], ent->origin[1], ent->origin[2]-64);
	CG_PMTrace (&tr, ent->origin, NULL, NULL, end, qFalse, qTrue);

	if (!tr.surface || !tr.surface->name || !tr.surface->name[0]) {
		sound = cgMedia.sfx.steps.standard[rand () & 3];
	}
	else {
		stepType = CG_StepTypeForTexture (tr.surface);

		switch (stepType) {
		case STEP_CONCRETE:		sound = cgMedia.sfx.steps.concrete[rand () & 3];	break;
		case STEP_DIRT:			sound = cgMedia.sfx.steps.dirt[rand () & 3];		break;
		case STEP_DUCT:			sound = cgMedia.sfx.steps.duct[rand () & 3];		break;
		case STEP_GRASS:		sound = cgMedia.sfx.steps.grass[rand () & 3];		break;
		case STEP_GRAVEL:		sound = cgMedia.sfx.steps.gravel[rand () & 3];		break;
		case STEP_METAL:		sound = cgMedia.sfx.steps.metal[rand () & 3];		break;
		case STEP_METALGRATE:	sound = cgMedia.sfx.steps.metalGrate[rand () & 3];	break;
		case STEP_METALLADDER:	sound = cgMedia.sfx.steps.metalLadder[rand () & 3];	break;
		case STEP_MUD:			sound = cgMedia.sfx.steps.mud[rand () & 3];			break;
		case STEP_SAND:			sound = cgMedia.sfx.steps.sand[rand () & 3];		break;
		case STEP_SLOSH:		sound = cgMedia.sfx.steps.slosh[rand () & 3];		break;
		case STEP_SNOW:			sound = cgMedia.sfx.steps.snow[rand () % 6];		break;
		case STEP_TILE:			sound = cgMedia.sfx.steps.tile[rand () & 3];		break;
		case STEP_WADE:			sound = cgMedia.sfx.steps.wade[rand () & 3];		break;
		case STEP_WOOD:			sound = cgMedia.sfx.steps.wood[rand () & 3];		break;
		case STEP_WOODPANEL:	sound = cgMedia.sfx.steps.woodPanel[rand () & 3];	break;

		default:
		case STEP_NORMAL:
			sound = cgMedia.sfx.steps.standard[rand () & 3];
			break;
		}
	}

	cgi.Snd_StartSound (NULL, ent->number, CHAN_BODY, sound, 1.0f, ATTN_NORM, 0);
}
static void CG_EntityEvent (entityState_t *ent)
{
	switch (ent->event) {
	case EV_ITEM_RESPAWN:
		cgi.Snd_StartSound (NULL, ent->number, CHAN_WEAPON, cgMedia.sfx.itemRespawn, 1, ATTN_IDLE, 0);
		CG_ItemRespawnEffect (ent->origin);
		break;

	case EV_FOOTSTEP:
		if (cl_footsteps->intVal)
			CG_FootStep (ent);
		break;

	case EV_FALL:
		cgi.Snd_StartSound (NULL, ent->number, CHAN_AUTO, cgMedia.sfx.playerFall, 1, ATTN_NORM, 0);
		break;

	case EV_FALLSHORT:
		cgi.Snd_StartSound (NULL, ent->number, CHAN_AUTO, cgMedia.sfx.playerFallShort, 1, ATTN_NORM, 0);
		break;

	case EV_FALLFAR:
		cgi.Snd_StartSound (NULL, ent->number, CHAN_AUTO, cgMedia.sfx.playerFallFar, 1, ATTN_NORM, 0);
		break;

	case EV_PLAYER_TELEPORT:
		cgi.Snd_StartSound (NULL, ent->number, CHAN_WEAPON, cgMedia.sfx.playerTeleport, 1, ATTN_IDLE, 0);
		CG_TeleportParticles (ent->origin);
		break;

	case EV_NONE:
	default:
		break;
	}
}


/*
==================
CG_FireEntityEvents
==================
*/
static void CG_FireEntityEvents (void)
{
	entityState_t	*state;
	int				pNum;

	for (pNum=0 ; pNum<cg.frame.numEntities ; pNum++) {
		state = &cg_parseEntities[(cg.frame.parseEntities+pNum)&(MAX_PARSEENTITIES_MASK)];
		if (state->event)
			CG_EntityEvent (state);
	}
}


/*
==================
CG_EndFrameSequence
==================
*/
void CG_EndFrameSequence (int numEntities)
{
	if (!cg_inFrameSequence) {
		Com_Error (ERR_DROP, "CG_EndFrameSequence: no sequence started");
		return;
	}

	cg_inFrameSequence = qFalse;

	// Clamp time
	cg.netTime = clamp (cg.netTime, cg.frame.serverTime - 100, cg.frame.serverTime);
	cg.refreshTime = clamp (cg.refreshTime, cg.frame.serverTime - 100, cg.frame.serverTime);

	if (!cg.frame.valid)
		return;

	// Verify our data is valid
	if (cg.frame.numEntities != numEntities) {
		Com_Error (ERR_DROP, "CG_EndFrameSequence: bad sequence");
		return;
	}

	// Build a list of collision solids
	CG_BuildSolidList ();

	// Fire entity events
	CG_FireEntityEvents ();

	// Check for a prediction error
	if (!cl_predict->intVal || !(cg.frame.playerState.pMove.pmFlags & PMF_NO_PREDICTION))
		CG_CheckPredictionError ();
}

/*
==========================================================================

	INTERPOLATE BETWEEN FRAMES TO GET RENDERING PARMS

==========================================================================
*/

/*
===============
CG_AddPacketEntities
===============
*/
static float	cg_gloomFLightMins[3] = { -4, -4, -4 };
static float	cg_gloomFLightMaxs[3] = { 4, 4, 4 };
static void CG_AddEntityShells (refEntity_t *ent)
{
	// Double
	if (ent->flags & RF_SHELL_DOUBLE) {
		ent->skin = cgMedia.modelShellDouble;
		cgi.R_AddEntity (ent);
	}
	// Half-dam
	if (ent->flags & RF_SHELL_HALF_DAM) {
		ent->skin = cgMedia.modelShellHalfDam;
		cgi.R_AddEntity (ent);
	}

	// God mode
	if (ent->flags & RF_SHELL_RED && ent->flags & RF_SHELL_GREEN && ent->flags & RF_SHELL_BLUE) {
		ent->skin = cgMedia.modelShellGod;
		cgi.R_AddEntity (ent);
	}
	else {
		// Red
		if (ent->flags & RF_SHELL_RED) {
			ent->skin = cgMedia.modelShellRed;
			cgi.R_AddEntity (ent);
		}
		// Green
		if (ent->flags & RF_SHELL_GREEN) {
			ent->skin = cgMedia.modelShellGreen;
			cgi.R_AddEntity (ent);
		}
		// Blue
		if (ent->flags & RF_SHELL_BLUE) {
			ent->skin = cgMedia.modelShellBlue;
			cgi.R_AddEntity (ent);
		}
	}
}
void CG_AddPacketEntities (void)
{
	refEntity_t		ent;
	entityState_t	*state;
	clientInfo_t	*clInfo;
	cgEntity_t		*cent;
	vec3_t			autoRotate, angles;
	mat3x3_t		autoRotateAxis;
	int				i, pNum, autoAnim;
	uint32			effects;
	qBool			isSelf, isPred, isDrawn;
	uint32			delta;

	// bonus items rotate at a fixed rate
	Vec3Set (autoRotate, 0, AngleModf (cg.realTime * 0.1f), 0);
	Angles_Matrix3 (autoRotate, autoRotateAxis);

	autoAnim = cg.realTime / 1000;	// brush models can auto animate their frames

	memset (&ent, 0, sizeof (ent));

	cg.thirdPerson = qFalse;
	for (pNum=0 ; pNum<cg.frame.numEntities ; pNum++) {
		state = &cg_parseEntities[(cg.frame.parseEntities+pNum)&(MAX_PARSEENTITIES_MASK)];
		cent = &cg_entityList[state->number];

		effects = state->effects;
		ent.flags = state->renderFx;

		isSelf = isPred = qFalse;
		isDrawn = qTrue;

		// Set frame
		if (effects & EF_ANIM01)
			ent.frame = autoAnim & 1;
		else if (effects & EF_ANIM23)
			ent.frame = 2 + (autoAnim & 1);
		else if (effects & EF_ANIM_ALL)
			ent.frame = autoAnim;
		else if (effects & EF_ANIM_ALLFAST)
			ent.frame = cg.realTime / 100;
		else
			ent.frame = state->frame;
		ent.oldFrame = cent->prev.frame;

		// Check effects
		if (effects & EF_PENT) {
			effects &= ~EF_PENT;
			effects |= EF_COLOR_SHELL;
			ent.flags |= RF_SHELL_RED;
		}

		if (effects & EF_POWERSCREEN)
			ent.flags |= RF_SHELL_GREEN;

		if (effects & EF_QUAD) {
			effects &= ~EF_QUAD;
			effects |= EF_COLOR_SHELL;
			ent.flags |= RF_SHELL_BLUE;
		}

		if (effects & EF_DOUBLE) {
			effects &= ~EF_DOUBLE;
			effects |= EF_COLOR_SHELL;
			ent.flags |= RF_SHELL_DOUBLE;
		}

		if (effects & EF_HALF_DAMAGE) {
			effects &= ~EF_HALF_DAMAGE;
			effects |= EF_COLOR_SHELL;
			ent.flags |= RF_SHELL_HALF_DAM;
		}

		ent.backLerp = 1.0f - cg.lerpFrac;
		ent.scale = 1;
		Vec4Set (ent.color, 255, 255, 255, 255);

		// Is it me?
		if (state->number == cg.playerNum+1) {
			isSelf = qTrue;

			if (cl_predict->intVal
			&& !(cg.frame.playerState.pMove.pmFlags & PMF_NO_PREDICTION)
			&& cg.frame.playerState.pMove.pmType == PMT_NORMAL) {
				// Use prediction origins, add predicted.error since it seems to solve stutteryness on platforms
				ent.origin[0] = cg.predicted.origin[0] - ((1.0f-cg.lerpFrac) * cg.predicted.error[0]);
				ent.origin[1] = cg.predicted.origin[1] - ((1.0f-cg.lerpFrac) * cg.predicted.error[1]);
				ent.origin[2] = cg.predicted.origin[2] - ((1.0f-cg.lerpFrac) * cg.predicted.error[2]);

				// Smooth out stair climbing
				delta = cg.realTime - cg.predicted.stepTime;
				if (delta < 150)
					ent.origin[2] -= cg.predicted.step * (150 - delta) / 150;

				Vec3Copy (ent.origin, ent.oldOrigin);
				isPred = qTrue;
			}
		}

		if (!isPred) {
			if (ent.flags & (RF_FRAMELERP|RF_BEAM)) {
				// Step origin discretely, because the frames do the animation properly
				Vec3Copy (cent->current.origin, ent.origin);
				Vec3Copy (cent->current.oldOrigin, ent.oldOrigin);
			}
			else {
				// Interpolate origin
				ent.origin[0] = ent.oldOrigin[0] = cent->prev.origin[0] + cg.lerpFrac * (cent->current.origin[0] - cent->prev.origin[0]);
				ent.origin[1] = ent.oldOrigin[1] = cent->prev.origin[1] + cg.lerpFrac * (cent->current.origin[1] - cent->prev.origin[1]);
				ent.origin[2] = ent.oldOrigin[2] = cent->prev.origin[2] + cg.lerpFrac * (cent->current.origin[2] - cent->prev.origin[2]);
			}
		}
	
		// Tweak the color of beams
		if (ent.flags & RF_BEAM) {
			// The four beam colors are encoded in 32 bits of skinNum (hack)
			int		clr;
			vec3_t	length;

			clr = ((state->skinNum >> ((rand () % 4)*8)) & 0xff);

			if (rand () % 2)
				CG_BeamTrail (ent.origin, ent.oldOrigin,
					clr, (float)ent.frame,
					0.33f + (frand () * 0.2f), -1.0f / (5 + (frand () * 0.3f)));

			Vec3Subtract (ent.oldOrigin, ent.origin, length);

			CG_SpawnParticle (
				ent.origin[0],					ent.origin[1],					ent.origin[2],
				length[0],						length[1],						length[2],
				0,								0,								0,
				0,								0,								0,
				palRed (clr),					palGreen (clr),					palBlue (clr),
				palRed (clr),					palGreen (clr),					palBlue (clr),
				0.30f,							PART_INSTANT,
				ent.frame + ((ent.frame * 0.1f) * (rand () & 1)),
				ent.frame + ((ent.frame * 0.1f) * (rand () & 1)),
				PT_BEAM,						0,
				0,								qFalse,
				PART_STYLE_BEAM,
				0);

			goto done;
		}
		else {
			// Set skin
			if (state->modelIndex == 255) {
				// Use custom player skin
				ent.skinNum = 0;
				clInfo = &cg.clientInfo[state->skinNum & 0xff];
				ent.skin = clInfo->skin;
				ent.model = clInfo->model;
				if (!ent.skin || !ent.model) {
					ent.skin = cg.baseClientInfo.skin;
					ent.model = cg.baseClientInfo.model;
				}

				//PGM
				if (ent.flags & RF_USE_DISGUISE) {
					if (!Q_strnicmp ((char *)ent.skin, "players/male", 12)) {
						ent.skin = cgMedia.maleDisguiseSkin;
						ent.model = cgMedia.maleDisguiseModel;
					}
					else if (!Q_strnicmp ((char *)ent.skin, "players/female", 14)) {
						ent.skin = cgMedia.femaleDisguiseSkin;
						ent.model = cgMedia.femaleDisguiseModel;
					}
					else if (!Q_strnicmp ((char *)ent.skin, "players/cyborg", 14)) {
						ent.skin = cgMedia.cyborgDisguiseSkin;
						ent.model = cgMedia.cyborgDisguiseModel;
					}
				}
				//PGM
			}
			else {
				ent.skinNum = state->skinNum;
				ent.skin = NULL;
				ent.model = cg.modelCfgDraw[state->modelIndex];
			}
		}

		if (ent.model) {
			// Gloom-specific effects
			if (cg.currGameMod == GAME_MOD_GLOOM) {
				// Stinger fire/C4 debris
				if (!Q_stricmp ((char *)ent.model, "sprites/s_firea.sp2")
				|| !Q_stricmp ((char *)ent.model, "sprites/s_fireb.sp2")
				|| !Q_stricmp ((char *)ent.model, "sprites/s_flame.sp2")) {
					if (effects & EF_ROCKET) {
						// C4 debris
						CG_GloomEmberTrail (cent->lerpOrigin, ent.origin);
					}
					else if (glm_advstingfire->intVal) {
						// Stinger fire
						CG_GloomStingerFire (cent->lerpOrigin, ent.origin, 25 + (frand () * 15), qTrue);
					}

					// Skip the original lighting/trail effects
					if (effects & EF_ROCKET || glm_advstingfire->intVal)
						goto done;
				}

				// Bio flare
				else if ((effects & EF_ROCKET || !(effects & EF_BLASTER)) && !Q_stricmp ((char *)ent.model, "models/objects/laser/tris.md2")) {
					CG_GloomFlareTrail (cent->lerpOrigin, ent.origin);

					if (effects & EF_ROCKET) {
						effects &= ~EF_ROCKET;
						cgi.R_AddLight (ent.origin, 200, 0, 1, 0);
					}
				}

				// Blob model
				else if (!Q_stricmp ((char *)ent.model, "models/objects/tlaser/tris.md2")) {
					CG_GloomBlobTip (cent->lerpOrigin, ent.origin);
					isDrawn = qFalse;
				}

				// ST/Stinger gas
				else if (!Q_stricmp ((char *)ent.model, "models/objects/smokexp/tris.md2")) {
					if (glm_advgas->intVal) {
						CG_GloomGasEffect (ent.origin);
						goto done;
					}
				}

				// C4 explosion sprite
				else if (!Q_stricmp ((char *)ent.model, "models/objects/r_explode/tris.md2")) {	
					cgi.R_AddLight (ent.origin, 200.0f + (150*(ent.frame - 29)/36), 1.0f, 0.8f, 0.6f);
					goto done;
				}
				else if (!Q_stricmp ((char *)ent.model, "models/objects/r_explode/tris2.md2") ||
						!Q_stricmp ((char *)ent.model, "models/objects/explode/tris.md2")) {
					// Just don't draw this crappy looking crap
					goto done;
				}
			}

			// Xatrix-specific effects
			else if (cg.currGameMod == GAME_MOD_XATRIX) {
				// Ugly phalanx tip
				if (!Q_stricmp ((char *)ent.model, "sprites/s_photon.sp2")) {
					CG_PhalanxTip (cent->lerpOrigin, ent.origin);
					isDrawn = qFalse;
				}
			}

			// Ugly model-based blaster tip
			if (!Q_stricmp ((char *)ent.model, "models/objects/laser/tris.md2")) {
				CG_BlasterTip (cent->lerpOrigin, ent.origin);
				isDrawn = qFalse;
			}

			// Don't draw the BFG sprite
			if (effects & EF_BFG && Q_WildcardMatch ("sprites/s_bfg*.sp2", (char *)ent.model, 1))
				isDrawn = qFalse;
		}

		// Generically translucent
		if (ent.flags & RF_TRANSLUCENT)
			ent.color[3] = 255 * 0.70f;

		// Calculate angles
		if (effects & EF_ROTATE) {
			// Some bonus items auto-rotate
			Matrix3_Copy (autoRotateAxis, ent.axis);
		}
		else if (effects & EF_SPINNINGLIGHTS) {
			vec3_t	forward;
			vec3_t	start;

			angles[0] = 0;
			angles[1] = AngleModf (cg.realTime/2.0f) + state->angles[1];
			angles[2] = 180;

			Angles_Matrix3 (angles, ent.axis);

			Angles_Vectors (angles, forward, NULL, NULL);
			Vec3MA (ent.origin, 64, forward, start);
			cgi.R_AddLight (start, 100, 1, 0, 0);
		}
		else {
			if (isPred) {
				if (cg.predicted.angles[PITCH] > 180)
					angles[PITCH] = (-360 + cg.predicted.angles[PITCH]) * 0.333f;
				else
					angles[PITCH] = cg.predicted.angles[PITCH] * 0.333f;

				angles[YAW] = cg.predicted.angles[YAW];
				angles[ROLL] = cg.predicted.angles[ROLL];
			}
			else {
				angles[0] = LerpAngle (cent->prev.angles[0], cent->current.angles[0], cg.lerpFrac);
				angles[1] = LerpAngle (cent->prev.angles[1], cent->current.angles[1], cg.lerpFrac);
				angles[2] = LerpAngle (cent->prev.angles[2], cent->current.angles[2], cg.lerpFrac);
			}

			if (angles[0] || angles[1] || angles[2])
				Angles_Matrix3 (angles, ent.axis);
			else
				Matrix3_Identity (ent.axis);
		}

		// Flip your shadow around for lefty
		if (isSelf && hand->intVal == 1) {
			ent.flags |= RF_CULLHACK;
			Vec3Negate (ent.axis[1], ent.axis[1]);
		}

		// If set to invisible, skip
		if (!state->modelIndex)
			goto done;

		if (effects & EF_BFG) {
			ent.flags |= RF_TRANSLUCENT;
			ent.color[3] = 255 * 0.30f;
		}

		// RAFAEL
		if (effects & EF_PLASMA) {
			ent.flags |= RF_TRANSLUCENT;
			ent.color[3] = 255 * 0.6f;
		}

		if (effects & EF_SPHERETRANS) {
			ent.flags |= RF_TRANSLUCENT;

			// PMM - *sigh*  yet more EF overloading
			if (effects & EF_TRACKERTRAIL)
				ent.color[3] = 255 * 0.6f;
			else
				ent.color[3] = 255 * 0.3f;
		}

		// Some items dont deserve a shadow
		if (effects & (EF_GIB|EF_GREENGIB|EF_GRENADE|EF_ROCKET|EF_BLASTER|EF_HYPERBLASTER|EF_BLUEHYPERBLASTER))
			ent.flags |= RF_NOSHADOW;

		// Hackish mod handling for shells
		if (effects & EF_COLOR_SHELL) {
			if (ent.flags & RF_SHELL_HALF_DAM) {
				if (cg.currGameMod == GAME_MOD_ROGUE) {
					if (ent.flags & (RF_SHELL_RED|RF_SHELL_BLUE|RF_SHELL_DOUBLE))
						ent.flags &= ~RF_SHELL_HALF_DAM;
				}
			}

			if (ent.flags & RF_SHELL_DOUBLE) {
				if (cg.currGameMod == GAME_MOD_ROGUE) {
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

		// Check for third person
		if (isSelf) {
			if (cg_thirdPerson->intVal && cl_predict->intVal && !(cg.frame.playerState.pMove.pmFlags & PMF_NO_PREDICTION) && !cg.attractLoop) {
				cg.thirdPerson = (state->modelIndex && cg.frame.playerState.pMove.pmType != PMT_SPECTATOR);
			}
			else {
				cg.thirdPerson = qFalse;
			}

			if (cg.thirdPerson && cg.cameraTrans > 0) {
				ent.color[3] = cg.cameraTrans;
				if (cg.cameraTrans < 255)
					ent.flags |= RF_TRANSLUCENT;
			}
			else {
				ent.flags |= RF_VIEWERMODEL;
			}
		}

		// Force entity colors for these flags
		if (cg.refDef.rdFlags & RDF_IRGOGGLES && ent.flags & RF_IR_VISIBLE) {
			ent.flags |= RF_FULLBRIGHT;
			Vec3Set (ent.color, 255, 0, 0);
		}
		else if (cg.refDef.rdFlags & RDF_UVGOGGLES) {
			ent.flags |= RF_FULLBRIGHT;
			Vec3Set (ent.color, 0, 255, 0);
		}

		// Add lights to shells
		if (effects & EF_COLOR_SHELL) {
			if (ent.flags & RF_SHELL_RED) {
				ent.flags |= RF_MINLIGHT;
				cgi.R_AddLight (ent.origin, 50, 1, 0, 0);
			}
			if (ent.flags & RF_SHELL_GREEN) {
				ent.flags |= RF_MINLIGHT;
				cgi.R_AddLight (ent.origin, 50, 0, 1, 0);
			}
			if (ent.flags & RF_SHELL_BLUE) {
				ent.flags |= RF_MINLIGHT;
				cgi.R_AddLight (ent.origin, 50, 0, 0, 1);
			}
			if (ent.flags & RF_SHELL_DOUBLE) {
				ent.flags |= RF_MINLIGHT;
				cgi.R_AddLight (ent.origin, 50, 0.9f, 0.7f, 0);
			}
			if (ent.flags & RF_SHELL_HALF_DAM) {
				ent.flags |= RF_MINLIGHT;
				cgi.R_AddLight (ent.origin, 50, 0.56f, 0.59f, 0.45f);
			}
		}

		// Add to refresh list
		if (isDrawn) {
			cgi.R_AddEntity (&ent);

			// Add entity shell(s)
			if (effects & EF_COLOR_SHELL) {
				ent.skinNum = 0;
				CG_AddEntityShells (&ent);
			}
		}

		// Linked models
		if (state->modelIndex2) {
			ent.skin = NULL;	// Never use a custom skin on others
			ent.skinNum = 0;

			if (state->modelIndex2 == 255) {
				// Custom weapon
				clInfo = &cg.clientInfo[state->skinNum & 0xff];
				i = (state->skinNum >> 8);	// 0 is default weapon model
				if (!cl_vwep->intVal || i > MAX_CLIENTWEAPONMODELS-1)
					i = 0;

				ent.model = clInfo->weaponModels[i];
				if (!ent.model) {
					if (i)
						ent.model = clInfo->weaponModels[0];
					if (!ent.model)
						ent.model = cg.baseClientInfo.weaponModels[0];
				}
			}
			else {
				ent.model = cg.modelCfgDraw[state->modelIndex2];
			}

			// PMM - check for the defender sphere shell .. make it translucent
			// replaces the previous version which used the high bit on modelIndex2 to determine transparency
			if (!Q_stricmp (cg.configStrings[CS_MODELS+(state->modelIndex2)], "models/items/shell/tris.md2")) {
				ent.flags |= RF_TRANSLUCENT;
				ent.color[3] = 255 * 0.32f;
			}
			// pmm

			if (isDrawn) {
				cgi.R_AddEntity (&ent);

				// Add entity shell(s)
				if (effects & EF_COLOR_SHELL) {
					ent.skinNum = 0;
					CG_AddEntityShells (&ent);
				}
			}
		}

		if (state->modelIndex3) {
			ent.skin = NULL;	// Never use a custom skin on others
			ent.skinNum = 0;
			ent.model = cg.modelCfgDraw[state->modelIndex3];

			if (isDrawn) {
				cgi.R_AddEntity (&ent);

				// Add entity shell(s)
				if (effects & EF_COLOR_SHELL) {
					ent.skinNum = 0;
					CG_AddEntityShells (&ent);
				}
			}
		}

		if (state->modelIndex4) {
			ent.skin = NULL;	// Never use a custom skin on others
			ent.skinNum = 0;
			ent.model = cg.modelCfgDraw[state->modelIndex4];

			if (isDrawn) {
				cgi.R_AddEntity (&ent);

				// Add entity shell(s)
				if (effects & EF_COLOR_SHELL) {
					ent.skinNum = 0;
					CG_AddEntityShells (&ent);
				}
			}
		}

		// EF_POWERSCREEN shield
		if (effects & EF_POWERSCREEN) {
			ent.skin = NULL;	// Never use a custom skin on others
			ent.skinNum = 0;
			ent.model = cgMedia.powerScreenModel;
			ent.oldFrame = 0;
			ent.frame = 0;
			ent.flags |= RF_TRANSLUCENT|RF_FULLBRIGHT;
			ent.color[0] = ent.color[2] = 0;
			ent.color[1] = 255;
			ent.color[3] = 0.3f * 255;
			cgi.R_AddEntity (&ent);
		}

		// EF_TELEPORTER acts like an event, but is not cleared each frame
		// moved so it doesn't stutter on packet loss
		if (effects & EF_TELEPORTER)
			CG_TeleporterParticles (state);

		// add automatic particle trails
		if (effects &~ EF_ROTATE) {
			if (effects & EF_ROCKET) {
				CG_RocketTrail (cent->lerpOrigin, ent.origin);
				cgi.R_AddLight (ent.origin, 200, 1, 1, 0.6f);
			}
			// PGM - Do not reorder EF_BLASTER and EF_HYPERBLASTER
			else if (effects & EF_BLASTER) {
				//PGM
				// EF_BLASTER | EF_TRACKER is a special case for EF_BLASTER2... Cheese!
				if (effects & EF_TRACKER) {
					CG_BlasterGreenTrail (cent->lerpOrigin, ent.origin);
					cgi.R_AddLight (ent.origin, 200, 0, 1, 0);		
				}
				else {
					CG_BlasterGoldTrail (cent->lerpOrigin, ent.origin);
					cgi.R_AddLight (ent.origin, 200, 1, 1, 0);
				}
				//PGM
			}
			else if (effects & EF_HYPERBLASTER) {
				if (effects & EF_TRACKER)						// PGM	overloaded for blaster2.
					cgi.R_AddLight (ent.origin, 200, 0, 1, 0);		// PGM
				else {
					// See if it is a Gloom flashlight
					if (ent.model && cg.currGameMod == GAME_MOD_GLOOM && !Q_stricmp ((char *)ent.model, "sprites/s_shine.sp2")) {
						static int	flPredLastTime = -1;
						qBool		flPred;
						vec3_t		flOrg;

						// Flashlight prediction
						switch (cg.gloomClassType) {
							default:
							case GLM_DEFAULT:

							case GLM_OBSERVER:
							case GLM_BREEDER:
							case GLM_HATCHLING:
							case GLM_DRONE:
							case GLM_WRAITH:
							case GLM_KAMIKAZE:
							case GLM_STINGER:
							case GLM_GUARDIAN:
							case GLM_STALKER:
								flPred = qFalse;
								break;

							case GLM_ENGINEER:
							case GLM_GRUNT:
							case GLM_ST:
							case GLM_BIOTECH:
							case GLM_HT:
							case GLM_COMMANDO:
							case GLM_EXTERM:
							case GLM_MECH:
								flPred = qTrue;
								break;
						}
						if (glm_flashpred->intVal && flPred && flPredLastTime != cg.realTime) {
							playerStateNew_t	*ps;
							vec3_t				org, forward;
							trace_t				tr;

							ps = &cg.frame.playerState; // Calc server side player origin
							org[0] = ps->pMove.origin[0] * (1.0f/8.0f);
							org[1] = ps->pMove.origin[1] * (1.0f/8.0f);
							org[2] = ps->pMove.origin[2] * (1.0f/8.0f);

							// Project our own flashlight forward
							Angles_Vectors (ps->viewAngles, forward, NULL, NULL);
							Vec3Scale (forward, 2048, forward);
							Vec3Add (forward, org, forward);
							tr = cgi.CM_BoxTrace (org, forward, cg_gloomFLightMins, cg_gloomFLightMaxs, 0,
												CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_DEADMONSTER);

							// Check if it's close
							Vec3Subtract (tr.endPos, ent.origin, forward); 
							if (Vec3LengthFast (forward) > 256) {
								Vec3Copy (ent.origin, flOrg);
							}
							else {
								flPredLastTime = cg.realTime;

								// Project a flashlight
								Vec3Scale (cg.refDef.viewAxis[0], 2048, forward);
								Vec3Add (forward, cg.refDef.viewOrigin, forward);
								tr = cgi.CM_BoxTrace (cg.refDef.viewOrigin, forward, cg_gloomFLightMins, cg_gloomFLightMaxs, 0,
													CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_DEADMONSTER);

								Vec3Copy (tr.endPos, flOrg);
							}
						}
						else
							Vec3Copy (ent.origin, flOrg);

						// White flashlight?
						if (glm_flwhite->intVal)
							cgi.R_AddLight (flOrg, 200, 1, 1, 1);
						else
							cgi.R_AddLight (flOrg, 200, 1, 1, 0);
					}
					else
						cgi.R_AddLight (ent.origin, 200, 1, 1, 0);
				}
			}
			else if (effects & EF_GIB)
				CG_GibTrail (cent->lerpOrigin, ent.origin, EF_GIB);
			else if (effects & EF_GRENADE)
				CG_GrenadeTrail (cent->lerpOrigin, ent.origin);
			else if (effects & EF_FLIES)
				CG_FlyEffect (cent, ent.origin);
			else if (effects & EF_BFG) {
				if (effects & EF_ANIM_ALLFAST) {
					// flying
					CG_BfgTrail (&ent);
					i = 200;
				}
				else {
					// explosion
					static const int BFG_BrightRamp[6] = { 300, 400, 600, 300, 150, 75 };
					i = BFG_BrightRamp[state->frame%6];
				}

				cgi.R_AddLight (ent.origin, (float)i, 0, 1, 0);
			// RAFAEL
			}
			else if (effects & EF_TRAP) {
				ent.origin[2] += 32;
				CG_TrapParticles (&ent);
				i = ((int)frand () * 100) + 100;
				cgi.R_AddLight (ent.origin, (float)i, 1, 0.8f, 0.1f);
			}
			else if (effects & EF_FLAG1) {
				CG_FlagTrail (cent->lerpOrigin, ent.origin, EF_FLAG1);
				cgi.R_AddLight (ent.origin, 225, 1, 0.1f, 0.1f);
			}
			else if (effects & EF_FLAG2) {
				CG_FlagTrail (cent->lerpOrigin, ent.origin, EF_FLAG2);
				cgi.R_AddLight (ent.origin, 225, 0.1f, 0.1f, 1);

			//ROGUE
			}
			else if (effects & EF_TAGTRAIL) {
				CG_TagTrail (cent->lerpOrigin, ent.origin);
				cgi.R_AddLight (ent.origin, 225, 1.0, 1.0, 0.0);
			}
			else if (effects & EF_TRACKERTRAIL) {
				if (effects & EF_TRACKER) {
					float intensity;

					intensity = 50 + (500 * ((float)sin (cg.realTime/500.0f) + 1.0f));
					cgi.R_AddLight (ent.origin, intensity, -1.0, -1.0, -1.0);
				}
				else {
					CG_TrackerShell (cent->lerpOrigin);
					cgi.R_AddLight (ent.origin, 155, -1.0, -1.0, -1.0);
				}
			}
			else if (effects & EF_TRACKER) {
				CG_TrackerTrail (cent->lerpOrigin, ent.origin);
				cgi.R_AddLight (ent.origin, 200, -1, -1, -1);
			//ROGUE

			// RAFAEL
			}
			else if (effects & EF_GREENGIB)
				CG_GibTrail (cent->lerpOrigin, ent.origin, EF_GREENGIB);
			// RAFAEL

			else if (effects & EF_IONRIPPER) {
				CG_IonripperTrail (cent->lerpOrigin, ent.origin);
				if (cg.currGameMod == GAME_MOD_GLOOM)
					cgi.R_AddLight (ent.origin, 100, 0.3f, 1, 0.3f);
				else
					cgi.R_AddLight (ent.origin, 100, 1, 0.5f, 0.5f);
			}

			// RAFAEL
			else if (effects & EF_BLUEHYPERBLASTER)
				cgi.R_AddLight (ent.origin, 200, 0, 0, 1);
			// RAFAEL

			else if (effects & EF_PLASMA) {
				if (effects & EF_ANIM_ALLFAST)
					CG_BlasterGoldTrail (cent->lerpOrigin, ent.origin);

				cgi.R_AddLight (ent.origin, 130, 1, 0.5, 0.5);
			}
		}
done:
		if (cent->muzzleOn) {
			cent->muzzleOn = qFalse;
			cent->muzzType = -1;
			cent->muzzSilenced = qFalse;
			cent->muzzVWeap = qFalse;
		}
		Vec3Copy (ent.origin, cent->lerpOrigin);
	}
}


/*
================
CG_AddEntities
================
*/
void CG_AddEntities (void)
{
	CG_AddViewWeapon ();
	CG_AddPacketEntities ();
	CG_AddTempEnts ();
	CG_AddLocalEnts ();
	CG_AddDLights ();
	CG_AddLightStyles ();
	CG_AddDecals ();
	CG_AddParticles ();
}


/*
==============
CG_ClearEntities
==============
*/
void CG_ClearEntities (void)
{
	memset (cg_entityList, 0, sizeof (cg_entityList));
	memset (cg_parseEntities, 0, sizeof (cg_parseEntities));

	CG_ClearTempEnts ();
	CG_ClearLocalEnts ();
	CG_ClearDLights ();
	CG_ClearLightStyles ();
	CG_ClearDecals ();
	CG_ClearParticles ();
}


/*
===============
CG_GetEntitySoundOrigin

Called to get the sound spatialization origin, so that interpolated origins are used.
===============
*/
void CG_GetEntitySoundOrigin (int entNum, vec3_t origin, vec3_t velocity)
{
	cgEntity_t	*ent;

	if (entNum < 0 || entNum >= MAX_CS_EDICTS)
		Com_Error (ERR_DROP, "CG_GetEntitySoundOrigin: bad entNum");

	ent = &cg_entityList[entNum];

	if (ent->current.renderFx & (RF_FRAMELERP|RF_BEAM)) {
		origin[0] = ent->current.oldOrigin[0] + (ent->current.origin[0] - ent->current.oldOrigin[0]) * cg.lerpFrac;
		origin[1] = ent->current.oldOrigin[1] + (ent->current.origin[1] - ent->current.oldOrigin[1]) * cg.lerpFrac;
		origin[2] = ent->current.oldOrigin[2] + (ent->current.origin[2] - ent->current.oldOrigin[2]) * cg.lerpFrac;

		Vec3Subtract (ent->current.origin, ent->current.oldOrigin, velocity);
		Vec3Scale (velocity, 10, velocity);
	}
	else {
		origin[0] = ent->prev.origin[0] + (ent->current.origin[0] - ent->prev.origin[0]) * cg.lerpFrac;
		origin[1] = ent->prev.origin[1] + (ent->current.origin[1] - ent->prev.origin[1]) * cg.lerpFrac;
		origin[2] = ent->prev.origin[2] + (ent->current.origin[2] - ent->prev.origin[2]) * cg.lerpFrac;

		Vec3Subtract (ent->current.origin, ent->prev.origin, velocity);
		Vec3Scale (velocity, 10, velocity);
	}

	if (ent->current.solid == 31) {
		struct cBspModel_s	*cModel;
		vec3_t				mins, maxs;

		cModel = cgi.CM_InlineModel (cg.configStrings[CS_MODELS+ent->current.modelIndex]);
		if (!cModel)
			return;

		cgi.CM_InlineModelBounds (cModel, mins, maxs);

		origin[0] += 0.5f * (mins[0] + maxs[0]);
		origin[1] += 0.5f * (mins[1] + maxs[1]);
		origin[2] += 0.5f * (mins[2] + maxs[2]);
	}
}
