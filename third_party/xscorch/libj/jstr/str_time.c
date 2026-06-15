/* $Header: /fridge/cvs/xscorch/libj/jstr/str_time.c,v 1.8 2009-04-26 17:39:30 jacob Exp $ */
/*

   libj - str_time.c             Copyright (C) 1998-2003 Justin David Smith
   justins (a) chaos2.org        http://chaos2.org/

   Time manipulation and strings


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



#if HAVE_GETTIMEOFDAY



bool decodetime(struct tm *t, const char *s) {
/* Decode-Time
   Decodes the timestring (encoded by encodetime) <s> into the time
   structure <t>. Returns false if the source string is not a valid
   encoded time. Note that <t> must already be allocated. */

   char buf[20];
   udword count = 0;

   if(!t || !s) return(false);

   /* Check validity of the string */
   if(STRLEN(s) < 19) return(false);
   if(*(s + 4)  != '.') return(false);
   if(*(s + 7)  != '.') return(false);
   if(*(s + 10) != '@') return(false);
   if(*(s + 13) != ':') return(false);
   if(*(s + 16) != ':') return(false);

   /* Copy to a temp buffer for processing */
   MEMCPY(buf, s, 19);
   *(buf + 4)  = 0;
   *(buf + 7)  = 0;
   *(buf + 10) = 0;
   *(buf + 13) = 0;
   *(buf + 16) = 0;
   *(buf + 19) = 0;

   /* More checks */
   while(count < 19) {
      if(*(buf + count) && (*(buf + count) < '0' || *(buf + count) > '9')) return(false);
      count++;
   }

   /* Get the actual values and write them to the time structure. */
   t->tm_year =   strtoint(buf, 0) - 1900;
   t->tm_mon =    strtoint(buf + 5, 0) - 1;
   t->tm_mday =   strtoint(buf + 8, 0);
   t->tm_hour =   strtoint(buf + 11, 0);
   t->tm_min =    strtoint(buf + 14, 0);
   t->tm_sec =    strtoint(buf + 17, 0);
   return(true);

}


char *encodetime(char *d) {
/* Encodes the _current_ system time into the string <d>. Useful for
   logging functions and anything else needing a timestamp in it :)
   Note that the character buffer pointed to by <d> needs to be at
   least 20 characters long. */

   time_t tpacked;
   struct tm *t;

   if(!d) return(NULL);

   /* Grab the current system time  */
   time(&tpacked);
   t = localtime(&tpacked);

   /* Encode the time string  */
   MEMCPY(d, "yyyy.mm.dd@hh:mm:ss", 20);
   inttostr(d,      t->tm_year + 1900, 4);
   *(d + 4) = '.';
   inttostr(d + 5,  t->tm_mon + 1, 2);
   *(d + 7) = '.';
   inttostr(d + 8,  t->tm_mday, 2);
   *(d + 10) = '@';
   inttostr(d + 11, t->tm_hour, 2);
   *(d + 13) = ':';
   inttostr(d + 14, t->tm_min, 2);
   *(d + 16) = ':';
   inttostr(d + 17, t->tm_sec, 2);
   return(d);

}



#endif /* HAVE_GETTIMEOFDAY */
