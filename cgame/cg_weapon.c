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
// cg_weapon.c
// View weapon stuff
//

#include "cg_local.h"

static int				cg_gunFrame;
static struct model_s	*cg_gunModel;

/*
=======================================================================

	VIEW WEAPON

=======================================================================
*/

/*
==============
CG_AddViewWeapon
==============
*/
void CG_AddViewWeapon (void) {
	entity_t			gun;
	entityState_t		*state;
	playerStateNew_t	*ps, *ops;
	cgEntity_t			*ent;
	vec3_t				angles;
	int					pnum;

	// don't draw the weapon in third person
	if (cg.thirdPerson)
		return;

	// allow the gun to be completely removed
	if (!cl_gun->integer || (hand->integer == 2))
		return;

	// find the previous frame to interpolate from
	ps = &cg.frame.playerState;
	if ((cg.oldFrame.serverFrame != cg.frame.serverFrame-1) || !cg.oldFrame.valid)
		ops = &cg.frame.playerState;		// previous frame was dropped or invalid
	else
		ops = &cg.oldFrame.playerState;

	memset (&gun, 0, sizeof (gun));

	if (cg_gunModel) {
		// development tool
		gun.model = cg_gunModel;
	}
	else
		gun.model = cg.modelDraw[ps->gunIndex];

	if (!gun.model)
		return;

	// set up gun position
	gun.origin[0] = cg.refDef.viewOrigin[0] + ops->gunOffset[0] + cg.lerpFrac * (ps->gunOffset[0] - ops->gunOffset[0]);
	gun.origin[1] = cg.refDef.viewOrigin[1] + ops->gunOffset[1] + cg.lerpFrac * (ps->gunOffset[1] - ops->gunOffset[1]);
	gun.origin[2] = cg.refDef.viewOrigin[2] + ops->gunOffset[2] + cg.lerpFrac * (ps->gunOffset[2] - ops->gunOffset[2]);

	angles[0] = cg.refDef.viewAngles[0] + InterpolateAngle (ops->gunAngles[0], ps->gunAngles[0], cg.lerpFrac);
	angles[1] = cg.refDef.viewAngles[1] + InterpolateAngle (ops->gunAngles[1], ps->gunAngles[1], cg.lerpFrac);
	angles[2] = cg.refDef.viewAngles[2] + InterpolateAngle (ops->gunAngles[2], ps->gunAngles[2], cg.lerpFrac);

	AnglesToAxis (angles, gun.axis);

	if (cg_gunFrame) {
		// development tool
		gun.frame = cg_gunFrame;
		gun.oldFrame = cg_gunFrame;
	}
	else {
		gun.frame = ps->gunFrame;
		// just changed weapons, don't lerp from old
		if (gun.frame == 0)
			gun.oldFrame = 0;
		else
			gun.oldFrame = ops->gunFrame;
	}

	ent = &cg_entityList[cg.playerNum+1];
	gun.flags = RF_MINLIGHT|RF_DEPTHHACK|RF_WEAPONMODEL;
	for (pnum = 0 ; pnum<cg.frame.numEntities ; pnum++) {
		state = &cg_parseEntities[(cg.frame.parseEntities+pnum)&(MAX_PARSEENTITIES_MASK)];
		if (state->number != cg.playerNum + 1)
			continue;

		gun.flags |= state->renderFx;
		if (state->effects & EF_PENT)
			gun.flags |= RF_SHELL_RED;
		if (state->effects & EF_QUAD)
			gun.flags |= RF_SHELL_BLUE;
		if (state->effects & EF_DOUBLE)
			gun.flags |= RF_SHELL_DOUBLE;
		if (state->effects & EF_HALF_DAMAGE)
			gun.flags |= RF_SHELL_HALF_DAM;

		if (state->renderFx & RF_TRANSLUCENT)
			gun.color[3] = 255 * 0.70f;

		if (state->effects & EF_BFG) {
			gun.flags |= RF_TRANSLUCENT;
			gun.color[3] = 255 * 0.30f;
		}

		if (state->effects & EF_PLASMA) {
			gun.flags |= RF_TRANSLUCENT;
			gun.color[3] = 255 * 0.6f;
		}

		if (state->effects & EF_SPHERETRANS) {
			gun.flags |= RF_TRANSLUCENT;

			if (state->effects & EF_TRACKERTRAIL)
				gun.color[3] = 255 * 0.6f;
			else
				gun.color[3] = 255 * 0.3f;
		}
		break;
	}

	gun.scale = 1.0f;
	gun.backLerp = 1.0f - cg.lerpFrac;
	VectorCopy (gun.origin, gun.oldOrigin);	// don't lerp origins at all
	Vector4Set (gun.color, 255, 255, 255, 255);

	if (hand->integer == 1) {
		gun.flags |= RF_CULLHACK;
		VectorNegate (gun.axis[1], gun.axis[1]);
	}

	cgi.R_AddEntity (&gun);

	gun.skinNum = 0;
	if (gun.flags & RF_SHELLMASK) {
		if (gun.flags & RF_SHELL_DOUBLE) {
			gun.skin = cgMedia.modelShellDouble;
			cgi.R_AddEntity (&gun);
		}
		if (gun.flags & RF_SHELL_HALF_DAM) {
			gun.skin = cgMedia.modelShellHalfDam;
			cgi.R_AddEntity (&gun);
		}

		if (gun.flags & RF_SHELL_RED && gun.flags & RF_SHELL_GREEN && gun.flags & RF_SHELL_BLUE) {
			gun.skin = cgMedia.modelShellGod;
			cgi.R_AddEntity (&gun);
		}
		else {
			if (gun.flags & RF_SHELL_RED) {
				gun.skin = cgMedia.modelShellRed;
				cgi.R_AddEntity (&gun);
			}
			if (gun.flags & RF_SHELL_GREEN) {
				gun.skin = cgMedia.modelShellGreen;
				cgi.R_AddEntity (&gun);
			}
			if (gun.flags & RF_SHELL_BLUE) {
				gun.skin = cgMedia.modelShellBlue;
				cgi.R_AddEntity (&gun);
			}
		}
	}
}

/*
=======================================================================

	CONSOLE FUNCTIONS

=======================================================================
*/

/*
=============
CG_Gun_FrameNext_f
=============
*/
static void CG_Gun_FrameNext_f (void) {
	Com_Printf (0, "Gun frame %i\n", ++cg_gunFrame);
}


/*
=============
CG_Gun_FramePrev_f
=============
*/
static void CG_Gun_FramePrev_f (void) {
	if (--cg_gunFrame < 0)
		cg_gunFrame = 0;
	Com_Printf (0, "Gun frame %i\n", cg_gunFrame);
}


/*
=============
CG_Gun_Model_f
=============
*/
static void CG_Gun_Model_f (void) {
	char	name[MAX_QPATH];

	if (CGI_Cmd_Argc () != 2) {
		cg_gunModel = NULL;
		return;
	}
	
	Q_snprintfz (name, sizeof (name), "models/%s/tris.md2", CGI_Cmd_Argv (1));
	cg_gunModel = cgi.R_RegisterModel (name);
}

/*
=======================================================================

	INIT / SHUTDOWN

=======================================================================
*/

static cmdFunc_t	*cmd_gunNext;
static cmdFunc_t	*cmd_gunPrev;
static cmdFunc_t	*cmd_gunModel;

/*
=============
CG_WeapRegister
=============
*/
void CG_WeapRegister (void) {
	cmd_gunNext		= CGI_Cmd_AddCommand ("gun_next",		CG_Gun_FrameNext_f,	"Increments view weapon frame number");
	cmd_gunPrev		= CGI_Cmd_AddCommand ("gun_prev",		CG_Gun_FramePrev_f,	"Decrements view weapon frame number");
	cmd_gunModel	= CGI_Cmd_AddCommand ("gun_model",	CG_Gun_Model_f,		"Changes the view weapon model");
}


/*
=============
CG_WeapUnregister
=============
*/
void CG_WeapUnregister (void) {
	CGI_Cmd_RemoveCommand ("gun_next", cmd_gunNext);
	CGI_Cmd_RemoveCommand ("gun_prev", cmd_gunPrev);
	CGI_Cmd_RemoveCommand ("gun_model", cmd_gunModel);
}
