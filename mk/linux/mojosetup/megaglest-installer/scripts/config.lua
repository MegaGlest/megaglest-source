local GAME_INSTALL_SIZE = 680000000;
local GAME_VERSION = "3.7.0-beta1";

local _ = MojoSetup.translate

Setup.Package
{
    vendor = "megaglest.org",
    id = "megaglest",
    description = _("MegaGlest v" .. GAME_VERSION),
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

    precheck = function(package)
	-- MojoSetup.msgbox('Test#1', MojoSetup.info.homedir .. '/megaglest/uninstall-megaglest.sh')

	local previousPath = ''
        if MojoSetup.platform.exists(MojoSetup.info.homedir .. '/megaglest/uninstall-megaglest.sh') then
		previousPath = MojoSetup.info.homedir .. '/megaglest/'
	elseif MojoSetup.platform.exists('/opt/games/megaglest/uninstall-megaglest.sh') then
		previousPath = '/opt/games/megaglest/'
	elseif MojoSetup.platform.exists('/usr/local/games/megaglest/uninstall-megaglest.sh') then
		previousPath = '/usr/local/games/megaglest/'
	end

	if previousPath ~= '' then
		if MojoSetup.promptyn(_("MegaGlest Uninstall Title"), _("MegaGlest Uninstall Prompt") .. '\n\n[' .. previousPath .. ']') then
			os.execute(previousPath .. 'uninstall-megaglest.sh')
		end

	end
    end,

    preinstall = function(package)
	local previousPath = ''
        if MojoSetup.platform.exists(MojoSetup.info.homedir .. '/megaglest/mydata/') then
		previousPath = MojoSetup.info.homedir .. '/megaglest/'
	elseif MojoSetup.platform.exists('/opt/games/megaglest/mydata/') then
		previousPath = '/opt/games/megaglest/'
	elseif MojoSetup.platform.exists('/usr/local/games/megaglest/mydata/') then
		previousPath = '/usr/local/games/megaglest/'
	end

	-- Move mod data folder to new location if we find it
	if previousPath ~= '' then
		local instPathData =  MojoSetup.info.homedir .. '/.megaglest/'
		local instPath =  instPathData
		-- MojoSetup.msgbox('Moving mod folder','About to move mod folder from [' .. previousPath .. '] to [' .. instPath .. ']')

		os.execute('mkdir ' .. instPathData)
		os.execute('mv ' .. previousPath .. 'mydata/* ' .. instPath)
		os.execute('mv ' .. previousPath .. 'glestuser.ini ' .. instPath .. 'glestuser.ini')
	end
    end,

    postinstall = function(package)
	if MojoSetup.promptyn(_("MegaGlest Visit Website Title"), _("MegaGlest Visit Website Prompt")) then
        	MojoSetup.launchbrowser("http://megaglest.org/get-started.html")
	end
    end,

    postuninstall = function(package)
	-- Cleanup additional files 
	if MojoSetup.destination ~= '' then	
		if MojoSetup.platform.exists(MojoSetup.destination .. '/lib/') then
			os.execute('rm -rf ' .. MojoSetup.destination .. '/lib/')
		end
		if MojoSetup.platform.exists(MojoSetup.destination) then
			os.execute('rm -rf ' .. MojoSetup.destination)
		end
	end
    end,

    Setup.Eula
    {
        description = _("MegaGlest Game License"),
        source = _("docs/gnu_gpl_3.0.txt")
    },

    Setup.Eula
    {
        description = _("MegaGlest Data License"),
        source = _("docs/cc-by-sa-3.0-unported.txt")
    },

    Setup.Readme
    {
        description = _("MegaGlest README"),
        source = _("docs/README.txt")
    },

    Setup.Option
    {
        value = true,
        required = true,
        disabled = false,
        bytes = GAME_INSTALL_SIZE,
        description = _("MegaGlest v" .. GAME_VERSION),

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
            commandline = "%0/start_megaglest",
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
            category = "Game;StrategyGame",
            --mimetype = {"application/x-gbm", "application/mgm"}
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
            category = "Game;StrategyGame",
            --mimetype = {"application/x-g3d"}
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

