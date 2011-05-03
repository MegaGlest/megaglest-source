
                                  MEGAGLEST

                     by Titus Tscharntke and Mark Vejvoda

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                           README file for Windows
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. System requirements

Developed on Windows with Microsoft Visual C++ 2008 Express (free version),
little endian CPU. Compiling using the mingw32 toolset is now possible, which
also allows for cross compiling.

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

The Windows version runs on 32 and 64 bit variants of Windows and has been
reported to run on Windows versions up to and including Windows 7.

Current dedicated Nvidia and ATI hardware with vendor-supplied proprietary
drivers installed will provide the best experience. However, MegaGlest also
runs on most integrated Intel GMA, Nvidia and ATI GPUs, but you will feel an
urge to reduce effects.

Issues with proprietary drivers for 'legacy' ATI hardware have been and are
continuously reported. On Windows, 'patched (current) drivers' can sometimes
help there (and other times they make things worse).

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

2. Building and Installation

2.1 Prerequesites

The game depends on some tools and libraries to be present, before you can
start compiling it. Here's a list of them:

* Microsoft Visual C++ Express 2008
OR
* mingw32

* MegaGlest Windows 32 bit dependencies for building with VC++:
  http://sourceforge.net/projects/megaglest/files/win32_deps.7z/download
OR
* MegaGlest Windows 32 bit dependencies for building with mingw:
  http://sourceforge.net/projects/megaglest/files/win32_deps_mingw.7z/download

These include:
* Xerces-C
  http://xerces.apache.org/xerces-c/
* wxWidgets
  http://wxwidgets.org/


2.2 Building

To build and install the game use the following steps:

1. Download the dependencies archive listed above and decompress its contents
   into the 'source' directory (where you see glest_game, g3d_viewer, etc).
   This should create a subdirectory called "win32_deps" with many files and
   subdirectories in it.

2. Open the VC++ 2008 Solution file in trunk\mk\windoze called Glest.sln within
   the Visual C++ IDE.

3. Right Click on the top level Glest node in Solution Explorer and select
   'Rebuild All'.

If you had no errors all binaries will be created in trunk\data\glest_game.
Before running MegaGlest you must run the CopyWindowsRuntimeDlls.bat batch.
You should now be able to just run glest_game.exe.


2.3 Installation

We provide NSIS based installation packages. By default, these will create a
system-wide installation below Program Files and setup Desktop icons for
simplified access. User specific configuration will be stored within the
directory tree the %AppData% environment variable points to.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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
