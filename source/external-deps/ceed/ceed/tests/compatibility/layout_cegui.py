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

import unittest

from ceed.compatibility.layout import cegui
from xml.etree import cElementTree as ElementTree

import os

class test_Layout3To4Layer(unittest.TestCase):
    def setUp(self):
        self.layer = cegui.Layout3To4Layer()

    def test_transformAttribute(self):
        element = ElementTree.Element("Window")
        element.set("Name", "name_test")

        self.layer.transformAttribute(element, "Name")
        self.assertEqual(element.get("name"), "name_test")
        self.assertEqual(element.get("Name"), None)

    def test_convertToRelativeNames(self):
        element = ElementTree.Element("Window")
        element.set("Name", "Root")

        childElement = ElementTree.Element("Window")
        childElement.set("Name", "Root/Child")

        element.append(childElement)

        self.layer.convertToRelativeNames(element)

        self.assertEqual(element.get("Name"), "Root")
        self.assertEqual(childElement.get("Name"), "Child")

class test_Layout3and4Layers(unittest.TestCase):
    def setUp(self):
        self.maxDiff = None

        self.layer3to4 = cegui.Layout3To4Layer()
        self.layer4to3 = cegui.Layout4To3Layer()

    def _test_conversion(self, layout):
        previousCwd = os.getcwdu()

        try:
            os.chdir(os.path.dirname(__file__))

            layout3 = file("layout_data/%s_0_7.layout" % (layout), "r").read()
            layout4 = file("layout_data/%s_1_0.layout" % (layout), "r").read()

            self.assertMultiLineEqual(layout4, self.layer3to4.transform(layout3))
            self.assertMultiLineEqual(layout3, self.layer4to3.transform(layout4))

        finally:
            os.chdir(previousCwd)

    def test_tabPage1(self):
        self._test_conversion("TabPage1")

    def test_textDemo(self):
        self._test_conversion("TextDemo")

    def test_vanillaWindows(self):
        self._test_conversion("VanillaWindows")
