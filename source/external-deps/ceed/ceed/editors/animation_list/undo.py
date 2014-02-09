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

from ceed import commands

idbase = 1300

class ChangeCurrentAnimationDefinitionCommand(commands.UndoCommand):
    """Changes currently edited animation definition.

    We have to make this an undo command to be sure that the context for other
    undo commands is always right.
    """

    def __init__(self, visual, newName, oldName):
        super(ChangeCurrentAnimationDefinitionCommand, self).__init__()

        self.visual = visual

        self.newName = newName
        self.oldName = oldName

        self.refreshText()

    def refreshText(self):
        self.setText("Now editing '%s'" % (self.newName))

    def id(self):
        return idbase + 1

    def mergeWith(self, cmd):
        self.newName = cmd.newName

        return True

    def undo(self):
        super(ChangeCurrentAnimationDefinitionCommand, self).undo()

        if self.oldName is None:
            self.visual.setCurrentAnimation(None)
        else:
            self.visual.setCurrentAnimationWrapper(self.visual.getAnimationWrapper(self.oldName))

    def redo(self):
        if self.newName is None:
            self.visual.setCurrentAnimation(None)
        else:
            self.visual.setCurrentAnimationWrapper(self.visual.getAnimationWrapper(self.newName))

        super(ChangeCurrentAnimationDefinitionCommand, self).redo()

class MoveKeyFramesCommand(commands.UndoCommand):
    """Moves gives key frames to given positions
    """

    def __init__(self, visual, movedKeyFrames):
        super(MoveKeyFramesCommand, self).__init__()

        self.visual = visual
        self.movedKeyFrames = movedKeyFrames

        # the next redo will be skipped
        self.dryRun = True

        self.refreshText()

    def refreshText(self):
        self.setText("Moved '%i' keyframe%s" % (len(self.movedKeyFrames), "s" if len(self.movedKeyFrames) > 1 else ""))

    def id(self):
        return idbase + 2

    def mergeWith(self, cmd):
        return False

    def undo(self):
        super(MoveKeyFramesCommand, self).undo()

        newMovedKeyFrames = []
        for movedKeyFrame in self.movedKeyFrames:
            keyFrameIndex, affectorIndex, oldPosition, newPosition = movedKeyFrame
            keyFrame = self.visual.getKeyFrameOfCurrentAnimation(affectorIndex, keyFrameIndex)
            keyFrame.moveToPosition(oldPosition)

            newMovedKeyFrames.append([keyFrame.getIdxInParent(), affectorIndex, oldPosition, newPosition])

        self.movedKeyFrames = newMovedKeyFrames

        self.visual.timelineDockWidget.timeline.refresh()

    def redo(self):
        if self.dryRun:
            self.dryRun = False
            return

        newMovedKeyFrames = []
        for movedKeyFrame in self.movedKeyFrames:
            keyFrameIndex, affectorIndex, oldPosition, newPosition = movedKeyFrame
            keyFrame = self.visual.getKeyFrameOfCurrentAnimation(affectorIndex, keyFrameIndex)
            keyFrame.moveToPosition(newPosition)

            newMovedKeyFrames.append([keyFrame.getIdxInParent(), affectorIndex, oldPosition, newPosition])

        self.movedKeyFrames = newMovedKeyFrames

        self.visual.timelineDockWidget.timeline.refresh()
