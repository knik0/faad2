libdir = `xmms-config --input-plugin-dir`

lib_LTLIBRARIES = libmp4.la
libmp4_la_CFLAGS = -I$(top_builddir)/common/mp4v2 \
	-I$(top_builddir)/include \
	`xmms-config --cflags` -DHAVE_GLIB_H=1

libmp4_la_LDFLAGS = -module -avoid-version `xmms-config --libs` -lpthread -lstdc++ 

libmp4_la_LIBADD = $(top_builddir)/libfaad/libfaad.la $(top_builddir)/common/mp4v2/libmp4v2.la

libmp4_la_SOURCES = libmp4.c libmp4_utils.c libmp4_utils.h
