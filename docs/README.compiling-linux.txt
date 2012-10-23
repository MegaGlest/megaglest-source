
                                  MEGAGLEST

                     by Titus Tscharntke and Mark Vejvoda

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                         Build instructions for Linux
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. Architecture

Developed on Linux with glibc, little endian CPU. While MacIntel builds exist
(for some versions of the game), MegaGlest does not currently work on big
endian CPUs like PPC (though some unfinished patches for vanilla Glest float
around on the Glest boards, e.g. http://glest.org/glest_board/?topic=1426).


On debian systems please find license information in: /usr/share/common-licenses
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

2. Building and Installation

2.1 Prerequesites

The game depends on some tools and libraries to be present, before you can
start compiling it:

* Standard GNU compiler and additional tools (g++ version 3.2 or later is
  required at the moment)

* Kitware CMake 2.8 or later (used as build tool)

* X11 libraries + headers
  http://x.org/

* SDL 1.2.5 or later
  http://libsdl.org/

* OpenGL
  http://dri.freedesktop.org/wiki/libGL

* libvlc
  http://www.videolan.org/vlc/libvlc.html

* libcurl
  http://curl.haxx.se/libcurl/

* wxWidgets
  http://wxwidgets.org/

* Xerces-C
  http://xerces.apache.org/xerces-c/

* OpenAL
  http://openal.org/

* Ogg
  http://xiph.org/ogg/

* Vorbis
  http://xiph.org/vorbis/

* Xerces-C
  http://xerces.apache.org/xerces-c/

* Lua 5.1 or later
  http://www.lua.org/

* JPEG
  http://www.ijg.org/

* PNG
  http://www.libpng.org/

* Zlib
  http://zlib.net/

* GnuTLS
  http://www.gnu.org/software/gnutls/

* ICU
  http://site.icu-project.org/

* libdl

If the configure script can't find some of the libraries, make sure that you
also have the -dev packages installed that some distributions provide.

At this point we would like to thank all the authors of these helpful libraries 
who made our development easy and straight forward.


2.2 Building

To build the game simply invoke the build script:

./build-mg.sh

*NOTE: We have produced a script that tries to install build dependencies
on many Linux distros. The script is located in mk/linux/setupBuildDeps.sh

2.3 Installation

We provide MojoSetup based installers for Linux and NSIS based installers for
Windows. By default, the Linux installers install to your home directory. The
Windows installers install to %ProgramFiles%, so to the systems' global scope.

There are also community maintained packages available for several Linux and
BSD distributions. Please see the website, forums and wiki for details.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

3. Troubleshooting

Some hints for troubleshooting.

In General:
* Make sure both hard- and software of your system match the requirements
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

The game complains about OpenGL 1.3 not available, is missing OpenGL extensions
or works very slowly:
* Look at glxinfo and make sure the system is using the drivers you want to
  use. Often the proprietary ATI or NVIDIA drivers work better, but for Intel,
  Mesa drivers ("glxinfo | grep -i mesa") can work, too (but slowly since these
  GPUs are lacking on hardware acceleration support).

The game crashes:
* Check the forums at http://forums.megaglest.org/
* It would be nice if you could report any other crashes and freezes that are
  not yet described on the forums, preferably with a gdb backtrace from a
  debugging enabled build (cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo)


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

4. More information

* Website
  http://megaglest.org/

* Wiki
  http://wiki.megaglest.org/

* Forums
  http://forums.megaglest.org/


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

5. Contact + Credits

MegaGlest is developed by:
  Titus Tscharntke (info@titusgames.de)
  Mark Vejvoda (www.soft-haus.com - mark_vejvoda@hotmail.com)

General contact:
  contact@megaglest.org

Website:
  http://megaglest.org

MegaGlest is a fork of Glest:
  http://glest.org/

Linux port by:
  Matthias Braun <matze@braunis.de> with help from Karl Robillard
  <krobbillard@san.rr.com>

Please also refer to the copyright file.
