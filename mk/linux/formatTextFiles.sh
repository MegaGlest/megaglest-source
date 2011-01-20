#!/bin/sh

cd ../../
find -name "*\.cpp" -exec fromdos -d {} \;
find -name "*\.c" -exec fromdos -d {} \;
find -name "*\.h" -exec fromdos -d {} \;
find -name "*\.txt" -exec fromdos -d {} \;
find -name "*\.lng" -exec fromdos -d {} \;
find -name "*\.xml" -exec fromdos -d {} \;
find -name "*\.ini" -exec fromdos -d {} \;
find -name "*\.sh" -exec fromdos -d {} \;

