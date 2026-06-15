/* $Header: /fridge/cvs/xscorch/libj/jstr/str_printf.c,v 1.9 2009-04-26 17:39:30 jacob Exp $ */
/*

   libj - str_printf.c           Copyright (C) 2001-2003 Justin David Smith
   justins (a) chaos2.org        http://chaos2.org/

   sprintf helper functions


   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation, version 2 of the License ONLY.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program, in the file COPYING. If not, write to the Free
   Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA


   This file is part of LIBJ.

*/
#define  LIBJ_ALLOW_LIBC_STRING    1
#include <libjstr.h>
#include <_str.h>
#include <stdarg.h>


char *sbprintf(char *dest, sizea size, const char *fmt, ...) {
/* sbprintf */

   va_list args;

   if(size > 0 && dest != NULL && fmt != NULL) {
      va_start(args, fmt);
      vsnprintf(dest, size, fmt, args);
      va_end(args);
      dest[size - 1] = '\0';
      return(dest);
   }
   return(NULL);

}


char *sbprintf_concat(char *dest, sizea size, const char *fmt, ...) {
/* sbprintf_concat */

   va_list args;
   sizea destlen;

   if(size > 0 && dest != NULL && fmt != NULL) {
      destlen = STRLEN(dest);
      if(destlen < size) {
         va_start(args, fmt);
         vsnprintf(dest + destlen, size - destlen, fmt, args);
         va_end(args);
      }
      dest[size - 1] = '\0';
      return(dest);
   }
   return(NULL);

}
