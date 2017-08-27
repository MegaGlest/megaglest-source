#!/bin/sh
# 2015 Written by filux <heross(@@)o2.pl>
# Copyright (c) 2015-2017 under GNU GPL v3.0+
# ----------------------------------------------------------------------------
LANG=C

CURRENTDIR="$(dirname "$(readlink -f "$0")")"
cd "$CURRENTDIR"

ENGINE_VERSION="$(../mg-version.sh --version)"
MU_PACKAGE_NAME='megaglest-mu-$ENGINE_VERSION-linux.tar.gz'
mkdir -p lib-x86; mkdir -p lib-x86_64
if [ "$?" -eq "0" ]; then
    echo '#!/bin/sh
# 2015 Written by filux <heross(@@)o2.pl>
# Copyright (c) 2015-2017 under GNU GPL v3.0+
# ----------------------------------------------------------------------------
LANG=C

CURRENTDIR="$(dirname "$(readlink -f "$0")")"
cd "$CURRENTDIR"

ENGINE_VERSION="'"$ENGINE_VERSION"'"
MU_PACKAGE_NAME="'"$MU_PACKAGE_NAME"'"
MU_ADDRESS="https://github.com/MegaGlest/megaglest-source/releases/download/$ENGINE_VERSION/$MU_PACKAGE_NAME"
if [ "$(which curl 2>/dev/null)" = "" ]; then
    echo "Downloading tool '"'curl'"' DOES NOT EXIST on this system, please install it."
    exit 1
fi
ARCHITECTURE="$(uname -m | tr '"'[A-Z]'"' '"'[a-z]'"')"
if [ "$ARCHITECTURE" = "x86_64" ]; then LibDir="lib-x86_64"; else LibDir="lib-x86"; fi
if [ ! -e "$MU_PACKAGE_NAME" ]; then
    echo "Downloading $MU_PACKAGE_NAME ..."
    curl -L -# "$MU_ADDRESS" -o "$MU_PACKAGE_NAME"; sleep 2s
    if [ -e "$MU_PACKAGE_NAME" ] && [ "$(tar -tf "$MU_PACKAGE_NAME" 2>/dev/null)" != "" ]; then
	echo "Extracting $MU_PACKAGE_NAME ..."; sleep 2s
	tar xzf "$MU_PACKAGE_NAME" -C "./"; sleep 1s
    fi
    if [ ! -e "$MU_PACKAGE_NAME" ]; then echo "The $MU_PACKAGE_NAME was not found ..."; fi
    if [ -e "megaglest-mini_update/megaglest-mini-update.sh" ]; then
	cp -f --no-dereference --preserve=all "megaglest-mini_update/megaglest-mini-update.sh" ./; sleep 0.5s
	./megaglest-mini-update.sh; sleep 0.5s
    else
	rm -f "$MU_PACKAGE_NAME"
    fi
else
    if [ -d "megaglest-mini_update" ]; then
	if [ -d "megaglest-mini_update/$LibDir" ]; then
	    mv "megaglest-mini_update/$LibDir" "megaglest-mini_update/lib"; sleep 0.25s
	    rm -rf "lib" ".lib_bak" "lib-x86_64" "lib-x86" "megaglest-mini_update/lib-x86_64" "megaglest-mini_update/lib-x86"
	    sleep 0.25s
	fi
	mv -f "megaglest-mini_update/"* "./"; sleep 0.5s; rm -rf "megaglest-mini_update"
	echo "Mini update finished."
    fi
    rm -f "$MU_PACKAGE_NAME"
fi
exit 0' > "megaglest-mini-update.sh"
    chmod +x "megaglest-mini-update.sh"

    if [ "$1" != "--only_script" ]; then
	rm -f "megaglest-mu-"*"-linux.tar.gz"
	if [ -d "megaglest-mini_update" ]; then rm -rf "megaglest-mini_update"; fi
	mkdir -p "megaglest-mini_update"

	cp -f --no-dereference --preserve=all start_megaglest \
	    start_megaglest_mapeditor start_megaglest_g3dviewer \
	    megaglest-mini-update.sh megaglest-configure-desktop.sh "megaglest-mini_update"
	cp -R -f --no-dereference --preserve=all lib-x86 lib-x86_64 "megaglest-mini_update"
	sleep 0.5s
	GZIP=-9 tar czf "megaglest-mu-$ENGINE_VERSION-linux.tar.gz" "megaglest-mini_update"
	rm -rf "megaglest-mini_update"
	rm -f "megaglest-mini-update.sh"
    fi
fi
exit 0
