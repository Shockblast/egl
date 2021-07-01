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
// ui_backend.c
//

#include "ui_local.h"

qBool		uiEnterSound;
qBool		uiExitSound;

uiState_t	uis;
uiMedia_t	uiMedia;

uiLayer_t	uiLayers[MAX_UI_DEPTH];
static int	uiMenuDepth;

uiFrameWork_t	*uiActiveUI;

void	(*UI_DrawFunc) (void);

cVar_t	*ui_sensitivity;

/*
=======================================================================

	UI MEDIA

=======================================================================
*/

/*
================
UI_InitSoundMedia

Called on UI init and on snd_restart
================
*/
void UI_InitSoundMedia (void) {
	// sounds
	uiMedia.menuInSfx			= uii.Snd_RegisterSound ("misc/menu1.wav");
	uiMedia.menuMoveSfx			= uii.Snd_RegisterSound ("misc/menu2.wav");
	uiMedia.menuOutSfx			= uii.Snd_RegisterSound ("misc/menu3.wav");
}


/*
=================
UI_InitMedia
=================
*/
static void UI_InitMedia (void) {
	char	cursorName[MAX_QPATH];
	int		i;

	if (uiMedia.initialized)
		return;
	uiMedia.initialized = qTrue;

	// sound media
	UI_InitSoundMedia ();

	// generic texture
	uiMedia.whiteTexture		= uii.Img_FindImage ("***r_WhiteTexture***", 0);

	// chars image
	uiMedia.uiCharsImage		= uii.Img_RegisterPic ("/egl/ui/uichars.pcx");

	// background images
	uiMedia.bgBig				= uii.Img_RegisterPic ("/egl/ui/bg_big.pcx");

	// cursor image
	uiMedia.cursorImage			= uii.Img_RegisterPic ("/egl/ui/cursor.pcx");

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
		Q_snprintfz (cursorName, sizeof (cursorName), "m_cursor%d", i);
		uiMedia.mainCursors[i]		= uii.Img_RegisterPic (cursorName);
	}

	// main menu
	uiMedia.mainPlaque			= uii.Img_RegisterPic ("m_main_plaque");
	uiMedia.mainLogo			= uii.Img_RegisterPic ("m_main_logo");

	uiMedia.mainGame			= uii.Img_RegisterPic ("m_main_game");
	uiMedia.mainMultiplayer		= uii.Img_RegisterPic ("m_main_multiplayer");
	uiMedia.mainOptions			= uii.Img_RegisterPic ("m_main_options");
	uiMedia.mainVideo			= uii.Img_RegisterPic ("m_main_video");
	uiMedia.mainQuit			= uii.Img_RegisterPic ("m_main_quit");

	uiMedia.mainGameSel			= uii.Img_RegisterPic ("m_main_game_sel");
	uiMedia.mainMultiplayerSel	= uii.Img_RegisterPic ("m_main_multiplayer_sel");
	uiMedia.mainOptionsSel		= uii.Img_RegisterPic ("m_main_options_sel");
	uiMedia.mainVideoSel		= uii.Img_RegisterPic ("m_main_video_sel");
	uiMedia.mainQuitSel			= uii.Img_RegisterPic ("m_main_quit_sel");
}


/*
=================
UI_ShutdownMedia
=================
*/
static void UI_ShutdownMedia (void) {
	if (!uiMedia.initialized)
		return;

	uiMedia.initialized = qFalse;
}

/*
=======================================================================

	UI INIT/SHUTDOWN

=======================================================================
*/

/*
=================
UI_Init
=================
*/
void UI_Init (int vidWidth, int vidHeight) {
	// clear everything
	memset (&uis, 0, sizeof (uis));
	memset (&uiMedia, 0, sizeof (uiMedia));

	// collect new values
	uis.vidWidth = vidWidth;
	uis.vidHeight = vidHeight;

	// register media
	UI_InitMedia ();

	// cursor init
	UI_CursorInit ();

	// register cvars
	ui_sensitivity		= uii.Cvar_Get ("ui_sensitivity",	"0.5",		CVAR_ARCHIVE);

	// add commands
	uii.Cmd_AddCommand ("menu_main",					UI_MainMenu_f,					"Opens the main menu");

		uii.Cmd_AddCommand ("menu_game",				UI_GameMenu_f,					"Opens the single player menu");
			uii.Cmd_AddCommand ("menu_loadgame",		UI_LoadGameMenu_f,				"Opens the load game menu");
			uii.Cmd_AddCommand ("menu_savegame",		UI_SaveGameMenu_f,				"Opens the save game menu");
			uii.Cmd_AddCommand ("menu_credits",			UI_CreditsMenu_f,				"Opens the credits menu");

		uii.Cmd_AddCommand ("menu_multiplayer",			UI_MultiplayerMenu_f,			"Opens the multiplayer menu");
			uii.Cmd_AddCommand ("menu_dloptions",		UI_DLOptionsMenu_f,				"Opens the download options menu");
			uii.Cmd_AddCommand ("menu_joinserver",		UI_JoinServerMenu_f,			"Opens the join server menu");
				uii.Cmd_AddCommand ("menu_addressbook",	UI_AddressBookMenu_f,			"Opens the address book menu");
			uii.Cmd_AddCommand ("menu_playerconfig",	UI_PlayerConfigMenu_f,			"Opens the player configuration menu");
			uii.Cmd_AddCommand ("menu_startserver",		UI_StartServerMenu_f,			"Opens the start server menu");
				uii.Cmd_AddCommand ("menu_dmflags",		UI_DMFlagsMenu_f,				"Opens the deathmatch flags menu");

		uii.Cmd_AddCommand ("menu_options",				UI_OptionsMenu_f,				"Opens the options menu");
			uii.Cmd_AddCommand ("menu_controls",		UI_ControlsMenu_f,				"Opens the controls menu");
			uii.Cmd_AddCommand ("menu_effects",			UI_EffectsMenu_f,				"Opens the effects menu");
			uii.Cmd_AddCommand ("menu_fog",				UI_FogMenu_f,					"Opens the fog menu");
			uii.Cmd_AddCommand ("menu_gloom",			UI_GloomMenu_f,					"Opens the gloom menu");
			uii.Cmd_AddCommand ("menu_hud",				UI_HUDMenu_f,					"Opens the hud menu");
			uii.Cmd_AddCommand ("menu_input",			UI_InputMenu_f,					"Opens the input menu");
			uii.Cmd_AddCommand ("menu_misc",			UI_MiscMenu_f,					"Opens the misc menu");
			uii.Cmd_AddCommand ("menu_screen",			UI_ScreenMenu_f,				"Opens the screen menu");
			uii.Cmd_AddCommand ("menu_sound",			UI_SoundMenu_f,					"Opens the sound menu");

		uii.Cmd_AddCommand ("menu_video",				UI_VideoMenu_f,					"Opens the video menu");
			uii.Cmd_AddCommand ("menu_glexts",			UI_GLExtsMenu_f,				"Opens the opengl extensions menu");
			uii.Cmd_AddCommand ("menu_vidsettings",		UI_VIDSettingsMenu_f,			"Opens the video settings menu");

		uii.Cmd_AddCommand ("menu_quit",				UI_QuitMenu_f,					"Opens the quit menu");
}


/*
=================
UI_Shutdown
=================
*/
void UI_Shutdown (void) {
	UI_ShutdownMedia ();

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
void UI_PushMenu (uiFrameWork_t *m, void (*draw) (void), struct sfx_s *(*key) (int k)) {
	int		i;

	// pause single-player games
	if ((uii.Cvar_VariableValue ("maxclients") == 1) && uis.serverState)
		uii.Cvar_Set ("paused", "1");

	// if this menu is already present, drop back to that level
	// to avoid stacking menus by hotkeys
	for (i=0 ; i<uiMenuDepth ; i++) {
		if ((uiLayers[i].m == m) && (uiLayers[i].draw == draw) && (uiLayers[i].key == key)) {
			uiMenuDepth = i;
		}
	}

	if (i == uiMenuDepth) {
		if (uiMenuDepth >= MAX_UI_DEPTH)
			Com_Error (ERR_FATAL, "UI_PushMenu: MAX_UI_DEPTH");

		uiLayers[uiMenuDepth].m = uiActiveUI;
		uiLayers[uiMenuDepth].draw = UI_DrawFunc;
		uiLayers[uiMenuDepth].key = UI_KeyFunc;

		uiMenuDepth++;
	}

	UI_DrawFunc = draw;
	UI_KeyFunc = key;
	uiActiveUI = m;

	uiCursorLock = qFalse;
	uiCursorItem = NULL;
	uiMouseItem = NULL;
	uiSelItem = NULL;

	uiEnterSound = qTrue;
	uiExitSound = qTrue;

	uii.Key_SetKeyDest (KD_MENU);
	UI_UpdateMousePos ();
}


/*
=============
UI_PopMenu
=============
*/
void UI_PopMenu (void) {
	uii.Snd_StartLocalSound (uiMedia.menuOutSfx);
	UI_UpdateMousePos ();

	if (uiMenuDepth < 1)
		Com_Error (ERR_FATAL, "UI_PopMenu: uiMenuDepth < 1");
	uiMenuDepth--;

	UI_DrawFunc = uiLayers[uiMenuDepth].draw;
	UI_KeyFunc = uiLayers[uiMenuDepth].key;
	uiActiveUI = uiLayers[uiMenuDepth].m;

	uiCursorItem = NULL;
	uiMouseItem = NULL;
	uiSelItem = NULL;

	uiExitSound = qFalse;
	if (!uiMenuDepth)
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
	uiActiveUI = 0;
	uiMenuDepth = 0;

	uiCursorItem = NULL;
	uiMouseItem = NULL;
	uiSelItem = NULL;

	uii.Key_SetKeyDest (KD_GAME);
	uii.Key_ClearStates ();

	StartServerMenu_ClearMapNames ();

	uii.Cvar_Set ("paused", "0");
	UI_UpdateMousePos ();

	if (uiExitSound) {
		uii.Snd_StartLocalSound (uiMedia.menuOutSfx);
		uiExitSound = qFalse;
	}
}


/*
=============
UI_Center
=============
*/
void UI_Center (uiFrameWork_t *menu) {
	int		i, flags, type;
	float	y, lowest, highest;
	int		height;

	if (!menu)
		return;

	highest = lowest = height = 0;
	for (i=0 ; i<menu->numItems ; i++) {
		flags	= ((uiCommon_t *) menu->items[i])->flags;
		type	= ((uiCommon_t *) menu->items[i])->type;
		y		= ((uiCommon_t *) menu->items[i])->y;

		if (y < highest)
			highest = y;

		if (type == UITYPE_IMAGE)
			uii.Img_GetSize (((uiImage_t *) menu->items[i])->image, NULL, &height);
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
	m->statusBar = string;
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
	if (item && (menu->numItems < MAXMENUITEMS)) {
		menu->items[menu->numItems] = item;
		((uiCommon_t *) menu->items[menu->numItems])->parent = menu;

		menu->numItems++;
	}
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
			uiCursorItem = citem;
			break;
		}

		m->cursor += dir;

		if (m->cursor >= m->numItems)	m->cursor = 0;
		else if (m->cursor < 0)			m->cursor = m->numItems - 1;
	}
}


/*
=============
UI_ItemAtCursor
=============
*/
void *UI_ItemAtCursor (uiFrameWork_t *m) {
	if (!m)				return NULL;

	if (m->cursor >= m->numItems)	m->cursor = 0;
	else if (m->cursor < 0)			m->cursor = m->numItems - 1;

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
	if (a->generic.callBack)
		a->generic.callBack (a);
}

qBool Field_DoEnter (uiField_t *f) {
	if (f->generic.callBack) {
		f->generic.callBack (f);
		return qTrue;
	}

	return qFalse;
}

void SpinControl_DoEnter (uiList_t *s) {
	s->curValue++;
	if (s->itemNames[s->curValue] == 0)
		s->curValue = 0;

	if (s->generic.callBack)
		s->generic.callBack (s);
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
	s->curValue += dir;

	if (s->curValue > s->maxValue)
		s->curValue = s->maxValue;
	else if (s->curValue < s->minValue)
		s->curValue = s->minValue;

	if (s->generic.callBack)
		s->generic.callBack (s);
}

void SpinControl_DoSlide (uiList_t *s, int dir) {
	s->curValue += dir;

	if (s->curValue < 0)
		s->curValue = 0;
	else if (s->itemNames[s->curValue] == 0)
		s->curValue--;

	if (s->generic.callBack)
		s->generic.callBack (s);
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

/*
==================
UI_StrDup

No need to null terminate the extra spot because Mem_Alloc returns zero-filled memory
==================
*/
char *UI_StrDup (const char *in) {
	char	*out;

	out = UI_MemAlloc (strlen(in)+1);
	strcpy (out, in);

	return out;
}

//======================================================================

#ifndef UI_HARD_LINKED
/*
==================
Com_Printf
==================
*/
void Com_Printf (int flags, char *fmt, ...) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	vsnprintf (text, sizeof (text), fmt, argptr);
	va_end (argptr);

	uii.Com_Printf (flags, text);
}


/*
==================
Com_Error
==================
*/
void Com_Error (int code, char *fmt, ...) {
	va_list		argptr;
	char		msg[1024];

	va_start (argptr, fmt);
	vsnprintf (msg, sizeof (msg), fmt, argptr);
	va_end (argptr);

	uii.Com_Error (code, msg);
}
#endif
