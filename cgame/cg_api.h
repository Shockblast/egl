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
// cg_api.h
//

#ifndef __CGAMEAPI_H__
#define __CGAMEAPI_H__

#define CGAME_APIVERSION	031		// Just the engine version number

typedef struct cgExport_s {
	int			apiVersion;

	void		(*Init) (void);
	void		(*Shutdown) (void);

	void		(*UpdateConnectInfo) (char *serverName, char *serverMessage, int connectCount, char *dlFileName, int dlPercent, float bytesDownloaded);
	void		(*LoadMap) (int playerNum, int serverProtocol, qBool attractLoop, qBool strafeHack, refConfig_t *inConfig);

	void		(*DebugGraph) (float value, int color);

	void		(*BeginFrameSequence) (frame_t frame);
	void		(*EndFrameSequence) (int numEntities);
	void		(*NewPacketEntityState) (int entNum, entityState_t state);
	// the sound code makes callbacks to the client for entitiy position
	// information, so entities can be dynamically re-spatialized
	void		(*GetEntitySoundOrigin) (int entNum, vec3_t origin, vec3_t velocity);

	void		(*ParseConfigString) (int num, char *str);

	void		(*StartServerMessage) (void);
	qBool		(*ParseServerMessage) (int command);
	void		(*EndServerMessage) (int realTime);

	void		(*StartSound) (vec3_t origin, int entNum, entChannel_t entChannel, int soundNum, float volume, float attenuation, float timeOffset);

	void		(*Pmove) (pMoveNew_t *pmove, float airAcceleration);

	void		(*RegisterSounds) (void);

	void		(*RenderView) (int realTime, float netFrameTime, float refreshFrameTime, float stereoSeparation, qBool refreshPrepped);

	void		(*SetRefConfig) (refConfig_t *inConfig);

	void		(*MainMenu) (void);
	void		(*ForceMenuOff) (void);

	void		(*MoveMouse) (float x, float y);
	void		(*KeyEvent) (keyNum_t keyNum, qBool isDown);

	qBool		(*ParseServerInfo) (char *adr, char *info);
	qBool		(*ParseServerStatus) (char *adr, char *info);
} cgExport_t;

typedef struct cgImport_s {
	void		(*Cbuf_AddText) (char *text);
	void		(*Cbuf_Execute) (void);
	void		(*Cbuf_ExecuteString) (char *text);
	void		(*Cbuf_InsertText) (char *text);

	qBool		(*CL_ForwardCmdToServer) (void);
	void		(*CL_ResetServerCount) (void);

	trace_t		(*CM_BoxTrace) (vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int headNode, int brushMask);
	int			(*CM_HeadnodeForBox) (vec3_t mins, vec3_t maxs);
	struct cBspModel_s	*(*CM_InlineModel) (char *name);
	void		(*CM_InlineModelBounds) (struct cBspModel_s *model, vec3_t mins, vec3_t maxs);
	int			(*CM_InlineModelHeadNode) (struct cBspModel_s *model);
	int			(*CM_PointContents) (vec3_t point, int headNode);
	trace_t		(*CM_Trace) (vec3_t start, vec3_t end, float size, int contentMask);
	void		(*CM_TransformedBoxTrace) (trace_t *out, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int headNode, int brushMask, vec3_t origin, vec3_t angles);
	int			(*CM_TransformedPointContents) (vec3_t point, int headNode, vec3_t origin, vec3_t angles);

	void		*(*Cmd_AddCommand) (char *name, void (*function) (void), const char *description);
	void		(*Cmd_RemoveCommand) (char *name, void *command);

	void		(*Cmd_TokenizeString) (char *text, qBool macroExpand);
	int			(*Cmd_Argc) (void);
	char		*(*Cmd_Args) (void);
	char		*(*Cmd_Argv) (int arg);

	void		(*Com_Error) (comError_t code, char *text);
	void		(*Com_Printf) (comPrint_t flags, char *text);
	void		(*Com_DevPrintf) (comPrint_t flags, char *text);
	caState_t	(*Com_ClientState) (void);
	ssState_t	(*Com_ServerState) (void);

	cVar_t		*(*Cvar_Register) (char *name, char *value, int flags);
	cVar_t		*(*Cvar_Exists) (char *varName);

	int			(*Cvar_GetIntegerValue) (char *varName);
	char		*(*Cvar_GetStringValue) (char *varName);
	float		(*Cvar_GetFloatValue) (char *varName);

	cVar_t		*(*Cvar_VariableSet) (cVar_t *var, char *value, qBool force);
	cVar_t		*(*Cvar_VariableSetValue) (cVar_t *var, float value, qBool force);
	cVar_t		*(*Cvar_VariableReset) (cVar_t *var, qBool force);

	cVar_t		*(*Cvar_Set) (char *name, char *value, qBool force);
	cVar_t		*(*Cvar_SetValue) (char *name, float value, qBool force);
	cVar_t		*(*Cvar_Reset) (char *varName, qBool force);

	void		(*FS_CreatePath) (char *path);
	char		*(*FS_Gamedir) (void);
	int			(*FS_FileExists) (char *path);
	size_t		(*FS_FindFiles) (char *path, char *filter, char *extension, char **fileList, size_t maxFiles, qBool addGameDir, qBool recurse);
	void		(*FS_FreeFileList) (char **list, size_t num, const char *fileName, const int fileLine);
	int			(*FS_LoadFile) (char *path, void **buffer, char *terminate);
	void		(*FS_FreeFile) (void *buffer, const char *fileName, const int fileLine);
	char		*(*FS_NextPath) (char *prevPath);
	size_t		(*FS_Read) (void *buffer, size_t len, fileHandle_t fileNum);
	size_t		(*FS_Write) (void *buffer, size_t size, fileHandle_t fileNum);
	void		(*FS_Seek) (fileHandle_t fileNum, long offset, fsSeekOrigin_t seekOrigin);
	int			(*FS_OpenFile) (char *fileName, fileHandle_t *fileNum, fsOpenMode_t openMode);
	void		(*FS_CloseFile) (fileHandle_t fileNum);

	void		(*GetConfigString) (int i, char *str, int size);

	struct gui_s *(*GUI_RegisterGUI) (char *name);
	void		(*GUI_OpenGUI) (struct gui_s *gui);
	void		(*GUI_CloseGUI) (struct gui_s *gui);
	void		(*GUI_CloseAllGUIs) (void);
	void		(*GUI_NamedGlobalEvent) (char *name);
	void		(*GUI_NamedGUIEvent) (struct gui_s *gui, char *name);

	struct guiVar_s	*(*GUIVar_Register) (char *name, guiVarType_t type);
	qBool		(*GUIVar_GetFloatValue) (struct guiVar_s *var, float *dest);
	qBool		(*GUIVar_GetStrValue) (struct guiVar_s *var, char *dest, size_t size);
	qBool		(*GUIVar_GetVecValue) (struct guiVar_s *var, vec4_t dest);
	void		(*GUIVar_SetFloatValue) (struct guiVar_s *var, float value);
	void		(*GUIVar_SetStrValue) (struct guiVar_s *var, char *value);
	void		(*GUIVar_SetVecValue) (struct guiVar_s *var, vec4_t value);

	void		(*Key_ClearStates) (void);
	char		*(*Key_GetBindingBuf) (keyNum_t keyNum);
	keyDest_t	(*Key_GetDest) (void);
	qBool		(*Key_IsDown) (keyNum_t keyNum);
	char		*(*Key_KeynumToString) (keyNum_t keyNum);
	void		(*Key_SetBinding) (keyNum_t keyNum, char *binding);
	void		(*Key_SetDest) (keyDest_t keyDest);
	qBool		(*Key_InsertOn) (void);
	qBool		(*Key_CapslockOn) (void);
	qBool		(*Key_ShiftDown) (void);

	void		*(*Mem_Alloc) (size_t size, qBool zeroFill, const int tagNum, const char *fileName, const int fileLine);
	size_t		(*Mem_Free) (const void *ptr, const char *fileName, const int fileLine);
	size_t		(*Mem_FreeTag) (const int tagNum, const char *fileName, const int fileLine);
	char		*(*Mem_StrDup) (const char *in, const int tagNum, const char *fileName, const int fileLine);
	size_t		(*Mem_TagSize) (const int tagNum);
	size_t		(*Mem_ChangeTag) (const int tagFrom, const int tagTo);

	int			(*MSG_ReadChar) (void);
	int			(*MSG_ReadByte) (void);
	int			(*MSG_ReadShort) (void);
	int			(*MSG_ReadLong) (void);
	float		(*MSG_ReadFloat) (void);
	void		(*MSG_ReadDir) (vec3_t dir);
	void		(*MSG_ReadPos) (vec3_t pos);
	char		*(*MSG_ReadString) (void);

	int			(*NET_GetCurrentUserCmdNum) (void);
	int			(*NET_GetPacketDropCount) (void);
	int			(*NET_GetRateDropCount) (void);
	void		(*NET_GetSequenceState) (int *outgoingSequence, int *incomingAcknowledged);
	void		(*NET_GetUserCmd) (int frame, userCmd_t *cmd);
	int			(*NET_GetUserCmdTime) (int frame);

	void		(*R_AddDecal) (refDecal_t *decal, bvec4_t color, struct shader_s *material, float materialTime);
	void		(*R_AddEntity) (refEntity_t *ent);
	void		(*R_AddPoly) (refPoly_t *poly);
	void		(*R_AddLight) (vec3_t org, float intensity, float r, float g, float b);
	void		(*R_AddLightStyle) (int style, float r, float g, float b);

	void		(*R_ClearScene) (void);

	qBool		(*R_CullBox) (vec3_t mins, vec3_t maxs, int clipFlags);
	qBool		(*R_CullSphere) (const vec3_t origin, const float radius, int clipFlags);

	struct font_s *(*R_RegisterFont) (char *name);
	void		(*R_GetFontDimensions) (struct font_s *font, float xScale, float yScale, uint32 flags, vec2_t dest);
	void		(*R_DrawChar) (struct font_s *font, float x, float y, float xScale, float yScale, uint32 flags, int num, vec4_t color);
	size_t		(*R_DrawString) (struct font_s *font, float x, float y, float xScale, float yScale, uint32 flags, char *string, vec4_t color);
	size_t		(*R_DrawStringLen) (struct font_s *font, float x, float y, float xScale, float yScale, uint32 flags, char *string, size_t len, vec4_t color);

	void		(*R_DrawPic) (struct shader_s *shader, float shaderTime, float x, float y, int w, int h, float s1, float t1, float s2, float t2, vec4_t color);
	void		(*R_DrawRectangle) (struct shader_s *shader, float shaderTime, vec2_t tl, vec2_t tr, vec2_t br, vec2_t bl, float s1, float t1, float s2, float t2, vec4_t color);

	void		(*R_GetRefConfig) (refConfig_t *outConfig);
	void		(*R_GetImageSize) (struct shader_s *shader, int *width, int *height);

	qBool		(*R_CreateDecal) (refDecal_t *d, vec3_t origin, vec3_t direction, float angle, float size);
	qBool		(*R_FreeDecal) (refDecal_t *d);

	void		(*R_RegisterMap) (char *mapName);
	struct refModel_s *(*R_RegisterModel) (char *name);
	void		(*R_ModelBounds) (struct refModel_s *model, vec3_t mins, vec3_t maxs);

	void		(*R_UpdateScreen) (void);
	void		(*R_RenderScene) (refDef_t *rd);
	void		(*R_BeginFrame) (float cameraSeparation);
	void		(*R_EndFrame) (void);

	struct shader_s *(*R_RegisterPic) (char *name);
	struct shader_s *(*R_RegisterPoly) (char *name);
	struct shader_s *(*R_RegisterSkin) (char *name);

	void		(*R_LightPoint) (vec3_t point, vec3_t light);
	void		(*R_TransformVectorToScreen) (refDef_t *rd, vec3_t in, vec2_t out);
	void		(*R_SetSky) (char *name, float rotate, vec3_t axis);

	struct sfx_s *(*Snd_RegisterSound) (char *sample);
	void		(*Snd_StartSound) (vec3_t origin, int entNum, entChannel_t entChannel, struct sfx_s *sfx, float volume, float attenuation, float timeOffset);
	void		(*Snd_StartLocalSound) (struct sfx_s *sfx, float volume);
	void		(*Snd_Update) (refDef_t *rd);

	void		(*Sys_FindClose) (void);
	char		*(*Sys_FindFirst) (char *path, uint32 mustHave, uint32 cantHave);
	char		*(*Sys_GetClipboardData) (void);
	int			(*Sys_Milliseconds) (void);
	void		(*Sys_SendKeyEvents) (void);
} cgImport_t;

typedef cgExport_t (*GetCGameAPI_t) (cgImport_t);

#endif // __CGAMEAPI_H__
