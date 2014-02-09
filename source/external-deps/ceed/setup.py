#!/usr/bin/env python2

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

import os
import os.path
import platform

from distutils.core import setup
from ceed import version
from ceed import paths

from ceed import compileuifiles

if platform.system() == "Windows":
    raise NotImplementedError("Installing CEED like this is not supported on Windows")

# always compile UI files to ensure they are up to date before installing
compileuifiles.main()

def get_packages():
    """Returns the whole list of ceed packages"""

    # Distutils requires us to list all packages, this is very tedious and prone
    # to errors. While I believe I should know which packages ceed has at all times
    # I also believe that saving work on my part is a "good thing".

    ret = ["ceed"]

    for dirpath, _, files in os.walk("ceed"):
        if "__init__.py" in files:
            ret.append(dirpath.replace(os.path.sep, "."))

    return ret

def get_directoryfilepairs(directory, base = "data", install_base = paths.SYSTEM_DATA_DIR):
    ret = []
    files = []
    for path in os.listdir(os.path.join(base, directory)):
        full_path = os.path.join(base, directory, path)
        if os.path.isdir(full_path):
            ret.extend(get_directoryfilepairs(os.path.join(directory, path), base))

        elif os.path.isfile(full_path):
            files.append(os.path.join(base, directory, path))

        else:
            print("[W] I don't know what '%s' is (checked for file or directory)" % (full_path))

    ret.append((os.path.join(install_base, directory), files))

    return ret

setup(
    name = "CEED",
    version = version.CEED,
    description = "The CEGUI Unified Editor",
    author = "Martin Preisler and others (see AUTHORS)",
    author_email = "martin@preisler.me",
    url = "http://www.cegui.org.uk/",
    packages = get_packages(),
    scripts = ["bin/ceed-gui", "bin/ceed-mic", "bin/ceed-migrate"],
    data_files =
        get_directoryfilepairs("", "data", paths.SYSTEM_DATA_DIR) +
        [(paths.SYSTEM_DOC_DIR, [
            "doc/quickstart-guide.pdf",
            "doc/user-manual.pdf",
            "doc/developer-manual.pdf"
        ])]
)
