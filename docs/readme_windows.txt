					Mega-Glest
        	        	by Titus Tscharntke and Mark Vejvoda

        			original Glest code
        	        	by Martiño Figueroa

1. System requirements
Developed on Windows with Microsoft Visual C++ 2008 Express (free version), 
little endian CPU. Compiling using the mingw32 toolset is now possible, which
also allows for cross compiling. The game does not currently work on big endian
CPUs like PPC. There are, however, community contributed builds available for 
MacIntel. The app has been reported to run fine on a 900Mhz Athlon box with 
Nvidia GeForce 3 graphics card. It seems that the game also runs on GeForce 2 
and GeForce Mx class hardware when you disable 3D textures and shadow maps in 
the options menu. The windows version runs on 32 and 64 bit versions of Windows 
and has been reported to run successfully on Windows 2000 - Windows 7.

2. Building and Installation

2.1 Prerequesites
The game depends on some tools and libraries to be present, before you can
start compiling it. Here's a list of them:

* Microsoft Visual C++ Express 2008 or higher
OR
* mingw32

* Glest Windows 32 bit Dependencies
  http://sourceforge.net/projects/megaglest/files/win32_deps.7z/download
  to build with VC++ or, to build with mingw:
  http://sourceforge.net/projects/megaglest/files/win32_deps_mingw.7z/download

which include:

* Xerces-C
  http://xerces.apache.org/xerces-c/

* wxWidgets
  http:/wxwidgets.org/downloads/

2.2 Building

To build and install the game use the following steps:

1. Download the dependencies archive mentioned above and decompress its contents
into the 'source' folder (where you see glest_game, g3d_viewer, etc).
This should create a folder called "win32_deps" with many sub files/folders 
in it.

2. Open the VC++ 2008 Solution file in trunk\mk\windoze called Glest.sln within
   the Visual C++ IDE.

3. Right Click on the top level Glest node in Solution Explorer and select
   Rebuild All.

If you had no errors all binaries will be created in trunk\data\glest_game.
Before running MegaGlest you must run the batchfile CopyWindowsRuntimeDlls.bat
You should now be able to just run glest_game.exe.

2.3 Installation

We provide NSIS based installation packages. By default, these will create a 
system-wide installation below Program Files and setup Desktop icons for
simplified access.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

4. Contact

The main MegaGlest website is http://megaglest.org/
Development information is available at http://wiki.megaglest.org/
Forums are at http://forums.megaglest.org/

Titus Tscharntke (info@titusgames.de) 
Mark Vejvoda (www.soft-haus.com - mark_vejvoda@hotmail.com)
