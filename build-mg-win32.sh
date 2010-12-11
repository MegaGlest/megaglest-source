#!/bin/sh

mkdir build-win32
cd build-win32

cmake -DCMAKE_TOOLCHAIN_FILE=../mk/cmake/Modules/Toolchain-mingw32.cmake -DXERCESC_INCLUDE_DIR=${PROJECT_SOURCE_DIR}/source/win32_deps/xerces-c-src_2_8_0/include -DXERCESC_LIBRARY=${PROJECT_SOURCE_DIR}/source/win32_deps/xerces-c-src_2_8_0/lib/libxerces-c2_8_0.dll -DOPENAL_INCLUDE_DIR=${PROJECT_SOURCE_DIR}/source/win32_deps/openal-soft-1.12.854 -DOPENAL_LIBRARY=${PROJECT_SOURCE_DIR}/source/win32_deps/libs/OpenAL32.dll -DOGG_INCLUDE_DIR=${PROJECT_SOURCE_DIR}/source/win32_deps/libogg-1.2.1/include -DOGG_LIBRARY=${PROJECT_SOURCE_DIR}/source/win32_deps/libs/libogg.dll -DVORBIS_LIBRARY=${PROJECT_SOURCE_DIR}/source/win32_deps/libs/libvorbis.dll -DVORBIS_FILE_LIBRARY=${PROJECT_SOURCE_DIR}/source/win32_deps/libs/libvorbisfile.dll -DLUA_INCLUDE_DIR=${PROJECT_SOURCE_DIR}/source/win32_deps/lua-5.1/src -DLUA_LIBRARIES=${PROJECT_SOURCE_DIR}/source/win32_deps/libs/lua5.1.dll -DJPEG_INCLUDE_DIR=${PROJECT_SOURCE_DIR}/source/win32_deps/jpeg-8b -DJPEG_LIBRARY=${PROJECT_SOURCE_DIR}/source/win32_deps/libs/libjpeg.dll -DZLIB_INCLUDE_DIR=${PROJECT_SOURCE_DIR}/source/win32_deps/zlib-1.2.5 -DZLIB_LIBRARY=${PROJECT_SOURCE_DIR}/source/win32_deps/libs/libz.a -DPNG_PNG_INCLUDE_DIR=${PROJECT_SOURCE_DIR}/source/win32_deps/lpng141 -DPNG_LIBRARY=${PROJECT_SOURCE_DIR}/source/win32_deps/libs/libpng.dll -DCURL_INCLUDE_DIR=${PROJECT_SOURCE_DIR}/source/win32_deps/curl-7.20.1/include -DCURL_LIBRARY=${PROJECT_SOURCE_DIR}/source/win32_deps/libs/libcurl.dll -DSDL_INCLUDE_DIR=${PROJECT_SOURCE_DIR}/source/win32_deps/SDL-1.2.x/include ..

make

#echo 'You may now launch mega-glest from this folder like this:'
#echo '../mk/linux/glest.bin --ini-path=../mk/linux/ --data-path=../mk/linux/'


