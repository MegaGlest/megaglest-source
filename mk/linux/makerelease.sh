#!/bin/bash

VERSION=`./mg-version.sh --version`
RELEASENAME=megaglest-source
#RELEASEDIR="`pwd`/release/$RELEASENAME-$VERSION"
RELEASEDIR="`pwd`/release/$RELEASENAME-$VERSION/megaglest-$VERSION"

echo "Creating source package in $RELEASEDIR"

rm -rf $RELEASEDIR
mkdir -p $RELEASEDIR
mkdir -p $RELEASEDIR/source
# copy sources
pushd "`pwd`/../../source"
find glest_game/ \( -name "*.cpp" -o -name "*.h" -o -name "*.c" -o -name "CMake*" \) -exec cp -p --parents "{}" $RELEASEDIR/source ';'
find shared_lib/ \( -name "*.cpp" -o -name "*.h" -o -name "*.c" -o -name "CMake*" \) -exec cp -p --parents "{}" $RELEASEDIR/source ';'
find glest_map_editor/ \( -name "*.cpp" -o -name "*.h" -o -name "*.xpm" -o -name "*.c" -o -name "CMake*" \) -exec cp -p --parents "{}" $RELEASEDIR/source ';'
find g3d_viewer/ \( -name "*.cpp" -o -name "*.h" -o -name "*.c" -o -name "CMake*" \) -exec cp -p --parents "{}" $RELEASEDIR/source ';'
find configurator/ \( -name "*.cpp" -o -name "*.h" -o -name "*.c" -o -name "CMake*" \) -exec cp -p --parents "{}" $RELEASEDIR/source ';'
find masterserver/ \( -name "*.php" -o -name "*.sql" -o -name "CMake*" \) -exec cp -p --parents "{}" $RELEASEDIR/source ';'
find tools/ \( -name "*.cpp" -o -name "*.h" -o -name "*.c" -o -name "*.pl" -o -name "*.sh" -o -name "*.css" -o -name "*.html" -o -name "*.ini" -o -name "*.ico" -o -name "*.txt" -o -name "*.dtd" -o -name "*.png" -o -name "*.py" -o -name "README" -o -name "INSTALL" -o -name "COPYING" -o -name "CMake*" \) -exec cp -p --parents "{}" $RELEASEDIR/source ';'

pushd "../"
find mk/cmake/ \( -name "*.cmake" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find mk/macosx/ \( -name "*.txt" -o -name "*.plist" -o -name "*.icns" -o -name "PkgInfo" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
popd

popd

cp -p ../../docs/*.txt $RELEASEDIR
cp -p glest.ini $RELEASEDIR
cp -p glestkeys.ini $RELEASEDIR
cp -p servers.ini $RELEASEDIR
cp -p glest.ico $RELEASEDIR
cp -p megaglest.png $RELEASEDIR
cp -p megaglest.desktop $RELEASEDIR
cp -p megaglest $RELEASEDIR
cp -p start_megaglest_configurator $RELEASEDIR
cp -p start_megaglest_mapeditor $RELEASEDIR
cp -p start_megaglest_g3dviewer $RELEASEDIR
cp -p ../../CMake* $RELEASEDIR

pushd release
PACKAGE="$RELEASENAME-$VERSION.tar.bz2"
echo "creating $PACKAGE"
rm "$PACKAGE"

tar -c --bzip2 -f "$PACKAGE" "$RELEASENAME-$VERSION"
#7za a "$RELEASENAME-$VERSION.7z" "$RELEASENAME-$VERSION"
popd
