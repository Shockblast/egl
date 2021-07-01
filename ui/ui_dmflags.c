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

typedef struct m_dmflagsmenumenu_s {
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
} m_dmflagsmenumenu_t;

m_dmflagsmenumenu_t	m_dmflags_menu;

static char dmflags_statusbar[128];

static void DMFlagCallback (void *self) {
	uiList_t	*f = (uiList_t *) self;
	int			flags, bit = 0;

	flags = uii.Cvar_VariableInteger ("dmflags");

	if (f == &m_dmflags_menu.friendlyfire_toggle) {
		if (f->curValue)
			flags &= ~DF_NO_FRIENDLY_FIRE;
		else
			flags |= DF_NO_FRIENDLY_FIRE;
		goto setvalue;
	} else if (f == &m_dmflags_menu.falldmg_toggle) {
		if (f->curValue)
			flags &= ~DF_NO_FALLING;
		else
			flags |= DF_NO_FALLING;
		goto setvalue;
	} else if (f == &m_dmflags_menu.weapons_stay_toggle)
		bit = DF_WEAPONS_STAY;
	else if (f == &m_dmflags_menu.instant_powerups_toggle)
		bit = DF_INSTANT_ITEMS;
	else if (f == &m_dmflags_menu.allow_exit_toggle)
		bit = DF_ALLOW_EXIT;
	else if (f == &m_dmflags_menu.powerups_toggle) {
		if (f->curValue)
			flags &= ~DF_NO_ITEMS;
		else
			flags |= DF_NO_ITEMS;
		goto setvalue;
	} else if (f == &m_dmflags_menu.health_toggle) {
		if (f->curValue)
			flags &= ~DF_NO_HEALTH;
		else
			flags |= DF_NO_HEALTH;
		goto setvalue;
	} else if (f == &m_dmflags_menu.farthest_toggle)
		bit = DF_SPAWN_FARTHEST;
	else if (f == &m_dmflags_menu.teamplay_toggle) {
		if (f->curValue == 1) {
			flags |=  DF_SKINTEAMS;
			flags &= ~DF_MODELTEAMS;
		} else if (f->curValue == 2) {
			flags |=  DF_MODELTEAMS;
			flags &= ~DF_SKINTEAMS;
		} else
			flags &= ~(DF_MODELTEAMS | DF_SKINTEAMS);

		goto setvalue;
	} else if (f == &m_dmflags_menu.samelevel_toggle)
		bit = DF_SAME_LEVEL;
	else if (f == &m_dmflags_menu.force_respawn_toggle)
		bit = DF_FORCE_RESPAWN;
	else if (f == &m_dmflags_menu.armor_toggle) {
		if (f->curValue)
			flags &= ~DF_NO_ARMOR;
		else
			flags |= DF_NO_ARMOR;
		goto setvalue;
	} else if (f == &m_dmflags_menu.infinite_ammo_toggle)
		bit = DF_INFINITE_AMMO;
	else if (f == &m_dmflags_menu.fixed_fov_toggle)
		bit = DF_FIXED_FOV;
	else if (f == &m_dmflags_menu.quad_drop_toggle)
		bit = DF_QUAD_DROP;
	else if (uii.FS_CurrGame ("rogue")) {
		if (f == &m_dmflags_menu.rogue_no_mines_toggle)
			bit = DF_NO_MINES;
		else if (f == &m_dmflags_menu.rogue_no_nukes_toggle)
			bit = DF_NO_NUKES;
		else if (f == &m_dmflags_menu.rogue_stack_double_toggle)
			bit = DF_NO_STACK_DOUBLE;
		else if (f == &m_dmflags_menu.rogue_no_spheres_toggle)
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

	Q_snprintfz (dmflags_statusbar, sizeof (dmflags_statusbar), "dmflags = %d", flags);
}


/*
=============
DMFlagsMenu_SetValues
=============
*/
static void DMFlagsMenu_SetValues (void) {
	int		dmflags = uii.Cvar_VariableInteger ("dmflags");

	m_dmflags_menu.falldmg_toggle.curValue				= !(dmflags & DF_NO_FALLING);
	m_dmflags_menu.weapons_stay_toggle.curValue			= !!(dmflags & DF_WEAPONS_STAY);
	m_dmflags_menu.instant_powerups_toggle.curValue		= !!(dmflags & DF_INSTANT_ITEMS);
	m_dmflags_menu.powerups_toggle.curValue				= !(dmflags & DF_NO_ITEMS);
	m_dmflags_menu.health_toggle.curValue				= !(dmflags & DF_NO_HEALTH);
	m_dmflags_menu.armor_toggle.curValue				= !(dmflags & DF_NO_ARMOR);
	m_dmflags_menu.farthest_toggle.curValue				= !!(dmflags & DF_SPAWN_FARTHEST);
	m_dmflags_menu.samelevel_toggle.curValue			= !!(dmflags & DF_SAME_LEVEL);
	m_dmflags_menu.force_respawn_toggle.curValue		= !!(dmflags & DF_FORCE_RESPAWN);
	m_dmflags_menu.allow_exit_toggle.curValue			= !!(dmflags & DF_ALLOW_EXIT);
	m_dmflags_menu.infinite_ammo_toggle.curValue		= !!(dmflags & DF_INFINITE_AMMO);
	m_dmflags_menu.fixed_fov_toggle.curValue			= !!(dmflags & DF_FIXED_FOV);
	m_dmflags_menu.quad_drop_toggle.curValue			= !!(dmflags & DF_QUAD_DROP);
	m_dmflags_menu.friendlyfire_toggle.curValue			= !(dmflags & DF_NO_FRIENDLY_FIRE);

	if (uii.FS_CurrGame ("rogue")) {
		m_dmflags_menu.rogue_no_mines_toggle.curValue		= !!(dmflags & DF_NO_MINES);
		m_dmflags_menu.rogue_no_nukes_toggle.curValue		= !!(dmflags & DF_NO_NUKES);
		m_dmflags_menu.rogue_stack_double_toggle.curValue	= !!(dmflags & DF_NO_STACK_DOUBLE);
		m_dmflags_menu.rogue_no_spheres_toggle.curValue		= !!(dmflags & DF_NO_SPHERES);
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

	m_dmflags_menu.framework.x			= uis.vidWidth * 0.50;
	m_dmflags_menu.framework.y			= 0;
	m_dmflags_menu.framework.numItems	= 0;

	UI_Banner (&m_dmflags_menu.banner, uiMedia.startServerBanner, &y);

	m_dmflags_menu.header.generic.type		= UITYPE_ACTION;
	m_dmflags_menu.header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_dmflags_menu.header.generic.x			= 0;
	m_dmflags_menu.header.generic.y			= y += UIFT_SIZEINC;
	m_dmflags_menu.header.generic.name		= "Deathmatch Flags";

	m_dmflags_menu.falldmg_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_dmflags_menu.falldmg_toggle.generic.x			= 0;
	m_dmflags_menu.falldmg_toggle.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_dmflags_menu.falldmg_toggle.generic.name		= "Falling damage";
	m_dmflags_menu.falldmg_toggle.generic.callBack	= DMFlagCallback;
	m_dmflags_menu.falldmg_toggle.itemNames			= yesno_names;

	m_dmflags_menu.weapons_stay_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_dmflags_menu.weapons_stay_toggle.generic.x		= 0;
	m_dmflags_menu.weapons_stay_toggle.generic.y		= y += UIFT_SIZEINC;
	m_dmflags_menu.weapons_stay_toggle.generic.name		= "Weapons stay";
	m_dmflags_menu.weapons_stay_toggle.generic.callBack	= DMFlagCallback;
	m_dmflags_menu.weapons_stay_toggle.itemNames		= yesno_names;

	m_dmflags_menu.instant_powerups_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_dmflags_menu.instant_powerups_toggle.generic.x		= 0;
	m_dmflags_menu.instant_powerups_toggle.generic.y		= y += UIFT_SIZEINC;
	m_dmflags_menu.instant_powerups_toggle.generic.name		= "Instant powerups";
	m_dmflags_menu.instant_powerups_toggle.generic.callBack	= DMFlagCallback;
	m_dmflags_menu.instant_powerups_toggle.itemNames		= yesno_names;

	m_dmflags_menu.powerups_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_dmflags_menu.powerups_toggle.generic.x		= 0;
	m_dmflags_menu.powerups_toggle.generic.y		= y += UIFT_SIZEINC;
	m_dmflags_menu.powerups_toggle.generic.name		= "Allow powerups";
	m_dmflags_menu.powerups_toggle.generic.callBack	= DMFlagCallback;
	m_dmflags_menu.powerups_toggle.itemNames		= yesno_names;

	m_dmflags_menu.health_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_dmflags_menu.health_toggle.generic.x			= 0;
	m_dmflags_menu.health_toggle.generic.y			= y += UIFT_SIZEINC;
	m_dmflags_menu.health_toggle.generic.callBack	= DMFlagCallback;
	m_dmflags_menu.health_toggle.generic.name		= "Allow health";
	m_dmflags_menu.health_toggle.itemNames			= yesno_names;

	m_dmflags_menu.armor_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_dmflags_menu.armor_toggle.generic.x			= 0;
	m_dmflags_menu.armor_toggle.generic.y			= y += UIFT_SIZEINC;
	m_dmflags_menu.armor_toggle.generic.name		= "Allow armor";
	m_dmflags_menu.armor_toggle.generic.callBack	= DMFlagCallback;
	m_dmflags_menu.armor_toggle.itemNames			= yesno_names;

	m_dmflags_menu.farthest_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_dmflags_menu.farthest_toggle.generic.x			= 0;
	m_dmflags_menu.farthest_toggle.generic.y			= y += UIFT_SIZEINC;
	m_dmflags_menu.farthest_toggle.generic.name			= "Spawn farthest";
	m_dmflags_menu.farthest_toggle.generic.callBack		= DMFlagCallback;
	m_dmflags_menu.farthest_toggle.itemNames			= yesno_names;

	m_dmflags_menu.samelevel_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_dmflags_menu.samelevel_toggle.generic.x			= 0;
	m_dmflags_menu.samelevel_toggle.generic.y			= y += UIFT_SIZEINC;
	m_dmflags_menu.samelevel_toggle.generic.name		= "Same map";
	m_dmflags_menu.samelevel_toggle.generic.callBack	= DMFlagCallback;
	m_dmflags_menu.samelevel_toggle.itemNames			= yesno_names;

	m_dmflags_menu.force_respawn_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_dmflags_menu.force_respawn_toggle.generic.x			= 0;
	m_dmflags_menu.force_respawn_toggle.generic.y			= y += UIFT_SIZEINC;
	m_dmflags_menu.force_respawn_toggle.generic.name		= "Force respawn";
	m_dmflags_menu.force_respawn_toggle.generic.callBack	= DMFlagCallback;
	m_dmflags_menu.force_respawn_toggle.itemNames			= yesno_names;

	m_dmflags_menu.teamplay_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_dmflags_menu.teamplay_toggle.generic.x			= 0;
	m_dmflags_menu.teamplay_toggle.generic.y			= y += UIFT_SIZEINC;
	m_dmflags_menu.teamplay_toggle.generic.name			= "Teamplay";
	m_dmflags_menu.teamplay_toggle.generic.callBack		= DMFlagCallback;
	m_dmflags_menu.teamplay_toggle.itemNames			= teamplay_names;

	m_dmflags_menu.allow_exit_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_dmflags_menu.allow_exit_toggle.generic.x			= 0;
	m_dmflags_menu.allow_exit_toggle.generic.y			= y += UIFT_SIZEINC;
	m_dmflags_menu.allow_exit_toggle.generic.name		= "Allow exit";
	m_dmflags_menu.allow_exit_toggle.generic.callBack	= DMFlagCallback;
	m_dmflags_menu.allow_exit_toggle.itemNames			= yesno_names;

	m_dmflags_menu.infinite_ammo_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_dmflags_menu.infinite_ammo_toggle.generic.x			= 0;
	m_dmflags_menu.infinite_ammo_toggle.generic.y			= y += UIFT_SIZEINC;
	m_dmflags_menu.infinite_ammo_toggle.generic.name		= "Infinite ammo";
	m_dmflags_menu.infinite_ammo_toggle.generic.callBack	= DMFlagCallback;
	m_dmflags_menu.infinite_ammo_toggle.itemNames			= yesno_names;

	m_dmflags_menu.fixed_fov_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_dmflags_menu.fixed_fov_toggle.generic.x			= 0;
	m_dmflags_menu.fixed_fov_toggle.generic.y			= y += UIFT_SIZEINC;
	m_dmflags_menu.fixed_fov_toggle.generic.name		= "Fixed FOV";
	m_dmflags_menu.fixed_fov_toggle.generic.callBack	= DMFlagCallback;
	m_dmflags_menu.fixed_fov_toggle.itemNames			= yesno_names;

	m_dmflags_menu.quad_drop_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_dmflags_menu.quad_drop_toggle.generic.x			= 0;
	m_dmflags_menu.quad_drop_toggle.generic.y			= y += UIFT_SIZEINC;
	m_dmflags_menu.quad_drop_toggle.generic.name		= "Quad drop";
	m_dmflags_menu.quad_drop_toggle.generic.callBack	= DMFlagCallback;
	m_dmflags_menu.quad_drop_toggle.itemNames			= yesno_names;

	m_dmflags_menu.friendlyfire_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_dmflags_menu.friendlyfire_toggle.generic.x		= 0;
	m_dmflags_menu.friendlyfire_toggle.generic.y		= y += UIFT_SIZEINC;
	m_dmflags_menu.friendlyfire_toggle.generic.name		= "Friendly fire";
	m_dmflags_menu.friendlyfire_toggle.generic.callBack	= DMFlagCallback;
	m_dmflags_menu.friendlyfire_toggle.itemNames		= yesno_names;

	if (uii.FS_CurrGame ("rogue")) {
		m_dmflags_menu.rogue_header.generic.type		= UITYPE_ACTION;
		m_dmflags_menu.rogue_header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM;
		m_dmflags_menu.rogue_header.generic.x			= 0;
		m_dmflags_menu.rogue_header.generic.y			= y += UIFT_SIZEINC;
		m_dmflags_menu.rogue_header.generic.name		= "Rogue Flags";

		m_dmflags_menu.rogue_no_mines_toggle.generic.type		= UITYPE_SPINCONTROL;
		m_dmflags_menu.rogue_no_mines_toggle.generic.x			= 0;
		m_dmflags_menu.rogue_no_mines_toggle.generic.y			= y += UIFT_SIZEINC;
		m_dmflags_menu.rogue_no_mines_toggle.generic.name		= "Remove mines";
		m_dmflags_menu.rogue_no_mines_toggle.generic.callBack	= DMFlagCallback;
		m_dmflags_menu.rogue_no_mines_toggle.itemNames			= yesno_names;

		m_dmflags_menu.rogue_no_nukes_toggle.generic.type		= UITYPE_SPINCONTROL;
		m_dmflags_menu.rogue_no_nukes_toggle.generic.x			= 0;
		m_dmflags_menu.rogue_no_nukes_toggle.generic.y			= y += UIFT_SIZEINC;
		m_dmflags_menu.rogue_no_nukes_toggle.generic.name		= "Remove nukes";
		m_dmflags_menu.rogue_no_nukes_toggle.generic.callBack	= DMFlagCallback;
		m_dmflags_menu.rogue_no_nukes_toggle.itemNames			= yesno_names;

		m_dmflags_menu.rogue_stack_double_toggle.generic.type		= UITYPE_SPINCONTROL;
		m_dmflags_menu.rogue_stack_double_toggle.generic.x			= 0;
		m_dmflags_menu.rogue_stack_double_toggle.generic.y			= y += UIFT_SIZEINC;
		m_dmflags_menu.rogue_stack_double_toggle.generic.name		= "2x/4x stacking off";
		m_dmflags_menu.rogue_stack_double_toggle.generic.callBack	= DMFlagCallback;
		m_dmflags_menu.rogue_stack_double_toggle.itemNames			= yesno_names;

		m_dmflags_menu.rogue_no_spheres_toggle.generic.type		= UITYPE_SPINCONTROL;
		m_dmflags_menu.rogue_no_spheres_toggle.generic.x		= 0;
		m_dmflags_menu.rogue_no_spheres_toggle.generic.y		= y += UIFT_SIZEINC;
		m_dmflags_menu.rogue_no_spheres_toggle.generic.name		= "Remove spheres";
		m_dmflags_menu.rogue_no_spheres_toggle.generic.callBack	= DMFlagCallback;
		m_dmflags_menu.rogue_no_spheres_toggle.itemNames		= yesno_names;

	}

	m_dmflags_menu.back_action.generic.type			= UITYPE_ACTION;
	m_dmflags_menu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_dmflags_menu.back_action.generic.x			= 0;
	m_dmflags_menu.back_action.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCLG;
	m_dmflags_menu.back_action.generic.name			= "< Back";
	m_dmflags_menu.back_action.generic.callBack		= Back_Menu;
	m_dmflags_menu.back_action.generic.statusBar	= "Back a menu";

	DMFlagsMenu_SetValues ();

	UI_AddItem (&m_dmflags_menu.framework,		&m_dmflags_menu.banner);
	UI_AddItem (&m_dmflags_menu.framework,		&m_dmflags_menu.header);

	UI_AddItem (&m_dmflags_menu.framework,		&m_dmflags_menu.falldmg_toggle);
	UI_AddItem (&m_dmflags_menu.framework,		&m_dmflags_menu.weapons_stay_toggle);
	UI_AddItem (&m_dmflags_menu.framework,		&m_dmflags_menu.instant_powerups_toggle);
	UI_AddItem (&m_dmflags_menu.framework,		&m_dmflags_menu.powerups_toggle);
	UI_AddItem (&m_dmflags_menu.framework,		&m_dmflags_menu.health_toggle);
	UI_AddItem (&m_dmflags_menu.framework,		&m_dmflags_menu.armor_toggle);
	UI_AddItem (&m_dmflags_menu.framework,		&m_dmflags_menu.farthest_toggle);
	UI_AddItem (&m_dmflags_menu.framework,		&m_dmflags_menu.samelevel_toggle);
	UI_AddItem (&m_dmflags_menu.framework,		&m_dmflags_menu.force_respawn_toggle);
	UI_AddItem (&m_dmflags_menu.framework,		&m_dmflags_menu.teamplay_toggle);
	UI_AddItem (&m_dmflags_menu.framework,		&m_dmflags_menu.allow_exit_toggle);
	UI_AddItem (&m_dmflags_menu.framework,		&m_dmflags_menu.infinite_ammo_toggle);
	UI_AddItem (&m_dmflags_menu.framework,		&m_dmflags_menu.fixed_fov_toggle);
	UI_AddItem (&m_dmflags_menu.framework,		&m_dmflags_menu.quad_drop_toggle);
	UI_AddItem (&m_dmflags_menu.framework,		&m_dmflags_menu.friendlyfire_toggle);

	if (uii.FS_CurrGame ("rogue")) {
		UI_AddItem (&m_dmflags_menu.framework,	&m_dmflags_menu.rogue_header);
		UI_AddItem (&m_dmflags_menu.framework,	&m_dmflags_menu.rogue_no_mines_toggle);
		UI_AddItem (&m_dmflags_menu.framework,	&m_dmflags_menu.rogue_no_nukes_toggle);
		UI_AddItem (&m_dmflags_menu.framework,	&m_dmflags_menu.rogue_stack_double_toggle);
		UI_AddItem (&m_dmflags_menu.framework,	&m_dmflags_menu.rogue_no_spheres_toggle);
	}

	UI_AddItem (&m_dmflags_menu.framework,	&m_dmflags_menu.back_action);

	UI_Center (&m_dmflags_menu.framework);
	UI_Setup (&m_dmflags_menu.framework);

	// set the original dmflags statusbar
	DMFlagCallback (0);

	UI_SetStatusBar (&m_dmflags_menu.framework, dmflags_statusbar);
}


/*
=============
DMFlagsMenu_Draw
=============
*/
void DMFlagsMenu_Draw (void) {
	UI_Center (&m_dmflags_menu.framework);
	UI_Setup (&m_dmflags_menu.framework);

	UI_AdjustTextCursor (&m_dmflags_menu.framework, 1);
	UI_Draw (&m_dmflags_menu.framework);
}


/*
=============
DMFlagsMenu_Key
=============
*/
struct sfx_s *DMFlagsMenu_Key (int key) {
	return DefaultMenu_Key (&m_dmflags_menu.framework, key);
}


/*
=============
UI_DMFlagsMenu_f
=============
*/
void UI_DMFlagsMenu_f (void) {
	DMFlagsMenu_Init();
	UI_PushMenu (&m_dmflags_menu.framework, DMFlagsMenu_Draw, DMFlagsMenu_Key);
}
