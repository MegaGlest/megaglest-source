# Locate miniupnp library
# This module defines
#  MINIUPNP_FOUND, if false, do not try to link to miniupnp
#  MINIUPNP_LIBRARY, the miniupnp variant
#  MINIUPNP_INCLUDE_DIR, where to find miniupnpc.h and family)
#
# Note that the expected include convention is
#  #include "miniupnpc.h"
# and not
#  #include <miniupnpc/miniupnpc.h>
# This is because, the miniupnpc location is not standardized and may exist
# in locations other than miniupnpc/

#=============================================================================
# Copyright 2011 Mark Vejvoda
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distributed this file outside of CMake, substitute the full
#  License text for the above reference.)

if (MINIUPNP_INCLUDE_DIR AND MINIUPNP_LIBRARY)
  # Already in cache, be silent
  set(MINIUPNP_FIND_QUIETLY TRUE)
endif (MINIUPNP_INCLUDE_DIR AND MINIUPNP_LIBRARY)

find_path(MINIUPNP_INCLUDE_DIR miniupnpc.h
   PATH_SUFFIXES miniupnpc)
find_library(MINIUPNP_LIBRARY miniupnpc)

if (MINIUPNP_INCLUDE_DIR AND MINIUPNP_LIBRARY)
    set (MINIUPNP_FOUND TRUE)
endif ()

if (MINIUPNP_FOUND)
  if (NOT MINIUPNP_FIND_QUIETLY)
    message (STATUS "Found the miniupnpc libraries at ${MINIUPNP_LIBRARY}")
    message (STATUS "Found the miniupnpc headers at ${MINIUPNP_INCLUDE_DIR}")
  endif (NOT MINIUPNP_FIND_QUIETLY)
else ()
    message (STATUS "Could not find miniupnp")
endif ()

MARK_AS_ADVANCED(MINIUPNP_INCLUDE_DIR MINIUPNP_LIBRARY)

