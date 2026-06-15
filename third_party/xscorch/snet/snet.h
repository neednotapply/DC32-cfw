/* $Header: /fridge/cvs/xscorch/snet/snet.h,v 1.10 2011-08-01 00:01:43 jacob Exp $ */
/*
   
   xscorch - snet.h           Copyright(c) 2001,2000 Justin David Smith
                              Copyright(c) 2001,2000 Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/
    
   Network header files
    

   This program is free software; you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation, version 2 of the License ONLY. 

   This program is distributed in the hope that it will be useful, 
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

*/
#ifndef __snet_h_included
#define __snet_h_included


/* Get system headers and preprocessor definitions */
#include <xscorch.h>
#include <sgame/sconfig.h>

/* Make sure we have the USE_NETWORK definition */
#ifndef USE_NETWORK
   #error USE_NETWORK must be defined
#endif


/* Forward declarations. */
struct _tn_connection;


/* Shorthand macros */
#if USE_NETWORK
   #define  SC_NETWORK_MODE(c)   ((c)->client != NULL)
   #define  SC_NETWORK_AUTH(c)   ((c)->server != NULL || (c)->client == NULL)
   #define  SC_NETWORK_SERVER(c) ((c)->server != NULL)
#else
   #define  SC_NETWORK_MODE(c)   false
   #define  SC_NETWORK_AUTH(c)   true
#endif /* macros */


#if USE_NETWORK /* allow network support */


/* Check for the headers that we need to use */
#if !HAVE_SYS_TYPES_H
   #error "Requires <sys/types.h> for network support"
#endif
#include <sys/types.h>
#if !HAVE_SYS_SOCKET_H
   #error "Requires <sys/socket.h> for network support"
#endif
#include <sys/socket.h>
#if !HAVE_NETDB_H
   #error "Requires <netdb.h> for network support"
#endif
#include <netdb.h>
#if !HAVE_NETINET_IN_H
   #error "Requires <netinet/in.h> for network support"
#endif
#include <netinet/in.h>
#if !HAVE_ARPA_INET_H
   #error "Requires <arpa/inet.h> for network support"
#endif
#include <arpa/inet.h>


/* Define the default port and server address */
#define  SC_NET_LOCALHOST        "localhost" /* Assume localhost works */
#define  SC_NET_DEFAULT_PORT     8367        /* ``SC'', in decimal :) */
#define  SC_NET_DEFAULT_SERVER   SC_NET_LOCALHOST /* deja-vu */


/* Define lengths for various ``globals'', e.g. player name */
#define  SC_NET_NAME_SIZE        32          /* Player name length */


/* Address shorthand - note, the code assumes IPv4 addresses with this
   definition.  But in theory, this could be changed to the IPv6 address
   structure, fix a few compile errors, and everything should work. */
typedef struct sockaddr_in addr; /* Much shorter to type */
typedef struct hostent     host; /* Also shorter and handy */


/* Connection flags */
#define  SC_CONN_OKAY         0           /* Connection is okay/idle */
#define  SC_CONN_NEED_ACCEPT  0x00000001  /* Cli or srv: waiting for accept */
#define  SC_CONN_NEED_CONFIG  0x00000002  /* Cli: waiting for config */
#define  SC_CONN_NEED_PLAYERS 0x00000004  /* Cli: waiting for players */
#define  SC_CONN_WAIT_SYNC    0x00000008  /* Cli/Svr: waiting for sync */

/* TEMP - this will eventually stop being a connection killer - JL */
#define  SC_CONN_TCPNET_ERROR 0x01000000  /* Cli/Svr: TCP NET needs reset */

#define  SC_CONN_DEAD         0xff000000  /* Deadly error states */
#define  SC_CONN_LOCAL_ERROR  0x10000000  /* Cli/Svr: local error occurred */
#define  SC_CONN_REJECTED     0x20000000  /* Cli: was rejected from server */
#define  SC_CONN_QUIT         0x40000000  /* Cli/Svr: remote player quit */
#define  SC_CONN_UNKNOWN      0x80000000  /* Cli/Svr: status is unknown. */

#define  SC_CONN_SYNC_GAME    0x01        /* Sync'd to begin game */
#define  SC_CONN_SYNC_INV     0x02        /* Sync'd to begin inventories */
#define  SC_CONN_SYNC_ROUND   0x03        /* Sync'd to begin round */
#define  SC_CONN_SYNC_TURN    0x04        /* Sync'd to beign turn */
#define  SC_CONN_SYNC_SERV    0x80        /* Bit set if sync is server->server */


/* Connection macros */
#define  SC_CONN_IS_OKAY(conn)      ((conn).flags == SC_CONN_OKAY)
#define  SC_CONN_IS_DEAD(conn)      ((conn).flags & SC_CONN_DEAD)
#define  SC_CONN_IS_SYNC(conn)      ((conn).flags & SC_CONN_WAIT_SYNC)


/* Current player status - this structure is informational
   only and is sent from the server to other clients as it
   updates each player's status - it may not be up to date. */
typedef struct _sc_net_status {
   addr   address;               /* Player's address */
   dword  cli_flags;             /* Connection flags from client */
   dword  cli_syncarg;           /* Connection  arg  from client */
   dword  srv_flags;             /* Connection flags from server */
   dword  srv_syncarg;           /* Connection  arg  from server */
} sc_net_status;


/* Connection state */
typedef struct _sc_connection {
   int    socket;                /* Remote socket to client */
   addr   address;               /* Remote address for connection */
   dword  flags;                 /* Connection flags; 0 == okay. */
   dword  syncarg;               /* Argument associated with flag */
   struct _tn_connection *connection;/* Network internal connection. */
} sc_connection;


/* Server network structure */
typedef struct _sc_server {
   int   linein;                 /* Server socket handle */
   int   connections;            /* Number of connects */
   int   current;                /* Current conn to deal with */
   sc_connection clients[SC_MAX_PLAYERS]; /* Links to clients */
} sc_server;


/* Client network structure */
typedef struct _sc_client {
   sc_connection server;         /* Link to server */
   char  name[SC_NET_NAME_SIZE]; /* Player name */
   sc_net_status status[SC_MAX_PLAYERS];  /* Status structure */
} sc_client;


/* Error messages, general info */
const char *sc_net_get_error(void);
int sc_net_get_hostname(char *buf, int size);


/* Functions to create/release server/client */
sc_client *sc_net_client_new(const char *name, const char *server, int port);
sc_server *sc_net_server_new(sc_config *c, int port);
void sc_net_client_free(sc_client **cli, const char *msg);
void sc_net_server_free(sc_server **srv, const char *msg);


/* Functions to run the main loop of svr/cli recvfrom() */
bool sc_net_client_run(sc_config *c, sc_client *cli);
bool sc_net_server_run(sc_config *c, sc_server *srv);


/* Checks if the client is dead; if so, close and deallocate */
bool sc_net_client_death(sc_client **cli);
bool sc_net_server_prune(sc_server *svr);


/* Send from client */
bool sc_net_client_chat(sc_client *cli, const char *msg);
bool sc_net_client_sync(sc_client *cli, dword flag, bool isserver);
bool sc_net_client_send_orders(const sc_config *c, sc_client *cli, int playerid);
bool sc_net_client_send_shields(const sc_config *c, sc_client *cli, int playerid);
bool sc_net_client_send_battery(const sc_config *c, sc_client *cli, int playerid);
bool sc_net_client_send_flags(const sc_config *c, sc_client *cli, int playerid);
bool sc_net_client_send_inventory(const sc_config *c, sc_client *cli, int playerid);
bool sc_net_client_send_player_state(const sc_config *c, sc_client *cli);


/* Server broadcast */
bool sc_net_server_send_config(sc_config *c, sc_server *svr);


/* Informational/status messages */
bool sc_net_client_update_status(sc_client *cli);


#endif /* Network? */


#endif /* __snet_h_included */
