build_game = get_option('build_game')
build_editor = get_option('build_editor')
build_viewer = get_option('build_viewer')
#required by all
sdl_dep = dependency('sdl2')

# required to build both the game and the shared library, but not
# the editor or viewer
shared_lib_game_deps = []
shared_lib_game_common = ['libcurl', 'ftgl', 'lua', 'openal', 'fribidi']

foreach dep : shared_lib_game_common
  shared_lib_game_deps += dependency(dep)
endforeach

conf.set('USE_FTGL', 1)
# will output '#define USE_FTGL 1' to config.h
conf.set('HAVE_FRIBIDI', 1)

# This will get changed to 5.3
lua_dep = dependency('lua', required: false)
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

# streflop
#
# By default, meson will look in the repo root's 'subproject' directory
#
# Note changing a default options might not be enough to trigger meson to rerun
# setup. In the build directory, you can change the option manually
# Example: meson configure -Dstreflop:fpu=soft (where the value preceding the
# colon is the subproject name) and then rebuild.
streflop_subproj = subproject(
  'streflop-ng',
  default_options: ['default_library=static', 'fpu=sse', 'denormals=false'],
)
streflop_dep = streflop_subproj.get_variable('streflop_dep')
conf.set('USE_STREFLOP', 1)

# required by map editor and viewer
if build_editor or build_viewer
  wx_dep = dependency(
    'wxwidgets',
    version: '>=3.0.0',
    modules: ['gl', 'core', 'base'],
  )
endif

subdir('shared_lib')
if build_game
  subdir('glest_game')
endif
if build_editor
  subdir('glest_map_editor')
endif
if build_viewer
  subdir('g3d_viewer')
endif
subdir('tools/glexemel')
