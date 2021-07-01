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

qBool	ui_EnterSound;
char	*ui_InSFX		= "misc/menu1.wav";
char	*ui_MoveSFX		= "misc/menu2.wav";
char	*ui_OutSFX		= "misc/menu3.wav";

uiState_t	uis;
uiMedia_t	uiMedia;

char	**mapnames;
int		nummaps;

uiLayer_t	ui_Layers[MAX_UI_DEPTH];
int			ui_MenuDepth;

uiFrameWork_t	*ui_ActiveUI;

void	(*UI_DrawFunc) (void);

/*
=======================================================================

	UI INIT/SHUTDOWN

=======================================================================
*/

/*
=================
UI_RegisterMedia
=================
*/
void UI_RegisterMedia (void) {
	char	cursorName[MAX_QPATH];
	int		i;

	// background images
	uiMedia.bgBig = uii.Img_RegisterPic ("/egl/ui/bg_big.pcx");

	// cursor image
	uiMedia.cursorImage = uii.Img_RegisterPic ("/egl/ui/cursor.pcx");

	// banners
	uiMedia.addressBookBanner	= uii.Img_RegisterPic ("m_banner_addressbook");
	uiMedia.multiplayerBanner	= uii.Img_RegisterPic ("m_banner_multiplayer");
	uiMedia.startServerBanner	= uii.Img_RegisterPic ("m_banner_start_server");
	uiMedia.joinServerBanner	= uii.Img_RegisterPic ("m_banner_join_server");
	uiMedia.optionsBanner		= uii.Img_RegisterPic ("m_banner_options");
	uiMedia.gameBanner			= uii.Img_RegisterPic ("m_banner_game");
	uiMedia.loadGameBanner		= uii.Img_RegisterPic ("m_banner_load_game");
	uiMedia.saveGameBanner		= uii.Img_RegisterPic ("m_banner_save_game");
	uiMedia.videoBanner			= uii.Img_RegisterPic ("m_banner_video");
	uiMedia.quitBanner			= uii.Img_RegisterPic ("m_main_quit");

	// main menu cursors
	for (i=0 ; i<MAINMENU_CURSOR_NUMFRAMES ; i++) {
		Com_sprintf (cursorName, sizeof (cursorName), "m_cursor%d", i);
		uiMedia.mainCursors[i] = uii.Img_RegisterPic (cursorName);
	}

	// main menu
	uiMedia.mainPlaque		= uii.Img_RegisterPic ("m_main_plaque");
	uiMedia.mainLogo		= uii.Img_RegisterPic ("m_main_logo");

	uiMedia.mainGame		= uii.Img_RegisterPic ("m_main_game");
	uiMedia.mainMultiplayer	= uii.Img_RegisterPic ("m_main_multiplayer");
	uiMedia.mainOptions		= uii.Img_RegisterPic ("m_main_options");
	uiMedia.mainVideo		= uii.Img_RegisterPic ("m_main_video");
	uiMedia.mainQuit		= uii.Img_RegisterPic ("m_main_quit");

	uiMedia.mainGameSel			= uii.Img_RegisterPic ("m_main_game_sel");
	uiMedia.mainMultiplayerSel	= uii.Img_RegisterPic ("m_main_multiplayer_sel");
	uiMedia.mainOptionsSel		= uii.Img_RegisterPic ("m_main_options_sel");
	uiMedia.mainVideoSel		= uii.Img_RegisterPic ("m_main_video_sel");
	uiMedia.mainQuitSel			= uii.Img_RegisterPic ("m_main_quit_sel");
}


/*
=================
UI_Init
=================
*/
void UI_Init (int vidWidth, int vidHeight) {
	uis.vidWidth = vidWidth;
	uis.vidHeight = vidHeight;

	// register media
	UI_RegisterMedia ();

	// cursor init
	UI_CursorInit ();

	// add commands
	uii.Cmd_AddCommand ("menu_main",			UI_MainMenu_f);

		uii.Cmd_AddCommand ("menu_game",			UI_GameMenu_f);
			uii.Cmd_AddCommand ("menu_loadgame",		UI_LoadGameMenu_f);
			uii.Cmd_AddCommand ("menu_savegame",		UI_SaveGameMenu_f);
			uii.Cmd_AddCommand ("menu_credits",			UI_CreditsMenu_f);

		uii.Cmd_AddCommand ("menu_multiplayer",		UI_MultiplayerMenu_f);
			uii.Cmd_AddCommand ("menu_dloptions",		UI_DLOptionsMenu_f);
			uii.Cmd_AddCommand ("menu_joinserver",		UI_JoinServerMenu_f);
				uii.Cmd_AddCommand ("menu_addressbook",		UI_AddressBookMenu_f);
			uii.Cmd_AddCommand ("menu_playerconfig",	UI_PlayerConfigMenu_f);
			uii.Cmd_AddCommand ("menu_startserver",		UI_StartServerMenu_f);
				uii.Cmd_AddCommand ("menu_dmflags",		UI_DMFlagsMenu_f);

		uii.Cmd_AddCommand ("menu_options",			UI_OptionsMenu_f);
			uii.Cmd_AddCommand ("menu_controls",		UI_ControlsMenu_f);
			uii.Cmd_AddCommand ("menu_effects",			UI_EffectsMenu_f);
			uii.Cmd_AddCommand ("menu_fog",				UI_FogMenu_f);
			uii.Cmd_AddCommand ("menu_gloom",			UI_GloomMenu_f);
			uii.Cmd_AddCommand ("menu_hud",				UI_HUDMenu_f);
			uii.Cmd_AddCommand ("menu_input",			UI_InputMenu_f);
			uii.Cmd_AddCommand ("menu_misc",			UI_MiscMenu_f);
			uii.Cmd_AddCommand ("menu_screen",			UI_ScreenMenu_f);
			uii.Cmd_AddCommand ("menu_sound",			UI_SoundMenu_f);

		uii.Cmd_AddCommand ("menu_video",			UI_VideoMenu_f);
			uii.Cmd_AddCommand ("menu_glexts",			UI_GLExtsMenu_f);
			uii.Cmd_AddCommand ("menu_vidsettings",		UI_VIDSettingsMenu_f);

		uii.Cmd_AddCommand ("menu_quit",			UI_QuitMenu_f);
}


/*
=================
UI_Shutdown
=================
*/
void UI_Shutdown (void) {
	// remove commands
	uii.Cmd_RemoveCommand ("menu_main");

		uii.Cmd_RemoveCommand ("menu_game");
			uii.Cmd_RemoveCommand ("menu_loadgame");
			uii.Cmd_RemoveCommand ("menu_savegame");
			uii.Cmd_RemoveCommand ("menu_credits");

		uii.Cmd_RemoveCommand ("menu_multiplayer");
			uii.Cmd_RemoveCommand ("menu_dloptions");
			uii.Cmd_RemoveCommand ("menu_joinserver");
				uii.Cmd_RemoveCommand ("menu_addressbook");
			uii.Cmd_RemoveCommand ("menu_playerconfig");
			uii.Cmd_RemoveCommand ("menu_startserver");
				uii.Cmd_RemoveCommand ("menu_dmflags");

		uii.Cmd_RemoveCommand ("menu_options");
			uii.Cmd_RemoveCommand ("menu_controls");
			uii.Cmd_RemoveCommand ("menu_effects");
			uii.Cmd_RemoveCommand ("menu_fog");
			uii.Cmd_RemoveCommand ("menu_gloom");
			uii.Cmd_RemoveCommand ("menu_hud");
			uii.Cmd_RemoveCommand ("menu_input");
			uii.Cmd_RemoveCommand ("menu_misc");
			uii.Cmd_RemoveCommand ("menu_screen");
			uii.Cmd_RemoveCommand ("menu_sound");

		uii.Cmd_RemoveCommand ("menu_video");
			uii.Cmd_RemoveCommand ("menu_glexts");
			uii.Cmd_RemoveCommand ("menu_vidsettings");

		uii.Cmd_RemoveCommand ("menu_quit");
}

/*
=======================================================================

	MENU HANDLING

=======================================================================
*/

/*
=============
UI_PushMenu
=============
*/
void UI_PushMenu (uiFrameWork_t *m, void (*draw) (void), const char *(*key) (int k)) {
	int		i;

	/* pause single-player games */
	if ((uii.Cvar_VariableValue ("maxclients") == 1) && uis.serverState)
		uii.Cvar_Set ("paused", "1");

	// if this menu is already present, drop back to that level
	// to avoid stacking menus by hotkeys
	for (i=0 ; i<ui_MenuDepth ; i++) {
		if ((ui_Layers[i].m == m) && (ui_Layers[i].draw == draw) && (ui_Layers[i].key == key)) {
			ui_MenuDepth = i;
		}
	}

	if (i == ui_MenuDepth) {
		if (ui_MenuDepth >= MAX_UI_DEPTH)
			uii.Com_Error (ERR_FATAL, "UI_PushMenu: MAX_UI_DEPTH");

		ui_Layers[ui_MenuDepth].m = ui_ActiveUI;
		ui_Layers[ui_MenuDepth].draw = UI_DrawFunc;
		ui_Layers[ui_MenuDepth].key = UI_KeyFunc;

		ui_MenuDepth++;
	}

	UI_DrawFunc = draw;
	UI_KeyFunc = key;
	ui_ActiveUI = m;

	ui_CursorLock = qFalse;
	ui_CursorItem = NULL;
	ui_MouseItem = NULL;
	ui_SelItem = NULL;

	ui_EnterSound = qTrue;

	uii.Key_SetKeyDest (KD_MENU);
	UI_UpdateMousePos ();
}


/*
=============
UI_PopMenu
=============
*/
void UI_PopMenu (void) {
	uii.Snd_StartLocalSound (ui_OutSFX);
	UI_UpdateMousePos ();

	if (ui_MenuDepth < 1)
		uii.Com_Error (ERR_FATAL, "UI_PopMenu: ui_MenuDepth < 1");
	ui_MenuDepth--;

	UI_DrawFunc = ui_Layers[ui_MenuDepth].draw;
	UI_KeyFunc = ui_Layers[ui_MenuDepth].key;
	ui_ActiveUI = ui_Layers[ui_MenuDepth].m;

	ui_CursorItem = NULL;
	ui_MouseItem = NULL;
	ui_SelItem = NULL;

	if (!ui_MenuDepth)
		UI_ForceMenuOff ();
	UI_UpdateMousePos ();
}


/*
=============
UI_ForceMenuOff
=============
*/
void UI_ForceMenuOff (void) {
	UI_DrawFunc = 0;
	UI_KeyFunc = 0;
	ui_ActiveUI = 0;
	ui_MenuDepth = 0;

	ui_CursorItem = NULL;
	ui_MouseItem = NULL;
	ui_SelItem = NULL;

	uii.Key_SetKeyDest (KD_GAME);
	uii.Key_ClearStates ();

	if (mapnames) {
		int i;

		for (i=0 ; i<nummaps ; i++)
			free (mapnames[i]);

		free (mapnames);
	}

	mapnames = 0;
	nummaps = 0;

	uii.Cvar_Set ("paused", "0");
	UI_UpdateMousePos ();
}


/*
=============
UI_Center
=============
*/
void UI_Center (uiFrameWork_t *menu) {
	int		i, flags, type;
	float	y, lowest, highest;
	float	height;

	if (!menu)
		return;

	highest = lowest = height = 0;
	for (i=0 ; i<menu->nitems ; i++) {
		flags	= ((uiCommon_t *) menu->items[i])->flags;
		type	= ((uiCommon_t *) menu->items[i])->type;
		y		= ((uiCommon_t *) menu->items[i])->y;

		if (y < highest)
			highest = y;

		if (type == UITYPE_IMAGE)
			height = (((uiImage_t *) menu->items[i])->image->height);
		else if (flags & UIF_MEDIUM)
			height = UIFT_SIZEINCMED;
		else if (flags & UIF_LARGE)
			height = UIFT_SIZEINCLG;
		else
			height = UIFT_SIZEINC;

		if ((y + height) > lowest)
			lowest = y + height;
	}

	menu->y = (uis.vidHeight - (lowest - highest)) * 0.5 ;
}


/*
=============
UI_SetStatusBar
=============
*/
void UI_SetStatusBar (uiFrameWork_t *m, char *string) {
	m->statusbar = string;
}

/*
=======================================================================

	ITEM SELECTION

=======================================================================
*/

/*
=============
UI_AddItem
=============
*/
void UI_AddItem (uiFrameWork_t *menu, void *item) {
	if (menu->nitems == 0)
		menu->nslots = 0;

	if (item && (menu->nitems < MAXMENUITEMS)) {
		menu->items[menu->nitems] = item;
		((uiCommon_t *) menu->items[menu->nitems])->parent = menu;

		menu->nitems++;
	}

	menu->nslots = menu->nitems;
}


/*
=============
UI_AdjustTextCursor

This function takes the given menu, the direction, and attempts to adjust the menu's
cursor so that it's at the next available slot.
=============
*/
void UI_AdjustTextCursor (uiFrameWork_t *m, int dir) {
	uiCommon_t *citem = NULL;

	if (!m)		return;

	while (m && (dir != 0)) {
		if ((citem = UI_ItemAtCursor (m)) != NULL) {
			ui_CursorItem = citem;
			break;
		}

		m->cursor += dir;

		if (m->cursor >= m->nitems)	m->cursor = 0;
		else if (m->cursor < 0)		m->cursor = m->nitems - 1;
	}
}


/*
=============
UI_ItemAtCursor
=============
*/
void *UI_ItemAtCursor (uiFrameWork_t *m) {
	if (!m)				return NULL;

	if (m->cursor >= m->nitems)	m->cursor = 0;
	else if (m->cursor < 0)		m->cursor = m->nitems - 1;

	if (((uiCommon_t *)m->items[m->cursor])->flags & UIF_NOSELECT)
		return NULL;

	return m->items[m->cursor];
}


/*
=============
UI_SelectItem
=============
*/
void Action_DoEnter (uiAction_t *a) {
	if (a->generic.callback)
		a->generic.callback (a);
}

qBool Field_DoEnter (uiField_t *f) {
	if (f->generic.callback) {
		f->generic.callback (f);
		return qTrue;
	}

	return qFalse;
}

void SpinControl_DoEnter (uiList_t *s) {
	s->curvalue++;
	if (s->itemnames[s->curvalue] == 0)
		s->curvalue = 0;

	if (s->generic.callback)
		s->generic.callback (s);
}

qBool UI_SelectItem (uiFrameWork_t *s) {
	uiCommon_t *item = ((uiCommon_t *) UI_ItemAtCursor (s));

	if (item) {
		switch (item->type) {
		case UITYPE_ACTION:
		case UITYPE_IMAGE:
			Action_DoEnter ((uiAction_t *) item);
			return qTrue;
		case UITYPE_FIELD:
			return Field_DoEnter ((uiField_t *) item);
		}
	}

	return qFalse;
}


/*
=============
UI_SlideItem
=============
*/
void Slider_DoSlide (uiSlider_t *s, int dir) {
	s->curvalue += dir;

	if (s->curvalue > s->maxvalue)
		s->curvalue = s->maxvalue;
	else if (s->curvalue < s->minvalue)
		s->curvalue = s->minvalue;

	if (s->generic.callback)
		s->generic.callback (s);
}

void SpinControl_DoSlide (uiList_t *s, int dir) {
	s->curvalue += dir;

	if (s->curvalue < 0)
		s->curvalue = 0;
	else if (s->itemnames[s->curvalue] == 0)
		s->curvalue--;

	if (s->generic.callback)
		s->generic.callback (s);
}

qBool UI_SlideItem (uiFrameWork_t *s, int dir) {
	uiCommon_t *item = ((uiCommon_t *) UI_ItemAtCursor (s));

	if (item) {
		if ((item)->flags & UIF_NOSELECT)
			return qFalse;

		switch (item->type) {
		case UITYPE_SLIDER:
			Slider_DoSlide ((uiSlider_t *) item, dir);
			return qTrue;
		case UITYPE_SPINCONTROL:
			SpinControl_DoSlide ((uiList_t *) item, dir);
			return qTrue;
		}
	}

	return qFalse;
}

//======================================================================

#ifndef UI_HARD_LINKED
void Com_Printf (int flags, char *fmt, ...) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	vsprintf (text, fmt, argptr);
	va_end (argptr);

	uii.Com_Printf (flags, text);
}
#endif
