lib_LTLIBRARIES = libfaad.la
include_HEADERS = ../include/faad.h

libfaad_la_SOURCES = bits.c cfft.c data.c decoder.c dither.c drc.c \
error.c filtbank.c \
ic_predict.c is.c lt_predict.c mdct.c mp4.c ms.c output.c pns.c \
pulse.c specrec.c syntax.c tns.c hcr.c \
rvlc.c ssr.c ssr_fb.c ssr_ipqf.c common.c

libfaad_la_INCLUDES = analysis.h bits.h cfft.h cfft_tab.h common.h \
data.h decoder.h dither.h drc.h error.h fixed.h filtbank.h \
huffman.h ic_predict.h iq_table.h is.h kbd_win.h lt_predict.h mdct.h mp4.h \
ms.h output.h pns.h pulse.h rvlc.h sine_win.h ssr.h ssr_fb.h ssr_ipqf.h \
ssr_win.h specrec.h syntax.h tns.h codebook/hcb.h \
codebook/hcb_1.h codebook/hcb_2.h codebook/hcb_3.h codebook/hcb_4.h \
codebook/hcb_5.h codebook/hcb_6.h codebook/hcb_7.h codebook/hcb_8.h \
codebook/hcb_9.h codebook/hcb_10.h codebook/hcb_11.h codebook/hcb_sf.h


AM_CFLAGS = -O2
AM_LDFLAGS = -lm
