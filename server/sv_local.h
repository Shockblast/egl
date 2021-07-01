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

// sv_local.h

//#define	PARANOID			// speed sapping error checking

#include "../common/common.h"
#include "../game/game.h"

//=============================================================================

#define	MAX_MASTERS	8				// max recipients for heartbeat packets

typedef enum
{
	SS_DEAD,			// no map loaded
	SS_LOADING,			// spawning level edicts
	SS_GAME,			// actively running
	SS_CINEMATIC,
	SS_DEMO,
	SS_PIC
} serverState_t;
// some qc commands are only valid before the server has finished
// initializing (precache commands, static sounds / objects, etc)

typedef struct
{
	serverState_t		state;				// precache commands are only valid during load

	qBool				attractLoop;		// running cinematics and demos for the local system only
	qBool				loadGame;			// client begins should reuse existing entity

	unsigned			time;				// always sv.frameNum * 100 msec
	int					frameNum;

	char				name[MAX_QPATH];	// map name, or cinematic name
	struct cBspModel_s	*models[MAX_MODELS];

	char				configStrings[MAX_CONFIGSTRINGS][MAX_QPATH];
	entityState_t		baseLines[MAX_EDICTS];

	// the multicast buffer is used to send a message to a set of clients
	// it is only used to marshall data until SV_Multicast is called
	netMsg_t			multiCast;
	byte				multiCast_Buff[MAX_MSGLEN];

	/* demo server information */
	FILE				*demo_File;
	qBool				timeDemo;			// don't time sync
} server_t;

#define EDICT_NUM(n) ((edict_t *)((byte *)ge->edicts + ge->edictSize*(n)))
#define NUM_FOR_EDICT(e) (((byte *)(e)-(byte *)ge->edicts ) / ge->edictSize)

typedef enum
{
	CS_FREE,		// can be reused for a new connection
	CS_ZOMBIE,		// client has been disconnected, but don't reuse
					// connection for a couple seconds
	CS_CONNECTED,	// has been assigned to a client_t, but not in game yet
	CS_SPAWNED		// client is fully in game
} svClientState_t;

typedef struct
{
	int				areaBytes;
	byte			areaBits[BSP_MAX_AREAS/8];		// portalarea visibility bits
	playerState_t	ps;
	int				numEntities;
	int				firstEntity;		// into the circular sv_packet_entities[]
	int				sentTime;			// for ping calculations
} clientFrame_t;

#define	LATENCY_COUNTS	16
#define	RATE_MESSAGES	10

typedef struct client_s
{
	svClientState_t	state;

	char			userInfo[MAX_INFO_STRING];		// name, etc

	int				lastFrame;			// for delta compression
	userCmd_t		lastCmd;			// for filling in big drops

	int				commandMsec;		// every seconds this is reset, if user
										// commands exhaust it, assume time cheating

	int				frameLatency[LATENCY_COUNTS];
	int				ping;

	int				messageSize[RATE_MESSAGES];	// used to rate drop packets
	int				rate;
	int				surpressCount;		// number of messages rate supressed

	edict_t			*edict;				// EDICT_NUM(clientnum+1)
	char			name[32];			// extracted from userinfo, high bits masked
	int				messageLevel;		// for filtering printed messages

	// The datagram is written to by sound calls, prints, temp ents, etc.
	// It can be harmlessly overflowed.
	netMsg_t		datagram;
	byte			datagramBuff[MAX_MSGLEN];

	clientFrame_t	frames[UPDATE_BACKUP];	// updates can be delta'd from here

	byte			*download;			// file being downloaded
	int				downloadSize;		// total bytes (can't use EOF because of paks)
	int				downloadCount;		// bytes sent

	int				lastMessage;		// sv.frameNum when packet was last received
	int				lastConnect;

	int				challenge;			// challenge of this user, randomly generated

	netChan_t		netChan;
} client_t;

// a client can leave the server in one of four ways:
// dropping properly by quiting or disconnecting
// timing out if no valid messages are received for timeout.value seconds
// getting kicked off by the server operator
// a program error, like an overflowed reliable buffer

//=============================================================================

// MAX_CHALLENGES is made large to prevent a denial
// of service attack that could cycle all of them
// out before legitimate users connected
#define	MAX_CHALLENGES	1024

typedef struct
{
	netAdr_t	adr;
	int			challenge;
	int			time;
} challenge_t;

typedef struct
{
	qBool			initialized;				// sv_init has completed
	int				realTime;					// always increasing, no clamping, etc

	char			mapCmd[MAX_TOKEN_CHARS];	// ie: *intro.cin+base 

	int				spawnCount;					// incremented each server start
											// used to check late spawns

	client_t		*clients;					// [maxclients->value];
	int				num_ClientEntities;			// maxclients->value*UPDATE_BACKUP*MAX_PACKET_ENTITIES
	int				next_ClientEntities;		// next client_entity to use
	entityState_t	*client_Entities;		// [num_ClientEntities]

	int				last_HeartBeat;

	challenge_t		challenges[MAX_CHALLENGES];	// to prevent invalid IPs from connecting

	/* serverrecord values */
	FILE			*demo_File;
	netMsg_t		demo_MultiCast;
	byte			demo_MultiCastBuf[MAX_MSGLEN];
} serverStatic_t;

//=============================================================================

extern	netAdr_t	net_From;
extern	netMsg_t	net_Message;

extern	netAdr_t	master_adr[MAX_MASTERS];	// address of the master server

extern	serverStatic_t	svs;				// persistant server info
extern	server_t		sv;					// local server

extern	cvar_t		*sv_paused;
extern	cvar_t		*maxclients;
extern	cvar_t		*sv_noreload;			// don't reload level state when reentering
extern	cvar_t		*sv_airaccelerate;		// don't reload level state when reentering
											// development tool
extern	cvar_t		*sv_enforcetime;

extern	client_t	*sv_client;
extern	edict_t		*sv_player;

//===========================================================

//
// sv_main.c
//
void SV_FinalMessage (char *message, qBool reconnect);
void SV_DropClient (client_t *drop);

int SV_ModelIndex (char *name);
int SV_SoundIndex (char *name);
int SV_ImageIndex (char *name);

void SV_WriteClientdataToMessage (client_t *client, netMsg_t *msg);

void SV_ExecuteUserCommand (char *s);
void SV_InitOperatorCommands (void);

void SV_SendServerinfo (client_t *client);
void SV_UserinfoChanged (client_t *cl);


void Master_Heartbeat (void);
void Master_Packet (void);

//
// sv_init.c
//
void SV_InitGame (void);
void SV_Map (qBool attractloop, char *levelstring, qBool loadgame);


//
// sv_phys.c
//
void SV_PrepWorldFrame (void);

//
// sv_send.c
//
typedef enum
{
	RD_NONE,
	RD_CLIENT,
	RD_PACKET
} redirect_t;
#define	SV_OUTPUTBUF_LENGTH	(MAX_MSGLEN - 16)

extern	char	sv_outputbuf[SV_OUTPUTBUF_LENGTH];

void SV_FlushRedirect (int sv_redirected, char *outputbuf);

void SV_DemoCompleted (void);
void SV_SendClientMessages (void);

void SV_Multicast (vec3_t origin, multiCast_t to);
void SV_StartSound (vec3_t origin, edict_t *entity, int channel, int soundindex, float volume,
					float attenuation, float timeofs);
void SV_ClientPrintf (client_t *cl, int level, char *fmt, ...);
void SV_BroadcastPrintf (int level, char *fmt, ...);
void SV_BroadcastCommand (char *fmt, ...);

//
// sv_user.c
//
void SV_Nextserver (void);
void SV_ExecuteClientMessage (client_t *cl);

//
// sv_ccmds.c
//
void SV_ReadLevelFile (void);
void SV_Status_f (void);

//
// sv_ents.c
//
void SV_WriteFrameToClient (client_t *client, netMsg_t *msg);
void SV_RecordDemoMessage (void);
void SV_BuildClientFrame (client_t *client);


void SV_Error (char *error, ...);

//
// sv_game.c
//
extern	game_export_t	*ge;

void SV_InitGameProgs (void);
void SV_ShutdownGameProgs (void);
void SV_InitEdict (edict_t *e);



//============================================================

//
// high level object sorting to reduce interaction tests
//

void SV_ClearWorld (void);
// called after the world model has been loaded, before linking any entities

void SV_UnlinkEdict (edict_t *ent);
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself

void SV_LinkEdict (edict_t *ent);
// Needs to be called any time an entity changes origin, mins, maxs,
// or solid.  Automatically unlinks if needed.
// sets ent->v.absMin and ent->v.absMax
// sets ent->leafnums[] for pvs determination even if the entity
// is not solid

int SV_AreaEdicts (vec3_t mins, vec3_t maxs, edict_t **list, int maxcount, int areatype);
// fills in a table of edict pointers with edicts that have bounding boxes
// that intersect the given area.  It is possible for a non-axial bmodel
// to be returned that doesn't actually intersect the area on an exact
// test.
// returns the number of pointers filled in
// ??? does this always return the world?

//===================================================================

//
// functions that interact with everything apropriate
//
int SV_PointContents (vec3_t p);
// returns the CONTENTS_* value from the world at the given point.
// Quake 2 extends this to also check entities, to allow moving liquids


trace_t SV_Trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, edict_t *passedict, int contentmask);
// mins and maxs are relative

// if the entire move stays in a solid volume, trace.allsolid will be set,
// trace.startsolid will be set, and trace.fraction will be 0

// if the starting point is in a solid, it will be allowed to move out
// to an open area

// passedict is explicitly excluded from clipping checks (normally NULL)
