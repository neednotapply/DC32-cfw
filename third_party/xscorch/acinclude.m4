dnl acinclude.m4
dnl Support functions for configure.in.
dnl
dnl These programs are free software; you can redistribute them and/or
dnl modify them under the terms of the GNU General Public License as
dnl published by the Free Software Foundation; either version 2 of the
dnl License, or (at your option) any later version.
dnl
dnl These programs are distributed in the hope that they will be
dnl useful, but WITHOUT ANY WARRANTY; without even the implied warranty
dnl of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with these programs; if not, write to the Free Software Foundation,
dnl Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
dnl

dnl FC_CHECK_READLINE_RUNTIME(EXTRA-LIBS, ACTION-IF-FOUND)
dnl
dnl This tests whether readline works at runtime.  Here, "works"
dnl means "doesn't dump core", as some versions do if linked
dnl against wrong ncurses library.  Compiles with LIBS modified
dnl to include -lreadline and parameter EXTRA-LIBS.
dnl Should already have checked that header and library exist.
dnl
dnl If readline "works", or if cross-compiling, defines
dnl READLINE=1 and runs ACTION-IF-FOUND.  Otherwise prints
dnl a warning (and leaves READLINE untouched).
dnl
AC_DEFUN([FC_CHECK_READLINE_RUNTIME],
[AC_MSG_CHECKING(whether readline works at runtime)
templibs="$LIBS"
LIBS="-lreadline $1 $LIBS"
AC_TRY_RUN([
/*
 * testrl.c
 * File revision 0
 * Check to make sure that readline works at runtime.
 * (Specifically, some readline packages link against a wrong
 * version of ncurses library and dump core at runtime.)
 * (c) 2000 Jacob Lundberg, jacob(at)gnifty.net
 */

#include <stdio.h>
/* We assume that the presence of readline has already been verified. */
#include <readline/readline.h>
#include <readline/history.h>

/* Setup for readline. */
#define TMP_FILE "./conftest.readline.runtime"

static void handle_readline_input_callback(char *line) {
/* Generally taken from freeciv-1.11.4/server/sernet.c. */
   if(line) {
      if(*line)
         add_history(line);
      /* printf(line); */
   }
}
int main(void) {
/* Try to init readline and see if it barfs. */
   using_history();
   read_history(TMP_FILE);
   rl_initialize();
   rl_callback_handler_install("_ ", handle_readline_input_callback);
   rl_callback_handler_remove();  /* needed to re-set terminal */
   return(0);
}
],
[AC_MSG_RESULT(yes)
   READLINE="1"
   [$2]],
[AC_MSG_RESULT(no)
   AC_MSG_WARN(Readline fails at runtime and will not be used.)
   AC_MSG_WARN(You probably need to get a newer readline package.)],
[AC_MSG_RESULT(unknown: cross-compiling)
   READLINE="1"
   [$2]])
LIBS="$templibs"
])
