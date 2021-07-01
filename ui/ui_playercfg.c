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

#define MAX_PLAYERMODELS	512
#define MAX_PLAYERSKINS		256

typedef struct modelInfo_s {
	int		numSkins;
	char	**skinDisplayNames;

	char	displayName[16];
	char	directory[MAX_QPATH];

	// Pointers
	struct model_s	*model;
	struct shader_s	**shaders;
	struct shader_s	**iconShaders;
} modelInfo_t;

typedef struct m_playerConfigMenu_s {
	// Player model info
	qBool				modelsFound;

	modelInfo_t			modelInfo[MAX_PLAYERMODELS];
	char				*modelNames[MAX_PLAYERMODELS];
	int					numPlayerModels;

	// Menu items
	uiFrameWork_t		frameWork;

	uiImage_t			banner;
	uiAction_t			header;

	uiField_t			nameField;
	uiList_t			modelList;
	uiList_t			skinList;
	uiList_t			handednessList;
	uiList_t			rateList;

	uiAction_t			backAction;
} m_playerConfigMenu_t;

static m_playerConfigMenu_t m_playerConfigMenu;

static int rateTable[] = {
	2500,
	3200,
	5000,
	10000,
	25000,
	0
};

static void HandednessCallback (void *unused)
{
	uii.Cvar_SetValue ("hand", m_playerConfigMenu.handednessList.curValue);
}

static void RateCallback (void *unused)
{
	if (m_playerConfigMenu.rateList.curValue != sizeof (rateTable) / sizeof (*rateTable) - 1)
		uii.Cvar_SetValue ("rate", rateTable[m_playerConfigMenu.rateList.curValue]);
}

static void ModelCallback (void *unused)
{
	if (m_playerConfigMenu.modelList.curValue >= m_playerConfigMenu.numPlayerModels)
		m_playerConfigMenu.modelList.curValue = m_playerConfigMenu.numPlayerModels-1;

	m_playerConfigMenu.skinList.itemNames = m_playerConfigMenu.modelInfo[m_playerConfigMenu.modelList.curValue].skinDisplayNames;
	m_playerConfigMenu.skinList.curValue = 0;
}


/*
=============
PlayerConfig_ScanDirectories
=============
*/
static qBool PlayerConfig_ScanDirectories (void)
{
	char	*modelList[MAX_PLAYERMODELS];
	char	*skinList[MAX_PLAYERSKINS];
	char	**modelSkins;
	char	scratch[1024];
	char	directory[1024];
	int		numModels;
	int		numSkins;
	int		numModelSkins;
	int		i, j, k;
	char	*p;

	// Model list
	numModels = uii.FS_FindFiles ("players", "players/*/tris.md*", "md*", modelList, MAX_PLAYERMODELS, qFalse, qTrue);
	if (!numModels)
		return qFalse;

	for (i=0 ; i<numModels ; i++) {
		// skip duplicates
		for (j=i+1 ; j<numModels ; j++) {
			if (strlen (modelList[i]) != strlen (modelList[j]))
				continue;
			if (!Q_strnicmp (modelList[i], modelList[j], strlen (modelList[i])-3))
				break;
		}

		if (j != numModels)
			continue;

		// Get the directory
		Q_strncpyz (directory, modelList[i], sizeof (directory));
		p = strstr (directory, "/tris.md");
		if (p)
			*p = '\0';

		// Get the skins
		numSkins = uii.FS_FindFiles (directory, "*.pcx", NULL, skinList, MAX_PLAYERSKINS, qFalse, qFalse);
		if (!skinList)
			continue;

		// Make sure it's valid (has a "_i" icon)
		for (j=0, numModelSkins=0 ; j<numSkins ; j++) {
			if (strstr (skinList[j], "_i.pcx"))
				continue;

			// Replace ".pcx" with "_i.pcx"
			Q_strncpyz (scratch, skinList[j], sizeof (scratch));
			p = strstr (scratch, ".pcx");
			if (p)
				*p = '\0';
			Q_strcatz (scratch, "_i.pcx", sizeof (scratch));

			for (k=0 ; k<numSkins ; k++) {
				if (!Q_stricmp (scratch, skinList[k]))
					break;
			}

			if (k == numSkins)
				continue;

			numModelSkins++;
		}

		if (!numModelSkins)
			continue;

		// Copy valid skins to list
		modelSkins = UI_MemAlloc (sizeof (char *) * (numModelSkins + 1));

		for (j=0, numModelSkins=0 ; j<numSkins ; j++) {
			if (strstr (skinList[j], "_i.pcx"))
				continue;

			// Replace ".pcx" with "_i.pcx"
			Q_strncpyz (scratch, skinList[j], sizeof (scratch));
			p = strstr (scratch, ".pcx");
			if (p)
				*p = '\0';
			Q_strcatz (scratch, "_i.pcx", sizeof (scratch));

			for (k=0 ; k<numSkins ; k++) {
				if (!Q_stricmp (scratch, skinList[k]))
					break;
			}

			if (k == numSkins)
				continue;

			Q_strncpyz (scratch, skinList[j]+8, sizeof (scratch));
			p = strchr (scratch, '/');
			if (!p)
				p = strchr (scratch, '\\');
			if (p)
				Q_strncpyz (scratch, p+1, sizeof (scratch));

			p = strstr (scratch, ".pcx");
			if (p)
				*p = '\0';

			modelSkins[numModelSkins++] = UI_StrDup (scratch);
		}

		// Free list
		UI_FS_FreeFileList (skinList, numSkins);

		// At this point we have a valid player model
		m_playerConfigMenu.modelInfo[m_playerConfigMenu.numPlayerModels].numSkins = numModelSkins;
		m_playerConfigMenu.modelInfo[m_playerConfigMenu.numPlayerModels].skinDisplayNames = modelSkins;

		// Find the model dir
		Q_strncpyz (scratch, directory+8, sizeof (scratch));

		Q_strncpyz (m_playerConfigMenu.modelInfo[m_playerConfigMenu.numPlayerModels].displayName, scratch, sizeof (m_playerConfigMenu.modelInfo[m_playerConfigMenu.numPlayerModels].displayName));
		Q_strncpyz (m_playerConfigMenu.modelInfo[m_playerConfigMenu.numPlayerModels].directory, scratch, sizeof (m_playerConfigMenu.modelInfo[m_playerConfigMenu.numPlayerModels].directory));

		// Register the model
		m_playerConfigMenu.modelInfo[m_playerConfigMenu.numPlayerModels].model = uii.R_RegisterModel (Q_VarArgs ("players/%s/tris.md2",
		m_playerConfigMenu.modelInfo[m_playerConfigMenu.numPlayerModels].directory));

		p = UI_MemAlloc (sizeof (struct shader_s *) * numModelSkins * 2);
		m_playerConfigMenu.modelInfo[m_playerConfigMenu.numPlayerModels].shaders = (struct shader_s **)p;
		p += sizeof (struct shader_s *) * numModelSkins;
		m_playerConfigMenu.modelInfo[m_playerConfigMenu.numPlayerModels].iconShaders = (struct shader_s **)p;

		for (j=0 ; j<numModelSkins ; j++) {
			m_playerConfigMenu.modelInfo[m_playerConfigMenu.numPlayerModels].shaders[j] = uii.R_RegisterSkin (Q_VarArgs ("players/%s/%s.pcx",
			m_playerConfigMenu.modelInfo[m_playerConfigMenu.numPlayerModels].directory,
			m_playerConfigMenu.modelInfo[m_playerConfigMenu.numPlayerModels].skinDisplayNames[j]));

			m_playerConfigMenu.modelInfo[m_playerConfigMenu.numPlayerModels].iconShaders[j] = uii.R_RegisterPic (Q_VarArgs ("players/%s/%s_i.pcx", 
			m_playerConfigMenu.modelInfo[m_playerConfigMenu.numPlayerModels].directory,
			m_playerConfigMenu.modelInfo[m_playerConfigMenu.numPlayerModels].skinDisplayNames[j]));
		}

		// Next
		m_playerConfigMenu.numPlayerModels++;
	}

	// Free
	UI_FS_FreeFileList (modelList, numModels);

	return (m_playerConfigMenu.numPlayerModels > 0);
}


/*
=============
PlayerConfigMenu_Init
=============
*/
static int pmicmpfnc (const void *_a, const void *_b)
{
	const modelInfo_t *a = (const modelInfo_t *) _a;
	const modelInfo_t *b = (const modelInfo_t *) _b;

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
static void PlayerConfigMenu_Init (void)
{
	char	currentDirectory[1024];
	int		currentDirectoryIndex;
	char	currentSkin[1024];
	int		currentSkinIndex;
	int		i, j;

	static char *rateNames[] = {
		"28.8 Modem",
		"33.6 Modem",
		"Single ISDN",
		"Dual ISDN/Cable",
		"T1/LAN",
		"User defined",
		0
	};

	static char *handedness[] = {
		"right",
		"left",
		"center",
		0
	};

	currentDirectoryIndex = 0;
	currentSkinIndex = 0;

	// clamp "hand"
	if (uii.Cvar_VariableInteger ("hand") < 0 || uii.Cvar_VariableInteger ("hand") > 2)
		uii.Cvar_SetValue ("hand", 0);

	// find current directory and skin
	Q_strncpyz (currentDirectory, uii.Cvar_VariableString ("skin"), sizeof (currentDirectory));
	if (strchr (currentDirectory, '/')) {
		Q_strncpyz (currentSkin, strchr (currentDirectory, '/') + 1, sizeof (currentSkin));
		*strchr (currentDirectory, '/' ) = 0;
	}
	else if (strchr (currentDirectory, '\\')) {
		Q_strncpyz (currentSkin, strchr (currentDirectory, '\\') + 1, sizeof (currentSkin));
		*strchr (currentDirectory, '\\') = 0;
	}
	else {
		Q_strncpyz (currentDirectory, "male", sizeof (currentDirectory));
		Q_strncpyz (currentSkin, "grunt", sizeof (currentSkin));
	}

	// sort
	qsort (m_playerConfigMenu.modelInfo, m_playerConfigMenu.numPlayerModels, sizeof (m_playerConfigMenu.modelInfo[0]), pmicmpfnc);

	// generate model and skin list
	memset (m_playerConfigMenu.modelNames, 0, sizeof (m_playerConfigMenu.modelNames));
	for (i=0 ; i<m_playerConfigMenu.numPlayerModels ; i++) {
		m_playerConfigMenu.modelNames[i] = m_playerConfigMenu.modelInfo[i].displayName;
		if (!Q_stricmp (m_playerConfigMenu.modelInfo[i].directory, currentDirectory)) {
			currentDirectoryIndex = i;

			for (j=0 ; j<m_playerConfigMenu.modelInfo[i].numSkins ; j++) {
				if (!Q_stricmp (m_playerConfigMenu.modelInfo[i].skinDisplayNames[j], currentSkin)) {
					currentSkinIndex = j;
					break;
				}
			}
		}
	}

	UI_StartFramework (&m_playerConfigMenu.frameWork);

	m_playerConfigMenu.banner.generic.type		= UITYPE_IMAGE;
	m_playerConfigMenu.banner.generic.flags		= UIF_NOSELECT|UIF_CENTERED;
	m_playerConfigMenu.banner.generic.name		= NULL;
	m_playerConfigMenu.banner.shader			= uiMedia.banners.multiplayer;

	m_playerConfigMenu.header.generic.type		= UITYPE_ACTION;
	m_playerConfigMenu.header.generic.flags		= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_playerConfigMenu.header.generic.name		= "Player Configuration";

	m_playerConfigMenu.nameField.generic.type		= UITYPE_FIELD;
	m_playerConfigMenu.nameField.generic.name		= "Name";
	m_playerConfigMenu.nameField.generic.callBack	= 0;
	m_playerConfigMenu.nameField.length				= 20;
	m_playerConfigMenu.nameField.visibleLength		= 20;
	Q_strncpyz (m_playerConfigMenu.nameField.buffer, uii.Cvar_VariableString ("name"), sizeof (m_playerConfigMenu.nameField.buffer));
	m_playerConfigMenu.nameField.cursor				= strlen (uii.Cvar_VariableString ("name"));

	if (m_playerConfigMenu.modelsFound) {
		m_playerConfigMenu.modelList.generic.type		= UITYPE_SPINCONTROL;
		m_playerConfigMenu.modelList.generic.name		= "Model";
		m_playerConfigMenu.modelList.generic.callBack	= ModelCallback;
		m_playerConfigMenu.modelList.curValue			= currentDirectoryIndex;
		m_playerConfigMenu.modelList.itemNames			= m_playerConfigMenu.modelNames;

		m_playerConfigMenu.skinList.generic.type		= UITYPE_SPINCONTROL;
		m_playerConfigMenu.skinList.generic.name		= "Skin";
		m_playerConfigMenu.skinList.generic.callBack	= 0;
		m_playerConfigMenu.skinList.curValue			= currentSkinIndex;
		m_playerConfigMenu.skinList.itemNames			= m_playerConfigMenu.modelInfo[currentDirectoryIndex].skinDisplayNames;
	}

	m_playerConfigMenu.handednessList.generic.type		= UITYPE_SPINCONTROL;
	m_playerConfigMenu.handednessList.generic.name		= "Hand";
	m_playerConfigMenu.handednessList.generic.callBack	= HandednessCallback;
	m_playerConfigMenu.handednessList.curValue			= uii.Cvar_VariableInteger ("hand");
	m_playerConfigMenu.handednessList.itemNames			= handedness;

	for (i=0 ; i < (sizeof (rateTable) / sizeof (*rateTable) - 1) ; i++)
		if (uii.Cvar_VariableValue ("rate") == rateTable[i])
			break;

	m_playerConfigMenu.rateList.generic.type		= UITYPE_SPINCONTROL;
	m_playerConfigMenu.rateList.generic.name		= "Speed";
	m_playerConfigMenu.rateList.generic.callBack	= RateCallback;
	m_playerConfigMenu.rateList.curValue			= i;
	m_playerConfigMenu.rateList.itemNames			= rateNames;

	m_playerConfigMenu.backAction.generic.type		= UITYPE_ACTION;
	m_playerConfigMenu.backAction.generic.flags		= UIF_SHADOW;
	m_playerConfigMenu.backAction.generic.name		= S_COLOR_GREEN"< back";
	m_playerConfigMenu.backAction.generic.callBack	= Menu_Pop;
	m_playerConfigMenu.backAction.generic.statusBar	= "Back a menu";

	UI_AddItem (&m_playerConfigMenu.frameWork,		&m_playerConfigMenu.banner);
	UI_AddItem (&m_playerConfigMenu.frameWork,		&m_playerConfigMenu.header);

	UI_AddItem (&m_playerConfigMenu.frameWork,		&m_playerConfigMenu.nameField);
	if (m_playerConfigMenu.modelsFound) {
		UI_AddItem (&m_playerConfigMenu.frameWork,		&m_playerConfigMenu.modelList);
		if (m_playerConfigMenu.skinList.itemNames)
			UI_AddItem (&m_playerConfigMenu.frameWork,	&m_playerConfigMenu.skinList);
	}

	UI_AddItem (&m_playerConfigMenu.frameWork,		&m_playerConfigMenu.handednessList);
	UI_AddItem (&m_playerConfigMenu.frameWork,		&m_playerConfigMenu.rateList);

	UI_AddItem (&m_playerConfigMenu.frameWork,		&m_playerConfigMenu.backAction);

	UI_FinishFramework (&m_playerConfigMenu.frameWork, qTrue);
}


/*
=============
PlayerConfigMenu_Close
=============
*/
struct sfx_s *PlayerConfigMenu_Close (void)
{
	int		i, j;

	// Set name
	uii.Cvar_Set ("name", m_playerConfigMenu.nameField.buffer);

	if (m_playerConfigMenu.modelsFound) {
		// Set skin
		uii.Cvar_Set ("skin", Q_VarArgs ("%s/%s", 
			m_playerConfigMenu.modelInfo[m_playerConfigMenu.modelList.curValue].directory, 
			m_playerConfigMenu.modelInfo[m_playerConfigMenu.modelList.curValue].skinDisplayNames[m_playerConfigMenu.skinList.curValue]));
	}

	// Free model lists
	for (i=0 ; i<m_playerConfigMenu.numPlayerModels ; i++) {
		// Free pointers
		UI_MemFree (m_playerConfigMenu.modelInfo[i].shaders);

		m_playerConfigMenu.modelInfo[i].shaders = NULL;
		m_playerConfigMenu.modelInfo[i].iconShaders = NULL;

		// Free skins
		for (j=0 ; j<m_playerConfigMenu.modelInfo[i].numSkins ; j++) {
			if (m_playerConfigMenu.modelInfo[i].skinDisplayNames[j])
				UI_MemFree (m_playerConfigMenu.modelInfo[i].skinDisplayNames[j]);
			m_playerConfigMenu.modelInfo[i].skinDisplayNames[j] = NULL;
		}

		// Free display names
		UI_MemFree (m_playerConfigMenu.modelInfo[i].skinDisplayNames);
		m_playerConfigMenu.modelInfo[i].skinDisplayNames = 0;
		m_playerConfigMenu.modelInfo[i].numSkins = 0;
	}

	m_playerConfigMenu.modelsFound = qFalse;
	memset (&m_playerConfigMenu.modelInfo, 0, sizeof (m_playerConfigMenu.modelInfo));
	memset (&m_playerConfigMenu.modelNames, 0, sizeof (m_playerConfigMenu.modelNames));
	m_playerConfigMenu.numPlayerModels = 0;

	return uiMedia.sounds.menuOut;
}


/*
=============
PlayerConfigMenu_Draw
=============
*/
void PlayerConfigMenu_Draw (void)
{
	static int	yaw;
	entity_t	entity[2];
	refDef_t	refDef;
	vec3_t		angles;
	struct		shader_s *icon;
	int			height;
	float		y, xoffset;

	// Initialize if necessary
	if (!m_playerConfigMenu.frameWork.initialized)
		PlayerConfigMenu_Init ();

	// Dynamically position
	xoffset = -(UIFT_SIZE * 9);

	m_playerConfigMenu.frameWork.x				= uiState.vidWidth * 0.5f;
	m_playerConfigMenu.frameWork.y				= m_playerConfigMenu.modelsFound ? -((4 * 6) * UIFT_SIZE) * 0.5f : 0;

	m_playerConfigMenu.banner.generic.x			= 0;
	m_playerConfigMenu.banner.generic.y			= 0;

	y = m_playerConfigMenu.banner.height * UI_SCALE;

	m_playerConfigMenu.header.generic.x			= 0;
	m_playerConfigMenu.header.generic.y			= y += UIFT_SIZEINC;

	m_playerConfigMenu.nameField.generic.x		= xoffset;
	m_playerConfigMenu.nameField.generic.y		= y += UIFT_SIZEINC + UIFT_SIZEINCMED;

	if (m_playerConfigMenu.modelsFound) {
		m_playerConfigMenu.modelList.generic.x	= xoffset;
		m_playerConfigMenu.modelList.generic.y	= y += (UIFT_SIZEINC*2);
		m_playerConfigMenu.skinList.generic.x	= xoffset;
		m_playerConfigMenu.skinList.generic.y	= y += UIFT_SIZEINC;
	}
	else {
		y += UIFT_SIZEINC;
	}

	m_playerConfigMenu.handednessList.generic.x	= xoffset;
	m_playerConfigMenu.handednessList.generic.y	= y += UIFT_SIZEINC;
	m_playerConfigMenu.rateList.generic.x		= xoffset;
	m_playerConfigMenu.rateList.generic.y		= y += UIFT_SIZEINC;
	m_playerConfigMenu.backAction.generic.x		= xoffset + UIFT_SIZE;
	m_playerConfigMenu.backAction.generic.y		= y += (UIFT_SIZEINC*2);

	UI_CenterMenuHeight (&m_playerConfigMenu.frameWork);
	UI_AdjustCursor (&m_playerConfigMenu.frameWork, 1);
	UI_Draw (&m_playerConfigMenu.frameWork);
	UI_SetupMenuBounds (&m_playerConfigMenu.frameWork);

	if (!m_playerConfigMenu.modelsFound) {
		char	str[64];

		Q_strncpyz (str, S_COLOR_RED S_STYLE_SHADOW "No player models found!", sizeof (str));
		UI_DrawString (
			(uiState.vidWidth-((strlen(str)-4)*UIFT_SIZELG)) * 0.5f,
			m_playerConfigMenu.frameWork.y + (y + UIFT_SIZEINC),
			0, UIFT_SCALELG, str, colorWhite);
		return;
	}

	// setup refDef
	memset (&refDef, 0, sizeof (refDef));

	refDef.x = (uiState.vidWidth * 0.5f) - (UIFT_SIZE * 10);
	refDef.y = m_playerConfigMenu.frameWork.y + (y + UIFT_SIZEINC);
	refDef.width = (UIFT_SIZE * 20);
	refDef.height = ((4 * 6) * UIFT_SIZE);
	refDef.fovX = refDef.fovY = 20.0f / UIFT_SCALE;
	refDef.time = uiState.time * 0.001f;

	if (++yaw > 360)
		yaw = 0;

	memset (entity, 0, sizeof (entity_t) * 2);

	// player
	entity[0].model = m_playerConfigMenu.modelInfo[m_playerConfigMenu.modelList.curValue].model;
	if (entity[0].model) {
		entity[0].skin = m_playerConfigMenu.modelInfo[m_playerConfigMenu.modelList.curValue].shaders[m_playerConfigMenu.skinList.curValue];

		if (entity[0].skin) {
			entity[0].origin[0] = entity[0].oldOrigin[0] = refDef.x;
			entity[0].origin[1] = entity[0].oldOrigin[1] = 0;
			entity[0].origin[2] = entity[0].oldOrigin[2] = 0;

			entity[0].color[0] = 255;
			entity[0].color[1] = 255;
			entity[0].color[2] = 255;
			entity[0].color[3] = 255;

			entity[0].flags = RF_DEPTHHACK|RF_NOSHADOW;
			entity[0].frame = 0;
			entity[0].oldFrame = 0;
			entity[0].backLerp = 0.0;
			entity[0].scale = 1;

			angles[0] = 0;
			angles[1] = yaw;
			angles[2] = 0;

			AnglesToAxis (angles, entity[0].axis);
		}
	}

	// weapon
	entity[1].model = uii.R_RegisterModel (Q_VarArgs ("players/%s/weapon.md2",
		m_playerConfigMenu.modelInfo[m_playerConfigMenu.modelList.curValue].directory));

	if (0) { //entity[1].model) {
		entity[1].origin[0] = entity[1].oldOrigin[0] = refDef.x;
		entity[1].origin[1] = entity[1].oldOrigin[1] = 0;
		entity[1].origin[2] = entity[1].oldOrigin[2] = 0;

		entity[1].color[0] = 255;
		entity[1].color[1] = 255;
		entity[1].color[2] = 255;
		entity[1].color[3] = 255;

		entity[1].flags = RF_DEPTHHACK|RF_NOSHADOW;
		entity[1].frame = 0;
		entity[1].oldFrame = 0;
		entity[1].backLerp = 0.0;
		entity[1].scale = 1;

		angles[0] = 0;
		angles[1] = yaw;
		angles[2] = 0;

		AnglesToAxis (angles, entity[1].axis);
	}

	refDef.areaBits = 0;
	refDef.rdFlags = RDF_NOWORLDMODEL;

	uii.R_ClearScene ();
	if (entity[0].skin) {
		uii.R_AddEntity (&entity[0]);
		if (entity[1].model)
			uii.R_AddEntity (&entity[1]);
	}

	height = uii.Cvar_VariableInteger ("gl_clear");
	if (height)
		uii.Cvar_ForceSetValue ("gl_clear", 0);

	uii.R_RenderScene (&refDef);

	if (height)
		uii.Cvar_ForceSetValue ("gl_clear", height);

	// pic selection
	icon = m_playerConfigMenu.modelInfo[m_playerConfigMenu.modelList.curValue].iconShaders[m_playerConfigMenu.skinList.curValue];
	if (icon) {
		int		width;
		int		height;

		uii.R_GetImageSize (icon, &width, &height);
		UI_DrawPic (icon, refDef.x - (width * UIFT_SCALE), refDef.y,
					width*UIFT_SCALE, height*UIFT_SCALE, 0, 0, 1, 1, colorWhite);
	}
}


/*
=============
PlayerConfigMenu_Key
=============
*/
static struct sfx_s *PlayerConfigMenu_Key (int key)
{
	return DefaultMenu_Key (&m_playerConfigMenu.frameWork, key);
}


/*
=============
UI_PlayerConfigMenu_f
=============
*/
void UI_PlayerConfigMenu_f (void)
{
	float	midrow = (uiState.vidWidth*0.5) - (18*UIFT_SIZEMED);
	float	midcol = (uiState.vidHeight*0.5) - (3*UIFT_SIZEMED);

	// show box
	UI_DrawTextBox (midrow, midcol, UIFT_SCALEMED, 36, 4);
	UI_DrawString (midrow + (UIFT_SIZEMED*2), midcol + UIFT_SIZEMED, 0, UIFT_SCALEMED,		"       --- PLEASE WAIT! ---       ",	colorGreen);
	UI_DrawString (midrow + (UIFT_SIZEMED*2), midcol + (UIFT_SIZEMED*2), 0, UIFT_SCALEMED,	"Player models, skins and icons are",	colorGreen);
	UI_DrawString (midrow + (UIFT_SIZEMED*2), midcol + (UIFT_SIZEMED*3), 0, UIFT_SCALEMED,	"being loaded, and this might take ",		colorGreen);
	UI_DrawString (midrow + (UIFT_SIZEMED*2), midcol + (UIFT_SIZEMED*4), 0, UIFT_SCALEMED,	"a minute so, please be patient.   ",		colorGreen);

	uii.R_EndFrame ();	// the text box won't show up unless we do a buffer swap

	// load models
	m_playerConfigMenu.modelsFound = PlayerConfig_ScanDirectories ();
	PlayerConfigMenu_Init ();

	UI_PushMenu (&m_playerConfigMenu.frameWork, PlayerConfigMenu_Draw, PlayerConfigMenu_Close, PlayerConfigMenu_Key);
}