project(
  'MegaGlest',
  'cpp',
  version: '3.13.0.999',
  meson_version : '>= 0.56.0',
  default_options: ['warning_level=0']
  )

# Setup compiler.
compiler = meson.get_compiler('cpp')

add_global_arguments(
  [
    # '-DUSE_STREFLOP',
    '-DSTREFLOP_SOFT',
    # '-DSTREFLOP_RANDOM_GEN_SIZE=32',
    '-DLIBM_COMPILING_FLT32',
    '-DN_SPECIALIZED=32',
    '-frounding-math',
    '-fsignaling-nans',
    # '-Wno-switch',
    # '-Wuninitialized',
    # '-Wsign-compare',
    # '-Wunused-function',
    # '-Wunused-variable',
    # '-Wreturn-type',
    '-fno-strict-aliasing',
    '-DHAVE_SYS_IOCTL_H',
    '-DUNICODE',
    # '-O3',
    # '-w',
  ],
  language: 'cpp')

subdir('source')
