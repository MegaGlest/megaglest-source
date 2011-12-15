
                                  MEGAGLEST

                     by Titus Tscharntke and Mark Vejvoda

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                         Version Release instructions for Linux Only
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

There are 3 archives that are built for a given release (starting with versions 
after 3.6.0). In order to build a release you must have all files checked out
of svn for that specific release (example: trunk)

To set the current version for release, modify the file mk/linux/mg-version.sh
and change the two variables:

OLD_MG_VERSION=3.6.0.1
MG_VERSION=3.6.0.2

*Note: Please ensure the MG_VERSION variable matches the version set in the 
MegaGlest binary, thius can be checked by running: ./megaglest --version
It is possible to have the mg-version.sh script NOT match the binary in cases
where we are releasing binary compatible updates to the archives in which case 
we typically use a forth digit in the version # (as seen above)

#1. The source archive:
This archive contains source code for binary compilation of the application and 
tools

The naming convention for the source archive is:
megaglest-source-<VERSION>.tar.xz

To build this archive open a terminal and from the mk/linux folder run:
./makerelease.sh

This will produce megaglest-source-<VERSION>.tar.xz in the release subfolder

#2. The data archive:
This archive contains pre-compiled data content which is used by the game and 
its tools

The naming convention for the data archive is:
megaglest-data-<VERSION>.tar.xz

To build this archive open a terminal and from the mk/linux folder run:
./makedata.sh

This will produce megaglest-data-<VERSION>.tar.xz in the release subfolder


#3. The data source archive:
This archive contains data source (such as .blend files) for people to mod 
original game data content

The naming convention for the data source archive is:
megaglest-data-source-<VERSION>.tar.xz

To build this archive open a terminal and from the mk/linux folder run:
./makedata-source.sh

This will produce megaglest-data-source-<VERSION>.tar.xz in the release subfolder
*NOTE: Currently this script only works for the trunk level release

Once these files
