src_editor = ['program.cpp', 'main.cpp', '../glest_game/global/config.cpp']

# Some includes from the game are required for the editor.
# Other required includes are imported from the dep_libmegaglest object
inc_editor = [
  '../glest_game/global',
  '../glest_game/game',
  '../glest_game/facilities',
]

dep_glu = dependency('glu')

executable(
  'megaglest_editor',
  src_editor,
  include_directories: inc_editor,
  dependencies: [dep_libmegaglest, wx_dep, gl_dep, dep_glu],
)
