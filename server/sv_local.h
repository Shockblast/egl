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
// sv_local.h
//

#include "../common/common.h"
#include "../game/game.h"

extern struct memPool_s *sv_gameSysPool;
extern struct memPool_s *sv_genericPool;

/*
=============================================================================

	SERVER STATE

=============================================================================
*/

// some qc commands are only valid before the server has finished
// initializing (precache commands, static sounds / objects, etc)

typedef struct server_s {
	qBool				attractLoop;		// running cinematics and demos for the local system only
	qBool				loadGame;			// client begins should reuse existing entity

	uint32				time;				// always sv.frameNum * 100 msec
	int					frameNum;

	char				name[MAX_QPATH];	// map name, or cinematic name
	struct cBspModel_s	*models[MAX_CS_MODELS];

	char				configStrings[MAX_CFGSTRINGS][MAX_CFGSTRLEN];
	entityStateOld_t	baseLines[MAX_CS_EDICTS];

	// the multicast buffer is used to send a message to a set of clients
	// it is only used to marshall data until SV_Multicast is called
	netMsg_t			multiCast;
	byte				multiCastBuf[MAX_SV_MSGLEN];

	// demo server information
	fileHandle_t		demoFile;
	qBool				timeDemo;			// don't time sync
} serverState_t;

extern serverState_t	sv;					// local server

/*
=============================================================================

	SERVER CLIENT STATE

=============================================================================
*/

#define EDICT_NUM(n) ((edict_t *)((byte *)ge->edicts + ge->edictSize*(n)))
#define NUM_FOR_EDICT(e) (((byte *)(e)-(byte *)ge->edicts ) / ge->edictSize)

typedef struct clientFrame_s {
	int				areaBytes;
	byte			areaBits[MAX_AREA_BITS];		// portalarea visibility bits
	playerState_t	playerState;
	int				numEntities;
	int				firstEntity;					// into the circular sv_packet_entities[]
	int				sentTime;						// for ping calculations
} clientFrame_t;

#define LATENCY_COUNTS	16
#define RATE_MESSAGES	10

// svClient->state options
typedef enum svCLState_s {
	SVCS_FREE,		// can be reused for a new connection
	SVCS_ZOMBIE,	// client has been disconnected, but don't reuse
					// connection for a couple seconds
	SVCS_CONNECTED,	// has been assigned to a svClient_t, but not in game yet
	SVCS_SPAWNED	// client is fully in game
} svCLState_t;

typedef struct svClient_s {
	svCLState_t		state;

	char			userInfo[MAX_INFO_STRING];		// name, etc

	int				lastFrame;						// for delta compression
	userCmd_t		lastCmd;						// for filling in big drops

	int				commandMsec;					// every seconds this is reset, if user
													// commands exhaust it, assume time cheating

	int				frameLatency[LATENCY_COUNTS];
	int				ping;

	int				messageSize[RATE_MESSAGES];		// used to rate drop packets
	int				rate;
	int				surpressCount;					// number of messages rate supressed

	edict_t			*edict;							// EDICT_NUM(clientnum+1)
	char			name[32];						// extracted from userinfo, high bits masked
	int				messageLevel;					// for filtering printed messages

	// The datagram is written to by sound calls, prints, temp ents, etc.
	// It can be harmlessly overflowed.
	netMsg_t		datagram;
	byte			datagramBuff[MAX_SV_MSGLEN];

	clientFrame_t	frames[UPDATE_BACKUP];			// updates can be delta'd from here

	byte			*download;						// file being downloaded
	int				downloadSize;					// total bytes (can't use EOF because of paks)
	int				downloadCount;					// bytes sent

	int				lastMessage;					// sv.frameNum when packet was last received
	int				lastConnect;

	int				challenge;						// challenge of this user, randomly generated

	netChan_t		netChan;

	uint32			protocol;						// client protocol
} svClient_t;

// a client can leave the server in one of four ways:
// dropping properly by quiting or disconnecting
// timing out if no valid messages are received for timeout.value seconds
// getting kicked off by the server operator
// a program error, like an overflowed reliable buffer

/*
=============================================================================

	CONNECTION

	Persistant server info
=============================================================================
*/

// MAX_CHALLENGES is made large to prevent a denial
// of service attack that could cycle all of them
// out before legitimate users connected
#define MAX_CHALLENGES	1024

typedef struct challenge_s {
	netAdr_t	adr;
	int			challenge;
	uint32		time;
} challenge_t;

typedef struct serverStatic_s {
	qBool				initialized;				// sv_init has completed
	int					realTime;					// always increasing, no clamping, etc

	char				mapCmd[MAX_TOKEN_CHARS];	// ie: *intro.cin+base 

	int					spawnCount;					// incremented each server start -- used to check late spawns

	svClient_t			*clients;					// [maxclients->floatVal];
	int					numClientEntities;			// maxclients->floatVal*UPDATE_BACKUP*MAX_PACKET_ENTITIES
	int					nextClientEntities;			// next client_entity to use
	entityStateOld_t	*clientEntities;			// [numClientEntities]

	int					lastHeartBeat;

	challenge_t			challenges[MAX_CHALLENGES];	// to prevent invalid IPs from connecting

	// Serverrecord values
	fileHandle_t		demoFile;
	netMsg_t			demoMultiCast;
	byte				demoMultiCastBuf[MAX_SV_MSGLEN];
} serverStatic_t;

extern serverStatic_t	svs;

// Network transmission
extern netAdr_t			sv_netFrom;
extern netMsg_t			sv_netMessage;
extern byte				sv_netBuffer[MAX_SV_MSGLEN];

// ==========================================================================

#define MAX_MASTERS	8								// max recipients for heartbeat packets
extern netAdr_t	sv_masterAddresses[MAX_MASTERS];	// address of the master server

/*
=============================================================================

	CVARS

=============================================================================
*/

extern	cVar_t		*sv_paused;
extern	cVar_t		*maxclients;
extern	cVar_t		*sv_noreload;			// don't reload level state when reentering
extern	cVar_t		*sv_airaccelerate;		// don't reload level state when reentering
											// development tool
extern	cVar_t		*sv_enforcetime;

extern	svClient_t	*sv_currentClient;
extern	edict_t		*sv_currentEdict;

/*
=============================================================================

	WORLD

=============================================================================
*/

void	SV_ClearWorld (void);
// called after the world model has been loaded, before linking
// any entities

void	SV_UnlinkEdict (edict_t *ent);
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself

void	SV_LinkEdict (edict_t *ent);
// Needs to be called any time an entity changes origin, mins, maxs,
// or solid.  Automatically unlinks if needed.
// sets ent->v.absMin and ent->v.absMax
// sets ent->leafnums[] for pvs determination even if the entity
// is not solid

int		SV_AreaEdicts (vec3_t mins, vec3_t maxs, edict_t **list, int maxCount, int areaType);
// fills in a table of edict pointers with edicts that have
// bounding boxes that intersect the given area. It is possible
// for a non-axial bmodel to be returned that doesn't actually
// intersect the area on an exact test.
// returns the number of pointers filled in
// ??? does this always return the world?

// ==========================================================================

//
// functions that interact with everything apropriate
//
int		SV_PointContents (vec3_t p);
// returns the CONTENTS_* value from the world at the given point.
// Quake 2 extends this to also check entities, to allow moving liquids


trace_t	SV_Trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, edict_t *passEdict, int contentMask);
// mins and maxs are relative

// if the entire move stays in a solid volume, trace.allsolid will be set,
// trace.startsolid will be set, and trace.fraction will be 0

// if the starting point is in a solid, it will be allowed to move out
// to an open area

// passedict is explicitly excluded from clipping checks (normally NULL)

// ==========================================================================

//
// sv_ccmds.c
//

void		SV_ReadLevelFile (void);

//
// sv_ents.c
//

void		SV_WriteFrameToClient (svClient_t *client, netMsg_t *msg);
void		SV_RecordDemoMessage (void);
void		SV_BuildClientFrame (svClient_t *client);

//
// sv_gameapi.c
//

extern gameExport_t		*ge;

void		SV_GameAPI_Init (void);
void		SV_GameAPI_Shutdown (void);

//
// sv_init.c
//

void		SV_GameInit (void);
void		SV_LoadMap (qBool attractLoop, char *levelString, qBool loadGame, qBool devMap);

//
// sv_main.c
//

void		SV_SetState (ssState_t state);
void		SV_DropClient (svClient_t *drop);
void		SV_UserinfoChanged (svClient_t *cl);
void		SV_UpdateTitle (void);

void		SV_OperatorCommandInit (void);

//
// sv_pmove.c
//

extern float	sv_airAcceleration;

void		SV_Pmove (pMoveNew_t *pMove, float airAcceleration);

//
// sv_send.c
//

void		SV_SendClientMessages (void);

void		SV_Unicast (edict_t *ent, qBool reliable);
void		SV_Multicast (vec3_t origin, multiCast_t to);

void		SV_StartSound (vec3_t origin, edict_t *entity, int channel, int soundIndex, float volume, float attenuation, float timeOffset);
void		SV_ClientPrintf (svClient_t *cl, int level, char *fmt, ...);
void		SV_BroadcastPrintf (int level, char *fmt, ...);
void		SV_BroadcastCommand (char *fmt, ...);

//
// sv_user.c
//

void		SV_Nextserver (void);
void		SV_ExecuteClientMessage (svClient_t *cl);
