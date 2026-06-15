/* $Header: /fridge/cvs/xscorch/snet/tcpnet/tn_setup.c,v 1.4 2011-08-01 00:01:44 jacob Exp $
 *
 * tn_setup.c
 * File revision 3.xscorch
 * Setup and termination functions for tcp net.
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
 * 2001.07.15	initial revision
 * 2001.07.25	clean up shutdown more
 * 		added tn_error_string
 * 		added GPL notice
 * 2002.05.20	cosmetic change
 * 2002.05.20	branch for xscorch
 */


#include "tn_internal.h"


bool tn_instantiate(tn_connection **tnc, int socket) {
/*
 * tn_initiate
 * Create a new connection struct.
 * Will overwrite *tnc so don't pass it something precious!
 */

   /* Only operate on an extant pointer. */
   if(tnc == NULL) return(false);

   /* Allocate the new connection struct. */
   (*tnc) = tn_connection_new();
   if((*tnc) == NULL) return(false);

   (*tnc)->socket = socket;
   tn_set_state((*tnc), TN_STATE_CONNECTED);

   return(true);

}


bool tn_terminate(tn_connection **tnc) {
/*
 * tn_terminate
 * Close out a connection and free up its structs.
 */

   if(tnc == NULL || (*tnc) == NULL) return(true);

   /* Try and make sure the packet queues are flushed. */
   if(!TN_STATE_IS_ERROR(*tnc)) {
      tn_write_flush(*tnc);
      tn_scan_read(*tnc, NULL);
   }

   /* Shut the socket down. */
   if((*tnc)->state & TN_STATE_CONNECTED && (*tnc)->socket >= 0) {
      shutdown((*tnc)->socket, 2);
      close((*tnc)->socket);
      (*tnc)->socket = -1;
   }

   /* Free the structs. */
   tn_connection_free(tnc);

   return(true);

}


const char *tn_error_string(tn_connection *tnc) {
/*
 * tn_error_string
 * Generate a string describing the connection state.
 */

   int state;
   const char *ret;

   static const char *strings[11] = {
      /* Single-bit errors. */
      "Uninitialized",
      "Connected, no error",
      "Connection lost",
      "Malloc failed",
      "Packet misalignment detected",
      "Packet received out of order",
      "Select failed",
      "Unknown error",

      /* Other. */
      "Multiple errors detected",
      "Invalid argument to tn_error_string",
       NULL
   };

   /* Early return. */
   if(tnc == NULL) return(strings[9]);

   state = tnc->state;

   /* Figure out what's up and tell the user. */
   switch(state) {
      case TN_STATE_DEFAULT:
         ret = strings[0];
         break;
      case TN_STATE_CONNECTED:
         ret = strings[1];
         break;
      case TN_STATE_ERR_CONNLOST:
         ret = strings[2];
         break;
      case TN_STATE_ERR_MALLOC:
         ret = strings[3];
         break;
      case TN_STATE_ERR_MISALIGN:
         ret = strings[4];
         break;
      case TN_STATE_ERR_MISORDER:
         ret = strings[5];
         break;
      case TN_STATE_ERR_SELECT:
         ret = strings[6];
         break;
      case TN_STATE_ERR_UNKNOWN:
         ret = strings[7];
         break;
      default:
         ret = strings[8];
         break;
   }

   return(ret);

}
