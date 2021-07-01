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
// protocol.h
//

/*
=============================================================================

	PROTOCOL

=============================================================================
*/

#define UPDATE_BACKUP		16	// copies of entityState_t to keep buffered must be power of two
#define UPDATE_MASK			(UPDATE_BACKUP-1)

//
// client to server
//
enum {
	CLC_BAD,
	CLC_NOP,
	CLC_MOVE,				// [[userCmd_t]
	CLC_USERINFO,			// [[userinfo string]
	CLC_STRINGCMD,			// [string] message
	CLC_SETTING				// [setting][value] R1Q2 settings support.
};

enum {
	CLSET_NOGUN,
	CLSET_NOBLEND,
	CLSET_RECORDING,
	CLSET_MAX
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
	PS_RDFLAGS			= 1 << 14,

	PS_BBOX				= 1 << 15		// for new ENHANCED_PROTOCOL_VERSION
};

// enhanced protocol
enum {
	EPS_GUNOFFSET		= 1 << 0,
	EPS_GUNANGLES		= 1 << 1,
	EPS_PMOVE_VELOCITY2	= 1 << 2,
	EPS_PMOVE_ORIGIN2	= 1 << 3,
	EPS_VIEWANGLE2		= 1 << 4,
	EPS_STATS			= 1 << 5
};


// userCmd_t communication
// ms and light always sent, the others are optional
enum {
	CM_ANGLE1			= 1 << 0,
	CM_ANGLE2			= 1 << 1,
	CM_ANGLE3			= 1 << 2,
	CM_FORWARD			= 1 << 3,
	CM_SIDE				= 1 << 4,
	CM_UP				= 1 << 5,
	CM_BUTTONS			= 1 << 6,
	CM_IMPULSE			= 1 << 7
};


// sound communication
// a sound without an ent or pos will be a local only sound
enum {
	SND_VOLUME			= 1 << 0,	// a byte
	SND_ATTENUATION		= 1 << 1,	// a byte
	SND_POS				= 1 << 2,	// three coordinates
	SND_ENT				= 1 << 3,	// a short 0-2: channel, 3-12: entity
	SND_OFFSET			= 1 << 4	// a byte, msec offset from frame start
};

#define DEFAULT_SOUND_PACKET_VOLUME			1.0
#define DEFAULT_SOUND_PACKET_ATTENUATION	1.0

// entityState_t communication
enum {
	// try to pack the common update flags into the first byte
	U_ORIGIN1			= 1 << 0,
	U_ORIGIN2			= 1 << 1,
	U_ANGLE2			= 1 << 2,
	U_ANGLE3			= 1 << 3,
	U_FRAME8			= 1 << 4,		// frame is a byte
	U_EVENT				= 1 << 5,
	U_REMOVE			= 1 << 6,		// REMOVE this entity, don't add it
	U_MOREBITS1			= 1 << 7,		// read one additional byte

	// second byte
	U_NUMBER16			= 1 << 8,		// NUMBER8 is implicit if not set
	U_ORIGIN3			= 1 << 9,
	U_ANGLE1			= 1 << 10,
	U_MODEL				= 1 << 11,
	U_RENDERFX8			= 1 << 12,		// fullbright, etc
	U_EFFECTS8			= 1 << 14,		// autorotate, trails, etc
	U_MOREBITS2			= 1 << 15,		// read one additional byte

	// third byte
	U_SKIN8				= 1 << 16,
	U_FRAME16			= 1 << 17,		// frame is a short
	U_RENDERFX16		= 1 << 18,		// 8 + 16 = 32
	U_EFFECTS16			= 1 << 19,		// 8 + 16 = 32
	U_MODEL2			= 1 << 20,		// weapons, flags, etc
	U_MODEL3			= 1 << 21,
	U_MODEL4			= 1 << 22,
	U_MOREBITS3			= 1 << 23,		// read one additional byte

	// fourth byte
	U_OLDORIGIN			= 1 << 24,		// FIXME: get rid of this
	U_SKIN16			= 1 << 25,
	U_SOUND				= 1 << 26,
	U_SOLID				= 1 << 27,

	U_VELOCITY			= 1 << 28		// new for ENHANCED_PROTOCOL_VERSION
};

/*
=============================================================================

	MESSAGE FUNCTIONS

=============================================================================
*/

typedef struct netMsg_s {
	qBool		allowOverflow;	// if false, do a Com_Error
	qBool		overFlowed;		// set to true if the buffer size failed
	byte		*data;
	int			maxSize;
	int			curSize;
	int			readCount;
	int			bufferSize;
} netMsg_t;

// supporting functions
void	MSG_Init (netMsg_t *buf, byte *data, int length);
void	MSG_Clear (netMsg_t *buf);
void	*MSG_GetSpace (netMsg_t *buf, int length);
void	MSG_Write (netMsg_t *buf, void *data, int length);
void	MSG_Print (netMsg_t *buf, char *data);	// strcats onto the sizebuf

// writing
#define MSG_WriteAngle(msg,f)	(MSG_WriteByte((msg), ANGLE2BYTE ((f))))
#define MSG_WriteAngle16(msg,f)	(MSG_WriteShort((msg), ANGLE2SHORT ((f))))
#define MSG_WriteCoord(msg,f)	(MSG_WriteShort((msg), (int)((f)*8)))
#define MSG_WritePos(msg,pos)	(MSG_WriteCoord((msg),(pos)[0]), MSG_WriteCoord((msg),(pos)[1]), MSG_WriteCoord((msg),(pos)[2]))

void	MSG_WriteByte (netMsg_t *dest, int c);
void	MSG_WriteChar (netMsg_t *dest, int c);
void	MSG_WriteDeltaUsercmd (netMsg_t *dest, struct userCmd_s *from, struct userCmd_s *cmd);
void	MSG_WriteDeltaEntity (entityStateOld_t *from, entityStateOld_t *to, netMsg_t *dest, qBool force, qBool newEntity);
void	MSG_WriteDir (netMsg_t *dest, vec3_t vector);
void	MSG_WriteFloat (netMsg_t *dest, float f);
void	MSG_WriteInt3 (netMsg_t *dest, int c);
void	MSG_WriteLong (netMsg_t *dest, int c);
void	MSG_WriteShort (netMsg_t *dest, int c);
void	MSG_WriteString (netMsg_t *dest, char *s);

// reading
#define MSG_ReadAngle(msg)		(BYTE2ANGLE (MSG_ReadChar ((msg))))
#define MSG_ReadAngle16(msg)	(SHORT2ANGLE (MSG_ReadShort ((msg))))
#define MSG_ReadCoord(msg)		(MSG_ReadShort((msg)) * (1.0f/8.0f))
#define MSG_ReadPos(msg,pos)	((pos)[0]=MSG_ReadCoord((msg)), (pos)[1]=MSG_ReadCoord((msg)), (pos)[2]=MSG_ReadCoord((msg)))

void	MSG_BeginReading (netMsg_t *src);
int		MSG_ReadByte (netMsg_t *src);
int		MSG_ReadChar (netMsg_t *src);
void	MSG_ReadData (netMsg_t *src, void *buffer, int size);
void	MSG_ReadDeltaUsercmd (netMsg_t *src, struct userCmd_s *from, struct userCmd_s *cmd);
void	MSG_ReadDir (netMsg_t *src, vec3_t vector);
float	MSG_ReadFloat (netMsg_t *src);
int		MSG_ReadInt3 (netMsg_t *src);
int		MSG_ReadLong (netMsg_t *src);
int		MSG_ReadShort (netMsg_t *src);
char	*MSG_ReadString (netMsg_t *src);
char	*MSG_ReadStringLine (netMsg_t *src);

/*
=============================================================================

	NET

=============================================================================
*/

// Default ports
#define PORT_ANY		-1
#define PORT_MASTER		27900
#define PORT_SERVER		27910

// Maximum message lengths
#define PACKET_HEADER		10			// two ints and a short

#define MAX_SV_MSGLEN		1400
#define MAX_SV_USABLEMSG	(MAX_SV_MSGLEN - PACKET_HEADER)

#define MAX_CL_MSGLEN		4096
#define MAX_CL_USABLEMSG	(MAX_CL_MSGLEN - PACKET_HEADER)

typedef struct netStats_s {
	qBool		initialized;
	uint32		initTime;

	uint32		sizeIn;
	uint32		sizeOut;

	uint32		packetsIn;
	uint32		packetsOut;
} netStats_t;

extern netStats_t	netStats;

enum {
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,

	NA_MAX
};

enum {
	NS_CLIENT,
	NS_SERVER,

	NS_MAX
};

enum {
	NET_NONE,

	NET_CLIENT,
	NET_SERVER,
};

typedef struct netAdr_s {
	int			naType;

	byte		ip[4];

	uint16		port;
} netAdr_t;

// Compares with the port
#define NET_CompareAdr(a,b)					\
(a.naType != b.naType) ? qFalse :			\
(a.naType == NA_LOOPBACK) ? qTrue :			\
(a.naType == NA_IP) ?						\
	((a.ip[0] == b.ip[0]) &&				\
	(a.ip[1] == b.ip[1]) &&					\
	(a.ip[2] == b.ip[2]) &&					\
	(a.ip[3] == b.ip[3]) &&					\
	(a.port == b.port)) ? qTrue : qFalse	\
	: qFalse

// Compares without the port
#define NET_CompareBaseAdr(a,b)				\
(a.naType != b.naType) ? qFalse :			\
(a.naType == NA_LOOPBACK) ? qTrue :			\
(a.naType == NA_IP) ?						\
	((a.ip[0] == b.ip[0]) &&				\
	(a.ip[1] == b.ip[1]) &&					\
	(a.ip[2] == b.ip[2]) &&					\
	(a.ip[3] == b.ip[3])) ? qTrue : qFalse	\
	: qFalse

// Converts from sockaddr_in to netAdr_t
#define NET_SockAdrToNetAdr(s,a)			\
	a->naType = NA_IP;						\
	*(int *)&a->ip = ((struct sockaddr_in *)s)->sin_addr.s_addr;	\
	a->port = ((struct sockaddr_in *)s)->sin_port;

// Checks if an address is a loopback address
#ifdef WIN32
# define NET_IsLocalAddress(adr) ((adr.naType == NA_LOOPBACK) ? qTrue : qFalse)
#else
qBool	NET_IsLocalAddress (netAdr_t adr);
#endif

void	NET_Init (void);
void	NET_Shutdown (void);

int		NET_Config (int openFlags);

qBool	NET_GetPacket (int sock, netAdr_t *fromAddr, netMsg_t *message);
int		NET_SendPacket (int sock, int length, void *data, netAdr_t to);

char	*NET_AdrToString (netAdr_t a);
qBool	NET_StringToAdr (char *s, netAdr_t *a);
void	NET_Sleep (int msec);

typedef struct netChan_s {
	qBool		fatalError;

	int			sock;

	int			dropped;						// between last packet and previous

	int			lastReceived;					// for timeouts
	int			lastSent;						// for retransmits

	netAdr_t	remoteAddress;
	int			qPort;							// qport value to write when transmitting
	int			protocol;

	// Sequencing variables
	int			incomingSequence;
	int			incomingAcknowledged;
	int			incomingReliableAcknowledged;	// single bit
	int			incomingReliableSequence;		// single bit, maintained local
	qBool		gotReliable;

	int			outgoingSequence;
	int			reliableSequence;				// single bit
	int			lastReliableSequence;			// sequence number of last send

	// Reliable staging and holding areas
	netMsg_t	message;		// writing buffer to send to server
	byte		messageBuff[MAX_CL_USABLEMSG];	// leave space for header

	// Message is copied to this buffer when it is first transfered
	int			reliableLength;
	byte		reliableBuff[MAX_CL_USABLEMSG];	// unacked reliable message
} netChan_t;

void	Netchan_Init (void);
void	Netchan_Setup (int sock, netChan_t *chan, netAdr_t adr, int protocol, int qPort, uint32 msgLen);

int		Netchan_Transmit (netChan_t *chan, int length, byte *data);
void	Netchan_OutOfBand (int net_socket, netAdr_t adr, int length, byte *data);
void	Netchan_OutOfBandPrint (int net_socket, netAdr_t adr, char *format, ...);
qBool	Netchan_Process (netChan_t *chan, netMsg_t *msg);
