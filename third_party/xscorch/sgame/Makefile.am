## $Header: /fridge/cvs/xscorch/sgame/Makefile.am,v 1.10 2003/10/04 05:15:08 justins Exp $
## Require automake 1.3
##
AUTOMAKE_OPTIONS	=  1.3


# Extra distribution files, and cleanfiles
#
EXTRA_DIST		=  
CLEANFILES		=  *~ *.bak *.bb *.bbg *.da *.gcov shelpdata.c
MAINTAINERCLEANFILES	=  *.orig


# Convenience library for AI code
#
noinst_LIBRARIES	=  libscorch_game.a


# Game code
#
libscorch_game_a_SOURCES =\
	saccessory.c	\
	saccessory.h	\
	saddconf.c	\
	saddconf.h	\
	scffile.c	\
	scffile.h	\
	scffileold.c	\
	scolor.c	\
	scolor.h	\
	sconfig.c	\
	sconfig.h	\
	seconomy.c	\
	seconomy.h	\
	sexplosion.c	\
	sexplosion.h	\
	sgame.c		\
	sgame.h		\
	shelp.c		\
	shelp.h		\
	shelpdata.c	\
	shelpdata.h	\
	sinfo.c		\
	sinfo.h		\
	sinventory.c	\
	sinventory.h	\
	sland.c		\
	sland.h		\
	slscape.c	\
	slscape.h	\
	slscapetools.c	\
	slscapetools.h	\
	soptions.c	\
	soptions.h	\
	sphoenix.c	\
	sphoenix.h	\
	sphysics.c	\
	sphysics.h	\
	splayer.c	\
	splayer.h	\
	spreround.c	\
	spreround.h	\
	sregistry.c	\
	sregistry.h	\
	sshield.c	\
	sshield.h	\
	sspill.c	\
	sspill.h	\
	sstate.c	\
	sstate.h	\
	stankpro.c	\
	stankpro.h	\
	strack.c	\
	strack.h	\
	sweapon.c	\
	sweapon.h	\
	swindow.h	


shelpdata.c: ../doc/shelpdata.c
	cp -f ../doc/shelpdata.c shelpdata.c


## idiot justin.. don't remove -I../sutil, it'll mess up getopt.
INCLUDES   = -I../sutil -I../libj
