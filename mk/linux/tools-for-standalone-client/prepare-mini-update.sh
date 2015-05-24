#!/bin/sh
# 2015 Written by filux <heross(@@)o2.pl>
# Copyright (c) 2015 under GNU GPL v3.0+
# ----------------------------------------------------------------------------
LANG=C

CURRENTDIR="$(dirname "$(readlink -f "$0")")"
cd "$CURRENTDIR"

ENGINE_VERSION="$(../mg-version.sh --version)"
MU_PACKAGE_NAME="megaglest-mu-$ENGINE_VERSION-linux.tar.gz"
mkdir -p lib-x86; mkdir -p lib-x86_64
if [ "$?" -eq "0" ]; then
    echo '#!/bin/sh
# 2015 Written by filux <heross(@@)o2.pl>
# Copyright (c) 2015 under GNU GPL v3.0+
# ----------------------------------------------------------------------------
LANG=C

CURRENTDIR="$(dirname "$(readlink -f "$0")")"
cd "$CURRENTDIR"

ENGINE_VERSION="'"$ENGINE_VERSION"'"
MU_PACKAGE_NAME="megaglest-mu-$ENGINE_VERSION-linux.tar.gz"
MU_ADDRESS="https://github.com/MegaGlest/megaglest-source/releases/download/$ENGINE_VERSION/$MU_PACKAGE_NAME"
ARCHITECTURE="$(uname -m | tr '"'[A-Z]'"' '"'[a-z]'"')"
if [ "$ARCHITECTURE" = "x86_64" ]; then LibDir="lib-x86_64"; else LibDir="lib-x86"; fi
wget -c --progress=bar $MU_ADDRESS 2>&1; sleep 0.5s
if [ -e "$MU_PACKAGE_NAME" ]; then tar xzf "$MU_PACKAGE_NAME" -C "./"; rm -f "$MU_PACKAGE_NAME"; fi
if [ -d "megaglest-mini_update" ]; then
    if [ -d "megaglest-mini_update/$LibDir" ]; then
	mv "megaglest-mini_update/$LibDir" "megaglest-mini_update/lib"
	rm -rf "lib" "lib-x86_64" "lib-x86" "megaglest-mini_update/lib-x86_64" "megaglest-mini_update/lib-x86"
    fi
    mv "megaglest-mini_update/"* "./"; rm -rf "megaglest-mini_update"
fi
exit 0' > "megaglest-mini-update.sh"
    chmod +x "megaglest-mini-update.sh"

    if [ "$1" != "--only_script" ]; then
	if [ -d "megaglest-mini_update" ]; then rm -rf "megaglest-mini_update"; fi
	mkdir -p "megaglest-mini_update"

	cp -f --no-dereference --preserve=all ../start_megaglest \
	    ../start_megaglest_mapeditor ../start_megaglest_g3dviewer \
	    megaglest-mini-update.sh megaglest-configure-desktop.sh "megaglest-mini_update"
	cp -R -f --no-dereference --preserve=all lib-x86 lib-x86_64 "megaglest-mini_update"
	sleep 0.5s
	GZIP=-9 tar czf "$MU_PACKAGE_NAME" "megaglest-mini_update"
	rm -rf "megaglest-mini_update"
	rm -f "megaglest-mini-update.sh"
    fi
fi
exit 0
