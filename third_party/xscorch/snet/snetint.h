/* $Header: /fridge/cvs/xscorch/snet/snetint.h,v 1.14 2011-08-01 00:01:43 jacob Exp $ */
/*
   
   xscorch - snetint.h        Copyright(c) 2000-2003 Justin David Smith
                              Copyright(c) 2001      Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/
    
   Internal network header files
    

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
#ifndef __snetint_h_included
#define __snetint_h_included


/* Get USE_NETWORK and svr/cli structures */
#include <snet.h>
#if USE_NETWORK      /* Allow network support? */


/* Default buffer size */
#define  SC_NET_BUFFER_SIZE   0x400 /* Default size for internal buffers */


/* Define the default protocol for socket() call */
#ifndef DEFAULT_PROTOCOL         /* Check if it's already defined */
#define DEFAULT_PROTOCOL   0     /* socket() says this is zero */
#endif /* DEFAULT_PROTOCOL definition */



/* Connection initiation, close packets */
#define  SC_NET_CLI_CONNECT      0x4E4E4F43  /* "CONN" - connect request; flag is version, data is version string */
#define  SC_NET_SVR_ACCEPTED     0x54504341  /* "ACPT" - connect response; flag is version, data is version string */
#define  SC_NET_SVR_REJECTED     0x204A4552  /* "REJ " - connect response; data is text error message */
#define  SC_NET_CLI_DISCONNECT   0x43534944  /* "DISC" - client disconnecting; data is text reason */
#define  SC_NET_SVR_QUIT         0x54495551  /* "QUIT" - server shutting down; data is text reason */

/* Retransmission control packets. */
#define  SC_NET_RETRANSMIT       0x72746572  /* "retr" - packet retransmission request */
#define  SC_NET_RETR_FAIL        0x40746572  /* "ret@" - packet retransmission failure */

/* Game configuration data */
#define  SC_NET_SVR_CONFIG_DATA  0x666E6F63  /* "conf" - server sending config; data is arbitrary */
#define  SC_NET_SVR_PLAYER_DATA  0x72796C70  /* "plyr" - server sending config; data is arbitrary */
#define  SC_NET_CLI_PLAYER_NAME  0x656D616E  /* "name" - client sending name; data is text name */

/* Game event synchronization */
#define  SC_NET_CLI_SYNC_RQST    0x434E5953  /* "SYNC" - client asking to sync; flag is event ID */
#define  SC_NET_SVR_SYNC_RESP    0x404E5953  /* "SYN@" - server permitting sync */

/* Player orders */
#define  SC_NET_CLI_ORDERS       0x7264726F  /* "ordr" - client sending player orders; data */
#define  SC_NET_SVR_ORDERS       0x4064726F  /* "ord@" - server sending player orders; data */
#define  SC_NET_INVENTORY        0x20766E69  /* "inv " - client sending inventory; data */
#define  SC_NET_SHIELDS          0x646C6873  /* "shld" - client sending shield status; data */
#define  SC_NET_BATTERY          0x74746162  /* "batt" - client sending shield status; data */
#define  SC_NET_PLFLAGS          0x676C6670  /* "pflg" - client sending command flags; data */
#define  SC_NET_PLAYER_STATE     0x74736C70  /* "plst" - client sending player state; data */

/* Status packets */
#define  SC_NET_CLIENT_STATUS    0x74617473  /* "stat" - client sending connection status */

/* Miscellaneous broadcast data */
#define  SC_NET_UNKNOWN          0x6E6B6E75  /* "unkn" - unknown packet; don't send this */
#define  SC_NET_CHAT             0x74616863  /* "chat" - player chat; data is text message */



/* Network protocol versions */
#define  SC_NET_MIN_VERSION      0xD6000210  /* minimum version the server can accept */
#define  SC_NET_VERSION          0xD6000210  /* protocol version supported by client */
#define  SC_NET_MAJOR_VERSION    ((SC_NET_VERSION >> 16) & 0xFF)
#define  SC_NET_MINOR_VERSION    ((SC_NET_VERSION >> 8) & 0xFF)
#define  SC_NET_PATCH_VERSION    ((SC_NET_VERSION) & 0xFF)



/* Connection control macros */
#define  SC_CONN_SET_FLAGS(conn, flag)   ((conn).flags = ((conn).flags | (flag)))
#define  SC_CONN_CLEAR_FLAGS(conn, flag) ((conn).flags = ((conn).flags & ~(flag)))
#define  SC_CONN_SET_ARG(conn, arg)      ((conn).syncarg = (arg))
#define  SC_CONN_GET_ARG(conn)           ((conn).syncarg)

/* Connection control definitions */
#define  SC_CONN_NEED_ALL        (SC_CONN_NEED_ACCEPT | SC_CONN_NEED_CONFIG | SC_CONN_NEED_PLAYERS)
#define  SC_CONN_CLI_IFLAGS      SC_CONN_NEED_ALL
#define  SC_CONN_SVR_IFLAGS      SC_CONN_NEED_ACCEPT



/* Data Packet Structure */
#define  SC_PACKET_HEADER_SIZE   (3 * sizeof(dword))  /* Size of the header */
typedef struct _sc_packet {
   dword next_rnd;      /* next expected random value */
   dword msg_type;      /* 32bit message type */
   dword data_size;     /* size of data block */
   byte *data;          /* data block */
} sc_packet;



/* Packet handler for received packets */
typedef bool (*packet_handler)(sc_config *, void *, sc_packet *);



/* Packet management */
bool sc_net_packet_init(sc_packet *packet, dword type, dword data_size);
bool sc_net_packet_release(sc_packet *packet);



/* General functionals */
bool sc_net_check_param(dword actual, dword expected, const char *description, const char *param);
bool sc_net_check_size(const sc_packet *packet, sizea expectedsize, const char *description);
void sc_net_set_error(const char *function, const char *errormsg);
void sc_net_set_info(const char *function, const char *errormsg);
void sc_net_version_info(char *buf, int size);



/* Basic communications */
bool sc_net_shutdown(int *socket);
bool sc_net_set_nonblocking(int socket);
bool sc_net_flush_packets(sc_connection *conn);
bool sc_net_send_packet(sc_connection *conn, sc_packet *packet);
bool sc_net_send_packet_now(sc_connection *conn, sc_packet *packet);
bool sc_net_send_message(sc_connection *conn, dword msg_type, const char *msg);
bool sc_net_recv_packet(sc_connection *conn, sc_config *c, void *parm, packet_handler handler);



/* Configuration */
bool sc_net_svr_send_config(sc_config *c, sc_server *svr);
bool sc_net_cli_recv_config(sc_config *c, sc_client *cli, sc_packet *packet);
bool sc_net_cli_recv_players(sc_config *c, sc_client *cli, sc_packet *packet);



/* Informational/status messages */
bool sc_net_client_recv_status(const sc_config *c, sc_client *cli, sc_packet *packet);
bool sc_net_server_relay_status(sc_server *srv, sc_packet *packet, int connid);
void sc_net_status_init(sc_client *cli);



#endif /* Use network? */


#endif /* __snetint_h_included */
