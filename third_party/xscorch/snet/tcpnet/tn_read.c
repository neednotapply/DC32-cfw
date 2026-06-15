/* $Header: /fridge/cvs/xscorch/snet/tcpnet/tn_read.c,v 1.7 2011-08-01 00:01:44 jacob Exp $
 *
 * tn_read.c
 * File revision 4.xscorch
 * Packet reading via tcp net.
 * (c) 2002,2001 Jacob Lundberg, jacob(at)gnifty.net
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
 * 2001.07.14	initial revision
 * 2001.07.25	removing void
 * 		fixed static buffer evilness
 * 		added GPL notice
 * 2001.07.26	formatting changes
 * 		fixed bad NULL check
 * 2001.08.15	slight optimization
 * 		protect the BOP from meanies
 * 2002.05.20	make MSG flags portable
 * 2002.05.20	branch for xscorch
 */


#include "tn_internal.h"


bool _tn_read_select(tn_connection *tnc, struct timeval *timeout) {
/*
 * tn_read_select
 * Scan the connection buffer to see if data is waiting.
 * Returns true if there is any data waiting to be read.
 * Waits for timeout if one is given.
 */

   fd_set fds;
   struct timeval maxwait;

   /* Perpare the descriptor. */
   FD_ZERO(&fds);
   FD_SET(tnc->socket, &fds);

   /* Set the maximum wait time. */
   maxwait.tv_sec  = timeout ? timeout->tv_sec  : 0;
   maxwait.tv_usec = timeout ? timeout->tv_usec : 0;

   /* Find out if there is data to read. */
   switch(select(tnc->socket + 1, &fds, NULL, NULL, &maxwait)) {
      case  1:
         /* Data is available to read. */
         return(true);
      case  0:
         /* There was no data to read. */
         return(false);
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



bool tn_scan_read(tn_connection *tnc, struct timeval *timeout) {
/*
 * tn_scan_read
 * Scan the connection buffer and read in new packets.
 * Returns true if there are any packets awaiting a read.
 * Waits for timeout if one is given.
 */

   bool ret;
   int length;
   tn_packet_list *pl;
   byte buffer[TN_BUFFER_SIZE];
   byte *bufptr;

   /* Make sure the connection is up. */
   if(tnc == NULL || !TN_CONNECTION_IS_UP(tnc)) return(false);

   /* Check for data. */
   if(!_tn_read_select(tnc, timeout)) return(false);

   /* Set up the working pointer. */
   pl = tnc->incoming;

   /* There may be no packets at all... */
   if(pl == NULL) {
      pl = tn_packet_list_new();
      if(pl == NULL) {
         tn_set_state(tnc, TN_STATE_ERR_MALLOC);
         return(false);
      }
      tnc->incoming = pl;
   }

   /* Find the last packet in the read chain. */
   while(pl->next != NULL) pl = pl->next;

   /* Read the data. */
   errno = 0;
   ret = false;
   while((length = recv(tnc->socket, buffer, TN_BUFFER_SIZE, MSG_FLAGS))) {

      /* Check for errors. */
      if(length < 0) {
         switch(errno) {
            case ENOTCONN:
            case ENOTSOCK:
               /* Connection is down. */
               tn_set_state(tnc, TN_STATE_ERR_CONNLOST);
               return(false);
            case EAGAIN:
               /* We've seen all there is to see here today. */
               return(ret);
            case EINTR:
               /* Wait and pick up the rest on the next call. */
               return(false);
            case 0:
            default:
               /* Unknown error, whee! */
               tn_set_state(tnc, TN_STATE_ERR_UNKNOWN);
               return(false);
         }
      }

      /* Iterate on new packets until we run out of data. */
      bufptr = buffer;
      while(length) {
         /* Allocate a new packet, if needed. */
         if(pl->complete) {
            pl->next = tn_packet_list_new();
            if(pl->next == NULL) {
               tn_set_state(tnc, TN_STATE_ERR_MALLOC);
               return(false);
            }
            pl = pl->next;
         }

         /* Complete the header. */
         if(tn_buffer_copy((pl->packet->payload + pl->size), &(bufptr),
                           &(pl->size), &(length), TN_HEADER_SIZE - pl->size)) {
            /* Perform tasks that happen once per packet. */
            if(pl->packet->bop == TN_ALIGN_MUGGLE) {
               /* Read the packet control data with proper byte ordering. */
               pl->packet->bop  = ntohl(((dword *)pl->packet->payload)[0]);
               pl->packet->size = ntohl(((dword *)pl->packet->payload)[1]);
               pl->packet->id   = ntohl(((dword *)pl->packet->payload)[2]);

               /* Security measure.  Avoid people spoofing incomplete packets. */
               if(pl->packet->bop == TN_ALIGN_MUGGLE) pl->packet->bop = 0;

               /* Allocate space for the payload if necessary. */
               if(pl->packet->size > TN_BUFFER_SIZE) {
                  pl->packet->payload = (byte *)realloc(pl->packet->payload, pl->packet->size * sizeof(byte));
                  if(pl->packet->payload == NULL) {
                     tn_set_state(tnc, TN_STATE_ERR_MALLOC);
                     return(false);
                  }
               }
            }
            /* Test for packet compliance via alignment. */
            if(pl->packet->bop != TN_ALIGN_MAGIC) {
               tn_set_state(tnc, TN_STATE_ERR_MISALIGN);
               return(false);
            }
            /* Test for packet compliance via ordering. */
            if(pl->packet->id != tnc->inc_id + 1) {
               tn_set_state(tnc, TN_STATE_ERR_MISORDER);
               return(false);
            }
            /* Complete the payload. */
            if(tn_buffer_copy((pl->packet->payload + pl->size - TN_HEADER_SIZE),
                              &(bufptr), &(pl->size), &(length),
                              pl->packet->size - pl->size + TN_HEADER_SIZE)) {
               pl->complete = true;
               ++(tnc->inc_id);
               ret = true;
            }
         }
      }
      errno = 0;
   }

   return(ret);

}


bool tn_read(tn_connection *tnc, byte **payload, size_t *size) {
/*
 * tn_read
 * Read a packet payload into a newly created buffer.
 * Upon returning, size will be the length of the payload.
 */

   tn_packet_list *pl;

   /* Basic requirements. */
   if(payload == NULL || size == NULL) return(false);

   /* If the packet exists and is a completed packet: */
   if(tnc != NULL && tnc->incoming != NULL && tnc->incoming->complete) {
      /* Drop it off the incoming packet chain. */
      pl = tnc->incoming;
      tnc->incoming = tnc->incoming->next;

      /* Extract the packet payload. */
      (*payload) = pl->packet->payload;
      (*size)    = pl->packet->size;

      /* Destroy the packet list item. */
      pl->packet->payload = NULL;
      tn_packet_list_free(&pl);
      return(true);
   } else {
      /* No packets were available. */
      (*payload) = NULL;
      (*size)    = 0;
      return(false);
   }

}
