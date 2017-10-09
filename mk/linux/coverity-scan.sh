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
# - tar, in $PATH
# - Coverity Scan Build Tool, in $PATH
# 


# Default to English language output so we can understand your bug reports
export LANG=C

CURRENTDIR="$(dirname $(readlink -f $0))"
echo "Script path [${CURRENTDIR}]"

# Project title on Coverity Scan (case sensitive)
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

GITBRANCH=$(git rev-parse --abbrev-ref HEAD | tr '/' '-')
GITVERSION_SHA1=$(git log -1 --format=%h --abbrev=7)
GITVERSION_REV=$(git rev-list HEAD --count)
VERSION=${GITBRANCH}.${GITVERSION_REV}.${GITVERSION_SHA1}

distribution=$(lsb_release -si | tr '[A-Z]' '[a-z]' | tr '[._]' '-')
dist_release=$(lsb_release -sr | tr '[A-Z]' '[a-z]' | tr '[._]' '-')
architecture=$(uname -m        | tr '[A-Z]' '[a-z]' | tr '[._]' '-')
hostname=$(hostname)

DESCRIPTION=${GITBRANCH}-${GITVERSION_SHA1}_${distribution}_${architecture}_${hostname}
FILENAME=$(echo "${PROJECT}" | tr '/' '_')_${DESCRIPTION}
# echo "FILENAME = [${FILENAME}]"
# exit 1

# export coverity analysis directory
# echo "Adding coverity folder to path: ${COVERITY_ANALYSIS_ROOT}/bin"
# exit 1

export PATH="${PATH}:${COVERITY_ANALYSIS_ROOT}/bin"

sudo /sbin/sysctl vsyscall=emulate

# cleanup old build files
cd $CURRENTDIR
rm -rf build
./build-mg.sh -m 1

# Build using Coverity Scan build tool
echo "About to use cov-build to analyse code..."
cd build/
# cov-build --dir ${BUILDTOOL} make -j ${NUMCORES}
cov-build --dir ${BUILDTOOL} make -j ${NUMCORES}

# Create archive to upload to coverity
echo "About to send analyzed code to coverity website..."
tar czf ${FILENAME}.tar.gz ${BUILDTOOL}/
ls -la ${FILENAME}.tar.gz
# exit 1

echo "Running curl to upload analysis file..."
curl --progress-bar --insecure --form "token=${TOKEN}" --form "email=${EMAIL}" --form "version=${VERSION}" --form "description=${DESCRIPTION}" --form "file=@${FILENAME}.tar.gz" "https://scan.coverity.com/builds?project=${PROJECT}" | tee -a "coverity-scan.log"

if [[ ${PIPESTATUS[0]} -ne 0 ]]; then
        echo "An error occurred trying to send the archive to coverity. Error: $?"
else
        echo "CURL was SUCCESSFUL!"
        rm -rf ${FILENAME}.tar.gz
        rm -rf ${BUILDTOOL}/
fi

# This currently fails to detect the following error situation, as reported in the HTML of the HTTP response (to the upload request):
# ERROR: Too many build submitted. Wait for few days before submitting next build: Refer build frequency at https://scan.coverity.com/faq#frequency
