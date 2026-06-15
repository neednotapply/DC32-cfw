/* $Header: /fridge/cvs/xscorch/snet/snetserver.c,v 1.20 2011-08-01 00:01:43 jacob Exp $ */
/*

   xscorch - snetserver.c     Copyright(c) 2000-2003 Justin David Smith
                              Copyright(c) 2001-2003 Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/

   Server control loop


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
#include <unistd.h>

#include <snetint.h>

#include <sgame/saccessory.h>
#include <sgame/sgame.h>
#include <sgame/splayer.h>
#include <sgame/sstate.h>
#include <sgame/stankpro.h>
#include <sgame/sweapon.h>
#include <sgame/swindow.h>
#include <sutil/srand.h>

#include <libj/jstr/libjstr.h>
#include <snet/tcpnet/tcpnet.h>



static bool _sc_net_sync_all(sc_server *srv) {
/* _sc_net_sync_all

   I think you want to read snetclient.c (search for "_sync") before looking
   through this function, since it's not entirely obvious what all is going
   on here.  This code is a bit dense, but that's because we have to figure
   out _what_ the sync event is, then we have to figure out if any clients
   are not on the same event.

   Note we will send out the expected next random value for the server, so
   the client may check and see if they are actually on the correct value. 
   If the client is off, it is up to them to report the discrepancy back to
   the server and try to negotiate the error with us.  Otherwise, their
   orders will be run with respect to OUR map, and they will probably be
   very unhappy...  

   This returns true if a sync occurred (all clients were ready) and false
   if we are still waiting on a client to sync with us. */

   sc_packet packet;    /* New outgoing packet data */
   dword *p;            /* Random pointer into data */
   dword type = 0;      /* Sync event type - 0 == don't know */
   int rnd  = 0;        /* Next random value or bogus == wrong event */
   int i;               /* Iterate through client connections */

   for(i = 0; i < srv->connections; ++i) {
      /* Make sure every client is ready to sync */
      if(!SC_CONN_IS_SYNC(srv->clients[i])) return(false);

      /* If server, we can find out what the sync type is */
      if((SC_CONN_GET_ARG(srv->clients[i]) & SC_CONN_SYNC_SERV) != 0) {
         /* The server knows what type of event we're syncing */
         type = SC_CONN_GET_ARG(srv->clients[i]) & (~SC_CONN_SYNC_SERV);
      }
   }

   for(i = 0; i < srv->connections; ++i) {
      /* Sanity check to make sure everyone is syncing to the same event */
      rnd = game_rand_peek();
      if((SC_CONN_GET_ARG(srv->clients[i]) & (~SC_CONN_SYNC_SERV)) != type) {
         /* This client is not syncing to the right event.  We send back an
            invalid random value so the client will report ``random values
            out of alignment'' and hopefully call back to us to try to
            straighten things out. This is a hack of sorts, we'll handle
            this more gracefully later. */
         sc_net_set_error("svr_relay_orders", "Player was waiting for invalid sync");
         rnd = -1;
      }

      /* Prepare a packet to send out to the client */      
      if(sc_net_packet_init(&packet, SC_NET_SVR_SYNC_RESP, 2 * sizeof(dword))) {
         p = (dword *)packet.data;
         *p++ = htonl(type);
         *p++ = htonl(rnd);
         sc_net_send_packet_now(&srv->clients[i], &packet);
         SC_CONN_CLEAR_FLAGS(srv->clients[i], SC_CONN_WAIT_SYNC);
         sc_net_packet_release(&packet);
      }
   }

   /* IF we made it here, then something went right... */
   sc_net_set_info("svr_sync_all", "All clients sync; advancing state");
   return(true);

}



static bool _sc_net_relay_orders(const sc_config *c, sc_server *srv, const sc_packet *incoming, int connid) {
/* _sc_net_relay_orders
   Push orders from one player to all the other players. */

   const sc_player *pl;
   sdword playerid;
   dword turret;
   dword power;
   dword weapon;
   dword shield;
   dword x;
   dword y;
   sc_packet packet;
   dword *p;
   int size;
   int i;

   if(!sc_net_check_size(incoming, 7 * sizeof(dword), "relay_orders")) return(false);

   p = (dword *)incoming->data;
   playerid = ntohl(*p++);
   turret   = ntohl(*p++);
   power    = ntohl(*p++);
   weapon   = ntohl(*p++);
   shield   = ntohl(*p++);
   x        = ntohl(*p++);
   y        = ntohl(*p++);
   if(playerid < 0 || playerid >= c->numplayers) {
      sc_net_set_error("svr_relay_orders", "PlayerID received was invalid");
      return(false);
   }

   size = (1 + 7 * c->numplayers) * sizeof(dword);
   if(!sc_net_packet_init(&packet, SC_NET_SVR_ORDERS, size)) return(false);
   p = (dword *)packet.data;
   *p++ = htonl(c->numplayers);
   for(i = 0; i < c->numplayers; ++i) {
      *p++ = htonl(i);
      if(i == playerid) {
         *p++ = htonl(turret);
         *p++ = htonl(power);
         *p++ = htonl(weapon);
         *p++ = htonl(shield);
         *p++ = htonl(x);
         *p++ = htonl(y);
      } else {
         pl = c->players[i];
         *p++ = htonl(pl->turret);
         *p++ = htonl(pl->power);
         *p++ = htonl(pl->selweapon->ident);
         *p++ = htonl(pl->selshield->ident);
         *p++ = htonl(pl->x);
         *p++ = htonl(pl->y);
      }
   }

   for(i = 0; i < srv->connections; ++i) {
      if(i != connid && !SC_CONN_IS_DEAD(srv->clients[i])) {
         sc_net_send_packet_now(&srv->clients[i], &packet);
      }
   }

   sc_net_packet_release(&packet);
   return(true);

}



static void _sc_net_process_message(sc_config *c, sc_server *srv, sc_packet *incoming, int connid) {

   sc_connection *client = &srv->clients[connid];
   int i;

   switch(incoming->msg_type) {
   case SC_NET_CLI_DISCONNECT:
      SC_CONN_SET_FLAGS(*client, SC_CONN_QUIT);
      sc_net_set_info("process_message", "server: client has disconnected");
      break;
   case SC_NET_CLI_PLAYER_NAME:
      memcpy(c->players[connid]->name, incoming->data, SC_PLAYER_NAME_LENGTH);
      sc_net_set_info("process_message", "server: client gave us their name");
      sc_net_svr_send_config(c, srv);
      break;
   case SC_NET_CLI_SYNC_RQST:
      SC_CONN_SET_FLAGS(*client, SC_CONN_WAIT_SYNC);
      SC_CONN_SET_ARG(*client, ntohl(*(dword *)incoming->data));
      _sc_net_sync_all(srv);
      break;
   case SC_NET_CHAT:
   case SC_NET_SHIELDS:
   case SC_NET_BATTERY:
   case SC_NET_PLFLAGS:
   case SC_NET_INVENTORY:
   case SC_NET_PLAYER_STATE:
      /* Items that get relayed to the clients by the server. */
      for(i = 0; i < srv->connections; ++i) {
         if(i != connid && !SC_CONN_IS_DEAD(srv->clients[i])) {
            sc_net_send_packet_now(&srv->clients[i], incoming);
         }
      }
      break;
   case SC_NET_CLIENT_STATUS:
      sc_net_server_relay_status(srv, incoming, connid);
      break;
   case SC_NET_CLI_ORDERS:
      _sc_net_relay_orders(c, srv, incoming, connid);
      break;
   default:
      sc_net_set_error("process_message", "invalid packet type received by server");
      break;
   }

}



static void _sc_net_connect_request(sc_config *c, sc_server *srv, const sc_packet *incoming, int connid) {

   char versionstr[SC_NET_BUFFER_SIZE];
   char infomsg[SC_NET_BUFFER_SIZE];
   sc_connection *client;
   sc_packet reply;
   dword version;

   client = &srv->clients[connid];

   /* Make sure this actually looks like a connect request */
   if(incoming->msg_type != SC_NET_CLI_CONNECT) {
      sc_net_set_error("connect_request", "Incoming packet from unknown source, not a connect request");
      return;
   }

   /* From this point, we assume a connection from an xscorch client.
      Any failures from this point should be sent to them. */

   /* Check the version number data (should be in flags) */
   version = ntohl(*(dword *)incoming->data);
   if(version < SC_NET_MIN_VERSION) {
      sc_net_send_message(client, SC_NET_SVR_REJECTED, "Client version is too old");
      sc_net_set_error("connect_request", "Incoming packet had protocol version that was too ancient");
      return;
   }
   if(version > SC_NET_VERSION) {
      sc_net_send_message(client, SC_NET_SVR_REJECTED, "Client version is too new");
      sc_net_set_error("connect_request", "Incoming packet had protocol version that was too recent");
      return;
   }

   /* notify client they are on */
   sc_net_version_info(versionstr, sizeof(versionstr));
   if(!sc_net_packet_init(&reply, SC_NET_SVR_ACCEPTED, strlenn(versionstr) + sizeof(dword))) return;
   *(dword *)reply.data = htonl(SC_NET_VERSION);
   memcpy(reply.data + sizeof(dword), versionstr, strlenn(versionstr));
   sc_net_send_packet_now(client, &reply);

   /* Informative */
   memcpy(versionstr, incoming->data + sizeof(dword), incoming->data_size - sizeof(dword));
   versionstr[incoming->data_size - sizeof(dword)] = '\0';
   sbprintf(infomsg, sizeof(infomsg), "Client accepted, version \"%s\"", versionstr);
   sc_net_set_info("connect_request", infomsg);

   /* Send the game configuration */
   sc_net_svr_send_config(c, srv);

   /* Update server setup */
   sc_net_packet_release(&reply);
   SC_CONN_CLEAR_FLAGS(*client, SC_CONN_NEED_ACCEPT);

}



bool sc_net_server_handle_packet(sc_config *c, void *parm, sc_packet *p) {

   sc_server *srv = (sc_server *)parm;
   int connid = srv->current;

   /* Check if this player is officially connected? */
   if(srv->clients[connid].flags & SC_CONN_NEED_ACCEPT) {
      /* This is hopefully a connection request/new player */
      _sc_net_connect_request(c, srv, p, connid);
   } else {
      /* This is an existing connection, process as event */
      _sc_net_process_message(c, srv, p, connid);
   } /* New player? */
   return(true);

}



static void _sc_net_incoming_connect(sc_config *c, sc_server *srv, int socket, const addr *fromaddr, int fromaddrsize) {

   sc_connection *client;
   sc_player *p;

   /* Make sure socket is nonblocking */
   sc_net_set_nonblocking(socket);

   /* Check that we have space to accept this connection into */
   if(srv->connections >= SC_MAX_PLAYERS) {
      sc_net_shutdown(&socket);
      sc_net_set_error("connect_request", "Incoming connection rejected because server is full");
      return;
   }

   /* Check that a game is not in progress */
   if((c->game->state & SC_STATE_OPTIONS_FLAG) == 0) {
      sc_net_shutdown(&socket);
      sc_net_set_error("connect_request", "Incoming connection rejected because game is already running");
      return;
   }

   /* Register the connection with the TCP NET packet engine. */
   if(!tn_instantiate(&srv->clients[srv->connections].connection, socket)) {
      sc_net_shutdown(&socket);
      sc_net_set_error("connect_request", "Unable to register socket with packet engine");
      return;
   }

   /* Below we set up the connection so we can recieve a configuration packet from the client. */

   /* Add this player to the list of active players */
   client = &srv->clients[srv->connections++];
   client->socket = socket;
   memcpy(&client->address, fromaddr, fromaddrsize);
   client->flags = SC_CONN_SVR_IFLAGS;

   /* Setup player data */
   p = c->players[c->numplayers];
   sbprintf(p->name, sizeof(p->name), "Network Player %d", c->numplayers);
   p->aitype = SC_AI_NETWORK;
   p->tank = sc_tank_profile_lookup(c->tanks, 0);
   ++c->numplayers;

}



bool sc_net_server_run(sc_config *c, sc_server *srv) {

   sc_connection *client;
   addr address;
   socklen_t addrsize;
   int connid;
   int socket;
   bool tryagain;

   /* Sanity checks */
   if(c == NULL || srv == NULL) return(false);

   /* Look for new incoming connections */
   do {
      addrsize = sizeof(addr);
      socket = accept(srv->linein, (struct sockaddr *)&address, &addrsize);
      if(socket != -1) _sc_net_incoming_connect(c, srv, socket, &address, addrsize);
   } while(socket != -1);

   /* Check for incoming packets from existing clients */
   for(connid = 0; connid < srv->connections; ++connid) {
      srv->current = connid;
      client = &srv->clients[connid];
      tryagain = !SC_CONN_IS_DEAD(*client);
      while(tryagain) {
         tryagain = sc_net_recv_packet(client, c, (void *)srv, &sc_net_server_handle_packet);
         if(SC_CONN_IS_DEAD(*client)) tryagain = false;
      }
   }

   return(true);

}
