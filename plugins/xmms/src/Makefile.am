lib_LTLIBRARIES = libaac.la

if USE_STATIC_FAAD2
  INCLUDES = -I../../../include
  libaac_la_LDFLAGS = -module -avoid-version `xmms-config --libs` -lpthread -lid3 -lstdc++ -lz
  libaac_la_LIBADD = ../../../libfaad/libfaad.la
else
  libaac_la_LDFLAGS = -module -avoid-version `xmms-config --libs` -lpthread -lfaad -lz -lid3 -lstdc++
endif
libaac_la_SOURCES = libaac.c id3.cpp dialog_gtk1.c aac_utils.c
