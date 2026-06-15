## $Header: /fridge/cvs/xscorch/sgtk/Makefile.am,v 1.16 2003/02/26 03:49:07 justins Exp $
## Require automake 1.3
##
AUTOMAKE_OPTIONS	=  1.3


# Subdirectories to build in
#
SUBDIRS	 =  swidgets


# Extra distribution files, and cleanfiles
#
EXTRA_DIST		=  
CLEANFILES		=  *~ *.bak *.bb *.bbg *.da *.gcov
MAINTAINERCLEANFILES	=  *.orig


if ANYGTK
noinst_LIBRARIES	=  libscorch_gtk.a
endif


libscorch_gtk_a_SOURCES =\
	sai-gtk.c	\
	sautodef-gtk.c	\
	scolor-gtk.c	\
	scolor-gtk.h	\
	sdialog-gtk.h	\
	seconomy-gtk.c	\
	send-gtk.c	\
	sexplosion-gtk.c\
	sexplosion-gtk.h\
	sfile-gtk.c	\
	sfont-gtk.c	\
	sgraphics-gtk.c	\
	sgtk.h		\
	simage-gtk.c	\
	simage-gtk.h	\
	sinventory-gtk.c\
	sland-gtk.c	\
	slottery-gtk.c	\
	smainmenu-gtk.c	\
	smenu-gtk.c	\
	smenu-gtk.h	\
	snet-gtk.c	\
	snet-gtk.h	\
	soptions-gtk.c	\
	spaint-gtk.c	\
	sphysics-gtk.c	\
	splayer-gtk.c	\
	ssetup-gtk.h	\
	ssound-gtk.c	\
	sspill-gtk.c	\
	sstatus-gtk.c	\
	sstatus-gtk.h	\
	ssystem-gtk.c	\
	ssystem-gtk.h	\
	stank-gtk.c	\
	stank-gtk.h	\
	sweapons-gtk.c	\
	swindow-gtk.c	\
	swindow-gtk.h


INCLUDES  =  $(GTK_CFLAGS) -Iswidgets -I../libj
