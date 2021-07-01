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
// net_wins.c
//

#include "winsock.h"
#include "wsipx.h"
#include "../common/common.h"

#define	MAX_LOOPBACK	4

typedef struct loopMsg_s {
	byte		data[MAX_MSGLEN];
	int			dataLen;
} loopMsg_t;

typedef struct loopBack_s {
	loopMsg_t	msgs[MAX_LOOPBACK];
	int			get;
	int			send;
} loopBack_t;

static cVar_t	*noudp;
static cVar_t	*noipx;

loopBack_t		loopBacks[2];
int				ipSockets[2];
int				ipxSockets[2];

static WSADATA	wsData;

/*
====================
NET_ErrorString
====================
*/
char *NET_ErrorString (void) {
	int		code;

	code = WSAGetLastError ();
	switch (code) {
	case WSAEINTR: return "WSAEINTR";
	case WSAEBADF: return "WSAEBADF";
	case WSAEACCES: return "WSAEACCES";
	case WSAEDISCON: return "WSAEDISCON";
	case WSAEFAULT: return "WSAEFAULT";
	case WSAEINVAL: return "WSAEINVAL";
	case WSAEMFILE: return "WSAEMFILE";
	case WSAEWOULDBLOCK: return "WSAEWOULDBLOCK";
	case WSAEINPROGRESS: return "WSAEINPROGRESS";
	case WSAEALREADY: return "WSAEALREADY";
	case WSAENOTSOCK: return "WSAENOTSOCK";
	case WSAEDESTADDRREQ: return "WSAEDESTADDRREQ";
	case WSAEMSGSIZE: return "WSAEMSGSIZE";
	case WSAEPROTOTYPE: return "WSAEPROTOTYPE";
	case WSAENOPROTOOPT: return "WSAENOPROTOOPT";
	case WSAEPROTONOSUPPORT: return "WSAEPROTONOSUPPORT";
	case WSAESOCKTNOSUPPORT: return "WSAESOCKTNOSUPPORT";
	case WSAEOPNOTSUPP: return "WSAEOPNOTSUPP";
	case WSAEPFNOSUPPORT: return "WSAEPFNOSUPPORT";
	case WSAEAFNOSUPPORT: return "WSAEAFNOSUPPORT";
	case WSAEADDRINUSE: return "WSAEADDRINUSE";
	case WSAEADDRNOTAVAIL: return "WSAEADDRNOTAVAIL";
	case WSAENETDOWN: return "WSAENETDOWN";
	case WSAENETUNREACH: return "WSAENETUNREACH";
	case WSAENETRESET: return "WSAENETRESET";
	case WSAECONNABORTED: return "WSWSAECONNABORTEDAEINTR";
	case WSAECONNRESET: return "WSAECONNRESET: Connection reset by peer";
	case WSAENOBUFS: return "WSAENOBUFS";
	case WSAEISCONN: return "WSAEISCONN";
	case WSAENOTCONN: return "WSAENOTCONN";
	case WSAESHUTDOWN: return "WSAESHUTDOWN";
	case WSAETOOMANYREFS: return "WSAETOOMANYREFS";
	case WSAETIMEDOUT: return "WSAETIMEDOUT";
	case WSAECONNREFUSED: return "WSAECONNREFUSED";
	case WSAELOOP: return "WSAELOOP";
	case WSAENAMETOOLONG: return "WSAENAMETOOLONG";
	case WSAEHOSTDOWN: return "WSAEHOSTDOWN";
	case WSASYSNOTREADY: return "WSASYSNOTREADY";
	case WSAVERNOTSUPPORTED: return "WSAVERNOTSUPPORTED";
	case WSANOTINITIALISED: return "WSANOTINITIALISED";
	case WSAHOST_NOT_FOUND: return "WSAHOST_NOT_FOUND";
	case WSATRY_AGAIN: return "WSATRY_AGAIN";
	case WSANO_RECOVERY: return "WSANO_RECOVERY";
	case WSANO_DATA: return "WSANO_DATA";
	default: return "NO ERROR";
	}
}


/*
===================
NET_NetAdrToSockAdr
===================
*/
void NET_NetAdrToSockAdr (netAdr_t *a, struct sockaddr *s) {
	memset (s, 0, sizeof (*s));

	if (a->naType == NA_BROADCAST) {
		((struct sockaddr_in *)s)->sin_family = AF_INET;
		((struct sockaddr_in *)s)->sin_port = a->port;
		((struct sockaddr_in *)s)->sin_addr.s_addr = INADDR_BROADCAST;
	} else if (a->naType == NA_IP) {
		((struct sockaddr_in *)s)->sin_family = AF_INET;
		((struct sockaddr_in *)s)->sin_addr.s_addr = *(int *)&a->ip;
		((struct sockaddr_in *)s)->sin_port = a->port;
	} else if (a->naType == NA_IPX) {
		((struct sockaddr_ipx *)s)->sa_family = AF_IPX;
		memcpy(((struct sockaddr_ipx *)s)->sa_netnum, &a->ipx[0], 4);
		memcpy(((struct sockaddr_ipx *)s)->sa_nodenum, &a->ipx[4], 6);
		((struct sockaddr_ipx *)s)->sa_socket = a->port;
	} else if (a->naType == NA_BROADCAST_IPX) {
		((struct sockaddr_ipx *)s)->sa_family = AF_IPX;
		memset(((struct sockaddr_ipx *)s)->sa_netnum, 0, 4);
		memset(((struct sockaddr_ipx *)s)->sa_nodenum, 0xff, 6);
		((struct sockaddr_ipx *)s)->sa_socket = a->port;
	}
}


/*
===================
NET_SockAdrToNetAdr
===================
*/
void NET_SockAdrToNetAdr (struct sockaddr *s, netAdr_t *a) {
	if (s->sa_family == AF_INET) {
		a->naType = NA_IP;
		*(int *)&a->ip = ((struct sockaddr_in *)s)->sin_addr.s_addr;
		a->port = ((struct sockaddr_in *)s)->sin_port;
	} else if (s->sa_family == AF_IPX) {
		a->naType = NA_IPX;
		memcpy(&a->ipx[0], ((struct sockaddr_ipx *)s)->sa_netnum, 4);
		memcpy(&a->ipx[4], ((struct sockaddr_ipx *)s)->sa_nodenum, 6);
		a->port = ((struct sockaddr_ipx *)s)->sa_socket;
	}
}


/*
===================
NET_CompareAdr
===================
*/
qBool NET_CompareAdr (netAdr_t a, netAdr_t b) {
	if (a.naType != b.naType)
		return qFalse;

	if (a.naType == NA_LOOPBACK)
		return qTrue;

	if (a.naType == NA_IP) {
		if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] && a.port == b.port)
			return qTrue;
		return qFalse;
	}

	if (a.naType == NA_IPX) {
		if ((memcmp(a.ipx, b.ipx, 10) == 0) && a.port == b.port)
			return qTrue;
		return qFalse;
	}

	return qFalse;
}


/*
===================
NET_CompareBaseAdr

Compares without the port
===================
*/
qBool NET_CompareBaseAdr (netAdr_t a, netAdr_t b) {
	if (a.naType != b.naType)
		return qFalse;

	if (a.naType == NA_LOOPBACK)
		return qTrue;

	if (a.naType == NA_IP) {
		if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3])
			return qTrue;
		return qFalse;
	}

	if (a.naType == NA_IPX) {
		if ((memcmp(a.ipx, b.ipx, 10) == 0))
			return qTrue;
		return qFalse;
	}

	return qFalse;
}


/*
===================
NET_AdrToString
===================
*/
char *NET_AdrToString (netAdr_t a) {
	static	char	s[64];

	if (a.naType == NA_LOOPBACK)
		Q_snprintfz (s, sizeof (s), "loopback");
	else if (a.naType == NA_IP)
		Q_snprintfz (s, sizeof (s), "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], ntohs(a.port));
	else
		Q_snprintfz (s, sizeof (s), "%02x%02x%02x%02x:%02x%02x%02x%02x%02x%02x:%i", a.ipx[0], a.ipx[1], a.ipx[2], a.ipx[3], a.ipx[4], a.ipx[5], a.ipx[6], a.ipx[7], a.ipx[8], a.ipx[9], ntohs(a.port));

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
#define DO(src,dest)	\
	copy[0] = s[src];	\
	copy[1] = s[src + 1];	\
	sscanf (copy, "%x", &val);	\
	((struct sockaddr_ipx *)sadr)->dest = val

qBool NET_StringToSockaddr (char *s, struct sockaddr *sadr) {
	struct hostent	*h;
	char	*colon;
	int		val;
	char	copy[128];
	
	memset (sadr, 0, sizeof (*sadr));

	if ((strlen(s) >= 23) && (s[8] == ':') && (s[21] == ':')) {	// check for an IPX address
		((struct sockaddr_ipx *)sadr)->sa_family = AF_IPX;
		copy[2] = 0;
		DO(0, sa_netnum[0]);
		DO(2, sa_netnum[1]);
		DO(4, sa_netnum[2]);
		DO(6, sa_netnum[3]);
		DO(9, sa_nodenum[0]);
		DO(11, sa_nodenum[1]);
		DO(13, sa_nodenum[2]);
		DO(15, sa_nodenum[3]);
		DO(17, sa_nodenum[4]);
		DO(19, sa_nodenum[5]);
		sscanf (&s[22], "%u", &val);
		((struct sockaddr_ipx *)sadr)->sa_socket = htons((uShort)val);
	} else {
		((struct sockaddr_in *)sadr)->sin_family = AF_INET;
		
		((struct sockaddr_in *)sadr)->sin_port = 0;

		strcpy (copy, s);
		// strip off a trailing :port if present
		for (colon = copy ; *colon ; colon++) {
			if (*colon == ':') {
				*colon = 0;
				((struct sockaddr_in *)sadr)->sin_port = htons ((short)atoi (colon+1));	
			}
		}
		
		if ((copy[0] >= '0') && (copy[0] <= '9')) {
			*(int *)&((struct sockaddr_in *)sadr)->sin_addr = inet_addr (copy);
		} else {
			if (!(h = gethostbyname(copy)))
				return 0;
			*(int *)&((struct sockaddr_in *)sadr)->sin_addr = *(int *)h->h_addr_list[0];
		}
	}
	
	return qTrue;
}
#undef DO


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
qBool NET_StringToAdr (char *s, netAdr_t *a) {
	struct sockaddr sadr;
	
	if (!strcmp (s, "localhost")) {
		memset (a, 0, sizeof (*a));
		a->naType = NA_LOOPBACK;
		return qTrue;
	}

	if (!NET_StringToSockaddr (s, &sadr))
		return qFalse;
	
	NET_SockAdrToNetAdr (&sadr, a);

	return qTrue;
}


/*
===================
NET_IsLocalAddress
===================
*/
qBool NET_IsLocalAddress (netAdr_t adr) {
	return (adr.naType == NA_LOOPBACK) ? qTrue : qFalse;
}

/*
=============================================================================

	LOOPBACK BUFFERS FOR LOCAL PLAYER

=============================================================================
*/

/*
===================
NET_GetLoopPacket
===================
*/
qBool NET_GetLoopPacket (int sock, netAdr_t *fromAddr, netMsg_t *message) {
	int			i;
	loopBack_t	*loop;

	loop = &loopBacks[sock];

	if (loop->send - loop->get > MAX_LOOPBACK)
		loop->get = loop->send - MAX_LOOPBACK;

	if (loop->get >= loop->send)
		return qFalse;

	i = loop->get & (MAX_LOOPBACK-1);
	loop->get++;

	memcpy (message->data, loop->msgs[i].data, loop->msgs[i].dataLen);
	message->curSize = loop->msgs[i].dataLen;
	memset (fromAddr, 0, sizeof (*fromAddr));
	fromAddr->naType = NA_LOOPBACK;
	return qTrue;
}


/*
===================
NET_SendLoopPacket
===================
*/
void NET_SendLoopPacket (int sock, int length, void *data) {
	int			i;
	loopBack_t	*loop;

	loop = &loopBacks[sock^1];

	i = loop->send & (MAX_LOOPBACK-1);
	loop->send++;

	memcpy (loop->msgs[i].data, data, length);
	loop->msgs[i].dataLen = length;
}


/*
===================
NET_GetPacket
===================
*/
qBool NET_GetPacket (int sock, netAdr_t *fromAddr, netMsg_t *message) {
	struct	sockaddr fromSockAddr;
	int		ret, fromLen, net_socket, protocol, err;

	if (NET_GetLoopPacket (sock, fromAddr, message))
		return qTrue;

	for (protocol=0 ; protocol<2 ; protocol++) {
		if (protocol == 0)
			net_socket = ipSockets[sock];
		else
			net_socket = ipxSockets[sock];

		if (!net_socket)
			continue;

		fromLen = sizeof (fromSockAddr);
		ret = recvfrom (net_socket, message->data, message->maxSize, 0, (struct sockaddr *)&fromSockAddr, &fromLen);

		NET_SockAdrToNetAdr (&fromSockAddr, fromAddr);

		if (ret == -1) {
			err = WSAGetLastError();

			if (err == WSAEWOULDBLOCK)
				continue;

			if (err == WSAEMSGSIZE) {
				Com_Printf (0, S_COLOR_YELLOW "WARNING: NET_GetPacket: Oversize packet from %s\n", NET_AdrToString (*fromAddr));
				continue;
			}

			if (dedicated->integer || (err == WSAECONNRESET))
				Com_Printf (0, S_COLOR_YELLOW "NET_GetPacket: %s from %s\n", NET_ErrorString (), NET_AdrToString (*fromAddr));
			else
				Com_Printf (0, S_COLOR_RED "NET_GetPacket: %s from %s\n", NET_ErrorString (), NET_AdrToString (*fromAddr));

			continue;
		}

		if (ret == message->maxSize) {
			Com_Printf (0, S_COLOR_YELLOW "NET_GetPacket: Oversize packet from %s\n", NET_AdrToString (*fromAddr));
			continue;
		}

		message->curSize = ret;
		return qTrue;
	}

	return qFalse;
}


/*
===================
NET_SendPacket
===================
*/
void NET_SendPacket (int sock, int length, void *data, netAdr_t to) {
	int				ret;
	struct sockaddr	addr;
	int				net_socket = 0;

	if (to.naType == NA_LOOPBACK) {
		NET_SendLoopPacket (sock, length, data);
		return;
	}

	if (to.naType == NA_BROADCAST) {
		net_socket = ipSockets[sock];
		if (!net_socket)
			return;
	} else if (to.naType == NA_IP) {
		net_socket = ipSockets[sock];
		if (!net_socket)
			return;
	} else if (to.naType == NA_IPX) {
		net_socket = ipxSockets[sock];
		if (!net_socket)
			return;
	} else if (to.naType == NA_BROADCAST_IPX) {
		net_socket = ipxSockets[sock];
		if (!net_socket)
			return;
	} else
		Com_Error (ERR_FATAL, "NET_SendPacket: bad address naType");

	NET_NetAdrToSockAdr (&to, &addr);

	ret = sendto (net_socket, data, length, 0, &addr, sizeof (addr));
	if (ret == -1) {
		int err = WSAGetLastError ();

		// wouldblock is silent
		if (err == WSAEWOULDBLOCK)
			return;

		// Knightmare - NO ERROR fix for pinging servers w/ unplugged LAN cable
		if (((err == WSAEADDRNOTAVAIL) || !strcmp (NET_ErrorString(), "NO ERROR"))
		   && ((to.naType == NA_BROADCAST) || (to.naType == NA_BROADCAST_IPX)))
		   return;

		// some PPP links dont allow broadcasts
		if ((err == WSAEADDRNOTAVAIL) && (to.naType == NA_BROADCAST))
			return;

		Com_Printf (0, S_COLOR_RED "NET_SendPacket: %s to %s\n", NET_ErrorString (), NET_AdrToString (to));
	}
}


/*
====================
NET_IPSocket
====================
*/
int NET_IPSocket (char *net_interface, int port) {
	int		newsocket;
	struct	sockaddr_in		address;
	int		hasArgs = 1;
	int		i = 1, err;

	if ((newsocket = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		err = WSAGetLastError ();
		if (err != WSAEAFNOSUPPORT)
			Com_Printf (0, S_COLOR_YELLOW "WARNING: NET_IPSocket: socket: %s", NET_ErrorString ());
		return 0;
	}

	// make it non-blocking
	if (ioctlsocket (newsocket, FIONBIO, &hasArgs) == -1) {
		Com_Printf (0, S_COLOR_YELLOW "WARNING: NET_IPSocket: ioctl FIONBIO: %s\n", NET_ErrorString ());
		return 0;
	}

	// make it broadcast capable
	if (setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof (i)) == -1) {
		Com_Printf (0, S_COLOR_YELLOW "WARNING: NET_IPSocket: setsockopt SO_BROADCAST: %s\n", NET_ErrorString ());
		return 0;
	}

	if (!net_interface || !net_interface[0] || !stricmp(net_interface, "localhost"))
		address.sin_addr.s_addr = INADDR_ANY;
	else
		NET_StringToSockaddr (net_interface, (struct sockaddr *)&address);

	if (port == PORT_ANY)
		address.sin_port = 0;
	else
		address.sin_port = htons((short)port);

	address.sin_family = AF_INET;

	if (bind (newsocket, (void *)&address, sizeof (address)) == -1) {
		Com_Printf (0, S_COLOR_YELLOW "WARNING: NET_IPSocket: bind: %s\n", NET_ErrorString ());
		closesocket (newsocket);
		return 0;
	}

	return newsocket;
}


/*
====================
NET_OpenIP
====================
*/
void NET_OpenIP (void) {
	cVar_t	*ip;
	int		port;

	ip = Cvar_Get ("ip", "localhost", CVAR_NOSET);

	if (!ipSockets[NS_SERVER]) {
		port = Cvar_Get("ip_hostport", "0", CVAR_NOSET)->value;
		if (!port) {
			port = Cvar_Get("hostport", "0", CVAR_NOSET)->value;
			if (!port)
				port = Cvar_Get("port", Q_VarArgs ("%i", PORT_SERVER), CVAR_NOSET)->value;
		}

		ipSockets[NS_SERVER] = NET_IPSocket (ip->string, port);
		if (!ipSockets[NS_SERVER] && Cvar_VariableValue ("dedicated"))
			Com_Error (ERR_FATAL, "Couldn't allocate dedicated server IP port");
	}


	// dedicated servers don't need client ports
	if (dedicated->integer)
		return;

	if (!ipSockets[NS_CLIENT]) {
		port = Cvar_Get("ip_clientport", "0", CVAR_NOSET)->value;
		if (!port) {
			srand (Sys_Milliseconds ());
			port = Cvar_Get("clientport", Q_VarArgs ("%i", PORT_CLIENT), CVAR_NOSET)->value;
			if (!port)
				port = PORT_ANY;
		}

		ipSockets[NS_CLIENT] = NET_IPSocket (ip->string, port);
		if (!ipSockets[NS_CLIENT])
			ipSockets[NS_CLIENT] = NET_IPSocket (ip->string, PORT_ANY);
	}
}


/*
====================
NET_IPXSocket
====================
*/
int NET_IPXSocket (int port) {
	int		newsocket;
	struct	sockaddr_ipx	address;
	int		hasArgs = 1;
	int		err;

	if ((newsocket = socket (PF_IPX, SOCK_DGRAM, NSPROTO_IPX)) == -1) {
		err = WSAGetLastError();
		if (err != WSAEAFNOSUPPORT)
			Com_Printf (0, S_COLOR_YELLOW "WARNING: NET_IPXSocket: socket: %s\n", NET_ErrorString ());

		return 0;
	}

	// make it non-blocking
	if (ioctlsocket (newsocket, FIONBIO, &hasArgs) == -1) {
		Com_Printf (0, S_COLOR_YELLOW "WARNING: NET_IPXSocket: ioctl FIONBIO: %s\n", NET_ErrorString ());
		return 0;
	}

	// make it broadcast capable
	if (setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&hasArgs, sizeof (hasArgs)) == -1) {
		Com_Printf (0, S_COLOR_YELLOW "WARNING: NET_IPXSocket: setsockopt SO_BROADCAST: %s\n", NET_ErrorString ());
		return 0;
	}

	address.sa_family = AF_IPX;
	memset (address.sa_netnum, 0, 4);
	memset (address.sa_nodenum, 0, 6);
	if (port == PORT_ANY)
		address.sa_socket = 0;
	else
		address.sa_socket = htons((short)port);

	if (bind (newsocket, (void *)&address, sizeof (address)) == -1) {
		Com_Printf (0, S_COLOR_YELLOW "WARNING: NET_IPXSocket: bind: %s\n", NET_ErrorString ());
		closesocket (newsocket);
		return 0;
	}

	return newsocket;
}


/*
====================
NET_OpenIPX
====================
*/
void NET_OpenIPX (void) {
	int		port;

	if (!ipxSockets[NS_SERVER]) {
		port = Cvar_Get("ipx_hostport", "0", CVAR_NOSET)->value;
		if (!port) {
			port = Cvar_Get("hostport", "0", CVAR_NOSET)->value;
			if (!port)
				port = Cvar_Get("port", Q_VarArgs ("%i", PORT_SERVER), CVAR_NOSET)->value;
		}
		ipxSockets[NS_SERVER] = NET_IPXSocket (port);
	}

	// dedicated servers don't need client ports
	if (dedicated->integer)
		return;

	if (!ipxSockets[NS_CLIENT]) {
		port = Cvar_Get("ipx_clientport", "0", CVAR_NOSET)->value;
		if (!port) {
			srand (Sys_Milliseconds ());
			port = Cvar_Get("clientport", Q_VarArgs ("%i", PORT_CLIENT), CVAR_NOSET)->value;
			if (!port)
				port = PORT_ANY;
		}
		ipxSockets[NS_CLIENT] = NET_IPXSocket (port);
		if (!ipxSockets[NS_CLIENT])
			ipxSockets[NS_CLIENT] = NET_IPXSocket (PORT_ANY);
	}
}


/*
====================
NET_Config

A single player game will only use the loopback code
====================
*/
void NET_Config (qBool multiplayer) {
	int		i;
	static	qBool	old_config;

	if (old_config == multiplayer)
		return;

	old_config = multiplayer;

	if (!multiplayer) {
		// shut down any existing sockets
		for (i=0 ; i<2 ; i++) {
			if (ipSockets[i]) {
				closesocket (ipSockets[i]);
				ipSockets[i] = 0;
			}

			if (ipxSockets[i]) {
				closesocket (ipxSockets[i]);
				ipxSockets[i] = 0;
			}
		}
	} else {
		// open sockets
		if (! noudp->value)
			NET_OpenIP ();
		if (! noipx->value)
			NET_OpenIPX ();
	}
}


/*
====================
NET_Sleep

Sleeps for msec or until net socket is ready
====================
*/
void NET_Sleep (int msec) {
	struct timeval timeout;
	fd_set	fdset;
	int i;

	if (!dedicated || !dedicated->integer)
		return; // we're not a server, just run full speed

	FD_ZERO(&fdset);
	i = 0;
	if (ipSockets[NS_SERVER]) {
		FD_SET(ipSockets[NS_SERVER], &fdset); // network socket
		i = ipSockets[NS_SERVER];
	}

	if (ipxSockets[NS_SERVER]) {
		FD_SET (ipxSockets[NS_SERVER], &fdset); // network socket
		if (ipxSockets[NS_SERVER] > i)
			i = ipxSockets[NS_SERVER];
	}

	timeout.tv_sec = msec / 1000;
	timeout.tv_usec = (msec % 1000) * 1000;
	select (i+1, &fdset, NULL, NULL, &timeout);
}

//===================================================================

/*
====================
NET_ShowIP_f
====================
*/
void NET_ShowIP_f (void) {
	char			s[512];
	int				i;
	struct hostent	*h;
	struct in_addr	in;

	gethostname (s, sizeof (s));
	if (!(h = gethostbyname (s))) {
		Com_Printf (0, S_COLOR_RED "Can't get host\n");
		return;
	}

	Com_Printf (0, "Hostname: %s\n", h->h_name);

	i = 0;
	while (h->h_addr_list[i]) {
		in.s_addr = *(int *)h->h_addr_list[i];
		Com_Printf (0, "IP Address: %s\n", inet_ntoa(in));
		i++;
	}
}


/*
====================
NET_Restart_f
====================
*/
void NET_Restart_f (void) {
	NET_Shutdown ();
	NET_Init ();
}


/*
====================
NET_Init
====================
*/
void NET_Init (void) {
	WORD	wVersionRequested; 
	int		err;

	wVersionRequested = MAKEWORD (1, 1); 

	if ((err = WSAStartup (MAKEWORD(1, 1), &wsData)) != 0)
		Com_Error (ERR_FATAL, "Winsock initialization failed.");

	Com_Printf (0, "\nWinsock initialized\n");

	NET_ShowIP_f ();

	noudp		= Cvar_Get ("noudp", "0", CVAR_NOSET);
	noipx		= Cvar_Get ("noipx", "0", CVAR_NOSET);

	Cmd_AddCommand ("showip",		NET_ShowIP_f,		"Prints your IP to the console");
	Cmd_AddCommand ("net_restart",	NET_Restart_f,		"Restarts net subsystem");
}


/*
====================
NET_Shutdown
====================
*/
void NET_Shutdown (void) {
	Cmd_RemoveCommand ("showip");
	Cmd_RemoveCommand ("net_restart");

	NET_Config (qFalse);	// close sockets

	WSACleanup ();
}
