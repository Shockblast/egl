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
//

#include "ui_local.h"

static char		*ui_dlFileName;
static int		ui_dlPercent;
static float	ui_dlBytesDown;

/*
==============
UI_UpdateDownloadInfo
==============
*/
void UI_UpdateDownloadInfo (char *fileName, int percent, float downloaded)
{
	ui_dlFileName = fileName;
	ui_dlPercent = percent;

	if (ui_dlPercent < 0)
		ui_dlPercent = 0;
	else if (ui_dlPercent > 100)
		ui_dlPercent = 100;

	ui_dlBytesDown = downloaded;
}


/*
==============
UI_DrawConnectScreen

If cGameOn is true, the map is loading
==============
*/
#define UISCALE		(uiState.vidWidth / 640.0)
#define FTSCALE		(8*UISCALE)
void UI_DrawConnectScreen (char *serverName, int connectCount, qBool backGround, qBool cGameOn, qBool downloading)
{
	qBool	isLocalHost;
	char	buffer[MAX_QPATH];
	vec4_t	brownText;

	isLocalHost = (!serverName || !serverName[0] || !Q_stricmp (serverName, "localhost"));
	Vector4Set (brownText, 0.85f, 0.7f, 0.2f, 1);

	if (backGround) {
		UI_DrawFill (0, 0, uiState.vidWidth, uiState.vidHeight, colorBlack);
	}

	if (!isLocalHost) {
		Q_snprintfz (buffer, sizeof (buffer), "Connecting to: %s", serverName);
		UI_DrawString (10*UISCALE, 158*UISCALE, FS_SHADOW, UISCALE, buffer, colorWhite);
	}

	if (downloading) {
		char	dlBar[28]; // left[percent/4]right|terminate
		int		spot;

		Q_snprintfz (buffer, sizeof (buffer), "Downloading %s...", ui_dlFileName);
		UI_DrawString (10*UISCALE, 158*UISCALE+(FTSCALE+2), FS_SHADOW, UISCALE, buffer, colorWhite);

		memset (&dlBar, '\x81', sizeof (dlBar));
		dlBar[0] = '\x80';		// left
		dlBar[26] = '\x82';		// right
		dlBar[27] = '\0';		// terminate

		// percent location
		spot = (ui_dlPercent * 0.25f) + 1;
		if (spot > 25)
			spot = 25;
		dlBar[spot] = '\x83';

		Q_snprintfz (buffer, sizeof (buffer), "%s %i%%", dlBar, ui_dlPercent);
		UI_DrawString (10*UISCALE+(FTSCALE*12), 158*UISCALE+((FTSCALE+2)*2), FS_SHADOW, UISCALE, buffer, colorWhite);

		Q_snprintfz (buffer, sizeof (buffer), "Downloaded: %.0fKBytes", ui_dlBytesDown*0.0009765625f);
		UI_DrawString (10*UISCALE+(FTSCALE*12), 158*UISCALE+((FTSCALE+2)*3), FS_SHADOW, UISCALE, buffer, colorWhite);
	}
	else {
		if (cGameOn) {
			// stuff that shows while the loading screen shows
		}
		else {
			// show connection status
			if (isLocalHost) {
				Q_strncpyz (buffer, "Loading...", sizeof (buffer));
				UI_DrawString (10*UISCALE, 158*UISCALE+FTSCALE+2, 0, UISCALE, buffer, colorWhite);
			}
			else {
				Q_snprintfz (buffer, sizeof (buffer), "Awaiting connection... %i", connectCount);
				UI_DrawString (10*UISCALE, 158*UISCALE+FTSCALE+2, 0, UISCALE, buffer, colorWhite);
			}
		}
	}
}
