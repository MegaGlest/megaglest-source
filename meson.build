project(
  'MegaGlest',
  ['c', 'cpp'],
  version: '3.13.0.999',
  meson_version : '>= 0.55.0',
  default_options: ['warning_level=1', 'buildtype=debugoptimized']
  )

# Setup compiler.
cxx = meson.get_compiler('cpp')
cc = meson.get_compiler('c')

add_global_arguments(['-DUSE_FTGL'], language: 'cpp')

inc_common = [
  'platform',
  'platform/posix',
  'platform/common',
  'platform/win32',
#  ' platform/miniupnpc',
  'platform/sdl',
  'util',
  'util/utf8',
  'map',
  'compression',
  'feathery_ftp',
#  ' libircclient',
#  ' libircclient/include',
#  ' streflop',
#  ' streflop/softfloat',
#  ' streflop/libm',
#  ' streflop/libm/headers',
#  ' streflop/libm/flt-32',
  'lua',
  'xml',
  'xml/rapidxml',
  'sound',
  'sound/openal',
  'graphics',
  'graphics/gl',
  ]

inc_shared_lib = []
foreach dir : inc_common
  inc_shared_lib += [join_paths('../../source/shared_lib/include', dir)]
endforeach

inc_game = [
    'types',
    'ai',
    'menu',
    'steam',
    'facilities',
    'main',
    'steamshim',
    'network',
    'world',
    'global',
    'type_instances',
    'gui',
    'sound',
    'graphics',
    'game',
]

game_includes = []
foreach dir : inc_game
  game_includes += [join_paths('../../source/glest_game', dir)]
endforeach

subdir('source/shared_lib')
subdir('source/glest_game')
subdir('source/glest_map_editor')
subdir('source/g3d_viewer')
subdir('source/tools/glexemel')
