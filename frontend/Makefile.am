bin_PROGRAMS = faad

faad_SOURCES = main.c audio.c

CXXFLAGS = -O2
CFLAGS = -O2 -D_FILE_OFFSET_BITS=64
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/common/mp4v2
LDFLAGS =
LDADD = $(top_builddir)/libfaad/libfaad.la $(top_builddir)/common/mp4v2/libmp4v2.la  -lm -lsndfile 
