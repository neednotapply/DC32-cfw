## $Header: /fridge/cvs/xscorch/libj/jreg/Makefile.am,v 1.9 2011-06-12 08:12:16 justins Exp $
## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.4


EXTRADIST = overview.txt
CLEANFILES = *~ *.bak libs/*
MAINTAINERCLEANFILES = aclocal.m4 configure 


JREG_SRCS = \
	reg.c		\
	regblock.c	\
	regio.c		\
	regvar.c	\
	_reg.h

JREG_HDRS = \
	libjreg.h


if LIBJ_STANDALONE

noinst_LTLIBRARIES = libjreg.la
libjreg_la_SOURCES = $(JREG_SRCS)
libjreg_la_CFLAGS = $(AM_CFLAGS)
include_HEADERS = $(JREG_HDRS)

else

noinst_LIBRARIES = libjreg.a
libjreg_a_SOURCES = $(JREG_SRCS) $(JREG_HDRS)
libjreg_a_CFLAGS = $(AM_CFLAGS)

endif ## LIBJ_STANDALONE


if LIBJ_USE_LIBC_STRING

INCLUDES = -I../jstr -I../

else

INCLUDES = -I../jmem -I../jstr -I../

endif ## LIBJ_USE_LIBC_STRING
