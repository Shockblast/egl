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
// cg_parse.c
//

#include "cg_local.h"

/*
================
CG_ParseClientinfo

Load the skin, icon, and model for a client
================
*/
void CG_ParseClientinfo (int player) {
	char			*s;
	clientInfo_t	*ci;

	s = cg.configStrings[player+CS_PLAYERSKINS];

	ci = &cg.clientInfo[player];

	glmChangePrediction = (cg.playerNum == player) ? qTrue : qFalse;

	CG_LoadClientinfo (ci, s);
}


/*
=================
CG_ParseConfigString
=================
*/
void CG_ParseConfigString (int num, char *str) {
	char	oldcs[MAX_QPATH];

	if ((num < 0) || (num >= MAX_CONFIGSTRINGS))
		Com_Error (ERR_DROP, "CG_ParseConfigString: bad num");

	strncpy (oldcs, cg.configStrings[num], sizeof (oldcs));
	oldcs[sizeof (oldcs) - 1] = 0;

	strcpy (cg.configStrings[num], str);

	// do something apropriate
	if ((num >= CS_LIGHTS) && (num < CS_LIGHTS+MAX_CS_LIGHTSTYLES)) {
		// lightstyle
		CG_SetLightstyle (num-CS_LIGHTS);
	}
	else if ((num >= CS_MODELS) && (num < CS_MODELS+MAX_CS_MODELS)) {
		// model
		cg.modelDraw[num-CS_MODELS] = cgi.Mod_RegisterModel (cg.configStrings[num]);
		if (cg.configStrings[num][0] == '*')
			cg.modelClip[num-CS_MODELS] = CGI_CM_InlineModel (cg.configStrings[num]);
		else
			cg.modelClip[num-CS_MODELS] = NULL;
	}
	else if ((num >= CS_SOUNDS) && (num < CS_SOUNDS+MAX_CS_SOUNDS)) {
		// sound
		cgi.Snd_RegisterSound (cg.configStrings[num]);
	}
	else if ((num >= CS_IMAGES) && (num < CS_IMAGES+MAX_CS_IMAGES)) {
		// image
		CG_RegisterPic (cg.configStrings[num]);
	}
	else if ((num >= CS_PLAYERSKINS) && (num < CS_PLAYERSKINS+MAX_CS_CLIENTS)) {
		// skin
		if (strcmp (oldcs, str))
			CG_ParseClientinfo (num-CS_PLAYERSKINS);
	}
}

/*
==============================================================

	SERVER MESSAGE PARSING

==============================================================
*/

static qBool	inParseSequence = qFalse;

/*
==============
CG_StartServerMessage

Called by the client BEFORE all server messages have been parsed
==============
*/
void CG_StartServerMessage (void) {
	inParseSequence = qTrue;
}


/*
==============
CG_EndServerMessage

Called by the client AFTER all server messages have been parsed
==============
*/
void CG_EndServerMessage (float realTime) {
	inParseSequence = qFalse;

	cgs.realTime = realTime;

	SCR_UpdatePING ();
}


/*
==============
CG_ParseServerMessage

Parses command operations known to the game dll
Returns qTrue if the message was parsed
==============
*/
qBool CG_ParseServerMessage (int command) {
	switch (command) {
	case SVC_INVENTORY:
		Inv_ParseInventory ();
		return qTrue;

	case SVC_LAYOUT:
		HUD_CopyLayout ();
		return qTrue;

	case SVC_MUZZLEFLASH:
		CG_ParseMuzzleFlash ();
		return qTrue;

	case SVC_MUZZLEFLASH2:
		CG_ParseMuzzleFlash2 ();
		return qTrue;

	case SVC_TEMP_ENTITY:
		CG_ParseTempEnt ();
		return qTrue;

	default:
		return qFalse;
	}
}
