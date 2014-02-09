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
from PySide import QtCore

# taken from ElementLib and slightly tweaked for readability
def indent(elem, level = 0, tabImpostor = "    "):
    i = "\n" + level * tabImpostor
    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = i + tabImpostor

        last = None
        for e in elem:
            indent(e, level + 1)
            if not e.tail or not e.tail.strip():
                e.tail = i + tabImpostor

            last = e

        if not last.tail or not last.tail.strip():
            last.tail = i
    else:
        if level and (not elem.tail or not elem.tail.strip()):
            elem.tail = i

class XMLSyntaxHighlighter(QtGui.QSyntaxHighlighter):
    def __init__(self, parent = None):
        super(XMLSyntaxHighlighter, self).__init__(parent)

        self.highlightingRules = []

        # todo: some fail colour highlighting :D please someone change the colours
        keywordFormat = QtGui.QTextCharFormat()
        keywordFormat.setFontWeight(QtGui.QFont.Bold)
        keywordFormat.setForeground(QtCore.Qt.darkCyan)

        for pattern in ["\\b?xml\\b", "/>", ">", "</", "<"]:
            self.highlightingRules.append((QtCore.QRegExp(pattern), keywordFormat))

        elementNameFormat = QtGui.QTextCharFormat()
        elementNameFormat.setFontWeight(QtGui.QFont.Bold)
        elementNameFormat.setForeground(QtCore.Qt.red)

        self.highlightingRules.append((QtCore.QRegExp("\\b[A-Za-z0-9_]+(?=[\s/>])"), elementNameFormat))

        attributeKeyFormat = QtGui.QTextCharFormat()
        attributeKeyFormat.setFontItalic(True)
        attributeKeyFormat.setForeground(QtCore.Qt.blue)
        self.highlightingRules.append((QtCore.QRegExp("\\b[A-Za-z0-9_]+(?=\\=)"), attributeKeyFormat))

    def highlightBlock(self, text):
        for expression, format_ in self.highlightingRules:
            index = expression.indexIn(text)

            while index >= 0:
                length = expression.matchedLength()
                self.setFormat(index, length, format_)

                index = expression.indexIn(text, index + length)

class XMLEditWidget(QtGui.QTextEdit):
    def __init__(self):
        super(XMLEditWidget, self).__init__()

        self.setAcceptRichText(False)
        self.zoomIn()

        self.highlighter = XMLSyntaxHighlighter(self.document())
