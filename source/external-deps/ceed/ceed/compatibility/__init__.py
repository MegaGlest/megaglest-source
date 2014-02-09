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

"""This module is the root of all compatibility support and layers in the editor
"""

import logging

# NOTE: It should be importable with as few dependencies as possible because
#       this is used in the command line migration tool!

class Layer(object):
    """Compatibility layer can transform given code from source type to target type.

    If you want transparent loading and saving you need to implement 2 layers!
    From your type to editor's supported type (or type for which there already are
    compatibility layers) and back!
    """

    def getSourceType(self):
        raise NotImplementedError("Compatibility layers have to override Layer.getSourceType!")

    def getTargetType(self):
        raise NotImplementedError("Compatibility layers have to override Layer.getTargetType!")

    def transform(self, data):
        """Transforms given data from sourceType (== self.getSourceType())
        to targetType (== self.getTargetType())
        """

        raise NotImplementedError("Compatibility layers have to override Layer.transform!")

class TypeDetector(object):
    def getType(self):
        """Gets the type this detector detects"""

        raise NotImplementedError("Compatibility type detectors have to override TypeDetector.getType!")

    def getPossibleExtensions(self):
        """Retrieves all possible extensions the type this detector detects can have as a set."""

        raise NotImplementedError("Compatibility type detectors have to override TypeDetector.getPossibleExtensions!")

    def matches(self, data, extension):
        """Checks whether given source code and extension match this detector's type.

        The detector should be as strict as possible, if it returns that the type matches, the editor will
        assume it can safely open it as that type. If unsure, return False!

        User will be prompted to choose which type the file is if the editor can't make a 100% match.
        """

        raise NotImplementedError("Compatibility type detectors have to override TypeDetector.getType!")

class LayerNotFoundError(RuntimeError):
    """Exception thrown when no compatibility layer or path can be found between 2 types"""

    def __init__(self, sourceType, targetType):
        super(LayerNotFoundError, self).__init__("Can't find any compatibility path from sourceType '%s' to targetType '%s'" % (sourceType, targetType))

class MultiplePossibleTypesError(RuntimeError):
    """Exception thrown when multiple types match given data (from the guessType method), user should be
    asked to choose the right type in this case
    """

    def __init__(self, possibleTypes):
        super(MultiplePossibleTypesError, self).__init__("Given data matches multiple types (%i possible types)" % (len(possibleTypes)))

        # we store possible types so that developers can catch this and offer a choice to the user
        self.possibleTypes = possibleTypes

class NoPossibleTypesError(RuntimeError):
    """Exception thrown when no types match given data (from the guessType method), user should be
    asked to choose the right type in this case
    """

    def __init__(self):
        super(NoPossibleTypesError, self).__init__("Can't decide type of given code and extension, no positives turned up!")

class Manager(object):
    """Manager holds type detectors and compatibility layers and is able to perform transformation between data.

    It is usually used as a singleton and this is just the base class! See compatibility.imageset.Manager for
    example of use of this class
    """

    def __init__(self):
        # derived Managers should override this and provide the info
        self.CEGUIVersionTypes = {}
        # as well as this
        self.EditorNativeType = None

        self.detectors = []
        self.layers = []

    def getKnownTypes(self):
        """Retrieves types that we have detectors for."""

        ret = []
        for detector in self.detectors:
            ret.append(detector.getType())

        return ret

    def getAllPossibleExtensions(self):
        """Retrieves all possible extensions of all types this manager knows of."""

        ret = set()
        for detector in self.detectors:
            ret.update(detector.getPossibleExtensions())

        return ret

    def getCEGUIVersionsCompatibleWithType(self, type_):
        ret = []

        for version, otherType in self.CEGUIVersionTypes.iteritems():
            if type_ == otherType:
                ret.append(version)

        return ret

    def getSuitableDataTypeForCEGUIVersion(self, ceguiVersion):
        ret = []

        for version, dataType in self.CEGUIVersionTypes.iteritems():
            if version == ceguiVersion:
                ret.append(dataType)

        if len(ret) > 1:
            raise RuntimeError("More than one data type is suitable for given CEGUI version '%s', this must be a mistake in the compatibility code in CEED!" % (ceguiVersion))

        if len(ret) == 0:
            raise RuntimeError("Can't find any suitable data type for given CEGUI version '%s'. It's possible that this version isn't supported for the editing you are about to do." % (ceguiVersion))

        return ret[0]

    def transform(self, sourceType, targetType, data):
        """Performs transformation of given source code from sourceType to targetType.

        TODO: This method doesn't even bother to try to find the shortest path possible or such,
              I leave this as an exercise for future generations :-D
        """

        logging.debug("Attempting to transform type '%s' into '%s' (compatibility path fragments are in reverse order)", sourceType, targetType)

        # special case:
        if sourceType == targetType:
            logging.debug("Returning data with no transformation applied, both types are the same!")
            return data

        for layer in self.layers:
            if layer.getSourceType() == sourceType:
                try:
                    ret = self.transform(layer.getTargetType(), targetType, layer.transform(data))
                    logging.debug("Compatibility path fragment: '%s' <- '%s'", layer.getTargetType(), sourceType)
                    return ret

                except LayerNotFoundError:
                    # this path doesn't lead anywhere,
                    # lets try to find another one
                    pass


        raise LayerNotFoundError(sourceType, targetType)

    def guessType(self, code, extension = ""):
        """Attempts to make an informed guess based on given data and extension. If the guess is positive, the
        data *should be* of returned type. It depends on type detectors however.

        If you pass full file path instead of the extension, the extension will be extracted from it.
        """

        logging.debug("Attempting to guess type (code size: %i, extension: '%s')", len(code), extension)

        extSplit = extension.rsplit(".", 1)

        if len(extSplit) > 0:
            extension = extSplit[1] if len(extSplit) == 2 else extSplit[0]
        else:
            extension = extension.lstrip(".", 1)

        ret = []

        for detector in self.detectors:
            if detector.matches(code, extension):
                logging.debug("Detector '%s' reported a positive match!", detector.getType())
                ret.append(detector.getType())

        if len(ret) > 1:
            raise MultiplePossibleTypesError(ret)

        if len(ret) == 0:
            raise NoPossibleTypesError()

        return ret[0]

    def transformTo(self, targetType, code, extension = ""):
        """Transforms given code to given target type.

        extension is optional and used as a hint for the type guessing, you can pass the full file path,
        extension will be extracted.

        This method tries to guess type of given code and extension, in the case this fails, exception is thrown.
        """

        sourceType = self.guessType(code, extension)
        return self.transform(sourceType, targetType, code)

CEGUIVersions = ["0.6", "0.7", "1.0"]
EditorEmbeddedCEGUIVersion = "1.0"
