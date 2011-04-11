local GAME_INSTALL_SIZE = 680000000;
local GAME_VERSION = "3.5.0-beta2";

local _ = MojoSetup.translate

Setup.Package
{
    vendor = "megaglest.org",
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
        MojoSetup.launchbrowser("http://www.megaglest.org")
    end,

    Setup.Eula
    {
        description = _("Megaglest License"),
        source = _("docs/LICENSE")
    },

    Setup.Readme
    {
        description = _("Megaglest README"),
        source = _("docs/README")
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
	    source = "base:///mgdata.tar.xz",
        },

        Setup.DesktopMenuItem
        {
            disabled = false,
            name = _("MegaGlest v" .. GAME_VERSION),
            genericname = _("MegaGlest"),
            tooltip = _("A real time strategy game."),
            builtin_icon = false,
            icon = "megaglest.ico",
            commandline = "%0/megaglest",
            category = "Game;StrategyGame"
        },

        Setup.DesktopMenuItem
        {
            disabled = false,
            name = _("MegaGlest Map Editor v" .. GAME_VERSION),
            genericname = _("MegaGlest"),
            tooltip = _("A real time strategy game."),
            builtin_icon = false,
            icon = "editor.ico",
            commandline = "%0/start_megaglest_mapeditor",
            category = "Game;StrategyGame"
        },

        Setup.DesktopMenuItem
        {
            disabled = false,
            name = _("MegaGlest G3D Model Viewer v" .. GAME_VERSION),
            genericname = _("MegaGlest"),
            tooltip = _("A real time strategy game."),
            builtin_icon = false,
            icon = "g3dviewer.ico",
            commandline = "%0/start_megaglest_g3dviewer",
            category = "Game;StrategyGame"
        },

        Setup.DesktopMenuItem
        {
            disabled = false,
            name = _("MegaGlest Uninstall v" .. GAME_VERSION),
            genericname = _("MegaGlest"),
            tooltip = _("A real time strategy game."),
            builtin_icon = false,
	    icon = "megaglest_uninstall.ico",
            commandline = "%0/uninstall-megaglest.sh",
            category = "Game;StrategyGame"
        }
		
    }
}

-- end of config.lua ...

