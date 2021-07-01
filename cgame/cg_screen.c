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
// cg_screen.c
//

#include "cg_local.h"

// center print
static char		Scr_CenterString[1024];
static float	Scr_CenterTime_Start;
static float	Scr_CenterTime_Off;

cVar_t			*crosshair;
static cVar_t	*ch_alpha;
static cVar_t	*ch_pulse;
static cVar_t	*ch_scale;
static cVar_t	*ch_red;
static cVar_t	*ch_green;
static cVar_t	*ch_blue;

static cVar_t	*cl_showfps;
static cVar_t	*cl_showping;
static cVar_t	*cl_showtime;

static cVar_t	*scr_centertime;
static cVar_t	*scr_showpause;

// ============================================================================

/*
=================
SCR_DrawCrosshair
=================
*/
static void SCR_DrawCrosshair (void) {
	vec4_t	color;
	int		width, height;

	if (!crosshair->integer)
		return;

	if (crosshair->modified)
		CL_InitCrosshairImage ();

	if (!cgMedia.crosshairImage)
		return;

	color[3] = ch_alpha->value;
	if (ch_pulse->value)
		color[3] = (0.75*ch_alpha->value) + ((0.25*ch_alpha->value) * sin (AngleMod ((cgs.realTime * 0.005) * ch_pulse->value)));

	Vector4Set (color,
				clamp (ch_red->value, 0, 1),
				clamp (ch_green->value, 0, 1),
				clamp (ch_blue->value, 0, 1),
				clamp (color[3], 0, 1));

	cgi.Img_GetSize (cgMedia.crosshairImage, &width, &height);
	cgi.Draw_Pic (cgMedia.crosshairImage,
		(cg.vidWidth * 0.5) - (width * ch_scale->value * 0.5),
		(cg.vidHeight * 0.5) - (height * ch_scale->value * 0.5),
		width * ch_scale->value,
		height * ch_scale->value,
		0, 0, 1, 1,
		color);
}


/*
==============
SCR_DrawLagIcon
==============
*/
static void SCR_DrawLagIcon (void) {
	vec4_t	color;
	int		outgoingSequence;
	int		incomingAcknowledged;
	int		width, height;

	cgi.NET_GetSequenceState (&outgoingSequence, &incomingAcknowledged);
	if (outgoingSequence - incomingAcknowledged < CMD_MASK)
		return;

	Vector4Set (color, colorWhite[0], colorWhite[1], colorWhite[2], scr_hudalpha->value);
	cgi.Img_GetSize (cgMedia.hudNetImage, &width, &height);
	cgi.Draw_Pic (
		cgMedia.hudNetImage, 64, 0,
		width*HUD_SCALE, height*HUD_SCALE,
		0, 0, 1, 1, color);
}


/*
==============
SCR_DrawPause
==============
*/
static void SCR_DrawPause (void) {
	vec4_t	color;
	int		width, height;

	if (!scr_showpause->value)
		return;
	if (!cgi.Cvar_VariableInteger ("paused"))
		return;
	if (cgi.Key_KeyDest (KD_MENU))
		return;

	Vector4Set (color, colorWhite[0], colorWhite[1], colorWhite[2], scr_hudalpha->value);
	cgi.Img_GetSize (cgMedia.hudPausedImage, &width, &height);
	cgi.Draw_Pic (
		cgMedia.hudPausedImage,
		(cg.vidWidth - (width*HUD_SCALE))*0.5,
		((cg.vidHeight*0.5) - (height*HUD_SCALE)),
		width*HUD_SCALE, height*HUD_SCALE,
		0, 0, 1, 1, color);
}

/*
===============================================================================

	DISPLAY INFORMATION

===============================================================================
*/

// information display
static int			Scr_PingDisplay;

/*
================
SCR_UpdatePING
================
*/
void SCR_UpdatePING (void) {
	int		incAck;

	cgi.NET_GetSequenceState (NULL, &incAck);
	Scr_PingDisplay = cgs.realTime - cgi.NET_GetUserCmdTime (incAck & CMD_MASK);
}


/*
================
SCR_DrawInfo
================
*/
static void SCR_DrawInfo (void) {
	int		vOffset = 4;
	vec4_t	color;

	static uInt		Scr_FpsDelay;
	static char		Scr_FpsDispBuff[32];
	static float	Scr_FpsOffset;

	static char		Scr_PingDispBuff[32];
	static float	Scr_PingOffset;
	static char		Scr_TimeDispBuff[32];

	Vector4Set (color, colorCyan[0], colorCyan[1], colorCyan[2], scr_hudalpha->value);

	// gloom class display
	if (glm_showclass->integer && cgi.FS_CurrGame ("gloom")) {
		int		x, y;
		vec4_t	hudColor;

		Vector4Set (hudColor, colorRed[0], colorRed[1], colorRed[2], scr_hudalpha->value);

		x = HUD_FTSIZE/3.0;
		y = cg.vidHeight - ((HUD_FTSIZE*7) + FT_SHAOFFSET);

		switch (glmClassType) {
		case GLM_BREEDER:	cgi.Draw_String (NULL, x, y, FS_SHADOW, HUD_SCALE, S_COLOR_RED "Breeder",		hudColor);	break;
		case GLM_HATCHLING:	cgi.Draw_String (NULL, x, y, FS_SHADOW, HUD_SCALE, S_COLOR_RED "Hatchling",		hudColor);	break;
		case GLM_DRONE:		cgi.Draw_String (NULL, x, y, FS_SHADOW, HUD_SCALE, S_COLOR_RED "Drone",			hudColor);	break;
		case GLM_WRAITH:	cgi.Draw_String (NULL, x, y, FS_SHADOW, HUD_SCALE, S_COLOR_RED "Wraith",		hudColor);	break;
		case GLM_KAMIKAZE:	cgi.Draw_String (NULL, x, y, FS_SHADOW, HUD_SCALE, S_COLOR_RED "Kamikaze",		hudColor);	break;
		case GLM_STINGER:	cgi.Draw_String (NULL, x, y, FS_SHADOW, HUD_SCALE, S_COLOR_RED "Stinger",		hudColor);	break;
		case GLM_GUARDIAN:	cgi.Draw_String (NULL, x, y, FS_SHADOW, HUD_SCALE, S_COLOR_RED "Guardian",		hudColor);	break;
		case GLM_STALKER:	cgi.Draw_String (NULL, x, y, FS_SHADOW, HUD_SCALE, S_COLOR_RED "Stalker",		hudColor);	break;

		case GLM_ENGINEER:	cgi.Draw_String (NULL, x, y, FS_SHADOW, HUD_SCALE, S_COLOR_GREEN "Engineer",		hudColor);	break;
		case GLM_GRUNT:		cgi.Draw_String (NULL, x, y, FS_SHADOW, HUD_SCALE, S_COLOR_GREEN "Grunt",			hudColor);	break;
		case GLM_ST:		cgi.Draw_String (NULL, x, y, FS_SHADOW, HUD_SCALE, S_COLOR_GREEN "Shock Trooper",	hudColor);	break;
		case GLM_BIOTECH:	cgi.Draw_String (NULL, x, y, FS_SHADOW, HUD_SCALE, S_COLOR_GREEN "Biotech",			hudColor);	break;
		case GLM_HT:		cgi.Draw_String (NULL, x, y, FS_SHADOW, HUD_SCALE, S_COLOR_GREEN "Heavy Trooper",	hudColor);	break;
		case GLM_COMMANDO:	cgi.Draw_String (NULL, x, y, FS_SHADOW, HUD_SCALE, S_COLOR_GREEN "Commando",		hudColor);	break;
		case GLM_EXTERM:	cgi.Draw_String (NULL, x, y, FS_SHADOW, HUD_SCALE, S_COLOR_GREEN "Exterminator",	hudColor);	break;
		case GLM_MECH:		cgi.Draw_String (NULL, x, y, FS_SHADOW, HUD_SCALE, S_COLOR_GREEN "Mech",			hudColor);	break;

		case GLM_OBSERVER:	cgi.Draw_String (NULL, x, y, FS_SHADOW, HUD_SCALE, S_COLOR_YELLOW "Observer",		hudColor);	break;

		case GLM_DEFAULT:
		default:
			break;
		}
	}

	// framerate
	if (cl_showfps->integer) {
		Scr_FpsOffset = 0;

		if ((cgs.realTime + 1000) < Scr_FpsDelay)
			Scr_FpsDelay = cgs.realTime + 100;

		if (cgs.realTime > Scr_FpsDelay) {
			Q_snprintfz (Scr_FpsDispBuff, sizeof (Scr_FpsDispBuff)," %3.0ffps", 1/cgs.frameTime);
			Scr_FpsDelay = cgs.realTime + 100;
		}

		Scr_FpsOffset = (strlen (Scr_FpsDispBuff) * HUD_FTSIZE);
		cgi.Draw_String (
			NULL,
			cg.vidWidth - 2 - Scr_FpsOffset,
			cg.vidHeight - ((HUD_FTSIZE + FT_SHAOFFSET) * vOffset),
			FS_SHADOW, HUD_SCALE, Scr_FpsDispBuff, color);
	}

	// ping display
	if (cl_showping->integer) {
		Scr_PingOffset = 0;

		Q_snprintfz (Scr_PingDispBuff, sizeof (Scr_PingDispBuff), " %4.1ims", Scr_PingDisplay);
		Scr_PingOffset = (strlen (Scr_PingDispBuff) * HUD_FTSIZE);
		cgi.Draw_String (
			NULL,
			cg.vidWidth - 2 - Scr_PingOffset - Scr_FpsOffset,
			cg.vidHeight - ((HUD_FTSIZE + FT_SHAOFFSET) * vOffset),
			FS_SHADOW, HUD_SCALE, Scr_PingDispBuff, color);
	}

	// map time
	if (cl_showtime->integer) {
		int		time, mins, secs, hour;

		time = cg.time;
		mins = time/60000;
		secs = time/1000;

		while (secs > 59)
			secs -= 60;

		hour = 0;
		while (mins > 59) {
			hour++;
			mins -= 60;
		}

		Q_snprintfz (Scr_TimeDispBuff, sizeof (Scr_TimeDispBuff), "%i:%0.2i:%0.2i", hour, mins, secs);
		cgi.Draw_String (
			NULL,
			cg.vidWidth - 2 - (strlen (Scr_TimeDispBuff) * HUD_FTSIZE) - Scr_PingOffset - Scr_FpsOffset,
			cg.vidHeight - ((HUD_FTSIZE + FT_SHAOFFSET) * vOffset),
			FS_SHADOW, HUD_SCALE, Scr_TimeDispBuff, color);
	}
}

/*
===============================================================================

	CENTER PRINTING

===============================================================================
*/

/*
==============
SCR_ParseCenterPrint

For important messages that stay in the center of the screen temporarily
==============
*/
void SCR_ParseCenterPrint (void) {
	char	*s, line[64], space[64];
	char	*str = cgi.MSG_ReadString ();
	int		i, j, l;
	int		clrCnt;

	strncpy (Scr_CenterString, str, sizeof (Scr_CenterString)-1);
	Scr_CenterTime_Off = scr_centertime->value;
	Scr_CenterTime_Start = cgs.realTime;

	// echo it to the console
	Com_Printf (PRNT_CONSOLE, "\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");

	s = str;
	while (*s) {
		// scan the width of the line
		for (clrCnt=0, l=0 ; l<40 ; l++) {
			if ((s[l] == Q_COLOR_ESCAPE) && (l+1 < 40) && s[l+1] && (l+1 != '\n'))
				clrCnt+=2;
			if (!s[l] || (s[l] == '\n'))
				break;
		}

		for (i=0 ; i<(40 - l)/2 ; i++)
			line[i] = ' ';

		for (j=0 ; j<l ; j++)
			line[i++] = s[j];

		line[i] = '\n';
		line[i+1] = 0;

		for (j=0 ; j<clrCnt/2 ; j++)
			space[j] = ' ';
		space[j] = 0;

		Com_Printf (PRNT_CONSOLE, "%s%s", space, line);

		while (*s && (*s != '\n'))
			s++;

		if (!*s)
			break;

		// skip the \n
		s++;
	}

	Com_Printf (PRNT_CONSOLE, "\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	//cgi.Con_ClearNotify (); // no need to do this anymore, PRNT_CONSOLE skips notify lines
}


/*
=================
SCR_DrawCenterString
=================
*/
static void SCR_DrawCenterString (void) {
	char	*start;
	int		l, len, remaining;
	float	x, y;
	vec4_t	color;

	Scr_CenterTime_Off -= cgs.frameTime;

	if (Scr_CenterTime_Off <= 0)
		return;

	Vector4Set (color, colorWhite[0], colorWhite[1], colorWhite[2], scr_hudalpha->value);

	// the finale prints the characters one at a time
	remaining = 9999;

	start = Scr_CenterString;
	y = cg.vidHeight * 0.25;

	while (*start) {
		// scan the width of the line
		for (len=0, l=0 ; l<40 ; l++, len++) {
			if ((start[l] == Q_COLOR_ESCAPE) && (l+1 < 40) && start[l+1] && (l+1 != '\n'))
				len-=2;
			if (!start[l] || (start[l] == '\n'))
				break;
		}

		x = (cg.vidWidth - len*HUD_FTSIZE) * 0.5;

		if ((remaining -= l) == 0)
			return;

		cgi.Draw_StringLen (NULL, x, y, 0, HUD_SCALE, start, l, color);

		y += HUD_FTSIZE + 1;

		while (*start && (*start != '\n'))
			start++;

		if (!*start)
			break;

		// skip the \n
		start++;
	}
}

/*
=======================================================================

	CVAR/CMD REGISTERING

=======================================================================
*/

/*
==================
SCR_Register
==================
*/
void SCR_Register (void) {
	crosshair			= cgi.Cvar_Get ("crosshair",			"0",			CVAR_ARCHIVE);
	ch_alpha			= cgi.Cvar_Get ("ch_alpha",				"1",			CVAR_ARCHIVE);
	ch_pulse			= cgi.Cvar_Get ("ch_pulse",				"1",			CVAR_ARCHIVE);
	ch_scale			= cgi.Cvar_Get ("ch_scale",				"1",			CVAR_ARCHIVE);
	ch_red				= cgi.Cvar_Get ("ch_red",				"1",			CVAR_ARCHIVE);
	ch_green			= cgi.Cvar_Get ("ch_green",				"1",			CVAR_ARCHIVE);
	ch_blue				= cgi.Cvar_Get ("ch_blue",				"1",			CVAR_ARCHIVE);

	cl_showfps			= cgi.Cvar_Get ("cl_showfps",			"1",			CVAR_ARCHIVE);
	cl_showping			= cgi.Cvar_Get ("cl_showping",			"1",			CVAR_ARCHIVE);
	cl_showtime			= cgi.Cvar_Get ("cl_showtime",			"1",			CVAR_ARCHIVE);

	scr_centertime		= cgi.Cvar_Get ("scr_centertime",		"2.5",			0);
	scr_showpause		= cgi.Cvar_Get ("scr_showpause",		"1",			0);

	// set defaults
	glmClassType = 0;
}

/*
=======================================================================

	RENDERING

=======================================================================
*/

/*
==================
SCR_Draw
==================
*/
void SCR_Draw (void) {
	SCR_DrawCrosshair ();

	HUD_DrawStatusBar ();
	HUD_DrawLayout ();
	Inv_DrawInventory ();

	SCR_DrawCenterString ();
	SCR_DrawLagIcon ();
	SCR_DrawPause ();

	if (cgs.frameTime)
		SCR_DrawInfo ();
}
