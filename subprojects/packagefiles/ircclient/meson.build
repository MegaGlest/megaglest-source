project(
  'ircclient',
  'c',
  version: '1.10.05',
  meson_version : '>= 0.48.0',
  default_options: [
    'warning_level=2',
    ]
  )

cc = meson.get_compiler('c')
extra_flags = [
  '-fno-common',
  '-Wshadow',
  '-Wstrict-overflow=5',
  '-Wformat-security',
  '-Wformat-overflow=2',
  '-Wunused-result',
  '-Werror=odr',
  '-Werror=lto-type-mismatch',
  '-Werror=strict-aliasing',
]

add_project_arguments(cc.get_supported_arguments(extra_flags), language: 'c')

inc = include_directories('include')

src = ('src/libircclient.c')
buildtarget = library(
  meson.project_name(),
  src,
  include_directories : inc,
  version : meson.project_version(),
  install: not meson.is_subproject()
  )

# How to use in a superproject and other info
# https://mesonbuild.com/Subprojects.html
ircclient_dep = declare_dependency(
  include_directories : inc,
  link_with : buildtarget
  )

if not meson.is_subproject()
  pkg = import('pkgconfig')
  pkg.generate(
    buildtarget,
    description : 'Simple irc client')
endif
