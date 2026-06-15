/* $Header: /fridge/cvs/xscorch/sgame/scffile.h,v 1.6 2009-04-26 17:39:38 jacob Exp $ */
/*
   
   xscorch - scffile.h        Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   Configuration file processing
    

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
#ifndef __scffile_h_included
#define __scffile_h_included


/* Includes */
#include <xscorch.h>


/* Forward structure definitions */
struct _sc_config;


/* Constants */
#define  SC_CONFIG_VERSION    0x0002


int  sc_config_file_init(struct _sc_config *c);
int  sc_config_file_load(struct _sc_config *c);
int  sc_config_file_save(struct _sc_config *c);


#endif /* __scffile_h_included */
