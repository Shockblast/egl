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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

//
// net_udp.c
//

#include "../common/common.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <errno.h>

#ifdef NeXT
#include <libc.h>
#endif

netAdr_t	net_local_adr;

#define	LOOPBACK	0x7f000001

#define	MAX_LOOPBACK	4

typedef struct
{
	byte	data[MAX_MSGLEN];
	int		datalen;
} loopmsg_t;

typedef struct
{
	loopmsg_t	msgs[MAX_LOOPBACK];
	int			get, send;
} loopback_t;

loopback_t	loopbacks[2];
int			ipSockets[2];

int NET_Socket (char *net_interface, int port);
char *NET_ErrorString (void);

//=============================================================================

void NetadrToSockadr (netAdr_t *a, struct sockaddr_in *s)
{
	memset (s, 0, sizeof(*s));

	if (a->naType == NA_BROADCAST)
	{
		s->sin_family = AF_INET;

		s->sin_port = a->port;
		*(int *)&s->sin_addr = -1;
	}
	else if (a->naType == NA_IP)
	{
		s->sin_family = AF_INET;

		*(int *)&s->sin_addr = *(int *)&a->ip;
		s->sin_port = a->port;
	}
}

char	*NET_AdrToString (netAdr_t a)
{
	static	char	s[64];
	
	Q_snprintfz (s, sizeof(s), "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], ntohs(a.port));

	return s;
}

char	*NET_BaseAdrToString (netAdr_t a)
{
	static	char	s[64];
	
	Q_snprintfz (s, sizeof(s), "%i.%i.%i.%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3]);

	return s;
}

/*
=============
NET_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
qBool	NET_StringToSockaddr (char *s, struct sockaddr *sadr)
{
	struct hostent	*h;
	char	*colon;
	char	copy[128];
	
	memset (sadr, 0, sizeof(*sadr));
	((struct sockaddr_in *)sadr)->sin_family = AF_INET;
	
	((struct sockaddr_in *)sadr)->sin_port = 0;

	strcpy (copy, s);
	// strip off a trailing :port if present
	for (colon = copy ; *colon ; colon++)
		if (*colon == ':')
		{
			*colon = 0;
			((struct sockaddr_in *)sadr)->sin_port = htons((short)atoi(colon+1));	
		}
	
	if (copy[0] >= '0' && copy[0] <= '9')
	{
		*(int *)&((struct sockaddr_in *)sadr)->sin_addr = inet_addr(copy);
	}
	else
	{
		if (! (h = gethostbyname(copy)) )
			return 0;
		*(int *)&((struct sockaddr_in *)sadr)->sin_addr = *(int *)h->h_addr_list[0];
	}
	
	return qTrue;
}

/*
=============
NET_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
qBool	NET_StringToAdr (char *s, netAdr_t *a)
{
	struct sockaddr_in sadr;
	
	if (!strcmp (s, "localhost"))
	{
		memset (a, 0, sizeof(*a));
		a->naType = NA_LOOPBACK;
		return qTrue;
	}

	if (!NET_StringToSockaddr (s, (struct sockaddr *)&sadr))
		return qFalse;
	
	NET_SockAdrToNetAdr (&sadr, a);

	return qTrue;
}


qBool	NET_IsLocalAddress (netAdr_t adr)
{
	return NET_CompareAdr (adr, net_local_adr);
}

/*
=============================================================================

LOOPBACK BUFFERS FOR LOCAL PLAYER

=============================================================================
*/

qBool	NET_GetLoopPacket (int sock, netAdr_t *net_from, netMsg_t *net_message)
{
	int		i;
	loopback_t	*loop;

	loop = &loopbacks[sock];

	if (loop->send - loop->get > MAX_LOOPBACK)
		loop->get = loop->send - MAX_LOOPBACK;

	if (loop->get >= loop->send)
		return qFalse;

	i = loop->get & (MAX_LOOPBACK-1);
	loop->get++;

	memcpy (net_message->data, loop->msgs[i].data, loop->msgs[i].datalen);
	net_message->curSize = loop->msgs[i].datalen;
	*net_from = net_local_adr;
	return qTrue;

}


void NET_SendLoopPacket (int sock, int length, void *data, netAdr_t to)
{
	int		i;
	loopback_t	*loop;

	loop = &loopbacks[sock^1];

	i = loop->send & (MAX_LOOPBACK-1);
	loop->send++;

	memcpy (loop->msgs[i].data, data, length);
	loop->msgs[i].datalen = length;
}

//=============================================================================

qBool NET_GetPacket (int sock, netAdr_t *net_from, netMsg_t *net_message)
{
	int 	ret;
	struct sockaddr_in	from;
	int		fromlen;
	int		net_socket;
	int		err;

	if (NET_GetLoopPacket (sock, net_from, net_message))
		return qTrue;

	net_socket = ipSockets[sock];

	if (!net_socket)
		return qFalse;

	fromlen = sizeof(from);
	ret = recvfrom (net_socket, net_message->data, net_message->maxSize
		, 0, (struct sockaddr *)&from, &fromlen);

	NET_SockAdrToNetAdr (&from, net_from);

	if (ret == -1)
	{
		err = errno;

		if (err == EWOULDBLOCK || err == ECONNREFUSED)
			return qFalse;
		Com_Printf (0, "NET_GetPacket: %s from %s\n", NET_ErrorString(),
					NET_AdrToString(*net_from));
		return 0;
	}

	if (ret == net_message->maxSize)
	{
		Com_Printf (0, "Oversize packet from %s\n", NET_AdrToString (*net_from));
		return qFalse;
	}

	net_message->curSize = ret;
	return qTrue;
}

//=============================================================================

int NET_SendPacket (int sock, int length, void *data, netAdr_t to)
{
	int		ret;
	struct sockaddr_in	addr;
	int		net_socket;

	switch (to.naType) {
	case NA_LOOPBACK:
		NET_SendLoopPacket (sock, length, data, to);
		return 0;

	case NA_BROADCAST:
		net_socket = ipSockets[sock];
		if (!net_socket)
			return 0;

	case NA_IP:
		net_socket = ipSockets[sock];
		if (!net_socket)
			return 0;

	default:
		Com_Error (ERR_FATAL, "NET_SendPacket: bad address type");
		break;
	}

	NetadrToSockadr (&to, &addr);

	ret = sendto (net_socket, data, length, 0, (struct sockaddr *)&addr, sizeof(addr));
	if (ret == -1) {
		Com_Printf (0, "NET_SendPacket ERROR: %s to %s\n", NET_ErrorString(),
				NET_AdrToString (to));
		return 0;
		// FIXME: return -1 for certain errors like in net_wins.c
	}

	return 1;
}


//=============================================================================

/*
====================
NET_Config

A single player game will only use the loopback code
====================
*/
int NET_Config (int openFlags)
{
	int		i;

	static int	oldFlags;
	int			oldest;
	cVar_t		*ip;
	int			port;

	if (oldFlags == openFlags)
		return oldFlags;

	memset (&netStats, 0, sizeof (netStats));

	if (openFlags == NET_NONE) {
		oldest = oldFlags;
		oldFlags = NET_NONE;

		// shut down any existing sockets
		if (ipSockets[NS_CLIENT]) {
			close (ipSockets[NS_CLIENT]);
			ipSockets[NS_CLIENT] = 0;
		}

		if (ipSockets[NS_SERVER]) {
			close (ipSockets[NS_SERVER]);
			ipSockets[NS_SERVER] = 0;
		}
	}
	else {
		oldest = oldFlags;
		oldFlags |= openFlags;

		ip = Cvar_Get ("ip", "localhost", CVAR_NOSET);

		// open sockets
		netStats.initTime = time (0);
		netStats.initialized = qTrue;

		if (openFlags & NET_SERVER) {
			if (!ipSockets[NS_SERVER]) {
				port = Cvar_Get ("ip_hostport", "0", CVAR_NOSET)->integer;
				if (!port) {
					port = Cvar_Get ("hostport", "0", CVAR_NOSET)->integer;
					if (!port)
						port = Cvar_Get ("port", Q_VarArgs ("%i", PORT_SERVER), CVAR_NOSET)->integer;
				}

				ipSockets[NS_SERVER] = NET_Socket (ip->string, port);
				if (!ipSockets[NS_SERVER] && dedicated->integer)
					Com_Error (ERR_FATAL, "Couldn't allocate dedicated server IP port");
			}
		}

		// dedicated servers don't need client ports
		if (!dedicated->integer && openFlags & NET_CLIENT) {
			if (!ipSockets[NS_CLIENT]) {
 				int		newport = frand() * 64000 + 1024;

				port = Cvar_Get ("ip_clientport", Q_VarArgs ("%i", newport), CVAR_NOSET)->value;
				if (!port) {
					port = Cvar_Get ("clientport", Q_VarArgs ("%i", newport), CVAR_NOSET)->value;
					if (!port) {
						port = PORT_ANY;
 						Cvar_Set ("clientport", Q_VarArgs ("%d", newport));
					}
				}

				ipSockets[NS_CLIENT] = NET_Socket (ip->string, port);
				if (!ipSockets[NS_CLIENT])
					ipSockets[NS_CLIENT] = NET_Socket (ip->string, PORT_ANY);
			}
		}
	}

	return oldest;	
}


//===================================================================


/*
====================
NET_Init
====================
*/
void NET_Init (void)
{
}


/*
====================
NET_Socket
====================
*/
int NET_Socket (char *net_interface, int port)
{
	int newsocket;
	struct sockaddr_in address;
	qBool _qTrue = qTrue;
	int	i = 1;

	if ((newsocket = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		Com_Printf (0, "ERROR: UDP_OpenSocket: socket: %s", NET_ErrorString());
		return 0;
	}

	// make it non-blocking
	if (ioctl (newsocket, FIONBIO, &_qTrue) == -1)
	{
		Com_Printf (0, "ERROR: UDP_OpenSocket: ioctl FIONBIO:%s\n", NET_ErrorString());
		return 0;
	}

	// make it broadcast capable
	if (setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) == -1)
	{
		Com_Printf (0, "ERROR: UDP_OpenSocket: setsockopt SO_BROADCAST:%s\n", NET_ErrorString());
		return 0;
	}

	if (!net_interface || !net_interface[0] || !Q_stricmp(net_interface, "localhost"))
		address.sin_addr.s_addr = INADDR_ANY;
	else
		NET_StringToSockaddr (net_interface, (struct sockaddr *)&address);

	if (port == PORT_ANY)
		address.sin_port = 0;
	else
		address.sin_port = htons((short)port);

	address.sin_family = AF_INET;

	if( bind (newsocket, (void *)&address, sizeof(address)) == -1)
	{
		Com_Printf (0, "ERROR: UDP_OpenSocket: bind: %s\n", NET_ErrorString());
		close (newsocket);
		return 0;
	}

	return newsocket;
}


/*
====================
NET_Shutdown
====================
*/
void	NET_Shutdown (void)
{
	NET_Config (qFalse);	// close sockets
}


/*
====================
NET_ErrorString
====================
*/
char *NET_ErrorString (void)
{
	int		code;

	code = errno;
	return strerror (code);
}

// sleeps msec or until net socket is ready
void NET_Sleep (int msec)
{
	struct timeval timeout;
	fd_set	fdset;
	extern cVar_t *dedicated;
	extern qBool stdin_active;

	if (!ipSockets[NS_SERVER] || (dedicated && !dedicated->value))
		return; // we're not a server, just run full speed

	FD_ZERO(&fdset);
	if (stdin_active)
		FD_SET(0, &fdset); // stdin is processed too
	FD_SET(ipSockets[NS_SERVER], &fdset); // network socket
	timeout.tv_sec = msec/1000;
	timeout.tv_usec = (msec%1000)*1000;
	select(ipSockets[NS_SERVER]+1, &fdset, NULL, NULL, &timeout);
}

