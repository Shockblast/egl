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
// cg_main.c
//

#include "cg_local.h"

cgState_t	cg;
cgMedia_t	cgMedia;
uiMedia_t	uiMedia;

cVar_t	*cg_advInfrared;
cVar_t	*cg_mapEffects;

cVar_t	*cg_thirdPerson;
cVar_t	*cg_thirdPersonAngle;
cVar_t	*cg_thirdPersonClip;
cVar_t	*cg_thirdPersonDist;

cVar_t	*cg_decals;
cVar_t	*cg_decalBurnLife;
cVar_t	*cg_decalLife;
cVar_t	*cg_decalLOD;
cVar_t	*cg_decalMax;

cVar_t	*cl_explorattle;
cVar_t	*cl_explorattle_scale;
cVar_t	*cl_footsteps;
cVar_t	*cl_gun;
cVar_t	*cl_noskins;
cVar_t	*cl_predict;
cVar_t	*cl_showmiss;
cVar_t	*cl_vwep;

cVar_t	*gender_auto;
cVar_t	*gender;
cVar_t	*hand;
cVar_t	*skin;

cVar_t	*glm_advgas;
cVar_t	*glm_advstingfire;
cVar_t	*glm_blobtype;
cVar_t	*glm_bluestingfire;
cVar_t	*glm_flashpred;
cVar_t	*glm_flwhite;
cVar_t	*glm_forcecache;
cVar_t	*glm_jumppred;
cVar_t	*glm_showclass;

cVar_t	*cl_add_particles;
cVar_t	*cg_particleCulling;
cVar_t	*cg_particleGore;
cVar_t	*cg_particleShading;
cVar_t	*cg_particleSmokeLinger;

cVar_t	*cg_railCoreRed;
cVar_t	*cg_railCoreGreen;
cVar_t	*cg_railCoreBlue;
cVar_t	*cg_railSpiral;
cVar_t	*cg_railSpiralRed;
cVar_t	*cg_railSpiralGreen;
cVar_t	*cg_railSpiralBlue;

cVar_t	*cl_testblend;
cVar_t	*cl_testentities;
cVar_t	*cl_testlights;
cVar_t	*cl_testparticles;

cVar_t	*r_hudscale;
cVar_t	*r_fontscale;

cVar_t	*crosshair;
cVar_t	*ch_alpha;
cVar_t	*ch_pulse;
cVar_t	*ch_scale;
cVar_t	*ch_red;
cVar_t	*ch_green;
cVar_t	*ch_blue;

cVar_t	*cl_showfps;
cVar_t	*cl_showping;
cVar_t	*cl_showtime;

cVar_t	*con_chatHud;
cVar_t	*con_chatHudLines;
cVar_t	*con_chatHudPosX;
cVar_t	*con_chatHudPosY;
cVar_t	*con_chatHudShadow;
cVar_t	*con_notifyfade;
cVar_t	*con_notifylarge;
cVar_t	*con_notifylines;
cVar_t	*con_notifytime;
cVar_t	*con_alpha;
cVar_t	*con_clock;
cVar_t	*con_drop;
cVar_t	*con_scroll;

cVar_t	*scr_conspeed;
cVar_t	*scr_centertime;
cVar_t	*scr_showpause;
cVar_t	*scr_hudalpha;

cVar_t	*scr_netgraph;
cVar_t	*scr_timegraph;
cVar_t	*scr_debuggraph;
cVar_t	*scr_graphheight;
cVar_t	*scr_graphscale;
cVar_t	*scr_graphshift;
cVar_t	*scr_graphalpha;

cVar_t	*viewsize;
cVar_t	*gl_polyblend;

// ====================================================================

/*
==================
CG_SetGLConfig
==================
*/
void CG_SetGLConfig (glConfig_t *inConfig)
{
	cg.glConfig = *inConfig;
}


/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars (void)
{
	if (r_hudscale->modified) {
		r_hudscale->modified = qFalse;
		if (r_hudscale->value <= 0)
			cgi.Cvar_VariableSetValue (r_hudscale, 1, qTrue);
	}
	if (r_fontscale->modified) {
		r_fontscale->modified = qFalse;
		if (r_fontscale->value <= 0)
			cgi.Cvar_VariableSetValue (r_fontscale, 1, qTrue);
	}
}

/*
=======================================================================

	CONSOLE FUNCTIONS

=======================================================================
*/

/*
=================
CG_Skins_f

Load or download any custom player skins and models
=================
*/
static void CG_Skins_f (void)
{
	int		i;

	if (cg.currGameMod == GAME_MOD_GLOOM) {
		Com_Printf (PRNT_WARNING, "Command cannot be used in Gloom (considered cheating).\n");
		return;
	}

	for (i=0 ; i<MAX_CS_CLIENTS ; i++) {
		if (!cg.configStrings[CS_PLAYERSKINS+i][0])
			continue;

		Com_Printf (0, "client %i: %s\n", i, cg.configStrings[CS_PLAYERSKINS+i]); 
		cgi.R_UpdateScreen ();
		cgi.Sys_SendKeyEvents ();	// pump message loop
		CG_ParseClientinfo (i);
	}
}


/*
=================
CG_ThirdPerson_f
=================
*/
static void CG_ThirdPerson_f (void)
{
	cgi.Cvar_SetValue ("cg_thirdPerson", !cg_thirdPerson->integer, qFalse);
}

/*
=======================================================================

	INIT / SHUTDOWN

=======================================================================
*/

static void	*cmd_glmCache;
static void	*cmd_skins;
static void	*cmd_thirdPerson;

/*
=================
CG_CopyConfigStrings

This is necessary to make certain configstrings are consistant between
CGame and the client. Sometimes while connecting a configstring is sent
just before the CGame module is loaded.
=================
*/
void CG_CopyConfigStrings (void)
{
	int		num;

	for (num=0 ; num<MAX_CFGSTRINGS ; num++) {
		cgi.GetConfigString (num, cg.configStrings[num], MAX_CFGSTRLEN);

		CG_ParseConfigString (num, cg.configStrings[num]);
	}
}


/*
==================
CG_RegisterMain
==================
*/
static void CG_RegisterMain (void)
{
	cg_advInfrared			= cgi.Cvar_Register ("cg_advInfrared",			"1",			CVAR_ARCHIVE);
	cg_mapEffects			= cgi.Cvar_Register ("cg_mapEffects",			"1",			CVAR_ARCHIVE);

	if (cg.currGameMod == GAME_MOD_GLOOM)
		cg_thirdPerson		= cgi.Cvar_Register ("cg_thirdPerson",			"0",			CVAR_ARCHIVE|CVAR_CHEAT);
	else
		cg_thirdPerson		= cgi.Cvar_Register ("cg_thirdPerson",			"0",			CVAR_ARCHIVE);
	cg_thirdPersonAngle		= cgi.Cvar_Register ("cg_thirdPersonAngle",		"30",			CVAR_ARCHIVE);
	cg_thirdPersonClip		= cgi.Cvar_Register ("cg_thirdPersonClip",		"1",			CVAR_ARCHIVE);
	cg_thirdPersonDist		= cgi.Cvar_Register ("cg_thirdPersonDist",		"50",			CVAR_ARCHIVE);

	cg_decals				= cgi.Cvar_Register ("cg_decals",				"1",			CVAR_ARCHIVE);
	cg_decalBurnLife		= cgi.Cvar_Register ("cg_decalBurnLife",		"10",			CVAR_ARCHIVE);
	cg_decalLife			= cgi.Cvar_Register ("cg_decalLife",			"60",			CVAR_ARCHIVE);
	cg_decalLOD				= cgi.Cvar_Register ("cg_decalLOD",				"1",			CVAR_ARCHIVE);
	cg_decalMax				= cgi.Cvar_Register ("cg_decalMax",				"4000",			CVAR_ARCHIVE);

	cl_explorattle			= cgi.Cvar_Register ("cl_explorattle",			"1",			CVAR_ARCHIVE);
	cl_explorattle_scale	= cgi.Cvar_Register ("cl_explorattle_scale",	"0.3",			CVAR_ARCHIVE);
	cl_footsteps			= cgi.Cvar_Register ("cl_footsteps",			"1",			0);
	cl_gun					= cgi.Cvar_Register ("cl_gun",					"1",			0);
	cl_noskins				= cgi.Cvar_Register ("cl_noskins",				"0",			CVAR_CHEAT);
	cl_predict				= cgi.Cvar_Register ("cl_predict",				"1",			0);
	cl_showmiss				= cgi.Cvar_Register ("cl_showmiss",				"0",			0);
	cl_vwep					= cgi.Cvar_Register ("cl_vwep",					"1",			CVAR_ARCHIVE);

	gender_auto				= cgi.Cvar_Register ("gender_auto",				"1",			CVAR_ARCHIVE);
	gender					= cgi.Cvar_Register ("gender",					"male",			CVAR_USERINFO|CVAR_ARCHIVE);
	hand					= cgi.Cvar_Register ("hand",					"0",			CVAR_USERINFO|CVAR_ARCHIVE);
	skin					= cgi.Cvar_Register ("skin",					"male/grunt",	CVAR_USERINFO|CVAR_ARCHIVE);

	glm_advgas				= cgi.Cvar_Register ("glm_advgas",				"0",			CVAR_ARCHIVE);
	glm_advstingfire		= cgi.Cvar_Register ("glm_advstingfire",		"1",			CVAR_ARCHIVE);
	glm_blobtype			= cgi.Cvar_Register ("glm_blobtype",			"1",			CVAR_ARCHIVE);
	glm_bluestingfire		= cgi.Cvar_Register ("glm_bluestingfire",		"0",			CVAR_ARCHIVE);
	glm_flashpred			= cgi.Cvar_Register ("glm_flashpred",			"0",			CVAR_ARCHIVE);
	glm_flwhite				= cgi.Cvar_Register ("glm_flwhite",				"1",			CVAR_ARCHIVE);
	glm_forcecache			= cgi.Cvar_Register ("glm_forcecache",			"0",			CVAR_ARCHIVE);
	glm_jumppred			= cgi.Cvar_Register ("glm_jumppred",			"0",			CVAR_ARCHIVE);
	glm_showclass			= cgi.Cvar_Register ("glm_showclass",			"1",			CVAR_ARCHIVE);

	cl_add_particles		= cgi.Cvar_Register ("cl_particles",			"1",			0);
	cg_particleGore			= cgi.Cvar_Register ("cg_particleGore",			"3",			CVAR_ARCHIVE);
	cg_particleSmokeLinger	= cgi.Cvar_Register ("cg_particleSmokeLinger",	"3",			CVAR_ARCHIVE);
	cg_particleCulling		= cgi.Cvar_Register ("cg_particleCulling",		"1",			CVAR_ARCHIVE);
	cg_particleShading		= cgi.Cvar_Register ("cg_particleShading",		"1",			CVAR_ARCHIVE);

	cg_railCoreRed			= cgi.Cvar_Register ("cg_railCoreRed",			"0.75",			CVAR_ARCHIVE);
	cg_railCoreGreen		= cgi.Cvar_Register ("cg_railCoreGreen",		"1",			CVAR_ARCHIVE);
	cg_railCoreBlue			= cgi.Cvar_Register ("cg_railCoreBlue",			"1",			CVAR_ARCHIVE);
	cg_railSpiral			= cgi.Cvar_Register ("cg_railSpiral",			"1",			CVAR_ARCHIVE);
	cg_railSpiralRed		= cgi.Cvar_Register ("cg_railSpiralRed",		"0",			CVAR_ARCHIVE);
	cg_railSpiralGreen		= cgi.Cvar_Register ("cg_railSpiralGreen",		"0.5",			CVAR_ARCHIVE);
	cg_railSpiralBlue		= cgi.Cvar_Register ("cg_railSpiralBlue",		"0.9",			CVAR_ARCHIVE);

	cl_testblend			= cgi.Cvar_Register ("cl_testblend",			"0",			CVAR_CHEAT);
	cl_testentities			= cgi.Cvar_Register ("cl_testentities",			"0",			CVAR_CHEAT);
	cl_testlights			= cgi.Cvar_Register ("cl_testlights",			"0",			CVAR_CHEAT);
	cl_testparticles		= cgi.Cvar_Register ("cl_testparticles",		"0",			CVAR_CHEAT);

	r_hudscale				= cgi.Cvar_Register ("r_hudscale",				"1",			CVAR_ARCHIVE);
	r_fontscale				= cgi.Cvar_Register ("r_fontscale",				"1",			CVAR_ARCHIVE);

	if (cg.currGameMod == GAME_MOD_DDAY)
		crosshair			= cgi.Cvar_Register ("crosshair",				"0",			CVAR_ARCHIVE|CVAR_CHEAT);
	else
		crosshair			= cgi.Cvar_Register ("crosshair",				"0",			CVAR_ARCHIVE);
	ch_alpha				= cgi.Cvar_Register ("ch_alpha",				"1",			CVAR_ARCHIVE);
	ch_pulse				= cgi.Cvar_Register ("ch_pulse",				"1",			CVAR_ARCHIVE);
	ch_scale				= cgi.Cvar_Register ("ch_scale",				"1",			CVAR_ARCHIVE);
	ch_red					= cgi.Cvar_Register ("ch_red",					"1",			CVAR_ARCHIVE);
	ch_green				= cgi.Cvar_Register ("ch_green",				"1",			CVAR_ARCHIVE);
	ch_blue					= cgi.Cvar_Register ("ch_blue",					"1",			CVAR_ARCHIVE);

	cl_showfps				= cgi.Cvar_Register ("cl_showfps",				"1",			CVAR_ARCHIVE);
	cl_showping				= cgi.Cvar_Register ("cl_showping",				"1",			CVAR_ARCHIVE);
	cl_showtime				= cgi.Cvar_Register ("cl_showtime",				"1",			CVAR_ARCHIVE);

	con_chatHud				= cgi.Cvar_Register ("con_chatHud",				"0",			CVAR_ARCHIVE);
	con_chatHudLines		= cgi.Cvar_Register ("con_chatHudLines",		"4",			CVAR_ARCHIVE);
	con_chatHudPosX			= cgi.Cvar_Register ("con_chatHudPosX",			"8",			CVAR_ARCHIVE);
	con_chatHudPosY			= cgi.Cvar_Register ("con_chatHudPosY",			"-14",			CVAR_ARCHIVE);
	con_chatHudShadow		= cgi.Cvar_Register ("con_chatHudShadow",		"0",			CVAR_ARCHIVE);
	con_notifyfade			= cgi.Cvar_Register ("con_notifyfade",			"1",			CVAR_ARCHIVE);
	con_notifylarge			= cgi.Cvar_Register ("con_notifylarge",			"0",			CVAR_ARCHIVE);
	con_notifylines			= cgi.Cvar_Register ("con_notifylines",			"4",			CVAR_ARCHIVE);
	con_notifytime			= cgi.Cvar_Register ("con_notifytime",			"3",			CVAR_ARCHIVE);
	con_alpha				= cgi.Cvar_Register ("con_alpha",				"1",			CVAR_ARCHIVE);
	con_clock				= cgi.Cvar_Register ("con_clock",				"1",			CVAR_ARCHIVE);
	con_drop				= cgi.Cvar_Register ("con_drop",				"0.5",			CVAR_ARCHIVE);
	con_scroll				= cgi.Cvar_Register ("con_scroll",				"2",			CVAR_ARCHIVE);

	scr_conspeed			= cgi.Cvar_Register ("scr_conspeed",			"3",			0);
	scr_centertime			= cgi.Cvar_Register ("scr_centertime",			"2.5",			0);
	scr_showpause			= cgi.Cvar_Register ("scr_showpause",			"1",			0);
	scr_hudalpha			= cgi.Cvar_Register ("scr_hudalpha",			"1",			CVAR_ARCHIVE);

	scr_netgraph			= cgi.Cvar_Register ("netgraph",				"0",			0);
	scr_timegraph			= cgi.Cvar_Register ("timegraph",				"0",			0);
	scr_debuggraph			= cgi.Cvar_Register ("debuggraph",				"0",			0);
	scr_graphheight			= cgi.Cvar_Register ("graphheight",				"32",			0);
	scr_graphscale			= cgi.Cvar_Register ("graphscale",				"1",			0);
	scr_graphshift			= cgi.Cvar_Register ("graphshift",				"0",			0);
	scr_graphalpha			= cgi.Cvar_Register ("scr_graphalpha",			"0.6",			CVAR_ARCHIVE);

	viewsize				= cgi.Cvar_Register ("viewsize",				"100",			CVAR_ARCHIVE);

	if (cg.currGameMod == GAME_MOD_GLOOM)
		gl_polyblend		= cgi.Cvar_Register ("gl_polyblend",			"1",			CVAR_CHEAT);
	else
		gl_polyblend		= cgi.Cvar_Register ("gl_polyblend",			"1",			0);

	gender->modified = qFalse; // clear this so we know when user sets it manually

	cmd_glmCache	= cgi.Cmd_AddCommand ("glmcache",		CG_CacheGloomMedia,	"Forces caching of Gloom media right now");
	cmd_skins		= cgi.Cmd_AddCommand ("skins",			CG_Skins_f,			"Lists skins of players connected");
	cmd_thirdPerson	= cgi.Cmd_AddCommand ("thirdPerson",	CG_ThirdPerson_f,	"Toggles the third person camera");
}


/*
==================
CG_RemoveCmds
==================
*/
static void CG_RemoveCmds (void)
{
	cgi.Cmd_RemoveCommand ("glmcache",		cmd_glmCache);
	cgi.Cmd_RemoveCommand ("skins",			cmd_skins);
	cgi.Cmd_RemoveCommand ("thirdPerson",	cmd_thirdPerson);
}


/*
==================
CG_LoadMap
==================
*/
void CG_LoadMap (int playerNum, int serverProtocol, qBool attractLoop, qBool strafeHack, glConfig_t *inConfig)
{
	// Default values
	cg.frameCount = 1;
	cg.gloomCheckClass = qFalse;
	cg.gloomClassType = GLM_OBSERVER;
	cg.playerNum = playerNum;
	cg.serverProtocol = serverProtocol;
	cg.attractLoop = attractLoop;	// true if demo playback
	cg.strafeHack = strafeHack; // god damnit

	// Video settings
	cg.glConfig = *inConfig;

	// Copy config strings
	CG_CopyConfigStrings ();

	// Init media
	CG_InitBaseMedia ();

	cg.mapLoading = qTrue;
	cg.mapLoaded = qFalse;

	CG_MapInit ();

	cg.mapLoading = qFalse;
	cg.mapLoaded = qTrue;
}


/*
==================
CG_Init
==================
*/
void CG_Init (void)
{
	char	*gameDir;

	// Clear everything
	memset (&cg, 0, sizeof (cgState_t));
	memset (&cgMedia, 0, sizeof (cgMedia_t));
	memset (&uiMedia, 0, sizeof (uiMedia));

	CG_ClearEntities ();

	// This nastiness is done because I don't
	// feel like compiling several cgame libraries...
	gameDir = cgi.FS_Gamedir ();
	if (strstr (gameDir, "dday"))
		cg.currGameMod = GAME_MOD_DDAY;
	else if (strstr (gameDir, "lox"))
		cg.currGameMod = GAME_MOD_LOX;
	else if (strstr (gameDir, "gloom"))
		cg.currGameMod = GAME_MOD_GLOOM;
	else if (strstr (gameDir, "rogue"))
		cg.currGameMod = GAME_MOD_ROGUE;
	else if (strstr (gameDir, "xatrix"))
		cg.currGameMod = GAME_MOD_XATRIX;
	else
		cg.currGameMod = GAME_MOD_DEFAULT;

	// Update glConfig
	cgi.R_GetGLConfig (&cg.glConfig);

	// Copy config strings
	CG_CopyConfigStrings ();

	// Initialize early needed media
	CG_InitBaseMedia ();

	// Register cvars and commands
	V_Register ();
	CG_WeapRegister ();
	CG_RegisterMain ();

	// Check cvar sanity
	CG_UpdateCvars ();

	// Load the UI
	UI_Init ();
}


/*
==================
CG_Shutdown
==================
*/
void CG_Shutdown (void)
{
	// Shutdown map fx
	CG_ShutdownMapFX ();

	// Clean out entities
	CG_ClearEntities ();

	// Remove commands
	CG_RemoveCmds ();
	V_Unregister ();
	CG_WeapUnregister ();

	// Shutdown media
	CG_ShutdownMap ();

	// Shutdown the UI
	UI_Shutdown ();

	// The above function calls should release all of this anyways, but this is to be certain
	CG_FreeTag (CGTAG_ANY);
	CG_FreeTag (CGTAG_DECALS);
	CG_FreeTag (CGTAG_MAPFX);
	CG_FreeTag (CGTAG_MENU);
}

//======================================================================

#ifndef CGAME_HARD_LINKED
/*
==================
Com_Printf
==================
*/
void Com_Printf (comPrint_t flags, char *fmt, ...)
{
	va_list		argptr;
	char		text[MAX_COMPRINT];

	// Evaluate args
	va_start (argptr, fmt);
	vsnprintf (text, sizeof (text), fmt, argptr);
	va_end (argptr);

	// Print
	cgi.Com_Printf (flags, text);
}


/*
==================
Com_DevPrintf
==================
*/
void Com_DevPrintf (comPrint_t flags, char *fmt, ...)
{
	va_list		argptr;
	char		text[MAX_COMPRINT];

	// Evaluate args
	va_start (argptr, fmt);
	vsnprintf (text, sizeof (text), fmt, argptr);
	va_end (argptr);

	// Print
	cgi.Com_DevPrintf (flags, text);
}


/*
==================
Com_Error
==================
*/
void Com_Error (comError_t code, char *fmt, ...)
{
	va_list		argptr;
	char		text[MAX_COMPRINT];

	// Evaluate args
	va_start (argptr, fmt);
	vsnprintf (text, sizeof (text), fmt, argptr);
	va_end (argptr);

	// Print
	cgi.Com_Error (code, text);
}
#endif // CGAME_HARD_LINKED
