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
// ui_api.h
//

#ifndef __UIAPI_H__
#define __UIAPI_H__

#include "../cgame/cgref.h"

#define UI_APIVERSION	6

typedef struct uiExport_s {
	int			apiVersion;

	void		(*Init) (int vidWidth, int vidHeight, float oldCursorX, float oldCursorY);
	void		(*Shutdown) (void);
	void		(*DrawConnectScreen) (char *serverName, int connectCount, qBool backGround, qBool cGameOn, qBool downloading);
	void		(*UpdateDownloadInfo) (char *fileName, int percent, float downloaded);
	void		(*Refresh) (int time, int connectState, int serverState, int vidWidth, int vidHeight, qBool fullScreen);

	void		(*MainMenu) (void);
	void		(*ForceMenuOff) (void);

	void		(*MoveMouse) (float x, float y);

	void		(*Keydown) (int key);
	qBool		(*ParseServerInfo) (char *adr, char *info);
	qBool		(*ParseServerStatus) (char *adr, char *info);

	void		(*RegisterMedia) (void);
	void		(*RegisterSounds) (void);
} uiExport_t;

typedef struct uiImport_s {
	void		(*Cbuf_AddText) (char *text);
	void		(*Cbuf_Execute) (void);
	void		(*Cbuf_ExecuteString) (char *text);
	void		(*Cbuf_InsertText) (char *text);

	void		(*CL_ResetServerCount) (void);

	cmdFunc_t	*(*Cmd_AddCommand) (char *cmdName, void (*function) (void), char *description);
	void		(*Cmd_RemoveCommand) (char *cmdName, cmdFunc_t *command);
	int			(*Cmd_Argc) (void);
	char		*(*Cmd_Args) (void);
	char		*(*Cmd_Argv) (int arg);

	void		(*Com_Error) (int code, char *fmt, ...);
	void		(*Com_Printf) (int flags, char *fmt, ...);

	cVar_t		*(*Cvar_Get) (char *varName, char *value, int flags);

	int			(*Cvar_VariableInteger) (char *varName);
	char		*(*Cvar_VariableString) (char *varName);
	float		(*Cvar_VariableValue) (char *varName);

	cVar_t		*(*Cvar_VariableForceSet) (cVar_t *var, char *value);
	cVar_t		*(*Cvar_VariableForceSetValue) (cVar_t *var, float value);

	cVar_t		*(*Cvar_ForceSet) (char *varName, char *value);
	cVar_t		*(*Cvar_ForceSetValue) (char *varName, float value);
	cVar_t		*(*Cvar_Set) (char *name, char *value);
	cVar_t		*(*Cvar_SetValue) (char *name, float value);

	void		(*StoreCursorPos) (float x, float y);

	qBool		(*FS_CurrGame) (char *name);
	void		(*FS_FreeFileList) (char **list, int num, const char *fileName, const int fileLine);
	char		*(*FS_Gamedir) (void);
	int			(*FS_FindFiles) (char *path, char *filter, char *extension, char **fileList, int maxFiles, qBool addGameDir, qBool recurse);
	int			(*FS_LoadFile) (char *path, void **buffer);
	void		(*FS_FreeFile) (void *buffer, const char *fileName, const int fileLine);
	char		*(*FS_NextPath) (char *prevPath);
	int			(*FS_Read) (void *buffer, int len, int fileNum);
	int			(*FS_Write) (void *buffer, int size, int fileNum);
	void		(*FS_Seek) (int fileNum, int offset, int seekOrigin);
	int			(*FS_OpenFile) (char *fileName, int *fileNum, int mode);
	void		(*FS_CloseFile) (int fileNum);

	void		(*GetConfigString) (int i, char *str, int size);

	char		*(*Key_GetBindingBuf) (int binding);
	qBool		(*Key_IsDown) (int keyNum);
	void		(*Key_ClearStates) (void);
	qBool		(*Key_KeyDest) (int keyDest);
	char		*(*Key_KeynumToString) (int keyNum);
	void		(*Key_SetBinding) (int keyNum, char *binding);
	void		(*Key_SetKeyDest) (int keyDest);

	void		*(*Mem_Alloc) (size_t size, char *fileName, int fileLine);
	void		(*Mem_Free) (const void *ptr, char *fileName, int fileLine);

	void		(*R_AddEntity) (entity_t *ent);
	void		(*R_AddPoly) (poly_t *poly);
	void		(*R_AddLight) (vec3_t org, float intensity, float r, float g, float b);
	void		(*R_AddLightStyle) (int style, float r, float g, float b);

	void		(*R_ClearScene) (void);

	void		(*R_DrawFill) (float x, float y, int w, int h, vec4_t color);
	void		(*R_DrawPic) (struct shader_s *shader, float x, float y, int w, int h, float s1, float t1, float s2, float t2, vec4_t color);

	void		(*R_GetImageSize) (struct shader_s *shader, int *width, int *height);

	struct shader_s *(*R_RegisterPic) (char *name);
	struct shader_s *(*R_RegisterPoly) (char *name);
	struct shader_s *(*R_RegisterSkin) (char *name);
	struct shader_s *(*R_RegisterTexture) (char *name, int surfParams);

	struct model_s	*(*R_RegisterModel) (char *name);
	void		(*R_ModelBounds) (struct model_s *model, vec3_t mins, vec3_t maxs);

	void		(*R_UpdateScreen) (void);
	void		(*R_RenderScene) (refDef_t *fd);
	void		(*R_EndFrame) (void);

	void		(*R_LightPoint) (vec3_t point, vec3_t light);

	struct sfx_s *(*Snd_RegisterSound) (char *sample);
	void		(*Snd_StartLocalSound) (struct sfx_s *sfx);

	void		(*Sys_FindClose) (void);
	char		*(*Sys_FindFirst) (char *path, uInt mustHave, uInt cantHave);
	char		*(*Sys_GetClipboardData) (void);
	int			(*Sys_Milliseconds) (void);
} uiImport_t;

typedef uiExport_t (*GetUIAPI_t) (uiImport_t);

#endif // __UIAPI_H__
