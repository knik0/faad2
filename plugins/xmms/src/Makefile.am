libdir = `xmms-config --input-plugin-dir`
lib_LTLIBRARIES = libmp4.la

INCLUDES = -I../../../common/mp4v2 -DHAVE_GLIB_H=1 -I../../../

libmp4_la_CFLAGS = `xmms-config --cflags` -Wall -I$(top_srcdir)/include

libmp4_la_LDFLAGS = -module -avoid-version `xmms-config --libs` -lfaad -lpthread -lstdc++ -lmp4v2
libmp4_la_SOURCES = libmp4.c mp4_utils.c aac_utils.c
