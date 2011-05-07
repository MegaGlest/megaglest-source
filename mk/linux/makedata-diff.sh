#!/bin/bash

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
diff --strip-trailing-cr --brief -r -x "*~" megaglest-data-$OLD_VERSION megaglest-data-$VERSION > megaglest-data-$VERSION-changes.txt
#fi

cd megaglest-data-$VERSION

if [ -e ../megaglest-data-$VERSION-fileslist.txt ]; then
	rm ../megaglest-data-$VERSION-fileslist.txt
fi

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
onlyinpos=`expr match "$line" 'Only in '`
#echo [$onlyinpos]

if [ "$onlyinpos" -eq "8" ]; then

	echo **NOTE: Found ONLY IN string...

	line=${line:$addfilepos-1}	
	line=${line/: //}
    line=${line/megaglest-data-$VERSION\/}

	echo New path: [$line]
else

	line=${line:$addfilepos-1}	
	line=${line/ differ/}
    line=${line/megaglest-data-$VERSION\/}

	echo New path: [$line]
fi

#compress_files="${compress_files} ${line}"

#echo compress_files = [$compress_files]
#echo ${line##megaglest-data-$VERSION*}

echo "${line} " >> ../megaglest-data-$VERSION-fileslist.txt

done
#exit

files_list=`cat ../megaglest-data-$VERSION-fileslist.txt`

#echo compress_files = [$files_list]

if [ -e ../$RELEASENAME.7z ]; then
	rm ../$RELEASENAME.7z
fi

#echo 7za a "../$RELEASENAME.7z" $files_list
7za a -mx=9 -ms=on -mhc=on "../$RELEASENAME.7z" $files_list

cd ..

cd ..
