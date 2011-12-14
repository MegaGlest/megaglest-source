#!/bin/bash
# Use this script to Analyze world synchronization log files between two or 
# more networked players
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

MAX_SPLIT_SIZE_BYTES=75000000

echo 'Max split file size = '"$MAX_SPLIT_SIZE_BYTES"

echo 'Setting up temp folders...'

if [ -d temp ]; then
  echo 'Purging temp folders...'
  rm -r temp
fi

if [ ! -d temp ]; then
  echo 'Creating temp folders...'
  mkdir temp
fi

if [ ! -d temp ]; then
  echo 'Could not find temp folders, exiting...'
  exit
fi

cd temp
if [ ! -d temp1 ]; then
  echo 'Creating temp sub folders...'
  mkdir temp1
fi

if [ -f xaa ] ; then
  rm x*
fi

echo 'Copying logs to temp folders...'
cp ../debugWorldSynch.log* .
cp debugWorldSynch.log1 temp1/

echo 'Splitting logs in temp folders...'
split -b $MAX_SPLIT_SIZE_BYTES debugWorldSynch.log

cd temp1
if [ -f xaa ] ; then
  rm x*
fi

split -b $MAX_SPLIT_SIZE_BYTES debugWorldSynch.log1
cd ../

echo 'Please diff the files xaa (and other split files) in temp folders temp and temp1...'
