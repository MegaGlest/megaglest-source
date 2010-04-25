-- MojoSetup; a portable, flexible installation application.
--
-- Please see the file LICENSE.txt in the source's root directory.
--
--  This file written by Ryan C. Gordon.


-- !!! FIXME: go through the schema and ditch loki_setup things we don't want.

-- These are various things that need to be exposed to Lua, or are just
--  better written in Lua than C. All work will be done in the "MojoSetup"
--  table (for generic functionality) or the "Setup" table (for config file
--  elements), so the rest of the namespace is available to end-user code,
--  minus what the Lua runtime claims for itself.
--
-- This file is loaded and executed at MojoSetup startup. The Base Archive,
--  GUI, and Lua are initialized, the MojoSetup table is created and has some
--  initial CFunctions and such inserted. The localization script runs, and
--  then this code executes to do the heavy lifting. All Lua state should be
--  sane for the rest of the app once this script successfully completes.

local _ = MojoSetup.translate

-- Set up the garbage counter. Things that are done in possibly long-running
--  loops that create a lot of junk can optionally set a threshold and call
--  MojoSetup.incrementgarbagecounter() on each iteration. Whenever you hit
--  the threshold, garbage collection runs and resets the counter. This keeps
--  memory usage from spiraling out of control for pathological cases.
-- Note that the counter resets in MojoSetup.collectgarbage() itself, so you
--  don't have to worry about the counter accumulating and firing off extra
--  unnecessary collections if you happen to hit a loop at the wrong time.
MojoSetup.garbagecounter = 0
MojoSetup.garbagethreshold = 500

function MojoSetup.incrementgarbagecount()
    MojoSetup.garbagecounter = MojoSetup.garbagecounter + 1
    if MojoSetup.garbagecounter >= MojoSetup.garbagethreshold then
        MojoSetup.collectgarbage()
    end
end

-- Returns three elements: protocol, host, path
function MojoSetup.spliturl(url)
    return string.match(url, "^(.+://)(.-)/(.*)")
end


-- This is MojoSetup.stringtofile() for string tables.
function MojoSetup.stringtabletofile(t, dest, perms, len, callback)
    -- !!! FIXME: We could do this entirely in C to avoid a concatenated
    -- !!! FIXME:  string on the Lua heap.
    return MojoSetup.stringtofile(table.concat(t), dest, perms, len, callback)
end


-- This is handy for debugging.
function MojoSetup.dumptable(tabname, tab, depth)
    if depth == nil then  -- first call, before any recursion?
        local loglevel = MojoSetup.info.loglevel
        if (loglevel ~= "debug") and (loglevel ~= "everything") then
            return  -- don't spend time on this if there's no output...
        end
        depth = 1
    end

    if tabname ~= nil then
        if tab == nil then
            MojoSetup.logdebug(tabname .. " = nil")
            return
        else
            MojoSetup.logdebug(tabname .. " = {")
        end
    end

    local depthstr = ""
    for i=1,(depth*4) do
        depthstr = depthstr .. " "
    end

    if tab.MOJOSETUP_DUMPTABLE_ITERATED then
        MojoSetup.logdebug(depthstr .. "(...circular reference...)")
    else
        tab.MOJOSETUP_DUMPTABLE_ITERATED = true
        for k,v in pairs(tab) do
            if type(v) == "table" then
                MojoSetup.logdebug(depthstr .. tostring(k) .. " = {")
                MojoSetup.dumptable(nil, v, depth + 1)
                MojoSetup.logdebug(depthstr .. "}")
            else
                if k ~= "MOJOSETUP_DUMPTABLE_ITERATED" then
                    MojoSetup.logdebug(depthstr .. tostring(k) .. " = " .. tostring(v))
                end
            end
        end
        tab.MOJOSETUP_DUMPTABLE_ITERATED = nil
    end

    if tabname ~= nil then
        MojoSetup.logdebug("}")
    end
end


-- Our namespace for config API...
Setup = {}

local function schema_assert(test, fnname, elem, errstr)
    if not test then
        local msg = MojoSetup.format(_("BUG: Config %0 %1"),
                                     fnname .. "::" .. elem, errstr)
        MojoSetup.fatal(msg)
    end
end

local function mustExist(fnname, elem, val)
    schema_assert(val ~= nil, fnname, elem, _("must be explicitly specified"))
end

local function mustBeSomething(fnname, elem, val, elemtype)
    -- Can be nil...please use mustExist if this is a problem!
    if val ~= nil then
        if type(val) ~= elemtype then
            local msg = MojoSetup.format(_("must be %0"),
                                         MojoSetup.translate(elemtype))
            schema_assert(false, fnname, elem, msg)
        end
    end
end

local function mustBeString(fnname, elem, val)
    mustBeSomething(fnname, elem, val, "string")
end

local function mustBeBool(fnname, elem, val)
    mustBeSomething(fnname, elem, val, "boolean")
end

local function mustBeNumber(fnname, elem, val)
    mustBeSomething(fnname, elem, val, "number")
end

local function mustBeFunction(fnname, elem, val)
    mustBeSomething(fnname, elem, val, "function")
end

local function mustBeTable(fnname, elem, val)
    mustBeSomething(fnname, elem, val, "table")
end

local function cantBeEmpty(fnname, elem, val)
    -- Can be nil...please use mustExist if this is a problem!
    if val ~= nil then
        schema_assert(val ~= "", fnname, elem, _("can't be empty string"))
    end
end

local function mustBeStringOrTableOfStrings(fnname, elem, val)
    -- Can be nil...please use mustExist if this is a problem!
    if val ~= nil then
        local err = _("must be string or table of strings")
        if type(val) == "string" then
            val = { val }
        end
        schema_assert(type(val) == "table", fnname, elem, err)
        for k,v in pairs(val) do
            schema_assert(type(v) == "string", fnname, elem, err)
        end
    end
end

local function mustBeStringOrNumber(fnname, elem, val)
    -- Can be nil...please use mustExist if this is a problem!
    if val ~= nil then
        local t = type(val)
        schema_assert((t == "string") or (t == "number"), fnname, elem,
                        _("must be a string or number"))
    end
end

local function mustBeUrl(fnname, elem, val)
    mustBeString(fnname, elem, val)
    cantBeEmpty(fnname, elem, val)
    if (val ~= nil) then
        local prot,host,path = MojoSetup.spliturl(val)
        schema_assert(prot ~= nil,fnname,elem,_("URL doesn't have protocol"))
        schema_assert(host ~= nil,fnname,elem,_("URL doesn't have host"))
        schema_assert(path ~= nil,fnname,elem,_("URL doesn't have path"))
        prot = string.gsub(prot, "://$", "")
        local supported = MojoSetup.info.supportedurls[prot]
        schema_assert(supported,fnname,elem,_("URL protocol is unsupported"))
    end
end

local function mustBeSplashPosition(fnname, elem, val)
    mustBeString(fnname, elem, val)
    local valid = (val == nil) or (val == "right") or (val == "bottom") or
                  (val == "left") or (val == "top") or (val == "background");
    schema_assert(valid, fnname, elem, _("Splash position is invalid"))
end

local function mustBePerms(fnname, elem, val)
    mustBeString(fnname, elem, val)
    local valid = MojoSetup.isvalidperms(val)
    schema_assert(valid, fnname, elem, _("Permission string is invalid"))
end

local function mustBeValidProductKeyFormat(fnname, elem, val)
    -- Can be nil...please use mustExist if this is a problem!
    if val ~= nil then
        mustBeString(fnname, elem, val)
        cantBeEmpty(fnname, elem, val)
        local valid = (string.match(val, "^[X#%*%? %-]*$") ~= nil)
        schema_assert(valid, fnname, elem, _("invalid string"))
    end
end

local function sanitize(fnname, tab, elems)
    mustBeTable(fnname, "", tab)
    tab._type_ = string.lower(fnname) .. "s"   -- "Eula" becomes "eulas".
    for i,elem in ipairs(elems) do
        local child = elem[1]
        local defval = elem[2]

        if tab[child] == nil and defval ~= nil then
            tab[child] = defval
        end
        local j = 3
        while elem[j] do
            elem[j](fnname, child, tab[child])  -- will assert on problem.
            j = j + 1
        end
    end

    local notvaliderr = _("is not a valid property")
    for k,v in pairs(tab) do
        local found = false
        if k == "_type_" then
            found = true
        elseif (type(k) == "number") and (type(v) == "table") then
            found = true
        else
            for i,elem in ipairs(elems) do
                local child = elem[1]
                if (child == k) then
                    found = true
                    break
                end
            end
        end
        schema_assert(found, fnname, k, notvaliderr)
    end

    return tab
end

local function reform_schema_table(tab)
    local killlist = {}
    for k,v in pairs(tab) do
        local ktype = type(k)
        local vtype = type(v)
        if (ktype == "number") and (vtype == "table") and (v._type_ ~= nil) then
            -- add element to proper named array.
            local typestr = v._type_
            v._type_ = nil
            MojoSetup.logdebug("schema: reforming '" .. typestr .. "', '" .. k .. "'")
            if tab[typestr] == nil then
                tab[typestr] = { v }
            else
                table.insert(tab[typestr], v)
            end
            -- can't just set tab[k] to nil here, since it screws up pairs()...
            killlist[#killlist+1] = k
        elseif vtype == "table" then
            tab[k] = reform_schema_table(v)
        end
    end

    for i,v in ipairs(killlist) do
        tab[v] = nil
    end

    return tab
end


-- Actual schema elements are below...

function Setup.Package(tab)
    -- !!! FIXME: allow_uninstall
    -- !!! FIXME: write_manifest
    -- !!! FIXME: allow_uninstall must check write_manifest, write_manifest
    -- !!! FIXME:  must check for Lua parser support...or something like that.

    tab = sanitize("Package", tab,
    {
        { "vendor", nil, mustExist, mustBeString, cantBeEmpty },
        { "id", nil, mustExist, mustBeString, cantBeEmpty },
        { "disabled", nil, mustBeBool },
        { "description", nil, mustExist, mustBeString, cantBeEmpty },
        { "version", nil, mustExist, mustBeString, cantBeEmpty },
        { "destination", nil, mustBeString, cantBeEmpty },
        { "recommended_destinations", nil, mustBeStringOrTableOfStrings },
        { "precheck", nil, mustBeFunction },
        { "preflight", nil, mustBeFunction },
        { "preinstall", nil, mustBeFunction },
        { "postinstall", nil, mustBeFunction },
        { "splash", nil, mustBeString, cantBeEmpty },
        { "splashpos", nil, mustBeSplashPosition },
        { "url", nil, mustBeString, cantBeEmpty },
        { "once", true, mustBeBool },
        { "category", "Games", mustBeString, cantBeEmpty },
        { "promptoverwrite", true, mustBeBool },
        { "binarypath", nil, mustBeString, cantBeEmpty },
        { "updateurl", nil, mustBeUrl },
        { "superuser", false, mustBeBool },
        { "write_manifest", true, mustBeBool },
        { "support_uninstall", true, mustBeBool },
        { "preuninstall", nil, mustBeFunction },
        { "postuninstall", nil, mustBeFunction }
    })

    if MojoSetup.installs == nil then
        MojoSetup.installs = {}
    end

    tab._type_ = nil
    tab = reform_schema_table(tab)
    table.insert(MojoSetup.installs, tab)
    return tab

--[[
 promptbinaries When set to "yes", setup will create a checkbox
                to allow the user whether or not to create 
                a symbolic link to the binaries.

                This setting has no effect if nobinaries is "yes".

 meta       When this attribute is set to "yes", then setup will act as
            a meta-installer, i.e. it will allow the user to select a
            product and set-up will respawn itself for the selected
            product installation.
            See the section "About Meta-Installation" below.

 component  When this attribute is present, its value is the name of the component
            that will created by this installer. This means that the files will be added
            to an existing installation of the product, and that basic configuration parameters
            such as the installation path will be used from the previous installation.
            Currently setup is not able to update an existing component, thus if the component
            is detected as already installed the installation will fail.
            Important: This attribute can't be mixed with <component> elements.

 appbundle  (CARBON ONLY) If this is "yes", the destination folder does not include the product
            name as part of the path.  An application bundle is typically installed much like
            a single file would be...and so is treated as such.
            
            The app bundle path usually specified in the "path" attribute of the "files" element,
            or it is specified in the archives directly.

 manpages   If set to "yes", then the user will be prompted for the install pages installation path.
            Should be used when using the MANPAGE element described below.

 appbundleid  (CARBON ONLY) This string means that you are installing new files into an existing
              Application Bundle. If the bundle isn't found, the installation aborts, otherwise, all
              files are added relative to the base of the app bundle. The string specified here is
              the app bundle's CFBundleIdentifier entry in its Info.plist file. LaunchServices are
              used to find the bundle and the user can be prompted to manually locate the product
              if LaunchServices fails to find it. This is all MacOS-specific, and unrelated to the
              "component" attribute.

 appbundledesc (CARBON ONLY) This string is used with the "appbundleid" attribute...it's a human
               readable text describing the app bundle in question.
]]--

end

function Setup.Eula(tab)
    return sanitize("Eula", tab,
    {
        { "description", nil, mustExist, mustBeString, cantBeEmpty },
        { "source", nil, mustExist, mustBeString, cantBeEmpty },
    })
end

function Setup.Readme(tab)
    return sanitize("Readme", tab,
    {
        { "description", nil, mustExist, mustBeString, cantBeEmpty },
        { "source", nil, mustExist, mustBeString, cantBeEmpty },
    })
end

function Setup.ProductKey(tab)
    return sanitize("ProductKey", tab,
    {
        { "description", nil, mustExist, mustBeString, cantBeEmpty },
        { "format", nil, mustBeValidProductKeyFormat },
        { "verify", nil, mustBeFunction },
        { "destination", nil, mustBeString, cantBeEmpty },
    })
end

function Setup.Media(tab)
    return sanitize("Media", tab,
    {
        { "id", nil, mustExist, mustBeString, cantBeEmpty },
        { "description", nil, mustExist, mustBeString, cantBeEmpty },
        { "uniquefile", nil, mustExist, mustBeString, cantBeEmpty },
    })
end

function Setup.File(tab)
    return sanitize("File", tab,
    {
        { "source", nil, mustBeUrl },
        { "destination", nil, mustBeString, cantBeEmpty },
        { "wildcards", nil, mustBeStringOrTableOfStrings },
        { "filter", nil, mustBeFunction },
        { "allowoverwrite", nil, mustBeBool },
        { "permissions", nil, mustBePerms },
    })
end

function Setup.Option(tab)
    return sanitize("Option", tab,
    {
        { "value", false, mustBeBool },
        { "required", false, mustBeBool },
        { "disabled", false, mustBeBool },
        { "bytes", nil, mustExist, mustBeNumber },
        { "description", nil, mustExist, mustBeString, cantBeEmpty },
        { "tooltip", nil, mustBeString, cantBeEmpty },
    })
end

function Setup.OptionGroup(tab)
    return sanitize("OptionGroup", tab,
    {
        { "disabled", nil, mustBeBool },
        { "description", nil, mustExist, mustBeString, cantBeEmpty },
        { "tooltip", nil, mustBeString, cantBeEmpty },
    })
end

function Setup.DesktopMenuItem(tab)
    return sanitize("DesktopMenuItem", tab,
    {
        { "disabled", nil, mustBeBool },
        { "name", nil, mustExist, mustBeString, cantBeEmpty },
        { "genericname", nil, mustExist, mustBeString, cantBeEmpty },
        { "tooltip", nil, mustExist, mustBeString, cantBeEmpty },
        { "builtin_icon", nil, mustBeBool },
        { "icon", nil, mustExist, mustBeString, cantBeEmpty },
        { "commandline", nil, mustExist, mustBeString, cantBeEmpty },
        { "category", nil, mustExist, mustBeStringOrTableOfStrings },
        { "mimetype", nil, mustBeStringOrTableOfStrings },
    })
end

local function prepare_localization()
    -- Map some legacy language identifiers into updated equivalents.
    local lang_remap =
    {
        no = "nb",  -- "Norwegian" split into "Bokmal" (nb) and "Nynorsk" (nn)
    }

    local function sanity_check_localization_entry(str, translations)
        local maxval = -1;

        for val in string.gmatch(str, "%%.") do
            val = string.sub(val, 2)
            if string.match(val, "^[^%%0-9]$") ~= nil then
                MojoSetup.fatal("BUG: localization key ['" .. str .. "'] has invalid escape sequence.")
            end
            if val ~= "%" then
                local num = tonumber(val)
                if num > maxval then
                    maxval = num
                end
            end
        end

        for k,v in pairs(translations) do
            for val in string.gmatch(v, "%%.") do
                val = string.sub(val, 2)
                if string.match(val, "^[^%%0-9]$") ~= nil then
                    MojoSetup.fatal("BUG: '" .. k .. "' localization ['" .. v .. "'] has invalid escape sequence for translation of ['" .. str .. "'].")
                end
                if val ~= "%" then
                    if tonumber(val) > maxval then
                        MojoSetup.fatal("BUG: '" .. k .. "' localization ['" .. v .. "'] has escape sequence > max for translation of ['" .. str .. "'].")
                    end
                end
            end
        end
    end

    -- Build the translations table from the localizations table supplied in
    --  localizations.lua...
    if type(MojoSetup.localization) ~= "table" then
        MojoSetup.localization = nil
    end

    -- Merge the applocalization table into localization.
    if type(MojoSetup.applocalization) == "table" then
        if MojoSetup.localization == nil then
            MojoSetup.localization = {}
        end
        for k,v in pairs(MojoSetup.applocalization) do
            if MojoSetup.localization[k] == nil then
                MojoSetup.localization[k] = v  -- just take the whole table as-is.
            else
                -- This can add or overwrite entries...
                for lang,str in pairs(v) do
                    MojoSetup.localization[k][lang] = str
                end
            end
        end
    end
    MojoSetup.applocalization = nil  -- done with this; garbage collect it.

    if MojoSetup.localization ~= nil then
        local at_least_one = false
        local locale = MojoSetup.info.locale
        local lang = string.gsub(locale, "_%w+", "", 1)  -- make "en_US" into "en"

        if lang_remap[lang] ~= nil then
            lang = lang_remap[lang]
        end

        MojoSetup.translations = {}
        for k,v in pairs(MojoSetup.localization) do
            if MojoSetup.translations[k] ~= nil then
                MojoSetup.fatal("BUG: Duplicate localization key ['" .. k .. "']")
            end
            if type(v) == "table" then
                sanity_check_localization_entry(k, v)
                if v[locale] ~= nil then
                    MojoSetup.translations[k] = v[locale]
                    at_least_one = true
                elseif v[lang] ~= nil then
                    MojoSetup.translations[k] = v[lang]
                    at_least_one = true
                end
            end
        end

        -- Delete the table if there's no actual useful translations for this run.
        if (not at_least_one) then
            MojoSetup.translations = nil
        end

        -- This is eligible for garbage collection now. We're done with it.
        MojoSetup.localization = nil
    end
end


-- Mainline...

MojoSetup.loginfo("MojoSetup Lua initialization at " .. MojoSetup.date())
MojoSetup.loginfo("buildver: " .. MojoSetup.info.buildver)
MojoSetup.loginfo("locale: " .. MojoSetup.info.locale)
MojoSetup.loginfo("platform: " .. MojoSetup.info.platform)
MojoSetup.loginfo("arch: " .. MojoSetup.info.arch)
MojoSetup.loginfo("ostype: " .. MojoSetup.info.ostype)
MojoSetup.loginfo("osversion: " .. MojoSetup.info.osversion)
MojoSetup.loginfo("ui: " .. MojoSetup.info.ui)
MojoSetup.loginfo("loglevel: " .. MojoSetup.info.loglevel)

MojoSetup.loginfo("command line:")
for i,v in ipairs(MojoSetup.info.argv) do
    MojoSetup.loginfo("  " .. i .. ": " .. v)
end

--MojoSetup.loginfo(MojoSetup.info.license)
--MojoSetup.loginfo(MojoSetup.info.lualicense)

-- These scripts are optional, but hopefully exist...
MojoSetup.runfile("localization")
MojoSetup.runfile("app_localization")
prepare_localization()

-- okay, we're initialized!

-- end of mojosetup_init.lua ...

