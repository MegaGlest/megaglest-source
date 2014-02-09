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

from ceed.settings import interface_types

class SettingsInterface(object):
    def __init__(self, settings):
        self.settings = settings

class QtSettingsInterface(SettingsInterface, QtGui.QDialog):
    def __init__(self, settings):
        SettingsInterface.__init__(self, settings)
        QtGui.QDialog.__init__(self)

        self.setWindowTitle(self.settings.label)
        self.setWindowModality(QtCore.Qt.ApplicationModal)

        # sort everything so that it comes at the right spot when iterating
        self.settings.sort()

        # the basic UI
        self.layout = QtGui.QVBoxLayout()

        self.label = QtGui.QLabel(self.settings.help)
        self.label.setWordWrap(True)
        self.layout.addWidget(self.label)

        self.tabs = QtGui.QTabWidget()
        self.tabs.setTabPosition(QtGui.QTabWidget.North)
        self.layout.addWidget(self.tabs)

        self.setLayout(self.layout)

        # for each category, add a tab
        addTab = self.tabs.addTab
        for category in self.settings.categories:
            addTab(interface_types.InterfaceCategory(category, self.tabs), category.label)

        # apply, cancel, etc...
        self.buttonBox = QtGui.QDialogButtonBox(QtGui.QDialogButtonBox.Apply | QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel)
        self.buttonBox.clicked.connect(self.slot_buttonBoxClicked)
        self.layout.addWidget(self.buttonBox)

        # Restart required
        self.needRestart = QtGui.QMessageBox()
        self.needRestart.setWindowTitle("CEED")
        self.needRestart.setIcon(QtGui.QMessageBox.Warning)
        self.needRestart.setText("Restart is required for the changes to "
                                 "take effect.")

    def restartRequired(self):
        if self.settings.changesRequireRestart:
            self.needRestart.exec_()
            # FIXME: Kill the app; then restart it.
            #
            # - This may or may not be the way to get rid of this, but for the
            #   moment we use it as a "the user has been notified they must restart
            #   the application" flag.
            self.settings.changesRequireRestart = False
        return

    def slot_buttonBoxClicked(self, button):
        if self.buttonBox.buttonRole(button) == QtGui.QDialogButtonBox.ApplyRole:
            self.settings.applyChanges()

            # Check if restart required
            self.restartRequired()

        elif self.buttonBox.buttonRole(button) == QtGui.QDialogButtonBox.AcceptRole:
            self.settings.applyChanges()
            self.accept()

            # Check if restart required
            self.restartRequired()

        elif self.buttonBox.buttonRole(button) == QtGui.QDialogButtonBox.RejectRole:
            self.settings.discardChanges()
            self.reject()

            # - Reset any entries with changes to their stored value.
            for tabIndex in xrange(self.tabs.count()):
                self.tabs.widget(tabIndex).discardChanges()

        # - Regardless of the action above, all categories are now unchanged.
        for tabIndex in xrange(self.tabs.count()):
            self.tabs.widget(tabIndex).markAsUnchanged()

        # FIXME: That is not entirely true; using the 'X' to close the Settings
        # dialog is not handled here; although, this "bug as a feature" allows
        # Settings to be modified, closed, and it will remember (but not apply)
        # the previous changes.
