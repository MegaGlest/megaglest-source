
                                  MEGAGLEST

                     by Titus Tscharntke and Mark Vejvoda

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                             README file for Linux
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. System requirements

Developed on Linux with glibc, little endian CPU. While MacIntel builds exist
(for some versions of the game), MegaGlest does not currently work on big
endian CPUs like PPC (though some unfinished patches for vanilla Glest float
around on the Glest boards, e.g. http://glest.org/glest_board/?topic=1426).

Hardware requirements:
* 5th generation x86 CPU with 1.5 GHz or better
  (modern CPU series with at least two cores of at least 1.5 Ghz recommended)
* 1.0 GB RAM
  (2.0 GB recommended)
* Graphics chip supporting OpenGL 1.3 with GL_ARB_env_crossbar and shader
  extensions (=OpenGL 1.4 or glUseProgramObjectARB etc.) or higher
  (dedicated video card with hardware 3D acceleration recommended)
* Audio chip supporting OpenAL

Software requirements:
* A supported (by its producer) operating system version
* Graphics drivers which work well with this operating system version and
  support the OpenGL requirements discussed above
* Audio drivers supporting OpenAL
* A file archiving utility which provides a command line interface and can
  decompress 7-zip archives

Current dedicated Nvidia and ATI hardware with vendor-supplied proprietary
drivers installed will provide the best experience. However, MegaGlest also
runs on most integrated Intel GMA, Nvidia and ATI GPUs, but you will feel an
urge to reduce effects.
Issues with proprietary drivers for 'legacy' ATI hardware have been and are
continuously reported. On Linux, open source 3D drivers are now (Linux 2.6.38,
Gallium 0.4 via DRI) starting to become usable for both many current and
legacy GPUs, and may help with this.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

2. Building and Installation

2.1 Prerequesites

The game depends on some tools and libraries to be present, before you can
start compiling it. Here's a list of them:

* normal GNU compiler and additional tools (g++ version 3.2 or later is
  required at the moment)

* Kitware CMake 2.8 or later (used as build tool)

* X11 libraries + headers
  http://x.org/

* SDL 1.2.5 or later
  http://libsdl.org/

* OpenGL
  http://dri.freedesktop.org/wiki/libGL

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

At this point I'd like to thank all the authors of these helpful libraries that
made our development easy and straight forward.


2.2 Building

To build the game simply invoke the build script:

./build-mg.sh


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

The game complains about OpenGL1.3 not available, is missing OpenGL extensions
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

MegaGlest is a fork of Glest:
  http://glest.org/

Linux port by:
  Matthias Braun <matze@braunis.de> with help from Karl Robillard
  <krobbillard@san.rr.com>

Please also refer the copyright file.
