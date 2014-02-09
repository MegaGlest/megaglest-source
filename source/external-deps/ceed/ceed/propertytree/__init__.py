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

"""Provides a way to build, categorise, visualise and edit a list of properties.

properties -- Built-in properties, representing one simple value.
compositeproperties -- Built-in composite properties based on several simple properties.
editors -- Editors, each supporting a specific set of property types.
ui -- The Qt GUI widget and its supporting classes.

Example::

    class TestDock(QDockWidget):
        def __init__(self):
            super(TestDock, self).__init__()

            self.setWindowTitle("Test Dock")
            self.setMinimumWidth(400)

            self.view = PropertyTreeWidget()
            self.view.setupRegistry(PropertyEditorRegistry(True))

            # root widget and layout
            contentsWidget = QWidget()
            contentsLayout = QVBoxLayout()
            contentsWidget.setLayout(contentsLayout)
            margins = contentsLayout.contentsMargins()
            margins.setTop(0)
            contentsLayout.setContentsMargins(margins)

            contentsLayout.addWidget(self.view)
            self.setWidget(contentsWidget)

            def test():
                self.colourProp.setValue(OrderedDict([
                                                     ("Red", 160),
                                                     ("Green", 255),
                                                     ("Blue", 160),
                                                     ("Alpha", 255)
                                                     ]))

            testButton = QPushButton()
            testButton.setText("Test")
            testButton.clicked.connect(test)
            contentsLayout.addWidget(testButton)

            self.setup()

        def setup(self):

            colourProp = DictionaryProperty(
                                            name = "Colour",
                                            value = OrderedDict([
                                                                 ("Red", 160),
                                                                 ("Green", 255),
                                                                 ("Blue", 160),
                                                                 ("Alpha", Property(name="Alpha", value=0.5, defaultValue=0.5)),
                                                                 ("You_can_edit", "The whole dictionary string"),
                                                                 ("and_even_add", "or remove members")
                                                                 ]),
                                            editorOptions = {"instantApply":False, "numeric": {"min":0, "max":255, "step": 8}},
                                            strReprMode = DictionaryProperty.StringRepresentationMode.EditKeysAndValues
                                            )
            xyzProp = DictionaryProperty(
                                         name = "XYZ",
                                         value = OrderedDict([
                                                             ("X", 0),
                                                             ("Y", 0),
                                                             ("Z", 0),
                                                             ("Only_Values", "can be edited here.")
                                                             ]),
                                         strReprMode = DictionaryProperty.StringRepresentationMode.EditValuesRestrictTypes
                                        )

            props = [
                    Property("stringProperty", "Hello", "Hello", "Default Category", "This is a string property"),
                    Property(name = "fourHexChars", value = "0xDEAD", defaultValue="0xBEEF", editorOptions = {
                                                                             "string": {"inputMask":"\\0\\xHHHH;_"}
                                                                             }),
                    Property(name = "fourDigits", value = "1234", defaultValue="1337", editorOptions = {
                                                                             "string": {"inputMask":"9999;_"}
                                                                             }),
                    DictionaryProperty("dictionary", OrderedDict([
                                                                  ("X", 0),
                                                                  ("Y", 0),
                                                                  ("Width", 50),
                                                                  ("Height", 50),
                                                                  ("Colour", colourProp)
                                                                  ]),
                                       readOnly=False),
                    xyzProp
                    ]
            categories = PropertyCategory.categorisePropertyList(props)
            # test: add the property to another category too!
            categories["Default Category"].properties["dictionary"] = props[1]
            # load
            self.view.load(categories)
            self.props = props
            self.colourProp = colourProp

"""

__all__ = ["properties", "compositeproperties", "editors", "ui"]
