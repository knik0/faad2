# Available CMake versions:
#  - Ubuntu 20.04 LTS    : 3.16.3
#  - Solaris 11.4 SRU 15 : 3.15
cmake_minimum_required(VERSION 3.15)

project(LIBFAAD2 LANGUAGES C)

# If FAAD is being bundled in another project, we don't want to
# install anything. However, we want to let people override this, so
# we'll use the FAAD_BUNDLED_MODE variable to let them do that; just
# set it to OFF in your project before you add_subdirectory(faad2).
get_directory_property(FAAD_PARENT_DIRECTORY PARENT_DIRECTORY)
if(NOT DEFINED FAAD_BUNDLED_MODE)
  # Bundled mode hasn't been set one way or the other, set the default
  # depending on whether or not we are the top-level project.
  if(FAAD_PARENT_DIRECTORY)
    set(FAAD_BUNDLED_MODE ON)
  else()
    set(FAAD_BUNDLED_MODE OFF)
  endif()
endif()
mark_as_advanced(FAAD_BUNDLED_MODE)

find_library(MATH_LIBRARY m)

include(GNUInstallDirs)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(CMAKE_MACOS_RPATH TRUE)
  set(CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")
endif()

# Extract version information

file(STRINGS "libfaad/version.h" _faad_version_line REGEX "^#define PACKAGE_VERSION .+$")
string(REGEX REPLACE "^#define PACKAGE_VERSION \"(.+)\"" "\\1" FAAD_VERSION "${_faad_version_line}")
message(STATUS "FAAD PACKAGE_VERSION: ${FAAD_VERSION}")
mark_as_advanced(FAAD_VERSION)

file(READ include/faad.h.in FAAD_H_SRC)
string(REGEX REPLACE "@VERSION@" "${FAAD_VERSION}" FAAD_H_SRC ${FAAD_H_SRC})
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/include/faad.h ${FAAD_H_SRC})

# Read sources list

file(GLOB_RECURSE LIBFAAD_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} libfaad/*.c libfaad/*.h)
mark_as_advanced(LIBFAAD_SOURCES)

file(GLOB_RECURSE FAAD_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} frontend/*.c frontend/*.h)
list(FILTER FAAD_SOURCES EXCLUDE REGEX ".*getopt\\.[ch]$")

# Build

# Lets stick to predefined config, until more flexibility is actually requested.
set(FAAD_DEFINES
  APPLY_DRC
  HAVE_INTTYPES_H=1
  HAVE_MEMCPY=1
  HAVE_STRINGS_H=1
  HAVE_SYS_STAT_H=1
  HAVE_SYS_TYPES_H=1
  STDC_HEADERS=1
)

set(FAAD_FLAGS
  -Wall
  -pedantic
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU")
  list(APPEND FAAD_FLAGS -ffloat-store)
endif()  # GCC

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/include/neaacdec.h
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include)

foreach(LIB faad faad_drm faad_fixed faad_drm_fixed)
  add_library(${LIB} ${LIBFAAD_SOURCES})
  if(MATH_LIBRARY)
    target_link_libraries(${LIB} PUBLIC ${MATH_LIBRARY})
  endif()
  target_include_directories(${LIB} PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/include
  )
  target_include_directories(${LIB} PRIVATE
    libfaad
  )
  target_compile_definitions(${LIB} PRIVATE
    ${FAAD_DEFINES}
  )
  target_compile_options(${LIB} PRIVATE
    ${FAAD_FLAGS}
  )
endforeach()

foreach(LIB faad_drm faad_drm_fixed)
target_compile_definitions(${LIB} PRIVATE
  DRM_SUPPORT
)
endforeach()

foreach(LIB faad_fixed faad_drm_fixed)
target_compile_definitions(${LIB} PRIVATE
  FIXED_POINT
)
endforeach()

# Generate a pkg-config files

function(generate_pkg_config_path outvar path)
  string(LENGTH "${path}" path_length)

  set(path_args ${ARGV})
  list(REMOVE_AT path_args 0 1)
  list(LENGTH path_args path_args_remaining)

  set("${outvar}" "${path}")

  while(path_args_remaining GREATER 1)
    list(GET path_args 0 name)
    list(GET path_args 1 value)

    get_filename_component(value_full "${value}" ABSOLUTE)
    string(LENGTH "${value}" value_length)

    if(path_length EQUAL value_length AND path STREQUAL value)
      set("${outvar}" "\${${name}}")
      break()
    elseif(path_length GREATER value_length)
      # We might be in a subdirectory of the value, but we have to be
      # careful about a prefix matching but not being a subdirectory
      # (for example, /usr/lib64 is not a subdirectory of /usr/lib).
      # We'll do this by making sure the next character is a directory
      # separator.
      string(SUBSTRING "${path}" ${value_length} 1 sep)
      if(sep STREQUAL "/")
        string(SUBSTRING "${path}" 0 ${value_length} s)
        if(s STREQUAL value)
          string(SUBSTRING "${path}" "${value_length}" -1 suffix)
          set("${outvar}" "\${${name}}${suffix}")
          break()
        endif()
      endif()
    endif()

    list(REMOVE_AT path_args 0 1)
    list(LENGTH path_args path_args_remaining)
  endwhile()

  set("${outvar}" "${${outvar}}" PARENT_SCOPE)
endfunction(generate_pkg_config_path)

function(transform_pc_file INPUT_FILE OUTPUT_FILE VERSION)
  file(READ ${INPUT_FILE} TEXT)

  set(PREFIX "${CMAKE_INSTALL_PREFIX}")
  string(REGEX REPLACE "@prefix@" "${PREFIX}" TEXT ${TEXT})
  string(REGEX REPLACE "@exec_prefix@" "\${prefix}" TEXT ${TEXT})

  generate_pkg_config_path(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}" exec_prefix "${PREFIX}")
  string(REGEX REPLACE "@libdir@" "${LIBDIR}" TEXT ${TEXT})

  generate_pkg_config_path(INCLUDEDIR "${CMAKE_INSTALL_FULL_INCLUDEDIR}" prefix "${PREFIX}")
  string(REGEX REPLACE "@includedir@" "${INCLUDEDIR}" TEXT ${TEXT})

  string(REGEX REPLACE "@VERSION@" "${VERSION}" TEXT ${TEXT})

  file(WRITE ${OUTPUT_FILE} ${TEXT})
endfunction()

transform_pc_file("libfaad/faad2.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/faad2.pc" "${FAAD_VERSION}")

# CLI

add_executable(faad_cli ${FAAD_SOURCES})
target_link_libraries(faad_cli faad)
target_compile_definitions(faad_cli PRIVATE ${FAAD_DEFINES})
set_target_properties(faad_cli PROPERTIES OUTPUT_NAME faad)

# Installation

if(NOT FAAD_BUNDLED_MODE)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/faad2.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

  install(
    TARGETS faad faad_cli faad_drm
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  )

  # exclude faad.h.in
  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/include/faad.h
    FILES ${CMAKE_CURRENT_BINARY_DIR}/include/neaacdec.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  )
endif()  # FAAD_BUNDLED_MODE
