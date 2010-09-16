# Set defaults for Universal Binaries. We want 32-bit Intel/PPC on 10.4
# and 32/64-bit Intel/PPC on >= 10.5. Anything <= 10.3 doesn't support.
# These are just defaults/recommendations, but how we want to build
# out of the box. But the user needs to be able to change these options.
# So we must only set the values the first time CMake is run, or we
# will overwrite any changes the user sets.
# FORCE is used because the options are not reflected in the UI otherwise.
# Seems like a good place to add version specific compiler flags too.
IF(NOT CONFIG_HAS_BEEN_RUN_BEFORE)
	IF(EXISTS /Developer/SDKs/MacOSX10.6.sdk)
		SET(CMAKE_OSX_ARCHITECTURES "i386" CACHE STRING "Build architectures for OSX" FORCE)
	    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.6 -ftree-vectorize -fvisibility-inlines-hidden" CACHE STRING "Flags used by the compiler during all build types." FORCE)
	
	ELSEIF(EXISTS /Developer/SDKs/MacOSX10.5.sdk)
	    # 64-bit compiles are not supported with Carbon. We should enable
	    # 64-bit compilation by default once osgviewer has been
	    # rewritten with Cocoa.
	    #SET(CMAKE_OSX_ARCHITECTURES "ppc;i386;ppc64;x86_64" CACHE STRING "Build architectures for OSX" FORCE)
	    SET(CMAKE_OSX_ARCHITECTURES "i386" CACHE STRING "Build architectures for OSX" FORCE)
	    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.5 -ftree-vectorize -fvisibility-inlines-hidden" CACHE STRING "Flags used by the compiler during all build types." FORCE)
	ELSE()
	    # No Universal Binary support
	    # Should break down further to set the -mmacosx-version-min,
	    # but the SDK detection is too unreliable here.
	ENDIF()
ENDIF(NOT CONFIG_HAS_BEEN_RUN_BEFORE)



##install part



include (InstallRequiredSystemLibraries)

set (CPACK_RESOURCE_FILE_LICENSE  
     "${CMAKE_CURRENT_SOURCE_DIR}/docs/code_license.txt")
set (CPACK_GENERATOR Bundle)
set (CPACK_BUNDLE_NAME "MegaGlest")
set (CPACK_BUNDLE_STARTUP_COMMAND "${PROJECT_SOURCE_DIR}/data/glest_game/Debug/glest.bin")
set (CPACK_BUNDLE_ICON  "${CMAKE_CURRENT_SOURCE_DIR}/mk/macosx/MegaGlest.icns")
set (CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/mk/macosx/MegaGlest.icns")
set (CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/mk/macosx/Info.plist")
INSTALL(FILES 
	${LUA_LIBRARIES}
    DESTINATION ../Frameworks
  )
INSTALL(DIRECTORY 
	${OGG_LIBRARY}
	${XERCESC_LIBRARY}
	${VORBIS_LIBRARY}
	${SDL_LIBRARY}
    DESTINATION ../Frameworks
  )
  
include (CPack)
# Set defaults for Universal Binaries. We want 32-bit Intel/PPC on 10.4
# and 32/64-bit Intel/PPC on >= 10.5. Anything <= 10.3 doesn't support.
# These are just defaults/recommendations, but how we want to build
# out of the box. But the user needs to be able to change these options.
# So we must only set the values the first time CMake is run, or we
# will overwrite any changes the user sets.
# FORCE is used because the options are not reflected in the UI otherwise.
# Seems like a good place to add version specific compiler flags too.
IF(NOT CONFIG_HAS_BEEN_RUN_BEFORE)
	IF(EXISTS /Developer/SDKs/MacOSX10.6.sdk)
		SET(CMAKE_OSX_ARCHITECTURES "i386" CACHE STRING "Build architectures for OSX" FORCE)
	    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.6 -ftree-vectorize -fvisibility-inlines-hidden" CACHE STRING "Flags used by the compiler during all build types." FORCE)
	
	ELSEIF(EXISTS /Developer/SDKs/MacOSX10.5.sdk)
	    # 64-bit compiles are not supported with Carbon. We should enable
	    # 64-bit compilation by default once osgviewer has been
	    # rewritten with Cocoa.
	    #SET(CMAKE_OSX_ARCHITECTURES "ppc;i386;ppc64;x86_64" CACHE STRING "Build architectures for OSX" FORCE)
	    SET(CMAKE_OSX_ARCHITECTURES "i386" CACHE STRING "Build architectures for OSX" FORCE)
	    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.5 -ftree-vectorize -fvisibility-inlines-hidden" CACHE STRING "Flags used by the compiler during all build types." FORCE)
	ELSE()
	    # No Universal Binary support
	    # Should break down further to set the -mmacosx-version-min,
	    # but the SDK detection is too unreliable here.
	ENDIF()
ENDIF(NOT CONFIG_HAS_BEEN_RUN_BEFORE)



##install part



include (InstallRequiredSystemLibraries)

set (CPACK_RESOURCE_FILE_LICENSE  
     "${CMAKE_CURRENT_SOURCE_DIR}/docs/code_license.txt")
set (CPACK_GENERATOR Bundle)
set (CPACK_BUNDLE_NAME "MegaGlest")
set (CPACK_BUNDLE_STARTUP_COMMAND "${PROJECT_SOURCE_DIR}/data/glest_game/Debug/glest.bin")
set (CPACK_BUNDLE_ICON  "${CMAKE_CURRENT_SOURCE_DIR}/mk/macosx/MegaGlest.icns")
set (CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/mk/macosx/MegaGlest.icns")
set (CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/mk/macosx/Info.plist")
INSTALL(FILES 
	${LUA_LIBRARIES}
    DESTINATION ../Frameworks
  )
INSTALL(DIRECTORY 
	${OGG_LIBRARY}
	${XERCESC_LIBRARY}
	${VORBIS_LIBRARY}
	${SDL_LIBRARY}
    DESTINATION ../Frameworks
  )
  
include (CPack)
# Set defaults for Universal Binaries. We want 32-bit Intel/PPC on 10.4
# and 32/64-bit Intel/PPC on >= 10.5. Anything <= 10.3 doesn't support.
# These are just defaults/recommendations, but how we want to build
# out of the box. But the user needs to be able to change these options.
# So we must only set the values the first time CMake is run, or we
# will overwrite any changes the user sets.
# FORCE is used because the options are not reflected in the UI otherwise.
# Seems like a good place to add version specific compiler flags too.
IF(NOT CONFIG_HAS_BEEN_RUN_BEFORE)
	IF(EXISTS /Developer/SDKs/MacOSX10.6.sdk)
		SET(CMAKE_OSX_ARCHITECTURES "i386" CACHE STRING "Build architectures for OSX" FORCE)
	    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.6 -ftree-vectorize -fvisibility-inlines-hidden" CACHE STRING "Flags used by the compiler during all build types." FORCE)
	
	ELSEIF(EXISTS /Developer/SDKs/MacOSX10.5.sdk)
	    # 64-bit compiles are not supported with Carbon. We should enable
	    # 64-bit compilation by default once osgviewer has been
	    # rewritten with Cocoa.
	    #SET(CMAKE_OSX_ARCHITECTURES "ppc;i386;ppc64;x86_64" CACHE STRING "Build architectures for OSX" FORCE)
	    SET(CMAKE_OSX_ARCHITECTURES "i386" CACHE STRING "Build architectures for OSX" FORCE)
	    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.5 -ftree-vectorize -fvisibility-inlines-hidden" CACHE STRING "Flags used by the compiler during all build types." FORCE)
	ELSE()
	    # No Universal Binary support
	    # Should break down further to set the -mmacosx-version-min,
	    # but the SDK detection is too unreliable here.
	ENDIF()
ENDIF(NOT CONFIG_HAS_BEEN_RUN_BEFORE)



##install part



include (InstallRequiredSystemLibraries)

set (CPACK_RESOURCE_FILE_LICENSE  
     "${CMAKE_CURRENT_SOURCE_DIR}/docs/code_license.txt")
set (CPACK_GENERATOR Bundle)
set (CPACK_BUNDLE_NAME "MegaGlest")
set (CPACK_BUNDLE_STARTUP_COMMAND "${PROJECT_SOURCE_DIR}/data/glest_game/Debug/glest.bin")
set (CPACK_BUNDLE_ICON  "${CMAKE_CURRENT_SOURCE_DIR}/mk/macosx/MegaGlest.icns")
set (CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/mk/macosx/MegaGlest.icns")
set (CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/mk/macosx/Info.plist")
INSTALL(FILES 
	${LUA_LIBRARIES}
    DESTINATION ../Frameworks
  )
INSTALL(DIRECTORY 
	${OGG_LIBRARY}
	${XERCESC_LIBRARY}
	${VORBIS_LIBRARY}
	${SDL_LIBRARY}
    DESTINATION ../Frameworks
  )
  
include (CPack)
