#! /bin/sh
# Use this script to set data files svn properties quickly
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2013 Mark Vejvoda under GNU GPL v3.0+

CURRENTDIR="$(dirname $(readlink -f $0))"

# lng files
#find ${CURRENTDIR}/../../data/glest_game/ -name "*\.lng" -exec echo {} \;
find ${CURRENTDIR}/../../data/glest_game/ -name "*\.lng" -exec svn propset svn:mime-type text/plain {} \;
find ${CURRENTDIR}/../../data/glest_game/ -name "*\.lng" -exec svn propset svn:eol-style native {} \;

# scripts
find ${CURRENTDIR}/ -name "*\.sh" -exec svn propset svn:mime-type text/plain {} \;
find ${CURRENTDIR}/ -name "*\.sh" -exec svn propset svn:eol-style native {} \;

