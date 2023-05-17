#!/bin/sh

set -x

# FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
# Copyright (C) 2003-2005 M. Bakker, Nero AG, http://www.nero.com
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# Any non-GPL usage of this software or parts of this software is strictly
# forbidden.
#
# The "appropriate copyright message" mentioned in section 2c of the GPLv2
# must read: "Code from FAAD2 is copyright (c) Nero AG, www.nero.com"
#
# Commercial non-GPL licensing of this software is possible.
# For more info contact Nero AG through Mpeg4AAClicense@nero.com.

# ASAN:
export SANITIZER=address
export SANITIZER_FLAGS="-fsanitize=$SANITIZER -fsanitize-address-use-after-scope"
# MSAN:
#export SANITIZER=memory
#export SANITIZER_FLAGS="-fsanitize=$SANITIZER -fsanitize-memory-track-origins=2"
# UBSAN:
#export SANITIZER=array-bounds,bool,builtin,enum,float-divide-by-zero,function,integer-divide-by-zero,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,unsigned-integer-overflow,unreachable,vla-bound,vptr
#export SANITIZER_FLAGS="-fsanitize=$SANITIZER -fno-sanitize-recover=$SANITIZER -fsanitize-recover=unsigned-integer-overflow"

export CC="clang"
export CXX="clang++"
export BASE_FLAGS="-O1 -fno-omit-frame-pointer -gline-tables-only -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION $SANITIZER_FLAGS -fsanitize=fuzzer-no-link"
export CFLAGS="${BASE_FLAGS}"
export LDFLAGS="${BASE_FLAGS}"
export CXXFLAGS="${BASE_FLAGS} -stdlib=libc++"

./bootstrap
./configure
cd libfaad
make clean -j `nproc`
make -j `nproc`
cd ../

function build_fuzzer () {
  local fname=$1
  local affix=$2
  local extra_flags=""
  if [[ "$affix" == "_drm" ]]; then
    extra_flags="-DDRM_SUPPORT"
  fi
  $CC $CFLAGS $extra_flags -fsanitize=fuzzer,$SANITIZER -I./include ./fuzz/fuzz_${fname}.c -o ./fuzz/fuzz${affix}_${fname} ./libfaad/.libs/libfaad${affix}.a
}

for fname in "config" "decode"; do
  for affix in "" "_drm"; do
    build_fuzzer $fname $affix
  done
done
