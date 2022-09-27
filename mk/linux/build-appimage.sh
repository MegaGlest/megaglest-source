#!/bin/sh
set +H

rm -r BuildAppImage/AppDir
cmake ../.. -B BuildAppImage -DBUILD_MEGAGLEST_MODEL_VIEWER=OFF -DBUILD_MEGAGLEST_MAP_EDITOR=OFF -DBUILD_MEGAGLEST_MODEL_IMPORT_EXPORT_TOOLS=OFF
cd BuildAppImage
make DESTDIR=AppDir -j$(nproc) install
strip AppDir/usr/local/bin/megaglest

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
