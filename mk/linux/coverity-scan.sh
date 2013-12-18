#!/bin/sh
# Use this script to build MegaGlest using coverity analysis tool
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2013 Mark Vejvoda under GNU GPL v3.0+

# ----------------------------------------------------------------------------

# Default to English language output so we can understand your bug reports
export LANG=C

CURRENTDIR="$(dirname $(readlink -f $0))"
echo "Script path [${CURRENTDIR}]"

# 
# Upload Coverity s
# Requires:
# - data\glest_game\curl.exe, built with SSL support: http://curl.haxx.se/download.html
# - ..\..\data\glest_game\wget.exe (should get installed automatically during a build)
# - ..\..\data\glest_game\7z.exe (should get installed automatically during a build)
# - Coverity Scan Build Tool installed and in %PATH%
# 

# Project name (case sensitive)
PROJECT=MegaGlest

# Coverity Scan project token as listed on the Coverity Scan project page
# TOKEN=iTRBOPAgypWFuP9a9u0LtQ

# E-Mail address of registered Coverity Scan user with project access
# EMAIL=softcoder@megaglest.org

# read in config settings
if [ ! -f ${CURRENTDIR}/.coverity-submit ] ; then
        echo "-----------------------------------------"
        echo "**Missing Config** To use this script please create a config file named [${CURRENTDIR}/.coverity-submit]"
        echo "Containing: TOKEN=x , EMAIL=x , COVERITY_ANALYSIS_ROOT=x , NUMCORES=x"
        exit 1
fi

. ${CURRENTDIR}/.coverity-submit
# echo "Read config values: TOKEN [$TOKEN] EMAIL [$EMAIL] COVERITY_ANALYSIS_ROOT [$COVERITY_ANALYSIS_ROOT] NUMCORES [${NUMCORES}]"
# exit 1

# Description of this build (can be any string)
# Determine distro title, release, codename
LSB_PATH=$(which lsb_release)
if [ '${LSB_PATH}x' = 'x' ] ; then
	lsb=0
	if [ -e /etc/debian_version ]
	        then distribution='Debian';   release='unknown release version'; codename=`cat /etc/debian_version`
	elif [ -e /etc/SuSE-release ]
	        then distribution='SuSE';     release='unknown release version'; codename=`cat /etc/SuSE-release`
	elif [ -e /etc/redhat-release ]
	        then 
		if [ -e /etc/fedora-release ]
		then distribution='Fedora';   release='unknown release version'; codename=`cat /etc/fedora-release`
		else distribution='Redhat';   release='unknown release version'; codename=`cat /etc/redhat-release`
		fi
	elif [ -e /etc/fedora-release ]
	        then distribution='Fedora';   release='unknown release version'; codename=`cat /etc/fedora-release`
	elif [ -e /etc/mandrake-release ]
	        then distribution='Mandrake'; release='unknown release version'; codename=`cat /etc/mandrake-release`
	fi
else
	lsb=1

	distribution=$(lsb_release -i | awk -F':' '{ gsub(/^[ \t]*/,"",$2); print $2 }')
	release=$(lsb_release -r | awk -F':' '{ gsub(/^[  \t]*/,"",$2); print $2 }')
	codename=$(lsb_release -c | awk -F':' '{ gsub(/^[ \t]*/,"",$2); print $2 }')
fi

computername=$(hostname)
architecture=$(uname -m)
#DESCRIPTION=${distribution}-${release}-${architecture}_${computername}
DESCRIPTION=${distribution}-${architecture}_${computername}

# Where to store the data gathered by the Coverity Scan Build Tool
BUILDTOOL=cov-int

# ------------------------------------------------------------------------------

GITVERSION_SHA1=$(git log -1 --format=%h)
GITVERSION_REV=$(git rev-list HEAD --count)

VERSION=${GITVERSION_REV}.${GITVERSION_SHA1}
FILENAME=${PROJECT}_${DESCRIPTION}_${VERSION}

# echo "FILENAME = [${FILENAME}]"
# exit 1

# export coverity analysis directory
# echo "Adding coverity folder to path: ${COVERITY_ANALYSIS_ROOT}/bin"
# exit 1

export PATH="${PATH}:${COVERITY_ANALYSIS_ROOT}/bin"

# cleanup old build files
# rm -rf ../../build && ../../build-mg.sh -m 1
cd ../../
rm -rf build
./build-mg.sh -m 1

# Build megaglest using coverity build tool
# cov-build --dir $BUILDTOOL ../../build-mg.sh -n 1 -c 4
cd build/
cov-build --dir ${BUILDTOOL} make -j ${NUMCORES}

# Create archive to upload to coverity
7z a ${FILENAME}.tar ${BUILDTOOL}/
7z a ${FILENAME}.tar.gz ${FILENAME}.tar
rm -rf ${FILENAME}.tar
ls -la ${FILENAME}.tar.gz
# exit 1

echo "Running curl to upload analysis file..."
# echo "curl --progress-bar --insecure --form \"project=${PROJECT}\" --form \"token=${TOKEN}\" --form \"email=${EMAIL}\" --form \"version=${VERSION}\" --form \"description=${DESCRIPTION}\" --form \"file=@${FILENAME}.tar.gz\" https://scan5.coverity.com/cgi-bin/upload.py"
# exit 1
curl --progress-bar --insecure --form "project=${PROJECT}" --form "token=${TOKEN}" --form "email=${EMAIL}" --form "version=${VERSION}" --form "description=${DESCRIPTION}" --form "file=@${FILENAME}.tar.gz" https://scan5.coverity.com/cgi-bin/upload.py | tee -a "coverity-submit.log" ; test ${PIPESTATUS[0]} -eq 0

echo "CURL returned: $?"

if [ $? != 0 ]; then 
        echo "An error occurred trying to send the archive to coverity. Error: $?"
else
        echo "CURL was SUCCESSFUL!"
        rm -rf ${FILENAME}.tar.gz
        rm -rf ${BUILDTOOL}/
fi

