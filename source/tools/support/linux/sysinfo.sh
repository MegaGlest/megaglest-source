#!/bin/sh
#
# This script dumps this systems' hardware specs and software info to OUTFILE.
# Place it next to your megaglest binary, then run it.
#
# -----------------------------------------------------------------------------
#
# Configuration section

OUTFILE=sysinfo.log

# End of configuration section
#
# -----------------------------------------------------------------------------
#

VERSION='0.2'

echo 'Gathering information on this computer and MegaGlest, please stand by.'

echo '--------------------------------------------------------------------------------' >> $OUTFILE
echo '                          MEGAGLEST SYSTEM REPORT '"$VERSION" >> $OUTFILE
echo '                        '"`date -Ru`" >> $OUTFILE
echo '--------------------------------------------------------------------------------' >> $OUTFILE
echo '' >> $OUTFILE
echo '' >> $OUTFILE
echo '* CPU' | tee -a $OUTFILE
echo '' >> $OUTFILE
echo '>>> cat /proc/cpuinfo | awk -F : '"'"'/^model name/ { print $2 }' >> $OUTFILE
cat /proc/cpuinfo | awk -F : '/^model name/ { print $2 }' >> $OUTFILE
sleep 1

echo '' >> $OUTFILE
echo '' >> $OUTFILE
echo '* Memory' | tee -a $OUTFILE
echo '' >> $OUTFILE
echo '>>> free -mt' >> $OUTFILE
free -mt >> $OUTFILE
sleep 1

echo '' >> $OUTFILE
echo '' >> $OUTFILE
echo '* Graphics' | tee -a $OUTFILE
echo '' >> $OUTFILE

echo ">>> lspci -knnv | grep -EA10 '(VGA|Display)'" >> $OUTFILE
lspci -knnv | grep -EA10 '(VGA|Display)' >> $OUTFILE
sleep 1
echo '' >> $OUTFILE
echo '>>> glxinfo' >> $OUTFILE
glxinfo >> $OUTFILE 2>&1
sleep 1
echo '' >> $OUTFILE
echo '>>> xrandr' >> $OUTFILE
xrandr >> $OUTFILE  2>&1
sleep 1
echo '  (I will try to start MegaGlest now, but it should quit automatically.)'
echo '' >> $OUTFILE
echo '>>> ./start_megaglest --opengl-info' >> $OUTFILE
./start_megaglest --opengl-info >> $OUTFILE 2>&1
sleep 1

echo '' >> $OUTFILE
echo '' >> $OUTFILE
echo '* Operating system' | tee -a $OUTFILE
echo '' >> $OUTFILE
echo '>>> uname -a' >> $OUTFILE
uname -a >> $OUTFILE 2>&1
sleep 1
echo '' >> $OUTFILE
echo '>>> cat /etc/issue' >> $OUTFILE
cat /etc/issue >> $OUTFILE 2>&1
sleep 1

echo '' >> $OUTFILE
echo '' >> $OUTFILE
echo '* MegaGlest version' | tee -a $OUTFILE
echo '' >> $OUTFILE
echo '>>> ./start_megaglest --version' >> $OUTFILE
./start_megaglest --version >>$OUTFILE 2>&1
#v3.5.2-GNUC: 40401 [64bit]-May 26 2011 09:59:59, SVN: [Rev: 2305], [STREFLOP]
sleep 1

echo '' >> $OUTFILE
echo '--------------------------------------------------------------------------------' >> $OUTFILE
echo '' >> $OUTFILE
echo '' >> $OUTFILE
echo '' >> $OUTFILE

echo ''
echo 'Processing complete.'
sleep 1
echo ''
echo 'Please find your report in this file:'
echo '  '"$OUTFILE"
echo ''
echo 'Please post this report to http://paste.megaglest.org/'
echo 'After posting it, you will be taken to a new location. Please take note of'
echo 'this new location (URI/Internet address) and send it to the MegaGlest team:'
echo '  mailto:support@megaglest.org'
echo 'Please be sure to add a proper description of the problems you experience.'
echo ''
echo 'Alternatively you can just post the report and your description to the forums:'
echo '  http://forums.megaglest.org/'
echo ''
echo 'Press Enter to exit.'
read input >/dev/null
