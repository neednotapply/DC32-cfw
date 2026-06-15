/* $Header: /fridge/cvs/xscorch/libj/jstr/str_uni.c,v 1.6 2009-04-26 17:39:31 jacob Exp $ */
/*

   libj - str_uni.c              Copyright (C) 1998-2003 Justin David Smith
   justins (a) chaos2.org        http://chaos2.org/

   Unicode conversions


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



char *unitocharc(char *d, const wchar *s, unsigned int maxlen) {
/* Unicode-to-Character-Countbuffer
   Converts a source unicode string into a character string by stripping
   off the high bits of the source string. The target buffer should be
   at least wide_strlen(source) + 1 bytes long if <maxlen> = 0, otherwise
   <maxlen> will specify the size of the target buffer. */

   const unsigned char *ps = (const unsigned char *)s;
   unsigned char *pd = (unsigned char *)d;

   if(!s || !d) return(NULL);

   /* Convert a unicode string to char, stripping the most significant bytes */
   while(*ps && --maxlen) {
      *pd = *ps;
      ps += 2;
      pd++;
   }
   *pd = '\0';
   return(d);

}



char *unitochar(char *d, const wchar *s) {
/* Unicode-to-Character-Countbuffer
   Converts a source unicode string into a character string by stripping
   off the high bits of the source string. The target buffer should be
   at least wide_strlen(source) + 1 bytes long. */

   return(unitocharc(d, s, 0));

}



wchar *chartounic(wchar *d, const char *s, unsigned int maxlen) {
/* Character-to-Unicode-Countbuffer
   Converts a source character string into a unicode string by appending
   a leading 0 byte to each byte in the source string. The target buffer
   should be at least strlen(source) * 2 + 2 bytes long if <maxlen> = 0,
   otherwise <maxlen> will specify the size of the target buffer IN
   2-BYTE INCREMENTS! */

   const unsigned char *ps = (const unsigned char *)s;
   unsigned char *pd = (unsigned char *)d;

   if(!s || !d) return(NULL);

   /* Convert a char string to ASCII characters in unicode */
   while(*ps && --maxlen) {
      *pd = *ps;
      *(pd + 1) = '\0';
      ps++;
      pd += 2;
   }
   *pd = '\0';
   *(pd + 1) = '\0';
   return(d);

}



wchar *chartouni(wchar *d, const char *s) {
/* Character-to-Unicode
   Converts a source character string into a unicode string by appending
   a leading 0 byte to each byte in the source string. The target buffer
   should be at least strlen(source) * 2 + 2 bytes long. */

   return(chartounic(d, s, 0));

}



