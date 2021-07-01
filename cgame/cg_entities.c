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

cgEntity_t		cgEntities[MAX_CS_EDICTS];
entityState_t	cgParseEntities[MAX_PARSE_ENTITIES];

static qBool	cgInFrameSequence = qFalse;

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
void CG_BeginFrameSequence (frame_t frame) {
	if (cgInFrameSequence) {
		Com_Error (ERR_DROP, "CG_BeginFrameSequence: already in sequence");
		return;
	}

	cg.oldFrame = cg.frame;
	cg.frame = frame;

	VectorScale (cg.frame.playerState.pMove.origin, ONEDIV8, cg.predictedOrigin);
	VectorCopy (cg.frame.playerState.viewAngles, cg.predictedAngles);

	cgInFrameSequence = qTrue;
}


/*
==================
CG_NewPacketEntityState
==================
*/
void CG_NewPacketEntityState (int entnum, entityState_t state) {
	cgEntity_t		*ent;

	if (!cgInFrameSequence)
		Com_Error (ERR_DROP, "CG_NewPacketEntityState: no sequence");

	ent = &cgEntities[entnum];
	cgParseEntities[(cg.frame.parseEntities+cg.frame.numEntities) & (MAX_PARSEENTITIES_MASK)] = state;
	cg.frame.numEntities++;

	// some data changes will force no lerping
	if ((state.modelIndex != ent->current.modelIndex)			||
		(state.modelIndex2 != ent->current.modelIndex2)			||
		(state.modelIndex3 != ent->current.modelIndex3)			||
		(state.modelIndex4 != ent->current.modelIndex4)			||
		(abs (state.origin[0] - ent->current.origin[0]) > 512)	||
		(abs (state.origin[1] - ent->current.origin[1]) > 512)	||
		(abs (state.origin[2] - ent->current.origin[2]) > 512)	||
		(state.event == EV_PLAYER_TELEPORT)						||
		(state.event == EV_OTHER_TELEPORT))
			ent->serverFrame = -99;

	if (ent->serverFrame != cg.frame.serverFrame - 1) {
		// wasn't in last update, so initialize some things duplicate
		// the current state so lerping doesn't hurt anything
		ent->prev = state;
		if (state.event == EV_OTHER_TELEPORT) {
			VectorCopy (state.origin, ent->prev.origin);
			VectorCopy (state.origin, ent->lerpOrigin);
		} else {
			VectorCopy (state.oldOrigin, ent->prev.origin);
			VectorCopy (state.oldOrigin, ent->lerpOrigin);
		}
	} else {
		// shuffle the last state to previous
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
static void CG_EntityEvent (entityState_t *ent) {
	switch (ent->event) {
	case EV_ITEM_RESPAWN:
		cgi.Snd_StartSound (NULL, ent->number, CHAN_WEAPON, cgMedia.itemRespawnSfx, 1, ATTN_IDLE, 0);
		CG_ItemRespawnEffect (ent->origin);
		break;

	case EV_FOOTSTEP:
		if (cl_footsteps->value)
			cgi.Snd_StartSound (NULL, ent->number, CHAN_BODY, cgMedia.footstepSfx[rand()%4], 1, ATTN_NORM, 0);
		break;

	case EV_FALL:
		cgi.Snd_StartSound (NULL, ent->number, CHAN_AUTO, cgMedia.playerFallSfx, 1, ATTN_NORM, 0);
		break;

	case EV_FALLSHORT:
		cgi.Snd_StartSound (NULL, ent->number, CHAN_AUTO, cgMedia.playerFallShortSfx, 1, ATTN_NORM, 0);
		break;

	case EV_FALLFAR:
		cgi.Snd_StartSound (NULL, ent->number, CHAN_AUTO, cgMedia.playerFallFarSfx, 1, ATTN_NORM, 0);
		break;

	case EV_PLAYER_TELEPORT:
		cgi.Snd_StartSound (NULL, ent->number, CHAN_WEAPON, cgMedia.playerTeleport, 1, ATTN_IDLE, 0);
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
static void CG_FireEntityEvents (void) {
	entityState_t	*s1;
	int				pNum;

	for (pNum=0 ; pNum<cg.frame.numEntities ; pNum++) {
		s1 = &cgParseEntities[(cg.frame.parseEntities+pNum)&(MAX_PARSEENTITIES_MASK)];
		if (s1->event)
			CG_EntityEvent (s1);

		// EF_TELEPORTER acts like an event, but is not cleared each frame
		if (s1->effects & EF_TELEPORTER)
			CG_TeleporterParticles (s1);
	}
}


/*
==================
CG_EndFrameSequence
==================
*/
void CG_EndFrameSequence (int numEntities) {
	if (!cgInFrameSequence) {
		Com_Error (ERR_DROP, "CG_EndFrameSequence: no sequence started");
		return;
	}

	cgInFrameSequence = qFalse;

	// clamp time
	cg.time = clamp (cg.time, cg.frame.serverTime - 100, cg.frame.serverTime);

	if (!cg.frame.valid)
		return;

	// verify our data is valid
	if (cg.frame.numEntities != numEntities) {
		Com_Error (ERR_DROP, "CG_EndFrameSequence: bad sequence");
		return;
	}

	// fire entity events
	CG_FireEntityEvents ();

	// check for a prediction error
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
static float flightminmaxs[6] = {-4, -4, -4, 4, 4, 4};
static void AddClientFlashlight (vec3_t origin) {
	vec3_t	forward;
	trace_t	tr;

	VectorScale (cg.v_Forward, 2048, forward);
	VectorAdd (forward, cg.refDef.viewOrigin, forward);
	tr = cgi.CM_BoxTrace (cg.refDef.viewOrigin, forward, flightminmaxs, flightminmaxs + 3, 0,
						CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_DEADMONSTER);

	VectorCopy (tr.endPos, origin);
}
void CG_AddPacketEntities (void) {
	entity_t			ent;
	entityState_t		*s1;
	clientInfo_t		*ci;
	cgEntity_t			*cent;
	vec3_t				autoRotate, angles;
	vec3_t				autoRotateAxis[3];
	int					i, pNum, autoanim;
	uInt				effects;
	qBool				isSelf, isPred, isDrawn;

	// bonus items rotate at a fixed rate
	VectorSet (autoRotate, 0, AngleMod (cgs.realTime * 0.1), 0);
	AnglesToAxis (autoRotate, autoRotateAxis);

	autoanim = cgs.realTime * 0.001;								// brush models can auto animate their frames

	memset (&ent, 0, sizeof (ent));

	for (pNum=0 ; pNum<cg.frame.numEntities ; pNum++) {
		s1 = &cgParseEntities[(cg.frame.parseEntities+pNum)&(MAX_PARSEENTITIES_MASK)];

		cent = &cgEntities[s1->number];

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
			ent.frame = cgs.realTime / 100;
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

		ent.backLerp = 1.0 - cg.lerpFrac;
		ent.scale = 1;
		Vector4Set (ent.color, 1, 1, 1, 1);

		// is it me?
		if (s1->number == cg.playerNum+1) {
			isSelf = qTrue;

			if (cl_predict->value && !(cg.frame.playerState.pMove.pmFlags & PMF_NO_PREDICTION)) {
				VectorCopy (cg.predictedOrigin, ent.origin);
				VectorCopy (cg.predictedOrigin, ent.oldOrigin);
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
					ent.origin[i] = ent.oldOrigin[i] = cent->prev.origin[i] + cg.lerpFrac * 
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
				CG_BeamTrail (s1->origin, s1->oldOrigin,
					clr, ent.frame,
					0.33 + (frand () * 0.2), -1.0 / (5 + (frand () * 0.3)));

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
				ci = &cg.clientInfo[s1->skinNum & 0xff];
				ent.skin = ci->skin;
				ent.model = ci->model;
				if (!ent.skin || !ent.model) {
					ent.skin = cg.baseClientInfo.skin;
					ent.model = cg.baseClientInfo.model;
				}

				//PGM
				if (ent.flags & RF_USE_DISGUISE) {
					if (!Q_strnicmp ((char *)ent.skin, "players/male", 12)) {
						ent.skin = cgi.Img_RegisterSkin ("players/male/disguise.pcx");
						ent.model = cgi.Mod_RegisterModel ("players/male/tris.md2");
					} else if (!Q_strnicmp ((char *)ent.skin, "players/female", 14)) {
						ent.skin = cgi.Img_RegisterSkin ("players/female/disguise.pcx");
						ent.model = cgi.Mod_RegisterModel ("players/female/tris.md2");
					} else if (!Q_strnicmp ((char *)ent.skin, "players/cyborg", 14)) {
						ent.skin = cgi.Img_RegisterSkin ("players/cyborg/disguise.pcx");
						ent.model = cgi.Mod_RegisterModel ("players/cyborg/tris.md2");
					}
				}
				//PGM
			} else {
				ent.skinNum = s1->skinNum;
				ent.skin = NULL;
				ent.model = cg.modelDraw[s1->modelIndex];
			}
		}

		if (ent.model) {
			// gloom-specific effects
			if (cgi.FS_CurrGame ("gloom")) {
				// stinger fire/C4 debris
				if (!Q_stricmp ((char *)ent.model, "sprites/s_firea.sp2") ||
					!Q_stricmp ((char *)ent.model, "sprites/s_fireb.sp2") ||
					!Q_stricmp ((char *)ent.model, "sprites/s_flame.sp2")) {
					if (effects & EF_ROCKET) {
						// C4 debris
						CG_GloomEmberTrail (cent->lerpOrigin, ent.origin);
					} else if (glm_advstingfire->integer) {
						// stinger fire
						CG_GloomStingerFire (cent->lerpOrigin, ent.origin, 25 + (frand () * 15), qTrue);
					}

					// skip the original lighting/trail effects
					if ((effects & EF_ROCKET) || glm_advstingfire->integer)
						goto done;
				}

				// bio flare
				else if (!Q_stricmp ((char *)ent.model, "models/objects/laser/tris.md2")) {
					if ((effects & EF_ROCKET) || !(effects & EF_BLASTER)) {
						CG_GloomFlareTrail (cent->lerpOrigin, ent.origin);

						if (effects & EF_ROCKET) {
							effects &= ~EF_ROCKET;
							cgi.R_AddLight (ent.origin, 200, 0, 1, 0);
						}
					}
				}

				// blob model
				else if (!Q_stricmp ((char *)ent.model, "models/objects/tlaser/tris.md2")) {
					CG_GloomBlobTip (cent->lerpOrigin, ent.origin);
					isDrawn = qFalse;
				}

				// st/stinger gas
				else if (!Q_stricmp ((char *)ent.model, "models/objects/smokexp/tris.md2")) {
					if (glm_advgas->integer) {
						CG_GloomGasEffect (ent.origin);
						goto done;
					}
				}

				// c4 explosion sprite
				else if (!Q_stricmp ((char *)ent.model, "models/objects/r_explode/tris.md2")) {	
					cgi.R_AddLight (ent.origin, 200 + (150*(ent.frame - 29)/36), 1, 0.8, 0.6);
					goto done;
				}
				else if (!Q_stricmp ((char *)ent.model, "models/objects/r_explode/tris2.md2") ||
						!Q_stricmp ((char *)ent.model, "models/objects/explode/tris.md2")) {
					// just don't draw this crappy looking crap
					goto done;
				}
			}

			// xatrix-specific effects
			else if (cgi.FS_CurrGame ("xatrix")) {
				// ugly phalanx tip
				if (!Q_stricmp ((char *)ent.model, "sprites/s_photon.sp2")) {
					CG_PhalanxTip (cent->lerpOrigin, ent.origin);
					isDrawn = qFalse;
				}
			}

			// ugly model-based blaster tip
			if (!Q_stricmp ((char *)ent.model, "models/objects/laser/tris.md2")) {
				CG_BlasterTip (cent->lerpOrigin, ent.origin);
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
			angles[1] = AngleMod (cgs.realTime/2.0) + s1->angles[1];
			angles[2] = 180;

			AnglesToAxis (angles, ent.axis);

			AngleVectors (angles, forward, NULL, NULL);
			VectorMA (ent.origin, 64, forward, start);
			cgi.R_AddLight (start, 100, 1, 0, 0);
		} else {
			// interpolate angles
			float	a1, a2;

			for (i=0 ; i<3 ; i++) {
				a1 = cent->current.angles[i];
				a2 = cent->prev.angles[i];
				angles[i] = InterpolateAngle (a2, a1, cg.lerpFrac);
			}

			if (angles[0] || angles[1] || angles[2])
				AnglesToAxis (angles, ent.axis);
			else
				Matrix_Identity (ent.axis);
		}

		// flip your shadow around for lefty
		if (isSelf && (hand->integer == 1)) {
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
				if (cgi.FS_CurrGame ("rogue")) {
					if (ent.flags & (RF_SHELL_RED|RF_SHELL_BLUE|RF_SHELL_DOUBLE))
						ent.flags &= ~RF_SHELL_HALF_DAM;
				}
			}

			if (ent.flags & RF_SHELL_DOUBLE) {
				if (cgi.FS_CurrGame ("rogue")) {
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
		if ((cg.refDef.rdFlags & RDF_IRGOGGLES) && (ent.flags & RF_IR_VISIBLE)) {
			if (effects & EF_COLOR_SHELL)
				cgi.R_AddLight (ent.origin, 50, 1, 0, 0);

			if (!(ent.flags & RF_FULLBRIGHT))
				ent.flags |= RF_FULLBRIGHT;
			VectorSet (ent.color, 1, 0, 0);
		} else if (cg.refDef.rdFlags & RDF_UVGOGGLES) {
			if (effects & EF_COLOR_SHELL)
				cgi.R_AddLight (ent.origin, 50, 0, 1, 0);

			if (!(ent.flags & RF_FULLBRIGHT))
				ent.flags |= RF_FULLBRIGHT;
			VectorSet (ent.color, 0, 1, 0);
		} else {
			if (effects & EF_COLOR_SHELL) {
				if (ent.flags & RF_SHELL_RED)
					cgi.R_AddLight (ent.origin, 50, 1, 0, 0);
				if (ent.flags & RF_SHELL_GREEN)
					cgi.R_AddLight (ent.origin, 50, 0, 1, 0);
				if (ent.flags & RF_SHELL_BLUE)
					cgi.R_AddLight (ent.origin, 50, 0, 0, 1);
				if (ent.flags & RF_SHELL_DOUBLE)
					cgi.R_AddLight (ent.origin, 50, 0.9, 0.7, 0);
				if (ent.flags & RF_SHELL_HALF_DAM)
					cgi.R_AddLight (ent.origin, 50, 0.56, 0.59, 0.45);
			}
		}

		// add to refresh list
		if (isDrawn)
			cgi.R_AddEntity (&ent);

		ent.skin = NULL;	// never use a custom skin on others
		ent.skinNum = 0;

		// duplicate for linked models
		if (s1->modelIndex2) {
			if (s1->modelIndex2 == 255) {
				// custom weapon
				ci = &cg.clientInfo[s1->skinNum & 0xff];
				i = (s1->skinNum >> 8);	// 0 is default weapon model
				if (!cl_vwep->integer || (i > MAX_CLIENTWEAPONMODELS - 1))
					i = 0;

				ent.model = ci->weaponModel[i];
				if (!ent.model) {
					if (i != 0)
						ent.model = ci->weaponModel[0];
					if (!ent.model)
						ent.model = cg.baseClientInfo.weaponModel[0];
				}
			} else
				ent.model = cg.modelDraw[s1->modelIndex2];

			// PMM - check for the defender sphere shell .. make it translucent
			// replaces the previous version which used the high bit on modelIndex2 to determine transparency
			if (!Q_stricmp (cg.configStrings[CS_MODELS+(s1->modelIndex2)], "models/items/shell/tris.md2")) {
				if (!(ent.flags & RF_TRANSLUCENT))
					ent.flags |= RF_TRANSLUCENT;
				ent.color[3] = 0.32;
			}
			// pmm

			if (isDrawn)
				cgi.R_AddEntity (&ent);
		}

		if (s1->modelIndex3) {
			ent.model = cg.modelDraw[s1->modelIndex3];

			if (isDrawn)
				cgi.R_AddEntity (&ent);
		}

		if (s1->modelIndex4) {
			ent.model = cg.modelDraw[s1->modelIndex4];

			if (isDrawn)
				cgi.R_AddEntity (&ent);
		}

		// add automatic particle trails
		if (effects &~ EF_ROTATE) {
			if (effects & EF_ROCKET) {
				CG_RocketTrail (cent->lerpOrigin, ent.origin);
				cgi.R_AddLight (ent.origin, 200, 1, 1, 0.6);
			}
			// PGM - Do not reorder EF_BLASTER and EF_HYPERBLASTER
			else if (effects & EF_BLASTER) {
				//PGM
				// EF_BLASTER | EF_TRACKER is a special case for EF_BLASTER2... Cheese!
				if (effects & EF_TRACKER) {
					CG_BlasterGreenTrail (cent->lerpOrigin, ent.origin);
					cgi.R_AddLight (ent.origin, 200, 0, 1, 0);		
				} else {
					CG_BlasterGoldTrail (cent->lerpOrigin, ent.origin);
					cgi.R_AddLight (ent.origin, 200, 1, 1, 0);
				}
				//PGM
			} else if (effects & EF_HYPERBLASTER) {
				if (effects & EF_TRACKER)						// PGM	overloaded for blaster2.
					cgi.R_AddLight (ent.origin, 200, 0, 1, 0);		// PGM
				else {
					if (ent.model && cgi.FS_CurrGame ("gloom")) {
						if (!Q_stricmp ((char *)ent.model, "sprites/s_shine.sp2")) {
							static int	lasttime = -1;
							vec3_t		florigin;

							if (glm_flashpred->integer && (lasttime != cgs.realTime)) {
								vec3_t			org, forward;
								trace_t			tr;
								playerState_t	*ps;

								ps = &cg.frame.playerState; //calc server side player origin
								org[0] = ps->pMove.origin[0] * ONEDIV8;
								org[1] = ps->pMove.origin[1] * ONEDIV8;
								org[2] = ps->pMove.origin[2] * ONEDIV8;

								// get server side player viewangles
								AngleVectors (ps->viewAngles, forward, NULL, NULL);
								VectorScale (forward, 2048, forward);
								VectorAdd (forward, org, forward);
								tr = cgi.CM_BoxTrace (org, forward, flightminmaxs, flightminmaxs+3, 0,
													CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_DEADMONSTER);
								VectorSubtract (tr.endPos, ent.origin, forward); 

								if (VectorLengthFast (forward) > 256)
									VectorCopy (ent.origin, florigin);
								else {
									lasttime = cgs.realTime;
									AddClientFlashlight (florigin);
								}
							} else
								VectorCopy (ent.origin, florigin);

							if (glm_flwhite->integer)
								cgi.R_AddLight (florigin, 200, 1, 1, 1);
							else
								cgi.R_AddLight (florigin, 200, 1, 1, 0);
						} else
							cgi.R_AddLight (ent.origin, 200, 1, 1, 0);
					} else
						cgi.R_AddLight (ent.origin, 200, 1, 1, 0);
				}
			} else if (effects & EF_GIB)
				CG_GibTrail (cent->lerpOrigin, ent.origin, EF_GIB);
			else if (effects & EF_GRENADE)
				CG_GrenadeTrail (cent->lerpOrigin, ent.origin);
			else if (effects & EF_FLIES)
				CG_FlyEffect (cent, ent.origin);
			else if (effects & EF_BFG) {
				// flying
				if (effects & EF_ANIM_ALLFAST) {
					CG_BfgTrail (&ent);
					i = 200;
				// explosion
				} else {
					static int BFG_BrightRamp[6] = { 300, 400, 600, 300, 150, 75 };
					i = BFG_BrightRamp[s1->frame%6];
				}

				cgi.R_AddLight (ent.origin, i, 0, 1, 0);
			// RAFAEL
			} else if (effects & EF_TRAP) {
				ent.origin[2] += 32;
				CG_TrapParticles (&ent);
				i = (frand () * 100) + 100;
				cgi.R_AddLight (ent.origin, i, 1, 0.8, 0.1);
			} else if (effects & EF_FLAG1) {
				CG_FlagTrail (cent->lerpOrigin, ent.origin, EF_FLAG1);
				cgi.R_AddLight (ent.origin, 225, 1, 0.1, 0.1);
			} else if (effects & EF_FLAG2) {
				CG_FlagTrail (cent->lerpOrigin, ent.origin, EF_FLAG2);
				cgi.R_AddLight (ent.origin, 225, 0.1, 0.1, 1);

			//ROGUE
			} else if (effects & EF_TAGTRAIL) {
				CG_TagTrail (cent->lerpOrigin, ent.origin);
				cgi.R_AddLight (ent.origin, 225, 1.0, 1.0, 0.0);
			} else if (effects & EF_TRACKERTRAIL) {
				if (effects & EF_TRACKER) {
					float intensity;

					intensity = 50 + (500 * (sin (cgs.realTime/500.0) + 1.0));
					cgi.R_AddLight (ent.origin, intensity, -1.0, -1.0, -1.0);
				} else {
					CG_TrackerShell (cent->lerpOrigin);
					cgi.R_AddLight (ent.origin, 155, -1.0, -1.0, -1.0);
				}
			} else if (effects & EF_TRACKER) {
				CG_TrackerTrail (cent->lerpOrigin, ent.origin);
				cgi.R_AddLight (ent.origin, 200, -1, -1, -1);
			//ROGUE

			// RAFAEL
			} else if (effects & EF_GREENGIB)
				CG_GibTrail (cent->lerpOrigin, ent.origin, EF_GREENGIB);
			// RAFAEL

			else if (effects & EF_IONRIPPER) {
				CG_IonripperTrail (cent->lerpOrigin, ent.origin);
				if (cgi.FS_CurrGame ("gloom"))
					cgi.R_AddLight (ent.origin, 100, 0.3, 1, 0.3);
				else
					cgi.R_AddLight (ent.origin, 100, 1, 0.5, 0.5);
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
		VectorCopy (ent.origin, cent->lerpOrigin);
	}
}


/*
================
CG_AddEntities
================
*/
void CG_AddEntities (void) {
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
void CG_ClearEntities (void) {
	memset (cgEntities, 0, sizeof (cgEntities));
	memset (cgParseEntities, 0, sizeof (cgParseEntities));

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

Called to get the sound spatialization origin
===============
*/
void CG_GetEntitySoundOrigin (int entNum, vec3_t org) {
	cgEntity_t		*ent;

	if ((entNum < 0) || (entNum >= MAX_CS_EDICTS))
		Com_Error (ERR_DROP, "CG_GetEntitySoundOrigin: bad entNum");

	ent = &cgEntities[entNum];
	VectorCopy (ent->lerpOrigin, org);

	// FIXME: bmodel issues...
}
