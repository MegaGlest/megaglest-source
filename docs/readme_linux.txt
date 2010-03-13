					Mega-Glest
        	        	by Titus Tscharntke and Mark Vejvoda

        			original Glest linux port
        	        	by Matthias Braun

1. System requirements
Developed on Linux with glibc, little endian cpu. The game does currently not
work on big endian cpus like ppc. There are some unfinished patches floating
around the glest board, so this might improve (feel free to send me
updated/finished ppc patches).
Graphics card + OpenGL libraries that support OpenGL 1.3 and shader extensions
(=opengl 1.4 or glUseProgramObjectARB and co.)
The app has been reported to run fine on a 900Mhz Athlon box with nvidia geforce
3 graphics card.
It seems that the game also runs on geforce 2 and geforce mx class hardware when
you disable 3d textures and shadowmaps in the options menu. The game seems not
to work with the open source (ATI) DRI drivers. It has reported to run nicely
with ATIs proprietary drivers though.


2. Building and Installation

2.1 Prerequesites
The game depends on some tools and libraries to be present, before you can
start compiling it. Here's a list of them:

* normal gnu compiler and additional tools (g++ version 3.2 or later is
  required at the moment)

* perforce jam 2.5 or later (used as build tool)
  ftp://ftp.perforce.com/pub/jam

* X11 libraries +headers
  http://www.x.org (or the older http://www.xfree86.org)

* SDL 1.2.5 or later
  http://www.libsdl.org

* Xerces-C
  http://xml.apache.org/xerces-c/index.html

* OpenAL
  http://www.openal.org

* Ogg
  http://www.xiph.org/ogg/vorbis  

* Vorbis
  http://www.vorbis.com

If the configure script can't find some of the libraries, make sure that you
also have the -dev packages installed that some distributions provide.
At this point I'd like to thank all the authors of these helpful libraries
that made our development easy and straightforward.

2.2 Building

To build and install the game use the following commands

./configure
jam

Note: if you got the game from the subversion repository then you have to
change to the mk/linux directory first and create a configure script:
	cd mk/linux
	./autogen.sh

2.3 Installation

Unfortunately there is no real installation mechanism present yet (jam install
does NOT work correctly). Please copy the executable file named "glest" and the
example linux config "glest.ini" into the directory where you extracted the
datafiles and start the game there.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

3. Troubleshooting

Some hints for troubleshooting.

In General:
* Make sure you fullfill the system requirement above
* Sound is played through OpenAL you might take a look at your openal
  configuration: http://supertux.lethargik.org/wiki/OpenAL_Configuration

Compiling:
* If configure fails make sure you have read the Building section above
* If you don't manage to get it compiled, you can get a precompiled version for
  x86 (32bit) at http://www.lifl.org
Sound/Audio errors when starting
* If the game doesn't start because of audio/sound errors:
	Make sure no other application is using your soundcard.  Typical problems
	are the gnome/kde sound dameons esd and artsd. You can kill these 2
	daemons with 
		killall esd ; killall artsd
* If this doesn't solve your sound problems try to get an updated OpenAL from
  somewhere, unfortunately there hasn't been an official OpenAL version up to
  now so distributions are only packing some preliminary snapshots. Some
  distributions seem to have bad versions that don't work (SuSE 9.2 is such a
  distribution)
  (The compiled binary from lifl.org has openal included and should not be
   affected by this problem)
The game complains about opengl1.3 not available, is missing opengl extensions
or works very slowly
* Make sure you fullfill the system requirements
* look at glxinfo and make sure the system is using ATI or NVIDIA driver and NOT the
  mesa drivers ("glxinfo | grep -i mesa" should return nothing)

The game crashs
* Fedora3 seems to have problems with the precompiled glest binaries -> compile
  the application yourself
* There are some bugs related to the font code which I can't reproduce and am
  unable to fix without further input. Someone can help me with this?
* The game does not work on ppc (big endian architectures) yet
* Check the forums at www.glest.org
* It would be nice if you could report any other crashs that are not described
  here yet (or post them to the forum at www.glest.org) preferably with gdb
  backtrace from a debugging enabled build (configure --enable-debug)

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

4. Contact

Original Glest is from http://www.glest.org
Linux port by Matthias Braun <matze@braunis.de> with help from
	Karl Robillard <krobbillard@san.rr.com>Mega-Glest by:
	Titus Tscharntke (info@titusgames.de) 
	Mark Vejvoda (www.soft-haus.com - mark_vejvoda@hotmail.com ) 
