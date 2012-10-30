#!/bin/bash

# Run this script from its own folder on a Fedora based system.
# This script will install everything needed and copy the source and data archives
# then build RPM's and copy them to a destination. See the section below and
# change the variables as required (I build from my local system). Commented out 
# below for example are wget calls to get source and data from the official
# sourceforge links
# Originally written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2012 Mark Vejvoda under GNU GPL v3.0

#
# Start changes here
#
SOURCE_PACKAGE_VER=3.7.0.1
DATA_PACKAGE_VER=$SOURCE_PACKAGE_VER

SOURCE_PACKAGE_RPM_VER=3.7.0
DATA_PACKAGE_RPM_VER=$SOURCE_PACKAGE_RPM_VER

SOURCE_COPY_CMD="cp /media/dlinknas/games/MegaGlest/megaglest-source-$SOURCE_PACKAGE_VER.tar.xz ./"
DATA_COPY_CMD="cp /media/dlinknas/games/MegaGlest/megaglest-data-$DATA_PACKAGE_VER.tar.xz ./"

#SOURCE_COPY_CMD="wget http://sourceforge.net/projects/megaglest/files/megaglest_$SOURCE_PACKAGE_RPM_VER/megaglest-source-$SOURCE_PACKAGE_VER.tar.xz/download -O megaglest-source-$SOURCE_PACKAGE_VER.tar.xz"
#DATA_COPY_CMD="wget http://sourceforge.net/projects/megaglest/files/megaglest_$DATA_PACKAGE_RPM_VER/megaglest-data-$DATA_PACKAGE_VER.tar.xz/download -O megaglest-data-$DATA_PACKAGE_VER.tar.xz"

SOURCE_PUBLISH_CMD="cp RPMS/x86_64/*.rpm /media/dlinknas/games/MegaGlest/"
DATA_PUBLISH_CMD="cp RPMS/noarch/*.rpm /media/dlinknas/games/MegaGlest/; cp RPMS/x86/*.rpm /media/dlinknas/games/MegaGlest/; cp RPMS/x86_64/*.rpm /media/dlinknas/games/MegaGlest/"
#
# End changes here
#

#remove old archives
[[ -f "megaglest-source-$SOURCE_PACKAGE_VER.tar.xz" ]] && rm "megaglest-source-$SOURCE_PACKAGE_VER.tar.xz"
[[ -f "megaglest-source-$SOURCE_PACKAGE_RPM_VER.tar.bz2" ]] && rm "megaglest-source-$SOURCE_PACKAGE_RPM_VER.tar.bz2"

[[ -f "megaglest-data-$DATA_PACKAGE_VER.tar.xz" ]] && rm "megaglest-data-$DATA_PACKAGE_VER.tar.xz"
[[ -f "megaglest-data-$DATA_PACKAGE_RPM_VER.tar.bz2" ]] && rm "megaglest-data-$DATA_PACKAGE_RPM_VER.tar.bz2"

# Get the source and data archives
echo "copying source archive [$SOURCE_COPY_CMD]"
$SOURCE_COPY_CMD

echo "setting up source archive..."
tar Jxf megaglest-source-$SOURCE_PACKAGE_VER.tar.xz >/dev/null 2>&1
tar -cvjf megaglest-source-$SOURCE_PACKAGE_RPM_VER.tar.bz2 megaglest-$SOURCE_PACKAGE_RPM_VER/ >/dev/null 2>&1
rm -rf megaglest-$SOURCE_PACKAGE_RPM_VER/
rm megaglest-source-$SOURCE_PACKAGE_VER.tar.xz

echo "copying data archive [$DATA_COPY_CMD]"
$DATA_COPY_CMD

echo "setting up data archive..."
tar Jxf megaglest-data-$DATA_PACKAGE_VER.tar.xz >/dev/null 2>&1 
cd megaglest-$DATA_PACKAGE_RPM_VER/
tar -cvjf megaglest-data-$DATA_PACKAGE_RPM_VER.tar.bz2 * >/dev/null 2>&1
cd ../
mv megaglest-$DATA_PACKAGE_RPM_VER/megaglest-data-$DATA_PACKAGE_RPM_VER.tar.bz2 ./
rm -rf megaglest-$DATA_PACKAGE_RPM_VER/
rm megaglest-data-$DATA_PACKAGE_VER.tar.xz

# Install fedora dev and package tools
echo "installing fedora build tools..."
sudo yum install @development-tools
sudo yum install fedora-packager

# Install dependencies for Megaglest
echo "installing MegaGlest dependencies..."
sudo yum install cmake libX11-devel SDL-devel openal-soft-devel xerces-c-devel freeglut-devel krb5-devel libdrm-devel libidn-devel libjpeg-devel libpng-devel libssh2-devel openldap-devel libxml2-devel subversion mesa-libGL-devel mesa-libGLU-devel openal-soft-devel SDL-devel libcurl-devel c-ares-devel wxGTK-devel glew-devel libogg-devel libvorbis-devel lua-devel wxGTK-devel openssl-devel wxBase desktop-file-utils recode gcc gcc-c++ ftgl-devel ftgl autogen autogen-libopts

# create an rpm user
echo "creating rpm user (please enter the password: makerpm)"
sudo /usr/sbin/useradd makerpm
echo makerpm | sudo /usr/bin/passwd makerpm --stdin

echo "copying rpm config files..."
sudo cp megaglest-rpm-meta.tar.bz2 /home/makerpm
sudo cp megaglest-source-$SOURCE_PACKAGE_RPM_VER.tar.bz2 /home/makerpm/rpmbuild/SOURCES/
sudo cp megaglest-data-$DATA_PACKAGE_RPM_VER.tar.bz2 /home/makerpm/rpmbuild/SOURCES/

# now build the binary and data rpms
echo "building rpms..."
su - makerpm -c "rpmdev-setuptree; cd rpmbuild/; tar xvf ../megaglest-rpm-meta.tar.bz2; cd SPECS/; rpmbuild -ba megaglest.spec; cd ../; cp RPMS/x86/*.rpm /media/dlinknas/games/MegaGlest/; $SOURCE_PUBLISH_CMD; mv SOURCES/megaglest-data-$SOURCE_PACKAGE_RPM_VER.tar.bz2 ./; rm -f SOURCES/*; mv megaglest-data-$SOURCE_PACKAGE_RPM_VER.tar.bz2 SOURCES/; rm -f SPECS/*; mv megaglest-data.spec SPECS/; cd SPECS/; rpmbuild -ba megaglest-data.spec; cd ../; $DATA_PUBLISH_CMD; rm -f SOURCES/*; rm -f SPECS/*;"

echo "DONE building rpms"
