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

// cl_screen.c
// - master for refresh, status bar, console, chat, notify, etc

#include "cl_local.h"

byte	cl_DefaultPal[] =
{
	#include "../renderer/defpal.h"
};
float	palRed		(int color) { return (cl_DefaultPal[color*3+0]); }
float	palGreen	(int color) { return (cl_DefaultPal[color*3+1]); }
float	palBlue		(int color) { return (cl_DefaultPal[color*3+2]); }

float	palRedf		(int color) { return ((cl_DefaultPal[color*3+0] > 0) ? cl_DefaultPal[color*3+0] / 255.0 : 0); }
float	palGreenf	(int color) { return ((cl_DefaultPal[color*3+1] > 0) ? cl_DefaultPal[color*3+1] / 255.0 : 0); }
float	palBluef	(int color) { return ((cl_DefaultPal[color*3+2] > 0) ? cl_DefaultPal[color*3+2] / 255.0 : 0); }

qBool			Scr_Initialized;		// ready to draw
vRect_t			Scr_vRect;				// position of render window on screen

int				Scr_DrawLoading;

// center print
char			Scr_CenterString[1024];
float			Scr_CenterTime_Start;
float			Scr_CenterTime_Off;

// information display
int				Scr_PingTime;

unsigned int	Scr_FpsDelay;
char			Scr_FpsDispBuff[32];

char			Scr_PingDispBuff[32];
char			Scr_TimeDispBuff[32];

float			Scr_FpsOffset;
float			Scr_PingOffset;

cvar_t		*crosshair;

cvar_t		*scr_viewsize;
cvar_t		*scr_conspeed;
cvar_t		*scr_centertime;
cvar_t		*scr_showturtle;
cvar_t		*scr_showpause;
cvar_t		*scr_printspeed;

cvar_t		*scr_netgraph;
cvar_t		*scr_timegraph;
cvar_t		*scr_debuggraph;
cvar_t		*scr_graphheight;
cvar_t		*scr_graphscale;
cvar_t		*scr_graphshift;

cvar_t		*scr_demomsg;
cvar_t		*scr_graphalpha;
cvar_t		*scr_hudalpha;

/*
=======================================================================

	GRAPHS

=======================================================================
*/

/*
==============
CL_AddNetgraph

A new packet was just parsed
==============
*/
void CL_AddNetgraph (void) {
	int		i;
	int		in;
	int		ping;

	// if using the debuggraph for something else, don't add the net lines
	if (scr_debuggraph->value || scr_timegraph->value)
		return;

	for (i=0 ; i<cls.netChan.dropped ; i++)
		SCR_DebugGraph (30, 0x40);

	for (i=0 ; i<cl.surpressCount ; i++)
		SCR_DebugGraph (30, 0xdf);

	// see what the latency was on this packet
	in = cls.netChan.incomingAcknowledged & (CMD_BACKUP-1);
	ping = cls.realTime - cl.cmdTime[in];
	Scr_PingTime = clamp (ping, 0, 9990);

	ping /= 30;
	if (ping > 30)
		ping = 30;

	SCR_DebugGraph (ping, 0xd0);
}

typedef struct {
	float	value;
	int		color;
} graphsamp_t;

static	int			current;
static	graphsamp_t	values[1024];


/*
==============
SCR_DebugGraph
==============
*/
void SCR_DebugGraph (float value, int color) {
	values[current&1023].value = value;
	values[current&1023].color = color;
	current++;
}


/*
==============
SCR_DrawDebugGraph
==============
*/
void SCR_DrawDebugGraph (void) {
	int		a, y, i, h;
	float	v;
	vec4_t	color = { colorDkGrey[0], colorDkGrey[1], colorDkGrey[2], scr_graphalpha->value };

	if (!scr_debuggraph->value && !scr_timegraph->value && !scr_netgraph->value)
		return;

	// draw the graph
	y = Scr_vRect.y + Scr_vRect.height;

	Draw_Fill (Scr_vRect.x, y-scr_graphheight->value, Scr_vRect.width, scr_graphheight->value, color);

	for (a=0 ; a<Scr_vRect.width ; a++) {
		i = (current-1-a+1024) & 1023;
		v = values[i].value;
		v = v*scr_graphscale->value + scr_graphshift->value;
		
		if (v < 0)
			v += scr_graphheight->value * (1+(int)(-v/scr_graphheight->value));

		h = (int)v % scr_graphheight->integer;
		VectorSet (color, palRedf (values[i].color), palGreenf (values[i].color), palBluef (values[i].color));
		Draw_Fill (Scr_vRect.x+Scr_vRect.width-1-a, y - h, 1, h, color);
	}
}

// ====================================================================

/*
=================
SCR_DrawCrosshair
=================
*/
void SCR_DrawCrosshair (void) {
	vec4_t	color;
	char	chPic[MAX_QPATH];

	if (!crosshair->integer)
		return;

	if (crosshair->modified) {
		crosshair->modified = qFalse;

		if (crosshair->integer) {
			crosshair->integer = (crosshair->integer < 0) ? 0 : crosshair->integer;

			Com_sprintf (chPic, sizeof (chPic), "ch%d", crosshair->integer);
			clMedia.crosshairImage = Img_RegisterPic (chPic);
		}
	}

	color[3] = ch_alpha->value;
	if (ch_pulse->value)
		color[3] = (0.75*ch_alpha->value) + ((0.25*ch_alpha->value) * sin (AngleMod ((cls.realTime * 0.005) * ch_pulse->value)));

	Vector4Set (color,
				clamp (ch_red->value, 0, 1),
				clamp (ch_green->value, 0, 1),
				clamp (ch_blue->value, 0, 1),
				clamp (color[3], 0, 1));

	Draw_Pic (clMedia.crosshairImage,
		(vidDef.width * 0.5) - (clMedia.crosshairImage->width * ch_scale->value * 0.5),
		(vidDef.height * 0.5) - (clMedia.crosshairImage->height * ch_scale->value * 0.5),
		clMedia.crosshairImage->width * ch_scale->value,
		clMedia.crosshairImage->height * ch_scale->value,
		0, 0, 1, 1,
		color);
}


/*
================
SCR_ShowClass
================
*/
void SCR_ShowClass (void) {
	int		x, y, vOffset = 7;
	vec4_t	hudColor = { colorRed[0], colorRed[1], colorRed[2], scr_hudalpha->value };

	if (!FS_CurrGame ("gloom"))
		return;

	if (!(glm_showclass->integer))
		return;

	x = HUD_FTSIZE/3.0;
	y = vidDef.height - ((HUD_FTSIZE*vOffset) + FT_SHAOFFSET);

	switch (classtype) {
	case GLM_BREEDER:	Draw_String (x, y, 0, HUD_SCALE, S_STYLE_SHADOW S_COLOR_RED "Breeder",	hudColor);		break;
	case GLM_HATCHLING:	Draw_String (x, y, 0, HUD_SCALE, S_STYLE_SHADOW S_COLOR_RED "Hatchling",hudColor);		break;
	case GLM_DRONE:		Draw_String (x, y, 0, HUD_SCALE, S_STYLE_SHADOW S_COLOR_RED "Drone",	hudColor);		break;
	case GLM_WRAITH:	Draw_String (x, y, 0, HUD_SCALE, S_STYLE_SHADOW S_COLOR_RED "Wraith",	hudColor);		break;
	case GLM_KAMIKAZE:	Draw_String (x, y, 0, HUD_SCALE, S_STYLE_SHADOW S_COLOR_RED "Kamikaze",	hudColor);		break;
	case GLM_STINGER:	Draw_String (x, y, 0, HUD_SCALE, S_STYLE_SHADOW S_COLOR_RED "Stinger",	hudColor);		break;
	case GLM_GUARDIAN:	Draw_String (x, y, 0, HUD_SCALE, S_STYLE_SHADOW S_COLOR_RED "Guardian",	hudColor);		break;
	case GLM_STALKER:	Draw_String (x, y, 0, HUD_SCALE, S_STYLE_SHADOW S_COLOR_RED "Stalker",	hudColor);		break;

	case GLM_ENGINEER:	Draw_String (x, y, 0, HUD_SCALE, S_STYLE_SHADOW S_COLOR_GREEN "Engineer",		hudColor);	break;
	case GLM_GRUNT:		Draw_String (x, y, 0, HUD_SCALE, S_STYLE_SHADOW S_COLOR_GREEN "Grunt",			hudColor);	break;
	case GLM_ST:		Draw_String (x, y, 0, HUD_SCALE, S_STYLE_SHADOW S_COLOR_GREEN "Shock Trooper",	hudColor);	break;
	case GLM_BIOTECH:	Draw_String (x, y, 0, HUD_SCALE, S_STYLE_SHADOW S_COLOR_GREEN "Biotech",		hudColor);	break;
	case GLM_HT:		Draw_String (x, y, 0, HUD_SCALE, S_STYLE_SHADOW S_COLOR_GREEN "Heavy Trooper",	hudColor);	break;
	case GLM_COMMANDO:	Draw_String (x, y, 0, HUD_SCALE, S_STYLE_SHADOW S_COLOR_GREEN "Commando",		hudColor);	break;
	case GLM_EXTERM:	Draw_String (x, y, 0, HUD_SCALE, S_STYLE_SHADOW S_COLOR_GREEN "Exterminator",	hudColor);	break;
	case GLM_MECH:		Draw_String (x, y, 0, HUD_SCALE, S_STYLE_SHADOW S_COLOR_GREEN "Mech",			hudColor);	break;

	case GLM_OBSERVER:	Draw_String (x, y, 0, HUD_SCALE, S_STYLE_SHADOW S_COLOR_YELLOW "Observer",		hudColor);	break;

	case GLM_DEFAULT:
	default:
		break;
	}
}


/*
================
SCR_ShowFPS
================
*/
void SCR_ShowFPS (void) {
	int		vOffset = ((Cvar_VariableInteger ("r_speeds"))?(Cvar_VariableInteger ("r_shaders"))?9:8:4);
	vec4_t	color = { colorCyan[0], colorCyan[1], colorCyan[2], scr_hudalpha->value };

	Scr_FpsOffset = 0;
	if (!cl_showfps->integer)
		return;

	if ((cls.realTime + 1000) < Scr_FpsDelay)
		Scr_FpsDelay = cls.realTime + 100;

	if (cls.realTime > Scr_FpsDelay) {
		Com_sprintf (Scr_FpsDispBuff, sizeof (Scr_FpsDispBuff)," %3.0ffps", 1/cls.frameTime);
		Scr_FpsDelay = cls.realTime + 100;
	}

	Scr_FpsOffset = (strlen (Scr_FpsDispBuff) * HUD_FTSIZE);
	Draw_String (
		vidDef.width - 2 - Scr_FpsOffset,
		vidDef.height - ((HUD_FTSIZE + FT_SHAOFFSET) * vOffset),
		FS_SHADOW, HUD_SCALE, Scr_FpsDispBuff, color);
}


/*
================
SCR_ShowPING
================
*/
void SCR_ShowPING (void) {
	int		vOffset = ((Cvar_VariableInteger ("r_speeds"))?(Cvar_VariableInteger ("r_shaders"))?9:8:4);
	vec4_t	color = { colorCyan[0], colorCyan[1], colorCyan[2], scr_hudalpha->value };

	Scr_PingOffset = 0;
	if (!cl_showping->integer)
		return;

	Com_sprintf (Scr_PingDispBuff, sizeof (Scr_PingDispBuff), " %4.0ims", Scr_PingTime);
	Scr_PingOffset = (strlen (Scr_PingDispBuff) * HUD_FTSIZE);
	Draw_String (
		vidDef.width - 2 - Scr_PingOffset - Scr_FpsOffset,
		vidDef.height - ((HUD_FTSIZE + FT_SHAOFFSET) * vOffset),
		FS_SHADOW, HUD_SCALE, Scr_PingDispBuff, color);
}


/*
================
SCR_ShowTIME
================
*/
void SCR_ShowTIME (void) {
	int		time, mins, secs, hour;
	int		vOffset = ((Cvar_VariableInteger ("r_speeds"))?(Cvar_VariableInteger ("r_shaders"))?9:8:4);
	vec4_t	color = { colorCyan[0], colorCyan[1], colorCyan[2], scr_hudalpha->value };

	if (!cl_showtime->integer)
		return;

	time = cl.time;
	mins = time/60000;
	secs = time/1000;

	while (secs > 59)
		secs -= 60;

	hour = 0;
	while (mins > 59) {
		hour++;
		mins -= 60;
	}

	Com_sprintf (Scr_TimeDispBuff, sizeof (Scr_TimeDispBuff), "%i:%0.2i:%0.2i", hour, mins, secs);
	Draw_String (
		vidDef.width - 2 - (strlen (Scr_TimeDispBuff) * HUD_FTSIZE) - Scr_PingOffset - Scr_FpsOffset,
		vidDef.height - ((HUD_FTSIZE + FT_SHAOFFSET) * vOffset),
		FS_SHADOW, HUD_SCALE, Scr_TimeDispBuff, color);
}


/*
==============
SCR_DrawNet
==============
*/
void SCR_DrawNet (void) {
	vec4_t	color = { colorWhite[0], colorWhite[1], colorWhite[2], scr_hudalpha->value };

	if (cls.netChan.outgoingSequence - cls.netChan.incomingAcknowledged < CMD_BACKUP-1)
		return;

	Draw_Pic (
		clMedia.hudNetImage, 64, 0,
		clMedia.hudNetImage->width*HUD_SCALE,
		clMedia.hudNetImage->height*HUD_SCALE,
		0, 0, 1, 1, color);
}


/*
==============
SCR_DrawPause
==============
*/
void SCR_DrawPause (void) {
	vec4_t	color = { colorWhite[0], colorWhite[1], colorWhite[2], scr_hudalpha->value };

	if (!scr_showpause->value)
		return;
	if (!cl_paused->value)
		return;
	if (Key_KeyDest (KD_MENU))
		return;

	Draw_Pic (
		clMedia.hudPausedImage,
		(vidDef.width - (clMedia.hudPausedImage->width*HUD_SCALE))*0.5,
		((vidDef.height*0.5) - (clMedia.hudPausedImage->height*HUD_SCALE)),
		clMedia.hudPausedImage->width*HUD_SCALE, clMedia.hudPausedImage->height*HUD_SCALE,
		0, 0, 1, 1, color);
}


/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading (qBool force) {
	vec4_t		color = { colorWhite[0], colorWhite[1], colorWhite[2], scr_hudalpha->value };

	if (!Scr_DrawLoading && !force)
		return;

	Scr_DrawLoading = 0;

	Draw_Pic (
		clMedia.hudLoadingImage,
		(vidDef.width - (clMedia.hudLoadingImage->width*HUD_SCALE))*0.5,
		((vidDef.height*0.5) - (clMedia.hudLoadingImage->height*HUD_SCALE)),
		clMedia.hudLoadingImage->width*HUD_SCALE, clMedia.hudLoadingImage->height*HUD_SCALE,
		0, 0, 1, 1, color);
}


/*
================
SCR_BeginLoadingPlaque
================
*/
void SCR_BeginLoadingPlaque (void) {
	Snd_StopAllSounds ();
	cl.soundPrepped = qFalse;	// don't play ambients
	CDAudio_Stop ();

	if (cls.disableScreen)
		return;
	if (developer->value)
		return;
	if (CL_ConnectionState (CA_DISCONNECTED))
		return;
	if (Key_KeyDest (KD_CONSOLE))
		return; // if at console, don't bring up the plaque

	if (cl.cinematicTime > 0)
		Scr_DrawLoading = 2;	// clear to black first
	else
		Scr_DrawLoading = 1;

	SCR_UpdateScreen ();
	cls.disableScreen = Sys_Milliseconds ();
	cls.disableServerCount = cl.serverCount;
}


/*
================
SCR_EndLoadingPlaque
================
*/
void SCR_EndLoadingPlaque (void) {
	cls.disableScreen = qFalse;
	Con_ClearNotify ();
}

/*
===============================================================================

	CENTER PRINTING

===============================================================================
*/

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint (char *str) {
	char	*s;
	char	line[64];
	char	space[64];
	int		i, j, l;
	int		clrCnt;

	strncpy (Scr_CenterString, str, sizeof (Scr_CenterString)-1);
	Scr_CenterTime_Off = scr_centertime->value;
	Scr_CenterTime_Start = cls.realTime;

	// echo it to the console
	Com_Printf (PRINT_ALL, "\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");

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

		Com_Printf (PRINT_ALL, "%s%s", space, line);

		while (*s && (*s != '\n'))
			s++;

		if (!*s)
			break;

		// skip the \n
		s++;
	}

	Com_Printf (PRINT_ALL, "\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	Con_ClearNotify ();
}


/*
=================
SCR_DrawCenterString
=================
*/
void SCR_DrawCenterString (void) {
	char	*start;
	int		l, len, remaining;
	float	x, y;
	vec4_t	color = { colorWhite[0], colorWhite[1], colorWhite[2], scr_hudalpha->value };

	// the finale prints the characters one at a time
	remaining = 9999;

	start = Scr_CenterString;
	y = vidDef.height * 0.25;

	while (*start) {
		// scan the width of the line
		for (len=0, l=0 ; l<40 ; l++, len++) {
			if ((start[l] == Q_COLOR_ESCAPE) && (l+1 < 40) && start[l+1] && (l+1 != '\n'))
				len-=2;
			if (!start[l] || (start[l] == '\n'))
				break;
		}

		x = (vidDef.width - len*HUD_FTSIZE) * 0.5;

		if ((remaining -= l) == 0)
			return;

		Draw_StringLen (x, y, 0, HUD_SCALE, start, l, color);

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
=================
SCR_CheckDrawCenterString
=================
*/
void SCR_CheckDrawCenterString (void) {
	Scr_CenterTime_Off -= cls.frameTime;

	if (Scr_CenterTime_Off <= 0)
		return;

	SCR_DrawCenterString ();
}

/*
=======================================================================

	SCREEN SIZE

=======================================================================
*/

/*
=================
SCR_CalcVrect

Sets Scr_vRect, the coordinates of the rendered window
=================
*/
static void SCR_CalcVrect (void) {
	int		size;

	size = scr_viewsize->integer;

	Scr_vRect.width = vidDef.width*size/100;
	Scr_vRect.width &= ~7;

	Scr_vRect.height = vidDef.height*size/100;
	Scr_vRect.height &= ~1;

	Scr_vRect.x = (vidDef.width - Scr_vRect.width)/2;
	Scr_vRect.y = (vidDef.height - Scr_vRect.height)/2;
}


/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f (void) {
	Cvar_SetValue ("viewsize", scr_viewsize->integer + 10);
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
void SCR_SizeDown_f (void) {
	Cvar_SetValue ("viewsize", scr_viewsize->integer - 10);
}


/*
==============
SCR_TileClear

Clear any parts of the tiled background that were drawn on last frame
==============
*/
void SCR_TileRect (int x, int y, int w, int h) {
	if ((w == 0) || (h == 0))
		return;	// prevents div by zero (should never happen)

	if (clMedia.tileBackImage)
		Draw_Pic (clMedia.tileBackImage, x, y, w, h, x/64.0, y/64.0, (x+w)/64.0, (y+h)/64.0, colorWhite);
	else
		Draw_Fill (x, y, w, h, colorBlack);
}
void SCR_TileClear (void) {
	int		top, bottom, left, right;

	if (scr_viewsize->integer >= 100)
		return;		// full screen rendering
	if (cl.cinematicTime > 0)
		return;		// full screen cinematic

	top = Scr_vRect.y;
	bottom = top + Scr_vRect.height-1;
	left = Scr_vRect.x;
	right = left + Scr_vRect.width-1;

	// clear above view screen
	SCR_TileRect (0, 0, vidDef.width, top);

	// clear below view screen
	SCR_TileRect (0, bottom, vidDef.width, vidDef.height - bottom);

	// clear left of view screen
	SCR_TileRect (0, top, left, bottom - top + 1);

	// clear right of view screen
	SCR_TileRect (right, top, vidDef.width - right, bottom - top + 1);
}

/*
=======================================================================

	INIT / SHUTDOWN

=======================================================================
*/

/*
==================
SCR_Register
==================
*/
void SCR_Register (void) {
	crosshair		= Cvar_Get ("crosshair",		"0",		CVAR_ARCHIVE);

	scr_viewsize	= Cvar_Get ("viewsize",			"100",		CVAR_ARCHIVE);
	scr_conspeed	= Cvar_Get ("scr_conspeed",		"3",		0);
	scr_showturtle	= Cvar_Get ("scr_showturtle",	"0",		0);
	scr_showpause	= Cvar_Get ("scr_showpause",	"1",		0);
	scr_centertime	= Cvar_Get ("scr_centertime",	"2.5",		0);
	scr_printspeed	= Cvar_Get ("scr_printspeed",	"8",		0);
	scr_netgraph	= Cvar_Get ("netgraph",			"0",		0);
	scr_timegraph	= Cvar_Get ("timegraph",		"0",		0);
	scr_debuggraph	= Cvar_Get ("debuggraph",		"0",		0);
	scr_graphheight	= Cvar_Get ("graphheight",		"32",		0);
	scr_graphscale	= Cvar_Get ("graphscale",		"1",		0);
	scr_graphshift	= Cvar_Get ("graphshift",		"0",		0);

	scr_demomsg		= Cvar_Get ("scr_demomsg",		"1",		CVAR_ARCHIVE);
	scr_graphalpha	= Cvar_Get ("scr_graphalpha",	"0.6",		CVAR_ARCHIVE);
	scr_hudalpha	= Cvar_Get ("scr_hudalpha",		"1",		CVAR_ARCHIVE);

	Cmd_AddCommand ("benchmark",	SCR_Benchmark_f);
	Cmd_AddCommand ("timerefresh",	SCR_TimeRefresh_f);
	Cmd_AddCommand ("loading",		SCR_BeginLoadingPlaque);
	Cmd_AddCommand ("sizeup",		SCR_SizeUp_f);
	Cmd_AddCommand ("sizedown",		SCR_SizeDown_f);
}


/*
==================
SCR_Unregister
==================
*/
void SCR_Unregister (void) {
	Cmd_RemoveCommand ("benchmark");
	Cmd_RemoveCommand ("timerefresh");
	Cmd_RemoveCommand ("loading");
	Cmd_RemoveCommand ("sizeup");
	Cmd_RemoveCommand ("sizedown");
}


/*
==================
SCR_Init
==================
*/
void SCR_Init (void) {
	SCR_Register ();

	Scr_CenterString[0] = 0;
	Scr_CenterTime_Start = 0;
	Scr_CenterTime_Off = 0;

	Scr_PingTime = 0;

	Scr_FpsDelay = 0;
	Scr_FpsDispBuff[0] = 0;

	Scr_PingDispBuff[0] = 0;
	Scr_TimeDispBuff[0] = 0;

	Scr_FpsOffset = 0;
	Scr_PingOffset = 0;

	Scr_Initialized = qTrue;
}


/*
==================
SCR_Shutdown
==================
*/
void SCR_Shutdown (void) {
	SCR_Unregister ();

	Scr_Initialized = qFalse;
}

/*
=======================================================================

	FRAMERATE BENCHMARKING

=======================================================================
*/

/*
================
SCR_Benchmark_f
================
*/
void SCR_Benchmark_f (void) {
	int		i, j, times;
	int		start, stop;
	float	time, timeinc;
	float	result;

	if (!CL_ConnectionState (CA_ACTIVE))
		return;

	if (Cmd_Argc () >= 2)
		times = atoi (Cmd_Argv (1));
	else
		times = 50;

	for (j=0, result=0, timeinc=0 ; j<times ; j++) {
		start = Sys_Milliseconds ();

		if (Cmd_Argc () == 3) {
			// run without page flipping
			R_BeginFrame (0);
			for (i=0 ; i<128 ; i++) {
				cl.refDef.viewAngles[1] = i/128.0*360.0;
				AnglesToAxis (cl.refDef.viewAngles, cl.refDef.viewAxis);

				R_RenderFrame (&cl.refDef);
			}
			R_EndFrame ();
		} else {
			for (i=0 ; i<128 ; i++) {
				cl.refDef.viewAngles[1] = i/128.0*360.0;
				AnglesToAxis (cl.refDef.viewAngles, cl.refDef.viewAxis);

				R_BeginFrame (0);
				R_RenderFrame (&cl.refDef);
				R_EndFrame ();
			}
		}

		stop = Sys_Milliseconds ();
		time = (stop - start) / 1000.0;
		timeinc += time;
		result += 128.0 / time;
	}

	Com_Printf (PRINT_ALL, "%f seconds, %f average seconds (%f fps)\n", timeinc, timeinc/times, result/times);
}


/*
================
SCR_TimeRefresh_f
================
*/
void SCR_TimeRefresh_f (void) {
	int		i;
	int		start, stop;
	float	time;

	if (!CL_ConnectionState (CA_ACTIVE))
		return;

	start = Sys_Milliseconds ();

	if (Cmd_Argc () == 2) {
		// run without page flipping
		R_BeginFrame (0);
		for (i=0 ; i<128 ; i++) {
			cl.refDef.viewAngles[1] = i/128.0*360.0;
			AnglesToAxis (cl.refDef.viewAngles, cl.refDef.viewAxis);

			R_RenderFrame (&cl.refDef);
		}
		R_EndFrame ();
	} else {
		for (i=0 ; i<128 ; i++) {
			cl.refDef.viewAngles[1] = i/128.0*360.0;
			AnglesToAxis (cl.refDef.viewAngles, cl.refDef.viewAxis);

			R_BeginFrame (0);
			R_RenderFrame (&cl.refDef);
			R_EndFrame ();
		}
	}

	stop = Sys_Milliseconds ();
	time = (stop - start) / 1000.0;
	Com_Printf (PRINT_ALL, "%f seconds (%f fps)\n", time, 128/time);
}

/*
=======================================================================

	DRAW IT

=======================================================================
*/

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void SCR_UpdateScreen (void) {
	int		i, numFrames;
	float	separation[2];

	// if the screen is disabled (loading plaque is up, or vid mode changing) do nothing at all
	if (cls.disableScreen) {
		if ((Sys_Milliseconds () - cls.disableScreen) > 120000) {
			cls.disableScreen = 0;
			Com_Printf (PRINT_ALL, "Loading plaque timed out.\n");
		}

		return;
	}

	if (!Scr_Initialized || !con.initialized)
		return;		// not initialized yet

	if (cl_stereo->integer) {
		numFrames = 2;

		// range check cl_camera_separation so we don't inadvertently fry someone's brain
		cl_stereo_separation->value = clamp (cl_stereo_separation->value, 0.0, 1.0);

		separation[0] = -cl_stereo_separation->value * 0.5;
		separation[1] = cl_stereo_separation->value * 0.5;
	} else {
		numFrames = 1;
		separation[0] = separation[1] = 0;
	}

	for (i=0 ; i<numFrames ; i++) {
		R_BeginFrame (separation[i]);

		if (Scr_DrawLoading == 2) {
			R_SetPalette (NULL);
			SCR_DrawLoading (qTrue);
		} else if (cl.cinematicTime > 0) {
			if (Key_KeyDest (KD_MENU)) {
				if (cl.cinematicPaletteActive) {
					R_SetPalette (NULL);
					cl.cinematicPaletteActive = qFalse;
				}

				CL_UIModule_Refresh (qFalse);
			} else if (Key_KeyDest (KD_CONSOLE)) {
				if (cl.cinematicPaletteActive) {
					R_SetPalette (NULL);
					cl.cinematicPaletteActive = qFalse;
				}

				Con_DrawConsole ();
			} else
				CIN_DrawCinematic ();
		} else {
			// make sure the game palette is active
			if (cl.cinematicPaletteActive) {
				R_SetPalette (NULL);
				cl.cinematicPaletteActive = qFalse;
			}

			// do 3D refresh drawing, and then update the screen
			SCR_CalcVrect ();
			SCR_TileClear ();

			View_RenderView (Scr_vRect, separation[i]);

			SCR_DrawCrosshair ();
			HUD_DrawLayout ();

			SCR_DrawNet ();
			SCR_CheckDrawCenterString ();

			if (scr_timegraph->value)
				SCR_DebugGraph (cls.frameTime*300, 0);
			SCR_DrawDebugGraph ();

			SCR_DrawPause ();
			Con_DrawConsole ();
			SCR_DrawLoading (qFalse);

			if (!CL_ConnectionState (CA_DISCONNECTED) && !CL_ConnectionState (CA_CONNECTING) && !CL_ConnectionState (CA_CONNECTED) && cl.refreshPrepped) {
				SCR_ShowClass ();

				SCR_ShowFPS ();
				SCR_ShowPING ();
				SCR_ShowTIME ();
			}

			if (scr_demomsg->integer && cl.attractLoop && (!((cl.cinematicTime > 0) && ((cls.realTime - cl.cinematicTime) > 1000))))
				CL_UIModule_DrawStatusBar ("Demo Playing");

			CL_UIModule_Refresh (CL_ConnectionState (CA_DISCONNECTED));
		}

		R_EndFrame ();
	}
}
