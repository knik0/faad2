libdir = `xmms-config --input-plugin-dir`
lib_LTLIBRARIES = libmp4.la

INCLUDES = -I../../../common/mp4v2 -DHAVE_GLIB_H=1 -I../../../

libaac_la_CFLAGS = `xmms-config --cflags` -Wall -I$(top_srcdir)/include

libaac_la_LDFLAGS = -module -avoid-version `xmms-config --libs` -lfaad -lpthread -lstdc++ -lmp4v2
libaac_la_SOURCES = libmp4.c mp4_utils.c aac_utils.c
