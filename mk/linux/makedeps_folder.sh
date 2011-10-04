#!/bin/bash
set -e
# Use this script to copy shared (libs) files to specified location
# ----------------------------------------------------------------------------
# Written by Vivek Gite <http://www.cyberciti.biz/>
# (c) 2006 nixCraft under GNU GPL v2.0+
# Last updated on: Apr/06/2010 by Vivek Gite
# ----------------------------------------------------------------------------
# + Modified for megaglest deployment - Softcoder
# + Added ld-linux support
# + Added error checking support
# + Added for loop so that we can process all files on cmd
# ----------------------------------------------------------------------------
# Set libs output directory name
BASE="lib"
file="$@"
 
sync_support_libs(){
	local d="$1"         	# folder to copy dependencies to
	local pFILE="$2"        # bin file to scan for dependencies from
	local files=""
	local _cp="/bin/cp"
	#local skip_deps="libm.so libpthread.so libstdc++.so libgcc_s.so libc.so libdl.so libX11.so libpulse libfusion libdirect libnvidia libXext librt libxcb libICE libSM libXtst libwrap libdbus libXau libXdmcp libnsl libFLAC libGL"
	local skip_deps=""
	local keep_deps="libcurl libgnu libicu liblua libxerces libjpeg libpng libwx libgtk libgdk libftgl libfreetype" 
	
	local scan_via_skiplist=1 

	if [ -n "$skip_deps" ]; then
		scan_via_skiplist=1
		echo 'scanning for deps TO SKIP...'
	elif [ -n "$keep_deps" ]; then  
		scan_via_skiplist=0
		echo 'scanning for deps TO KEEP...'
	fi

	
	# get rid of blanks and (0x00007fff0117f000)
	files="$(ldd $pFILE |  awk '{ print $3 }' | sed -e '/^$/d' -e '/(*)$/d')"
 
	for i in $files
	do
	  dcc="${i%/*}"	# get dirname only
#	  [ ! -d ${d}${dcc} ] && mkdir -p ${d}${dcc}
#	  ${_cp} -f $i ${d}${dcc}
#      ${_cp} -f $i ${d}
#		echo ${_cp} -f $i ${d}

		skipfile=0

		if [ $scan_via_skiplist -eq 1 ]; then 
			for j in $(echo $skip_deps)
			do
				if [ `awk "BEGIN {print index(\"$i\", \"$j\")}"` -ne 0 ]; then
#			    	echo Skipping file = [$i]
					skipfile=1
					break
				fi
			done
		elif [ $scan_via_skiplist -eq 0 ]; then 
			skipfile=1
			for j in $(echo $keep_deps)
			do
				if [ `awk "BEGIN {print index(\"$i\", \"$j\")}"` -ne 0 ]; then
#			    	echo Skipping file = [$i]
					skipfile=0
					break
				fi
			done
		fi

		if [ $skipfile -eq 0 ]; then
			echo Including file = [$i]
			${_cp} -f $i ${d}
		fi
	done
 
	# Works with 32 and 64 bit ld-linux
	#sldl="$(ldd $pFILE | grep 'ld-linux' | awk '{ print $1}')"
	#sldlsubdir="${sldl%/*}"
#	[ ! -f ${d}${sldl} ] && ${_cp} -f ${sldl} ${d}${sldlsubdir}
	#if [ ! -f ${d}${sldl} ] ; then
	#	echo Including file = [${sldl}]
	#	${_cp} -f ${sldl} ${d}
	#fi
}
 
usage(){
	echo "Syntax : $0 megaglest
	echo "Example: $0 megaglest
	exit 1
}
 
[ $# -eq 0 ] && usage
#[ ! -d $BASE ] && mkdir -p $BASE
[ -d $BASE ] && rm -r $BASE
mkdir -p $BASE
 
# copy all files
for f in $file
do
	sync_support_libs "${BASE}" "${f}"
done
