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

	QUIT MENU

=======================================================================
*/

typedef struct m_quitMenu_s {
	// Menu items
	uiFrameWork_t		frameWork;

	uiImage_t			banner;

	uiAction_t			yes_action;
	uiAction_t			no_action;
} m_quitMenu_t;

static m_quitMenu_t		m_quitMenu;

static void QuitYes (void *unused)
{
	uii.Key_SetKeyDest (KD_CONSOLE);
	uii.Cbuf_AddText ("quit");
}


/*
=============
QuitMenu_Init
=============
*/
static void QuitMenu_Init (void)
{
	UI_StartFramework (&m_quitMenu.frameWork);

	m_quitMenu.banner.generic.type			= UITYPE_IMAGE;
	m_quitMenu.banner.generic.flags			= UIF_NOSELECT|UIF_CENTERED;
	m_quitMenu.banner.generic.name			= NULL;
	m_quitMenu.banner.shader				= uiMedia.banners.quit;

	m_quitMenu.yes_action.generic.type		= UITYPE_ACTION;
	m_quitMenu.yes_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_quitMenu.yes_action.generic.name		= "Yes";
	m_quitMenu.yes_action.generic.callBack	= QuitYes;
	m_quitMenu.yes_action.generic.statusBar	= "Sell out";

	m_quitMenu.no_action.generic.type		= UITYPE_ACTION;
	m_quitMenu.no_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_quitMenu.no_action.generic.name		= "No";
	m_quitMenu.no_action.generic.callBack	= Menu_Pop;
	m_quitMenu.no_action.generic.statusBar	= "No";

	UI_AddItem (&m_quitMenu.frameWork,		&m_quitMenu.banner);

	UI_AddItem (&m_quitMenu.frameWork,		&m_quitMenu.yes_action);
	UI_AddItem (&m_quitMenu.frameWork,		&m_quitMenu.no_action);

	UI_FinishFramework (&m_quitMenu.frameWork, qTrue);
}


/*
=============
QuitMenu_Close
=============
*/
static struct sfx_s *QuitMenu_Close (void)
{
	return uiMedia.sounds.menuOut;
}


/*
=============
QuitMenu_Draw
=============
*/
static void QuitMenu_Draw (void)
{
	float	y;

	// Initialize if necessary
	if (!m_quitMenu.frameWork.initialized)
		QuitMenu_Init ();

	// Dynamically position
	m_quitMenu.frameWork.x				= uiState.vidWidth * 0.5f;
	m_quitMenu.frameWork.y				= 0;

	m_quitMenu.banner.generic.x			= 0;
	m_quitMenu.banner.generic.y			= 0;

	y = m_quitMenu.banner.height * UI_SCALE;

	m_quitMenu.yes_action.generic.x		= 0;
	m_quitMenu.yes_action.generic.y		= y += UIFT_SIZEINC;
	m_quitMenu.no_action.generic.x		= 0;
	m_quitMenu.no_action.generic.y		= y += UIFT_SIZEINCLG;

	// Render
	UI_CenterMenuHeight (&m_quitMenu.frameWork);
	UI_SetupMenuBounds (&m_quitMenu.frameWork);

	UI_AdjustCursor (&m_quitMenu.frameWork, 1);
	UI_Draw (&m_quitMenu.frameWork);
}


/*
=============
QuitMenu_Key
=============
*/
static struct sfx_s *QuitMenu_Key (int key)
{
	switch (key) {
	case 'n':
	case 'N':
		UI_PopMenu ();
		return NULL;

	case 'Y':
	case 'y':
		QuitYes (NULL);
		return NULL;
	}

	return DefaultMenu_Key (&m_quitMenu.frameWork, key);
}


/*
=============
UI_QuitMenu_f
=============
*/
void UI_QuitMenu_f (void)
{
	QuitMenu_Init ();
	UI_PushMenu (&m_quitMenu.frameWork, QuitMenu_Draw, QuitMenu_Close, QuitMenu_Key);
}
