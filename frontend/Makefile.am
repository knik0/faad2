bin_PROGRAMS = faad
dist_man1_MANS = faad.man

AM_CPPFLAGS = -I$(top_srcdir)/include

faad_LDADD = $(top_builddir)/libfaad/libfaad.la

faad_SOURCES = main.c audio.c audio.h mp4read.c mp4read.h

EXTRA_faad_SOURCES =  getopt.c
