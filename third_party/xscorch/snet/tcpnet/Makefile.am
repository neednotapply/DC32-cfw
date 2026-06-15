## $Header: /fridge/cvs/xscorch/snet/tcpnet/Makefile.am,v 1.1 2001/07/26 05:47:08 jacob Exp $
## Require automake 1.3
##
AUTOMAKE_OPTIONS	=  1.3


# Extra distribution files, and cleanfiles
#
EXTRA_DIST		=  
CLEANFILES		=  *~ *.bak *.bb *.bbg *.da *.gcov
MAINTAINERCLEANFILES	=  *.orig


# Convenience library for tcp net.
#
if NETWORK
noinst_LIBRARIES	=  libtcpnet.a
endif


# AI code
#
libtcpnet_a_SOURCES =\
	tcpnet.h	\
	tn_internal.c	\
	tn_internal.h	\
	tn_read.c	\
	tn_setup.c	\
	tn_write.c


INCLUDES = 
