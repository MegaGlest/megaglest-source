					MegaGlest
        	        	by Titus Tscharntke and Mark Vejvoda

        			original Glest linux port
        	        	by Matthias Braun

1. System requirements
Developed on Linux with glibc, little endian cpu. While MacIntel builds exist,
the game does not currently work on big endian CPUs like PPC. There are some 
unfinished patches floating around the glest board, so this might improve (feel
free to send updated/finished PPC patches).
Graphics card + OpenGL libraries that support OpenGL 1.3 and shader extensions
(=OpenGL 1.4 or glUseProgramObjectARB etc.)
The app has been reported to run fine on a 900Mhz Athlon box with Nvidia GeForce
3 graphics card.
It seems that the game also runs on GeForce 2 and GeForce Mx class hardware when
you disable 3D textures and shadow maps in the options menu. The game seems not
to work with the open source (ATI) DRI drivers. It has reported to run nicely
with ATIs proprietary drivers though.


2. Building and Installation

2.1 Prerequesites
The game depends on some tools and libraries to be present, before you can
start compiling it. Here's a list of them:

* normal GNU compiler and additional tools (g++ version 3.2 or later is
  required at the moment)

* Kitware CMake 2.8 or later (used as build tool)

* X11 libraries +headers
  http://x.org/ (or the older http://xfree86.org/)

* SDL 1.2.5 or later
  http://libsdl.org/

* Xerces-C
  http://xerces.apache.org/xerces-c/

* OpenAL
  http://openal.org/

* Ogg
  http://xiph.org/ogg/vorbis/

* Vorbis
  http://vorbis.com/

If the configure script can't find some of the libraries, make sure that you
also have the -dev packages installed that some distributions provide.
At this point I'd like to thank all the authors of these helpful libraries
that made our development easy and straight forward.

2.2 Building

To build the game simply invoke the build script:

./build-mg.sh


2.3 Installation

We provide MojoSetup based installers for Linux. By default, they install to 
your home directory. There are also community maintained packages available for
several distributions. Please see the forums and wiki for details.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

3. Troubleshooting

Some hints for troubleshooting.

In General:
* Make sure you fullfill the system requirements above
* Sound is played through OpenAL - you might need to take a look at your
  configuration: http://supertux.lethargik.org/wiki/OpenAL_Configuration

Compiling:
* If configure fails make sure you have read the Building section above

Sound/Audio errors when starting:
* If the game doesn't start because of audio/sound errors:
  Make sure no other application is using your soundcard. Typical problems are
  the Gnome/KDE sound dameons esd and artsd. You can kill these daemons with
  	killall esd ; killall artsd
* If this doesn't solve your sound problems try to get an updated OpenAL from
  http://openal.org or a newer repository provided by your distribution.

The game complains about OpenGL1.3 not available, is missing OpenGL extensions
or works very slowly:
* Make sure your system provides for the system requirements
* Look at glxinfo and make sure the system is using the drivers you want to 
  use. Often the proprietary ATI or NVIDIA drivers work better, but for Intel,
  mesa drivers ("glxinfo | grep -i mesa") can work, too (though slowly due to
  limited abilities of these chipsets).

The game crashes:
* Check the forums at http://forums.megaglest.org/
* It would be nice if you could report any other crashes and freezes that are 
  not yet described on the forums, preferably with a gdb backtrace from a 
  debugging enabled build (cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo)

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

4. Contact

Original Glest is from http://glest.org/
Linux port by: 
  Matthias Braun <matze@braunis.de> with help from 
  Karl Robillard <krobbillard@san.rr.com>
MegaGlest by:
  Titus Tscharntke (info@titusgames.de)
  Mark Vejvoda (www.soft-haus.com - mark_vejvoda@hotmail.com)
