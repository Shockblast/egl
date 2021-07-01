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
// cg_loadscreen.c
//

#include "cg_local.h"

/*
=============================================================================

	LOADING SCREEN

=============================================================================
*/

/*
=================
CG_PrepLoadScreen
=================
*/
static void CG_PrepLoadScreen (void)
{
	char	buffer[MAX_QPATH];

	if (cgMedia.loadScreenPrepped)
		return;

	if (cg.configStrings[CS_MODELS+1][0]) {
		Com_FileBase (cg.configStrings[CS_MODELS+1], buffer);
		cgMedia.loadMapShot = cgi.R_RegisterPic (Q_VarArgs ("maps/%s.tga", buffer));

		cgMedia.loadScreenPrepped = qTrue;
	}
	else {
		cgMedia.loadMapShot = NULL;
	}
}


/*
=================
CG_DrawLoadingScreen
=================
*/
static void CG_DrawLoadingScreen (void)
{
	char	buffer[MAX_QPATH];
	vec4_t	brownText;
	float	barSize;
	int		yOffset;
	char	*token;

	if (cgMedia.initialized)
		return;

	// Fill back with black
	cgi.R_DrawFill (0, 0, cg.glConfig.vidWidth, cg.glConfig.vidHeight, colorBlack);

	if (!cgMedia.loadScreenPrepped) {
		CG_PrepLoadScreen ();
	}
	if (!cg.configStrings[CS_MODELS+1][0])
		return;

	Vector4Set (brownText, 0.85f, 0.7f, 0.2f, 1);

	// Backsplash
	cgi.R_DrawPic (cgMedia.loadSplash, 0, 0, 0, cg.glConfig.vidWidth, cg.glConfig.vidHeight, 0, 0, 1, 1, colorWhite);

	// Map name
	Q_strncpyz (buffer, cg.configStrings[CS_NAME], sizeof (buffer));
	for (token=strtok(buffer, "\n"), yOffset=0 ; token ; token=strtok(NULL, "\n")) {
		CG_DrawString (10*CG_HSCALE, (yOffset+170)*CG_VSCALE, FS_SHADOW, CG_HSCALE*1.25f, token, brownText);
		yOffset += 10;
	}

	// Maps/map.bsp name
	VectorScale (brownText, 0.7f, brownText);

	Q_strncpyz (buffer, cg.configStrings[CS_MODELS+1], sizeof (buffer));
	CG_DrawString (10*CG_HSCALE, (yOffset+170)*CG_VSCALE, FS_SHADOW, CG_HSCALE*1.25f, buffer, brownText);

	// Loading item
	if (cg.loadingString[0]) {
		CG_DrawString (10*CG_HSCALE, (yOffset+200)*CG_VSCALE, FS_SHADOW, CG_HSCALE, cg.loadingString, colorLtGrey);
		CG_DrawString (10*CG_HSCALE, (yOffset+200)*CG_VSCALE+CGFT_VSCALE, FS_SHADOW, CG_HSCALE, cg.loadFileName, colorLtGrey);
	}

	// Map image
	cgi.R_DrawFill (cg.glConfig.vidWidth - ((18+260)*CG_HSCALE), 18*CG_VSCALE, 260*CG_HSCALE, 260*CG_VSCALE, colorBlack);
	cgi.R_DrawPic (cgMedia.loadMapShot ? cgMedia.loadMapShot : cgMedia.loadNoMapShot, 0,
		cg.glConfig.vidWidth - ((20+256)*CG_HSCALE), 20*CG_VSCALE,
		256*CG_HSCALE, 256*CG_VSCALE,
		0, 0, 1, 1, colorWhite);

	// Loading bar
	barSize = cg.glConfig.vidWidth*(cg.loadingPercent/100.0);
	cgi.R_DrawPic (cgMedia.loadBarNeg, 0, barSize, 310*CG_VSCALE, cg.glConfig.vidWidth-(int)barSize, 32*CG_VSCALE, 0, 0, 1, 1, colorWhite);
	if (cg.loadingPercent)
		cgi.R_DrawPic (cgMedia.loadBarPos, 0, 0, 310*CG_VSCALE, (int)barSize, 32*CG_VSCALE, 0, 0, 1, 1, colorWhite);

	// Percentage display
	Q_snprintfz (buffer, sizeof (buffer), "%3.1f%%", cg.loadingPercent);
	CG_DrawString ((cg.glConfig.vidWidth-(strlen(buffer)*CGFT_HSCALE*1.5))*0.5, (310+32)*CG_VSCALE-(CGFT_VSCALE*2), FS_SHADOW, CG_HSCALE*1.5, buffer, colorLtGrey);
}


/*
=================
CG_LoadingPercent
=================
*/
void CG_LoadingPercent (float percent)
{
	if (cgMedia.initialized)
		return;
	if (!cg.mapLoading)
		return;

	cg.loadingPercent = percent;
	cgi.R_UpdateScreen ();
	cgi.Sys_SendKeyEvents ();	// pump message loop
}


/*
=================
CG_IncLoadPercent
=================
*/
void CG_IncLoadPercent (float increment)
{
	if (cgMedia.initialized)
		return;
	if (!cg.mapLoading)
		return;

	cg.loadingPercent += increment;
	cgi.R_UpdateScreen ();
	cgi.Sys_SendKeyEvents ();	// pump message loop
}


/*
=================
CG_LoadingString
=================
*/
void CG_LoadingString (char *str)
{
	if (cgMedia.initialized)
		return;
	if (!cg.mapLoading)
		return;

	if (str) {
		Q_strncpyz (cg.loadingString, str, sizeof (cg.loadingString));
		cgi.R_UpdateScreen ();
		cgi.Sys_SendKeyEvents ();	// pump message loop
	}
	else
		cg.loadFileName[0] = '\0';
}


/*
=================
CG_LoadingFilename
=================
*/
void CG_LoadingFilename (char *str)
{
	if (cgMedia.initialized)
		return;
	if (!cg.mapLoading)
		return;

	if (str) {
		Q_strncpyz (cg.loadFileName, str, sizeof (cg.loadFileName));
		cgi.R_UpdateScreen ();
		cgi.Sys_SendKeyEvents ();	// pump message loop
	}
	else
		cg.loadFileName[0] = '\0';
}


/*
==============
CG_UpdateConnectInfo

-1 is passed to dlPercent and bytesDownloaded if there is no file downloading
==============
*/
void CG_UpdateConnectInfo (char *serverName, int connectCount, char *dlFileName, int dlPercent, float bytesDownloaded)
{
	cg.serverName = serverName;
	cg.connectCount = connectCount;
	cg.download.fileName = dlFileName;
	cg.download.percent = dlPercent;
	cg.download.bytesDown = bytesDownloaded;

	cg.download.downloading = (dlFileName && dlFileName[0] && dlPercent >= 0 && bytesDownloaded >= 0);
	cg.localServer = (cg.maxClients <= 1 || !cg.serverName || !cg.serverName[0] || !Q_stricmp (cg.serverName, "localhost"));
}


/*
==============
CG_DrawConnectScreen
==============
*/
#define UISCALE		(cg.glConfig.vidWidth / 640.0)
#define FTSCALE		(8*UISCALE)
void CG_DrawConnectScreen (void)
{
	char	buffer[64];
	vec4_t	brownText;

	Vector4Set (brownText, 0.85f, 0.7f, 0.2f, 1);

	// Only draw the background if the loading screen isn't up
	if (!cg.mapLoading)
		cgi.R_DrawFill (0, 0, cg.glConfig.vidWidth, cg.glConfig.vidHeight, colorBlack);

	if (!cg.localServer) {
		Q_snprintfz (buffer, sizeof (buffer), "Connecting to: %s", cg.serverName);
		CG_DrawString (10*UISCALE, 158*UISCALE, FS_SHADOW, UISCALE, buffer, colorWhite);
	}

	if (cg.download.downloading) {
		char	dlBar[28]; // left[percent/4]right|terminate
		int		spot;

		Q_snprintfz (buffer, sizeof (buffer), "Downloading %s...", cg.download.fileName);
		CG_DrawString (10*UISCALE, 158*UISCALE+(FTSCALE+2), FS_SHADOW, UISCALE, buffer, colorWhite);

		memset (&dlBar, '\x81', sizeof (dlBar));
		dlBar[0] = '\x80';		// left
		dlBar[26] = '\x82';		// right
		dlBar[27] = '\0';		// terminate

		// Percent location
		spot = (cg.download.percent * 0.25f) + 1;
		if (spot > 25)
			spot = 25;
		dlBar[spot] = '\x83';

		Q_snprintfz (buffer, sizeof (buffer), "%s %i%%", dlBar, cg.download.percent);
		CG_DrawString (10*UISCALE+(FTSCALE*12), 158*UISCALE+((FTSCALE+2)*2), FS_SHADOW, UISCALE, buffer, colorWhite);

		Q_snprintfz (buffer, sizeof (buffer), "Downloaded: %.0fKBytes", cg.download.bytesDown*0.0009765625f);
		CG_DrawString (10*UISCALE+(FTSCALE*12), 158*UISCALE+((FTSCALE+2)*3), FS_SHADOW, UISCALE, buffer, colorWhite);
	}
	else {
		if (cg.mapLoading) {
			// Stuff that shows while the loading screen shows
			CG_DrawLoadingScreen ();
		}
		else {
			// Show connection status
			if (cg.localServer) {
				Q_strncpyz (buffer, "Loading...", sizeof (buffer));
				CG_DrawString (10*UISCALE, 158*UISCALE+FTSCALE+2, 0, UISCALE, buffer, colorWhite);
			}
			else {
				Q_snprintfz (buffer, sizeof (buffer), "Awaiting connection... %i", cg.connectCount);
				CG_DrawString (10*UISCALE, 158*UISCALE+FTSCALE+2, 0, UISCALE, buffer, colorWhite);
			}
		}
	}
}
