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

import ceed.ui.filesystembrowser

class FileSystemBrowser(QtGui.QDockWidget):
    """This class represents the file system browser dock widget, usually located right bottom
    in the main window. It can browse your entire filesystem and if you double click a file
    it will open an editor tab for it.
    """

    fileOpenRequested = QtCore.Signal(str)

    def __init__(self):
        super(FileSystemBrowser, self).__init__()

        self.ui = ceed.ui.filesystembrowser.Ui_FileSystemBrowser()
        self.ui.setupUi(self)

        self.view = self.findChild(QtGui.QListView, "view")
        self.model = QtGui.QFileSystemModel()
        # causes way too many problems
        #self.model.setReadOnly(False)
        self.view.setModel(self.model)

        self.view.doubleClicked.connect(self.slot_itemDoubleClicked)

        self.parentDirectoryButton = self.findChild(QtGui.QToolButton, "parentDirectoryButton")
        self.parentDirectoryButton.pressed.connect(self.slot_parentDirectoryButton)
        self.homeDirectoryButton = self.findChild(QtGui.QToolButton, "homeDirectoryButton")
        self.homeDirectoryButton.pressed.connect(self.slot_homeDirectoryButton)
        self.projectDirectoryButton = self.findChild(QtGui.QToolButton, "projectDirectoryButton")
        self.projectDirectoryButton.pressed.connect(self.slot_projectDirectoryButton)
        self.activeFileDirectoryButton = self.findChild(QtGui.QToolButton, "activeFileDirectoryButton")
        self.activeFileDirectoryButton.pressed.connect(self.slot_activeFileDirectoryButton)

        self.pathBox = self.findChild(QtGui.QComboBox, "pathBox")
        self.pathBox.currentIndexChanged.connect(self.slot_pathBoxIndexChanged)

        self.directory = ""

        # Set to project directory if project open, otherwise to user's home
        if ceed.mainwindow.MainWindow.instance.project is not None:
            self.setDirectory(ceed.mainwindow.MainWindow.instance.project.getAbsolutePathOf(""))
        else:
            self.setDirectory(os.path.expanduser("~"))

    def setDirectory(self, directory):
        """Sets the browser to view given directory"""

        directory = os.path.abspath(directory)
        if not os.path.isdir(directory):
            return

        self.model.setRootPath(directory)
        self.view.setRootIndex(self.model.index(directory))

        # Add the path to pathBox and select it
        #
        # If the path already exists in the pathBox, remove it and
        # add it to the top.
        # Path comparisons are done case-sensitive because there's
        # no true way to tell if the path is on a case-sensitive
        # file system or not, apart from creating a (temp) file
        # on that file system (and this can't be done once at start-up
        # because the user may have and use multiple file systems).
        existingIndex = self.pathBox.findText(directory)
        self.pathBox.blockSignals(True)
        if existingIndex != -1:
            self.pathBox.removeItem(existingIndex)
        self.pathBox.insertItem(0, directory)
        self.pathBox.setCurrentIndex(0)
        self.pathBox.blockSignals(False)

        self.directory = directory

    def slot_itemDoubleClicked(self, modelIndex):
        """Slot that gets triggered whenever user double clicks anything
        in the filesystem view
        """

        childPath = modelIndex.data()
        absolutePath = os.path.normpath(os.path.join(self.directory, childPath))

        if (os.path.isdir(absolutePath)):
            self.setDirectory(absolutePath)
        else:
            self.fileOpenRequested.emit(absolutePath)

    def slot_parentDirectoryButton(self):
        """Slot that gets triggered whenever the "Parent Directory" button gets pressed"""
        self.setDirectory(os.path.dirname(self.directory))

    def slot_homeDirectoryButton(self):
        self.setDirectory(os.path.expanduser("~"))

    def slot_projectDirectoryButton(self):
        if ceed.mainwindow.MainWindow.instance.project is not None:
            self.setDirectory(ceed.mainwindow.MainWindow.instance.project.getAbsolutePathOf(""))

    def slot_activeFileDirectoryButton(self):
        if ceed.mainwindow.MainWindow.instance.activeEditor is not None:
            filePath = ceed.mainwindow.MainWindow.instance.activeEditor.filePath
            dirPath = os.path.dirname(filePath)
            self.setDirectory(dirPath)
            # select the active file
            modelIndex = self.model.index(filePath)
            if modelIndex and modelIndex.isValid():
                self.view.setCurrentIndex(modelIndex)

    def slot_pathBoxIndexChanged(self, index):
        """Slot that gets triggered whenever the user selects an path from the list
        or enters a new path and hits enter"""
        if index != -1:
            # Normally this should be a simple:
            #   self.setDirectory(self.pathBox.currentText())
            # However, when the user edits the text and hits enter, their text
            # is automatically appended to the list of items and this signal
            # is fired. This is fine except that the text may not be a valid
            # directory (typo) and then the pathBox becomes poluted with junk
            # entries.
            # To solve all this, we get the new text, remove the item and then
            # call set directory which will validate and then add the path
            # to the list.
            # The alternative would be to prevent the editted text from being
            # automatically inserted (InsertPolicy(QComboBox::NoInsert)) but
            # then we need custom keyPress handling to detect the enter key
            # press etc (editTextChanged is fired on every keyPress!).
            newPath = self.pathBox.currentText()
            self.pathBox.blockSignals(True)
            self.pathBox.removeItem(index)
            self.pathBox.blockSignals(False)
            self.setDirectory(newPath)

import ceed.mainwindow
