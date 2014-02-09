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

"""This module provides all metaimageset API core functionality (except editing)
"""

import ceed.compatibility.imageset as imageset_compatibility
from xml.etree import cElementTree as ElementTree
import os.path

from ceed.metaimageset.inputs import registry as input_registry

class MetaImageset(object):
    def __init__(self, filePath):
        self.filePath = filePath

        self.name = ""
        self.nativeHorzRes = 800
        self.nativeVertRes = 600
        self.autoScaled = False

        self.onlyPOT = False

        self.output = ""
        self.outputTargetType = imageset_compatibility.manager.EditorNativeType
        self.inputs = []

    def getOutputDirectory(self):
        return os.path.abspath(os.path.dirname(self.filePath))

    def loadFromElement(self, element):
        self.name = element.get("name", "")
        self.nativeHorzRes = int(element.get("nativeHorzRes", "800"))
        self.nativeVertRes = int(element.get("nativeVertRes", "600"))
        self.autoScaled = element.get("autoScaled", "false")

        self.outputTargetType = element.get("outputTargetType", imageset_compatibility.manager.EditorNativeType)
        self.output = element.get("output", "")

        input_registry.loadFromElement(self, element)

    def saveToElement(self):
        ret = ElementTree.Element("MetaImageset")
        ret.set("name", self.name)
        ret.set("nativeHorzRes", str(self.nativeHorzRes))
        ret.set("nativeVertRes", str(self.nativeVertRes))
        ret.set("autoScaled", self.autoScaled)

        ret.set("outputTargetType", self.outputTargetType)
        ret.set("output", self.output)

        # FIXME: Should we be using registry for this too?
        for input_ in self.inputs:
            element = input_.saveToElement()
            ret.append(element)

        return ret
