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

##
# This module contains interfaces for multi mode tabbed editors (visual, source, ...)

from ceed import editors
from ceed import commands

from PySide.QtGui import QTabWidget, QTextEdit, QMessageBox, QWidget

class ModeSwitchCommand(commands.UndoCommand):
    """Undo command that is pushed to the undo stack whenever user switches edit modes.

    Switching edit mode has to be an undoable command because the other commands might
    or might not make sense if user is not in the right mode.

    This has a drawback that switching to Live Preview (layout editing) and back is
    undoable even though you can't affect the document in any way whilst in Live Preview
    mode.
    """

    def __init__(self, tabbedEditor, oldTabIndex, newTabIndex):
        super(ModeSwitchCommand, self).__init__()

        self.tabbedEditor = tabbedEditor

        self.oldTabIndex = oldTabIndex
        self.newTabIndex = newTabIndex

        # we never every merge edit mode changes, no need to define this as refreshText
        self.setText("Change edit mode to '%s'" % self.tabbedEditor.tabText(newTabIndex))

    def undo(self):
        super(ModeSwitchCommand, self).undo()

        self.tabbedEditor.ignoreCurrentChangedForUndo = True
        self.tabbedEditor.setCurrentIndex(self.oldTabIndex)
        self.tabbedEditor.ignoreCurrentChangedForUndo = False

    def redo(self):
        # to avoid multiple event firing
        if self.tabbedEditor.currentIndex() != self.newTabIndex:
            self.tabbedEditor.ignoreCurrentChangedForUndo = True
            self.tabbedEditor.setCurrentIndex(self.newTabIndex)
            self.tabbedEditor.ignoreCurrentChangedForUndo = False

        super(ModeSwitchCommand, self).redo()

class EditMode(object):
    """Interface class for the edit mode widgets (more a mixin class)
    This practically just ensures that the inherited classes have activate and deactivate
    methods.
    """

    def __init__(self):
        pass

    def activate(self):
        """This is called whenever this edit mode is activated (user clicked on the tab button
        representing it). It's not called when user switches from a different file tab whilst
        this tab was already active when user was switching from this file tab to another one!

        Activation can't be canceled and must always happen when requested!
        """
        pass

    def deactivate(self):
        """This is called whenever this edit mode is deactivated (user clicked on another tab button
        representing another mode). It's not called when user switches to this file tab from another
        file tab and this edit mode was already active before user switched from the file tab to another
        file tab.

        if this returns True, all went well
        if this returns False, the action is terminated and the current edit mode stays in place
        """

        return True

class CodeEditModeCommand(commands.UndoCommand):
    """
    Undo command for code edit mode.

    TODO: Extremely memory hungry implementation for now, I have to figure out how to use my own
    QUndoStack with QTextDocument in the future to fix this.
    """

    def __init__(self, codeEditing, oldText, newText, totalChange):
        super(CodeEditModeCommand, self).__init__()

        self.codeEditing = codeEditing
        self.oldText = oldText
        self.newText = newText

        self.totalChange = totalChange

        self.dryRun = True

        self.refreshText()

    def refreshText(self):
        if self.totalChange == 1:
            self.setText("Code edit, changed 1 character")
        else:
            self.setText("Code edit, changed %i characters" % (self.totalChange))

    def id(self):
        return 1000 + 1

    def mergeWith(self, cmd):
        assert(self.codeEditing == cmd.codeEditing)

        # TODO: 10 chars for now for testing
        if self.totalChange + cmd.totalChange < 10:
            self.totalChange += cmd.totalChange
            self.newText = cmd.newText

            self.refreshText()

            return True

        return False

    def undo(self):
        super(CodeEditModeCommand, self).undo()

        self.codeEditing.ignoreUndoCommands = True
        self.codeEditing.setPlainText(self.oldText)
        self.codeEditing.ignoreUndoCommands = False

    def redo(self):
        if not self.dryRun:
            self.codeEditing.ignoreUndoCommands = True
            self.codeEditing.setPlainText(self.newText)
            self.codeEditing.ignoreUndoCommands = False

        self.dryRun = False

        super(CodeEditModeCommand, self).redo()

class CodeEditMode(QTextEdit, EditMode):
    """This is the most used alternative editing mode that allows you to edit raw code.

    Raw code is mostly XML in CEGUI formats but can be anything else in a generic sense
    """

    # TODO: Some highlighting and other aids

    def __init__(self):
        super(CodeEditMode, self).__init__()

        self.ignoreUndoCommands = False
        self.lastUndoText = None

        self.document().setUndoRedoEnabled(False)
        self.document().contentsChange.connect(self.slot_contentsChange)

    def getNativeCode(self):
        """Returns native source code from your editor implementation."""

        raise NotImplementedError("Every CodeEditing derived class must implement CodeEditing.getRefreshedNativeSource")
        #return ""

    def propagateNativeCode(self, code):
        """Synchronizes your editor implementation with given native source code.

        Returns True if changes were accepted (the code was valid, etc...)
        Returns False if changes weren't accepted (invalid code most likely)
        """

        raise NotImplementedError("Every CodeEditing derived class must implement CodeEditing.propagateNativeSource")
        #return False

    def refreshFromVisual(self):
        """Refreshes this Code editing mode with current native source code."""

        source = self.getNativeCode()

        self.ignoreUndoCommands = True
        self.setPlainText(source)
        self.ignoreUndoCommands = False

    def propagateToVisual(self):
        """Propagates source code from this Code editing mode to your editor implementation."""

        source = self.document().toPlainText()

        # for some reason, Qt calls hideEvent even though the tab widget was never shown :-/
        # in this case the source will be empty and parsing it will fail
        if source == "":
            return True

        return self.propagateNativeCode(source)

    def activate(self):
        super(CodeEditMode, self).activate()

        self.refreshFromVisual()

    def deactivate(self):
        changesAccepted = self.propagateToVisual()
        ret = changesAccepted

        if not changesAccepted:
            # the file contains more than just CR LF
            result = QMessageBox.question(self,
                                 "Parsing the Code changes failed!",
                                 "Parsing of the changes done in Code edit mode failed, the result couldn't be accepted.\n"
                                 "Press Cancel to stay in the Code edit mode to correct the mistake(s) or press Discard to "
                                 "discard the changes and go back to the previous state (before you entered the code edit mode).",
                                 QMessageBox.Cancel | QMessageBox.Discard, QMessageBox.Cancel)

            if result == QMessageBox.Cancel:
                # return False to indicate we don't want to switch out of this widget
                ret = False

            elif result == QMessageBox.Discard:
                # we return True, the visual element wasn't touched (the error is thrown before that)
                ret = True

        return ret and super(CodeEditMode, self).deactivate()

    def slot_contentsChange(self, position, charsRemoved, charsAdded):
        if not self.ignoreUndoCommands:
            totalChange = charsRemoved + charsAdded

            cmd = CodeEditModeCommand(self, self.lastUndoText, self.toPlainText(), totalChange)
            self.tabbedEditor.undoStack.push(cmd)

        self.lastUndoText = self.toPlainText()

class MultiModeTabbedEditor(editors.UndoStackTabbedEditor, QTabWidget):
    """This class represents tabbed editor that has little tabs on the bottom
    allowing you to switch editing "modes" - visual, code, ...

    Not all modes have to be able to edit! Switching modes pushes undo actions
    onto the UndoStack to avoid confusion when undoing. These actions never merge
    together.

    You yourself are responsible for putting new tabs into this widget!
    You should not add/remove modes after the construction!
    """

    def __init__(self, compatibilityManager, filePath):
        editors.UndoStackTabbedEditor.__init__(self, compatibilityManager, filePath)
        QTabWidget.__init__(self)

        self.setTabPosition(QTabWidget.South)
        self.setTabShape(QTabWidget.Triangular)

        self.currentChanged.connect(self.slot_currentChanged)

        # will be -1, that means no tabs are selected
        self.currentTabIndex = self.currentIndex()
        # when canceling tab transfer we have to switch back and avoid unnecessary deactivate/activate cycle
        self.ignoreCurrentChanged = False
        # to avoid unnecessary undo command pushes we ignore currentChanged if we are
        # inside ModeChangeCommand.undo or redo
        self.ignoreCurrentChangedForUndo = False

    def initialise(self, mainWindow):
        super(MultiModeTabbedEditor, self).initialise(mainWindow)

        # the emitted signal only contains the new signal, we have to keep track
        # of the current index ourselves so that we know which one is the "old" one
        # for the undo command

        # by this time tabs should have been added so this should not be -1
        self.currentTabIndex = self.currentIndex()
        assert(self.currentTabIndex != -1)

    def slot_currentChanged(self, newTabIndex):
        if self.ignoreCurrentChanged:
            return

        oldTab = self.widget(self.currentTabIndex)

        # FIXME: workaround for PySide 1.0.6, I suspect this is a bug in PySide! http://bugs.pyside.org/show_bug.cgi?id=988
        if newTabIndex is None:
            newTabIndex = -1

        elif isinstance(newTabIndex, QWidget):
            for i in xrange(0, self.count()):
                if newTabIndex is self.widget(i):
                    newTabIndex = i
                    break

            assert(not isinstance(newTabIndex, QWidget))
        # END OF FIXME

        newTab = self.widget(newTabIndex)

        if oldTab:
            if not oldTab.deactivate():
                self.ignoreCurrentChanged = True
                self.setCurrentWidget(oldTab)
                self.ignoreCurrentChanged = False

                return

        if newTab:
            newTab.activate()

        if not self.ignoreCurrentChangedForUndo:
            cmd = ModeSwitchCommand(self, self.currentTabIndex, newTabIndex)
            self.undoStack.push(cmd)

        self.currentTabIndex = newTabIndex
