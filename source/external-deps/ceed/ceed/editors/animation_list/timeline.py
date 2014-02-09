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

import PyCEGUI

import time

# default amount of pixels per second for 100% zoom
pixelsPerSecond = 1000

class TimecodeLabel(QtGui.QGraphicsRectItem):
    """Simply displays time position labels depending on the zoom level
    """

    range = property(lambda self: self._getRange(),
                     lambda self, value: self._setRange(value))

    def __init__(self, parentItem = None):
        super(TimecodeLabel, self).__init__(parentItem)

        self.range = 1

        self.setFlags(QtGui.QGraphicsItem.ItemIsFocusable)

    def _setRange(self, range):
        assert(range > 0)

        self.setRect(QtCore.QRectF(0, 0, range * pixelsPerSecond, 15))

    def _getRange(self):
        return self.rect().width() / pixelsPerSecond

    def paint(self, painter, option, widget = None):
        #super(TimecodeLabel, self).paint(painter, option, widget)

        painter.save()

        try:
            transform = painter.worldTransform()
            xScale = transform.m11()

            assert(xScale > 0)

            minLabelWidth = 50
            maxLabelsPerSecond = pixelsPerSecond * xScale / minLabelWidth

            timeLabels = {}

            ## {{{ http://code.activestate.com/recipes/66472/ (r1)
            def frange(start, end=None, inc=None):
                "A range function, that does accept float increments..."

                if end == None:
                    end = start + 0.0
                    start = 0.0

                if inc == None:
                    inc = 1.0

                L = []
                while 1:
                    next = start + len(L) * inc
                    if inc > 0 and next >= end:
                        break
                    elif inc < 0 and next <= end:
                        break
                    L.append(next)

                return L
            ## end of http://code.activestate.com/recipes/66472/ }}}

            # always draw start and end
            timeLabels[0] = True
            timeLabels[self.range] = True

            if maxLabelsPerSecond >= 0.1:
                for i in frange(0, self.range, 10):
                    # 0, 10, 20, ...
                    timeLabels[i] = True

            if maxLabelsPerSecond >= 0.2:
                for i in frange(0, self.range, 5):
                    # 0, 5, 10, ...
                    timeLabels[i] = True

            if maxLabelsPerSecond >= 1.0:
                for i in frange(0, self.range):
                    # 0, 1, 2, ...
                    timeLabels[i] = True

            if maxLabelsPerSecond >= 2.0:
                for i in frange(0, self.range, 0.5):
                    # 0, 0.5, 1.0, ...
                    timeLabels[i] = True

            if maxLabelsPerSecond >= 10.0:
                for i in frange(0, self.range, 0.1):
                    # 0, 0.1, 0.2, ...
                    timeLabels[i] = True

            painter.scale(1.0 / xScale, 1)
            font = QtGui.QFont()
            font.setPixelSize(8)
            painter.setFont(font)
            painter.setPen(QtGui.QPen())

            for key, value in timeLabels.iteritems():
                if value:
                    # round(key, 6) because we want at most 6 digits, %g to display it as compactly as possible
                    painter.drawText(key * pixelsPerSecond * xScale - minLabelWidth * 0.5, 0, minLabelWidth, 10, QtCore.Qt.AlignHCenter | QtCore.Qt.AlignBottom, "%g" % (round(key, 6)))
                    painter.drawLine(QtCore.QPointF(key * pixelsPerSecond * xScale, 10), QtCore.QPointF(pixelsPerSecond * key * xScale, 15))

        finally:
            painter.restore()

class AffectorTimelineKeyFrame(QtGui.QGraphicsRectItem):
    keyFrame = property(lambda self: self.data(0),
                        lambda self, value: self.setData(0, value))

    def __init__(self, timelineSection = None, keyFrame = None):
        super(AffectorTimelineKeyFrame, self).__init__(timelineSection)

        self.timelineSection = timelineSection

        self.setKeyFrame(keyFrame)

        self.setFlags(QtGui.QGraphicsItem.ItemIsSelectable |
                      QtGui.QGraphicsItem.ItemIsMovable |
                      QtGui.QGraphicsItem.ItemIgnoresTransformations |
                      QtGui.QGraphicsItem.ItemSendsGeometryChanges)

        # the parts between keyframes are z-value 0 so this makes key frames always "stand out"
        self.setZValue(1)
        self.setRect(0, 1, 15, 29)

        #palette = QApplication.palette()
        self.setPen(QtGui.QPen(QtGui.QColor(QtCore.Qt.GlobalColor.transparent)))
        self.setBrush(QtGui.QBrush(QtGui.QColor(QtCore.Qt.GlobalColor.lightGray)))

        self.moveInProgress = False
        self.oldPosition = None

    def setKeyFrame(self, keyFrame):
        self.keyFrame = keyFrame

        self.refresh()

    def refresh(self):
        self.setPos(pixelsPerSecond * self.keyFrame.getPosition() if self.keyFrame is not None else 0, 0)

    def paint(self, painter, option, widget = None):
        #super(AffectorTimelineKeyFrame, self).paint(painter, option, widget)
        painter.save()

        try:
            palette = QtGui.QApplication.palette()

            painter.setPen(self.pen())
            painter.setBrush(self.brush())

            painter.drawRect(self.rect())

            # draw the circle representing the keyframe
            self.setPen(QtGui.QPen(QtGui.QColor(QtCore.Qt.GlobalColor.transparent)))
            painter.setBrush(QtGui.QBrush(palette.color(QtGui.QPalette.Normal, QtGui.QPalette.ButtonText)))
            painter.drawEllipse(QtCore.QPointF(7.5, 14.5), 4, 4)

        finally:
            painter.restore()

    def itemChange(self, change, value):
        if change == QtGui.QGraphicsItem.ItemSelectedHasChanged:
            #palette = QApplication.palette()
            self.setBrush(QtGui.QColor(114, 159, 207) if value else QtGui.QColor(QtCore.Qt.GlobalColor.lightGray))
            self.parentItem().prepareGeometryChange()
            return super(AffectorTimelineKeyFrame, self).itemChange(change, value)

        elif change == QtGui.QGraphicsItem.ItemPositionChange:
            newPosition = max(0, min(value.x() / pixelsPerSecond, self.keyFrame.getParent().getParent().getDuration()))

            if not self.moveInProgress:
                self.moveInProgress = True
                self.oldPosition = self.keyFrame.getPosition()

            while self.keyFrame.getParent().hasKeyFrameAtPosition(newPosition):
                # FIXME: we want newPosition * epsilon, but how do we get epsilon in python?
                newPosition += 0.00001

            self.timelineSection.prepareGeometryChange()
            self.keyFrame.moveToPosition(newPosition)

            return QtCore.QPointF(newPosition * pixelsPerSecond, self.pos().y())

        return super(AffectorTimelineKeyFrame, self).itemChange(change, value)

class AffectorTimelineSection(QtGui.QGraphicsRectItem):
    def __init__(self, timeline = None, affector = None):
        super(AffectorTimelineSection, self).__init__(timeline)

        self.timeline = timeline

        self.setFlags(QtGui.QGraphicsItem.ItemIsFocusable)

        #palette = QApplication.palette()
        self.setPen(QtGui.QPen(QtGui.QColor(QtCore.Qt.GlobalColor.lightGray)))
        self.setBrush(QtGui.QBrush(QtGui.QColor(QtCore.Qt.GlobalColor.transparent)))

        self.setAffector(affector)

    def setAffector(self, affector):
        self.affector = affector

        self.refresh()

    def refresh(self):
        self.setRect(0, 0, self.timeline.animation.getDuration() * pixelsPerSecond, 30)

        for item in self.childItems():
            # refcount drops and python should destroy that item
            self.scene().removeItem(item)

        if self.affector is None:
            return

        i = 0
        while i < self.affector.getNumKeyFrames():
            AffectorTimelineKeyFrame(self, self.affector.getKeyFrameAtIdx(i))

            # affector timeline key frame will refresh itself automatically to the right position
            # upon construction

            i += 1

    def paint(self, painter, option, widget = None):
        super(AffectorTimelineSection, self).paint(painter, option, widget)

        painter.save()

        try:
            #palette = QApplication.palette()

            transform = painter.worldTransform()
            xScale = transform.m11()

            painter.setRenderHint(QtGui.QPainter.RenderHint.Antialiasing, True)

            if self.affector.getNumKeyFrames() == 0:
                return

            def drawArrow(x, y):
                painter.setPen(QtCore.Qt.SolidLine)
                painter.drawLine(QtCore.QPointF(x, y), QtCore.QPointF(x - 3 / xScale, y - 3))
                painter.drawLine(QtCore.QPointF(x, y), QtCore.QPointF(x - 3 / xScale, y + 3))

            previousKeyFrame = self.affector.getKeyFrameAtIdx(0)
            i = 1
            while i < self.affector.getNumKeyFrames():
                keyFrame = self.affector.getKeyFrameAtIdx(i)

                # previousKeyFrame -------- line we will draw -------> keyFrame

                previousPosition = previousKeyFrame.getPosition() * pixelsPerSecond
                currentPosition = keyFrame.getPosition() * pixelsPerSecond
                span = currentPosition - previousPosition
                assert(span >= 0)

                selected = False
                for item in self.childItems():
                    if item.data(0).getPosition() == previousKeyFrame.getPosition():
                        selected = item.isSelected()
                        break

                painter.setPen(QtGui.QColor(QtCore.Qt.GlobalColor.transparent))
                painter.setBrush(QtGui.QBrush(QtGui.QColor(114, 159, 207) if selected else QtGui.QColor(255, 255, 255)))
                painter.drawRect(QtCore.QRectF(previousPosition, 1, span, 29))
                painter.setBrush(QtGui.QBrush())

                lineStartPosition = min(previousPosition + 15 / xScale, currentPosition)

                if keyFrame.getProgression() == PyCEGUI.KeyFrame.P_Linear:
                    # just a straight line in this case
                    painter.setPen(QtCore.Qt.DashLine)
                    painter.drawLine(QtCore.QPointF(lineStartPosition, 15), QtCore.QPointF(currentPosition, 15))
                    drawArrow(currentPosition, 15)

                elif keyFrame.getProgression() == PyCEGUI.KeyFrame.P_QuadraticAccelerating:
                    path = QtGui.QPainterPath()
                    path.moveTo(lineStartPosition, 27)
                    path.quadTo(previousPosition + span * 3/4, 30, currentPosition, 5)
                    painter.setPen(QtCore.Qt.DashLine)
                    painter.drawPath(path)

                    drawArrow(currentPosition, 5)

                elif keyFrame.getProgression() == PyCEGUI.KeyFrame.P_QuadraticDecelerating:
                    path = QtGui.QPainterPath()
                    path.moveTo(lineStartPosition, 3)
                    path.quadTo(previousPosition + span * 3/4, 0, currentPosition, 25)
                    painter.setPen(QtCore.Qt.DashLine)
                    painter.drawPath(path)

                    drawArrow(currentPosition, 25)

                elif keyFrame.getProgression() == PyCEGUI.KeyFrame.P_Discrete:
                    painter.setPen(QtCore.Qt.DashLine)
                    painter.drawLine(QtCore.QPointF(lineStartPosition, 27), QtCore.QPointF(previousPosition + span / 2, 27))
                    painter.drawLine(QtCore.QPointF(previousPosition + span / 2, 5), QtCore.QPointF(previousPosition + span / 2, 27))
                    painter.drawLine(QtCore.QPointF(previousPosition + span / 2, 5), QtCore.QPointF(currentPosition, 5))

                    drawArrow(currentPosition, 5)
                else:
                    # wrong progression?
                    assert(False)

                previousKeyFrame = keyFrame
                i += 1

        finally:
            painter.restore()

class AffectorTimelineLabel(QtGui.QGraphicsProxyWidget):
    def __init__(self, timeline = None, affector = None):
        super(AffectorTimelineLabel, self).__init__(timeline)

        self.setFlags(QtGui.QGraphicsItem.ItemIsFocusable |
                      QtGui.QGraphicsItem.ItemIgnoresTransformations)

        self.widget = QtGui.QComboBox()
        self.widget.setEditable(True)
        self.widget.resize(150, 25)
        self.setWidget(self.widget)

        self.setAffector(affector)

    def setAffector(self, affector):
        self.affector = affector

        self.refresh()

    def refresh(self):
        self.widget.setEditText(self.affector.getTargetProperty())

class TimelinePositionBar(QtGui.QGraphicsRectItem):
    def __init__(self, timeline):
        super(TimelinePositionBar, self).__init__(timeline)

        self.timeline = timeline

        self.setFlags(QtGui.QGraphicsItem.ItemIsMovable |
                      QtGui.QGraphicsItem.ItemIgnoresTransformations |
                      QtGui.QGraphicsItem.ItemSendsGeometryChanges)

        self.setPen(QtGui.QPen(QtGui.QColor(QtCore.Qt.GlobalColor.transparent)))
        self.setBrush(QtGui.QBrush(QtGui.QColor(255, 0, 0, 128)))

        # keep on top of everything
        self.setZValue(1)

        self.setHeight(1)

        self.setCursor(QtCore.Qt.SplitHCursor)

    def setHeight(self, height):
        self.setRect(QtCore.QRectF(-1, 6, 3, height - 6))

    def itemChange(self, change, value):
        if change == QtGui.QGraphicsItem.ItemPositionChange:
            oldPosition = self.pos().x() / pixelsPerSecond
            newPosition = value.x() / pixelsPerSecond

            animationDuration = self.timeline.animation.getDuration() if self.timeline.animation is not None else 0.0

            newPosition = max(0, min(newPosition, animationDuration))

            self.timeline.timePositionChanged.emit(oldPosition, newPosition)
            return QtCore.QPointF(newPosition * pixelsPerSecond, self.pos().y())

        return super(TimelinePositionBar, self).itemChange(change, value)

    def paint(self, painter, option, widget = None):
        super(TimelinePositionBar, self).paint(painter, option, widget)

        painter.save()

        try:
            painter.setPen(self.pen())
            painter.setBrush(self.brush())
            painter.drawConvexPolygon([QtCore.QPointF(-6, 0), QtCore.QPointF(6, 0),
                                       QtCore.QPointF(1, 6), QtCore.QPointF(-1, 6)])

        finally:
            painter.restore()

    def shape(self):
        ret = QtGui.QPainterPath()
        # the arrow on top so that it's easier to grab for for usability
        ret.addRect(QtCore.QRectF(-6, 0, 12, 6))
        # the rest
        ret.addRect(self.rect())

        return ret

class AnimationTimeline(QtGui.QGraphicsRectItem, QtCore.QObject):
    """A timeline widget for just one CEGUI animation"""

    # old timeline position, new timeline position
    timePositionChanged = QtCore.Signal(float, float)
    # old position, new position
    keyFramesMoved = QtCore.Signal(list)

    def __init__(self, parentItem = None, animation = None):
        # we only inherit from QObject to be able to define QtCore.Signals in our class
        QtCore.QObject.__init__(self)
        QtGui.QGraphicsRectItem.__init__(self, parentItem)

        self.playDelta = 1.0 / 60.0
        self.lastPlayInjectTime = time.time()
        self.playing = False

        self.setFlags(QtGui.QGraphicsItem.ItemIsFocusable)

        self.timecode = TimecodeLabel(self)
        self.timecode.setPos(QtCore.QPointF(0, 0))
        self.timecode.range = 1

        self.positionBar = TimelinePositionBar(self)

        self.animation = None
        self.animationInstance = None

        self.setAnimation(animation)

        self.ignoreTimelineChanges = False
        self.timePositionChanged.connect(self.slot_timePositionChanged)

    def setAnimation(self, animation):
        self.animation = animation

        if self.animationInstance is not None:
            PyCEGUI.AnimationManager.getSingleton().destroyAnimationInstance(self.animationInstance)
            self.animationInstance = None

        # we only use this animation instance to avoid reimplementing looping modes
        if self.animation is not None:
            self.animationInstance = PyCEGUI.AnimationManager.getSingleton().instantiateAnimation(self.animation)
            # we don't want CEGUI's injectTimePulse to step our animation instances, we will step them manually
            self.animationInstance.setAutoSteppingEnabled(False)
        else:
            # FIXME: Whenever AnimationTimeline is destroyed, one animation instance is leaked!
            self.animationInstance = None

        self.refresh()

    def refresh(self):
        for item in self.childItems():
            if item is self.timecode or item is self.positionBar:
                continue

            # refcount drops and python should destroy that item
            item.setParentItem(None)
            if self.scene():
                self.scene().removeItem(item)

        if self.animation is None:
            self.timecode.range = 1.0
            return

        self.timecode.range = self.animation.getDuration()

        i = 0
        while i < self.animation.getNumAffectors():
            affector = self.animation.getAffectorAtIdx(i)

            label = AffectorTimelineLabel(self, affector)
            label.setPos(-150, 17 + i * 32 + 2)
            label.setZValue(1)

            section = AffectorTimelineSection(self, affector)
            section.setPos(QtCore.QPointF(0, 17 + i * 32))

            i += 1

        self.positionBar.setHeight(17 + i * 32)

    def notifyZoomChanged(self, zoom):
        assert(zoom > 0)

        for item in self.childItems():
            if isinstance(item, AffectorTimelineLabel):
                item.setPos(-150 / zoom, item.pos().y())

    def setTimelinePosition(self, position):
        self.positionBar.setPos(position * pixelsPerSecond, self.positionBar.pos().y())

        if self.animationInstance is not None:
            self.animationInstance.setPosition(position)

    def playTick(self):
        if not self.playing:
            return

        if self.animationInstance is None:
            return

        delta = time.time() - self.lastPlayInjectTime
        # we get a little CEGUI internals abusive here, brace yourself!
        self.animationInstance.step(delta)
        self.setTimelinePosition(self.animationInstance.getPosition())
        # end of abuses! :D

        self.lastPlayInjectTime = time.time()
        QtCore.QTimer.singleShot(self.playDelta * 1000, lambda: self.playTick())

    def play(self):
        if self.playing:
            # if we didn't do this we would spawn multiple playTicks in parallel
            # it would bog the app down to a crawl
            return

        if self.animationInstance is None:
            self.playing = False
            return

        self.playing = True
        self.lastPlayInjectTime = time.time()
        self.animationInstance.start()

        self.playTick()

    def pause(self):
        if self.animationInstance is None:
            self.playing = False
            return

        self.animationInstance.togglePause()
        self.playing = self.animationInstance.isRunning()

        self.playTick()

    def stop(self):
        if self.animationInstance is None:
            self.playing = False
            return

        self.playing = False
        self.setTimelinePosition(0)
        self.animationInstance.stop()

    def slot_timePositionChanged(self, oldPosition, newPosition):
        if self.ignoreTimelineChanges:
            return

        try:
            self.ignoreTimelineChanges = True
            self.setTimelinePosition(newPosition)

        finally:
            self.ignoreTimelineChanges = False

    def notifyMouseReleased(self):
        """Called when mouse is released, grabs all moved keyframes
        and makes a list of old and new positions for them.
        """

        # list of lists, each list is [newIndex, oldPosition, newPosition]
        # newIndex is also current index
        movedKeyFrames = []
        for item in self.scene().selectedItems():
            if isinstance(item, AffectorTimelineKeyFrame):
                if item.moveInProgress:
                    affector = item.keyFrame.getParent()

                    movedKeyFrames.append([item.keyFrame.getIdxInParent(),
                                           affector.getIdxInParent(),
                                           item.oldPosition,
                                           item.keyFrame.getPosition()])
                    item.moveInProgress = False

        if len(movedKeyFrames) > 0:
            self.keyFramesMoved.emit(movedKeyFrames)
