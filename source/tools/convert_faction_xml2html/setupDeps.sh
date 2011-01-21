#!/bin/sh

OSTYPE=`uname -m`
echo "Detected distro: [$OSTYPE]"

if [ -f /etc/fedora-release ]; then
  echo "Fedora..."
  #sudo yum groupinstall "Development Tools"
  #sudo yum install subversion automake autoconf autogen jam
elif [ -f /etc/SuSE-release ]; then
  echo "SuSE..."
  #sudo zypper install subversion gcc gcc-c++ automake
else
  echo "Debian / Ubuntu..."
  sudo apt-get install perl
  sudo apt-get install graphviz
  sudo apt-get install libgraphviz-perl
  sudo apt-get install libconfig-inifiles-perl
fi

echo "To run the techtree html builder edit mg.ini and run the script as follows:"
echo "./convert_faction_xml2html.pl mg.ini"

