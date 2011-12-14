#!/bin/sh
# Use this script formats all text files to use consistent line endings
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

cd ../../
find -name "*\.cpp" -exec fromdos -d {} \;
find -name "*\.c" -exec fromdos -d {} \;
find -name "*\.h" -exec fromdos -d {} \;
find -name "*\.txt" -exec fromdos -d {} \;
find -name "*\.lng" -exec fromdos -d {} \;
find -name "*\.xml" -exec fromdos -d {} \;
find -name "*\.ini" -exec fromdos -d {} \;
find -name "*\.sh" -exec fromdos -d {} \;

