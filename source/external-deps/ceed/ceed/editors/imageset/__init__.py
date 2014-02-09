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

import os
import sys

from ceed import settings
from ceed import editors
from ceed import xmledit
import ceed.compatibility.imageset as imageset_compatibility

from ceed.editors.imageset import visual
from ceed.editors.imageset import code

from xml.etree import cElementTree as ElementTree

"""Special words used in imageset editing code:
Imageset - definition of image rectangles on specified underlying image (~texture atlas)
Image Entry - one rectangle of the imageset
Image Offset - allows you to change the pivot point of the image entry which by default is at the
               top left corner of the image. To make the pivot point be at the centre of the image
               that is 25x25 pixels, set the offset to -12, -12
Underlying image - the image that lies under the image entries/rectangles (bitmap image)
"""

class ImagesetTabbedEditor(editors.multi.MultiModeTabbedEditor):
    """Binds all imageset editing functionality together
    """

    def __init__(self, filePath):
        super(ImagesetTabbedEditor, self).__init__(imageset_compatibility.manager, filePath)

        self.visual = visual.VisualEditing(self)
        self.addTab(self.visual, "Visual")

        self.code = code.CodeEditing(self)
        self.addTab(self.code, "Code")

        self.tabWidget = self

        # set the toolbar icon size according to the setting and subscribe to it
        self.tbIconSizeEntry = settings.getEntry("global/ui/toolbar_icon_size")
        self.updateToolbarSize(self.tbIconSizeEntry.value)
        self.tbIconSizeCallback = lambda value: self.updateToolbarSize(value)
        self.tbIconSizeEntry.subscribe(self.tbIconSizeCallback)

    def initialise(self, mainWindow):
        super(ImagesetTabbedEditor, self).initialise(mainWindow)

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
                                           "Can't parse given imageset!",
                                           "Parsing '%s' failed, it's most likely not a valid XML file. "
                                           "Constructing empty imageset instead (if you save you will override the invalid data!). "
                                           "Exception details follow:\n%s" % (self.filePath, sys.exc_info()[1]),
                                           QtGui.QMessageBox.Ok)

            # we construct the minimal empty imageset
            root = ElementTree.Element("Imageset")
            root.set("Name", "")
            root.set("Imagefile", "")

        self.visual.initialise(root)

    def finalise(self):
        # unsubscribe from the toolbar icon size setting
        self.tbIconSizeEntry.unsubscribe(self.tbIconSizeCallback)

        super(ImagesetTabbedEditor, self).finalise()

    def rebuildEditorMenu(self, editorMenu):
        editorMenu.setTitle("&Imageset")
        self.visual.rebuildEditorMenu(editorMenu)

        return True, self.currentWidget() == self.visual

    def activate(self):
        super(ImagesetTabbedEditor, self).activate()

        self.mainWindow.addToolBar(QtCore.Qt.ToolBarArea.TopToolBarArea, self.visual.toolBar)
        self.visual.toolBar.show()

        self.mainWindow.addDockWidget(QtCore.Qt.LeftDockWidgetArea, self.visual.dockWidget)
        self.visual.dockWidget.setVisible(True)

    def updateToolbarSize(self, size):
        if size < 16:
            size = 16
        self.visual.toolBar.setIconSize(QtCore.QSize(size, size))

    def deactivate(self):
        self.mainWindow.removeDockWidget(self.visual.dockWidget)
        self.mainWindow.removeToolBar(self.visual.toolBar)

        super(ImagesetTabbedEditor, self).deactivate()

    def saveAs(self, targetPath, updateCurrentPath = True):
        codeMode = self.currentWidget() is self.code

        # if user saved in code mode, we process the code by propagating it to visual
        # (allowing the change propagation to do the code validating and other work for us)

        if codeMode:
            self.code.propagateToVisual()

        rootElement = self.visual.imagesetEntry.saveToElement()
        # we indent to make the resulting files as readable as possible
        xmledit.indent(rootElement)

        self.nativeData = ElementTree.tostring(rootElement, "utf-8")

        return super(ImagesetTabbedEditor, self).saveAs(targetPath, updateCurrentPath)

    def performCut(self):
        if self.currentWidget() is self.visual:
            return self.visual.performCut()

        return False

    def performCopy(self):
        if self.currentWidget() is self.visual:
            return self.visual.performCopy()

        return False

    def performPaste(self):
        if self.currentWidget() is self.visual:
            return self.visual.performPaste()

        return False

    def performDelete(self):
        if self.currentWidget() is self.visual:
            return self.visual.performDelete()

        return False

    def zoomIn(self):
        if self.currentWidget() is self.visual:
            self.visual.zoomIn()

    def zoomOut(self):
        if self.currentWidget() is self.visual:
            self.visual.zoomOut()

    def zoomReset(self):
        if self.currentWidget() is self.visual:
            self.visual.zoomOriginal()

class ImagesetTabbedEditorFactory(editors.TabbedEditorFactory):
    def getFileExtensions(self):
        extensions = imageset_compatibility.manager.getAllPossibleExtensions()
        return extensions

    def canEditFile(self, filePath):
        extensions = self.getFileExtensions()

        for extension in extensions:
            if filePath.endswith("." + extension):
                return True

        return False

    def create(self, filePath):
        return ImagesetTabbedEditor(filePath)
