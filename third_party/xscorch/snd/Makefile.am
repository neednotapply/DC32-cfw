## $Header: /fridge/cvs/xscorch/snd/Makefile.am,v 1.3 2001/04/07 20:06:16 justins Exp $
## Require automake 1.3
##
AUTOMAKE_OPTIONS	=  1.3


# Extra distribution files, and cleanfiles
#
EXTRA_DIST 		=  README
CLEANFILES		=  *~ *.bak *.bb *.bbg *.da *.gcov
MAINTAINERCLEANFILES	=  *.orig


install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datadir)/xscorch/sounds
	for file in $(EXTRA_DIST); do $(INSTALL_DATA) "$$file" "$(DESTDIR)$(datadir)/xscorch/sounds/$$file"; done
