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

typedef struct {
	uiFrameWork_t	framework;

	uiImage_t		banner;

	uiAction_t		start_action;
	uiAction_t		dmflags_action;
	uiField_t		timelimit_field;
	uiField_t		fraglimit_field;
	uiField_t		maxclients_field;
	uiField_t		hostname_field;
	uiList_t		startmap_list;
	uiList_t		rules_box;

	uiAction_t		back_action;
} m_startservermenu_t;

m_startservermenu_t m_startserver_menu;

void DMFlagsFunc (void *self) {
	self = self;

	if (m_startserver_menu.rules_box.curvalue == 1)
		return;

	UI_DMFlagsMenu_f ();
}

void RulesChangeFunc (void *self) {
	self = self;

	// DM
	if (m_startserver_menu.rules_box.curvalue == 0) {
		m_startserver_menu.maxclients_field.generic.statusbar = NULL;
		m_startserver_menu.dmflags_action.generic.statusbar = NULL;
	} else if(m_startserver_menu.rules_box.curvalue == 1) {		// coop				// PGM
		m_startserver_menu.maxclients_field.generic.statusbar = "4 maximum for cooperative";
		if (atoi (m_startserver_menu.maxclients_field.buffer) > 4)
			strcpy (m_startserver_menu.maxclients_field.buffer, "4");
		m_startserver_menu.dmflags_action.generic.statusbar = "N/A for cooperative";
	}

	// ROGUE
	else if(uii.FS_CurrGame ("rogue")) {
		if (m_startserver_menu.rules_box.curvalue == 2)			// tag	
		{
			m_startserver_menu.maxclients_field.generic.statusbar = NULL;
			m_startserver_menu.dmflags_action.generic.statusbar = NULL;
		}
	}
}

void StartServerActionFunc (void *self) {
	char	startmap[1024];
	int		timelimit;
	int		fraglimit;
	int		maxclients;
	char	*spot;

	self = self;
	strcpy (startmap, strchr (mapnames[m_startserver_menu.startmap_list.curvalue], '\n') + 1);

	maxclients  = atoi (m_startserver_menu.maxclients_field.buffer);
	timelimit	= atoi (m_startserver_menu.timelimit_field.buffer);
	fraglimit	= atoi (m_startserver_menu.fraglimit_field.buffer);

	uii.Cvar_SetValue ("maxclients",	clamp (maxclients, 0, maxclients));
	uii.Cvar_SetValue ("timelimit",		clamp (timelimit, 0, timelimit));
	uii.Cvar_SetValue ("fraglimit",		clamp (fraglimit, 0, fraglimit));
	uii.Cvar_Set ("hostname",			m_startserver_menu.hostname_field.buffer);

	if ((m_startserver_menu.rules_box.curvalue < 2) || (!uii.FS_CurrGame ("rogue"))) {
		uii.Cvar_SetValue ("deathmatch",	!m_startserver_menu.rules_box.curvalue);
		uii.Cvar_SetValue ("coop",			m_startserver_menu.rules_box.curvalue);
		uii.Cvar_SetValue ("gamerules",		0);
	} else {
		uii.Cvar_SetValue ("deathmatch",	1);	// deathmatch is always true for rogue games, right?
		uii.Cvar_SetValue ("coop",			0);
		uii.Cvar_SetValue ("gamerules",		m_startserver_menu.rules_box.curvalue);
	}

	spot = NULL;
	if (m_startserver_menu.rules_box.curvalue == 1) {
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
		if (uis.serverState)
			uii.Cbuf_AddText ("disconnect\n");
		uii.Cbuf_AddText (va ("gamemap \"*%s$%s\"\n", startmap, spot));
	} else
		uii.Cbuf_AddText (va("map %s\n", startmap));

	UI_ForceMenuOff ();
}


/*
=============
StartServerMenu_Init
=============
*/
void StartServerMenu_Init (void) {
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
	float	y;
	FILE	*fp;

	/*
	** load the list of map names
	*/
	Com_sprintf (mapsname, sizeof (mapsname), "%s/maps.lst", uii.FS_Gamedir());
	if ((fp = fopen (mapsname, "rb")) == 0) {
		if ((length = uii.FS_LoadFile ("maps.lst", (void **) &buffer)) == -1)
			uii.Com_Error (ERR_DROP, "couldn't find maps.lst\n");
	} else {
		fseek (fp, 0, SEEK_END);
		length = ftell (fp);

		fseek (fp, 0, SEEK_SET);
		buffer = malloc (length);
		fread (buffer, length, 1, fp);
	}

	s = buffer;

	i = 0;
	while (i < length) {
		if (s[i] == '\r')
			nummaps++;
		i++;
	}

	if (nummaps == 0)
		uii.Com_Error (ERR_DROP, "no maps in maps.lst\n");

	mapnames = malloc (sizeof (char *) * (nummaps + 1));
	memset (mapnames, 0, sizeof (char *) * (nummaps + 1));

	s = buffer;

	for (i=0 ; i<nummaps ; i++) {
		char  scratch[200];
		int		j, l;

		strcpy (shortname, Com_Parse (&s));
		l = strlen (shortname);
		for (j=0 ; j<l ; j++)
			shortname[j] = toupper(shortname[j]);

		strcpy (longname, Com_Parse (&s));
		Com_sprintf (scratch, sizeof (scratch), "%s\n%s", longname, shortname);

		mapnames[i] = malloc (strlen (scratch) + 1);
		strcpy (mapnames[i], scratch);
	}

	mapnames[nummaps] = 0;

	if (fp != 0) {
		fp = 0;
		uii.free (buffer);
	} else
		uii.FS_FreeFile (buffer);

	/*
	** initialize the menu stuff
	*/
	m_startserver_menu.framework.x		= uis.vidWidth * 0.5;
	m_startserver_menu.framework.y		= 0;
	m_startserver_menu.framework.nitems	= 0;

	UI_Banner (&m_startserver_menu.banner, uiMedia.startServerBanner, &y);

	m_startserver_menu.startmap_list.generic.type	= UITYPE_SPINCONTROL;
	m_startserver_menu.startmap_list.generic.x		= 0;
	m_startserver_menu.startmap_list.generic.y		= y += UIFT_SIZE;
	m_startserver_menu.startmap_list.generic.name	= "Initial map";
	m_startserver_menu.startmap_list.itemnames		= mapnames;

	m_startserver_menu.rules_box.generic.type	= UITYPE_SPINCONTROL;
	m_startserver_menu.rules_box.generic.x		= 0;
	m_startserver_menu.rules_box.generic.y		= y += (UIFT_SIZEINC*2);
	m_startserver_menu.rules_box.generic.name	= "Rules";
	
	if (uii.FS_CurrGame ("rogue"))
		m_startserver_menu.rules_box.itemnames = dm_coop_names_rogue;
	else
		m_startserver_menu.rules_box.itemnames = dm_coop_names;

	if (uii.Cvar_VariableInteger("coop"))
		m_startserver_menu.rules_box.curvalue = 1;
	else
		m_startserver_menu.rules_box.curvalue = 0;

	m_startserver_menu.rules_box.generic.callback = RulesChangeFunc;

	m_startserver_menu.timelimit_field.generic.type			= UITYPE_FIELD;
	m_startserver_menu.timelimit_field.generic.name			= "Time limit";
	m_startserver_menu.timelimit_field.generic.flags		= UIF_NUMBERSONLY;
	m_startserver_menu.timelimit_field.generic.x			= 0;
	m_startserver_menu.timelimit_field.generic.y			= y += (UIFT_SIZEINC*2);
	m_startserver_menu.timelimit_field.generic.statusbar	= "0 = no limit";
	m_startserver_menu.timelimit_field.length				= 3;
	m_startserver_menu.timelimit_field.visible_length		= 3;
	strcpy (m_startserver_menu.timelimit_field.buffer, uii.Cvar_VariableString("timelimit"));

	m_startserver_menu.fraglimit_field.generic.type			= UITYPE_FIELD;
	m_startserver_menu.fraglimit_field.generic.name			= "Frag limit";
	m_startserver_menu.fraglimit_field.generic.flags		= UIF_NUMBERSONLY;
	m_startserver_menu.fraglimit_field.generic.x			= 0;
	m_startserver_menu.fraglimit_field.generic.y			= y += (UIFT_SIZEINC*2);
	m_startserver_menu.fraglimit_field.generic.statusbar	= "0 = no limit";
	m_startserver_menu.fraglimit_field.length				= 3;
	m_startserver_menu.fraglimit_field.visible_length		= 3;
	strcpy (m_startserver_menu.fraglimit_field.buffer, uii.Cvar_VariableString("fraglimit"));

	/*
	** maxclients determines the maximum number of players that can join
	** the game.  If maxclients is only "1" then we should default the menu
	** option to 8 players, otherwise use whatever its current value is. 
	** Clamping will be done when the server is actually started.
	*/
	m_startserver_menu.maxclients_field.generic.type	= UITYPE_FIELD;
	m_startserver_menu.maxclients_field.generic.name	= "Max players";
	m_startserver_menu.maxclients_field.generic.flags	= UIF_NUMBERSONLY;
	m_startserver_menu.maxclients_field.generic.x		= 0;
	m_startserver_menu.maxclients_field.generic.y		= y += (UIFT_SIZEINC*2);
	m_startserver_menu.maxclients_field.length			= 3;
	m_startserver_menu.maxclients_field.visible_length	= 3;

	if (uii.Cvar_VariableInteger ("maxclients") == 1)
		strcpy (m_startserver_menu.maxclients_field.buffer, "8");
	else 
		strcpy (m_startserver_menu.maxclients_field.buffer, uii.Cvar_VariableString("maxclients"));

	m_startserver_menu.hostname_field.generic.type		= UITYPE_FIELD;
	m_startserver_menu.hostname_field.generic.name		= "Hostname";
	m_startserver_menu.hostname_field.generic.flags		= 0;
	m_startserver_menu.hostname_field.generic.x			= 0;
	m_startserver_menu.hostname_field.generic.y			= y += (UIFT_SIZEINC*2);
	m_startserver_menu.hostname_field.length			= 12;
	m_startserver_menu.hostname_field.visible_length	= 12;
	strcpy (m_startserver_menu.hostname_field.buffer, uii.Cvar_VariableString("hostname"));

	m_startserver_menu.dmflags_action.generic.type		= UITYPE_ACTION;
	m_startserver_menu.dmflags_action.generic.name		= "Deathmatch flags";
	m_startserver_menu.dmflags_action.generic.flags		= UIF_LEFT_JUSTIFY;
	m_startserver_menu.dmflags_action.generic.x			= 0;
	m_startserver_menu.dmflags_action.generic.y			= y += (UIFT_SIZEINC*2);
	m_startserver_menu.dmflags_action.generic.callback	= DMFlagsFunc;

	m_startserver_menu.start_action.generic.type		= UITYPE_ACTION;
	m_startserver_menu.start_action.generic.name		= "Begin";
	m_startserver_menu.start_action.generic.flags		= UIF_LEFT_JUSTIFY;
	m_startserver_menu.start_action.generic.x			= 0;
	m_startserver_menu.start_action.generic.y			= y += (UIFT_SIZEINC);
	m_startserver_menu.start_action.generic.callback	= StartServerActionFunc;

	m_startserver_menu.back_action.generic.type			= UITYPE_ACTION;
	m_startserver_menu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_startserver_menu.back_action.generic.x			= 0;
	m_startserver_menu.back_action.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCLG;
	m_startserver_menu.back_action.generic.name			= "< Back";
	m_startserver_menu.back_action.generic.callback		= Back_Menu;
	m_startserver_menu.back_action.generic.statusbar	= "Back a menu";

	UI_AddItem (&m_startserver_menu.framework,		&m_startserver_menu.banner);

	UI_AddItem (&m_startserver_menu.framework,		&m_startserver_menu.startmap_list);
	UI_AddItem (&m_startserver_menu.framework,		&m_startserver_menu.rules_box);
	UI_AddItem (&m_startserver_menu.framework,		&m_startserver_menu.timelimit_field);
	UI_AddItem (&m_startserver_menu.framework,		&m_startserver_menu.fraglimit_field);
	UI_AddItem (&m_startserver_menu.framework,		&m_startserver_menu.maxclients_field);
	UI_AddItem (&m_startserver_menu.framework,		&m_startserver_menu.hostname_field);
	UI_AddItem (&m_startserver_menu.framework,		&m_startserver_menu.dmflags_action);
	UI_AddItem (&m_startserver_menu.framework,		&m_startserver_menu.start_action);

	UI_AddItem (&m_startserver_menu.framework,		&m_startserver_menu.back_action);

	UI_Center (&m_startserver_menu.framework);
	UI_Setup (&m_startserver_menu.framework);

	// call this now to set proper inital state
	RulesChangeFunc (NULL);
}


/*
=============
StartServerMenu_Draw
=============
*/
void StartServerMenu_Draw (void) {
	UI_Center (&m_startserver_menu.framework);
	UI_Setup (&m_startserver_menu.framework);

	UI_AdjustTextCursor (&m_startserver_menu.framework, 1);
	UI_Draw (&m_startserver_menu.framework);
}


/*
=============
StartServerMenu_Key
=============
*/
const char *StartServerMenu_Key (int key) {
	if (key == K_ESCAPE) {
		if (mapnames) {
			int i;

			for (i=0 ; i<nummaps ; i++)
				free (mapnames[i]);
			free (mapnames);
		}

		mapnames = 0;
		nummaps = 0;
	}

	return DefaultMenu_Key (&m_startserver_menu.framework, key);
}


/*
=============
UI_StartServerMenu_f
=============
*/
void UI_StartServerMenu_f (void) {
	StartServerMenu_Init ();
	UI_PushMenu (&m_startserver_menu.framework, StartServerMenu_Draw, StartServerMenu_Key);
}
