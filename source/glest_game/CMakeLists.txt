#########################################################################################
# common libraries

SET(EXTERNAL_LIBS "")
SET(TARGET_NAME "megaglest")
SET(TARGET_NAME_MANPAGE "${TARGET_NAME}.6")

IF(BUILD_MEGAGLEST)
	MESSAGE(STATUS "Will try to build MegaGlest game")

	IF(WIN32)
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
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/SDL-1.2.x/include)
				       
		link_directories(${PROJECT_SOURCE_DIR}/source/win32_deps/lib)
		link_directories(${PROJECT_SOURCE_DIR}/source/win32_deps/xerces-c-src_2_8_0/lib)
	ENDIF()

	FIND_PACKAGE(SDL REQUIRED)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
	IF(UNIX)
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${SDL_LIBRARY})
	ENDIF()

	FIND_PACKAGE(OpenGL REQUIRED)
	INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})
	IF(UNIX)
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${OPENGL_LIBRARY})
	ENDIF()

	FIND_PACKAGE(XercesC REQUIRED)
	INCLUDE_DIRECTORIES(${XERCESC_INCLUDE})
	IF(UNIX)
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${XERCESC_LIBRARY})
	ENDIF()

	FIND_PACKAGE(OpenAL REQUIRED)
	INCLUDE_DIRECTORIES(${OPENAL_INCLUDE_DIR})
	IF(UNIX)
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${OPENAL_LIBRARY})
	ENDIF()

	FIND_PACKAGE(OGG REQUIRED)
	INCLUDE_DIRECTORIES(${OGG_INCLUDE_DIR})
	IF(UNIX OR APPLE)
	#	SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${OGG_LIBRARY})
	#	SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${VORBIS_LIBRARY})
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${VORBIS_FILE_LIBRARY})
	ENDIF()

	FIND_PACKAGE(LUA REQUIRED)
	INCLUDE_DIRECTORIES(${LUA_INCLUDE_DIR})
	IF(UNIX)
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${LUA_LIBRARIES})
	ENDIF()

	FIND_PACKAGE(JPEG REQUIRED)
	INCLUDE_DIRECTORIES(${JPEG_INCLUDE_DIR})
	IF(UNIX)
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${JPEG_LIBRARY})
	ENDIF()

	OPTION(WANT_STATIC_LIBS "builds as many static libs as possible" OFF)
	IF(WANT_STATIC_LIBS)
		SET(PNG_NAMES libpng.a ${PNG_NAMES}) 
	ENDIF()
	FIND_PACKAGE(PNG REQUIRED)
	INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIR})
	IF(UNIX)
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${PNG_LIBRARY})
	ENDIF()

	FIND_PACKAGE(CURL REQUIRED)
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRS})
	IF(UNIX)
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${CURL_LIBRARIES})
	ENDIF()

	FIND_PACKAGE(GLEW REQUIRED)
	INCLUDE_DIRECTORIES(${GLEW_INCLUDE_PATH})
	IF(UNIX)
		SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${GLEW_LIBRARY})
	ENDIF()

        #SET(LIBVLC_MIN_VERSION "2.0.1")
	FIND_PACKAGE(LIBVLC)

        IF(LIBVLC_FOUND AND NOT LIBVLC_VERSION STRLESS "${LIBVLC_MIN_VERSION}")
                MESSAGE(STATUS "**NOTE: LIBVLC found so intro videos are supported.")
	        INCLUDE_DIRECTORIES(${LIBVLC_INCLUDE_DIR})
	        IF(UNIX)
		        SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${LIBVLCCORE_LIBRARY} ${LIBVLC_LIBRARY})
                        ADD_DEFINITIONS(-DHAS_LIBVLC)
	        ENDIF()
		IF(LIBVLC_VERSION_PRE_V2)
			ADD_DEFINITIONS(-DLIBVLC_VERSION_PRE_2)
		ENDIF()
		IF(LIBVLC_VERSION_PRE_1_1_13)
			ADD_DEFINITIONS(-DLIBVLC_VERSION_PRE_1_1_13)
		ENDIF()
		
        ELSE()
                MESSAGE(STATUS "**WARNING: LIBVLC NOT found so intro videos are NOT supported.")
        ENDIF()

	#########################################################################################
	# megaglest game

	SET(DIRS_WITH_SRC
	    	ai
		facilities
		game
		global
		graphics
		gui
		main
		menu
		network
		sound
		type_instances
		types
		world)

	SET(GLEST_LIB_INCLUDE_ROOT "../shared_lib/include/")
	SET(GLEST_LIB_INCLUDE_DIRS
	    ${GLEST_LIB_INCLUDE_ROOT}platform/common
		${GLEST_LIB_INCLUDE_ROOT}platform/posix
		${GLEST_LIB_INCLUDE_ROOT}util
		${GLEST_LIB_INCLUDE_ROOT}graphics
		${GLEST_LIB_INCLUDE_ROOT}graphics/gl
		${GLEST_LIB_INCLUDE_ROOT}graphics/iqm
		${GLEST_LIB_INCLUDE_ROOT}sound
		${GLEST_LIB_INCLUDE_ROOT}sound/openal
		${GLEST_LIB_INCLUDE_ROOT}xml
                ${GLEST_LIB_INCLUDE_ROOT}xml/rapidxml
		${GLEST_LIB_INCLUDE_ROOT}glew
		${GLEST_LIB_INCLUDE_ROOT}lua
		${GLEST_LIB_INCLUDE_ROOT}map)

	IF(WANT_STREFLOP)
		SET(GLEST_LIB_INCLUDE_DIRS
			${GLEST_LIB_INCLUDE_DIRS}
			${GLEST_LIB_INCLUDE_ROOT}streflop
			${GLEST_LIB_INCLUDE_ROOT}streflop/libm_flt32_source)
	ENDIF()

	INCLUDE_DIRECTORIES( ${GLEST_LIB_INCLUDE_DIRS} )

	#INCLUDE_DIRECTORIES( ${GLEST_LIB_INCLUDE_ROOT}platform/sdl )

	IF(WIN32)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/win32_deps/include)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/shared_lib/include/platform/posix)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/shared_lib/include/platform/win32)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/shared_lib/include/sound/ds8)
		INCLUDE_DIRECTORIES( ${GLEST_LIB_INCLUDE_ROOT}platform/sdl )
	ELSE()
		INCLUDE_DIRECTORIES( ${GLEST_LIB_INCLUDE_ROOT}platform/sdl )
		SET(GLEST_LIB_INCLUDE_DIRS ${GLEST_LIB_INCLUDE_DIRS} ${GLEST_LIB_INCLUDE_ROOT}platform/sdl)
		SET(GLEST_LIB_INCLUDE_DIRS ${GLEST_LIB_INCLUDE_DIRS} ${GLEST_LIB_INCLUDE_ROOT}platform/unix)
	ENDIF()

	SET(MG_SOURCE_FILES "")
	
	FOREACH(DIR IN LISTS DIRS_WITH_SRC)
		INCLUDE_DIRECTORIES( ${DIR} )

		set(SRC_DIR_TO_GLOB ${DIR})
		FILE(GLOB SRC_FILES_FROM_THIS_DIR ${SRC_DIR_TO_GLOB}/*.cpp)
		SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${SRC_FILES_FROM_THIS_DIR})
		FILE(GLOB SRC_FILES_FROM_THIS_DIR ${SRC_DIR_TO_GLOB}/*.c)
		SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${SRC_FILES_FROM_THIS_DIR})
		FILE(GLOB INCLUDE_FILES_FROM_THIS_DIR ${SRC_DIR_TO_GLOB}/*.h)
		SET(MG_INCLUDE_FILES ${MG_INCLUDE_FILES} ${INCLUDE_FILES_FROM_THIS_DIR})	

		IF(APPLE)
			FILE(GLOB SRC_FILES_FROM_THIS_DIR ${SRC_DIR_TO_GLOB}/*.mm)
			#MESSAGE(STATUS "SRC_FILES_FROM_THIS_DIR: ${SRC_FILES_FROM_THIS_DIR}")
			SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${SRC_FILES_FROM_THIS_DIR})
			FILE(GLOB SRC_FILES_FROM_THIS_DIR ${SRC_DIR_TO_GLOB}/*.m)
			#MESSAGE(STATUS "SRC_FILES_FROM_THIS_DIR: ${SRC_FILES_FROM_THIS_DIR}")
			SET(MG_SOURCE_FILES ${MG_SOURCE_FILES} ${SRC_FILES_FROM_THIS_DIR})	
		ENDIF(APPLE)
	ENDFOREACH(DIR)

	# Force the version and date/time stamp file to be touched so the compile stamp is always right
	IF(UNIX)
	add_custom_command(
	  OUTPUT mg.tmp
	  COMMAND touch ${PROJECT_SOURCE_DIR}/source/glest_game/facilities/game_util.cpp)
	add_custom_target(run ALL
	    DEPENDS mg.tmp)
	ENDIF()

	IF(WIN32)
		SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wl,-subsystem,console -mconsole")
		SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wl,-subsystem,console -mconsole")
		SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -Wl,-subsystem,console -mconsole")
		SET(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Wl,-subsystem,console -mconsole")

	ENDIF()

        OPTION(WANT_DEV_OUTPATH "use developer friendly output paths" OFF)
        IF(WANT_DEV_OUTPATH)
	        IF(UNIX AND NOT APPLE)
		        SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/mk/linux)
	        ELSE()
		        SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/data/glest_game)
	        ENDIF()
        ENDIF()

        MESSAGE(STATUS "***Note: The final output path for Megaglest Game will be [${EXECUTABLE_OUTPUT_PATH}]")

	SET_SOURCE_FILES_PROPERTIES(${MG_INCLUDE_FILES} PROPERTIES HEADER_FILE_ONLY 1)

	ADD_EXECUTABLE(${TARGET_NAME} ${MG_SOURCE_FILES} ${MG_INCLUDE_FILES})

	SET(MG_STREFLOP "")
	IF(WANT_STREFLOP)
		SET(MG_STREFLOP "streflop")
	ENDIF()

	IF(WIN32)
		TARGET_LINK_LIBRARIES(${TARGET_NAME} stdc++ gcc odbc32 wsock32 winspool winmm shell32 comctl32 ctl3d32  advapi32 wsock32 opengl32 glu32 ole32 oleaut32 uuid mingw32 ddraw dsound dxguid ws2_32 iphlpapi wsock32 libogg libvorbis libvorbisfile zlib jpeg libpng xerces-c2_8_0 OpenAL32 libcurl winmm gdi32 opengl32 glu32 SDL SDLmain lua5.1 ${MG_STREFLOP} libmegaglest stdc++ moldname mingwex msvcrt user32 kernel32)
	ENDIF()
	IF(NOT WIN32)
		IF(WANT_STREFLOP)
			TARGET_LINK_LIBRARIES(${TARGET_NAME} ${MG_STREFLOP})
		ENDIF()
		TARGET_LINK_LIBRARIES(${TARGET_NAME} libmegaglest)
	ENDIF()
	TARGET_LINK_LIBRARIES(${TARGET_NAME} ${EXTERNAL_LIBS})

	IF(EXISTS ${HELP2MAN})
		MESSAGE("***-- Found help2man: ${HELP2MAN} about to build manpage in [${EXECUTABLE_OUTPUT_PATH}/${TARGET_NAME_MANPAGE}]")

                SET(MG_MANPAGE_DESCRIPTION "3D multi-player real time strategy game")

		IF(EXISTS ${XVFB_EXEC})
			MESSAGE("***-- Found xvfb-run: ${XVFB_EXEC} about to build manpage with it.")

		  	add_custom_target(${TARGET_NAME_MANPAGE} ALL
		    		COMMAND ${XVFB_EXEC} --auto-servernum --server-num=770 ${HELP2MAN} --name='${MG_MANPAGE_DESCRIPTION}' --section=6 -N -o ${EXECUTABLE_OUTPUT_PATH}/${TARGET_NAME_MANPAGE} ${EXECUTABLE_OUTPUT_PATH}/${TARGET_NAME}
		    	DEPENDS ${TARGET_NAME})
		ELSE()
		  	add_custom_target(${TARGET_NAME_MANPAGE} ALL
		    		COMMAND ${HELP2MAN} --name='${MG_MANPAGE_DESCRIPTION}' --section=6 -N -o ${EXECUTABLE_OUTPUT_PATH}/${TARGET_NAME_MANPAGE} ${EXECUTABLE_OUTPUT_PATH}/${TARGET_NAME}
		    	DEPENDS ${TARGET_NAME})
		ENDIF()
	ENDIF()

        # Requires an install prefix for the items below to work
        IF(NOT CMAKE_INSTALL_PREFIX STREQUAL "")
	        MESSAGE(STATUS "**Source package [${PROJECT_SOURCE_DIR}]")	

	        # Installation of the program
	        INSTALL(TARGETS 
		        ${TARGET_NAME} 
		        DESTINATION ${MEGAGLEST_BIN_INSTALL_PATH})

	        IF(EXISTS ${HELP2MAN})
         		MESSAGE(STATUS "**NOTE: Will Install manpage [${TARGET_NAME_MANPAGE}] to [${MEGAGLEST_MANPAGE_INSTALL_PATH}]")	

		        # Installation of the program manpage file
		        INSTALL(FILES 
		          "${EXECUTABLE_OUTPUT_PATH}/${TARGET_NAME_MANPAGE}"
		          DESTINATION ${MEGAGLEST_MANPAGE_INSTALL_PATH})
	        ENDIF()

	        # Check if we are running from an SVN folder structure or tarballs
	        IF(EXISTS "${PROJECT_SOURCE_DIR}/mk/linux/megaglest.desktop")
		        MESSAGE(STATUS "**Source package detected svn folder structure.")

		        # Installation of the program desktop file
		        INSTALL(FILES 
		          "${PROJECT_SOURCE_DIR}/mk/linux/megaglest.desktop"
		          DESTINATION ${MEGAGLEST_DESKTOP_INSTALL_PATH})

		        # Installation of the program desktop icon file
		        INSTALL(FILES 
		          "${PROJECT_SOURCE_DIR}/mk/linux/megaglest.png"
		          "${PROJECT_SOURCE_DIR}/mk/linux/megaglest.xpm"
		          "${PROJECT_SOURCE_DIR}/mk/linux/megaglest.bmp"
		          "${PROJECT_SOURCE_DIR}/data/glest_game/editor.ico"
		          "${PROJECT_SOURCE_DIR}/mk/linux/glest.ico"
		          "${PROJECT_SOURCE_DIR}/data/glest_game/g3dviewer.ico"
		          DESTINATION ${MEGAGLEST_ICON_INSTALL_PATH})
	        ELSE()

		        # Installation of the program desktop file
		        INSTALL(FILES 
		          "${PROJECT_SOURCE_DIR}/megaglest.desktop"
		          DESTINATION ${MEGAGLEST_DESKTOP_INSTALL_PATH})

		        # Installation of the program desktop icon file
		        INSTALL(FILES 
		          "${PROJECT_SOURCE_DIR}/megaglest.png"
		          "${PROJECT_SOURCE_DIR}/megaglest.xpm"
		          "${PROJECT_SOURCE_DIR}/megaglest.bmp"
		          "${PROJECT_SOURCE_DIR}/editor.ico"
		          "${PROJECT_SOURCE_DIR}/glest.ico"
		          "${PROJECT_SOURCE_DIR}/g3dviewer.ico"
		          DESTINATION ${MEGAGLEST_ICON_INSTALL_PATH})
	        ENDIF()
        ENDIF()
ENDIF()
