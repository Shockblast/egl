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

typedef struct {
	uiFrameWork_t	framework;

	uiImage_t		banner;

	uiAction_t		yes_action;
	uiAction_t		no_action;
} m_quitmenu_t;

m_quitmenu_t	m_quit_menu;

static void QuitYes (void *unused) {
	uii.Key_SetKeyDest (KD_CONSOLE);
	uii.Cbuf_ExecuteText (EXEC_NOW, "quit");
}


/*
=============
QuitMenu_Init
=============
*/
void QuitMenu_Init (void) {
	float		y;

	m_quit_menu.framework.x			= uis.vidWidth * 0.5;
	m_quit_menu.framework.y			= 0;
	m_quit_menu.framework.nitems	= 0;

	UI_Banner (&m_quit_menu.banner, uiMedia.quitBanner, &y);

	m_quit_menu.yes_action.generic.type			= UITYPE_ACTION;
	m_quit_menu.yes_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_quit_menu.yes_action.generic.x			= 0;
	m_quit_menu.yes_action.generic.y			= y += UIFT_SIZEINC;
	m_quit_menu.yes_action.generic.name			= "Yes";
	m_quit_menu.yes_action.generic.callback		= QuitYes;
	m_quit_menu.yes_action.generic.statusbar	= "Sell out";

	m_quit_menu.no_action.generic.type			= UITYPE_ACTION;
	m_quit_menu.no_action.generic.flags			= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_quit_menu.no_action.generic.x				= 0;
	m_quit_menu.no_action.generic.y				= y += UIFT_SIZEINCLG;
	m_quit_menu.no_action.generic.name			= "No";
	m_quit_menu.no_action.generic.callback		= Back_Menu;
	m_quit_menu.no_action.generic.statusbar		= "No";

	UI_AddItem (&m_quit_menu.framework,		&m_quit_menu.banner);

	UI_AddItem (&m_quit_menu.framework,		&m_quit_menu.yes_action);
	UI_AddItem (&m_quit_menu.framework,		&m_quit_menu.no_action);

	UI_Center (&m_quit_menu.framework);
	UI_Setup (&m_quit_menu.framework);
}


/*
=============
QuitMenu_Draw
=============
*/
void QuitMenu_Draw (void) {
	UI_Center (&m_quit_menu.framework);
	UI_Setup (&m_quit_menu.framework);

	UI_AdjustTextCursor (&m_quit_menu.framework, 1);
	UI_Draw (&m_quit_menu.framework);
}


/*
=============
QuitMenu_Key
=============
*/
const char *QuitMenu_Key (int key) {
	switch (key) {
	case 'n':
	case 'N':
		Back_Menu (NULL);
		return NULL;

	case 'Y':
	case 'y':
		QuitYes (NULL);
		return NULL;
	}

	return DefaultMenu_Key (&m_quit_menu.framework, key);
}


/*
=============
UI_QuitMenu_f
=============
*/
void UI_QuitMenu_f (void) {
	QuitMenu_Init ();
	UI_PushMenu (&m_quit_menu.framework, QuitMenu_Draw, QuitMenu_Key);
}
