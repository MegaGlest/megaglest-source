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

from ceed.compatibility.imageset import cegui
from ceed.compatibility.imageset import gorilla

class Manager(compatibility.Manager):
    """Manager of imageset compatibility layers"""

    def __init__(self):
        super(Manager, self).__init__()

        self.EditorNativeType = cegui.CEGUIImageset2
        self.CEGUIVersionTypes = {
            "0.6" : cegui.CEGUIImageset1,
            "0.7" : cegui.CEGUIImageset1,
            "1.0" : cegui.CEGUIImageset2
        }

        self.detectors.append(cegui.Imageset1TypeDetector())
        self.detectors.append(cegui.Imageset2TypeDetector())
        self.layers.append(cegui.CEGUI1ToCEGUI2Layer())
        self.layers.append(cegui.CEGUI2ToCEGUI1Layer())

        self.detectors.append(gorilla.GorillaTypeDetector())
        self.layers.append(gorilla.GorillaToCEGUI1Layer())
        self.layers.append(gorilla.CEGUI1ToGorillaLayer())

manager = Manager()
