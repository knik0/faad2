libdir = `xmms-config --input-plugin-dir`
lib_LTLIBRARIES = libaac.la

libaac_la_CFLAGS = `xmms-config --cflags` `$GTK_CONFIG --cflags` -Wall \
	-I$(top_builddir)/include 
	
libaac_la_LIBADD = $(top_builddir)/libfaad/libfaad.la

libaac_la_LDFLAGS = -module -avoid-version `xmms-config --libs` \
	`$GTK_CONFIG --libs` -lpthread -lz -lid3 -lstdc++
libaac_la_SOURCES = libaac.c id3.cpp dialog_gtk1.c aac_utils.c \
	aac_utils.h

