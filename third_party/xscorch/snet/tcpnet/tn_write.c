/* $Header: /fridge/cvs/xscorch/snet/tcpnet/tn_write.c,v 1.4 2011-08-01 00:01:44 jacob Exp $
 *
 * tn_write.c
 * File revision 2.xscorch
 * Packet writing via tcp net.
 * (c) 2001 Jacob Lundberg, jacob(at)gnifty.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


/*
 * 2001.07.15	initial revision
 * 2001.07.25	removing void
 * 		added GPL notice
 * 2001.07.26	fixed slight counting error
 * 		slight flushing optimization
 * 2001.07.26	branch for xscorch
 */


#include <string.h>
#include "tn_internal.h"


bool _tn_write_select(tn_connection *tnc) {
/*
 * tn_write_select
 * Scan the connection buffer to see if there are errors.
 * Returns true if there are no errors (it's safe to try and send).
 */

   fd_set fds;
   struct timeval maxwait;

   /* Perpare the descriptor. */
   FD_ZERO(&fds);
   FD_SET(tnc->socket, &fds);

   /* Set the wait time to 0. */
   maxwait.tv_sec  = 0;
   maxwait.tv_usec = 0;

   /* Find out if there are errors. */
   switch(select(tnc->socket + 1, NULL, &fds, NULL, &maxwait)) {
      case  1:
      case  0:
         /* There are no errors. */
         return(true);
      case -1:
         /* There was an error. */
         tn_set_state(tnc, TN_STATE_ERR_SELECT);
         return(false);
      default:
         /* Wrong number of selectors modified ... uhh, lol. */
         tn_set_state(tnc, TN_STATE_ERR_UNKNOWN);
         return(false);
   }

}


bool tn_write_flush(tn_connection *tnc) {
/*
 * tn_write_flush
 * Flush the transmit buffer.
 * Returns true if the buffer is now empty.
 */

   tn_packet_list *pl;
   byte header[TN_HEADER_SIZE];

   /* Make sure the connection is up. */
   if(tnc == NULL || !TN_CONNECTION_IS_UP(tnc)) return(false);

   /* Check for socket errors. */
   if(!_tn_write_select(tnc)) return(false);

   /* Get the top unwritten packet in the write chain. */
   pl = tnc->outgoing;
   while(pl != NULL && pl->complete) pl = pl->next;

   /* Send any unsent packets on the queue. */
   while(pl != NULL) {
      /* Send the header. */
      ((dword *)header)[0] = htonl(pl->packet->bop);
      ((dword *)header)[1] = htonl(pl->packet->size);
      ((dword *)header)[2] = htonl(pl->packet->id);
      if(!tn_send_buffer(tnc, header, TN_HEADER_SIZE))
         return(false);

      /* Send the payload. */
      if(!tn_send_buffer(tnc, pl->packet->payload, pl->packet->size))
         return(false);

      /* Advance the packet pointer. */
      pl->complete = true;
      pl = pl->next;
   }

   return(true);

}


bool tn_write(tn_connection *tnc, const byte *payload, size_t size) {
/*
 * tn_write
 * Write a packet onto the outgoing chain.
 */

   int counter = 0;
   tn_packet_list *pl;

   if(tnc == NULL || payload == NULL || size <= 0) return(false);

   /* Tack the packet onto the chain. */
   if(tnc->outgoing == NULL) {
      /* Starting a new chain. */
      tnc->outgoing = tn_packet_list_new();
      pl = tnc->outgoing;
   } else {
      /* Tack onto a chain. */
      pl = tnc->outgoing;
      while(pl->next != NULL) {
         if(pl->complete) ++counter;
         pl = pl->next;
      }
      pl->next = tn_packet_list_new();
      if(pl->complete) ++counter;
      pl = pl->next;
   }

   /* Make sure the packet list allocation succeeded. */
   if(pl == NULL) {
      tn_set_state(tnc, TN_STATE_ERR_MALLOC);
      return(false);
   }

   /* Allocate the packet data payload. */
   pl->packet->payload = (byte *)malloc(size);
   if(pl->packet->payload == NULL) {
      tn_packet_list_free(&pl);
      tn_set_state(tnc, TN_STATE_ERR_MALLOC);
      return(false);
   }

   /* Set up the new outgoing packet. */
   memcpy(pl->packet->payload, payload, size);
   pl->packet->bop  = TN_ALIGN_MAGIC;
   pl->packet->id   = ++(tnc->out_id);
   pl->packet->size = size;

   /* Drop expired packets. */
   pl = tnc->outgoing;
   while(--counter >= TN_LEN_RETAIN_OLD) {
      tnc->outgoing = pl->next;
      tn_packet_list_free(&pl);
      pl = tnc->outgoing;
   }

   return(true);

}
