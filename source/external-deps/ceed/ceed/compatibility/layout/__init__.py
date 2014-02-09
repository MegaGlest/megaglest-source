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

from ceed import compatibility
from ceed.compatibility.layout import cegui

class Manager(compatibility.Manager):
    """Manager of layout compatibility layers"""

    def __init__(self):
        super(Manager, self).__init__()

        self.CEGUIVersionTypes = {
            "0.5" : cegui.CEGUILayout2,
            "0.6" : cegui.CEGUILayout2,
            "0.7" : cegui.CEGUILayout3,
            "1.0" : cegui.CEGUILayout4
        }

        self.EditorNativeType = cegui.CEGUILayout4

        self.detectors.append(cegui.Layout2TypeDetector())
        self.detectors.append(cegui.Layout3TypeDetector())
        self.detectors.append(cegui.Layout4TypeDetector())

        self.layers.append(cegui.Layout3To4Layer())
        self.layers.append(cegui.Layout4To3Layer())

manager = Manager()
