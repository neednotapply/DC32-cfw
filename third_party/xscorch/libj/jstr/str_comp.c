/* $Header: /fridge/cvs/xscorch/libj/jstr/str_comp.c,v 1.16 2011-08-01 00:01:39 jacob Exp $ */
/*

   libj - str_comp.c             Copyright (C) 1998-2003 Justin David Smith
                                 Copyright (C) 2003 Jacob Luna Lundberg
   justins (a) chaos2.org        http://chaos2.org/
   jacob (a) chaos2.org          http://www.gnifty.net/

   String Comparison functions


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



int strcomp(const char *s, const char *d) {
/* String-Compare-Nullsafe
   This is a null-pointer-safe version of strcmp. This will return 0
   if both pointers are NULL.  This function considers a NULL pointer
   to be "less than" any valid string, including the empty string.  */

   int result;

   /* Handle trivial equality and NULL cases */
   if(s == d) return(0);
   if(s == NULL) return(-1);
   if(d == NULL) return(+1);

   /* At this point, neither s nor d are NULL */
   result = STRCOMP(s, d);
   if(result == 0) return(0);
   if(result < 0) return(-1);
   return(+1);

}



int strcompn(const char *s, const char *d, sizea n) {
/* String-N-Compare-Nullsafe
   This is a null-pointer-safe version of strncmp. This will return 0
   if both pointers are NULL.  This function considers a NULL pointer
   to be "less than" any valid string, including the empty string.  */

   int result;

   /* Handle trivial equality and NULL cases */
   if(s == d) return(0);
   if(s == NULL) return(-1);
   if(d == NULL) return(+1);

   /* At this point, neither s nor d are NULL */
   result = STRNCOMP(s, d, n);
   if(result == 0) return(0);
   if(result < 0) return(-1);
   return(+1);

}



bool strequal(const char *s, const char *d) {
/* String-N-Equal */

   if(s == d) return(true);
   if(s == NULL || d == NULL) return(false);
   return(STREQL(s, d));

}



bool strequaln(const char *s, const char *d, sizea n) {
/* String-N-Equal */

   if(s == d) return(true);
   if(s == NULL || d == NULL) return(false);
   return(STRNEQL(s, d, n));

}



char *rkstrpat(rkstate *rk, char *s, const char *d) {
/* Rabin-Karp-String-Pattern
   Searches for substring <d> in source string <s> using the Rabin Karp
   algorithm. */

   unsigned char *sp = (unsigned char *)s;
   const unsigned char *dp = (const unsigned char *)d;
   unsigned char *rk_source;
   const unsigned char *rk_pattern;
   sizea  rk_i;
   sdword rk_t;
   sdword rk_p;
   sdword rk_h;
   sizea  rk_slen;
   sizea  rk_dlen;

   if(s != NULL) {
      rk_p = 0;
      rk_t = 0;
      rk_h = 1;
      rk_i = 0;
      rk_source = (unsigned char *)s;
      rk_pattern = (const unsigned char *)d;
      if(d != NULL && *d != '\0') {

         /* Scan the strings */
         while(*sp != '\0' && *dp != '\0') {
            rk_t = (rk_t << 8) + *sp;
            if(rk_t >= LIBJ_RK_PRIME) rk_t %= LIBJ_RK_PRIME;
            rk_p = (rk_p << 8) + *dp;
            if(rk_p >= LIBJ_RK_PRIME) rk_p %= LIBJ_RK_PRIME;
            ++sp;
            ++dp;
         }
         if(*sp != '\0' || *dp == '\0') {

            /* Calculate the lengths of the strings */
            while(*sp != '\0') ++sp;
            rk_slen = (sp - (unsigned char *)s);
            rk_dlen = (dp - (const unsigned char *)d);

            /* Precalc the infamous "h" value.  */
            if(rk_dlen != 0) {
               rk_i = rk_dlen - 1;
               while(rk_i != 0) {
                  rk_h = (rk_h << 8);
                  if(rk_h >= LIBJ_RK_PRIME) rk_h %= LIBJ_RK_PRIME;
                  --rk_i;
               }
            }

            /* Pattern matching loop */
            rk_i = (rk_slen - rk_dlen) + 1;
            rk_source = (unsigned char *)s;
            while(rk_i) {
               if(rk_p == rk_t) {
                  if(MEMEQL(rk_source, rk_pattern, rk_dlen)) {
                     if(rk != NULL) {
                        rk->rk_i = rk_i;
                        rk->rk_t = rk_t;
                        rk->rk_p = rk_p;
                        rk->rk_h = rk_h;
                        rk->rk_source  = (char *)rk_source;
                        rk->rk_pattern = (const char *)rk_pattern;
                        rk->rk_dlen = rk_dlen;
                     }
                     return((char *)rk_source);
                  }
               }
               --rk_i;
               if(rk_i != 0) {
                  rk_t = (((rk_t - (((unsigned char)*rk_source) * rk_h)) << 8)
                       + ((unsigned char)*(rk_source + rk_dlen))) % LIBJ_RK_PRIME;
                  if(rk_t < 0) rk_t += LIBJ_RK_PRIME;
               }
               ++rk_source;
            }
         } /* Did we reach end of source before end of pattern? */
      } else {
         /* destination buffer started out empty */
         if(rk != NULL) {
            rk->rk_source  = (char *)rk_source;
            rk->rk_pattern = (const char *)rk_pattern;
         }
         return((char *)rk_source);
      }

   }

   if(rk != NULL) rk->rk_source = NULL;
   return(NULL);

}



char *rkstrnext(rkstate *rk) {
/* Rabin-Karp-String-Next
   Searches for the next substring <d> in source string <s> using the
   Rabin Karp algorithm. Returns NULL if no more entries were found, or
   if there was no successful call to rkstrpat() to initiate the search. */

   unsigned char *rk_source;
   sizea  rk_i;
   sdword rk_t;
   sdword rk_p;
   sdword rk_h;
   sizea  rk_dlen;
   bool   need_to_skip;

   if(rk != NULL && rk->rk_source != NULL) {

      /* We need to update the calculations to reflect the first shift.
         As a result, we set rk_source equal to the location of the
         last match; we'll explicitly ignore it further down... */
      rk_source  = (unsigned char *)rk->rk_source;
      need_to_skip = true;
      if(*rk_source != '\0') {
         if(rk->rk_pattern != NULL && *rk->rk_pattern != '\0') {

            /* Pattern matching loop */
            rk_i = rk->rk_i;
            rk_t = rk->rk_t;
            rk_p = rk->rk_p;
            rk_h = rk->rk_h;
            rk_dlen = rk->rk_dlen;
            while(rk_i != 0) {
               if(rk_p == rk_t) {
                  if(!need_to_skip && MEMEQL(rk_source, rk->rk_pattern, rk_dlen)) {
                     rk->rk_i = rk_i;
                     rk->rk_t = rk_t;
                     rk->rk_source = (char *)rk_source;
                     return((char *)rk_source);
                  }
               }
               --rk_i;
               if(rk_i != 0) {
                  rk_t = (((rk_t - (((unsigned char)*rk_source) * rk_h)) << 8)
                       + ((unsigned char)*(rk_source + rk_dlen))) % LIBJ_RK_PRIME;
                  if(rk_t < 0) rk_t += LIBJ_RK_PRIME;
               }
               ++rk_source;
               need_to_skip = false;
            }

         } else return(rk->rk_source);

      } /* Source is not an empty buffer? */

      /* Assign source to NULL (we've reached end of search) */
      rk->rk_source = NULL;

   } /* rk was not null */

   return(NULL);

}



char *kmpstrpat(char *s, const char *d) {
/* Kmith-Morris-Pratt-String-Pattern
   A different algorithm from rabin karp. Best suited for algorithms with
   longer patterns with a relatively small alphabet. Not a guaranteed
   one-pass algorithm like rabin-karp. Similar interface as in rabin-karp
   rkstrpat() function. */

   sizea dlen = 0;
   sizea *pi = NULL;
   sizea *pip;
   sizea k = 0;
   char *sp = s;
   char *dp = (char *)d;

   if(d == NULL) return(s);

   if(s != NULL && d != NULL) {

      /* Get the pattern string length */
      while(*dp != '\0') ++dp;
      dlen = dp - d;

      /* Allocate the data buffer for pi - calculate pi values */
      if(dlen != 0) {
         pi = (sizea *)malloc(sizeof(sizea) * dlen);
         if(pi != NULL) {
            pip = pi + 1;
            *pi = 0;
            dp = (char *)(d + 1);
            while(*dp != '\0') {
               while(k && d[k] != *dp) k = pi[k];
               if(d[k] == *dp) ++k;
               *pip = k;
               ++dp;
               ++pip;
            }
         } else dlen = 0;
      }

      /* Now, search! Search, my friend, search! */
      k = 0;
      while(*sp != '\0') {
         while(k && d[k] != *sp) k = pi[k];
         if(d[k] == *sp) ++k;
         if(k == dlen) {
            /* Match found! Yay... */
            free(pi);
            return(sp - dlen + 1);
         }
         ++sp;
      }
      free(pi);

   } /* Source, dest weren't NULL */

   return(NULL);

}



char *strscan(char *s, const char *pat) {
/* strscan
   Scans the string s for first occurrence of pat.
   s must be NULL-terminated.  */

   return(rkstrpat(NULL, s, pat));

}



char *strscan_list(char *s, const char *const*patlist, sizea *index) {
/* strscan_list
   Searches the string s for the first occurence of any string in the
   pattern list given (where pattern-list is terminated by a NULL entry).
   On success, *index is set to the matching pattern and the location in
   the string is returned.  This is currently using the naive algorithm
   which applies rabin-karp on every pattern and takes the smallest res. */

   sizea best_index = (sizea)-1;
   char *best_p = NULL;
   const char *const*patlistp;
   char *p;

   if(s == NULL || patlist == NULL) return(NULL);

   /* Start scanning each pattern */
   patlistp = patlist;
   while(*patlistp != NULL && **patlistp != '\0') {
      p = strscan(s, *patlistp);
      if(p != NULL && (best_p == NULL || p < best_p)) {
         best_p = p;
         best_index = patlistp - patlist;
      }
      ++patlistp;
   }

   /* Return best result */
   if(index != NULL) *index = best_index;
   return(best_p);

}



bool strsimilar(const char *A, const char *B) {
/* strsimilar
   Sloppy string comparison.
   Basically, make sure the strings are the same, allowing for case and
   punctuation.  The return type is bool.  This function is courtesy of
   Jacob Lundberg.  */

   if(A == B) return(true);
   if(A == NULL || B == NULL) return(false);

   while(*A != '\0' && *B != '\0') {
      /* next valid A char */
      while(!( (*A == '\0') || (*A >= '0' && *A <= '9') ||
               (*A >= 'A' && *A <= 'Z') || (*A >= 'a' && *A <= 'z') ))
         ++A;
      /* next valid B char */
      while(!( (*B == '\0') || (*B >= '0' && *B <= '9') ||
               (*B >= 'A' && *B <= 'Z') || (*B >= 'a' && *B <= 'z') ))
         ++B;
      /* test for disparate strings at this char pair */
      if( ((*A >= 'a' && *A <= 'z') ? (*A - ('a' - 'A')) : *A) !=
          ((*B >= 'a' && *B <= 'z') ? (*B - ('a' - 'A')) : *B) )
         break;
      ++A;
      ++B;
   }

   /* If both made it to the end then they're ``equal'' enough for us. */
   return(*A == '\0' && *B == '\0');

}
