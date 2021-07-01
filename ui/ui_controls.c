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
=======================================================================

	CONTROLS MENU

=======================================================================
*/

char *bindNames[][2] = {
	{"+attack",			S_COLOR_GREEN "Attack"},
	{"weapprev",		S_COLOR_GREEN "Previous Weapon"},
	{"weapnext",		S_COLOR_GREEN "Next Weapon"},

	{"messagemode",		S_COLOR_GREEN "Chat"},
	{"messagemode2",	S_COLOR_GREEN "Team Chat"},

	{"+speed",			S_COLOR_GREEN "Run"},
	{"+forward",		S_COLOR_GREEN "Walk Forward"},
	{"+back",			S_COLOR_GREEN "Backpedal"},
	{"+moveup",			S_COLOR_GREEN "Up / Jump"},
	{"+movedown",		S_COLOR_GREEN "Down / Crouch"},

	{"+left",			S_COLOR_GREEN "Turn Left"},
	{"+right",			S_COLOR_GREEN "Turn Right"},
	{"+moveleft",		S_COLOR_GREEN "Step Left"},
	{"+moveright",		S_COLOR_GREEN "Step Right"},
	{"+strafe",			S_COLOR_GREEN "Sidestep"},

	{"+lookup",			S_COLOR_GREEN "Look Up"},
	{"+lookdown",		S_COLOR_GREEN "Look Down"},
	{"centerview",		S_COLOR_GREEN "Center View"},
	{"+mlook",			S_COLOR_GREEN "Mouse Look"},
	{"+klook",			S_COLOR_GREEN "Keyboard Look"},

	{"inven",			S_COLOR_GREEN "Inventory"},
	{"invuse",			S_COLOR_GREEN "Use Item"},
	{"invdrop",			S_COLOR_GREEN "Drop Item"},
	{"invprev",			S_COLOR_GREEN "Previous Item"},
	{"invnext",			S_COLOR_GREEN "Next Item"},

	{"cmd help",		S_COLOR_GREEN "Help Computer" }, 
	{ 0, 0 }
};

typedef struct m_controlsmenu_s {
	uiFrameWork_t	framework;

	uiImage_t		banner;
	uiAction_t		header;

	uiAction_t		attack_action;
	uiAction_t		prev_weapon_action;
	uiAction_t		next_weapon_action;

	uiAction_t		chat_action;
	uiAction_t		teamchat_action;

	uiAction_t		run_action;
	uiAction_t		walk_forward_action;
	uiAction_t		backpedal_action;
	uiAction_t		move_up_action;
	uiAction_t		move_down_action;
	uiAction_t		turn_left_action;
	uiAction_t		turn_right_action;

	uiAction_t		step_left_action;
	uiAction_t		step_right_action;
	uiAction_t		sidestep_action;

	uiAction_t		look_up_action;
	uiAction_t		look_down_action;
	uiAction_t		center_view_action;
	uiAction_t		mouse_look_action;
	uiAction_t		keyboard_look_action;

	uiAction_t		inventory_action;
	uiAction_t		inv_use_action;
	uiAction_t		inv_drop_action;
	uiAction_t		inv_prev_action;
	uiAction_t		inv_next_action;

	uiAction_t		help_computer_action;

	uiAction_t		back_action;
} m_controlsmenu_t;

m_controlsmenu_t	m_controls_menu;

static void M_UnbindCommand (char *command) {
	int		j;
	int		l;
	char	*b;

	l = strlen (command);

	for (j=0 ; j<K_MAXKEYS ; j++) {
		b = uii.Key_GetBindingBuf (j);
		if (!b)
			continue;

		if (!Q_strnicmp (b, command, l))
			uii.Key_SetBinding (j, "");
	}
}

static void M_FindKeysForCommand (char *command, int *twokeys) {
	int		count, j, l;
	char	*b;

	twokeys[0] = twokeys[1] = -1;
	l = strlen (command);
	count = 0;

	for (j=0 ; j<K_MAXKEYS ; j++) {
		b = uii.Key_GetBindingBuf (j);
		if (!b)
			continue;

		if (!Q_stricmp (b, command)) {
			twokeys[count++] = j;
			if (count >= 2)
				break;
		}
	}
}

static void KeyCursorDrawFunc (uiFrameWork_t *menu) {
	uiCommon_t	*item = NULL;
	int				cursor;

	cursor = (uiCursorLock) ? '=' : ((int)(uii.Sys_Milliseconds()/250)&1) + 12;

	item = UI_ItemAtCursor (menu);
	if (item->flags & UIF_LEFT_JUSTIFY)
		UI_DrawChar (menu->x + item->x + LCOLUMN_OFFSET + item->cursorOffset, menu->y + item->y, 0, UISCALE_TYPE (item->flags), cursor, colorWhite);
	else if (item->flags & UIF_CENTERED)
		UI_DrawChar (menu->x + item->x - (((Q_ColorCharCount (item->name, strlen (item->name)) + 4) * UISIZE_TYPE (item->flags))*0.5) + item->cursorOffset, menu->y + item->y, 0, UISCALE_TYPE (item->flags), cursor, colorWhite);
	else
		UI_DrawChar (menu->x + item->x + item->cursorOffset, menu->y + item->y, 0, UISCALE_TYPE (item->flags), cursor, colorWhite);
}

static void DrawKeyBindingFunc (void *self) {
	int			keys[2];
	uiAction_t	*a = (uiAction_t *) self;

	M_FindKeysForCommand (bindNames[a->generic.localData[0]][0], keys);
		
	if (keys[0] == -1) {
		UI_DrawString (a->generic.x + a->generic.parent->x + (UISIZE_TYPE (a->generic.flags)*2), a->generic.y + a->generic.parent->y,
			0, UISCALE_TYPE (a->generic.flags), "???", colorWhite);

		a->generic.br[0] = a->generic.x + a->generic.parent->x + (UISIZE_TYPE (a->generic.flags)*5);
		a->generic.br[1] = a->generic.y + a->generic.parent->y + UISIZE_TYPE (a->generic.flags);
	} else {
		int		x;
		char	*name;

		name = uii.Key_KeynumToString (keys[0]);

		UI_DrawString (a->generic.x + a->generic.parent->x + (UISIZE_TYPE (a->generic.flags)*2), a->generic.y + a->generic.parent->y,
			0, UISCALE_TYPE (a->generic.flags), name, colorWhite);

		x = Q_ColorCharCount (name, strlen (name)) * UISIZE_TYPE (a->generic.flags);

		a->generic.br[0] = a->generic.x + a->generic.parent->x + (UISIZE_TYPE (a->generic.flags)*2) + x;
		a->generic.br[1] = a->generic.y + a->generic.parent->y + UISIZE_TYPE (a->generic.flags);

		if (keys[1] != -1) {
			UI_DrawString (a->generic.x + a->generic.parent->x + (UISIZE_TYPE (a->generic.flags)*3) + x, a->generic.y + a->generic.parent->y,
				0, UISCALE_TYPE (a->generic.flags), "or", colorWhite);
			UI_DrawString (a->generic.x + a->generic.parent->x + (UISIZE_TYPE (a->generic.flags)*6) + x, a->generic.y + a->generic.parent->y,
				0, UISCALE_TYPE (a->generic.flags), uii.Key_KeynumToString (keys[1]), colorWhite);

			a->generic.br[0] = a->generic.x + a->generic.parent->x + (strlen (uii.Key_KeynumToString (keys[1])) * UISIZE_TYPE (a->generic.flags)) + (UISIZE_TYPE (a->generic.flags)*6) + x;
			a->generic.br[1] = a->generic.y + a->generic.parent->y + UISIZE_TYPE (a->generic.flags);
		}
	}
}

static void KeyBindingFunc (void *self) {
	int				keys[2];
	uiAction_t	*a = (uiAction_t *) self;

	M_FindKeysForCommand (bindNames[a->generic.localData[0]][0], keys);

	if (keys[1] != -1)
		M_UnbindCommand (bindNames[a->generic.localData[0]][0]);

	uiCursorLock = qTrue;
	uii.Snd_StartLocalSound (uiMedia.menuInSfx);

	UI_SetStatusBar (&m_controls_menu.framework, "Press a button for this action");
}


/*
=============
ControlsMenu_Init
=============
*/
static void ControlsMenu_Init (void) {
	float	y;
	int		i = 0;

	m_controls_menu.framework.x				= uis.vidWidth * 0.50;
	m_controls_menu.framework.y				= 0;
	m_controls_menu.framework.numItems		= 0;
	m_controls_menu.framework.cursorDraw	= KeyCursorDrawFunc;

	UI_Banner (&m_controls_menu.banner, uiMedia.optionsBanner, &y);

	m_controls_menu.header.generic.type		= UITYPE_ACTION;
	m_controls_menu.header.generic.flags	= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_controls_menu.header.generic.x		= 0;
	m_controls_menu.header.generic.y		= y += UIFT_SIZEINC;
	m_controls_menu.header.generic.name		= "Controls";

	m_controls_menu.attack_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.attack_action.generic.flags			= 0;
	m_controls_menu.attack_action.generic.x				= 0;
	m_controls_menu.attack_action.generic.y				= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_controls_menu.attack_action.generic.ownerDraw		= DrawKeyBindingFunc;
	m_controls_menu.attack_action.generic.localData[0]	= i;
	m_controls_menu.attack_action.generic.name			= bindNames[m_controls_menu.attack_action.generic.localData[0]][1];
	m_controls_menu.attack_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.prev_weapon_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.prev_weapon_action.generic.flags		= 0;
	m_controls_menu.prev_weapon_action.generic.x			= 0;
	m_controls_menu.prev_weapon_action.generic.y			= y += UIFT_SIZEINC;
	m_controls_menu.prev_weapon_action.generic.ownerDraw	= DrawKeyBindingFunc;
	m_controls_menu.prev_weapon_action.generic.localData[0]	= ++i;
	m_controls_menu.prev_weapon_action.generic.name			= bindNames[m_controls_menu.prev_weapon_action.generic.localData[0]][1];
	m_controls_menu.prev_weapon_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.next_weapon_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.next_weapon_action.generic.flags		= 0;
	m_controls_menu.next_weapon_action.generic.x			= 0;
	m_controls_menu.next_weapon_action.generic.y			= y += UIFT_SIZEINC;
	m_controls_menu.next_weapon_action.generic.ownerDraw	= DrawKeyBindingFunc;
	m_controls_menu.next_weapon_action.generic.localData[0]	= ++i;
	m_controls_menu.next_weapon_action.generic.name			= bindNames[m_controls_menu.next_weapon_action.generic.localData[0]][1];
	m_controls_menu.next_weapon_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.chat_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.chat_action.generic.flags			= 0;
	m_controls_menu.chat_action.generic.x				= 0;
	m_controls_menu.chat_action.generic.y				= y += (UIFT_SIZEINC*2);
	m_controls_menu.chat_action.generic.ownerDraw		= DrawKeyBindingFunc;
	m_controls_menu.chat_action.generic.localData[0]	= ++i;
	m_controls_menu.chat_action.generic.name			= bindNames[m_controls_menu.chat_action.generic.localData[0]][1];
	m_controls_menu.chat_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.teamchat_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.teamchat_action.generic.flags			= 0;
	m_controls_menu.teamchat_action.generic.x				= 0;
	m_controls_menu.teamchat_action.generic.y				= y += UIFT_SIZEINC;
	m_controls_menu.teamchat_action.generic.ownerDraw		= DrawKeyBindingFunc;
	m_controls_menu.teamchat_action.generic.localData[0]	= ++i;
	m_controls_menu.teamchat_action.generic.name			= bindNames[m_controls_menu.teamchat_action.generic.localData[0]][1];
	m_controls_menu.teamchat_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.run_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.run_action.generic.flags		= 0;
	m_controls_menu.run_action.generic.x			= 0;
	m_controls_menu.run_action.generic.y			= y += (UIFT_SIZEINC*2);
	m_controls_menu.run_action.generic.ownerDraw	= DrawKeyBindingFunc;
	m_controls_menu.run_action.generic.localData[0]	= ++i;
	m_controls_menu.run_action.generic.name			= bindNames[m_controls_menu.run_action.generic.localData[0]][1];
	m_controls_menu.run_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.walk_forward_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.walk_forward_action.generic.flags			= 0;
	m_controls_menu.walk_forward_action.generic.x				= 0;
	m_controls_menu.walk_forward_action.generic.y				= y += UIFT_SIZEINC;
	m_controls_menu.walk_forward_action.generic.ownerDraw		= DrawKeyBindingFunc;
	m_controls_menu.walk_forward_action.generic.localData[0]	= ++i;
	m_controls_menu.walk_forward_action.generic.name			= bindNames[m_controls_menu.walk_forward_action.generic.localData[0]][1];
	m_controls_menu.walk_forward_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.backpedal_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.backpedal_action.generic.flags			= 0;
	m_controls_menu.backpedal_action.generic.x				= 0;
	m_controls_menu.backpedal_action.generic.y				= y += UIFT_SIZEINC;
	m_controls_menu.backpedal_action.generic.ownerDraw		= DrawKeyBindingFunc;
	m_controls_menu.backpedal_action.generic.localData[0]	= ++i;
	m_controls_menu.backpedal_action.generic.name			= bindNames[m_controls_menu.backpedal_action.generic.localData[0]][1];
	m_controls_menu.backpedal_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.move_up_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.move_up_action.generic.flags		= 0;
	m_controls_menu.move_up_action.generic.x			= 0;
	m_controls_menu.move_up_action.generic.y			= y += UIFT_SIZEINC;
	m_controls_menu.move_up_action.generic.ownerDraw	= DrawKeyBindingFunc;
	m_controls_menu.move_up_action.generic.localData[0]	= ++i;
	m_controls_menu.move_up_action.generic.name			= bindNames[m_controls_menu.move_up_action.generic.localData[0]][1];
	m_controls_menu.move_up_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.move_down_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.move_down_action.generic.flags			= 0;
	m_controls_menu.move_down_action.generic.x				= 0;
	m_controls_menu.move_down_action.generic.y				= y += UIFT_SIZEINC;
	m_controls_menu.move_down_action.generic.ownerDraw		= DrawKeyBindingFunc;
	m_controls_menu.move_down_action.generic.localData[0]	= ++i;
	m_controls_menu.move_down_action.generic.name			= bindNames[m_controls_menu.move_down_action.generic.localData[0]][1];
	m_controls_menu.move_down_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.turn_left_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.turn_left_action.generic.flags			= 0;
	m_controls_menu.turn_left_action.generic.x				= 0;
	m_controls_menu.turn_left_action.generic.y				= y += UIFT_SIZEINC;
	m_controls_menu.turn_left_action.generic.ownerDraw		= DrawKeyBindingFunc;
	m_controls_menu.turn_left_action.generic.localData[0]	= ++i;
	m_controls_menu.turn_left_action.generic.name			= bindNames[m_controls_menu.turn_left_action.generic.localData[0]][1];
	m_controls_menu.turn_left_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.turn_right_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.turn_right_action.generic.flags			= 0;
	m_controls_menu.turn_right_action.generic.x				= 0;
	m_controls_menu.turn_right_action.generic.y				= y += UIFT_SIZEINC;
	m_controls_menu.turn_right_action.generic.ownerDraw		= DrawKeyBindingFunc;
	m_controls_menu.turn_right_action.generic.localData[0]	= ++i;
	m_controls_menu.turn_right_action.generic.name			= bindNames[m_controls_menu.turn_right_action.generic.localData[0]][1];
	m_controls_menu.turn_right_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.step_left_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.step_left_action.generic.flags			= 0;
	m_controls_menu.step_left_action.generic.x				= 0;
	m_controls_menu.step_left_action.generic.y				= y += (UIFT_SIZEINC*2);
	m_controls_menu.step_left_action.generic.ownerDraw		= DrawKeyBindingFunc;
	m_controls_menu.step_left_action.generic.localData[0]	= ++i;
	m_controls_menu.step_left_action.generic.name			= bindNames[m_controls_menu.step_left_action.generic.localData[0]][1];
	m_controls_menu.step_left_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.step_right_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.step_right_action.generic.flags			= 0;
	m_controls_menu.step_right_action.generic.x				= 0;
	m_controls_menu.step_right_action.generic.y				= y += UIFT_SIZEINC;
	m_controls_menu.step_right_action.generic.ownerDraw		= DrawKeyBindingFunc;
	m_controls_menu.step_right_action.generic.localData[0]	= ++i;
	m_controls_menu.step_right_action.generic.name			= bindNames[m_controls_menu.step_right_action.generic.localData[0]][1];
	m_controls_menu.step_right_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.sidestep_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.sidestep_action.generic.flags			= 0;
	m_controls_menu.sidestep_action.generic.x				= 0;
	m_controls_menu.sidestep_action.generic.y				= y += UIFT_SIZEINC;
	m_controls_menu.sidestep_action.generic.ownerDraw		= DrawKeyBindingFunc;
	m_controls_menu.sidestep_action.generic.localData[0]	= ++i;
	m_controls_menu.sidestep_action.generic.name			= bindNames[m_controls_menu.sidestep_action.generic.localData[0]][1];
	m_controls_menu.sidestep_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.look_up_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.look_up_action.generic.flags		= 0;
	m_controls_menu.look_up_action.generic.x			= 0;
	m_controls_menu.look_up_action.generic.y			= y += (UIFT_SIZEINC*2);
	m_controls_menu.look_up_action.generic.ownerDraw	= DrawKeyBindingFunc;
	m_controls_menu.look_up_action.generic.localData[0]	= ++i;
	m_controls_menu.look_up_action.generic.name			= bindNames[m_controls_menu.look_up_action.generic.localData[0]][1];
	m_controls_menu.look_up_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.look_down_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.look_down_action.generic.flags			= 0;
	m_controls_menu.look_down_action.generic.x				= 0;
	m_controls_menu.look_down_action.generic.y				= y += UIFT_SIZEINC;
	m_controls_menu.look_down_action.generic.ownerDraw		= DrawKeyBindingFunc;
	m_controls_menu.look_down_action.generic.localData[0]	= ++i;
	m_controls_menu.look_down_action.generic.name			= bindNames[m_controls_menu.look_down_action.generic.localData[0]][1];
	m_controls_menu.look_down_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.center_view_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.center_view_action.generic.flags		= 0;
	m_controls_menu.center_view_action.generic.x			= 0;
	m_controls_menu.center_view_action.generic.y			= y += UIFT_SIZEINC;
	m_controls_menu.center_view_action.generic.ownerDraw	= DrawKeyBindingFunc;
	m_controls_menu.center_view_action.generic.localData[0]	= ++i;
	m_controls_menu.center_view_action.generic.name			= bindNames[m_controls_menu.center_view_action.generic.localData[0]][1];
	m_controls_menu.center_view_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.mouse_look_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.mouse_look_action.generic.flags			= 0;
	m_controls_menu.mouse_look_action.generic.x				= 0;
	m_controls_menu.mouse_look_action.generic.y				= y += UIFT_SIZEINC;
	m_controls_menu.mouse_look_action.generic.ownerDraw		= DrawKeyBindingFunc;
	m_controls_menu.mouse_look_action.generic.localData[0]	= ++i;
	m_controls_menu.mouse_look_action.generic.name			= bindNames[m_controls_menu.mouse_look_action.generic.localData[0]][1];
	m_controls_menu.mouse_look_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.keyboard_look_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.keyboard_look_action.generic.flags			= 0;
	m_controls_menu.keyboard_look_action.generic.x				= 0;
	m_controls_menu.keyboard_look_action.generic.y				= y += UIFT_SIZEINC;
	m_controls_menu.keyboard_look_action.generic.ownerDraw		= DrawKeyBindingFunc;
	m_controls_menu.keyboard_look_action.generic.localData[0]	= ++i;
	m_controls_menu.keyboard_look_action.generic.name			= bindNames[m_controls_menu.keyboard_look_action.generic.localData[0]][1];
	m_controls_menu.keyboard_look_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.inventory_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.inventory_action.generic.flags			= 0;
	m_controls_menu.inventory_action.generic.x				= 0;
	m_controls_menu.inventory_action.generic.y				= y += (UIFT_SIZEINC*2);
	m_controls_menu.inventory_action.generic.ownerDraw		= DrawKeyBindingFunc;
	m_controls_menu.inventory_action.generic.localData[0]	= ++i;
	m_controls_menu.inventory_action.generic.name			= bindNames[m_controls_menu.inventory_action.generic.localData[0]][1];
	m_controls_menu.inventory_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.inv_use_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.inv_use_action.generic.flags		= 0;
	m_controls_menu.inv_use_action.generic.x			= 0;
	m_controls_menu.inv_use_action.generic.y			= y += UIFT_SIZEINC;
	m_controls_menu.inv_use_action.generic.ownerDraw	= DrawKeyBindingFunc;
	m_controls_menu.inv_use_action.generic.localData[0]	= ++i;
	m_controls_menu.inv_use_action.generic.name			= bindNames[m_controls_menu.inv_use_action.generic.localData[0]][1];
	m_controls_menu.inv_use_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.inv_drop_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.inv_drop_action.generic.flags			= 0;
	m_controls_menu.inv_drop_action.generic.x				= 0;
	m_controls_menu.inv_drop_action.generic.y				= y += UIFT_SIZEINC;
	m_controls_menu.inv_drop_action.generic.ownerDraw		= DrawKeyBindingFunc;
	m_controls_menu.inv_drop_action.generic.localData[0]	= ++i;
	m_controls_menu.inv_drop_action.generic.name			= bindNames[m_controls_menu.inv_drop_action.generic.localData[0]][1];
	m_controls_menu.inv_drop_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.inv_prev_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.inv_prev_action.generic.flags			= 0;
	m_controls_menu.inv_prev_action.generic.x				= 0;
	m_controls_menu.inv_prev_action.generic.y				= y += UIFT_SIZEINC;
	m_controls_menu.inv_prev_action.generic.ownerDraw		= DrawKeyBindingFunc;
	m_controls_menu.inv_prev_action.generic.localData[0]	= ++i;
	m_controls_menu.inv_prev_action.generic.name			= bindNames[m_controls_menu.inv_prev_action.generic.localData[0]][1];
	m_controls_menu.inv_prev_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.inv_next_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.inv_next_action.generic.flags			= 0;
	m_controls_menu.inv_next_action.generic.x				= 0;
	m_controls_menu.inv_next_action.generic.y				= y += UIFT_SIZEINC;
	m_controls_menu.inv_next_action.generic.ownerDraw		= DrawKeyBindingFunc;
	m_controls_menu.inv_next_action.generic.localData[0]	= ++i;
	m_controls_menu.inv_next_action.generic.name			= bindNames[m_controls_menu.inv_next_action.generic.localData[0]][1];
	m_controls_menu.inv_next_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.help_computer_action.generic.type			= UITYPE_ACTION;
	m_controls_menu.help_computer_action.generic.flags			= 0;
	m_controls_menu.help_computer_action.generic.x				= 0;
	m_controls_menu.help_computer_action.generic.y				= y += (UIFT_SIZEINC*2);
	m_controls_menu.help_computer_action.generic.ownerDraw		= DrawKeyBindingFunc;
	m_controls_menu.help_computer_action.generic.localData[0]	= ++i;
	m_controls_menu.help_computer_action.generic.name			= bindNames[m_controls_menu.help_computer_action.generic.localData[0]][1];
	m_controls_menu.help_computer_action.generic.callBack		= KeyBindingFunc;

	m_controls_menu.back_action.generic.type		= UITYPE_ACTION;
	m_controls_menu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_controls_menu.back_action.generic.x			= 0;
	m_controls_menu.back_action.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCLG;
	m_controls_menu.back_action.generic.name		= "< Back";
	m_controls_menu.back_action.generic.callBack	= Back_Menu;
	m_controls_menu.back_action.generic.statusBar	= "Back a menu";

	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.banner);
	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.header);

	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.attack_action);
	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.prev_weapon_action);
	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.next_weapon_action);

	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.chat_action);
	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.teamchat_action);

	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.run_action);
	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.walk_forward_action);
	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.backpedal_action);
	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.move_up_action);
	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.move_down_action);
	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.turn_left_action);
	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.turn_right_action);

	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.step_left_action);
	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.step_right_action);
	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.sidestep_action);

	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.look_up_action);
	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.look_down_action);
	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.center_view_action);
	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.mouse_look_action);
	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.keyboard_look_action);

	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.inventory_action);
	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.inv_use_action);
	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.inv_drop_action);
	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.inv_prev_action);
	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.inv_next_action);

	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.help_computer_action);

	UI_AddItem (&m_controls_menu.framework,			&m_controls_menu.back_action);

	UI_SetStatusBar (&m_controls_menu.framework,	"[ENTER] to change, [BACKSPACE] to clear");

	UI_Center (&m_controls_menu.framework);
	UI_Setup (&m_controls_menu.framework);
}


/*
=============
ControlsMenu_Draw
=============
*/
static void ControlsMenu_Draw (void) {
	UI_Center (&m_controls_menu.framework);
	UI_Setup (&m_controls_menu.framework);

	UI_AdjustTextCursor (&m_controls_menu.framework, 1);
	UI_Draw (&m_controls_menu.framework);
}


/*
=============
ControlsMenu_Key
=============
*/
static struct sfx_s *ControlsMenu_Key (int key) {
	uiAction_t *item = (uiAction_t *) UI_ItemAtCursor (&m_controls_menu.framework);

	if (uiCursorLock) {	
		if ((key != K_ESCAPE) && (key != '`') && (key != '~')) {
			char cmd[1024];

			Q_snprintfz (cmd, sizeof (cmd), "bind \"%s\" \"%s\"\n", uii.Key_KeynumToString(key), bindNames[item->generic.localData[0]][0]);
			uii.Cbuf_InsertText (cmd);
		} else
			M_UnbindCommand (bindNames[item->generic.localData[0]][0]);
		
		UI_SetStatusBar (&m_controls_menu.framework, "[ENTER] to change, [BACKSPACE] to clear");
		uiCursorLock = qFalse;
		return uiMedia.menuOutSfx;
	}

	return DefaultMenu_Key (&m_controls_menu.framework, key);
}


/*
=============
UI_ControlsMenu_f
=============
*/
void UI_ControlsMenu_f (void) {
	ControlsMenu_Init ();

	UI_SetStatusBar (&m_controls_menu.framework, "[ENTER] to change, [BACKSPACE] to clear");
	UI_PushMenu (&m_controls_menu.framework, ControlsMenu_Draw, ControlsMenu_Key);
}
