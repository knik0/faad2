libaac-XMMS.so:	aac-XMMS.c
	gcc -shared -o $@ aac-XMMS.c -I/usr/include/glib-1.2/ -I/usr/lib/glib/include -I/usr/include/gtk-1.2/ -lfaad2 -lid3 -lz

clean:
	rm -f libaac-XMMS.so aac-XMMS.o
