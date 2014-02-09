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

from ceed.editors import multi
import PyCEGUI

class CodeEditing(multi.CodeEditMode):
    def __init__(self, tabbedEditor):
        super(CodeEditing, self).__init__()

        self.tabbedEditor = tabbedEditor

    def getNativeCode(self):
        currentRootWidget = self.tabbedEditor.visual.getCurrentRootWidget()

        if currentRootWidget is None:
            return ""

        else:
            return PyCEGUI.WindowManager.getSingleton().getLayoutAsString(currentRootWidget)

    def propagateNativeCode(self, code):
        # we have to make the context the current context to ensure textures are fine
        mainwindow.MainWindow.instance.ceguiContainerWidget.makeGLContextCurrent()

        if code == "":
            self.tabbedEditor.visual.setRootWidget(None)

        else:
            try:
                newRoot = PyCEGUI.WindowManager.getSingleton().loadLayoutFromString(code)
                self.tabbedEditor.visual.setRootWidget(newRoot)

                return True

            except:
                return False

# needs to be at the end, imported to get the singleton
from ceed import mainwindow
