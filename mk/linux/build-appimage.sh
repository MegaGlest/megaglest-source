#!/bin/sh

set -ev

# Set default workspace if not provided
WORKSPACE=${WORKSPACE:-$(pwd)}
# Check if the workspace path is absolute
if [[ "$WORKSPACE" != /* ]]; then
  echo "The workspace path must be absolute"
  exit 1
fi
test -d "$WORKSPACE"

# Set default source root if not provided
SOURCE_ROOT=${SOURCE_ROOT:-$WORKSPACE}
# Check if the source root path is absolute
if [[ "$SOURCE_ROOT" != /* ]]; then
  echo "The source root path must be absolute"
  exit 1
fi

mkdir -p $SOURCE_ROOT/BuildAppImage/game
cd $SOURCE_ROOT/BuildAppImage/game

if [ -d $SOURCE_ROOT/BuildAppImage/game/AppDir ]; then
    rm -r $SOURCE_ROOT/BuildAppImage/game/AppDir
fi
 
sudo DEBIAN_FRONTEND=noninteractive apt install -y build-essential libcurl4-gnutls-dev libsdl2-dev libopenal-dev liblua5.3-dev libjpeg-dev libpng-dev libfreetype6-dev libwxgtk3.0-gtk3-dev libcppunit-dev libfribidi-dev libftgl-dev libglew-dev libogg-dev libvorbis-dev libminiupnpc-dev libircclient-dev libxml2-dev libx11-dev libgl1-mesa-dev libglu1-mesa-dev librtmp-dev libkrb5-dev libldap2-dev libidn2-dev libpsl-dev libgnutls28-dev libnghttp2-dev libssh-dev libbrotli-dev p7zip-full imagemagick

cmake $SOURCE_ROOT -DBUILD_MEGAGLEST_MODEL_VIEWER=OFF -DBUILD_MEGAGLEST_MAP_EDITOR=OFF -DBUILD_MEGAGLEST_MODEL_IMPORT_EXPORT_TOOLS=OFF
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

LINUXDEPLOY_OUTPUT_VERSION="$VERSION"

linuxdeploy -d AppDir/usr/local/share/applications/megaglest.desktop \
  --icon-file=megaglest.png \
   --icon-filename=megaglest \
   --executable AppDir/usr/local/bin/megaglest \
   --appdir AppDir \
   --output appimage

mv MegaGlest*.AppImage $WORKSPACE

# MapEditor
mkdir -p $SOURCE_ROOT/BuildAppImage/mapeditor
cd $SOURCE_ROOT/BuildAppImage/mapeditor

if [ -d $SOURCE_ROOT/BuildAppImage/mapeditor/AppDir ]; then
    rm -r $SOURCE_ROOT/BuildAppImage/mapeditor/AppDir
fi

cmake $SOURCE_ROOT -DBUILD_MEGAGLEST=OFF -DBUILD_MEGAGLEST_MODEL_VIEWER=OFF -DBUILD_MEGAGLEST_MAP_EDITOR=ON -DBUILD_MEGAGLEST_MODEL_IMPORT_EXPORT_TOOLS=OFF -DINCLUDE_DATA=OFF
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

linuxdeploy -d AppDir/usr/local/share/applications/megaglest_editor.desktop \
  --icon-file=editor.png \
   --icon-filename=editor --executable AppDir/usr/local/bin/megaglest_editor --appdir AppDir --plugin gtk --output appimage

mv MegaGlest*.AppImage $WORKSPACE

# G3D viewer
mkdir -p $SOURCE_ROOT/BuildAppImage/g3dviewer
cd $SOURCE_ROOT/BuildAppImage/g3dviewer

if [ -d $SOURCE_ROOT/BuildAppImage/g3dviewer/AppDir ]; then
    rm -r $SOURCE_ROOT/BuildAppImage/g3dviewer/AppDir
fi

cmake $SOURCE_ROOT -DBUILD_MEGAGLEST=OFF -DBUILD_MEGAGLEST_MODEL_VIEWER=ON -DBUILD_MEGAGLEST_MAP_EDITOR=OFF -DBUILD_MEGAGLEST_MODEL_IMPORT_EXPORT_TOOLS=OFF -DINCLUDE_DATA=OFF
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

linuxdeploy -d AppDir/usr/local/share/applications/megaglest_g3dviewer.desktop \
  --icon-file=g3dviewer.png \
   --icon-filename=g3dviewer --executable AppDir/usr/local/bin/megaglest_g3dviewer --appdir AppDir --plugin gtk --output appimage

mv MegaGlest*.AppImage $WORKSPACE

# Tools
mkdir -p $SOURCE_ROOT/BuildAppImage/tools
cd $SOURCE_ROOT/BuildAppImage/tools

cmake $SOURCE_ROOT -DBUILD_MEGAGLEST=OFF -DBUILD_MEGAGLEST_MODEL_VIEWER=OFF -DBUILD_MEGAGLEST_MAP_EDITOR=OFF -DBUILD_MEGAGLEST_MODEL_IMPORT_EXPORT_TOOLS=ON
make -j$(nproc)

strip source/tools/glexemel/g2xml source/tools/glexemel/xml2g
mv source/tools/glexemel/g2xml $WORKSPACE 
mv source/tools/glexemel/xml2g $WORKSPACE 
