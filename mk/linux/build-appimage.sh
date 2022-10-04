#!/bin/sh

SCRIPTLOC=$(dirname "$0")

if [ -d $SCRIPTLOC/BuildAppImage/AppDir ]; then
    rm -r $SCRIPTLOC/BuildAppImage/AppDir
fi

cmake $SCRIPTLOC/../.. -B $SCRIPTLOC/BuildAppImage -DBUILD_MEGAGLEST_MODEL_VIEWER=OFF -DBUILD_MEGAGLEST_MAP_EDITOR=OFF -DBUILD_MEGAGLEST_MODEL_IMPORT_EXPORT_TOOLS=OFF
cd $SCRIPTLOC/BuildAppImage
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

wget https://raw.githubusercontent.com/MegaGlest/megaglest-data/develop/others/icons/megaglest.png

if [ ! -f linuxdeploy-x86_64.AppImage ]; then 
    wget https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20220822-1/linuxdeploy-x86_64.AppImage
fi

chmod +x linuxdeploy-x86_64.AppImage 

./linuxdeploy-x86_64.AppImage -d AppDir/usr/local/share/applications/megaglest.desktop \
  --icon-file=megaglest.png \
   --icon-filename=megaglest --executable AppDir/usr/local/bin/megaglest --appdir AppDir --output appimage

mv MegaGlest*.AppImage ../
