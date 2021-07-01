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

qBool		ui_playEnterSound;
qBool		ui_playExitSound;

uiMedia_t	uiMedia;
uiState_t	uiState;

typedef struct uiLayer_s {
	uiFrameWork_t	*frameWork;
	void			(*draw) (void);
	struct sfx_s	*(*close) (void);
	struct sfx_s	*(*key) (int k);
} uiLayer_t;

uiLayer_t	ui_menuLayers[MAX_UI_DEPTH];
static int	ui_menuLayerDepth;

uiFrameWork_t	*UI_ActiveUI;
void			(*UI_DrawFunc) (void);
struct sfx_s	*(*UI_CloseFunc) (void);
struct sfx_s	*(*UI_KeyFunc) (int key);

cVar_t	*ui_filtermouse;
cVar_t	*ui_sensitivity;

cVar_t	*ui_jsMenuPage;
cVar_t	*ui_jsSortItem;
cVar_t	*ui_jsSortMethod;

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
void UI_InitSoundMedia (void)
{
	// Sounds
	uiMedia.sounds.menuIn		= uii.Snd_RegisterSound ("misc/menu1.wav");
	uiMedia.sounds.menuMove		= uii.Snd_RegisterSound ("misc/menu2.wav");
	uiMedia.sounds.menuOut		= uii.Snd_RegisterSound ("misc/menu3.wav");
}


/*
=================
UI_InitMedia
=================
*/
void UI_InitMedia (void)
{
	int		i;

	// Sound media
	UI_InitSoundMedia ();

	uiMedia.blackTexture			= uii.R_RegisterTexture ("***r_blackTexture***", -1);
	uiMedia.noTexture				= uii.R_RegisterTexture ("***r_noTexture***", -1);
	uiMedia.whiteTexture			= uii.R_RegisterTexture ("***r_whiteTexture***", -1);

	uiMedia.conCharsShader			= uii.R_RegisterPic (Q_VarArgs ("fonts/%s.tga", uii.Cvar_VariableString ("con_font")));
	if (!uiMedia.conCharsShader)
		uiMedia.conCharsShader		= uii.R_RegisterPic ("pics/conchars.tga");
	uiMedia.uiCharsShader			= uii.R_RegisterPic ("egl/ui/uichars.tga");

	uiMedia.bgBig					= uii.R_RegisterPic ("egl/ui/bg_big.tga");

	uiMedia.cursorShader			= uii.R_RegisterPic ("egl/ui/cursor.tga");
	uiMedia.cursorHoverShader		= uii.R_RegisterPic ("egl/ui/cursorhover.tga");

	// Banners
	uiMedia.banners.addressBook		= uii.R_RegisterPic ("pics/m_banner_addressbook.tga");
	uiMedia.banners.multiplayer		= uii.R_RegisterPic ("pics/m_banner_multiplayer.tga");
	uiMedia.banners.startServer		= uii.R_RegisterPic ("pics/m_banner_start_server.tga");
	uiMedia.banners.joinServer		= uii.R_RegisterPic ("pics/m_banner_join_server.tga");
	uiMedia.banners.options			= uii.R_RegisterPic ("pics/m_banner_options.tga");
	uiMedia.banners.game			= uii.R_RegisterPic ("pics/m_banner_game.tga");
	uiMedia.banners.loadGame		= uii.R_RegisterPic ("pics/m_banner_load_game.tga");
	uiMedia.banners.saveGame		= uii.R_RegisterPic ("pics/m_banner_save_game.tga");
	uiMedia.banners.video			= uii.R_RegisterPic ("pics/m_banner_video.tga");
	uiMedia.banners.quit			= uii.R_RegisterPic ("pics/m_main_quit.tga");

	// Main menu cursors
	for (i=0 ; i<MAINMENU_CURSOR_NUMFRAMES ; i++) {
		uiMedia.menus.mainCursors[i]	= uii.R_RegisterPic (Q_VarArgs ("pics/m_cursor%d.tga", i));
	}

	// Main menu
	uiMedia.menus.mainPlaque			= uii.R_RegisterPic ("pics/m_main_plaque.tga");
	uiMedia.menus.mainLogo				= uii.R_RegisterPic ("pics/m_main_logo.tga");

	uiMedia.menus.mainGame				= uii.R_RegisterPic ("pics/m_main_game.tga");
	uiMedia.menus.mainMultiplayer		= uii.R_RegisterPic ("pics/m_main_multiplayer.tga");
	uiMedia.menus.mainOptions			= uii.R_RegisterPic ("pics/m_main_options.tga");
	uiMedia.menus.mainVideo				= uii.R_RegisterPic ("pics/m_main_video.tga");
	uiMedia.menus.mainQuit				= uii.R_RegisterPic ("pics/m_main_quit.tga");

	uiMedia.menus.mainGameSel			= uii.R_RegisterPic ("pics/m_main_game_sel.tga");
	uiMedia.menus.mainMultiplayerSel	= uii.R_RegisterPic ("pics/m_main_multiplayer_sel.tga");
	uiMedia.menus.mainOptionsSel		= uii.R_RegisterPic ("pics/m_main_options_sel.tga");
	uiMedia.menus.mainVideoSel			= uii.R_RegisterPic ("pics/m_main_video_sel.tga");
	uiMedia.menus.mainQuitSel			= uii.R_RegisterPic ("pics/m_main_quit_sel.tga");
}

/*
=======================================================================

	UI INIT/SHUTDOWN

=======================================================================
*/

static cmdFunc_t	*cmd_menuMain;

static cmdFunc_t	*cmd_menuGame;
static cmdFunc_t	*cmd_menuLoadGame;
static cmdFunc_t	*cmd_menuSaveGame;
static cmdFunc_t	*cmd_menuCredits;

static cmdFunc_t	*cmd_menuMultiplayer;
static cmdFunc_t	*cmd_menuDLOptions;
static cmdFunc_t	*cmd_menuJoinServer;
static cmdFunc_t	*cmd_menuAddressBook;
static cmdFunc_t	*cmd_menuPlayerConfig;
static cmdFunc_t	*cmd_menuStartServer;
static cmdFunc_t	*cmd_menuDMFlags;

static cmdFunc_t	*cmd_menuOptions;
static cmdFunc_t	*cmd_menuControls;
static cmdFunc_t	*cmd_menuEffects;
static cmdFunc_t	*cmd_menuGloom;
static cmdFunc_t	*cmd_menuHUD;
static cmdFunc_t	*cmd_menuInput;
static cmdFunc_t	*cmd_menuMisc;
static cmdFunc_t	*cmd_menuScreen;
static cmdFunc_t	*cmd_menuSound;

static cmdFunc_t	*cmd_menuVideo;
static cmdFunc_t	*cmd_menuGLExts;
static cmdFunc_t	*cmd_menuVidSettings;

static cmdFunc_t	*cmd_menuQuit;

static cmdFunc_t	*cmd_startSStatus;

/*
=================
UI_Init
=================
*/
void JoinMenu_StartSStatus (void);
void UI_Init (int vidWidth, int vidHeight, float oldCursorX, float oldCursorY)
{
	int		i;

	if (uiState.initialized)
		return;

	// Clear everything
	memset (&uiMedia, 0, sizeof (uiMedia));
	memset (&uiState, 0, sizeof (uiState));

	// Collect new values
	uiState.vidWidth = vidWidth;
	uiState.vidHeight = vidHeight;

	// Draw loading screen
	UI_Refresh (0, 0, 0, vidWidth, vidHeight, qTrue);

	// Get the old cursor position from the client so it will stop resetting so much
	UI_SetCursorPos (oldCursorX, oldCursorY);

	// Register media
	UI_InitMedia ();

	// Cursor init
	UI_CursorInit ();

	// Register cvars
	for (i=0 ; i<MAX_ADDRBOOK_SAVES ; i++)
		uii.Cvar_Get (Q_VarArgs ("adr%i", i),				"",			CVAR_ARCHIVE);

	ui_jsMenuPage		= uii.Cvar_Get ("ui_jsMenuPage",	"0",		CVAR_ARCHIVE);
	ui_jsSortItem		= uii.Cvar_Get ("ui_jsSortItem",	"0",		CVAR_ARCHIVE);
	ui_jsSortMethod		= uii.Cvar_Get ("ui_jsSortMethod",	"0",		CVAR_ARCHIVE);

	ui_filtermouse		= uii.Cvar_Get ("ui_filtermouse",	"1",		CVAR_ARCHIVE);
	ui_sensitivity		= uii.Cvar_Get ("ui_sensitivity",	"2",		CVAR_ARCHIVE);

	// Add commands
	cmd_menuMain		= uii.Cmd_AddCommand ("menu_main",			UI_MainMenu_f,				"Opens the main menu");

	cmd_menuGame		= uii.Cmd_AddCommand ("menu_game",			UI_GameMenu_f,				"Opens the single player menu");
	cmd_menuLoadGame	= uii.Cmd_AddCommand ("menu_loadgame",		UI_LoadGameMenu_f,			"Opens the load game menu");
	cmd_menuSaveGame	= uii.Cmd_AddCommand ("menu_savegame",		UI_SaveGameMenu_f,			"Opens the save game menu");
	cmd_menuCredits		= uii.Cmd_AddCommand ("menu_credits",		UI_CreditsMenu_f,			"Opens the credits menu");

	cmd_menuMultiplayer	= uii.Cmd_AddCommand ("menu_multiplayer",	UI_MultiplayerMenu_f,		"Opens the multiplayer menu");
	cmd_menuDLOptions	= uii.Cmd_AddCommand ("menu_dloptions",		UI_DLOptionsMenu_f,			"Opens the download options menu");
	cmd_menuJoinServer	= uii.Cmd_AddCommand ("menu_joinserver",	UI_JoinServerMenu_f,		"Opens the join server menu");
	cmd_menuAddressBook	= uii.Cmd_AddCommand ("menu_addressbook",	UI_AddressBookMenu_f,		"Opens the address book menu");
	cmd_menuPlayerConfig= uii.Cmd_AddCommand ("menu_playerconfig",	UI_PlayerConfigMenu_f,		"Opens the player configuration menu");
	cmd_menuStartServer	= uii.Cmd_AddCommand ("menu_startserver",	UI_StartServerMenu_f,		"Opens the start server menu");
	cmd_menuDMFlags		= uii.Cmd_AddCommand ("menu_dmflags",		UI_DMFlagsMenu_f,			"Opens the deathmatch flags menu");

	cmd_menuOptions		= uii.Cmd_AddCommand ("menu_options",		UI_OptionsMenu_f,			"Opens the options menu");
	cmd_menuControls	= uii.Cmd_AddCommand ("menu_controls",		UI_ControlsMenu_f,			"Opens the controls menu");
	cmd_menuEffects		= uii.Cmd_AddCommand ("menu_effects",		UI_EffectsMenu_f,			"Opens the effects menu");
	cmd_menuGloom		= uii.Cmd_AddCommand ("menu_gloom",			UI_GloomMenu_f,				"Opens the gloom menu");
	cmd_menuHUD			= uii.Cmd_AddCommand ("menu_hud",			UI_HUDMenu_f,				"Opens the hud menu");
	cmd_menuInput		= uii.Cmd_AddCommand ("menu_input",			UI_InputMenu_f,				"Opens the input menu");
	cmd_menuMisc		= uii.Cmd_AddCommand ("menu_misc",			UI_MiscMenu_f,				"Opens the misc menu");
	cmd_menuScreen		= uii.Cmd_AddCommand ("menu_screen",		UI_ScreenMenu_f,			"Opens the screen menu");
	cmd_menuSound		= uii.Cmd_AddCommand ("menu_sound",			UI_SoundMenu_f,				"Opens the sound menu");

	cmd_menuVideo		= uii.Cmd_AddCommand ("menu_video",			UI_VideoMenu_f,				"Opens the video menu");
	cmd_menuGLExts		= uii.Cmd_AddCommand ("menu_glexts",		UI_GLExtsMenu_f,			"Opens the opengl extensions menu");
	cmd_menuVidSettings	= uii.Cmd_AddCommand ("menu_vidsettings",	UI_VIDSettingsMenu_f,		"Opens the video settings menu");

	cmd_menuQuit		= uii.Cmd_AddCommand ("menu_quit",			UI_QuitMenu_f,				"Opens the quit menu");

	cmd_startSStatus	= uii.Cmd_AddCommand ("ui_startSStatus",	JoinMenu_StartSStatus,		"");

	// Done
	uiState.initialized = qTrue;
}


/*
=================
UI_Shutdown
=================
*/
void UI_Shutdown (void)
{
	if (!uiState.initialized)
		return;

	ui_playExitSound = qFalse;
	UI_ForceMenuOff ();

	// Send the old cursor position to the client
	uii.StoreCursorPos (uiState.cursorX, uiState.cursorY);

	// Remove commands
	uii.Cmd_RemoveCommand ("menu_main", cmd_menuMain);

	uii.Cmd_RemoveCommand ("menu_game", cmd_menuGame);
	uii.Cmd_RemoveCommand ("menu_loadgame", cmd_menuLoadGame);
	uii.Cmd_RemoveCommand ("menu_savegame", cmd_menuSaveGame);
	uii.Cmd_RemoveCommand ("menu_credits", cmd_menuCredits);

	uii.Cmd_RemoveCommand ("menu_multiplayer", cmd_menuMultiplayer);
	uii.Cmd_RemoveCommand ("menu_dloptions", cmd_menuDLOptions);
	uii.Cmd_RemoveCommand ("menu_joinserver", cmd_menuJoinServer);
	uii.Cmd_RemoveCommand ("menu_addressbook", cmd_menuAddressBook);
	uii.Cmd_RemoveCommand ("menu_playerconfig", cmd_menuPlayerConfig);
	uii.Cmd_RemoveCommand ("menu_startserver", cmd_menuStartServer);
	uii.Cmd_RemoveCommand ("menu_dmflags", cmd_menuDMFlags);

	uii.Cmd_RemoveCommand ("menu_options", cmd_menuOptions);
	uii.Cmd_RemoveCommand ("menu_controls", cmd_menuControls);
	uii.Cmd_RemoveCommand ("menu_effects", cmd_menuEffects);
	uii.Cmd_RemoveCommand ("menu_gloom", cmd_menuGloom);
	uii.Cmd_RemoveCommand ("menu_hud", cmd_menuHUD);
	uii.Cmd_RemoveCommand ("menu_input", cmd_menuInput);
	uii.Cmd_RemoveCommand ("menu_misc", cmd_menuMisc);
	uii.Cmd_RemoveCommand ("menu_screen", cmd_menuScreen);
	uii.Cmd_RemoveCommand ("menu_sound", cmd_menuSound);

	uii.Cmd_RemoveCommand ("menu_video", cmd_menuVideo);
	uii.Cmd_RemoveCommand ("menu_glexts", cmd_menuGLExts);
	uii.Cmd_RemoveCommand ("menu_vidsettings", cmd_menuVidSettings);

	uii.Cmd_RemoveCommand ("menu_quit", cmd_menuQuit);

	uii.Cmd_RemoveCommand ("ui_startSStatus", cmd_startSStatus);

	// Done
	uiState.initialized = qFalse;
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
void UI_PushMenu (uiFrameWork_t *frameWork, void (*draw) (void), struct sfx_s *(*close) (void), struct sfx_s *(*key) (int k))
{
	int		i;

	// Pause single-player games
	if (uii.Cvar_VariableValue ("maxclients") == 1 && uiState.serverState)
		uii.Cvar_Set ("paused", "1");

	// If this menu is already present, drop back to that level
	// to avoid stacking menus by hotkeys
	for (i=0 ; i<ui_menuLayerDepth ; i++) {
		if (ui_menuLayers[i].frameWork != frameWork)
			continue;
		if (ui_menuLayers[i].close != close)
			continue;
		if (ui_menuLayers[i].draw != draw)
			continue;
		if (ui_menuLayers[i].key != key)
			continue;

		ui_menuLayerDepth = i;
	}

	// Create a new layer
	if (i == ui_menuLayerDepth) {
		if (ui_menuLayerDepth >= MAX_UI_DEPTH)
			Com_Error (ERR_FATAL, "UI_PushMenu: MAX_UI_DEPTH");

		ui_menuLayers[ui_menuLayerDepth].frameWork = UI_ActiveUI;
		ui_menuLayers[ui_menuLayerDepth].draw = UI_DrawFunc;
		ui_menuLayers[ui_menuLayerDepth].close = UI_CloseFunc;
		ui_menuLayers[ui_menuLayerDepth].key = UI_KeyFunc;

		ui_menuLayerDepth++;
	}

	UI_ActiveUI = frameWork;
	UI_DrawFunc = draw;
	UI_CloseFunc = close;
	UI_KeyFunc = key;

	ui_cursorLock = qFalse;
	ui_cursorItem = NULL;
	ui_mouseItem = NULL;
	ui_selectedItem = NULL;

	ui_playEnterSound = qTrue;
	ui_playExitSound = qTrue;

	uii.Key_SetKeyDest (KD_MENU);
	UI_UpdateMousePos ();
}


/*
=============
UI_PopMenu
=============
*/
void UI_PopMenu (void)
{
	struct sfx_s	*outSound;

	if (ui_menuLayerDepth == 1) {
		// Start the demo loop again
		if (uiState.connectState < CA_CONNECTING)
			return;

		UI_ForceMenuOff ();
		return;
	}

	// Call close function
	if (UI_ActiveUI && UI_CloseFunc) {
		outSound = UI_CloseFunc ();
		UI_ActiveUI->initialized = qFalse;
		UI_ActiveUI->numItems = 0;
	}
	else
		outSound = uiMedia.sounds.menuOut;

	if (outSound)
		uii.Snd_StartLocalSound (outSound);

	// Update mouse position
	UI_UpdateMousePos ();

	// Fall back a layer
	if (ui_menuLayerDepth < 1)
		Com_Error (ERR_FATAL, "UI_PopMenu: ui_menuLayerDepth < 1");
	ui_menuLayerDepth--;

	UI_ActiveUI = ui_menuLayers[ui_menuLayerDepth].frameWork;
	UI_DrawFunc = ui_menuLayers[ui_menuLayerDepth].draw;
	UI_CloseFunc = ui_menuLayers[ui_menuLayerDepth].close;
	UI_KeyFunc = ui_menuLayers[ui_menuLayerDepth].key;

	ui_cursorItem = NULL;
	ui_mouseItem = NULL;
	ui_selectedItem = NULL;

	// Play exit sound
	ui_playExitSound = qFalse;
	if (!ui_menuLayerDepth)
		UI_ForceMenuOff ();

	// Update mouse position
	UI_UpdateMousePos ();
}


/*
=============
UI_ForceMenuOff
=============
*/
void UI_ForceMenuOff (void)
{
	int		i;

	// Call all close functions
	if (UI_ActiveUI && UI_CloseFunc) {
		UI_CloseFunc ();
		UI_ActiveUI->initialized = qFalse;
		UI_ActiveUI->numItems = 0;
	}
	for (i=ui_menuLayerDepth-1 ; i>=0 ; i--) {
		if (!ui_menuLayers[i].close)
			continue;
		if (ui_menuLayers[i].close == UI_CloseFunc)
			continue;

		ui_menuLayers[i].close ();
		ui_menuLayers[i].frameWork->initialized = qFalse;
		ui_menuLayers[i].frameWork->numItems = 0;
	}

	// Clear values
	UI_ActiveUI = NULL;
	UI_DrawFunc = NULL;
	UI_CloseFunc = NULL;
	UI_KeyFunc = NULL;

	ui_menuLayerDepth = 0;

	ui_cursorItem = NULL;
	ui_mouseItem = NULL;
	ui_selectedItem = NULL;

	// Back to game and clear key states
	uii.Key_SetKeyDest (KD_GAME);
	uii.Key_ClearStates ();

	// Unpause
	uii.Cvar_Set ("paused", "0");

	// Update mouse position
	UI_UpdateMousePos ();

	// Play exit sound
	if (ui_playExitSound) {
		uii.Snd_StartLocalSound (uiMedia.sounds.menuOut);
		ui_playExitSound = qFalse;
	}
}


/*
=============
UI_CenterMenuHeight
=============
*/
void UI_CenterMenuHeight (uiFrameWork_t *menu)
{
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
			height = ((uiImage_t *) menu->items[i])->height * UISCALE_TYPE (flags);
		else if (flags & UIF_MEDIUM)
			height = UIFT_SIZEINCMED;
		else if (flags & UIF_LARGE)
			height = UIFT_SIZEINCLG;
		else
			height = UIFT_SIZEINC;

		if (y + height > lowest)
			lowest = y + height;
	}

	menu->y += (uiState.vidHeight - (lowest - highest)) * 0.5f;
}

/*
=======================================================================

	FRAMEWORK HANDLING

=======================================================================
*/

/*
=============
UI_StartFramework
=============
*/
void UI_StartFramework (uiFrameWork_t *menu)
{
	menu->locked = qFalse;
	menu->initialized = qFalse;

	// No items yet
	menu->numItems = 0;
}


/*
=============
UI_FinishFramework
=============
*/
void UI_FinishFramework (uiFrameWork_t *menu, qBool lock)
{
	uiImage_t	*image;
	int			i;

	if (!menu->numItems)
		Com_Error (ERR_FATAL, "UI_FinishFramework: Framework has no items");

	menu->locked = lock;
	menu->initialized = qTrue;

	// Do anything item-specific work now
	for (i=0 ; i<menu->numItems ; i++) {
		switch (((uiCommon_t *) menu->items[i])->type) {
		case UITYPE_IMAGE:
			image = menu->items[i];

			uii.R_GetImageSize (image->shader, &image->width, &image->height);
			break;
		}
	}
}


/*
=============
UI_AddItem
=============
*/
void UI_AddItem (uiFrameWork_t *menu, void *item)
{
	uiList_t	*list;
	int			i;

	if (!item)
		return;
	if (menu->numItems >= MAX_UI_ITEMS)
		Com_Error (ERR_FATAL, "UI_AddItem: MAX_UI_ITEMS hit");
	if (menu->locked)
		Com_Error (ERR_FATAL, "UI_AddItem: Attempted to add item when framework is locked");

	// Check to see if it already exists
	for (i=0 ; i<menu->numItems ; i++) {
		if (menu->items[i] == item)
			Com_Error (ERR_FATAL, "UI_AddItem: Attempted to add item that is already in list");
	}

	// Add to list and set parent
	menu->items[menu->numItems] = item;
	((uiCommon_t *) menu->items[menu->numItems])->parent = menu;

	// Do necessary work
	if (((uiCommon_t *) menu->items[menu->numItems])->type == UITYPE_SPINCONTROL) {
		list = menu->items[menu->numItems];

		for (i=0 ; list->itemNames[i] ; i++) ;
		list->numItemNames = i;
	}

	menu->numItems++;
}


/*
=============
UI_RemoveItem
=============
*/
void UI_RemoveItem (uiFrameWork_t *menu, void *item)
{
	int		i;
	qBool	found;

	if (!item)
		return;
	if (menu->locked)
		Com_Error (ERR_FATAL, "UI_RemoveItem: Attempted to remove item when framework is locked");

	// Pull the list backwards starting at the item
	found = qFalse;
	for (i=0 ; i<menu->numItems ; i++) {
		if (found) {
			menu->items[i-1] = menu->items[i];
		}
		else if (menu->items[i] == item)
			found = qTrue;
	}

	// Remove the last entry, since it's now a duplicate
	if (found) {
		menu->items[menu->numItems-1] = NULL;
		menu->numItems--;
	}
}


/*
=============
UI_AdjustCursor

This function takes the given menu, the direction, and attempts to adjust the menu's
cursor so that it's at the next available slot.
=============
*/
void UI_AdjustCursor (uiFrameWork_t *m, int dir)
{
	uiCommon_t *curItem = NULL;

	if (!m)
		return;
	if (!m->numItems)
		return;

	while (m && dir != 0) {
		if ((curItem = UI_ItemAtCursor (m)) != NULL) {
			ui_cursorItem = curItem;
			break;
		}

		m->cursor += dir;
		if (m->cursor >= m->numItems)
			m->cursor = 0;
		else if (m->cursor < 0)
			m->cursor = m->numItems - 1;
	}
}


/*
=============
UI_ItemAtCursor
=============
*/
void *UI_ItemAtCursor (uiFrameWork_t *m)
{
	if (!m)
		return NULL;
	if (!m->numItems)
		return NULL;

	if (m->cursor >= m->numItems)
		m->cursor = 0;
	else if (m->cursor < 0)
		m->cursor = m->numItems - 1;

	if (((uiCommon_t *)m->items[m->cursor])->flags & UIF_NOSELECT)
		return NULL;

	return m->items[m->cursor];
}


/*
=============
UI_SelectItem
=============
*/
static void Action_DoEnter (uiAction_t *a)
{
	if (a->generic.callBack)
		a->generic.callBack (a);
}

static qBool Field_DoEnter (uiField_t *f)
{
	if (f->generic.callBack) {
		f->generic.callBack (f);
		return qTrue;
	}

	return qFalse;
}

qBool UI_SelectItem (uiFrameWork_t *s)
{
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
static void Slider_DoSlide (uiSlider_t *s, int dir)
{
	s->curValue += dir;

	if (s->curValue > s->maxValue)
		s->curValue = s->maxValue;
	else if (s->curValue < s->minValue)
		s->curValue = s->minValue;

	if (s->generic.callBack)
		s->generic.callBack (s);
}

static void SpinControl_DoSlide (uiList_t *s, int dir)
{
	int		i;

	if (!s->itemNames || !s->numItemNames)
		return;

	// Move
	s->curValue += dir;
	if (s->curValue < 0) {
		for (i=0 ; s->itemNames[i] ; i++) ;
		s->curValue = i-1;
	}
	else {
		if (s->curValue >= s->numItemNames)
			s->curValue = 0;
	}

	// Callback
	if (s->generic.callBack)
		s->generic.callBack (s);
}

qBool UI_SlideItem (uiFrameWork_t *s, int dir)
{
	uiCommon_t *item = ((uiCommon_t *) UI_ItemAtCursor (s));

	if (!item)
		return qFalse;
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

	return qFalse;
}

//======================================================================

/*
==================
UI_StrDup

No need to null terminate the extra spot because Mem_Alloc returns zero-filled memory
==================
*/
char *UI_StrDup (const char *in)
{
	char	*out;

	out = UI_MemAlloc (strlen (in) + 1);
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
void Com_Printf (int flags, char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	vsnprintf (text, sizeof (text), fmt, argptr);
	va_end (argptr);

	uii.Com_Printf (flags, "%s", text);
}


/*
==================
Com_Error
==================
*/
void Com_Error (int code, char *fmt, ...)
{
	va_list		argptr;
	char		msg[1024];

	va_start (argptr, fmt);
	vsnprintf (msg, sizeof (msg), fmt, argptr);
	va_end (argptr);

	uii.Com_Error (code, "%s", msg);
}
#endif
