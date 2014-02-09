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

"""Implementation of a convenience Qt and CEGUI interaction containment.

Allows you to use CEGUI as if it were a Qt widget.
"""

from PySide import QtCore
from PySide import QtGui
from PySide import QtWebKit

import collections
import logging

from ceed.cegui import qtgraphics

import ceed.ui.ceguidebuginfo
import PyCEGUI

class LogMessageWrapper(object):
    def __init__(self, message, level):
        self.message = message
        self.level = level

    def asTableRow(self):
        bgColour = "transparent"

        if self.level == PyCEGUI.LoggingLevel.Errors:
            stringLevel = "E"
            bgColour = "#ff5f5f"

        elif self.level == PyCEGUI.LoggingLevel.Warnings:
            stringLevel = "W"
            bgColour = "#fff76f"

        else:
            stringLevel = " "
            bgColour = "transparent"

        return "<tr><td style=\"background: %s\">%s</td><td>%s</td></tr>\n" % (bgColour, stringLevel, self.message)

class DebugInfo(QtGui.QDialog):
    """A debugging/info widget about the embedded CEGUI instance"""

    # This will allow us to view logs in Qt in the future

    def __init__(self, containerWidget):
        super(DebugInfo, self).__init__()

        self.setVisible(False)

        self.setWindowFlags(self.windowFlags() | QtCore.Qt.WindowStaysOnTopHint)

        self.containerWidget = containerWidget
        # update FPS and render time very second
        self.boxUpdateInterval = 1

        self.ui = ceed.ui.ceguidebuginfo.Ui_CEGUIWidgetInfo()
        self.ui.setupUi(self)

        self.currentFPSBox = self.findChild(QtGui.QLineEdit, "currentFPSBox")
        self.currentRenderTimeBox = self.findChild(QtGui.QLineEdit, "currentRenderTimeBox")

        self.errors = 0
        self.errorsBox = self.findChild(QtGui.QLineEdit, "errorsBox")

        self.warnings = 0
        self.warningsBox = self.findChild(QtGui.QLineEdit, "warningsBox")

        self.others = 0
        self.othersBox = self.findChild(QtGui.QLineEdit, "othersBox")

        self.logViewArea = self.findChild(QtGui.QWidget, "logViewArea")
        self.logViewAreaLayout = QtGui.QVBoxLayout()

        self.logView = QtWebKit.QWebView()
        self.logViewAreaLayout.addWidget(self.logView)

        self.logViewArea.setLayout(self.logViewAreaLayout)

        self.logMessagesLimit = settings.getEntry("global/cegui_debug_info/log_limit").value
        self.logMessages = collections.deque([])

        self.containerWidget.ceguiInstance.logger.registerSubscriber(self.logEvent)

    def logEvent(self, message, level):
        if level == PyCEGUI.LoggingLevel.Errors:
            self.errors += 1
            self.errorsBox.setText(str(self.errors))

        elif level == PyCEGUI.LoggingLevel.Warnings:
            self.warnings += 1
            self.warningsBox.setText(str(self.warnings))

        else:
            self.others += 1
            self.othersBox.setText(str(self.others))

        # log info using the logging message, allows debug outputs without GUI
        logging.info("CEGUI message: %s", message)

        # remove old messages
        while len(self.logMessages) >= self.logMessagesLimit:
            self.logMessages.popleft()

        self.logMessages.append(LogMessageWrapper(message, level))

    def show(self):
        htmlLog = "\n".join([msg.asTableRow() for msg in self.logMessages])

        self.logView.setHtml("""
<html>
<body>
<style type="text/css">
font-size: 10px;
</style>
<table>
<thead>
<th></th><th>Message</th>
</thead>
<tbody>
""" + htmlLog + """
</tbody>
</table>
</html>""")

        super(DebugInfo, self).show()
        self.updateFPSTick()

    def updateFPSTick(self):
        if not self.isVisible():
            return

        lastRenderDelta = self.containerWidget.ceguiInstance.lastRenderTimeDelta
        if lastRenderDelta <= 0:
            lastRenderDelta = 1

        self.currentFPSBox.setText("%0.6f" % (1.0 / lastRenderDelta))

        QtCore.QTimer.singleShot(500, self.updateFPSTick)

# we import here to avoid circular dependencies (GraphicsView has to be defined at this point)
import ceed.ui.ceguicontainerwidget

class ContainerWidget(QtGui.QWidget):
    """
    This widget is what you should use (alongside your GraphicsScene derived class) to
    put CEGUI inside parts of the editor.

    Provides resolution changes, auto expanding and debug widget
    """

    def __init__(self, ceguiInstance, mainWindow):
        super(ContainerWidget, self).__init__()

        self.ceguiInstance = ceguiInstance
        self.mainWindow = mainWindow

        self.ui = ceed.ui.ceguicontainerwidget.Ui_CEGUIContainerWidget()
        self.ui.setupUi(self)

        self.currentParentWidget = None

        self.debugInfo = DebugInfo(self)
        self.view = self.findChild(qtgraphics.GraphicsView, "view")
        self.ceguiInstance.setGLContextProvider(self.view)
        self.view.setBackgroundRole(QtGui.QPalette.Dark)
        self.view.containerWidget = self

        self.resolutionBox = self.findChild(QtGui.QComboBox, "resolutionBox")
        self.resolutionBox.editTextChanged.connect(self.slot_resolutionBoxChanged)

        self.debugInfoButton = self.findChild(QtGui.QPushButton, "debugInfoButton")
        self.debugInfoButton.clicked.connect(self.slot_debugInfoButton)

    def enableInput(self):
        """If you have already activated this container, you can call this to enable CEGUI input propagation
        (The CEGUI instance associated will get mouse and keyboard events if the widget has focus)
        """

        self.view.injectInput = True

    def disableInput(self):
        """Disables input propagation to CEGUI instance.
        see enableInput
        """

        self.view.injectInput = False

    def makeGLContextCurrent(self):
        """Activates OpenGL context of the associated CEGUI instance"""

        self.view.viewport().makeCurrent()

    def setViewFeatures(self, wheelZoom = False, middleButtonScroll = False, continuousRendering = True):
        """The CEGUI view class has several enable/disable features that are very hard to achieve using
        inheritance/composition so they are kept in the CEGUI view class and its base class.

        This method enables/disables various features, calling it with no parameters switches to default.

        wheelZoom - mouse wheel will zoom in and out
        middleButtonScroll - pressing and dragging with the middle button will cause panning/scrolling
        continuousRendering - CEGUI will render continuously (not just when you tell it to)
        """

        # always zoom to the original 100% when changing view features
        self.view.zoomOriginal()
        self.view.wheelZoomEnabled = wheelZoom

        self.view.middleButtonDragScrollEnabled = middleButtonScroll

        self.view.continuousRendering = continuousRendering

    def activate(self, parentWidget, scene = None):
        """Activates the CEGUI Widget for the given parentWidget (QWidget derived class).
        """

        # sometimes things get called in the opposite order, lets be forgiving and robust!
        if self.currentParentWidget is not None:
            self.deactivate(self.currentParentWidget)

        self.currentParentWidget = parentWidget

        if scene is None:
            scene = qtgraphics.GraphicsScene(self.ceguiInstance)

        self.currentParentWidget.setUpdatesEnabled(False)
        self.view.setScene(scene)
        # make sure the resolution is set right for the given scene
        self.slot_resolutionBoxChanged(self.resolutionBox.currentText())

        if self.currentParentWidget.layout():
            self.currentParentWidget.layout().addWidget(self)
        else:
            self.setParent(self.currentParentWidget)
        self.currentParentWidget.setUpdatesEnabled(True)

        # cause full redraw of the default context to ensure that nothing gets stuck
        PyCEGUI.System.getSingleton().getDefaultGUIContext().markAsDirty()

        # and mark the view as dirty to force Qt to redraw it
        self.view.update()

        # finally, set the OpenGL context for CEGUI as current as other code may rely on it
        self.makeGLContextCurrent()

    def deactivate(self, parentWidget):
        """Deactivates the widget from use in given parentWidget (QWidget derived class)
        see activate

        Note: We strive to be very robust about various differences across platforms (the order in which hide/show events
        are triggered, etc...), so we automatically deactivate if activating with a preexisting parentWidget. That's the
        reason for the parentWidget parameter.
        """

        if self.currentParentWidget != parentWidget:
            return

        self.currentParentWidget.setUpdatesEnabled(False)
        # back to the defaults
        self.setViewFeatures()
        self.view.setScene(None)

        if self.currentParentWidget.layout():
            self.currentParentWidget.layout().removeWidget(self)
        else:
            self.setParent(None)
        self.currentParentWidget.setUpdatesEnabled(True)

        self.currentParentWidget = None

    def updateResolution(self):
        text = self.resolutionBox.currentText()

        if text == "Project default":
            # special case
            project = self.mainWindow.project

            if project is not None:
                text = project.CEGUIDefaultResolution

        res = text.split("x", 1)
        if len(res) == 2:
            try:
                # clamp both to 1 - 4096, should suit 99% of all cases
                width = max(1, min(4096, int(res[0])))
                height = max(1, min(4096, int(res[1])))

                self.ceguiInstance.makeGLContextCurrent()
                self.view.scene().setCEGUIDisplaySize(width, height, lazyUpdate = False)

            except ValueError:
                # ignore invalid literals
                pass

    def slot_resolutionBoxChanged(self, _):
        self.updateResolution()

    def slot_debugInfoButton(self):
        self.debugInfo.show()

from ceed import settings
