/* $Header: /fridge/cvs/xscorch/snet/snetcomm.c,v 1.15 2011-08-01 00:01:43 jacob Exp $ */
/*

   xscorch - snetcomm.c       Copyright(c) 2000-2003 Jacob Luna Lundberg
                              Copyright(c) 2000-2003 Justin David Smith
   jacob(at)gnifty.net        http://www.gnifty.net/
   justins(at)chaos2.org      http://chaos2.org/

   Communication functions, packet setup

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
#include <unistd.h>

#include <snetint.h>

#include <sutil/sslink.h>

#include <libj/jstr/libjstr.h>
#include <snet/tcpnet/tcpnet.h>



bool sc_net_set_nonblocking(int socket) {

   /* Setup the socket as nonblocking */
   if(fcntl(socket, F_SETFL, O_NONBLOCK) != 0) {
      sc_net_set_error("set_nonblocking", strerror(errno));
      return(false);
   }
   return(true);

}



bool sc_net_shutdown(int *socket) {

   /* Close down a TCP socket */
   if(socket == NULL || *socket < 0) return(true);
   shutdown(*socket, 2);
   close(*socket);
   *socket = -1;
   return(true);

}



bool sc_net_recv_packet(sc_connection *conn, sc_config *c, void *parm, packet_handler handler) {
/* sc_net_recv_packet()
   Pull packets off the network stream and tokenize them into the packet queue.
   Then return a single packet off the top of the queue. */

   sc_packet packet;
   dword *header;
   size_t size;

   /* Scan for new packets. */
   tn_scan_read(conn->connection, NULL);

   if(tn_read(conn->connection, &packet.data, &size)) {
      if(packet.data == NULL || size < SC_PACKET_HEADER_SIZE) {
         /* Supersmall packets! (This is by definition not possible!) */
         sc_net_set_error("recv_packet", "Packet size fields mismatch");
         SC_CONN_SET_FLAGS(*conn, SC_CONN_TCPNET_ERROR);
         sc_net_packet_release(&packet);
         return(false);
      }

      /* We read packet data.  Munch it! */
      header = (dword *)(packet.data + size);
      packet.data_size = ntohl(*(--header));
      packet.msg_type  = ntohl(*(--header));
      packet.next_rnd  = ntohl(*(--header));

      /* This is a little helper to catch buffer overwrites. */
      strcopyb((char *)header, "BUFOFLO", 2 * sizeof(dword));

      if(size != packet.data_size + SC_PACKET_HEADER_SIZE) {
         /* A badly damaged packet came in (shouldn't happen). */
         sc_net_set_error("recv_packet", "Packet size fields mismatch");
      }

      /* Handle the packet. */
      handler(c, parm, &packet);

      /* Clean up. */
      sc_net_packet_release(&packet);

      /* Ask for a rescan in case there are more packets. */
      return(true);
   } else {
      /* Check for error conditions. */
      if(TN_STATE_IS_ERROR(conn->connection)) {
         sc_net_set_error("recv_packet", tn_error_string(conn->connection));
         if(!TN_CONNECTION_IS_UP(conn->connection))
            /* A fatal error killed the connection. */
            SC_CONN_SET_FLAGS(*conn, SC_CONN_LOCAL_ERROR);
         else
            /* Eventually, the TCP NET library will have error recovery. */
            SC_CONN_SET_FLAGS(*conn, SC_CONN_TCPNET_ERROR);
      }
      return(false);
   }

}



bool sc_net_flush_packets(sc_connection *conn) {
/* sc_net_flush_packets
   Flush the packet queue. */

   if(!conn) return(false);

   return(tn_write_flush(conn->connection));

}



bool sc_net_send_packet(sc_connection *conn, sc_packet *packet) {
/* sc_net_send_packet()
   Try to queue a packet; true indicates success. */

   dword *header;

   if(!conn || !packet || !packet->data) return(false);

   /* Set up the header dwords on the end of the data. */
   header = (dword *)(packet->data + packet->data_size);
   *header++ = htonl(packet->next_rnd);
   *header++ = htonl(packet->msg_type);
   *header++ = htonl(packet->data_size);

   /* Queue the packet data. */
   if(tn_write(conn->connection, packet->data, packet->data_size + SC_PACKET_HEADER_SIZE))
      return(true);
   else
      return(false);

}



bool sc_net_send_packet_now(sc_connection *conn, sc_packet *packet) {
/* sc_net_send_packet_now()
   Try to transmit a packet; true indicates success. */

   if(!conn || !packet || !packet->data) return(false);

   /* Queue and flush the packet. */
   return(sc_net_send_packet(conn, packet) && sc_net_flush_packets(conn));

}



bool sc_net_send_message(sc_connection *conn, udword msg_type, const char *msg) {
/* sc_net_send_message()
   Wrapper for send_packet; the data payload will be a text message. */

   bool ret;
   sc_packet packet;

   if(!sc_net_packet_init(&packet, msg_type, min(strlenn(msg) + 1, SC_NET_BUFFER_SIZE))) return(false);
   strcopyb((char *)packet.data, msg, packet.data_size);
   ret = sc_net_send_packet_now(conn, &packet);
   sc_net_packet_release(&packet);
   return(ret);

}
