# This file was automatically generated
# DO NOT EDIT!

SUBDIRS = GSM610 G72x

lib_LTLIBRARIES = libsndfile.la
include_HEADERS = sndfile.h

noinst_HEADERS = common.h sfendian.h float_cast.h wav_w64.h au.h sf_command.h

COMMON = sndfile.c common.c file_io.c command.c pcm.c ulaw.c alaw.c float32.c \
			double64.c ima_adpcm.c ms_adpcm.c gsm610.c dwvw.c 

FILESPECIFIC = aiff.c au.c au_g72x.c nist.c paf.c raw.c samplitude.c svx.c \
				wav.c wav_w64.c voc.c ircam.c w64.c \
				sd2.c rx2.c txw.c
				
libsndfile_la_SOURCES = $(COMMON) $(FILESPECIFIC) sndfile.h
libsndfile_la_LDFLAGS = -version-info @SHARED_VERSION_INFO@
libsndfile_la_LIBADD  = GSM610/libgsm.la G72x/libg72x.la

# Disable autoheader. 
AUTOHEADER=echo
