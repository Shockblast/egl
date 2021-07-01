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
// ui_loadscreen.c

// FIXME is this having it's own file pointless?
//

#include "ui_local.h"

/*
==============
UI_DrawConnectScreen

If cGameOn is true, the map is loading
==============
*/
#define UISCALE		(uis.vidWidth / 640.0)
#define FTSCALE		(8*UISCALE)
void UI_DrawConnectScreen (char *serverName, int connectCount, qBool backGround, qBool cGameOn) {
	qBool	isLocalHost;
	char	buffer[MAX_QPATH];
	vec4_t	brownText;

	isLocalHost = (!serverName || !serverName[0] || !Q_stricmp (serverName, "localhost"));
	Vector4Set (brownText, 0.85, 0.7, 0.2, 1);

	if (backGround) {
		UI_DrawFill (0, 0, uis.vidWidth, uis.vidHeight, colorBlack);
	}

	if (!isLocalHost) {
		Q_snprintfz (buffer, sizeof (buffer), "Connecting to: %s", serverName);
		UI_DrawString (10*UISCALE, 158*UISCALE, FS_SHADOW, UISCALE, buffer, colorWhite);
	}

	if (!cGameOn) {
		if (!isLocalHost) {
			Q_snprintfz (buffer, sizeof (buffer), "Awaiting connection... %i", connectCount);
			UI_DrawString (10*UISCALE, 158*UISCALE+FTSCALE+2, 0, UISCALE, buffer, colorWhite);
		}
		else {
			Q_strncpyz (buffer, "Loading...", sizeof (buffer));
			UI_DrawString (10*UISCALE, 158*UISCALE+FTSCALE+2, 0, UISCALE, buffer, colorWhite);
		}
	}
}
