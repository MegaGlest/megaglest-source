-- MojoSetup; a portable, flexible installation application.
--
-- Please see the file LICENSE.txt in the source's root directory.
--
--  This file written by Ryan C. Gordon.


-- This is a setup file. Lines that start with "--" are comments.
--  This config file is actually just Lua code. Even though you'll only
--  need a small subset of the language, there's a lot of flexibility
--  available to you if you need it.   http://www.lua.org/
--
-- All functionality supplied by the installer is encapsulated in either the
--  "Setup" or "MojoSetup" table, so you can use any other symbol name without
--  namespace clashes, assuming it's not a Lua keyword or a symbol supplied
--  by the standard lua libraries.
--
-- So here's the actual configuration...we used loki_setup's xml schema
--  as a rough guideline.

-- You can define functions like in any other Lua script. For example, here's
--  a function that figures out how many bytes are in a megabyte, so you don't
--  have to list exact values in Setup.Option's "bytes" attribute.
local function megabytes(num)
    return num * 1024 * 1024
end

-- And naturally, functions can build on others. It's a programming language,
--  after all! But if you don't want to screw with programming, you can treat
--  this strictly as a config file. This just gives you flexibility if you
--  need it.
local function gigabytes(num)
    return megabytes(num) * 1024
end

Setup.Package
{
    vendor = "com.mycompany",
    id = "mygame",
    description = "My Game",
    version = "1.0",
    splash = "splash.jpg",
    superuser = false,
    destination = "/usr/local/bin",
    recommended_destinations = { "/opt/games", "/usr/local/games" },
    updateurl = "http://localhost/updates/",
    write_manifest = true,
    support_uninstall = true,

    -- Things named Setup.Something are internal functions we supply.
    --  Generally these return the table you pass to them, but they
    --  may sanitize the values, add defaults, and verify the data.

    -- End User License Agreement(s). You can specify multiple
    --  Setup.Eula sections here.
    --  Also, Note the "translate" call.
    --  This shows up as the first thing the user sees, and must
    --  agree to before anything goes forward. You could put this
    --  in the base Setup.Option and they won't be shown it until
    --  installation is about to start, if you would rather
    --  defer this until necessary and/or show the README first.
    Setup.Eula
    {
        description = "My Game License",
        source = MojoSetup.translate("MyGame_EULA.html")
    },

    -- README file(s) to show and install.
    Setup.Readme
    {
        description = "My Game README",
        source = MojoSetup.translate("README.html"),
    },

    -- Specify media (discs) we may need for this install and how to find them.
    Setup.Media
    {
        id = "cd1",
        description = "MyGame CD 1",
        uniquefile = "Sound/blip.wav"
    },

    Setup.Media
    {
        id = "cd2",
        description = "MyGame CD 2",
        uniquefile = "Maps/town.map"
    },

    -- Specify chunks to install...optional or otherwise.
    Setup.Option
    {
        value = true,
        required = true,
        disabled = false,
        bytes = megabytes(600),
        description = "Base Install",

        -- Install a desktop menu item with the base install.
        Setup.DesktopMenuItem
        {
            disabled = false,
            name = "My Game",
            genericname = "Shoot-em up",
            tooltip = "A game of alien hunting.",
            builtin_icon = false,
            icon = "icon.png",  -- relative to the dest; you must install it!
            commandline = "command-line",
            category = "Game",
            mimetype = { 'application/x-mygame-map', 'application/x-mygame-url' },
        },

        -- File(s) to install with this option.
        Setup.File
        {
            -- source can be a directory, an archive, or a supported URL.
            --  You can use "media://" to get data from a disc that the user
            --  will be prompted to insert. Everything in the source will
            --  be installed, but the "wildcards" and "filter" attributes
            --  can be used to cull the archive's contents.
            source = "media://cd1/Maps/m.zip",

            -- This is a directory where files will be installed from this
            --  source. If this isn't specified, the directory tree structure
            --  in the source will be recreated for the installation.
            -- You can change the destination on a per-file basis using
            --  the filter attribute instead. It overrides this attribute,
            --  but the parameter passed to the filter will use this value
            --  if it exists.
            destination = "MyGame/MyGame.app",

            -- Files in here need to match at least one wildcard to be
            --  installed. If they pass here, they go to the filter function.
            -- This can be a single string or a table of strings.
            wildcards = { "Single/*/*.map", "Multi/*/*.map" },

            -- You can optionally assign a lua function...we'll call this for
            --  each file to see if we should avoid installing it. It returns
            --  nil to drop the file from the install list, or a string of
            --  the new install destination...you can return the original
            --  string to just pass it through for installation, or
            --  use this opportunity to rename a file on the fly.
            filter = function(fn)
                if fn == "Single/x/dontinstall.map" then return nil end
                return fn
            end
        },

        -- Here's a suboption that has it's own EULA.
        Setup.Option
        {
            value = true,
            required = false,
            disabled = false,
            bytes = megabytes(1),
            description = "PunkBuster support",

            Setup.Eula
            {
                description = "Punkbuster License",
                source = MojoSetup.translate("PunkBuster_EULA.html"),
            },

            Setup.File
            {
                source = "media://cd1/pb.zip",
            },
        },

        -- Radio buttons.
        Setup.OptionGroup
        {
            disabled = false,
            description = "Language",
            Setup.Option
            {
                value = string.match(MojoSetup.info.locale, "^en_") ~= nil,
                bytes = megabytes(10),
                description = "English",
                Setup.File { source = "base:///Lang/English.zip" },
            },
            Setup.Option
            {
                value = string.match(MojoSetup.info.locale, "^fr_") ~= nil,
                bytes = 0,
                description = "French",
                Setup.OptionGroup
                {
                    disabled = false,
                    description = "French locale",
                    Setup.Option
                    {
                        value = (MojoSetup.info.locale == fr_CA),
                        bytes = megabytes(10),
                        description = "French Canadian",
                        Setup.File { source = "base:///Lang/FrenchCA.zip" },
                    },
                    Setup.Option
                    {
                        value = (MojoSetup.info.locale ~= fr_CA),
                        bytes = megabytes(10),
                        description = "Generic French",
                        Setup.File { source = "base:///Lang/French.zip" },
                    },
                },
            },
            Setup.Option
            {
                value = string.match(MojoSetup.info.locale, "^de_") ~= nil,
                bytes = megabytes(10),
                description = "German",
                Setup.File { source = "base:///Lang/German.zip" },
            },
        },
    },

    Setup.Option
    {
        value = true,
        required = false,
        disabled = false,
        bytes = 19384292,
        description = "Downloadable extras",

        -- File(s) to install.
        Setup.File
        {
            destination = "MyGame/MyGame.app",
            source = "http://hostname.dom/extras/extras.zip",
        },
    },
}

-- end of config.lua ...

