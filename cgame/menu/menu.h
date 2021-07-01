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
// menu.h
//

#define MAX_LOCAL_SERVERS	24
#define MAX_ADDRBOOK_SAVES	8
#define MAX_SAVEGAMES		16

/*
=============================================================================

	MENUS

=============================================================================
*/

void UI_MainMenu_f (void);

void UI_GameMenu_f (void);
	void UI_LoadGameMenu_f (void);
	void UI_SaveGameMenu_f (void);
	void UI_CreditsMenu_f (void);

void UI_MultiplayerMenu_f (void);
		void UI_DLOptionsMenu_f (void);
	void UI_JoinServerMenu_f (void);
		void UI_AddressBookMenu_f (void);
	void UI_PlayerConfigMenu_f (void);
	void UI_StartServerMenu_f (void);
		void UI_DMFlagsMenu_f (void);

void UI_OptionsMenu_f (void);
	void UI_ControlsMenu_f (void);
	void UI_EffectsMenu_f (void);
	void UI_GloomMenu_f (void);
	void UI_HUDMenu_f (void);
	void UI_InputMenu_f (void);
	void UI_MiscMenu_f (void);
	void UI_ScreenMenu_f (void);
	void UI_SoundMenu_f (void);

void UI_VideoMenu_f (void);
	void UI_GLExtsMenu_f (void);
	void UI_VIDSettingsMenu_f (void);

void UI_QuitMenu_f (void);

/*
=============================================================================

	CVARS

=============================================================================
*/

extern cVar_t	*ui_jsMenuPage;
extern cVar_t	*ui_jsSortItem;
extern cVar_t	*ui_jsSortMethod;

/*
=============================================================================

	SUPPORTING FUNCTIONS

=============================================================================
*/

//
// menu.c
//

void		M_Init (void);
void		M_Shutdown (void);

void		M_Refresh (void);

void		M_ForceMenuOff (void);
void		M_PopMenu (void);

//
// m_mp_join.c
//

qBool		UI_ParseServerInfo (char *adr, char *info);
qBool		UI_ParseServerStatus (char *adr, char *info);

//
// m_sp_loadgame.c
//

extern char	ui_saveStrings[MAX_SAVEGAMES][32];

void		Create_Savestrings (void);
