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

// common.h
// - definitions common between client and server, but not game.dll

#ifndef __COMMON_H__
#define __COMMON_H__

#include "../shared/shared.h"
#include "../common/files.h"
#include "font.h"

// sub-system queued restarting
extern qBool	snd_Restart;
extern qBool	ui_Restart;
extern qBool	vid_Restart;

#define	BASE_MODDIRNAME		"baseq2"

/*
=============================================================================

	PROTOCOL

=============================================================================
*/

#define	PROTOCOL_VERSION	34

#define	UPDATE_BACKUP		16	// copies of entityState_t to keep buffered must be power of two
#define	UPDATE_MASK			(UPDATE_BACKUP-1)

//
// client to server
//
enum {
	CLC_BAD,
	CLC_NOP,
	CLC_MOVE,				// [[userCmd_t]
	CLC_USERINFO,			// [[userinfo string]
	CLC_STRINGCMD			// [string] message
};


//
// server to client
// the svc_strings[] array in cl_parse.c should mirror this
//
enum svc_ops_e {
	SVC_BAD,

	// these ops are known to the game dll
	SVC_MUZZLEFLASH,
	SVC_MUZZLEFLASH2,
	SVC_TEMP_ENTITY,
	SVC_LAYOUT,
	SVC_INVENTORY,

	// the rest are private to the client and server
	SVC_NOP,
	SVC_DISCONNECT,
	SVC_RECONNECT,
	SVC_SOUND,					// <see code>
	SVC_PRINT,					// [byte] id [string] null terminated string
	SVC_STUFFTEXT,				// [string] stuffed into client's console buffer, should be \n terminated
	SVC_SERVERDATA,				// [long] protocol ...
	SVC_CONFIGSTRING,			// [short] [string]
	SVC_SPAWNBASELINE,		
	SVC_CENTERPRINT,			// [string] to put in center of the screen
	SVC_DOWNLOAD,				// [short] size [size bytes]
	SVC_PLAYERINFO,				// variable
	SVC_PACKETENTITIES,			// [...]
	SVC_DELTAPACKETENTITIES,	// [...]
	SVC_FRAME
};


// playerState_t communication
enum {
	PS_M_TYPE			= 1 << 0,
	PS_M_ORIGIN			= 1 << 1,
	PS_M_VELOCITY		= 1 << 2,
	PS_M_TIME			= 1 << 3,
	PS_M_FLAGS			= 1 << 4,
	PS_M_GRAVITY		= 1 << 5,
	PS_M_DELTA_ANGLES	= 1 << 6,

	PS_VIEWOFFSET		= 1 << 7,
	PS_VIEWANGLES		= 1 << 8,
	PS_KICKANGLES		= 1 << 9,
	PS_BLEND			= 1 << 10,
	PS_FOV				= 1 << 11,
	PS_WEAPONINDEX		= 1 << 12,
	PS_WEAPONFRAME		= 1 << 13,
	PS_RDFLAGS			= 1 << 14
};


// userCmd_t communication
// ms and light always sent, the others are optional
enum {
	CM_ANGLE1		= 1 << 0,
	CM_ANGLE2		= 1 << 1,
	CM_ANGLE3		= 1 << 2,
	CM_FORWARD		= 1 << 3,
	CM_SIDE			= 1 << 4,
	CM_UP			= 1 << 5,
	CM_BUTTONS		= 1 << 6,
	CM_IMPULSE		= 1 << 7
};


// sound communication
// a sound without an ent or pos will be a local only sound
enum {
	SND_VOLUME		= 1 << 0,	// a byte
	SND_ATTENUATION	= 1 << 1,	// a byte
	SND_POS			= 1 << 2,	// three coordinates
	SND_ENT			= 1 << 3,	// a short 0-2: channel, 3-12: entity
	SND_OFFSET		= 1 << 4	// a byte, msec offset from frame start
};

#define DEFAULT_SOUND_PACKET_VOLUME			1.0
#define DEFAULT_SOUND_PACKET_ATTENUATION	1.0

// entityState_t communication
enum {
	// try to pack the common update flags into the first byte
	U_ORIGIN1		= 1 << 0,
	U_ORIGIN2		= 1 << 1,
	U_ANGLE2		= 1 << 2,
	U_ANGLE3		= 1 << 3,
	U_FRAME8		= 1 << 4,		// frame is a byte
	U_EVENT			= 1 << 5,
	U_REMOVE		= 1 << 6,		// REMOVE this entity, don't add it
	U_MOREBITS1		= 1 << 7,		// read one additional byte

	// second byte
	U_NUMBER16		= 1 << 8,		// NUMBER8 is implicit if not set
	U_ORIGIN3		= 1 << 9,
	U_ANGLE1		= 1 << 10,
	U_MODEL			= 1 << 11,
	U_RENDERFX8		= 1 << 12,		// fullbright, etc
	U_EFFECTS8		= 1 << 14,		// autorotate, trails, etc
	U_MOREBITS2		= 1 << 15,		// read one additional byte

	// third byte
	U_SKIN8			= 1 << 16,
	U_FRAME16		= 1 << 17,		// frame is a short
	U_RENDERFX16	= 1 << 18,		// 8 + 16 = 32
	U_EFFECTS16		= 1 << 19,		// 8 + 16 = 32
	U_MODEL2		= 1 << 20,		// weapons, flags, etc
	U_MODEL3		= 1 << 21,
	U_MODEL4		= 1 << 22,
	U_MOREBITS3		= 1 << 23,		// read one additional byte

	// fourth byte
	U_OLDORIGIN		= 1 << 24,		// FIXME: get rid of this
	U_SKIN16		= 1 << 25,
	U_SOUND			= 1 << 26,
	U_SOLID			= 1 << 27
};

/*
=============================================================================

	MESSAGE FUNCTIONS

=============================================================================
*/

typedef struct {
	qBool		allowOverflow;	// if false, do a Com_Error
	qBool		overFlowed;		// set to true if the buffer size failed
	byte		*data;
	int			maxSize;
	int			curSize;
	int			readCount;
} netMsg_t;

// supporting functions
void	MSG_Init (netMsg_t *buf, byte *data, int length);
void	MSG_Clear (netMsg_t *buf);
void	*MSG_GetSpace (netMsg_t *buf, int length);
void	MSG_Write (netMsg_t *buf, void *data, int length);
void	MSG_Print (netMsg_t *buf, char *data);	// strcats onto the sizebuf

// writing
void	MSG_WriteChar (netMsg_t *sb, int c);
void	MSG_WriteByte (netMsg_t *sb, int c);
void	MSG_WriteShort (netMsg_t *sb, int c);
void	MSG_WriteLong (netMsg_t *sb, int c);
void	MSG_WriteFloat (netMsg_t *sb, float f);
void	MSG_WriteString (netMsg_t *sb, char *s);
void	MSG_WriteCoord (netMsg_t *sb, float f);
void	MSG_WritePos (netMsg_t *sb, vec3_t pos);
void	MSG_WriteAngle (netMsg_t *sb, float f);
void	MSG_WriteAngle16 (netMsg_t *sb, float f);
void	MSG_WriteDeltaUsercmd (netMsg_t *sb, struct userCmd_s *from, struct userCmd_s *cmd);
void	MSG_WriteDeltaEntity (entityState_t *from, entityState_t *to, netMsg_t *msg, qBool force, qBool newentity);
void	MSG_WriteDir (netMsg_t *sb, vec3_t vector);

// reading
void	MSG_BeginReading (netMsg_t *sb);
int		MSG_ReadChar (netMsg_t *sb);
int		MSG_ReadByte (netMsg_t *sb);
int		MSG_ReadShort (netMsg_t *sb);
int		MSG_ReadLong (netMsg_t *sb);
float	MSG_ReadFloat (netMsg_t *sb);
char	*MSG_ReadString (netMsg_t *sb);
char	*MSG_ReadStringLine (netMsg_t *sb);
float	MSG_ReadCoord (netMsg_t *sb);
void	MSG_ReadPos (netMsg_t *sb, vec3_t pos);
float	MSG_ReadAngle (netMsg_t *sb);
float	MSG_ReadAngle16 (netMsg_t *sb);
void	MSG_ReadDeltaUsercmd (netMsg_t *sb, struct userCmd_s *from, struct userCmd_s *cmd);
void	MSG_ReadDir (netMsg_t *sb, vec3_t vector);
void	MSG_ReadData (netMsg_t *sb, void *buffer, int size);

/*
=============================================================================

	COMMANDS

=============================================================================
*/

enum {
	EXEC_NOW	= 0,	// don't return until completed
	EXEC_INSERT,		// insert at current position, but don't run yet
	EXEC_APPEND			// add to end of the command buffer
};

void	Cbuf_AddText (char *text);

void	Cbuf_InsertText (char *text);
void	Cbuf_ExecuteText (int execWhen, char *text);
void	Cbuf_Execute (void);

void	Cbuf_CopyToDefer (void);
void	Cbuf_InsertFromDefer (void);

// ==========================================================================

void	Cmd_AddCommand (char *cmd_name, void (*function) (void));
void	Cmd_RemoveCommand (char *cmd_name);

qBool	Cmd_Exists (char *cmd_name);
char	*Cmd_CompleteCommand (char *partial);

int		Cmd_Argc (void);
char	*Cmd_Argv (int arg);
char	*Cmd_Args (void);

void	Cmd_TokenizeString (char *text, qBool macroExpand);
void	Cmd_ExecuteString (char *text);
void	Cmd_ForwardToServer (void);

void	Cmd_Init (void);
void	Cmd_Shutdown (void);

/*
=============================================================================

	CVAR

=============================================================================
*/

extern	cvar_t		*cvar_Variables;

extern	qBool		cvar_UserInfoModified;

cvar_t		*Cvar_Get (char *varName, char *value, int flags);
void		Cvar_GetLatchedVars (int flags);

int			Cvar_VariableInteger (char *varName);
char		*Cvar_VariableString (char *varName);
float		Cvar_VariableValue (char *varName);

qBool		Cvar_Command (void);

cvar_t		*Cvar_ForceSet (char *varName, char *value);
cvar_t		*Cvar_ForceSetValue (char *varName, float value);
cvar_t		*Cvar_FullSet (char *varName, char *value, int flags);
cvar_t		*Cvar_Set (char *varName, char *value);
void		Cvar_SetValue (char *varName, float value);
void		Cvar_WriteVariables (FILE *f);
char		*Cvar_CompleteVariable (char *partial);

char		*Cvar_Userinfo (void);
char		*Cvar_Serverinfo (void);

void		Cvar_Init (void);
void		Cvar_Shutdown (void);

/*
=============================================================================

	NET

=============================================================================
*/

#define	PORT_ANY		-1
#define	PORT_MASTER		27900
#define PORT_CLIENT		27901
#define	PORT_SERVER		27910

#define	MAX_MSGLEN		1400		// max length of a message

enum {
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
	NA_IPX,
	NA_BROADCAST_IPX
};

enum {
	NS_CLIENT,
	NS_SERVER
};

typedef struct {
	int				naType;

	byte			ip[4];
	byte			ipx[10];

	unsigned short	port;
} netAdr_t;

void	NET_Init (void);
void	NET_Shutdown (void);

void	NET_Config (qBool multiplayer);

qBool	NET_GetPacket (int sock, netAdr_t *fromAddr, netMsg_t *message);
void	NET_SendPacket (int sock, int length, void *data, netAdr_t to);

qBool	NET_CompareAdr (netAdr_t a, netAdr_t b);
qBool	NET_CompareBaseAdr (netAdr_t a, netAdr_t b);
qBool	NET_IsLocalAddress (netAdr_t adr);
char	*NET_AdrToString (netAdr_t a);
qBool	NET_StringToAdr (char *s, netAdr_t *a);
void	NET_Sleep (int msec);

typedef struct {
	qBool		fatalError;

	int			sock;

	int			dropped;			// between last packet and previous

	int			lastReceived;		// for timeouts
	int			lastSent;			// for retransmits

	netAdr_t	remoteAddress;
	int			qPort;				// qport value to write when transmitting

	// sequencing variables
	int			incomingSequence;
	int			incomingAcknowledged;
	int			incomingReliableAcknowledged;	// single bit
	int			incomingReliableSequence;		// single bit, maintained local

	int			outgoingSequence;
	int			reliableSequence;			// single bit
	int			lastReliableSequence;		// sequence number of last send

	// reliable staging and holding areas
	netMsg_t	message;		// writing buffer to send to server
	byte		messageBuff[MAX_MSGLEN-16];		// leave space for header

	// message is copied to this buffer when it is first transfered
	int			reliableLength;
	byte		reliableBuff[MAX_MSGLEN-16];	// unacked reliable message
} netChan_t;

extern	netAdr_t	net_From;
extern	netMsg_t	net_Message;
extern	byte		net_MessageBuffer[MAX_MSGLEN];

void	Netchan_Init (void);
void	Netchan_Setup (int sock, netChan_t *chan, netAdr_t adr, int qport);

qBool	Netchan_NeedReliable (netChan_t *chan);
void	Netchan_Transmit (netChan_t *chan, int length, byte *data);
void	Netchan_OutOfBand (int net_socket, netAdr_t adr, int length, byte *data);
void	Netchan_OutOfBandPrint (int net_socket, netAdr_t adr, char *format, ...);
qBool	Netchan_Process (netChan_t *chan, netMsg_t *msg);

qBool	Netchan_CanReliable (netChan_t *chan);

/*
=============================================================================

	CMODEL

=============================================================================
*/

cBspModel_t	*CM_LoadMap (char *name, qBool clientload, unsigned *checksum);
void		CM_UnloadMap (void);
cBspModel_t	*CM_InlineModel (char *name);	// *1, *2, etc

int			CM_NumClusters (void);
int			CM_NumInlineModels (void);
char		*CM_EntityString (void);

int			CM_HeadnodeForBox (vec3_t mins, vec3_t maxs);	// creates a clipping hull for an arbitrary box

int			CM_PointContents (vec3_t p, int headnode);	// returns an ORed contents mask
int			CM_TransformedPointContents (vec3_t p, int headnode, vec3_t origin, vec3_t angles);

trace_t		CL_Trace (vec3_t start, vec3_t end, float size,  int contentmask);
trace_t		CM_BoxTrace (vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs,  int headnode, int brushmask);
trace_t		CM_TransformedBoxTrace (vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int headnode,
									int brushmask, vec3_t origin, vec3_t angles);

byte		*CM_ClusterPVS (int cluster);
byte		*CM_ClusterPHS (int cluster);

int			CM_PointLeafnum (vec3_t p);

// call with topnode set to the headnode, returns with topnode
// set to the first node that splits the box
int			CM_BoxLeafnums (vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode);

int			CM_LeafContents (int leafnum);
int			CM_LeafCluster (int leafnum);
int			CM_LeafArea (int leafnum);

void		CM_SetAreaPortalState (int portalnum, qBool open);
qBool		CM_AreasConnected (int area1, int area2);

int			CM_WriteAreaBits (byte *buffer, int area);
qBool		CM_HeadnodeVisible (int headnode, byte *visbits);

void		CM_WritePortalState (FILE *f);
void		CM_ReadPortalState (FILE *f);

/*
=============================================================================

	FILESYSTEM

=============================================================================
*/

void	FS_CreatePath (char *path);

void	FS_CopyFile (char *src, char *dst);
void	FS_FCloseFile (FILE *f);	// note: this can't be called from another DLL, due to MS libc issues
int		FS_FOpenFile (char *filename, FILE **file);
void	FS_Read (void *buffer, int len, FILE *f);	// properly handles partial reads

// a null buffer will just return the file length without loading a -1 length is not present
int		FS_LoadFile (char *path, void **buffer);
void	FS_FreeFile (void *buffer);
void	FS_FreeFileList (char **list, int n);

qBool	FS_CurrGame (char *name);
char	*FS_Gamedir (void);
void	FS_SetGamedir (char *dir);

void	FS_ExecAutoexec (void);

char	**FS_ListFiles (char *findName, int *numFiles, unsigned mustHave, unsigned cantHave);
char	*FS_NextPath (char *prevpath);
char	*FS_FindFirst (char *find);
char	*FS_FindNext (char *find);

void	FS_Init (void);
void	FS_Shutdown (void);

/*
=============================================================================

	MISCELLANEOUS

=============================================================================
*/

// Com_Error flags
enum {
	ERR_FATAL,				// exit the entire game with a popup window
	ERR_DROP,				// print to console and disconnect from game
	ERR_DISCONNECT			// don't kill server
};

extern cvar_t	*developer;
extern cvar_t	*dedicated;
extern cvar_t	*host_speeds;
extern cvar_t	*log_stats;

extern FILE		*logStatsFP;

// host_speeds times
extern int		time_before_game;
extern int		time_after_game;
extern int		time_before_ref;
extern int		time_after_ref;

// client/server interactions
void	Com_BeginRedirect (int target, char *buffer, int buffersize, void (*flush));
void	Com_EndRedirect (void);
void	Com_Printf (int flags, char *fmt, ...);
void	Com_Error (int code, char *fmt, ...);
void	Com_Quit (void);
int		Com_ServerState (void);		// this should have just been a cvar...
void	Com_SetServerState (int state);

// misc
float	crand (void);	// -1 to 1
float	frand (void);	// 0 to 1
int		Com_WildCmp (const char *filter, const char *string, int ignoreCase);
void	Com_WriteConfig (void);

// initialization and processing
void	Com_Init (int argc, char **argv);
void	FASTCALL Com_Frame (int msec);
void	Com_Shutdown (void);

// misc functions
byte			Com_BlockSequenceCRCByte (byte *base, int length, int sequence);
unsigned int	Com_BlockChecksum (void *buffer, int length);

/*
==============================================================================

	ZONE MEMORY ALLOCATION

==============================================================================
*/

#define ZTAG_BASE		( ( 'E' + 'G' + 'L' ) + 1024 )

#define ZTAG_CMDSYS		( ZTAG_BASE + 00 )
#define ZTAG_CVARSYS	( ZTAG_BASE + 01 )
#define ZTAG_FILESYS	( ZTAG_BASE + 02 )
#define ZTAG_IMAGESYS	( ZTAG_BASE + 03 )
#define ZTAG_KEYSYS		( ZTAG_BASE + 04 )
#define ZTAG_SHADERSYS	( ZTAG_BASE + 05 )
#define ZTAG_SOUNDSYS	( ZTAG_BASE + 06 )

void	Z_Free (void *ptr);
void	Z_FreeTags (int tag);
void	*Z_Malloc (int size);		// returns 0 filled memory
void	*Z_TagMalloc (int size, int tag);

void	Z_Init (void);
void	Z_Shutdown (void);

/*
==============================================================================

	NON-PORTABLE SYSTEM SERVICES

==============================================================================
*/

enum {
	LIB_CGAME,
	LIB_GAME,
	LIB_UI
};

void	Sys_Init (void);
void	Sys_AppActivate (void);

void	Sys_UnloadLibrary (int libType);
void	*Sys_LoadLibrary (int libType, void *parms);

char	*Sys_ConsoleInput (void);
void	Sys_ConsoleOutput (char *string);
void	Sys_SendKeyEvents (void);
void	Sys_Error (char *error, ...);
void	Sys_Quit (void);
char	*Sys_GetClipboardData (void);

/*
==============================================================

	PLAYER MOVEMENT

	Common between the server and client for consistancy
==============================================================
*/

extern float	pm_airaccelerate;

void	Pmove (pMove_t *pmove);

enum {
	GLM_DEFAULT,

	GLM_OBSERVER,
	GLM_BREEDER,
	GLM_HATCHLING,
	GLM_DRONE,
	GLM_WRAITH,
	GLM_KAMIKAZE,
	GLM_STINGER,
	GLM_GUARDIAN,
	GLM_STALKER,
	GLM_ENGINEER,
	GLM_GRUNT,
	GLM_ST,
	GLM_BIOTECH,
	GLM_HT,
	GLM_COMMANDO,
	GLM_EXTERM,
	GLM_MECH
};

/*
==============================================================

	CLIENT / SERVER SYSTEMS

==============================================================
*/

//
// console.c
//

void	Con_Init (void);
void	Con_Print (int flags, const char *text);

//
// cl_main.c
//

extern	cvar_t	*glm_jumppred;

void	CL_Drop (void);
void	FASTCALL CL_Frame (int msec);
void	CL_Init (void);
void	CL_Shutdown (void);

//
// cl_parse.c
//

extern int	classtype;

//
// cl_screen.c
//

void	SCR_BeginLoadingPlaque (void);
void	SCR_EndLoadingPlaque (void);

// this is in the client code, but can be used for debugging from server
void	SCR_DebugGraph (float value, int color);

//
// keys.c
//

void	Key_WriteBindings (FILE *f);

void	Key_Init (void);
void	Key_Shutdown (void);

//
// sv_main.c
//

void	SV_Init (void);
void	SV_Shutdown (char *finalmsg, qBool reconnect);
void	SV_Frame (int msec);

#endif // __COMMON_H__