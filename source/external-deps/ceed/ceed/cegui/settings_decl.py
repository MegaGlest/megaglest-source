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

from PySide import QtCore
from PySide import QtGui

def declare(settings):
    category = settings.createCategory(name = "cegui", label = "Embedded CEGUI")

    background = category.createSection(name = "background", label = "Rendering background (checkerboard)")

    background.createEntry(name = "checker_width", type_ = int, label = "Width of the checkers",
                           help_ = "Width of one of the checkers.",
                           defaultValue = 10, widgetHint = "int",
                           sortingWeight = 1)

    background.createEntry(name = "checker_height", type_ = int, label = "Height of the checkers",
                           help_ = "Height of one of the checkers.",
                           defaultValue = 10, widgetHint = "int",
                           sortingWeight = 2)

    background.createEntry(name = "first_colour", type_ = QtGui.QColor, label = "First colour",
                           help_ = "First of the alternating colours to use.",
                           defaultValue = QtGui.QColor(QtCore.Qt.darkGray), widgetHint = "colour",
                           sortingWeight = 3)

    background.createEntry(name = "second_colour", type_ = QtGui.QColor, label = "Second colour",
                           help_ = "Second of the alternating colours to use. (use the same as first to get a solid background)",
                           defaultValue = QtGui.QColor(QtCore.Qt.lightGray), widgetHint = "colour",
                           sortingWeight = 4)
