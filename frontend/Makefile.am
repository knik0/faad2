bin_PROGRAMS = faad

faad_SOURCES = main.c audio.c

CXXFLAGS = -O0
CFLAGS = -O0
INCLUDES = -I$(top_srcdir)/include
LDFLAGS =
LDADD = $(top_builddir)/libfaad/libfaad.la -lm -lsndfile -lmp4v2
