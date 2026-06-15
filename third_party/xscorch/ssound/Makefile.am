## $Header: /fridge/cvs/xscorch/ssound/Makefile.am,v 1.5 2003/02/17 00:15:35 justins Exp $
## Require automake 1.3
##
AUTOMAKE_OPTIONS	=  1.3


# Extra distribution files, and cleanfiles
#
EXTRA_DIST		=  
CLEANFILES		=  *~ *.bak *.bb *.bbg *.da *.gcov
MAINTAINERCLEANFILES	=  *.orig


# Convenience library for sound code
#
if MIKMOD
SOUND_LIBS		=  libscorch_sound.a
else
SOUND_LIBS		=  
endif


noinst_LIBRARIES	=  libscorch_sound_null.a $(SOUND_LIBS)


# sound code
#
libscorch_sound_a_SOURCES =\
	ssound.c	\
	ssound.h

libscorch_sound_null_a_SOURCES =\
	ssoundnull.c


INCLUDES =  $(LIBMIKMOD_CFLAGS) -I../sutil -I../libj
