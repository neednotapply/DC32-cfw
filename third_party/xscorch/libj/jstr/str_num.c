/* $Header: /fridge/cvs/xscorch/libj/jstr/str_num.c,v 1.7 2009-04-26 17:39:30 jacob Exp $ */
/*

   libj - str_num.c              Copyright (C) 1998-2003 Justin David Smith
   justins (a) chaos2.org        http://chaos2.org/

   Number-string conversions


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
#include <libjstr.h>
#include <math.h>



sdword strtoint(const char *s, int *succ) {
/* String-To-Integer
   Converts string <s> to a long integer, if possible. If succ
   is not null, then the return value in <*succ> will be zero
   on success, and on failure it will be a pointer to the
   character which failed PLUS ONE. If <succ> is NULL, then the
   returned value will be zero but there will be no indication
   of the error having occured. */

   /* random thoughts, random code, random bugs, random crashes */
   sdword result = 0;
   int state = 0;
   int possign = -1;    /* +1 for plus, 0 for minus, -1 for default (plus) */

   if(!s) {
      if(succ) *succ = 1;
      return(0);
   }
   if(succ) *succ = 0;

   while(state >= 0) {
      switch(state) {

         /* Initial-state processing
            Accepts '+', '-', '0'..'9', 'x', 'o', 'b'
            -> state 1     if character is '+', '-'
            -> state 2     if character is '1'..'9'
            -> state 3     if character is '0'
            -> state 4     if character is 'x'
            -> state 6     if character is 'o'
            -> state 8     if character is 'b'
            -> state -1    if error */
         case 0:
            state = 1;
            if(*s == '+') possign = 1;
            else if(*s == '-') possign = 0;
            if(possign >= 0) break;
            state = 2;
            if(*s == '0') state = 3;
            else if(*s >= '1' && *s <= '9') result = *s - '0';
            else if(*s == 'x' || *s == 'X') state = 4;
            else if(*s == 'o' || *s == 'O') state = 6;
            else if(*s == 'b' || *s == 'B') state = 8;
            else state = -1;
            break;

         /* Require-decimal-number state
            -> state 2     if character is '0'..'9'
            -> state -1    if error */
         case 1:
            state = 2;
            if(*s >= '0' && *s <= '9') result = (result * 10) + (*s - '0');
            else state = -1;
            break;

         /* Optional-decimal-number state
            -> state 2     if character is '0'..'9'
            -> state -1    if error
            -> state -2    if finished */
         case 2:
            if(*s >= '0' && *s <= '9') result = (result * 10) + (*s - '0');
            else if(!*s) state = -2;
            else state = -1;
            break;

         /* Optional-decimal-or-hex-delimiter state
            -> state 2     if character is '0'..'9'
            -> state 4     if character is 'x'
            -> state -1    if error
            -> state -2    if finished */
         case 3:
            state = 2;
            if(*s >= '0' && *s <= '9') result = (result * 10) + (*s - '0');
            else if(*s == 'x' || *s == 'X') state = 4;
            else if(!*s) state = -2;
            else state = -1;
            break;

         /* Required-hexadecimal-character state
            -> state 5     if character is '0'..'F'
            -> state -1    if error */
         case 4:
            state = 5;
            if(*s >= '0' && *s <= '9') result = (result << 4) | (*s - '0');
            else if(*s >= 'a' && *s <= 'f') result = (result << 4) | (*s - 'a' + 10);
            else if(*s >= 'A' && *s <= 'F') result = (result << 4) | (*s - 'A' + 10);
            else state = -1;
            break;

         /* Optional-hexadecimal-character-state
            -> state 5     if character is '0'..'F'
            -> state -1    if error
            -> state -2    if finished */
         case 5:
            if(*s >= '0' && *s <= '9') result = (result << 4) | (*s - '0');
            else if(*s >= 'a' && *s <= 'f') result = (result << 4) | (*s - 'a' + 10);
            else if(*s >= 'A' && *s <= 'F') result = (result << 4) | (*s - 'A' + 10);
            else if(!*s) state = -2;
            else state = -1;
            break;

         /* Required-octal-character state
            -> state 7     if character is '0'..'7'
            -> state -1    if error */
         case 6:
            state = 7;
            if(*s >= '0' && *s <= '7') result = (result << 3) | (*s - '0');
            else state = -1;
            break;

         /* Optional-octal-character-state
            -> state 7     if character is '0'..'7'
            -> state -1    if error
            -> state -2    if finished */
         case 7:
            if(*s >= '0' && *s <= '7') result = (result << 3) | (*s - '0');
            else if(!*s) state = -2;
            else state = -1;
            break;

         /* Required-binary-character state
            -> state 9     if character is '0'..'1'
            -> state -1    if error */
         case 8:
            state = 9;
            if(*s == '0') result = (result << 1);
            else if(*s == '1') result = (result << 1) | 1;
            else state = -1;
            break;

         /* Optional-binary-character-state
            -> state 9     if character is '0'..'1'
            -> state -1    if error
            -> state -2    if finished */
         case 9:
            if(*s == '0') result = (result << 1);
            else if(*s == '1') result = (result << 1) | 1;
            else if(!*s) state = -2;
            else state = -1;
            break;

      }
      s++;
      if(succ) (*succ)++;
   }

   if(state == -1) return(0);
   if(!possign) result = -result;
   if(succ) *succ = 0;
   return(result);

}


double strtofloat(const char *s, int *succ) {
/* String-To-Float
   Converts string <s> to a double value, if possible. If succ
   is not null, then the return value in <*succ> will be zero
   on success, and on failure it will be a pointer to the
   character which failed PLUS ONE. If <succ> is NULL, then the
   returned value will be zero but there will be no indication
   of the error having occured. */

   /* random thoughts, random code, random bugs, random crashes */
   int state = 0;
   double result = 0;
   long mantissa = 0;
   int posbase = -1;    /* +1 for plus, 0 for minus, -1 for default (plus) */
   int posmant = -1;    /* +1 for plus, 0 for minux, -1 for default (plus) */
   double decval = 0.1; /* power to multiply for decimal digit */

   if(!s) {
      if(succ) *succ = 1;
      return(0);
   }
   if(succ) *succ = 0;

   while(state >= 0) {
      switch(state) {

         /* Initial-state processing
            -> state 1        if character is '+', '-'
            -> state 2        if character is '0'..'9'
            -> state 3        if character is '.'
            -> state -1       on error */
         case 0:
            state = 1;
            if(*s == '+') posbase = 1;
            else if(*s == '-') posbase = 0;
            if(posbase >= 0) break;
            state = 2;
            if(*s >= '0' && *s <= '9') result = (*s - '0');
            else if(*s == '.') state = 3;
            else state = -1;
            break;

         /* Require-whole-number-or-decimal state
            -> state 2        if character is '0'..'9'
            -> state 3        if character is '.'
            -> state -1       if error */
         case 1:
            state = 2;
            if(*s >= '0' && *s <= '9') result = (result * 10) + (*s - '0');
            else if(*s == '.') state = 3;
            else state = -1;
            break;

         /* Optional-whole-number-or-decimal state
            -> state 2        if character is '0'..'9'
            -> state 4        if character is '.'
            -> state 5        if character is 'e'
            -> state -1       if error
            -> state -2       if finished */
         case 2:
            if(*s >= '0' && *s <= '9') result = (result * 10) + (*s - '0');
            else if(*s == '.') state = 4;
            else if(*s == 'e' || *s == 'E') state = 5;
            else if(!*s) state = -2;
            else state = -1;
            break;

         /* Require-decimal-digit state
            -> state 4        if character is '0'..'9'
            -> state -1       if error */
         case 3:
            state = 4;
            if(*s >= '0' && *s <= '9') {
               result = result + decval * (*s - '0');
               decval /= 10;
            } else state = -1;
            break;

         /* Optional-decimal-digit state
            -> state 4        if character is '0'..'9'
            -> state 5        if character is 'e'
            -> state -1       if error
            -> state -2       if finished */
         case 4:
            if(*s >= '0' && *s <= '9') {
               result = result + decval * (*s - '0');
               decval /= 10;
            } else if(*s == 'e' || *s == 'E') state = 5;
            else if(!*s) state = -2;
            else state = -1;
            break;

         /* Exponential-init state
            -> state 6        if character is '+', '-'
            -> state 7        if character is '0'..'9'
            -> state -1       on error */
         case 5:
            state = 6;
            if(*s == '+') posmant = 1;
            else if(*s == '-') posmant = 0;
            if(posmant >= 0) break;
            state = 7;
            if(*s >= '0' && *s <= '9') mantissa = (*s - '0');
            else state = -1;
            break;

         /* Require-exponential-digit state
            -> state 7        if character is '0'..'9'
            -> state -1       on error */
         case 6:
            state = 7;
            if(*s >= '0' && *s <= '9') mantissa = (mantissa * 10) + (*s - '0');
            else state = -1;
            break;

         /* Optional-exponential-digit state
            -> state 7        if character is '0'..'9'
            -> state -1       if error
            -> state -2       if finished */
         case 7:
            if(*s >= '0' && *s <= '9') mantissa = (mantissa * 10) + (*s - '0');
            else if(!*s) state = -2;
            else state = -1;
            break;

      }
      s++;
      if(succ) (*succ)++;
   }

   if(state == -1) return(0);
   if(!posbase) result = -result;
   if(!posmant) mantissa = -mantissa;
   result = pow(result, mantissa);
   if(succ) *succ = 0;
   return(result);

}


char *inttostr(char *d, sdword num, udword digits) {
/* Integer-To_String
   Converts integer <num> to a string with <digits> digits. If
   <digits> is less than number of digits in <num>, then the
   _least_ significant digits are copied. If digits is larger,
   then leading zeroes will be copied. If <digits> = 0, then the
   number will simply be copied without leading zeroes. Note for
   negative numbers: the "-" sign is considered to take up a digit
   space, so be warned... The dest buffer should be at least
   <digits> + 1 characters long. */

   char *dstart = d;
   udword mag = 1;      /* What magnitude are we on right now? */

   if(!d) return(NULL);

   /* Handling for negative numbers */
   if (num < 0) {
      *d = '-';
      d++;
      num = -num;
      digits--;
      if(!digits) {
         *d = '\0';
         return(dstart);
      }
   }

   /* Figure out how many digits to bother with */
   if(digits > 0) {

      /* Print specified number of digits */
      while(--digits) mag *= 10;
   } else {

      /* Print all digits */
      while (num / (mag * 10) > 0) mag *= 10;
   }

   /* Start copying off the digits :) */
   while (mag) {
      *d = (num / mag) % 10 + '0';
      d++;
      num %= mag;
      mag /= 10;
   }

   /* Set ending NULL on the destination string (kinda important..) */
   *d = '\0';
   return(dstart);

}


char *inttohex(char *d, udword num, udword digits) {
/* Integer-To_Hexadecimal
   Converts integer <num> to a string with <digits> digits. If
   <digits> is less than number of digits in <num>, then the
   _least_ significant digits are copied. If digits is larger,
   then leading zeroes will be copied. If <digits> = 0, then the
   number will simply be copied without leading zeroes. Note for
   negative numbers: the "-" sign is considered to take up a digit
   space, so be warned... The dest buffer should be at least
   <digits> + 1 characters long. */

   char *dstart = d;
   unsigned int mag = 0;      /* What magnitude are we on right now? */

   if(!d) return(NULL);

   /* Figure out how many digits to bother with */
   if(digits > 0) {

      /* Print specified number of digits */
      while(--digits) mag += 4;
   } else {

      /* Print all digits */
      while ((num >> mag) > 0) mag += 4;
   }

   /* Start copying off the digits :) */
   while (mag) {
      *d = (num >> mag) & 0xf;
      #ifdef PREFER_UPPER
      if(*d > 9) *d += 'A';
      #else
      if(*d > 9) *d += 'a';
      #endif
      else *d += '0';
      d++;
      mag -= 4;
   }

   /* Set ending NULL on the destination string (kinda important..) */
   *d = '\0';
   return(dstart);

}


bool getbool(bool *result, const char *s) {
/* Get-Boolean
   Returns "true" if the value in <s> is a boolean identifier string, in
   which case, <result> is set to true or false based on <s>. If <result>
   is NULL, then this function just returns whether or not <s> is a valid
   boolean identifier. Valid identifiers are...
      true: "true" "True" "TRUE" "t" "T" "yes" "Yes" "YES" "y" "Y" "+"
      false: "false" "False" "FALSE" "f" "F" "no" "No" "NO" "n" "N" "-" */

   if( streql(s, "true") || streql(s, "True") || streql(s, "TRUE")
    || streql(s, "t") || streql(s, "T")
    || streql(s, "yes") || streql(s, "Yes") || streql(s, "YES")
    || streql(s, "y") || streql(s, "Y")
    || streql(s, "+")) {
      if(result) *result = true;
   } else if(streql(s, "false") || streql(s, "False") || streql(s, "FALSE")
    || streql(s, "f") || streql(s, "F")
    || streql(s, "no") || streql(s, "No") || streql(s, "NO")
    || streql(s, "n") || streql(s, "N")
    || streql(s, "-")) {
      if(result) *result = false;
   } else return(false);
   return(true);

}
