
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
* >= (2011) Intel Sandy Bridge x86 CPU with 1.6 GHz or higher frequency
  (modern multi-core CPU series, at least 2.0 GHz recommended)
* 2.0 GB RAM
  (4.0 GB RAM recommended)
* Graphics chip supporting OpenGL 2.1 or higher
  (dedicated/discrete video card with hardware 3D acceleration recommended)
* 512 MB video memory
* Audio chip supporting OpenAL

Software requirements:
* A supported (by its producer) operating system version
* Graphics drivers which work well with this operating system version and
  support the OpenGL requirements discussed above
* Audio drivers supporting OpenAL
* A file archiving utility which provides a command line interface and can
  decompress 7-zip archives

The MegaGlest Team currently provides builds and installers for Linux and
Windows. The Linux version is available in 32 and 64 bit variants which have
been reported to run on any supported Ubuntu LTS releases, various versions of
Debian, OpenSuSE 11.1 to 13.1, and many other distributions. The Windows
version runs on 32 and 64 bit variants and has been reported to run on Windows
versions up to and including Windows 10.

There are also packages available for several Linux and BSD distributions and
OS X, maintained by these distributions or the wider MegaGlest community.
Please see the MegaGlest website, forums and wiki, as well as the package
directories these distributions provide for details.

Graphics hardware and drivers:
Current dedicated Nvidia and AMD hardware with up to date vendor-supplied
proprietary drivers installed usually provide the best experience. However, it
is not unheard of that they may be difficult to get working on Linux and other
open platforms (due to insufficient support by hardware vendors). If you
experience such issues, open source 3D drivers are now becoming usable for both
many current and legacy GPUs, and may help with the proprietary drivers'
shortcomings.

Next to standard dedicated gaming video cards, MegaGlest also runs fine on most
integrated Intel, Nvidia and AMD APUs, but you will feel an urge to reduce
visual effects. See http://faq.megaglest.org for potential optimizations.

To run MegaGlest in headless mode, neither video nor audio hardware and driver
support are neccessary. For a good user experience, please ensure your system
meets all other minimum hardware requirements, as listed above.


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
~/megaglest is fine) or any location of your choice (within the boundaries of
where your Linux user may write to). Once the installer completes, a MegaGlest
starter/shortcut will show up on your window managers' application menu.

Windows:
All you should need to do on Windows is to double-click the downloaded file.
You may get to see a warning saying that this file you downloaded from the
Internet may be unsafe. However, if you downloaded this file from a trusted
source, i.e. megaglest.org (which currently forwards to GitHub, which then
forwards to Amazon AWS S3), it should be safe to proceed. However, we can not
make any guarantees. You are encouraged to scan the downloaded installer for
viruses.

Other platforms:
Please refer to the packagers' installation guides.

All platforms:
We may cryptographically sign (OpenPGP) releases and provide checksums, so
please use these to verify the integrity and authenticity of the files you
have downloaded.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

3. Configuration

MegaGlest can be configured in two ways:
- in-game config menu: most options
- manual editing of glestuser.ini (options) and glestuserkeys.ini (hotkeys)

DO NOT directly edit glest.ini and glestkeys.ini but rather edit glestuser.ini
and glestuserkeys.ini overwriting global defaults.

On Linux, these files are located in ~/.megaglest/ (note the leading dot, this
is a hidden directory).

On Windows, these files are stored at %AppData%\megaglest. '%AppData%' is an
environment variable which points to a different location depending on your
login name and Windows version. The MegaGlest installer places a shortcut to
this directory in your start menu. Alternatively, you may access this location
by pasting "%AppData%\megaglest" (omitting the quotation marks and keeping the
'%AppData%' variable unmodified) into Windows Explorers' location bar. To
activate the location bar in Windows Explorer, you need to click ony any of the
buttons indicating your current location on top of the Explorer window.

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
b) scheduling a game on the MegaGlest forums
c) finding other players on the #megaglest-lobby IRC channel on irc.libera.chat.

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
megaglest --starthost                => start in the custom game screen
                                        with open network slots
megaglest --headless-server-mode     => start a headless server, the first
                                        connecting user will manage it
megaglest --connect IPADDR[:PORT]    => start game connecting to the server
                                        at the given IPv4 address

For a complete list, run:
megaglest --help

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

7. Support and further information

Further information:

- The MegaGlest website:
  https://megaglest.org

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
  https://forum.megaglest.org

- Contact us on IRC:
  Network: irc.libera.chat
  Channel: #megaglest
  Or use the webchat at http://chat.megaglest.org/
