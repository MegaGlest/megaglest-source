
                                  MEGAGLEST

                     by Titus Tscharntke and Mark Vejvoda

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                        Build Instructions for Windows
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 1. Architecture ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Developed on Windows with Microsoft Visual Studio Community 2015,
little endian CPU. Compiling using the mingw32 toolset is experimental, which
also allows for cross compiling.

On Debian GNU/Linux systems please find license information in: 
/usr/share/common-licenses
Also see Build Instructions for Linux

~~~~~~~~~~~~~~~~~~~~~~~~ 2. Building and Installation ~~~~~~~~~~~~~~~~~~~~~~~~

--- 2.1 Prerequesites ---

To build the game, the following tools and libraries must be present:

- Pick 1:
* Microsoft Visual Studio Community 2015
  https://www.microsoft.com/en-us/download/details.aspx?id=48146
  (Experimental Microsoft Visual C++ Express 2012 projects also exist in the vc2012
  folder)
* mingw32
  http://mingw.org/
  
- Pick 1:
* MegaGlest dependencies for building with VC++, which which will be downloaded
  automatically in 'Option A' (see below)
* MegaGlest Windows 32 bit dependencies for building with mingw:
  https://github.com/MegaGlest/megaglest-source/releases/download/3.2.3/win32_deps_mingw.7z

- These include:
* Xerces-C
  http://xerces.apache.org/xerces-c/
* wxWidgets
  http://wxwidgets.org/
* And many more. For a more verbose list, please inspect the archive contents and refer to the
  Linux build instructions.


--- 2.2 Building ---

To build and install the game proceed as follows:

*** Option A ~ (recommended) Automated build on the command line: 

1. Open a command prompt and navigate to the root folder where you have acquired the source code.

2. cd mk\windoze

3. build-mg-2015.bat (or build-mg32bit-2015.bat for 32 bit build)

4. megaglest.exe --version

5. megaglest.exe

*** Option B ~ Using VC++ IDE:

1. Download the dependencies archive (which one and from where you can find
   out in the "code" of build-...bat script from 'Option A' approach, some line
   with "wget.exe") and decompress its contents into the 'source' directory. This
   should create a subdirectory called "windows_deps..." with many files and
   subdirectories in it.

2. Start the Visual C++ 2015 IDE and open the solution file:
   \mk\windoze\Glest_vc2015.sln

3. Right Click on the top level 'Glest' node in Solution Explorer and select
   'Rebuild All' (choose release mode or release with debug info).

Note: If you had no errors all binaries will be created.
      Before running MegaGlest you must run CopyWindowsRuntimeDlls_2015.bat.
      You should now be able to just run megaglest.exe.


--- 2.3 Installation ---

We provide NSIS based installation packages. By default, these will create a
system-wide installation below %ProgramFiles% and setup Desktop icons for
simplified access. User specific configuration will be stored within the
directory tree the %AppData% environment variable points to.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 3. Troubleshooting ~~~~~~~~~~~~~~~~~~~~~~~~~~~~

--- General ---
* Make sure that the hardware and software of your system match the 
  requirements

* If you cannot find what you are looking for on here please check the FAQs
  (https://docs.megaglest.org/MG/FAQ) before contacting.
  
--- Compiling ---
* If configure fails make sure you have read the Building section above

--- Sound/Audio errors when starting ---
* If the game doesn't start because of audio/sound errors:
  Make sure no other application is using your soundcard.
* If this doesn't solve your sound problems try to get an updated OpenAL from
  http://openal.org
* Sound is played through OpenAL - you might need to take a look at your
  configuration: http://supertux.lethargik.org/wiki/OpenAL_Configuration

--- The game complains that OpenGL 1.3: is not available, is missing OpenGL 
    extensions, or works very slowly ---
* Try to get updated graphics drivers.

-- The game crashes ---
* Check the forums at http://forums.megaglest.org/
* Please report any other crashes and freezes that are not yet described on 
  the forums, preferably with a backtrace from a debugging enabled build
  
~~~~~~~~~~~~~~~~~~~~~~~~~~ 4. More information ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* Website
  http://megaglest.org/

* Wiki
  https://docs.megaglest.org/Main_Page

* Forums
  http://forums.megaglest.org/

~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 5. Contact + Credits ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* MegaGlest is developed by:
    Titus Tscharntke (info@titusgames.de)
    Mark Vejvoda (www.soft-haus.com - mark_vejvoda@hotmail.com)

* General contact:
    contact@megaglest.org

* MegaGlest is a fork of Glest:
    http://glest.org/

* Linux port by:
    Matthias Braun 
    <matze@braunis.de> 
  
    with help from 
  
    Karl Robillard
    <krobbillard@san.rr.com>

*** Please also refer to the copyright file. ***
