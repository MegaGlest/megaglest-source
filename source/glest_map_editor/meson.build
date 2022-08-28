src_map_editor = [
  'program.cpp',
  'main.cpp',
  '../glest_game/global/config.cpp'
]

inc_common = [
  '../shared_lib/include/map',
  '../shared_lib/include/util',
  '../shared_lib/include/graphics',
  '../shared_lib/include/graphics/gl',
  '../shared_lib/include/platform/sdl',
  '../shared_lib/include/platform/common',
  '../glest_game/global',
  '../glest_game/game',
  '../glest_game/facilities',
]

dep_wx = dependency('wxwidgets', version : '>=3.0.0', modules : ['gl', 'core', 'base'])
dep_glu = dependency('glu')

executable('megaglest_map_editor',
  src_map_editor,
  include_directories: inc_common,
  dependencies: [
    dep_libmegaglest,
    dep_wx,
    dep_sdl,
    dep_gl,
    dep_glu,
  ],
  cpp_args: [
    '-DNDEBUG',
    ])
