#.rst:
# FindGLEW
# --------
#
# Find the OpenGL Extension Wrangler Library (GLEW)
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines the :prop_tgt:`IMPORTED` target ``GLEW::GLEW``,
# if GLEW has been found.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables:
#
# ::
#
#   GLEW_INCLUDE_DIRS - include directories for GLEW
#   GLEW_LIBRARIES - libraries to link against GLEW
#   GLEW_FOUND - true if GLEW has been found and can be used

#=============================================================================
# Copyright 2012 Benjamin Eikel
# Copyright 2015 filux <heross(@@)o2.pl>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

find_path(GLEW_INCLUDE_DIR GL/glew.h)

SET(GLEW_NAMES ${GLEW_NAMES} GLEW glew32 glew glew32s)
IF(STATIC_GLEW)
	SET(GLEW_NAMES libGLEW.a libglew32.a libglew.a libglew32s.a GLEW.a glew32.a glew.a glew32s.a ${GLEW_NAMES})
ENDIF()

find_library(GLEW_LIBRARY NAMES ${GLEW_NAMES} PATH_SUFFIXES lib64)

set(GLEW_INCLUDE_DIRS ${GLEW_INCLUDE_DIR})
set(GLEW_LIBRARIES ${GLEW_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLEW REQUIRED_VARS GLEW_INCLUDE_DIR GLEW_LIBRARY)

if(GLEW_FOUND AND NOT TARGET GLEW::GLEW)
  add_library(GLEW::GLEW UNKNOWN IMPORTED)
  set_target_properties(GLEW::GLEW PROPERTIES
    IMPORTED_LOCATION "${GLEW_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${GLEW_INCLUDE_DIRS}")

  message(STATUS "GLEW_LIBRARY: ${GLEW_LIBRARIES}")
endif()

mark_as_advanced(GLEW_INCLUDE_DIR GLEW_LIBRARY)
