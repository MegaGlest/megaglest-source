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

"""You only have to extend these functions to add more inputs to metaimageset.
"""

from ceed.metaimageset.inputs import bitmap, imageset, qsvg, inkscape_svg

def loadFromElement(metaImageset, element):
    for childElement in element.findall("Imageset"):
        im = imageset.Imageset(metaImageset)
        im.loadFromElement(childElement)

        metaImageset.inputs.append(im)

    for childElement in element.findall("Bitmap"):
        b = bitmap.Bitmap(metaImageset)
        b.loadFromElement(childElement)

        metaImageset.inputs.append(b)

    for childElement in element.findall("QSVG"):
        svg = qsvg.QSVG(metaImageset)
        svg.loadFromElement(childElement)

        metaImageset.inputs.append(svg)

    for childElement in element.findall("InkscapeSVG"):
        svg = inkscape_svg.InkscapeSVG(metaImageset)
        svg.loadFromElement(childElement)

        metaImageset.inputs.append(svg)

# We currently just use the input classes to save to element
#def saveElement(metaImageset, element):
#    pass
