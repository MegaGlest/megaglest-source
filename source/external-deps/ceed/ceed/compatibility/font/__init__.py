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
from ceed.compatibility.font import cegui

class Manager(compatibility.Manager):
    """Manager of font compatibility layers"""

    def __init__(self):
        super(Manager, self).__init__()

        self.EditorNativeType = cegui.CEGUIFont3
        self.CEGUIVersionTypes = {
            "0.4" : cegui.CEGUIFont1,
            "0.5" : cegui.CEGUIFont2, # font rewrite
            "0.6" : cegui.CEGUIFont2,
            "0.7" : cegui.CEGUIFont2,
            "1.0" : cegui.CEGUIFont3
        }

        self.detectors.append(cegui.Font2TypeDetector())
        self.detectors.append(cegui.Font3TypeDetector())

        self.layers.append(cegui.Font2ToFont3Layer())
        self.layers.append(cegui.Font3ToFont2Layer())

manager = Manager()
