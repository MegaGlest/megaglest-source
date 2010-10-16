local GAME_INSTALL_SIZE = 510000000;
local GAME_VERSION = "3.3.7.2";

local _ = MojoSetup.translate

Setup.Package
{
    vendor = "glest.org",
    id = "megaglest",
    description = _("Mega Glest v" .. GAME_VERSION),
    version = GAME_VERSION,
    splash = "glestforumsheader.bmp",
    superuser = false,
    write_manifest = true,
    support_uninstall = true,
    recommended_destinations =
    {
        MojoSetup.info.homedir,
        "/opt/games",
        "/usr/local/games"
    },

    postinstall = function(package)
        MojoSetup.launchbrowser("http://www.glest.org/glest_board/index.php?topic=4930.0")
    end,

    Setup.Eula
    {
        description = _("Megaglest License"),
        source = _("docs/license.txt")
    },

    Setup.Readme
    {
        description = _("Megaglest README"),
        source = _("docs/readme.txt")
    },

    Setup.Option
    {
        value = true,
        required = true,
        disabled = false,
        bytes = GAME_INSTALL_SIZE,
        description = _("Mega Glest v" .. GAME_VERSION),

        Setup.File
        {
            -- Just install everything we see...
        },

        Setup.DesktopMenuItem
        {
            disabled = false,
            name = _("MegaGlest"),
            genericname = _("MegaGlest"),
            tooltip = _("A real time strategy game."),
            builtin_icon = false,
            icon = "megaglest.ico",
            commandline = "%0/glest",
            category = "Game"
        },

        Setup.DesktopMenuItem
        {
            disabled = false,
            name = _("MegaGlest Map Editor"),
            genericname = _("MegaGlest"),
            tooltip = _("A real time strategy game."),
            builtin_icon = false,
            icon = "editor.ico",
            commandline = "%0/editor",
            category = "Game"
        },

        Setup.DesktopMenuItem
        {
            disabled = false,
            name = _("MegaGlest G3D Model Viewer"),
            genericname = _("MegaGlest"),
            tooltip = _("A real time strategy game."),
            builtin_icon = false,
            icon = "g3dviewer.ico",
            commandline = "%0/g3dviewer",
            category = "Game"
        }
		
    }
}

-- end of config.lua ...

