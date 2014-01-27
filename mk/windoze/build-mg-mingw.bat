@echo off
mkdir build
cd build

rem set build_verbose="-D CMAKE_VERBOSE_MAKEFILE=ON"
set build_verbose=
cmake -G "MinGW Makefiles" %build_verbose% -D wxWidgets_ROOT_DIR="..\source\win32_deps\wxWidgets-2.8.10" -D wxWidgets_LIB_DIR="..\source\win32_deps\wxWidgets-2.8.10\lib" -D wxWidgets_CONFIGURATION="mswu" -DCMAKE_INSTALL_PREFIX= -DWANT_STATIC_LIBS=ON ..

mingw32-make
cd ..\
