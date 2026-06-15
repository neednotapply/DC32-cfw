/* $Header: /fridge/cvs/xscorch/libj/jstr/str_creat.c,v 1.8 2009-04-26 17:39:30 jacob Exp $ */
/*

   libj - str_creat.c            Copyright (C) 1998-2003 Justin David Smith
   justins (a) chaos2.org        http://chaos2.org/

   String cloning and creation


   This library is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation, version 2 of the License ONLY.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this library; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

   This file is part of LIBJ.

*/
#define  LIBJ_ALLOW_LIBC_STRING    1
#include <libjstr.h>
#include <_str.h>



char *strdupl(const char *s) {
/* String-Duplicate
   This function behaves like strdup() */

   char *t = NULL;
   sizea l;

   if(s != NULL) {
      l = STRLEN(s);
      t = (char *)malloc(l + 1);
      if(t != NULL) {
         MEMCPY(t, s, l + 1);
      } /* Allocate succeeded? */
   } /* Source not NULL? */
   return(t);

}

