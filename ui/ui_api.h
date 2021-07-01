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

// ui_api.h

#ifndef __UIAPI_H__
#define __UIAPI_H__

#include "../renderer/refresh.h"

#define UI_API_VERSION	005

typedef struct {
	int			api_version;

	void		(*Init) (int vidWidth, int vidHeight);
	void		(*Shutdown) (void);
	void		(*Refresh) (int time, int connectionState, int serverState, int vidWidth, int vidHeight, qBool backGround);

	void		(*MainMenu) (void);
	void		(*ForceMenuOff) (void);
	void		(*MoveMouse) (int mx, int my);
	void		(*Keydown) (int key);
	void		(*AddToServerList) (netAdr_t adr, char *info);
	void		(*DrawStatusBar) (char *string);
} ui_export_t;

typedef struct {
	void		(*CL_ResetServerCount) (void);

	void		(*Cbuf_AddText) (char *text);
	void		(*Cbuf_Execute) (void);
	void		(*Cbuf_ExecuteText) (int execWhen, char *text);
	void		(*Cbuf_InsertText) (char *text);

	void		(*CL_PingServers) (void);

	void		(*Cmd_AddCommand) (char *cmd_name, void (*function) (void));
	void		(*Cmd_RemoveCommand) (char *cmd_name);

	void		(*Com_Error) (int code, char *fmt, ...);
	void		(*Com_Printf) (int flags, char *fmt, ...);

	cvar_t		*(*Cvar_ForceSet) (char *varName, char *value);
	cvar_t		*(*Cvar_ForceSetValue) (char *varName, float value);
	cvar_t		*(*Cvar_Get) (char *name, char *value, int flags);
	cvar_t		*(*Cvar_Set) (char *name, char *value);
	void		(*Cvar_SetValue) (char *name, float value);
	int			(*Cvar_VariableInteger) (char *varName);
	char		*(*Cvar_VariableString) (char *varName);
	float		(*Cvar_VariableValue) (char *varName);

	void		(*Draw_Char) (float x, float y, int flags, float scale, int num, vec4_t color);
	void		(*Draw_Fill) (float x, float y, int w, int h, vec4_t color);
	void		(*Draw_StretchPic) (char *pic, float x, float y, int w, int h, vec4_t color);
	void		(*Draw_Pic) (image_t *image, float x, float y, int w, int h, float s1, float t1, float s2, float t2, vec4_t color);
	int			(*Draw_String) (float x, float y, int flags, float scale, char *string, vec4_t color);

	void		(*free) (void *);
	qBool		(*FS_CurrGame) (char *name);
	void		(*FS_FreeFile) (void *buffer);
	void		(*FS_FreeFileList) (char **list, int n);
	char		*(*FS_Gamedir) (void);
	char		**(*FS_ListFiles) (char *findname, int *numfiles, unsigned musthave, unsigned canthave);
	int			(*FS_LoadFile) (char *path, void **buffer);
	char		*(*FS_NextPath) (char *prevpath);
	void		(*FS_Read) (void *buffer, int len, FILE *f);

	struct image_s	*(*Img_FindImage) (char *name, int flags);
	void			(*Img_GetPicSize) (int *w, int *h, char *pic);
	struct image_s	*(*Img_RegisterPic) (char *name);
	struct image_s	*(*Img_RegisterSkin) (char *name);

	char		*(*key_Bindings);
	qBool		(*key_Down);
	void		(*Key_ClearStates) (void);
	qBool		(*Key_KeyDest) (int keyDest);
	char		*(*Key_KeynumToString) (int keyNum);
	void		(*Key_SetBinding) (int keyNum, char *binding);
	void		(*Key_SetKeyDest) (int keyDest);

	char		*(*NET_AdrToString) (netAdr_t a);

	void		(*Sys_Error) (char *error, ...);
	void		(*Sys_FindClose) (void);
	char		*(*Sys_FindFirst) (char *path, unsigned musthave, unsigned canthave);
	char		*(*Sys_GetClipboardData) (void);
	int			(*Sys_Milliseconds) (void);

	void		(*R_AddEntity) (entity_t *ent);
	void		(*R_AddParticle) (vec3_t org, vec3_t angle, dvec4_t color, double size, image_t *image, int flags, int sFactor, int dFactor, double orient);
	void		(*R_AddDecal) (vec2_t *coords, vec3_t *verts, int numVerts, struct mNode_s *node, dvec4_t color, image_t *image, int flags, int sFactor, int dFactor);
	void		(*R_AddLight) (vec3_t org, float intensity, float r, float g, float b);
	void		(*R_AddLightStyle) (int style, float r, float g, float b);

	void		(*R_ClearFrame) (void);
	void		(*R_RenderFrame) (refDef_t *fd);
	void		(*R_EndFrame) (void);

	struct model_s	*(*Mod_RegisterModel) (char *name);

	void		(*Snd_Restart_f) (void);
	void		(*Snd_StartLocalSound) (char *s);

	void		(*VID_Restart_f) (void);
} ui_import_t;

typedef ui_export_t (*GetUIAPI_t) (ui_import_t);

#endif // __UIAPI_H__
