bin_PROGRAMS = faad
dist_man1_MANS = faad.man

AM_CPPFLAGS = -I$(top_srcdir)/include

faad_LDADD = $(top_builddir)/libfaad/libfaad.la

faad_SOURCES = mp4read.c audio.c main.c audio.h mp4read.h unicode_support.c unicode_support.h

EXTRA_faad_SOURCES = getopt.c getopt.h
