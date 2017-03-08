#!/bin/sh
#
# Use this script to make autodocumentation for docs in data's repository
# ----------------------------------------------------------------------------
# 2015 Written by filux <heross(@@)o2.pl>
# Copyright (c) 2015-2016 under GNU GPL v3.0+
LANG=C

if [ "$1" = "" ]; then
	techtree="megapack"
else
	techtree="$1"
fi
if [ "$2" = "" ]; then
	rel_path_docs_icons="../../techs"
else
	rel_path_docs_icons="$2"
fi
if [ "$3" = "" ]; then
	rel_path_tech_test="../../../../data/glest_game/techs"
else
	rel_path_tech_test="$3"
fi
if [ "$4" = "" ]; then
	remote_logo="https://megaglest.org/uploads/megaglest2011/logo/logo.png"
else
	remote_logo="$4"
fi
if [ "$5" = "" ]; then
	remote_footer_logo="https://megaglest.org/uploads/megaglest2011/logo/gpl_osi.png"
else
	remote_footer_logo="$5"
fi
SCRIPTDIR="$(dirname "$(readlink -f "$0")")"
cd "$SCRIPTDIR"
rm -rf html; sleep 0.1s
mkdir -p html/images
sed "s|^generate_g3d_images = .*|generate_g3d_images = 0|" "$techtree.ini" > "$techtree-temp.ini"
if [ "$(which curl 2>/dev/null)" != "" ]; then
	if [ "$(grep -c "$remote_logo" "$techtree-temp.ini")" -gt "0" ]; then
		curl -L -s "$remote_logo" -o "html/images/logo.png"
		if [ "$?" -eq "0" ]; then sed -i "s|$remote_logo|images/logo.png|" "$techtree-temp.ini"; sleep 0.1s; fi
	fi
	if [ "$(grep -c "$remote_footer_logo" "$techtree-temp.ini")" -gt "0" ]; then
		curl -L -s "$remote_footer_logo" -o "html/images/footer_logo.png"
		if [ "$?" -eq "0" ]; then sed -i "s|$remote_footer_logo|images/footer_logo.png|" "$techtree-temp.ini"; sleep 0.1s; fi
	fi
else
	echo "Downloading tool 'curl' DOES NOT EXIST on this system, please install it."
fi
./convert_faction_xml2html.pl "$techtree-temp.ini"

cd html
icons_list1="$(grep -o "images/[^/ ]*/[^/ ]*/[^. ]*.bmp" *.html)"
icons_list2="$(grep -o "images/[^/ ]*/[^/ ]*/[^. ]*.jpg" *.html)"
icons_list3="$(grep -o "images/[^/ ]*/[^/ ]*/[^. ]*.png" *.html)"
icons_list="$(echo -e "$icons_list1 $icons_list2 $icons_list3" | xargs)"
echo ">>> replacing icon for (unit='|', upgrade='\', resource='/') ..."
for icon_file in $icons_list; do
	filename="$(echo "$icon_file" | awk -F ':' '{print $1}')"
	icon="$(echo "$icon_file" | awk -F ':' '{print $2}')"

	faction="$(echo "$icon" | awk -F '/' '{print $2}')"
	uu_name="$(echo "$icon" | awk -F '/' '{print $3}')"
	bmp_file="$(echo "$icon" | awk -F '/' '{print $4}')"
	find_file="$(find $rel_path_tech_test/$techtree/factions/$faction -type f -name "$bmp_file")"

	echo "$find_file" | grep -q "factions/$faction/upgrades"
	if [ "$?" -eq "0" ]; then :
		sed -i "s|images/$faction/$uu_name/$bmp_file|$rel_path_docs_icons/$techtree/factions/$faction/upgrades/$uu_name/images/$bmp_file|" "$filename"
		echo -n "\\"
	else
		echo "$find_file" | grep -q "factions/$faction/units"
		if [ "$?" -eq "0" ]; then :
			sed -i "s|images/$faction/$uu_name/$bmp_file|$rel_path_docs_icons/$techtree/factions/$faction/units/$uu_name/images/$bmp_file|" "$filename"
			echo -n "|"
		fi
	fi
	sleep 0.01s
done
icons_list1="$(grep -o "images/resources/[^. ]*.bmp" *.html)"
icons_list2="$(grep -o "images/resources/[^. ]*.jpg" *.html)"
icons_list3="$(grep -o "images/resources/[^. ]*.png" *.html)"
icons_list="$(echo -e "$icons_list1 $icons_list2 $icons_list3" | xargs)"
for icon_file in $icons_list; do
	filename="$(echo "$icon_file" | awk -F ':' '{print $1}')"
	icon="$(echo "$icon_file" | awk -F ':' '{print $2}')"

	bmp_file="$(echo "$icon" | awk -F '/' '{print $3}')"
	res_name="$(echo "$bmp_file" | awk -F '.' '{print $1}')"
	sed -i "s|images/resources/$bmp_file|$rel_path_docs_icons/$techtree/resources/$res_name/images/$bmp_file|" "$filename"
	echo -n "/"
	sleep 0.01s
done
echo

echo ">>> removing not required files ..."
rm -rf css js
find ./images/*/* -type d | xargs rm -rf
rm -rf images/datatables images/resources
find ./images/* -type f -name "*.canon" | xargs rm -f
find ./images/* -type f -name "*.cmapx" | xargs rm -f
find ./images/* -type f -name "*.text" | xargs rm -f

rm -f all.html
all_html_list="$(grep '"all.html"' *.html | awk -F ':' '{print $1}' | xargs)"
for filename in $all_html_list; do
	sed -i '/All Units of all Factions/d' "$filename"
done

if [ "$(which optipng)" != "" ]; then
	echo ">>> optimizing .png files ..."
	find ./images -type f -name '*.png' -exec optipng '{}' \;
else
	echo "Warning: 'optipng' not found"
fi
cd "$SCRIPTDIR"
rm -f "$techtree-temp.ini"
