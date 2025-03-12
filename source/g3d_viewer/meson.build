src_viewer = [
  'main.cpp',
  'renderer.cpp',
  '../glest_game/global/config.cpp',
  '../glest_game/graphics/unit_particle_type.cpp',
  '../glest_game/graphics/particle_type.cpp',
]

# Some includes from the game are required for the viewer.
# Other required includes are imported from the dep_libmegaglest object
inc_viewer = [
  '../glest_game/global',
  '../glest_game/game',
  '../glest_game/graphics',
  '../glest_game/facilities',
  '../glest_game/sound',
]

executable(
  'megaglest_g3d_viewer',
  src_viewer,
  include_directories: inc_viewer,
  dependencies: [
    dep_libmegaglest,
    wx_dep,
    gl_dep,
    x11_dep,
    jpeg_dep,
    png_dep,
    glew_dep,
  ],
)
