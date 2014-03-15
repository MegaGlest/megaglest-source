# common libraries

SET(EXTERNAL_LIBS "")
SET(TARGET_NAME "megaglest_editor")
SET(TARGET_NAME_MANPAGE "${TARGET_NAME}.6")

IF(BUILD_MEGAGLEST_MAP_EDITOR)
	MESSAGE(STATUS "Will try to build MegaGlest map editor")

	FIND_PACKAGE(SDL REQUIRED)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
	SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${SDL_LIBRARY})

	#FIND_PACKAGE(OpenGL REQUIRED)
	#INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})
	#SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${OPENGL_LIBRARY})

	#QT
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_INCLUDE_CURRENT_DIR ON)
	FILE(GLOB UI_FILES_FROM_THIS_DIR *.ui)#those decscribe the actual user interface
	FILE(GLOB QRC_FILES_FROM_THIS_DIR *.qrc)#contains all used icons
	
	IF(USE_QT5)#QT5
		FIND_PACKAGE(Qt5Widgets REQUIRED)
		QT5_WRAP_UI(UI_INCLUDE_FILES ${UI_FILES_FROM_THIS_DIR})
		QT5_ADD_RESOURCES(UI_RESOURCES ${QRC_FILES_FROM_THIS_DIR})
	ELSE()#QT4
		FIND_PACKAGE(Qt4 REQUIRED)
		QT4_WRAP_UI(UI_INCLUDE_FILES ${UI_FILES_FROM_THIS_DIR})
		QT4_ADD_RESOURCES(UI_RESOURCES ${QRC_FILES_FROM_THIS_DIR})
		INCLUDE(${QT_USE_FILE})
		#ADD_DEFINITIONS(${QT_DEFINITIONS})
	ENDIF()

	#########################################################################################
	# megaglest map editor

	SET(DIRS_WITH_SRC
	    . )

	SET(GLEST_MAIN_INCLUDE_ROOT "../glest_game/")
	SET(GLEST_LIB_INCLUDE_ROOT "../shared_lib/include/")

	SET(GLEST_LIB_INCLUDE_DIRS
	    ${GLEST_LIB_INCLUDE_ROOT}platform/common
	#    ${GLEST_LIB_INCLUDE_ROOT}platform/sdl
	    ${GLEST_LIB_INCLUDE_ROOT}platform/posix
	    ${GLEST_LIB_INCLUDE_ROOT}util
	    ${GLEST_LIB_INCLUDE_ROOT}graphics
	    ${GLEST_LIB_INCLUDE_ROOT}graphics/gl
	    ${GLEST_LIB_INCLUDE_ROOT}map
	    ${GLEST_MAIN_INCLUDE_ROOT}facilities
	    ${GLEST_MAIN_INCLUDE_ROOT}game
            ${GLEST_LIB_INCLUDE_ROOT}xml/rapidxml
	    ${GLEST_MAIN_INCLUDE_ROOT}global)

	IF(WANT_STREFLOP)
		SET(GLEST_LIB_INCLUDE_DIRS
			${GLEST_LIB_INCLUDE_DIRS}
    			${GLEST_LIB_INCLUDE_ROOT}streflop)
	ENDIF()

	INCLUDE_DIRECTORIES( ${GLEST_LIB_INCLUDE_DIRS} )

	IF(WIN32)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/source/shared_lib/include/platform/win32)
		INCLUDE_DIRECTORIES( ${GLEST_LIB_INCLUDE_ROOT}platform/sdl )
	ELSE()
		INCLUDE_DIRECTORIES( ${GLEST_LIB_INCLUDE_ROOT}platform/sdl )
	ENDIF()


	SET(MG_SOURCE_FILES "../glest_game/global/config.cpp")
	
	FOREACH(DIR IN LISTS DIRS_WITH_SRC)
		INCLUDE_DIRECTORIES( ${DIR} )

		set(SRC_DIR_TO_GLOB ${DIR})
		FILE(GLOB SRC_FILES_FROM_THIS_DIR ${SRC_DIR_TO_GLOB}/*.cpp)
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

        OPTION(WANT_DEV_OUTPATH "use developer friendly output paths" OFF)
        IF(WANT_DEV_OUTPATH)
	        IF(UNIX AND NOT APPLE)
		        SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/mk/linux/)
	        ELSE()
		        SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/data/glest_game/)
	        ENDIF()
        ENDIF()

        SET(HELP2MAN_OUT_PATH ${EXECUTABLE_OUTPUT_PATH})
        IF("${EXECUTABLE_OUTPUT_PATH}" STREQUAL "")
                SET(HELP2MAN_OUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/")
        ENDIF()

        MESSAGE(STATUS "***Note: The final output path for Megaglest Map Editor will be [${HELP2MAN_OUT_PATH}]")

	FIND_PROGRAM(HELP2MAN "help2man")

	SET_SOURCE_FILES_PROPERTIES(${MG_INCLUDE_FILES} PROPERTIES HEADER_FILE_ONLY 1)

	ADD_EXECUTABLE(${TARGET_NAME} ${MG_SOURCE_FILES} ${MG_INCLUDE_FILES} ${UI_INCLUDE_FILES} ${UI_RESOURCES})

	IF(USE_QT5)#QT5
		qt5_use_modules(${TARGET_NAME} Widgets Core Gui)
	ELSE()#QT4
		TARGET_LINK_LIBRARIES(${TARGET_NAME} ${QT_LIBRARIES})
	ENDIF()
	
	SET(MG_STREFLOP "")
	IF(WANT_STREFLOP)
		SET(MG_STREFLOP "streflop")
	ENDIF()

	IF(WIN32)
		TARGET_LINK_LIBRARIES(${TARGET_NAME} stdc++ gcc odbc32 wsock32 winspool winmm shell32 comctl32 ctl3d32  advapi32 wsock32 opengl32 glu32 ole32 oleaut32 uuid mingw32 ddraw dsound dxguid ws2_32 iphlpapi wsock32 libogg libvorbis libvorbisfile zlib jpeg libpng xerces-c2_8_0 OpenAL32 libcurl winmm gdi32 opengl32 glu32 SDL SDLmain lua5.1 ${MG_STREFLOP} libmegaglest stdc++ moldname mingwex msvcrt user32 kernel32)
	ENDIF()

	IF(WANT_STREFLOP)
		TARGET_LINK_LIBRARIES(${TARGET_NAME} ${MG_STREFLOP})
	ENDIF()

	IF(WIN32)
		TARGET_LINK_LIBRARIES(${TARGET_NAME} libmegaglest)
	ELSE()
		TARGET_LINK_LIBRARIES(${TARGET_NAME} libmegaglest)
	ENDIF()
	TARGET_LINK_LIBRARIES(${TARGET_NAME} ${EXTERNAL_LIBS})

	IF(EXISTS ${HELP2MAN})
		MESSAGE("***-- Found help2man: ${HELP2MAN} about to build manpage in [${HELP2MAN_OUT_PATH}${TARGET_NAME_MANPAGE}]")

                SET(MG_MANPAGE_DESCRIPTION "Map editor for the popular 3D realtime stragegy game")

		IF(EXISTS ${XVFB_EXEC})
			MESSAGE("***-- Found xvfb-run: ${XVFB_EXEC} about to build manpage with it.")

		  	add_custom_target(${TARGET_NAME_MANPAGE} ALL
		    		COMMAND ${XVFB_EXEC} --auto-servernum --server-num=770 ${HELP2MAN} --name='${MG_MANPAGE_DESCRIPTION}' --section=6 -N -o ${HELP2MAN_OUT_PATH}${TARGET_NAME_MANPAGE} ${HELP2MAN_OUT_PATH}${TARGET_NAME}
		    	DEPENDS ${TARGET_NAME})
		ELSE()
		  	add_custom_target(${TARGET_NAME_MANPAGE} ALL
		    		COMMAND ${HELP2MAN} --name='${MG_MANPAGE_DESCRIPTION}' --section=6 -N -o ${HELP2MAN_OUT_PATH}${TARGET_NAME_MANPAGE} ${HELP2MAN_OUT_PATH}${TARGET_NAME}
		    	DEPENDS ${TARGET_NAME})
		ENDIF()
	ENDIF()


        # Requires an install prefix for the items below to work
        IF(NOT CMAKE_INSTALL_PREFIX STREQUAL "")
	        # Installation of the program
	        INSTALL(TARGETS 
		        ${TARGET_NAME} 
		        DESTINATION ${MEGAGLEST_BIN_INSTALL_PATH})

	        IF(EXISTS ${HELP2MAN})
         		MESSAGE(STATUS "**NOTE: Will Install manpage [${TARGET_NAME_MANPAGE}] to [${MEGAGLEST_MANPAGE_INSTALL_PATH}]")	

		        # Installation of the program manpage file
		        INSTALL(FILES 
		          "${HELP2MAN_OUT_PATH}${TARGET_NAME_MANPAGE}"
		          DESTINATION ${MEGAGLEST_MANPAGE_INSTALL_PATH})
	        ENDIF()

	        # Installation of the program config and image files
	        INSTALL(FILES 
                  "${PROJECT_SOURCE_DIR}/mk/shared/editor.ico"
	          DESTINATION ${MEGAGLEST_DATA_INSTALL_PATH})

	        # Installation of the program desktop file
                SET(MG_DESKTOP_FILE ${PROJECT_SOURCE_DIR}/mk/linux/megaglest_editor.desktop)

	        INSTALL(FILES
	          "${MG_DESKTOP_FILE}"
	          DESTINATION ${MEGAGLEST_DESKTOP_INSTALL_PATH})

        ENDIF()
ENDIF()
