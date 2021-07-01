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
// cg_api.c
//

#include "cg_local.h"

cgImport_t	cgi;

/*
=============
CG_RegisterPic
=============
*/
struct image_s *CG_RegisterPic (char *name) {
	struct image_s	*image;
	char			fullName[MAX_QPATH];

	if (!name)
		return NULL;
	if (!name[0])
		return NULL;

	if ((name[0] != '/') && (name[0] != '\\')) {
		Q_snprintfz (fullName, sizeof (fullName), "pics/%s.tga", name);
		image = cgi.Img_RegisterImage (fullName, IF_NOMIPMAP);
	}
	else {
		Q_strncpyz (fullName, name+1, sizeof (fullName));
		image = cgi.Img_RegisterImage (fullName, IF_NOMIPMAP);
	}

	return image;
}

/*
=============================================================================

	CGAME IMPORT API FUNCTIONS

=============================================================================
*/

void CGI_Cbuf_AddText (char *text) {
	cgi.Cbuf_AddText (text);
}
void CGI_Cbuf_Execute (void) {
	cgi.Cbuf_Execute ();
}
void CGI_Cbuf_ExecuteString (char *text) {
	cgi.Cbuf_ExecuteString (text);
}
void CGI_Cbuf_InsertText (char *text) {
	cgi.Cbuf_InsertText (text);
}

trace_t CGI_CM_BoxTrace (vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int headNode, int brushMask) {
	return cgi.CM_BoxTrace (start, end, mins, maxs, headNode, brushMask);
}
int CGI_CM_HeadnodeForBox (vec3_t mins, vec3_t maxs) {
	return cgi.CM_HeadnodeForBox (mins, maxs);
}
cBspModel_t *CGI_CM_InlineModel (char *name) {
	return cgi.CM_InlineModel (name);
}
int CGI_CM_PointContents (vec3_t p, int headNode) {
	return cgi.CM_PointContents (p, headNode);
}
trace_t CGI_CM_Trace (vec3_t start, vec3_t end, float size, int contentMask) {
	return cgi.CM_Trace (start, end, size, contentMask);
}
trace_t CGI_CM_TransformedBoxTrace (vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs,
						int headNode, int brushMask, vec3_t origin, vec3_t angles) {
	return cgi.CM_TransformedBoxTrace (start, end, mins, maxs, headNode, brushMask, origin, angles);
}
int CGI_CM_TransformedPointContents (vec3_t p, int headNode, vec3_t origin, vec3_t angles) {
	return cgi.CM_TransformedPointContents (p, headNode, origin, angles);
}

void CGI_Cmd_AddCommand (char *cmdName, void (*function) (void), char *description) {
	cgi.Cmd_AddCommand (cmdName, function, description);
}
void CGI_Cmd_RemoveCommand (char *cmdName) {
	cgi.Cmd_RemoveCommand (cmdName);
}
int CGI_Cmd_Argc (void) {
	return cgi.Cmd_Argc ();
}
char *CGI_Cmd_Args (void) {
	return cgi.Cmd_Args ();
}
char *CGI_Cmd_Argv (int arg) {
	return cgi.Cmd_Argv (arg);
}

int CGI_Com_ServerState (void) {
	return cgi.Com_ServerState ();
}

void CGI_Con_ClearNotify (void) {
	cgi.Con_ClearNotify ();
}
/*
	cVar_t		*(*Cvar_Get) (char *name, char *value, int flags);

	int			(*Cvar_VariableInteger) (char *varName);
	char		*(*Cvar_VariableString) (char *varName);
	float		(*Cvar_VariableValue) (char *varName);

	cVar_t		*(*Cvar_VariableForceSet) (cVar_t *var, char *value);
	cVar_t		*(*Cvar_VariableForceSetValue) (cVar_t *var, float value);

	cVar_t		*(*Cvar_ForceSet) (char *varName, char *value);
	cVar_t		*(*Cvar_ForceSetValue) (char *varName, float value);
	cVar_t		*(*Cvar_Set) (char *name, char *value);
	cVar_t		*(*Cvar_SetValue) (char *name, float value);

	void		(*Draw_Fill) (float x, float y, int w, int h, vec4_t color);
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

	void		(*GetConfigString) (int i, char *str, int size);

	struct image_s	*(*Img_RegisterImage) (char *name, int flags);
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

	void		(*Mod_RegisterMap) (char *mapName);
	struct		model_s *(*Mod_RegisterModel) (char *name);
	void		(*Mod_ModelBounds) (struct model_s *model, vec3_t mins, vec3_t maxs);

	int			(*MSG_ReadChar) (void);
	int			(*MSG_ReadByte) (void);
	int			(*MSG_ReadShort) (void);
	int			(*MSG_ReadLong) (void);
	float		(*MSG_ReadFloat) (void);
	void		(*MSG_ReadDir) (vec3_t dir);
	void		(*MSG_ReadPos) (vec3_t pos);
	char		*(*MSG_ReadString) (void);

	int			(*NET_GetCurrentUserCmdNum) (void);
	void		(*NET_GetSequenceState) (int *outgoingSequence, int *incomingAcknowledged);
	void		(*NET_GetUserCmd) (int frame, userCmd_t *cmd);
	int			(*NET_GetUserCmdTime) (int frame);

	uInt		(*R_GetClippedFragments) (vec3_t origin, float radius, vec3_t axis[3], int maxfverts, vec3_t *fverts, int maxfragments, fragment_t *fragments);

	qBool		(*R_CullBox) (vec3_t mins, vec3_t maxs, int clipFlags);
	qBool		(*R_CullSphere) (const vec3_t origin, const float radius, int clipFlags);
	qBool		(*R_CullVisNode) (struct mBspNode_s *node);

	void		(*R_ClearScene) (void);
	void		(*R_AddEntity) (entity_t *ent);
	void		(*R_AddParticle) (vec3_t org, vec3_t angle, dvec4_t color, double size, struct image_s *image, int flags, int sFactor, int dFactor, double orient);
	void		(*R_AddDecal) (vec2_t *coords, vec3_t *verts, int numVerts, struct mBspNode_s *node, dvec4_t color, struct image_s *image, int flags, int sFactor, int dFactor);
	void		(*R_AddLight) (vec3_t org, float intensity, float r, float g, float b);
	void		(*R_AddLightStyle) (int style, float r, float g, float b);

	void		(*R_UpdateScreen) (void);
	void		(*R_RenderScene) (refDef_t *rd);
	void		(*R_BeginFrame) (float cameraSeparation);
	void		(*R_EndFrame) (void);

	void		(*R_LightPoint) (vec3_t point, vec3_t light);
	void		(*R_TransformToScreen) (vec3_t in, vec2_t out);
	void		(*R_SetSky) (char *name, float rotate, vec3_t axis);

	struct sfx_s *(*Snd_RegisterSound) (char *sample);
	void		(*Snd_StartLocalSound) (struct sfx_s *sfx);
	void		(*Snd_StartSound) (vec3_t origin, int entNum, int entChannel, struct sfx_s *sfx, float volume, float attenuation, float timeOffset);
	void		(*Snd_Update) (vec3_t viewOrigin, vec3_t forward, vec3_t right, vec3_t up, qBool underWater);

	void		(*Sys_FindClose) (void);
	char		*(*Sys_FindFirst) (char *path, uInt mustHave, uInt cantHave);
	char		*(*Sys_GetClipboardData) (void);
	int			(*Sys_Milliseconds) (void);
	void		(*Sys_SendKeyEvents) (void);
*/

/*
=============================================================================

	CGAME EXPORT API

=============================================================================
*/

/*
===============
GetCGameAPI
===============
*/
cgExport_t *GetCGameAPI (cgImport_t *cgimp) {
	static cgExport_t	cge;

	cgi = *cgimp;

	cge.apiVersion				= CGAME_APIVERSION;

	cge.Init					= CG_Init;
	cge.Shutdown				= CG_Shutdown;

	cge.BeginFrameSequence		= CG_BeginFrameSequence;
	cge.EndFrameSequence		= CG_EndFrameSequence;
	cge.NewPacketEntityState	= CG_NewPacketEntityState;
	cge.GetEntitySoundOrigin	= CG_GetEntitySoundOrigin;

	cge.ParseCenterPrint		= SCR_ParseCenterPrint;
	cge.ParseConfigString		= CG_ParseConfigString;

	cge.StartServerMessage		= CG_StartServerMessage;
	cge.ParseServerMessage		= CG_ParseServerMessage;
	cge.EndServerMessage		= CG_EndServerMessage;

	cge.Pmove					= Pmove;

	cge.RegisterSounds			= CG_SoundMediaInit;

	cge.RenderView				= V_RenderView;

	return &cge;
}
