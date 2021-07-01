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

ui_export_t	*uie;

static qBool	ui_Active = qFalse;
qBool			ui_Restart = qTrue;

/*
=============================================================================

	UI EXPORT FUNCTIONS

=============================================================================
*/

void CL_UIModule_Init (void) {
	if (ui_Active)
		uie->Init (vidDef.width, vidDef.height);
}

void CL_UIModule_Shutdown (void) {
	if (ui_Active)
		uie->Shutdown ();
}

void CL_UIModule_Refresh (qBool backGround) {
	if (ui_Active)
		uie->Refresh (cls.realTime, cls.connState, Com_ServerState (), vidDef.width, vidDef.height, backGround);
}

void CL_UIModule_MainMenu (void) {
	if (ui_Active)
		uie->MainMenu ();
}

void CL_UIModule_ForceMenuOff (void) {
	if (ui_Active)
		uie->ForceMenuOff ();
}

void CL_UIModule_MoveMouse (int mx, int my) {
	if (ui_Active)
		uie->MoveMouse (mx, my);
}

void CL_UIModule_Keydown (int key) {
	if (ui_Active)
		uie->Keydown (key);
}

void CL_UIModule_AddToServerList (netAdr_t adr, char *info) {
	if (ui_Active)
		uie->AddToServerList (adr, info);
}

void CL_UIModule_DrawStatusBar (char *string) {
	if (ui_Active)
		uie->DrawStatusBar (string);
}

/*
=============================================================================

	UI IMPORT API

=============================================================================
*/

/*
===============
CL_UI_LoadLib
===============
*/
ui_export_t *GetUIAPI (ui_import_t *uiimp);
qBool CL_UI_LoadLib (void) {
	ui_import_t	uii;
	int			uiInitTime;

	uiInitTime = Sys_Milliseconds ();

	// shutdown if already active
	CL_UI_Shutdown ();

	// initialize pointers
	uii.CL_ResetServerCount	= CL_ResetServerCount;

	uii.Cbuf_AddText		= Cbuf_AddText;
	uii.Cbuf_Execute		= Cbuf_Execute;
	uii.Cbuf_ExecuteText	= Cbuf_ExecuteText;
	uii.Cbuf_InsertText		= Cbuf_InsertText;

	uii.CL_PingServers		= CL_PingServers_f;

	uii.Cmd_AddCommand		= Cmd_AddCommand;
	uii.Cmd_RemoveCommand	= Cmd_RemoveCommand;

	uii.Com_Error			= Com_Error;
	uii.Com_Printf			= Com_Printf;

	uii.Cvar_ForceSetValue	= Cvar_ForceSetValue;
	uii.Cvar_ForceSet		= Cvar_ForceSet;
	uii.Cvar_Get			= Cvar_Get;
	uii.Cvar_Set			= Cvar_Set;
	uii.Cvar_SetValue		= Cvar_SetValue;
	uii.Cvar_VariableInteger= Cvar_VariableInteger;
	uii.Cvar_VariableString	= Cvar_VariableString;
	uii.Cvar_VariableValue	= Cvar_VariableValue;

	uii.Draw_Char			= Draw_Char;
	uii.Draw_Fill			= Draw_Fill;
	uii.Draw_StretchPic		= Draw_StretchPic;
	uii.Draw_Pic			= Draw_Pic;
	uii.Draw_String			= Draw_String;

	uii.free				= free;
	uii.FS_CurrGame			= FS_CurrGame;
	uii.FS_FreeFile			= FS_FreeFile;
	uii.FS_FreeFileList		= FS_FreeFileList;
	uii.FS_Gamedir			= FS_Gamedir;
	uii.FS_ListFiles		= FS_ListFiles;
	uii.FS_LoadFile			= FS_LoadFile;
	uii.FS_NextPath			= FS_NextPath;
	uii.FS_Read				= FS_Read;

	uii.Img_FindImage		= Img_FindImage;
	uii.Img_GetPicSize		= Img_GetPicSize;
	uii.Img_RegisterPic		= Img_RegisterPic;
	uii.Img_RegisterSkin	= Img_RegisterSkin;

	uii.key_Bindings		= key_Bindings;
	uii.key_Down			= key_Down;
	uii.Key_ClearStates		= Key_ClearStates;
	uii.Key_KeyDest			= Key_KeyDest;
	uii.Key_KeynumToString	= Key_KeynumToString;
	uii.Key_SetBinding		= Key_SetBinding;
	uii.Key_SetKeyDest		= Key_SetKeyDest;

	uii.NET_AdrToString		= NET_AdrToString;

	uii.Sys_Error			= Sys_Error;
	uii.Sys_FindClose		= Sys_FindClose;
	uii.Sys_FindFirst		= Sys_FindFirst;
	uii.Sys_GetClipboardData= Sys_GetClipboardData;
	uii.Sys_Milliseconds	= Sys_Milliseconds;

	uii.R_AddEntity			= R_AddEntity;
	uii.R_AddParticle		= R_AddParticle;
	uii.R_AddDecal			= R_AddDecal;
	uii.R_AddLight			= R_AddLight;
	uii.R_AddLightStyle		= R_AddLightStyle;

	uii.R_ClearFrame		= R_ClearFrame;
	uii.R_RenderFrame		= R_RenderFrame;
	uii.R_EndFrame			= R_EndFrame;

	uii.Mod_RegisterModel	= Mod_RegisterModel;

	uii.Snd_Restart_f		= Snd_Restart_f;
	uii.Snd_StartLocalSound	= Snd_StartLocalSound;

	uii.VID_Restart_f		= VID_Restart_f;

	// get the ui api
	uie = (ui_export_t *) Sys_LoadLibrary (LIB_UI, &uii);
	if (!uie)
		return qFalse;

	// checking the ui api version
	if (uie->api_version != UI_API_VERSION) {
		Sys_UnloadLibrary (LIB_UI);
		uie = NULL;
		ui_Active = qFalse;
		Com_Printf (PRINT_ALL, S_COLOR_RED "..." S_COLOR_RED "incompatible api_version");
		return qFalse;
	}

	ui_Active = qTrue;

	Cmd_AddCommand ("ui_restart",	CL_UI_Restart_f);

	CL_UIModule_Init ();

	Com_Printf (PRINT_ALL, "Init time: %dms\n", Sys_Milliseconds () - uiInitTime);

	return qTrue;
}


/*
===============
CL_UI_CheckChanges
===============
*/
void CL_UI_CheckChanges (void) {
	if (ui_Restart) {
		ui_Restart = qFalse;

		// load UI dll
		Com_Printf (PRINT_ALL, "\nUI Initialization\n");

		if (!CL_UI_LoadLib ()) {
			ui_Active = qFalse;
			Com_Printf (PRINT_ALL, "..." S_COLOR_RED "find/load of ui library failed\n");
		}
	}
}


/*
=================
CL_UI_Restart_f
=================
*/
void CL_UI_Restart_f (void) {
	ui_Restart = qTrue;
}


/*
===============
CL_UI_Init
===============
*/
void CL_UI_Init (void) {
	ui_Restart = qTrue;

	CL_UI_CheckChanges ();
}


/*
===============
CL_UI_Shutdown
===============
*/
void CL_UI_Shutdown (void) {
	if (!uie)
		return;

	Cmd_RemoveCommand ("ui_restart");

	CL_UIModule_Shutdown ();
	Sys_UnloadLibrary (LIB_UI);
}
