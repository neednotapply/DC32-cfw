/* $Header: /fridge/cvs/xscorch/libj/jstr/str_io.c,v 1.9 2009-04-26 17:39:30 jacob Exp $ */
/*

   libj - str_io.c               Copyright (C) 1998-2003 Justin David Smith
   justins (a) chaos2.org        http://chaos2.org/

   File I/O


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



sizea fcount(const char *fname) {
/* File-Count
   Counts the number of lines in a standard text file. Reads through a
   local read buffer for parsing. Return value is zero if an error occurs,
   or if the file isn't found, etc.. */

   char *buf;
   sizea count = 0;
   FILE *f;

   /* Give up now if the name wasn't given. */
   if(fname != NULL && *fname != '\0') {

      buf = (char *)malloc(LIBJ_READBUF);
      if(buf != NULL) {

         /* Try to open the file. On error return zero. */
         if(NULL != (f = fopen(fname, "r"))) {

            /* Read data until an EOF... */
            while(fgets(buf, LIBJ_READBUF, f) != NULL) {
               if(*(buf + STRLEN(buf) - 1) == '\n') ++count;
            }
            fclose(f);
         }
         free(buf);
      }
   }

   /* Return the final count */
   return(count);

}



