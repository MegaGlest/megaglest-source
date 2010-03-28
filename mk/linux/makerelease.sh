#!/bin/bash

VERSION=`autoconf -t AC_INIT | sed -e 's/[^:]*:[^:]*:[^:]*:[^:]*:\([^:]*\):.*/\1/g'`
RELEASENAME=megaglest-source
RELEASEDIR="`pwd`/release/$RELEASENAME-$VERSION"

echo "Creating source package in $RELEASEDIR"

rm -rf $RELEASEDIR
mkdir -p $RELEASEDIR
# copy sources
pushd "`pwd`/../../source"
find glest_game/ \( -name "*.cpp" -o -name "*.h" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find shared_lib/ \( -name "*.cpp" -o -name "*.h" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find glest_map_editor/ \( -name "*.cpp" -o -name "*.h" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find g3d_viewer/ \( -name "*.cpp" -o -name "*.h" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find configurator/ \( -name "*.cpp" -o -name "*.h" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
popd
AUTOCONFSTUFF="configure.ac autogen.sh Jamrules Jamfile `find mk/jam -name "*.jam"` `find mk/autoconf -name "*.m4" -o -name "config.*" -o -name "*sh"`"

cp -p --parents $AUTOCONFSTUFF $RELEASEDIR
cp -p ../../docs/readme*.txt ../../docs/*license*.txt $RELEASEDIR
cp -p glest.ini $RELEASEDIR

pushd $RELEASEDIR
./autogen.sh
popd

pushd release
PACKAGE="$RELEASENAME-$VERSION.tar.bz2"
echo "creating $PACKAGE"
tar -c --bzip2 -f "$PACKAGE" "$RELEASENAME-$VERSION"
#7za a "$RELEASENAME-$VERSION.7z" "$RELEASENAME-$VERSION"
popd
