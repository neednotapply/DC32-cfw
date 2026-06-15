dnl $Header: /fridge/cvs/xscorch/configure.ac,v 1.18 2011-07-31 23:44:42 jacob Exp $
dnl
dnl
dnl Process this file with autoconf to produce a configure script.
dnl Copyright(c) 2001-2004 Justin David Smith
dnl Copyright(c) 2005-2011 Jacob Luna Lundberg
dnl
AC_PREREQ(2.50)
AC_INIT(xscorch.c)
AM_INIT_AUTOMAKE(xscorch, 0.2.1)
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_CONFIG_MACRO_DIR([m4])

dnl Watch out for problems with prefix, datadir
dnl
test "x$prefix" = "xNONE" && prefix="$ac_default_prefix"
test "x$datarootdir" = 'x${prefix}/share' && datarootdir="${prefix}/share"
test "x$datarootdir" = 'x${datarootdir}' && datarootdir="${prefix}/share"
dnl
test "x$datadir" = 'x${prefix}/share' && datadir="${prefix}/share"
test "x$datadir" = 'x${datarootdir}' && datadir="${prefix}/share"
test "x$mandir" = 'x${prefix}/man' && mandir="${prefix}/man"
test "x$mandir" = 'x${datarootdir}/man' && mandir="${prefix}/man"


dnl Checks for basic programs.
dnl
LIBJ_CONF_EMBEDDED
AM_PROG_CC_C_O
AC_PROG_INSTALL
AM_PROG_LIBTOOL
AC_PROG_LN_S

dnl This is a very ugly hack.  So is libtool...  ;)
LIBTOOL="$LIBTOOL --preserve-dup-deps"
AC_SUBST(LIBTOOL)


dnl Use/enable network support?
dnl
AC_MSG_CHECKING([for --enable-network flag])
AC_ARG_ENABLE(network, [  --enable-network        Enable network gameplay [yes]], , enable_network=yes)
if test "x$enable_network" != "xyes"; then
   AC_MSG_RESULT([Network play not enabled])
   NETWORK=0
else
   AC_MSG_RESULT([Network enabled])
   NETWORK=1
fi
AC_SUBST(NETWORK)
AM_CONDITIONAL(NETWORK, test "x$NETWORK" != "x0")


dnl Check for --with-readline flags
dnl
READLINE="0"
LIBREADLINE_LIBS=""
if test "x$NETWORK" = "x1"; then
   AC_MSG_CHECKING([for --with-readline flag])
   AC_ARG_WITH(readline,
   [  --with-readline         support fancy command line editing],
   WITH_READLINE=$withval, dnl yes/no - required to use / never use
   WITH_READLINE="maybe"   dnl maybe  - use if found [default]
   )
   AC_MSG_RESULT([$WITH_READLINE])
   if test "$WITH_READLINE" = "yes"; then
      AC_CHECK_LIB(readline, rl_callback_handler_install,
         [READLINE="1", LIBREADLINE_LIBS="-lreadline -ltermcap"],
         AC_MSG_ERROR([Specified --with-readline but did not find library.]))
         AC_CHECK_HEADER(readline/readline.h, ,
      AC_MSG_ERROR([Specified --with-readline; found library but not header file.
   You may need to install a readline \"development\" package.]))
   elif test "$WITH_READLINE" = "maybe"; then
      AC_CHECK_HEADER(readline/readline.h, have_readline_h=1, have_readline_h=0)
      if test "$have_readline_h" = "1"; then
         AC_CHECK_LIB(readline, rl_callback_handler_install,
                      have_readline_lib=1, have_readline_lib=0)
         if test "$have_readline_lib" = "1"; then
            FC_CHECK_READLINE_RUNTIME("", [LIBREADLINE_LIBS="-lreadline"])
         fi
      fi
   fi
else
   echo "No network support, not checking for libreadline."
fi

AM_CONDITIONAL(READLINE, test "x$READLINE" = "x1")
AC_SUBST(READLINE)
AC_SUBST(LIBREADLINE_LIBS)


dnl Check for --with-gtk flags
dnl

dnl Disable old GTK and GNOME subsystems
GTK12=0
GNOME=0

AM_PATH_GTK_2_0(2.20.0, [GTK20=1;
                        echo "GTK libs:  $GTK_LIBS"
                        echo "GTK flags:  $GTK_CFLAGS"],
                       [GTK20=0;
                        echo "GTK 2.20 not found, but you can ignore the errors above if you do not want an interface"])
LIBS=


dnl Figure out if ANY GTK interface is being used.
dnl
if test "x$GTK12" = "x1" -o "x$GTK20" = "x1"; then
   ANYGTK=1
else
   ANYGTK=0
fi


dnl GTK substitutions
dnl
AM_CONDITIONAL(GTK12, test "x$GTK12" = "x1")
AM_CONDITIONAL(GTK20, test "x$GTK20" = "x1")
AM_CONDITIONAL(ANYGTK, test "x$ANYGTK" = "x1")
AC_SUBST(GTK12)
AC_SUBST(GTK20)
AC_SUBST(ANYGTK)
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_CFLAGS)

dnl GNOME substitutions
dnl
AM_CONDITIONAL(GNOME, test "x$GNOME" = "x1")
AC_SUBST(GNOME)
AC_SUBST(GNOME_DESKTOP)
AC_SUBST(GNOME_PIXMAPS)


dnl Check for --enable-sound flag
dnl
if test "x$ANYGTK" = "x1"; then
   AC_MSG_CHECKING([for --enable-sound flag])
   AC_ARG_ENABLE(sound, [  --enable-sound          Enable sound/music [yes]], , enable_sound=yes)
   if test "x$enable_sound" != "xyes"; then
      AC_MSG_RESULT([Sound/music not enabled])
      MIKMOD=0
   else
      AC_MSG_RESULT([Sound/music enabled])
      MIKMOD=1
   fi
else
   echo "No client being built, so disabling MikMod support."
   MIKMOD=0
fi


dnl Check for --with-mikmod flags
dnl
if test "x$MIKMOD" = "x1"; then
   AM_PATH_LIBMIKMOD(3.1.5, [MIKMOD=1;
                             echo "libmikmod libs:  $LIBMIKMOD_LIBS"
                             echo "libmikmod flags:  $LIBMIKMOD_CFLAGS"],
                            [MIKMOD=0;
                             echo "libmikmod not found, but you can ignore the errors above if you do not want sound support"])
   LIBS=
fi

AM_CONDITIONAL(MIKMOD, test "x$MIKMOD" = "x1")
AC_SUBST(MIKMOD)


dnl Make sure we're building something at all...
dnl
if test "x$ANYGTK" = "x0" -a "x$NETWORK" = "x0"; then
   AC_MSG_ERROR([GTK and network support disabled. Nothing left to build!])
fi


dnl Check C structures that we will rely on later
dnl
critical_failure=0
AC_CHECK_HEADERS(sys/time.h, , critical_failure=1)
AC_CHECK_HEADERS(sys/stat.h, , critical_failure=1)
AC_CHECK_HEADERS(unistd.h, , critical_failure=1)
if test "x$critical_failure" = "x1"; then
   AC_MSG_ERROR([Required header missing, aborting.])
fi
AC_HEADER_TIME
if test "x$NETWORK" = "x1"; then
   AC_CHECK_HEADERS(sys/types.h, , critical_failure=1)
   AC_CHECK_HEADERS(sys/socket.h, , critical_failure=1)
   AC_CHECK_HEADERS(netdb.h, , critical_failure=1)
   AC_CHECK_HEADERS(netinet/in.h, , critical_failure=1)
   AC_CHECK_HEADERS(arpa/inet.h, , critical_failure=1)
   AC_CHECK_HEADERS(errno.h, , critical_failure=1)
   AC_CHECK_HEADERS(fcntl.h, , critical_failure=1)
   AC_CHECK_HEADERS(string.h, , critical_failure=1)
   if test "x$critical_failure" = "x1"; then
      AC_MSG_ERROR([Required network header missing, aborting.])
   fi
fi


dnl Checks for library functions.
dnl
AC_CHECK_FUNCS(gettimeofday, , [AC_MSG_ERROR([gettimeofday() not found, aborting.])])
AC_CHECK_FUNCS(usleep, , [AC_MSG_ERROR([usleep() not found, aborting.])])
AC_CHECK_FUNCS(memcpy, , [AC_MSG_ERROR([memcpy() not found, aborting.])])
AC_CHECK_FUNCS(memset, , [AC_MSG_ERROR([memset() not found, aborting.])])
AC_CHECK_FUNCS(vsnprintf, , [AC_MSG_ERROR([vsnprintf() not found, aborting.])])
AC_CHECK_FUNCS(stat, , [AC_MSG_ERROR([stat() not found, aborting.])])
AC_CHECK_FUNCS(strnlen)


dnl Check if we should use the built-in getopt
dnl
AC_CHECK_FUNCS(getopt_long, NEED_GETOPT=0, NEED_GETOPT=1)
AM_CONDITIONAL(NEED_GETOPT, test "x$NEED_GETOPT" = "x1")


dnl Checks for network library functions
dnl
NET_LIBS=""
AC_SUBST(NET_LIBS)
if test "x$NETWORK" = "x1"; then
   AC_CHECK_FUNCS(gethostbyname, ,
      [AC_CHECK_LIB(nsl, gethostbyname, NET_LIBS="$NET_LIBS -lnsl",
         [AC_MSG_ERROR([gethostbyname() network function not found, aborting.])])])
   AC_CHECK_FUNCS(socket, ,
      [AC_CHECK_LIB(socket, socket, NET_LIBS="$NET_LIBS -lsocket",
         [AC_MSG_ERROR([socket() network function not found, aborting.])])])
   echo "Network libraries we're linking against: "`if test "x$NET_LIBS" = "x"; then echo "(none)"; else echo "$NET_LIBS"; fi`
fi


dnl Checks whether netdb.h provides host.h_addr_list
dnl
AC_CHECK_MEMBER([struct hostent.h_addr_list],
   AC_DEFINE([HOSTENT_H_ADDR], [h_addr_list@<:@0@:>@], [How to access hostent.haddr]),
   AC_DEFINE([HOSTENT_H_ADDR], [h_addr], [How to access hostent.haddr]),
   [[#include <netdb.h>]])


dnl A random check
dnl
AC_CHECK_LIB(reality, convolution, , , )


dnl Checks for X Windows
dnl
if test "x$ANYGTK" = "x1"; then
   AC_PATH_XTRA
   AC_CHECK_LIB(X11, XOpenDisplay, , [AC_MSG_ERROR([libX11 is required for GTK interface, aborting.])], $X_LIBS)
   LIBS=
fi
AC_SUBST(X_LIBS)


LIBJ_PROG_CC_CFLAGS


dnl Output the makefiles and headers
dnl
AC_OUTPUT([ Makefile
            xscorch.h
            libj/libj.h
            sai/Makefile
            sconsole/Makefile
            sgame/Makefile
            sgtk/Makefile
            sgtk/swidgets/Makefile
            snet/Makefile
            snet/tcpnet/Makefile
            ssound/Makefile
            sutil/Makefile
            libj/Makefile
            libj/jreg/Makefile
            libj/jstr/Makefile
            data/Makefile
            doc/Makefile
            img/Makefile
            snd/Makefile
          ])


dnl Display final configure status
dnl
echo ""
echo "$PACKAGE $VERSION Configuration:"
echo "   Prefix:        $prefix"
echo "   Datadir:       $datadir"
echo "   Mandir:        $mandir"
echo "   CFLAGS:        $CFLAGS"
echo -n "   Sound:         "
LIBJ_PRINT_YES_NO($MIKMOD)
echo -n "   Network:       "
LIBJ_PRINT_YES_NO($NETWORK)
echo -n "   Readline:      "
LIBJ_PRINT_YES_NO($READLINE)
echo -n "   GTK 2+:        "
LIBJ_PRINT_YES_NO($GTK20)
echo ""

if test "x$NETWORK" = "x1"; then
   echo "*** WARNING ***"
   echo "Network games are UNSTABLE.  PLEASE read doc/NETWORK before sending in bug"
   echo "reports.  Also, check out the document if you are interested in network"
   echo "development.  We appreciate any feedback/bugfixes on the networking code."
   echo ""
fi

if test "x$MIKMOD" = "x1"; then
   echo "*** NOTE ***"
   echo "You have enabled sound support, but currently there is no music, nor are"
   echo "there sound effects in the game.  xscorch will be linked to mikmod but the"
   echo "sound options will currently do nothing.  We are currently looking for"
   echo "volunteers to create music for xscorch.  If you'd like to write us some"
   echo "tracks, please drop us an e-mail!"
   echo ""
fi
