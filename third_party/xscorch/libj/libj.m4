# $Header: /fridge/cvs/xscorch/libj/libj.m4,v 1.11 2010-09-03 07:49:28 justins Exp $
# Hacked from configure for GLIB
# Owen Taylor     97-11-3
# Hacked by Justin David Smith, 2001.12.10
# Includes some libj-specific functions as well


dnl AM_PATH_LIBJ([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND [, MODULES]]]])
dnl   Test for LIBJ, and define LIBJ_CFLAGS and LIBJ_LIBS.
dnl   This should be used by applications which wish to link
dnl   to an external copy of libj.  Applications which want
dnl   to embed libj directly need to import the libjconf.m4
dnl   internal configuration macros instead.
dnl
AC_DEFUN([AM_PATH_LIBJ],
[dnl
dnl Get the cflags and libraries from the libj-config script
dnl
AC_ARG_WITH(libj-prefix,[  --with-libj-prefix=PFX   Prefix where LIBJ is installed (optional)],
            libj_config_prefix="$withval", libj_config_prefix="")
AC_ARG_WITH(libj-exec-prefix,[  --with-libj-exec-prefix=PFX Exec prefix where LIBJ is installed (optional)],
            libj_config_exec_prefix="$withval", libj_config_exec_prefix="")
AC_ARG_ENABLE(libjtest, [  --disable-libjtest       Do not try to compile and run a test LIBJ program],
                    , enable_libjtest=yes)

  if test x$libj_config_exec_prefix != x ; then
     libj_config_args="$libj_config_args --exec-prefix=$libj_config_exec_prefix"
     if test x${LIBJ_CONFIG+set} != xset ; then
        LIBJ_CONFIG=$libj_config_exec_prefix/bin/libj-config
     fi
  fi
  if test x$libj_config_prefix != x ; then
     libj_config_args="$libj_config_args --prefix=$libj_config_prefix"
     if test x${LIBJ_CONFIG+set} != xset ; then
        LIBJ_CONFIG=$libj_config_prefix/bin/libj-config
     fi
  fi

  for module in . $4
  do
      case "$module" in
         jmtx | libjmtx)
             libj_config_args="$libj_config_args libjmtx"
         ;;
         jconio | libjconio)
             libj_config_args="$libj_config_args libjconio"
         ;;
      esac
  done

  AC_PATH_PROG(LIBJ_CONFIG, libj-config, no)
  min_libj_version=ifelse([$1], ,0.99.7,$1)
  AC_MSG_CHECKING(for LIBJ - version >= $min_libj_version)
  no_libj=""
  if test "$LIBJ_CONFIG" = "no" ; then
    no_libj=yes
  else
    LIBJ_CFLAGS=`$LIBJ_CONFIG $libj_config_args --cflags`
    LIBJ_LIBS=`$LIBJ_CONFIG $libj_config_args --libs`
    LIBJ_VERSION=`$LIBJ_CONFIG $libj_config_args --version`
    libj_config_major_version=`$LIBJ_CONFIG $libj_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    libj_config_minor_version=`$LIBJ_CONFIG $libj_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    libj_config_micro_version=`$LIBJ_CONFIG $libj_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_libjtest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $LIBJ_CFLAGS"
      LIBS="$LIBJ_LIBS $LIBS"
dnl
dnl Now check if the installed LIBJ is sufficiently new. (Also sanity
dnl checks the results of libj-config to some extent
dnl
      rm -f conf.libjtest
      AC_TRY_RUN([
#define  LIBJ_ALLOW_LIBC_STRING    1
#include <string.h>
#include <libj.h>
#include <stdio.h>
#include <stdlib.h>

int
main ()
{
  int major, minor, micro;
  unsigned int libj_v;
  unsigned int libj_major_version;
  unsigned int libj_minor_version;
  unsigned int libj_micro_version;
  unsigned int LIBJ_MAJOR_VERSION;
  unsigned int LIBJ_MINOR_VERSION;
  unsigned int LIBJ_MICRO_VERSION;
  char *tmp_version;

  system ("touch conf.libjtest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = strdup("$min_libj_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_libj_version");
     exit(1);
   }

  libj_v = libj_version();
  libj_major_version = (libj_v >> 24) & 0xff;
  libj_minor_version = (libj_v >> 16) & 0xff;
  libj_micro_version = (libj_v >> 8)  & 0xff;
  LIBJ_MAJOR_VERSION = (__libj_version >> 24) & 0xff;
  LIBJ_MINOR_VERSION = (__libj_version >> 16) & 0xff;
  LIBJ_MICRO_VERSION = (__libj_version >> 8)  & 0xff;

  if ((libj_major_version != $libj_config_major_version) ||
      (libj_minor_version != $libj_config_minor_version) ||
      (libj_micro_version != $libj_config_micro_version))
    {
      printf("\n*** 'libj-config --version' returned %d.%d.%d, but LIBJ (%d.%d.%d)\n",
             $libj_config_major_version, $libj_config_minor_version, $libj_config_micro_version,
             libj_major_version, libj_minor_version, libj_micro_version);
      printf ("*** was found! If libj-config was correct, then it is best\n");
      printf ("*** to remove the old version of LIBJ. You may also be able to fix the error\n");
      printf("*** by modifying your LD_LIBRARY_PATH enviroment variable, or by editing\n");
      printf("*** /etc/ld.so.conf. Make sure you have run ldconfig if that is\n");
      printf("*** required on your system.\n");
      printf("*** If libj-config was wrong, set the environment variable LIBJ_CONFIG\n");
      printf("*** to point to the correct copy of libj-config, and remove the file config.cache\n");
      printf("*** before re-running configure\n");
    }
  else if ((libj_major_version != LIBJ_MAJOR_VERSION) ||
           (libj_minor_version != LIBJ_MINOR_VERSION) ||
           (libj_micro_version != LIBJ_MICRO_VERSION))
    {
      printf("*** LIBJ header files (version %d.%d.%d) do not match\n",
             LIBJ_MAJOR_VERSION, LIBJ_MINOR_VERSION, LIBJ_MICRO_VERSION);
      printf("*** library (version %d.%d.%d)\n",
             libj_major_version, libj_minor_version, libj_micro_version);
    }
  else
    {
      if ((libj_major_version > major) ||
        ((libj_major_version == major) && (libj_minor_version > minor)) ||
        ((libj_major_version == major) && (libj_minor_version == minor) && (libj_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of LIBJ (%d.%d.%d) was found.\n",
               libj_major_version, libj_minor_version, libj_micro_version);
        printf("*** You need a version of LIBJ newer than %d.%d.%d. The latest version of\n",
               major, minor, micro);
        printf("*** LIBJ is always available from http://chaos2.org./.\n");
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the libj-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of LIBJ, but you can also set the LIBJ_CONFIG environment to point to the\n");
        printf("*** correct copy of libj-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
      }
    }
  return 1;
}
],, no_libj=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_libj" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])
  else
     AC_MSG_RESULT(no)
     if test "$LIBJ_CONFIG" = "no" ; then
       echo "*** The libj-config script installed by LIBJ could not be found"
       echo "*** If LIBJ was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the LIBJ_CONFIG environment variable to the"
       echo "*** full path to libj-config."
     else
       if test -f conf.libjtest ; then
        :
       else
          echo "*** Could not run LIBJ test program, checking why..."
          CFLAGS="$CFLAGS $LIBJ_CFLAGS"
          LIBS="$LIBS $LIBJ_LIBS"
          AC_TRY_LINK([
#include <libj.h>
#include <stdio.h>
],      [ return (__libj_version == 0); ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding LIBJ or finding the wrong"
          echo "*** version of LIBJ. If it is not finding LIBJ, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
          echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH" ],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means LIBJ was incorrectly installed"
          echo "*** or that you have moved LIBJ since it was installed. In the latter case, you"
          echo "*** may want to edit the libj-config script: $LIBJ_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     LIBJ_CFLAGS=""
     LIBJ_LIBS=""
     LIBJ_VERSION=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST([LIBJ_CFLAGS])
  AC_SUBST([LIBJ_LIBS])
  AC_SUBST([LIBJ_VERSION])
  rm -f conf.libjtest
])


dnl LIBJ_PROG_CC
dnl   Search for GCC or a related compiler and utilities.
dnl
AC_DEFUN([LIBJ_PROG_CC], [
   dnl Make sure we have a C compiler and preprocessor lying around.
   AC_REQUIRE([AC_CANONICAL_HOST])
   AC_REQUIRE([AC_PROG_CC])
   AC_REQUIRE([AC_PROG_CPP])
   
   dnl Make sure the desired constructs exist in the language.
   AC_REQUIRE([AC_C_CONST])
   AC_REQUIRE([AC_C_INLINE])
])


dnl LIBJ_PROG_CC_CFLAGS
dnl   Setup the CFLAGS as appropriate.
dnl
dnl   This provides --enable-opt, --enable-warn, and --enable-debug.
dnl   Currently, this will try to enforce the C99 standard language
dnl   if --enable-debug is set, assuming the compiler supports the
dnl   standard.
dnl
dnl   This now provides handling for --enable-warn-error as well.
dnl
dnl   This defines LIBJ_C99_STANDARD, and sets it to 1 if the compiler
dnl   is GCC supporting the C99 standard.
dnl
AC_DEFUN([LIBJ_PROG_CC_CFLAGS], [
   dnl Make sure we have a C compiler and preprocessor lying around.
   AC_REQUIRE([LIBJ_PROG_CC])

   dnl Check for GCC-specific extensions.
   if test "x$ac_cv_prog_gcc" = "xyes"; then
      echo "GCC compiler found, trying GCC-specific optimizations and warnings."
      AC_MSG_CHECKING([if compiler supports C99 standard])
      LIBJ_OLD_CFLAGS="$CFLAGS"
      CFLAGS=
      AC_TRY_COMPILE(, ,
         [AC_MSG_RESULT([yes])
          LIBJ_C99_STANDARD=1],
         [AC_MSG_RESULT([no])
          LIBJ_C99_STANDARD=0])
      CFLAGS="$LIBJ_OLD_CFLAGS"

      AC_MSG_CHECKING([for --enable-opt flag])
      AC_ARG_ENABLE(opt, [[  --enable-opt            Enable optimizations [yes if CFLAGS not set]]], , enable_opt=maybe)
      if test "x$enable_opt" = "xyes" -o \( "x$enable_opt" = "xmaybe" -a "x$CFLAGS" = "x" \); then
         AC_MSG_RESULT([enabled])
         LIBJ_CFLAGS_OPT="-O3 -fomit-frame-pointer -finline-functions -funroll-loops -fthread-jumps -DNDEBUG"
      else
         AC_MSG_RESULT([not enabled])
         LIBJ_CFLAGS_OPT=
      fi
      AC_MSG_CHECKING([for --enable-warn flag])
      AC_ARG_ENABLE(warn, [[  --enable-warn           Enable warnings [yes if CFLAGS not set]]], , enable_warn=maybe)
      if test "x$enable_warn" = "xyes" -o \( "x$enable_warn" = "xmaybe" -a "x$CFLAGS" = "x" \); then
         AC_MSG_RESULT([enabled])
         LIBJ_CFLAGS_WARN="-Wall -Wpointer-arith"
      else
         AC_MSG_RESULT([not enabled])
         LIBJ_CFLAGS_WARN=
      fi
      AC_MSG_CHECKING([for --enable-warn-error flag])
      AC_ARG_ENABLE(warn, [[  --enable-warn-error     Enable warnings as errors [yes if CFLAGS not set]]], , enable_warn_error=maybe)
      if test "x$enable_warn_error" = "xyes" -o \( "x$enable_warn_error" = "xmaybe" -a "x$CFLAGS" = "x" \); then
         AC_MSG_RESULT([enabled])
         dnl Warning: on recent gcc we need no-strict-aliasing with Werror
         LIBJ_CFLAGS_WARN_ERROR="-Werror -fno-strict-aliasing"
      else
         AC_MSG_RESULT([not enabled])
         LIBJ_CFLAGS_WARN_ERROR=
      fi
      AC_MSG_CHECKING([for --enable-debug flag])
      AC_ARG_ENABLE(debug, [[  --enable-debug          Enable debug flags [no]]], , enable_debug=no)
      if test "x$enable_debug" = "xyes"; then
         AC_MSG_RESULT([enabled])
         LIBJ_CFLAGS_DEBUG="-g"
         if test "x$LIBJ_C99_STANDARD" = "x1"; then
            LIBJ_CFLAGS_DEBUG="$LIBJ_CFLAGS_DEBUG -W -Wunused"
         fi
      else
         AC_MSG_RESULT([not enabled])
         LIBJ_CFLAGS_DEBUG=
      fi
      CFLAGS="$CFLAGS $LIBJ_CFLAGS_OPT $LIBJ_CFLAGS_WARN $LIBJ_CFLAGS_WARN_ERROR $LIBJ_CFLAGS_DEBUG"
      echo "CFLAGS = $CFLAGS"
   else
      echo "No GCC compiler, will not enable optimizations or warnings."
      LIBJ_C99_STANDARD=0
   fi
   AC_SUBST([LIBJ_C99_STANDARD])
])


dnl LIBJ_PRINT_YES_NO(value)
dnl   Prints "yes" or "no", depending on if value is 1 or 0.
dnl
AC_DEFUN([LIBJ_PRINT_YES_NO], [
   if test "x$1" = "x1"; then
      echo "Yes"
   else
      echo "NO"
   fi
])
