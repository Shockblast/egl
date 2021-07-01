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

typedef struct {
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

typedef struct {
	char		name[64];
	netAdr_t	netadr;

	// kthx do these
//	char		map[32];

//	int			players;
//	int			maxplyrs;
} js_serverlist_t;

js_serverlist_t	js_servers[MAX_LOCAL_SERVERS];

int		js_numservers;
#define SERVER_BLANKSTRING	"<no server>"

void UI_AddToServerList (netAdr_t adr, char *info) {
	int		i;

	if (js_numservers == MAX_LOCAL_SERVERS)
		return;

	while (*info == ' ')
			info++;

	// ignore if duplicated
	for (i=0 ; i<js_numservers ; i++)
		if ((js_servers[i].netadr.ip == adr.ip) &&
			(js_servers[i].netadr.ipx == adr.ipx) &&
			(js_servers[i].netadr.port == adr.port) &&
			(js_servers[i].netadr.naType == adr.naType))//!strcmp (info, local_server_names[i]))
			return;

	js_servers[js_numservers].netadr = adr;
	strncpy (js_servers[js_numservers].name, info, sizeof (js_servers[js_numservers].name)-1);

	js_numservers++;
}

void JoinServerFunc (void *used) {
	char	buffer[128];
	int		index;

	if (ui_SelItem)	index = (uiAction_t *)ui_SelItem - m_joinserver_menu.actions;
	else			index = (uiAction_t *)used - m_joinserver_menu.actions;

	if (Q_stricmp (js_servers[index].name, SERVER_BLANKSTRING) == 0)
		return;

	if (index >= js_numservers)
		return;

	Com_sprintf (buffer, sizeof (buffer), "connect %s\n", uii.NET_AdrToString (js_servers[index].netadr));
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

	js_numservers = 0;
	for (i=0 ; i<MAX_LOCAL_SERVERS ; i++)
		strcpy (js_servers[i].name, SERVER_BLANKSTRING);

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

	m_joinserver_menu.framework.x		= uis.vidWidth * 0.5;
	m_joinserver_menu.framework.y		= 0;
	m_joinserver_menu.framework.nitems	= 0;

	UI_Banner (&m_joinserver_menu.banner, uiMedia.joinServerBanner, &y);

	m_joinserver_menu.address_book_action.generic.type		= UITYPE_ACTION;
	m_joinserver_menu.address_book_action.generic.name		= "Address Book";
	m_joinserver_menu.address_book_action.generic.flags		= UIF_LEFT_JUSTIFY|UIF_SHADOW;
	m_joinserver_menu.address_book_action.generic.x			= xoffset;
	m_joinserver_menu.address_book_action.generic.y			= y += UIFT_SIZEINC;
	m_joinserver_menu.address_book_action.generic.callback	= ADDRBOOK_Menu;

	m_joinserver_menu.refresh_action.generic.type		= UITYPE_ACTION;
	m_joinserver_menu.refresh_action.generic.name		= "Refresh Server List";
	m_joinserver_menu.refresh_action.generic.flags		= UIF_LEFT_JUSTIFY|UIF_SHADOW;
	m_joinserver_menu.refresh_action.generic.x			= xoffset;
	m_joinserver_menu.refresh_action.generic.y			= y += UIFT_SIZEINC;
	m_joinserver_menu.refresh_action.generic.callback	= SearchLocalGamesFunc;
	m_joinserver_menu.refresh_action.generic.statusbar	= "search for servers";

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
		strcpy (js_servers[i].name, SERVER_BLANKSTRING);
		m_joinserver_menu.actions[i].generic.name		= js_servers[i].name;
		m_joinserver_menu.actions[i].generic.flags		= UIF_LEFT_JUSTIFY|UIF_DBLCLICK|UIF_SELONLY;
		m_joinserver_menu.actions[i].generic.x			= xoffset;
		m_joinserver_menu.actions[i].generic.y			= y += UIFT_SIZEINC;
		m_joinserver_menu.actions[i].generic.callback	= JoinServerFunc;
		m_joinserver_menu.actions[i].generic.statusbar	= "Press [ENTER] or [Click & 'Play'] or [DBLCLICK] to connect";

		UI_AddItem (&m_joinserver_menu.framework,		&m_joinserver_menu.actions[i]);
	}

	m_joinserver_menu.back_action.generic.type			= UITYPE_ACTION;
	m_joinserver_menu.back_action.generic.flags			= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_joinserver_menu.back_action.generic.x				= (-6 * UIFT_SIZELG);
	m_joinserver_menu.back_action.generic.y				= y += UIFT_SIZEINCLG * 2;
	m_joinserver_menu.back_action.generic.name			= "< Back";
	m_joinserver_menu.back_action.generic.callback		= Back_Menu;
	m_joinserver_menu.back_action.generic.statusbar		= "Back a menu";

	m_joinserver_menu.play_action.generic.type			= UITYPE_ACTION;
	m_joinserver_menu.play_action.generic.flags			= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_joinserver_menu.play_action.generic.x				= (6 * UIFT_SIZELG);
	m_joinserver_menu.play_action.generic.y				= y;
	m_joinserver_menu.play_action.generic.name			= "Play >";
	m_joinserver_menu.play_action.generic.callback		= JoinServerFunc;
	m_joinserver_menu.play_action.generic.statusbar		= "Join Game";

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
const char *JoinServerMenu_Key (int key) {
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
