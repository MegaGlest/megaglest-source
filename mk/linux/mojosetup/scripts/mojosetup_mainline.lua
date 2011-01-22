-- MojoSetup; a portable, flexible installation application.
--
-- Please see the file LICENSE.txt in the source's root directory.
--
--  This file written by Ryan C. Gordon.


-- This is where most of the magic happens. Everything is initialized, and
--  the user's config script has successfully run. This Lua chunk drives the
--  main application code.

-- !!! FIXME: add a --dryrun option.

local _ = MojoSetup.translate

MojoSetup.metadatakey = ".mojosetup_metadata."
MojoSetup.metadatadesc = _("Metadata")
MojoSetup.metadatadirname = ".mojosetup"

if MojoSetup.info.platform == "windows" then
    MojoSetup.controlappname = "mojosetup.exe"
else
    MojoSetup.controlappname = "mojosetup"
end


local function badcmdline()
    MojoSetup.fatal(_("Invalid command line"))
end


-- This currently counts on (base) ending with a single "/", and not having
--  any strange distortions: "/blah/../blah/.//" would not match "/blah/x",
--  although it _should_ if we parsed it out.
local function make_relative(fname, base)
    local baselen = string.len(base)
    if string.sub(fname, 0, baselen) == base then
        fname = string.sub(fname, baselen+2)  -- make it relative.
    end
    return fname
end


local function manifest_resync(man, fname, _key)
    if fname == nil then return end

    local fullpath = fname
    fname = make_relative(fname, MojoSetup.destination)

    if man[fname] == nil then
        MojoSetup.logwarning("Tried to resync unknown file '" ..fname.. "' in manifest!")
    else
        local perms = "0644" -- !!! FIXME MojoSetup.platform.perms(fullpath)
        local sums = nil
        local lndest = nil
        local ftype = nil

        if MojoSetup.platform.issymlink(fullpath) then
            ftype = "symlink"
            -- !!! FIXME: linkdest?
        elseif MojoSetup.platform.isdir(fullpath) then
            ftype = "dir"
        else  -- !!! FIXME: other types?
            ftype = "file"
            sums = MojoSetup.checksum(fullpath)
        end

        man[fname] =
        {
            key = _key,
            type = ftype,
            mode = perms,
            checksums = sums,
            linkdest = lndest
        }

        MojoSetup.logwarning("Resync'd file '" ..fname.. "' in manifest")
    end
end


-- !!! FIXME: I need to go back from managing everything installed through
-- !!! FIXME:  the manifest to a separate table of "things written to disk
-- !!! FIXME:  on just this run" ... the existing manifest code can stay as-is,
-- !!! FIXME:  rollbacks should be done exclusively from that other table.
-- !!! FIXME: Right now we're relying on dumb stuff like sorting the filenames
-- !!! FIXME:  to get a safe deletion order on revert, but we should just
-- !!! FIXME:  keep a chronological array instead.

local function manifest_add(man, fname, _key, ftype, mode, sums, lndest)
    if (fname ~= nil) and (_key ~= nil) then
        local destlen = string.len(MojoSetup.destination)
        if string.sub(fname, 0, destlen) == MojoSetup.destination then
            fname = string.sub(fname, destlen+2)  -- make it relative.
        end

        if man[fname] ~= nil then
            MojoSetup.logwarning("Overwriting file '" .. fname .. "' in manifest!")
        end

        man[fname] = {
            key = _key,
            type = ftype,
            mode = perms,
            checksums = sums,
            linkdest = lndest
        }
    end
end


local function manifest_delete(man, fname)
    if fname ~= nil then
        local destlen = string.len(MojoSetup.destination)
        if string.sub(fname, 0, destlen) == MojoSetup.destination then
            fname = string.sub(fname, destlen+2)  -- make it relative.
        end

        if man[fname] == nil then
            MojoSetup.logwarning("Deleting unknown file '" .. fname .. "' from manifest!")
        else
            man[fname] = nil
        end
    end
end


local function flatten_list(list)
    local retval = list
    if type(list) == "table" then
        retval = {}
        for i,v in ipairs(list) do
            if #retval == 0 then
                retval[1] = v
            else
                retval[#retval+1] = ';'
                retval[#retval+1] = v
            end
        end
        retval = table.concat(retval)
    end
    return retval
end


local function do_delete(fname)
    local retval = false
    if fname == nil then
        retval = true
    else
        -- Try to unlink() first, so we'll catch broken symlinks, then try
        --  exists(): if it really wasn't there, we'll call it success anyhow.
        if MojoSetup.platform.unlink(fname) then
            MojoSetup.loginfo("Deleted '" .. fname .. "'")
            retval = true
        elseif not MojoSetup.platform.exists(fname) then
            retval = true
        else
            MojoSetup.logerror("Deleting '" .. fname .. "'" .. " failed!")
        end
    end
    return retval
end


local function delete_files(filelist, callback, error_is_fatal)
    if filelist ~= nil then
        local max = #filelist
        for i = max,1,-1 do
            local fname = filelist[i]
            if not do_delete(fname) and error_is_fatal then
                MojoSetup.fatal(_("Deletion failed!"))
            end

            if callback ~= nil then
                callback(fname, i, max)
            end
        end
    end
end

local function delete_rollbacks()
    if MojoSetup.rollbacks == nil then
        return
    end
    local fnames = {}
    local max = #MojoSetup.rollbacks
    for id = 1,max,1 do
        fnames[id] = MojoSetup.rollbackdir .. "/" .. id
    end
    MojoSetup.rollbacks = {}   -- just in case this gets called again...
    delete_files(fnames)  -- !!! FIXME: callback for gui queue pump?
end

local function delete_scratchdirs()
    do_delete(MojoSetup.downloaddir)
    do_delete(MojoSetup.rollbackdir)
    do_delete(MojoSetup.scratchdir)  -- must be after dirs it contains!
    do_delete(MojoSetup.metadatadir)  -- must be last! (and is okay to fail.)
end


local function do_rollbacks()
    if MojoSetup.rollbacks == nil then
        return
    end

    local max = #MojoSetup.rollbacks
    for id = max,1,-1 do
        local src = MojoSetup.rollbackdir .. "/" .. id
        local dest = MojoSetup.rollbacks[id]
        if not MojoSetup.movefile(src, dest) then
            -- we're already in fatal(), so we can only throw up a msgbox...
            MojoSetup.msgbox(_("Serious problem"),
                             _("Couldn't restore some files. Your existing installation is likely damaged."))
        end
        MojoSetup.loginfo("Restored rollback #" .. id .. ": '" .. src .. "' -> '" .. dest .. "'")
    end

    MojoSetup.rollbacks = {}   -- just in case this gets called again...
end


-- get a linear array of filenames in the manifest.
local function flatten_manifest(man, postprocess)
    local files = {}
    if postprocess == nil then
        postprocess = function(x) return x end
    end
    if man ~= nil then
        for fname,items in pairs(man) do
            files[#files+1] = postprocess(fname)
        end
    end

    table.sort(files, function(a,b) return MojoSetup.strcmp(a,b) < 0 end)
    return files
end


local function prepend_dest_dir(fname)
    if fname == "" then
        return MojoSetup.destination
    end
    return MojoSetup.destination .. "/" .. fname
end


-- This gets called by fatal()...must be a global function.
function MojoSetup.revertinstall()
    -- (The real revertinstall is set later. This is a stub for startup.)
end


local function calc_percent(current, total)
    if total == 0 then
        return 0
    elseif total < 0 then
        return -1
    end

    local retval = MojoSetup.truncatenum((current / total) * 100)
    if retval > 100 then
        retval = 100
    elseif retval < 0 then
        retval = 0
    end
    return retval
end


local function make_rate_string(rate, bw, total)
    local retval = nil

    if rate <= 0 then
        retval = _("stalled")
    else
        local ratetype = _("B/s")
        if rate > 1024 then
            rate = MojoSetup.truncatenum(rate / 1024)
            ratetype = _("KB/s")
        end

        if total > 0 then  -- can approximate time left if we know the goal.
            local bytesleft = total - bw
            local secsleft = MojoSetup.truncatenum(bytesleft / rate)
            local minsleft = MojoSetup.truncatenum(secsleft / 60)
            local hoursleft = MojoSetup.truncatenum(minsleft / 60)

            secsleft = string.sub("00" .. (secsleft - (minsleft * 60)), -2)
            minsleft = string.sub("00" .. (minsleft - (hoursleft * 60)), -2)

            if hoursleft < 10 then
                hoursleft = "0" .. hoursleft
            else
                hoursleft = tostring(hoursleft)
            end

            retval = MojoSetup.format(_("%0 %1, %2:%3:%4 remaining"),
                                      rate, ratetype,
                                      hoursleft, minsleft, secsleft)
        else
            retval = MojoSetup.format(_("%0 %1"), rate, ratetype)
        end
    end

    return retval
end


local function split_path(path)
    local retval = {}
    for item in string.gmatch(path .. "/", "(.-)/") do
        if item ~= "" then
            retval[#retval+1] = item
        end
    end
    return retval
end

local function rebuild_path(paths, n)
    local retval = paths[n]
    n = n + 1
    while paths[n] ~= nil do
        retval = retval .. "/" .. paths[n]
        n = n + 1
    end
    return retval
end

local function normalize_path(path)
    return rebuild_path(split_path(path), 1)
end


local function close_archive_list(arclist)
    for i = #arclist,1,-1 do
        MojoSetup.archive.close(arclist[i])
        arclist[i] = nil
    end
end


-- This code's a little nasty...
local function drill_for_archive(archive, path, arclist)
    if not MojoSetup.archive.enumerate(archive) then
        MojoSetup.fatal(_("Couldn't enumerate archive"))
    end

    local pathtab = split_path(path)
    local ent = MojoSetup.archive.enumnext(archive)
    while ent ~= nil do
        if ent.type == "file" then
            local i = 1
            local enttab = split_path(ent.filename)
            while (enttab[i] ~= nil) and (enttab[i] == pathtab[i]) do
                i = i + 1
            end

            if enttab[i] == nil then
                -- It's a file that makes up some of the specified path.
                --  open it as an archive and keep drilling...
                local arc = MojoSetup.archive.fromentry(archive)
                if arc == nil then
                    MojoSetup.fatal(_("Couldn't open archive"))
                end
                arclist[#arclist+1] = arc
                if pathtab[i] == nil then
                    return arc  -- this is the end of the path! Done drilling!
                end
                return drill_for_archive(arc, rebuild_path(pathtab, i), arclist)
            end
        end
        ent = MojoSetup.archive.enumnext(archive)
    end

    MojoSetup.fatal(_("Archive not found"))
end


local function install_file(dest, perms, writefn, desc, manifestkey)
    -- Upvalued so we don't look these up each time...
    local fname = string.gsub(dest, "^.*/", "", 1)  -- chop the dirs off...
    local ptype = _("Installing")
    local component = desc
    local keepgoing = true
    local callback = function(ticks, justwrote, bw, total)
        local percent = -1
        local item = fname
        if total >= 0 then
            MojoSetup.written = MojoSetup.written + justwrote
            percent = calc_percent(MojoSetup.written, MojoSetup.totalwrite)
            item = MojoSetup.format(_("%0: %1%%"), fname, calc_percent(bw, total))
        end
        keepgoing = MojoSetup.gui.progress(ptype, component, percent, item, true)
        return keepgoing
    end

    -- !!! FIXME: maybe keep a separate list, so we can rollback installs
    -- !!! FIXME:  that are building on previous installations?
    -- Add to manifest first, so we can delete it during rollback if i/o fails.
    -- !!! FIXME: perms may be nil...we need a MojoSetup.defaultPermsString()...
    manifest_add(MojoSetup.manifest, dest, manifestkey, "file", perms, nil, nil)

    MojoSetup.gui.progressitem()
    local written, sums = writefn(callback)
    if not written then
        if not keepgoing then
            MojoSetup.logerror("User cancelled install during file write.")
            MojoSetup.fatal()
        else
            MojoSetup.logerror("Failed to create file '" .. dest .. "'")
            MojoSetup.fatal(_("File creation failed!"))
        end
    end

    -- Readd it to the manifest, now with a checksum!
    if manifestkey ~= nil then
        manifest_delete(MojoSetup.manifest, dest)
        manifest_add(MojoSetup.manifest, dest, manifestkey, "file", perms, sums, nil)
    end

    MojoSetup.loginfo("Created file '" .. dest .. "'")
    MojoSetup.incrementgarbagecount()
end


local function install_file_from_archive(dest, archive, perms, desc, manifestkey)
    local fn = function(callback)
        return MojoSetup.writefile(archive, dest, perms, nil, callback)
    end
    return install_file(dest, perms, fn, desc, manifestkey)
end


local function install_file_from_stringtable(dest, t, perms, desc, manifestkey)
    local fn = function(callback)
        return MojoSetup.stringtabletofile(t, dest, perms, nil, callback)
    end
    return install_file(dest, perms, fn, desc, manifestkey)
end


local function install_file_from_string(dest, str, perms, desc, manifestkey)
    local fn = function(callback)
        return MojoSetup.stringtofile(str, dest, perms, nil, callback)
    end
    return install_file(dest, perms, fn, desc, manifestkey)
end


local function install_file_from_filesystem(dest, src, perms, desc, manifestkey, maxbytes)
    local fn = function(callback)
        return MojoSetup.copyfile(src, dest, perms, maxbytes, callback)
    end
    return install_file(dest, perms, fn, desc, manifestkey)
end


-- !!! FIXME: we should probably pump the GUI queue here, in case there are
-- !!! FIXME:  thousands of symlinks in a row or something.
local function install_symlink(dest, lndest, manifestkey)
    if not MojoSetup.platform.symlink(dest, lndest) then
        MojoSetup.logerror("Failed to create symlink '" .. dest .. "'")
        MojoSetup.fatal(_("Symlink creation failed!"))
    end

    manifest_add(MojoSetup.manifest, dest, manifestkey, "symlink", nil, nil, lndest)
    MojoSetup.loginfo("Created symlink '" .. dest .. "' -> '" .. lndest .. "'")
end


-- !!! FIXME: we should probably pump the GUI queue here, in case there are
-- !!! FIXME:  thousands of dirs in a row or something.
local function install_directory(dest, perms, manifestkey)
    -- Chop any '/' chars from the end of the string...
    dest = string.gsub(dest, "/+$", "")

    if not MojoSetup.platform.mkdir(dest, perms) then
        MojoSetup.logerror("Failed to create dir '" .. dest .. "'")
        MojoSetup.fatal(_("Directory creation failed"))
    end

    manifest_add(MojoSetup.manifest, dest, manifestkey, "directory", perms, nil, nil)
    MojoSetup.loginfo("Created directory '" .. dest .. "'")
end


local function install_parent_dirs(path, manifestkey)
    -- Chop any '/' chars from the end of the string...
    path = string.gsub(path, "/+$", "")

    -- Build each piece of final path. The gmatch() skips the last element.
    local fullpath = ""
    for item in string.gmatch(path, "(.-)/") do
        if item ~= "" then
            fullpath = fullpath .. "/" .. item
            if not MojoSetup.platform.exists(fullpath) then
                install_directory(fullpath, nil, manifestkey)
            end
        end
    end
end


local function permit_write(dest, entinfo, file)
    local allowoverwrite = true
    if MojoSetup.platform.exists(dest) then
        -- never "permit" existing dirs, so they don't rollback.
        if entinfo.type == "dir" then
            allowoverwrite = false
        else
            if MojoSetup.forceoverwrite ~= nil then
                allowoverwrite = MojoSetup.forceoverwrite
            else
                -- !!! FIXME: option/package-wide overwrite?
                allowoverwrite = file.allowoverwrite
                if not allowoverwrite then
                    MojoSetup.loginfo("File '" .. dest .. "' already exists.")
                    local text = MojoSetup.format(_("File '%0' already exists! Replace?"), dest)
                    local ynan = MojoSetup.promptynan(_("Conflict!"), text, true)
                    if ynan == "always" then
                        MojoSetup.forceoverwrite = true
                        allowoverwrite = true
                    elseif ynan == "never" then
                        MojoSetup.forceoverwrite = false
                        allowoverwrite = false
                    elseif ynan == "yes" then
                        allowoverwrite = true
                    elseif ynan == "no" then
                        allowoverwrite = false
                    end
                end
            end

            -- !!! FIXME: Setup.File.mustoverwrite to override "never"?

            if allowoverwrite then
                local id = #MojoSetup.rollbacks + 1
                local f = MojoSetup.rollbackdir .. "/" .. id
                install_parent_dirs(f, MojoSetup.metadatakey)
                MojoSetup.rollbacks[id] = dest
                if not MojoSetup.movefile(dest, f) then
                    MojoSetup.fatal(_("Couldn't backup file for rollback"))
                end
                MojoSetup.loginfo("Moved rollback #" .. id .. ": '" .. dest .. "' -> '" .. f .. "'")

                -- Make sure this isn't already in the manifest...
                if MojoSetup.manifest[dest] ~= nil then
                    manifest_delete(MojoSetup.manifest, dest)
                end
            end
        end
    end

    return allowoverwrite
end


local function install_archive_entity(dest, ent, archive, desc, manifestkey, perms)
    install_parent_dirs(dest, manifestkey)
    if ent.type == "file" then
        install_file_from_archive(dest, archive, perms, desc, manifestkey)
    elseif ent.type == "dir" then
        install_directory(dest, perms, manifestkey)
    elseif ent.type == "symlink" then
        install_symlink(dest, ent.linkdest, manifestkey)
    else  -- !!! FIXME: device nodes, etc...
        -- !!! FIXME: should this be fatal?
        MojoSetup.fatal(_("Unknown file type in archive"))
    end
end


local function install_archive_entry(archive, ent, file, option)
    local entdest = ent.filename
    if entdest == nil then return end   -- probably can't happen...

    -- Set destination in native filesystem. May be default or explicit.
    local dest = file.destination
    if dest == nil then
        dest = entdest
    else
        dest = dest .. "/" .. entdest
    end

    local perms = file.permissions   -- may be nil

    if file.filter ~= nil then
        local filterperms
        dest, filterperms = file.filter(dest)
        if filterperms ~= nil then
            perms = filterperms
        end
    end

    if dest ~= nil then  -- Only install if file wasn't filtered out
        dest = MojoSetup.destination .. "/" .. dest
        if permit_write(dest, ent, file) then
            local desc = option.description
            install_archive_entity(dest, ent, archive, desc, desc, perms)
        end
    end
end


local function install_archive(archive, file, option, dataprefix)
    if not MojoSetup.archive.enumerate(archive) then
        MojoSetup.fatal(_("Couldn't enumerate archive"))
    end

    local isbase = (archive == MojoSetup.archive.base)
    local single_match = true
    local wildcards = file.wildcards

    -- If there's only one explicit file we're looking for, we don't have to
    --  iterate the whole archive...we can stop as soon as we find it.
    if wildcards == nil then
        single_match = false
    else
        if type(wildcards) == "string" then
            wildcards = { wildcards }
        end
        if #wildcards > 1 then
            single_match = false
        else
            for i,v in ipairs(wildcards) do
                if string.find(v, "[*?]") ~= nil then
                    single_match = false
                    break  -- no reason to keep iterating...
                end
            end
        end
    end

    local ent = MojoSetup.archive.enumnext(archive)
    while ent ~= nil do
        -- If inside GBaseArchive (no URL lead in string), then we
        --  want to clip to data/ directory...
        if isbase and (string.len(dataprefix) > 0) then
            local count
            ent.filename, count = string.gsub(ent.filename, "^" .. dataprefix, "", 1)
            if count == 0 then
                ent.filename = nil
            end
        end
        
        -- See if we should install this file...
        if (ent.filename ~= nil) and (ent.filename ~= "") then
            local should_install = false
            if wildcards == nil then
                should_install = true
            else
                for i,v in ipairs(wildcards) do
                    if MojoSetup.wildcardmatch(ent.filename, v) then
                        should_install = true
                        break  -- no reason to keep iterating...
                    end
                end
            end

            if should_install then
                install_archive_entry(archive, ent, file, option)
                if single_match then
                    break   -- no sense in iterating further if we're done.
                end
            end
        end

        -- and check the next entry in the archive...
        ent = MojoSetup.archive.enumnext(archive)
    end
end


local function install_basepath(basepath, file, option, dataprefix)
    -- Obviously, we don't want to enumerate the entire physical filesystem,
    --  so we'll dig through each path element with MojoPlatform_exists()
    --  until we find one that doesn't, then we'll back up and try to open
    --  that as a directory, and then a file archive, and start drilling from
    --  there. Fun.

    local function create_basepath_archive(path)
        local archive = MojoSetup.archive.fromdir(path)
        if archive == nil then
            archive = MojoSetup.archive.fromfile(path)
            if archive == nil then
                MojoSetup.fatal(_("Couldn't open archive"))
            end
        end
        return archive
    end

    -- fast path: See if the whole path exists. This is probably the normal
    --  case, but it won't work for archives-in-archives.
    if MojoSetup.platform.exists(basepath) then
        local archive = create_basepath_archive(basepath)
        install_archive(archive, file, option, dataprefix)
        MojoSetup.archive.close(archive)
    else
        -- Check for archives-in-archives...
        local path = ""
        local paths = split_path(basepath)
        for i,v in ipairs(paths) do
            local knowngood = path
            path = path .. "/" .. v
            if not MojoSetup.platform.exists(path) then
                if knowngood == "" then
                    MojoSetup.fatal(_("Archive not found"))
                end
                local archive = create_basepath_archive(knowngood)
                local arclist = { archive }
                path = rebuild_path(paths, i)
                local arc = drill_for_archive(archive, path, arclist)
                install_archive(arc, file, option, dataprefix)
                close_archive_list(arclist)
                return  -- we're done here
            end
        end

        -- wait, the whole thing exists now? Did this just move in?
        install_basepath(basepath, file, option, dataprefix)  -- try again, I guess...
    end
end


local function set_destination(dest)
    -- Chop any '/' chars from the end of the string...
    dest = string.gsub(dest, "/+$", "")

    MojoSetup.loginfo("Install dest: '" .. dest .. "'")
    MojoSetup.destination = dest
    MojoSetup.metadatadir = MojoSetup.destination .. "/" .. MojoSetup.metadatadirname
    MojoSetup.controldir = MojoSetup.metadatadir  -- .. "/control"
    MojoSetup.manifestdir = MojoSetup.metadatadir .. "/manifest"
    MojoSetup.scratchdir = MojoSetup.metadatadir .. "/tmp"
    MojoSetup.rollbackdir = MojoSetup.scratchdir .. "/rollbacks"
    MojoSetup.downloaddir = MojoSetup.scratchdir .. "/downloads"
end


local function run_config_defined_hook(func, pkg)
    if func ~= nil then
        local errstr = func(pkg)
        if errstr ~= nil then
            MojoSetup.fatal(errstr)
        end
    end
end


-- The XML manifest is compatible with the loki_setup manifest schema, since
--  it has a reasonable set of data, and allows you to use loki_update or
--  loki_patch with a MojoSetup installation. Please note that we never ever
--  look at this data! You are responsible for updating the other files if
--  you think it'll be important. The Unix MojoSetup uninstaller uses the
--  lua manifest, for example (but loki_uninstall can use the xml one,
--  so if you want, you can just drop in MojoSetup to replace loki_setup and
--  use the Loki tools for everything else.
local function build_xml_manifest(package)
    local retval = {};

    local function addstr(str)
        retval[#retval+1] = str
    end

    addstr('<?xml version="1.0" encoding="UTF-8"?>\n')
    addstr('<product name="')
    addstr(package.id)
    addstr('" desc="')
    addstr(package.description)
    addstr('" xmlversion="1.6" root="')
    addstr(package.root)
    addstr('" ')
    if package.update_url ~= nil then
        addstr('update_url="')
        addstr(package.update_url)
        addstr('"')
    end
    addstr('>\n')
    addstr('\t<component name="Default" version="')
    addstr(package.version)
    addstr('" default="yes">\n')

    -- Need to group these by options.
    local grouped = {}
    for fname,entity in pairs(package.manifest) do
        local key = entity.key
        if grouped[key] == nil then
            grouped[key] = {}
        end
        entity.path = fname
        local list = grouped[key]
        list[#list+1] = entity
    end

    for desc,items in pairs(grouped) do
        addstr('\t\t<option name="')
        addstr(desc)
        addstr('">\n')
        for i,item in ipairs(items) do
            local type = item.type
            if type == "dir" then
                type = "directory"  -- loki_setup expects this string.
            end

            -- !!! FIXME: files from archives aren't filling item.mode in
            -- !!! FIXME:  because it figures out the perms from the archive's
            -- !!! FIXME:  C struct in native code. Need to get that into Lua.
            -- !!! FIXME: (and get the perms uint16/string conversion cleaned up)
            local mode = item.mode
            if mode == nil then
                mode = "0644"   -- !!! FIXME
            end

            local path = item.path
            item.path = nil  -- we added this when grouping. Remove it now.

            addstr('\t\t\t<')
            addstr(type)

            if type == "file" then
                if item.checksums ~= nil then
                    for k,v in pairs(item.checksums) do
                        addstr(' ')
                        addstr(k)
                        addstr('="')
                        addstr(v)
                        addstr('"')
                    end
                end
                addstr(' mode="')
                addstr(mode)
                addstr('"')
            elseif type == "directory" then
                addstr(' mode="')
                addstr(mode)
                addstr('"')
            elseif type == "symlink" then
                addstr(' dest="')
                addstr(item.linkdest)
                addstr('" mode="0777"')
            end

            addstr('>')
            addstr(path)
            addstr('</')
            addstr(type)
            addstr('>\n')
        end
        addstr('\t\t</option>\n');
    end

    addstr('\t</component>\n</product>\n\n')

    return retval
end


local function serialize(obj, prefix, postfix)
    local retval = {}
    local function addstr(str)
        retval[#retval+1] = str
    end

    if prefix ~= nil then
        addstr(prefix)
    end

    local function _serialize(obj, indent)
        local objtype = type(obj)
        if objtype == "nil" then
            addstr("nil")
        elseif (objtype == "number") or (objtype == "boolean") then
            addstr(tostring(obj))
        elseif objtype == "string" then
            addstr(string.format("%q", obj))
        elseif objtype == "function" then
            addstr("loadstring(")
            addstr(string.format("%q", string.dump(obj)))
            addstr(")")
        elseif objtype == "table" then
            addstr("{\n")
            local tab = string.rep("\t", indent)
            for k,v in pairs(obj) do
                local key = k
                addstr(tab)
                if type(key) == "number" then
                    addstr('[')
                    addstr(key)
                    addstr(']')
                elseif not string.match(key, "^[_a-zA-Z][_a-zA-Z0-9]*$") then
                    addstr('[')
                    addstr(string.format("%q", key))
                    addstr(']')
                else
                    addstr(key)
                end
                addstr(" = ")
                _serialize(v, indent+1)
                addstr(",\n")
            end
            addstr(string.rep("\t", indent-1))
            addstr("}")
        else
            MojoSetup.logerror("unexpected object to serialize (" ..
                                 objtype .. "): '" .. tostring(obj) .. "'")
            MojoSetup.fatal(_("BUG: Unhandled data type"))
        end
    end

    _serialize(obj, 1)

    if postfix ~= nil then
        addstr(postfix)
    end

    return retval
end


local function build_lua_manifest(package)
    return serialize(package, 'MojoSetup.package = ', '\n\n')
end


local function build_txt_manifest(package)
    local flat = flatten_manifest(package.manifest)
    local retval = {}
    for i,v in pairs(flat) do
        retval[#retval+1] = v
        retval[#retval+1] = "\n"
    end
    return retval
end


local function install_control_app(desc, key)
    local dst, src

    -- We copy the installer binary itself, and any auxillary files it needs,
    --  like this Lua script, to a metadata directory in the installation.
    -- Unfortunately, the binary might be a self-extracting installer that
    --  has gigabytes of now-unnecessary data appended to it, so we need to
    --  decide if that's the case and, if so, extract just the program itself
    --  from the start of the file.
    local maxbytes = -1  -- copy whole thing by default.
    local base = MojoSetup.archive.base

    dst = MojoSetup.controldir .. "/" .. MojoSetup.controlappname
    src = MojoSetup.info.binarypath
    if src == MojoSetup.info.basearchivepath then
        maxbytes = MojoSetup.archive.offsetofstart(base)
        if maxbytes <= 0 then
            MojoSetup.fatal(_("BUG: Unexpected value"))
        end
    end

    local perms = "0755"  -- !!! FIXME
    install_parent_dirs(dst, key)
    install_file_from_filesystem(dst, src, perms, desc, key, maxbytes)

    -- Okay, now we need all the support files.
    if not MojoSetup.archive.enumerate(base) then
        MojoSetup.fatal(_("Couldn't enumerate archive"))
    end

    local needdirs = { "scripts", "guis", "meta" }

    local ent = MojoSetup.archive.enumnext(base)
    while ent ~= nil do
        -- Make sure this is in a directory we want to write out...
        local should_write = false

        if (ent.filename ~= nil) and (ent.filename ~= "") then
            for i,dir in ipairs(needdirs) do
                local clipdir = "^" .. dir .. "/"
                if string.find(ent.filename, clipdir) ~= nil then
                    should_write = true
                    break
                end
            end
        end

        if should_write then
            dst = MojoSetup.controldir .. "/" .. ent.filename
            -- don't overwrite preexisting stuff.
            if not MojoSetup.platform.exists(dst) then
                install_archive_entity(dst, ent, base, desc, key, perms)
            end
        end

        -- and check the next entry in the archive...
        ent = MojoSetup.archive.enumnext(base)
    end

    -- okay, we're written out.
end

local function shquote_string(str)
    -- Quote the string so it can safely be passed to the shell
    str = string.gsub(str, "\\", "\\\\")
    str = string.gsub(str, "[$]", "\\$")
    str = string.gsub(str, "\"", "\\\"")
    str = string.gsub(str, "`", "\\`")
    return "\"" ..str.. "\""
end

local function install_unix_uninstaller(desc, key)
    -- Write a script out that calls the uninstaller.
    local fname = MojoSetup.destination .. "/" ..
                  "uninstall-" .. MojoSetup.install.id .. ".sh"

    -- Man, I hate escaping shell strings...
    local bin = "\"`dirname \\\"$0\\\"`\"/" ..
                shquote_string(MojoSetup.metadatadirname .. "/" .. MojoSetup.controlappname)

    local script =
        "#!/bin/sh\n" ..
        "exec " .. bin .. " uninstall " .. shquote_string(MojoSetup.install.id) .. " \"$@\"\n"

    install_parent_dirs(fname, key)
    install_file_from_string(fname, script, "0755", desc, key)
end


local function install_product_keys(productkeys)
    for desc,prodkey in pairs(productkeys) do
        local dest = MojoSetup.destination .. "/" .. prodkey.destination
        local productkey = prodkey.productkey
        local component = prodkey.component
        -- !!! FIXME: Windows registry support.
        -- !!! FIXME: file permissions for product keys?
        install_parent_dirs(dest, component)
        install_file_from_string(dest, productkey, "0644", desc, component)
    end
end


local function install_manifests(desc, key)
    -- We write out a Lua script as a data definition language, a
    --  loki_setup-compatible XML manifest, and a straight text file of
    --  all the filenames. Take your pick.

    local perms = "0644"  -- !!! FIXME
    local basefname = MojoSetup.manifestdir .. "/" .. MojoSetup.install.id
    local lua_fname = basefname .. ".lua"
    local xml_fname = basefname .. ".xml"
    local txt_fname = basefname .. ".txt"

    -- We have to cheat and just plug these into the manifest directly, since
    --  they won't show up until after we write them out, otherwise.
    -- Obviously, we don't have checksums for them, either, as we haven't
    --  assembled the data yet!

    manifest_add(MojoSetup.manifest, lua_fname, key, "file", perms, nil, nil)
    manifest_add(MojoSetup.manifest, xml_fname, key, "file", perms, nil, nil)
    manifest_add(MojoSetup.manifest, txt_fname, key, "file", perms, nil, nil)

    -- build the "package" table that we serialize, etc.
    local package =
    {
        id = MojoSetup.install.id,
        vendor = MojoSetup.install.vendor,
        description = MojoSetup.install.description,
        root = MojoSetup.destination,
        update_url = MojoSetup.install.updateurl,
        version = MojoSetup.install.version,
        manifest = MojoSetup.manifest,
        splash = MojoSetup.install.splash,
        splashpos = MojoSetup.install.splashpos,
        desktopmenuitems = MojoSetup.install.desktopmenuitems,
        preuninstall = MojoSetup.install.preuninstall,
        postuninstall = MojoSetup.install.postuninstall
    }

    -- now build these things...
    install_parent_dirs(lua_fname, key)
    install_file_from_stringtable(lua_fname, build_lua_manifest(package), perms, desc, nil)
    install_file_from_stringtable(xml_fname, build_xml_manifest(package), perms, desc, nil)
    install_file_from_stringtable(txt_fname, build_txt_manifest(package), perms, desc, nil)
end


local function freedesktop_menuitem_filename(pkg, idx)  -- only for Unix.
    local vendor = string.gsub(pkg.vendor, "%.", "_")
    local fname = vendor .. "-" .. pkg.id .. "_" .. idx .. ".desktop"
    return MojoSetup.metadatadir .. "/" .. fname
end


local function uninstall_desktop_menu_items(pkg)
    -- !!! FIXME: GUI progress?
    if pkg.desktopmenuitems ~= nil then
        for i,v in ipairs(pkg.desktopmenuitems) do
            if MojoSetup.info.platform == "windows" then
                MojoSetup.fatal(_("Unimplemented"))  -- !!! FIXME: write me.
            elseif MojoSetup.info.platform == "macosx" then
                MojoSetup.fatal(_("Unimplemented"))  -- !!! FIXME: write me.
            elseif MojoSetup.info.platform == "beos" then
                MojoSetup.fatal(_("Unimplemented"))  -- !!! FIXME: write me.
            else  -- freedesktop, we hope.
                local fname = freedesktop_menuitem_filename(pkg, i)
                if not MojoSetup.platform.uninstalldesktopmenuitem(fname) then
                    MojoSetup.fatal(_("Failed to uninstall desktop menu item"))
                end
            end
        end
    end
end


local function install_freedesktop_menuitem(pkg, idx, item)  -- only for Unix.
    local icon
    local dest = MojoSetup.destination
    if item.builtin_icon then
        icon = item.icon
    else
        icon = dest .. "/" .. item.icon
    end

    local cmdline = MojoSetup.format(item.commandline, dest)

    -- Try to escape some characters...
    cmdline = '"' .. string.gsub(string.gsub(cmdline, "\"","\\\""), "%%", "%%%%") .. '"'

    local t = { "[Desktop Entry]\n" }
    local function addpair(key, val)
        t[#t+1] = key
        t[#t+1] = '='
        t[#t+1] = val
        t[#t+1] = '\n'
    end

    addpair("Encoding", "UTF-8")
    addpair("Value", "1.0")
    addpair("Type", "Application")
    addpair("Name", item.name)
    addpair("GenericName", item.genericname)
    addpair("Comment", item.tooltip)
    addpair("Icon", icon)
    addpair("Exec", cmdline)
    addpair("Categories", flatten_list(item.category))

    if item.mimetype ~= nil then
        addpair("MimeType", flatten_list(item.mimetype))
    end

    if item.workingdir ~= nil then
        addpair("Path", MojoSetup.format(item.workingdir, dest))
    end

    t[#t+1] = '\n'

    local fname = freedesktop_menuitem_filename(pkg, idx)
    local perms = "0644"  -- !!! FIXME
    local key = MojoSetup.metadatakey
    local desc = MojoSetup.metadatadesc

    --MojoSetup.logdebug("Install FreeDesktop file")
    --MojoSetup.logdebug(fname)
    --MojoSetup.logdebug(str)
    install_parent_dirs(fname, key)
    install_file_from_stringtable(fname, t, perms, desc, key)
    if not MojoSetup.platform.installdesktopmenuitem(fname) then
        MojoSetup.fatal(_("Failed to install desktop menu item"))
    end
end


local function install_desktop_menu_items(pkg)
    -- !!! FIXME: GUI progress?
    if pkg.desktopmenuitems ~= nil then
        for i,item in ipairs(pkg.desktopmenuitems) do
            if not item.disabled then
                if MojoSetup.info.platform == "windows" then
                    MojoSetup.fatal(_("Unimplemented"))  -- !!! FIXME: write me.
                elseif MojoSetup.info.platform == "macosx" then
                    MojoSetup.fatal(_("Unimplemented"))  -- !!! FIXME: write me.
                elseif MojoSetup.info.platform == "beos" then
                    MojoSetup.fatal(_("Unimplemented"))  -- !!! FIXME: write me.
                else  -- freedesktop, we hope.
                    install_freedesktop_menuitem(pkg, i, item)
                end
            end
        end
    end
end


local function get_productkey(thisstage, maxstage, desc, fmt, verify, dest, manifestkey)
    local key = nil
    local userkey = nil
    local retval = nil

    -- Retrieve the previous entry, in case we're stepping back over a stage.
    --  This lets the user edit it or jsut move forward without typing the
    --  whole thing again.
    if MojoSetup.productkeys[desc] ~= nil then
        userkey = MojoSetup.productkeys[desc].user_productkey
    end

    while key == nil do
        retval, userkey = MojoSetup.gui.productkey(desc, fmt, userkey, thisstage, maxstage)
        if retval ~= 1 then
            return retval  -- user hit back or cancel.
        end

        key = userkey
        if verify ~= nil then
            local ok, newkey = verify(userkey)
            if not ok then
                MojoSetup.msgbox(
                    _("Invalid product key"),
                    _("That key appears to be invalid. Please try again."))
                key = nil
            elseif newkey ~= nil then
                key = newkey
            end
        end
    end

    for desckey,prodkey in pairs(MojoSetup.productkeys) do
        if (prodkey.destination == dest) and (desckey ~= desc) then
            MojoSetup.logwarning("More than one product key with same destination!")
            break
        end
    end

    MojoSetup.productkeys[desc] = {
        destination = dest,
        productkey = key,
        user_productkey = userkey,
        component = manifestkey
    }

    return 1
end


local function start_gui(desc, splashfname, splashpos)
    if splashfname ~= nil then
        splashfname = 'meta/' .. splashfname
    end

    if not MojoSetup.gui.start(desc, splashfname, splashpos) then
        MojoSetup.fatal(_("GUI failed to start"))
    end

    MojoSetup.gui_started = true
end


local function stop_gui()
    MojoSetup.gui.stop()
    MojoSetup.gui_started = nil
end


local function do_install(install)
    MojoSetup.forceoverwrite = nil
    MojoSetup.written = 0
    MojoSetup.totalwrite = 0
    MojoSetup.downloaded = 0
    MojoSetup.totaldownload = 0
    MojoSetup.install = install
    MojoSetup.installed_menu_items = false

    -- !!! FIXME: need a cmdline to automate cdkey entry?
    local skipeulas = MojoSetup.cmdline("i-agree-to-all-licenses")
    local skipreadmes = MojoSetup.cmdline("noreadme")
    local skipoptions = MojoSetup.cmdline("nooptions")

    -- !!! FIXME: try to sanity check everything we can here
    -- !!! FIXME:  (unsupported URLs, bogus media IDs, etc.)
    -- !!! FIXME:  I would like everything possible to fail here instead of
    -- !!! FIXME:  when a user happens to pick an option no one tested...

    if (install.options == nil) and (install.optiongroups == nil) then
        MojoSetup.fatal(_("BUG: no options"))
    end

    -- The uninstaller support needs a manifest to know what to do. Force it on.
    if (install.support_uninstall) and (not install.write_manifest) then
        MojoSetup.fatal(_("BUG: support_uninstall requires write_manifest"))
    end

    -- Desktop icons should probably require uninstall so we don't clutter
    --  the system with no option for reversal later.
    -- !!! FIXME: will miss menu items that are Setup.Option children...
    if (install.desktopmenuitems ~= nil) and (not install.support_uninstall) then
        MojoSetup.fatal(_("BUG: Setup.DesktopMenuItem requires support_uninstall"))
    end

    -- Manifest support requires the Lua parser.
    if (install.write_manifest) and (not MojoSetup.info.luaparser) then
        MojoSetup.fatal(_("BUG: write_manifest requires Lua parser support"))
    end

    -- This is to save us the trouble of a loop every time we have to
    --  find media by id...
    MojoSetup.media = {}
    if install.medias ~= nil then
        for k,v in pairs(install.medias) do
            if MojoSetup.media[v.id] ~= nil then
                MojoSetup.fatal(_("BUG: duplicate media id"))
            end
            MojoSetup.media[v.id] = v
        end
    end

    -- Build a bunch of functions into a linear array...this lets us move
    --  back and forth between stages of the install with customized functions
    --  for each bit that have their own unique params embedded as upvalues.
    -- So if there are three EULAs to accept, we'll call three variations of
    --  the EULA function with three different tables that appear as local
    --  variables, and the driver that calls this function will be able to
    --  skip back and forth based on user input. This is a cool Lua thing.
    local stages = {}

    -- First stage: Make sure installer can run. Always fails or steps forward.
    -- !!! FIXME: you can step back onto this...need a way to run some stages
    -- !!! FIXME:  only once...
    if install.precheck ~= nil then
        stages[#stages+1] = function ()
            run_config_defined_hook(install.precheck, install)
            return 1
        end
    end

    -- Next stage: accept all global EULAs. Rejection of any EULA is considered
    --  fatal. These are global EULAs for the install, per-option EULAs come
    --  later.
    if (install.eulas ~= nil) and (not skipeulas) then
        for k,eula in pairs(install.eulas) do
            local desc = eula.description
            local fname = install.dataprefix .. eula.source
            local accept_needed = not eula.accept_not_needed

            -- (desc) and (fname) become upvalues in this function.
            stages[#stages+1] = function (thisstage, maxstage)
                local retval = MojoSetup.gui.readme(desc,fname,thisstage,maxstage)
                if retval == 1 then
                    if accept_needed then
                        if not MojoSetup.promptyn(desc, _("Accept this license?"), false) then
                            MojoSetup.fatal(_("You must accept the license before you may install"))
                        end
                    end
                end
                return retval
            end
        end
    end

    -- Next stage: enter all global products keys. These are global keys
    --  for the install, per-option keys come later.
    MojoSetup.productkeys = {}
    if install.productkeys ~= nil then
        for k,prodkey in pairs(install.productkeys) do
            -- (prodkey) becomes an upvalue in this function.
            stages[#stages+1] = function(thisstage, maxstage)
                return get_productkey(thisstage, maxstage, prodkey.description,
                                      prodkey.format, prodkey.verify,
                                      prodkey.destination,
                                      MojoSetup.metadatakey)
            end
        end
    end

    -- Next stage: show any READMEs.
    if (install.readmes ~= nil) and (not skipreadmes) then
        for k,readme in pairs(install.readmes) do
            local desc = readme.description
            -- !!! FIXME: pull from archive?
            local fname = install.dataprefix .. readme.source
            -- (desc) and (fname) become upvalues in this function.
            stages[#stages+1] = function(thisstage, maxstage)
                return MojoSetup.gui.readme(desc, fname, thisstage, maxstage)
            end
        end
    end

    -- Next stage: let user choose install destination.
    --  The config file can force a destination if it has a really good reason
    --  (system drivers that have to go in a specific place, for example),
    --  but really really shouldn't in 99% of the cases.
    local destcmdline = MojoSetup.cmdlinestr("destination")
    if install.destination ~= nil then
        set_destination(install.destination)
    elseif destcmdline ~= nil then
        set_destination(destcmdline)
    else
        local recommend = nil
        local recommended_cfg = install.recommended_destinations
        if recommended_cfg ~= nil then
            if type(recommended_cfg) == "string" then
                recommended_cfg = { recommended_cfg }
            end

            recommend = {}
            for i,v in ipairs(recommended_cfg) do
                if MojoSetup.platform.isdir(v) then
                    if MojoSetup.platform.writable(v) then
                        recommend[#recommend+1] = v .. "/" .. install.id
                    end
                end
            end

            if #recommend == 0 then
                recommend = nil
            end
        end

        -- (recommend) becomes an upvalue in this function.
        stages[#stages+1] = function(thisstage, maxstage)
            local rc, dst
            rc, dst = MojoSetup.gui.destination(recommend, thisstage, maxstage)
            if rc == 1 then
                set_destination(dst)
            end
            return rc
        end
    end

    -- Next stage: let user choose install options.
    --  This may not produce a GUI stage if it decides that all options
    --  are either required or disabled.
    if not skipoptions then  -- (just take the defaults...)
        -- (install) becomes an upvalue in this function.
        stages[#stages+1] = function(thisstage, maxstage)
            -- This does some complex stuff with a hierarchy of tables in C.
            return MojoSetup.gui.options(install, thisstage, maxstage)
        end
    end


    -- Next stage: accept all option-specific EULAs.
    --  Rejection of any EULA will put you back one stage (usually to the
    --  install options), but if there is no previous stage, this becomes
    --  fatal.
    -- This may not produce a GUI stage if there are no selected options with
    --  EULAs to accept.
    if not skipeulas then
        stages[#stages+1] = function(thisstage, maxstage)
            local option_eulas = {}

            local function find_option_eulas(opts)
                local options = opts['options']
                if options ~= nil then
                    for k,v in pairs(options) do
                        if v.value then
                            if v.eulas ~= nil then
                                for ek,ev in pairs(v.eulas) do
                                    option_eulas[#option_eulas+1] = ev
                                end
                            end
                            find_option_eulas(v)
                        end
                    end
                end

                local optiongroups = opts['optiongroups']
                if optiongroups ~= nil then
                    for k,v in pairs(optiongroups) do
                        if not v.disabled then
                            find_option_eulas(v)
                        end
                    end
                end
            end

            find_option_eulas(install)

            for k,eula in pairs(option_eulas) do
                local desc = eula.description
                local fname = install.dataprefix .. eula.source
                local accept_needed = not eula.accept_not_needed
                local retval = MojoSetup.gui.readme(desc,fname,thisstage,maxstage)
                if retval == 1 then
                    if accept_needed then
                        if not MojoSetup.promptyn(desc, _("Accept this license?"), false) then
                            -- can't go back? We have to die here instead.
                            if thisstage == 1 then
                                MojoSetup.fatal(_("You must accept the license before you may install"))
                            else
                                retval = -1  -- just step back a stage.
                            end
                        end
                    end
                end

                if retval ~= 1 then
                    return retval
                end
            end

            return 1   -- all licenses were agreed to. Go to the next stage.
        end
    end

    -- Next stage: enter all option-specific product keys.
    -- This may not produce a GUI stage if there are no selected options with
    --  product keys. Many installers will use a single global key instead.
    stages[#stages+1] = function(thisstage, maxstage)
        local options_with_keys = {}

        local function find_options_with_keys(opts)
            local options = opts['options']
            if options ~= nil then
                for k,v in pairs(options) do
                    if v.value then
                        if v.productkeys ~= nil then
                            options_with_keys[#options_with_keys+1] = v;
                        end
                        find_options_with_keys(v)
                    end
                end
            end

            local optiongroups = opts['optiongroups']
            if optiongroups ~= nil then
                for k,v in pairs(optiongroups) do
                    if not v.disabled then
                        find_options_with_keys(v)
                    end
                end
            end
        end

        find_options_with_keys(install)

        for optk,option in ipairs(options_with_keys) do
            for k,prodkey in ipairs(option.productkeys) do
                local retval = get_productkey(thisstage, maxstage,
                                              prodkey.description,
                                              prodkey.format, prodkey.verify,
                                              prodkey.destination,
                                              option.description)
                if retval ~= 1 then
                    return retval
                end
            end
        end

        return 1   -- all product keys are entered. Go to the next stage.
    end

    -- Next stage: Make sure source list is sane.
    --  This is not a GUI stage, it just needs to run between them.
    --  This gets a little hairy.
    stages[#stages+1] = function(thisstage, maxstage)
        -- Make sure we install the destination dir, so it's in the manifest.
        if not MojoSetup.platform.exists(MojoSetup.destination) then
            install_parent_dirs(MojoSetup.destination, MojoSetup.metadatakey)
            install_directory(MojoSetup.destination, nil, MojoSetup.metadatakey)
        end

        local function process_file(option, file)
            -- !!! FIXME: what happens if a file shows up in multiple options?
            local src = file.source
            local prot,host,path
            if src ~= nil then  -- no source, it's in GBaseArchive
                prot,host,path = MojoSetup.spliturl(src)
            end
            if (src == nil) or (prot == "base://") then  -- included content?
                if MojoSetup.files.included == nil then
                    MojoSetup.files.included = {}
                end
                MojoSetup.files.included[file] = option
            elseif prot == "media://" then
                -- !!! FIXME: make sure media id is valid.
                if MojoSetup.files.media == nil then
                    MojoSetup.files.media = {}
                end
                if MojoSetup.files.media[host] == nil then
                    MojoSetup.files.media[host] = {}
                end
                MojoSetup.files.media[host][file] = option
            else
                -- !!! FIXME: make sure we can handle this URL...
                if MojoSetup.files.downloads == nil then
                    MojoSetup.files.downloads = {}
                end

                if option.bytes > 0 then
                    MojoSetup.totaldownload = MojoSetup.totaldownload + option.bytes
                end
                MojoSetup.files.downloads[file] = option
            end
        end

        -- Sort an option into tables that say what sort of install it is.
        --  This lets us batch all the things from one CD together,
        --  do all the downloads first, etc.
        local function process_option(option)
            if option.bytes > 0 then
                MojoSetup.totalwrite = MojoSetup.totalwrite + option.bytes
            end
            if option.files ~= nil then
                for k,v in pairs(option.files) do
                    process_file(option, v)
                end
            end

            if option.desktopmenuitems ~= nil then
                for i,item in ipairs(option.desktopmenuitems) do
                    if install.desktopmenuitems == nil then
                        install.desktopmenuitems = {}
                    end
                    install.desktopmenuitems[#install.desktopmenuitems+1] = item
                end
            end
        end

        local function build_source_tables(opts)
            local options = opts['options']
            if options ~= nil then
                for k,v in pairs(options) do
                    if v.value then
                        process_option(v)
                        build_source_tables(v)
                    end
                end
            end

            local optiongroups = opts['optiongroups']
            if optiongroups ~= nil then
                for k,v in pairs(optiongroups) do
                    if not v.disabled then
                        build_source_tables(v)
                    end
                end
            end
        end

        run_config_defined_hook(install.preflight, install)

        MojoSetup.files = {}
        build_source_tables(install)

        -- This dumps the files tables using MojoSetup.logdebug,
        --  so it only spits out crap if debug-level logging is enabled.
        MojoSetup.dumptable("MojoSetup.files", MojoSetup.files)

        return 1   -- always go forward from non-GUI stage.
    end

    -- Next stage: Download external packages.
    stages[#stages+1] = function(thisstage, maxstage)
        if MojoSetup.files.downloads ~= nil then
            -- !!! FIXME: id will cause problems for download resume
            -- !!! FIXME: id will chop filename extension
            local id = 0
            for file,option in pairs(MojoSetup.files.downloads) do
                local f = MojoSetup.downloaddir .. "/" .. id
                install_parent_dirs(f, MojoSetup.metadatakey)
                id = id + 1

                -- Upvalued so we don't look these up each time...
                local url = file.source
                local fname = string.gsub(url, "^.*/", "", 1)  -- chop the dirs off...
                local ptype = _("Downloading")
                local component = option.description
                local bps = 0
                local bpsticks = 0
                local ratestr = ''
                local item = fname
                local percent = -1
                local callback = function(ticks, justwrote, bw, total)
                    if total < 0 then
                        -- adjust start point for d/l rate calculation...
                        bpsticks = ticks + 1000
                    else
                        if ticks >= bpsticks then
                            ratestr = make_rate_string(bps, bw, total)
                            bpsticks = ticks + 1000
                            bps = 0
                        end
                        bps = bps + justwrote
                        MojoSetup.downloaded = MojoSetup.downloaded + justwrote
                        percent = calc_percent(MojoSetup.downloaded,
                                               MojoSetup.totaldownload)

                        item = MojoSetup.format(_("%0: %1%% (%2)"),
                                                fname,
                                                calc_percent(bw, total),
                                                ratestr)
                    end
                    return MojoSetup.gui.progress(ptype, component, percent, item, true)
                end

                MojoSetup.loginfo("Download '" .. url .. "' to '" .. f .. "'")
                MojoSetup.gui.progressitem()
                local downloaded, sums = MojoSetup.download(url, f, nil, nil, callback)
                if not downloaded then
                    MojoSetup.fatal(_("File download failed!"))
                end
                MojoSetup.downloads[#MojoSetup.downloads+1] = f
            end
        end
        return 1
    end

    -- Next stage: actual installation.
    stages[#stages+1] = function(thisstage, maxstage)
        run_config_defined_hook(install.preinstall)

        -- Do stuff on media first, so the user finds out he's missing
        --  disc 3 of 57 as soon as possible...

        -- Since we sort all things to install by the media that contains
        --  the source data, you should only have to insert each disc
        --  once, no matter how they landed in the config file.

        if MojoSetup.files.media ~= nil then
            -- Build an array of media ids so we can sort them into a
            --  reasonable order...disc 1 before disc 2, etc.
            local medialist = {}
            for mediaid,mediavalue in pairs(MojoSetup.files.media) do
                medialist[#medialist+1] = mediaid
            end
            table.sort(medialist)

            for mediaidx,mediaid in ipairs(medialist) do
                local media = MojoSetup.media[mediaid]
                local basepath = MojoSetup.findmedia(media.uniquefile)
                while basepath == nil do
                    if not MojoSetup.gui.insertmedia(media.description) then
                        return 0   -- user cancelled.
                    end
                    basepath = MojoSetup.findmedia(media.uniquefile)
                end

                -- Media is ready, install everything from this one...
                MojoSetup.loginfo("Found correct media at '" .. basepath .. "'")
                local files = MojoSetup.files.media[mediaid]
                for file,option in pairs(files) do
                    local prot,host,path = MojoSetup.spliturl(file.source)
                    install_basepath(basepath .. "/" .. path, file, option, install.dataprefix)
                end
            end
            medialist = nil   -- done with this.
        end

        if MojoSetup.files.downloads ~= nil then
            local id = 0
            for file,option in pairs(MojoSetup.files.downloads) do
                local f = MojoSetup.downloaddir .. "/" .. id
                id = id + 1
                install_basepath(f, file, option, install.dataprefix)
            end
        end

        if MojoSetup.files.included ~= nil then
            for file,option in pairs(MojoSetup.files.included) do
                local arc = MojoSetup.archive.base
                if file.source == nil then
                    install_archive(arc, file, option, install.dataprefix)
                else
                    local prot,host,path = MojoSetup.spliturl(file.source)
                    local arclist = {}
                    arc = drill_for_archive(arc, install.dataprefix .. path, arclist)
                    install_archive(arc, file, option, install.dataprefix)
                    close_archive_list(arclist)
                end
            end
        end

        if install.desktopmenuitems ~= nil then
            install_desktop_menu_items(install)
            MojoSetup.installed_menu_items = true
        end

        if install.support_uninstall then
            if MojoSetup.info.platform == "windows" then
                MojoSetup.fatal(_("Unimplemented"))  -- !!! FIXME: write me.
            else  -- Unix, Mac OS X, BeOS...
                install_unix_uninstaller(MojoSetup.metadatadesc, MojoSetup.metadatakey)
            end
        end

        install_product_keys(MojoSetup.productkeys)

        run_config_defined_hook(install.postinstall, install)

        if install.write_manifest then
            install_control_app(MojoSetup.metadatadesc, MojoSetup.metadatakey)
            install_manifests(MojoSetup.metadatadesc, MojoSetup.metadatakey)
        end

        return 1   -- go to next stage.
    end

    -- Next stage: show results gui
    stages[#stages+1] = function(thisstage, maxstage)
        MojoSetup.gui.final(_("Installation was successful."))
        return 1  -- go forward.
    end

    -- Now make all this happen.
    start_gui(install.description, install.splash, install.splashpos)

    -- Make the stages available elsewhere.
    MojoSetup.stages = stages

    MojoSetup.manifest = {}
    MojoSetup.rollbacks = {}
    MojoSetup.downloads = {}

    local i = 1
    while MojoSetup.stages[i] ~= nil do
        local stage = MojoSetup.stages[i]
        local rc = stage(i, #MojoSetup.stages)

        -- Too many times I forgot to return something.   :)
        if type(rc) ~= "number" then
            MojoSetup.fatal(_("BUG: stage returned wrong type"))
        end

        if rc == 1 then
            i = i + 1   -- next stage.
        elseif rc == -1 then
            if i == 1 then
                MojoSetup.fatal(_("BUG: stepped back over start of stages"))
            else
                i = i - 1
            end
        elseif rc == 0 then
            MojoSetup.fatal()  -- user cancelled
        else
            MojoSetup.fatal(_("BUG: stage returned wrong value"))
        end
    end

    -- Successful install, so delete conflicts we no longer need to rollback.
    delete_rollbacks()
    delete_files(MojoSetup.downloads)
    delete_scratchdirs()

    -- Don't let future errors delete files from successful installs...
    MojoSetup.downloads = nil
    MojoSetup.rollbacks = nil

    stop_gui()

    -- Done with these things. Make them eligible for garbage collection.
    stages = nil
    MojoSetup.manifest = nil
    MojoSetup.manifestdir = nil
    MojoSetup.metadatadir = nil
    MojoSetup.controldir = nil
    MojoSetup.scratchdir = nil
    MojoSetup.rollbackdir = nil
    MojoSetup.downloaddir = nil
    MojoSetup.install = nil
    MojoSetup.forceoverwrite = nil
    MojoSetup.installed_menu_items = nil
    MojoSetup.stages = nil
    MojoSetup.files = nil
    MojoSetup.productkeys = nil
    MojoSetup.media = nil
    MojoSetup.written = 0
    MojoSetup.totalwrite = 0
    MojoSetup.downloaded = 0
    MojoSetup.totaldownload = 0
end


local function real_revertinstall()
    if MojoSetup.gui_started then
        MojoSetup.gui.final(_("Incomplete installation. We will revert any changes we made."))
    end

    MojoSetup.loginfo("Cleaning up half-finished installation...")

    -- !!! FIXME: callbacks here.
    if MojoSetup.installed_menu_items then
        uninstall_desktop_menu_items(MojoSetup.install)
    end

    delete_files(MojoSetup.downloads)
    delete_files(flatten_manifest(MojoSetup.manifest, prepend_dest_dir))
    do_rollbacks()
    delete_scratchdirs()
end


local function installer()
    local postexec = nil
    MojoSetup.loginfo("Installer starting")

    MojoSetup.revertinstall = real_revertinstall   -- replace the stub.

    -- This dumps the table built from the user's config script using logdebug,
    --  so it only spits out crap if debug-level logging is enabled.
    MojoSetup.dumptable("MojoSetup.installs", MojoSetup.installs)

    local saw_an_installer = false
    for installkey,install in pairs(MojoSetup.installs) do
        if not install.disabled then
            saw_an_installer = true
            do_install(install)

            if (install.postexec ~= nil) then
                postexec = string.gsub(install.postexec, "$DESTINATION", MojoSetup.destination)
            end

            MojoSetup.collectgarbage()  -- nuke all the tables we threw around...
        end
    end

    if not saw_an_installer then
        MojoSetup.fatal(_("Nothing to do!"))
    end

    if postexec ~= nil then
        MojoSetup.platform.exec(postexec)
    end

    MojoSetup.destination = nil
end


local function load_manifest(pkg)
    local package = nil
    if pkg == nil then
        badcmdline()
    end

    MojoSetup.package = nil   -- just in case.

    -- We need to be in the metadata directory of an install or this fails.
    local base = string.gsub(MojoSetup.info.basearchivepath, "/[^/]*$", "", 1)
    set_destination(base)  -- set up directories
    if MojoSetup.metadatadir == MojoSetup.info.basearchivepath then
        -- load the existing manifest first...
        local ran = MojoSetup.runfilefromdir("manifest", pkg)
        if (not ran) or (MojoSetup.package == nil) then
            MojoSetup.fatal(MojoSetup.format(
                               _("Couldn't load manifest file for '%0'"), pkg))
        end

        -- Move this out of the global...
        package = MojoSetup.package
        MojoSetup.package = nil
    end

    -- note that we discard the base directory specified in the manifest, as
    --  someone could have moved the installation's folder elsewhere. We'll
    --  keep it up to date for loki_update or whatever to use, though.
    package.destination = MojoSetup.destination

    return package
end


local function manifest_management()
    MojoSetup.loginfo("Manifest management starting")
    local package = load_manifest(MojoSetup.info.argv[3])

    local i = 4
    while MojoSetup.info.argv[i] ~= nil do
        local cmd = MojoSetup.info.argv[i]
        i = i + 1

        if cmd == "add" then
            local key = MojoSetup.info.argv[i]
            i = i + 1
            local fname = MojoSetup.info.argv[i]
            i = i + 1
            if (key == nil) or (fname == nil) then
                badcmdline()
            end

            MojoSetup.loginfo("Add '" ..fname.. "', '" ..key.. "' to manifest")
            fname = MojoSetup.destination .. "/" .. fname
            if not MojoSetup.platform.exists(fname) then
                MojoSetup.fatal(MojoSetup.format(_("File %0 not found"), fname))
            end

            manifest_add(package.manifest, fname, key, nil, nil, nil, nil)
            manifest_resync(package.manifest, fname, key)

        elseif cmd == "delete" then
            local fname = MojoSetup.info.argv[i]
            i = i + 1
            if fname == nil then
                badcmdline()
            end
            MojoSetup.loginfo("Delete '" .. fname .. "' from manifest")
            fname = MojoSetup.destination .. "/" .. fname
            manifest_delete(package.manifest, fname)

        elseif cmd == "resync" then
            local fname = MojoSetup.info.argv[i]
            i = i + 1
            if fname == nil then
                badcmdline()
            end
            MojoSetup.loginfo("Resync '" .. fname .. "' in manifest")
            fname = MojoSetup.destination .. "/" .. fname
            if not MojoSetup.platform.exists(fname) then
                MojoSetup.fatal(MojoSetup.format(_("File %0 not found"), fname))
            end
            manifest_resync(package.manifest, fname)

        elseif string.match(cmd, "^-") == nil then   -- skip "-option" strings
            MojoSetup.logerror("Unknown command '" .. cmd .. "'")
            badcmdline()
        end
    end

    -- !!! FIXME: duplication with install_manifests()
    local perms = "0644"  -- !!! FIXME
    local basefname = MojoSetup.manifestdir .. "/" .. package.id
    local lua_fname = basefname .. ".lua"
    local xml_fname = basefname .. ".xml"
    local txt_fname = basefname .. ".txt"

    MojoSetup.loginfo("rebuilding manifests...")

    -- !!! FIXME: rollback!
    delete_files({lua_fname, xml_fname, txt_fname}, nil, false)
    MojoSetup.stringtabletofile(build_lua_manifest(package), lua_fname, perms, nil, nil)
    MojoSetup.stringtabletofile(build_xml_manifest(package), xml_fname, perms, nil, nil)
    MojoSetup.stringtabletofile(build_txt_manifest(package), txt_fname, perms, nil, nil)

    MojoSetup.loginfo("manifests rebuilt!")
end


local function uninstaller()
    MojoSetup.loginfo("Uninstaller starting")
    local package = load_manifest(MojoSetup.info.argv[3])

    local title = _("Uninstall")

    local uninstall_permitted = false
    if MojoSetup.cmdline("force") then
        uninstall_permitted = true
    else
        local question = _("Are you sure you want to uninstall '%0'?")
        question = MojoSetup.format(question, package.description)
        uninstall_permitted = MojoSetup.promptyn(title, question, false)
    end

    if uninstall_permitted then
        start_gui(package.description, package.splash, package.splashpos)
        run_config_defined_hook(package.preuninstall, package)

        uninstall_desktop_menu_items(package)

        -- Upvalued in callback so we don't look this up each time...
        local ptype = _("Uninstalling")
        local callback = function(fname, current, total)
            fname = make_relative(fname, MojoSetup.destination)
            local percent = 100 - calc_percent(current, total)
            local component = package.manifest[fname].key
            if component == nil then
                component = ""
            elseif component == MojoSetup.metadatakey then
                component = MojoSetup.metadatadesc
            end

            local item = string.gsub(fname, "^.*/", "", 1)  -- chop off dirs...
            MojoSetup.gui.progressitem()
            MojoSetup.gui.progress(ptype, component, percent, item, false)
            return true  -- !!! FIXME: need to disable cancel button in UI...
        end

        local filelist = flatten_manifest(package.manifest, prepend_dest_dir)
        delete_files(filelist, callback, package.delete_error_is_fatal)
        run_config_defined_hook(package.postuninstall, package)
        MojoSetup.gui.final(_("Uninstall complete"))
        stop_gui()
    end

    -- !!! FIXME: postuninstall hook?
end



-- Mainline...

local purpose = nil

-- Have to check argv instead of using cmdline(), for precision's sake.
-- Remember that unlike main()'s argv in C, Lua starts arrays at 1,
--  so argv[2] would be argv[1] in C. Fun.
local argv2 = MojoSetup.info.argv[2]
if argv2 == "manifest" then
    purpose = manifest_management
elseif argv2 == "uninstall" then
    purpose = uninstaller
else
    purpose = installer
    MojoSetup.runfile("config")  -- This builds the MojoSetup.installs table.
end

-- We don't need the "Setup" namespace anymore. Make it eligible
--  for garbage collection.
Setup = nil
MojoSetup.collectgarbage()  -- and collect it, etc.

-- Now do the real work...
purpose()

-- end of mojosetup_mainline.lua ...

