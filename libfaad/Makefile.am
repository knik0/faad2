lib_LTLIBRARIES = libfaad.la
include_HEADERS = ../include/faad.h

VPATH = @srcdir@:codebook:

libfaad_la_SOURCES = bits.c cfft.c data.c decoder.c dither.c drc.c \
error.c filtbank.c \
ic_predict.c is.c lt_predict.c mdct.c mp4.c ms.c output.c pns.c \
pulse.c specrec.c syntax.c tns.c hcr.c \
rvlc.c ssr.c ssr_fb.c ssr_ipqf.c common.c \
hcb_1.c hcb_2.c hcb_3.c hcb_4.c hcb_5.c hcb_6.c hcb_7.c hcb_8.c \
hcb_9.c hcb_10.c hcb_11.c hcb_sf.c

libfaad_la_INCLUDES = analysis.h bits.h cfft.h cfft_tab.h common.h \
data.h decoder.h dither.h drc.h error.h fixed.h filtbank.h \
huffman.h ic_predict.h iq_table.h is.h kbd_win.h lt_predict.h mdct.h mp4.h \
ms.h output.h pns.h pulse.h rvlc.h sine_win.h ssr.h ssr_fb.h ssr_ipqf.h\
ssr_win.h specrec.h syntax.h tns.h codebook/hcb.h


CFLAGS = -O2
