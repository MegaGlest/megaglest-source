"Installing MegaGlest deps."
git pull
.\bootstrap.bat
.\vcpkg.exe update
.\vcpkg.exe install --disable-metrics `
    brotli:x64-windows-static `
    bzip2:x64-windows-static `
    curl:x64-windows-static `
    expat:x64-windows-static `
    freetype:x64-windows-static `
    fribidi:x64-windows-static `
    ftgl:x64-windows-static `
    glew:x64-windows-static `
    libiconv:x64-windows-static `
    libjpeg-turbo:x64-windows-static `
    liblzma:x64-windows-static `
    libogg:x64-windows-static `
    libpng:x64-windows-static `
    libvorbis:x64-windows-static `
    libxml2:x64-windows-static `
    lua:x64-windows-static `
    miniupnpc:x64-windows-static `
    openal-soft:x64-windows-static `
    opengl:x64-windows-static `
    sdl2:x64-windows-static `
    sqlite3:x64-windows-static `
    tiff:x64-windows-static `
    wxwidgets:x64-windows-static `
    zlib:x64-windows-static
