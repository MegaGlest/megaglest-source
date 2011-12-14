
                                  MEGAGLEST

                     by Titus Tscharntke and Mark Vejvoda

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                        Build instructions for Windows
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. Architecture

Developed on Windows with Microsoft Visual C++ 2008 Express (free version),
little endian CPU. Compiling using the mingw32 toolset is now possible, which
also allows for cross compiling.

On debian systems please find license information in: /usr/share/common-licenses
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
* For a more verbose list, please inspect the archive contents and refer to the
  Linux build instructions.


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
  Make sure no other application is using your soundcard.
* If this doesn't solve your sound problems try to get an updated OpenAL from
  http://openal.org

The game complains about OpenGL 1.3 not available, is missing OpenGL extensions
or works very slowly:
* Try to get updated graphics drivers.

The game crashes:
* Check the forums at http://forums.megaglest.org/
* It would be nice if you could report any other crashes and freezes that are
  not yet described on the forums, preferably with a backtrace from a
  debugging enabled build


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
