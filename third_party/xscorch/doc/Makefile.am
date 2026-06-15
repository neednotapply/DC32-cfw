## $Header: /fridge/cvs/xscorch/doc/Makefile.am,v 1.8 2003/10/04 17:54:41 justins Exp $
## Require automake 1.3
##
AUTOMAKE_OPTIONS	=  1.3


# Extra distribution files, and cleanfiles
#
man_MANS 		=  xscorch.6
EXTRA_DIST 		=  $(man_MANS) xscorch.txt NOTES NETWORK AI make_text_data.pl
CLEANFILES		=  *~ *.bak *.bb *.bbg *.da *.gcov shelpdata.c
MAINTAINERCLEANFILES	=  *.orig


all-local: xscorch.txt shelpdata.c


xscorch.txt: xscorch.6
	cat xscorch.6 | groff -Tascii -mandoc > xscorch.txt


shelpdata.c: xscorch.txt xscorch.6
	echo "#include                       <shelpdata.h>"> shelpdata.c
	./make_text_data.pl data_xscorch_txt <xscorch.txt>>  shelpdata.c
	./make_text_data.pl data_xscorch_man <xscorch.6>>    shelpdata.c
	./make_text_data.pl data_copying_txt <../COPYING>>   shelpdata.c


install-data-local: xscorch.txt
	$(mkinstalldirs) $(DESTDIR)$(datadir)/xscorch
	$(INSTALL_DATA) ../COPYING $(DESTDIR)$(datadir)/xscorch/copying.txt
	$(INSTALL_DATA) xscorch.txt $(DESTDIR)$(datadir)/xscorch/xscorch.txt
