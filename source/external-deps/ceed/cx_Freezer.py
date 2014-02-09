##############################################################################
#   CEED - Unified CEGUI asset editor
#
#   Copyright (C) 2011-2012   Martin Preisler <martin@preisler.me>
#                             and contributing authors (see AUTHORS file)
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
##############################################################################

"""Allows freezing the executable

Freezed bundles are most commonly used on Windows but efforts are underway
to make this work on MacOSX as well. It does work on Linux but is kind of
pointless in my opinion.
"""

import platform

from ceed import version

if version.DEVELOPER_MODE:
    raise RuntimeError("I politely refuse to freeze CEED in developer mode! "
                       "I will only freeze end-user versions.")

from ceed import compileuifiles
# make sure .ui files are freshly compiled before freezing
compileuifiles.main()

VERSION = version.CEED
# TODO: these should branch depending on the platform
GUI_BASE_APP = "Console"
CONSOLE_BASE_APP = "Console"
EXECUTABLE_EXTENSION = ""

if platform.system() == "Windows":
    # Windows is being special again
    GUI_BASE_APP = "Win32GUI"
    EXECUTABLE_EXTENSION = ".exe"

from cx_Freeze import setup, Executable

buildOptions = dict(
    packages =
    [
        "OpenGL",
        "OpenGL.GL",
        "OpenGL.GLU",
        "OpenGL.platform",
        "OpenGL.arrays.formathandler",

        "encodings.ascii",
        "encodings.utf_8",

        "PySide.QtNetwork",

        "PyCEGUI",
        "PyCEGUIOpenGLRenderer"
    ],

    include_files =
    [
        ["data", "data"]
    ]
)

setup(
    name = "CEED",
    version = VERSION,
    description = "CEGUI Unified Editor",
    options = dict(build_exe = buildOptions),
    executables = [
        # this starts the GUI editor main application
        Executable(
            "bin/ceed-gui",
            base = GUI_BASE_APP,
            targetName = "ceed-gui" + EXECUTABLE_EXTENSION,
            icon = "data/icons/application_icon.ico"
        ),

        # this starts the MetaImageset compiler
        Executable(
            "bin/ceed-mic",
            base = CONSOLE_BASE_APP,
            targetName = "ceed-mic" + EXECUTABLE_EXTENSION
        ),
        # this starts the Asset Migration tool
        Executable(
            "bin/ceed-migrate",
            base = CONSOLE_BASE_APP,
            targetName = "ceed-migrate" + EXECUTABLE_EXTENSION
        )
    ]
)
