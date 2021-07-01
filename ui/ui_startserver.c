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

	START SERVER MENU

=============================================================================
*/

typedef struct m_startServerMenu_s {
	// Local info
	char				**mapNames;
	int					numMaps;

	// Menu items
	uiFrameWork_t		frameWork;

	uiImage_t			banner;

	uiAction_t			start_action;
	uiAction_t			dmflags_action;
	uiField_t			timelimit_field;
	uiField_t			fraglimit_field;
	uiField_t			maxclients_field;
	uiField_t			hostname_field;
	uiList_t			startmap_list;
	uiList_t			rules_box;

	uiAction_t			back_action;
} m_startServerMenu_t;

static m_startServerMenu_t m_startServerMenu;

static void DMFlagsFunc (void *self)
{
	self = self;

	if (m_startServerMenu.rules_box.curValue == 1)
		return;

	UI_DMFlagsMenu_f ();
}

static void RulesChangeFunc (void *self)
{
	self = self;

	// DM
	if (m_startServerMenu.rules_box.curValue == 0) {
		m_startServerMenu.maxclients_field.generic.statusBar = NULL;
		m_startServerMenu.dmflags_action.generic.statusBar = NULL;
	}
	else if (m_startServerMenu.rules_box.curValue == 1) {		// coop				// PGM
		m_startServerMenu.maxclients_field.generic.statusBar = "4 maximum for cooperative";
		if (atoi (m_startServerMenu.maxclients_field.buffer) > 4)
			Q_strncpyz (m_startServerMenu.maxclients_field.buffer, "4", sizeof (m_startServerMenu.maxclients_field.buffer));
		m_startServerMenu.dmflags_action.generic.statusBar = "N/A for cooperative";
	}

	// ROGUE
	else if (uii.FS_CurrGame ("rogue")) {
		if (m_startServerMenu.rules_box.curValue == 2) {		// tag
			m_startServerMenu.maxclients_field.generic.statusBar = NULL;
			m_startServerMenu.dmflags_action.generic.statusBar = NULL;
		}
	}
}

static void StartServerActionFunc (void *self)
{
	char	startmap[1024];
	int		timelimit;
	int		fraglimit;
	int		maxclients;
	char	*spot;

	self = self;
	Q_strncpyz (startmap, strchr (m_startServerMenu.mapNames[m_startServerMenu.startmap_list.curValue], '\n') + 1, sizeof (startmap));

	maxclients  = atoi (m_startServerMenu.maxclients_field.buffer);
	timelimit	= atoi (m_startServerMenu.timelimit_field.buffer);
	fraglimit	= atoi (m_startServerMenu.fraglimit_field.buffer);

	uii.Cvar_SetValue ("maxclients",	clamp (maxclients, 0, maxclients));
	uii.Cvar_SetValue ("timelimit",		clamp (timelimit, 0, timelimit));
	uii.Cvar_SetValue ("fraglimit",		clamp (fraglimit, 0, fraglimit));
	uii.Cvar_Set ("hostname",			m_startServerMenu.hostname_field.buffer);

	if (m_startServerMenu.rules_box.curValue < 2 || !uii.FS_CurrGame ("rogue")) {
		uii.Cvar_SetValue ("deathmatch",	!m_startServerMenu.rules_box.curValue);
		uii.Cvar_SetValue ("coop",			m_startServerMenu.rules_box.curValue);
		uii.Cvar_SetValue ("gamerules",		0);
	}
	else {
		uii.Cvar_SetValue ("deathmatch",	1);	// deathmatch is always true for rogue games, right?
		uii.Cvar_SetValue ("coop",			0);
		uii.Cvar_SetValue ("gamerules",		m_startServerMenu.rules_box.curValue);
	}

	spot = NULL;
	if (m_startServerMenu.rules_box.curValue == 1) {
		if (!Q_stricmp (startmap, "bunk1"))
			spot = "start";
		else if (!Q_stricmp (startmap, "mintro"))
			spot = "start";
		else if (!Q_stricmp (startmap, "fact1"))
			spot = "start";
		else if (!Q_stricmp (startmap, "power1"))
			spot = "pstart";
		else if (!Q_stricmp (startmap, "biggun"))
			spot = "bstart";
		else if (!Q_stricmp (startmap, "hangar1"))
			spot = "unitstart";
		else if (!Q_stricmp (startmap, "city1"))
			spot = "unitstart";
		else if (!Q_stricmp (startmap, "boss1"))
			spot = "bosstart";
	}

	if (spot) {
		if (uiState.serverState)
			uii.Cbuf_AddText ("disconnect\n");
		uii.Cbuf_AddText (Q_VarArgs ("gamemap \"*%s$%s\"\n", startmap, spot));
	}
	else
		uii.Cbuf_AddText (Q_VarArgs ("map %s\n", startmap));

	UI_ForceMenuOff ();
}


/*
=============
UI_FreeMapNames
=============
*/
static void UI_FreeMapNames (void)
{
	if (m_startServerMenu.mapNames) {
		int i;

		for (i=0 ; i<m_startServerMenu.numMaps ; i++)
			UI_MemFree (m_startServerMenu.mapNames[i]);
		UI_MemFree (m_startServerMenu.mapNames);
	}

	m_startServerMenu.mapNames = NULL;
	m_startServerMenu.numMaps = 0;
}


/*
=============
StartServerMenu_Init
=============
*/
static void StartServerMenu_Init (void)
{
	static char *dm_coop_names[] = {
		"deathmatch",
		"cooperative",
		0
	};

	static char *dm_coop_names_rogue[] = {
		"deathmatch",
		"cooperative",
		"tag",
		0
	};

	char	*buffer, *s;
	char	mapsname[1024];
	char	shortname[MAX_TOKEN_CHARS];
	char	longname[MAX_TOKEN_CHARS];
	int		length, i;
	FILE	*fp;

	// clear old names
	UI_FreeMapNames ();

	/*
	** load the list of map names
	*/
	Q_snprintfz (mapsname, sizeof (mapsname), "%s/maps.lst", uii.FS_Gamedir());
	if ((fp = fopen (mapsname, "rb")) == 0) {
		if ((length = uii.FS_LoadFile ("maps.lst", (void **)&buffer)) == -1)
			Com_Error (ERR_DROP, "couldn't find maps.lst\n");
	}
	else {
		fseek (fp, 0, SEEK_END);
		length = ftell (fp);

		fseek (fp, 0, SEEK_SET);
		buffer = UI_MemAlloc (length);
		fread (buffer, length, 1, fp);
	}

	s = buffer;

	i = 0;
	while (i < length) {
		if (s[i] == '\r')
			m_startServerMenu.numMaps++;
		i++;
	}

	if (m_startServerMenu.numMaps == 0)
		Com_Error (ERR_DROP, "no maps in maps.lst\n");

	m_startServerMenu.mapNames = UI_MemAlloc (sizeof (char *) * (m_startServerMenu.numMaps + 1));

	s = buffer;

	for (i=0 ; i<m_startServerMenu.numMaps ; i++) {
		char	scratch[200];
		int		j, l;

		Q_strncpyz (shortname, Com_Parse (&s), sizeof (shortname));
		l = strlen (shortname);
		for (j=0 ; j<l ; j++)
			shortname[j] = toupper(shortname[j]);

		Q_strncpyz (longname, Com_Parse (&s), sizeof (longname));
		Q_snprintfz (scratch, sizeof (scratch), "%s\n%s", longname, shortname);

		m_startServerMenu.mapNames[i] = UI_StrDup (scratch);
	}

	m_startServerMenu.mapNames[m_startServerMenu.numMaps] = 0;

	if (fp != 0) {
		fp = 0;
		UI_MemFree (buffer);
	}
	else {
		UI_FS_FreeFile (buffer);
	}

	/*
	** initialize the menu stuff
	*/
	UI_StartFramework (&m_startServerMenu.frameWork);

	m_startServerMenu.banner.generic.type		= UITYPE_IMAGE;
	m_startServerMenu.banner.generic.flags		= UIF_NOSELECT|UIF_CENTERED;
	m_startServerMenu.banner.generic.name		= NULL;
	m_startServerMenu.banner.shader				= uiMedia.banners.startServer;

	m_startServerMenu.startmap_list.generic.type	= UITYPE_SPINCONTROL;
	m_startServerMenu.startmap_list.generic.name	= "Initial map";
	m_startServerMenu.startmap_list.itemNames		= m_startServerMenu.mapNames;

	m_startServerMenu.rules_box.generic.type		= UITYPE_SPINCONTROL;
	m_startServerMenu.rules_box.generic.name		= "Rules";
	
	if (uii.FS_CurrGame ("rogue"))
		m_startServerMenu.rules_box.itemNames = dm_coop_names_rogue;
	else
		m_startServerMenu.rules_box.itemNames = dm_coop_names;

	if (uii.Cvar_VariableInteger ("coop"))
		m_startServerMenu.rules_box.curValue = 1;
	else
		m_startServerMenu.rules_box.curValue = 0;

	m_startServerMenu.rules_box.generic.callBack = RulesChangeFunc;

	m_startServerMenu.timelimit_field.generic.type			= UITYPE_FIELD;
	m_startServerMenu.timelimit_field.generic.name			= "Time limit";
	m_startServerMenu.timelimit_field.generic.flags			= UIF_NUMBERSONLY;
	m_startServerMenu.timelimit_field.generic.statusBar		= "0 = no limit";
	m_startServerMenu.timelimit_field.length				= 3;
	m_startServerMenu.timelimit_field.visibleLength			= 3;
	Q_strncpyz (m_startServerMenu.timelimit_field.buffer, uii.Cvar_VariableString ("timelimit"), sizeof (m_startServerMenu.timelimit_field.buffer));

	m_startServerMenu.fraglimit_field.generic.type			= UITYPE_FIELD;
	m_startServerMenu.fraglimit_field.generic.name			= "Frag limit";
	m_startServerMenu.fraglimit_field.generic.flags			= UIF_NUMBERSONLY;
	m_startServerMenu.fraglimit_field.generic.statusBar		= "0 = no limit";
	m_startServerMenu.fraglimit_field.length				= 3;
	m_startServerMenu.fraglimit_field.visibleLength			= 3;
	Q_strncpyz (m_startServerMenu.fraglimit_field.buffer, uii.Cvar_VariableString ("fraglimit"), sizeof (m_startServerMenu.fraglimit_field.buffer));

	/*
	** maxclients determines the maximum number of players that can join
	** the game.  If maxclients is only "1" then we should default the menu
	** option to 8 players, otherwise use whatever its current value is. 
	** Clamping will be done when the server is actually started.
	*/
	m_startServerMenu.maxclients_field.generic.type		= UITYPE_FIELD;
	m_startServerMenu.maxclients_field.generic.name		= "Max players";
	m_startServerMenu.maxclients_field.generic.flags	= UIF_NUMBERSONLY;
	m_startServerMenu.maxclients_field.length			= 3;
	m_startServerMenu.maxclients_field.visibleLength	= 3;

	if (uii.Cvar_VariableInteger ("maxclients") == 1)
		Q_strncpyz (m_startServerMenu.maxclients_field.buffer, "8", sizeof (m_startServerMenu.maxclients_field.buffer));
	else 
		Q_strncpyz (m_startServerMenu.maxclients_field.buffer, uii.Cvar_VariableString ("maxclients"), sizeof (m_startServerMenu.maxclients_field.buffer));

	m_startServerMenu.hostname_field.generic.type		= UITYPE_FIELD;
	m_startServerMenu.hostname_field.generic.name		= "Hostname";
	m_startServerMenu.hostname_field.generic.flags		= 0;
	m_startServerMenu.hostname_field.length				= 12;
	m_startServerMenu.hostname_field.visibleLength		= 12;
	Q_strncpyz (m_startServerMenu.hostname_field.buffer, uii.Cvar_VariableString ("hostname"), sizeof (m_startServerMenu.hostname_field.buffer));

	m_startServerMenu.dmflags_action.generic.type		= UITYPE_ACTION;
	m_startServerMenu.dmflags_action.generic.name		= "Deathmatch flags";
	m_startServerMenu.dmflags_action.generic.flags		= UIF_LEFT_JUSTIFY;
	m_startServerMenu.dmflags_action.generic.callBack	= DMFlagsFunc;

	m_startServerMenu.start_action.generic.type		= UITYPE_ACTION;
	m_startServerMenu.start_action.generic.name		= "Begin";
	m_startServerMenu.start_action.generic.flags	= UIF_LEFT_JUSTIFY;
	m_startServerMenu.start_action.generic.callBack	= StartServerActionFunc;

	m_startServerMenu.back_action.generic.type		= UITYPE_ACTION;
	m_startServerMenu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_startServerMenu.back_action.generic.name		= "< Back";
	m_startServerMenu.back_action.generic.callBack	= Menu_Pop;
	m_startServerMenu.back_action.generic.statusBar	= "Back a menu";

	UI_AddItem (&m_startServerMenu.frameWork,		&m_startServerMenu.banner);

	UI_AddItem (&m_startServerMenu.frameWork,		&m_startServerMenu.startmap_list);
	UI_AddItem (&m_startServerMenu.frameWork,		&m_startServerMenu.rules_box);
	UI_AddItem (&m_startServerMenu.frameWork,		&m_startServerMenu.timelimit_field);
	UI_AddItem (&m_startServerMenu.frameWork,		&m_startServerMenu.fraglimit_field);
	UI_AddItem (&m_startServerMenu.frameWork,		&m_startServerMenu.maxclients_field);
	UI_AddItem (&m_startServerMenu.frameWork,		&m_startServerMenu.hostname_field);
	UI_AddItem (&m_startServerMenu.frameWork,		&m_startServerMenu.dmflags_action);
	UI_AddItem (&m_startServerMenu.frameWork,		&m_startServerMenu.start_action);

	UI_AddItem (&m_startServerMenu.frameWork,		&m_startServerMenu.back_action);

	// Call this now to set proper inital state
	RulesChangeFunc (NULL);

	UI_FinishFramework (&m_startServerMenu.frameWork, qTrue);
}


/*
=============
StartServerMenu_Close
=============
*/
static struct sfx_s *StartServerMenu_Close (void)
{
	UI_FreeMapNames ();

	return uiMedia.sounds.menuOut;
}


/*
=============
StartServerMenu_Draw
=============
*/
static void StartServerMenu_Draw (void)
{
	float	y;

	// Initialize if necessary
	if (!m_startServerMenu.frameWork.initialized)
		StartServerMenu_Init ();

	// Dynamically position
	m_startServerMenu.frameWork.x			= uiState.vidWidth * 0.5f;
	m_startServerMenu.frameWork.y			= 0;

	m_startServerMenu.banner.generic.x		= 0;
	m_startServerMenu.banner.generic.y		= 0;

	y = m_startServerMenu.banner.height * UI_SCALE;

	m_startServerMenu.startmap_list.generic.x		= 0;
	m_startServerMenu.startmap_list.generic.y		= y += UIFT_SIZE;
	m_startServerMenu.rules_box.generic.x			= 0;
	m_startServerMenu.rules_box.generic.y			= y += (UIFT_SIZEINC*2);
	m_startServerMenu.timelimit_field.generic.x		= 0;
	m_startServerMenu.timelimit_field.generic.y		= y += (UIFT_SIZEINC*2);
	m_startServerMenu.fraglimit_field.generic.x		= 0;
	m_startServerMenu.fraglimit_field.generic.y		= y += (UIFT_SIZEINC*2);
	m_startServerMenu.maxclients_field.generic.x	= 0;
	m_startServerMenu.maxclients_field.generic.y	= y += (UIFT_SIZEINC*2);
	m_startServerMenu.hostname_field.generic.x		= 0;
	m_startServerMenu.hostname_field.generic.y		= y += (UIFT_SIZEINC*2);
	m_startServerMenu.dmflags_action.generic.x		= 0;
	m_startServerMenu.dmflags_action.generic.y		= y += (UIFT_SIZEINC*2);
	m_startServerMenu.start_action.generic.x		= 0;
	m_startServerMenu.start_action.generic.y		= y += (UIFT_SIZEINC);
	m_startServerMenu.back_action.generic.x			= 0;
	m_startServerMenu.back_action.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCLG;

	// Render
	UI_CenterMenuHeight (&m_startServerMenu.frameWork);
	UI_SetupMenuBounds (&m_startServerMenu.frameWork);

	UI_AdjustCursor (&m_startServerMenu.frameWork, 1);
	UI_Draw (&m_startServerMenu.frameWork);
}


/*
=============
StartServerMenu_Key
=============
*/
static struct sfx_s *StartServerMenu_Key (int key)
{
	return DefaultMenu_Key (&m_startServerMenu.frameWork, key);
}


/*
=============
UI_StartServerMenu_f
=============
*/
void UI_StartServerMenu_f (void)
{
	StartServerMenu_Init ();
	UI_PushMenu (&m_startServerMenu.frameWork, StartServerMenu_Draw, StartServerMenu_Close, StartServerMenu_Key);
}
