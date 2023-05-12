lib_LTLIBRARIES = libfaad.la libfaad_drm.la

AM_CPPFLAGS	= -iquote $(top_srcdir)/include
include_HEADERS = $(top_srcdir)/include/faad.h \
		  $(top_srcdir)/include/neaacdec.h

libfaad_la_LDFLAGS = -version-info 2:0:0 -no-undefined
libfaad_la_LIBADD = -lm
libfaad_la_CFLAGS = -fvisibility=hidden

include sources.lst

libfaad_la_SOURCES = ${LIBFAAD_SOURCES}

libfaad_drm_la_LDFLAGS = ${libfaad_la_LDFLAGS}
libfaad_drm_la_LIBADD = ${libfaad_la_LIBADD}
libfaad_drm_la_CFLAGS = ${libfaad_la_CFLAGS} -DDRM_SUPPORT
libfaad_drm_la_SOURCES = ${LIBFAAD_SOURCES}

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = faad2.pc

EXTRA_DIST = faad2.pc.in
