#!/bin/sh
# Use this script to improve configuration of '.desktop' files.
# ----------------------------------------------------------------------------
# 2014 Written by filux <heross(@@)o2.pl>
# Copyright (c) 2014-2016 under GNU GPL v3.0+
LANG=C

CURRENTDIR="$(dirname "$(readlink -f "$0")")"
cd "$CURRENTDIR"; prmtr="Icon="; prmtr2="Exec="
if [ -f "megaglest.desktop" ] && [ -f "megaglest.png" ] && [ -f "megaglest" ] \
    && [ -f "start_megaglest" ] && [ ! -f "glest-dev.ini" ]; then
    desktop_location="$CURRENTDIR/megaglest.desktop"; icon_location="$CURRENTDIR/megaglest.png"
    exec_location="$CURRENTDIR/start_megaglest"
    sed -i -e "s#$prmtr.*#$prmtr$icon_location#" -e "s#$prmtr2.*#$prmtr2\"$exec_location\"#" \
	"$desktop_location"
    chmod +x "$desktop_location"
fi
if [ -f "megaglest_editor.desktop" ] && [ -f "megaglest.png" ] && [ -f "megaglest_editor" ] \
    && [ -f "start_megaglest_mapeditor" ] && [ ! -f "glest-dev.ini" ]; then
    desktop_location="$CURRENTDIR/megaglest_editor.desktop"
    icon_location="$CURRENTDIR/megaglest.png"; exec_location="$CURRENTDIR/start_megaglest_mapeditor"
    sed -i -e "s#$prmtr.*#$prmtr$icon_location#" -e "s#$prmtr2.*#$prmtr2\"$exec_location\"#" \
	"$desktop_location"
    chmod +x "$desktop_location"
fi
if [ -f "megaglest_g3dviewer.desktop" ] && [ -f "megaglest.png" ] && [ -f "megaglest_g3dviewer" ] \
    && [ -f "start_megaglest_g3dviewer" ] && [ ! -f "glest-dev.ini" ]; then
    desktop_location="$CURRENTDIR/megaglest_g3dviewer.desktop"
    icon_location="$CURRENTDIR/megaglest.png"; exec_location="$CURRENTDIR/start_megaglest_g3dviewer"
    sed -i -e "s#$prmtr.*#$prmtr$icon_location#" -e "s#$prmtr2.*#$prmtr2\"$exec_location\"#" \
	"$desktop_location"
    chmod +x "$desktop_location"
fi
