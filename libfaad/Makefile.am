lib_LTLIBRARIES = libfaad.la
include_HEADERS = ../include/faad.h

VPATH = @srcdir@:codebook:

libfaad_la_SOURCES = bits.c cfft.c data.c decoder.c drc.c error.c filtbank.c \
ic_predict.c is.c lt_predict.c mdct.c mp4.c ms.c output.c pns.c \
pulse.c specrec.c syntax.c tns.c reordered_spectral_data.c \
hcb_1.c hcb_2.c hcb_3.c hcb_4.c hcb_5.c hcb_6.c hcb_7.c hcb_8.c \
hcb_9.c hcb_10.c hcb_11.c hcb_sf.c

libfaad_la_INCLUDES = analysis.h bits.h cfft.h data.h decoder.h drc.h error.h \
filtbank.h huffman.h ic_predict.h is.h kbd_win.h lt_predict.h mdct.h mp4.h \
ms.h output.h pns.h pulse.h specrec.h syntax.h tns.h \
codebook/hcb.h
libfaad_la_LIBADD = fftw/libfftw.la

CFLAGS = -O2
INCLUDES = -I$(top_srcdir)/include -Ifftw

SUBDIRS = fftw
