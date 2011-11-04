
                                  MEGAGLEST

                     by Titus Tscharntke and Mark Vejvoda

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                              MegaGlest README
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

                                  Contents:

1. System requirements
2. Installation
3. Configuration
4. Controls
5. Network play
6. Command line options
7. Support and further information


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


1. System requirements

Hardware requirements:
* 6th generation x86 CPU with 1.5 GHz or better
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

The MegaGlest Team currently provides builds and Installers for Linux and
Windows. The Linux version is available in 32 and 64 bit variants which have
been reported to run on Ubuntu 8.04 to 11.10, various versions of Debian,
OpenSuSE 11.1 to 11.3, and many other distributions. The Windows version runs
on 32 and 64 bit variants of Windows and has been reported to run on Windows
versions up to and including Windows 7.

There are also packages available for several Linux and BSD distributions and 
OS X, maintained by these distributions or the wider MegaGlest community. 
Please see the website, forums and wiki for details.

Graphics hardware and drivers:
Current dedicated Nvidia and ATI hardware with up to date vendor-supplied 
proprietary drivers installed usually provide the best experience. However, it
is not unheard of that they may be difficult to get working on Linux and other
open platforms (due to insufficient support by hardware vendors). If you 
experience such issues, open source 3D drivers are now becoming usable for both
many current and legacy GPUs, and may help with the proprietary drivers'
shortcomings.

Next to standard dedicated gaming video cards, MegaGlest also runs fine on most
integrated Intel GMA, Nvidia and ATI GPUs, but you will feel an urge to reduce
visual effects.

To run MegaGlest in headless mode, neither video nor audio hardware and driver
support are neccessary. For a good user experience, please ensure your system
meets the other minimum hardware requirements, as listed above.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

2. Installation

Linux:
Note that due to the various Linux window manager variants we can only provide
generic installation instructions here. Please check the FAQ and read up on the
forums for more verbose instructions.

After downloading the installer package, you need to make it executable. Use
your file manager/browser to browse to the directory containing the downloaded
file. Right-click the file, select the option to inspect and change this files'
properties/permissions, and make the file executable by its owner. Now double-
click the file to execute it. You need not and should not run the installer as
the administrative user (root) or via sudo, but as a normal (restricted) user.

The graphical installer will show up and display the MegaGlest license. Install
the game into a directory below your home directory (the default location of
~/.megaglest is fine) or any location of your choice (within the boundaries of
where your Linux user may write to). Once the installer completes, a MegaGlest
starter/shortcut will show up on your window managers' application menu.

Windows:
All you should need to do on Windows is to double-click the downloaded file.
You may get to see a warning saying that this file you downloaded from the
Internet may be unsafe. However, if you downloaded this file from a trusted
source, i.e. megaglest.org or *.mirror.sourceforge.net, it is generally safe 
to proceed. However, we can not make any guarantees. You are encouraged to 
scan the downloaded installer for viruses.

Other platforms:
Please refer to the packagers' installation guides.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

3. Configuration

MegaGlest can be configured in three ways:
- in-game config menu: most options
- using the separate MegaGlest Configurator
- manual editing of glestuser.ini (options) and glestuserkeys.ini (hotkeys)

DO NOT directly edit glest.ini and glestkeys.ini but rather edit glestuser.ini
and glestuserkeys.ini and create your user over-ride values in these files.

On Linux, these files are located in ~/.megaglest/. On Windows, these files are
stored at %AppData%\megaglest (Windows XP) or %AppData%\roaming\megaglest (any
later version). '%AppData%' is an environment variable which may point to a 
different location depending on your login name and Windows version. The 
MegaGlest installer places a shortcut to this directory in your start menu.
Alternatively, you may access this location by pasting the above location
(keeping the '%AppData%' variable as is) into Windows Explorers' location bar.

The entire list of glestuser.ini options is available at
http://wiki.megaglest.org/INI


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

4. Controls

Most (if not all) keyboard controls are defined in glestkeys.ini and should be
changed ONLY in the user defined file called glestuserkeys.ini (which overrides
the default keyboard values).


* Camera keyboard controls *

f                                => toggle free camera mode
w                                => move camera up (free camera mode only)
s                                => move camera down (free camera mode only)
a                                => rotate camera left (free camera mode only)
d                                => rotate camera right (free camera mode only)
g                                => center camera on selection
space                            => reset camera to default position
up arrow or mouse on top         => move camera forward
down arrow or mouse on bottom    => move camera backward
left arrow or mouse on left      => move camera left
right arrow or mouse on right    => move camera right


* Orders and Unit selection keyboard controls *

left mouse button                => select or deselect units
shift + left mouse button        => add unit to selection
control + left mouse button      => remove unit from selection
left mouse double click          => select nearby units of this type
right mouse button               => auto order
menu click                       => activate order
left mouse when order activated  => give order
right mouse when order activated => cancel state
number                           => recall group
control + number                 => assign group


* Network keyboard controls*

enter                            => start typing/send chat message
h                                => toggle between 'All' and 'Team' chat modes
n                                => show network status


* Hotkeys (game camera mode only) *

a                                => activate attack command for selection
s                                => issue stop command to selection
i                                => select next idle harvester
b                                => select next building
d                                => select next damaged unit
t                                => select next storage unit
r                                => rotate building before placement


* Other Keys *

- +                              => adjust game speed (disabled in multiplayer)
p                                => pause game (disabled in multiplayer)
e                                => save screen shot to file
c                                => toggle ingame font color (and font shadow)
m                                => show faded mesages again
?                                => when DebugMode=true, display debug info
/                                => toggle mouse pointer rendering mode (OS/MG)


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

5. Network play

To start a network game, one of the players has to create a new game, open some
network slots and the other players have to join.

The best ways to find people to play a multi-player game are:
a) using the Internet Game menu
b) scheduling a game on the Glest forums
c) finding other players on the #glest IRC channel on irc.freenode.net.

For LAN play, clients may click the 'Find LAN games' menu item to find servers
with an open network slot on the local network (this uses UDP broadcasting).

For Internet play there is an Internet Game menu item which allows clients to
find published servers. Players who wish to host Internet games (servers) may
'publish' their game to the masterserver via the 'Publish on Masterserver
option in the Custom Game menu.

If you want to host games, you should know that MegaGlest uses TCP ports
61357-61366 by default. If you are behind a router you will have to forward
these ports to the LAN IP address of your computer. These ports may be changed
via glestuser.ini.

All players have to be using the same binaries of the game and have exactly
the same map, tileset and tech tree. This means that if you have mods installed
in the tech tree that is being used for the game, every player has to have the
same mods.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

6. Command line options

megaglest                            => start game normally
megaglest --version                  => display the version string
megaglest --server                   => start in the custom game screen
                                        with all slots open
megaglest --headless-server-mode     => start a headless server, the first
                                        connecting user will manage it
megaglest --client ServerIp          => start game connecting to a server


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

7. Support and further information

Further information:

- The MegaGlest website:
  http://megaglest.org

- The MegaGlest wiki:
  http://wiki.megaglest.org

- Contact us (see below)


Support:

Please keep in mind that this is a completely volunteer run project. We do this
in our spare time and look forward to meet people who try to fix problems on
their own by reading up on available documentation before getting in touch.

- The MegaGlest FAQ:
  http://faq.megaglest.org

- Post to the forums:
  If you are reporting a bug, please be sure to read the bug reporting guide.
  http://forums.megaglest.org

- Contact us on IRC:
  Network: irc.freenode.net
  Channel: #megaglest
  Or use the webchat at http://chat.megaglest.org/
