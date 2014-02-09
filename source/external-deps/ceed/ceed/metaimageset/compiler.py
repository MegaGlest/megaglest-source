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

import math
import os.path

from ceed.metaimageset import rectanglepacking
import ceed.compatibility.imageset as imageset_compatibility

import threading
import Queue
import sys

from PySide import QtCore
from PySide import QtGui

class ImageInstance(object):
    def __init__(self, x, y, image):
        self.x = x
        self.y = y

        self.image = image

class CompilerInstance(object):
    def __init__(self, metaImageset):
        self.jobs = 1
        self.sizeIncrement = 5
        # if True, the images will be padded on all sizes to prevent UV
        # rounding/interpolation artefacts
        self.padding = True

        self.metaImageset = metaImageset

    def estimateMinimalSize(self, images):
        """Tries to estimate minimal side of the underlying image of the output imageset.

        This is used merely as a starting point in the packing process.
        """

        def getNextPOT(number):
            """Returns the next power of two that is greater than given number"""

            return int(2 ** math.ceil(math.log(number + 1, 2)))

        area = 0
        for image in images:
            if self.padding:
                area += (image.qimage.width() + 2) * (image.qimage.height() + 2)
            else:
                area += (image.qimage.width()) * (image.qimage.height())

        ret = math.sqrt(area)

        if ret < 1:
            ret = 1

        if self.metaImageset.onlyPOT:
            ret = getNextPOT(ret)

        return ret

    def findSideSize(self, startingSideSize, images):
        sideSize = startingSideSize
        imageInstances = []

        i = 0

        # This could be way sped up if we used some sort of a "binary search" approach
        while True:
            packer = rectanglepacking.CygonRectanglePacker(sideSize, sideSize)
            try:
                imageInstances = []

                for image in images:
                    if self.padding:
                        point = packer.pack(image.qimage.width() + 2, image.qimage.height() + 2)
                    else:
                        point = packer.pack(image.qimage.width(), image.qimage.height())

                    imageInstances.append(ImageInstance(point.x, point.y, image))

                # everything seems to have gone smoothly, lets use this configuration then
                break

            except rectanglepacking.OutOfSpaceError:
                sideSize = getNextPOT(sideSize) if self.metaImageset.onlyPOT else sideSize + self.sizeIncrement

                i += 1
                if i % 5 == 0:
                    print("%i candidate sizes checked" % (i))

                continue

        print("Correct texture side size found after %i iterations" % (i))
        print("")

        return sideSize, imageInstances

    def buildAllImages(self, inputs, parallelJobs):
        assert(parallelJobs >= 1)

        images = []

        queue = Queue.Queue()
        for input_ in inputs:
            queue.put_nowait(input_)

        # we use a nasty trick of adding None elements to a list
        # because Python's int type is immutable
        doneTasks = []

        errorsEncountered = threading.Event()

        def imageBuilder():
            while True:
                try:
                    input_ = queue.get(False)

                    # In case an error occurs in any of the builders, we short circuit
                    # everything.
                    if not errorsEncountered.is_set():
                        try:
                            # We do not have to do anything extra thanks to GIL
                            images.extend(input_.buildImages())
                        except Exception as e:
                            print("Error building input '%s'. %s" % (input_.getDescription(), e))
                            errorsEncountered.set()

                    # If an exception was caught above, fake the task as done to allow the builders to end
                    queue.task_done()
                    doneTasks.append(None)

                    percent = "{0:6.2f}%".format(float(len(doneTasks) * 100) / len(inputs))

                    # same as above
                    sys.stdout.write("[%s] Images from %s\n" % (percent, input_.getDescription()))

                except Queue.Empty:
                    break

        workers = []
        for workerId in range(parallelJobs):
            worker = threading.Thread(name = "MetaImageset compiler image builder worker #%i" % (workerId), target = imageBuilder)
            workers.append(worker)
            worker.start()

        queue.join()

        if errorsEncountered.is_set():
            raise RuntimeError("Errors encountered when building images!")

        return images

    def compile(self):
        print("Gathering and rendering all images in %i parallel jobs..." % (self.jobs))
        print("")

        images = self.buildAllImages(self.metaImageset.inputs, self.jobs)
        print("")

        theoreticalMinSize = self.estimateMinimalSize(images)

        # the image packer performs better if images are inserted by width, thinnest come first
        images = sorted(images, key = lambda image: image.qimage.width())

        print("Performing texture side size determination...")
        sideSize, imageInstances = self.findSideSize(theoreticalMinSize, images)

        print("Rendering the underlying image...")
        underlyingImage = QtGui.QImage(sideSize, sideSize, QtGui.QImage.Format_ARGB32)
        underlyingImage.fill(0)

        painter = QtGui.QPainter()
        painter.begin(underlyingImage)

        # Sort image instances by name to give us nicer diffs of the resulting imageset
        imageInstances = sorted(imageInstances, key = lambda instance: instance.image.name)

        for imageInstance in imageInstances:
            qimage = imageInstance.image.qimage

            if self.padding:
                # FIXME: This could use a review!

                # top without corners
                painter.drawImage(QtCore.QPointF(imageInstance.x + 1, imageInstance.y), qimage.copy(0, 0, qimage.width(), 1))
                # bottom without corners
                painter.drawImage(QtCore.QPointF(imageInstance.x + 1, imageInstance.y + 1 + qimage.height()), qimage.copy(0, qimage.height() - 1, qimage.width(), 1))
                # left without corners
                painter.drawImage(QtCore.QPointF(imageInstance.x, imageInstance.y + 1), qimage.copy(0, 0, 1, qimage.height()))
                # right without corners
                painter.drawImage(QtCore.QPointF(imageInstance.x + 1 + qimage.width(), imageInstance.y + 1), qimage.copy(qimage.width() - 1, 0, 1, qimage.height()))

                # top left corner
                painter.drawImage(QtCore.QPointF(imageInstance.x, imageInstance.y), qimage.copy(0, 0, 1, 1))
                # top right corner
                painter.drawImage(QtCore.QPointF(imageInstance.x + 1 + qimage.width(), imageInstance.y), qimage.copy(qimage.width() - 1, 0, 1, 1))
                # bottom left corner
                painter.drawImage(QtCore.QPointF(imageInstance.x, imageInstance.y + 1 + qimage.height()), qimage.copy(0, qimage.height() - 1, 1, 1))
                # bottom right corner
                painter.drawImage(QtCore.QPointF(imageInstance.x + 1 + qimage.width(), imageInstance.y + 1 + qimage.height()), qimage.copy(qimage.width() - 1, qimage.height() - 1, 1, 1))

                # and then draw the real image on top
                painter.drawImage(QtCore.QPointF(imageInstance.x + 1, imageInstance.y + 1),
                                  qimage)
            else:
                # padding disabled, just draw the real image
                painter.drawImage(QtCore.QPointF(imageInstance.x, imageInstance.y),
                                  qimage)

        painter.end()

        print("Saving underlying image...")

        outputSplit = self.metaImageset.output.rsplit(".", 1)
        underlyingImageFileName = "%s.png" % (outputSplit[0])
        underlyingImage.save(os.path.join(self.metaImageset.getOutputDirectory(), underlyingImageFileName))

        # CEGUI imageset format is very simple and easy to work with, using serialisation in the editor for this
        # seemed like a wasted effort :-)

        nativeData = "<Imageset name=\"%s\" imagefile=\"%s\" nativeHorzRes=\"%i\" nativeVertRes=\"%i\" autoScaled=\"%s\" version=\"2\">\n" % (self.metaImageset.name, underlyingImageFileName, self.metaImageset.nativeHorzRes, self.metaImageset.nativeVertRes, self.metaImageset.autoScaled)
        for imageInstance in imageInstances:
            paddingOffset = 1 if self.padding else 0

            nativeData += "    <Image name=\"%s\" xPos=\"%i\" yPos=\"%i\" width=\"%i\" height=\"%i\" xOffset=\"%i\" yOffset=\"%i\" />\n" % (imageInstance.image.name, imageInstance.x + paddingOffset, imageInstance.y + paddingOffset, imageInstance.image.qimage.width(), imageInstance.image.qimage.height(), imageInstance.image.xOffset, imageInstance.image.yOffset)

        nativeData += "</Imageset>\n"

        outputData = imageset_compatibility.manager.transform(imageset_compatibility.manager.EditorNativeType, self.metaImageset.outputTargetType, nativeData)
        open(os.path.join(self.metaImageset.getOutputDirectory(), self.metaImageset.output), "w").write(outputData)

        print("All done and saved!")
        print("")

        rjustChars = 40
        print("Amount of inputs: ".rjust(rjustChars) + "%i" % (len(self.metaImageset.inputs)))
        print("Amount of images on the atlas: ".rjust(rjustChars) + "%i" % (len(imageInstances)))
        print("")
        print("Theoretical minimum texture size: ".rjust(rjustChars) + "%i x %i" % (theoreticalMinSize, theoreticalMinSize))
        print("Actual texture size: ".rjust(rjustChars) + "%i x %i" % (sideSize, sideSize))
        print("")
        print("Side size overhead: ".rjust(rjustChars) + "%f%%" % ((sideSize - theoreticalMinSize) / (theoreticalMinSize) * 100))
        print("Area (squared) overhead: ".rjust(rjustChars) + "%f%%" % ((sideSize * sideSize - theoreticalMinSize * theoreticalMinSize) / (theoreticalMinSize * theoreticalMinSize) * 100))
