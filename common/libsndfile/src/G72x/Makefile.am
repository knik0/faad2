# This file was automatically generated
# DO NOT EDIT!

EXTRA_DIST = README README.original ChangeLog

noinst_HEADERS = g72x.h private.h
noinst_LTLIBRARIES = libg72x.la

noinst_PROGRAMS = g72x_test

CFILES = g72x.c g721.c g723_16.c g723_24.c g723_40.c

libg72x_la_SOURCES = $(CFILES) $(noinst_HEADERS)

g72x_test_SOURCES = g72x_test.c $(CFILES)
g72x_test_LDADD = -lm

check: g72x_test
	./g72x_test all

# Disable autoheader. 
AUTOHEADER=echo
