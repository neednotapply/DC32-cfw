## $Header: /fridge/cvs/xscorch/snet/Makefile.am,v 1.7 2003/02/17 00:15:35 justins Exp $
## Require automake 1.3
##
AUTOMAKE_OPTIONS	=  1.3


# Subdirectories to build in
#
SUBDIRS  =  tcpnet


# Extra distribution files, and cleanfiles
#
EXTRA_DIST		=  
CLEANFILES		=  *~ *.bak *.bb *.bbg *.da *.gcov
MAINTAINERCLEANFILES	=  *.orig


# Convenience library for network code
#
if NETWORK
noinst_LIBRARIES	=  libscorch_net.a
endif


# Network code
#
libscorch_net_a_SOURCES =\
	snet.c		\
	snet.h		\
	snetclient.c	\
	snetconfig.c	\
	snetcomm.c	\
	snetconnect.c	\
	snetint.h	\
	snetserver.c	\
	snetstatus.c


INCLUDES = -I../libj

