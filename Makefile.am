SUBDIRS = libfaad frontend

EXTRA_DIST = faad2.spec docs/libfaad.3 project

rpm:	Makefile
	make dist
	$(RPMBUILD) -ta $(PACKAGE)-$(VERSION).tar.gz
	rm $(PACKAGE)-$(VERSION).tar.gz
