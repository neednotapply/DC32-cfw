/* $Header: /fridge/cvs/xscorch/libj/jstr/str_trim.c,v 1.8 2009-04-26 17:39:30 jacob Exp $ */
/*

   libj - str_trim.c             Copyright (C) 1998-2003 Justin David Smith
   justins (a) chaos2.org        http://chaos2.org/

   String trimming


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



char *trim(char *s) {
/* Trim
   trims off leading and trailing whitespace, \n, \t, etc off of the
   string passed in <cmd>. Since this function only removes characters,
   sizeof(final_<s>) <= sizeof(init_<s>), so we won't have to worry
   about overflowing memory or anything :) */

   /* Pointers into the string (used to parse whitespace out) */
   char *d = s;
   whitespace ws;

   if(s != NULL) {

      SKIM_WHITESPACE(s);
      SET_FIRST_NWS(ws, s);
      SET_LAST_NWS(ws, s);

      /* Copy the non-ws characters in <s>. */
      if(ws.fnws > d) MEMCPY(d, ws.fnws, NWS_SIZE(ws));
      *(d + NWS_SIZE(ws)) = '\0';
      return(d);

   }
   return(NULL);

}



char *rtrim(char *s) {
/* R-Trim
   trims off trailing whitespace, \n, \t, etc off of the
   string passed in <cmd>. Since this function only removes characters,
   sizeof(final_<s>) <= sizeof(init_<s>), so we won't have to worry
   about overflowing memory or anything :) */

   /* Pointers into the string (used to parse whitespace out) */
   char *d = s;
   whitespace ws;

   if(s != NULL) {
      ws.lnws = s;
      SET_LAST_NWS(ws, s);
      *ws.lnws = '\0';
      return(d);
   }
   return(NULL);

}



char *ltrim(char *s) {
/* L-Trim
   trims off leading whitespace, \n, \t, etc off of the
   string passed in <cmd>. Since this function only removes characters,
   sizeof(final_<s>) <= sizeof(init_<s>), so we won't have to worry
   about overflowing memory or anything :) */

   /* Pointers into the string (used to parse whitespace out) */
   char *d = s;

   if(s != NULL) {
      SKIM_WHITESPACE(s);
      MEMCPY(d, s, STRLEN(s) + 1);
      return(d);
   }
   return(NULL);

}



char *trimcomment(char *s, char c) {
/* Trim-Comment
   Trims off the comment in string <s>, which is specified as anything
   following the character specified in <c>. */

   char *p = s;

   if(s != NULL) {
      while(*p != '\0') {
         if(*p == c) *p = '\0';
         else p++;
      }
   }
   return(s);

}



char *trimbracecomment(char *s, char c1, char c2) {
/* Trim-Brace-Comment
   Trims off the comment in string <s>, which is specified as anything
   following the character specified in <c1>, up to the occurrence of
   <c2>. */

   char *p = s;
   char *p1;

   if(s != NULL) {
      while(*p != '\0') {
         if(*p == c1) {
            p1 = p;
            while(*p != '\0' && *p != c2) p++;
            if(*p != '\0') {
               p++;
               STRCPY(p1, p);
               p = p1;
            } else *p1 = '\0';
         } else p++;
      }
   }
   return(s);

}
