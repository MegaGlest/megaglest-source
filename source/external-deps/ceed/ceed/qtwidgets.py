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

"""Contains reusable widgets that I haven't found in Qt for some reason
"""

from PySide import QtCore
from PySide import QtGui

import ceed.ui.widgets.filelineedit
import ceed.ui.widgets.keysequencebuttondialog

class FileLineEdit(QtGui.QWidget):
    ExistingFileMode = 1
    NewFileMode = 2
    ExistingDirectoryMode = 3

    def __init__(self, parent = None):
        super(FileLineEdit, self).__init__(parent)

        self.ui = ceed.ui.widgets.filelineedit.Ui_FileLineEdit()
        self.ui.setupUi(self)

        self.filter = "Any file (*.*)"

        self.lineEdit = self.findChild(QtGui.QLineEdit, "lineEdit")
        self.browseButton = self.findChild(QtGui.QToolButton, "browseButton")

        self.browseButton.pressed.connect(self.slot_browse)

        self.mode = FileLineEdit.ExistingFileMode
        self.directoryMode = False

        self.startDirectory = lambda: ""

    def setText(self, text):
        self.lineEdit.setText(text)

    def text(self):
        return self.lineEdit.text()

    def slot_browse(self):
        path = None

        if self.mode == FileLineEdit.ExistingFileMode:
            path, _ = QtGui.QFileDialog.getOpenFileName(self,
                                                        "Choose a path",
                                                        self.startDirectory(),
                                                        self.filter)

        elif self.mode == FileLineEdit.NewFileMode:
            path, _ = QtGui.QFileDialog.getSaveFileName(self,
                                                        "Choose a path",
                                                        self.startDirectory(),
                                                        self.filter)

        elif self.mode == FileLineEdit.ExistingDirectoryMode:
            path = QtGui.QFileDialog.getExistingDirectory(self,
                                                          "Choose a directory",
                                                          self.startDirectory())

        if path != "":
            self.lineEdit.setText(path)

class ColourButton(QtGui.QPushButton):
    colourChanged = QtCore.Signal(QtGui.QColor)

    colour = property(fset = lambda button, colour: button.setColour(colour),
                      fget = lambda button: button._colour)

    def __init__(self, parent = None):
        super(ColourButton, self).__init__(parent)

        self._colour = None

        self.setAutoFillBackground(True)

        # seems to look better on most systems
        self.setFlat(True)
        self.colour = QtGui.QColor(255, 255, 255, 255)

        self.clicked.connect(self.slot_clicked)

    def setColour(self, colour):
        if colour != self._colour:
            self._colour = colour
            self.setStyleSheet("background-color: rgba(%i, %i, %i, %i)" % (colour.red(), colour.green(), colour.blue(), colour.alpha()))
            self.setText("R: %03i, G: %03i, B: %03i, A: %03i" % (colour.red(), colour.green(), colour.blue(), colour.alpha()))
            self.colourChanged.emit(colour)

    def slot_clicked(self):
        colour = QtGui.QColorDialog.getColor(self.colour, self, "", QtGui.QColorDialog.ColorDialogOption.ShowAlphaChannel|QtGui.QColorDialog.ColorDialogOption.DontUseNativeDialog)

        if colour.isValid():
            self.colour = colour

# this depends on ColourButton, can't be moved upwards
import ceed.ui.widgets.penbuttondialog

class PenButton(QtGui.QPushButton):
    class Dialog(QtGui.QDialog):
        def __init__(self, parent = None):
            super(PenButton.Dialog, self).__init__(parent)

            self.ui = ceed.ui.widgets.penbuttondialog.Ui_PenButtonDialog()
            self.ui.setupUi(self)

            self.colour = self.findChild(ColourButton, "colour")
            self.lineStyle = self.findChild(QtGui.QComboBox, "lineStyle")
            self.lineWidth = self.findChild(QtGui.QDoubleSpinBox, "lineWidth")

        def setPen(self, pen):
            self.colour.setColour(pen.color())

            if pen.style() == QtCore.Qt.SolidLine:
                self.lineStyle.setCurrentIndex(0)
            elif pen.style() == QtCore.Qt.DashLine:
                self.lineStyle.setCurrentIndex(1)
            elif pen.style() == QtCore.Qt.DotLine:
                self.lineStyle.setCurrentIndex(2)
            elif pen.style() == QtCore.Qt.DashDotLine:
                self.lineStyle.setCurrentIndex(3)
            elif pen.style() == QtCore.Qt.DashDotDotLine:
                self.lineStyle.setCurrentIndex(4)
            else:
                # unknown line style
                assert(False)

            self.lineWidth.setValue(pen.widthF())

        def getPen(self):
            ret = QtGui.QPen()

            ret.setColor(self.colour.colour)

            style = QtCore.Qt.SolidLine
            if self.lineStyle.currentIndex() == 0:
                style = QtCore.Qt.SolidLine
            elif self.lineStyle.currentIndex() == 1:
                style = QtCore.Qt.DashLine
            elif self.lineStyle.currentIndex() == 2:
                style = QtCore.Qt.DotLine
            elif self.lineStyle.currentIndex() == 3:
                style = QtCore.Qt.DashDotLine
            elif self.lineStyle.currentIndex() == 4:
                style = QtCore.Qt.DashDotDotLine
            else:
                # unknown combobox index
                assert(False)

            ret.setStyle(style)
            ret.setWidth(self.lineWidth.value())

            return ret

    # TODO: This is not implemented at all pretty much
    penChanged = QtCore.Signal(QtGui.QPen)

    pen = property(fset = lambda button, pen: button.setPen(pen),
                   fget = lambda button: button._pen)

    def __init__(self, parent = None):
        super(PenButton, self).__init__(parent)

        self._pen = None

        self.setAutoFillBackground(True)
        # seems to look better on most systems
        self.setFlat(True)
        self.pen = QtGui.QPen()

        self.clicked.connect(self.slot_clicked)

    def setPen(self, pen):
        if pen != self._pen:
            self._pen = pen

            lineStyleStr = ""
            if pen.style() == QtCore.Qt.SolidLine:
                lineStyleStr = "solid"
            elif pen.style() == QtCore.Qt.DashLine:
                lineStyleStr = "dash"
            elif pen.style() == QtCore.Qt.DotLine:
                lineStyleStr = "dot"
            elif pen.style() == QtCore.Qt.DashDotLine:
                lineStyleStr = "dash dot"
            elif pen.style() == QtCore.Qt.DashDotDotLine:
                lineStyleStr = "dash dot dot"
            elif pen.style() == QtCore.Qt.CustomDashLine:
                lineStyleStr = "custom dash"
            else:
                raise RuntimeError("Unknown pen line style!")

            """
            capStyleStr = ""
            if pen.capStyle() == QtCore.Qt.FlatCap:
                capStyleStr = "flat"
            elif pen.capStyle() == QtCore.Qt.RoundCap:
                capStyleStr = "round"
            elif pen.capStyle() == QtCore.Qt.SquareCap:
                capStyleStr = "square"
            else:
                raise RuntimeError("Unknown pen cap style!")

            joinStyleStr = ""
            if pen.joinStyle() == QtCore.Qt.MiterJoin:
                joinStyleStr = "miter"
            elif pen.joinStyle() == QtCore.Qt.BevelJoin:
                joinStyleStr = "bevel"
            elif pen.joinStyle() == QtCore.Qt.RoundJoin:
                joinStyleStr = "round"
            elif pen.joinStyle() == QtCore.Qt.SvgMiterJoin:
                joinStyleStr = "svg miter"
            else:
                raise RuntimeError("Unknown pen join style!")
            """

            self.setText("line style: %s, width: %g" % (lineStyleStr, pen.widthF()))
            colour = pen.color()
            #self.setStyleSheet("background-color: rgba(%i, %i, %i, %i)" % (colour.red(), colour.green(), colour.blue(), colour.alpha()))

            self.penChanged.emit(pen)

    def slot_clicked(self):
        dialog = PenButton.Dialog(self)
        dialog.setPen(self.pen)

        if dialog.exec_() == QtGui.QDialog.Accepted:
            self.pen = dialog.getPen()

class KeySequenceButton(QtGui.QPushButton):
    class Dialog(QtGui.QDialog):
        def __init__(self, parent = None):
            super(KeySequenceButton.Dialog, self).__init__(parent)

            self.setFocusPolicy(QtCore.Qt.StrongFocus)

            self.ui = ceed.ui.widgets.keysequencebuttondialog.Ui_KeySequenceButtonDialog()
            self.ui.setupUi(self)

            self.keySequence = QtGui.QKeySequence()
            self.keyCombination = self.findChild(QtGui.QLineEdit, "keyCombination")

        def setKeySequence(self, keySequence):
            if keySequence is None:
                keySequence = QtGui.QKeySequence()

            self.keySequence = keySequence
            self.keyCombination.setText(self.keySequence.toString())

        def keyPressEvent(self, event):
            self.setKeySequence(QtGui.QKeySequence(event.modifiers() | event.key()))

    keySequenceChanged = QtCore.Signal(QtGui.QKeySequence)

    keySequence = property(fset = lambda button, keySequence: button.setKeySequence(keySequence),
                           fget = lambda button: button._keySequence)

    def __init__(self, parent = None):
        super(KeySequenceButton, self).__init__(parent)

        self._keySequence = None

        self.setAutoFillBackground(True)
        self.keySequence = QtGui.QKeySequence()

        self.clicked.connect(self.slot_clicked)

    def setKeySequence(self, keySequence):
        if keySequence != self._keySequence:
            self._keySequence = keySequence
            self.setText(keySequence.toString())
            self.keySequenceChanged.emit(keySequence)

    def slot_clicked(self):
        dialog = KeySequenceButton.Dialog(self)
        dialog.setKeySequence(self.keySequence)

        if dialog.exec_() == QtGui.QDialog.Accepted:
            self.keySequence = dialog.keySequence

class LineEditWithClearButton(QtGui.QLineEdit):
    """A QLineEdit with an inline clear button.

    Hitting Escape in the line edit clears it.

    Based on http://labs.qt.nokia.com/2007/06/06/lineedit-with-a-clear-button/
    """

    def __init__(self, parent=None):
        super(LineEditWithClearButton, self).__init__(parent)

        btn = self.button = QtGui.QToolButton(self)
        icon = QtGui.QPixmap("icons/widgets/edit-clear.png")
        btn.setIcon(icon)
        btn.setIconSize(icon.size())
        btn.setCursor(QtCore.Qt.ArrowCursor)
        btn.setStyleSheet("QToolButton { border: none; padding: 0px; }")
        btn.hide()

        btn.clicked.connect(self.clear)
        self.textChanged.connect(self.updateCloseButton)

        clearAction = QtGui.QAction(self)
        clearAction.setShortcut(QtGui.QKeySequence("Esc"))
        clearAction.setShortcutContext(QtCore.Qt.ShortcutContext.WidgetShortcut)
        clearAction.triggered.connect(self.clear)
        self.addAction(clearAction)

        frameWidth = self.style().pixelMetric(QtGui.QStyle.PM_DefaultFrameWidth)
        self.setStyleSheet("QLineEdit { padding-right: %ipx; }" % (btn.sizeHint().width() + frameWidth + 1))

        minSizeHint = self.minimumSizeHint()
        self.setMinimumSize(max(minSizeHint.width(), btn.sizeHint().width() + frameWidth * 2 + 2),
                            max(minSizeHint.height(), btn.sizeHint().height() + frameWidth * 2 + 2))

    def resizeEvent(self, event):
        sz = self.button.sizeHint()
        frameWidth = self.style().pixelMetric(QtGui.QStyle.PM_DefaultFrameWidth)
        self.button.move(self.rect().right() - frameWidth - sz.width(),
                         (self.rect().bottom() + 1 - sz.height()) / 2)

    def updateCloseButton(self, text):
        self.button.setVisible(not not text)

def getCheckerboardBrush(halfWidth = 5, halfHeight = 5,
                         firstColour = QtGui.QColor(QtCore.Qt.darkGray),
                         secondColour = QtGui.QColor(QtCore.Qt.gray)):
    """Small helper function that generates a brush usually seen in graphics
    editing tools. The checkerboard brush that draws background seen when
    edited images are transparent
    """

    # disallow too large half sizes to prevent crashes in QPainter
    # and slowness in general
    halfWidth = min(halfWidth, 1000)
    halfHeight = min(halfHeight, 1000)

    ret = QtGui.QBrush()
    texture = QtGui.QPixmap(2 * halfWidth, 2 * halfHeight)
    painter = QtGui.QPainter(texture)
    painter.fillRect(0, 0, halfWidth, halfHeight, firstColour)
    painter.fillRect(halfWidth, halfHeight, halfWidth, halfHeight, firstColour)
    painter.fillRect(halfWidth, 0, halfWidth, halfHeight, secondColour)
    painter.fillRect(0, halfHeight, halfWidth, halfHeight, secondColour)
    painter.end()
    ret.setTexture(texture)

    return ret
