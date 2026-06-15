## $Header: /fridge/cvs/xscorch/sgtk/swidgets/Makefile.am,v 1.8 2003/02/17 22:44:52 justins Exp $
## Require automake 1.3
##
AUTOMAKE_OPTIONS	=  1.3


# Extra distribution files, and cleanfiles
#
EXTRA_DIST		=  
CLEANFILES		=  *~ *.bak *.bb *.bbg *.da *.gcov
MAINTAINERCLEANFILES	=  *.orig


if ANYGTK
noinst_LIBRARIES	=  libscorch_widget_gtk.a
endif


libscorch_widget_gtk_a_SOURCES =\
	sactiveconsole.c\
	sactiveconsole.h\
	sactoggle.c	\
	sactoggle.h	\
	sconsole.c	\
	sconsole.h	\
	sdialog.c	\
	sdialog.h	\
	sdisplay.c	\
	sdisplay.h	\
	sdrawbuf.c	\
	sdrawbuf.h	\
	slabel.c	\
	slabel.h	\
	slinkcheck.c	\
	slinkcheck.h	\
	slinkcombo.c	\
	slinkcombo.h	\
	slinkentry.c	\
	slinkentry.h	\
	slinkspin.c	\
	slinkspin.h


INCLUDES                =  $(GTK_CFLAGS) -I../ -I../../sutil -I../../libj
