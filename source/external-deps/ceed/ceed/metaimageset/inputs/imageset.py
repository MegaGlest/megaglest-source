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

"""Implements CEGUI imagesets as inputs of metaimageset.
"""

from ceed.metaimageset import inputs

import ceed.editors.imageset.elements as imageset_elements
import ceed.compatibility.imageset as imageset_compatibility

import os.path
from xml.etree import cElementTree as ElementTree

from PySide import QtGui

class Imageset(inputs.Input):
    class FakeImagesetEntry(imageset_elements.ImagesetEntry):
        class FakeVisual(object):
            def refreshSceneRect(self):
                pass

        def __init__(self, filePath):
            super(Imageset.FakeImagesetEntry, self).__init__(Imageset.FakeImagesetEntry.FakeVisual())

            self.filePath = filePath

        def getAbsoluteImageFile(self):
            return os.path.join(os.path.dirname(self.filePath), self.imageFile)

    def __init__(self, metaImageset):
        super(Imageset, self).__init__(metaImageset)

        self.filePath = ""
        self.imagesetEntry = None

    def loadFromElement(self, element):
        self.filePath = os.path.join(os.path.dirname(self.metaImageset.filePath), element.get("path", ""))

        rawData = open(self.filePath, "r").read()
        nativeData = imageset_compatibility.manager.transformTo(imageset_compatibility.manager.EditorNativeType, rawData, self.filePath)

        element = ElementTree.fromstring(nativeData)

        self.imagesetEntry = Imageset.FakeImagesetEntry(self.filePath)
        self.imagesetEntry.loadFromElement(element)

    def saveToElement(self):
        ret = ElementTree.Element("Imageset")
        ret.set("path", self.filePath)

        return ret

    def getDescription(self):
        return "Imageset '%s'" % (self.filePath)

    def buildImages(self):
        assert(self.imagesetEntry is not None)

        ret = []

        entireImage = QtGui.QImage(self.imagesetEntry.getAbsoluteImageFile())

        for imageEntry in self.imagesetEntry.imageEntries:
            subImage = entireImage.copy(imageEntry.xpos, imageEntry.ypos, imageEntry.width, imageEntry.height)

            ret.append(inputs.Image(self.imagesetEntry.name + "/" + imageEntry.name, subImage, imageEntry.xoffset, imageEntry.yoffset))

        return ret
