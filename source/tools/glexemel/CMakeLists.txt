# glexemel

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/source/tools/glexemel)
add_executable(g2xml g2xml.c g3dv4.h)

find_package(LibXml2)
IF(LIBXML2_FOUND)
  include_directories(${LIBXML2_INCLUDE_DIR})  
  add_executable(xml2g xml2g.c g3dv4.h)
  target_link_libraries(xml2g ${LIBXML2_LIBRARIES})
ELSE()
  MESSAGE(STATUS "WARNING... xml2g will NOT be built since we cannot find libXml2 on this machine")
ENDIF()
