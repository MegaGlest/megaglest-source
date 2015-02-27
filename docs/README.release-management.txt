
                                  MEGAGLEST

                     by Titus Tscharntke and Mark Vejvoda

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                         Version Release instructions for Linux Only
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Source and Data Archives:
=========================

There are 3 archives that are built for a given release (starting with versions 
after 3.6.0). In order to build a release you must have all git repositories
checked out for that specific release (example, tag: 3.11.0) or downloaded all
source archives/"tarballs" which in their name have same version, equal to tag.

To set the current version for release, modify the file source/version.txt
and change there available variables, then run mk/linux/mg-version-synch.sh script
for updating version number everywhere where it is needed and then you only have
to commit changed files.

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

To stamp a snapshot in git for a release we use the following 
(substitute version #'s of course and shasum to be the commit shasum):

git tag 3.9.2 <shasum>
git push --tags

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

