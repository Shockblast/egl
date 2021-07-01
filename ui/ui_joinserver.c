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

	JOIN SERVER MENU

=============================================================================
*/

typedef struct m_joinservermenu_s {
	uiFrameWork_t	framework;

	uiImage_t		banner;

	// kthx do these
//	uiAction_t		namesort_action;
//	uiAction_t		mapsort_action;
//	uiAction_t		playerssort_action;

	uiAction_t		address_book_action;
	uiAction_t		refresh_action;

	uiAction_t		header;

	uiAction_t		actions[MAX_LOCAL_SERVERS];

	uiAction_t		back_action;
	uiAction_t		play_action;
} m_joinservermenu_t;

m_joinservermenu_t m_joinserver_menu;

int		numServers;
#define SERVER_BLANKSTRING	"<no server>"

static char		serverNames[MAX_LOCAL_SERVERS][80];
static char		serverNetAdr[MAX_LOCAL_SERVERS][80];

void UI_AddToServerList (char *adr, char *info) {
	int		i;

	if (!info)
		return;
	if (!info[0])
		return;

	Com_Printf (0, "%s\n", info);
	info[strlen (info) - 1] = '\0';
	if (numServers == MAX_LOCAL_SERVERS)
		return;

	while (*info == ' ')
			info++;

	// ignore if duplicated
	for (i=0 ; i<numServers ; i++) {
		if (!strcmp (info, serverNames[i]))
			return;
	}

	strncpy (serverNames[numServers], info, sizeof (serverNames[0])-1);
	strncpy (serverNetAdr[numServers], adr, sizeof (serverNetAdr[0])-1);

	numServers++;
}

void JoinServerFunc (void *used) {
	char	buffer[128];
	int		index;

	if (uiSelItem)
		index = (uiAction_t *)uiSelItem - m_joinserver_menu.actions;
	else
		index = (uiAction_t *)used - m_joinserver_menu.actions;

	if (strcmp (serverNames[index], SERVER_BLANKSTRING) == 0)
		return;

	if (index >= numServers)
		return;

	Q_snprintfz (buffer, sizeof (buffer), "connect %s\n", serverNetAdr[index]);
	uii.Cbuf_AddText (buffer);
	UI_ForceMenuOff ();
}

void ADDRBOOK_Menu (void *unused) {
	UI_AddressBookMenu_f ();
}

void SearchLocalGamesFunc (void *unused) {
	int		i;
	float	midrow = (uis.vidWidth*0.5) - (18*UIFT_SIZEMED);
	float	midcol = (uis.vidHeight*0.5) - (3*UIFT_SIZEMED);

	numServers = 0;
	for (i=0 ; i<MAX_LOCAL_SERVERS ; i++)
		strcpy (serverNames[i], SERVER_BLANKSTRING);

	UI_DrawTextBox (midrow, midcol, UIFT_SCALEMED, 36, 4);
	UI_DrawString (midrow + (UIFT_SIZEMED*2), midcol + UIFT_SIZEMED, 0, UIFT_SCALEMED,		"       --- PLEASE WAIT! ---       ",	colorGreen);
	UI_DrawString (midrow + (UIFT_SIZEMED*2), midcol + (UIFT_SIZEMED*2), 0, UIFT_SCALEMED,	"Searching for local servers, this",	colorGreen);
	UI_DrawString (midrow + (UIFT_SIZEMED*2), midcol + (UIFT_SIZEMED*3), 0, UIFT_SCALEMED,	"could take up to a minute, please",	colorGreen);
	UI_DrawString (midrow + (UIFT_SIZEMED*2), midcol + (UIFT_SIZEMED*4), 0, UIFT_SCALEMED,	"please be patient.",					colorGreen);

	uii.R_EndFrame ();		// the text box won't show up unless we do a buffer swap
	uii.CL_PingServers ();	// send out info packets
}


/*
=============
JoinServerMenu_Init
=============
*/
void JoinServerMenu_Init (void) {
	int		i;
	float	y, xoffset = -(UIFT_SIZE*22);

	m_joinserver_menu.framework.x			= uis.vidWidth * 0.5;
	m_joinserver_menu.framework.y			= 0;
	m_joinserver_menu.framework.numItems	= 0;

	UI_Banner (&m_joinserver_menu.banner, uiMedia.joinServerBanner, &y);

	m_joinserver_menu.address_book_action.generic.type		= UITYPE_ACTION;
	m_joinserver_menu.address_book_action.generic.name		= "Address Book";
	m_joinserver_menu.address_book_action.generic.flags		= UIF_LEFT_JUSTIFY|UIF_SHADOW;
	m_joinserver_menu.address_book_action.generic.x			= xoffset;
	m_joinserver_menu.address_book_action.generic.y			= y += UIFT_SIZEINC;
	m_joinserver_menu.address_book_action.generic.callBack	= ADDRBOOK_Menu;

	m_joinserver_menu.refresh_action.generic.type		= UITYPE_ACTION;
	m_joinserver_menu.refresh_action.generic.name		= "Refresh Server List";
	m_joinserver_menu.refresh_action.generic.flags		= UIF_LEFT_JUSTIFY|UIF_SHADOW;
	m_joinserver_menu.refresh_action.generic.x			= xoffset;
	m_joinserver_menu.refresh_action.generic.y			= y += UIFT_SIZEINC;
	m_joinserver_menu.refresh_action.generic.callBack	= SearchLocalGamesFunc;
	m_joinserver_menu.refresh_action.generic.statusBar	= "search for servers";

	m_joinserver_menu.header.generic.type		= UITYPE_ACTION;
	m_joinserver_menu.header.generic.name		= S_COLOR_GREEN"connect to...";
	m_joinserver_menu.header.generic.flags		= UIF_NOSELECT|UIF_LEFT_JUSTIFY|UIF_SHADOW;
	m_joinserver_menu.header.generic.x			= xoffset;
	m_joinserver_menu.header.generic.y			= y += (UIFT_SIZEINC*2);

	UI_AddItem (&m_joinserver_menu.framework,			&m_joinserver_menu.banner);
	UI_AddItem (&m_joinserver_menu.framework,			&m_joinserver_menu.address_book_action);
	UI_AddItem (&m_joinserver_menu.framework,			&m_joinserver_menu.refresh_action);
	UI_AddItem (&m_joinserver_menu.framework,			&m_joinserver_menu.header);

	// kthx dont show blank spots
	for (i=0 ; i<MAX_LOCAL_SERVERS ; i++) {
		m_joinserver_menu.actions[i].generic.type		= UITYPE_ACTION;
		strcpy (serverNames[i], SERVER_BLANKSTRING);
		m_joinserver_menu.actions[i].generic.name		= serverNames[i];
		m_joinserver_menu.actions[i].generic.flags		= UIF_LEFT_JUSTIFY|UIF_DBLCLICK|UIF_SELONLY;
		m_joinserver_menu.actions[i].generic.x			= xoffset;
		m_joinserver_menu.actions[i].generic.y			= y += UIFT_SIZEINC;
		m_joinserver_menu.actions[i].generic.callBack	= JoinServerFunc;
		m_joinserver_menu.actions[i].generic.statusBar	= "Press [ENTER] or [Click & 'Play'] or [DBLCLICK] to connect";

		UI_AddItem (&m_joinserver_menu.framework,		&m_joinserver_menu.actions[i]);
	}

	m_joinserver_menu.back_action.generic.type			= UITYPE_ACTION;
	m_joinserver_menu.back_action.generic.flags			= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_joinserver_menu.back_action.generic.x				= (-6 * UIFT_SIZELG);
	m_joinserver_menu.back_action.generic.y				= y += UIFT_SIZEINCLG * 2;
	m_joinserver_menu.back_action.generic.name			= "< Back";
	m_joinserver_menu.back_action.generic.callBack		= Back_Menu;
	m_joinserver_menu.back_action.generic.statusBar		= "Back a menu";

	m_joinserver_menu.play_action.generic.type			= UITYPE_ACTION;
	m_joinserver_menu.play_action.generic.flags			= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_joinserver_menu.play_action.generic.x				= (6 * UIFT_SIZELG);
	m_joinserver_menu.play_action.generic.y				= y;
	m_joinserver_menu.play_action.generic.name			= "Play >";
	m_joinserver_menu.play_action.generic.callBack		= JoinServerFunc;
	m_joinserver_menu.play_action.generic.statusBar		= "Join Game";

	UI_AddItem (&m_joinserver_menu.framework,			&m_joinserver_menu.back_action);
	UI_AddItem (&m_joinserver_menu.framework,			&m_joinserver_menu.play_action);

	SearchLocalGamesFunc (&m_joinserver_menu.framework);

	UI_Center (&m_joinserver_menu.framework);
	UI_Setup (&m_joinserver_menu.framework);
}


/*
=============
JoinServerMenu_Draw
=============
*/
void JoinServerMenu_Draw (void) {
	UI_Center (&m_joinserver_menu.framework);
	UI_Setup (&m_joinserver_menu.framework);

	UI_AdjustTextCursor (&m_joinserver_menu.framework, 1);
	UI_Draw (&m_joinserver_menu.framework);
}


/*
=============
JoinServerMenu_Key
=============
*/
struct sfx_s *JoinServerMenu_Key (int key) {
	return DefaultMenu_Key (&m_joinserver_menu.framework, key);
}


/*
=============
UI_JoinServerMenu_f
=============
*/
void UI_JoinServerMenu_f (void) {
	JoinServerMenu_Init ();
	UI_PushMenu (&m_joinserver_menu.framework, JoinServerMenu_Draw, JoinServerMenu_Key);
}
