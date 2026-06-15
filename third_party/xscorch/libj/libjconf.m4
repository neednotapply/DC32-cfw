dnl $Header: /fridge/cvs/xscorch/libj/libjconf.m4,v 1.22 2010-09-03 07:49:30 justins Exp $
dnl Utilities for the libj configure script. This file is only of interest
dnl to projects which embed libj; projects linking against the shared lib
dnl version of libj only need libj.m4.


dnl LIBJ_VERSION_CHECK
dnl   Setup all version values based on the current libj version.
dnl
AC_DEFUN([LIBJ_VERSION_CHECK], [

   dnl
   dnl *** Current libj version is defined here ***
   dnl
   LIBJ_VERSION_MAJOR=4
   LIBJ_VERSION_MINOR=1
   LIBJ_VERSION_PATCH=3

   dnl Build the library version number based on the numbers above.
   AC_MSG_CHECKING([for libj version number])
   LIBJ_VERSION="${LIBJ_VERSION_MAJOR}.${LIBJ_VERSION_MINOR}.${LIBJ_VERSION_PATCH}"
   LIBJ_VERSION_EXPR="((((long)${LIBJ_VERSION_MAJOR} & 0xff) << 24) | (((long)${LIBJ_VERSION_MINOR} & 0xff) << 16) | (((long)${LIBJ_VERSION_PATCH} & 0xff) << 8)) /* Version ${LIBJ_VERSION} */"
   AC_SUBST([LIBJ_VERSION])
   AC_SUBST([LIBJ_VERSION_MAJOR])
   AC_SUBST([LIBJ_VERSION_MINOR])
   AC_SUBST([LIBJ_VERSION_PATCH])
   AC_SUBST([LIBJ_VERSION_EXPR])
   LIBJ_LT_CURRENT=`expr $LIBJ_VERSION_MAJOR + $LIBJ_VERSION_MINOR`
   LIBJ_LT_REVISION=$LIBJ_VERSION_PATCH
   LIBJ_LT_AGE=$LIBJ_VERSION_MINOR
   AC_SUBST([LIBJ_LT_CURRENT])
   AC_SUBST([LIBJ_LT_REVISION])
   AC_SUBST([LIBJ_LT_AGE])
   AC_MSG_RESULT([$LIBJ_VERSION])
])


dnl LIBJ_CONF_SIZES
dnl   Check the sizes of various data types.  Used to setup the
dnl   fixed-size type definitions in libj.h.
dnl
AC_DEFUN([LIBJ_CONF_SIZES], [
   dnl Requirements
   AC_REQUIRE([LIBJ_PROG_CC])

   dnl Checks for object sizes
   AC_CHECK_SIZEOF([short],      2)
   AC_CHECK_SIZEOF([int],        4)
   AC_CHECK_SIZEOF([long],       4)
   AC_CHECK_SIZEOF([long long],  8)
   AC_CHECK_SIZEOF([int16_t],    0)
   AC_CHECK_SIZEOF([int32_t],    0)
   AC_CHECK_SIZEOF([int64_t],    0)

   LIBJ_BYTE_TYPE=char

   case 2 in
      $ac_cv_sizeof_short)       LIBJ_WORD_TYPE=short       ;;
      $ac_cv_sizeof_int)         LIBJ_WORD_TYPE=int         ;;
      $ac_cv_sizeof_int16_t)     LIBJ_WORD_TYPE=int16_t     ;;
      *)                         AC_MSG_ERROR([Cannot find a suitable 16-bit data type])
   esac

   case 4 in
      $ac_cv_sizeof_short)       LIBJ_DWORD_TYPE=short      ;;
      $ac_cv_sizeof_int)         LIBJ_DWORD_TYPE=int        ;;
      $ac_cv_sizeof_long)        LIBJ_DWORD_TYPE=long       ;;
      $ac_cv_sizeof_int32_t)     LIBJ_DWORD_TYPE=int32_t    ;;
      *)                         AC_MSG_ERROR([Cannot find a suitable 32-bit data type])
   esac

   case 8 in
      $ac_cv_sizeof_int)         LIBJ_QWORD_TYPE=int        ;;
      $ac_cv_sizeof_long_long)   LIBJ_QWORD_TYPE="long long";;
      $ac_cv_sizeof_int64_t)     LIBJ_QWORD_TYPE=int64_t    ;;
      *)                         AC_MSG_ERROR([Cannot find a suitable 64-bit data type])
   esac

   AC_SUBST([LIBJ_BYTE_TYPE])
   AC_SUBST([LIBJ_WORD_TYPE])
   AC_SUBST([LIBJ_DWORD_TYPE])
   AC_SUBST([LIBJ_QWORD_TYPE])
])


dnl LIBJ_CONF_HEADERS
dnl   Check several headers that libj requires, and make
dnl   the appropriate definitions.
dnl
AC_DEFUN([LIBJ_CONF_HEADERS], [
   dnl Check for standard C headers (may have already run)
   AC_REQUIRE([LIBJ_PROG_CC])
   AC_REQUIRE([AC_HEADER_STDC])

   dnl Check C headers
   AC_CHECK_HEADERS([sys/time.h])
   if test "$ac_cv_header_sys_time_h" = "yes"; then
      LIBJ_HAVE_SYS_TIME_H=1
   else
      LIBJ_HAVE_SYS_TIME_H=0
   fi
   AC_SUBST([LIBJ_HAVE_SYS_TIME_H])
   AC_HEADER_TIME
   if test "$ac_cv_header_time" = "yes"; then
      LIBJ_TIME_WITH_SYS_TIME=1
   else
      LIBJ_TIME_WITH_SYS_TIME=0
   fi
   AC_SUBST([LIBJ_TIME_WITH_SYS_TIME])
])


dnl LIBJ_CONF_FUNCTIONS
dnl   Check several functions that libj requires, and make
dnl   the appropriate definitions.
dnl
AC_DEFUN([LIBJ_CONF_FUNCTIONS], [
   dnl Make sure we checked for the compiler.
   AC_REQUIRE([LIBJ_PROG_CC])

   dnl Checks for library functions.
   AC_CHECK_FUNCS([gettimeofday])
   if test "$ac_cv_func_gettimeofday" = "yes"; then
      LIBJ_HAVE_GETTIMEOFDAY=1
   else
      LIBJ_HAVE_GETTIMEOFDAY=0
   fi
   AC_SUBST([LIBJ_HAVE_GETTIMEOFDAY])
   AC_CHECK_FUNCS([snprintf], , [AC_MSG_ERROR([snprintf not found, aborting])])
])


dnl LIBJ_CONF_STANDALONE
dnl LIBJ_CONF_EMBEDDED
dnl   Setup STANDALONE or EMBEDDED mode build.
dnl   LIBJ_STANDALONE is always 1 for a libj build.
dnl   Programs that embed libj should set this 0 in
dnl   their configure scripts.  These macros are
dnl   convenient shorthands for setting the build.
dnl
dnl   The embedded macro conveniently does all the
dnl   standard libj checks, and also defines defaults
dnl   for the libj compilation options.
dnl
AC_DEFUN([LIBJ_CONF_STANDALONE], [
   LIBJ_STANDALONE=1
   AC_SUBST([LIBJ_STANDALONE])
   AM_CONDITIONAL(LIBJ_STANDALONE, [true])
])

AC_DEFUN([LIBJ_CONF_EMBEDDED], [
   dnl Requirements for the embedded build
   dnl Note that requirements are *always* expanded first.
   AC_REQUIRE([LIBJ_VERSION_CHECK])
   AC_REQUIRE([LIBJ_PROG_CC])
   AC_REQUIRE([LIBJ_CONF_HEADERS])
   AC_REQUIRE([LIBJ_CONF_FUNCTIONS])
   AC_REQUIRE([LIBJ_CONF_SIZES])

   dnl Setup for embedded (not standalone) build.
   LIBJ_STANDALONE=0
   AC_SUBST([LIBJ_STANDALONE])
   AM_CONDITIONAL(LIBJ_STANDALONE, [false])

   dnl Set default compile options for libj
   LIBJ_USE_LIBC_STRING=1
   LIBJ_C99_STANDARD=0
   AC_SUBST([LIBJ_USE_LIBC_STRING])
   AC_SUBST([LIBJ_C99_STANDARD])
   AM_CONDITIONAL(LIBJ_USE_LIBC_STRING, [true])
])
