# glexemel

IF(BUILD_MEGAGLEST_MODEL_IMPORT_EXPORT_TOOLS)
	MESSAGE(STATUS "Will try to build MegaGlest model import/export tools")

	SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/source/tools/glexemel)
	add_executable(g2xml g2xml.c g3dv4.h)

	IF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
	     SET(CMAKE_INCLUDE_PATH /usr/local/include)
	ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")

	find_package(LibXml2)
	IF(LIBXML2_FOUND)
	  include_directories(${CMAKE_INCLUDE_PATH} ${LIBXML2_INCLUDE_DIR})  
	  add_executable(xml2g xml2g.c g3dv4.h)
	  target_link_libraries(xml2g ${LIBXML2_LIBRARIES})
	ELSE()
	  MESSAGE(STATUS "WARNING... xml2g will NOT be built since we cannot find libXml2 on this machine")
	ENDIF()
ENDIF()
