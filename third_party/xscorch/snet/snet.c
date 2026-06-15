/* $Header: /fridge/cvs/xscorch/snet/snet.c,v 1.13 2011-08-01 00:01:43 jacob Exp $ */
/*
   
   xscorch - snet.c           Copyright(c) 2000-2003 Justin David Smith
                              Copyright(c) 2001-2003 Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/
    
   Network main file
    

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
#define  _BSD_SOURCE       /* Needed for gethostname */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <snetint.h>

#include <libj/jstr/libjstr.h>
#include <snet/tcpnet/tcpnet.h>
#include <sutil/srand.h>



#define SC_NET_ERROR_LENGTH       0x100
static char _net_error[SC_NET_ERROR_LENGTH] = { '\0' };



void sc_net_set_error(const char *function, const char *errormsg) {

   /* Write the new error message to the internal error variable */
   sbprintf(_net_error, sizeof(_net_error), "(%s)  %s", function, errormsg);
   fprintf(stderr, "network error: %s\n", _net_error);

}



void sc_net_set_info(const char *function, const char *errormsg) {

   /* Write the new error message to the internal error variable */
   printf("network: (%s)  %s\n", function, errormsg);

}



const char *sc_net_get_error(void) {

   /* Return a pointer to the internal error variable */
   return(_net_error);

}



int sc_net_get_hostname(char *buf, int size) {

   if(buf == NULL || size <= 0) return(0);
   if(gethostname(buf, size) == 0) {
      buf[size - 1] = '\0';
      return(1);
   } else {
      strcopyb(buf, "localhost", size);
      return(0);
   }

}



void sc_net_version_info(char *buf, int size) {

   if(buf == NULL || size <= 0) return;
   sbprintf(buf, size, "XScorch %s-net%d.%d.%d",
            VERSION, SC_NET_MAJOR_VERSION, SC_NET_MINOR_VERSION, SC_NET_PATCH_VERSION);

}



bool sc_net_check_size(const sc_packet *packet, sizea expectedsize, const char *description) {

   char buffer[SC_NET_BUFFER_SIZE];

   if(packet->data_size < expectedsize) {
      sbprintf(buffer, sizeof(buffer), "Size of data received is too small, %d < %d", packet->data_size, expectedsize);
      sc_net_set_error(description, buffer);
      return(false);
   }
   return(true);

}



bool sc_net_check_param(dword actual, dword expected, const char *description, const char *param) {

   char buffer[SC_NET_BUFFER_SIZE];

   if(actual != expected) {
      sbprintf(buffer, sizeof(buffer), "Data mismatch in expected %s data: %08x != %08x", param, actual, expected);
      sc_net_set_error(description, buffer);
      return(false);
   }
   return(true);

}



bool sc_net_packet_init(sc_packet *packet, dword type, dword size) {
/* sc_net_packet_init
   Prepare a packet to have data dumped into it. */

   if(packet == NULL) return(false);

   /* Packets get a small extra data size so the header can be sent easily. */
   packet->data = (byte *)malloc((size + SC_PACKET_HEADER_SIZE) * sizeof(byte));
   if(packet->data == NULL) return(false);
   packet->next_rnd  = game_rand_peek();
   packet->msg_type  = type;
   packet->data_size = size;
   return(true);

}



bool sc_net_packet_release(sc_packet *packet) {
/* sc_net_packet_release()
   Free the data from a packet; true on success. */

   if(packet == NULL) return(false);

   free(packet->data);
   packet->next_rnd  = -1;
   packet->msg_type  = 0;
   packet->data_size = 0;
   packet->data = NULL;
   return(true);

}
