/* $Header: /fridge/cvs/xscorch/sgtk/snet-gtk.h,v 1.6 2009-04-26 17:39:49 jacob Exp $ */
/*
   
   xscorch - snet-gtk.h       Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   Function prototypes for network windows
    

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
#ifndef __snet_gtk_h_included
#define __snet_gtk_h_included


#include <swindow-gtk.h>
#include <snet/snet.h>


#if USE_NETWORK


#define SC_NET_INPUT_SIZE     0x100


/* Interface functions */
void sc_chat_window_gtk(sc_window_gtk *w);
void sc_network_server_gtk(sc_window_gtk *w);
void sc_network_client_gtk(sc_window_gtk *w);


#endif /* USE_NETWORK */


#endif /* __snet_gtk_h_included */
