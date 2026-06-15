/* $Header: /fridge/cvs/xscorch/snet/snetconnect.c,v 1.17 2011-08-01 00:01:43 jacob Exp $ */
/*
   
   xscorch - snetconnect.c    Copyright(c) 2000-2003 Justin David Smith
                              Copyright(c) 2001-2011 Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/

   Network connection open, close


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

#include <sgame/splayer.h>

#include <libj/jstr/libjstr.h>
#include <snet/tcpnet/tcpnet.h>



sc_server *sc_net_server_new(sc_config *c, int port) {

   addr socket_info;    /* Information about us */
   sc_server *srv;      /* Server object */
   int i;               /* Iterator */

   /* Construct a new server object and initialise it */   
   srv = (sc_server *)malloc(sizeof(sc_server));
   if(srv == NULL) {
      sc_net_set_error("server_new", "malloc failed");
      return(NULL);
   }
   srv->connections = 0;

   /* Open a new TCP socket */
   srv->linein = socket(PF_INET, SOCK_STREAM, DEFAULT_PROTOCOL);
   if(srv->linein == -1) {
      sc_net_set_error("server_new, socket", strerror(errno));
      free(srv);
      return(NULL);
   }

   /* Setup socket information about us; attempt to bind to a port */
   memset((char *)&socket_info, '\0', sizeof(socket_info));
   socket_info.sin_family = AF_INET;
   socket_info.sin_port = htons(port);
   socket_info.sin_addr.s_addr = INADDR_ANY;
   if(bind(srv->linein, (struct sockaddr *)&socket_info, sizeof(socket_info)) < 0) {
      sc_net_set_error("server_new, bind", strerror(errno));
      free(srv);
      return(NULL);
   }

   /* setup server socket not to block */
   sc_net_set_nonblocking(srv->linein);

   /* Game state initalisation */
   c->numplayers = 0;
   for(i = 0; i < SC_MAX_PLAYERS; ++i) {
      c->players[i]->aitype = SC_AI_RANDOM;
   }

   /* Prepare to listen for incoming packets */
   if(listen(srv->linein, 5) < 0) {
      sc_net_set_error("server_new, listen", strerror(errno));
      free(srv);
      return(NULL);
   }

   /* Display informative message */
   sc_net_set_info("server_new", "Network server established");

   /* Return the new socket object */
   return(srv);

}



void sc_net_server_free(sc_server **srv, const char *msg) {
/* sc_net_server_free
   Clean out a stale server struct. */

   int i;
   if(srv == NULL || *srv == NULL) return;
   for(i = 0; i < (*srv)->connections; ++i) {
      sc_net_send_message(&(*srv)->clients[i], SC_NET_SVR_QUIT, msg);
      tn_terminate(&(*srv)->clients[i].connection);
      sc_net_shutdown(&(*srv)->clients[i].socket);
   }
   sc_net_shutdown(&(*srv)->linein);
   free(*srv);
   *srv = NULL;
   return;

}



sc_client *sc_net_client_new(const char *name, const char *hostname, int port) {

   char versionstr[SC_NET_BUFFER_SIZE];  /* version info */
   sc_packet packet;    /* Transmitted data packet */
   host *host_info;     /* Remote host information */
   sc_client *cli;      /* Client object */

   /* Get my version data */
   sc_net_version_info(versionstr, sizeof(versionstr));

   /* Construct a new client object and initialise it */   
   cli = (sc_client *)malloc(sizeof(sc_client));
   if(cli == NULL) {
      sc_net_set_error("client_new", "malloc failed");
      return(NULL);
   }
   strcopyb(cli->name, name, sizeof(cli->name));

   /* Open a new TCP socket */
   cli->server.socket = socket(PF_INET, SOCK_STREAM, DEFAULT_PROTOCOL);
   if(cli->server.socket == -1) {
      sc_net_set_error("client_new, socket", strerror(errno));
      free(cli);
      return(NULL);
   }

   /* Get information about the remote host */
   host_info = gethostbyname(hostname);
   if(host_info == NULL) {
      sc_net_set_error("client_new, gethostbyname", "Cannot resolve hostname.");
      free(cli);
      return(NULL);
   }

   /* setup client destination address */
   memset((char *)&cli->server.address, '\0', sizeof(addr));
   memcpy((char *)&cli->server.address.sin_addr, host_info->HOSTENT_H_ADDR, host_info->h_length);
   cli->server.address.sin_family = host_info->h_addrtype;
   cli->server.address.sin_port = htons(port);

   /* Below we initiate a connection request to the server */

   /* Initialise server-client data */
   cli->server.flags = SC_CONN_CLI_IFLAGS;
   cli->server.syncarg = 0;

   /* Attempt a connect request. We want this call to block */
   if(connect(cli->server.socket, (const struct sockaddr *)&cli->server.address, sizeof(addr)) < 0) {
      sc_net_set_error("client_new, connect", strerror(errno));
      free(cli);
      return(NULL);
   }

   /* setup client socket not to block from now on */
   sc_net_set_nonblocking(cli->server.socket);

   /* Register the new connection with the TCP NET packet engine. */
   if(!tn_instantiate(&cli->server.connection, cli->server.socket)) {
      sc_net_set_error("client_new, TCP NET", strerror(errno));
      free(cli);
      return(NULL);
   }

   /* Initiate a connection request to the server */
   sc_net_packet_init(&packet, SC_NET_CLI_CONNECT, strlenn(versionstr) + sizeof(dword));
   *(dword *)packet.data = htonl(SC_NET_VERSION);
   memcpy(packet.data + sizeof(dword), versionstr, strlenn(versionstr));

   /* Attempt to send the packet */
   if(!sc_net_send_packet_now(&cli->server, &packet)) {
      free(cli);
      return(NULL);
   }
   sc_net_packet_release(&packet);
   
   /* Initialise the status data */
   sc_net_status_init(cli);

   /* Return the client connection */
   return(cli);

}



void sc_net_client_free(sc_client **cli, const char *msg) {
/* sc_net_client_free
   Let them run free! */

   if(cli == NULL || *cli == NULL) return;
   sc_net_send_message(&(*cli)->server, SC_NET_CLI_DISCONNECT, msg);
   sc_net_shutdown(&(*cli)->server.socket);
   free(*cli);
   *cli = NULL;
   return;

}
