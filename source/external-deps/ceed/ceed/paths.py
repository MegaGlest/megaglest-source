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

"""This module contains means to get various CEED related paths in various
environments.
"""

import os
from ceed import version

# Whether the application is frozen using cx_Freeze
FROZEN = False

from ceed import fake
# What's the absolute path to the package directory
PACKAGE_DIR = os.path.dirname(os.path.abspath(fake.__file__))

if PACKAGE_DIR.endswith(os.path.join("library.zip", "ceed")):
    FROZEN = True
    PACKAGE_DIR = os.path.dirname(PACKAGE_DIR)

# What's the absolute path to the data directory
DATA_DIR = ""

# Potential system data dir, we check it's existence and set
# DATA_DIR as system_data_dir if it exists
SYSTEM_DATA_DIR = "/usr/share/ceed"
SYSTEM_DATA_DIR_EXISTS = False
try:
    if os.path.exists(SYSTEM_DATA_DIR):
        DATA_DIR = SYSTEM_DATA_DIR
        SYSTEM_DATA_DIR_EXISTS = True
except:
    pass

if not SYSTEM_DATA_DIR_EXISTS:
    DATA_DIR = os.path.join(os.path.dirname(PACKAGE_DIR), "data")

# What's the absolute path to the doc directory
DOC_DIR = ""

# Potential system doc dir, we check it's existence and set
# DOC_DIR as system_data_dir if it exists
SYSTEM_DOC_DIR = "/usr/share/doc/ceed-%s" % (version.CEED)
SYSTEM_DOC_DIR_EXISTS = False
try:
    if os.path.exists(SYSTEM_DOC_DIR):
        DOC_DIR = SYSTEM_DOC_DIR
        SYSTEM_DOC_DIR_EXISTS = True
except:
    pass

if not SYSTEM_DOC_DIR_EXISTS:
    DOC_DIR = os.path.join(os.path.dirname(PACKAGE_DIR), "doc")

# What's the absolute path to the ui directory
UI_DIR = os.path.join(PACKAGE_DIR, "ui")

# if one of these assertions fail your installation is not valid!
if not FROZEN:
    # these two checks will always fail in a frozen instance
    assert(os.path.exists(PACKAGE_DIR))
    assert(os.path.exists(UI_DIR))

assert(os.path.exists(DATA_DIR))
