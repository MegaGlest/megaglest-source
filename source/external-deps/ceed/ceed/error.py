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

"""Implements a hook that displays a dialog whenever an exception is uncaught.
We display information to the user about where to submit a bug report and what
to include.
"""

import sys

from PySide.QtGui import QDialog, QTextBrowser, QLabel
import logging

from OpenGL import GL

from ceed import version
import ceed.ui.exceptiondialog

class ExceptionDialog(QDialog):
    """This is a dialog that gets shown whenever an exception is thrown and
    isn't caught. This is done via duck overriding the sys.excepthook.
    """

    # TODO: Add an option to pack all the relevant data and error messages
    #       to a zip file for easier to reproduce bug reports.

    def __init__(self, excType, excMessage, excTraceback, mainWindow):
        super(ExceptionDialog, self).__init__()

        self.ui = ceed.ui.exceptiondialog.Ui_ExceptionDialog()
        self.ui.setupUi(self)

        self.mainWindow = mainWindow

        self.details = self.findChild(QTextBrowser, "details")
        self.mantisLink = self.findChild(QLabel, "mantisLink")

        self.setWindowTitle("Uncaught exception '%s'" % (excType))

        self.detailsStr = unicode("Exception message: %s\n\n"
                                  "Traceback:\n"
                                  "========================\n"
                                  "%s\n"
                                  "Versions:\n"
                                  "========================\n"
                                  "%s" % (excMessage, self.getTracebackStr(excTraceback), self.getVersionsStr()))

        self.details.setPlainText(self.detailsStr)

    def getTracebackStr(self, excTraceback):
        import traceback

        formattedTraceback = traceback.format_tb(excTraceback)
        return "\n".join(formattedTraceback)

    def getVersionsStr(self):
        lines = []
        lines.append("CEED revision: %s" % (version.MERCURIAL_REVISION))

        lines.append("CEED version: %s" % (version.CEED))

        lines.append("HW architecture: %s" % (unicode(version.SYSTEM_ARCH)))
        lines.append("HW type: %s" % (version.SYSTEM_TYPE))
        lines.append("HW processor: %s" % (version.SYSTEM_PROCESSOR))
        lines.append("OS type: %s" % (version.OS_TYPE))
        lines.append("OS release: %s" % (version.OS_RELEASE))
        lines.append("OS version: %s" % (version.OS_VERSION))

        if version.OS_TYPE == "Windows":
            lines.append("OS Windows: %s" % (unicode(version.WINDOWS)))
        elif version.OS_TYPE == "Linux":
            lines.append("OS Linux: %s" % (unicode(version.LINUX)))
        elif version.OS_TYPE == "Java":
            lines.append("OS Java: %s" % (unicode(version.JAVA)))
        elif version.OS_TYPE == "Darwin":
            lines.append("OS Darwin: %s" % (unicode(version.MAC)))
        else:
            lines.append("OS Unknown: No version info available")

        lines.append("SW Python: %s" % (version.PYTHON))
        lines.append("SW PySide: %s" % (version.PYSIDE))
        lines.append("SW Qt: %s" % (version.QT))
        lines.append("SW PyCEGUI: %s" % (version.PYCEGUI))

        lines.append("GL bindings version: %s" % (version.OPENGL))

        if self.mainWindow.ceguiInstance is not None:
            self.mainWindow.ceguiInstance.makeGLContextCurrent()
            lines.append("GL version: %s" % (GL.glGetString(GL.GL_VERSION)))
            lines.append("GL vendor: %s" % (GL.glGetString(GL.GL_VENDOR)))
            lines.append("GL renderer: %s" % (GL.glGetString(GL.GL_RENDERER)))

            # FIXME: This is not available in OpenGL 3.1 and above
            extensionList = unicode(GL.glGetString(GL.GL_EXTENSIONS)).split(" ")
            # sometimes the OpenGL vendors return superfluous spaces at the
            # end of the extension str, this causes the last element be ""
            if extensionList[-1] == "":
                extensionList.pop()
            lines.append("GL extensions:\n    - %s" %
                    (",\n    - ".join(extensionList)))

        else:
            lines.append("Can't query OpenGL info, CEGUI instance hasn't been started!")

        return "\n".join(lines)

class ErrorHandler(object):
    """This class is responsible for all error handling. It only handles exceptions for now.
    """

    # TODO: handle stderr messages as soft errors

    def __init__(self, mainWindow):
        self.mainWindow = mainWindow

    def installExceptionHook(self):
        sys.excepthook = self.excepthook

    def uninstallExceptionHook(self):
        sys.excepthook = sys.__excepthook__

    def excepthook(self, exc_type, exc_message, exc_traceback):
        # to be compatible with as much as possible we have the default
        # except hook argument names in the method signature, these do not
        # conform our guidelines so we alias them here

        excType = exc_type
        excMessage = exc_message
        excTraceback = exc_traceback

        if not self.mainWindow:
            sys.__excepthook__(excType, excMessage, excTraceback)

        else:
            dialog = ExceptionDialog(excType, excMessage, excTraceback, self.mainWindow)
            # we shouldn't use logging.exception here since we are not in an "except" block
            logging.error("Uncaught exception '%s'\n%s", excType, dialog.detailsStr)

            # We don't call the standard excepthook anymore since we output to stderr with the logging module
            #sys.__excepthook__(excType, excMessage, excTraceback)

            result = dialog.exec_()

            # if the dialog was rejected, the user chose to quit the whole app immediately (coward...)
            if result == QDialog.Rejected:
                # stop annoying the user
                self.uninstallExceptionHook()
                # and try to quit as soon as possible
                # we don't care about exception safety/cleaning up at this point
                sys.exit(1)
