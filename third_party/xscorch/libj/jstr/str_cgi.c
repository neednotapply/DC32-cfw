/* $Header: /fridge/cvs/xscorch/libj/jstr/str_cgi.c,v 1.10 2009-04-26 17:39:29 jacob Exp $ */
/*

   libj - str_cgi.c              Copyright (C) 1998-2003 Justin David Smith
   justins (a) chaos2.org        http://chaos2.org/

   Routines to help CGI processing


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



static inline bool __getassignb(char *var, char *val, const char *s, sizea lvar, sizea lval) {
/* internal-Get-Assignment-Countbuffer
   This is a useful function for more than just CGI processing,
   actually. What this will do is it takes the string <s>, which
   is typically an argument out of a CGI string (but can be
   any string, like a line from a config file, f.e.), and it will
   chop off the variable name and the value assign if the string
   is of the form "variable = value". It will also trim off
   whitespace in both the variable name and the value. This
   function returns false if the string <s> is not an assign
   string (e.g., it does not contain a '=' character). The size
   of <var> and <val> can be specified in <lvar> and <lval>,
   respectively. If these values are zero then the <var> and/or
   <val> buffers are assumed to be at least as large as the
   source buffer <s>. Note: if ANY of the buffer pointers are
   NULL then this function will AUTOMATICALLY return false. */

   const char *p = s;
   const char *scanp;
   const_whitespace ws;
   sizea size;

   if(var != NULL && val != NULL && s != NULL) {
      *var = '\0';
      *val = '\0';

      /* Search for the equals sign */
      while(*p != '\0' && *p != '=') ++p;
      if(*p != '\0') {

         /* Get the value. */
         scanp = p + 1;
         SKIM_WHITESPACE(scanp);
         if(*scanp) {
            SET_FIRST_NWS(ws, scanp);
            SET_LAST_NWS(ws, scanp);
            size = NWS_SIZE(ws) + 1;
            if(lval > 0 && size > lval) size = lval;
            STRNCPY(val, ws.fnws, size);
         }

         /* Get the variable name. */
         scanp = s;
         SKIM_WHITESPACE_T(scanp, '=');
         if(scanp != p) {
            SET_FIRST_NWS(ws, scanp);
            SET_LAST_NWS_T(ws, scanp, '=');
            size = NWS_SIZE(ws) + 1;
            if(lvar > 0 && size > lvar) size = lvar;
            STRNCPY(var, ws.fnws, size);
         }
         return(true);

      }
   }

   /* Invalid arguments given */
   return(false);

}


bool getassign(char *var, char *val, const char *s) {
/* Get-Assignment
   This is a useful function for more than just CGI processing,
   actually. What this will do is it takes the string <s>, which
   is typically an argument out of a CGI string (but can be
   any string, like a line from a config file, f.e.), and it will
   chop off the variable name and the value assign if the string
   is of the form "variable = value". It will also trim off
   whitespace in both the variable name and the value. This
   function returns false if the string <s> is not an assign
   string (e.g., it does not contain a '=' character). Each
   target buffer should be at least as large as the source. */

   return(__getassignb(var, val, s, 0, 0));

}


bool getassignbb(char *var, char *val, const char *s, sizea lvar, sizea lval) {
/* Get-Assignment-Countbuffer */

   return(__getassignb(var, val, s, lvar, lval));

}


bool getassignbn(char *var, char *val, const char *s, sizea lvar, sizea lval) {
/* Get-Assignment-Countbuffer */

   return(__getassignb(var, val, s, lvar, lval + 1));

}


bool getassignnb(char *var, char *val, const char *s, sizea lvar, sizea lval) {
/* Get-Assignment-Countbuffer */

   return(__getassignb(var, val, s, lvar + 1, lval));

}


bool getassignnn(char *var, char *val, const char *s, sizea lvar, sizea lval) {
/* Get-Assignment-Countbuffer */

   return(__getassignb(var, val, s, lvar + 1, lval + 1));

}


static inline char *__unescapeCGI(char *s) {
/* internal-Unescape-CGIstring
   This function will unescape characters in a CGI-encoded string.
   Note that since the target string is always smaller than the
   source string we don't have to worry about buffer overflows
   (unless you were stupid enough not to terminate <s> with a
   null character! :) */

   char *d = s;
   char *dstart = d;
   unsigned char num;

   if(s != NULL) {

      /* This part of the function converts CGI "+" to spaces, expands CGI
         escapes, etc etc etc... */
      while(*s != '\0') {
         if(*s == '+') {
            /* Expand the space */
            *d = ' ';
         } else if(*s == '%') {
            /* Expand that escape! yum yum */
            ++s;
            num = 0;
            if (*s >= 'a' && *s <= 'f')      num = (unsigned)*s - 'a' + 10;
            else if (*s >= 'A' && *s <= 'F') num = (unsigned)*s - 'A' + 10;
            else if (*s >= '0' && *s <= '9') num = (unsigned)*s - '0';
            ++s;
            num *= 16;
            if (*s >= 'a' && *s <= 'f')      num += (unsigned)*s - 'a' + 10;
            else if (*s >= 'A' && *s <= 'F') num += (unsigned)*s - 'A' + 10;
            else if (*s >= '0' && *s <= '9') num += (unsigned)*s - '0';
            *d = num;
         } else *d = *s;
         ++s;
         ++d;
      }
      *d = '\0';
      return(dstart);
   }

   return(NULL);

}


char *unescapeCGI(char *s) {
/* Unescape-CGIstring */

   return(__unescapeCGI(s));

}


static inline char *__escapeCGI(char *s) {
/* internal-Escape-CGIstring
   This function will escape characters in a string to their
   safe CGI representation.  Note that the target string must
   be capable of holding 3*strlen(s) characters (also the NULL
   terminator).  */

   sizea len = STRLENN(s);
   char *in = s + 2 * len;
   char *out = s;
   char ch;
   sizea num;

   if(s == NULL) return(NULL);

   /* Move the working copy of the string elsewhere, for now */
   MEMCPY(in, s, len + 1);

   /* Iterate */
   while(*in != '\0') {
      ch = *in;
      if((ch >= 'a' && ch <= 'z') ||
         (ch >= 'A' && ch <= 'Z') ||
         (ch >= '0' && ch <= '9') ||
         (ch == '.' || ch == '/' || ch == '_') || ch == '-') {
         /* Safe character */
         *out = ch;
         ++out;
      } else {
         /* Unsafe character */
         *out = '%';
         num = (unsigned)ch / 16;
         if(num < 10) *(out + 1) = num + '0';
         else         *(out + 1) = num + 'A' - 10;
         num = (unsigned)ch % 16;
         if(num < 10) *(out + 2) = num + '0';
         else         *(out + 2) = num + 'A' - 10;
         out += 3;
      }
      ++in;
   }
   *out = '\0';

   return(s);

}


char *escapeCGI(char *s) {
/* Escape-CGIstring */

   return(__escapeCGI(s));

}


unsigned int getnumargsCGI(const char *s) {
/* Get-Num-Args-CGIstring
   returns the number of arguments in the CGI cmd string. Each
   argument separated by "&". In fact, this code is so similar
   to getnumargs() that we'll just use the getnumargs call :) */

   return(getnumargs(s, '&'));

}


char *getargCGI(char *d, const char *s, unsigned int argnum) {
/* Get-Arg-CGIstring
   Returns the argnum'th argument in the CGI cmd string. This
   code is amazingly similar to getarg so we'll just use the
   getarg code and do modifications later. */

   getarg(d, s, argnum, '&');
   return(__unescapeCGI(d));

}


char *getargCGIb(char *d, const char *s, sizea argnum, sizea maxlen) {
/* Get-Arg-CGIstring-Countbuffer
   Returns the argnum'th argument in the CGI cmd string. This
   code is amazingly similar to getarg so we'll just use the
   getarg code and do modifications later. */

   getargb(d, s, argnum, '&', maxlen);
   return(__unescapeCGI(d));

}


char *getargCGIn(char *d, const char *s, sizea argnum, sizea n) {
/* Get-Arg-CGIstring-Countbuffer
   Returns the argnum'th argument in the CGI cmd string. This
   code is amazingly similar to getarg so we'll just use the
   getarg code and do modifications later. */

   getargn(d, s, argnum, '&', n);
   return(__unescapeCGI(d));

}


