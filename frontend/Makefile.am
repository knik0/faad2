bin_PROGRAMS = faad

faad_SOURCES = main.c audio.c $(top_srcdir)/common/faad/getopt.c \
   audio.h

AM_CFLAGS = -O2
if HAVE_MPEG4IP
einc=
elib=-lmp4v2
else
einc=-I$(top_srcdir)/common/mp4v2
elib=$(top_builddir)/common/mp4v2/libmp4v2.la
endif

INCLUDES = -I$(top_srcdir)/include $(einc) -I$(top_srcdir)/common/faad
LDADD = $(top_builddir)/libfaad/libfaad.la $(elib) -lm

CCLD = $(CXX)
