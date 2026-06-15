/* $Header: /fridge/cvs/xscorch/libj/jstr/str_info.c,v 1.10 2009-04-26 17:39:30 jacob Exp $ */
/*

   libj - str_info.c             Copyright (C) 1998-2003 Justin David Smith
   justins (a) chaos2.org        http://chaos2.org/

   Miscellaneous information about the string


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



sizea strlenn(const char *s) {
/* String-Length-Nullsafe
   Returns the string length, except if <s> = NULL, in which case 0 is
   returned. */

   return(STRLENN(s));

}



sizea strnlenn(const char *src, sizea maxlen) {
/* strnlenn
   Returns the length of the string, never scanning beyond maxlen
   characters of the string.  This function returns a value between
   0 and maxlen.  */

   const char *p = src;
   if(src == NULL || maxlen <= 0) return(0);
   while(maxlen > 0 && *p != '\0') {
      ++p;
      --maxlen;
   }
   return(p - src);

}



sizea strblenn(const char *src, sizea size) {
/* strblenn
   Returns the length of the string, assuming that the buffer size
   of src is size (so it never scans more than size-1 characters).
   This function returns a value between 0 and size-1.  */

   return(strnlenn(src, size - 1));

}



sizea strnumwords(const char *s) {
/* String-Number-of-Words */

   bool inword = false;
   sizea numwords = 0;

   if(s != NULL) {
      while(*s != '\0') {
         if(WHITESPACE(*s)) inword = false;
         else if(!inword) {
            ++numwords;
            inword = true;
         }
         ++s;
      }
   }
   return(numwords);

}



sizea strnumlines(const char *s) {
/* String-Number-of-Lines */

   sizea lines = 0;

   if(s != NULL && *s != '\0') {
      ++lines;
      while(*s != '\0') {
         if(*s == '\n') ++lines;
         ++s;
      }
   }
   return(lines);

}
