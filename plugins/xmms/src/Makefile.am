libdir = `$(XMMS_CONFIG) --input-plugin-dir`
lib_LTLIBRARIES = libmp4.la

libmp4_la_CFLAGS = `$(XMMS_CONFIG) --cflags` -Wall \
       -I$(top_srcdir)/include -I$(top_srcdir)/common/mp4ff

libmp4_la_LIBADD = $(top_builddir)/libfaad/libfaad.la \
       $(top_builddir)/common/mp4ff/libmp4ff.a

libmp4_la_LDFLAGS = -module -avoid-version `$(XMMS_CONFIG) --libs` -lpthread

libmp4_la_SOURCES = libmp4.c mp4_utils.c aac_utils.c
