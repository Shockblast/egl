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

	ADDRESS BOOK MENU

=============================================================================
*/

typedef struct m_addrbookmenu_s {
	uiFrameWork_t	framework;

	uiImage_t		banner;

	uiField_t		fields[MAX_ADDRBOOK_SAVES];

	uiAction_t		back_action;
} m_addrbookmenu_t;

m_addrbookmenu_t m_addrbook_menu;

void AddressBook_SaveChanges (void) {
	int index;
	char buffer[20];

	for (index=0 ; index<MAX_ADDRBOOK_SAVES ; index++) {
		Q_snprintfz (buffer, sizeof (buffer), "adr%d", index);
		uii.Cvar_Set (buffer, m_addrbook_menu.fields[index].buffer);
	}
}

void AddressBook_Back (void *unused) {
	AddressBook_SaveChanges ();
	UI_PopMenu ();
}

/*
=============
AddressBook_MenuInit
=============
*/
void AddressBook_MenuInit (void) {
	int		i;
	cVar_t	*adr;
	char	buffer[20];
	float	y;

	m_addrbook_menu.framework.x			= uis.vidWidth * 0.5;
	m_addrbook_menu.framework.y			= 0;
	m_addrbook_menu.framework.numItems	= 0;

	UI_Banner (&m_addrbook_menu.banner, uiMedia.banners.addressBook, &y);
	UI_AddItem (&m_addrbook_menu.framework,				&m_addrbook_menu.banner);

	for (i=0 ; i<MAX_ADDRBOOK_SAVES ; i++) {
		Q_snprintfz (buffer, sizeof (buffer), "adr%d", i);

		adr = uii.Cvar_Get (buffer, "", CVAR_ARCHIVE);

		m_addrbook_menu.fields[i].generic.type			= UITYPE_FIELD;
		m_addrbook_menu.fields[i].generic.flags			= UIF_CENTERED;
		m_addrbook_menu.fields[i].generic.name			= 0;
		m_addrbook_menu.fields[i].generic.callBack		= 0;
		m_addrbook_menu.fields[i].generic.x				= 0;
		m_addrbook_menu.fields[i].generic.y				= y + ((i + 1) * (UIFT_SIZEINC*2));
		m_addrbook_menu.fields[i].generic.localData[0]	= i;
		m_addrbook_menu.fields[i].cursor				= 0;
		m_addrbook_menu.fields[i].length				= 60;
		m_addrbook_menu.fields[i].visibleLength			= 50;

		Q_strncpyz (m_addrbook_menu.fields[i].buffer, adr->string, sizeof (m_addrbook_menu.fields[i].buffer));

		UI_AddItem (&m_addrbook_menu.framework,		&m_addrbook_menu.fields[i]);
	}

	m_addrbook_menu.back_action.generic.type		= UITYPE_ACTION;
	m_addrbook_menu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW|UIF_SHADOW;
	m_addrbook_menu.back_action.generic.x			= 0;
	m_addrbook_menu.back_action.generic.y			= y + ((i + 1) * (UIFT_SIZEINC*2)) + UIFT_SIZEINCLG;
	m_addrbook_menu.back_action.generic.name		= "< Back";
	m_addrbook_menu.back_action.generic.callBack	= AddressBook_Back;
	m_addrbook_menu.back_action.generic.statusBar	= "Back a menu";

	UI_AddItem (&m_addrbook_menu.framework,			&m_addrbook_menu.back_action);

	UI_Center (&m_addrbook_menu.framework);
	UI_Setup (&m_addrbook_menu.framework);
}


/*
=============
AddressBook_MenuDraw
=============
*/
void AddressBook_MenuDraw (void) {
	UI_Center (&m_addrbook_menu.framework);
	UI_Setup (&m_addrbook_menu.framework);

	UI_AdjustTextCursor (&m_addrbook_menu.framework, 1);
	UI_Draw (&m_addrbook_menu.framework);
}


/*
=============
AddressBook_MenuKey
=============
*/
struct sfx_s *AddressBook_MenuKey (int key) {
	if (key == K_ESCAPE)
		AddressBook_SaveChanges ();

	return DefaultMenu_Key (&m_addrbook_menu.framework, key);
}


/*
=============
UI_AddressBookMenu_f
=============
*/
void UI_AddressBookMenu_f (void) {
	AddressBook_MenuInit ();
	UI_PushMenu (&m_addrbook_menu.framework, AddressBook_MenuDraw, AddressBook_MenuKey);
}
