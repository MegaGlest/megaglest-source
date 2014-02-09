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

"""Defines interfaces for metaimageset inputs and the images that are returned
from these inputs.
"""

class Image(object):
    """Instance of the image, containing a bitmap (QImage)
    and xOffset and yOffset
    """

    def __init__(self, name, qimage, xOffset = 0, yOffset = 0):
        self.name = name

        self.qimage = qimage

        # X and Y offsets are related to the "crosshair" effect for images
        # moving the origin of image around...
        # These have nothing to do with packing and the final x, y position
        # on the texture atlas.
        self.xOffset = xOffset
        self.yOffset = yOffset

class Input(object):
    """Describes any input image source for the meta imageset.

    This can be imageset, bitmap image, SVG image, ...
    """

    def __init__(self, metaImageset):
        """metaImageset - the parent MetaImageset class"""

        self.metaImageset = metaImageset

    def loadFromElement(self, element):
        raise NotImplementedError("Each Input subclass must override Input.loadFromElement!")

    def saveToElement(self):
        raise NotImplementedError("Each Input subclass must override Input.saveToElement!")

    def getDescription(self):
        raise NotImplementedError("Each Input subclass must override Input.getDescription")

    def buildImages(self):
        """Retrieves list of Image objects each containing a bitmap representation
        of some image this input provided, xOffset and yOffset.

        For simple images, this will return [ImageInstance(QImage(self.path))],
        For imagesets, this will return list of all images in the imageset
        (Each QImage containing only the specified portion of the underlying image)
        """

        raise NotImplementedError("Each Input subclass must override Input.buildImages!")
