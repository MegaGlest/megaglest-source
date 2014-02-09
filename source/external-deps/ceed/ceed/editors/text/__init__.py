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
from ceed import editors

# TODO: This could get replaced by QScintilla once PySide guys get it to work.
#       Scintilla would probably be overkill though, I can't imagine anyone
#       doing any serious text editing in this application

class TextTabbedEditor(editors.TabbedEditor):
    """Multi purpose text editor
    """

    def __init__(self, filePath):
        super(TextTabbedEditor, self).__init__(None, filePath)

        self.tabWidget = QtGui.QTextEdit()
        self.textDocument = None

    def initialise(self, mainWindow):
        super(TextTabbedEditor, self).initialise(mainWindow)

        self.textDocument = QtGui.QTextDocument()
        with open(self.filePath, "r") as file_:
            self.textDocument.setPlainText(file_.read())

        self.tabWidget.setDocument(self.textDocument)
        self.textDocument.setModified(False)

        self.textDocument.setUndoRedoEnabled(True)
        self.textDocument.undoAvailable.connect(self.slot_undoAvailable)
        self.textDocument.redoAvailable.connect(self.slot_redoAvailable)

        self.textDocument.contentsChanged.connect(self.slot_contentsChanged)

    def finalise(self):
        super(TextTabbedEditor, self).finalise()

        self.textDocument = None

    def hasChanges(self):
        return self.textDocument.isModified()

    def undo(self):
        self.textDocument.undo()

    def redo(self):
        self.textDocument.redo()

    def slot_undoAvailable(self, available):
        self.mainWindow.undoAction.setEnabled(available)

    def slot_redoAvailable(self, available):
        self.mainWindow.redoAction.setEnabled(available)

    def slot_contentsChanged(self):
        self.markHasChanges(self.hasChanges())

    def saveAs(self, targetPath, updateCurrentPath = True):
        self.nativeData = self.textDocument.toPlainText()

        return super(TextTabbedEditor, self).saveAs(targetPath, updateCurrentPath)

class TextTabbedEditorFactory(editors.TabbedEditorFactory):
    def getFileExtensions(self):
        extensions = {"py", "lua", "txt", "xml"}
        # this is just temporary, will go away when scheme, looknfeel and font editors are in place
        temporaryExtensions = {"scheme", "looknfeel", "font"}
        extensions.update(temporaryExtensions)
        return extensions

    def canEditFile(self, filePath):
        extensions = self.getFileExtensions()

        for extension in extensions:
            if filePath.endswith("." + extension):
                return True

        return False

    def create(self, filePath):
        return TextTabbedEditor(filePath)
