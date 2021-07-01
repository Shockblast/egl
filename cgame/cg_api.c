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
struct shader_s *CG_RegisterPic (char *name)
{
	struct shader_s	*shader;
	char			fullName[MAX_QPATH];

	if (!name)
		return NULL;
	if (!name[0])
		return NULL;

	if ((name[0] != '/') && (name[0] != '\\')) {
		Q_snprintfz (fullName, sizeof (fullName), "pics/%s.tga", name);
		shader = cgi.R_RegisterPic (fullName);
	}
	else {
		Q_strncpyz (fullName, name+1, sizeof (fullName));
		shader = cgi.R_RegisterPic (fullName);
	}

	return shader;
}

/*
=============================================================================

	CGAME IMPORT API FUNCTIONS

=============================================================================
*/

void CGI_Cbuf_AddText (char *text)
{
	cgi.Cbuf_AddText (text);
}
void CGI_Cbuf_Execute (void)
{
	cgi.Cbuf_Execute ();
}
void CGI_Cbuf_ExecuteString (char *text)
{
	cgi.Cbuf_ExecuteString (text);
}
void CGI_Cbuf_InsertText (char *text)
{
	cgi.Cbuf_InsertText (text);
}

trace_t CGI_CM_BoxTrace (vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int headNode, int brushMask)
{
	return cgi.CM_BoxTrace (start, end, mins, maxs, headNode, brushMask);
}
int CGI_CM_HeadnodeForBox (vec3_t mins, vec3_t maxs)
{
	return cgi.CM_HeadnodeForBox (mins, maxs);
}
cBspModel_t *CGI_CM_InlineModel (char *name)
{
	return cgi.CM_InlineModel (name);
}
int CGI_CM_PointContents (vec3_t p, int headNode)
{
	return cgi.CM_PointContents (p, headNode);
}
trace_t CGI_CM_Trace (vec3_t start, vec3_t end, float size, int contentMask)
{
	return cgi.CM_Trace (start, end, size, contentMask);
}
trace_t CGI_CM_TransformedBoxTrace (vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs,
						int headNode, int brushMask, vec3_t origin, vec3_t angles)
{
	return cgi.CM_TransformedBoxTrace (start, end, mins, maxs, headNode, brushMask, origin, angles);
}
int CGI_CM_TransformedPointContents (vec3_t p, int headNode, vec3_t origin, vec3_t angles)
{
	return cgi.CM_TransformedPointContents (p, headNode, origin, angles);
}

cmdFunc_t *CGI_Cmd_AddCommand (char *cmdName, void (*function) (void), char *description)
{
	return cgi.Cmd_AddCommand (cmdName, function, description);
}
void CGI_Cmd_RemoveCommand (char *cmdName, cmdFunc_t *command)
{
	cgi.Cmd_RemoveCommand (cmdName, command);
}
int CGI_Cmd_Argc (void)
{
	return cgi.Cmd_Argc ();
}
char *CGI_Cmd_Args (void)
{
	return cgi.Cmd_Args ();
}
char *CGI_Cmd_Argv (int arg)
{
	return cgi.Cmd_Argv (arg);
}

int CGI_Com_ServerState (void)
{
	return cgi.Com_ServerState ();
}

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
cgExport_t *GetCGameAPI (cgImport_t *cgimp)
{
	static cgExport_t	cge;

	cgi = *cgimp;

	cge.apiVersion				= CGAME_APIVERSION;

	cge.Init					= CG_Init;
	cge.Shutdown				= CG_Shutdown;

	cge.BeginFrameSequence		= CG_BeginFrameSequence;
	cge.EndFrameSequence		= CG_EndFrameSequence;
	cge.NewPacketEntityState	= CG_NewPacketEntityState;
	cge.GetEntitySoundOrigin	= CG_GetEntitySoundOrigin;

	cge.ParseConfigString		= CG_ParseConfigString;

	cge.StartServerMessage		= CG_StartServerMessage;
	cge.ParseServerMessage		= CG_ParseServerMessage;
	cge.EndServerMessage		= CG_EndServerMessage;

	cge.Pmove					= Pmove;

	cge.RegisterSounds			= CG_SoundMediaInit;

	cge.RenderView				= V_RenderView;

	return &cge;
}
