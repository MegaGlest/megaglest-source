project(
  'MegaGlest',
  ['c', 'cpp'],
  version: '3.13.0.999',
  meson_version : '>= 0.53.2',
  default_options: ['warning_level=1', 'buildtype=release']
  )

# Setup compiler.
cxx = meson.get_compiler('cpp')
cc = meson.get_compiler('c')

add_global_arguments(['-DUSE_FTGL'], language: 'cpp')

subdir('source/shared_lib')
subdir('source/glest_game')
subdir('source/glest_map_editor')
subdir('source/g3d_viewer')
subdir('source/tools/glexemel')
