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

"""Implements simple bitmaps as inputs of metaimageset.
"""

from ceed.metaimageset import inputs

from PySide import QtGui

import os.path
import glob
from xml.etree import cElementTree as ElementTree

class Bitmap(inputs.Input):
    def __init__(self, metaImageset):
        super(Bitmap, self).__init__(metaImageset)

        self.path = ""

        self.xOffset = 0
        self.yOffset = 0

    def loadFromElement(self, element):
        self.path = element.get("path", "")
        self.xOffset = int(element.get("xOffset", "0"))
        self.yOffset = int(element.get("yOffset", "0"))

    def saveToElement(self):
        ret = ElementTree.Element("Bitmap")
        ret.set("path", self.path)
        ret.set("xOffset", str(self.xOffset))
        ret.set("yOffset", str(self.yOffset))

        return ret

    def getDescription(self):
        return "Bitmap image(s) '%s'" % (self.path)

    def buildImages(self):
        paths = glob.glob(os.path.join(os.path.dirname(self.metaImageset.filePath), self.path))

        images = []
        for path in paths:
            pathSplit = path.rsplit(".", 1)
            name = os.path.basename(pathSplit[0])

            image = inputs.Image(name, QtGui.QImage(path), self.xOffset, self.yOffset)
            images.append(image)

        return images
