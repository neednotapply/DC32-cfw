/* $Header: /fridge/cvs/xscorch/libj/jstr/str_arg.c,v 1.10 2010-09-03 07:49:37 justins Exp $ */
/*

   libj - str_arg.c              Copyright (C) 1998-2003 Justin David Smith
   justins (a) chaos2.org        http://chaos2.org/

   Argument processing


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



sizea getnumargs(const char *s, char sep) {
/* Get-Number-of-Arguments
   total number of arguments in a cmd string (assuming '|' separates
   arguments) You can optionally specify the "sep" character to
   denote your own character to separate arguments. Note, this
   command will stop processing when it reaches a null character.
   This command will return 0 if the string is empty, or 1 if no
   separator characters are found. */

   sizea r = 0;

   if(s != NULL && *s != '\0') {
      r = 1;
      while(*s != '\0') {
         if(*s == sep) ++r;
         ++s;
      }
   }
   return(r);

}



static inline sizea __getargpt(char **d, char *s, sizea argnum, char sep) {
/* internal-Get-Argument-Pointer-Trim
   returns a pointer to the argnum'th argument in '|'-delimited cmd
   string (or you can optionally specify your own separator to use).
   This command will stop processing at a null character. If argnum
   is not valid, a NULL pointer is returned and the return length
   is zero. Otherwise, the return length is the length of the
   argument. This command will trim the argument beforehand, which
   is reflected in the returned pointer and size. */

   whitespace ws;

   /* Check for validity of argnum (ie, give up if it is too small), d, s */
   if(d != NULL) {
      *d = NULL;
      if(s != NULL) {
         /* Search for the <argnum>'th argument. */
         while(argnum > 0 && *s != '\0') {
            if(*s == sep) --argnum;
            ++s;
         }

         /* If argnum > 0, then argnum supplied was too large. */
         if(argnum == 0) {
            /* Start tracking the argument until the end of argument; we'll
               also trim this argument for user convenience :) */
            SKIM_WHITESPACE_T(s, sep);
            SET_FIRST_NWS(ws, s);
            SET_LAST_NWS_T(ws, s, sep);

            /* Return pointer and size */
            *d = ws.fnws;
            return(NWS_SIZE(ws));
         } /* Did we have an arg to fetch? */
      }
   }

   return(0);

}



static inline char *__getargb(char *d, const char *s, sizea argnum, char sep, sizea maxlen) {
/* internal-Get-Argument-Countbuffer
   returns the argnum'th argument in '|'-delimited cmd string (or
   you can optionally specify your own separator to use). This command
   will stop processing at a null character. If argnum is not valid,
   an empty string is returned. The target buffer should be at least
   <maxlen> characters (counting NULL); any trailing characters will be
   truncated, and a NULL will always be squeezed in as needed. If
   <maxlen> is <= zero, then the target buffer should be at least as
   large as <s> buffer.
   If <s> = <NULL>, then this function will laugh at you. If <d> is
   NULL, you're screwed. */

   char *p;
   sizea size;

   if(d != NULL) {
      *d = '\0';

      /* Search for the <argnum>'th argument. */
      char *s_ptr = (char *)s;
      size = __getargpt(&p, s_ptr, argnum, sep);
      if(p != NULL && size > 0) {
         /* Copy the argument and exit */
         ++size;
         if(maxlen > 0 && size > maxlen) size = maxlen;
         STRNCPY(d, p, size);
      }

      /* Always return the destination */
      return(d);

   /* Destination was NULL */
   } else return(NULL);

}


static inline char *__getarg(char *d, const char *s, sizea argnum, char sep) {
/* Internal-Get-Argument
   returns the argnum'th argument in '|'-delimited cmd string (or
   you can optionally specify your own separator to use). This command
   will stop processing at a null character. If argnum is not valid,
   an empty string is returned. The target buffer should be at least
   as large as the source string. */

   return(__getargb(d, s, argnum, sep, 0));

}


char *getarg(char *d, const char *s, sizea argnum, char sep) {
/* Get-Argument */

   return(__getarg(d, s, argnum, sep));

}


char *getargb(char *d, const char *s, sizea argnum, char sep, sizea maxlen) {
/* Get-Argument-Countbuffer */

   return(__getargb(d, s, argnum, sep, maxlen));

}


char *getargn(char *d, const char *s, sizea argnum, char sep, sizea n) {
/* Get-Argument-Countbuffer */

   return(__getargb(d, s, argnum, sep, n + 1));

}


sizea getargp(char **d, char *s, sizea argnum, char sep) {
/* Get-Argument-Pointer
   returns a pointer to the argnum'th argument in '|'-delimited cmd
   string (or you can optionally specify your own separator to use).
   This command will stop processing at a null character. If argnum
   is not valid, a NUILL pointer is returned and the return length
   is zero. Otherwise, the return length is the length of the
   argument. This command makes no effort to trim the argument
   beforehand. */

   if(d != NULL) {
      *d = NULL;

      /* Is <argnum> inside the valid range for this string?  */
      if(s != NULL) {
         /* Search for the <argnum>'th argument.  */
         while(argnum > 0 && *s) {
            if(*s == sep) --argnum;
            ++s;
         }
         if(argnum == 0) {
            /* Start tracking the argument until the end of argument */
            *d = s;
            while(*s != '\0' && *s != sep) ++s;
            return(s - *d);
         }
      }
   }

   return(0);

}



sizea getargp_trim(char **d, char *s, sizea argnum, char sep) {
/* Get-Argument-Pointer-Trim */

   return(__getargpt(d, s, argnum, sep));

}


