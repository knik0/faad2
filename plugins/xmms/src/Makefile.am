CFLAGS = @CFLAGS@ -Wall
bin_PROGRAMS = libaac.so

INCLUDES = `xmms-config --cflags` -I@FAAD2_DIR@/include -I@ID3_DIR@/include
libaac_so_LDFLAGS = -shared -L@FAAD2_DIR@/lib -L@ID3_DIR@/lib `xmms-config --libs` -lpthread -lfaad -lz

libaac_so_SOURCES = libaac.c id3.cpp
