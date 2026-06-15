/* $Header: /fridge/cvs/xscorch/snet/tcpnet/tn_internal.h,v 1.6 2011-08-01 00:01:44 jacob Exp $
 *
 * tn_internal.h
 * File revision 3.xscorch
 * Internally available declarations for tcp net.
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
 * 2001.07.25	removing void
 * 		added GPL notice
 * 2001.08.15	security cookie
 * 		buffer_copy len must be int
 * 2003.02.24	define _BSD_SOURCE
 * 		allow assert()
 * 		larger buffers
 * 		size_t -> int, for C99
 * 2003.02.24	branch for xscorch
 */


#ifndef  __TN_INTERNAL_H__
#define  __TN_INTERNAL_H__


/* Needed for usleep */
#ifndef  _BSD_SOURCE
#define  _BSD_SOURCE
#endif


/* Need the public declarations and common headers. */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "tcpnet.h"


/* Constants. */
#define  TN_ALIGN_MAGIC         0x5AD1571C      /* Packet alignment cookie. */
#define  TN_ALIGN_MUGGLE        0x8057bde1      /* Packet initialization cookie. */
#define  TN_BUFFER_SIZE         0x00000400      /* Working buffer size. */
#define  TN_BLOCKING_TIMEOUT    1000            /* Timeout in microseconds. */
#define  TN_LEN_RETAIN_OLD      16              /* How many old packets to keep. */
#define  TN_PROTOCOL_TCP        0               /* The socket() id for TCP. */


/* Functions implemented inline. */

inline static void tn_set_state(tn_connection *tnc, int state) {
/*
 * tn_set_state
 * Add flags to the connection state.
 */

  tnc->state |= state;

}

inline static void tn_clr_state(tn_connection *tnc, int state) {
/*
 * tn_clr_state
 * Remove flags from the connection state.
 */

  tnc->state &= ~state;

}


/* Various supporting functions. */
bool tn_buffer_copy(byte *dst, byte **src, size_t *cur, int *avl, int len);
bool tn_send_buffer(tn_connection *tnc, const byte *buffer, size_t size);


/* Constructors. */
tn_packet *tn_packet_new(void);
tn_packet_list *tn_packet_list_new(void);
tn_connection *tn_connection_new(void);


/* Destructors. */
void tn_packet_free(tn_packet **packet);
void tn_packet_list_free(tn_packet_list **item);
void tn_connection_free(tn_connection **tnc);


#endif /* ndef __TN_INTERNAL_H__ */
