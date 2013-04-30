#
# http://root.cern.ch/viewvc/trunk/cint/reflex/cmake/modules/FindCppUnit.cmake
#
# - Find CppUnit
#
# This module finds the CppUnit include directory and library
#
# It sets the following variables:
#  CPPUNIT_FOUND       - Set to false, or undefined, if CppUnit isn't found.
#  CPPUNIT_INCLUDE_DIR - The CppUnit include directory.
#  CPPUNIT_LIBRARY     - The CppUnit library to link against.
	
set (_cppunit_DEBUG false)

# If CPPUNIT_ROOT was defined in the environment, use it.

if (NOT CPPUNIT_ROOT AND NOT $ENV{CPPUNIT_ROOT} STREQUAL "")
  set(CPPUNIT_ROOT $ENV{CPPUNIT_ROOT})
endif(NOT CPPUNIT_ROOT AND NOT $ENV{CPPUNIT_ROOT} STREQUAL "")

# If CPPUNIT_INCLUDEDIR was defined in the environment, use it.
if( NOT $ENV{CPPUNIT_INCLUDEDIR} STREQUAL "" )
  set(CPPUNIT_INCLUDEDIR $ENV{CPPUNIT_INCLUDEDIR})
endif( NOT $ENV{CPPUNIT_INCLUDEDIR} STREQUAL "" )

# If CPPUNIT_LIBRARYDIR was defined in the environment, use it.
if( NOT $ENV{CPPUNIT_LIBRARYDIR} STREQUAL "" )
  set(CPPUNIT_LIBRARYDIR $ENV{CPPUNIT_LIBRARYDIR})
endif( NOT $ENV{CPPUNIT_LIBRARYDIR} STREQUAL "" )

if( CPPUNIT_ROOT )
  set(_cppunit_INCLUDE_SEARCH_DIRS
    ${CPPUNIT_ROOT}/include )
  set(_cppunit_LIBRARY_SEARCH_DIRS
    ${CPPUNIT_ROOT}/lib )
endif( CPPUNIT_ROOT )

if( CPPUNIT_INCLUDEDIR )
  file(TO_CMAKE_PATH ${CPPUNIT_INCLUDEDIR} CPPUNIT_INCLUDEDIR)
  SET(_cppunit_INCLUDE_SEARCH_DIRS
    ${CPPUNIT_INCLUDEDIR} )
endif( CPPUNIT_INCLUDEDIR )

if( CPPUNIT_LIBRARYDIR )
  file(TO_CMAKE_PATH ${CPPUNIT_LIBRARYDIR} CPPUNIT_LIBRARYDIR)
  SET(_cppunit_LIBRARY_SEARCH_DIRS
    ${CPPUNIT_LIBRARYDIR} )
endif( CPPUNIT_LIBRARYDIR )

# now find CPPUNIT_INCLUDE_DIR

if ( _cppunit_DEBUG)
  message(STATUS "search include dirs for cppunit = ${_cppunit_INCLUDE_SEARCH_DIRS}")
endif ( _cppunit_DEBUG)

find_path(CPPUNIT_INCLUDE_DIR 
          NAMES cppunit/Test.h
          HINTS ${_cppunit_INCLUDE_SEARCH_DIRS})

if ( _cppunit_DEBUG)
  message(STATUS "include dir for cppunit = ${CPPUNIT_INCLUDE_DIR}")
endif ( _cppunit_DEBUG)

# now find CPPUNIT_LIBRARY

if ( _cppunit_DEBUG)
  message(STATUS "search library dirs for cppunit = ${_cppunit_LIBRARY_SEARCH_DIRS}")
endif ( _cppunit_DEBUG)

find_library(CPPUNIT_LIBRARY 
             NAMES cppunit
             HINTS ${_cppunit_LIBRARY_SEARCH_DIRS})

if ( _cppunit_DEBUG)
  message(STATUS "library dir for cppunit = ${CPPUNIT_LIBRARY}")
endif ( _cppunit_DEBUG)

if (CPPUNIT_INCLUDE_DIR AND CPPUNIT_LIBRARY)
   SET(CPPUNIT_FOUND TRUE)
endif (CPPUNIT_INCLUDE_DIR AND CPPUNIT_LIBRARY)

if (CPPUNIT_FOUND)
   # show which CppUnit was found only if not quiet
   if (NOT CppUnit_FIND_QUIETLY)
      message(STATUS "Found CppUnit: ${CPPUNIT_LIBRARY}")
   endif (NOT CppUnit_FIND_QUIETLY)
else (CPPUNIT_FOUND)
   # fatal error if CppUnit is required but not found
   if (CppUnit_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find CppUnit")
   endif (CppUnit_FIND_REQUIRED)
endif (CPPUNIT_FOUND)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CPPUNIT  DEFAULT_MSG  CPPUNIT_LIBRARY CPPUNIT_INCLUDE_DIR)

mark_as_advanced(CPPUNIT_INCLUDE_DIR CPPUNIT_LIBRARY )
