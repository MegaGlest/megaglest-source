project(
  'MegaGlest',
  ['c', 'cpp'],
  version: '3.13.0.999',
  meson_version: '>= 0.55.0',
  default_options: [
    'warning_level=1',
    'buildtype=debugoptimized',
    'cpp_std=c++11',
    'b_ndebug=true',
  ],
)
# -Db_ndebug=true | Disable assertions.
# Normally meson will add _GLIBCXX_ASSERTIONS to the build flags. When that's
# added, multiplayer network games will crash after starting.

# Setup compiler.
cxx = meson.get_compiler('cpp')
cc = meson.get_compiler('c')

# OS name reference chart
#https://mesonbuild.com/Reference-tables.html#operating-system-names
#
# s_posix = host_machine.system() not in ['windows', 'emscripten', 'android']
host_system = host_machine.system()

# Compiler id reference chart
# https://mesonbuild.com/Reference-tables.html#compiler-ids
#
#if cxx.get_id() == 'msvc'

extra_flags = [
  '-fno-common',
  '-Werror=odr',
  '-Werror=lto-type-mismatch',
  '-fno-strict-aliasing',
  # '-Werror=strict-aliasing',
  '-DHAVE_SYS_IOCTL_H',
]

# Defined in cmake but I didn't find references in the source code
# '-DX11_AVAILABLE'
# -DHAVE_FONTCONFIG'

add_project_arguments(cc.get_supported_arguments(extra_flags), language: 'c')
add_project_arguments(cxx.get_supported_arguments(extra_flags), language: 'cpp')

git = find_program('git')
git_version = 'unversioned'
if git.found()
  git_version = run_command(
    [git, 'rev-parse', '--short', 'HEAD'],
    capture: true,
    check: true,
  )
endif
gitver = configuration_data()
gitver.set_quoted('GITVERSION', git_version.stdout().strip())
gitversion_h = configure_file(output: 'gitversion.h', configuration: gitver)

# Macros will be written to config.h later when configure_file() runs
conf = configuration_data()
conf.set('MESON_BUILD', 1)

subdir('source')

# The config.h file finally is output here, all the data assigned by
# conf.set() will be written to it.
config_h = configure_file(output: 'config_whole.h', configuration: conf)
