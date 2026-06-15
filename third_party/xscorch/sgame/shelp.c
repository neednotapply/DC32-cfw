/* $Header: /fridge/cvs/xscorch/sgame/shelp.c,v 1.12 2009-04-26 17:39:40 jacob Exp $ */
/*

   xscorch - shelp.c          Copyright(c) 2001-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched simple help system


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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <sgetline.h>
#include <shelp.h>
#include <shelpdata.h>

#include <libj/jstr/libjstr.h>



static void _unmunge_line(char *buf) {

   char *d = buf;
   char *s = buf;
   while(*s != '\0') {
      if(strequaln(s, "\\-", 2)) {
         *d = '-';
         ++d;
         ++s;
      } else if(strequaln(s, "\\f", 2)) {
         s += 2;
      } else {
         *d = *s;
         ++d;
      }
      ++s;
   }
   *d = '\0';

}



static void _trim_line(char *buf) {

   bool countedspace = true;
   char *d = buf;
   char *s = buf;
   while(*s != '\0') {
      if(*s == ' ' || *s == '\t' || *s == '\n') {
         if(!countedspace) {
            countedspace = true;
            *d = ' ';
            ++d;
         }
      } else {
         countedspace = false;
         *d = *s;
         ++d;
      }
      ++s;
   }
   if(countedspace && d > buf) --d;
   *d = '\0';

}



void sc_help_text(char *res, int size, const char *idphrase) {
/* sc_help_text */

   char buf[0x1000];
   char key[0x1000];
   int keysize;
   bool match;
   bool done;
   int offset = 0;
 
   /* Sanity checks */
   if(res == NULL || size <= 0 || idphrase == NULL) return;

   /* Prep and size the ID string */
   sbprintf(key, sizeof(key), "\\fB%s\\fP", idphrase);
   keysize = strblenn(key, sizeof(key));

   /* Scan help file for the requested text */
   match = false;
   while(!match && sgetline(buf, sizeof(buf), data_xscorch_man, &offset) != NULL) {
      match = strequaln(buf, key, keysize);
   }

   /* Make sure we found something */
   if(!match) {
      sbprintf(res, size, "Cannot find help text for %s.", idphrase);
      return;
   }

   /* Start writing data to the result buffer until we reach a blank line */
   done = false;
   strcopyb(res, idphrase, size);
   strconcatb(res, ": ", size);
   while(!done && sgetline(buf, sizeof(buf), data_xscorch_man, &offset) != NULL) {
      if(*buf == '.' && !strequaln(buf, ".\\\"", 3)) done = true;
      else {
         /* Copy this line (unmunged) into the help buffer */
         strconcatb(res, buf, size);
      }
   }

   /* We are done! */
   _unmunge_line(res);
   _trim_line(res);

}

