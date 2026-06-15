## $Header: /fridge/cvs/xscorch/libj/jstr/Makefile.am,v 1.10 2009-05-24 19:38:46 jacob Exp $
## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.3


CLEANFILES = *~ *.bak libs/*
MAINTAINERCLEANFILES = aclocal.m4 configure 


JSTR_SRCS = \
	str_arg.c	\
	str_case.c	\
	str_cgi.c	\
	str_comp.c	\
	str_con.c	\
	str_copy.c	\
	str_creat.c	\
	str_info.c	\
	str_io.c	\
	str_num.c	\
	str_printf.c	\
	str_replace.c 	\
	str_time.c	\
	str_trim.c	\
	str_uni.c	\
	_str.h

JSTR_HDRS = \
	libjstr.h


if LIBJ_STANDALONE

noinst_LTLIBRARIES = libjstr.la
libjstr_la_SOURCES = $(JSTR_SRCS)
libjstr_la_CFLAGS = $(AM_CFLAGS)
include_HEADERS = $(JSTR_HDRS)
noinst_PROGRAMS = str_test

else

noinst_LIBRARIES = libjstr.a
libjstr_a_SOURCES = $(JSTR_SRCS) $(JSTR_HDRS)
libjstr_a_CFLAGS = $(AM_CFLAGS)

endif ## LIBJ_STANDALONE


str_test_SOURCES = \
	str_test.c


if LIBJ_USE_LIBC_STRING

str_test_LDADD = libjstr.la
INCLUDES = -I../

else ## !LIBJ_USE_LIBC_STRING

str_test_LDADD = libjstr.la ../jmem/libjmem.la
INCLUDES = -I../jmem -I../

endif ## ?LIBJ_USE_LIBC_STRING
