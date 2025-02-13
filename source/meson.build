# required to build both the game and the shared library, but not
# the editor or viewer
shared_lib_game_deps = []

shared_lib_game_common = [
  'libcurl',
  'ftgl',
  'lua',
  'openal',
]

foreach dep : shared_lib_game_common
    shared_lib_game_deps += dependency(dep)
endforeach

# This will get changed to 5.3
lua_dep = dependency('lua-5.3', required: true)
if not lua_dep.found()
    lua_dep = cc.find_library('lua', has_headers: 'lua.h', required: true)
    #  if not lua_dep.found()
    #    # This will use the fallback
    #    lua_dep = dependency(
    #      'lua-5.3',
    #      required: true,
    #      fallback: 'lua',
    #      default_options: ['default_library=static']
    #      )
    #  endif
endif

shared_lib_game_deps += lua_dep

# required by all
sdl_dep = dependency('sdl2')

# required by map editor and viewer
wx_dep = dependency(
    'wxwidgets',
    version: '>=3.0.0',
    modules: ['gl', 'core', 'base'],
)

# streflop
#
# By default, meson will look in the repo roots 'subproject' directory # Note
# changing a default options might not be enough to trigger meson to rerun
# setup. In the build directory, you can change the option manually
# Example: meson configure -Dstreflop:fpu=soft (where the value preceding the
# colon is the subproject name) and then rebuild.
streflop_subproj = subproject(
    'streflop',
    default_options: ['default_library=static', 'fpu=sse', 'denormals=false'],
)
# TODO: The binaries don't need the dep object, but they do need the includes.
# Get them from the dep object instead of hard-coding them in the three
# binary meson builds.
streflop_dep = streflop_subproj.get_variable('streflop_dep')

# Only the include paths, not the whole dep object is required to
# build the game
streflop_includes = streflop_dep[0].partial_dependency(includes: true)

subdir('shared_lib')
subdir('glest_game')
subdir('glest_map_editor')
subdir('g3d_viewer')
subdir('tools/glexemel')
