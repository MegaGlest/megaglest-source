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

def declare(actionManager):
    cat = actionManager.createCategory(name = "imageset", label = "Imageset Editor")

    cat.createAction(name = "edit_offsets", label = "Edit &Offsets",
                     help_ = "When you select an image definition, a crosshair will appear in it representing it's offset centrepoint.",
                     icon = QtGui.QIcon("icons/imageset_editing/edit_offsets.png"),
                     defaultShortcut = QtGui.QKeySequence(QtCore.Qt.Key_Space)).setCheckable(True)

    cat.createAction(name = "cycle_overlapping", label = "Cycle O&verlapping Image Definitions",
                     help_ = "When images definition overlap in such a way that makes it hard/impossible to select the definition you want, this allows you to select on of them and then just cycle until the right one is selected.",
                     icon = QtGui.QIcon("icons/imageset_editing/cycle_overlapping.png"),
                     defaultShortcut = QtGui.QKeySequence(QtCore.Qt.Key_Q))

    cat.createAction(name = "create_image", label = "&Create Image Definition",
                     help_ = "Creates a new image definition at the current cursor position, sized 50x50 pixels.",
                     icon = QtGui.QIcon("icons/imageset_editing/create_image.png"))

    cat.createAction(name = "duplicate_image", label = "&Duplicate Image Definition",
                     help_ = "Duplicates selected image definitions.",
                     icon = QtGui.QIcon("icons/imageset_editing/duplicate_image.png"))

    cat.createAction(name = "focus_image_list_filter_box", label = "&Focus Image Definition List Filter Box",
                     help_ = "This allows you to easily press a shortcut and immediately search through image definitions without having to reach for a mouse.",
                     icon = QtGui.QIcon("icons/imageset_editing/focus_image_list_filter_box.png"),
                     defaultShortcut = QtGui.QKeySequence(QtGui.QKeySequence.Find))
