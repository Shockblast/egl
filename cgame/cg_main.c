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
cgStatic_t	cgs;
cgMedia_t	cgMedia;

cVar_t	*cg_mapeffects;

cVar_t	*cl_add_decals;
cVar_t	*cl_decal_burnlife;
cVar_t	*cl_decal_life;
cVar_t	*cl_decal_lod;
cVar_t	*cl_decal_max;

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
cVar_t	*cl_gore;
cVar_t	*cl_smokelinger;
cVar_t	*part_degree;
cVar_t	*part_cull;
cVar_t	*part_lod;
cVar_t	*part_shade;

cVar_t	*cl_railred;
cVar_t	*cl_railgreen;
cVar_t	*cl_railblue;
cVar_t	*cl_railtrail;
cVar_t	*cl_spiralred;
cVar_t	*cl_spiralgreen;
cVar_t	*cl_spiralblue;

cVar_t	*r_hudscale;

cVar_t	*scr_hudalpha;


// ====================================================================

/*
==================
CG_StrDup

No need to null terminate the extra spot because Mem_Alloc returns zero-filled memory
==================
*/
char *CG_StrDup (const char *in) {
	char	*out;

	out = CG_MemAlloc (Q_strlen (in) + 1);
	strcpy (out, in);

	return out;
}

/*
=======================================================================

	CVAR MONITORING

=======================================================================
*/

/*
==================
CG_FixCvarCheats
==================
*/
typedef struct {
	char	*name;
	int		value;
	cVar_t	*var;
} cheatVar_t;
static cheatVar_t cheatVars[] = {
	{"timescale",			1},
	{"timedemo",			0},
	{"r_drawworld",			1},
	{"cl_testlights",		0},
	{"r_fullbright",		0},
	{"paused",				0},
	{"fixedtime",			0},
	{"gl_lightmap",			0},
	{"gl_saturatelighting",	0},


	{"r_caustics",			0}, // orly
	{NULL,					0}
};
static cheatVar_t gloomCheatVars[] = {
	{"cl_noskins",			0},
	{"cl_testblend",		0},
	{"gl_nobind",			0},
	{"gl_polyblend",		1},
	{"r_bloom",				0},
	{"r_fog",				0},
	{"gl_showtris",			0},
	{"gl_shownormals",		0},
	{NULL,					0}
};
static cheatVar_t ddayCheatVars[] = {
	{"crosshair",			0},
	{NULL,					0},
};
static void CG_FixCvarCheats (void) {
	int			i;
	cheatVar_t	*var;
	static int	numCheatVars;

	if (!strcmp (cg.configStrings[CS_MAXCLIENTS], "1")  || !cg.configStrings[CS_MAXCLIENTS][0])
		return;		// single player can cheat

	// find all the cvars if we haven't done it yet
	if (!numCheatVars) {
		while (cheatVars[numCheatVars].name) {
			cheatVars[numCheatVars].var = cgi.Cvar_Get (cheatVars[numCheatVars].name, Q_VarArgs ("%d", cheatVars[numCheatVars].value), 0);
			numCheatVars++;
		}
	}

	// make sure they are all set to the proper values
	for (i=0, var=cheatVars ; i<numCheatVars ; i++, var++) {
		if (var->var->integer != var->value)
			cgi.Cvar_VariableForceSetValue (var->var, var->value);
	}

	//
	// Gloom cheats
	//
	if (cgi.FS_CurrGame ("gloom")) {
		static int	numGlmCheatVars;

		// find all the cvars if we haven't done it yet
		if (!numGlmCheatVars) {
			while (gloomCheatVars[numGlmCheatVars].name) {
				gloomCheatVars[numGlmCheatVars].var = cgi.Cvar_Get (gloomCheatVars[numGlmCheatVars].name, Q_VarArgs ("%d", gloomCheatVars[numGlmCheatVars].value), 0);
				numGlmCheatVars++;
			}
		}

		// make sure they are all set to the proper values
		for (i=0, var=gloomCheatVars ; i<numGlmCheatVars ; i++, var++) {
			if (var->var->integer != var->value)
				cgi.Cvar_VariableForceSetValue (var->var, var->value);
		}
	}

	//
	// DDay cheats
	//
	if (cgi.FS_CurrGame ("dday")) {
		static int	numDDayCheatVars;

		// find all the cvars if we haven't done it yet
		if (!numDDayCheatVars) {
			while (ddayCheatVars[numDDayCheatVars].name) {
				ddayCheatVars[numDDayCheatVars].var = cgi.Cvar_Get (ddayCheatVars[numDDayCheatVars].name, Q_VarArgs ("%d", ddayCheatVars[numDDayCheatVars].value), 0);
				numDDayCheatVars++;
			}
		}

		// make sure they are all set to the proper values
		for (i=0, var=ddayCheatVars ; i<numDDayCheatVars ; i++, var++) {
			if (var->var->integer != var->value)
				cgi.Cvar_VariableForceSetValue (var->var, var->value);
		}
	}
}


/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars (void) {
	if (!cg.initialized)
		return;

	if (r_hudscale->modified) {
		r_hudscale->modified = qFalse;
		if (r_hudscale->value <= 0)
			cgi.Cvar_VariableForceSetValue (r_hudscale, 1);
	}

	CG_FixCvarCheats ();
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
static void CG_Skins_f (void) {
	int		i;

	if (cgi.FS_CurrGame ("gloom")) {
		Com_Printf (0, S_COLOR_CYAN "Command cannot be used in Gloom (considered cheating).\n");
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
=======================================================================

	INIT / SHUTDOWN

=======================================================================
*/

/*
=================
CG_CopyConfigStrings
=================
*/
void CG_CopyConfigStrings (void) {
	int		num;

	for (num=0 ; num<MAX_CONFIGSTRINGS ; num++) {
		cgi.GetConfigString (num, cg.configStrings[num], MAX_QPATH);

		// do something apropriate
		if ((num >= CS_LIGHTS) && (num < CS_LIGHTS+MAX_CS_LIGHTSTYLES)) {
			// lightstyle
			CG_SetLightstyle (num-CS_LIGHTS);
		}
		else if ((num >= CS_IMAGES) && (num < CS_IMAGES+MAX_CS_IMAGES)) {
			// image
			CG_RegisterPic (cg.configStrings[num]);
		}
	}
}


/*
==================
CG_RegisterCvars
==================
*/
static void CG_RegisterCvars (void) {
	cg_mapeffects		= cgi.Cvar_Get ("cg_mapeffects",		"1",			CVAR_ARCHIVE);

	cl_add_decals		= cgi.Cvar_Get ("cl_decals",			"1",			CVAR_ARCHIVE);
	cl_decal_burnlife	= cgi.Cvar_Get ("cl_decal_burnlife",	"10",			CVAR_ARCHIVE);
	cl_decal_life		= cgi.Cvar_Get ("cl_decal_life",		"60",			CVAR_ARCHIVE);
	cl_decal_lod		= cgi.Cvar_Get ("cl_decal_lod",			"1",			CVAR_ARCHIVE);
	cl_decal_max		= cgi.Cvar_Get ("cl_decal_max",			"4000",			CVAR_ARCHIVE);

	cl_explorattle		= cgi.Cvar_Get ("cl_explorattle",		"1",			CVAR_ARCHIVE);
	cl_explorattle_scale= cgi.Cvar_Get ("cl_explorattle_scale",	"0.3",			CVAR_ARCHIVE);
	cl_footsteps		= cgi.Cvar_Get ("cl_footsteps",			"1",			0);
	cl_gun				= cgi.Cvar_Get ("cl_gun",				"1",			0);
	cl_noskins			= cgi.Cvar_Get ("cl_noskins",			"0",			0);
	cl_predict			= cgi.Cvar_Get ("cl_predict",			"1",			0);
	cl_showmiss			= cgi.Cvar_Get ("cl_showmiss",			"0",			0);
	cl_vwep				= cgi.Cvar_Get ("cl_vwep",				"1",			CVAR_ARCHIVE);

	gender_auto			= cgi.Cvar_Get ("gender_auto",			"1",			CVAR_ARCHIVE);
	gender				= cgi.Cvar_Get ("gender",				"male",			CVAR_USERINFO|CVAR_ARCHIVE);
	hand				= cgi.Cvar_Get ("hand",					"0",			CVAR_USERINFO|CVAR_ARCHIVE);
	gender->modified = qFalse; // clear this so we know when user sets it manually
	skin				= cgi.Cvar_Get ("skin",					"male/grunt",	CVAR_USERINFO|CVAR_ARCHIVE);

	glm_advgas			= cgi.Cvar_Get ("glm_advgas",			"0",			CVAR_ARCHIVE);
	glm_advstingfire	= cgi.Cvar_Get ("glm_advstingfire",		"1",			CVAR_ARCHIVE);
	glm_blobtype		= cgi.Cvar_Get ("glm_blobtype",			"1",			CVAR_ARCHIVE);
	glm_bluestingfire	= cgi.Cvar_Get ("glm_bluestingfire",	"0",			CVAR_ARCHIVE);
	glm_flashpred		= cgi.Cvar_Get ("glm_flashpred",		"0",			CVAR_ARCHIVE);
	glm_flwhite			= cgi.Cvar_Get ("glm_flwhite",			"1",			CVAR_ARCHIVE);
	glm_forcecache		= cgi.Cvar_Get ("glm_forcecache",		"0",			CVAR_ARCHIVE);
	glm_jumppred		= cgi.Cvar_Get ("glm_jumppred",			"0",			CVAR_ARCHIVE);
	glm_showclass		= cgi.Cvar_Get ("glm_showclass",		"1",			CVAR_ARCHIVE);

	cl_add_particles	= cgi.Cvar_Get ("cl_particles",			"1",			0);
	cl_gore				= cgi.Cvar_Get ("cl_gore",				"3",			CVAR_ARCHIVE);
	cl_smokelinger		= cgi.Cvar_Get ("cl_smokelinger",		"0",			CVAR_ARCHIVE);
	part_cull			= cgi.Cvar_Get ("part_cull",			"1",			CVAR_ARCHIVE);
	part_degree			= cgi.Cvar_Get ("part_degree",			"1",			CVAR_ARCHIVE);
	part_lod			= cgi.Cvar_Get ("part_lod",				"1",			CVAR_ARCHIVE);
	part_shade			= cgi.Cvar_Get ("part_shade",			"1",			CVAR_ARCHIVE);

	cl_railred			= cgi.Cvar_Get ("cl_railred",			"1",			CVAR_ARCHIVE);
	cl_railgreen		= cgi.Cvar_Get ("cl_railgreen",			"1",			CVAR_ARCHIVE);
	cl_railblue			= cgi.Cvar_Get ("cl_railblue",			"1",			CVAR_ARCHIVE);
	cl_railtrail		= cgi.Cvar_Get ("cl_railtrail",			"2",			CVAR_ARCHIVE);
	cl_spiralred		= cgi.Cvar_Get ("cl_spiralred",			"0",			CVAR_ARCHIVE);
	cl_spiralgreen		= cgi.Cvar_Get ("cl_spiralgreen",		"0.6",			CVAR_ARCHIVE);
	cl_spiralblue		= cgi.Cvar_Get ("cl_spiralblue",		"0.9",			CVAR_ARCHIVE);

	r_hudscale			= cgi.Cvar_Get ("r_hudscale",			"1",			CVAR_ARCHIVE);

	scr_hudalpha		= cgi.Cvar_Get ("scr_hudalpha",			"1",			CVAR_ARCHIVE);
}


/*
==================
CG_RegisterCmds
==================
*/
static void CG_RegisterCmds (void) {
	CGI_Cmd_AddCommand ("skins",		CG_Skins_f,			"Lists skins of players connected");

	CGI_Cmd_AddCommand ("glmcache",		CG_CacheGloomMedia,	"Forces caching of Gloom media right now");
}


/*
==================
CG_RemoveCmds
==================
*/
static void CG_RemoveCmds (void) {
	CGI_Cmd_RemoveCommand ("skins");

	CGI_Cmd_RemoveCommand ("glmcache");
}


/*
==================
CG_Init
==================
*/
void CG_Init (int playerNum, int serverProtocol, qBool attractLoop, int vidWidth, int vidHeight) {
	if (cg.initialized)
		return;

	// clear everything
	memset (&cg, 0, sizeof (cgState_t));
	memset (&cgs, 0, sizeof (cgStatic_t));
	memset (&cgMedia, 0, sizeof (cgMedia_t));

	CG_ClearEntities ();

	// default values
	cgs.frameCount = 1;

	// save local player number
	cg.playerNum = playerNum;
	cgs.serverProtocol = serverProtocol;
	cg.attractLoop = attractLoop;	// true if demo playback

	// video dimensions
	cg.vidWidth = vidWidth;
	cg.vidHeight = vidHeight;

	// copy config strings
	CG_CopyConfigStrings ();

	// prep loadscreen images
	CG_PrepLoadScreen ();

	// register cvars and commands
	SCR_Register ();
	V_Register ();
	CG_WeapRegister ();
	CG_RegisterCvars ();
	CG_RegisterCmds ();

	// check cvar sanity
	CG_UpdateCvars ();

	// init media
	CG_MediaInit ();

	// all done
	cg.initialized = qTrue;
}


/*
==================
CG_Shutdown
==================
*/
void CG_Shutdown (qBool full) {
	if (!cg.initialized)
		return;

	// shutdown map fx
	CG_ShutdownMapFX ();

	// clean out entities
	CG_ClearEntities ();

	if (!full) {
		// remove commands
		CG_RemoveCmds ();
		V_Unregister ();
		CG_WeapUnregister ();
	}

	// shutdown media
	CG_ShutdownMedia ();

	cg.initialized = qFalse;
}

//======================================================================

#ifndef CGAME_HARD_LINKED
/*
==================
Com_Printf
==================
*/
void Com_Printf (int flags, char *fmt, ...) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	vsnprintf (text, sizeof (text), fmt, argptr);
	va_end (argptr);

	cgi.Com_Printf (flags, "%s", text);
}


/*
==================
Com_Error
==================
*/
void Com_Error (int code, char *fmt, ...) {
	va_list		argptr;
	char		msg[1024];

	va_start (argptr, fmt);
	vsnprintf (msg, sizeof (msg), fmt, argptr);
	va_end (argptr);

	cgi.Com_Error (code, "%s", msg);
}
#endif
