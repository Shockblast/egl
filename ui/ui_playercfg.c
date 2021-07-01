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

	PLAYER CONFIG MENU

=============================================================================
*/

typedef struct m_plyrcfgmenu_s {
	uiFrameWork_t	framework;

	uiImage_t		banner;
	uiAction_t		header;

	uiField_t		name_field;
	uiList_t		model_list;
	uiList_t		skin_list;
	uiList_t		handedness_list;
	uiList_t		rate_list;

	uiAction_t		back_action;
} m_plyrcfgmenu_t;

m_plyrcfgmenu_t m_plyrcfg_menu;

#define MAX_DISPLAYNAME		16
#define MAX_PLAYERMODELS	1024

typedef struct playerModelInfo_s {
	int		numSkins;
	char	**skinDisplayNames;
	char	displayName[MAX_DISPLAYNAME];
	char	directory[MAX_QPATH];
} playerModelInfo_t;

static playerModelInfo_t	s_pmi[MAX_PLAYERMODELS];
static char					*s_pmnames[MAX_PLAYERMODELS];
static int					s_numplayermodels;

static void HandednessCallback (void *unused) {
	uii.Cvar_SetValue ("hand", m_plyrcfg_menu.handedness_list.curValue);
}

static int rateTable[] = {
	2500,
	3200,
	5000,
	10000,
	25000,
	0
};
static char *rateNames[] = {
	"28.8 Modem",
	"33.6 Modem",
	"Single ISDN",
	"Dual ISDN/Cable",
	"T1/LAN",
	"User defined",
	0
};

static void RateCallback (void *unused) {
	if (m_plyrcfg_menu.rate_list.curValue != sizeof (rateTable) / sizeof (*rateTable) - 1)
		uii.Cvar_SetValue ("rate", rateTable[m_plyrcfg_menu.rate_list.curValue]);
}

static void ModelCallback (void *unused) {
	m_plyrcfg_menu.skin_list.itemNames	= s_pmi[m_plyrcfg_menu.model_list.curValue].skinDisplayNames;
	m_plyrcfg_menu.skin_list.curValue	= 0;
}

static qBool IconOfSkinExists (char *skin, char **pcxfiles, int npcxfiles) {
	int i;
	char scratch[1024];

	strcpy (scratch, skin);
	*strrchr (scratch, '.') = 0;
	strcat (scratch, "_i.pcx");

	for (i=0 ; i<npcxfiles ; i++) {
		if (strcmp (pcxfiles[i], scratch) == 0)
			return qTrue;
	}

	return qFalse;
}

static void PlayerConfig_ScanDirectories (void) {
	char	findname[1024], scratch[1024];
	char	**directoryNames = NULL;
	char	*path = NULL;
	int		numDirectories = 0;
	int		numPlayerModels = 0;
	int		i;

	s_numplayermodels = 0;

	// get a list of directories
	do {
		path = uii.FS_NextPath (path);
		Q_snprintfz (findname, sizeof (findname), "%s/players/*.*", path);

		if ((directoryNames = uii.FS_ListFiles (findname, &numDirectories, SFF_SUBDIR, 0)) != 0)
			break;
	} while (path);

	if (!directoryNames)
		return;

	// go through the subdirectories
	numPlayerModels = numDirectories;
	if (numPlayerModels > MAX_PLAYERMODELS)
		numPlayerModels = MAX_PLAYERMODELS;

	for (i=0 ; i<numPlayerModels ; i++) {
		int		k, s;
		char	*a, *b, *c;
		char	**pcxnames, **skinnames;
		int		npcxfiles, numSkins = 0;

		if (directoryNames[i] == 0)
			continue;

		// verify the existence of tris.md2
		strcpy (scratch, directoryNames[i]);
		strcat (scratch, "/tris.md2");
		if (!uii.Sys_FindFirst (scratch, 0, SFF_SUBDIR|SFF_HIDDEN|SFF_SYSTEM)) {
			UI_MemFree (directoryNames[i]);
			directoryNames[i] = 0;
			uii.Sys_FindClose ();
			continue;
		}
		uii.Sys_FindClose ();

		// verify the existence of at least one pcx skin
		strcpy (scratch, directoryNames[i]);
		strcat (scratch, "/*.pcx");
		pcxnames = uii.FS_ListFiles (scratch, &npcxfiles, 0, SFF_SUBDIR|SFF_HIDDEN|SFF_SYSTEM);

		if (!pcxnames) {
			UI_MemFree (directoryNames[i]);
			directoryNames[i] = 0;
			continue;
		}

		// count valid skins, which consist of a skin with a matching "_i" icon
		for (k=0 ; k<npcxfiles-1 ; k++) {
			if (!strstr (pcxnames[k], "_i.pcx")) {
				if (IconOfSkinExists (pcxnames[k], pcxnames, npcxfiles - 1))
					numSkins++;
			}
		}
		if (!numSkins)
			continue;

		skinnames = UI_MemAlloc (sizeof (char *) * (numSkins + 1));

		// copy the valid skins
		for (s=0, k=0 ; k<npcxfiles-1 ; k++) {
			char *a, *b, *c;

			if (!strstr (pcxnames[k], "_i.pcx")) {
				if (IconOfSkinExists (pcxnames[k], pcxnames, npcxfiles - 1)) {
					a = strrchr (pcxnames[k], '/');
					b = strrchr (pcxnames[k], '\\');

					if (a > b)
						c = a;
					else
						c = b;

					strcpy (scratch, c + 1);

					if (strrchr (scratch, '.'))
						*strrchr (scratch, '.') = 0;

					skinnames[s] = UI_StrDup (scratch);

					s++;
				}
			}
		}

		// at this point we have a valid player model
		s_pmi[s_numplayermodels].numSkins = numSkins;
		s_pmi[s_numplayermodels].skinDisplayNames = skinnames;

		// make short name for the model
		a = strrchr (directoryNames[i], '/');
		b = strrchr (directoryNames[i], '\\');

		if (a > b)
			c = a;
		else
			c = b;

		strncpy (s_pmi[s_numplayermodels].displayName, c + 1, MAX_DISPLAYNAME-1);
		strcpy (s_pmi[s_numplayermodels].directory, c + 1);

		uii.FS_FreeFileList (pcxnames, npcxfiles);

		s_numplayermodels++;
	}

	if (directoryNames)
		uii.FS_FreeFileList (directoryNames, numPlayerModels);
}


/*
=============
PlayerConfigMenu_Init
=============
*/
static int pmicmpfnc (const void *_a, const void *_b) {
	const playerModelInfo_t *a = (const playerModelInfo_t *) _a;
	const playerModelInfo_t *b = (const playerModelInfo_t *) _b;

	/*
	** sort by male, female, then alphabetical
	*/
	if (strcmp (a->directory, "male") == 0)
		return -1;
	else if (strcmp (b->directory, "male") == 0)
		return 1;

	if (strcmp (a->directory, "female") == 0)
		return -1;
	else if (strcmp (b->directory, "female") == 0)
		return 1;

	return strcmp (a->directory, b->directory);
}

static float player_top, fd_height;
qBool PlayerConfigMenu_Init (void) {
	char	currentdirectory[1024];
	char	currentskin[1024];
	float	y, xoffset = -(UIFT_SIZE*9);
	int		i = 0;

	int currentdirectoryindex = 0;
	int currentskinindex = 0;

	static char *handedness[] = { "right", "left", "center", 0 };

	PlayerConfig_ScanDirectories ();

	if (s_numplayermodels == 0)
		return qFalse;

	if (uii.Cvar_VariableInteger ("hand") < 0 || uii.Cvar_VariableInteger ("hand") > 2)
		uii.Cvar_SetValue ("hand", 0);

	strcpy (currentdirectory, uii.Cvar_VariableString ("skin"));

	if (strchr (currentdirectory, '/')) {
		strcpy (currentskin, strchr (currentdirectory, '/') + 1);
		*strchr (currentdirectory, '/' ) = 0;
	} else if (strchr( currentdirectory, '\\')) {
		strcpy (currentskin, strchr (currentdirectory, '\\') + 1);
		*strchr (currentdirectory, '\\') = 0;
	} else {
		strcpy (currentdirectory, "male");
		strcpy (currentskin, "grunt");
	}

	qsort (s_pmi, s_numplayermodels, sizeof (s_pmi[0]), pmicmpfnc);

	memset (s_pmnames, 0, sizeof (s_pmnames));
	for (i=0 ; i<s_numplayermodels ; i++) {
		s_pmnames[i] = s_pmi[i].displayName;
		if (Q_stricmp (s_pmi[i].directory, currentdirectory) == 0) {
			int j;

			currentdirectoryindex = i;

			for (j=0 ; j<s_pmi[i].numSkins ; j++) {
				if (Q_stricmp (s_pmi[i].skinDisplayNames[j], currentskin) == 0) {
					currentskinindex = j;
					break;
				}
			}
		}
	}

	m_plyrcfg_menu.framework.x			= uis.vidWidth * 0.5;
	m_plyrcfg_menu.framework.y			= 0;
	m_plyrcfg_menu.framework.numItems	= 0;

	UI_Banner (&m_plyrcfg_menu.banner, uiMedia.multiplayerBanner, &y);

	m_plyrcfg_menu.header.generic.type		= UITYPE_ACTION;
	m_plyrcfg_menu.header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_plyrcfg_menu.header.generic.x			= 0;
	m_plyrcfg_menu.header.generic.y			= y += UIFT_SIZEINC;
	m_plyrcfg_menu.header.generic.name		= "Player Configuration";

	m_plyrcfg_menu.name_field.generic.type		= UITYPE_FIELD;
	m_plyrcfg_menu.name_field.generic.name		= "Name";
	m_plyrcfg_menu.name_field.generic.callBack	= 0;
	m_plyrcfg_menu.name_field.generic.x			= xoffset;
	m_plyrcfg_menu.name_field.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_plyrcfg_menu.name_field.length			= 20;
	m_plyrcfg_menu.name_field.visibleLength	= 20;
	strcpy (m_plyrcfg_menu.name_field.buffer, uii.Cvar_VariableString ("name"));
	m_plyrcfg_menu.name_field.cursor = strlen (uii.Cvar_VariableString ("name"));

	m_plyrcfg_menu.model_list.generic.type		= UITYPE_SPINCONTROL;
	m_plyrcfg_menu.model_list.generic.name		= "Model";
	m_plyrcfg_menu.model_list.generic.x			= xoffset;
	m_plyrcfg_menu.model_list.generic.y			= y += (UIFT_SIZEINC*2);
	m_plyrcfg_menu.model_list.generic.callBack	= ModelCallback;
	m_plyrcfg_menu.model_list.curValue			= currentdirectoryindex;
	m_plyrcfg_menu.model_list.itemNames			= s_pmnames;

	m_plyrcfg_menu.skin_list.generic.type		= UITYPE_SPINCONTROL;
	m_plyrcfg_menu.skin_list.generic.name		= "Skin";
	m_plyrcfg_menu.skin_list.generic.x			= xoffset;
	m_plyrcfg_menu.skin_list.generic.y			= y += UIFT_SIZEINC;
	m_plyrcfg_menu.skin_list.generic.callBack	= 0;
	m_plyrcfg_menu.skin_list.curValue			= currentskinindex;
	m_plyrcfg_menu.skin_list.itemNames			= s_pmi[currentdirectoryindex].skinDisplayNames;

	m_plyrcfg_menu.handedness_list.generic.type			= UITYPE_SPINCONTROL;
	m_plyrcfg_menu.handedness_list.generic.name			= "Hand";
	m_plyrcfg_menu.handedness_list.generic.x			= xoffset;
	m_plyrcfg_menu.handedness_list.generic.y			= y += UIFT_SIZEINC;
	m_plyrcfg_menu.handedness_list.generic.callBack		= HandednessCallback;
	m_plyrcfg_menu.handedness_list.curValue				= uii.Cvar_VariableInteger ("hand");
	m_plyrcfg_menu.handedness_list.itemNames			= handedness;

	for (i=0 ; i < (sizeof (rateTable) / sizeof (*rateTable) - 1) ; i++)
		if (uii.Cvar_VariableValue ("rate") == rateTable[i])
			break;

	m_plyrcfg_menu.rate_list.generic.type		= UITYPE_SPINCONTROL;
	m_plyrcfg_menu.rate_list.generic.name		= "Speed";
	m_plyrcfg_menu.rate_list.generic.x			= xoffset;
	m_plyrcfg_menu.rate_list.generic.y			= y += UIFT_SIZEINC;
	m_plyrcfg_menu.rate_list.generic.callBack	= RateCallback;
	m_plyrcfg_menu.rate_list.curValue			= i;
	m_plyrcfg_menu.rate_list.itemNames			= rateNames;

	m_plyrcfg_menu.back_action.generic.type			= UITYPE_ACTION;
	m_plyrcfg_menu.back_action.generic.flags		= UIF_SHADOW;
	m_plyrcfg_menu.back_action.generic.x			= xoffset + UIFT_SIZE;
	m_plyrcfg_menu.back_action.generic.y			= y += (UIFT_SIZEINC*2);
	m_plyrcfg_menu.back_action.generic.name			= S_COLOR_GREEN"< back";
	m_plyrcfg_menu.back_action.generic.callBack		= Back_Menu;
	m_plyrcfg_menu.back_action.generic.statusBar	= "Back a menu";

	UI_AddItem (&m_plyrcfg_menu.framework,		&m_plyrcfg_menu.banner);
	UI_AddItem (&m_plyrcfg_menu.framework,		&m_plyrcfg_menu.header);

	UI_AddItem (&m_plyrcfg_menu.framework,		&m_plyrcfg_menu.name_field);
	UI_AddItem (&m_plyrcfg_menu.framework,		&m_plyrcfg_menu.model_list);
	if (m_plyrcfg_menu.skin_list.itemNames)
		UI_AddItem (&m_plyrcfg_menu.framework,	&m_plyrcfg_menu.skin_list);

	UI_AddItem (&m_plyrcfg_menu.framework,		&m_plyrcfg_menu.handedness_list);
	UI_AddItem (&m_plyrcfg_menu.framework,		&m_plyrcfg_menu.rate_list);

	UI_AddItem (&m_plyrcfg_menu.framework,		&m_plyrcfg_menu.back_action);

	UI_Center (&m_plyrcfg_menu.framework);

	player_top = y;
	fd_height = ((4*5)*UIFT_SIZE);
	m_plyrcfg_menu.framework.y -= fd_height * 0.5;

	UI_Setup (&m_plyrcfg_menu.framework);

	return qTrue;
}


/*
=============
PlayerConfigMenu_Draw
=============
*/
void PlayerConfigMenu_Draw (void) {
	refDef_t	fd;
	char		scratch[MAX_QPATH];
	vec3_t		angles;
	struct		image_s *icon;

	memset (&fd, 0, sizeof (fd));

	fd.x = (uis.vidWidth * 0.5) - (UIFT_SIZE * 10);
	fd.y = (m_plyrcfg_menu.framework.y) + (player_top + UIFT_SIZEINC);
	fd.width = (UIFT_SIZE * 20);
	fd.height = fd_height;
	fd.fovX = fd.fovY = 15 / UIFT_SCALE;
	fd.time = uis.time * 0.001;

	if (s_pmi[m_plyrcfg_menu.model_list.curValue].skinDisplayNames) {
		static int	yaw;
		entity_t	entity[2];

		if (++yaw > 360)
			yaw = 0;

		// player
		memset (&entity[0], 0, sizeof (entity[0]));

		Q_snprintfz (scratch, sizeof (scratch), "players/%s/tris.md2", s_pmi[m_plyrcfg_menu.model_list.curValue].directory);
		entity[0].model = uii.Mod_RegisterModel (scratch);

		if (entity[0].model) {
			Q_snprintfz (scratch, sizeof (scratch), "players/%s/%s.pcx", s_pmi[m_plyrcfg_menu.model_list.curValue].directory, s_pmi[m_plyrcfg_menu.model_list.curValue].skinDisplayNames[m_plyrcfg_menu.skin_list.curValue]);
			entity[0].skin = uii.Img_RegisterSkin (scratch);

			if (entity[0].skin) {
				entity[0].flags = RF_DEPTHHACK|RF_NOSHADOW;
				Vector4Set (entity[0].color, 1, 1, 1, 1);
				VectorSet (entity[0].origin, fd.x, 0, 0);
				VectorCopy (entity[0].origin, entity[0].oldOrigin);
				entity[0].frame = 0;
				entity[0].oldFrame = 0;
				entity[0].backLerp = 0.0;
				entity[0].scale = 1;
				VectorSet (angles, 0, yaw, 0);

				if (angles[0] || angles[1] || angles[2])
					AnglesToAxis (angles, entity[0].axis);
				else
					Matrix_Identity (entity[0].axis);
			}
		}

		// weapon
		memset (&entity[1], 0, sizeof (entity[1]));

		Q_snprintfz (scratch, sizeof (scratch), "players/%s/weapon.md2", s_pmi[m_plyrcfg_menu.model_list.curValue].directory);
		entity[1].model = uii.Mod_RegisterModel (scratch);

		if (entity[1].model) {
			entity[1].flags = RF_DEPTHHACK|RF_NOSHADOW;
			Vector4Set (entity[1].color, 1, 1, 1, 1);
			VectorSet (entity[1].origin, fd.x, 0, 0);
			VectorCopy (entity[1].origin, entity[1].oldOrigin);
			entity[1].frame = 0;
			entity[1].oldFrame = 0;
			entity[1].backLerp = 0.0;
			VectorSet (angles, 0, yaw, 0);
			entity[1].scale = 1;

			if (angles[0] || angles[1] || angles[2])
				AnglesToAxis (angles, entity[1].axis);
			else
				Matrix_Identity (entity[1].axis);
		}

		fd.areaBits = 0;
		fd.rdFlags = RDF_NOWORLDMODEL;

		UI_AdjustTextCursor (&m_plyrcfg_menu.framework, 1);
		UI_Draw (&m_plyrcfg_menu.framework);

		uii.R_ClearScene ();
		if (entity[0].skin) {
			uii.R_AddEntity (&entity[0]);
			if (entity[1].model)
				uii.R_AddEntity (&entity[1]);
		}

		uii.R_RenderScene (&fd);

		// pic selection
		Q_snprintfz (scratch, sizeof (scratch), "/players/%s/%s_i.pcx", 
			s_pmi[m_plyrcfg_menu.model_list.curValue].directory,
			s_pmi[m_plyrcfg_menu.model_list.curValue].skinDisplayNames[m_plyrcfg_menu.skin_list.curValue]);

		icon = uii.Img_RegisterPic (scratch);
		if (icon) {
			int		width;
			int		height;

			uii.Img_GetSize (icon, &width, &height);
			UI_DrawPic (icon, fd.x - (width * UIFT_SCALE), fd.y,
								width*UIFT_SCALE, height*UIFT_SCALE, 0, 0, 1, 1, colorWhite);
		}
	}

	UI_Setup (&m_plyrcfg_menu.framework);
}


/*
=============
PlayerConfigMenu_Key
=============
*/
struct sfx_s *PlayerConfigMenu_Key (int key) {
	int i;

	if (key == K_ESCAPE) {
		char scratch[1024];

		uii.Cvar_Set ("name", m_plyrcfg_menu.name_field.buffer);

		Q_snprintfz (scratch, sizeof (scratch), "%s/%s", 
			s_pmi[m_plyrcfg_menu.model_list.curValue].directory, 
			s_pmi[m_plyrcfg_menu.model_list.curValue].skinDisplayNames[m_plyrcfg_menu.skin_list.curValue]);

		uii.Cvar_Set ("skin", scratch);

		for (i=0 ; i<s_numplayermodels ; i++) {
			int j;

			for (j=0 ; j<s_pmi[i].numSkins ; j++) {
				if (s_pmi[i].skinDisplayNames[j])
					UI_MemFree (s_pmi[i].skinDisplayNames[j]);
				s_pmi[i].skinDisplayNames[j] = 0;
			}

			UI_MemFree (s_pmi[i].skinDisplayNames);
			s_pmi[i].skinDisplayNames = 0;
			s_pmi[i].numSkins = 0;
		}
	}

	return DefaultMenu_Key (&m_plyrcfg_menu.framework, key);
}


/*
=============
UI_PlayerConfigMenu_f
=============
*/
void UI_PlayerConfigMenu_f (void) {
	if (!PlayerConfigMenu_Init ()) {
		Com_Printf (0, S_COLOR_RED "No valid player models found\n");
		return;
	}

	UI_SetStatusBar (&m_plyrcfg_menu.framework, NULL);
	UI_PushMenu (&m_plyrcfg_menu.framework, PlayerConfigMenu_Draw, PlayerConfigMenu_Key);
}