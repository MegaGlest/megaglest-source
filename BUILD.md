# Building MegaGlest

## Getting the code

Get the source code plus the data (which is a git submodules), plus other
submodules

    git clone --recurse-submodules https://github.com/MegaGlest/megaglest-source

## Dependencies

On Linux and MacOS, dependencies can be downloaded using the 'setupBuildDeps.sh'
script (see the mk/linux and mk/macos directories).

On Windows, refer to this workflow:
https://github.com/MegaGlest/megaglest-source/blob/develop/.github/workflows/cmake.yml

## Compiling

On Linux or MacOS, go to mk/linux or mk/macos and run the build script:

    ./build-mg.sh (add `-h` to see options)

When completed, the game, map editor, and viewer binaries will be output the
current directory. You can run them from that location. When built using the
script as shown above, the binaries will read the ini files in that directory
for information (such as the path to the language files and game data) that is
required to run the game.

## Building with CMake

It is highly recommended to only use the script above. The CMake build system
we use is primarily for packagers. If you build with CMake, the game will
error when trying to find the ini files (which contain the paths to the data)
unless you install the binaries into system locations.

When building with [CMake](https://cmake.org/cmake/help/latest/), you can use
standard cmake syntax. It's recommended to do an out-of-tree build:

    cmake -B builddir
    cd builddir
    make -jn (where n is the number of processors you wish to use)

To see a list of various configuration options, from your builddir, run

    cd builddir
    cmake .. -LH

> ⚠️ **Warning:** CMake [does not natively generate an 'uninstall' target](https://stackoverflow.com/questions/41471620/cmake-support-make-uninstall)

and we have not manually implemented one. This means that `make uninstall`
will not work; you will only be able to uninstall everything (the binaries and
data)by manually removing the files. This is an important reason why we
recommend using the build script, Windows, or other distribution packages to
run the game.
