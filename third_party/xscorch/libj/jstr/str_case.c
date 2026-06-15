/* $Header: /fridge/cvs/xscorch/libj/jstr/str_case.c,v 1.8 2009-04-26 17:39:29 jacob Exp $ */
/*

   libj - str_case.c             Copyright (C) 1998-2003 Justin David Smith
   justins (a) chaos2.org        http://chaos2.org/

   Case sensitive changes


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
#include <_str.h>



char *unescape(char *s) {
/* Unescape
   This function will take an escaped string and "unescape" it. Standard C
   escape sequence is read. The escape sequences currently recognized are
      \a    bell
      \b    backspace
      \f    formfeed
      \n    newline
      \r    carriage return
      \t    tab
      \v    vertical tab
      \'    single quote
      \"    double quote
      \?    question mark
      \\    backslash
      \0    null
      \ooo  ascii character
      \xhh  ascii character
   If the escape sequence is not valid then the next character after the
   escape will be printed (but not the escape itself). */

   char *d = s;      /* Dest pointer */
   char *dstart = s; /* Original pointer */
   int num;          /* Dummy numerical value */

   if(s != NULL) {
      while(*s != '\0') {
         if(*s == '\\') {
            ++s;

            /* Escape character found */
            switch(*s) {

               /* If null, then copy a null and back up s pointer */
               case '\0':
                  *d = '\0';
                  s--;
                  break;

               /* If return, then remove the return */
               case '\n':
                  d--;
                  break;

               /* Numbers  */
               case '0':
               case '1':
               case '2':
               case '3':
               case '4':
               case '5':
               case '6':
               case '7':
                  num = *s - '0';
                  ++s;
                  if(*s >= '0' && *s <= '7') {
                     num = (num << 3) | (*s - '0');
                     ++s;
                     if(*s >= '0' && *s <= '3') num = (num << 3) | (*s - '0');
                     else s--;
                  } else s--;
                  *d = (unsigned char)num;
                  break;

               /* General escapes */
               case 'a':
                  *d = '\a';
                  break;
               case 'b':
                  *d = '\b';
                  break;
               case 'f':
                  *d = '\f';
                  break;
               case 'n':
                  *d = '\n';
                  break;
               case 'r':
                  *d = '\r';
                  break;
               case 't':
                  *d = '\t';
                  break;
               case 'v':
                  *d = '\v';
                  break;

               /* Hexadecimal numbers case */
               case 'x':
                  ++s;
                  if(*s >= '0' && *s <= '9') {
                     num = *s - '0';
                     ++s;
                     if(*s >= '0' && *s <= '9') num = (num << 4) | (*s - '0');
                     if(*s >= 'a' && *s <= 'f') num = (num << 4) | (*s - 'a' + 10);
                     if(*s >= 'A' && *s <= 'F') num = (num << 4) | (*s - 'A' + 10);
                     else s--;
                     *d = (unsigned char)num;
                  } else if(*s >= 'a' && *s <= 'f') {
                     num = *s - 'a' + 10;
                     ++s;
                     if(*s >= '0' && *s <= '9') num = (num << 4) | (*s - '0');
                     if(*s >= 'a' && *s <= 'f') num = (num << 4) | (*s - 'a' + 10);
                     if(*s >= 'A' && *s <= 'F') num = (num << 4) | (*s - 'A' + 10);
                     else s--;
                     *d = (unsigned char)num;
                  } else if(*s >= 'A' && *s <= 'F') {
                     num = *s - 'A' + 10;
                     ++s;
                     if(*s >= '0' && *s <= '9') num = (num << 4) | (*s - '0');
                     if(*s >= 'a' && *s <= 'f') num = (num << 4) | (*s - 'a' + 10);
                     if(*s >= 'A' && *s <= 'F') num = (num << 4) | (*s - 'A' + 10);
                     else s--;
                     *d = (unsigned char)num;
                  } else {
                     d--;
                     s--;
                  }
                  break;

               /* This will catch everything else */
               default:
                  *d = *s;
                  break;
            }
            ++s;
         } else {
            /* Normal character */
            *d = *s;
            ++s;
         }
         ++d;
      }
      *d = '\0';
      return(dstart);
   } /* source wasn't NULL */
   return(NULL);

}


char *unescape_quoted(char *s) {
/* Unescape-Quoted
   Same as the above function, but unescaped double quotes are also
   stripped from the input string first. */

   char *p = s;
   char *d = s;
   if(s != NULL) {
      while(*p != '\0') {
         if(*p == '\"') ++p;
         else *(d++) = *(p++);
      }
      *d = '\0';
      unescape(s);
   } /* Source wasn't null */
   return(s);

}


char *escapeb(char *str, sizea size) {
/* Escape
   Escapes potentially troublesome characters in the string.  The
   size indicates the maximum size of the buffer; trailing characters
   will be truncated if necessary.  To guarantee the entire string is
   left intact, allocate a buffer twice as large as the string.  */

   char *buf;
   char *src;
   char *dest;

   if(str == NULL || size <= 0) return(NULL);

   buf = (char *)malloc(size + size + 2);
   if(buf == NULL) return(NULL);

   src = str;
   dest = buf;
   while(*src != '\0') {
      if(*src == '\n') {
         *dest++ = '\\';
         *dest++ = 'n';
      } else if(*src == '\r') {
         *dest++ = '\\';
         *dest++ = 'r';
      } else if(*src == '\t') {
         *dest++ = '\\';
         *dest++ = 't';
      } else if(*src == '\\' || *src == '\"') {
         *dest++ = '\\';
         *dest++ = *src;
      } else {
         *dest++ = *src;
      }
      ++src;
   }

   *dest = '\0';
   MEMCPY(str, buf, size);
   str[size - 1] = '\0';
   free(buf);
   return(str);

}


char *escapen(char *str, sizea n) {
/* Escape
   Same as above function, but n is the maximum number of characters to
   put in the buffer (not including a terminating NULL). */

   return(escapeb(str, n + 1));

}


char *escaped_scan(char *src, char ch) {
/* Escaped-Scan
   Scans for occurrence of ch in the string.  Returns NULL if the character
   is not found, otherwise the first occurence is returned.  This function
   will ignore characters inside a quoted block, as well as escaped
   characters.  */

   char *p;
   int instring;

   if(src == NULL) return(NULL);

   p = src;
   instring = 0;
   while(*p != '\0') {
      if(*p == '\"') {
         instring = !instring;
      } else if(*p == '\\' && *(p + 1) != '\0') {
         ++p;
      } else if(!instring && *p == ch) return(p);
      ++p;
   }

   return(NULL);

}


char *escaped_chop(char *src, char ch) {
/* Escaped-Chop
   Chops off the string after the first occurrence of ch.  */

   char *p = escaped_scan(src, ch);
   if(p != NULL) *p = '\0';
   return(src);

}


char *strlower(char *s) {
/* String-Lowercase
   converts string <s> to lowercase characters */

   char *dstart = s;
   if(s != NULL) {
      while(*s != '\0') {
         if (UPPER(*s)) *s += 0x20;
         s++;
      }
      return(dstart);
   } /* Source wasn't null */
   return(NULL);

}


char *strupper(char *s) {
/* String-Uppercase
   converts string <s> to uppercase characters */

   char *dstart = s;
   if(s != NULL) {
      while(*s != '\0') {
         if (LOWER(*s)) *s -= 0x20;
         s++;
      }
      return(dstart);
   } /* Source wasn't null */
   return(NULL);

}


char *strproper(char *s) {
/* String-Proper
   converts string <s> to propercase character */

   char *d = s;
   bool whitespace = true;

   if(d != NULL) {
      while(*d != '\0') {
         if(WHITESPACE(*d)) whitespace = true;
         else if(whitespace) {
            if(LOWER(*d)) *d -= 0x20;
            whitespace = false;
         } else if(UPPER(*d)) *d += 0x20;
         d++;
      }
      return(s);
   } /* Destination wasn't null */
   return(NULL);

}
