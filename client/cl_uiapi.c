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

#include "cl_local.h"

uiExport_t		*uie;

static qBool	ui_Restart = qTrue;

/*
=============================================================================

	UI EXPORT FUNCTIONS

=============================================================================
*/

/*
===============
CL_UIModule_Refresh
===============
*/
void CL_UIModule_Refresh (qBool backGround) {
	if (uie)
		uie->Refresh (cls.realTime, Com_ClientState (), Com_ServerState (), vidDef.width, vidDef.height, backGround);
}


/*
===============
CL_UIModule_MainMenu
===============
*/
void CL_UIModule_MainMenu (void) {
	if (uie)
		uie->MainMenu ();
}


/*
===============
CL_UIModule_ForceMenuOff
===============
*/
void CL_UIModule_ForceMenuOff (void) {
	if (uie)
		uie->ForceMenuOff ();
}


/*
===============
CL_UIModule_MoveMouse
===============
*/
void CL_UIModule_MoveMouse (int mx, int my) {
	if (uie)
		uie->MoveMouse (mx, my);
}


/*
===============
CL_UIModule_Keydown
===============
*/
void CL_UIModule_Keydown (int key) {
	if (uie)
		uie->Keydown (key);
}


/*
===============
CL_UIModule_AddToServerList
===============
*/
void CL_UIModule_AddToServerList (char *adr, char *info) {
	if (uie)
		uie->AddToServerList (adr, info);
}


/*
===============
CL_UIModule_DrawStatusBar
===============
*/
void CL_UIModule_DrawStatusBar (char *string) {
	if (uie)
		uie->DrawStatusBar (string);
}


/*
===============
CL_UIModule_RegisterSounds
===============
*/
void CL_UIModule_RegisterSounds (void) {
	if (uie)
		uie->RegisterSounds ();
}

/*
=============================================================================

	CONSOLE FUNCTIONS

=============================================================================
*/

/*
=================
CL_UI_Restart_f
=================
*/
static void CL_UI_Restart_f (void) {
	ui_Restart = qTrue;
}

/*
=============================================================================

	UI IMPORT API

=============================================================================
*/

/*
===============
UII_Free
===============
*/
static void UII_Free (const void *ptr, char *fileName, int fileLine) {
	_Mem_Free (ptr, fileName, fileLine);
}

/*
===============
UII_Alloc
===============
*/
static void *UII_Alloc (size_t size, char *fileName, int fileLine) {
	return _Mem_PoolAlloc (size, MEMPOOL_UISYS, 0, fileName, fileLine);
}

// ==========================================================================

/*
===============
CL_UIAPI_Init
===============
*/
void CL_UIAPI_Init (void) {
	uiImport_t	uii;
	int			uiInitTime;

	uiInitTime = Sys_Milliseconds ();
	ui_Restart = qFalse;

	CL_UIAPI_Shutdown (); // shutdown if already active

	Com_Printf (PRNT_DEV, "UI Initialization start\n");

	// initialize pointers
	uii.CL_ResetServerCount			= CL_ResetServerCount;

	uii.Cbuf_AddText				= Cbuf_AddText;
	uii.Cbuf_Execute				= Cbuf_Execute;
	uii.Cbuf_ExecuteString			= Cmd_ExecuteString;
	uii.Cbuf_InsertText				= Cbuf_InsertText;

	uii.CL_PingServers				= CL_PingServers_f;

	uii.Cmd_AddCommand				= Cmd_AddCommand;
	uii.Cmd_RemoveCommand			= Cmd_RemoveCommand;
	uii.Cmd_Argc					= Cmd_Argc;
	uii.Cmd_Args					= Cmd_Args;
	uii.Cmd_Argv					= Cmd_Argv;

	uii.Com_Error					= Com_Error;
	uii.Com_Printf					= Com_Printf;

	uii.Cvar_Get					= Cvar_Get;

	uii.Cvar_VariableInteger		= Cvar_VariableInteger;
	uii.Cvar_VariableString			= Cvar_VariableString;
	uii.Cvar_VariableValue			= Cvar_VariableValue;

	uii.Cvar_VariableForceSet		= Cvar_VariableForceSet;
	uii.Cvar_VariableForceSetValue	= Cvar_VariableForceSetValue;
	uii.Cvar_VariableFullSet		= Cvar_VariableFullSet;
	uii.Cvar_VariableSet			= Cvar_VariableSet;
	uii.Cvar_VariableSetValue		= Cvar_VariableSetValue;

	uii.Cvar_ForceSet				= Cvar_ForceSet;
	uii.Cvar_ForceSetValue			= Cvar_ForceSetValue;
	uii.Cvar_Set					= Cvar_Set;
	uii.Cvar_SetValue				= Cvar_SetValue;

	uii.Draw_Fill					= Draw_Fill;
	uii.Draw_StretchPic				= Draw_StretchPic;
	uii.Draw_Pic					= Draw_Pic;
	uii.Draw_Char					= Draw_Char;
	uii.Draw_String					= Draw_String;
	uii.Draw_StringLen				= Draw_StringLen;

	uii.FS_CurrGame					= FS_CurrGame;
	uii.FS_FreeFile					= FS_FreeFile;
	uii.FS_FreeFileList				= FS_FreeFileList;
	uii.FS_Gamedir					= FS_Gamedir;
	uii.FS_ListFiles				= FS_ListFiles;
	uii.FS_LoadFile					= FS_LoadFile;
	uii.FS_NextPath					= FS_NextPath;
	uii.FS_Read						= FS_Read;
	uii.FS_FOpenFile				= FS_FOpenFile;
	uii.FS_FCloseFile				= FS_FCloseFile;

	uii.Img_FindImage				= Img_FindImage;
	uii.Img_RegisterPic				= Img_RegisterPic;
	uii.Img_RegisterSkin			= Img_RegisterSkin;
	uii.Img_GetSize					= Img_GetSize;

	uii.Key_GetBindingBuf			= Key_GetBindingBuf;
	uii.Key_IsDown					= Key_IsDown;
	uii.Key_ClearStates				= Key_ClearStates;
	uii.Key_KeyDest					= Key_KeyDest;
	uii.Key_KeynumToString			= Key_KeynumToString;
	uii.Key_SetBinding				= Key_SetBinding;
	uii.Key_SetKeyDest				= Key_SetKeyDest;

	uii.Mem_Alloc					= UII_Alloc;
	uii.Mem_Free					= UII_Free;

	uii.Mod_RegisterModel			= Mod_RegisterModel;
	uii.Mod_ModelBounds				= Mod_ModelBounds;

	uii.R_ClearScene				= R_ClearScene;
	uii.R_AddEntity					= R_AddEntity;
	uii.R_AddParticle				= R_AddParticle;
	uii.R_AddDecal					= R_AddDecal;
	uii.R_AddLight					= R_AddLight;
	uii.R_AddLightStyle				= R_AddLightStyle;

	uii.R_UpdateScreen				= SCR_UpdateScreen;
	uii.R_RenderScene				= R_RenderScene;
	uii.R_EndFrame					= R_EndFrame;

	uii.R_LightPoint				= R_LightPoint;

	uii.Snd_RegisterSound			= Snd_RegisterSound;
	uii.Snd_StartLocalSound			= Snd_StartLocalSound;

	uii.Sys_FindClose				= Sys_FindClose;
	uii.Sys_FindFirst				= Sys_FindFirst;
	uii.Sys_GetClipboardData		= Sys_GetClipboardData;
	uii.Sys_Milliseconds			= Sys_Milliseconds;

	// get the ui api
	uie = (uiExport_t *) Sys_LoadLibrary (LIB_UI, &uii);
	if (!uie) {
		cls.moduleUIActive = qFalse;
		Com_Error (ERR_FATAL, "Find/load of UI library failed!\n");
	}

	// checking the ui api version
	if (uie->apiVersion != UI_APIVERSION) {
		Com_Error (ERR_FATAL, "CL_UIAPI_Init: incompatible apiVersion (%i != %i)\n", uie->apiVersion, UI_APIVERSION);

		Sys_UnloadLibrary (LIB_UI);
		uie = NULL;
		cls.moduleUIActive = qFalse;
	}

	Cmd_AddCommand ("ui_restart",	CL_UI_Restart_f,		"Restart the UI module");

	uie->Init (vidDef.width, vidDef.height);
	Com_Printf (PRNT_DEV, "UI Initialized in: %.2fs\n", (Sys_Milliseconds () - uiInitTime)*0.001f);
	cls.moduleUIActive = qTrue;
}


/*
===============
CL_UI_CheckChanges
===============
*/
void CL_UI_CheckChanges (void) {
	// load UI dll
	if (ui_Restart)
		CL_UIAPI_Init ();
}


/*
===============
CL_UIAPI_Shutdown
===============
*/
void CL_UIAPI_Shutdown (void) {
	if (!uie)
		return;

	// kick the menu off so that bad pointers dont crash us
	CL_UIModule_ForceMenuOff ();

	Com_Printf (PRNT_DEV, "UI Shutdown start\n");

	Cmd_RemoveCommand ("ui_restart");

	// tell the module to shutdown locally and unload dll
	uie->Shutdown ();
	Sys_UnloadLibrary (LIB_UI);
	cls.moduleUIActive = qFalse;
	uie = NULL;

	// free the memory pool
	Mem_FreePool (MEMPOOL_UISYS);
}
