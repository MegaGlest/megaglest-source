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

from PySide.QtGui import QUndoCommand, QDockWidget, QUndoView, QIcon, QWidget, QVBoxLayout

class UndoCommand(QUndoCommand):
    """The base class of all undo commands of CEED.
    Internal: Currently serves no special purpose but might serve on in the future!
    """

    def __init__(self):
        super(UndoCommand, self).__init__()

class UndoViewer(QDockWidget):
    """A dockwidget able to view the entire undo history of given undo stack.
    """

    def __init__(self):
        super(UndoViewer, self).__init__()

        self.setObjectName("Undo Viewer dock widget")
        self.setWindowTitle("Undo Viewer")

        # main undo view
        self.view = QUndoView()
        self.view.setCleanIcon(QIcon("icons/clean_undo_state.png"))
        # root widget and layout
        contentsWidget = QWidget()
        contentsLayout = QVBoxLayout()
        contentsWidget.setLayout(contentsLayout)
        margins = contentsLayout.contentsMargins()
        margins.setTop(0)
        contentsLayout.setContentsMargins(margins)

        contentsLayout.addWidget(self.view)
        self.setWidget(contentsWidget)

    def setUndoStack(self, stack):
        self.view.setStack(stack)
        # if stack is None this effectively disables the entire dock widget to improve UX
        self.setEnabled(stack is not None)
