bin_PROGRAMS = faad
man1_MANS = faad.man

AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/common/mp4ff

faad_LDADD = $(top_builddir)/libfaad/libfaad.la \
	     $(top_builddir)/common/mp4ff/libmp4ff.a

faad_SOURCES = main.c audio.c audio.h
