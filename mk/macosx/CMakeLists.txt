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

# Include extra paths to search for includes; this is the default system wide macports path.
INCLUDE_DIRECTORIES(/opt/local/include)
LINK_DIRECTORIES(/opt/local/lib)

##install part

# This changes Info.plist from something with variables and CMakeisms to
# something that can be installed on disk.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/mk/macosx/Info.plist"
	"${CMAKE_CURRENT_BINARY_DIR}/Info.plist") 

include (InstallRequiredSystemLibraries)

# Use bundle generator (OSX has 3 other options if you feel adventurous)
set (CPACK_GENERATOR "Bundle")

# The following CPACK_* options are all required
set (CPACK_PACKAGE_FILE_NAME MegaGlest-${VER_MAJOR}.${VER_MINOR}.${VER_RELEASE})
set (CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/mk/macosx/MegaGlest.icns")

set (CPACK_RESOURCE_FILE_LICENSE  
     "${CMAKE_CURRENT_SOURCE_DIR}/docs/COPYRIGHT.source_code.txt")
set (CPACK_BUNDLE_NAME "MegaGlest")
set (CPACK_BUNDLE_STARTUP_COMMAND "${PROJECT_SOURCE_DIR}/data/glest_game/megaglest")
set (CPACK_BUNDLE_ICON  "${CMAKE_CURRENT_SOURCE_DIR}/mk/macosx/MegaGlest.icns")
set (CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")

# Install files in to the package (FIXME: how? we sure?)
INSTALL(FILES 
	${LUA_LIBRARIES}
    DESTINATION ../Frameworks
)

#the SDL_LIBRARY is seen as a framework thus we need to remove the suffix...
STRING(REGEX REPLACE ";.*" "" SDL_LIBRARY_DIR "${SDL_LIBRARY}")

#INSTALL(DIRECTORY 
#	${OGG_LIBRARY}
#	${XERCESC_LIBRARY}
#	${VORBIS_LIBRARY}
#	${SDL_LIBRARY_DIR}
#	${PNG_LIBRARY}
 #   DESTINATION ../Frameworks
 # )

