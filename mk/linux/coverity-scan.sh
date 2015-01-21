#!/bin/bash
# Use this script to build MegaGlest using coverity analysis tool
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2013 Mark Vejvoda under GNU GPL v3.0+

# ----------------------------------------------------------------------------
#
# Requires:
# - curl, built with SSL support, in $PATH
# - wget, built with SSL support, in $PATH
# - 7z (command line utility of 7-zip), in $PATH
# - Coverity Scan Build Tool, in $PATH
# 


# Default to English language output so we can understand your bug reports
export LANG=C

CURRENTDIR="$(dirname $(readlink -f $0))"
echo "Script path [${CURRENTDIR}]"

# Load shared functions
. $CURRENTDIR/mg_shared.sh

# Project name (case sensitive)
PROJECT=MegaGlest

# Coverity Scan project token as listed on the Coverity Scan project page
# TOKEN=x

# E-Mail address of registered Coverity Scan user with project access
# EMAIL=x

# Where to store the data gathered by the Coverity Scan Build Tool
BUILDTOOL=cov-int

# read in config settings
if [ ! -f ${CURRENTDIR}/.coverity-scan ] ; then
        echo "-----------------------------------------"
        echo "**Missing Config** To use this script please create a config file named [${CURRENTDIR}/.coverity-scan]"
        echo "Containing: TOKEN=x , EMAIL=x , COVERITY_ANALYSIS_ROOT=x , NUMCORES=x"
        exit 1
fi

. ${CURRENTDIR}/.coverity-scan
# echo "Read config values: TOKEN [$TOKEN] EMAIL [$EMAIL] COVERITY_ANALYSIS_ROOT [$COVERITY_ANALYSIS_ROOT] NUMCORES [${NUMCORES}]"
# exit 1

GITBRANCH=$(git rev-parse --abbrev-ref HEAD | tr '/' '_')
GITVERSION_SHA1=$(git log -1 --format=%h)
GITVERSION_REV=$(git rev-list HEAD --count)
VERSION=${GITBRANCH}.${GITVERSION_REV}.${GITVERSION_SHA1}

# Included from shared functions
detect_system

#DESCRIPTION=${distribution}-${release}-${architecture}_${hostname}
DESCRIPTION=${GITBRANCH}.${GITVERSION_SHA1}.${distribution}-${architecture}.${hostname}
FILENAME=${PROJECT}.${DESCRIPTION}
# echo "FILENAME = [${FILENAME}]"
# exit 1

# export coverity analysis directory
# echo "Adding coverity folder to path: ${COVERITY_ANALYSIS_ROOT}/bin"
# exit 1

export PATH="${PATH}:${COVERITY_ANALYSIS_ROOT}/bin"

# cleanup old build files
# rm -rf ../../build && ../../build-mg.sh -m 1
#cd ../../
cd $CURRENTDIR
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
curl --progress-bar --insecure --form "project=${PROJECT}" --form "token=${TOKEN}" --form "email=${EMAIL}" --form "version=${VERSION}" --form "description=${DESCRIPTION}" --form "file=@${FILENAME}.tar.gz" https://scan5.coverity.com/cgi-bin/upload.py | tee -a "coverity-scan.log" ; test ${PIPESTATUS[0]} -eq 0

echo "CURL returned: $?"

if [ $? != 0 ]; then 
        echo "An error occurred trying to send the archive to coverity. Error: $?"
else
        echo "CURL was SUCCESSFUL!"
        rm -rf ${FILENAME}.tar.gz
        rm -rf ${BUILDTOOL}/
fi

# This currently fails to detect the following error situation, as reported in the HTML of the HTTP response (to the upload request):
# ERROR: Too many build submitted. Wait for few days before submitting next build: Refer build frequency at https://scan.coverity.com/faq#frequency

