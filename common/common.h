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
// common.h
// Definitions common between client and server, but not game.dll
//

#ifndef __COMMON_H__
#define __COMMON_H__

#include "../shared/shared.h"
#include "../cgame/cg_shared.h"
#include "files.h"
#include "protocol.h"
#include "cm_public.h"
#include "alias.h"
#include "cmd.h"
#include "cvar.h"
#include "memory.h"
#include "parse.h"

#define EGL_VERSTR			"0.3.1"
#define BASE_MODDIRNAME		"baseq2"

#define USE_CURL	1

extern struct memPool_s *com_aliasSysPool;
extern struct memPool_s *com_cmdSysPool;
extern struct memPool_s	*com_cmodelSysPool;
extern struct memPool_s *com_cvarSysPool;
extern struct memPool_s *com_fileSysPool;
extern struct memPool_s *com_genericPool;

// ==========================================================================

extern cVar_t	*developer;
extern cVar_t	*cg_developer;
extern cVar_t	*fs_developer;

extern cVar_t	*dedicated;

// hash optimizing
uint32		Com_HashFileName (const char *fileName, const int hashSize);
uint32		Com_HashGeneric (const char *name, const int hashSize);
uint32		Com_HashGenericFast (const char *name, const int hashSize);

// client/server interactions
void		Com_BeginRedirect (int target, char *buffer, int bufferSize, void (*flush)(int target, char *buffer));
void		Com_EndRedirect (void);
void		Com_ConPrint (comPrint_t flags, char *string); // Does not evaluate args
void		Com_Quit (qBool error);

// client/server states
caState_t	Com_ClientState (void);
void		Com_SetClientState (caState_t state);

ssState_t	Com_ServerState (void);		// this should have just been a cvar...
void		Com_SetServerState (ssState_t state);

// initialization and processing
void		Com_Init (int argc, char **argv);
void		Com_Frame (int msec);
void		Com_Shutdown (void);

// crc and checksum
byte		Com_BlockSequenceCRCByte (byte *base, size_t length, int sequence);
uint32		Com_BlockChecksum (void *buffer, size_t length);

/*
==============================================================================

	NON-PORTABLE SYSTEM SERVICES

==============================================================================
*/

typedef enum libType_s {
	LIB_CGAME,
	LIB_GAME,

	LIB_MAX
} libType_t;

int			Sys_Milliseconds (void);
uint32		Sys_UMilliseconds (void);

void		Sys_Init (void);
void		Sys_AppActivate (void);

void		Sys_UnloadLibrary (libType_t libType);
void		*Sys_LoadLibrary (libType_t libType, void *parms);

void		Sys_SendKeyEvents (void);
void		Sys_Error (char *error, ...);
void		Sys_Print (const char *message);
void		Sys_Quit (qBool error);
char		*Sys_GetClipboardData (void);

void		Sys_Mkdir (char *path);

// pass in an attribute mask of things you wish to REJECT
char		*Sys_FindFirst (char *path, uint32 mustHave, uint32 cantHave);
char		*Sys_FindNext (uint32 mustHave, uint32 cantHave);
void		Sys_FindClose (void);

int			Sys_FindFiles (char *path, char *pattern, char **fileList, int maxFiles, int fileCount, qBool recurse, qBool addFiles, qBool addDirs);

// ==========================================================================

char		*Sys_ConsoleInput (void);
void		Sys_ShowConsole (int visLevel, qBool quitOnClose);
void		Sys_DestroyConsole (void);
void		Sys_CreateConsole (void);
void		Sys_SetConsoleTitle (const char *buf);
void		Sys_SetErrorText (const char *buf);

/*
=============================================================================

	CLIENT / SERVER SYSTEMS

=============================================================================
*/

//
// cl_cgapi.c
//

qBool		CL_CGModule_Pmove (pMoveNew_t *pMove, float airAcceleration);

//
// cl_console.c
//

void		CL_ConsoleInit (void);
void		CL_ConsolePrintf (comPrint_t flags, const char *text);

//
// cl_input.c
//

void		CL_UpdateFrameTime (uint32 time);

//
// cl_keys.c
//

void		Key_WriteBindings (FILE *f);

char		*Key_GetBindingBuf (keyNum_t keyNum);
qBool		Key_IsDown (keyNum_t keyNum);

void		Key_Init (void);

//
// cl_main.c
//

qBool		CL_ForwardCmdToServer (void);
void		CL_ForcePacket (void);

void		CL_Disconnect (qBool openMenu);

void		CL_Frame (int msec);

void		CL_ClientInit (void);
void		CL_ClientShutdown (qBool error);

//
// cl_screen.c
//

void		SCR_BeginLoadingPlaque (void);
void		SCR_EndLoadingPlaque (void);

//
// sv_main.c
//

void		SV_ServerInit (void);
void		SV_ServerShutdown (char *finalmsg, qBool reconnect, qBool crashing);
void		SV_Frame (int msec);

#endif // __COMMON_H__
