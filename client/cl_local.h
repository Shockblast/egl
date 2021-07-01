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
// cl_local.h
// Primary header for client
//

#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "../renderer/refresh.h"
#include "snd_public.h"
#include "../cgame/cg_shared.h"
#include "cl_keys.h"
#include "gui_public.h"

/*
=============================================================================

	CLIENT STATE

	The clientState_t structure is wiped at every server map change
=============================================================================
*/

typedef struct cinematicState_s {
	fileHandle_t		fileNum;
	int					time;						// cls.realTime for first cinematic frame
	int					frameNum;
	char				palette[768];
} cinematicState_t;

typedef struct clientState_s {
	int					timeOutCount;

	int					timeDemoFrames;
	int					timeDemoStart;

	int					maxClients;
	int					parseEntities;				// index (not anded off) into cl_parseEntities[]

	userCmd_t			cmd;
	userCmd_t			cmds[CMD_BACKUP];			// each mesage will send several old cmds
	int					cmdTime[CMD_BACKUP];		// time sent, for calculating pings
	int					cmdNum;

	frame_t				frame;						// received from server
	int					surpressCount;				// number of messages rate supressed
	frame_t				frames[UPDATE_BACKUP];

	refDef_t			refDef;
	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  It is cleared to 0 upon entering each level.
	// the server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes
	vec3_t				viewAngles;

	//
	// non-gameserver infornamtion
	//
	cinematicState_t	cin;

	//
	// demo recording
	//
	netMsg_t			demoBuffer;
	byte				demoFrame[MAX_SV_MSGLEN];

	//
	// server state information
	//
	qBool				attractLoop;				// running the attract loop, any key will menu
	int					serverCount;				// server identification for prespawns
	int					enhancedServer;				// ENHANCED_SERVER_PROTOCOL
	qBool				strafeHack;
	char				gameDir[MAX_QPATH];
	int					playerNum;

	char				configStrings[MAX_CFGSTRINGS][MAX_CFGSTRLEN];
	struct sfx_s		*soundCfgStrings[MAX_CS_SOUNDS];
} clientState_t;

extern	clientState_t	cl;

/*
=============================================================================

	MEDIA

=============================================================================
*/

typedef struct clMedia_s {
	qBool				initialized;

	// sounds
	struct sfx_s		*talkSfx;

	// images
	struct shader_s		*consoleShader;
} clMedia_t;

extern clMedia_t	clMedia;

void	CL_ImageMediaInit (void);
void	CL_SoundMediaInit (void);
void	CL_MediaInit (void);

void	CL_MediaShutdown (void);
void	CL_MediaRestart (void);
/*
=============================================================================

	CONNECTION

	Persistant through an arbitrary number of server connections
=============================================================================
*/

#define NET_RETRYDELAY	3000.0f

typedef struct downloadStatic_s {
	FILE				*file;						// file transfer from server
	char				tempName[MAX_OSPATH];
	char				name[MAX_OSPATH];
	int					percent;
} downloadStatic_t;

typedef struct clientStatic_s {
	int					connectCount;
	float				connectTime;				// for connection retransmits

	float				netFrameTime;				// seconds since last net packet frame
	float				trueNetFrameTime;			// un-clamped net frametime that is passed to cgame
	float				refreshFrameTime;			// seconds since last refresh frame
	float				trueRefreshFrameTime;		// un-clamped refresh frametime that is passed to cgame

	int					realTime;					// system realtime

	// Screen rendering information
	qBool				refreshPrepped;				// false if on new level or new ref dll
	qBool				disableScreen;				// skip rendering until this is true

	// Audio information
	qBool				soundPrepped;				// once loading is started, sounds can't play until the frame is valid

	// Connection information
	char				serverName[MAX_OSPATH];		// name of server from original connect
	char				serverNameLast[MAX_OSPATH];
	int					serverProtocol;				// in case we are doing some kind of version hack

	netAdr_t			netFrom;
	netMsg_t			netMessage;
	byte				netBuffer[MAX_CL_MSGLEN];

	netChan_t			netChan;
	int					quakePort;					// a 16 bit value that allows quake servers
													// to work around address translating routers
	int					challenge;					// from the server to use for connecting
	qBool				forcePacket;				// forces a packet to be sent the next frame

	downloadStatic_t	download;

	//
	// demo recording info must be here, so it isn't cleared on level change
	//
	fileHandle_t		demoFile;
	qBool				demoRecording;
	qBool				demoWaiting;				// don't record until a non-delta message is received

	//
	// cgame information
	//
	qBool				mapLoading;
	qBool				mapLoaded;

	//
	// video settings
	//
	glConfig_t			glConfig;
} clientStatic_t;

extern clientStatic_t	cls;

/*
=============================================================================

	NEAREST LOCATION SUPPORT

=============================================================================
*/

void	CL_Loc_Init (void);
void	CL_Loc_Shutdown (void);

void	CL_LoadLoc (char *mapName);
void	CL_FreeLoc (void);

void	CL_Say_Preprocessor (void);

/*
=============================================================================

	CVARS

=============================================================================
*/

extern cVar_t	*autosensitivity;

extern cVar_t	*cl_upspeed;
extern cVar_t	*cl_forwardspeed;
extern cVar_t	*cl_sidespeed;
extern cVar_t	*cl_yawspeed;
extern cVar_t	*cl_pitchspeed;
extern cVar_t	*cl_run;
extern cVar_t	*cl_anglespeedkey;
extern cVar_t	*cl_shownet;
extern cVar_t	*cl_stereo_separation;
extern cVar_t	*cl_lightlevel;
extern cVar_t	*cl_paused;
extern cVar_t	*cl_timedemo;

extern cVar_t	*freelook;
extern cVar_t	*lookspring;
extern cVar_t	*lookstrafe;
extern cVar_t	*sensitivity;
extern cVar_t	*s_khz;

extern cVar_t	*m_pitch;
extern cVar_t	*m_yaw;
extern cVar_t	*m_forward;
extern cVar_t	*m_side;

extern cVar_t	*cl_timestamp;
extern cVar_t	*con_chatHud;
extern cVar_t	*con_chatHudLines;
extern cVar_t	*con_chatHudPosX;
extern cVar_t	*con_chatHudPosY;
extern cVar_t	*con_chatHudShadow;
extern cVar_t	*con_notifyfade;
extern cVar_t	*con_notifylarge;
extern cVar_t	*con_notifylines;
extern cVar_t	*con_notifytime;
extern cVar_t	*con_alpha;
extern cVar_t	*con_clock;
extern cVar_t	*con_drop;
extern cVar_t	*con_scroll;
extern cVar_t	*m_accel;
extern cVar_t	*r_fontscale;

extern cVar_t	*scr_conspeed;

/*
=============================================================================

	MISCELLANEOUS

=============================================================================
*/

//
// cl_cgapi.c
//

void	CL_CGModule_LoadMap (void);

void	CL_CGModule_UpdateConnectInfo (void);

void	CL_CGModule_BeginFrameSequence (void);
void	CL_CGModule_NewPacketEntityState (int entNum, entityState_t state);
void	CL_CGModule_EndFrameSequence (void);

void	CL_CGModule_GetEntitySoundOrigin (int entNum, vec3_t origin, vec3_t velocity);

void	CL_CGModule_ParseConfigString (int num, char *str);

void	CL_CGModule_DebugGraph (float value, int color);

void	CL_CGModule_StartServerMessage (void);
qBool	CL_CGModule_ParseServerMessage (int command);
void	CL_CGModule_EndServerMessage (void);

qBool	CL_CGModule_Pmove (pMoveNew_t *pMove, float airAcceleration);

void	CL_CGModule_RegisterSounds (void);
void	CL_CGModule_RenderView (float stereoSeparation);

void	CL_CGameAPI_Init (void);
void	CL_CGameAPI_Shutdown (void);

void	CL_CGModule_SetGLConfig (void);

void	CL_CGModule_MainMenu (void);
void	CL_CGModule_ForceMenuOff (void);
void	CL_CGModule_MoveMouse (float mx, float my);
void	CL_CGModule_KeyEvent (int keyNum, qBool isDown);
qBool	CL_CGModule_ParseServerInfo (char *adr, char *info);
qBool	CL_CGModule_ParseServerStatus (char *adr, char *info);
void	CL_CGModule_StartSound (vec3_t origin, int entNum, entChannel_t entChannel, int soundNum, float volume, float attenuation, float timeOffset);
void	CL_CGModule_RegisterSounds (void);

//
// cl_cin.c
//

void	CIN_PlayCinematic (char *name);
void	CIN_DrawCinematic (void);
void	CIN_RunCinematic (void);
void	CIN_StopCinematic (void);
void	CIN_FinishCinematic (void);

//
// cl_console.c
//

void	CL_ClearNotifyLines (void);
void	CL_ConsoleClose (void);
void	CL_MoveConsoleDisplay (int value);
void	CL_SetConsoleDisplay (qBool top);

void	CL_ToggleConsole_f (void);

void	CL_ConsoleCheckResize (void);

void	CL_ConsoleInit (void);

void	CL_DrawConsole (void);

//
// cl_demo.c
//

void	CL_WriteDemoPlayerstate (frame_t *from, frame_t *to, netMsg_t *msg);
void	CL_WriteDemoPacketEntities (const frame_t *from, frame_t *to, netMsg_t *msg);
void	CL_WriteDemoMessageChunk (byte *buffer, int length, qBool forceFlush);
void	CL_WriteDemoMessageFull (void);

qBool	CL_StartDemoRecording (char *name);
void	CL_StopDemoRecording (void);

//
// cl_input.c
//

qBool	CL_StrafeState (void);
qBool	CL_RunState (void);

void	CL_RefreshCmd (void);
void	CL_SendCmd (void);
void	CL_SendMove (userCmd_t *cmd);

void	IN_CenterView (void);
void	CL_InputInit (void);

//
// cl_main.c
//

// delta from this if not from a previous frame
extern entityState_t	cl_baseLines[MAX_CS_EDICTS];

// the cl_parseEntities must be large enough to hold UPDATE_BACKUP frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original
extern	entityState_t	cl_parseEntities[MAX_PARSE_ENTITIES];

qBool	CL_ForwardCmdToServer (void);

void	CL_ResetServerCount (void);

void	CL_SetGLConfig (void);
void	CL_SetState (caState_t state);
void	CL_ClearState (void);

void	CL_Disconnect (qBool openMenu);

void	CL_RequestNextDownload (void);

void	__fastcall CL_Frame (int msec);

void	CL_ClientInit (void);
void	CL_ClientShutdown (qBool error);

//
// cl_parse.c
//

qBool	CL_CheckOrDownloadFile (char *filename);

void	CL_ParseServerMessage (void);

//
// cl_screen.c
//

void	SCR_BeginLoadingPlaque (void);
void	SCR_EndLoadingPlaque (void);

void	SCR_UpdateScreen (void);

/*
=============================================================================

	IMPLEMENTATION SPECIFIC

=============================================================================
*/

//
// in_imp.c
//

int		In_MapKey (int wParam, int lParam);
qBool	In_GetKeyState (int keyNum);

void	IN_Restart_f (void);
void	IN_Init (void);
void	IN_Shutdown (void);

void	IN_Commands (void);	// oportunity for devices to stick commands on the script buffer
void	IN_Frame (void);
void	IN_Move (userCmd_t *cmd);	// add additional movement on top of the keyboard move cmd

void	IN_Activate (qBool active);

//
// vid_imp.c
//

void	VID_CheckChanges (glConfig_t *outConfig);
void	VID_Init (glConfig_t *outConfig);
void	VID_Shutdown (void);
