/* $Header: /fridge/cvs/xscorch/snet/tcpnet/tn_internal.c,v 1.8 2011-08-01 00:01:44 jacob Exp $
 *
 * tn_internal.c
 * File revision 6.xscorch
 * Internal methods for tcp net.
 * (c) 2001-2003 Jacob Lundberg, jacob(at)gnifty.net
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
 * 2001.07.25	removing void crap
 * 		made packet_new allocate buffer
 * 		added GPL notice
 * 2001.07.27	int -> size_t, darnit
 * 2001.08.15	close packet magic security hole
 * 		size_t -> int, we need negative len in buffer_copy
 * 2002.05.20	make MSG flags portable
 * 2003.02.24	size_t -> int, for C99
 * 2003.02.24	branch for xscorch
 */


#include "tn_internal.h"
#include <string.h>


bool tn_buffer_copy(byte *dst, byte **src, size_t *cur, int *avl, int len) {
/*
 * tn_buffer_copy
 * Copy src to dst until cur becomes len unless not enough avl.
 * This has side-effects of increasing src and cur and decreasing avl.
 */

   bool ret = true;
   int toc = len;

   /* Although an int, *avl is required to be >= 0. */
   assert(*avl >= 0);

   /* Zero-copy always succeeds. */
   if(len <= 0) return(true);

   /* Check if we can fill the request. */
   if(toc > (*avl)) {
      toc = (*avl);
      ret = false;
   }

   /* Don't try copies on too little src. */
   if(toc <= 0) return(false);

   /* Perform the copy. */
   memcpy(dst, (*src), toc);

   /* Perform side-effects. */
   (*src) += toc;
   (*cur) += toc;
   (*avl) -= toc;

   /* Return true if we fully filled the request. */
   return(ret);

}


bool tn_send_buffer(tn_connection *tnc, const byte *buffer, size_t size) {
/*
 * tn_send_buffer
 * Transmist a buffer with fully blocking IO.
 */

   int sent;

   while(size) {
      errno = 0;
      sent = send(tnc->socket, buffer, size, MSG_FLAGS);
      if(sent < 0) {
         switch(errno) {
            case EAGAIN:
               /* Wait a bit before we loop. */
               usleep(TN_BLOCKING_TIMEOUT);
            case EINTR:
               /* Loop around again and try once more. */
               break;
            case ENOBUFS:
            case ENOMEM:
               /* Set error to OOM. */
               tn_set_state(tnc, TN_STATE_ERR_MALLOC);
               return(false);
            case EPIPE:
               /* Connection state entered shutdown. */
               tn_set_state(tnc, TN_STATE_ERR_CONNLOST);
               return(false);
            case 0:
            default:
               /* Dunno what's wrong. */
               tn_set_state(tnc, TN_STATE_ERR_UNKNOWN);
               return(false);
         }
      } else {
         /* Partial or full transmission. */
         size -= sent;
         buffer += sent;
      }
   }

   return(true);

}


tn_packet *tn_packet_new(void) {
/*
 * tn_packet_new
 * Create and default a new packet.
 */

   tn_packet *packet;

   /* Return NULL if we can't alloc the thing. */
   packet = (tn_packet *)malloc(sizeof(tn_packet));
   if(packet == NULL) return(NULL);

   /* Allocate the default buffer chunk. */
   packet->payload = (byte *)malloc(TN_BUFFER_SIZE * sizeof(byte));
   if(packet->payload == NULL) {
      free(packet);
      return(NULL);
   }
   memset(packet->payload, 0, TN_BUFFER_SIZE);

   /* Set defaults. */
   packet->bop     = TN_ALIGN_MUGGLE;
   packet->size    = 0;
   packet->id      = 0;

   return(packet);

}


tn_packet_list *tn_packet_list_new(void) {
/*
 * tn_packet_list_new
 * Create a new packet list item.
 */

   tn_packet_list *item;

   /* Allocate memory, failure return NULL. */
   item = (tn_packet_list *)malloc(sizeof(tn_packet_list));
   if(item == NULL) return(NULL);

   /* Tack a packet onto the item. */
   item->packet = tn_packet_new();
   if(item->packet == NULL) {
      free(item);
      return(NULL);
   }

   /* Set defaults on the item. */
   item->next     = NULL;
   item->size     = 0;
   item->complete = false;

   return(item);

}


tn_connection *tn_connection_new(void) {
/*
 * tn_connection_new
 * Create a new connection descriptor.
 */

   tn_connection *tnc;

   /* Try to snag some mem. */
   tnc = (tn_connection *)malloc(sizeof(tn_connection));
   if(tnc == NULL) return(NULL);

   /* Set up defaults. */
   tnc->state = TN_STATE_DEFAULT;
   tnc->socket = -1;
   tnc->inc_id = 0;
   tnc->out_id = 0;
   tnc->incoming = NULL;
   tnc->outgoing = NULL;

   return(tnc);

}


void tn_packet_free(tn_packet **packet) {
/*
 * tn_packet_free
 * Free a packet.
 */

   if(packet != NULL && (*packet) != NULL) {
      free((*packet)->payload);
      free(*packet);
      (*packet) = NULL;
   }

}


void tn_packet_list_free(tn_packet_list **item) {
/*
 * tn_packet_list_free
 * Free a packet list item.
 */

   if(item != NULL && (*item) != NULL) {
      tn_packet_free(&((*item)->packet));
      free(*item);
      (*item) = NULL;
   }

}


void _tn_packet_chain_free(tn_packet_list **list) {
/*
 * _tn_packet_chain_free
 * Free a chain of packet list items.
 */

   tn_packet_list *item;

   if(list == NULL) return;

   while((*list) != NULL) {
      item = (*list);
      (*list) = item->next;
      tn_packet_list_free(&item);
   }

}


void tn_connection_free(tn_connection **tnc) {
/*
 * tn_connection_free
 * Free a connection struct.
 */

   if(tnc != NULL && (*tnc) != NULL) {
      _tn_packet_chain_free(&((*tnc)->incoming));
      _tn_packet_chain_free(&((*tnc)->outgoing));
      free(*tnc);
      (*tnc) = NULL;
   }

}
