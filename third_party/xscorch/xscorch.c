/* $Header: /fridge/cvs/xscorch/xscorch.c,v 1.13 2011-08-01 00:01:39 jacob Exp $ */
/*

   xscorch - xscorch.c        Copyright(c) 2000-2004 Justin David Smith
                              Copyright(c) 2000-2004 Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/

   Scorched Earth, main function


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
#define  _POSIX_SOURCE     /* Needed for signal handling */
#define  _GNU_SOURCE       /* Needed for signal handling */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <xscorch.h>
#include <sconfig.h>
#include <sinfo.h>
#include <sland.h>
#include <swindow.h>



#if !HAVE_GETTIMEOFDAY
#error "This program requires gettimeofday()"
#endif



void sc_signal_handler(int sig) {
/* sc_sig_handler
   Extensible signal handler for xscorch. */

   switch(sig) {
      case SIGPIPE:
         /* SIGPIPE will most likely be a dead net cnxn.
            We try to ignore it and sort of hope for the best... */
         fprintf(stderr, "xscorch: Got SIGPIPE; there goes the neighborhood...\n");
         break;
      default:
         ;
   }

}



int main(int argc, char **argv) {

   struct sigaction sa;
   sigset_t sigset;
   sc_config *c;
   sc_window *w;

   /* Signal handlers */
   sigemptyset(&sigset);
   sigaddset(&sigset, SIGPIPE);
   sa.sa_handler = sc_signal_handler;
   sa.sa_mask = sigset;
   sa.sa_flags = 0;
   sigaction(SIGPIPE, &sa, NULL);

   /* Tell them what they've won! */
   sc_info();

   /* Create game configuration, X window.
      Parse command line options, and load
      images.                              */
   if(!(c = sc_config_new(&argc, &argv))) return(1);
   if(!(w = sc_window_new(c, argc, argv))) return(1);

   sc_land_generate(c, c->land);

   sc_window_run(w);

   /* Release all data */
   sc_window_free(&w);
   sc_config_free(&c);

   printf("War is the science of destruction.  -- John Abbott\n");

   return(0);

}
