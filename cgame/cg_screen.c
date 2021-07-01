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
static char		scr_centerString[1024];
static float	scr_centerTime_Off;

/*
=============
CG_RegisterPic
=============
*/
struct shader_s *CG_RegisterPic (char *name)
{
	struct shader_s	*shader;
	char			fullName[MAX_QPATH];

	if (!name)
		return NULL;
	if (!name[0])
		return NULL;

	if ((name[0] != '/') && (name[0] != '\\')) {
		Q_snprintfz (fullName, sizeof (fullName), "pics/%s.tga", name);
		shader = cgi.R_RegisterPic (fullName);
	}
	else {
		Q_strncpyz (fullName, name+1, sizeof (fullName));
		shader = cgi.R_RegisterPic (fullName);
	}

	return shader;
}

/*
=============================================================================

	PALETTE COLORING

	Fix by removing the need for these, by using actual RGB
	colors and ditch the stupid shit palette
=============================================================================
*/

static byte cg_colorPal[] = {
	#include "colorpal.h"
};
float	palRed		(int index) { return (cg_colorPal[index*3+0]); }
float	palGreen	(int index) { return (cg_colorPal[index*3+1]); }
float	palBlue		(int index) { return (cg_colorPal[index*3+2]); }

float	palRedf		(int index) { return ((cg_colorPal[index*3+0] > 0) ? cg_colorPal[index*3+0] * (1.0f/255.0f) : 0); }
float	palGreenf	(int index) { return ((cg_colorPal[index*3+1] > 0) ? cg_colorPal[index*3+1] * (1.0f/255.0f) : 0); }
float	palBluef	(int index) { return ((cg_colorPal[index*3+2] > 0) ? cg_colorPal[index*3+2] * (1.0f/255.0f) : 0); }

/*
=============================================================================

	GRAPHS

=============================================================================
*/

typedef struct graphStamp_s {
	float	value;
	int		color;
} graphStamp_t;

static int			scr_graphCurrent;
static graphStamp_t	scr_graphValues[1024];
#define GRAPHMASK	(1024-1)

/*
=============
CG_AddNetgraph
=============
*/
void CG_AddNetgraph (void)
{
	int		i, in, ping;

	// If using the debuggraph for something else, don't add the net lines
	if (scr_debuggraph->value || scr_timegraph->value)
		return;

	for (i=0 ; i<cgi.NET_GetPacketDropCount () ; i++)
		CG_DebugGraph (30, 0x40);

	for (i=0 ; i<cgi.NET_GetRateDropCount () ; i++)
		CG_DebugGraph (30, 0xdf);

	// See what the latency was on this packet
	cgi.NET_GetSequenceState (NULL, &in);
	ping = cg.realTime - cgi.NET_GetUserCmdTime (in);

	ping /= 30;
	if (ping > 30)
		ping = 30;

	CG_DebugGraph (ping, 0xd0);
}


/*
=============
CG_DebugGraph
=============
*/
void CG_DebugGraph (float value, int color)
{
	scr_graphValues[scr_graphCurrent & GRAPHMASK].value = value;
	scr_graphValues[scr_graphCurrent & GRAPHMASK].color = color;
	scr_graphCurrent++;
}


/*
=============
CG_DrawDebugGraph
=============
*/
void CG_DrawDebugGraph (void)
{
	int		a, i, h;
	float	v;
	vec4_t	color;

	if (!scr_debuggraph->value && !scr_timegraph->value && !scr_netgraph->value)
		return;

	// Draw the graph
	Vector4Set (color, colorDkGrey[0], colorDkGrey[1], colorDkGrey[2], scr_graphalpha->value);
	cgi.R_DrawFill (0, cg.glConfig.vidHeight-scr_graphheight->value, cg.glConfig.vidWidth, scr_graphheight->value, color);

	for (a=0 ; a<cg.glConfig.vidWidth ; a++) {
		i = (scr_graphCurrent-1-a+1024) & GRAPHMASK;
		v = scr_graphValues[i].value;
		v = v*scr_graphscale->value + scr_graphshift->value;
		
		if (v < 0)
			v += scr_graphheight->value * (1+(int)(-v/scr_graphheight->value));

		h = (int)v % scr_graphheight->integer;
		VectorSet (color, palRedf (scr_graphValues[i].color), palGreenf (scr_graphValues[i].color), palBluef (scr_graphValues[i].color));
		cgi.R_DrawFill (cg.glConfig.vidWidth-1-a, cg.glConfig.vidHeight-h, 1, h, color);
	}
}

/*
=============================================================================

	SCREEN EFFECTS

=============================================================================
*/

/*
=================
SCR_DrawGoggles
=================
*/
static void SCR_DrawGoggles (void)
{
//	static float alienIRColor[] = {0.5f, 0.5f, 1, 1};

	if (!(cg.refDef.rdFlags & RDF_IRGOGGLES))
		return;
	if (!cg_advInfrared->integer)
		return;

	// Pick a goggle type
	if (cg.currGameMod == GAME_MOD_GLOOM) {
		switch (cg.gloomClassType) {
			default:
			case GLM_DEFAULT:
			case GLM_OBSERVER:
				break;

			case GLM_ENGINEER:
			case GLM_GRUNT:
			case GLM_ST:
			case GLM_BIOTECH:
			case GLM_HT:
			case GLM_COMMANDO:
			case GLM_EXTERM:
			case GLM_MECH:
				if (cgMedia.infraredGoggles)
					cgi.R_DrawPic (cgMedia.infraredGoggles, 0, 0, 0, cg.refDef.width, cg.refDef.height, 0, 0, 1, 1, colorWhite);
				break;

			case GLM_BREEDER:
			case GLM_HATCHLING:
			case GLM_DRONE:
			case GLM_WRAITH:
			case GLM_KAMIKAZE:
			case GLM_STINGER:
			case GLM_GUARDIAN:
			case GLM_STALKER:
			//	if (cgMedia.alienInfraredVision)
			//		cgi.R_DrawPic (cgMedia.alienInfraredVision, 0, 0, cg.refDef.width, cg.refDef.height, 0, 0, 1, 1, alienIRColor);
				break;
		}
	}
	else {
		if (cgMedia.infraredGoggles)
			cgi.R_DrawPic (cgMedia.infraredGoggles, 0, 0, 0, cg.refDef.width, cg.refDef.height, 0, 0, 1, 1, colorWhite);
	}
}

/*
=============================================================================

	SCREEN ICONS

=============================================================================
*/

/*
=================
SCR_DrawCrosshair
=================
*/
static void SCR_DrawCrosshair (void)
{
	vec4_t	color;
	int		width, height;

	if (!crosshair->integer)
		return;

	if (crosshair->modified)
		CG_CrosshairShaderInit ();

	if (!cgMedia.crosshairShader)
		return;

	color[3] = ch_alpha->value;
	if (ch_pulse->value)
		color[3] = (0.75*ch_alpha->value) + ((0.25*ch_alpha->value) * sin (AngleModf ((cg.realTime * 0.005) * ch_pulse->value)));

	Vector4Set (color,
				clamp (ch_red->value, 0, 1),
				clamp (ch_green->value, 0, 1),
				clamp (ch_blue->value, 0, 1),
				clamp (color[3], 0, 1));

	cgi.R_GetImageSize (cgMedia.crosshairShader, &width, &height);
	cgi.R_DrawPic (cgMedia.crosshairShader, 0,
		(cg.glConfig.vidWidth * 0.5) - (width * ch_scale->value * 0.5),
		(cg.glConfig.vidHeight * 0.5) - (height * ch_scale->value * 0.5),
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
static void SCR_DrawLagIcon (void)
{
	vec4_t	color;
	int		outgoingSequence;
	int		incomingAcknowledged;
	int		width, height;

	cgi.NET_GetSequenceState (&outgoingSequence, &incomingAcknowledged);
	if (outgoingSequence - incomingAcknowledged < CMD_MASK)
		return;

	Vector4Set (color, colorWhite[0], colorWhite[1], colorWhite[2], scr_hudalpha->value);
	cgi.R_GetImageSize (cgMedia.hudNetShader, &width, &height);
	cgi.R_DrawPic (
		cgMedia.hudNetShader, 0, 64, 0,
		width*HUD_SCALE, height*HUD_SCALE,
		0, 0, 1, 1, color);
}


/*
==============
SCR_DrawPause
==============
*/
static void SCR_DrawPause (void)
{
	vec4_t	color;
	int		width, height;

	if (!scr_showpause->value)
		return;
	if (!cgi.Cvar_GetIntegerValue ("paused"))
		return;
	if (cgi.Key_GetDest () == KD_MENU || cg.menuOpen)
		return;

	Vector4Set (color, colorWhite[0], colorWhite[1], colorWhite[2], scr_hudalpha->value);
	cgi.R_GetImageSize (cgMedia.hudPausedShader, &width, &height);
	cgi.R_DrawPic (
		cgMedia.hudPausedShader, 0,
		(cg.glConfig.vidWidth - (width*HUD_SCALE))*0.5,
		((cg.glConfig.vidHeight*0.5) - (height*HUD_SCALE)),
		width*HUD_SCALE, height*HUD_SCALE,
		0, 0, 1, 1, color);
}

/*
=============================================================================

	DISPLAY INFORMATION

=============================================================================
*/

// Information display
#define MAX_PING_AVERAGE 10
static int		scr_pingValues[MAX_PING_AVERAGE];
static int		scr_pingNum;
static int		scr_pingDisplay;

/*
================
SCR_UpdatePING
================
*/
void SCR_UpdatePING (void)
{
	int		incAck;

	// Calculate and store ping
	cgi.NET_GetSequenceState (NULL, &incAck);
	scr_pingValues[scr_pingNum] = cg.realTime - cgi.NET_GetUserCmdTime (incAck & CMD_MASK);
	scr_pingNum++;

	if (scr_pingNum == MAX_PING_AVERAGE-1) {
		// Average out for the display value
		scr_pingDisplay = 0;
		scr_pingNum = 0;
		for (incAck=0 ; incAck<MAX_PING_AVERAGE ; incAck++) {
			scr_pingDisplay += scr_pingValues[incAck];
			scr_pingValues[incAck] = 0;
		}
		scr_pingDisplay /= (float)MAX_PING_AVERAGE;
	}
}


/*
================
SCR_DrawInfo
================
*/
static void SCR_DrawInfo (void)
{
	int		vOffset = 4;
	vec4_t	color;

	static int		scr_fpsDelay;
	static char		scr_fpsDispBuff[32];
	static float	scr_fpsLast;
	static float	scr_fpsOffset;

	static char		scr_pingDispBuff[32];
	static float	scr_pingOffset;
	static char		scr_timeDispBuff[32];

	Vector4Set (color, colorCyan[0], colorCyan[1], colorCyan[2], scr_hudalpha->value);

	// Gloom class display
	if (glm_showclass->integer && cg.currGameMod == GAME_MOD_GLOOM) {
		int		x, y;
		vec4_t	hudColor;

		Vector4Set (hudColor, colorRed[0], colorRed[1], colorRed[2], scr_hudalpha->value);

		x = HUD_FTSIZE/3.0;
		y = cg.glConfig.vidHeight - ((HUD_FTSIZE*7) + FT_SHAOFFSET);

		switch (cg.gloomClassType) {
		case GLM_BREEDER:	CG_DrawString (x, y, FS_SHADOW, HUD_SCALE, S_COLOR_RED "Breeder",			hudColor);	break;
		case GLM_HATCHLING:	CG_DrawString (x, y, FS_SHADOW, HUD_SCALE, S_COLOR_RED "Hatchling",			hudColor);	break;
		case GLM_DRONE:		CG_DrawString (x, y, FS_SHADOW, HUD_SCALE, S_COLOR_RED "Drone",				hudColor);	break;
		case GLM_WRAITH:	CG_DrawString (x, y, FS_SHADOW, HUD_SCALE, S_COLOR_RED "Wraith",			hudColor);	break;
		case GLM_KAMIKAZE:	CG_DrawString (x, y, FS_SHADOW, HUD_SCALE, S_COLOR_RED "Kamikaze",			hudColor);	break;
		case GLM_STINGER:	CG_DrawString (x, y, FS_SHADOW, HUD_SCALE, S_COLOR_RED "Stinger",			hudColor);	break;
		case GLM_GUARDIAN:	CG_DrawString (x, y, FS_SHADOW, HUD_SCALE, S_COLOR_RED "Guardian",			hudColor);	break;
		case GLM_STALKER:	CG_DrawString (x, y, FS_SHADOW, HUD_SCALE, S_COLOR_RED "Stalker",			hudColor);	break;

		case GLM_ENGINEER:	CG_DrawString (x, y, FS_SHADOW, HUD_SCALE, S_COLOR_GREEN "Engineer",		hudColor);	break;
		case GLM_GRUNT:		CG_DrawString (x, y, FS_SHADOW, HUD_SCALE, S_COLOR_GREEN "Grunt",			hudColor);	break;
		case GLM_ST:		CG_DrawString (x, y, FS_SHADOW, HUD_SCALE, S_COLOR_GREEN "Shock Trooper",	hudColor);	break;
		case GLM_BIOTECH:	CG_DrawString (x, y, FS_SHADOW, HUD_SCALE, S_COLOR_GREEN "Biotech",			hudColor);	break;
		case GLM_HT:		CG_DrawString (x, y, FS_SHADOW, HUD_SCALE, S_COLOR_GREEN "Heavy Trooper",	hudColor);	break;
		case GLM_COMMANDO:	CG_DrawString (x, y, FS_SHADOW, HUD_SCALE, S_COLOR_GREEN "Commando",		hudColor);	break;
		case GLM_EXTERM:	CG_DrawString (x, y, FS_SHADOW, HUD_SCALE, S_COLOR_GREEN "Exterminator",	hudColor);	break;
		case GLM_MECH:		CG_DrawString (x, y, FS_SHADOW, HUD_SCALE, S_COLOR_GREEN "Mech",			hudColor);	break;

		case GLM_OBSERVER:	CG_DrawString (x, y, FS_SHADOW, HUD_SCALE, S_COLOR_YELLOW "Observer",		hudColor);	break;

		case GLM_DEFAULT:
		default:
			break;
		}
	}

	// Framerate
	if (cl_showfps->integer) {
		scr_fpsOffset = 0;

		if (cg.realTime + 1000 < scr_fpsDelay)
			scr_fpsDelay = cg.realTime + 100;

		if (cg.realTime > scr_fpsDelay) {
			Q_snprintfz (scr_fpsDispBuff, sizeof (scr_fpsDispBuff)," %3.0ffps", (scr_fpsLast + (1.0f/cg.refreshFrameTime)) * 0.5f);
			scr_fpsLast = 1.0f/cg.refreshFrameTime;
			scr_fpsDelay = cg.realTime + 100;
		}

		scr_fpsOffset = (strlen (scr_fpsDispBuff) * HUD_FTSIZE);
		CG_DrawString (
			cg.glConfig.vidWidth - 2 - scr_fpsOffset,
			cg.glConfig.vidHeight - ((HUD_FTSIZE + FT_SHAOFFSET) * vOffset),
			FS_SHADOW, HUD_SCALE, scr_fpsDispBuff, color);
	}

	// Ping display
	if (cl_showping->integer) {
		scr_pingOffset = 0;

		Q_snprintfz (scr_pingDispBuff, sizeof (scr_pingDispBuff), " %4.1ims", scr_pingDisplay);
		scr_pingOffset = (strlen (scr_pingDispBuff) * HUD_FTSIZE);
		CG_DrawString (
			cg.glConfig.vidWidth - 2 - scr_pingOffset - scr_fpsOffset,
			cg.glConfig.vidHeight - ((HUD_FTSIZE + FT_SHAOFFSET) * vOffset),
			FS_SHADOW, HUD_SCALE, scr_pingDispBuff, color);
	}

	// Map time
	if (cl_showtime->integer) {
		int		time, mins, secs, hour;

		time = cg.netTime;
		mins = time/60000;
		secs = time/1000;

		while (secs > 59)
			secs -= 60;

		hour = 0;
		while (mins > 59) {
			hour++;
			mins -= 60;
		}

		Q_snprintfz (scr_timeDispBuff, sizeof (scr_timeDispBuff), "%i:%0.2i:%0.2i", hour, mins, secs);
		CG_DrawString (
			cg.glConfig.vidWidth - 2 - (strlen (scr_timeDispBuff) * HUD_FTSIZE) - scr_pingOffset - scr_fpsOffset,
			cg.glConfig.vidHeight - ((HUD_FTSIZE + FT_SHAOFFSET) * vOffset),
			FS_SHADOW, HUD_SCALE, scr_timeDispBuff, color);
	}
}

/*
=============================================================================

	CENTER PRINTING

=============================================================================
*/

/*
==============
SCR_ParseCenterPrint

For important messages that stay in the center of the screen temporarily
==============
*/
void SCR_ParseCenterPrint (void)
{
	char	*s, line[64], space[64];
	char	*str = cgi.MSG_ReadString ();
	int		i, j, l;
	int		clrCnt;

	Q_strncpyz (scr_centerString, str, sizeof (scr_centerString));
	scr_centerTime_Off = scr_centertime->value;

	// echo it to the console
	Com_Printf (PRNT_CONSOLE, "\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");

	s = str;
	while (*s) {
		// scan the width of the line
		for (clrCnt=0, l=0 ; l<40 ; l++) {
			if (s[l] == Q_COLOR_ESCAPE && l+1 < 40 && s[l+1] && l+1 != '\n')
				clrCnt+=2;
			if (!s[l] || s[l] == '\n')
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

		while (*s && *s != '\n')
			s++;

		if (!*s)
			break;

		// skip the \n
		s++;
	}

	Com_Printf (PRNT_CONSOLE, "\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
}


/*
=================
SCR_DrawCenterString
=================
*/
static void SCR_DrawCenterString (void)
{
	char	*start;
	int		l, len, remaining;
	float	x, y;
	vec4_t	color;

	scr_centerTime_Off -= cg.refreshFrameTime;
	if (scr_centerTime_Off <= 0)
		return;

	Vector4Set (color, colorWhite[0], colorWhite[1], colorWhite[2], scr_hudalpha->value);

	// The finale prints the characters one at a time
	remaining = 9999;

	start = scr_centerString;
	y = cg.glConfig.vidHeight * 0.25f;
	while (*start) {
		// Scan the width of the line
		for (len=0, l=0 ; l<40 ; l++, len++) {
			if (start[l] == Q_COLOR_ESCAPE && l+1 < 40 && start[l+1] && l+1 != '\n')
				len-=2;
			if (!start[l] || start[l] == '\n')
				break;
		}

		x = (cg.glConfig.vidWidth - len*HUD_FTSIZE) * 0.5f;

		if ((remaining -= l) == 0)
			return;

		CG_DrawStringLen (x, y, FS_SHADOW, HUD_SCALE, start, l, color);

		y += HUD_FTSIZE + 1;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;

		// Skip the \n
		start++;
	}
}

/*
=============================================================================

	RENDERING

=============================================================================
*/

/*
==================
SCR_Draw
==================
*/
void SCR_Draw (void)
{
	// Draw view blend
	if (gl_polyblend->integer && cg.viewBlend[3] > 0.01f)
		cgi.R_DrawFill (0, 0, cg.refDef.width, cg.refDef.height, cg.viewBlend);

	// Gloom IR goggles
	SCR_DrawGoggles ();

	// Crosshair
	SCR_DrawCrosshair ();

	// HUD
	HUD_DrawStatusBar ();
	HUD_DrawLayout ();
	Inv_DrawInventory ();

	// Center screen print, lag icon, pause icon
	SCR_DrawCenterString ();
	SCR_DrawLagIcon ();
	SCR_DrawPause ();

	// Ping, time, class
	if (cg.refreshFrameTime)
		SCR_DrawInfo ();

	// Draw the debug graph
	if (scr_timegraph->value)
		CG_DebugGraph (cg.netFrameTime*300, 0);
	CG_DrawDebugGraph ();

	// Render the menu
	UI_Refresh (qFalse);
}
