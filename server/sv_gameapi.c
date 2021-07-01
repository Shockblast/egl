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
// sv_gameapi.c
// Interface to the game dll
//

#include "sv_local.h"

gameExport_t	*ge;

/*
================
SV_FindIndex
================
*/
static int SV_FindIndex (char *name, int start, int max, qBool create)
{
	int		i;

	if (!name || !name[0])
		return 0;

	for (i=1 ; i<max && sv.configStrings[start+i][0] ; i++) {
		if (!strcmp(sv.configStrings[start+i], name))
			return i;
	}

	if (!create)
		return 0;

	if (i == max)
		Com_Error (ERR_DROP, "*Index: overflow");

	strncpy (sv.configStrings[start+i], name, sizeof (sv.configStrings[i]));

	if (sv.state != SS_LOADING) {
		// Send the update to everyone
		MSG_Clear (&sv.multiCast);
		MSG_WriteChar (&sv.multiCast, SVC_CONFIGSTRING);
		MSG_WriteShort (&sv.multiCast, start+i);
		MSG_WriteString (&sv.multiCast, name);
		SV_Multicast (vec3Origin, MULTICAST_ALL_R);
	}

	return i;
}

/*
=============================================================================

	GAME IMPORT API

=============================================================================
*/

static int GI_ModelIndex (char *name) { return SV_FindIndex (name, CS_MODELS, MAX_CS_MODELS, qTrue); }
static int GI_SoundIndex (char *name) { return SV_FindIndex (name, CS_SOUNDS, MAX_CS_SOUNDS, qTrue); }
static int GI_ImageIndex (char *name) { return SV_FindIndex (name, CS_IMAGES, MAX_CS_IMAGES, qTrue); }

// ==========================================================================

/*
=================
GI_SetModel

Also sets mins and maxs for inline bmodels
=================
*/
static void GI_SetModel (edict_t *ent, char *name)
{
	int			i;
	cBspModel_t	*mod;

	if (!name)
		Com_Error (ERR_DROP, "GI_SetModel: NULL");

	i = GI_ModelIndex (name);

	ent->s.modelIndex = i;

	// If it is an inline model, get the size information for it
	if (name[0] == '*') {
		mod = CM_InlineModel (name);
		VectorCopy (mod->mins, ent->mins);
		VectorCopy (mod->maxs, ent->maxs);
		SV_LinkEdict (ent);
	}
}


/*
===============
GI_ConfigString
===============
*/
static void GI_ConfigString (int index, char *val)
{
	if (index < 0 || index >= MAX_CONFIGSTRINGS)
		Com_Error (ERR_DROP, "configstring: bad index %i\n", index);

	if (!val)
		val = "";

	// Change the string in sv
	strcpy (sv.configStrings[index], val);

	if (sv.state != SS_LOADING) {
		// Send the update to everyone
		MSG_Clear (&sv.multiCast);
		MSG_WriteChar (&sv.multiCast, SVC_CONFIGSTRING);
		MSG_WriteShort (&sv.multiCast, index);
		MSG_WriteString (&sv.multiCast, val);

		SV_Multicast (vec3Origin, MULTICAST_ALL_R);
	}
}

// ==========================================================================

static void GI_WriteChar (int c)		{ MSG_WriteChar (&sv.multiCast, c); }
static void GI_WriteByte (int c)		{ MSG_WriteByte (&sv.multiCast, c); }
static void GI_WriteShort (int c)		{ MSG_WriteShort (&sv.multiCast, c); }
static void GI_WriteLong (int c)		{ MSG_WriteLong (&sv.multiCast, c); }
static void GI_WriteFloat (float f)		{ MSG_WriteFloat (&sv.multiCast, f); }
static void GI_WriteString (char *s)	{ MSG_WriteString (&sv.multiCast, s); }
static void GI_WritePos (vec3_t pos)	{ MSG_WritePos (&sv.multiCast, pos); }
static void GI_WriteDir (vec3_t dir)	{ MSG_WriteDir (&sv.multiCast, dir); }
static void GI_WriteAngle (float f)		{ MSG_WriteAngle (&sv.multiCast, f); }

// ==========================================================================

/*
=================
GI_IsInPVS

Also checks portalareas so that doors block sight
=================
*/
static qBool GI_IsInPVS (vec3_t p1, vec3_t p2)
{
	int		leafnum;
	int		cluster;
	int		area1, area2;
	byte	*mask;

	leafnum = CM_PointLeafnum (p1);
	cluster = CM_LeafCluster (leafnum);
	area1 = CM_LeafArea (leafnum);
	mask = CM_ClusterPVS (cluster);

	leafnum = CM_PointLeafnum (p2);
	cluster = CM_LeafCluster (leafnum);
	area2 = CM_LeafArea (leafnum);

	if (mask && (!(mask[cluster>>3] & (1<<(cluster&7)))))
		return qFalse;

	if (!CM_AreasConnected (area1, area2))
		return qFalse;		// A door blocks sight

	return qTrue;
}


/*
=================
GI_IsInPHS

Also checks portalareas so that doors block sound
=================
*/
static qBool GI_IsInPHS (vec3_t p1, vec3_t p2)
{
	int		leafnum;
	int		cluster;
	int		area1, area2;
	byte	*mask;

	leafnum = CM_PointLeafnum (p1);
	cluster = CM_LeafCluster (leafnum);
	area1 = CM_LeafArea (leafnum);
	mask = CM_ClusterPHS (cluster);

	leafnum = CM_PointLeafnum (p2);
	cluster = CM_LeafCluster (leafnum);
	area2 = CM_LeafArea (leafnum);

	if (mask && (!(mask[cluster>>3] & (1<<(cluster&7)))))
		return qFalse;		// More than one bounce away

	if (!CM_AreasConnected (area1, area2))
		return qFalse;		// A door blocks hearing

	return qTrue;
}

// ==========================================================================

/*
=================
GI_StartSound
=================
*/
static void GI_StartSound (edict_t *entity, int channel, int soundNum, float volume, float attenuation, float timeOffset)
{
	if (!entity)
		return;

	SV_StartSound (NULL, entity, channel, soundNum, volume, attenuation, timeOffset);
}

// ==========================================================================

/*
===============
GI_DevPrintf

Debug print to server console
===============
*/
static void GI_DevPrintf (char *fmt, ...)
{
	char		msg[1024];
	va_list		argptr;

	va_start (argptr, fmt);
	vsnprintf (msg, sizeof (msg), fmt, argptr);
	va_end (argptr);

	Com_Printf (0, "%s", msg);
}


/*
===============
GI_CPrintf

Print to a single client
===============
*/
static void GI_CPrintf (edict_t *ent, int level, char *fmt, ...)
{
	char		msg[1024];
	va_list		argptr;
	int			n = 0;

	if (ent) {
		n = NUM_FOR_EDICT(ent);
		if (n < 1 || n > maxclients->integer)
			Com_Error (ERR_DROP, "cprintf to a non-client");
	}

	va_start (argptr, fmt);
	vsnprintf (msg, sizeof (msg), fmt, argptr);
	va_end (argptr);

	if (ent)
		SV_ClientPrintf (svs.clients+(n-1), level, "%s", msg);
	else
		Com_Printf (0, "%s", msg);
}


/*
===============
GI_CenterPrintf

centerprint to a single client
===============
*/
static void GI_CenterPrintf (edict_t *ent, char *fmt, ...)
{
	char		msg[1024];
	va_list		argptr;
	int			n;

	n = NUM_FOR_EDICT(ent);
	if ((n < 1) || (n > maxclients->integer))
		return;

	va_start (argptr,fmt);
	vsnprintf (msg, sizeof (msg), fmt, argptr);
	va_end (argptr);

	MSG_WriteByte (&sv.multiCast, SVC_CENTERPRINT);
	MSG_WriteString (&sv.multiCast, msg);
	SV_Unicast (ent, qTrue);
}


/*
===============
GI_Error

Abort the server with a game error
===============
*/
static void GI_Error (char *fmt, ...)
{
	char		msg[1024];
	va_list		argptr;

	va_start (argptr,fmt);
	vsnprintf (msg, sizeof (msg), fmt, argptr);
	va_end (argptr);

	Com_Error (ERR_DROP, "Game Error: %s", msg);
}


/*
=================
GI_TagAlloc

Makes sure the game DLL does not use client, or signed tags
=================
*/
static void *GI_TagAlloc (int size, int tagNum)
{
	if (tagNum < 0)
		tagNum *= -1;

	return _Mem_PoolAlloc (size, MEMPOOL_GAMESYS, tagNum, "GAME DLL", -1);
}

/*
=================
GI_MemFree
=================
*/
static void GI_MemFree (const void *ptr)
{
	_Mem_Free (ptr, "GAME DLL", -1);
}


/*
=================
GI_FreeTags

Makes sure the game DLL does not use client, or signed tags
=================
*/
static void GI_FreeTags (int tagNum)
{
	if (tagNum < 0)
		tagNum *= -1;

	_Mem_FreeTags (MEMPOOL_GAMESYS, tagNum, "GAME DLL", -1);
}


/*
===============
GI_Pmove
===============
*/
static void GI_Pmove (pMove_t *pMove)
{
	pMoveNew_t	epm;

	memcpy (&epm, pMove, sizeof (pMove_t));
	epm.cmd = pMove->cmd;
	epm.groundEntity = pMove->groundEntity;
	epm.pointContents = pMove->pointContents;
	epm.trace = pMove->trace;
	epm.multiplier = 1;

#ifdef ENHANCED_SERVER
	if ((svCurrClient->protocol == ENHANCED_PROTOCOL_VERSION) && (pMove->state.pmType == PMT_SPECTATOR))
		epm.multiplier = 2;
#endif

	if (!CL_CGModule_Pmove (&epm, sv_airAcceleration))
		SV_Pmove (&epm, sv_airAcceleration);

	pMove->groundEntity = epm.groundEntity;
	memcpy (pMove, &epm, sizeof (pMove_t));
	memcpy (pMove->touchEnts, &epm.touchEnts, sizeof (pMove->touchEnts));
}

// ==========================================================================

/*
===============
SV_GameAPI_Init

Init the game subsystem for a new map
===============
*/
void SCR_DebugGraph (float value, int color);
void SV_GameAPI_Init (void)
{
	gameImport_t	gi;
	int				gInitTime;

	gInitTime = Sys_Milliseconds ();

	SV_GameAPI_Shutdown ();	// Shutdown if already active

	Com_Printf (PRNT_DEV, "Game Initialization start\n");

	// Load a new game dll
	gi.multicast			= SV_Multicast;
	gi.unicast				= SV_Unicast;
	gi.bprintf				= SV_BroadcastPrintf;
	gi.dprintf				= GI_DevPrintf;
	gi.cprintf				= GI_CPrintf;
	gi.centerprintf			= GI_CenterPrintf;
	gi.error				= GI_Error;

	gi.linkentity			= SV_LinkEdict;
	gi.unlinkentity			= SV_UnlinkEdict;
	gi.BoxEdicts			= SV_AreaEdicts;
	gi.trace				= SV_Trace;
	gi.pointcontents		= SV_PointContents;
	gi.setmodel				= GI_SetModel;
	gi.inPVS				= GI_IsInPVS;
	gi.inPHS				= GI_IsInPHS;
	gi.Pmove				= GI_Pmove;

	gi.modelindex			= GI_ModelIndex;
	gi.soundindex			= GI_SoundIndex;
	gi.imageindex			= GI_ImageIndex;

	gi.configstring			= GI_ConfigString;
	gi.sound				= GI_StartSound;
	gi.positioned_sound		= SV_StartSound;

	gi.WriteChar			= GI_WriteChar;
	gi.WriteByte			= GI_WriteByte;
	gi.WriteShort			= GI_WriteShort;
	gi.WriteLong			= GI_WriteLong;
	gi.WriteFloat			= GI_WriteFloat;
	gi.WriteString			= GI_WriteString;
	gi.WritePosition		= GI_WritePos;
	gi.WriteDir				= GI_WriteDir;
	gi.WriteAngle			= GI_WriteAngle;

	gi.TagMalloc			= GI_TagAlloc;
	gi.TagFree				= GI_MemFree;
	gi.FreeTags				= GI_FreeTags;

	gi.cvar					= Cvar_Get;
	gi.cvar_set				= Cvar_Set;
	gi.cvar_forceset		= Cvar_ForceSet;

	gi.argc					= Cmd_Argc;
	gi.argv					= Cmd_Argv;
	gi.args					= Cmd_Args;
	gi.AddCommandString		= Cbuf_AddText;

	gi.DebugGraph			= SCR_DebugGraph;
	gi.SetAreaPortalState	= CM_SetAreaPortalState;
	gi.AreasConnected		= CM_AreasConnected;

	ge = (gameExport_t *) Sys_LoadLibrary (LIB_GAME, &gi);
	if (!ge)
		Com_Error (ERR_DROP, "Failed to load game DLL!");

	if (ge->apiVersion != GAME_APIVERSION) {
		Com_Error (ERR_DROP, "SV_GameAPI_Init: incompatible apiVersion (%i != %i)", ge->apiVersion, GAME_APIVERSION);

		Sys_UnloadLibrary (LIB_GAME);
		ge = NULL;
	}

	ge->Init ();
	Com_Printf (0, "Game Init time: %.2fs\n", (Sys_Milliseconds () - gInitTime)*0.001f);
}


/*
===============
SV_GameAPI_Shutdown

Called when either the entire server is being killed, or
it is changing to a different game directory.
===============
*/
void SV_GameAPI_Shutdown (void)
{
	if (!ge)
		return;

	// Tell the module to shutdown locally and unload dll
	ge->Shutdown ();
	Sys_UnloadLibrary (LIB_GAME);
	ge = NULL;

	// Free the memory pool
	Mem_FreePool (MEMPOOL_GAMESYS);
}
