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
// cl_cgapi.c
//

#include "cl_local.h"

cgExport_t		*cge;

/*
=============================================================================

	CGAME EXPORT FUNCTIONS

=============================================================================
*/

/*
===============
CL_CGModule_Init
===============
*/
static void KeepAlive (void) {
	Netchan_Transmit (&cls.netChan, 0, NULL);
}
static void CL_CGModule_Init (void) {
	int			prepInitTime;

	prepInitTime = Sys_Milliseconds ();

	R_ClearScene (); // clear the scene so that old scene object pointers are cleared

	R_BeginRegistration ();
	Snd_BeginRegistration ();

	CL_InitImageMedia ();
	CL_InitSoundMedia ();
	SCR_UpdateScreen ();

	// init cgame and 'keep alive' to not timeout
	KeepAlive ();
	cge->Init (cl.playerNum, cl.attractLoop, vidDef.width, vidDef.height);
	KeepAlive ();

	R_InitMedia (); // because we can't register models before map load

	// clear any lines of console text
	Con_ClearNotify ();
	SCR_UpdateScreen ();

	CDAudio_Play (atoi (cl.configStrings[CS_CDTRACK]), qTrue); // start the cd track

	// the subsystems can now free unneeded stuff
	R_EndRegistration ();
	Snd_EndRegistration ();

	Sys_SendKeyEvents (); // pump message loop

	// all done!
	cl.refreshPrepped = qTrue;
	Com_Printf (0, "CGame Initialized in: %.2fs\n", (Sys_Milliseconds () - prepInitTime)*0.001f);
}


/*
===============
CL_CGModule_BeginFrameSequence
===============
*/
void CL_CGModule_BeginFrameSequence (void) {
	if (cge)
		cge->BeginFrameSequence (cl.frame);
}


/*
===============
CL_CGModule_NewPacketEntityState
===============
*/
void CL_CGModule_NewPacketEntityState (int entnum, entityState_t state) {
	if (cge)
		cge->NewPacketEntityState (entnum, state);
}


/*
===============
CL_CGModule_EndFrameSequence
===============
*/
void CL_CGModule_EndFrameSequence (void) {
	if (cge)
		cge->EndFrameSequence (cl.frame.numEntities);
}


/*
===============
CL_CGModule_GetEntitySoundOrigin
===============
*/
void CL_CGModule_GetEntitySoundOrigin (int entNum, vec3_t org) {
	if (cge)
		cge->GetEntitySoundOrigin (entNum, org);
}


/*
===============
CL_CGModule_ParseConfigString
===============
*/
void CL_CGModule_ParseConfigString (int num, char *str) {
	if (cge)
		cge->ParseConfigString (num, str);
}


/*
===============
CL_CGModule_ParseCenterPrint
===============
*/
void CL_CGModule_ParseCenterPrint (void) {
	if (cge)
		cge->ParseCenterPrint ();
}


/*
===============
CL_CGModule_StartServerMessage
===============
*/
void CL_CGModule_StartServerMessage (void) {
	if (cge)
		cge->StartServerMessage ();
}


/*
===============
CL_CGModule_ParseServerMessage
===============
*/
qBool CL_CGModule_ParseServerMessage (int command) {
	if (cge)
		return cge->ParseServerMessage (command);

	return qFalse;
}


/*
===============
CL_CGModule_EndServerMessage
===============
*/
void CL_CGModule_EndServerMessage (void) {
	if (cge)
		cge->EndServerMessage (cls.realTime);
}


/*
===============
CL_CGModule_Pmove
===============
*/
qBool CL_CGModule_Pmove (pMove_t *pMove, float airAcceleration) {
	if (cge)
		cge->Pmove (pMove, airAcceleration);
	else
		return qFalse;

	return qTrue;
}


/*
===============
CL_CGModule_RegisterSounds
===============
*/
void CL_CGModule_RegisterSounds (void) {
	if (cge)
		cge->RegisterSounds ();
}


/*
===============
CL_CGModule_RenderView
===============
*/
void CL_CGModule_RenderView (float stereoSeparation) {
	if (cge)
		cge->RenderView (vidDef.width, vidDef.height, cls.realTime, cls.trueFrameTime, stereoSeparation, qFalse);
}

/*
=============================================================================

	CGAME IMPORT API

=============================================================================
*/

/*
===============
CGI_NET_GetCurrentUserCmdNum
===============
*/
static int CGI_NET_GetCurrentUserCmdNum (void) {
	return cl.cmdNum;
}


/*
===============
CGI_NET_GetSequenceState
===============
*/
static void CGI_NET_GetSequenceState (int *outgoingSequence, int *incomingAcknowledged) {
	if (outgoingSequence)
		*outgoingSequence = cls.netChan.outgoingSequence;
	if (incomingAcknowledged)
		*incomingAcknowledged = cls.netChan.incomingAcknowledged;
}


/*
===============
CGI_NET_GetUserCmd
===============
*/
static void CGI_NET_GetUserCmd (int frame, userCmd_t *cmd) {
	if (cmd)
		*cmd = cl.cmds[frame & CMD_MASK];
}


/*
===============
CGI_NET_GetUserCmdTime
===============
*/
static int CGI_NET_GetUserCmdTime (int frame) {
	return cl.cmdTime[frame & CMD_MASK];
}

// ==========================================================================

/*
===============
CGI_GetConfigString
===============
*/
static void CGI_GetConfigString (int i, char *str, int size) {
	if ((i < 0) || (i >= MAX_CONFIGSTRINGS))
		Com_Printf (PRNT_DEV, S_COLOR_RED "CGI_GetConfigString: i > MAX_CONFIGSTRINGS");
	if (!str || size <= 0 )
		Com_Printf (PRNT_DEV, S_COLOR_RED "CGI_GetConfigString: NULL string");

	strncpy (str, cl.configStrings[i], size);
}

// ==========================================================================

static int CGI_MSG_ReadChar (void)			{ return MSG_ReadChar (&netMessage); }
static int CGI_MSG_ReadByte (void)			{ return MSG_ReadByte (&netMessage); }
static int CGI_MSG_ReadShort (void)			{ return MSG_ReadShort (&netMessage); }
static int CGI_MSG_ReadLong (void)			{ return MSG_ReadLong (&netMessage); }
static float CGI_MSG_ReadFloat (void)		{ return MSG_ReadFloat (&netMessage); }
static void CGI_MSG_ReadDir (vec3_t dir)	{ MSG_ReadDir (&netMessage, dir); }
static void CGI_MSG_ReadPos (vec3_t pos)	{ MSG_ReadPos (&netMessage, pos); }
static char *CGI_MSG_ReadString (void)		{ return MSG_ReadString (&netMessage); }

// ==========================================================================

/*
===============
CGI_R_RenderScene
===============
*/
static void CGI_R_RenderScene (refDef_t *rd) {
	cl.refDef = *rd;
	R_RenderScene (rd);
}

// ==========================================================================

/*
===============
CGI_Free
===============
*/
static void CGI_Free (const void *ptr, char *fileName, int fileLine) {
	_Mem_Free (ptr, fileName, fileLine);
}

/*
===============
CGI_Alloc
===============
*/
static void *CGI_Alloc (size_t size, char *fileName, int fileLine) {
	return _Mem_PoolAlloc (size, MEMPOOL_CGAMESYS, 0, fileName, fileLine);
}

// ==========================================================================

/*
===============
CL_CGameAPI_Init
===============
*/
void CL_CGameAPI_Init (void) {
	cgImport_t	cgi;
	int			cgInitTime;

	cgInitTime = Sys_Milliseconds ();

	CL_CGameAPI_Shutdown ();	// shutdown if already active

	Com_Printf (PRNT_DEV, "\nCGame Initialization start\n");

	// initialize pointers
	cgi.Cbuf_AddText				= Cbuf_AddText;
	cgi.Cbuf_Execute				= Cbuf_Execute;
	cgi.Cbuf_ExecuteString			= Cmd_ExecuteString;
	cgi.Cbuf_InsertText				= Cbuf_InsertText;

	cgi.CM_BoxTrace					= CM_BoxTrace;
	cgi.CM_HeadnodeForBox			= CM_HeadnodeForBox;
	cgi.CM_InlineModel				= CM_InlineModel;
	cgi.CM_PointContents			= CM_PointContents;
	cgi.CM_Trace					= CM_Trace;
	cgi.CM_TransformedBoxTrace		= CM_TransformedBoxTrace;
	cgi.CM_TransformedPointContents = CM_TransformedPointContents;

	cgi.Cmd_AddCommand				= Cmd_AddCommand;
	cgi.Cmd_RemoveCommand			= Cmd_RemoveCommand;
	cgi.Cmd_Argc					= Cmd_Argc;
	cgi.Cmd_Args					= Cmd_Args;
	cgi.Cmd_Argv					= Cmd_Argv;

	cgi.Com_Error					= Com_Error;
	cgi.Com_Printf					= Com_Printf;
	cgi.Com_ServerState				= Com_ServerState;

	cgi.Con_ClearNotify				= Con_ClearNotify;

	cgi.Cvar_Get					= Cvar_Get;

	cgi.Cvar_VariableInteger		= Cvar_VariableInteger;
	cgi.Cvar_VariableString			= Cvar_VariableString;
	cgi.Cvar_VariableValue			= Cvar_VariableValue;

	cgi.Cvar_VariableForceSet		= Cvar_VariableForceSet;
	cgi.Cvar_VariableForceSetValue	= Cvar_VariableForceSetValue;
	cgi.Cvar_VariableFullSet		= Cvar_VariableFullSet;
	cgi.Cvar_VariableSet			= Cvar_VariableSet;
	cgi.Cvar_VariableSetValue		= Cvar_VariableSetValue;

	cgi.Cvar_ForceSet				= Cvar_ForceSet;
	cgi.Cvar_ForceSetValue			= Cvar_ForceSetValue;
	cgi.Cvar_Set					= Cvar_Set;
	cgi.Cvar_SetValue				= Cvar_SetValue;

	cgi.Draw_Fill					= Draw_Fill;
	cgi.Draw_StretchPic				= Draw_StretchPic;
	cgi.Draw_Pic					= Draw_Pic;
	cgi.Draw_Char					= Draw_Char;
	cgi.Draw_String					= Draw_String;
	cgi.Draw_StringLen				= Draw_StringLen;

	cgi.FS_CurrGame					= FS_CurrGame;
	cgi.FS_FreeFile					= FS_FreeFile;
	cgi.FS_FreeFileList				= FS_FreeFileList;
	cgi.FS_Gamedir					= FS_Gamedir;
	cgi.FS_ListFiles				= FS_ListFiles;
	cgi.FS_LoadFile					= FS_LoadFile;
	cgi.FS_NextPath					= FS_NextPath;
	cgi.FS_Read						= FS_Read;
	cgi.FS_FOpenFile				= FS_FOpenFile;
	cgi.FS_FCloseFile				= FS_FCloseFile;

	cgi.GetConfigString				= CGI_GetConfigString;

	cgi.Img_FindImage				= Img_FindImage;
	cgi.Img_RegisterPic				= Img_RegisterPic;
	cgi.Img_RegisterSkin			= Img_RegisterSkin;
	cgi.Img_GetSize					= Img_GetSize;

	cgi.Key_GetBindingBuf			= Key_GetBindingBuf;
	cgi.Key_IsDown					= Key_IsDown;
	cgi.Key_ClearStates				= Key_ClearStates;
	cgi.Key_KeyDest					= Key_KeyDest;
	cgi.Key_KeynumToString			= Key_KeynumToString;
	cgi.Key_SetBinding				= Key_SetBinding;
	cgi.Key_SetKeyDest				= Key_SetKeyDest;

	cgi.Mem_Alloc					= CGI_Alloc;
	cgi.Mem_Free					= CGI_Free;

	cgi.Mod_RegisterMap				= Mod_RegisterMap;
	cgi.Mod_RegisterModel			= Mod_RegisterModel;
	cgi.Mod_ModelBounds				= Mod_ModelBounds;

	cgi.MSG_ReadChar				= CGI_MSG_ReadChar;
	cgi.MSG_ReadByte				= CGI_MSG_ReadByte;
	cgi.MSG_ReadShort				= CGI_MSG_ReadShort;
	cgi.MSG_ReadLong				= CGI_MSG_ReadLong;
	cgi.MSG_ReadFloat				= CGI_MSG_ReadFloat;
	cgi.MSG_ReadDir					= CGI_MSG_ReadDir;
	cgi.MSG_ReadPos					= CGI_MSG_ReadPos;
	cgi.MSG_ReadString				= CGI_MSG_ReadString;

	cgi.NET_GetCurrentUserCmdNum	= CGI_NET_GetCurrentUserCmdNum;
	cgi.NET_GetSequenceState		= CGI_NET_GetSequenceState;
	cgi.NET_GetUserCmd				= CGI_NET_GetUserCmd;
	cgi.NET_GetUserCmdTime			= CGI_NET_GetUserCmdTime;

	cgi.R_GetClippedFragments		= R_GetClippedFragments;

	cgi.R_CullBox					= R_CullBox;
	cgi.R_CullSphere				= R_CullSphere;
	cgi.R_CullVisNode				= R_CullVisNode;

	cgi.R_ClearScene				= R_ClearScene;
	cgi.R_AddEntity					= R_AddEntity;
	cgi.R_AddParticle				= R_AddParticle;
	cgi.R_AddDecal					= R_AddDecal;
	cgi.R_AddLight					= R_AddLight;
	cgi.R_AddLightStyle				= R_AddLightStyle;

	cgi.R_UpdateScreen				= SCR_UpdateScreen;
	cgi.R_RenderScene				= CGI_R_RenderScene;
	cgi.R_BeginFrame				= R_BeginFrame;
	cgi.R_EndFrame					= R_EndFrame;

	cgi.R_LightPoint				= R_LightPoint;
	cgi.R_TransformToScreen			= R_TransformToScreen;
	cgi.R_SetSky					= R_SetSky;

	cgi.Snd_RegisterSound			= Snd_RegisterSound;
	cgi.Snd_StartLocalSound			= Snd_StartLocalSound;
	cgi.Snd_StartSound				= Snd_StartSound;
	cgi.Snd_Update					= Snd_Update;

	cgi.Sys_FindClose				= Sys_FindClose;
	cgi.Sys_FindFirst				= Sys_FindFirst;
	cgi.Sys_GetClipboardData		= Sys_GetClipboardData;
	cgi.Sys_Milliseconds			= Sys_Milliseconds;
	cgi.Sys_SendKeyEvents			= Sys_SendKeyEvents;

	// get the cgame api
	cge = (cgExport_t *) Sys_LoadLibrary (LIB_CGAME, &cgi);
	if (!cge) {
		cls.moduleCGameActive = qFalse;
		Com_Error (ERR_FATAL, "Find/load of CGame library failed!\n");
	}

	// checking the cg api version
	if (cge->apiVersion != CGAME_APIVERSION) {
		Com_Error (ERR_FATAL, "CL_CGameAPI_Init: incompatible apiVersion (%i != %i)\n", cge->apiVersion, CGAME_APIVERSION);

		Sys_UnloadLibrary (LIB_CGAME);
		cge = NULL;
		cls.moduleCGameActive = qFalse;
	}

	CL_CGModule_Init ();
	Com_Printf (PRNT_DEV, "CGame Init time: %.2fs\n", (Sys_Milliseconds () - cgInitTime)*0.001f);
	cls.moduleCGameActive = qTrue;
}


/*
===============
CL_CGameAPI_Shutdown
===============
*/
void CL_CGameAPI_Shutdown (void) {
	if (!cge)
		return;

	Com_Printf (PRNT_DEV, "CGame Shutdown start\n");

	// tell the module to shutdown locally and unload dll
	cge->Shutdown ();
	Sys_UnloadLibrary (LIB_CGAME);
	cls.moduleCGameActive = qFalse;
	cge = NULL;

	// free the memory pool
	Mem_FreePool (MEMPOOL_CGAMESYS);
}
