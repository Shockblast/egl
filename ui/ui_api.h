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

#define UI_APIVERSION	7

typedef struct uiExport_s {
	int			apiVersion;

	void		(*Init) (int vidWidth, int vidHeight);
	void		(*Shutdown) (void);
	void		(*Refresh) (int time, int connectionState, int serverState, int vidWidth, int vidHeight, qBool backGround);

	void		(*MainMenu) (void);
	void		(*ForceMenuOff) (void);
	void		(*MoveMouse) (int mx, int my);
	void		(*Keydown) (int key);
	void		(*AddToServerList) (char *adr, char *info);
	void		(*DrawStatusBar) (char *string);

	void		(*RegisterSounds) (void);
} uiExport_t;

typedef struct uiImport_s {
	void		(*Cbuf_AddText) (char *text);
	void		(*Cbuf_Execute) (void);
	void		(*Cbuf_ExecuteString) (char *text);
	void		(*Cbuf_InsertText) (char *text);

	void		(*CL_PingServers) (void);
	void		(*CL_ResetServerCount) (void);

	void		(*Cmd_AddCommand) (char *cmdName, void (*function) (void), char *description);
	void		(*Cmd_RemoveCommand) (char *cmdName);
	int			(*Cmd_Argc) (void);
	char		*(*Cmd_Args) (void);
	char		*(*Cmd_Argv) (int arg);

	void		(*Com_Error) (int code, char *fmt, ...);
	void		(*Com_Printf) (int flags, char *fmt, ...);

	cVar_t		*(*Cvar_Get) (char *name, char *value, int flags);

	int			(*Cvar_VariableInteger) (char *varName);
	char		*(*Cvar_VariableString) (char *varName);
	float		(*Cvar_VariableValue) (char *varName);

	cVar_t		*(*Cvar_VariableForceSet) (cVar_t *var, char *value);
	cVar_t		*(*Cvar_VariableForceSetValue) (cVar_t *var, float value);
	cVar_t		*(*Cvar_VariableFullSet) (cVar_t *var, char *value, int flags);
	cVar_t		*(*Cvar_VariableSet) (cVar_t *var, char *value);
	cVar_t		*(*Cvar_VariableSetValue) (cVar_t *var, float value);

	cVar_t		*(*Cvar_ForceSet) (char *varName, char *value);
	cVar_t		*(*Cvar_ForceSetValue) (char *varName, float value);
	cVar_t		*(*Cvar_Set) (char *name, char *value);
	cVar_t		*(*Cvar_SetValue) (char *name, float value);

	void		(*Draw_Fill) (float x, float y, int w, int h, vec4_t color);
	void		(*Draw_StretchPic) (char *pic, float x, float y, int w, int h, vec4_t color);
	void		(*Draw_Pic) (struct image_s *image, float x, float y, int w, int h, float s1, float t1, float s2, float t2, vec4_t color);
	void		(*Draw_Char) (struct image_s *charsImage, float x, float y, int flags, float scale, int num, vec4_t color);
	int			(*Draw_String) (struct image_s *charsImage, float x, float y, int flags, float scale, char *string, vec4_t color);
	int			(*Draw_StringLen) (struct image_s *charsImage, float x, float y, int flags, float scale, char *string, int len, vec4_t color);

	qBool		(*FS_CurrGame) (char *name);
	void		(*FS_FreeFile) (void *buffer);
	void		(*FS_FreeFileList) (char **list, int n);
	char		*(*FS_Gamedir) (void);
	char		**(*FS_ListFiles) (char *findName, int *numFiles, uInt mustHave, uInt cantHave);
	int			(*FS_LoadFile) (char *path, void **buffer);
	char		*(*FS_NextPath) (char *prevpath);
	void		(*FS_Read) (void *buffer, int len, FILE *f);
	int			(*FS_FOpenFile) (char *filename, FILE **file);
	void		(*FS_FCloseFile) (FILE *f);

	struct image_s	*(*Img_FindImage) (char *name, int flags);
	struct image_s	*(*Img_RegisterPic) (char *name);
	struct image_s	*(*Img_RegisterSkin) (char *name);
	void		(*Img_GetSize) (struct image_s *image, int *width, int *height);

	char		*(*Key_GetBindingBuf) (int binding);
	qBool		(*Key_IsDown) (int keyNum);
	void		(*Key_ClearStates) (void);
	qBool		(*Key_KeyDest) (int keyDest);
	char		*(*Key_KeynumToString) (int keyNum);
	void		(*Key_SetBinding) (int keyNum, char *binding);
	void		(*Key_SetKeyDest) (int keyDest);

	void		*(*Mem_Alloc) (size_t size, char *fileName, int fileLine);
	void		(*Mem_Free) (const void *ptr, char *fileName, int fileLine);

	struct model_s	*(*Mod_RegisterModel) (char *name);
	void		(*Mod_ModelBounds) (struct model_s *model, vec3_t mins, vec3_t maxs);

	void		(*R_ClearScene) (void);
	void		(*R_AddEntity) (entity_t *ent);
	void		(*R_AddParticle) (vec3_t org, vec3_t angle, dvec4_t color, double size, struct image_s *image, int flags, int sFactor, int dFactor, double orient);
	void		(*R_AddDecal) (vec2_t *coords, vec3_t *verts, int numVerts, struct mBspNode_s *node, dvec4_t color, struct image_s *image, int flags, int sFactor, int dFactor);
	void		(*R_AddLight) (vec3_t org, float intensity, float r, float g, float b);
	void		(*R_AddLightStyle) (int style, float r, float g, float b);

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
