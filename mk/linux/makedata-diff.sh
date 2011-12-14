#!/bin/bash
# Use this script to build MegaGlest Data Diff Archive for a Version Release
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

# This script compares two mega-glest data content folders for file differences, 
# then creates an archive of ONLY the differences (including files ONLY in new version)
OLD_VERSION=`./mg-version.sh --oldversion`
VERSION=`./mg-version.sh --version`
NEW_SUBFOLDER_PATH="megaglest-$VERSION"

cd release
CURDIR="`pwd`"
cd ..

RELEASENAME=megaglest-data-updates-$VERSION

cd $CURDIR

echo "Creating data package $RELEASENAME (comparing against $OLD_VERSION)"

#if [ ! -e megaglest-data-$VERSION-changes.txt ]; then
diff --strip-trailing-cr --brief -r -x "*~" megaglest-data-$OLD_VERSION/megaglest-$OLD_VERSION megaglest-data-$VERSION/megaglest-$VERSION > megaglest-data-$VERSION-changes.txt
#fi

cd megaglest-data-$VERSION

[[ -f "../megaglest-data-$VERSION-fileslist.txt" ]] && rm "../megaglest-data-$VERSION-fileslist.txt"

cat ../megaglest-data-$VERSION-changes.txt | while read line;
do

#echo "$line"   # Output the line itself.
#echo `expr match "$line" 'megaglest-data-$VERSION'`
#addfilepos=`expr match "$line" 'megaglest-data-$VERSION'`

#echo [$line]
#echo `awk "BEGIN {print index(\"$line\", \"megaglest-data-$VERSION\")}"`

addfilepos=`awk "BEGIN {print index(\"$line\", \"megaglest-data-$VERSION\")}"`

#echo [$addfilepos]
#echo [${line:$addfilepos-1}]

#echo [Looking for ONLY in: `expr match "$line" 'Only in '`]
onlyinpos=`expr match "$line" "Only in "`
#echo [$onlyinpos]

if [ "$onlyinpos" -eq "8" ]; then

	echo **NOTE: Found ONLY IN string... original line [${line}]

	onlyinpos=`expr match "$line" "Only in megaglest-data-$VERSION"`
	if [ "$onlyinpos" -ge "24" ]; then
		line=${line:$addfilepos-1}	
		line=${line/: //}
	    	line=${line/megaglest-data-$VERSION\/}

		echo New path: [$line]
	else
		echo **NOTE: skipping file since it is not in current version [${line}] match [$onlyinpos]
		line=
		echo New path: [$line]
	fi
else

	echo Section B ... original line [${line}]

	line=${line:$addfilepos-1}	
	line=${line/ differ/}
    	line=${line/megaglest-data-$VERSION\/}

	echo New path: [$line]
fi

#compress_files="${compress_files} ${line}"

#echo compress_files = [$compress_files]
#echo ${line##megaglest-data-$VERSION*}

if [ -n "${line}" ]; then
	echo "${line} " >> ../megaglest-data-$VERSION-fileslist.txt
fi

done
#exit

files_list=`cat ../megaglest-data-$VERSION-fileslist.txt`

#echo compress_files = [$files_list]

[[ -f "../$RELEASENAME.tar.xz" ]] && rm "../$RELEASENAME.tar.xz"

echo Current Folder is [`pwd`]
#echo 7za a "../$RELEASENAME.7z" $files_list
#7za a -mx=9 -ms=on -mhc=on "../$RELEASENAME.7z" $files_list
tar -cf - --add-file $files_list | xz -9e > ../$RELEASENAME.tar.xz

cd ..

cd ..
