# $Id: Makefile.am,v 1.1 2002/01/21 08:16:21 menno Exp $
# 
# Copyright (C) 1999 Scott Thomas Haug <sth2@cs.wustl.edu>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

LT_VERSION = $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

if ID3_NEEDZLIB
zlib_include = -I$(top_srcdir)/zlib/include
else
zlib_include = 
endif

INCLUDES = \
  @ID3LIB_DEBUG_FLAGS@ -I$(top_srcdir)/include/id3 -I$(top_srcdir)/include $(zlib_include)

id3lib_sources =                \
  c_wrapper.cpp                 \
  error.cpp                     \
  field.cpp                     \
  field_binary.cpp              \
  field_integer.cpp             \
  field_string_ascii.cpp        \
  field_string_unicode.cpp      \
  frame.cpp                     \
  frame_parse.cpp               \
  frame_render.cpp              \
  globals.cpp                   \
  header.cpp                    \
  header_frame.cpp              \
  header_tag.cpp                \
  misc_support.cpp              \
  spec.cpp                      \
  tag.cpp                       \
  tag_file.cpp                  \
  tag_find.cpp                  \
  tag_parse.cpp                 \
  tag_parse_lyrics3.cpp         \
  tag_parse_v1.cpp              \
  tag_render.cpp                \
  tag_sync.cpp                  \
  uint28.cpp                    \
  utils.cpp                     

lib_LTLIBRARIES   = libid3.la 

libid3_la_SOURCES = $(id3lib_sources)
if ID3_NEEDZLIB
LDADD        = $(top_builddir)/zlib/src/libz.la
else
libid3_la_LIBADD  = -lz
endif
libid3_la_LDFLAGS = \
  -version-info $(LT_VERSION) \
  -release $(LT_RELEASE) \
  -export-dynamic
