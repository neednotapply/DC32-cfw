/* $Header: /fridge/cvs/xscorch/libj/jstr/str_copy.c,v 1.10 2009-04-26 17:39:30 jacob Exp $ */
/*

   libj - str_copy.c             Copyright (C) 1998-2003 Justin David Smith
   justins (a) chaos2.org        http://chaos2.org/

   Copies one string into another


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



char *strcopyb(char *d, const char *s, sizea n) {
/* StringN-Copy-Null
   Copies a string but appends a null character to the string.
   Be warned, the destination buffer <d> must be at least n
   bytes long for the null append to work! */

   if(d != NULL) {
      if(n <= 0) {
         /* Nothing to do... */
      } else if(s != NULL) {
         STRNCPY(d, s, n);
      } else {
         *d = '\0';
      }
   }
   return(d);

}


char *strcopyn(char *d, const char *s, sizea n) {
/* strcopyn
   Same as above, but we guarantee that n *characters* can be
   copied, so the buffer needs to be at least n+1 large. */

   return(strcopyb(d, s, n + 1));

}


char *strcopynb(char *d, const char *s, sizea n, sizea b) {
/* strcopynb */

   return(strcopyb(d, s, min(n + 1, b)));

}


char *strcopy(char *d, const char *s) {
/* String-Copy-Null
   Copies a string but appends a null character to the string.
   Be warned, the destination buffer <d> must be at least the
   size of the source buffer or else you are seriously fucked */

   if(d != NULL) {
      if(s != NULL) {
         STRCPY(d, s);
      } else {
         *d = '\0';
      }
   }
   return(d);

}


char *strconcatb(char *d, const char *s, sizea b) {
/* StringN-Concat-Null
   Concatenates 2 strings but appends a null character to the string.
   Be warned, the destination buffer <d> must be at least b bytes long
   for the operation to work.  The terminating NULL will always be
   written, so the concatenated part might be truncated. */

   sizea dlen;

   if(d != NULL) {
      dlen = STRLEN(d);
      if(dlen < b) {
         strcopyb(d + dlen, s, b - dlen);
      }
   }
   return(d);

}


char *strconcatn(char *d, const char *s, sizea n) {
/* StringN-Concat-Null
   Concatenates 2 strings by copying the first <n> characters from
   s into d, and appends a null character to the string.  The dest
   buffer must be able to handle n additional characters (the NULL
   need not be counted since the NULL from d will simply shift).  */

   if(d != NULL) {
      strcopyn(d + STRLEN(d), s, n);
   }
   return(d);

}


char *strconcatnb(char *d, const char *s, sizea n, sizea b) {
/* StringN-Concat-Null
   Concatenates 2 strings by copying the first <n> characters from
   s into d, and appends a null character to the string.  The dest
   buffer is <b> bytes long (counting the NULL that is stored in d),
   therefore s might be truncated (we might not be able to copy all
   n characters).  */

   sizea dlen;

   if(d != NULL) {
      dlen = STRLEN(d);
      if(b < dlen) {
         /* Cannot do a thing */
      } else if(b - dlen >= n + 1) {
         /* The buffer can hold all of the requested chars */
         strcopyn(d + dlen, s, n);
      } else if(b - dlen > 0) {
         /* The buffer can only hold some of requested chars */
         strcopyb(d + dlen, s, b - dlen);
      } /* If neither case holds, then we cannot grow */
   }
   return(d);

}


char *strconcat(char *d, const char *s) {
/* String-Concat-Null
   Concatenates 2 strings but appends a null character to the string. */

   if(d != NULL) {
      strcopy(d + STRLEN(d), s);
   }
   return(d);

}
