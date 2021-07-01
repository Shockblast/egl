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

typedef struct m_addressBookMenu_s {
	// Menu items
	uiFrameWork_t		frameWork;

	uiImage_t			banner;

	uiField_t			fields[MAX_ADDRBOOK_SAVES];

	uiAction_t			back_action;
} m_addressBookMenu_t;

static m_addressBookMenu_t m_addressBookMenu;

static void AddressBook_SaveChanges (void)
{
	char	buffer[20];
	int		index;

	for (index=0 ; index<MAX_ADDRBOOK_SAVES ; index++) {
		Q_snprintfz (buffer, sizeof (buffer), "adr%d", index);
		uii.Cvar_Set (buffer, m_addressBookMenu.fields[index].buffer);
	}
}


/*
=============
AddrBookMenu_Init
=============
*/
static void AddrBookMenu_Init (void)
{
	cVar_t	*adr;
	char	buffer[20];
	int		i;

	UI_StartFramework (&m_addressBookMenu.frameWork);

	m_addressBookMenu.banner.generic.type			= UITYPE_IMAGE;
	m_addressBookMenu.banner.generic.flags			= UIF_NOSELECT|UIF_CENTERED;
	m_addressBookMenu.banner.generic.name			= NULL;
	m_addressBookMenu.banner.shader					= uiMedia.banners.addressBook;

	UI_AddItem (&m_addressBookMenu.frameWork,		&m_addressBookMenu.banner);

	for (i=0 ; i<MAX_ADDRBOOK_SAVES ; i++) {
		Q_snprintfz (buffer, sizeof (buffer), "adr%d", i);

		adr = uii.Cvar_Get (buffer, "", CVAR_ARCHIVE);

		m_addressBookMenu.fields[i].generic.type			= UITYPE_FIELD;
		m_addressBookMenu.fields[i].generic.flags			= UIF_CENTERED;
		m_addressBookMenu.fields[i].generic.name			= 0;
		m_addressBookMenu.fields[i].generic.callBack		= 0;
		m_addressBookMenu.fields[i].cursor					= 0;
		m_addressBookMenu.fields[i].length					= 60;
		m_addressBookMenu.fields[i].visibleLength			= 50;

		Q_strncpyz (m_addressBookMenu.fields[i].buffer, adr->string, sizeof (m_addressBookMenu.fields[i].buffer));

		UI_AddItem (&m_addressBookMenu.frameWork,		&m_addressBookMenu.fields[i]);
	}

	m_addressBookMenu.back_action.generic.type		= UITYPE_ACTION;
	m_addressBookMenu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW|UIF_SHADOW;
	m_addressBookMenu.back_action.generic.name		= "< Back";
	m_addressBookMenu.back_action.generic.callBack	= Menu_Pop;
	m_addressBookMenu.back_action.generic.statusBar	= "Back a menu";

	UI_AddItem (&m_addressBookMenu.frameWork,			&m_addressBookMenu.back_action);

	UI_FinishFramework (&m_addressBookMenu.frameWork, qTrue);
}


/*
=============
AddrBookMenu_Close
=============
*/
static struct sfx_s *AddrBookMenu_Close (void)
{
	AddressBook_SaveChanges ();

	return uiMedia.sounds.menuOut;
}


/*
=============
AddrBookMenu_Draw
=============
*/
static void AddrBookMenu_Draw (void)
{
	int		i;
	float	y;

	// Initialize if necessary
	if (!m_addressBookMenu.frameWork.initialized)
		AddrBookMenu_Init ();

	// Dynamically position

	m_addressBookMenu.frameWork.x				= uiState.vidWidth * 0.5f;
	m_addressBookMenu.frameWork.y				= 0;

	m_addressBookMenu.banner.generic.x			= 0;
	m_addressBookMenu.banner.generic.y			= 0;

	y = m_addressBookMenu.banner.height * UI_SCALE;

	for (i=0 ; i<MAX_ADDRBOOK_SAVES ; i++) {
		m_addressBookMenu.fields[i].generic.x	= 0;
		m_addressBookMenu.fields[i].generic.y	= y + ((i + 1) * (UIFT_SIZEINC*2));
	}

	m_addressBookMenu.back_action.generic.x		= 0;
	m_addressBookMenu.back_action.generic.y		= y + ((i + 1) * (UIFT_SIZEINC*2)) + UIFT_SIZEINCLG;

	// Render
	UI_CenterMenuHeight (&m_addressBookMenu.frameWork);
	UI_SetupMenuBounds (&m_addressBookMenu.frameWork);

	UI_AdjustCursor (&m_addressBookMenu.frameWork, 1);
	UI_Draw (&m_addressBookMenu.frameWork);
}


/*
=============
AddrBookMenu_Key
=============
*/
struct sfx_s *AddrBookMenu_Key (int key)
{
	return DefaultMenu_Key (&m_addressBookMenu.frameWork, key);
}


/*
=============
UI_AddressBookMenu_f
=============
*/
void UI_AddressBookMenu_f (void)
{
	AddrBookMenu_Init ();
	UI_PushMenu (&m_addressBookMenu.frameWork, AddrBookMenu_Draw, AddrBookMenu_Close, AddrBookMenu_Key);
}
