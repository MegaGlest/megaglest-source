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
    cat = actionManager.createCategory(name = "layout", label = "Layout Editor")

    cat.createAction(name = "align_hleft", label = "Align &Left (horizontally)",
                     help_ = "Sets horizontal alignment of all selected widgets to left.",
                     icon = QtGui.QIcon("icons/layout_editing/align_hleft.png"))
    cat.createAction(name = "align_hcentre", label = "Align Centre (&horizontally)",
                     help_ = "Sets horizontal alignment of all selected widgets to centre.",
                     icon = QtGui.QIcon("icons/layout_editing/align_hcentre.png"))
    cat.createAction(name = "align_hright", label = "Align &Right (horizontally)",
                     help_ = "Sets horizontal alignment of all selected widgets to right.",
                     icon = QtGui.QIcon("icons/layout_editing/align_hright.png"))

    cat.createAction(name = "align_vtop", label = "Align &Top (vertically)",
                     help_ = "Sets vertical alignment of all selected widgets to top.",
                     icon = QtGui.QIcon("icons/layout_editing/align_vtop.png"))
    cat.createAction(name = "align_vcentre", label = "Align Centre (&vertically)",
                     help_ = "Sets vertical alignment of all selected widgets to centre.",
                     icon = QtGui.QIcon("icons/layout_editing/align_vcentre.png"))
    cat.createAction(name = "align_vbottom", label = "Align &Bottom (vertically)",
                     help_ = "Sets vertical alignment of all selected widgets to bottom.",
                     icon = QtGui.QIcon("icons/layout_editing/align_vbottom.png"))

    cat.createAction(name = "snap_grid", label = "Snap to &Grid",
                     help_ = "When resizing and moving widgets, if checked this makes sure they snap to a snap grid (see settings for snap grid related entries), also shows the snap grid if checked.",
                     icon = QtGui.QIcon("icons/layout_editing/snap_grid.png"),
                     defaultShortcut = QtGui.QKeySequence(QtCore.Qt.Key_Space)).setCheckable(True)

    absolute_mode = cat.createAction(
                     name = "absolute_mode", label = "&Absolute Resizing && Moving Deltas",
                     help_ = "When resizing and moving widgets, if checked this makes the delta absolute, it is relative if unchecked.",
                     icon = QtGui.QIcon("icons/layout_editing/absolute_mode.png"),
                     defaultShortcut = QtGui.QKeySequence(QtCore.Qt.Key_A))
    absolute_mode.setCheckable(True)
    absolute_mode.setChecked(True)

    cat.createAction(name = "normalise_position", label = "Normalise &Position (cycle)",
                     help_ = "If the position is mixed (absolute and relative) it becomes relative only, if it's relative it becomes absolute, if it's absolute it becomes relative.",
                     icon = QtGui.QIcon("icons/layout_editing/normalise_position.png"),
                     defaultShortcut = QtGui.QKeySequence(QtCore.Qt.Key_D))

    cat.createAction(name = "normalise_size", label = "Normalise &Size (cycle)",
                     help_ = "If the size is mixed (absolute and relative) it becomes relative only, if it's relative it becomes absolute, if it's absolute it becomes relative.",
                     icon = QtGui.QIcon("icons/layout_editing/normalise_size.png"),
                     defaultShortcut = QtGui.QKeySequence(QtCore.Qt.Key_S))

    cat.createAction(name = "focus_property_inspector_filter_box", label = "&Focus Property Inspector Filter Box",
                     help_ = "This allows you to easily press a shortcut and immediately search through properties without having to reach for a mouse.",
                     icon = QtGui.QIcon("icons/layout_editing/focus_property_inspector_filter_box.png"),
                     defaultShortcut = QtGui.QKeySequence(QtGui.QKeySequence.Find))

    cat.createAction(name = "copy_widget_path", label = "C&opy Widget Paths",
                     help_ = "Copies the 'NamePath' properties of the selected widgets to the clipboard.",
                     icon = QtGui.QIcon("icons/actions/copy.png"))

    cat.createAction(name = "rename", label = "&Rename Widget",
                     help_ = "Edits the selected widget's name.",
                     icon = QtGui.QIcon("icons/layout_editing/rename.png"))

    cat.createAction(name = "lock_widget", label = "&Lock Widget",
                     help_ = "Locks the widget for moving and resizing in the visual editing mode.",
                     icon = QtGui.QIcon("icons/layout_editing/lock_widget.png"))

    cat.createAction(name = "unlock_widget", label = "&Unlock Widget",
                     help_ = "Unlocks the widget for moving and resizing in the visual editing mode.",
                     icon = QtGui.QIcon("icons/layout_editing/unlock_widget.png"))

    cat.createAction(name = "recursively_lock_widget", label = "&Lock Widget (recursively)",
                     help_ = "Locks the widget and all its child widgets for moving and resizing in the visual editing mode.",
                     icon = QtGui.QIcon("icons/layout_editing/lock_widget_recursively.png"))

    cat.createAction(name = "recursively_unlock_widget", label = "&Unlock Widget (recursively)",
                     help_ = "Unlocks the widget and all its child widgets for moving and resizing in the visual editing mode.",
                     icon = QtGui.QIcon("icons/layout_editing/unlock_widget_recursively.png"))
