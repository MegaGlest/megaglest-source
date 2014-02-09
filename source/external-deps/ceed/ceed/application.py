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

from PySide.QtCore import Qt, QSettings
from PySide.QtGui import QApplication, QSplashScreen, QPixmap

import logging

from ceed import version

class SplashScreen(QSplashScreen):
    def __init__(self):
        super(SplashScreen, self).__init__(QPixmap("images/splashscreen.png"))

        self.setWindowModality(Qt.ApplicationModal)
        self.setWindowFlags(Qt.SplashScreen | Qt.WindowStaysOnTopHint)
        self.showMessage("version: %s" % (version.CEED), Qt.AlignTop | Qt.AlignRight, Qt.GlobalColor.white)

class Application(QApplication):
    """The central application class
    """

    def __init__(self, argv, debug = False):
        super(Application, self).__init__(argv)

        logging.basicConfig()

        if debug:
            # set debug logging
            logging.getLogger().setLevel(logging.DEBUG)

        if version.DEVELOPER_MODE:
            # print info about developer's mode to possibly prevent it being
            # forgotten about when releasing
            logging.info("Developer's mode enabled - recompiling all .ui files...")

            # in case we are in the developer's mode,
            # lets compile all UI files to ensure they are up to date
            from ceed import compileuifiles
            compileuifiles.main()

            logging.debug("All .ui files recompiled!")

        from ceed import settings
        self.qsettings = QSettings("CEGUI", "CEED")
        self.settings = settings.Settings(self.qsettings)
        # download all values from the persistence store
        self.settings.download()

        showSplash = settings.getEntry("global/app/show_splash").value
        if showSplash:
            self.splash = SplashScreen()
            self.splash.show()

            # this ensures that the splash screen is shown on all platforms
            self.processEvents()

        self.setOrganizationName("CEGUI")
        self.setOrganizationDomain("cegui.org.uk")
        self.setApplicationName("CEED - CEGUI editor")
        self.setApplicationVersion(version.CEED)

        # (we potentially have to compile all UI files first before this is imported,
        # otherwise out of date compiled .py layouts might be used!)
        from ceed import mainwindow

        self.mainWindow = mainwindow.MainWindow(self)
        self.mainWindow.show()
        self.mainWindow.raise_()
        if showSplash:
            self.splash.finish(self.mainWindow)

        # import error after UI files have been recompiled
        # - Truncate exception log, if it exists.
        from ceed import error

        self.errorHandler = error.ErrorHandler(self.mainWindow)
        self.errorHandler.installExceptionHook()
