#!/bin/sh

set -e

# SCRIPTLOC=$(dirname "$0")
SCRIPTLOC="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

if [ ! -f $TOOLS_DIR/squashfs-root/AppRun ]; then
    echo "Please specify the path to the extracted linuxdeploy (squashfs-root) using \$TOOLS_DIR env variable."
    exit -1
fi

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

$TOOLS_DIR/squashfs-root/AppRun -d AppDir/usr/local/share/applications/megaglest.desktop \
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

if [ ! -f AppDir/usr/local/share/applications/megaglest_editor.desktop ]; then 
    wget -P AppDir/usr/local/share/applications/ https://raw.githubusercontent.com/MegaGlest/megaglest-data/develop/others/desktop/megaglest_editor.desktop;
fi

convert AppDir/usr/local/share/megaglest/editor.ico editor.png
mv editor-2.png editor.png
# Another stupid hack to fix icons.
sed -i 's#Icon=megaglest#Icon=editor#' AppDir/usr/local/share/applications/megaglest_editor.desktop

ln -s $TOOLS_DIR/linuxdeploy-plugin-gtk.sh .

$TOOLS_DIR/squashfs-root/AppRun -d AppDir/usr/local/share/applications/megaglest_editor.desktop \
  --icon-file=editor.png \
   --icon-filename=editor --executable AppDir/usr/local/bin/megaglest_editor --appdir AppDir --plugin gtk --output appimage

mv MegaGlest*.AppImage $SCRIPTLOC

# G3D viewer
mkdir -p $SCRIPTLOC/BuildAppImage/g3dviewer
cd $SCRIPTLOC/BuildAppImage/g3dviewer

if [ -d $SCRIPTLOC/BuildAppImage/g3dviewer/AppDir ]; then
    rm -r $SCRIPTLOC/BuildAppImage/g3dviewer/AppDir
fi

cmake ../../../.. -DBUILD_MEGAGLEST=OFF -DBUILD_MEGAGLEST_MODEL_VIEWER=ON -DBUILD_MEGAGLEST_MAP_EDITOR=OFF -DBUILD_MEGAGLEST_MODEL_IMPORT_EXPORT_TOOLS=OFF
make DESTDIR=AppDir -j$(nproc) install
strip AppDir/usr/local/bin/megaglest_g3dviewer

if [ ! -d AppDir/usr/bin ]; then
    mkdir -p AppDir/usr/bin
fi

if [ ! -f AppDir/usr/local/share/applications/megaglest_g3dviewer.desktop ]; then 
    wget -P AppDir/usr/local/share/applications/ https://raw.githubusercontent.com/MegaGlest/megaglest-data/develop/others/desktop/megaglest_g3dviewer.desktop;
fi

convert AppDir/usr/local/share/megaglest/g3dviewer.ico g3dviewer.png
mv g3dviewer-2.png g3dviewer.png
# Another stupid hack to fix icons.
sed -i 's#Icon=megaglest#Icon=g3dviewer#' AppDir/usr/local/share/applications/megaglest_g3dviewer.desktop

ln -s $TOOLS_DIR/linuxdeploy-plugin-gtk.sh .

$TOOLS_DIR/squashfs-root/AppRun -d AppDir/usr/local/share/applications/megaglest_g3dviewer.desktop \
  --icon-file=g3dviewer.png \
   --icon-filename=g3dviewer --executable AppDir/usr/local/bin/megaglest_g3dviewer --appdir AppDir --plugin gtk --output appimage

mv MegaGlest*.AppImage $SCRIPTLOC

# Tools
mkdir -p $SCRIPTLOC/BuildAppImage/tools
cd $SCRIPTLOC/BuildAppImage/tools

cmake ../../../.. -DBUILD_MEGAGLEST=OFF -DBUILD_MEGAGLEST_MODEL_VIEWER=OFF -DBUILD_MEGAGLEST_MAP_EDITOR=OFF -DBUILD_MEGAGLEST_MODEL_IMPORT_EXPORT_TOOLS=ON
make -j$(nproc)

strip source/tools/glexemel/g2xml source/tools/glexemel/xml2g
mv source/tools/glexemel/g2xml $SCRIPTLOC 
mv source/tools/glexemel/xml2g $SCRIPTLOC 
