					Mega-Glest
        	        	by Titus Tscharntke and Mark Vejvoda

        			original Glest linux port
        	        	by Matthias Braun

1. System requirements
Developed on Windows with Microsoft Visual C++ 2008 Express (free version), 
little endian cpu. The game does currently not work on big endian cpus like ppc. 
There are some unfinished patches floating around the glest board, so this might 
improve (feel free to send me updated/finished ppc patches).
Graphics card + OpenGL libraries that support OpenGL 1.3 and shader extensions
(=opengl 1.4 or glUseProgramObjectARB and co.)
The app has been reported to run fine on a 900Mhz Athlon box with nvidia geforce
3 graphics card.
It seems that the game also runs on geforce 2 and geforce mx class hardware when
you disable 3d textures and shadowmaps in the options menu.

2. Building and Installation

2.1 Prerequesites
The game depends on some tools and libraries to be present, before you can
start compiling it. Here's a list of them:

* Microsoft Visual C++ Express 2008 or higher

* Glest Windows 32 bit Dependencies
  https://sourceforge.net/projects/megaglest/files/win32_deps.7z/download

which includes:

* Xerces-C
  http://xml.apache.org/xerces-c/index.html

* wxWidgets
  http://www.wxwidgets.org/downloads/

2.2 Building

To build and install the game use the following steps:

1. Download the win32_deps.7z mentioned above and decompress its contents
into the 'source' folder (where you see glest_game, g3d_viewer, etc).
This should create a folder called win32_deps with many sub files/folders 
in it.

2. Open the VC++ 2008 Solution file in trunk\mk\windoze called Glest.sln within
   the Visual C++ IDE.

3. Right Click on the top level Glest node in Solution Explorer and select
   Rebuild All.

If you had no errors all binaries will be created in trunk\data\glest_game.
You should be to to just run glest_game.exe

2.3 Installation

Unfortunately there is no real installation mechanism present yet, just run the
glest_game.exe from within the trunk\data\glest_game directory.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

4. Contact

Main Glest website from http://www.glest.org

Titus Tscharntke (info@titusgames.de) 
Mark Vejvoda (www.soft-haus.com - mark_vejvoda@hotmail.com )

