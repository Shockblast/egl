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

static int				cgGunFrame;
static struct model_s	*cgGunModel;

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
	entity_t		gun;
	entityState_t	*s1;
	playerState_t	*ps, *ops;
	cgEntity_t		*ent;
	vec3_t			angles;
	int				i, pnum;

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

	if (cgGunModel) {
		// development tool
		gun.model = cgGunModel;
	} else
		gun.model = cg.modelDraw[ps->gunIndex];

	if (!gun.model)
		return;

	// set up gun position
	for (i=0 ; i<3 ; i++) {
		gun.origin[i] = cg.refDef.viewOrigin[i] + ops->gunOffset[i] + cg.lerpFrac * (ps->gunOffset[i] - ops->gunOffset[i]);
		angles[i] = cg.refDef.viewAngles[i] + InterpolateAngle (ops->gunAngles[i], ps->gunAngles[i], cg.lerpFrac);
	}

	if (angles[0] || angles[1] || angles[2])
		AnglesToAxis (angles, gun.axis);
	else
		Matrix_Identity (gun.axis);

	if (cgGunFrame) {
		// development tool
		gun.frame = cgGunFrame;
		gun.oldFrame = cgGunFrame;
	} else {
		gun.frame = ps->gunFrame;
		// just changed weapons, don't lerp from old
		if (gun.frame == 0)
			gun.oldFrame = 0;
		else
			gun.oldFrame = ops->gunFrame;
	}

	ent = &cgEntities[cg.playerNum+1];
	gun.flags = RF_MINLIGHT|RF_DEPTHHACK|RF_WEAPONMODEL;
	for (pnum = 0 ; pnum<cg.frame.numEntities ; pnum++) {
		s1 = &cgParseEntities[(cg.frame.parseEntities+pnum)&(MAX_PARSEENTITIES_MASK)];
		if (s1->number != cg.playerNum + 1)
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
	gun.backLerp = 1.0 - cg.lerpFrac;
	VectorCopy (gun.origin, gun.oldOrigin);	// don't lerp origins at all
	Vector4Set (gun.color, 1, 1, 1, 1);

	if (hand->integer == 1) {
		gun.flags |= RF_CULLHACK;
		VectorNegate (gun.axis[1], gun.axis[1]);
	}

	cgi.R_AddEntity (&gun);
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
	Com_Printf (0, "Gun frame %i\n", ++cgGunFrame);
}


/*
=============
CG_Gun_FramePrev_f
=============
*/
static void CG_Gun_FramePrev_f (void) {
	if (--cgGunFrame < 0)
		cgGunFrame = 0;
	Com_Printf (0, "Gun frame %i\n", cgGunFrame);
}


/*
=============
CG_Gun_Model_f
=============
*/
static void CG_Gun_Model_f (void) {
	char	name[MAX_QPATH];

	if (cgi.Cmd_Argc () != 2) {
		cgGunModel = NULL;
		return;
	}
	
	Q_snprintfz (name, sizeof (name), "models/%s/tris.md2", cgi.Cmd_Argv (1));
	cgGunModel = cgi.Mod_RegisterModel (name);
}

/*
=======================================================================

	INIT / SHUTDOWN

=======================================================================
*/

/*
=============
CG_WeapRegister
=============
*/
void CG_WeapRegister (void) {
	cgi.Cmd_AddCommand ("gun_next",		CG_Gun_FrameNext_f,	"Increments view weapon frame number");
	cgi.Cmd_AddCommand ("gun_prev",		CG_Gun_FramePrev_f,	"Decrements view weapon frame number");
	cgi.Cmd_AddCommand ("gun_model",	CG_Gun_Model_f,		"Changes the view weapon model");
}


/*
=============
CG_WeapUnregister
=============
*/
void CG_WeapUnregister (void) {
	cgi.Cmd_RemoveCommand ("gun_next");
	cgi.Cmd_RemoveCommand ("gun_prev");
	cgi.Cmd_RemoveCommand ("gun_model");
}
