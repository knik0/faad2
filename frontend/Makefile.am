bin_PROGRAMS = faad

faad_SOURCES = main.c audio.c

AM_CFLAGS = -O2
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/common/mp4v2
LDADD = $(top_builddir)/libfaad/libfaad.la $(top_builddir)/common/mp4v2/libmp4v2.la -lm

AM_CCLD = $(CXX)
