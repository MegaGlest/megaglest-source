#! /bin/sh
# Use this script to set data files svn properties quickly
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2013 Mark Vejvoda under GNU GPL v3.0+

CURRENTDIR="$(dirname $(readlink -f $0))"

# cpp /.c / .h files
find ${CURRENTDIR}/../../source/ -iname '*.cpp' -exec svn propset svn:mime-type text/plain '{}' \;
find ${CURRENTDIR}/../../source/ -iname '*.cpp' -exec svn propset svn:eol-style native '{}' \;
find ${CURRENTDIR}/../../source/ -iname '*.c' -exec svn propset svn:mime-type text/plain '{}' \;
find ${CURRENTDIR}/../../source/ -iname '*.c' -exec svn propset svn:eol-style native '{}' \;
find ${CURRENTDIR}/../../source/ -iname '*.h' -exec svn propset svn:mime-type text/plain '{}' \;
find ${CURRENTDIR}/../../source/ -iname '*.h' -exec svn propset svn:eol-style native '{}' \;

# LNG files
#find ${CURRENTDIR}/../../data/glest_game/ -name "*\.lng" -exec echo {} \;
find ${CURRENTDIR}/../../data/glest_game/ -iname '*.lng' -exec svn propset svn:mime-type text/plain '{}' \;
find ${CURRENTDIR}/../../data/glest_game/ -iname '*.lng' -exec svn propset svn:eol-style native '{}' \;

# XML files
find ${CURRENTDIR}/../../data/glest_game/ -iname '*.xml' -exec svn propset svn:mime-type application/xml '{}' \;
#find ${CURRENTDIR}/../../data/glest_game/ -iname '*.xml' -exec svn propset svn:eol-style native '{}' \;

# shell scripts
find ${CURRENTDIR}/ -iname '*.sh' -exec svn propset svn:mime-type text/plain '{}' \;
find ${CURRENTDIR}/ -iname '*.sh' -exec svn propset svn:eol-style native '{}' \;

# php scripts
find ${CURRENTDIR}/../../source/masterserver -iname '*.php' -exec svn propset svn:mime-type text/plain '{}' \;
find ${CURRENTDIR}/../../source/masterserver -iname '*.php' -exec svn propset svn:eol-style native '{}' \;

# sql scripts
find ${CURRENTDIR}/../../source/masterserver -iname '*.sql' -exec svn propset svn:mime-type text/plain '{}' \;
find ${CURRENTDIR}/../../source/masterserver -iname '*.sql' -exec svn propset svn:eol-style native '{}' \;

# javascript scripts
find ${CURRENTDIR}/../../source/masterserver -iname '*.js' -exec svn propset svn:mime-type text/plain '{}' \;
find ${CURRENTDIR}/../../source/masterserver -iname '*.js' -exec svn propset svn:eol-style native '{}' \;

# css scripts
find ${CURRENTDIR}/../../source/masterserver -iname '*.css' -exec svn propset svn:mime-type text/plain '{}' \;
find ${CURRENTDIR}/../../source/masterserver -iname '*.css' -exec svn propset svn:eol-style native '{}' \;

