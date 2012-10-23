
                                  MEGAGLEST

                     by Titus Tscharntke and Mark Vejvoda

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                         Version Release instructions for Linux Only
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Source and Data Archives:
=========================

There are 3 archives that are built for a given release (starting with versions 
after 3.6.0). In order to build a release you must have all files checked out
of svn for that specific release (example: trunk)

To set the current version for release, modify the file mk/linux/mg-version.sh
and change the two variables:

OLD_MG_VERSION=3.6.0.1
MG_VERSION=3.6.0.2

*Note: Please ensure the MG_VERSION variable matches the version set in the 
MegaGlest binary, this can be checked by running: ./megaglest --version
It is possible to have the mg-version.sh script NOT match the binary in cases
where we are releasing binary compatible updates to the archives in which case 
we typically use a forth digit in the version # (as seen above)

After setting the correct verions in mg-version.sh you should call:
./mg-version-synch.sh

This will update associated installers to use the correct version stamps

#1. The source archive:
This archive contains source code for binary compilation of the application and 
tools

The naming convention for the source archive is:
megaglest-source-<VERSION>.tar.xz

To build this archive open a terminal and from the mk/linux folder run:
./makerelease.sh

This will produce megaglest-source-<VERSION>.tar.xz in the release subfolder

#2. The embedded source archive:
This archive contains 3rd party source code for binary compilation of the application and 
tools (which is often already included in Linux distros)

The naming convention for the source archive is:
megaglest-source-embedded-<VERSION>.tar.xz

To build this archive open a terminal and from the mk/linux folder run:
./makerelease-embedded.sh

This will produce megaglest-source-embedded-<VERSION>.tar.xz in the release subfolder

#3. The data archive:
This archive contains pre-compiled data content which is used by the game and 
its tools

The naming convention for the data archive is:
megaglest-data-<VERSION>.tar.xz

To build this archive open a terminal and from the mk/linux folder run:
./makedata.sh

This will produce megaglest-data-<VERSION>.tar.xz in the release subfolder


#4. The data source archive:
This archive contains data source (such as .blend files) for people to mod 
original game data content

The naming convention for the data source archive is:
megaglest-data-source-<VERSION>.tar.xz

To build this archive open a terminal and from the mk/linux folder run:
./makedata-source.sh

This will produce megaglest-data-source-<VERSION>.tar.xz in the release subfolder
*NOTE: Currently this script only works for the trunk level release

Once these files have been built they should be ftp'd to the sourceforge
files repository and and announcement made to the community. The folder on 
sourceforge where these files belong would following this naming convention:

http://sourceforge.net/projects/megaglest/files/megaglest_<VERSION>/

#5 To produce a standalone gcc based binary and data archive you may run:
./make-binary-archive.sh
./make-data-archive.sh

These will produce archives in the release folder name:
megaglest-binary-*
megaglest-standalone-data-*

Linux Installer(s):
=========================

#1 *Note: This particular step is only required once and is intended to setup
mojosetup on the platform that is building the installer.

For either 32 or 64 bit Linux installers open a terminal and navigate to:

mk/linux/mojosetup

mkdir build
cd build
cmake ../
make
cd ../

-----------------------
-- deprecation start --
NOTE this part is now deprecated as it is handled by the synch script above
but the info is left here for education

#2 Navigate into the megaglest-installer subfolder and modify  / save changes:

- config.lua
local GAME_INSTALL_SIZE = 680000000;
local GAME_VERSION = "3.6.0";
-- deprecation end --
---------------------

Now in a terminal session from inside the megaglest-installer folder run:

./make.sh

When complete this will produce the platform specific installer in the same
folder called: 

MegaGlest-Installer_<architecture>_<kernel>.run

This is a native binary installer that wil install MegaGlest on the same 
platform as was sued to build it. (ie: 32 or 64 bit Linux)

