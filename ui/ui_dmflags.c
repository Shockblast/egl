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

#include "ui_local.h"

/*
=============================================================================

	DMFLAGS MENU

=============================================================================
*/

typedef struct mDMFlagsMenu_s {
	uiFrameWork_t	framework;

	uiImage_t		banner;

	uiAction_t		header;

	uiList_t		friendlyfire_toggle;
	uiList_t		falldmg_toggle;
	uiList_t		weapons_stay_toggle;
	uiList_t		instant_powerups_toggle;
	uiList_t		powerups_toggle;
	uiList_t		health_toggle;
	uiList_t		farthest_toggle;
	uiList_t		teamplay_toggle;
	uiList_t		samelevel_toggle;
	uiList_t		force_respawn_toggle;
	uiList_t		armor_toggle;
	uiList_t		allow_exit_toggle;
	uiList_t		infinite_ammo_toggle;
	uiList_t		fixed_fov_toggle;
	uiList_t		quad_drop_toggle;

	uiAction_t		rogue_header;

	uiList_t		rogue_no_mines_toggle;
	uiList_t		rogue_no_nukes_toggle;
	uiList_t		rogue_stack_double_toggle;
	uiList_t		rogue_no_spheres_toggle;

	uiAction_t		back_action;
} mDMFlagsMenu_t;

mDMFlagsMenu_t	mDMFlagsMenu;
static char		mDMFlagsStatusbar[128];

static void DMFlagCallback (void *self) {
	uiList_t	*f = (uiList_t *) self;
	int			flags, bit = 0;

	flags = uii.Cvar_VariableInteger ("dmflags");

	if (f == &mDMFlagsMenu.friendlyfire_toggle) {
		if (f->curValue)
			flags &= ~DF_NO_FRIENDLY_FIRE;
		else
			flags |= DF_NO_FRIENDLY_FIRE;
		goto setvalue;
	}
	else if (f == &mDMFlagsMenu.falldmg_toggle) {
		if (f->curValue)
			flags &= ~DF_NO_FALLING;
		else
			flags |= DF_NO_FALLING;
		goto setvalue;
	}
	else if (f == &mDMFlagsMenu.weapons_stay_toggle)
		bit = DF_WEAPONS_STAY;
	else if (f == &mDMFlagsMenu.instant_powerups_toggle)
		bit = DF_INSTANT_ITEMS;
	else if (f == &mDMFlagsMenu.allow_exit_toggle)
		bit = DF_ALLOW_EXIT;
	else if (f == &mDMFlagsMenu.powerups_toggle) {
		if (f->curValue)
			flags &= ~DF_NO_ITEMS;
		else
			flags |= DF_NO_ITEMS;
		goto setvalue;
	}
	else if (f == &mDMFlagsMenu.health_toggle) {
		if (f->curValue)
			flags &= ~DF_NO_HEALTH;
		else
			flags |= DF_NO_HEALTH;
		goto setvalue;
	}
	else if (f == &mDMFlagsMenu.farthest_toggle)
		bit = DF_SPAWN_FARTHEST;
	else if (f == &mDMFlagsMenu.teamplay_toggle) {
		if (f->curValue == 1) {
			flags |=  DF_SKINTEAMS;
			flags &= ~DF_MODELTEAMS;
		}
		else if (f->curValue == 2) {
			flags |=  DF_MODELTEAMS;
			flags &= ~DF_SKINTEAMS;
		}
		else
			flags &= ~(DF_MODELTEAMS | DF_SKINTEAMS);

		goto setvalue;
	}
	else if (f == &mDMFlagsMenu.samelevel_toggle)
		bit = DF_SAME_LEVEL;
	else if (f == &mDMFlagsMenu.force_respawn_toggle)
		bit = DF_FORCE_RESPAWN;
	else if (f == &mDMFlagsMenu.armor_toggle) {
		if (f->curValue)
			flags &= ~DF_NO_ARMOR;
		else
			flags |= DF_NO_ARMOR;
		goto setvalue;
	}
	else if (f == &mDMFlagsMenu.infinite_ammo_toggle)
		bit = DF_INFINITE_AMMO;
	else if (f == &mDMFlagsMenu.fixed_fov_toggle)
		bit = DF_FIXED_FOV;
	else if (f == &mDMFlagsMenu.quad_drop_toggle)
		bit = DF_QUAD_DROP;
	else if (uii.FS_CurrGame ("rogue")) {
		if (f == &mDMFlagsMenu.rogue_no_mines_toggle)
			bit = DF_NO_MINES;
		else if (f == &mDMFlagsMenu.rogue_no_nukes_toggle)
			bit = DF_NO_NUKES;
		else if (f == &mDMFlagsMenu.rogue_stack_double_toggle)
			bit = DF_NO_STACK_DOUBLE;
		else if (f == &mDMFlagsMenu.rogue_no_spheres_toggle)
			bit = DF_NO_SPHERES;
	}

	if (f) {
		if (f->curValue == 0)
			flags &= ~bit;
		else
			flags |= bit;
	}

setvalue:
	uii.Cvar_SetValue ("dmflags", flags);

	Q_snprintfz (mDMFlagsStatusbar, sizeof (mDMFlagsStatusbar), "dmflags = %d", flags);
}


/*
=============
DMFlagsMenu_SetValues
=============
*/
static void DMFlagsMenu_SetValues (void) {
	int		dmflags = uii.Cvar_VariableInteger ("dmflags");

	mDMFlagsMenu.falldmg_toggle.curValue			= !(dmflags & DF_NO_FALLING);
	mDMFlagsMenu.weapons_stay_toggle.curValue		= !!(dmflags & DF_WEAPONS_STAY);
	mDMFlagsMenu.instant_powerups_toggle.curValue	= !!(dmflags & DF_INSTANT_ITEMS);
	mDMFlagsMenu.powerups_toggle.curValue			= !(dmflags & DF_NO_ITEMS);
	mDMFlagsMenu.health_toggle.curValue				= !(dmflags & DF_NO_HEALTH);
	mDMFlagsMenu.armor_toggle.curValue				= !(dmflags & DF_NO_ARMOR);
	mDMFlagsMenu.farthest_toggle.curValue			= !!(dmflags & DF_SPAWN_FARTHEST);
	mDMFlagsMenu.samelevel_toggle.curValue			= !!(dmflags & DF_SAME_LEVEL);
	mDMFlagsMenu.force_respawn_toggle.curValue		= !!(dmflags & DF_FORCE_RESPAWN);
	mDMFlagsMenu.allow_exit_toggle.curValue			= !!(dmflags & DF_ALLOW_EXIT);
	mDMFlagsMenu.infinite_ammo_toggle.curValue		= !!(dmflags & DF_INFINITE_AMMO);
	mDMFlagsMenu.fixed_fov_toggle.curValue			= !!(dmflags & DF_FIXED_FOV);
	mDMFlagsMenu.quad_drop_toggle.curValue			= !!(dmflags & DF_QUAD_DROP);
	mDMFlagsMenu.friendlyfire_toggle.curValue		= !(dmflags & DF_NO_FRIENDLY_FIRE);

	if (uii.FS_CurrGame ("rogue")) {
		mDMFlagsMenu.rogue_no_mines_toggle.curValue		= !!(dmflags & DF_NO_MINES);
		mDMFlagsMenu.rogue_no_nukes_toggle.curValue		= !!(dmflags & DF_NO_NUKES);
		mDMFlagsMenu.rogue_stack_double_toggle.curValue	= !!(dmflags & DF_NO_STACK_DOUBLE);
		mDMFlagsMenu.rogue_no_spheres_toggle.curValue	= !!(dmflags & DF_NO_SPHERES);
	}
}


/*
=============
DMFlagsMenu_Init
=============
*/
void DMFlagsMenu_Init (void) {
	static char *yesno_names[] = {
		"no",
		"yes",
		0
	};
	static char *teamplay_names[] = {
		"disabled",
		"by skin",
		"by model",
		0
	};

	float	y;

	mDMFlagsMenu.framework.x			= uis.vidWidth * 0.50;
	mDMFlagsMenu.framework.y			= 0;
	mDMFlagsMenu.framework.numItems		= 0;

	UI_Banner (&mDMFlagsMenu.banner, uiMedia.banners.startServer, &y);

	mDMFlagsMenu.header.generic.type		= UITYPE_ACTION;
	mDMFlagsMenu.header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	mDMFlagsMenu.header.generic.x			= 0;
	mDMFlagsMenu.header.generic.y			= y += UIFT_SIZEINC;
	mDMFlagsMenu.header.generic.name		= "Deathmatch Flags";

	mDMFlagsMenu.falldmg_toggle.generic.type		= UITYPE_SPINCONTROL;
	mDMFlagsMenu.falldmg_toggle.generic.x			= 0;
	mDMFlagsMenu.falldmg_toggle.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	mDMFlagsMenu.falldmg_toggle.generic.name		= "Falling damage";
	mDMFlagsMenu.falldmg_toggle.generic.callBack	= DMFlagCallback;
	mDMFlagsMenu.falldmg_toggle.itemNames			= yesno_names;

	mDMFlagsMenu.weapons_stay_toggle.generic.type		= UITYPE_SPINCONTROL;
	mDMFlagsMenu.weapons_stay_toggle.generic.x			= 0;
	mDMFlagsMenu.weapons_stay_toggle.generic.y			= y += UIFT_SIZEINC;
	mDMFlagsMenu.weapons_stay_toggle.generic.name		= "Weapons stay";
	mDMFlagsMenu.weapons_stay_toggle.generic.callBack	= DMFlagCallback;
	mDMFlagsMenu.weapons_stay_toggle.itemNames			= yesno_names;

	mDMFlagsMenu.instant_powerups_toggle.generic.type		= UITYPE_SPINCONTROL;
	mDMFlagsMenu.instant_powerups_toggle.generic.x			= 0;
	mDMFlagsMenu.instant_powerups_toggle.generic.y			= y += UIFT_SIZEINC;
	mDMFlagsMenu.instant_powerups_toggle.generic.name		= "Instant powerups";
	mDMFlagsMenu.instant_powerups_toggle.generic.callBack	= DMFlagCallback;
	mDMFlagsMenu.instant_powerups_toggle.itemNames			= yesno_names;

	mDMFlagsMenu.powerups_toggle.generic.type		= UITYPE_SPINCONTROL;
	mDMFlagsMenu.powerups_toggle.generic.x			= 0;
	mDMFlagsMenu.powerups_toggle.generic.y			= y += UIFT_SIZEINC;
	mDMFlagsMenu.powerups_toggle.generic.name		= "Allow powerups";
	mDMFlagsMenu.powerups_toggle.generic.callBack	= DMFlagCallback;
	mDMFlagsMenu.powerups_toggle.itemNames			= yesno_names;

	mDMFlagsMenu.health_toggle.generic.type			= UITYPE_SPINCONTROL;
	mDMFlagsMenu.health_toggle.generic.x			= 0;
	mDMFlagsMenu.health_toggle.generic.y			= y += UIFT_SIZEINC;
	mDMFlagsMenu.health_toggle.generic.callBack		= DMFlagCallback;
	mDMFlagsMenu.health_toggle.generic.name			= "Allow health";
	mDMFlagsMenu.health_toggle.itemNames			= yesno_names;

	mDMFlagsMenu.armor_toggle.generic.type		= UITYPE_SPINCONTROL;
	mDMFlagsMenu.armor_toggle.generic.x			= 0;
	mDMFlagsMenu.armor_toggle.generic.y			= y += UIFT_SIZEINC;
	mDMFlagsMenu.armor_toggle.generic.name		= "Allow armor";
	mDMFlagsMenu.armor_toggle.generic.callBack	= DMFlagCallback;
	mDMFlagsMenu.armor_toggle.itemNames			= yesno_names;

	mDMFlagsMenu.farthest_toggle.generic.type		= UITYPE_SPINCONTROL;
	mDMFlagsMenu.farthest_toggle.generic.x			= 0;
	mDMFlagsMenu.farthest_toggle.generic.y			= y += UIFT_SIZEINC;
	mDMFlagsMenu.farthest_toggle.generic.name		= "Spawn farthest";
	mDMFlagsMenu.farthest_toggle.generic.callBack	= DMFlagCallback;
	mDMFlagsMenu.farthest_toggle.itemNames			= yesno_names;

	mDMFlagsMenu.samelevel_toggle.generic.type		= UITYPE_SPINCONTROL;
	mDMFlagsMenu.samelevel_toggle.generic.x			= 0;
	mDMFlagsMenu.samelevel_toggle.generic.y			= y += UIFT_SIZEINC;
	mDMFlagsMenu.samelevel_toggle.generic.name		= "Same map";
	mDMFlagsMenu.samelevel_toggle.generic.callBack	= DMFlagCallback;
	mDMFlagsMenu.samelevel_toggle.itemNames			= yesno_names;

	mDMFlagsMenu.force_respawn_toggle.generic.type		= UITYPE_SPINCONTROL;
	mDMFlagsMenu.force_respawn_toggle.generic.x			= 0;
	mDMFlagsMenu.force_respawn_toggle.generic.y			= y += UIFT_SIZEINC;
	mDMFlagsMenu.force_respawn_toggle.generic.name		= "Force respawn";
	mDMFlagsMenu.force_respawn_toggle.generic.callBack	= DMFlagCallback;
	mDMFlagsMenu.force_respawn_toggle.itemNames			= yesno_names;

	mDMFlagsMenu.teamplay_toggle.generic.type		= UITYPE_SPINCONTROL;
	mDMFlagsMenu.teamplay_toggle.generic.x			= 0;
	mDMFlagsMenu.teamplay_toggle.generic.y			= y += UIFT_SIZEINC;
	mDMFlagsMenu.teamplay_toggle.generic.name		= "Teamplay";
	mDMFlagsMenu.teamplay_toggle.generic.callBack	= DMFlagCallback;
	mDMFlagsMenu.teamplay_toggle.itemNames			= teamplay_names;

	mDMFlagsMenu.allow_exit_toggle.generic.type			= UITYPE_SPINCONTROL;
	mDMFlagsMenu.allow_exit_toggle.generic.x			= 0;
	mDMFlagsMenu.allow_exit_toggle.generic.y			= y += UIFT_SIZEINC;
	mDMFlagsMenu.allow_exit_toggle.generic.name			= "Allow exit";
	mDMFlagsMenu.allow_exit_toggle.generic.callBack		= DMFlagCallback;
	mDMFlagsMenu.allow_exit_toggle.itemNames			= yesno_names;

	mDMFlagsMenu.infinite_ammo_toggle.generic.type		= UITYPE_SPINCONTROL;
	mDMFlagsMenu.infinite_ammo_toggle.generic.x			= 0;
	mDMFlagsMenu.infinite_ammo_toggle.generic.y			= y += UIFT_SIZEINC;
	mDMFlagsMenu.infinite_ammo_toggle.generic.name		= "Infinite ammo";
	mDMFlagsMenu.infinite_ammo_toggle.generic.callBack	= DMFlagCallback;
	mDMFlagsMenu.infinite_ammo_toggle.itemNames			= yesno_names;

	mDMFlagsMenu.fixed_fov_toggle.generic.type		= UITYPE_SPINCONTROL;
	mDMFlagsMenu.fixed_fov_toggle.generic.x			= 0;
	mDMFlagsMenu.fixed_fov_toggle.generic.y			= y += UIFT_SIZEINC;
	mDMFlagsMenu.fixed_fov_toggle.generic.name		= "Fixed FOV";
	mDMFlagsMenu.fixed_fov_toggle.generic.callBack	= DMFlagCallback;
	mDMFlagsMenu.fixed_fov_toggle.itemNames			= yesno_names;

	mDMFlagsMenu.quad_drop_toggle.generic.type		= UITYPE_SPINCONTROL;
	mDMFlagsMenu.quad_drop_toggle.generic.x			= 0;
	mDMFlagsMenu.quad_drop_toggle.generic.y			= y += UIFT_SIZEINC;
	mDMFlagsMenu.quad_drop_toggle.generic.name		= "Quad drop";
	mDMFlagsMenu.quad_drop_toggle.generic.callBack	= DMFlagCallback;
	mDMFlagsMenu.quad_drop_toggle.itemNames			= yesno_names;

	mDMFlagsMenu.friendlyfire_toggle.generic.type		= UITYPE_SPINCONTROL;
	mDMFlagsMenu.friendlyfire_toggle.generic.x			= 0;
	mDMFlagsMenu.friendlyfire_toggle.generic.y			= y += UIFT_SIZEINC;
	mDMFlagsMenu.friendlyfire_toggle.generic.name		= "Friendly fire";
	mDMFlagsMenu.friendlyfire_toggle.generic.callBack	= DMFlagCallback;
	mDMFlagsMenu.friendlyfire_toggle.itemNames			= yesno_names;

	if (uii.FS_CurrGame ("rogue")) {
		mDMFlagsMenu.rogue_header.generic.type		= UITYPE_ACTION;
		mDMFlagsMenu.rogue_header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM;
		mDMFlagsMenu.rogue_header.generic.x			= 0;
		mDMFlagsMenu.rogue_header.generic.y			= y += UIFT_SIZEINC;
		mDMFlagsMenu.rogue_header.generic.name		= "Rogue Flags";

		mDMFlagsMenu.rogue_no_mines_toggle.generic.type		= UITYPE_SPINCONTROL;
		mDMFlagsMenu.rogue_no_mines_toggle.generic.x		= 0;
		mDMFlagsMenu.rogue_no_mines_toggle.generic.y		= y += UIFT_SIZEINC;
		mDMFlagsMenu.rogue_no_mines_toggle.generic.name		= "Remove mines";
		mDMFlagsMenu.rogue_no_mines_toggle.generic.callBack	= DMFlagCallback;
		mDMFlagsMenu.rogue_no_mines_toggle.itemNames			= yesno_names;

		mDMFlagsMenu.rogue_no_nukes_toggle.generic.type		= UITYPE_SPINCONTROL;
		mDMFlagsMenu.rogue_no_nukes_toggle.generic.x		= 0;
		mDMFlagsMenu.rogue_no_nukes_toggle.generic.y		= y += UIFT_SIZEINC;
		mDMFlagsMenu.rogue_no_nukes_toggle.generic.name		= "Remove nukes";
		mDMFlagsMenu.rogue_no_nukes_toggle.generic.callBack	= DMFlagCallback;
		mDMFlagsMenu.rogue_no_nukes_toggle.itemNames		= yesno_names;

		mDMFlagsMenu.rogue_stack_double_toggle.generic.type		= UITYPE_SPINCONTROL;
		mDMFlagsMenu.rogue_stack_double_toggle.generic.x		= 0;
		mDMFlagsMenu.rogue_stack_double_toggle.generic.y		= y += UIFT_SIZEINC;
		mDMFlagsMenu.rogue_stack_double_toggle.generic.name		= "2x/4x stacking off";
		mDMFlagsMenu.rogue_stack_double_toggle.generic.callBack	= DMFlagCallback;
		mDMFlagsMenu.rogue_stack_double_toggle.itemNames		= yesno_names;

		mDMFlagsMenu.rogue_no_spheres_toggle.generic.type		= UITYPE_SPINCONTROL;
		mDMFlagsMenu.rogue_no_spheres_toggle.generic.x			= 0;
		mDMFlagsMenu.rogue_no_spheres_toggle.generic.y			= y += UIFT_SIZEINC;
		mDMFlagsMenu.rogue_no_spheres_toggle.generic.name		= "Remove spheres";
		mDMFlagsMenu.rogue_no_spheres_toggle.generic.callBack	= DMFlagCallback;
		mDMFlagsMenu.rogue_no_spheres_toggle.itemNames			= yesno_names;

	}

	mDMFlagsMenu.back_action.generic.type			= UITYPE_ACTION;
	mDMFlagsMenu.back_action.generic.flags			= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	mDMFlagsMenu.back_action.generic.x				= 0;
	mDMFlagsMenu.back_action.generic.y				= y += UIFT_SIZEINC + UIFT_SIZEINCLG;
	mDMFlagsMenu.back_action.generic.name			= "< Back";
	mDMFlagsMenu.back_action.generic.callBack		= Menu_Pop;
	mDMFlagsMenu.back_action.generic.statusBar		= "Back a menu";

	DMFlagsMenu_SetValues ();

	UI_AddItem (&mDMFlagsMenu.framework,		&mDMFlagsMenu.banner);
	UI_AddItem (&mDMFlagsMenu.framework,		&mDMFlagsMenu.header);

	UI_AddItem (&mDMFlagsMenu.framework,		&mDMFlagsMenu.falldmg_toggle);
	UI_AddItem (&mDMFlagsMenu.framework,		&mDMFlagsMenu.weapons_stay_toggle);
	UI_AddItem (&mDMFlagsMenu.framework,		&mDMFlagsMenu.instant_powerups_toggle);
	UI_AddItem (&mDMFlagsMenu.framework,		&mDMFlagsMenu.powerups_toggle);
	UI_AddItem (&mDMFlagsMenu.framework,		&mDMFlagsMenu.health_toggle);
	UI_AddItem (&mDMFlagsMenu.framework,		&mDMFlagsMenu.armor_toggle);
	UI_AddItem (&mDMFlagsMenu.framework,		&mDMFlagsMenu.farthest_toggle);
	UI_AddItem (&mDMFlagsMenu.framework,		&mDMFlagsMenu.samelevel_toggle);
	UI_AddItem (&mDMFlagsMenu.framework,		&mDMFlagsMenu.force_respawn_toggle);
	UI_AddItem (&mDMFlagsMenu.framework,		&mDMFlagsMenu.teamplay_toggle);
	UI_AddItem (&mDMFlagsMenu.framework,		&mDMFlagsMenu.allow_exit_toggle);
	UI_AddItem (&mDMFlagsMenu.framework,		&mDMFlagsMenu.infinite_ammo_toggle);
	UI_AddItem (&mDMFlagsMenu.framework,		&mDMFlagsMenu.fixed_fov_toggle);
	UI_AddItem (&mDMFlagsMenu.framework,		&mDMFlagsMenu.quad_drop_toggle);
	UI_AddItem (&mDMFlagsMenu.framework,		&mDMFlagsMenu.friendlyfire_toggle);

	if (uii.FS_CurrGame ("rogue")) {
		UI_AddItem (&mDMFlagsMenu.framework,	&mDMFlagsMenu.rogue_header);
		UI_AddItem (&mDMFlagsMenu.framework,	&mDMFlagsMenu.rogue_no_mines_toggle);
		UI_AddItem (&mDMFlagsMenu.framework,	&mDMFlagsMenu.rogue_no_nukes_toggle);
		UI_AddItem (&mDMFlagsMenu.framework,	&mDMFlagsMenu.rogue_stack_double_toggle);
		UI_AddItem (&mDMFlagsMenu.framework,	&mDMFlagsMenu.rogue_no_spheres_toggle);
	}

	UI_AddItem (&mDMFlagsMenu.framework,	&mDMFlagsMenu.back_action);

	UI_Center (&mDMFlagsMenu.framework);
	UI_Setup (&mDMFlagsMenu.framework);

	// set the original dmflags statusbar
	DMFlagCallback (0);

	UI_SetStatusBar (&mDMFlagsMenu.framework, mDMFlagsStatusbar);
}


/*
=============
DMFlagsMenu_Draw
=============
*/
void DMFlagsMenu_Draw (void) {
	UI_Center (&mDMFlagsMenu.framework);
	UI_Setup (&mDMFlagsMenu.framework);

	UI_AdjustTextCursor (&mDMFlagsMenu.framework, 1);
	UI_Draw (&mDMFlagsMenu.framework);
}


/*
=============
DMFlagsMenu_Key
=============
*/
struct sfx_s *DMFlagsMenu_Key (int key) {
	return DefaultMenu_Key (&mDMFlagsMenu.framework, key);
}


/*
=============
UI_DMFlagsMenu_f
=============
*/
void UI_DMFlagsMenu_f (void) {
	DMFlagsMenu_Init();
	UI_PushMenu (&mDMFlagsMenu.framework, DMFlagsMenu_Draw, DMFlagsMenu_Key);
}
