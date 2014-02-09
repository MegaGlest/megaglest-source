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
#from xml.etree import cElementTree as ElementTree

AnimationList1 = "CEGUI Animation List 1"

class AnimationList1TypeDetector(compatibility.TypeDetector):
    def getType(self):
        return AnimationList1

    def getPossibleExtensions(self):
        return set(["anims"])

    def matches(self, data, extension):
        if extension not in ["", "anims"]:
            return False

        # TODO
        return True

class Manager(compatibility.Manager):
    """Manager of CEGUI animation list compatibility layers"""

    def __init__(self):
        super(Manager, self).__init__()

        self.EditorNativeType = AnimationList1
        self.CEGUIVersionTypes = {
            "0.6" : None,
            "0.7" : AnimationList1,
            "1.0" : AnimationList1
        }

        self.detectors.append(AnimationList1TypeDetector())

manager = Manager()
