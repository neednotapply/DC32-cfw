/* $Header: /fridge/cvs/xscorch/sgame/soptions.h,v 1.4 2009-04-26 17:39:41 jacob Exp $ */
/*
   
   xscorch - soptions.h       Copyright(c) 2000 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched option processing
    

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
#ifndef __soptions_h_included
#define __soptions_h_included


#include <xscorch.h>


struct _sc_config;


int sc_options_usage(const char *progname);
int sc_options_parse(struct _sc_config *c, int argc, char **argv);


#endif /* __soptions_h_included */
