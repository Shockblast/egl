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
// cg_players.c
//

#include "cg_local.h"

char	cg_weaponModels[MAX_CLIENTWEAPONMODELS][MAX_CFGSTRLEN];
int		cg_numWeaponModels;

/*
================
CG_GloomClassForModel
================
*/
int CG_GloomClassForModel (char *model, char *skin)
{
	char		modelName[MAX_CFGSTRLEN];

	if (strlen (model) < 2 || strlen (skin) < 2)
		return GLM_DEFAULT;

	Q_strncpyz (modelName, model, sizeof (modelName));
	Q_strlwr (modelName);

	// Choptimize!
	switch (model[0]) {
	case 'B':
	case 'b':
		if (!strcmp (model, "breeder"))				return GLM_BREEDER;
		break;

	case 'D':
	case 'd':
		if (!strcmp (model, "drone"))				return GLM_DRONE;
		break;

	case 'E':
	case 'e':
		if (!strcmp (model, "engineer"))			return GLM_ENGINEER;
		if (!strcmp (model, "exterm"))				return GLM_EXTERM;
		break;

	case 'F':
	case 'f':
		if (!strcmp (model, "female"))				return GLM_BIOTECH;
		break;

	case 'G':
	case 'g':
		if (!strcmp (model, "guardian"))			return GLM_GUARDIAN;
		break;

	case 'H':
	case 'h':
		if (!strcmp (model, "hatch")) {
			if (!strcmp (skin, "kam"))				return GLM_KAMIKAZE;
			else									return GLM_HATCHLING;
		}
		else if (!strcmp (model, "hsold"))			return GLM_HT;
		break;

	case 'M':
	case 'm':
		if (!strcmp (model, "male")) {
			if (!strcmp (skin, "commando"))			return GLM_COMMANDO;
			else if (!strcmp (skin, "shotgun"))		return GLM_ST;
			else if (!strcmp (skin, "soldier"))		return GLM_GRUNT;
			else									return GLM_OBSERVER;
		}
		else if (!strcmp (model, "mech"))			return GLM_MECH;
		break;

	case 'S':
	case 's':
		if (!strcmp (model, "stalker"))				return GLM_STALKER;
		else if (!strcmp (model, "stinger"))		return GLM_STINGER;
		break;

	case 'W':
	case 'w':
		if (!strcmp (model, "wraith"))				return GLM_WRAITH;
		break;
	}

	return GLM_DEFAULT;
}


/*
================
CG_LoadClientinfo
================
*/
void CG_LoadClientinfo (clientInfo_t *ci, char *skin)
{
	char		modelName[MAX_CFGSTRLEN];
	char		skinName[MAX_CFGSTRLEN];
	char		modelFilename[MAX_QPATH];
	char		skinFilename[MAX_QPATH];
	char		weaponFilename[MAX_QPATH];
	char		*t;
	int			i;

	Q_strncpyz (ci->cInfo, skin, sizeof (ci->cInfo));

	// Isolate the player's name
	Q_strncpyz (ci->name, skin, sizeof (ci->name));
	t = strstr (skin, "\\");
	if (t) {
		ci->name[t-skin] = 0;
		skin = t+1;
	}

	if (cl_noskins->integer || *skin == 0) {
		// Model
		Q_snprintfz (modelFilename, sizeof (modelFilename), "players/male/tris.md2");
		ci->model = cgi.R_RegisterModel (modelFilename);

		// Weapon
		Q_snprintfz (weaponFilename, sizeof (weaponFilename), "players/male/weapon.md2");
		memset (ci->weaponModels, 0, sizeof (ci->weaponModels));
		ci->weaponModels[0] = cgi.R_RegisterModel (weaponFilename);

		// Skin
		Q_snprintfz (skinFilename, sizeof (skinFilename), "players/male/grunt.tga");
		ci->skin = cgi.R_RegisterSkin (skinFilename);

		// Icon
		Q_snprintfz (ci->iconName, sizeof (ci->iconName), "players/male/grunt_i.tga");
		ci->icon = cgi.R_RegisterPic (ci->iconName);
	}
	else {
		// Isolate the model name
		Q_strncpyz (modelName, skin, sizeof (modelName));
		t = strstr (modelName, "/");
		if (!t)
			t = strstr (modelName, "\\");
		if (!t)
			t = modelName;
		*t = 0;

		// Isolate the skin name
		Q_strncpyz (skinName, skin+strlen(modelName)+1, sizeof (skinName));

		// Find out gloom class
		if (cg.gloomCheckClass) {
			cg.gloomClassType = GLM_DEFAULT;
			cg.gloomCheckClass = qFalse;
			if (cg.currGameMod == GAME_MOD_GLOOM)
				cg.gloomClassType = CG_GloomClassForModel (modelName, skinName);
		}

		// Model file
		Q_snprintfz (modelFilename, sizeof (modelFilename), "players/%s/tris.md2", modelName);
		ci->model = cgi.R_RegisterModel (modelFilename);
		if (!ci->model) {
			Q_strncpyz (modelName, "male", sizeof (modelName));
			Q_snprintfz (modelFilename, sizeof (modelFilename), "players/male/tris.md2");
			ci->model = cgi.R_RegisterModel (modelFilename);
		}

		// Skin file
		Q_snprintfz (skinFilename, sizeof (skinFilename), "players/%s/%s.tga", modelName, skinName);
		ci->skin = cgi.R_RegisterSkin (skinFilename);

		// If we don't have the skin and the model wasn't male, see if the male has it (this is for CTF's skins)
		if (!ci->skin && Q_stricmp (modelName, "male")) {
			// Change model to male
			Q_strncpyz (modelName, "male", sizeof (modelName));
			Q_snprintfz (modelFilename, sizeof (modelFilename), "players/male/tris.md2");
			ci->model = cgi.R_RegisterModel (modelFilename);

			// See if the skin exists for the male model
			Q_snprintfz (skinFilename, sizeof (skinFilename), "players/%s/%s.tga", modelName, skinName);
			ci->skin = cgi.R_RegisterSkin (skinFilename);
		}

		// If we still don't have a skin, it means that the male model didn't have it, so default to grunt
		if (!ci->skin) {
			// See if the skin exists for the male model
			Q_snprintfz (skinFilename, sizeof (skinFilename), "players/%s/grunt.tga", modelName, skinName);
			ci->skin = cgi.R_RegisterSkin (skinFilename);
		}

		// Weapon file
		for (i=0 ; i<cg_numWeaponModels ; i++) {
			Q_snprintfz (weaponFilename, sizeof (weaponFilename), "players/%s/%s", modelName, cg_weaponModels[i]);
			ci->weaponModels[i] = cgi.R_RegisterModel (weaponFilename);
			if (!ci->weaponModels[i] && !Q_stricmp (modelName, "cyborg")) {
				// Try male
				Q_snprintfz (weaponFilename, sizeof (weaponFilename), "players/male/%s", cg_weaponModels[i]);
				ci->weaponModels[i] = cgi.R_RegisterModel (weaponFilename);
			}

			if (!cl_vwep->integer)
				break; // Only one when vwep is off
		}

		// Icon file
		Q_snprintfz (ci->iconName, sizeof (ci->iconName), "players/%s/%s_i.tga", modelName, skinName);
		ci->icon = cgi.R_RegisterPic (ci->iconName);
	}

	// Must have loaded all data types to be valud
	if (!ci->skin || !ci->icon || !ci->model || !ci->weaponModels[0]) {
		ci->skin = NULL;
		ci->icon = NULL;
		ci->model = NULL;
		ci->weaponModels[0] = NULL;
		return;
	}
}


/*
==============
CG_StartSound
==============
*/
void CG_StartSound (vec3_t origin, int entNum, entChannel_t entChannel, int soundNum, float volume, float attenuation, float timeOffset)
{
	if (!cg.soundCfgStrings[soundNum] && cg.configStrings[CS_SOUNDS+soundNum][0])
		cg.soundCfgStrings[soundNum] = cgi.Snd_RegisterSound (cg.configStrings[CS_SOUNDS+soundNum]);

	// FIXME: move sexed sound registration here
	cgi.Snd_StartSound (origin, entNum, entChannel, cg.soundCfgStrings[soundNum], volume, attenuation, timeOffset);
}


/*
==============
CG_FixUpGender
==============
*/
void CG_FixUpGender (void)
{
	char	*p, sk[80];

	if (!gender_auto->value)
		return;

	if (gender->modified) {
		// Was set directly, don't override the user
		gender->modified = qFalse;
		return;
	}

	Q_strncpyz (sk, skin->string, sizeof (sk));
	if ((p = strchr (sk, '/')) != NULL)
		*p = 0;

	if (!Q_stricmp (sk, "male") || !Q_stricmp (sk, "cyborg"))
		cgi.Cvar_Set ("gender", "male", qFalse);
	else if (!Q_stricmp (sk, "female") || !Q_stricmp (sk, "crackhor"))
		cgi.Cvar_Set ("gender", "female", qFalse);
	else
		cgi.Cvar_Set ("gender", "none", qFalse);
	gender->modified = qFalse;
}
