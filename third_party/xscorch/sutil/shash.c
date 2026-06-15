/* $Header: /fridge/cvs/xscorch/sutil/shash.c,v 1.14 2011-08-01 00:01:44 jacob Exp $ */
/*

   xscorch - shash.c          Copyright(c) 2001-2003 Jacob Luna Lundberg
   jacob(at)gnifty.net        http://www.gnifty.net/

   A very slim hashing implementation for xscorch.


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
#include <math.h>
#include <stdlib.h>

#include <xscorch.h>

#include <shash.h>
#include <md5.h>

#include <libj/jstr/libjstr.h>



/* This is (sqrt(5) - 1), used below. */
#define  SQRT5M1  1.23606797749978980505



int shash_string_to_int(const char *item) {
/* shash_string_to_int
   Convert a string to an int suitable for hashing (using md5). */

   int quad[4];

   /* Take the MD5 sum of the string. */
   md5_buffer(item, strlenn(item), quad);

   /* Compress the MD5 sum to a single integer. */
   return(quad[0] ^ quad[1] ^ quad[2] ^ quad[3]);

}



int shash(int bits, unsigned int item) {
/* shash
   Hash item to a list of 2^bits elements. */

   /* rationale -
      The below is a basic multiplication hash.
      (1 << (8 * sizeof(int) - 1)) should be 2^(number of bits in an int) / 2
      We divide by 2 there to avoid overflow and because the magic number
      we are using is actually (sqrt(5) - 1) / 2 so all is well. */

   /* Casting to double here is to remind us we must use high precision.
      Which is too bad because it makes for a much slower function... ;) */
   item *= (double)((double)(1 << (8 * sizeof(int) - 1)) * (double)SQRT5M1);
   bits = item >> (8 * sizeof(int) - bits);
   return(bits);

}



int shash_string(int bits, const char *item) {
/* shash_string
   Hash a string through the multiplicative hash using MD5. */

   return(shash(bits, shash_string_to_int(item)));

}
