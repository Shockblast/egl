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

typedef struct mJoinServerMenu_s {
	uiFrameWork_t	framework;

	uiImage_t		banner;

	uiAction_t		addressBookAction;
	uiAction_t		bookServersAction;
	uiAction_t		localServersAction;

	uiAction_t		nameSortAction;
	uiAction_t		gameSortAction;
	uiAction_t		mapSortAction;
	uiAction_t		playerSortAction;
	uiAction_t		pingSortAction;

	uiAction_t		hostNames[MAX_LOCAL_SERVERS];
	uiAction_t		gameNames[MAX_LOCAL_SERVERS];
	uiAction_t		serverMap[MAX_LOCAL_SERVERS];
	uiAction_t		serverPlayers[MAX_LOCAL_SERVERS];
	uiAction_t		serverPing[MAX_LOCAL_SERVERS];

	uiAction_t		backAction;
	uiAction_t		refreshAction;
	uiAction_t		playAction;
} mJoinServerMenu_t;

mJoinServerMenu_t mJoinServerMenu;

void JoinServerMenu_Init (qBool sort);

// ==========================================================================

typedef struct serverItem_s {
	char		*mapName;
	char		*hostName, *shortName;
	char		*gameName;
	char		*netAddress;

	char		*playersStr;
	int			numPlayers;
	int			maxPlayers;

	char		*pingString;
	int			ping;

	qBool		statusPacket;
} serverItem_t;

static int		totalServers;
serverItem_t	sortedServers[MAX_LOCAL_SERVERS];

static int		pingTime;

#define MAX_HOSTNAME_LEN	32
#define MAX_GAMENAME_LEN	10
#define MAX_MAPNAME_LEN		16
#define MAX_PING_LEN		10

enum {
	JS_SORT_HOSTNAME,
	JS_SORT_GAMENAME,
	JS_SORT_MAPNAME,
	JS_SORT_PLAYERCNT,
	JS_SORT_PINGCNT
};

enum {
	JS_PAGE_ADDRBOOK,
	JS_PAGE_LAN
};

/*
=============
UI_FreeServer
=============
*/
static void UI_FreeServer (serverItem_t *server) {
	if (server->mapName)			UI_MemFree (server->mapName);
	if (server->hostName)			UI_MemFree (server->hostName);
	if (server->shortName)			UI_MemFree (server->shortName);
	if (server->gameName)			UI_MemFree (server->gameName);
	if (server->netAddress)			UI_MemFree (server->netAddress);
	if (server->playersStr)			UI_MemFree (server->playersStr);
	if (server->pingString)			UI_MemFree (server->pingString);
	memset (server, 0, sizeof (serverItem_t));
}


/*
=============
UI_DupeCheckServerList

Checks for duplicates and returns true if there is one...
Since status has higher priority than info, if there is already an instance and
it's not status, and the current one is status, the old one is removed.
=============
*/
static qBool UI_DupeCheckServerList (char *adr, qBool status) {
	int		i;

	for (i=0 ; i<totalServers ; i++) {
		if (!sortedServers[i].netAddress && !sortedServers[i].hostName) {
			UI_FreeServer (&sortedServers[i]);
			continue;
		}

		if (sortedServers[i].netAddress && !strcmp (sortedServers[i].netAddress, adr)) {
			if (sortedServers[i].statusPacket && status)
				return qTrue;
			else if (status) {
				UI_FreeServer (&sortedServers[i]);
				return qFalse;
			}
		}
	}

	return qFalse;
}


/*
=============
UI_FreeServerList
=============
*/
void UI_FreeServerList (void) {
	int		i;

	for (i=0 ; i<totalServers ; i++)
		UI_FreeServer (&sortedServers[i]);

	totalServers = 0;
}


/*
=============
UI_ParseServerInfo

Not used by default anymore, but still works
Kind of dumb, since long host names fuck up parsing beyond all control
Don't use it.
=============
*/
qBool UI_ParseServerInfo (char *adr, char *info) {
	char			*token, name[128];
	serverItem_t	*server;

	if (!info || !info[0])
		return qFalse;
	if (!adr || !adr[0])
		return qFalse;

	// kill the retarded '_'
	info[Q_strlen (info)-1] = '\0';

	if (totalServers >= MAX_LOCAL_SERVERS)
		return qTrue;
	if (UI_DupeCheckServerList (adr, qFalse))
		return qTrue;

	server = &sortedServers[totalServers];
	UI_FreeServer (server);
	totalServers++;

	// add net address
	server->netAddress = UI_StrDup (adr);

	// start at end of string
	token = info + Q_strlen (info);

	// find max players
	while (*token != '/')
		token--;

	if (token < info) {
		// not found
		token = info + Q_strlen (info);
		server->playersStr = UI_StrDup ("?/?");
		server->mapName = UI_StrDup ("?");
		server->maxPlayers = -1;
		server->numPlayers = -1;
	}
	else {
		// found
		server->maxPlayers = atoi (token+1);

		// find current number of players
		*token = 0;
		token--;
		while ((token > info) && (*token >= '0') && (*token <= '9'))
			token--;
		server->numPlayers = atoi (token+1);

		// set the player string
		server->playersStr = UI_StrDup (Q_VarArgs ("%i/%i", server->numPlayers, server->maxPlayers));

		// find map name
		while ((token > info) && (*token == ' ')) // clear end whitespace
			token--;
		*(token+1) = 0;

		// go to the beginning of the single word
		while ((token > info) && (*token != ' '))
			token--;
		server->mapName = UI_StrDup (token+1);
	}

	// host name is what's left over
	*token = 0;
	if (Q_strlen (info) > MAX_HOSTNAME_LEN-1) {
		token = info + MAX_HOSTNAME_LEN-4;
		while ((token > info) && (*token == ' '))
			token--;
		*token++ = '.';
		*token++ = '.';
		*token++ = '.';
	}
	else
		token = info + Q_strlen (info);
	*token = 0;
	Com_StripPadding (info, name);
	server->hostName = UI_StrDup (name);
	server->shortName = UI_StrDup (name);

	// add the ping
	server->ping = uii.Sys_Milliseconds () - pingTime;
	server->pingString = UI_StrDup (Q_VarArgs ("%ims", server->ping));

	server->statusPacket = qFalse;

	// print information
	Com_Printf (0, "%s %s ", server->hostName, server->mapName);
	Com_Printf (0, "%i/%i %ims\n", server->numPlayers, server->maxPlayers, server->ping);

	// refresh menu
	// do after printing so that sorting doesn't throw the pointers off
	JoinServerMenu_Init (qTrue);

	return qTrue;
}


/*
=============
UI_ParseServerStatus

Parses a status packet from a server
=============
*/
#define TOKDELIMS	"\\\n"
qBool UI_ParseServerStatus (char *adr, char *info) {
	char			*token, shortName[MAX_HOSTNAME_LEN];
	serverItem_t	*server;

	if (!info || !info[0])
		return qFalse;
	if (!adr || !adr[0])
		return qFalse;

	if (totalServers >= MAX_LOCAL_SERVERS)
		return qTrue;
	if (UI_DupeCheckServerList (adr, qTrue))
		return qTrue;

	server = &sortedServers[totalServers];
	UI_FreeServer (server);
	totalServers++;

	// add net address
	server->netAddress = UI_StrDup (adr);

	// parse out what we want
	token = strtok (info, TOKDELIMS);
	while (token != NULL) {
		if (!strcmp (token, "mapname")) {
			// map name
			token = strtok (NULL, TOKDELIMS);
			server->mapName = UI_StrDup (token);
		}
		else if (!strcmp (token, "curplayers")) {
			// players connected
			token = strtok (NULL, TOKDELIMS);
			server->numPlayers = atoi (token);
		}
		else if (!strcmp (token, "maxclients")) {
			// max players
			token = strtok (NULL, TOKDELIMS);
			server->maxPlayers = atoi (token);
		}
		else if (!strcmp (token, "gamename")) {
			// mod
			server->gameName = UI_StrDup (strtok (NULL, TOKDELIMS));
		}
		else if (!strcmp (token, "hostname")) {
			// server name
			token = strtok (NULL, TOKDELIMS);
			server->hostName = UI_StrDup (token);

			if (server->hostName) {
				Q_strncpyz (shortName, server->hostName, sizeof (shortName));
				server->shortName = UI_StrDup (shortName);
			}
		}

		token = strtok (NULL, TOKDELIMS);
	}

	if (!server->mapName && !server->numPlayers && !server->maxPlayers && !server->gameName && !server->hostName) {
		UI_FreeServer (server);
		return qFalse;
	}

	server->playersStr = UI_StrDup (Q_VarArgs ("%i/%i", server->numPlayers, server->maxPlayers));

	// add the ping
	server->ping = uii.Sys_Milliseconds () - pingTime;
	server->pingString = UI_StrDup (Q_VarArgs ("%ims", server->ping));

	server->statusPacket = qTrue;

	// print information
	Com_Printf (0, "%s %s ", server->hostName, server->mapName);
	Com_Printf (0, "%i/%i %ims\n", server->numPlayers, server->maxPlayers, server->ping);

	// refresh menu
	// do after printing so that sorting doesn't throw the pointers off
	JoinServerMenu_Init (qTrue);

	return qTrue;
}

// ==========================================================================

static void JoinServerFunc (void *used) {
	char	buffer[128];
	int		index;

	if (uiSelItem)
		index = (uiAction_t *)uiSelItem - mJoinServerMenu.hostNames;
	else
		index = (uiAction_t *)used - mJoinServerMenu.hostNames;

	if (index >= totalServers)
		return;
	if (!sortedServers[index].netAddress)
		return;

	Q_snprintfz (buffer, sizeof (buffer), "connect %s\n", sortedServers[index].netAddress);
	uii.Cbuf_AddText (buffer);

	UI_ForceMenuOff ();
}

static void ADDRBOOK_MenuFunc (void *unused) {
	UI_AddressBookMenu_f ();
}

static void JS_Menu_PopFunc (void *unused) {
	UI_FreeServerList ();
	UI_PopMenu ();
}

static void SearchLocalGamesFunc (void *item) {
	float	midrow = (uis.vidWidth*0.5) - (18*UIFT_SIZEMED);
	float	midcol = (uis.vidHeight*0.5) - (3*UIFT_SIZEMED);
	int		i;
	char	*adrString;
	char	name[32];

	UI_FreeServerList ();
	JoinServerMenu_Init (qTrue);

	UI_DrawTextBox (midrow, midcol, UIFT_SCALEMED, 36, 4);
	UI_DrawString (midrow + (UIFT_SIZEMED*2), midcol + UIFT_SIZEMED, 0, UIFT_SCALEMED,		"       --- PLEASE WAIT! ---       ",	colorGreen);
	UI_DrawString (midrow + (UIFT_SIZEMED*2), midcol + (UIFT_SIZEMED*2), 0, UIFT_SCALEMED,	"Searching for local servers, this",	colorGreen);
	UI_DrawString (midrow + (UIFT_SIZEMED*2), midcol + (UIFT_SIZEMED*3), 0, UIFT_SCALEMED,	"could take up to a minute, please",	colorGreen);
	UI_DrawString (midrow + (UIFT_SIZEMED*2), midcol + (UIFT_SIZEMED*4), 0, UIFT_SCALEMED,	"please be patient.",					colorGreen);

	uii.R_EndFrame ();					// the text box won't show up unless we do a buffer swap
	pingTime = uii.Sys_Milliseconds ();

	if (item == &mJoinServerMenu.bookServersAction)
		uii.Cvar_VariableForceSetValue (ui_jsMenuPage, JS_PAGE_ADDRBOOK);
	else if (item == &mJoinServerMenu.localServersAction)
		uii.Cvar_VariableForceSetValue (ui_jsMenuPage, JS_PAGE_LAN);

	switch (ui_jsMenuPage->integer) {
	case JS_PAGE_LAN:
		mJoinServerMenu.localServersAction.generic.flags |= UIF_FORCESELBAR;
		mJoinServerMenu.bookServersAction.generic.flags &= ~UIF_FORCESELBAR;

//		uii.Cbuf_AddText ("pinglocal\n");	// send out info packet request
		uii.Cbuf_AddText ("statuslocal\n");	// send out status packet request
		break;
	case JS_PAGE_ADDRBOOK:
		mJoinServerMenu.localServersAction.generic.flags &= ~UIF_FORCESELBAR;
		mJoinServerMenu.bookServersAction.generic.flags |= UIF_FORCESELBAR;

		for (i=0 ; i<MAX_ADDRBOOK_SAVES ; i++) {
			Q_snprintfz (name, sizeof (name), "adr%i", i);
			adrString = uii.Cvar_VariableString (name);

			if (adrString)
				uii.Cbuf_AddText (Q_VarArgs ("sstatus %s\n", adrString));
		}
	}

	JoinServerMenu_Init (qTrue);
}

// ==========================================================================

/*
=============
Sort_SetHighlight
=============
*/
void Sort_SetHighlight (void) {
	mJoinServerMenu.nameSortAction.generic.flags &= ~UIF_FORCESELBAR;
	mJoinServerMenu.gameSortAction.generic.flags &= ~UIF_FORCESELBAR;
	mJoinServerMenu.mapSortAction.generic.flags &= ~UIF_FORCESELBAR;
	mJoinServerMenu.playerSortAction.generic.flags &= ~UIF_FORCESELBAR;
	mJoinServerMenu.pingSortAction.generic.flags &= ~UIF_FORCESELBAR;

	switch (ui_jsSortItem->integer) {
	case JS_SORT_HOSTNAME:	mJoinServerMenu.nameSortAction.generic.flags |= UIF_FORCESELBAR;	break;
	case JS_SORT_GAMENAME:	mJoinServerMenu.gameSortAction.generic.flags |= UIF_FORCESELBAR;	break;
	case JS_SORT_MAPNAME:	mJoinServerMenu.mapSortAction.generic.flags |= UIF_FORCESELBAR;		break;
	case JS_SORT_PLAYERCNT:	mJoinServerMenu.playerSortAction.generic.flags |= UIF_FORCESELBAR;	break;
	case JS_SORT_PINGCNT:	mJoinServerMenu.pingSortAction.generic.flags |= UIF_FORCESELBAR;	break;
	default:				Com_Printf (0, S_COLOR_RED "Invalid ui_jsSortItem value\n");		break;
	}
}


/*
=============
Sort_HostNameFunc
=============
*/
static int hostNameSortCmp (const serverItem_t *_a, const serverItem_t *_b) {
	const serverItem_t *a = (const serverItem_t *) _a;
	const serverItem_t *b = (const serverItem_t *) _b;

	if (a && a->hostName && b && b->hostName)
		return Q_stricmp (a->hostName, b->hostName);

	return 0;
}
static int hostNameInvSortCmp (const serverItem_t *_a, const serverItem_t *_b) {
	const serverItem_t *a = (const serverItem_t *) _a;
	const serverItem_t *b = (const serverItem_t *) _b;

	if (a && a->hostName && b && b->hostName)
		return -Q_stricmp (a->hostName, b->hostName);

	return 0;
}
static void Sort_HostNameFunc (void *item) {
	uii.Cvar_VariableForceSetValue (ui_jsSortItem, 0);

	// sort
	if (item)
		uii.Cvar_VariableForceSetValue (ui_jsSortMethod, ui_jsSortMethod->integer ? 0 : 1);
	switch (ui_jsSortMethod->integer) {
		case 0:		qsort (sortedServers, totalServers, sizeof (serverItem_t), hostNameSortCmp);	break;
		default:	qsort (sortedServers, totalServers, sizeof (serverItem_t), hostNameInvSortCmp);	break;
	}

	if (item)
		JoinServerMenu_Init (qFalse);
}


/*
=============
Sort_GameNameFunc
=============
*/
static int gameNameSortCmp (const serverItem_t *_a, const serverItem_t *_b) {
	const serverItem_t *a = (const serverItem_t *) _a;
	const serverItem_t *b = (const serverItem_t *) _b;

	if (a && a->gameName && b && b->gameName)
		return Q_stricmp (a->gameName, b->gameName);

	return 0;
}
static int gameNameInvSortCmp (const serverItem_t *_a, const serverItem_t *_b) {
	const serverItem_t *a = (const serverItem_t *) _a;
	const serverItem_t *b = (const serverItem_t *) _b;

	if (a && a->gameName && b && b->gameName)
		return -Q_stricmp (a->gameName, b->gameName);

	return 0;
}
static void Sort_GameNameFunc (void *item) {
	uii.Cvar_VariableForceSetValue (ui_jsSortItem, 1);
	// sort
	if (item)
		uii.Cvar_VariableForceSetValue (ui_jsSortMethod, ui_jsSortMethod->integer ? 0 : 1);
	switch (ui_jsSortMethod->integer) {
		case 0:		qsort (sortedServers, totalServers, sizeof (serverItem_t), gameNameSortCmp);	break;
		default:	qsort (sortedServers, totalServers, sizeof (serverItem_t), gameNameInvSortCmp);	break;
	}

	if (item)
		JoinServerMenu_Init (qFalse);
}


/*
=============
Sort_MapNameFunc
=============
*/
static int mapNameSortCmp (const serverItem_t *_a, const serverItem_t *_b) {
	const serverItem_t *a = (const serverItem_t *) _a;
	const serverItem_t *b = (const serverItem_t *) _b;

	if (a && a->mapName && b && b->mapName)
		return Q_stricmp (a->mapName, b->mapName);

	return 1;
}
static int mapNameInvSortCmp (const serverItem_t *_a, const serverItem_t *_b) {
	const serverItem_t *a = (const serverItem_t *) _a;
	const serverItem_t *b = (const serverItem_t *) _b;

	if (a && a->mapName && b && b->mapName)
		return -Q_stricmp (a->mapName, b->mapName);

	return 1;
}
static void Sort_MapNameFunc (void *item) {
	uii.Cvar_VariableForceSetValue (ui_jsSortItem, 2);
	// sort
	if (item)
		uii.Cvar_VariableForceSetValue (ui_jsSortMethod, ui_jsSortMethod->integer ? 0 : 1);
	switch (ui_jsSortMethod->integer) {
		case 0:		qsort (sortedServers, totalServers, sizeof (serverItem_t), mapNameSortCmp);		break;
		default:	qsort (sortedServers, totalServers, sizeof (serverItem_t), mapNameInvSortCmp);	break;
	}

	if (item)
		JoinServerMenu_Init (qFalse);
}


/*
=============
Sort_PlayerCntFunc
=============
*/
static int playerSortCmp (const serverItem_t *_a, const serverItem_t *_b) {
	const serverItem_t *a = (const serverItem_t *) _a;
	const serverItem_t *b = (const serverItem_t *) _b;

	if (a && b) {
		if (a->numPlayers > b->numPlayers)
			return 1;
		else
			return -1;
	}

	return 1;
}
static int playerInvSortCmp (const serverItem_t *_a, const serverItem_t *_b) {
	const serverItem_t *a = (const serverItem_t *) _a;
	const serverItem_t *b = (const serverItem_t *) _b;

	if (a && b) {
		if (a->numPlayers > b->numPlayers)
			return -1;
		else
			return 1;
	}

	return -1;
}
static void Sort_PlayerCntFunc (void *item) {
	uii.Cvar_VariableForceSetValue (ui_jsSortItem, 3);
	// sort
	if (item)
		uii.Cvar_VariableForceSetValue (ui_jsSortMethod, ui_jsSortMethod->integer ? 0 : 1);
	switch (ui_jsSortMethod->integer) {
		case 0:		qsort (sortedServers, totalServers, sizeof (serverItem_t), playerSortCmp);		break;
		default:	qsort (sortedServers, totalServers, sizeof (serverItem_t), playerInvSortCmp);	break;
	}

	if (item)
		JoinServerMenu_Init (qFalse);
}


/*
=============
Sort_PingFunc
=============
*/
static int pingSortCmp (const serverItem_t *_a, const serverItem_t *_b) {
	const serverItem_t *a = (const serverItem_t *) _a;
	const serverItem_t *b = (const serverItem_t *) _b;

	if (a && b) {
		if (a->ping > b->ping)
			return 1;
		else
			return -1;
	}

	return 1;
}
static int pingInvSortCmp (const serverItem_t *_a, const serverItem_t *_b) {
	const serverItem_t *a = (const serverItem_t *) _a;
	const serverItem_t *b = (const serverItem_t *) _b;

	if (a && b) {
		if (a->ping > b->ping)
			return -1;
		else
			return 1;
	}

	return -1;
}
static void Sort_PingFunc (void *item) {
	uii.Cvar_VariableForceSetValue (ui_jsSortItem, 4);
	// sort
	if (item)
		uii.Cvar_VariableForceSetValue (ui_jsSortMethod, ui_jsSortMethod->integer ? 0 : 1);
	switch (ui_jsSortMethod->integer) {
		case 0:		qsort (sortedServers, totalServers, sizeof (serverItem_t), pingSortCmp);	break;
		default:	qsort (sortedServers, totalServers, sizeof (serverItem_t), pingInvSortCmp);	break;
	}

	if (item)
		JoinServerMenu_Init (qFalse);
}

// ==========================================================================

/*
=============
JoinServerMenu_Init
=============
*/
void JoinServerMenu_Init (qBool sort) {
	float	xOffset = -(UIFT_SIZE*36);
	float	oldOffset = xOffset;
	float	y, oldy;
	int		i;

	mJoinServerMenu.framework.x			= uis.vidWidth * 0.5;
	mJoinServerMenu.framework.y			= 0;
	mJoinServerMenu.framework.numItems	= 0;

	if (sort) {
		switch (ui_jsSortItem->integer) {
		case JS_SORT_HOSTNAME:	Sort_HostNameFunc (NULL);	break;
		case JS_SORT_GAMENAME:	Sort_GameNameFunc (NULL);	break;
		case JS_SORT_MAPNAME:	Sort_MapNameFunc (NULL);	break;
		case JS_SORT_PLAYERCNT:	Sort_PlayerCntFunc (NULL);	break;
		case JS_SORT_PINGCNT:	Sort_PingFunc (NULL);		break;
		default:
			Com_Printf (0, S_COLOR_RED "Invalid ui_jsSortItem value\n");
			break;
		}
	}

	UI_Banner (&mJoinServerMenu.banner, uiMedia.banners.joinServer, &y);

	mJoinServerMenu.addressBookAction.generic.type			= UITYPE_ACTION;
	mJoinServerMenu.addressBookAction.generic.name			= "Edit Addresses";
	mJoinServerMenu.addressBookAction.generic.flags			= UIF_LEFT_JUSTIFY|UIF_SHADOW;
	mJoinServerMenu.addressBookAction.generic.x				= xOffset += UIFT_SIZE*3;
	mJoinServerMenu.addressBookAction.generic.y				= y + UIFT_SIZEINC*2;
	mJoinServerMenu.addressBookAction.generic.callBack		= ADDRBOOK_MenuFunc;
	mJoinServerMenu.addressBookAction.generic.statusBar		= "Edit address book entries";

	mJoinServerMenu.bookServersAction.generic.type			= UITYPE_ACTION;
	mJoinServerMenu.bookServersAction.generic.name			= S_COLOR_YELLOW"Address Book";
	mJoinServerMenu.bookServersAction.generic.flags			|= UIF_LEFT_JUSTIFY|UIF_SHADOW;
	mJoinServerMenu.bookServersAction.generic.x				= xOffset;
	mJoinServerMenu.bookServersAction.generic.y				= y += UIFT_SIZEINC;
	mJoinServerMenu.bookServersAction.generic.callBack		= SearchLocalGamesFunc;
	mJoinServerMenu.bookServersAction.generic.cursorDraw	= Cursor_NullFunc;

	mJoinServerMenu.localServersAction.generic.type			= UITYPE_ACTION;
	mJoinServerMenu.localServersAction.generic.name			= S_COLOR_YELLOW"LAN";
	mJoinServerMenu.localServersAction.generic.flags		|= UIF_LEFT_JUSTIFY|UIF_SHADOW;
	mJoinServerMenu.localServersAction.generic.x			= xOffset += UIFT_SIZE*16;
	mJoinServerMenu.localServersAction.generic.y			= y;
	mJoinServerMenu.localServersAction.generic.callBack		= SearchLocalGamesFunc;
	mJoinServerMenu.localServersAction.generic.cursorDraw	= Cursor_NullFunc;

	UI_AddItem (&mJoinServerMenu.framework,			&mJoinServerMenu.banner);
	UI_AddItem (&mJoinServerMenu.framework,			&mJoinServerMenu.addressBookAction);
	UI_AddItem (&mJoinServerMenu.framework,			&mJoinServerMenu.bookServersAction);
	UI_AddItem (&mJoinServerMenu.framework,			&mJoinServerMenu.localServersAction);

	xOffset = oldOffset;

	mJoinServerMenu.nameSortAction.generic.type			= UITYPE_ACTION;
	mJoinServerMenu.nameSortAction.generic.name			= S_COLOR_GREEN"Server name";
	mJoinServerMenu.nameSortAction.generic.flags		= UIF_LEFT_JUSTIFY|UIF_SHADOW;
	mJoinServerMenu.nameSortAction.generic.x			= xOffset;
	mJoinServerMenu.nameSortAction.generic.y			= oldy = y += (UIFT_SIZEINC*3);
	mJoinServerMenu.nameSortAction.generic.callBack		= Sort_HostNameFunc;
	mJoinServerMenu.nameSortAction.generic.statusBar	= "Sort by server name";
	mJoinServerMenu.nameSortAction.generic.cursorDraw	= Cursor_NullFunc;

	mJoinServerMenu.gameSortAction.generic.type			= UITYPE_ACTION;
	mJoinServerMenu.gameSortAction.generic.name			= S_COLOR_GREEN"Game";
	mJoinServerMenu.gameSortAction.generic.flags		= UIF_LEFT_JUSTIFY|UIF_SHADOW;
	mJoinServerMenu.gameSortAction.generic.x			= xOffset += UIFT_SIZE*MAX_HOSTNAME_LEN;
	mJoinServerMenu.gameSortAction.generic.y			= oldy;
	mJoinServerMenu.gameSortAction.generic.callBack		= Sort_GameNameFunc;
	mJoinServerMenu.gameSortAction.generic.statusBar	= "Sort by game name";
	mJoinServerMenu.gameSortAction.generic.cursorDraw	= Cursor_NullFunc;

	mJoinServerMenu.mapSortAction.generic.type			= UITYPE_ACTION;
	mJoinServerMenu.mapSortAction.generic.name			= S_COLOR_GREEN"Map";
	mJoinServerMenu.mapSortAction.generic.flags			= UIF_LEFT_JUSTIFY|UIF_SHADOW;
	mJoinServerMenu.mapSortAction.generic.x				= xOffset += UIFT_SIZE*MAX_GAMENAME_LEN;
	mJoinServerMenu.mapSortAction.generic.y				= oldy;
	mJoinServerMenu.mapSortAction.generic.callBack		= Sort_MapNameFunc;
	mJoinServerMenu.mapSortAction.generic.statusBar		= "Sort by mapname";
	mJoinServerMenu.mapSortAction.generic.cursorDraw	= Cursor_NullFunc;

	mJoinServerMenu.playerSortAction.generic.type		= UITYPE_ACTION;
	mJoinServerMenu.playerSortAction.generic.name		= S_COLOR_GREEN"Players";
	mJoinServerMenu.playerSortAction.generic.flags		= UIF_LEFT_JUSTIFY|UIF_SHADOW;
	mJoinServerMenu.playerSortAction.generic.x			= xOffset += UIFT_SIZE*MAX_MAPNAME_LEN;
	mJoinServerMenu.playerSortAction.generic.y			= oldy;
	mJoinServerMenu.playerSortAction.generic.callBack	= Sort_PlayerCntFunc;
	mJoinServerMenu.playerSortAction.generic.statusBar	= "Sort by player count";
	mJoinServerMenu.playerSortAction.generic.cursorDraw	= Cursor_NullFunc;

	mJoinServerMenu.pingSortAction.generic.type			= UITYPE_ACTION;
	mJoinServerMenu.pingSortAction.generic.name			= S_COLOR_GREEN"Ping";
	mJoinServerMenu.pingSortAction.generic.flags		= UIF_LEFT_JUSTIFY|UIF_SHADOW;
	mJoinServerMenu.pingSortAction.generic.x			= xOffset += UIFT_SIZE*MAX_PING_LEN;
	mJoinServerMenu.pingSortAction.generic.y			= oldy;
	mJoinServerMenu.pingSortAction.generic.callBack		= Sort_PingFunc;
	mJoinServerMenu.pingSortAction.generic.statusBar	= "Sort by player count";
	mJoinServerMenu.pingSortAction.generic.cursorDraw	= Cursor_NullFunc;

	Sort_SetHighlight ();

	UI_AddItem (&mJoinServerMenu.framework,			&mJoinServerMenu.nameSortAction);
	UI_AddItem (&mJoinServerMenu.framework,			&mJoinServerMenu.gameSortAction);
	UI_AddItem (&mJoinServerMenu.framework,			&mJoinServerMenu.mapSortAction);
	UI_AddItem (&mJoinServerMenu.framework,			&mJoinServerMenu.playerSortAction);
	UI_AddItem (&mJoinServerMenu.framework,			&mJoinServerMenu.pingSortAction);

	for (i=0 ; i<totalServers ; i++) {
		if (!sortedServers[i].netAddress)
			continue;

		xOffset = oldOffset;

		// server name
		mJoinServerMenu.hostNames[i].generic.type			= UITYPE_ACTION;
		mJoinServerMenu.hostNames[i].generic.name			= sortedServers[i].shortName;
		mJoinServerMenu.hostNames[i].generic.flags			= UIF_LEFT_JUSTIFY|UIF_DBLCLICK|UIF_SELONLY;
		mJoinServerMenu.hostNames[i].generic.x				= xOffset;
		mJoinServerMenu.hostNames[i].generic.y				= y += UIFT_SIZEINC;
		mJoinServerMenu.hostNames[i].generic.callBack		= JoinServerFunc;
		mJoinServerMenu.hostNames[i].generic.statusBar		= sortedServers[i].hostName;

		// game name
		mJoinServerMenu.gameNames[i].generic.type			= UITYPE_ACTION;
		mJoinServerMenu.gameNames[i].generic.name			= sortedServers[i].gameName;
		mJoinServerMenu.gameNames[i].generic.flags			= UIF_NOSELECT|UIF_LEFT_JUSTIFY;
		mJoinServerMenu.gameNames[i].generic.x				= xOffset += UIFT_SIZE*MAX_HOSTNAME_LEN;
		mJoinServerMenu.gameNames[i].generic.y				= y;

		// map name
		mJoinServerMenu.serverMap[i].generic.type			= UITYPE_ACTION;
		mJoinServerMenu.serverMap[i].generic.name			= sortedServers[i].mapName;
		mJoinServerMenu.serverMap[i].generic.flags			= UIF_NOSELECT|UIF_LEFT_JUSTIFY;
		mJoinServerMenu.serverMap[i].generic.x				= xOffset += UIFT_SIZE*MAX_GAMENAME_LEN;
		mJoinServerMenu.serverMap[i].generic.y				= y;

		// players connected
		mJoinServerMenu.serverPlayers[i].generic.type		= UITYPE_ACTION;
		mJoinServerMenu.serverPlayers[i].generic.name		= sortedServers[i].playersStr;
		mJoinServerMenu.serverPlayers[i].generic.flags		= UIF_NOSELECT|UIF_LEFT_JUSTIFY;
		mJoinServerMenu.serverPlayers[i].generic.x			= xOffset += UIFT_SIZE*MAX_MAPNAME_LEN;
		mJoinServerMenu.serverPlayers[i].generic.y			= y;

		// ping
		mJoinServerMenu.serverPing[i].generic.type			= UITYPE_ACTION;
		mJoinServerMenu.serverPing[i].generic.name			= sortedServers[i].pingString;
		mJoinServerMenu.serverPing[i].generic.flags			= UIF_NOSELECT|UIF_LEFT_JUSTIFY;
		mJoinServerMenu.serverPing[i].generic.x				= xOffset += UIFT_SIZE*MAX_PING_LEN;
		mJoinServerMenu.serverPing[i].generic.y				= y;

		UI_AddItem (&mJoinServerMenu.framework,		&mJoinServerMenu.hostNames[i]);
		UI_AddItem (&mJoinServerMenu.framework,		&mJoinServerMenu.gameNames[i]);
		UI_AddItem (&mJoinServerMenu.framework,		&mJoinServerMenu.serverMap[i]);
		UI_AddItem (&mJoinServerMenu.framework,		&mJoinServerMenu.serverPlayers[i]);
		UI_AddItem (&mJoinServerMenu.framework,		&mJoinServerMenu.serverPing[i]);
	}

	mJoinServerMenu.backAction.generic.type			= UITYPE_ACTION;
	mJoinServerMenu.backAction.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	mJoinServerMenu.backAction.generic.x			= (-10 * UIFT_SIZELG);
	mJoinServerMenu.backAction.generic.y			= y = oldy + (UIFT_SIZEINC*(MAX_LOCAL_SERVERS+1)) + (UIFT_SIZEINCLG*2);
	mJoinServerMenu.backAction.generic.name			= "< Back";
	mJoinServerMenu.backAction.generic.callBack		= JS_Menu_PopFunc;
	mJoinServerMenu.backAction.generic.statusBar	= "Back a menu";
	mJoinServerMenu.backAction.generic.cursorDraw	= Cursor_NullFunc;

	mJoinServerMenu.refreshAction.generic.type		= UITYPE_ACTION;
	mJoinServerMenu.refreshAction.generic.name		= "[Update]";
	mJoinServerMenu.refreshAction.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	mJoinServerMenu.refreshAction.generic.x			= 0;
	mJoinServerMenu.refreshAction.generic.y			= y;
	mJoinServerMenu.refreshAction.generic.callBack	= SearchLocalGamesFunc;
	mJoinServerMenu.refreshAction.generic.statusBar	= "Refresh the active server list";
	mJoinServerMenu.refreshAction.generic.cursorDraw= Cursor_NullFunc;

	mJoinServerMenu.playAction.generic.type			= UITYPE_ACTION;
	mJoinServerMenu.playAction.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	mJoinServerMenu.playAction.generic.x			= (10 * UIFT_SIZELG);
	mJoinServerMenu.playAction.generic.y			= y;
	mJoinServerMenu.playAction.generic.name			= "Play >";
	mJoinServerMenu.playAction.generic.callBack		= JoinServerFunc;
	mJoinServerMenu.playAction.generic.statusBar	= "Join Game";
	mJoinServerMenu.playAction.generic.cursorDraw	= Cursor_NullFunc;

	UI_AddItem (&mJoinServerMenu.framework,			&mJoinServerMenu.backAction);
	UI_AddItem (&mJoinServerMenu.framework,			&mJoinServerMenu.refreshAction);
	UI_AddItem (&mJoinServerMenu.framework,			&mJoinServerMenu.playAction);

	UI_Center (&mJoinServerMenu.framework);
	UI_Setup (&mJoinServerMenu.framework);
}


/*
=============
JoinServerMenu_Draw
=============
*/
void JoinServerMenu_Draw (void) {
	UI_Center (&mJoinServerMenu.framework);
	UI_Setup (&mJoinServerMenu.framework);

	UI_AdjustTextCursor (&mJoinServerMenu.framework, 1);
	UI_Draw (&mJoinServerMenu.framework);
}


/*
=============
JoinServerMenu_Key
=============
*/
struct sfx_s *JoinServerMenu_Key (int key) {
	return DefaultMenu_Key (&mJoinServerMenu.framework, key);
}


/*
=============
UI_JoinServerMenu_f
=============
*/
void UI_JoinServerMenu_f (void) {
	SearchLocalGamesFunc (NULL);
	UI_PushMenu (&mJoinServerMenu.framework, JoinServerMenu_Draw, JoinServerMenu_Key);
}
