# common libraries

SET(EXTERNAL_LIBS "")
SET(TARGET_NAME "megaglest_g3dviewer")
SET(TARGET_NAME_MANPAGE "${TARGET_NAME}.6")

IF(BUILD_MEGAGLEST_MODEL_VIEWER)
	MESSAGE(STATUS "Will try to build MegaGlest model viewer")

	IF(UNIX AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
		IF("${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS 4.7 OR NOT "${CMAKE_CXX_COMPILER_VERSION}")
			ADD_DEFINITIONS("-std=gnu++0x")
		ELSEIF("${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS 5.1)
			ADD_DEFINITIONS("-std=c++11")
		ENDIF()

		# Was required at least on bsd and macos.
	ENDIF()

	IF(NOT VCPKG_TARGET_TRIPLET)
		FIND_PACKAGE(${SDL_VERSION_NAME} REQUIRED)
	ELSE()
		FIND_PACKAGE(SDL2 CONFIG REQUIRED)
	ENDIF()

	INCLUDE_DIRECTORIES(${${SDL_VERSION_NAME}_INCLUDE_DIR})
	SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${${SDL_VERSION_NAME}_LIBRARY})

	FIND_PACKAGE(OpenGL REQUIRED)
	INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})
	SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${OPENGL_LIBRARY})

	FIND_PACKAGE(GLEW REQUIRED)
	INCLUDE_DIRECTORIES(${GLEW_INCLUDE_DIRS})

	IF(UNIX)
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${GLEW_LIBRARIES})
	ENDIF()

	IF(WANT_USE_XercesC)
		FIND_PACKAGE(XercesC REQUIRED)
		INCLUDE_DIRECTORIES(${XERCESC_INCLUDE})

		IF(UNIX)
			SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${XERCESC_LIBRARY})
		ENDIF()
	ENDIF()

	IF(STATIC_wxWidgets)
		SET(wxWidgets_USE_STATIC ON)

		# ^ This is not needed here (once, globally is enough) if every find run returns the same status.
		# With wx this is not so 100% sure.
	ENDIF()

	# It was noticed that when using MinGW gcc it is essential that 'core' is mentioned before 'base'.
	# Optimal order most likely is gl > core > base, in some cases it may do difference.
	FIND_PACKAGE(wxWidgets REQUIRED COMPONENTS gl core base)

	IF(UNIX OR WIN32)
		# wxWidgets include (this will do all the magic to configure everything)
		INCLUDE(${wxWidgets_USE_FILE})

		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${wxWidgets_LIBRARIES})

		IF(STATIC_wxWidgets)
			IF(wxWidgets_USE_STATIC)
				MESSAGE(STATUS "==========> wxWidgets using STATIC libs:")
			ELSE()
				MESSAGE(STATUS "==========> wxWidgets STATIC libs NOT found:")
			ENDIF()
		ENDIF()

		MESSAGE(STATUS " wxWidgets: ${wxWidgets_INCLUDE_DIRS} ;/;  ${wxWidgets_LIBRARIES}")
	ENDIF()

	IF(NOT MSVC)
		find_package(PkgConfig REQUIRED)
	ENDIF()

	IF(FORCE_STREFLOP_SOFTWRAPPER)
		pkg_search_module(STREFLOP streflop-soft)
	ELSE()
		IF(HAS_SSE_EXTENSIONS AND NOT ${FORCE_MAX_SSE_LEVEL} MATCHES "0")
			pkg_search_module(STREFLOP streflop-sse)
		ELSE()
			IF(HAS_X87_SUPPORT)
				pkg_search_module(STREFLOP streflop-x87)
			ELSE()
				pkg_search_module(STREFLOP streflop-soft)
			ENDIF()
		ENDIF()
	ENDIF()

	IF(NOT STREFLOP_FOUND)
		pkg_search_module(STREFLOP streflop)
	ENDIF()

	MESSAGE(STATUS "Search for Library STREFLOP result = ${STREFLOP_FOUND} libs: ${STREFLOP_LIBRARIES} include dirs: ${STREFLOP_INCLUDE_DIRS}")

	IF(FORCE_EMBEDDED_LIBS)
		SET(STREFLOP_FOUND OFF)
		MESSAGE(STATUS "FORCING USE of EMBEDDED Libraries...")
	ENDIF()

	IF(WANT_USE_STREFLOP)
		IF(STREFLOP_FOUND)
			INCLUDE_DIRECTORIES(${STREFLOP_INCLUDE_DIRS} ${STREFLOP_INCLUDE_DIRS}/streflop)
			SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${STREFLOP_LIBRARIES})

			ADD_DEFINITIONS("-DUSE_STREFLOP_PKG")
		ENDIF()
	ENDIF()

	IF(WIN32)
		IF(NOT MSVC)
			SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${PROJECT_SOURCE_DIR}/source/win32_deps/wxWidgets-2.8.10/lib/libwx_mswu-2.8-i586-mingw32msvc.dll.a)
			SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${PROJECT_SOURCE_DIR}/source/win32_deps/wxWidgets-2.8.10/lib/libwx_mswu_gl-2.8-i586-mingw32msvc.dll.a)
			SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${PROJECT_SOURCE_DIR}/source/win32_deps/wxWidgets-2.8.10/lib/libwxpng-2.8-i586-mingw32msvc.a)
			SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${PROJECT_SOURCE_DIR}/source/win32_deps/wxWidgets-2.8.10/lib/libwxjpeg-2.8-i586-mingw32msvc.a)
			SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${PROJECT_SOURCE_DIR}/source/win32_deps/wxWidgets-2.8.10/lib/libwxtiff-2.8-i586-mingw32msvc.a)
			SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${PROJECT_SOURCE_DIR}/source/win32_deps/wxWidgets-2.8.10/lib/libwxzlib-2.8-i586-mingw32msvc.a)

			INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/wxWidgets-2.8.10/include/)
			INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/wxWidgets-2.8.10/lib/wx/include/i586-mingw32msvc-msw-unicode-release-static-2.8)

			SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH}
				${PROJECT_SOURCE_DIR}/source/win32_deps/lib
				${PROJECT_SOURCE_DIR}/source/win32_deps/xerces-c-src_2_8_0/lib)

			INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/include)
			INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/openal-soft-1.12.854/include)
			INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/openal-soft-1.12.854)
			INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/xerces-c-src_2_8_0/include)
			INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/libogg-1.2.1/include)
			INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/lua-5.1/src)
			INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/jpeg-8b)
			INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/lpng141)
			INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/zlib-1.2.5)
			INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/curl-7.21.3/include)
			INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/${SDL_WINDOWS_DIR_DINC}/include)

			link_directories(${PROJECT_SOURCE_DIR}/source/win32_deps/lib)
			link_directories(${PROJECT_SOURCE_DIR}/source/win32_deps/xerces-c-src_2_8_0/lib)
		ENDIF()
	ENDIF()

	# ########################################################################################
	# megaglest G3D Model Viewer
	SET(DIRS_WITH_SRC
		.
	)

	SET(GLEST_MAIN_INCLUDE_ROOT "../glest_game/")
	SET(GLEST_LIB_INCLUDE_ROOT "../shared_lib/include/")

	SET(GLEST_LIB_INCLUDE_DIRS
		${GLEST_LIB_INCLUDE_ROOT}platform/common

		# ${GLEST_LIB_INCLUDE_ROOT}platform/${SDL_VERSION_SNAME}
		${GLEST_LIB_INCLUDE_ROOT}platform/posix
		${GLEST_LIB_INCLUDE_ROOT}util
		${GLEST_LIB_INCLUDE_ROOT}graphics
		${GLEST_LIB_INCLUDE_ROOT}graphics/gl
		${GLEST_LIB_INCLUDE_ROOT}map
		${GLEST_LIB_INCLUDE_ROOT}sound
		${GLEST_LIB_INCLUDE_ROOT}xml
		${GLEST_LIB_INCLUDE_ROOT}xml/rapidxml
		${GLEST_MAIN_INCLUDE_ROOT}facilities
		${GLEST_MAIN_INCLUDE_ROOT}graphics
		${GLEST_MAIN_INCLUDE_ROOT}game
		${GLEST_MAIN_INCLUDE_ROOT}global
		${GLEST_MAIN_INCLUDE_ROOT}sound)

	IF(WANT_USE_STREFLOP AND NOT STREFLOP_FOUND)
		SET(GLEST_LIB_INCLUDE_DIRS
			${GLEST_LIB_INCLUDE_DIRS}
			${GLEST_LIB_INCLUDE_ROOT}streflop)
	ENDIF()

	INCLUDE_DIRECTORIES(${GLEST_LIB_INCLUDE_DIRS})

	IF(WIN32)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/shared_lib/include/platform/win32)
	ENDIF()

	INCLUDE_DIRECTORIES(${GLEST_LIB_INCLUDE_ROOT}platform/${SDL_VERSION_SNAME})

	SET(MG_SOURCE_FILES "../glest_game/graphics/particle_type.cpp" "../glest_game/graphics/unit_particle_type.cpp" "../glest_game/global/config.cpp")

	FOREACH(DIR IN LISTS DIRS_WITH_SRC)
		INCLUDE_DIRECTORIES(${DIR})

		set(SRC_DIR_TO_GLOB ${DIR})
		FILE(GLOB SRC_FILES_FROM_THIS_DIR ${SRC_DIR_TO_GLOB}/*.cpp)
		SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${SRC_FILES_FROM_THIS_DIR})
		FILE(GLOB INCLUDE_FILES_FROM_THIS_DIR ${SRC_DIR_TO_GLOB}/*.h)
		SET(MG_INCLUDE_FILES ${MG_INCLUDE_FILES} ${INCLUDE_FILES_FROM_THIS_DIR})

		IF(APPLE)
			FILE(GLOB SRC_FILES_FROM_THIS_DIR ${SRC_DIR_TO_GLOB}/*.mm)

			# MESSAGE(STATUS "SRC_FILES_FROM_THIS_DIR: ${SRC_FILES_FROM_THIS_DIR}")
			SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${SRC_FILES_FROM_THIS_DIR})
			FILE(GLOB SRC_FILES_FROM_THIS_DIR ${SRC_DIR_TO_GLOB}/*.m)

			# MESSAGE(STATUS "SRC_FILES_FROM_THIS_DIR: ${SRC_FILES_FROM_THIS_DIR}")
			SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${SRC_FILES_FROM_THIS_DIR})
		ENDIF(APPLE)
	ENDFOREACH(DIR)

	IF(WANT_DEV_OUTPATH)
		SET(EXECUTABLE_OUTPUT_PATH "${MEGAGLEST_FRIENDLY_OUTPUT_PATH}")
	ENDIF()

	SET(HELP2MAN_OUT_PATH ${EXECUTABLE_OUTPUT_PATH})

	IF("${EXECUTABLE_OUTPUT_PATH}" STREQUAL "")
		SET(HELP2MAN_OUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/")
	ENDIF()

	MESSAGE(STATUS "***Note: The final output path for MegaGlest G3d Viewer will be [${HELP2MAN_OUT_PATH}]")

	FIND_PROGRAM(HELP2MAN "help2man")

	SET_SOURCE_FILES_PROPERTIES(${MG_INCLUDE_FILES} PROPERTIES HEADER_FILE_ONLY 1)

	ADD_EXECUTABLE(${TARGET_NAME} ${MG_SOURCE_FILES} ${MG_INCLUDE_FILES})

	IF(WIN32)
		# Must come after ADD_EXECUTABLE.
		TARGET_SOURCES(${TARGET_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/mk/windoze/g3d_viewer.rc)
	ENDIF()

	IF(WANT_USE_STREFLOP AND NOT STREFLOP_FOUND)
		TARGET_LINK_LIBRARIES(${TARGET_NAME} streflop)
	ENDIF()

	TARGET_LINK_LIBRARIES(${TARGET_NAME} libmegaglest)
	TARGET_LINK_LIBRARIES(${TARGET_NAME} ${EXTERNAL_LIBS})

	IF(EXISTS ${HELP2MAN} AND NOT APPLE)
		# on mac <> --help sometimes hangs, so we need "NOT APPLE" ^ or some timeout here for now
		MESSAGE(STATUS "***-- Found help2man: ${HELP2MAN} about to build manpage in [${HELP2MAN_OUT_PATH}${TARGET_NAME_MANPAGE}]")

		SET(MG_MANPAGE_DESCRIPTION "G3D Model editor for the popular 3D realtime strategy game")

		IF(EXISTS ${XVFB_EXEC})
			MESSAGE(STATUS "***-- Found xvfb-run: ${XVFB_EXEC} about to build manpage with it.")

			add_custom_target(${TARGET_NAME_MANPAGE} ALL
				COMMAND ${XVFB_EXEC} --auto-servernum --server-num=770 ${HELP2MAN} --no-discard-stderr --name="${MG_MANPAGE_DESCRIPTION}" --source="${TARGET_NAME}" --version-string="${TARGET_NAME}" --section=6 -N -o ${HELP2MAN_OUT_PATH}${TARGET_NAME_MANPAGE} ${HELP2MAN_OUT_PATH}${TARGET_NAME}
				DEPENDS ${TARGET_NAME})
		ELSE()
			add_custom_target(${TARGET_NAME_MANPAGE} ALL
				COMMAND ${HELP2MAN} --no-discard-stderr --name="${MG_MANPAGE_DESCRIPTION}" --source="${TARGET_NAME}" --version-string="${TARGET_NAME}" --section=6 -N -o ${HELP2MAN_OUT_PATH}${TARGET_NAME_MANPAGE} ${HELP2MAN_OUT_PATH}${TARGET_NAME}
				DEPENDS ${TARGET_NAME})
		ENDIF()
	ENDIF()

	# Requires an install prefix for the items below to work
	IF(NOT CMAKE_INSTALL_PREFIX STREQUAL "")
		# Installation of the program
		INSTALL(TARGETS
			${TARGET_NAME}
			DESTINATION ${MEGAGLEST_BIN_INSTALL_PATH})

		IF(EXISTS ${HELP2MAN} AND NOT "${MEGAGLEST_MANPAGE_INSTALL_PATH}" STREQUAL "" AND NOT APPLE)
			MESSAGE(STATUS "**NOTE: Will Install manpage [${TARGET_NAME_MANPAGE}] to [${MEGAGLEST_MANPAGE_INSTALL_PATH}]")

			# Installation of the program manpage file
			INSTALL(FILES
				"${HELP2MAN_OUT_PATH}${TARGET_NAME_MANPAGE}"
				DESTINATION ${MEGAGLEST_MANPAGE_INSTALL_PATH})
		ENDIF()

		# Install ini (usually installed with game bin, but in case only viewer is built.)
		IF(WIN32)
			INSTALL(FILES
				"${PROJECT_SOURCE_DIR}/mk/windoze/glest.ini"
				DESTINATION ${MEGAGLEST_INI_INSTALL_PATH})
		ELSEIF(UNIX AND NOT APPLE)
			INSTALL(FILES
				"${PROJECT_SOURCE_DIR}/mk/linux/glest.ini"
				DESTINATION ${MEGAGLEST_INI_INSTALL_PATH})
		ELSEIF(UNIX AND APPLE)
			INSTALL(FILES
				"${PROJECT_SOURCE_DIR}/mk/macos/glest.ini"
				DESTINATION ${MEGAGLEST_INI_INSTALL_PATH})
		ENDIF()

		# Installation of the program config and image files
		INSTALL(FILES
			"${PROJECT_SOURCE_DIR}/mk/shared/g3dviewer.ico"
			DESTINATION ${MEGAGLEST_INI_INSTALL_PATH})
	ENDIF()
ENDIF()
