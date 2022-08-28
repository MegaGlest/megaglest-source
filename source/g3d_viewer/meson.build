src_g3d_viewer = [
    'main.cpp',
    'renderer.cpp',
    '../glest_game/global/config.cpp',
    '../glest_game/graphics/unit_particle_type.cpp',
    '../glest_game/graphics/particle_type.cpp',

]

inc_common = [
    '../shared_lib/include/graphics',
    '../shared_lib/include/platform/sdl',
    '../shared_lib/include/platform/common',
    '../shared_lib/include/util',
    '../shared_lib/include/xml',
    '../shared_lib/include/map',
    '../shared_lib/include/graphics/gl',
    '../shared_lib/include/sound',
    '../glest_game/global',
    '../glest_game/game',
    '../glest_game/graphics',
    '../glest_game/facilities',
    '../glest_game/sound',
]

dep_xercesc = dependency('XercesC')

executable('megaglest_g3d_viewer',
  src_g3d_viewer,
  include_directories: inc_common,
  dependencies: [
    dep_libmegaglest,
    dep_wx,
    dep_sdl,
    dep_gl,
    dep_x11,
    dep_jpeg,
    dep_png,
    dep_glew,
    dep_xercesc
  ],
#   cpp_args: [
#     '-DNDEBUG',
#     ]
)
