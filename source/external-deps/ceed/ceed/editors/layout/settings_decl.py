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

from PySide import QtGui

def declare(settings):
    category = settings.createCategory(name = "layout", label = "Layout editing")

    visual = category.createSection(name = "visual", label = "Visual editing")

    # FIXME: Only applies to newly switched to visual modes!
    visual.createEntry(name = "continuous_rendering", type_ = bool, label = "Continuous rendering",
                       help_ = "Check this if you are experiencing redraw issues (your skin contains an idle animation or such).\nOnly applies to newly switched to visual modes so switch to Code mode or back or restart the application for this to take effect.",
                       defaultValue = False, widgetHint = "checkbox",
                       sortingWeight = -1)

    visual.createEntry(name = "prevent_manipulator_overlap", type_ = bool, label = "Prevent manipulator overlap",
                       help_ = "Only enable if you have a very fast computer and only edit small layouts. Very performance intensive!",
                       defaultValue = False, widgetHint = "checkbox",
                       sortingWeight = 0)

    visual.createEntry(name = "normal_outline", type_ = QtGui.QPen, label = "Normal outline",
                       help_ = "Pen for normal outline.",
                       defaultValue = QtGui.QPen(QtGui.QColor(255, 255, 0, 255)), widgetHint = "pen",
                       sortingWeight = 1)

    visual.createEntry(name = "hover_outline", type_ = QtGui.QPen, label = "Hover outline",
                       help_ = "Pen for hover outline.",
                       defaultValue = QtGui.QPen(QtGui.QColor(0, 255, 255, 255)), widgetHint = "pen",
                       sortingWeight = 2)

    visual.createEntry(name = "resizing_outline", type_ = QtGui.QPen, label = "Outline while resizing",
                       help_ = "Pen for resizing outline.",
                       defaultValue = QtGui.QPen(QtGui.QColor(255, 0, 255, 255)), widgetHint = "pen",
                       sortingWeight = 3)

    visual.createEntry(name = "moving_outline", type_ = QtGui.QPen, label = "Outline while moving",
                       help_ = "Pen for moving outline.",
                       defaultValue = QtGui.QPen(QtGui.QColor(255, 0, 255, 255)), widgetHint = "pen",
                       sortingWeight = 4)

    visual.createEntry(name = "snap_grid_x", type_ = float, label = "Snap grid cell width (X)",
                       help_ = "Snap grid X metric.",
                       defaultValue = 5, widgetHint = "float",
                       sortingWeight = 5)

    visual.createEntry(name = "snap_grid_y", type_ = float, label = "Snap grid cell height (Y)",
                       help_ = "Snap grid Y metric.",
                       defaultValue = 5, widgetHint = "float",
                       sortingWeight = 6)

    visual.createEntry(name = "snap_grid_point_colour", type_ = QtGui.QColor, label = "Snap grid point colour",
                       help_ = "Color of snap grid points.",
                       defaultValue = QtGui.QColor(255, 255, 255, 192), widgetHint = "colour",
                       sortingWeight = 7)

    visual.createEntry(name = "snap_grid_point_shadow_colour", type_ = QtGui.QColor, label = "Snap grid point shadow colour",
                       help_ = "Color of snap grid points (shadows).",
                       defaultValue = QtGui.QColor(64, 64, 64, 192), widgetHint = "colour",
                       sortingWeight = 8)

    # TODO: Full restart is not actually needed, just a refresh on all layout visual editing modes
    visual.createEntry(name = "hide_deadend_autowidgets", type_ = bool, label = "Hide deadend auto widgets",
                       help_ = "Should auto widgets with no non-auto widgets descendants be hidden in the widget hierarchy?",
                       defaultValue = True, widgetHint = "checkbox",
                       sortingWeight = 9, changeRequiresRestart = True)

    # FIXME: Only applies to newly refreshed visual modes!
    visual.createEntry(name = "auto_widgets_selectable", type_ = bool, label = "Make auto widgets selectable",
                       help_ = "Auto widgets are usually handled by LookNFeel and except in very special circumstances, you don't want to deal with them at all. Only for EXPERT use! Will make CEED crash in cases where you don't know what you are doing!",
                       defaultValue = False, widgetHint = "checkbox",
                       sortingWeight = 9)

    # FIXME: Only applies to newly refreshed visual modes!
    visual.createEntry(name = "auto_widgets_show_outline", type_ = bool, label = "Show outline of auto widgets",
                       help_ = "Auto widgets are usually handled by LookNFeel and except in very special circumstances, you don't want to deal with them at all. Only use if you know what you are doing! This might clutter the interface a lot.",
                       defaultValue = False, widgetHint = "checkbox",
                       sortingWeight = 10)
