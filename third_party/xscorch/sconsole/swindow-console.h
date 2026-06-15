/* $Header: /fridge/cvs/xscorch/sconsole/swindow-console.h,v 1.4 2009-04-26 17:39:36 jacob Exp $ */
/*
   
   xscorch - swindow-console.h   Copyright(c) 2001 Justin David Smith
   justins(at)chaos2.org         http://chaos2.org/
    
   Console interface to xscorch (server)
    

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
#ifndef __swindow_console_h_included
#define __swindow_console_h_included


/* The order of includes does matter! */
#include <xscorch.h>
#include <sgame/swindow.h>


/* Forward structure declarations */
struct _sc_config;


/* Main window structure */
typedef struct _sc_window_console {
   struct _sc_config *c;         /* Game config structure */
} sc_window_console;


#endif /* __swindow_console_h_included */
