/* $Header: /fridge/cvs/xscorch/snet/snetstatus.c,v 1.8 2011-08-01 00:01:43 jacob Exp $ */
/*
   
   xscorch - snetstatus.c     Copyright(c) 2001-2003 Justin David Smith
                              Copyright(c) 2001-2003 Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/
    
   Informational and status network messages.
    

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
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
#include <unistd.h>

#include <snetint.h>

#include <sgame/saccessory.h>
#include <sgame/sgame.h>
#include <sgame/sinventory.h>
#include <sgame/splayer.h>
#include <sgame/sstate.h>
#include <sgame/sweapon.h>
#include <sgame/swindow.h>
#include <sutil/srand.h>

#include <libj/jstr/libjstr.h>



bool sc_net_client_update_status(sc_client *cli) {
/* sc_net_client_update_status
   Send current information about our connection to the server. */

   sc_packet packet;
   dword *p;
   int size;
   
   /* Sanity checks */
   if(cli == NULL) return(false);
   
   /* Initialise packet data */
   size = 5 * sizeof(dword) + sizeof(addr);
   if(!sc_net_packet_init(&packet, SC_NET_CLIENT_STATUS, size)) return(false);
   p = (dword *)packet.data;   
   
   /* Write the player ID and client flags */
   *p++ = htonl(0);
   *p++ = htonl(cli->server.flags);
   *p++ = htonl(cli->server.syncarg);
   
   /* Zero the server data and address field */
   memset(p, '\0', 2 * sizeof(dword) + sizeof(addr));
   
   /* Attempt to send the packet */
   sc_net_set_info("client_update_status", "updating the client status");
   if(!sc_net_send_packet_now(&cli->server, &packet)) return(false);
   sc_net_packet_release(&packet);
   return(true);
   
}



bool sc_net_client_recv_status(const sc_config *c, sc_client *cli, sc_packet *packet) {
/* sc_net_client_recv_status
   Update the client status data. */

   sc_net_status *status;
   dword *p;
   int playerid;
   
   /* Check the packet size */
   if(!sc_net_check_size( packet,
                          6 * sizeof(dword),
                          "client_recv_status" )) return(false);

   /* Get the player ID */
   p = (dword *)packet->data;
   playerid = ntohl(*p++);
   if(playerid < 0 || playerid >= c->numplayers) {
      sc_net_set_error("client_recv_status", "Invalid playerid received");
      return(false);
   }
   status = &cli->status[playerid];
   
   /* Load in the new data */
   status->cli_flags   = ntohl(*p++);
   status->cli_syncarg = ntohl(*p++);
   status->srv_flags   = ntohl(*p++);
   status->srv_syncarg = ntohl(*p++);
   
   /* load in the player's address */
   memcpy(&status->address, p, sizeof(addr));
   
   /* Return with success */
   sc_net_set_info("client_recv_status", "Received player status from server");
   return(true);
   
}



bool sc_net_server_relay_status(sc_server *srv, sc_packet *incoming, int connid) {
/* sc_net_server_relay_status
   Relay a status packet from a client to all players currently connected. */

   sc_connection *conn;
   sc_packet packet;
   dword *p;
   int i;

   /* Fill in the blanks in this data structure */
   p = (dword *)incoming->data;
   conn = &srv->clients[connid];
   *p++ = htonl(connid);
    p  += 2;
   *p++ = htonl(conn->flags);
   *p++ = htonl(conn->syncarg);
   
   /* Fill in the client address */
   memcpy(p, &conn->address, sizeof(addr));
   
   /* Relay the modified packet to all connected clients */
   for(i = 0; i < srv->connections; ++i) {
      if(!SC_CONN_IS_DEAD(srv->clients[i])) {
         memcpy(&packet, incoming, sizeof(sc_packet));
         sc_net_send_packet_now(&srv->clients[i], &packet);
      }
   }
   
   /* Return success */
   return(true);

}



void sc_net_status_init(sc_client *cli) {
/* sc_net_status_init
   Initialise the connection status structures. */
   
   memset(cli->status, '\0', sizeof(cli->status));
   
}
