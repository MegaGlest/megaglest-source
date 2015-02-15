#!/bin/sh

# Try and change up to the repository root, this assumes the script is in mk/macos/
cd `dirname $0`/../../

# Only run if we think this is the repository root
if ! [ -d ./`dirname $0` ]; then exit 1; fi

# Remove files 
rm -rf \
	CMakeCache.txt \
	CMakeFiles/ \
	CPackConfig.cmake \
	Info.plist \
	Makefile \
	_CPack_Packages/ \
	cmake_install.cmake \
	source/g3d_viewer/CMakeFiles/ \
	source/g3d_viewer/Makefile \
	source/g3d_viewer/cmake_install.cmake \
	source/glest_game/CMakeFiles/ \
	source/glest_game/Makefile \
	source/glest_game/cmake_install.cmake \
	source/glest_map_editor/CMakeFiles/ \
	source/glest_map_editor/Makefile \
	source/glest_map_editor/cmake_install.cmake \
	source/shared_lib/CMakeFiles/ \
	source/shared_lib/Makefile \
	source/shared_lib/cmake_install.cmake \
	source/shared_lib/liblibmegaglest.a \
	source/shared_lib/sources/streflop/CMakeFiles/ \
	source/shared_lib/sources/streflop/Makefile \
	source/shared_lib/sources/streflop/cmake_install.cmake \
	source/shared_lib/sources/streflop/libstreflop.a \
	source/tests/CMakeFiles/ \
	source/tests/Makefile \
	source/tests/cmake_install.cmake \
	source/tools/glexemel/CMakeFiles/ \
	source/tools/glexemel/Makefile \
	source/tools/glexemel/cmake_install.cmake \
	CMakeScripts/ \
	CPackSourceConfig.cmake \
	MegaGlest.build/ \
	MegaGlest.xcodeproj/ \
	source/g3d_viewer/MegaGlest.build/ \
	source/glest_game/CMakeScripts/ \
	source/glest_game/MegaGlest.build/ \
	source/glest_map_editor/MegaGlest.build/ \
	source/shared_lib/MegaGlest.build/ \
	source/shared_lib/Release/ \
	source/shared_lib/sources/streflop/MegaGlest.build/ \
	source/shared_lib/sources/streflop/Release/ \
	source/tools/glexemel/MegaGlest.build/ \
	install_manifest.txt \
	source/g3d_viewer/megaglest_g3dviewer \
	source/glest_game/megaglest \
	source/glest_map_editor/megaglest_editor \
	source/g3d_viewer/Release/ \
	source/glest_game/Release/ \
	source/glest_map_editor/Release/ \
	data/glest_game/CMakeFiles/ \
	data/glest_game/Makefile \
	data/glest_game/Release/ \
	data/glest_game/cmake_install.cmake \
	data/glest_game/megaglest \
	data/glest_game/megaglest_editor \
	data/glest_game/megaglest_g3dviewer \
	MegaGlest-*.dmg

echo '\n\tCleanup complete. Current status:\n'

git status

