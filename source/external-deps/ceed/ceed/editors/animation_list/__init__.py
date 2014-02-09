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

from ceed import editors
import ceed.compatibility.animation_list as animation_list_compatibility
from ceed.editors.animation_list import visual
from ceed.editors.animation_list import code

import os.path
import sys
from xml.etree import cElementTree as ElementTree

class AnimationListTabbedEditor(editors.multi.MultiModeTabbedEditor):
    """Animation list file editor (XML file containing list of animations)
    """

    def __init__(self, filePath):
        super(AnimationListTabbedEditor, self).__init__(animation_list_compatibility.manager, filePath)

        self.requiresProject = True

        self.visual = visual.VisualEditing(self)
        self.addTab(self.visual, "Visual")

        self.code = code.CodeEditing(self)
        self.addTab(self.code, "Code")

        self.tabWidget = self

    def initialise(self, mainWindow):
        super(AnimationListTabbedEditor, self).initialise(mainWindow)

        # We do something most people would not expect here,
        # instead of asking CEGUI to load the animation list as it is,
        # we parse it ourself, mine each and every animation from it,
        # and use these chunks of code to load every animation one at a time

        # the reason we do this is more control (CEGUI just adds the animation
        # list to the pool of existing animations, we don't want to pollute that
        # pool and we want to group all loaded animations)

        root = None
        try:
            root = ElementTree.fromstring(self.nativeData)

        except:
            # things didn't go smooth
            # 2 reasons for that
            #  * the file is empty
            #  * the contents of the file are invalid
            #
            # In the first case we will silently move along (it is probably just a new file),
            # in the latter we will output a message box informing about the situation

            # the file should exist at this point, so we are not checking and letting exceptions
            # fly out of this method
            if os.path.getsize(self.filePath) > 2:
                # the file contains more than just CR LF
                QtGui.QMessageBox.question(self,
                                           "Can't parse given animation list!",
                                           "Parsing '%s' failed, it's most likely not a valid XML file. "
                                           "Constructing empty animation list instead (if you save you will override the invalid data!). "
                                           "Exception details follow:\n%s" % (self.filePath, sys.exc_info()[1]),
                                           QtGui.QMessageBox.Ok)

            # we construct the minimal empty imageset
            root = ElementTree.Element("Animations")

        self.visual.loadFromElement(root)

    def finalise(self):
        # this takes care of destroying the temporary animation instance, if any
        self.visual.setCurrentAnimation(None)

        super(AnimationListTabbedEditor, self).finalise()

    def activate(self):
        super(AnimationListTabbedEditor, self).activate()

        self.mainWindow.addDockWidget(QtCore.Qt.LeftDockWidgetArea, self.visual.animationListDockWidget)
        self.visual.animationListDockWidget.setVisible(True)
        self.mainWindow.addDockWidget(QtCore.Qt.RightDockWidgetArea, self.visual.propertiesDockWidget)
        self.visual.propertiesDockWidget.setVisible(True)
        self.mainWindow.addDockWidget(QtCore.Qt.RightDockWidgetArea, self.visual.keyFramePropertiesDockWidget)
        self.visual.keyFramePropertiesDockWidget.setVisible(True)
        self.mainWindow.addDockWidget(QtCore.Qt.BottomDockWidgetArea, self.visual.timelineDockWidget)
        self.visual.timelineDockWidget.setVisible(True)

    def deactivate(self):
        self.mainWindow.removeDockWidget(self.visual.animationListDockWidget)
        self.mainWindow.removeDockWidget(self.visual.propertiesDockWidget)
        self.mainWindow.removeDockWidget(self.visual.keyFramePropertiesDockWidget)
        self.mainWindow.removeDockWidget(self.visual.timelineDockWidget)

        super(AnimationListTabbedEditor, self).deactivate()

    def saveAs(self, targetPath, updateCurrentPath = True):
        codeMode = self.currentWidget() is self.code

        # if user saved in code mode, we process the code by propagating it to visual
        # (allowing the change propagation to do the code validating and other work for us)

        if codeMode:
            self.code.propagateToVisual()

        self.nativeData = self.visual.generateNativeData()

        return super(AnimationListTabbedEditor, self).saveAs(targetPath, updateCurrentPath)

    def zoomIn(self):
        if self.currentWidget() is self.visual:
            return self.visual.zoomIn()

        return False

    def zoomOut(self):
        if self.currentWidget() is self.visual:
            return self.visual.zoomOut()

        return False

    def zoomReset(self):
        if self.currentWidget() is self.visual:
            return self.visual.zoomReset()

        return False

class AnimationListTabbedEditorFactory(editors.TabbedEditorFactory):
    def getFileExtensions(self):
        extensions = set(["anims"])
        return extensions

    def canEditFile(self, filePath):
        extensions = self.getFileExtensions()

        for extension in extensions:
            if filePath.endswith("." + extension):
                return True

        return False

    def create(self, filePath):
        return AnimationListTabbedEditor(filePath)
