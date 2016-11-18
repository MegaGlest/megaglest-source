# Anything <= 10.3 isn't supported. We want 32-bit Intel/PPC on 10.4, 10.5.
# On 10.6 32/64-bit Intel. On >= 10.7 unset architecture mean default 64-bit Intel.
# Using oldest available sdk with current Xcode is recommended.
# User needs to be able to change these options.
# So we must only set the values the first time CMake is run, or we
# will overwrite any changes the user sets.
# FORCE is used because the options are not reflected in the UI otherwise.
# Seems like a good place to add version specific compiler flags too.

IF(NOT CONFIG_HAS_BEEN_RUN_BEFORE)
	IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
		#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")
	ENDIF()
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftree-vectorize" CACHE STRING "Flags used by the compiler during all build types." FORCE)
ENDIF()

# Include extra paths to search for includes; this is the default system wide macports and X11 path.
INCLUDE_DIRECTORIES(/usr/X11/include /opt/local/include)
#LINK_DIRECTORIES(/opt/local/lib)

##install part

# This file is in git CRLF but that causes the cpack run to fail
#CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/docs/COPYRIGHT.source_code.txt"
#	"${CMAKE_CURRENT_BINARY_DIR}/docs/COPYRIGHT.source_code.txt" NEWLINE_STYLE UNIX)
#SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_BINARY_DIR}/docs/COPYRIGHT.source_code.txt")

# This changes Info.plist from something with variables and CMakeisms to
# something that can be installed on disk.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/mk/macos/bundle_resources/Info.plist.in"
	"${CMAKE_CURRENT_BINARY_DIR}/mk/macos/bundle_resources/Info.plist")
SET(CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_BINARY_DIR}/mk/macos/bundle_resources/Info.plist")

include (InstallRequiredSystemLibraries)

# Use bundle generator (OSX has 3 other options if you feel adventurous)
SET(CPACK_GENERATOR "Bundle")
SET(CPACK_BUNDLE_NAME "MegaGlest")
SET(CPACK_PACKAGE_FILE_NAME "MegaGlest-game-macos-${MEGAGLEST_VERSION}")
SET(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/mk/macos/bundle_resources/MegaGlest.icns")
SET(CPACK_BUNDLE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/mk/macos/bundle_resources/MegaGlest.icns")
SET(CPACK_BUNDLE_STARTUP_COMMAND "${PROJECT_SOURCE_DIR}/mk/macos/bundle_resources/MegaGlest.sh")
