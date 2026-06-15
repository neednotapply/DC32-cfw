## $Header: /fridge/cvs/xscorch/Makefile.am,v 1.23 2011-07-31 19:47:58 jacob Exp $
## Require automake 1.3
##
AUTOMAKE_OPTIONS	=  1.11
ACLOCAL_AMFLAGS		=  -I m4


# Subdirectories to build in
#
SUBDIRS			=  doc sai sconsole sgame sgtk snet ssound sutil libj data img snd


# Extra distribution files, and cleanfiles
#
EXTRA_DIST		=  TODO
CLEANFILES		=  *~ *.bak *.bb *.bbg *.da *.gcov
MAINTAINERCLEANFILES	=  *.orig


if ANYGTK
GTK_PROGRAM=xscorch
else
GTK_PROGRAM=
endif


if NETWORK
NETWORK_PROGRAM=xscorch-server
NETWORK_LD_ADD=$(NET_LIBS) snet/libscorch_net.a snet/tcpnet/libtcpnet.a
else
NETWORK_PROGRAM=
NETWORK_LD_ADD=
endif


if MIKMOD
MIKMOD_LD_ADD=ssound/libscorch_sound.a
else
MIKMOD_LD_ADD=ssound/libscorch_sound_null.a
endif


bin_PROGRAMS=$(GTK_PROGRAM) $(NETWORK_PROGRAM)


xscorch_SOURCES = \
	xscorch.c
xscorch_LDADD = \
	sgame/libscorch_game.a \
	sgtk/libscorch_gtk.a \
	sgtk/swidgets/libscorch_widget_gtk.a \
	sai/libscorch_ai.a \
	$(MIKMOD_LD_ADD) \
	$(NETWORK_LD_ADD) \
	sutil/libscorch_util.a \
	sgame/libscorch_game.a \
	libj/jreg/libjreg.a \
	libj/jstr/libjstr.a \
	-lm \
	$(GTK_LIBS) \
	$(LIBMIKMOD_LIBS)


xscorch_server_SOURCES = \
	xscorch.c
xscorch_server_LDADD = \
	sgame/libscorch_game.a \
	sconsole/libscorch_console.a \
	sai/libscorch_ai.a \
	ssound/libscorch_sound_null.a \
	$(NETWORK_LD_ADD) \
	sutil/libscorch_util.a \
	sgame/libscorch_game.a \
	libj/jreg/libjreg.a \
	libj/jstr/libjstr.a \
	-lm \
	$(LIBREADLINE_LIBS)


work-list:
	egrep -n '(TEMP(\W|_)|#warning\W)' `find . \! -path ./old/\* -a -name \*.\[ch\]` | sort > doc/work-list.txt


INCLUDES=-Isgame -Isgtk


install-data-local:
	@echo ""
	@echo ""
	@echo "*** Installation successful ***"
	@echo ""
	@echo "Please note:  Until you use the 'Save Options' command in"
	@echo "xscorch, you will get a message indicating that the user"
	@echo "configuration could not be loaded.  This is normal and will"
	@echo "not affect gameplay; the program is just letting you know"
	@echo "that it will use the game defaults instead."
	@echo ""
	@echo "Note also: the keybindings for shields have changed.  See"
	@echo "the ChangeLog or the manual page for details."
	@echo ""
	@echo "Enjoy the game!"
	@echo ""
	@echo ""

