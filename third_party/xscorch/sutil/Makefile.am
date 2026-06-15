## $Header: /fridge/cvs/xscorch/sutil/Makefile.am,v 1.15 2003/10/04 17:54:41 justins Exp $
## Require automake 1.3
##
AUTOMAKE_OPTIONS	=  1.3


# Extra distribution files, and cleanfiles
#
EXTRA_DIST		=  getopt.c getopt.h getopt1.c
CLEANFILES		=  *~ *.bak *.bb *.bbg *.da *.gcov
MAINTAINERCLEANFILES	=  *.orig


# Convenience library for utilities
#
noinst_LIBRARIES	=  libscorch_util.a


if NEED_GETOPT
GETOPT_SRCS		=  getopt.c getopt.h getopt1.c
else ## don't need built-in getopt support
GETOPT_SRCS		=
endif


# Game code
#
libscorch_util_a_SOURCES =\
	$(GETOPT_SRCS)  \
	md5.c		\
	md5.h		\
	randserv.c	\
	randserv.h	\
	sfractal.c	\
	sfractal.h	\
	sgetline.h	\
	shash.c		\
	shash.h		\
	shashlist.c	\
	shashlist.h	\
	srand.c		\
	srand.h		\
	sslink.c	\
	sslink.h


INCLUDES   =  -I../libj
