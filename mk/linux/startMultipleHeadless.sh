#!/bin/bash
#hello
portstart=62000
cycles=$1
megaglest=./megaglest
headlessparam="--headless-server-mode=vps"
useports="--use-ports="

 if [[ `echo "$cycles" | grep -E ^[[:digit:]]+$` ]]
 then
   i=0 
   while [ $i -lt $cycles ]; do
      port=$[portstart + 1+ $i *11 ]
      statusport=$[port - 1 ]
      echo i=$i port=$port
      cmd="$megaglest $headlessparam $useports$port,$port,$statusport"
      echo $cmd
      $cmd &
      i=$[$i+1]
   done
   exit 0
 else
   echo "Wrong Input usage="
 fi



