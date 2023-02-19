#!/bin/sh

set -e

# SCRIPTLOC=$(dirname "$0")
SCRIPTLOC="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

mkdir -p $SCRIPTLOC/BuildAppImage/
cd $SCRIPTLOC/BuildAppImage/

if [ ! -f linuxdeploy-x86_64.AppImage ]; then 
    wget https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20220822-1/linuxdeploy-x86_64.AppImage
fi

chmod +x linuxdeploy-x86_64.AppImage
./linuxdeploy-x86_64.AppImage --appimage-extract

mkdir -p $SCRIPTLOC/BuildAppImage/game
cd $SCRIPTLOC/BuildAppImage/game

if [ -d $SCRIPTLOC/BuildAppImage/game/AppDir ]; then
    rm -r $SCRIPTLOC/BuildAppImage/game/AppDir
fi

cmake ../../../.. -DBUILD_MEGAGLEST_MODEL_VIEWER=OFF -DBUILD_MEGAGLEST_MAP_EDITOR=OFF -DBUILD_MEGAGLEST_MODEL_IMPORT_EXPORT_TOOLS=OFF
make DESTDIR=AppDir -j$(nproc) install
strip AppDir/usr/local/bin/megaglest

if [ ! -d AppDir/usr/bin ]; then
    mkdir -p AppDir/usr/bin
fi

cp $(whereis 7z | awk -F ' ' '{print $2;}') AppDir/usr/bin/
# Hacky workaround to use internal 7z.
sed -i 's#=7z#=$APPLICATIONPATH/7z#' AppDir/usr/local/share/megaglest/glest.ini

if [ ! -f AppDir/usr/local/share/applications/megaglest.desktop ]; then 
    wget -P AppDir/usr/local/share/applications/ https://raw.githubusercontent.com/MegaGlest/megaglest-data/develop/others/desktop/megaglest.desktop;
fi

convert AppDir/usr/local/share/megaglest/megaglest.ico megaglest.png
mv megaglest-2.png megaglest.png

$SCRIPTLOC/BuildAppImage/squashfs-root/AppRun -d AppDir/usr/local/share/applications/megaglest.desktop \
  --icon-file=megaglest.png \
   --icon-filename=megaglest --executable AppDir/usr/local/bin/megaglest --appdir AppDir --output appimage

mv MegaGlest*.AppImage $SCRIPTLOC

# MapEditor
mkdir -p $SCRIPTLOC/BuildAppImage/mapeditor
cd $SCRIPTLOC/BuildAppImage/mapeditor

if [ -d $SCRIPTLOC/BuildAppImage/mapeditor/AppDir ]; then
    rm -r $SCRIPTLOC/BuildAppImage/mapeditor/AppDir
fi

cmake ../../../.. -DBUILD_MEGAGLEST=OFF -DBUILD_MEGAGLEST_MODEL_VIEWER=OFF -DBUILD_MEGAGLEST_MAP_EDITOR=ON -DBUILD_MEGAGLEST_MODEL_IMPORT_EXPORT_TOOLS=OFF
make DESTDIR=AppDir -j$(nproc) install
strip AppDir/usr/local/bin/megaglest_editor

if [ ! -d AppDir/usr/bin ]; then
    mkdir -p AppDir/usr/bin
fi

if [ ! -f AppDir/usr/local/share/applications/megaglest.desktop ]; then 
    wget -P AppDir/usr/local/share/applications/ https://raw.githubusercontent.com/MegaGlest/megaglest-data/develop/others/desktop/megaglest_editor.desktop;
fi

convert AppDir/usr/local/share/megaglest/editor.ico editor.png
mv editor-2.png editor.png
# Another stupid hack to fix icons.
sed -i 's#Icon=megaglest#Icon=editor#' AppDir/usr/local/share/applications/megaglest_editor.desktop

$SCRIPTLOC/BuildAppImage/squashfs-root/AppRun -d AppDir/usr/local/share/applications/megaglest_editor.desktop \
  --icon-file=editor.png \
   --icon-filename=editor --executable AppDir/usr/local/bin/megaglest_editor --appdir AppDir --output appimage

mv MegaGlest*.AppImage $SCRIPTLOC
