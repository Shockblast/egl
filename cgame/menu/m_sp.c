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
// m_sp.c
//

#include "m_local.h"

/*
=============================================================================

	GAME MENU

=============================================================================
*/

typedef struct m_gameMenu_s {
	// Menu items
	uiFrameWork_t		frameWork;

	uiImage_t			banner;

	uiAction_t			easy_action;
	uiAction_t			medium_action;
	uiAction_t			hard_action;
	uiAction_t			nightmare_action;

	uiAction_t			load_action;
	uiAction_t			save_action;
	uiAction_t			credits_action;

	uiAction_t			back_action;
} m_gameMenu_t;

static m_gameMenu_t	m_gameMenu;

static void StartGame (void)
{
	// Disable updates and start the cinematic going
	cgi.CL_ResetServerCount ();
	M_ForceMenuOff ();
	cgi.Cvar_SetValue ("deathmatch", 0, qFalse);
	cgi.Cvar_SetValue ("coop", 0, qFalse);

	cgi.Cvar_SetValue ("gamerules", 0, qFalse);

	cgi.Cbuf_AddText ("killserver ; wait ; newgame\n");
	cgi.Key_SetDest (KD_GAME);
}

static void EasyGameFunc (void *unused)
{
	cgi.Cvar_Set ("skill", "0", qTrue);
	StartGame ();
}

static void MediumGameFunc (void *unused)
{
	cgi.Cvar_Set ("skill", "1", qTrue);
	StartGame ();
}

static void HardGameFunc (void *unused)
{
	cgi.Cvar_Set ("skill", "2", qTrue);
	StartGame ();
}

static void HardPlusGameFunc (void *unused)
{
	cgi.Cvar_Set ("skill", "3", qTrue);
	StartGame ();
}

static void LoadGameFunc (void *unused)
{
	UI_LoadGameMenu_f ();
}

static void SaveGameFunc (void *unused)
{
	UI_SaveGameMenu_f ();
}

static void CreditsFunc (void *unused)
{
	UI_CreditsMenu_f ();
}


/*
=============
GameMenu_Init
=============
*/
static void GameMenu_Init (void)
{
	UI_StartFramework (&m_gameMenu.frameWork, FWF_CENTERHEIGHT);

	m_gameMenu.banner.generic.type		= UITYPE_IMAGE;
	m_gameMenu.banner.generic.flags		= UIF_NOSELECT|UIF_CENTERED;
	m_gameMenu.banner.generic.name		= NULL;
	m_gameMenu.banner.mat			= uiMedia.banners.game;

	m_gameMenu.easy_action.generic.type		= UITYPE_ACTION;
	m_gameMenu.easy_action.generic.flags	= UIF_LEFT_JUSTIFY|UIF_LARGE|UIF_SHADOW;
	m_gameMenu.easy_action.generic.name		= "Easy";
	m_gameMenu.easy_action.generic.callBack	= EasyGameFunc;

	m_gameMenu.medium_action.generic.type		= UITYPE_ACTION;
	m_gameMenu.medium_action.generic.flags		= UIF_LEFT_JUSTIFY|UIF_LARGE|UIF_SHADOW;
	m_gameMenu.medium_action.generic.name		= "Medium";
	m_gameMenu.medium_action.generic.callBack	= MediumGameFunc;

	m_gameMenu.hard_action.generic.type		= UITYPE_ACTION;
	m_gameMenu.hard_action.generic.flags	= UIF_LEFT_JUSTIFY|UIF_LARGE|UIF_SHADOW;
	m_gameMenu.hard_action.generic.name		= "Hard";
	m_gameMenu.hard_action.generic.callBack	= HardGameFunc;

	m_gameMenu.nightmare_action.generic.type		= UITYPE_ACTION;
	m_gameMenu.nightmare_action.generic.flags		= UIF_LEFT_JUSTIFY|UIF_LARGE|UIF_SHADOW;
	m_gameMenu.nightmare_action.generic.name		= S_COLOR_RED "NIGHTMARE!";
	m_gameMenu.nightmare_action.generic.callBack	= HardPlusGameFunc;

	m_gameMenu.load_action.generic.type		= UITYPE_ACTION;
	m_gameMenu.load_action.generic.flags	= UIF_LEFT_JUSTIFY|UIF_LARGE|UIF_SHADOW;
	m_gameMenu.load_action.generic.name		= "Load game";
	m_gameMenu.load_action.generic.callBack	= LoadGameFunc;

	m_gameMenu.save_action.generic.type		= UITYPE_ACTION;
	m_gameMenu.save_action.generic.flags	= UIF_LEFT_JUSTIFY|UIF_LARGE|UIF_SHADOW;
	m_gameMenu.save_action.generic.name		= "Save game";
	m_gameMenu.save_action.generic.callBack	= SaveGameFunc;

	m_gameMenu.credits_action.generic.type		= UITYPE_ACTION;
	m_gameMenu.credits_action.generic.flags		= UIF_LEFT_JUSTIFY|UIF_LARGE|UIF_SHADOW;
	m_gameMenu.credits_action.generic.name		= "View credits";
	m_gameMenu.credits_action.generic.callBack	= CreditsFunc;

	m_gameMenu.back_action.generic.type			= UITYPE_ACTION;
	m_gameMenu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_gameMenu.back_action.generic.name			= "< Back";
	m_gameMenu.back_action.generic.callBack		= Menu_Pop;
	m_gameMenu.back_action.generic.statusBar	= "Back a menu";

	UI_AddItem (&m_gameMenu.frameWork,		&m_gameMenu.banner);

	UI_AddItem (&m_gameMenu.frameWork,		&m_gameMenu.easy_action);
	UI_AddItem (&m_gameMenu.frameWork,		&m_gameMenu.medium_action);
	UI_AddItem (&m_gameMenu.frameWork,		&m_gameMenu.hard_action);
	UI_AddItem (&m_gameMenu.frameWork,		&m_gameMenu.nightmare_action);

	UI_AddItem (&m_gameMenu.frameWork,		&m_gameMenu.load_action);
	UI_AddItem (&m_gameMenu.frameWork,		&m_gameMenu.save_action);
	UI_AddItem (&m_gameMenu.frameWork,		&m_gameMenu.credits_action);

	UI_AddItem (&m_gameMenu.frameWork,		&m_gameMenu.back_action);

	UI_FinishFramework (&m_gameMenu.frameWork, qTrue);
}


/*
=============
GameMenu_Close
=============
*/
static struct sfx_s *GameMenu_Close (void)
{
	return uiMedia.sounds.menuOut;
}


/*
=============
GameMenu_Draw
=============
*/
static void GameMenu_Draw (void)
{
	float	y;

	// Initialize if necessary
	if (!m_gameMenu.frameWork.initialized)
		GameMenu_Init ();

	// Dynamically position
	m_gameMenu.frameWork.x			= cg.refConfig.vidWidth * 0.5f;
	m_gameMenu.frameWork.y			= 0;

	m_gameMenu.banner.generic.x			= 0;
	m_gameMenu.banner.generic.y			= 0;

	y = m_gameMenu.banner.height * UI_SCALE;

	m_gameMenu.easy_action.generic.x		= 0;
	m_gameMenu.easy_action.generic.y		= y += UIFT_SIZEINC;
	m_gameMenu.medium_action.generic.x		= 0;
	m_gameMenu.medium_action.generic.y		= y += UIFT_SIZEINCLG;
	m_gameMenu.hard_action.generic.x		= 0;
	m_gameMenu.hard_action.generic.y		= y += UIFT_SIZEINCLG;
	m_gameMenu.nightmare_action.generic.x	= 0;
	m_gameMenu.nightmare_action.generic.y	= y += UIFT_SIZEINCLG;
	m_gameMenu.load_action.generic.x		= 0;
	m_gameMenu.load_action.generic.y		= y += (UIFT_SIZEINCLG*2);
	m_gameMenu.save_action.generic.x		= 0;
	m_gameMenu.save_action.generic.y		= y += UIFT_SIZEINCLG;
	m_gameMenu.credits_action.generic.x		= 0;
	m_gameMenu.credits_action.generic.y		= y += UIFT_SIZEINCLG;
	m_gameMenu.back_action.generic.x		= 0;
	m_gameMenu.back_action.generic.y		= y += (UIFT_SIZEINCLG*2);

	// Render
	UI_DrawInterface (&m_gameMenu.frameWork);
}


/*
=============
UI_GameMenu_f
=============
*/
void UI_GameMenu_f (void)
{
	GameMenu_Init ();
	M_PushMenu (&m_gameMenu.frameWork, GameMenu_Draw, GameMenu_Close, M_KeyHandler);
}
