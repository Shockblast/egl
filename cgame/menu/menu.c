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
// menu.c
//

#include "m_local.h"

menuState_t	menuState;

cVar_t	*ui_jsMenuPage;
cVar_t	*ui_jsSortItem;
cVar_t	*ui_jsSortMethod;

static void	*cmd_menuMain;

static void	*cmd_menuGame;
static void	*cmd_menuLoadGame;
static void	*cmd_menuSaveGame;
static void	*cmd_menuCredits;

static void	*cmd_menuMultiplayer;
static void	*cmd_menuDLOptions;
static void	*cmd_menuJoinServer;
static void	*cmd_menuAddressBook;
static void	*cmd_menuPlayerConfig;
static void	*cmd_menuStartServer;
static void	*cmd_menuDMFlags;

static void	*cmd_menuOptions;
static void	*cmd_menuControls;
static void	*cmd_menuEffects;
static void	*cmd_menuGloom;
static void	*cmd_menuHUD;
static void	*cmd_menuInput;
static void	*cmd_menuMisc;
static void	*cmd_menuScreen;
static void	*cmd_menuSound;

static void	*cmd_menuVideo;
static void	*cmd_menuGLExts;
static void	*cmd_menuVidSettings;

static void	*cmd_menuQuit;

static void	*cmd_startSStatus;

/*
=============================================================================

	MENU INITIALIZATION/SHUTDOWN

=============================================================================
*/

/*
=================
M_Init
=================
*/
void JoinMenu_StartSStatus (void); // FIXME
void M_Init (void)
{
	int		i;

   	// Register cvars
	for (i=0 ; i<MAX_ADDRBOOK_SAVES ; i++)
		cgi.Cvar_Register (Q_VarArgs ("adr%i", i),				"",			CVAR_ARCHIVE);

	ui_jsMenuPage		= cgi.Cvar_Register ("ui_jsMenuPage",	"0",		CVAR_ARCHIVE);
	ui_jsSortItem		= cgi.Cvar_Register ("ui_jsSortItem",	"0",		CVAR_ARCHIVE);
	ui_jsSortMethod		= cgi.Cvar_Register ("ui_jsSortMethod",	"0",		CVAR_ARCHIVE);

	// Add commands
	cmd_menuMain		= cgi.Cmd_AddCommand ("menu_main",			UI_MainMenu_f,				"Opens the main menu");

	cmd_menuGame		= cgi.Cmd_AddCommand ("menu_game",			UI_GameMenu_f,				"Opens the single player menu");
	cmd_menuLoadGame	= cgi.Cmd_AddCommand ("menu_loadgame",		UI_LoadGameMenu_f,			"Opens the load game menu");
	cmd_menuSaveGame	= cgi.Cmd_AddCommand ("menu_savegame",		UI_SaveGameMenu_f,			"Opens the save game menu");
	cmd_menuCredits		= cgi.Cmd_AddCommand ("menu_credits",		UI_CreditsMenu_f,			"Opens the credits menu");

	cmd_menuMultiplayer	= cgi.Cmd_AddCommand ("menu_multiplayer",	UI_MultiplayerMenu_f,		"Opens the multiplayer menu");
	cmd_menuDLOptions	= cgi.Cmd_AddCommand ("menu_dloptions",		UI_DLOptionsMenu_f,			"Opens the download options menu");
	cmd_menuJoinServer	= cgi.Cmd_AddCommand ("menu_joinserver",	UI_JoinServerMenu_f,		"Opens the join server menu");
	cmd_menuAddressBook	= cgi.Cmd_AddCommand ("menu_addressbook",	UI_AddressBookMenu_f,		"Opens the address book menu");
	cmd_menuPlayerConfig= cgi.Cmd_AddCommand ("menu_playerconfig",	UI_PlayerConfigMenu_f,		"Opens the player configuration menu");
	cmd_menuStartServer	= cgi.Cmd_AddCommand ("menu_startserver",	UI_StartServerMenu_f,		"Opens the start server menu");
	cmd_menuDMFlags		= cgi.Cmd_AddCommand ("menu_dmflags",		UI_DMFlagsMenu_f,			"Opens the deathmatch flags menu");

	cmd_menuOptions		= cgi.Cmd_AddCommand ("menu_options",		UI_OptionsMenu_f,			"Opens the options menu");
	cmd_menuControls	= cgi.Cmd_AddCommand ("menu_controls",		UI_ControlsMenu_f,			"Opens the controls menu");
	cmd_menuEffects		= cgi.Cmd_AddCommand ("menu_effects",		UI_EffectsMenu_f,			"Opens the effects menu");
	cmd_menuGloom		= cgi.Cmd_AddCommand ("menu_gloom",			UI_GloomMenu_f,				"Opens the gloom menu");
	cmd_menuHUD			= cgi.Cmd_AddCommand ("menu_hud",			UI_HUDMenu_f,				"Opens the hud menu");
	cmd_menuInput		= cgi.Cmd_AddCommand ("menu_input",			UI_InputMenu_f,				"Opens the input menu");
	cmd_menuMisc		= cgi.Cmd_AddCommand ("menu_misc",			UI_MiscMenu_f,				"Opens the misc menu");
	cmd_menuScreen		= cgi.Cmd_AddCommand ("menu_screen",		UI_ScreenMenu_f,			"Opens the screen menu");
	cmd_menuSound		= cgi.Cmd_AddCommand ("menu_sound",			UI_SoundMenu_f,				"Opens the sound menu");

	cmd_menuVideo		= cgi.Cmd_AddCommand ("menu_video",			UI_VideoMenu_f,				"Opens the video menu");
	cmd_menuGLExts		= cgi.Cmd_AddCommand ("menu_glexts",		UI_GLExtsMenu_f,			"Opens the opengl extensions menu");
	cmd_menuVidSettings	= cgi.Cmd_AddCommand ("menu_vidsettings",	UI_VIDSettingsMenu_f,		"Opens the video settings menu");

	cmd_menuQuit		= cgi.Cmd_AddCommand ("menu_quit",			UI_QuitMenu_f,				"Opens the quit menu");

	cmd_startSStatus	= cgi.Cmd_AddCommand ("ui_startSStatus",	JoinMenu_StartSStatus,		"");
}


/*
=================
M_Shutdown
=================
*/
void M_Shutdown (void)
{
	// Don't play the menu exit sound
	menuState.playExitSound = qFalse;

	// Get rid of the menu
	M_ForceMenuOff ();

	// Remove commands
	cgi.Cmd_RemoveCommand ("menu_main", cmd_menuMain);

	cgi.Cmd_RemoveCommand ("menu_game", cmd_menuGame);
	cgi.Cmd_RemoveCommand ("menu_loadgame", cmd_menuLoadGame);
	cgi.Cmd_RemoveCommand ("menu_savegame", cmd_menuSaveGame);
	cgi.Cmd_RemoveCommand ("menu_credits", cmd_menuCredits);

	cgi.Cmd_RemoveCommand ("menu_multiplayer", cmd_menuMultiplayer);
	cgi.Cmd_RemoveCommand ("menu_dloptions", cmd_menuDLOptions);
	cgi.Cmd_RemoveCommand ("menu_joinserver", cmd_menuJoinServer);
	cgi.Cmd_RemoveCommand ("menu_addressbook", cmd_menuAddressBook);
	cgi.Cmd_RemoveCommand ("menu_playerconfig", cmd_menuPlayerConfig);
	cgi.Cmd_RemoveCommand ("menu_startserver", cmd_menuStartServer);
	cgi.Cmd_RemoveCommand ("menu_dmflags", cmd_menuDMFlags);

	cgi.Cmd_RemoveCommand ("menu_options", cmd_menuOptions);
	cgi.Cmd_RemoveCommand ("menu_controls", cmd_menuControls);
	cgi.Cmd_RemoveCommand ("menu_effects", cmd_menuEffects);
	cgi.Cmd_RemoveCommand ("menu_gloom", cmd_menuGloom);
	cgi.Cmd_RemoveCommand ("menu_hud", cmd_menuHUD);
	cgi.Cmd_RemoveCommand ("menu_input", cmd_menuInput);
	cgi.Cmd_RemoveCommand ("menu_misc", cmd_menuMisc);
	cgi.Cmd_RemoveCommand ("menu_screen", cmd_menuScreen);
	cgi.Cmd_RemoveCommand ("menu_sound", cmd_menuSound);

	cgi.Cmd_RemoveCommand ("menu_video", cmd_menuVideo);
	cgi.Cmd_RemoveCommand ("menu_glexts", cmd_menuGLExts);
	cgi.Cmd_RemoveCommand ("menu_vidsettings", cmd_menuVidSettings);

	cgi.Cmd_RemoveCommand ("menu_quit", cmd_menuQuit);

	cgi.Cmd_RemoveCommand ("ui_startSStatus", cmd_startSStatus);
}

/*
=============================================================================

	PUBLIC FUNCTIONS

=============================================================================
*/

/*
=================
M_Refresh
=================
*/
void M_Refresh (void)
{
	// Delay playing the enter sound until after the menu has
	// been drawn, to avoid delay while caching images
	if (menuState.playEnterSound) {
		cgi.Snd_StartLocalSound (uiMedia.sounds.menuIn, 1);
		menuState.playEnterSound = qFalse;
	}
	else if (uiState.newCursorItem) {
		// Play menu open sound
		cgi.Snd_StartLocalSound (uiMedia.sounds.menuMove, 1);
		uiState.newCursorItem = qFalse;
	}
}


/*
=================
M_ForceMenuOff
=================
*/
void M_ForceMenuOff (void)
{
	cg.menuOpen = qFalse;

	// Unpause
	cgi.Cvar_Set ("paused", "0", qFalse);

	// Play exit sound
	if (menuState.playExitSound) {
		cgi.Snd_StartLocalSound (uiMedia.sounds.menuOut, 1);
		menuState.playExitSound = qFalse;
	}

	// Update mouse position
	UI_UpdateMousePos ();

	// Kill the interfaces
	UI_ForceAllOff ();
}


/*
=================
M_PopMenu
=================
*/
void M_PopMenu (void)
{
	UI_PopInterface ();
}

/*
=============================================================================

	LOCAL FUNCTIONS

=============================================================================
*/

/*
=================
M_PushMenu
=================
*/
void M_PushMenu (uiFrameWork_t *frameWork, void (*drawFunc) (void), struct sfx_s *(*closeFunc) (void), struct sfx_s *(*keyFunc) (uiFrameWork_t *fw, keyNum_t keyNum))
{
	// Pause single-player games
	if (cgi.Cvar_GetFloatValue ("maxclients") == 1 && cgi.Com_ServerState ())
		cgi.Cvar_Set ("paused", "1", qFalse);

	menuState.playEnterSound = qTrue;
	menuState.playExitSound = qTrue;

	UI_PushInterface (frameWork, drawFunc, closeFunc, keyFunc);

	cg.menuOpen = qTrue;
}


/*
=================
M_KeyHandler
=================
*/
struct sfx_s *M_KeyHandler (uiFrameWork_t *fw, keyNum_t keyNum)
{
	if (keyNum == K_ESCAPE) {
		M_PopMenu ();
		return NULL;
	}

	return UI_DefaultKeyFunc (fw, keyNum);
}
