/* $Header: /fridge/cvs/xscorch/libj/jstr/str_con.c,v 1.8 2009-04-26 17:39:29 jacob Exp $ */
/*

   libj - str_con.c              Copyright (C) 1998-2003 Justin David Smith
   justins (a) chaos2.org        http://chaos2.org/

   Console string helper functions


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



char *cpyslenn(char *d, const char *s, sizea size) {
/* Copy-String-Length
   Copies the first <size> characters of string <s> to string <d>. String
   <d> must already be allocated to support <size> + 1 characters (this
   will allow for the terminating null character). Remaining characters
   are filled in as spaces. <s> and <d> may point to the same string but
   <d> cannot point inside of <s>. Note: if <s> is bigger than <size> then
   trailing characters will be cut off. This command is just an
   overglorified memset() call if <s> = NULL :) */

   sizea count = STRLENN(s);
   char *dstart = d;       /* Original return pointer */

   if(d != NULL) {

      if(s != NULL) {
         /* Copy the most relevant characters in s. */
         if(count > size) count = size;
         MEMCPY(d, s, count);
         size -= count;
         d += count;
      }

      /* s is smaller than size -- fill in with some spaces. */
      MEMSET(d, ' ', size);
      d += size;
      *d = '\0';
      return(dstart);

   }
   return(NULL);

}


void putslenn(const char *s, sizea size) {
/* Put-String-Length
   Displays the first <size> characters of string <s> on the console.
   If <s> is smaller than <size> characters, then the remaining characters
   are filled in as spaces. Useful for table-like output. Note: if <s> is
   bigger than <size> then trailing characters will be cut off. */

   if(s != NULL) {
      while(size > 0 && *s != '\0') {
         /* Print out the first relevant characters of s. */
         putchar(*s);
         s++;
         size--;
      }
   }
   while(size > 0) {
      /* s is smaller than size -- fill in some spaces. */
      putchar(' ');
      size--;
   }

}


char *cpysrlenn(char *d, const char *s, sizea size) {
/* Copy-String-Rightaligned-Length
   Copies the first <size> characters of string <s> to string <d>. String
   <d> must already be allocated to support <size> + 1 characters (this
   will allow for the terminating null character). Preceding characters
   are filled in as spaces. Note: if <s> is bigger than <size> then
   trailing characters will be cut off. This command is just an
   overglorified memset() if <s> is NULL. :) */

   sizea count = STRLENN(s);
   char *dstart = d;    /* Original dest pointer. */

   if(d != NULL) {
      if(count < size) {
         /* We will have some preceding spaces. */
         count = size - count;
         MEMSET(d, ' ', count);
         size -= count;
         d += count;
      }
      if(s != NULL) {
         /* Copy the most relevant characters in s. */
         MEMCPY(d, s, size);
         d += size;
      }
      *d = '\0';
      return(dstart);

   }
   return(NULL);

}


void putsrlenn(const char *s, sizea size) {
/* Put-String-Rightaligned-Length
   Displays the first <size> characters of string <s> on the console.
   If <s> is smaller than <size> characters, then preceding characters
   are filled in as spaces. Useful for table-like output. Note: if <s> is
   bigger than <size> then trailing characters will be cut off. */

   sizea count = STRLENN(s);
   while(count < size) {
      /* We will have some preceding spaces. */
      putchar(' ');
      size--;
   }
   if(s != NULL) {
      while(size > 0 && *s != '\0') {
         /* Print out the first relevant characters of s. */
         putchar(*s);
         s++;
         size--;
      }
   }

}


