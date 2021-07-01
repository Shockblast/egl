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

#ifdef _DEBUG
# define	PARANOID			// speed sapping error checking
#endif // _DEBUG

#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "../common/common.h"
#include "../renderer/refresh.h"

#include "../sound/snd_pub.h"
#include "../sound/cdaudio.h"

#include "../ui/ui_api.h"
#include "../cgame/cg_api.h"

#include "keys.h"

/*
==================================================================

	CONSOLE

==================================================================
*/

#define CON_TEXTSIZE	32768
#define CON_MAXTIMES	128

typedef struct console_s {
	qBool	initialized;

	char	text[CON_TEXTSIZE];		// console text
	float	times[CON_MAXTIMES];	// cls.realTime time the line was generated
									// for transparent notify lines

	int		orMask;

	int		currentLine;	// line where next message will be printed
	int		display;		// bottom of console displays this line

	float	visLines;
	int		totalLines;		// total lines in console scrollback
	int		lineWidth;		// characters across screen

	float	currentDrop;	// aproaches con_DropHeight at scr_conspeed->value
	float	dropHeight;		// 0.0 to 1.0 lines of console to display
} console_t;

extern	console_t	con;

void	Con_ClearNotify (void);
void	Con_Close (void);

void	Con_ToggleConsole_f (void);

void	Con_CheckResize (void);

void	Con_Print (int flags, const char *txt);

void	Con_Init (void);

void	Con_RunConsole (void);
void	Con_DrawConsole (void);

/*
==================================================================

	ENTITY

==================================================================
*/

// delta from this if not from a previous frame
extern entityState_t	clBaseLines[MAX_CS_EDICTS];

// the clParseEntities must be large enough to hold UPDATE_BACKUP frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original
extern	entityState_t	clParseEntities[MAX_PARSE_ENTITIES];

/*
==================================================================

	CLIENT STATE

==================================================================
*/

// the clientState_t structure is wiped completely at every
// server map change

typedef struct {
	int					timeOutCount;

	int					timeDemoFrames;
	int					timeDemoStart;

	qBool				refreshPrepped;		// false if on new level or new ref dll
	qBool				soundPrepped;		// ambient sounds can start

	int					parseEntities;		// index (not anded off) into clParseEntities[]

	userCmd_t			cmd;
	userCmd_t			cmds[CMD_BACKUP];	// each mesage will send several old cmds
	int					cmdTime[CMD_BACKUP];	// time sent, for calculating pings
	int					cmdNum;

	frame_t				frame;				// received from server
	frame_t				*oldFrame;			// pointer to an item in cl.frames[]
	int					surpressCount;		// number of messages rate supressed
	frame_t				frames[UPDATE_BACKUP];

	refDef_t			refDef;
	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  It is cleared to 0 upon entering each level.
	// the server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes
	vec3_t				viewAngles;

	int					time;			// this is the time value that the client
										// is rendering at. always <= cls.realTime

	//
	// non-gameserver infornamtion
	//
	FILE				*cinematicFile;
	int					cinematicTime;		// cls.realTime for first cinematic frame
	int					cinematicFrame;
	char				cinematicPalette[768];
	qBool				cinematicPaletteActive;

	//
	// server state information
	//
	qBool				attractLoop;		// running the attract loop, any key will menu
	int					serverCount;		// server identification for prespawns
	char				gameDir[MAX_QPATH];
	int					playerNum;

	char				configStrings[MAX_CONFIGSTRINGS][MAX_QPATH];
} clientState_t;

extern	clientState_t	cl;

/*
==================================================================

	MEDIA

==================================================================
*/

typedef struct {
	qBool				initialized;

	// sounds
	struct sfx_s		*talkSfx;

	// images
	struct image_s		*consoleImage;
	struct image_s		*hudLoadingImage;
} clMedia_t;

extern clMedia_t	clMedia;

void	CL_InitImageMedia (void);
void	CL_InitSoundMedia (void);
void	CL_InitMedia (void);

void	CL_ShutdownMedia (void);
void	CL_RestartMedia (void);
/*
==================================================================

	CONNECTION

==================================================================
*/

#define NET_RETRYDELAY	3000.0f

// the clientStatic_t structure is persistant through an arbitrary number of server connections
typedef struct {
	int			connState;
	int			keyDest;

	int			frameCount;
	float		frameTime;			// seconds since last frame
	float		trueFrameTime;		// un-clamped frametime that is passed to cgame

	int			realTime;			// always increasing, no clamping, etc

	// screen rendering information
	float		disableScreen;		// showing loading plaque between levels
									// or changing rendering dlls
									// if time gets > 30 seconds ahead, break it
	int			disableServerCount;	// when we receive a frame and cl.servercount
									// > cls.disableServerCount, clear disableScreen

	// connection information
	char		serverName[MAX_OSPATH];	// name of server from original connect
	float		connectTime;			// for connection retransmits

	int			quakePort;			// a 16 bit value that allows quake servers
									// to work around address translating routers
	netChan_t	netChan;
	int			serverProtocol;		// in case we are doing some kind of version hack

	int			challenge;			// from the server to use for connecting

	FILE		*downloadFile;		// file transfer from server
	char		downloadTempName[MAX_OSPATH];
	char		downloadName[MAX_OSPATH];
	int			downloadNumber;
	int			downloadPercent;

	// demo recording info must be here, so it isn't cleared on level change
	FILE		*demoFile;
	qBool		demoRecording;
	qBool		demoWaiting;		// don't record until a non-delta message is received

	//
	// modules
	//
	qBool		moduleCGameActive;
	qBool		moduleUIActive;
} clientStatic_t;

extern clientStatic_t	cls;

/*
==================================================================

	NEAREST LOCATION SUPPORT

==================================================================
*/

void	CL_Loc_Init (void);
void	CL_Loc_Shutdown (void);

void	CL_LoadLoc (char *mapName);
void	CL_FreeLoc (void);

void	CL_Say_Preprocessor (void);

/*
==================================================================

	CVARS

==================================================================
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
extern cVar_t	*cl_stereo;
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
extern cVar_t	*con_alpha;
extern cVar_t	*con_clock;
extern cVar_t	*con_drop;
extern cVar_t	*con_scroll;
extern cVar_t	*m_accel;
extern cVar_t	*r_advir;
extern cVar_t	*r_fontscale;

extern cVar_t	*scr_conspeed;
extern cVar_t	*scr_debuggraph;
extern cVar_t	*scr_demomsg;
extern cVar_t	*scr_graphalpha;
extern cVar_t	*scr_graphheight;
extern cVar_t	*scr_graphscale;
extern cVar_t	*scr_graphshift;
extern cVar_t	*scr_netgraph;
extern cVar_t	*scr_printspeed;
extern cVar_t	*scr_timegraph;

extern cVar_t	*sv_defaultpaks;

/*
==================================================================

	MISCELLANEOUS

==================================================================
*/

//
// cl_cgapi.c
//

void	CL_CGModule_BeginFrameSequence (void);
void	CL_CGModule_NewPacketEntityState (int entnum, entityState_t state);
void	CL_CGModule_EndFrameSequence (void);

void	CL_CGModule_GetEntitySoundOrigin (int entNum, vec3_t org);

void	CL_CGModule_ParseCenterPrint (void);
void	CL_CGModule_ParseConfigString (int num, char *str);

void	CL_CGModule_StartServerMessage (void);
qBool	CL_CGModule_ParseServerMessage (int command);
void	CL_CGModule_EndServerMessage (void);

qBool	CL_CGModule_Pmove (pMove_t *pMove, float airAcceleration);

void	CL_CGModule_RegisterSounds (void);
void	CL_CGModule_RenderView (float stereoSeparation);

void	CL_CGameAPI_Init (void);
void	CL_CGameAPI_Shutdown (void);

//
// cl_cin.c
//

void	CIN_PlayCinematic (char *name);
qBool	CIN_DrawCinematic (void);
void	CIN_RunCinematic (void);
void	CIN_StopCinematic (void);
void	CIN_FinishCinematic (void);

//
// cl_demo.c
//

void	CL_WriteDemoMessage (void);
void	CL_Stop_f (void);
void	CL_Record_f (void);

//
// cl_input.c
//

typedef struct kButton_s {
	int			down[2];		// key nums holding it down
	uInt		downTime;		// msec timestamp
	uInt		msec;			// msec down this frame
	int			state;
} kButton_t;

extern	kButton_t	in_Strafe;
extern	kButton_t	in_Speed;

void	CL_InitInput (void);
void	CL_SendCmd (void);
void	CL_SendMove (userCmd_t *cmd);

int		CL_ReadFromServer (void);

void	CL_WriteToServer (userCmd_t *cmd);

void	IN_CenterView (void);

float	CL_KeyState (kButton_t *key);

//
// cl_main.c
//

void	CL_ResetServerCount (void);

void	CL_SetState (int state);
void	CL_ClearState (void);

void	CL_Disconnect (void);

void	CL_Quit_f (void);
void	CL_PingServers_f (void);

void	CL_RequestNextDownload (void);

void	FASTCALL CL_Frame (int msec);

void	CL_Init (void);
void	CL_Shutdown (void);

//
// cl_parse.c
//

void	CL_ParseServerMessage (void);
void	CL_Download_f (void);

qBool	CL_CheckOrDownloadFile (char *filename);

//
// cl_screen.c
//

void	CL_AddNetgraph (void);

void	SCR_BeginLoadingPlaque (void);
void	SCR_EndLoadingPlaque (void);
void	SCR_DebugGraph (float value, int color);

void	SCR_Init (void);

void	SCR_UpdateScreen (void);

//
// cl_uiapi.c
//

void	CL_UIModule_Refresh (qBool backGround);
void	CL_UIModule_MainMenu (void);
void	CL_UIModule_ForceMenuOff (void);
void	CL_UIModule_MoveMouse (int mx, int my);
void	CL_UIModule_Keydown (int key);
void	CL_UIModule_AddToServerList (char *adr, char *info);
void	CL_UIModule_DrawStatusBar (char *string);
void	CL_UIModule_RegisterSounds (void);

void	CL_UI_CheckChanges (void);
void	CL_UIAPI_Init (void);
void	CL_UIAPI_Shutdown (void);

/*
=============================================================

	IMPLEMENTATION SPECIFIC

=============================================================
*/

//
// in_imp.c
//

void IN_Init (void);
void IN_Shutdown (void);

void IN_Commands (void);	// oportunity for devices to stick commands on the script buffer
void IN_Frame (void);
void IN_Move (userCmd_t *cmd);	// add additional movement on top of the keyboard move cmd

void IN_Activate (qBool active);

//
// key_imp.c
//

int		Key_MapKey (int key);
qBool	Key_CapslockState (void);

//
// vid_imp.c
//

extern	vidDef_t	vidDef;				// global video state

void	VID_Init (void);
void	VID_Shutdown (void);
void	VID_CheckChanges (void);
