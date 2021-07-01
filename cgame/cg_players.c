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

char	cgWeaponModels[MAX_CLIENTWEAPONMODELS][MAX_QPATH];
int		cgNumWeaponModels;

qBool	glmChangePrediction = qFalse;
int		glmClassType = GLM_DEFAULT;

/*
================
CG_LoadClientinfo
================
*/
void CG_LoadClientinfo (clientInfo_t *ci, char *s) {
	int			i;
	char		*t;
	char		model_name[MAX_QPATH];
	char		skin_name[MAX_QPATH];
	char		model_filename[MAX_QPATH];
	char		skin_filename[MAX_QPATH];
	char		weapon_filename[MAX_QPATH];

	Q_strncpyz (ci->cInfo, s, sizeof (ci->cInfo));

	// isolate the player's name
	Q_strncpyz (ci->name, s, sizeof (ci->name));
	t = strstr (s, "\\");

	if (t) {
		ci->name[t-s] = 0;
		s = t+1;
	}

	if (cl_noskins->integer || (*s == 0)) {
		Q_snprintfz (model_filename, sizeof (model_filename),	"players/male/tris.md2");
		Q_snprintfz (weapon_filename, sizeof (weapon_filename),	"players/male/weapon.md2");
		Q_snprintfz (skin_filename, sizeof (skin_filename),		"players/male/grunt.tga");
		Q_snprintfz (ci->iconName, sizeof (ci->iconName),		"players/male/grunt_i.tga");

		ci->model = cgi.Mod_RegisterModel (model_filename);
		memset (ci->weaponModel, 0, sizeof (ci->weaponModel));
		ci->weaponModel[0] = cgi.Mod_RegisterModel (weapon_filename);
		ci->skin = cgi.Img_RegisterImage (skin_filename, 0);
		ci->icon = CG_RegisterPic (ci->iconName);
	}
	else {
		// isolate the model name
		Q_strncpyz (model_name, s, sizeof (model_name));
		t = strstr (model_name, "/");
		if (!t)
			t = strstr(model_name, "\\");
		if (!t)
			t = model_name;
		*t = 0;

		// isolate the skin name
		Q_strncpyz (skin_name, s + Q_strlen (model_name) + 1, sizeof (skin_name));

		// jump prediction hack
		if (cgi.FS_CurrGame ("gloom")) {
			if (glmChangePrediction) {
				glmClassType = GLM_DEFAULT;

				if (!strcmp (model_name,"male")) {
					if (!strcmp (skin_name, "commando"))		glmClassType = GLM_COMMANDO;
					else if (!strcmp (skin_name, "shotgun"))	glmClassType = GLM_ST;
					else if (!strcmp (skin_name, "soldier"))	glmClassType = GLM_GRUNT;
					else										glmClassType = GLM_OBSERVER;
				}
				else if (!strcmp (model_name, "female"))		glmClassType = GLM_BIOTECH;
				else if (!strcmp (model_name, "hatch")) {
					if (!strcmp (skin_name, "kam"))				glmClassType = GLM_KAMIKAZE;
					else										glmClassType = GLM_HATCHLING;
				}
				else if (!strcmp (model_name, "breeder"))		glmClassType = GLM_BREEDER;
				else if (!strcmp (model_name, "engineer"))		glmClassType = GLM_ENGINEER;
				else if (!strcmp (model_name, "drone"))			glmClassType = GLM_DRONE;
				else if (!strcmp (model_name, "wraith"))		glmClassType = GLM_WRAITH;
				else if (!strcmp (model_name, "stinger"))		glmClassType = GLM_STINGER;
				else if (!strcmp (model_name, "guardian"))		glmClassType = GLM_GUARDIAN;
				else if (!strcmp (model_name, "stalker"))		glmClassType = GLM_STALKER;
				else if (!strcmp (model_name, "hsold"))			glmClassType = GLM_HT;
				else if (!strcmp (model_name, "exterm"))		glmClassType = GLM_EXTERM;
				else if (!strcmp (model_name, "mech"))			glmClassType = GLM_MECH;
				else											glmClassType = GLM_DEFAULT;

				glmChangePrediction = qFalse;
			}
		}
		else
			glmClassType = GLM_DEFAULT;

		// model file
		Q_snprintfz (model_filename, sizeof (model_filename), "players/%s/tris.md2", model_name);
		ci->model = cgi.Mod_RegisterModel (model_filename);
		if (!ci->model) {
			Q_strncpyz (model_name, "male", sizeof (model_name));
			Q_snprintfz (model_filename, sizeof (model_filename), "players/male/tris.md2");
			ci->model = cgi.Mod_RegisterModel (model_filename);
		}

		// skin file
		Q_snprintfz (skin_filename, sizeof (skin_filename), "players/%s/%s.tga", model_name, skin_name);
		ci->skin = cgi.Img_RegisterImage (skin_filename, 0);

		// if we don't have the skin and the model wasn't male, see if the male has it (this is for CTF's skins)
		if (!ci->skin && Q_stricmp (model_name, "male")) {
			// change model to male
			Q_strncpyz (model_name, "male", sizeof (model_name));
			Q_snprintfz (model_filename, sizeof (model_filename), "players/male/tris.md2");
			ci->model = cgi.Mod_RegisterModel (model_filename);

			// see if the skin exists for the male model
			Q_snprintfz (skin_filename, sizeof (skin_filename), "players/%s/%s.tga", model_name, skin_name);
			ci->skin = cgi.Img_RegisterImage (skin_filename, 0);
		}

		// if we still don't have a skin, it means that the male model didn't have it, so default to grunt
		if (!ci->skin) {
			// see if the skin exists for the male model
			Q_snprintfz (skin_filename, sizeof (skin_filename), "players/%s/grunt.tga", model_name, skin_name);
			ci->skin = cgi.Img_RegisterImage (skin_filename, 0);
		}

		// weapon file
		for (i=0 ; i<cgNumWeaponModels ; i++) {
			Q_snprintfz (weapon_filename, sizeof (weapon_filename), "players/%s/%s", model_name, cgWeaponModels[i]);
			ci->weaponModel[i] = cgi.Mod_RegisterModel (weapon_filename);
			if (!ci->weaponModel[i] && strcmp (model_name, "cyborg") == 0) {
				// try male
				Q_snprintfz (weapon_filename, sizeof (weapon_filename), "players/male/%s", cgWeaponModels[i]);
				ci->weaponModel[i] = cgi.Mod_RegisterModel (weapon_filename);
			}

			if (!cl_vwep->integer)
				break; // only one when vwep is off
		}

		// icon file
		Q_snprintfz (ci->iconName, sizeof (ci->iconName), "/players/%s/%s_i.tga", model_name, skin_name);
		ci->icon = CG_RegisterPic (ci->iconName);
	}

	// must have loaded all data types to be valud
	if (!ci->skin || !ci->icon || !ci->model || !ci->weaponModel[0]) {
		ci->skin = NULL;
		ci->icon = NULL;
		ci->model = NULL;
		ci->weaponModel[0] = NULL;
		return;
	}
}


/*
==============
CG_FixUpGender
==============
*/
void CG_FixUpGender (void) {
	if (gender_auto->value) {
		char	*p, sk[80];
		if (gender->modified) {
			// was set directly, don't override the user
			gender->modified = qFalse;
			return;
		}

		Q_strncpyz (sk, skin->string, sizeof (sk));
		if ((p = strchr (sk, '/')) != NULL)
			*p = 0;

		if (!Q_stricmp (sk, "male") || !Q_stricmp (sk, "cyborg"))
			cgi.Cvar_Set ("gender", "male");
		else if (!Q_stricmp (sk, "female") || !Q_stricmp (sk, "crackhor"))
			cgi.Cvar_Set ("gender", "female");
		else
			cgi.Cvar_Set ("gender", "none");
		gender->modified = qFalse;
	}
}
