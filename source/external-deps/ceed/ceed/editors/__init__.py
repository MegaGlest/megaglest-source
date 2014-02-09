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

"""This module contains interfaces needed to run editors tabs (multi-file editing)

Also groups all the editors together to avoid cluttering the root directory.
"""

from PySide import QtCore
from PySide import QtGui

import os.path
import codecs

from ceed import compatibility

import ceed.ui.editors.notypedetected
import ceed.ui.editors.multipletypesdetected
import ceed.ui.editors.multiplepossiblefactories

class NoTypeDetectedDialog(QtGui.QDialog):
    def __init__(self, compatibilityManager):
        super(NoTypeDetectedDialog, self).__init__()

        self.ui = ceed.ui.editors.notypedetected.Ui_NoTypeDetectedDialog()
        self.ui.setupUi(self)

        self.typeChoice = self.findChild(QtGui.QListWidget, "typeChoice")

        for type_ in compatibilityManager.getKnownTypes():
            item = QtGui.QListWidgetItem()
            item.setText(type_)

            # TODO: We should give a better feedback about what's compatible with what
            item.setToolTip("Compatible with CEGUI: %s" % (", ".join(compatibilityManager.getCEGUIVersionsCompatibleWithType(type_))))

            self.typeChoice.addItem(item)

class MultipleTypesDetectedDialog(QtGui.QDialog):
    def __init__(self, compatibilityManager, possibleTypes):
        super(MultipleTypesDetectedDialog, self).__init__()

        self.ui = ceed.ui.editors.multipletypesdetected.Ui_MultipleTypesDetectedDialog()
        self.ui.setupUi(self)

        self.typeChoice = self.findChild(QtGui.QListWidget, "typeChoice")

        for type_ in compatibilityManager.getKnownTypes():
            item = QtGui.QListWidgetItem()
            item.setText(type_)

            if type_ in possibleTypes:
                font = QtGui.QFont()
                font.setBold(True)
                item.setFont(font)

            # TODO: We should give a better feedback about what's compatible with what
            item.setToolTip("Compatible with CEGUI: %s" % (", ".join(compatibilityManager.getCEGUIVersionsCompatibleWithType(type_))))

            self.typeChoice.addItem(item)

class MultiplePossibleFactoriesDialog(QtGui.QDialog):
    def __init__(self, possibleFactories):
        super(MultiplePossibleFactoriesDialog, self).__init__()

        self.ui = ceed.ui.editors.multiplepossiblefactories.Ui_MultiplePossibleFactoriesDialog()
        self.ui.setupUi(self)

        self.factoryChoice = self.findChild(QtGui.QListWidget, "factoryChoice")

        for factory in possibleFactories:
            item = QtGui.QListWidgetItem()
            item.setText(factory.getName())
            item.setData(QtCore.Qt.UserRole, factory)

            self.factoryChoice.addItem(item)

class TabbedEditor(object):
    """This is the base class for a class that takes a file and allows manipulation
    with it. It occupies exactly 1 tab space.
    """

    def __init__(self, compatibilityManager, filePath):
        """Constructs the editor.

        compatibilityManager - manager that should be used to transform data between
                               various data types using compatibility layers
        filePath - absolute file path of the file that should be opened
        """

        self.compatibilityManager = compatibilityManager
        self.desiredSavingDataType = "" if self.compatibilityManager is None else self.compatibilityManager.EditorNativeType
        self.nativeData = None

        self.requiresProject = False

        self.initialised = False
        self.active = False

        self.mainWindow = None

        self.filePath = os.path.normpath(filePath)

        self.tabWidget = None
        self.tabLabel = os.path.basename(self.filePath)

        #Set up a QFileSystemWatcher to watch for external changes to a file
        self.fileMonitor = None
        self.fileChangedByExternalProgram = False
        self.displayingReloadAlert = False

    def slot_fileChangedByExternalProgram(self):
        """The callback method for external file changes.  This method is
        immediately called when this tab is open.  Otherwise, it's called when
        the user clicks on the tab"""
        self.fileChangedByExternalProgram = True
        if self.active:
            self.askForFileReload()

    def askForFileReload(self):
        """Pops a alert menu open asking the user s/he would like to reload the
        file due to external changes being made"""

        # If multiple file writes are made by an external program, multiple
        # pop-ups will appear. Prevent that with a switch.
        if not self.displayingReloadAlert:
            self.displayingReloadAlert = True
            ret = QtGui.QMessageBox.question(self.mainWindow,
                                             "File has been modified externally!",
                                             "The file that you have currently opened has been modified outside the CEGUI Unified Editor.\n\nReload the file?\n\nIf you select Yes, ALL UNDO HISTORY WILL BE DESTROYED!",
                                             QtGui.QMessageBox.No | QtGui.QMessageBox.Yes,
                                             QtGui.QMessageBox.No) # defaulting to No is safer IMO

            self.fileChangedByExternalProgram = False

            if ret == QtGui.QMessageBox.Yes:
                self.reinitialise()

            elif ret == QtGui.QMessageBox.No:
                # FIXME: We should somehow make CEED think that we have changes :-/
                #        That is hard to do because CEED relies on QUndoStack to get that info
                pass

            else:
                # how did we get here?
                assert(False)

            self.displayingReloadAlert = False

    def initialise(self, mainWindow):
        """This method loads everything up so this editor is ready to be switched to"""

        assert(not self.initialised)
        assert(self.tabWidget)

        self.mainWindow = mainWindow
        self.tabWidget.tabbedEditor = self

        self.mainWindow.tabs.addTab(self.tabWidget, self.tabLabel)

        if self.compatibilityManager is not None:
            rawData = codecs.open(self.filePath, mode = "r", encoding = "utf-8").read()
            rawDataType = ""

            if rawData == "":
                # it's an empty new file, the derived classes deal with this separately
                self.nativeData = rawData

                if mainWindow.project is None:
                    self.desiredSavingDataType = self.compatibilityManager.EditorNativeType
                else:
                    self.desiredSavingDataType = self.compatibilityManager.getSuitableDataTypeForCEGUIVersion(mainWindow.project.CEGUIVersion)

            else:
                try:
                    rawDataType = self.compatibilityManager.guessType(rawData, self.filePath)

                    # A file exists and the editor has it open, so watch it for
                    # external changes.
                    self.addFileMonitor(self.filePath)

                except compatibility.NoPossibleTypesError:
                    dialog = NoTypeDetectedDialog(self.compatibilityManager)
                    result = dialog.exec_()

                    rawDataType = self.compatibilityManager.EditorNativeType
                    self.nativeData = ""

                    if result == QtGui.QDialog.Accepted:
                        selection = dialog.typeChoice.selectedItems()

                        if len(selection) == 1:
                            rawDataType = selection[0].text()
                            self.nativeData = None

                except compatibility.MultiplePossibleTypesError as e:
                    # if no project is opened or if the opened file was detected as something not suitable for the target CEGUI version of the project
                    if (mainWindow.project is None) or (self.compatibilityManager.getSuitableDataTypeForCEGUIVersion(mainWindow.project.CEGUIVersion) not in e.possibleTypes):
                        dialog = MultipleTypesDetectedDialog(self.compatibilityManager, e.possibleTypes)
                        result = dialog.exec_()

                        rawDataType = self.compatibilityManager.EditorNativeType
                        self.nativeData = ""

                        if result == QtGui.QDialog.Accepted:
                            selection = dialog.typeChoice.selectedItems()

                            if len(selection) == 1:
                                rawDataType = selection[0].text()
                                self.nativeData = None

                    else:
                        rawDataType = self.compatibilityManager.getSuitableDataTypeForCEGUIVersion(mainWindow.project.CEGUIVersion)
                        self.nativeData = None

                # by default, save in the same format as we opened in
                self.desiredSavingDataType = rawDataType

                if mainWindow.project is not None:
                    projectCompatibleDataType = self.compatibilityManager.CEGUIVersionTypes[mainWindow.project.CEGUIVersion]

                    if projectCompatibleDataType != rawDataType:
                        if QtGui.QMessageBox.question(mainWindow,
                                                      "Convert to format suitable for opened project?",
                                                      "File you are opening isn't suitable for the project that is opened at the moment.\n"
                                                      "Do you want to convert it to a suitable format upon saving?\n"
                                                      "(from '%s' to '%s')\n"
                                                      "Data COULD be lost, make a backup!)" % (rawDataType, projectCompatibleDataType),
                                                      QtGui.QMessageBox.Yes | QtGui.QMessageBox.No, QtGui.QMessageBox.No) == QtGui.QMessageBox.Yes:
                            self.desiredSavingDataType = projectCompatibleDataType

                # if nativeData is "" at this point, data type was not successful and user didn't select
                # any data type as well so we will just use given file as an empty file

                if self.nativeData != "":
                    try:
                        self.nativeData = self.compatibilityManager.transform(rawDataType, self.compatibilityManager.EditorNativeType, rawData)

                    except compatibility.LayerNotFoundError:
                        # TODO: Dialog, can't convert
                        self.nativeData = ""

        self.initialised = True

    def finalise(self):
        """Cleans up after itself
        this is usually called when you want the tab closed
        """

        assert(self.initialised)
        assert(self.tabWidget)

        self.initialised = False

    def reinitialise(self):
        """Reinitialises this tabbed editor, effectivelly reloading the file
        off the hard drive again
        """

        wasCurrent = self.mainWindow.activeEditor is self

        mainWindow = self.mainWindow
        self.finalise()
        self.initialise(mainWindow)

        if wasCurrent:
            self.makeCurrent()

    def destroy(self):
        """Removes itself from the tab list and irrevocably destroys
        data associated with itself
        """

        i = 0
        wdt = self.mainWindow.tabs.widget(i)
        tabRemoved = False

        while wdt:
            if wdt == self.tabWidget:
                self.mainWindow.tabs.removeTab(i)
                tabRemoved = True
                break

            i = i + 1
            wdt = self.mainWindow.tabs.widget(i)

        assert(tabRemoved)

    def editorMenu(self):
        """Returns the editorMenu for this editor, or None.
        The editorMenu is shared across editors so this returns
        None if this editor is not the active editor.
        """
        return self.mainWindow.editorMenu if self.active else None

    def rebuildEditorMenu(self, editorMenu):
        """The main window has one special menu on its menubar (editorMenu)
        whose items are updated dynamically to match the currently active
        editor. Implement this method if you want to use this menu for an editor.
        Returns two booleans: Visible, Enabled
        """

        return False, False

    def activate(self):
        """The tab gets "on stage", it's been clicked on and is now the only active
        tab. There can be either 0 tabs active (blank screen) or exactly 1 tab
        active.
        """

        currentActive = self.mainWindow.activeEditor

        # no need to deactivate and then activate again
        if currentActive == self:
            return

        if currentActive is not None:
            currentActive.deactivate()

        self.active = True

        self.mainWindow.activeEditor = self
        self.mainWindow.undoViewer.setUndoStack(self.getUndoStack())

        # If the file was changed by an external program, ask the user to reload
        # the changes
        if self.fileMonitor is not None and self.fileChangedByExternalProgram:
            self.askForFileReload()

        edMenu = self.mainWindow.editorMenu
        edMenu.clear()
        visible, enabled = self.rebuildEditorMenu(edMenu)
        edMenu.menuAction().setVisible(visible)
        edMenu.menuAction().setEnabled(enabled)

    def deactivate(self):
        """The tab gets "off stage", user switched to another tab.
        This is also called when user closes the tab (deactivate and then finalise
        is called).
        """

        self.active = False

        if self.mainWindow.activeEditor == self:
            self.mainWindow.activeEditor = None
            edMenu = self.mainWindow.editorMenu
            edMenu.clear()
            edMenu.menuAction().setEnabled(False)
            edMenu.menuAction().setVisible(False)

    def makeCurrent(self):
        """Makes this tab editor current (= the selected tab)"""

        # (this should automatically handle the respective deactivate and activate calls)

        self.mainWindow.tabs.setCurrentWidget(self.tabWidget)

    def hasChanges(self):
        """Checks whether this TabbedEditor contains changes
        (= it should be saved before closing it)"""

        return False

    def markHasChanges(self, hasChanges):
        """Marks that this tabbed editor has changes, in this implementation this means
        that the tab in the tab list gets an icon
        """

        if self.mainWindow is None:
            return

        if hasChanges:
            self.mainWindow.tabs.setTabIcon(self.mainWindow.tabs.indexOf(self.tabWidget), QtGui.QIcon("icons/tabs/has_changes.png"))
        else:
            self.mainWindow.tabs.setTabIcon(self.mainWindow.tabs.indexOf(self.tabWidget), QtGui.QIcon())

    def saveAs(self, targetPath, updateCurrentPath = True):
        """Causes the tabbed editor to save all it's progress to the file.
        targetPath should be absolute file path.
        """

        outputData = self.nativeData if self.nativeData is not None else ""
        if self.compatibilityManager is not None:
            outputData = self.compatibilityManager.transform(self.compatibilityManager.EditorNativeType, self.desiredSavingDataType, self.nativeData)

        # Stop monitoring the file, the changes that are about to occur are not
        # picked up as being from an external program!
        self.removeFileMonitor(self.filePath)

        try:
            f = codecs.open(targetPath, mode = "w", encoding = "utf-8")
            f.write(outputData)
            f.close()

        except IOError as e:
            # The rest of the code is skipped, so be sure to turn file
            # monitoring back on
            self.addFileMonitor(self.filePath)
            QtGui.QMessageBox.critical(self, "Error saving file!",
                                       "CEED encountered an error trying to save the file.\n\n(exception details: %s)" % (e))
            return False

        if updateCurrentPath:
            # changes current path to the path we saved to
            self.filePath = targetPath

            # update tab text
            self.tabLabel = os.path.basename(self.filePath)

            # because this might be called even before initialise is called!
            if self.mainWindow is not None:
                self.mainWindow.tabs.setTabText(self.mainWindow.tabs.indexOf(self.tabWidget), self.tabLabel)

        self.addFileMonitor(self.filePath)
        return True

    def addFileMonitor(self, path):
        """Adds a file monitor to the specified file so CEED will alert the
        user that an external change happened to the file"""
        if self.fileMonitor is None:
            self.fileMonitor = QtCore.QFileSystemWatcher(self.mainWindow)
            self.fileMonitor.fileChanged.connect(self.slot_fileChangedByExternalProgram)
        self.fileMonitor.addPath(path)

    def removeFileMonitor(self, path):
        """Removes the file monitor from the specified path"""
        if self.fileMonitor is not None: # FIXME: Will it ever be None at this point?
            self.fileMonitor.removePath(path)

    def save(self):
        """Saves all progress to the same file we have opened at the moment
        """

        return self.saveAs(self.filePath)

    def discardChanges(self):
        """Causes the tabbed editor to discard all it's progress"""

        # early out
        if not self.hasChanges():
            return

        # the default but kind of wasteful implementation
        self.reinitialise()

        # the state of the tabbed editor should be valid at this point

    def undo(self):
        """Called by the mainwindow whenever undo is requested"""
        pass

    def redo(self):
        """Called by the mainwindow whenever redo is requested"""
        pass

    def revert(self):
        """Called by the mainwindow whenever revert is requested

        Revert simply goes back to the state of the file on disk,
        disregarding any changes made since then
        """

        self.discardChanges()

    def getUndoStack(self):
        """Returns UndoStack or None is the tabbed editor doesn't have undo stacks.
        This is useful for QUndoView

        Note: If you use UndoStack in your tabbed editor, inherit from UndoStackTabbedEditor
              below, it will save you a lot of work (synchronising all the actions and their texts, etc..)
        """

        return None

    def getDesiredSavingDataType(self):
        """Returns current desired saving data type. Data type that will be used when user requests to save this file
        """

        return self.desiredSavingDataType if self.compatibilityManager is not None else None

    def performCopy(self):
        """Performs copy of the editor's selection to the clipboard.

        Default implementation doesn't do anything.

        Returns: True if the operation was successful
        """
        return False

    def performCut(self):
        """Performs cut of the editor's selection to the clipboard.

        Default implementation doesn't do anything.

        Returns: True if the operation was successful
        """
        return False

    def performPaste(self):
        """Performs paste from the clipboard to the editor.

        Default implementation doesn't do anything

        Returns: True if the operation was successful
        """
        return False

    def performDelete(self):
        """Deletes the selected items in the editor.

        Default implementation doesn't do anything

        Returns: True if the operation was successful
        """
        return False

    def zoomIn(self):
        """Called by the mainwindow whenever zoom is requested"""
        pass

    def zoomOut(self):
        """Called by the mainwindow whenever zoom is requested"""
        pass

    def zoomReset(self):
        """Called by the mainwindow whenever zoom is requested"""
        pass

class UndoStackTabbedEditor(TabbedEditor):
    """Used for tabbed editors that have one shared undo stack. This saves a lot
    of boilerplate code for undo/redo action synchronisation and the undo/redo itself
    """

    def __init__(self, compatibilityManager, filePath):
        super(UndoStackTabbedEditor, self).__init__(compatibilityManager, filePath)

        self.undoStack = QtGui.QUndoStack()

        self.undoStack.setUndoLimit(settings.getEntry("global/undo/limit").value)
        self.undoStack.setClean()

        self.undoStack.canUndoChanged.connect(self.slot_undoAvailable)
        self.undoStack.canRedoChanged.connect(self.slot_redoAvailable)

        self.undoStack.undoTextChanged.connect(self.slot_undoTextChanged)
        self.undoStack.redoTextChanged.connect(self.slot_redoTextChanged)

        self.undoStack.cleanChanged.connect(self.slot_cleanChanged)

    def initialise(self, mainWindow):
        super(UndoStackTabbedEditor, self).initialise(mainWindow)
        self.undoStack.clear()

    def activate(self):
        super(UndoStackTabbedEditor, self).activate()

        undoStack = self.getUndoStack()

        self.mainWindow.undoAction.setEnabled(undoStack.canUndo())
        self.mainWindow.redoAction.setEnabled(undoStack.canRedo())

        self.mainWindow.undoAction.setText("Undo %s" % (undoStack.undoText()))
        self.mainWindow.redoAction.setText("Redo %s" % (undoStack.redoText()))

        self.mainWindow.saveAction.setEnabled(not undoStack.isClean())

    def hasChanges(self):
        return not self.undoStack.isClean()

    def saveAs(self, targetPath, updateCurrentPath = True):
        if super(UndoStackTabbedEditor, self).saveAs(targetPath, updateCurrentPath):
            self.undoStack.setClean()
            return True

        return False

    def discardChanges(self):
        # since we have undo stack, we can simply use it instead of the slower
        # reinitialisation approach

        undoStack = self.getUndoStack()
        cleanIndex = undoStack.cleanIndex()

        if cleanIndex == -1:
            # it is possible that we can't undo/redo to cleanIndex
            # this can happen because of undo limitations (at most 100 commands
            # and we need > 100 steps)
            super(UndoStackTabbedEditor, self).discardChanges()

        else:
            undoStack.setIndex(cleanIndex)

    def undo(self):
        self.getUndoStack().undo()

    def redo(self):
        self.getUndoStack().redo()

    def getUndoStack(self):
        return self.undoStack

    def slot_undoAvailable(self, available):
        # conditional because this might be called even before initialise is called!
        if self.mainWindow is not None:
            self.mainWindow.undoAction.setEnabled(available)

    def slot_redoAvailable(self, available):
        # conditional because this might be called even before initialise is called!
        if self.mainWindow is not None:
            self.mainWindow.redoAction.setEnabled(available)

    def slot_undoTextChanged(self, text):
        # conditional because this might be called even before initialise is called!
        if self.mainWindow is not None:
            self.mainWindow.undoAction.setText("Undo %s" % (text))

    def slot_redoTextChanged(self, text):
        # conditional because this might be called even before initialise is called!
        if self.mainWindow is not None:
            self.mainWindow.redoAction.setText("Redo %s" % (text))

    def slot_cleanChanged(self, clean):
        # clean means that the undo stack is at a state where it's in sync with the underlying file
        # we set the undostack as clean usually when saving the file so we will assume that there
        if self.mainWindow is not None:
            self.mainWindow.saveAction.setEnabled(not clean)

        self.markHasChanges(not clean)

class TabbedEditorFactory(object):
    """Constructs instances of TabbedEditor (multiple instances of one TabbedEditor
    can coexist - user editing 2 layouts for example - with the ability to switch
    from one to another)
    """

    def getFileExtensions(self):
        """Returns a set of file extensions (without prefix dots) that can be edited by this factory"""

        raise NotImplementedError("All subclasses of TabbedEditorFactory have to override the 'getFileExtensions' method")

    def canEditFile(self, filePath):
        """This checks whether instance created by this factory can edit given file"""

        raise NotImplementedError("All subclasses of TabbedEditorFactory have to override the 'canEditFile' method")

    def create(self, filePath):
        """Creates the respective TabbedEditor instance

        This should only be called with a filePath the factory reported
        as editable by the instances
        """

        raise NotImplementedError("All subclasses of TabbedEditorFactory have to override the 'create' method")

    # note: destroy doesn't really make sense as python is reference counted
    #       and everything is garbage collected

class MessageTabbedEditor(TabbedEditor):
    """This is basically a stub tabbed editor, it simply displays a message
    and doesn't allow any sort of editing at all, all functionality is stubbed

    This is for internal use only so there is no factory for this particular editor
    """
    def __init__(self, filePath, message):
        super(MessageTabbedEditor, self).__init__(None, filePath)

        self.label = QtGui.QLabel(message)
        self.label.setWordWrap(True)
        self.tabWidget = QtGui.QScrollArea()
        self.tabWidget.setWidget(self.label)

    def hasChanges(self):
        return False

from ceed import settings
