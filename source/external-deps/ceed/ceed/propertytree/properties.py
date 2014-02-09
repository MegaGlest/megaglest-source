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

"""The basic properties.

Property -- The base class for all properties, has name, value, etc.
PropertyCategory -- A category, groups properties together.
PropertyEvent -- Custom event implementation for the Property system.
PropertyEventSubscription -- A subscription to a PropertyEvent.
StringWrapperProperty -- Special purpose property to edit the string representation of another property.
MultiPropertyWrapper -- Special purpose property used to group many properties of the same type in one.
EnumValue -- Interface for properties that have a predetermined list of possible values, like enums.
"""

import abc
import operator

from collections import OrderedDict

from . import utility

class PropertyCategory(object):
    """A category for properties.
    Categories have a name and hold a list of properties.
    """
    def __init__(self, name):
        """Initialise the instance with the specified name."""
        self.name = name
        self.properties = OrderedDict()

    @staticmethod
    def categorisePropertyList(propertyList, unknownCategoryName="Unknown"):
        """Given a list of properties, create categories and add the
        properties to them based on their 'category' field.

        The unknownCategoryName is used for a category that holds all
        properties that have no 'category' specified.
        """
        categories = {}
        for prop in propertyList:
            catName = prop.category if prop.category else unknownCategoryName
            if not catName in categories:
                categories[catName] = PropertyCategory(catName)
            category = categories[catName]
            category.properties[prop.name] = prop

        return categories

    def sortProperties(self, reverse=False):
        self.properties = OrderedDict(sorted(self.properties.items(), key=lambda t: t[0], reverse=reverse))

class PropertyEventSubscription(object):
    """A subscription to a PropertyEvent."""

    def __init__(self, callback, excludedReasons=None, includedReasons=None):
        """Initialise the subscription to call the specified callback.

        callback -- The callable to call, will take the arguments specified
                    during the call to PropertyEvent.trigger().
        excludedReasons -- If the 'reason' argument of PropertyEvent.trigger()
                    is in this set, the callback will not be called that time.
                    Has higher priority than 'includedReasons'.
        includedReasons -- Like 'excludedReasons' but specifies the values that
                    will cause the callback to be called. If None, all values
                    are valid.
                    Has lower priority than 'excludedReasons'
        """
        assert callable(callback), "Callback argument is not callable."
        self.callback = callback
        self.excludedReasons = excludedReasons
        self.includedReasons = includedReasons

    def isValidForReason(self, reason):
        """Return True if the callback should be called for the specified reason."""
        if (self.includedReasons is None) or (reason in self.includedReasons):
            if (self.excludedReasons is None) or (not reason in self.excludedReasons):
                return True
        return False

    def __hash__(self):
        """Return the hash of the callback.

        The __hash__, __eq__ and __ne__ methods have been re-implemented so that it's
        possible to manage this subscription via it's callback, without holding a
        reference to the subscription instance.
        """
        return hash(self.callback)

    def __eq__(self, other):
        """Return True if this subscription's callback is equal to the
        'other' argument (subscription or callback).

        See the notes on '__hash__'
        """
        if isinstance(other, PropertyEventSubscription):
            return self.callback == other.callback
        if callable(other) and self.callback == other:
            return True
        return False

    def __ne__(self, other):
        """Inverted '__eq__'.

        See the notes on '__hash__' and '__eq__'
        """
        return not self.__eq__(other)

class PropertyEvent(object):

    def __init__(self, maxRecursionDepth=None, assertOnDepthExceeded=False):
        """Custom event.

        An event can have subscribers and can guard against recursion.

        maxRecursionDepth -- 0 (zero) means no recursion at all, None means unlimited.
        assertOnDepthExcessed -- Will call 'assert False' if the recursion depth exceeds the maximum.
        """
        self.subscriptions = set()
        self.maxRecursionDepth = maxRecursionDepth
        self.assertOnDepthExceeded = assertOnDepthExceeded
        self.recursionDepth = -1

    def clear(self):
        """Remove all subscriptions."""
        self.subscriptions.clear()

    def subscribe(self, callback, excludedReasons=None, includedReasons=None):
        """Shortcut to create and add a subscription.

        Multiple subscriptions with the same callback are not supported.
        """
        sub = PropertyEventSubscription(callback, excludedReasons, includedReasons)
        self.subscriptions.add(sub)
        return sub

    def unsubscribe(self, eventSubOrCallback, safe=True):
        """Remove a subscription.

        eventSubOrCallback -- An event subscription instance or the callback
                            of a subscription to remove.
        safe -- If True, check that the subscription exists to avoid
                KeyErrors.
        """
        if (not safe) or (eventSubOrCallback in self.subscriptions):
            self.subscriptions.remove(eventSubOrCallback)

    def trigger(self, sender, reason, *args, **kwargs):
        """Raise the event by calling all subscriptions that are valid
        for the specified reason.
        """
        try:
            # increase the counter first, so the first call sets it to 0.
            self.recursionDepth += 1
            # if there's a max set and we're over it
            if (self.maxRecursionDepth is not None) and (self.recursionDepth > self.maxRecursionDepth):
                # if we want assertions
                if self.assertOnDepthExceeded:
                    assert False, "Excessive recursion detected ({} when the max is {}).".format(self.recursionDepth, self.maxRecursionDepth)
                # bail out
                return

            for subscription in self.subscriptions:
                if subscription.isValidForReason(reason):
                    subscription.callback(sender, reason, *args, **kwargs)
        finally:
            self.recursionDepth -= 1

class Property(object):
    """A property which is the base for all properties.

    The most important fields of a property are 'name' and 'value'.
    A property instance should be able to return the type of its value
    and has a simple mechanism to notify others when its value changes.
    """

    # TODO: Is it necessary to set this to False for releases or is
    # the release made with optimisations?
    DebugMode = __debug__

    class ChangeValueReason(object):
        #pylint: disable-msg=R0903
        # too few public methods - it's an enum dammit
        Unknown = "Unknown"
        ComponentValueChanged = "ComponentValueChanged"
        ParentValueChanged = "ParentValueChanged"
        Editor = "Editor"
        InnerValueChanged = "InnerValueChanged"
        WrapperValueChanged = "WrapperValueChanged"

    class ComponentsUpdateType(object):
        #pylint: disable-msg=R0903
        # too few public methods - it's an enum dammit
        AfterCreate = 0
        BeforeDestroy = 1

    def __init__(self, name, value=None, defaultValue=None, category=None, helpText=None, readOnly=False, editorOptions=None, createComponents=True):
        """Initialise an instance using the specified parameters.

        The 'category' field is usually a string that is used by
        'PropertyCategory.categorisePropertyList()' to place the
        property in a category.

        In the default implementation, the 'editorOptions' argument
        should be a dictionary of options that will be passed to the
        editor of the property's value. See getEditorOption().
        """

        # prevent values that are Property instances themselves;
        # these have to be added as components if needed.
        if isinstance(value, Property):
            raise TypeError("The 'value' argument of the '%s' Property can't be a Property." % name)

        self.name = unicode(name) # make sure it's string
        self.value = value
        self.defaultValue = defaultValue
        self.category = category
        self.helpText = helpText
        self.readOnly = readOnly
        self.editorOptions = editorOptions

        # Events
        self.valueChanged = PropertyEvent(0, Property.DebugMode)
        self.componentsUpdate = PropertyEvent(0, Property.DebugMode)

        # Create components
        self.components = None
        if createComponents:
            self.createComponents()

    def finalise(self):
        """Perform any cleanup necessary."""
        self.finaliseComponents()
        self.valueChanged.clear()

    def createComponents(self):
        """Create an OrderedDict with the component-properties that make up
        this property.

        The default implementation is incomplete,
        it simply subscribes to the components' valueChanged event
        to be able to update this property's own value when
        their value changes. It also calls raiseComponentsUpdate().

        Implementors should create the components, make sure they
        are accessible from getComponents() and then call this
        as super().
        """
        components = self.getComponents()
        if components:
            for comp in components.values():
                comp.valueChanged.subscribe(self.componentValueChanged, {self.ChangeValueReason.ParentValueChanged})
            self.componentsUpdate.trigger(self, self.ComponentsUpdateType.AfterCreate)

    def getComponents(self):
        """Return the OrderedDict of the components, or None."""
        return self.components

    def finaliseComponents(self):
        """Clean up components.

        The default implementation is usually enough, it will
        unsubscribe from the components' events, finalise them
        and clear the components field. It will also call
        raiseComponentsUpdate().
        """
        components = self.getComponents()
        if components:
            self.componentsUpdate.trigger(self, self.ComponentsUpdateType.BeforeDestroy)
            for comp in components.values():
                comp.valueChanged.unsubscribe(self.componentValueChanged)
                comp.finalise()
            components.clear()

    def valueType(self):
        """Return the type of this property's value.
        The default implementation simply returns the Python type()
        of the current value.
        """
        return type(self.value)

    def hasDefaultValue(self):
        return self.value == self.defaultValue

    def valueToString(self):
        """Return a string representation of the current value.
        """
        if self.value is not None:
            if issubclass(type(self.value), Property):
                return self.value.valueToString()
            return unicode(self.value)

        return ""

    def isStringRepresentationEditable(self):
        """Return True if the property supports editing its string representation,
        in addition to editing its components."""
        #pylint: disable-msg=R0201
        # "Method could be a function"
        # No, it couldn't, it's meant to be overriden but provides
        # the default implementation.
        return False

    def tryParse(self, strValue):
        """Parse the specified string value and return
        a tuple with the parsed value and a boolean
        specifying success or failure.
        """
        pass

    def setValue(self, value, reason=ChangeValueReason.Unknown):
        """Change the current value to the one specified
        and notify all subscribers of the change. Return True
        if the value was changed, otherwise False.

        If the property has components, this method is responsible
        for updating their values, if necessary. The default
        implementation does this by calling 'self.updateComponents()'.

        If the property is a wrapper property (a proxy to another
        property), this method tries to set the value of the
        inner property first and bails out if it can't. The default
        implementation does this by calling 'self.tryUpdateInner()'.
        """

        #if Property.DebugMode:
        #    print("{} ({}).setValue: Value={}, Changed={}, Reason={}".format(self.name, self.__class__.__name__, str(value), not value == self.value, reason))

        # Really complicated stuff :/
        #
        # This used to check if the new value is different than the current and
        # only run if it is.
        #    if self.value != value:
        # It has been removed because there are cases where a component property
        # modifies the parent's value directly (i.e. they use the same instance of
        # a class) and the check would return false because the parent's value
        # has already been changed. The problem would be that the parent wouldn't
        # update it's inner values and would not trigger the valueChanged event
        # even though it has changed.
        #
        # The check, however, was a nice way to prevent infinite recursion bugs
        # because it would stabilise the model sooner or later. We've worked around
        # that by:
        #    a) fixing the bugs :D
        #    b) the events are set to detect and disallow recursion
        #
        if True: #self.value != value:

            # Do not update inner properties if our own value was changed
            # in response to an inner property's value having changed.
            if reason != self.ChangeValueReason.InnerValueChanged:
                if not self.tryUpdateInner(value, self.ChangeValueReason.WrapperValueChanged):
                    return False

            self.value = value

            # Do not update components if our own value was changed
            # in response to a component's value having changed.
            if reason != self.ChangeValueReason.ComponentValueChanged:
                self.updateComponents(self.ChangeValueReason.ParentValueChanged)

            # This must be raised after updating the components because handlers
            # of the event will probably want to use the components.
            self.valueChanged.trigger(self, reason)

            return True

        return False

    def updateComponents(self, reason=ChangeValueReason.Unknown):
        """Update this property's components (if any) to match this property's value."""
        pass

    def tryUpdateInner(self, newValue, reason=ChangeValueReason.Unknown):
        """Try to update the inner properties (if any) to the new value.

        Return True on success, False on failure.
        """
        #pylint: disable-msg=W0613,R0201
        # Unused arguments, method could be a function: This is meant
        # to be overriden but provides a default implementation.
        return True

    def componentValueChanged(self, component, reason):
        """Callback called when a component's value changes.

        This will generally call setValue() to update this instance's
        value in response to the component's value change. If this
        happens, the call to setValue() should use ChangeValueReason.ComponentValueChanged.

        See the DictionaryProperty for a different implementation.
        """
        pass

    def getEditorOption(self, path, defaultValue=None):
        """Get the value of the editor option at the specified path string.

        Return 'defaultValue' if the option/path can't be found.
        """

        return utility.getDictionaryTreePath(self.editorOptions, path, defaultValue)

class StringWrapperProperty(Property):
    """Special purpose property used to wrap the string value
    of another property so it can be edited.
    """

    def __init__(self, innerProperty, instantApply=False):
        super(StringWrapperProperty, self).__init__(innerProperty.name + "__wrapper__",
                                                    value = innerProperty.valueToString(),
                                                    editorOptions = { "instantApply": instantApply }
                                                    )
        self.innerProperty = innerProperty
        self.innerProperty.valueChanged.subscribe(self.cb_innerValueChanged, {Property.ChangeValueReason.WrapperValueChanged})

    def finalise(self):
        self.innerProperty.valueChanged.unsubscribe(self.cb_innerValueChanged)
        super(StringWrapperProperty, self).finalise()

    def cb_innerValueChanged(self, innerProperty, reason):
        self.setValue(self.innerProperty.valueToString(), Property.ChangeValueReason.InnerValueChanged)

    def tryUpdateInner(self, newValue, reason=Property.ChangeValueReason.Unknown):
        value, valid = self.innerProperty.tryParse(newValue)
        if valid:
            # set the value to the inner property and return True
            self.innerProperty.setValue(value, reason)
            return True

        return False

class MultiPropertyWrapper(Property):
    """Special purpose property used to group many properties of the same type in one."""

    @classmethod
    def gatherValueData(cls, properties):
        """Go through the properties and return the unique values and default values found,
        ordered by use count.
        """
        values = dict()
        defaultValues = dict()

        for prop in properties:
            value = prop.value
            if value in values:
                values[value] += 1
            else:
                values[value] = 1

            defaultValue = prop.defaultValue
            if defaultValue in defaultValues:
                defaultValues[defaultValue] += 1
            else:
                defaultValues[defaultValue] = 1

        values = [value for value, _ in sorted(values.iteritems(), key = operator.itemgetter(1), reverse = True)]
        defaultValues = [value for value, _ in sorted(defaultValues.iteritems(), key = operator.itemgetter(1), reverse = True)]

        return values, defaultValues

    def __init__(self, templateProperty, innerProperties, takeOwnership):
        """Initialise the instance with the specified properties.

        templateProperty -- A newly created instance of a property of the same type
                            as the properties that are to be wrapped. This should be
                            already initialised with the proper name, category, settings,
                            etc. It will be used internally and it will be owned by
                            this wrapper.
        innerProperties -- The list of properties to wrap, must have at least one
                            property and all properties must have the same type
                            or an error is raised.
        takeOwnership -- Boolean flag indicating whether this wrapper should take
                            ownership of the inner properties, destroying them when
                            it is destroyed.
        """

        if len(innerProperties) == 0:
            raise ValueError("The 'innerProperties' argument has no elements; at least one is required.")

        # ensure all properties have the same valueType
        valueType = templateProperty.valueType()
        for prop in innerProperties:
            if prop.valueType() != valueType:
                raise ValueError("The valueType() of all the inner properties must be the same.")

        self.templateProperty = templateProperty
        self.innerProperties = innerProperties
        self.ownsInnerProperties = takeOwnership
        self.allValues, self.allDefaultValues = self.gatherValueData(self.innerProperties)

        self.templateProperty.setValue(self.allValues[0])
        self.templateProperty.defaultValue = self.allDefaultValues[0]

        # initialise with the most used value and default value
        super(MultiPropertyWrapper, self).__init__(name = self.templateProperty.name,
                                                   category = self.templateProperty.category,
                                                   helpText = self.templateProperty.helpText,
                                                   value = self.templateProperty.value,
                                                   defaultValue = self.templateProperty.defaultValue,
                                                   readOnly = self.templateProperty.readOnly,
                                                   editorOptions = self.templateProperty.editorOptions
                                                   )

        # subscribe to the valueChanged event of the inner properties
        # to update our value if the value of one of them changes.
        for prop in self.innerProperties:
            prop.valueChanged.subscribe(self.cb_innerValueChanged, {Property.ChangeValueReason.WrapperValueChanged})
        # subscribe to the valueChanged event of the template property.
        self.templateProperty.valueChanged.subscribe(self.cb_templateValueChanged, {Property.ChangeValueReason.WrapperValueChanged, Property.ChangeValueReason.ParentValueChanged})

    def finalise(self):
        for prop in self.innerProperties:
            prop.valueChanged.unsubscribe(self.cb_innerValueChanged)
            # if we own it, finalise it
            if self.ownsInnerProperties:
                prop.finalise()
        self.innerProperties = None

        self.templateProperty.valueChanged.unsubscribe(self.cb_templateValueChanged)
        self.templateProperty.finalise()
        self.templateProperty = None

        super(MultiPropertyWrapper, self).finalise()

    def createComponents(self):
        # We don't call super because we don't need to subscribe
        # to the templateProperty's components!
        self.componentsUpdate.trigger(self, self.ComponentsUpdateType.AfterCreate)

    def getComponents(self):
        return self.templateProperty.getComponents()

    def finaliseComponents(self):
        # We don't call super because we don't actually own any
        # components, they're owned by the templateProperty
        self.componentsUpdate.trigger(self, self.ComponentsUpdateType.BeforeDestroy)

    def valueType(self):
        return self.templateProperty.valueType()

    def hasDefaultValue(self):
        return self.templateProperty.hasDefaultValue()

    def isStringRepresentationEditable(self):
        return self.templateProperty.isStringRepresentationEditable()

    def tryParse(self, strValue):
        return self.templateProperty.tryParse(strValue)

    def updateComponents(self, reason=Property.ChangeValueReason.Unknown):
        self.templateProperty.setValue(self.value, reason)

    def valueToString(self):
        if len(self.allValues) == 1:
            return super(MultiPropertyWrapper, self).valueToString()
        return "<multiple values>"

    def cb_templateValueChanged(self, sender, reason):
        # The template's value is changed when it's components change
        # or when we change it's value directly to match our own.
        # Unnecessary? if reason == Property.ChangeValueReason.ComponentValueChanged:
        self.setValue(self.templateProperty.value, reason)

    def tryUpdateInner(self, newValue, reason=Property.ChangeValueReason.Unknown):
        for prop in self.innerProperties:
            prop.setValue(newValue, reason)

        return True

    def cb_innerValueChanged(self, innerProperty, reason):
        self.allValues, self.allDefaultValues = self.gatherValueData(self.innerProperties)

        self.setValue(self.allValues[0], Property.ChangeValueReason.InnerValueChanged)

class EnumValue(object):
    """Interface for properties that have a predetermined list
    of possible values, like enums.

    Used by the EnumValuePropertyEditor (combo box).
    """
    #pylint: disable-msg=R0903
    # too few public methods (1/2) - it's an interface

    __metaclass__ = abc.ABCMeta

    @abc.abstractmethod
    def getEnumValues(self):
        """Return a dictionary of all possible values and their display names."""
        pass
