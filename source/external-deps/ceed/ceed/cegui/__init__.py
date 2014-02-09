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
from PySide import QtOpenGL

from OpenGL import GL

import os.path

import PyCEGUI
import PyCEGUIOpenGLRenderer

class RedirectingCEGUILogger(PyCEGUI.Logger):
    """Allows us to register subscribers that want CEGUI log info

    This prevents writing CEGUI.log into CWD and will allow log display inside
    the app in the future
    """

    def __init__(self):
        # don't use super here, PyCEGUI.Logger is an old-style class
        PyCEGUI.Logger.__init__(self)

        self.subscribers = set()

    def registerSubscriber(self, subscriber):
        assert(callable(subscriber))

        self.subscribers.add(subscriber)

    def logEvent(self, message, level):
        for subscriber in self.subscribers:
            subscriber(message, level)

    def setLogFilename(self, name, append):
        # this is just a NOOP to satisfy CEGUI pure virtual method of the same name
        pass

class GLContextProvider(object):
    """Interface that provides a method to make OpenGL context
    suitable for CEGUI the current context.
    """

    def makeGLContextCurrent(self):
        """Activates the OpenGL context held by this provider"""

        raise NotImplementedError("All classes inheriting GLContextProvider must override GLContextProvider.makeGLContextCurrent")

class Instance(object):
    """Encapsulates a running CEGUI instance.

    Right now CEGUI can only be instantiated once because it's full of singletons.
    This might change in the future though...
    """

    def __init__(self, contextProvider = None):
        self.contextProvider = contextProvider

        self.logger = RedirectingCEGUILogger()

        self.initialised = False
        self.lastRenderTimeDelta = 0

    def setGLContextProvider(self, contextProvider):
        """CEGUI instance might need an OpenGL context provider to make sure the right context is active
        (to load textures, render, ...)

        see GLContextProvider
        """

        self.contextProvider = contextProvider

    def makeGLContextCurrent(self):
        """Activate the right OpenGL context.

        This is usually called internally and you don't need to worry about it, it generally needs to be called
        before any rendering is done, textures are loaded, etc...
        """

        self.contextProvider.makeGLContextCurrent()

    def ensureIsInitialised(self):
        """Ensures this CEGUI instance is properly initialised, if it's not it initialises it right away.
        """

        if not self.initialised:
            self.makeGLContextCurrent()

            # we don't want CEGUI Exceptions to output to stderr every time
            # they are constructed
            PyCEGUI.Exception.setStdErrEnabled(False)
            # FBOs are for sure supported at this point because CEED uses them internally
            PyCEGUIOpenGLRenderer.OpenGLRenderer.bootstrapSystem(PyCEGUIOpenGLRenderer.OpenGLRenderer.TTT_FBO)
            self.initialised = True

            self.setDefaultResourceGroups()

    def setResourceGroupDirectory(self, resourceGroup, absoluteDirPath):
        """Sets given resourceGroup to look into given absoluteDirPath
        """

        self.ensureIsInitialised()

        rp = PyCEGUI.System.getSingleton().getResourceProvider()

        rp.setResourceGroupDirectory(resourceGroup, absoluteDirPath)

    def setDefaultResourceGroups(self):
        """Puts the resource groups to a reasonable default value.

        ./datafiles followed by the respective folder, same as CEGUI stock datafiles
        """

        self.ensureIsInitialised()

        # reasonable default directories
        defaultBaseDirectory = os.path.join(os.path.curdir, "datafiles")

        self.setResourceGroupDirectory("imagesets",
                                       os.path.join(defaultBaseDirectory, "imagesets"))
        self.setResourceGroupDirectory("fonts",
                                       os.path.join(defaultBaseDirectory, "fonts"))
        self.setResourceGroupDirectory("schemes",
                                       os.path.join(defaultBaseDirectory, "schemes"))
        self.setResourceGroupDirectory("looknfeels",
                                       os.path.join(defaultBaseDirectory, "looknfeel"))
        self.setResourceGroupDirectory("layouts",
                                       os.path.join(defaultBaseDirectory, "layouts"))
        self.setResourceGroupDirectory("xml_schemas",
                                       os.path.join(defaultBaseDirectory, "xml_schemas"))

        # all this will never be set to anything else again
        PyCEGUI.ImageManager.setImagesetDefaultResourceGroup("imagesets")
        PyCEGUI.Font.setDefaultResourceGroup("fonts")
        PyCEGUI.Scheme.setDefaultResourceGroup("schemes")
        PyCEGUI.WidgetLookManager.setDefaultResourceGroup("looknfeels")
        PyCEGUI.WindowManager.setDefaultResourceGroup("layouts")

        parser = PyCEGUI.System.getSingleton().getXMLParser()
        if parser.isPropertyPresent("SchemaDefaultResourceGroup"):
            parser.setProperty("SchemaDefaultResourceGroup", "xml_schemas")

    def cleanCEGUIResources(self):
        # destroy all previous resources (if any)
        if self.initialised:
            PyCEGUI.WindowManager.getSingleton().destroyAllWindows()
            # we need to ensure all windows are destroyed, dangling pointers would
            # make us segfault later otherwise
            PyCEGUI.WindowManager.getSingleton().cleanDeadPool()
            PyCEGUI.FontManager.getSingleton().destroyAll()
            PyCEGUI.ImageManager.getSingleton().destroyAll()
            PyCEGUI.SchemeManager.getSingleton().destroyAll()
            PyCEGUI.WidgetLookManager.getSingleton().eraseAllWidgetLooks()
            PyCEGUI.AnimationManager.getSingleton().destroyAllAnimations()
            PyCEGUI.WindowFactoryManager.getSingleton().removeAllFalagardWindowMappings()
            PyCEGUI.WindowFactoryManager.getSingleton().removeAllWindowTypeAliases()
            PyCEGUI.WindowFactoryManager.getSingleton().removeAllFactories()
            # the previous call removes all Window factories, including the stock ones like DefaultWindow
            # lets add them back
            PyCEGUI.System.getSingleton().addStandardWindowFactories()
            PyCEGUI.System.getSingleton().getRenderer().destroyAllTextures()

    def syncToProject(self, project, mainWindow = None):
        """Synchronises the instance with given project, respecting it's paths and resources
        """

        progress = QtGui.QProgressDialog(mainWindow)
        progress.setWindowModality(QtCore.Qt.WindowModal)
        progress.setWindowTitle("Synchronising embedded CEGUI with the project")
        progress.setCancelButton(None)
        progress.resize(400, 100)
        progress.show()

        self.ensureIsInitialised()
        self.makeGLContextCurrent()

        schemeFiles = []
        absoluteSchemesPath = project.getAbsolutePathOf(project.schemesPath)
        if os.path.exists(absoluteSchemesPath):
            for file_ in os.listdir(absoluteSchemesPath):
                if file_.endswith(".scheme"):
                    schemeFiles.append(file_)
        else:
            progress.reset()
            raise IOError("Can't list scheme path '%s'" % (absoluteSchemesPath))

        progress.setMinimum(0)
        progress.setMaximum(2 + 9 * len(schemeFiles))

        progress.setLabelText("Purging all resources...")
        progress.setValue(0)
        QtGui.QApplication.instance().processEvents()

        # destroy all previous resources (if any)
        self.cleanCEGUIResources()

        progress.setLabelText("Setting resource paths...")
        progress.setValue(1)
        QtGui.QApplication.instance().processEvents()

        self.setResourceGroupDirectory("imagesets", project.getAbsolutePathOf(project.imagesetsPath))
        self.setResourceGroupDirectory("fonts", project.getAbsolutePathOf(project.fontsPath))
        self.setResourceGroupDirectory("schemes", project.getAbsolutePathOf(project.schemesPath))
        self.setResourceGroupDirectory("looknfeels", project.getAbsolutePathOf(project.looknfeelsPath))
        self.setResourceGroupDirectory("layouts", project.getAbsolutePathOf(project.layoutsPath))
        self.setResourceGroupDirectory("xml_schemas", project.getAbsolutePathOf(project.xmlSchemasPath))

        progress.setLabelText("Recreating all schemes...")
        progress.setValue(2)
        QtGui.QApplication.instance().processEvents()

        # we will load resources manually to be able to use the compatibility layer machinery
        PyCEGUI.SchemeManager.getSingleton().setAutoLoadResources(False)

        from ceed import compatibility
        import ceed.compatibility.scheme as scheme_compatibility
        import ceed.compatibility.imageset as imageset_compatibility
        import ceed.compatibility.font as font_compatibility
        import ceed.compatibility.looknfeel as looknfeel_compatibility

        try:
            for schemeFile in schemeFiles:
                def updateProgress(message):
                    progress.setValue(progress.value() + 1)
                    progress.setLabelText("Recreating all schemes... (%s)\n\n%s" % (schemeFile, message))

                    QtGui.QApplication.instance().processEvents()

                updateProgress("Parsing the scheme file")
                schemeFilePath = project.getResourceFilePath(schemeFile, PyCEGUI.Scheme.getDefaultResourceGroup())
                rawData = open(schemeFilePath, "r").read()
                rawDataType = scheme_compatibility.manager.EditorNativeType

                try:
                    rawDataType = scheme_compatibility.manager.guessType(rawData, schemeFilePath)

                except compatibility.NoPossibleTypesError:
                    QtGui.QMessageBox.warning(None, "Scheme doesn't match any known data type", "The scheme '%s' wasn't recognised by CEED as any scheme data type known to it. Please check that the data isn't corrupted. CEGUI instance synchronisation aborted!" % (schemeFilePath))
                    return

                except compatibility.MultiplePossibleTypesError as e:
                    suitableVersion = scheme_compatibility.manager.getSuitableDataTypeForCEGUIVersion(project.CEGUIVersion)

                    if suitableVersion not in e.possibleTypes:
                        QtGui.QMessageBox.warning(None, "Incorrect scheme data type", "The scheme '%s' checked out as some potential data types, however not any of these is suitable for your project's target CEGUI version '%s', please check your project settings! CEGUI instance synchronisation aborted!" % (schemeFilePath, suitableVersion))
                        return

                    rawDataType = suitableVersion

                nativeData = scheme_compatibility.manager.transform(rawDataType, scheme_compatibility.manager.EditorNativeType, rawData)
                scheme = PyCEGUI.SchemeManager.getSingleton().createFromString(nativeData)

                # NOTE: This is very CEGUI implementation specific unfortunately!
                #
                #       However I am not really sure how to do this any better.

                updateProgress("Loading XML imagesets")
                xmlImagesetIterator = scheme.getXMLImagesets()
                while not xmlImagesetIterator.isAtEnd():
                    loadableUIElement = xmlImagesetIterator.getCurrentValue()
                    imagesetFilePath = project.getResourceFilePath(loadableUIElement.filename, loadableUIElement.resourceGroup if loadableUIElement.resourceGroup != "" else PyCEGUI.ImageManager.getImagesetDefaultResourceGroup())
                    imagesetRawData = open(imagesetFilePath, "r").read()
                    imagesetRawDataType = imageset_compatibility.manager.EditorNativeType

                    try:
                        imagesetRawDataType = imageset_compatibility.manager.guessType(imagesetRawData, imagesetFilePath)

                    except compatibility.NoPossibleTypesError:
                        QtGui.QMessageBox.warning(None, "Imageset doesn't match any known data type", "The imageset '%s' wasn't recognised by CEED as any imageset data type known to it. Please check that the data isn't corrupted. CEGUI instance synchronisation aborted!" % (imagesetFilePath))
                        return

                    except compatibility.MultiplePossibleTypesError as e:
                        suitableVersion = imageset_compatibility.manager.getSuitableDataTypeForCEGUIVersion(project.CEGUIVersion)

                        if suitableVersion not in e.possibleTypes:
                            QtGui.QMessageBox.warning(None, "Incorrect imageset data type", "The imageset '%s' checked out as some potential data types, however none of these is suitable for your project's target CEGUI version '%s', please check your project settings! CEGUI instance synchronisation aborted!" % (imagesetFilePath, suitableVersion))
                            return

                        imagesetRawDataType = suitableVersion

                    imagesetNativeData = imageset_compatibility.manager.transform(imagesetRawDataType, imageset_compatibility.manager.EditorNativeType, imagesetRawData)

                    PyCEGUI.ImageManager.getSingleton().loadImagesetFromString(imagesetNativeData)
                    xmlImagesetIterator.next()

                updateProgress("Loading image file imagesets")
                scheme.loadImageFileImagesets()

                updateProgress("Loading fonts")
                fontIterator = scheme.getFonts()
                while not fontIterator.isAtEnd():
                    loadableUIElement = fontIterator.getCurrentValue()
                    fontFilePath = project.getResourceFilePath(loadableUIElement.filename, loadableUIElement.resourceGroup if loadableUIElement.resourceGroup != "" else PyCEGUI.Font.getDefaultResourceGroup())
                    fontRawData = open(fontFilePath, "r").read()
                    fontRawDataType = font_compatibility.manager.EditorNativeType

                    try:
                        fontRawDataType = font_compatibility.manager.guessType(fontRawData, fontFilePath)

                    except compatibility.NoPossibleTypesError:
                        QtGui.QMessageBox.warning(None, "Font doesn't match any known data type", "The font '%s' wasn't recognised by CEED as any font data type known to it. Please check that the data isn't corrupted. CEGUI instance synchronisation aborted!" % (fontFilePath))
                        return

                    except compatibility.MultiplePossibleTypesError as e:
                        suitableVersion = font_compatibility.manager.getSuitableDataTypeForCEGUIVersion(project.CEGUIVersion)

                        if suitableVersion not in e.possibleTypes:
                            QtGui.QMessageBox.warning(None, "Incorrect font data type", "The font '%s' checked out as some potential data types, however none of these is suitable for your project's target CEGUI version '%s', please check your project settings! CEGUI instance synchronisation aborted!" % (fontFilePath, suitableVersion))
                            return

                        fontRawDataType = suitableVersion

                    fontNativeData = font_compatibility.manager.transform(fontRawDataType, font_compatibility.manager.EditorNativeType, fontRawData)

                    PyCEGUI.FontManager.getSingleton().createFromString(fontNativeData)
                    fontIterator.next()

                updateProgress("Loading looknfeels")
                looknfeelIterator = scheme.getLookNFeels()
                while not looknfeelIterator.isAtEnd():
                    loadableUIElement = looknfeelIterator.getCurrentValue()
                    looknfeelFilePath = project.getResourceFilePath(loadableUIElement.filename, loadableUIElement.resourceGroup if loadableUIElement.resourceGroup != "" else PyCEGUI.WidgetLookManager.getDefaultResourceGroup())
                    looknfeelRawData = open(looknfeelFilePath, "r").read()
                    looknfeelRawDataType = looknfeel_compatibility.manager.EditorNativeType
                    try:
                        looknfeelRawDataType = looknfeel_compatibility.manager.guessType(looknfeelRawData, looknfeelFilePath)

                    except compatibility.NoPossibleTypesError:
                        QtGui.QMessageBox.warning(None, "LookNFeel doesn't match any known data type", "The looknfeel '%s' wasn't recognised by CEED as any looknfeel data type known to it. Please check that the data isn't corrupted. CEGUI instance synchronisation aborted!" % (looknfeelFilePath))
                        return

                    except compatibility.MultiplePossibleTypesError as e:
                        suitableVersion = looknfeel_compatibility.manager.getSuitableDataTypeForCEGUIVersion(project.CEGUIVersion)

                        if suitableVersion not in e.possibleTypes:
                            QtGui.QMessageBox.warning(None, "Incorrect looknfeel data type", "The looknfeel '%s' checked out as some potential data types, however none of these is suitable for your project's target CEGUI version '%s', please check your project settings! CEGUI instance synchronisation aborted!" % (looknfeelFilePath, suitableVersion))
                            return

                        looknfeelRawDataType = suitableVersion

                    looknfeelNativeData = looknfeel_compatibility.manager.transform(looknfeelRawDataType, looknfeel_compatibility.manager.EditorNativeType, looknfeelRawData)

                    PyCEGUI.WidgetLookManager.getSingleton().parseLookNFeelSpecificationFromString(looknfeelNativeData)
                    looknfeelIterator.next()

                updateProgress("Loading window renderer factory modules")
                scheme.loadWindowRendererFactories()
                updateProgress("Loading window factories")
                scheme.loadWindowFactories()
                updateProgress("Loading factory aliases")
                scheme.loadFactoryAliases()
                updateProgress("Loading falagard mappings")
                scheme.loadFalagardMappings()

        except:
            self.cleanCEGUIResources()
            raise

        finally:
            # put SchemeManager into the default state again
            PyCEGUI.SchemeManager.getSingleton().setAutoLoadResources(True)

            progress.reset()
            QtGui.QApplication.instance().processEvents()

    def getAvailableSkins(self):
        """Retrieves skins (as strings representing their names) that are available
        from the set of schemes that were loaded.

        see syncToProject
        """

        skins = []

        it = PyCEGUI.WindowFactoryManager.getSingleton().getFalagardMappingIterator()
        while not it.isAtEnd():
            currentSkin = it.getCurrentValue().d_windowType.split('/')[0]
            if currentSkin not in skins:
                skins.append(currentSkin)

            it.next()

        return sorted(skins)

    def getAvailableFonts(self):
        """Retrieves fonts (as strings representing their names) that are available
        from the set of schemes that were loaded.

        see syncToProject
        """

        fonts = []

        it = PyCEGUI.FontManager.getSingleton().getIterator()
        while not it.isAtEnd():
            fonts.append(it.getCurrentKey())
            it.next()

        return sorted(fonts)

    def getAvailableImages(self):
        """Retrieves images (as strings representing their names) that are available
        from the set of schemes that were loaded.

        see syncToProject
        """

        images = []

        it = PyCEGUI.ImageManager.getSingleton().getIterator()
        while not it.isAtEnd():
            images.append(it.getCurrentKey())
            it.next()

        return sorted(images)

    def getAvailableWidgetsBySkin(self):
        """Retrieves all mappings (string names) of all widgets that can be created

        see syncToProject
        """

        ret = {}
        ret["__no_skin__"] = ["DefaultWindow", "DragContainer",
                             "VerticalLayoutContainer", "HorizontalLayoutContainer",
                             "GridLayoutContainer"]

        it = PyCEGUI.WindowFactoryManager.getSingleton().getFalagardMappingIterator()
        while not it.isAtEnd():
            #base = it.getCurrentValue().d_baseType
            mappedType = it.getCurrentValue().d_windowType.split('/', 1)
            assert(len(mappedType) == 2)

            look = mappedType[0]
            widget = mappedType[1]

            # insert empty list for the look if it's a new look
            if not look in ret:
                ret[look] = []

            # append widget name to the list for its look
            ret[look].append(widget)

            it.next()

        # sort the lists
        for look in ret:
            ret[look].sort()

        return ret

    def getWidgetPreviewImage(self, widgetType, previewWidth = 128, previewHeight = 64):
        """Renders and retrieves a widget preview image (as QImage).

        This is useful for various widget selection lists as a preview.
        """

        self.ensureIsInitialised()
        self.makeGLContextCurrent()

        system = PyCEGUI.System.getSingleton()

        renderer = system.getRenderer()

        renderTarget = PyCEGUIOpenGLRenderer.OpenGLViewportTarget(renderer)
        renderTarget.setArea(PyCEGUI.Rectf(0, 0, previewWidth, previewHeight))
        renderingSurface = PyCEGUI.RenderingSurface(renderTarget)

        widgetInstance = PyCEGUI.WindowManager.getSingleton().createWindow(widgetType, "preview")
        widgetInstance.setRenderingSurface(renderingSurface)
        # set it's size and position so that it shows up
        widgetInstance.setPosition(PyCEGUI.UVector2(PyCEGUI.UDim(0, 0), PyCEGUI.UDim(0, 0)))
        widgetInstance.setSize(PyCEGUI.USize(PyCEGUI.UDim(0, previewWidth), PyCEGUI.UDim(0, previewHeight)))
        # fake update to ensure everything is set
        widgetInstance.update(1)

        temporaryFBO = QtOpenGL.QGLFramebufferObject(previewWidth, previewHeight, GL.GL_TEXTURE_2D)
        temporaryFBO.bind()

        renderingSurface.invalidate()

        renderer.beginRendering()

        try:
            widgetInstance.render()

        finally:
            # no matter what happens we have to clean after ourselves!
            renderer.endRendering()
            temporaryFBO.release()
            PyCEGUI.WindowManager.getSingleton().destroyWindow(widgetInstance)

        return temporaryFBO.toImage()
