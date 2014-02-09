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

import PyCEGUI

from ceed.editors import multi
from ceed import cegui
from ceed.editors.animation_list import timeline
from ceed.editors.animation_list import undo

import ceed.ui.editors.animation_list.animationlistdockwidget
import ceed.ui.editors.animation_list.keyframepropertiesdockwidget
import ceed.ui.editors.animation_list.visualediting

from ceed import propertysetinspector
from ceed import propertytree

from xml.etree import cElementTree as ElementTree
from ceed import xmledit

class AnimationListDockWidget(QtGui.QDockWidget):
    """Lists animations in the currently opened animation list XML
    """

    def __init__(self, visual):
        super(AnimationListDockWidget, self).__init__()

        self.visual = visual

        self.ui = ceed.ui.editors.animation_list.animationlistdockwidget.Ui_AnimationListDockWidget()
        self.ui.setupUi(self)

        self.list = self.findChild(QtGui.QListWidget, "list")
        self.list.currentItemChanged.connect(self.slot_currentItemChanged)

    def fillWithAnimations(self, animationWrappers):
        self.list.clear()

        for wrapper in animationWrappers:
            self.list.addItem(wrapper.realDefinitionName)

    def slot_currentItemChanged(self, newItem, oldItem):
        newName = newItem.text() if newItem else None
        oldName = oldItem.text() if oldItem else None

        cmd = undo.ChangeCurrentAnimationDefinitionCommand(self.visual, newName, oldName)
        self.visual.tabbedEditor.undoStack.push(cmd)

class PropertiesDockWidget(QtGui.QDockWidget):
    """Lists and allows editing of properties at current position of the timeline
    """

    def __init__(self, visual):
        super(PropertiesDockWidget, self).__init__()
        self.setObjectName("PropertiesDockWidget")
        self.visual = visual

        self.setWindowTitle("Property State")
        # Make the dock take as much space as it can vertically
        self.setSizePolicy(QtGui.QSizePolicy.Preferred, QtGui.QSizePolicy.Maximum)

        self.inspector = propertysetinspector.PropertyInspectorWidget()
        self.inspector.ptree.setupRegistry(propertytree.editors.PropertyEditorRegistry(True))

        pmap = mainwindow.MainWindow.instance.project.propertyMap
        self.inspector.setPropertyManager(propertysetinspector.CEGUIPropertyManager(pmap))

        self.setWidget(self.inspector)

class KeyFramePropertiesDockWidget(QtGui.QDockWidget):
    """Lists and allows editing of properties at current position of the timeline
    """

    def __init__(self, visual):
        super(KeyFramePropertiesDockWidget, self).__init__()

        self.setObjectName("KeyFramePropertiesDockWidget")
        self.visual = visual

        self.ui = ceed.ui.editors.animation_list.keyframepropertiesdockwidget.Ui_KeyFramePropertiesDockWidget()
        self.ui.setupUi(self)

        self.timePositionSpinBox = self.findChild(QtGui.QDoubleSpinBox, "timePositionSpinBox")
        self.progressionComboBox = self.findChild(QtGui.QComboBox, "progressionComboBox")

        self.fixedValueRadioButton = self.findChild(QtGui.QRadioButton, "fixedValueRadioButton")
        self.propertyRadioButton = self.findChild(QtGui.QRadioButton, "propertyRadioButton")

        self.fixedValueLineEdit = self.findChild(QtGui.QLineEdit, "fixedValueLineEdit")
        self.sourcePropertyComboBox = self.findChild(QtGui.QComboBox, "sourcePropertyComboBox")

        self.inspectedKeyFrame = None
        self.setInspectedKeyFrame(None)

    def setInspectedKeyFrame(self, keyFrame):
        self.setDisabled(keyFrame is None)
        self.inspectedKeyFrame = keyFrame

        self.syncWithKeyFrame()

    def syncSourcePropertyComboBox(self):
        self.sourcePropertyComboBox.clear()

        # TODO
        pass

    def syncWithKeyFrame(self):
        self.syncSourcePropertyComboBox()

        if self.inspectedKeyFrame is None:
            self.timePositionSpinBox.setRange(0, 1)
            self.timePositionSpinBox.setValue(0)

            self.progressionComboBox.setCurrentIndex(self.progressionComboBox.findText("Linear"))

            self.fixedValueRadioButton.setChecked(True)
            self.propertyRadioButton.setChecked(False)

            self.fixedValueLineEdit.setText("")
            self.sourcePropertyComboBox.setCurrentIndex(-1)

        else:
            assert(self.visual.currentAnimation is not None)
            self.timePositionSpinBox.setRange(0, self.visual.currentAnimation.getDuration())
            self.timePositionSpinBox.setValue(self.inspectedKeyFrame.getPosition())

            progression = self.inspectedKeyFrame.getProgression()
            if progression == PyCEGUI.KeyFrame.Progression.P_Linear:
                self.progressionComboBox.setCurrentIndex(self.progressionComboBox.findText("Linear"))
            elif progression == PyCEGUI.KeyFrame.Progression.P_QuadraticAccelerating:
                self.progressionComboBox.setCurrentIndex(self.progressionComboBox.findText("Quadratic Accelerating"))
            elif progression == PyCEGUI.KeyFrame.Progression.P_QuadraticDecelerating:
                self.progressionComboBox.setCurrentIndex(self.progressionComboBox.findText("Quadratic Decelerating"))
            elif progression == PyCEGUI.KeyFrame.Progression.P_Discrete:
                self.progressionComboBox.setCurrentIndex(self.progressionComboBox.findText("Discrete"))
            else:
                raise RuntimeError("Can't recognise progression of inspected keyframe")

            if self.inspectedKeyFrame.getSourceProperty() == "":
                self.fixedValueRadioButton.setChecked(True)
                self.propertyRadioButton.setChecked(False)

                self.fixedValueLineEdit.setText(self.inspectedKeyFrame.getValue())

                self.sourcePropertyComboBox.setCurrentIndex(-1)

            else:
                self.fixedValueRadioButton.setChecked(False)
                self.propertyRadioButton.setChecked(True)

                self.fixedValueLineEdit.setText("")

                self.sourcePropertyComboBox.lineEdit().setText(self.inspectedKeyFrame.getSourceProperty())

class TimelineGraphicsView(QtGui.QGraphicsView):
    def __init__(self, parent = None):
        super(TimelineGraphicsView, self).__init__(parent)

        self.dockWidget = None

    def mouseReleaseEvent(self, event):
        super(TimelineGraphicsView, self).mouseReleaseEvent(event)

        self.dockWidget.timeline.notifyMouseReleased()

class TimelineDockWidget(QtGui.QDockWidget):
    """Shows a timeline of currently selected animation (from the animation list dock widget)
    """

    def __init__(self, visual):
        super(TimelineDockWidget, self).__init__()

        self.visual = visual

        self.ui = ceed.ui.editors.animation_list.timelinedockwidget.Ui_TimelineDockWidget()
        self.ui.setupUi(self)

        self.zoomLevel = 1

        self.view = self.findChild(TimelineGraphicsView, "view")
        self.view.dockWidget = self

        self.scene = QtGui.QGraphicsScene()
        self.timeline = timeline.AnimationTimeline()
        self.timeline.keyFramesMoved.connect(self.slot_keyFramesMoved)
        self.scene.addItem(self.timeline)
        self.view.setScene(self.scene)

        self.playButton = self.findChild(QtGui.QPushButton, "playButton")
        self.playButton.clicked.connect(lambda: self.timeline.play())
        self.pauseButton = self.findChild(QtGui.QPushButton, "pauseButton")
        self.pauseButton.clicked.connect(lambda: self.timeline.pause())
        self.stopButton = self.findChild(QtGui.QPushButton, "stopButton")
        self.stopButton.clicked.connect(lambda: self.timeline.stop())

    def zoomIn(self):
        self.view.scale(2, 1)
        self.zoomLevel *= 2.0

        self.timeline.notifyZoomChanged(self.zoomLevel)

        return True

    def zoomOut(self):
        self.view.scale(0.5, 1)
        self.zoomLevel /= 2.0

        self.timeline.notifyZoomChanged(self.zoomLevel)

        return True

    def zoomReset(self):
        self.view.scale(1.0 / self.zoomLevel, 1)
        self.zoomLevel = 1.0

        self.timeline.notifyZoomChanged(self.zoomLevel)

        return True

    def slot_keyFramesMoved(self, movedKeyFrames):
        cmd = undo.MoveKeyFramesCommand(self.visual, movedKeyFrames)
        self.visual.tabbedEditor.undoStack.push(cmd)

class EditingScene(cegui.widgethelpers.GraphicsScene):
    """This scene is used just to preview the animation in the state user selects.
    """

    def __init__(self, visual):
        super(EditingScene, self).__init__(mainwindow.MainWindow.instance.ceguiInstance)

        self.visual = visual

class AnimationDefinitionWrapper(object):
    """Represents one of the animations in the list, takes care of loading and saving it from/to XML element
    """

    def __init__(self, visual):
        self.visual = visual
        self.animationDefinition = None
        # the real name that should be saved when saving to a file
        self.realDefinitionName = ""
        # the fake name that will never clash with anything
        self.fakeDefinitionName = ""

    def loadFromElement(self, element):
        self.realDefinitionName = element.get("name", "")
        self.fakeDefinitionName = self.visual.generateFakeAnimationDefinitionName()

        element.set("name", self.fakeDefinitionName)

        wrapperElement = ElementTree.Element("Animations")
        wrapperElement.append(element)

        fakeWrapperCode = ElementTree.tostring(wrapperElement, "utf-8")
        # tidy up what we abused
        element.set("name", self.realDefinitionName)

        PyCEGUI.AnimationManager.getSingleton().loadAnimationsFromString(fakeWrapperCode)

        self.animationDefinition = PyCEGUI.AnimationManager.getSingleton().getAnimation(self.fakeDefinitionName)

    def saveToElement(self):
        ceguiCode = PyCEGUI.AnimationManager.getSingleton().getAnimationDefinitionAsString(self.animationDefinition)

        ret = ElementTree.fromstring(ceguiCode)
        ret.set("name", self.realDefinitionName)

        return ret

class VisualEditing(QtGui.QWidget, multi.EditMode):
    """This is the default visual editing mode for animation lists

    see editors.multi.EditMode
    """

    fakeAnimationDefinitionNameSuffix = 1

    def __init__(self, tabbedEditor):
        super(VisualEditing, self).__init__()

        self.ui = ceed.ui.editors.animation_list.visualediting.Ui_VisualEditing()
        self.ui.setupUi(self)

        self.tabbedEditor = tabbedEditor

        # the check and return is there because we require a project but are
        # constructed before the "project is opened" check is performed
        # if rootPreviewWidget is None we will fail later, however that
        # won't happen since it will be checked after construction
        if mainwindow.MainWindow.instance.project is None:
            return

        self.animationListDockWidget = AnimationListDockWidget(self)
        self.propertiesDockWidget = PropertiesDockWidget(self)
        self.keyFramePropertiesDockWidget = KeyFramePropertiesDockWidget(self)

        self.timelineDockWidget = TimelineDockWidget(self)
        self.timelineDockWidget.timeline.timePositionChanged.connect(self.slot_timePositionChanged)
        self.timelineDockWidget.scene.selectionChanged.connect(self.slot_keyFrameSelectionChanged)

        self.fakeAnimationDefinitionNameSuffix = 1
        self.currentAnimation = None
        self.currentAnimationInstance = None
        self.currentPreviewWidget = None

        self.setCurrentAnimation(None)

        self.rootPreviewWidget = PyCEGUI.WindowManager.getSingleton().createWindow("DefaultWindow", "RootPreviewWidget")

        self.previewWidgetSelector = self.findChild(QtGui.QComboBox, "previewWidgetSelector")
        self.previewWidgetSelector.currentIndexChanged.connect(self.slot_previewWidgetSelectorChanged)
        self.populateWidgetSelector()
        self.ceguiPreview = self.findChild(QtGui.QWidget, "ceguiPreview")

        layout = QtGui.QVBoxLayout(self.ceguiPreview)
        layout.setContentsMargins(0, 0, 0, 0)
        self.ceguiPreview.setLayout(layout)

        self.scene = EditingScene(self)

    def generateFakeAnimationDefinitionName(self):
        VisualEditing.fakeAnimationDefinitionNameSuffix += 1

        return "CEED_InternalAnimationDefinition_%i" % (VisualEditing.fakeAnimationDefinitionNameSuffix)

    def showEvent(self, event):
        mainwindow.MainWindow.instance.ceguiContainerWidget.activate(self.ceguiPreview, self.scene)
        mainwindow.MainWindow.instance.ceguiContainerWidget.setViewFeatures(wheelZoom = True,
                                                                            middleButtonScroll = True,
                                                                            continuousRendering = True)

        PyCEGUI.System.getSingleton().getDefaultGUIContext().setRootWindow(self.rootPreviewWidget)

        self.animationListDockWidget.setEnabled(True)
        self.timelineDockWidget.setEnabled(True)

        super(VisualEditing, self).showEvent(event)

    def hideEvent(self, event):
        self.animationListDockWidget.setEnabled(False)
        self.timelineDockWidget.setEnabled(False)

        mainwindow.MainWindow.instance.ceguiContainerWidget.deactivate(self.ceguiPreview)

        super(VisualEditing, self).hideEvent(event)

    def synchInstanceAndWidget(self):
        if self.currentAnimationInstance is None:
            self.propertiesDockWidget.inspector.setPropertySets([])
            return

        self.currentAnimationInstance.setTargetWindow(None)

        if self.currentPreviewWidget is None:
            self.propertiesDockWidget.inspector.setPropertySets([])
            return

        self.currentAnimationInstance.setTargetWindow(self.currentPreviewWidget)
        self.currentAnimationInstance.apply()

        if self.propertiesDockWidget.inspector.getPropertySets() != [self.currentPreviewWidget]:
            self.propertiesDockWidget.inspector.setPropertySets([self.currentPreviewWidget])
        else:
            self.propertiesDockWidget.inspector.propertyManager.updateAllValues([self.currentPreviewWidget])

    def loadFromElement(self, rootElement):
        self.animationWrappers = {}

        for animation in rootElement.findall("AnimationDefinition"):
            animationWrapper = AnimationDefinitionWrapper(self)
            animationWrapper.loadFromElement(animation)
            self.animationWrappers[animationWrapper.realDefinitionName] = animationWrapper

        self.animationListDockWidget.fillWithAnimations(self.animationWrappers.itervalues())

    def saveToElement(self):
        root = ElementTree.Element("Animations")

        for animationWrapper in self.animationWrappers.itervalues():
            element = animationWrapper.saveToElement()
            root.append(element)

        return root

    def generateNativeData(self):
        element = self.saveToElement()
        xmledit.indent(element)

        return ElementTree.tostring(element, "utf-8")

    def setCurrentAnimation(self, animation):
        """Set animation we want to edit"""

        self.currentAnimation = animation

        if self.currentAnimationInstance is not None:
            PyCEGUI.AnimationManager.getSingleton().destroyAnimationInstance(self.currentAnimationInstance)
            self.currentAnimationInstance = None

        if self.currentAnimation is not None:
            self.currentAnimationInstance = PyCEGUI.AnimationManager.getSingleton().instantiateAnimation(self.currentAnimation)

        self.timelineDockWidget.timeline.setAnimation(self.currentAnimation)

        self.synchInstanceAndWidget()

    def setCurrentAnimationWrapper(self, animationWrapper):
        self.setCurrentAnimation(animationWrapper.animationDefinition)

    def getAnimationWrapper(self, name):
        return self.animationWrappers[name]

    def getAffectorOfCurrentAnimation(self, affectorIdx):
        return self.currentAnimation.getAffectorAtIdx(affectorIdx)

    def getKeyFrameOfCurrentAnimation(self, affectorIdx, keyFrameIdx):
        return self.getAffectorOfCurrentAnimation(affectorIdx).getKeyFrameAtIdx(keyFrameIdx)

    def setPreviewWidget(self, widgetType):
        if self.currentPreviewWidget is not None:
            self.rootPreviewWidget.removeChild(self.currentPreviewWidget)
            PyCEGUI.WindowManager.getSingleton().destroyWindow(self.currentPreviewWidget)
            self.currentPreviewWidget = None

        if widgetType != "":
            try:
                self.currentPreviewWidget = PyCEGUI.WindowManager.getSingleton().createWindow(widgetType, "PreviewWidget")

            except Exception as ex:
                QtGui.QMessageBox.warning(self, "Unable to comply!",
                                          "Your selected preview widget of type '%s' can't be used as a preview widget, error occured ('%s')." % (widgetType, ex))
                self.currentPreviewWidget = None
                self.synchInstanceAndWidget()
                return

            self.currentPreviewWidget.setPosition(PyCEGUI.UVector2(PyCEGUI.UDim(0.25, 0), PyCEGUI.UDim(0.25, 0)))
            self.currentPreviewWidget.setSize(PyCEGUI.USize(PyCEGUI.UDim(0.5, 0), PyCEGUI.UDim(0.5, 0)))
            self.rootPreviewWidget.addChild(self.currentPreviewWidget)

        self.synchInstanceAndWidget()

    def populateWidgetSelector(self):
        self.previewWidgetSelector.clear()
        self.previewWidgetSelector.addItem("") # no preview
        self.previewWidgetSelector.setCurrentIndex(0) # select no preview

        widgetsBySkin = mainwindow.MainWindow.instance.ceguiInstance.getAvailableWidgetsBySkin()
        for skin, widgets in widgetsBySkin.iteritems():
            if skin == "__no_skin__":
                # pointless to preview animations with invisible widgets
                continue

            for widget in widgets:
                widgetType = "%s/%s" % (skin, widget)

                self.previewWidgetSelector.addItem(widgetType)

    def slot_previewWidgetSelectorChanged(self, index):
        widgetType = self.previewWidgetSelector.itemText(index)

        self.setPreviewWidget(widgetType)

    def slot_timePositionChanged(self, oldPosition, newPosition):
        # there is intentionally no undo/redo for this (it doesn't change content or context)

        if self.currentAnimationInstance is not None:
            self.currentAnimationInstance.setPosition(newPosition)

        self.synchInstanceAndWidget()

    def slot_keyFrameSelectionChanged(self):
        selectedKeyFrames = []

        for item in self.timelineDockWidget.scene.selectedItems():
            if isinstance(item, timeline.AffectorTimelineKeyFrame):
                selectedKeyFrames.append(item.keyFrame)

        self.keyFramePropertiesDockWidget.setInspectedKeyFrame(selectedKeyFrames[0] if len(selectedKeyFrames) == 1 else None)

    def zoomIn(self):
        return self.timelineDockWidget.zoomIn()

    def zoomOut(self):
        return self.timelineDockWidget.zoomOut()

    def zoomReset(self):
        return self.timelineDockWidget.zoomReset()

import ceed.ui.editors.animation_list.timelinedockwidget
from ceed import mainwindow
