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

	cge.RegisterSounds			= CG_InitSoundMedia;

	cge.RenderView				= V_RenderView;

	return &cge;
}
