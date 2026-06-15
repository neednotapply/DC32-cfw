/* $Header: /fridge/cvs/xscorch/sutil/sgetline.h,v 1.4 2009-04-26 17:40:01 jacob Exp $ */
/*

   xscorch - sgetline.h       Copyright(c) 2003-2004 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Drop-in replacement for fgets() for functions previously reading
   data from files, which are now reading from embedded buffers.


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, version 2 of the License ONLY.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

*/
#ifndef __sgetline_h_included
#define __sgetline_h_included


#include <assert.h>


static inline char *sgetline(char *buf, int size, const char *inbuf, int *count) {
   int local_count;
   char *bufp = buf;

   assert(buf != NULL);
   assert(size > 0);
   assert(count != NULL);

   local_count = *count;

   if(inbuf[local_count] == '\0')
      return(NULL);

   while(size > 1 && inbuf[local_count] != '\0' && inbuf[local_count] != '\n') {
      *bufp = inbuf[local_count];
      ++local_count;
      ++bufp;
      --size;
   }

   if(size > 1 && inbuf[local_count] == '\n') {
      *bufp = '\n';
      ++bufp;
      ++local_count;
   }

   *bufp = '\0';
   *count = local_count;
   return(buf);
}


#endif // __sgetline_h_included
