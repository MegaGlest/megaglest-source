# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)

# which compilers to use for C and C++
#SET(CMAKE_C_COMPILER i586-mingw32msvc-gcc)
#SET(CMAKE_CXX_COMPILER i586-mingw32msvc-g++)
#SET(CMAKE_RC_COMPILER i586-mingw32msvc-windres)
SET(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
SET(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
SET(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# here is the target environment located
#SET(CMAKE_FIND_ROOT_PATH  /usr/i586-mingw32msvc ${PROJECT_SOURCE_DIR}/source/win32_deps/lib)
SET(CMAKE_FIND_ROOT_PATH  /usr/x86_64-w64-mingw32 ${PROJECT_SOURCE_DIR}/source/win32_deps/lib)
include_directories(${PROJECT_SOURCE_DIR}/source/win32_deps/freetype-2.4.8/include)
include_directories(${PROJECT_SOURCE_DIR}/source/win32_deps/glew-1.7.0/include)
include_directories(${PROJECT_SOURCE_DIR}/source/win32_deps/ftgl-2.1.3~rc5/src)
#add_definitions(-std=c++0x)
add_definitions( -std=gnu++0x )
add_definitions( -DTA3D_PLATFORM_MSVC=1 )
add_definitions( -DTA3D_PLATFORM_WINDOWS=1 )

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

