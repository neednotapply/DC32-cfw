## $Header: /fridge/cvs/xscorch/data/Makefile.am,v 1.6 2003/02/20 10:27:43 jacob Exp $
## Require automake 1.3
##
AUTOMAKE_OPTIONS	=  1.3


# Extra distribution files, and cleanfiles
#
EXTRA_DIST 		=  accessories.def profiles.def scorings.def weapons.def
CLEANFILES		=  *~ *.bak *.bb *.bbg *.da *.gcov
MAINTAINERCLEANFILES	=  *.orig


install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datadir)/xscorch
	for file in $(EXTRA_DIST); do $(INSTALL_DATA) "$$file" "$(DESTDIR)$(datadir)/xscorch/$$file"; done
